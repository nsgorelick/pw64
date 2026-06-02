#include "test.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <X11/Xlib.h>

#include "../hershey.h"

/* misc.c */
extern int ToTiff(unsigned char *s1, unsigned char *s2, int len);
extern int UnTiff(unsigned char *s1, unsigned char *s2, int len);
extern int is_dir(char *path);
extern int is_file(char *path);
extern int get_int(FILE *fp);
extern float get_float(FILE *fp);
extern int getbit(FILE *file);

/* xgets.c */
extern void insert_str(char *s, char *t, int at);

/* hershey.c */
extern int XfHersheyWidth(char c, int cset);

/* chd.c */
extern int get_sorted_dir(char *path, char ***ds);

static const char *fixture_path(const char *name)
{
    static char path[512];
    const char *base = getenv("PW64_TEST_FIXTURES");
    if (base == NULL || base[0] == '\0')
        base = "tests/fixtures";
    snprintf(path, sizeof(path), "%s/%s", base, name);
    return path;
}

static void packbits_roundtrip(const unsigned char *raw, int len)
{
    unsigned char packed[512];
    unsigned char out[512];
    int plen, ulen;

    plen = ToTiff((unsigned char *)raw, packed, len);
    ASSERT(plen >= 0);
    ulen = UnTiff(packed, out, plen);
    ASSERT_EQ_INT(ulen, len);
    ASSERT_MEM_EQ(raw, out, (size_t)len);
}

TEST(test_packbits_roundtrip)
{
    unsigned char raw[16];
    int i;

    for (i = 0; i < 16; i++)
        raw[i] = (unsigned char)(i & 3);
    packbits_roundtrip(raw, 16);
}

TEST(test_packbits_run)
{
    unsigned char raw[] = {7, 7, 7, 7, 1, 2, 3};
    packbits_roundtrip(raw, (int)sizeof(raw));
}

TEST(test_packbits_single_byte)
{
    unsigned char raw[] = {0xab};
    packbits_roundtrip(raw, 1);
}

TEST(test_packbits_zero_length_input)
{
    unsigned char packed[8];
    unsigned char out[8];
    int plen, ulen;

    /*
     * ToTiff(len==0) still emits a 2-byte literal (legacy quirk: loop sets
     * newbase=1). Document behavior; UnTiff with empty pack stream is empty.
     */
    plen = ToTiff((unsigned char *)"", packed, 0);
    ASSERT_EQ_INT(plen, 2);
    ulen = UnTiff(packed, out, 0);
    ASSERT_EQ_INT(ulen, 0);
}

TEST(test_packbits_long_run)
{
    unsigned char raw[130];
    int i;

    for (i = 0; i < 130; i++)
        raw[i] = 0x42;
    packbits_roundtrip(raw, 130);
}

TEST(test_is_dir)
{
    /* is_dir() does not check stat() failure — avoid missing paths. */
    ASSERT(is_dir("/tmp") == 1);
    ASSERT(is_dir("/etc/hosts") == 0);
}

TEST(test_is_file)
{
    char path[512];

    ASSERT(is_file("/tmp") == 0);
    ASSERT(is_file("/etc/hosts") == 1);
    snprintf(path, sizeof(path), "%s", fixture_path("chd/sorted/alpha"));
    ASSERT(is_file(path) == 1);
}

TEST(test_insert_str_middle)
{
    char buf[64] = "hello world";
    char ins[64] = "XX";

    insert_str(buf, ins, 5);
    ASSERT(strcmp(buf, "helloXX world") == 0);
}

TEST(test_insert_str_at_start)
{
    char buf[64] = "world";
    char ins[64] = "hello ";

    insert_str(buf, ins, 0);
    ASSERT(strcmp(buf, "hello world") == 0);
}

TEST(test_insert_str_at_end)
{
    char buf[64] = "hello";
    char ins[64] = " world";

    insert_str(buf, ins, 5);
    ASSERT(strcmp(buf, "hello world") == 0);
}

TEST(test_get_int)
{
    FILE *fp = tmpfile();
    int v;

    ASSERT(fp != NULL);
    fputs("  # leading comment\n42 trailing\n", fp);
    rewind(fp);
    v = get_int(fp);
    ASSERT_EQ_INT(v, 42);
    fclose(fp);
}

TEST(test_get_int_invalid)
{
    FILE *fp = tmpfile();
    int v;

    ASSERT(fp != NULL);
    fputs("no digits here\n", fp);
    rewind(fp);
    v = get_int(fp);
    ASSERT_EQ_INT(v, -1);
    fclose(fp);
}

TEST(test_get_float)
{
    FILE *fp = tmpfile();
    float v;

    ASSERT(fp != NULL);
    fputs("  3.5\n", fp);
    rewind(fp);
    v = get_float(fp);
    ASSERT_NEAR(v, 3.5, 1e-5);
    fclose(fp);
}

TEST(test_getbit)
{
    FILE *fp = tmpfile();
    int b;

    ASSERT(fp != NULL);
    fputs("1 0 1\n", fp);
    rewind(fp);
    b = getbit(fp);
    ASSERT_EQ_INT(b, 1);
    b = getbit(fp);
    ASSERT_EQ_INT(b, 0);
    b = getbit(fp);
    ASSERT_EQ_INT(b, 1);
    b = getbit(fp);
    ASSERT_EQ_INT(b, -1);
    fclose(fp);
}

TEST(test_hershey_width)
{
    int w_m, w_i, w_space;

    w_m = XfHersheyWidth('M', ROMAN_SIMPLEX);
    w_i = XfHersheyWidth('i', ROMAN_SIMPLEX);
    w_space = XfHersheyWidth(' ', ROMAN_SIMPLEX);
    ASSERT(w_m > 0);
    ASSERT(w_i > 0);
    ASSERT(w_space >= 0);
    ASSERT(w_m > w_i);
}

TEST(test_get_sorted_dir)
{
    char **dirs = NULL;
    char path[512];
    int n, i;

    snprintf(path, sizeof(path), "%s", fixture_path("chd/sorted"));
    n = get_sorted_dir(path, &dirs);
    ASSERT(n >= 3);
    ASSERT(dirs != NULL);
    ASSERT(strcmp(dirs[0], "alpha") == 0);
    ASSERT(strcmp(dirs[1], "beta") == 0);
    ASSERT(strcmp(dirs[2], "gamma") == 0);
    for (i = 0; i < n; i++) {
        ASSERT(strcmp(dirs[i], ".") != 0);
        ASSERT(strcmp(dirs[i], "..") != 0);
    }
    for (i = 0; i < n; i++)
        free(dirs[i]);
    free(dirs);
}

void register_xfred_tests(void)
{
    RUN_TEST(test_packbits_roundtrip);
    RUN_TEST(test_packbits_run);
    RUN_TEST(test_packbits_single_byte);
    RUN_TEST(test_packbits_zero_length_input);
    RUN_TEST(test_packbits_long_run);
    RUN_TEST(test_is_dir);
    RUN_TEST(test_is_file);
    RUN_TEST(test_insert_str_middle);
    RUN_TEST(test_insert_str_at_start);
    RUN_TEST(test_insert_str_at_end);
    RUN_TEST(test_get_int);
    RUN_TEST(test_get_int_invalid);
    RUN_TEST(test_get_float);
    RUN_TEST(test_getbit);
    RUN_TEST(test_hershey_width);
    RUN_TEST(test_get_sorted_dir);
}
