#include "display_rgb.h"
#include <stdlib.h>
#include <string.h>

extern Display *display;
extern int screen;
extern XColor Colors[];
extern int NColors;

static Visual *g_visual = NULL;
static int g_depth = 0;
static int g_bytes_per_pixel = 0;
static unsigned long g_red_mask = 0;
static unsigned long g_green_mask = 0;
static unsigned long g_blue_mask = 0;
static int g_red_shift = 0, g_green_shift = 0, g_blue_shift = 0;
static int g_red_bits = 0, g_green_bits = 0, g_blue_bits = 0;

static int mask_shift(unsigned long mask)
{
    int shift = 0;
    if (mask == 0)
        return 0;
    while ((mask & 1UL) == 0) {
        shift++;
        mask >>= 1;
    }
    return shift;
}

static int mask_bits(unsigned long mask)
{
    int bits = 0;
    while (mask) {
        bits += (int)(mask & 1UL);
        mask >>= 1;
    }
    return bits;
}

static unsigned long scale_to_bits(unsigned short in, int bits)
{
    unsigned long maxv;
    if (bits <= 0)
        return 0;
    maxv = (1UL << bits) - 1UL;
    return (unsigned long)((((unsigned long)in) * maxv + 32767UL) / 65535UL);
}

static int bytes_per_pixel_for_depth(Display *d, int dep)
{
    int count, i;
    XPixmapFormatValues *fmts = XListPixmapFormats(d, &count);
    int bpp = dep;
    if (fmts != NULL) {
        for (i = 0; i < count; i++) {
            if (fmts[i].depth == dep) {
                bpp = fmts[i].bits_per_pixel;
                break;
            }
        }
        XFree((char *)fmts);
    }
    return (bpp + 7) / 8;
}

static int resolve_pixmap_depth(Display *d, int s)
{
    int want = DefaultDepth(d, s);
    int count, i;
    XPixmapFormatValues *fmts = XListPixmapFormats(d, &count);

    if (fmts == NULL)
        return want;
    for (i = 0; i < count; i++) {
        if (fmts[i].depth == want) {
            XFree((char *)fmts);
            return want;
        }
    }
    /* Some servers report a DefaultDepth with no matching pixmap format. */
    for (i = 0; i < count; i++) {
        if (fmts[i].depth >= 15 && fmts[i].depth > want)
            want = fmts[i].depth;
    }
    XFree((char *)fmts);
    return want;
}

void pw_init_display(Display *d, int s)
{
    g_visual = DefaultVisual(d, s);
    g_depth = resolve_pixmap_depth(d, s);
    g_bytes_per_pixel = bytes_per_pixel_for_depth(d, g_depth);
    if (g_bytes_per_pixel < 3)
        g_bytes_per_pixel = 4;
    g_red_mask = g_visual->red_mask;
    g_green_mask = g_visual->green_mask;
    g_blue_mask = g_visual->blue_mask;
    g_red_shift = mask_shift(g_red_mask);
    g_green_shift = mask_shift(g_green_mask);
    g_blue_shift = mask_shift(g_blue_mask);
    g_red_bits = mask_bits(g_red_mask);
    g_green_bits = mask_bits(g_green_mask);
    g_blue_bits = mask_bits(g_blue_mask);
}

void pw_init_display_test(void)
{
    g_visual = NULL;
    g_depth = 24;
    g_bytes_per_pixel = 4;
    g_red_mask = 0x00FF0000UL;
    g_green_mask = 0x0000FF00UL;
    g_blue_mask = 0x000000FFUL;
    g_red_shift = 16;
    g_green_shift = 8;
    g_blue_shift = 0;
    g_red_bits = 8;
    g_green_bits = 8;
    g_blue_bits = 8;
}

int pw_display_depth(void)
{
    if (g_depth == 0 && display != NULL)
        pw_init_display(display, screen);
    return g_depth;
}

void pw_pack_xcolor(const XColor *c, unsigned char *dst)
{
    unsigned long pix = 0;
    if (g_depth == 0)
        pw_init_display_test();
    pix |= (scale_to_bits(c->red, g_red_bits) << g_red_shift) & g_red_mask;
    pix |= (scale_to_bits(c->green, g_green_bits) << g_green_shift) & g_green_mask;
    pix |= (scale_to_bits(c->blue, g_blue_bits) << g_blue_shift) & g_blue_mask;
    if (g_bytes_per_pixel >= 4) {
        dst[0] = (unsigned char)(pix & 0xFF);
        dst[1] = (unsigned char)((pix >> 8) & 0xFF);
        dst[2] = (unsigned char)((pix >> 16) & 0xFF);
        dst[3] = (unsigned char)((pix >> 24) & 0xFF);
    } else {
        dst[0] = (unsigned char)(pix & 0xFF);
        dst[1] = (unsigned char)((pix >> 8) & 0xFF);
        dst[2] = (unsigned char)((pix >> 16) & 0xFF);
    }
}

int pw_color_at(Image img, unsigned char slot, XColor *out)
{
    int idx = (int)slot;
    if (out == NULL)
        return 0;
    if (idx < 0 || idx > (NColors + 1))
        return 0;

    if (img != NULL && img->composite && idx >= 2 && idx < (2 + img->ncolors) && img->Colors != NULL) {
        *out = img->Colors[idx - 2];
        out->flags = DoRed | DoGreen | DoBlue;
        return 1;
    }

    *out = Colors[idx];
    out->flags = DoRed | DoGreen | DoBlue;
    return 1;
}

void pw_lut_index_to_rgb(Image img, const unsigned char *indices, unsigned char *rgb_data, int npixels)
{
    int i;
    if (g_depth == 0 && display != NULL)
        pw_init_display(display, screen);
    for (i = 0; i < npixels; i++) {
        XColor c;
        unsigned char *dst = rgb_data + i * g_bytes_per_pixel;
        if (!pw_color_at(img, indices[i], &c))
            c.red = c.green = c.blue = 0;
        pw_pack_xcolor(&c, dst);
    }
}

XImage *pw_create_rgb_image(Display *d, unsigned char *rgb_data, int w, int h)
{
    if (g_depth == 0)
        pw_init_display(d, DefaultScreen(d));
    return XCreateImage(d, g_visual, g_depth, ZPixmap, 0, (char *)rgb_data, w, h, 32, 0);
}

void pw_free_rgb_image(XImage *img)
{
    if (img == NULL)
        return;
    if (img->data != NULL)
        free(img->data);
    img->data = NULL;
    XFree((char *)img);
}

void pw_refresh_image(Image img)
{
    int npixels;
    unsigned char *rgb_data;

    if (img == NULL || img->sdata == NULL)
        return;

    npixels = img->subset.width * img->subset.height;
    if (npixels <= 0)
        return;

    if (img->ximage != NULL) {
        pw_free_rgb_image(img->ximage);
        img->ximage = NULL;
        img->rgb_data = NULL; /* freed via ximage->data */
    } else if (img->rgb_data != NULL) {
        free(img->rgb_data);
        img->rgb_data = NULL;
    }

    rgb_data = (unsigned char *)malloc(npixels * g_bytes_per_pixel);
    if (rgb_data == NULL)
        return;
    pw_lut_index_to_rgb(img, (unsigned char *)img->sdata, rgb_data, npixels);
    img->rgb_data = (char *)rgb_data;
    img->ximage = pw_create_rgb_image(display, rgb_data, img->subset.width, img->subset.height);
}
