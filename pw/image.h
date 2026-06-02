#ifndef _IMAGE_H
#define _IMAGE_H

#include "Xfred.h"
#include "quant.h"
#include "vicar.h"
#include <X11/Xlib.h>
#include <X11/Xos.h>
#include <X11/Xutil.h>

#include "io.h"

#define min(a, b) (a < b ? a : b)
#define max(a, b) (a > b ? a : b)

#define NIMAGE 26

/* Image file types */
#define VICAR_BIL 1
#define VICAR_BIP 2
#define VICAR_BSQ 3
#define FORMAT_GRD 4

#define BYTE 1 /* 8-bit int grayscale */
#define HALF 2 /* 16-bit int grayscale */
#define FULL 3 /* 32-bit int grayscale */
#define REAL 4 /* 32-bit float grayscale */
#define C8BIT 5 /* 8-bit color indexed */
#define C24BIT 6 /* 24bit direct color */

/* similar definitions, from io.h */
#define SHORT 2
#define INT 3
#define FLOAT 4
#define VAX_FLOAT 5
#define VAX_INTEGER 6
#define DOUBLE 8

struct _subset {
    int line;
    int sample;
    int width;
    int height;
    int lskip;
    int sskip;
    int uwidth;
    int uheight;
};

struct _DelRange {
    float Start;
    float End;
};

typedef struct _DelRange DelRange;

struct _image {
    void *data; /* unmodifed data values */
    char *sdata; /* stretched data values (LUT slot indices) */
    char *rgb_data; /* rendered display buffer at visual depth */

    char *image_hold; /* to hold old image data  (used for overlay) */

    int *histogram;
    int max_hist;

    char *hist_data;
    XImage *hist_image;
    int hist_type; /* Type of histogram loaded */
    int hist_scale; /* Type of histogram loaded */

    char *pan_data;
    XImage *pan_image;

    char *filename; /* filename of data */

    struct vicar_header header;

    struct _iheader iheader;
    struct _subset subset;

    int byte_order;
    int band; /* extracted band */

    double c_low, c_high; /* cut low, High */
    double s_low, s_high; /* stretch low, high */
    double d_low, d_high; /* actual data low, high */

    int ncolors; /* number of gray scale colors used with this image */

    Pixmap pixmap;
    XImage *ximage;

    XColor *Colors;

    int color_offsets[3];

    XColor C1;
    XColor C2;

    struct Point *MapPhoto;
    int *map;

    char composite;
    char rgb[NIMAGE];
    char overlays[NIMAGE];

    struct quant_data *quant;

    char MemMode;
    int scale; /* scaling factor for this image */

    int force_reload;
    int is_stretched; /* this image autoscaled? (it affects the name shown) */

    DelRange *Dranges; /*Pointer to list of deleted point values */
    int NumRanges;
};

typedef struct _image *Image;

extern Image Images[];

struct vicar_header *get_image_header(Image new);
void *get_image_data(Image new);

typedef struct {
    int width;
    int height;
    int min;
    int max;
    float *data;
} FIMAGE;

#define PACK_MAGIC "\037\036" /* Magic header for packed files */
#define GZIP_MAGIC "\037\213" /* Magic header for gzip files, 1F 8B */
#define OLD_GZIP_MAGIC "\037\236" /* Magic header for gzip 0.5 = freeze 1.x */
#define LZH_MAGIC "\037\240" /* Magic header for SCO LZH Compress files */
#define LZW_MAGIC "\037\235" /* Magic header for lzw files, 1F 9D */

double get_data(Image, int);

#endif
