#ifndef _VICAR_H
#define _VICAR_H

struct vicar_header {
    int label_size;
    int format;
    int bits; /* size of one pixel in bits */
    int record_size;
    int org;
    int lines;
    int samples;
    int bands;
    int nbb;
    int nlb;
    char title[256];
    char avlab[256];
    char dat_tim[256];
};

#define CUBE_SIZE(d) (d).samples, (d).lines, (d).bands, (d).label_size

/* this is what d->org translates into */

static char *vicar_org[] = {0, "BIL", "BIP", "BSQ"};

#endif
