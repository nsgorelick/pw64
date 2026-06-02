#ifndef _DISPLAY_RGB_H
#define _DISPLAY_RGB_H

#include "image.h"
#include <X11/Xlib.h>

/*
 * sdata slot encoding:
 *   0,1           : reserved black/white entries
 *   2..NColors+1  : absolute LUT slots for image/composite/overlay colors
 */

void pw_init_display(Display *d, int screen);
/* Headless tests: 32-bit 0x00RRGGBB masks, LSB-first byte order */
void pw_init_display_test(void);
int pw_display_depth(void);
void pw_pack_xcolor(const XColor *c, unsigned char *dst);

int pw_color_at(Image img, unsigned char slot, XColor *out);
void pw_lut_index_to_rgb(Image img, const unsigned char *indices, unsigned char *rgb_data, int npixels);

XImage *pw_create_rgb_image(Display *d, unsigned char *rgb_data, int w, int h);
/* Frees img and img->data (client-allocated RGB buffer passed to pw_create_rgb_image). */
void pw_free_rgb_image(XImage *img);

void pw_refresh_image(Image img);

#endif
