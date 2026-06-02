#include "test.h"

#include <string.h>
#include <errno.h>
#include <math.h>
#include <fcntl.h>
#include <unistd.h>

#include "../iomedley/iomedley.h"

static const char *fixture_path(const char *name)
{
    static char path[512];
    const char *base = getenv("PW64_TEST_FIXTURES");
    if (base == NULL || base[0] == '\0')
        base = "tests/fixtures";
    snprintf(path, sizeof(path), "%s/%s", base, name);
    return path;
}

static FILE *open_fixture(const char *name)
{
    FILE *fp = fopen(fixture_path(name), "rb");
    if (fp == NULL)
        fprintf(stderr, "cannot open fixture %s: %s\n", name, strerror(errno));
    return fp;
}

TEST(test_dimension_macros)
{
    int bsq[3] = { 100, 200, 5 };
    int bil[3] = { 100, 5, 200 };
    int bip[3] = { 5, 100, 200 };

    ASSERT_EQ_INT(iom_GetSamples(bsq, iom_BSQ), 100);
    ASSERT_EQ_INT(iom_GetLines(bsq, iom_BSQ), 200);
    ASSERT_EQ_INT(iom_GetBands(bsq, iom_BSQ), 5);

    ASSERT_EQ_INT(iom_GetSamples(bil, iom_BIL), 100);
    ASSERT_EQ_INT(iom_GetBands(bil, iom_BIL), 5);
    ASSERT_EQ_INT(iom_GetLines(bil, iom_BIL), 200);

    ASSERT_EQ_INT(iom_GetBands(bip, iom_BIP), 5);
    ASSERT_EQ_INT(iom_GetSamples(bip, iom_BIP), 100);
    ASSERT_EQ_INT(iom_GetLines(bip, iom_BIP), 200);
}

TEST(test_iom_swp)
{
    char pair[2] = { 'a', 'b' };
    iom_swp(&pair[0], &pair[1]);
    ASSERT(pair[0] == 'b' && pair[1] == 'a');
}

TEST(test_byte_swap_short)
{
    short v = 0x1234;
#ifndef WORDS_BIGENDIAN
    iom_MSB2(&v);
    ASSERT_EQ_INT((unsigned short) v, 0x3412u);
    iom_MSB2(&v);
    ASSERT_EQ_INT((unsigned short) v, 0x1234u);
#else
    iom_MSB2(&v);
    ASSERT_EQ_INT((unsigned short) v, 0x1234u);
#endif
}

TEST(test_vax_ieee_roundtrip)
{
    float orig = 3.14159f;
    int ieee_bits;
    float vax_rep, back;

    memcpy(&ieee_bits, &orig, sizeof(ieee_bits));
    iom_ieee_vax_r(&ieee_bits, &vax_rep);
    iom_vax_ieee_r(&vax_rep, &back);
    ASSERT_NEAR(back, orig, 1e-4f);
}

TEST(test_long_byte_swap)
{
    unsigned char in[4] = { 0, 1, 2, 3 };
    unsigned char out[4];

    unsigned char expect[4] = { 1, 0, 3, 2 };

    iom_long_byte_swap(in, out);
    ASSERT_MEM_EQ(out, expect, 4);
}

TEST(test_pnm_detection_and_header)
{
    FILE *fp = open_fixture("gray4x3.pgm");
    struct iom_iheader h;

    ASSERT(fp != NULL);
    ASSERT(iom_isPNM(fp));
    ASSERT(iom_GetPNMHeader(fp, (char *) "gray4x3.pgm", &h));
    ASSERT_EQ_INT(iom_GetSamples(h.size, h.org), 4);
    ASSERT_EQ_INT(iom_GetLines(h.size, h.org), 3);
    ASSERT_EQ_INT(iom_GetBands(h.size, h.org), 1);
    ASSERT_EQ_INT(h.format, iom_BYTE);
    iom_cleanup_iheader(&h);
    fclose(fp);
}

