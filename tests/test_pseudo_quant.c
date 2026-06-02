#include "test.h"

#include <math.h>
#include <stdio.h>

#include "../pw/pseudo.h"
#include "../pw/quant.h"

void quantize(char *data, int ncolors, int height, int width, struct quant_data **quant, int base);

int pseudocolor(unsigned char *red, unsigned char *green, unsigned char *blue,
                unsigned char *out, int n_red, int n_green, int n_blue,
                int n_bits, int n_out, unsigned char *red_lut,
                unsigned char *green_lut, unsigned char *blue_lut, unsigned char *out_map, char base, int size);

static FILE *silence_stdout(void)
{
    return freopen("/dev/null", "w", stdout);
}

TEST(test_pseudocolor_small_rgb_planes)
{
    /* 2x2 image, 2 levels per channel -> small color cube */
    unsigned char r[4] = { 0, 1, 0, 1 };
    unsigned char g[4] = { 0, 0, 1, 1 };
    unsigned char b[4] = { 0, 1, 1, 0 };
    unsigned char out[4];
    unsigned char rl[2] = { 0, 1 };
    unsigned char gl[2] = { 0, 1 };
    unsigned char bl[2] = { 0, 1 };
    unsigned char out_map[8];
    int n_out;
    FILE *saved = silence_stdout();

    for (int i = 0; i < 8; i++)
        out_map[i] = (unsigned char) (10 + i);

    n_out = pseudocolor(r, g, b, out, 2, 2, 2, 1, 8, rl, gl, bl, out_map, (char) 10, 4);
    if (saved != NULL)
        freopen("/dev/tty", "w", stdout);

    ASSERT(n_out > 0 && n_out <= 8);
    for (int i = 0; i < 4; i++) {
        int slot = out[i];
        ASSERT(slot >= 10 && slot < 10 + n_out);
    }
}

TEST(test_quantize_buckets)
{
    char data[6] = { 10, 10, 11, 11, 12, 12 };
    struct quant_data *q = NULL;

    quantize(data, 3, 2, 3, &q, 10);
    ASSERT(q != NULL);
    ASSERT_EQ_INT(q[0].count, 2);
    ASSERT_EQ_INT(q[1].count, 2);
    ASSERT_EQ_INT(q[2].count, 2);
    ASSERT_EQ_INT(q[0].pixels[0], 0);
    ASSERT_EQ_INT(q[0].pixels[1], 1);
    ASSERT_EQ_INT(q[2].pixels[1], 5);
    free(q);
}

TEST(test_quantize_slot_base_two)
{
    char data[4] = { 2, 2, 3, 3 };
    struct quant_data *q = NULL;

    quantize(data, 2, 2, 2, &q, 2);
    ASSERT(q != NULL);
    ASSERT_EQ_INT(q[0].count, 2);
    ASSERT_EQ_INT(q[1].count, 2);
    free(q);
}

void register_pseudo_quant_tests(void)
{
    RUN_TEST(test_pseudocolor_small_rgb_planes);
    RUN_TEST(test_quantize_buckets);
    RUN_TEST(test_quantize_slot_base_two);
}
