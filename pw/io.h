#ifndef IO_H
#define IO_H

#include <stdio.h>
#include <stdlib.h>

/*
 * element format.
 * Var->value.Sym->format
 */

#define BYTE 1
#define SHORT 2
#define INT 3
#define FLOAT 4
#define VAX_FLOAT 5
#define VAX_INTEGER 6
#define DOUBLE 8

#define NBYTES(a) ((a) == INT ? 4 : ((a) == VAX_FLOAT ? 4 : ((a) == VAX_INTEGER ? 2 : (a))))

/*
 * Data axis order
 * Var->value.Sym->order
 * !!! CAUTION: these values must be 0 based.  They are used as array
 * indices below.
 */

#define BSQ 0
#define BIL 1
#define BIP 2

#define Format2Str(i) FORMAT2STR[(i)]
#define Org2Str(i) ORG2STR[(i)]

#define GetSamples(s, org) (s)[((org) == BIP ? 1 : 0)]
#define GetLines(s, org) (s)[((org) == BSQ ? 1 : 2)]
#define GetBands(s, org) (s)[((org) == BIP ? 0 : ((org) == BIL ? 1 : 2))]

extern int orders[3][3];
extern char *FORMAT2STR[];
extern char *ORG2STR[];
extern int VERBOSE;

struct _iheader {
    int dptr; /* offset in bytes to first data value    */
    int prefix[3]; /* size of prefix data (bytes)            */
    int suffix[3]; /* size of suffix data (bytes)            */
    int size[3]; /* size of data (pixels)                  */
    int s_lo[3]; /* subset lower range (pixels)            */
    int s_hi[3]; /* subset upper range (pixels)            */
    int s_skip[3]; /* subset skip interval (pixels)          */
    int dim[3]; /* final dimension size */
    int corner;

    int byte_order; /* byteorder of data                      */
    int format; /* data format (INT, FLOAT, etc)          */
    int eformat; /*Add iomedley compatability */
    void *iom_h; /*Hook in the iomedley header...needed  */
    int org; /* data organization                      */

    float gain, offset; /* data multiplier and additive offset    */
};

#endif
