/**
 ** Make sure there is a default include directory.
 **/

#ifndef X11_INCLUDE
#define X11_INCLUDE	"/usr/include/X11"
#endif

#ifdef __STDC__
	Pixmap XfStipple(Display *, Drawable, char *);
	short XfColor(Display *, char *);
	XFontStruct *XfFont(Display *, char *);
	void XfSetDefaultFont(Display *, XFontStruct *);
#else
	Pixmap XfStipple();
	short XfColor();
	XFontStruct *XfFont();
	void XfSetDefaultFont();
#endif
