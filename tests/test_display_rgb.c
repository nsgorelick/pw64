#include "test.h"

#include <X11/Xlib.h>
#include "../pw/display_rgb.h"
#include "../pw/image.h"

/* Globals referenced by display_rgb.c */
XColor Colors[256];
int NColors = 8;
Display *display = NULL;
int screen = 0;

static void setup_lut(void)
{
    int i;

    NColors = 8;
    Colors[0].red = Colors[0].green = Colors[0].blue = 0;
    Colors[1].red = Colors[1].green = Colors[1].blue = 65535;
    for (i = 0; i < NColors; i++) {
        unsigned short g = (unsigned short) ((i * 65535) / (NColors > 1 ? NColors - 1 : 1));
        Colors[i + 2].red = g;
        Colors[i + 2].green = g;
        Colors[i + 2].blue = g;
        Colors[i + 2].flags = DoRed | DoGreen | DoBlue;
    }
    Colors[0].flags = Colors[1].flags = DoRed | DoGreen | DoBlue;
    pw_init_display_test();
}

TEST(test_color_at_global_lut)
{
    XColor out;

    setup_lut();
    Colors[2].red = 65535;
    Colors[2].green = 0;
    Colors[2].blue = 0;

    ASSERT(pw_color_at(NULL, 2, &out));
    ASSERT_EQ_INT(out.red, 65535);
    ASSERT_EQ_INT(out.green, 0);
    ASSERT_EQ_INT(out.blue, 0);
}

TEST(test_color_at_composite_lut)
{
    struct _image img;
    XColor out;
    XColor comp_lut[2];

    setup_lut();
    memset(&img, 0, sizeof(img));
    img.composite = 1;
    img.ncolors = 2;
    comp_lut[0].red = 1000;
    comp_lut[0].green = 2000;
    comp_lut[0].blue = 3000;
    comp_lut[0].flags = DoRed | DoGreen | DoBlue;
    comp_lut[1] = comp_lut[0];
    img.Colors = comp_lut;

    Colors[2].red = 65535;
    Colors[2].green = Colors[2].blue = 0;

    ASSERT(pw_color_at(&img, 2, &out));
    ASSERT_EQ_INT(out.red, 1000);
    ASSERT_EQ_INT(out.green, 2000);
    ASSERT_EQ_INT(out.blue, 3000);
}

TEST(test_color_at_invalid_slot)
{
    XColor out;

    setup_lut();
    ASSERT(!pw_color_at(NULL, 255, &out));
    ASSERT(!pw_color_at(NULL, 2, NULL));
}

TEST(test_lut_index_to_rgb_red_green)
{
    unsigned char indices[2] = { 2, 3 };
    unsigned char rgb[8];
    unsigned char expect_red[4] = { 0x00, 0x00, 0xFF, 0x00 };
    unsigned char expect_green[4] = { 0x00, 0xFF, 0x00, 0x00 };

    setup_lut();
    Colors[2].red = 65535;
    Colors[2].green = Colors[2].blue = 0;
    Colors[3].red = Colors[3].blue = 0;
    Colors[3].green = 65535;

    pw_lut_index_to_rgb(NULL, indices, rgb, 2);
    ASSERT_MEM_EQ(rgb, expect_red, 4);
    ASSERT_MEM_EQ(rgb + 4, expect_green, 4);
}

void register_display_rgb_tests(void)
{
    RUN_TEST(test_color_at_global_lut);
    RUN_TEST(test_color_at_composite_lut);
    RUN_TEST(test_color_at_invalid_slot);
    RUN_TEST(test_lut_index_to_rgb_red_green);
}
