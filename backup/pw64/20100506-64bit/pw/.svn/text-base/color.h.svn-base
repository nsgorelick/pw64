/* $Header$ */

/* 
 * color.h - color definitions
 * 
 * Author:  Christopher A. Kent
 *      Western Research Laboratory
 *      Digital Equipment Corporation
 * Date:    Sun Dec 13 1987
 * Copyright (c) 1987 Christopher A. Kent
 */

/*
 * $Log$
 * Revision 1.1  1999/09/09 17:50:35  gorelick
 * Initial revision
 *
 * Revision 1.1  91/09/23  17:49:52  17:49:52  ngorelic (Noel S. Gorelick)
 * Initial revision
 * 
 * Revision 1.2  88/06/30  09:58:56  mikey
 * Handles CMY also.
 * 
 * Revision 1.1  88/06/30  09:10:53  mikey
 * Initial revision
 * 
 */

#define MAX_INTENSITY   65535               /* for X11 */

#ifndef MIN
#define MIN(a,b)    ((a) < (b) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a,b)    ((a) > (b) ? (a) : (b))
#endif

#define ABS(x)      ((x)<0?-(x):(x))

typedef struct _RGB {
    unsigned short r, g, b;
} RGB;

typedef struct _HSV {
    float   h, s, v;    /* [0, 1] */
} HSV;

typedef struct _CMY {
    unsigned short c, m, y;
} CMY;

typedef struct _KCMY {
    unsigned short k, c, m, y;
} KCMY;

extern RGB  RGBWhite, RGBBlack;

RGB MixRGB(RGB r, float alpha, RGB s, float beta);
RGB MixHSV(RGB r, float alpha, RGB s, float beta);
RGB HSVToRGB(HSV hsv);
HSV RGBToHSV(RGB rgb);
float   RGBDist(RGB r, RGB s);
RGB PctToRGB(float rr, float gg, float bb);
HSV PctToHSV(float hh, float ss, float vv);
RGB CMYToRGB(CMY cmy);
CMY RGBToCMY(RGB rgb);
RGB XColorToRGB(XColor *x);
