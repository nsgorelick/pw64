/**
 ** Make sure there is a default include directory.
 **/

#ifndef X11_INCLUDE
#if defined(__APPLE__)
#define X11_INCLUDE "/opt/X11/include/X11"
#else
#define X11_INCLUDE "/usr/include/X11"
#endif
#endif

Pixmap XfStipple(Display *, Drawable, char *);
short XfColor(Display *, char *);
XFontStruct *XfFont(Display *, char *);
void XfSetDefaultFont(Display *, XFontStruct *);