TEST(test_load_header_embedded_data)
{
    FILE *fp = open_fixture("gray4x3.pgm");
    struct iom_iheader h;
    int i;

    ASSERT(fp != NULL);
    ASSERT(iom_LoadHeader(fp, (char *) "gray4x3.pgm", &h));
    /* PNM loaders attach decoded pixels in h->data */
    ASSERT(h.data != NULL);
    ASSERT_EQ_INT(iom_GetSamples(h.size, h.org), 4);
    ASSERT_EQ_INT(iom_GetLines(h.size, h.org), 3);
    for (i = 0; i < 12; i++) {
        int c = ((unsigned char *) h.data)[i];
        ASSERT(c >= 10 && c <= 120);
    }
    iom_cleanup_iheader(&h);
    fclose(fp);
}

TEST(test_rgb_ppm_header)
{
    FILE *fp = open_fixture("rgb2x2.ppm");
    struct iom_iheader h;

    ASSERT(fp != NULL);
    ASSERT(iom_isPNM(fp));
    ASSERT(iom_GetPNMHeader(fp, (char *) "rgb2x2.ppm", &h));
    ASSERT_EQ_INT(iom_GetBands(h.size, h.org), 3);
    ASSERT_EQ_INT(iom_GetSamples(h.size, h.org), 2);
    ASSERT_EQ_INT(iom_GetLines(h.size, h.org), 2);
    iom_cleanup_iheader(&h);
    fclose(fp);
}

TEST(test_convert_to_bip_bsq)
{
    unsigned char bsq[12] = {
        1, 2, 3, 4,             /* band 0 */
        5, 6, 7, 8,             /* band 1 */
        9, 10, 11, 12           /* band 2 */
    };
    struct iom_iheader h;
    unsigned char *bip = NULL;
    int x, y, z;

    iom_init_iheader(&h);
    h.size[0] = 2;
    h.size[1] = 2;
    h.size[2] = 3;
    h.org = iom_BSQ;
    h.format = iom_BYTE;
    h.eformat = iom_MSB_INT_1;

    ASSERT(iom__ConvertToBIP(bsq, &h, &bip));
    for (y = 0; y < 2; y++) {
        for (x = 0; x < 2; x++) {
            for (z = 0; z < 3; z++) {
                int bsq_idx = z * 4 + y * 2 + x;
                int bip_idx = y * 6 + x * 3 + z;
                ASSERT_EQ_INT(bip[bip_idx], bsq[bsq_idx]);
            }
        }
    }
    free(bip);
}

TEST(test_pnm_write_roundtrip)
{
    struct iom_iheader h;
    unsigned char pixels[4] = { 1, 2, 3, 4 };
    char outpath[] = "/tmp/pw64_test_XXXXXX.pgm";
    FILE *fp;
    struct iom_iheader h2;
    int fd;

    fd = mkstemps(outpath, 4);
    ASSERT(fd >= 0);
    close(fd);

    iom_init_iheader(&h);
    h.size[0] = 2;
    h.size[1] = 2;
    h.size[2] = 1;
    h.org = iom_BSQ;
    h.format = iom_BYTE;
    h.eformat = iom_MSB_INT_1;

    ASSERT(iom_WritePNM(outpath, pixels, &h, 1));

    fp = fopen(outpath, "rb");
    ASSERT(fp != NULL);
    ASSERT(iom_LoadHeader(fp, outpath, &h2));
    ASSERT(h2.data != NULL);
    ASSERT_MEM_EQ(h2.data, pixels, 4);

    iom_cleanup_iheader(&h2);
    fclose(fp);
    unlink(outpath);
}

void register_iomedley_tests(void)
{
    RUN_TEST(test_dimension_macros);
    RUN_TEST(test_iom_swp);
    RUN_TEST(test_byte_swap_short);
    RUN_TEST(test_vax_ieee_roundtrip);
    RUN_TEST(test_long_byte_swap);
    RUN_TEST(test_pnm_detection_and_header);
    RUN_TEST(test_load_header_embedded_data);
    RUN_TEST(test_rgb_ppm_header);
    RUN_TEST(test_convert_to_bip_bsq);
    RUN_TEST(test_pnm_write_roundtrip);
}
