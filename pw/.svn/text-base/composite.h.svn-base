#include "Xfred.h"
#include "image.h"
#include "util.h"

#define AddSolidVisual(b,s,fg,bg) \
    XfAddButtonVisual(b,s,XfCreateVisual(b,0,0,0,0,fg,bg,XfSolidVisual))

#define AddTextVisual(b,s,fg,bg,t,f,a) \
    XfAddButtonVisual(b,s,XfCreateVisual(b,0,3,0,0,fg,bg,XfTextVisual,t,f,a));


extern XColor pwHilite;
extern XColor pwBackground;

struct _Requestor
{
    Display* display;       /* Display the Requestor is on */
    Window window;      /* The window created for Requestor */
    Window parent;      /* Parent window's ID */
    int active;         /* Whether or not Requestor is active */
    int width, height;      /* sizing of Requestor */
    int x, y;           /* Position of Requestor relative to parent */
    int border_width;       /* width in pixels of border */
    unsigned long border_color; /* color of Requestor's border */
    char name[256];     /* Textual handle associated with Requestor */
    char* ext;          /* Simply a data handle */
    int* member;        /* A "class" identifier */
    Button letters;     /* The button that holds the letters */
    Button letter[NIMAGE];      /* The button that holds the letters */
    Button fnames;      /* The button that holds the filenames */
    Button bands;       /* The button for the bands */
    Button rgbs[NIMAGE];        /* The 26 RGB toggles */
    Button comps[NIMAGE];       /* The 26 composite toggles */
    Button fname[NIMAGE];   /* The visuals associated with fnames */
    struct VisualInfo* bnd[NIMAGE]; /* The vis info for bands */

    Button UserMsg;         /* Messages to the user go here */

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

    Button SlowMem;         /* memory mode selection */
    Button MediumMem;
    Button FastMem;

    Button UsedColors;      /* number of colors used */
    Button AvailColors;
    
    Button DeleteImage;     /* delete current image */
    Button RestartSave;     /* save restart file */
    Button RestartRead;     /* load restart file */
};

typedef struct _Requestor* Requestor;

Requestor CreateRequestor(Display *display, Window parent, int x, int y, int border_width, long unsigned int border_color, char *name, XFontStruct *font);
int ActivateRequestor(Requestor C);
int DeactivateRequestor(Requestor C);
int UpdateRequestor(Requestor R, int n);
int DestroyRequestor();
