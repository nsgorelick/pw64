#include "test.h"

#include <X11/Xlib.h>
#include "../pw/color.h"

TEST(test_rgb_hsv_roundtrip)
{
    RGB in = { 40000, 20000, 10000 };
    HSV hsv = RGBToHSV(in);
    RGB out = HSVToRGB(hsv);

    ASSERT_NEAR(out.r / 65535.0, in.r / 65535.0, 0.02);
    ASSERT_NEAR(out.g / 65535.0, in.g / 65535.0, 0.02);
    ASSERT_NEAR(out.b / 65535.0, in.b / 65535.0, 0.02);
}

TEST(test_rgb_dist_zero)
{
    RGB a = { 1000, 2000, 3000 };
    ASSERT_NEAR(RGBDist(a, a), 0.0, 1e-6);
}

TEST(test_mix_rgb)
{
    RGB black = { 0, 0, 0 };
    RGB white = { 65535, 65535, 65535 };
    RGB mid = MixRGB(black, 0.5f, white, 0.5f);

    ASSERT(mid.r > 30000 && mid.r < 36000);
    ASSERT(mid.g > 30000 && mid.g < 36000);
    ASSERT(mid.b > 30000 && mid.b < 36000);
}

TEST(test_pct_to_rgb)
{
    RGB c = PctToRGB(1.0f, 0.0f, 0.0f);
    ASSERT_EQ_INT(c.r, 65535);
    ASSERT_EQ_INT(c.g, 0);
    ASSERT_EQ_INT(c.b, 0);
}

TEST(test_xcolor_to_rgb)
{
    XColor xc;
    RGB rgb;

    xc.red = 32768;
    xc.green = 16384;
    xc.blue = 8192;
    rgb = XColorToRGB(&xc);
    ASSERT_EQ_INT(rgb.r, 32768);
    ASSERT_EQ_INT(rgb.g, 16384);
    ASSERT_EQ_INT(rgb.b, 8192);
}

void register_color_tests(void)
{
    RUN_TEST(test_rgb_hsv_roundtrip);
    RUN_TEST(test_rgb_dist_zero);
    RUN_TEST(test_mix_rgb);
    RUN_TEST(test_pct_to_rgb);
    RUN_TEST(test_xcolor_to_rgb);
}
