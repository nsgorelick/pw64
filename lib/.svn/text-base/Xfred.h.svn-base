/*
 * Xfred.h
 *
 * Main header file for the library Xfred.a. Mostly just includes the
 * header files for the various parts, but also defines a union-type
 * thing for generic visual manipulation.
 *
 * $Header$
 *
 * $Log$
 * Revision 1.2  2001/01/16 06:04:42  gorelick
 * Fixed a bunch of compiler warnings
 *
 * Revision 1.1.1.1  1999/09/09 17:50:38  gorelick
 * Initial import
 *
 * Revision 0.2  91/09/23  17:55:10  17:55:10  ngorelic (Noel S. Gorelick)
 * *** empty log message ***
 * 
 * Revision 0.1  91/07/24  18:04:15  18:04:15  rray (Randy Ray)
 * *** empty log message ***
 * 
 *
 */
#ifndef _XF_XFRED_H
#define _XF_XFRED_H

#define BLACK(d) BlackPixel(d, DefaultScreen(d))
#define WHITE(d) WhitePixel(d, DefaultScreen(d))


#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <values.h>
#include <X11/Xos.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "Callback.h"
#include "VisInfo.h"
#include "Button.h"
#include "Slider.h"
#include "Joystick.h"
#include "AMap.h"
#include "List.h"
#include "Composite.h"
#include "xf.h"
#include "3D.h"
#include "hershey.h"

#include "XB.h"

char *xgets(Display *display, Window window, int x, int y, int width, int height, long unsigned int fg, long unsigned int bg, XFontStruct *font, char *input, XEvent *e);
void toggle_state(Button B, XEvent *E);
void set_state(Button B, int i);

#ifndef MAX
#define MAX(a,b) (a > b ? a : b)
#endif

#ifndef MIN
#define MIN(a,b) (a < b ? a : b)
#endif

#define DiPRINT(i) printf("%s = %d\n",#i, i);
#define DsPRINT(i) printf("%s = %s\n",#i, i);


/**
*** some default globals
**/

extern Display *_xfDisplay;
extern int _xfScreen;
extern int _xfDepth;
extern GC *_xfgc;
extern XFontStruct *_xfFontStruct;
extern int _xf3Dheight;

int initx(char *name, Display **d, int *s, int *dpth, GC *gcptr);

#define malloc(s)	calloc(1,s)

/* Some significant, pre-provided callbacks */

/* The default assigned to expose- and update- callbacks for buttons */
/* void defaultButtonCallback(Button, XEvent *); */

/* The defaults assigned to the default slots of sliders */
void defaultSliderCallback(Slider S, XEvent *E);
void defaultSliderUpdateCallback(Slider S, XEvent *E);

/* The defaults for joysticks */
void defaultJoystickCallback(Joystick J, XEvent *E);
void defaultJoystickUpdateCallback(Joystick J, XEvent *E);

/* Default for AMap's */
void defaultAMapCallback(AMap A, XEvent *E);
void defaultAMapUpdateCallback(AMap A, XEvent *E);

int GetText (Button B, XEvent *E, char *s, int n, int copy);
int HitTest (MButton MB, MenuItem *menu, int x, int y);
int ListIsDoubleClick (List list, XEvent *E);
int Xf3DHeight (void);
int XfPushMB (MButton MB, XEvent *E);
int XfRescaleSlider (Slider S, float lo, float hi, float d);
int XfSetJoystickValue(Joystick J, int x, int y);
int complete_dir (char *str);
int do_keysym (Display *display, XEvent *event, char *str, int nchars);
int draw(List list, int i, short int bg);
int get_sorted_dir (char *path, char ***ds);
void ActivateList (List list);
void AddListCallback (List list, CallBack proc);
void Cedit_callback(Button B, XEvent *E);
void Clist_callback(List L, XEvent *E);
void Cpopdown_callback(Button B, XEvent *E);
void DeactivateList (List list);
void Draw3DBox (Display *disp, Window win, GC gc, int x, int y, int w, int h, int r, short int hi, short int lo, short int fg, short int bg, int state, int options);
void ListCallback(Button B, XEvent *E);
void ListSlider(Slider S, XEvent *E);
void MBActivatePopup(MButton MB);
void MBDeactivatePopup(MButton MB);
void ReCreateList (List list, int nitems, char **items);
void RedrawMB (MButton MB);
void RedrawMB(MButton MB);
void RefreshList(List list);
void XfClearModalList(void);
void XfDeleteFromModalList(XButton B);
void XfSetDefaultFont (Display *display, XFontStruct *fs);
void decode_control(Display *display, KeySym k, char *str);
void displayJoystick(Joystick J, GC localGC);
void file_completion(Display *display, char *str);
void insert_str(char *s, char *t, int at);
Composite CreateComposite(Display *display, Window parent, XFontStruct *font, int x, int y, int width, int height, short int hilite, char *instr);

#endif /* _XF_XFRED_H */
