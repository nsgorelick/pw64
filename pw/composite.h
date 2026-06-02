#ifndef PW_COMPOSITE_H
#define PW_COMPOSITE_H

#include "Xfred.h"
#include "image.h"
#include "pw_cast.h"
#include "util.h"

#define AddSolidVisual(b, s, fg, bg) XfAddButtonVisual(b, s, XfCreateVisual(b, 0, 0, 0, 0, fg, bg, XfSolidVisual))

#define AddTextVisual(b, s, fg, bg, t, f, a)                                                                           \
    XfAddButtonVisual(b, s, XfCreateVisual(b, 0, 3, 0, 0, fg, bg, XfTextVisual, t, f, a));

extern XColor pwHilite;
extern XColor pwBackground;

struct _Requestor {
    Display *display; /* Display the Requestor is on */
    Window window; /* The window created for Requestor */
    Window parent; /* Parent window's ID */
    int active; /* Whether or not Requestor is active */
    int width, height; /* sizing of Requestor */
    int x, y; /* Position of Requestor relative to parent */
    int border_width; /* width in pixels of border */
    unsigned long border_color; /* color of Requestor's border */
    char name[256]; /* Textual handle associated with Requestor */
    char *ext; /* Simply a data handle */
    int *member; /* A "class" identifier */
    Button letters; /* The button that holds the letters */
    Button letter[NIMAGE]; /* The button that holds the letters */
    Button fnames; /* The button that holds the filenames */
    Button bands; /* The button for the bands */
    Button rgbs[NIMAGE]; /* The 26 RGB toggles */
    Button comps[NIMAGE]; /* The 26 composite toggles */
    Button fname[NIMAGE]; /* The visuals associated with fnames */
    struct VisualInfo *bnd[NIMAGE]; /* The vis info for bands */

    Button UserMsg; /* Messages to the user go here */

    Button HeaderLines;
    Button HeaderSamples;
    Button HeaderBands;
    Button HeaderFormat;
    Button HeaderOrg;
    Button HeaderLabel;
    Button HeaderOrder;

    Button SubsetLine;
    Button SubsetSample;
    Button SubsetWidth;
    Button SubsetHeight;
    Button SubsetLSkip;
    Button SubsetSSkip;

    Button SlowMem; /* memory mode selection */
    Button MediumMem;
    Button FastMem;

    Button UsedColors; /* number of colors used */
    Button AvailColors;

    Button DeleteImage; /* delete current image */
    Button RestartSave; /* save restart file */
    Button RestartRead; /* load restart file */
};

typedef struct _Requestor *Requestor;

Requestor CreateRequestor(Display *display, Window parent, int x, int y, int border_width,
                          long unsigned int border_color, char *name, XFontStruct *font);
int ActivateRequestor(Requestor C);
int DeactivateRequestor(Requestor C);
int UpdateRequestor(Requestor R, int n);

/* composite.c internals (also used from buttons.c, restart.c, etc.) */
int is_file(char *filename);
int UpdatePushButtons(Requestor R, int n);
int off_load_filename(int i, char *buf, Requestor R, int stretch, int colors, int band);
char *trim_filename(char *s, int n);
Image allocate_image(void);
int free_image(Display *display, Image *new);
int load_info(Image new, Requestor R);
int create_image(Display *display, Image new);
int create_hist(Display *display, Image new, int size, int type, int scale);
int make_hist_freq(Image new, int *C, int size, int scale_in);
int make_hist_dist(Image new, int *C, int size, int scale_in);
int overlay(char *data, int start, struct quant_data *quant, int ncolors, int *map);
int update_display(Display *display, Image new);
int load_fast_mem(Display *display, Image new);
int default_map(Image new);
int create_pan(Display *display, Image new);
int set_no_header(Image new);
int GetNewText(Button B, XEvent *E, char *s, int n, char *str);
int ButtonHilite(Button B, unsigned int fg, unsigned int bg);
int FlagForLoad(Requestor R, int i);
int UnFlagForLoad(Requestor R, int i);
int compute_subset(Requestor R, int i);
int SetUserMsg(char *buf);

#endif /* PW_COMPOSITE_H */
