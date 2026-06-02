#ifndef _PB_H_
#define _PB_H_

struct tagPB {
    int type;
    Display *display;
    Window parent;
    Window window;
    int width, height;
    int hi, lo, fg, bg; /* 3-D colors */

    CallBack function;
    XButton nextB;

    int state;
    char *text;
    int align;
    XFontStruct *font;
    Pixmap pixmap;
    int pix_w;
    int pix_h;
};

#define XfResizePB(PB, w, h) XfPosXB((XButton)PB, MAXINT, MAXINT, w, h)
#define XfMovePB(PB, x, y) XfPosXB((XButton)PB, x, y, MAXINT, MAXINT)

/**
*** Function declarations
**/
PButton XfCreatePB(Display *, Window, int, int, int, int, int, int, int, int, char *, int, XFontStruct *, CallBack);
int XfPushPB(PButton, XEvent *);
void RedrawPB(PButton);
Pixmap XfSetPBPixmap(PButton, Pixmap, int, int);

#endif /* _PB_H_ */
