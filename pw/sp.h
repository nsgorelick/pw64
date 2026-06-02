#include <Xfred.h>

/*
 * This is a stack (list) of items that go onto a plot.
 */

#define PLOT 1
#define TEXT 2
#define SYMBOL 3

typedef struct { /* A PLOT struct */
    int type;
    float *x;
    float *y;
    int npts;
    XPoint *points;
    short color;
} XfpPlot;

typedef struct { /* A Text struct */
    int type;
    float x;
    float y;
    int nchar;
    char *text;
} XfpText;

typedef struct { /* A hershey symbol struct */
    int type;
    float x;
    float y;
    float angle;
    float scale;
    int symbol;
    int cset;
} XfpSymbol;

typedef union {
    int type;
    XfpPlot plot;
    XfpText text;
    XfpSymbol symbol;
} PlotItem;

typedef struct {
    Display *display;
    GC gc;
    float xlow, xhigh, ylow, yhigh;
    int width, height;

    Button Main; /* The parent window */
    Button Axis; /* Axis window */
    Button PlotArea; /* Plot Area Window */
    Button Command; /* The command/control window */

    Button Bresize;
    Button BresizeXlow;
    Button BresizeXhi;
    Button BresizeYlow;
    Button BresizeYhi;

    int nitems;
    PlotItem **itemlist; /* The stuff that is in this plot */
} Plot;
