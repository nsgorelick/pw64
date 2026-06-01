#ifndef _RB_H_
#define _RB_H_

#define RBSIZE 15               /* size of the pushbutton in a RadioButton */

struct tagRB {
    int type;
    Display *display;
    Window parent;
    Window window;
    int width, height;
    int hi, lo, fg, bg;         /* 3-D colors */

    CallBack function;
    XButton nextB;

    int state;
    char *text;
    int align;
    XFontStruct *font;
    Pixmap pixmap;
    int pix_w;
    int pix_h;

    RButton nextRB;             /* next element in xor list */
    RButton headRB;             /* head element in xor list */
};

#define XfResizeRB(RB, w, h)    XfPosXB(RB, MAXINT, MAXINT, w, h)
#define XfMoveRB(RB, x, y)      XfPosXB(RB, x, y, MAXINT, MAXINT)

/**
*** Function declarations
**/
int XfPushRB(RButton, XEvent *);
void RedrawRB(RButton);
void XfAddRB(RButton, RButton);
RButton XfCreateRB(Display * d, Window win,
                   int x, int y, int w, int h,
                   short int hi, short int lo,
                   short int fg, short int bg,
                   char *text, int align, XFontStruct * font, CallBack func, RButton RBlist);
void XfActivateRBList(RButton, int);
void RBPixmaps(Display *, GC);
void XfDestroyRB(RButton);
int XfClickedRB(RButton);
void XfSetActiveRB(RButton, int);
int XfCountRB(RButton);
int XfWhichRB(RButton);
RButton XfGetRB(RButton, int);

#endif                          /* _RB_H_ */
