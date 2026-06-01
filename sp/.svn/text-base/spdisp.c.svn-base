#include <math.h>
#include "Xfred.h"
#include <X11/keysym.h>
#include "pw/specpr.h"
#include "hershey.h"
#include "sp.h"

extern Display *display;
extern int screen;
extern int depth;
extern GC gc;
extern Font font;
extern Colormap ColorMap;

extern XColor Background;

void PlotMainCallback();
void redraw_axis();
PlotItem **make_item();
XPoint SpToPoint();
Plot *CreatePlot();
void getscale();
void ResizeDone();
void autoscale();


Plot *
CreatePlot(display, font)
Display *display;
Font font;
{
	Button B;
	Plot *P;
	int x,y;
	int width,height;

	P = (Plot *)malloc(sizeof(Plot));

	P->display = display;
	P->gc = gc;

/* all of these should get set when the thing gets created */
/* and the windows will get sized */

	P->xlow = P->ylow = 0.0;
	P->xhigh = P->yhigh = 1.0;
	P->width = 0;
	P->height = 0;
	P->nitems = 0;

	B = XfCreateButton(display, RootWindow(display, DefaultScreen(display)),
	    300, 300, 300, 300, 1, WHITE(display), "SP Plotting Window", 1);
	XfAddButtonCallback(B, 0, PlotMainCallback, NULL);
	B->member = (int *)P;
	P->Main = B;

	B = XfCreateButton(display, P->Main->window,
	    0, 0, 300, 50,
	    1, BLACK(display), "Command", 1);
	XfActivateButton(B, ExposureMask);
	B->member = (int *)P;
	P->Command = B;

	B = XfCreateButton(display, P->Main->window,
	    0, 0, 10, 10,
	    1, BLACK(display), "Axis", 1);
	XfAddButtonCallback(B, 0, redraw_axis, NULL);
	XfNoAutoExposeButton(B);
	XfActivateButton(B, ExposureMask | ButtonPressMask);
	B->member = (int *)P;
	P->Axis = B;

	B = XfCreateButton(display, P->Main->window,
	    0, 0, 10, 10,
	    1, BLACK(display), "Plot", 1);

	/*
		At some point this needs to respond to button clicks
		and all that sorta stuff, so that it can track the mouse
		and such.  That callback will want to be intercepted by
		controlling programs so that special things can happen
		on the plot surface.

		Currently, all that really needs to happen is for the plot
		window to update its contents.
	*/

	XfAddButtonCallback(B, 0, PlotMainCallback, NULL);
	XfNoAutoExposeButton(B);
	B->member = (int *)P;
	P->PlotArea = B;
	XfActivateButton(B, ExposureMask | ButtonPressMask);

	/* set Main to force resize update */

	P->Main->ext = (char *)-1;
	XfActivateButton(P->Main, ExposureMask | StructureNotifyMask);

	width = 50;
	height = 15;

	B = P->Bresize = XfCreateButton(display, 
		RootWindow(display, DefaultScreen(display)),
	    100, 100, width*3+60, 100, 1, WHITE(display), "SP Plot Resize", 1);

	x = 10;
	y = 20;
	XfAddButtonVisual(B, 0, XfCreateVisual(B, x, y+3, 0, 0,
	    BLACK(display), WHITE(display), 
	    XfTextVisual, "X", font, 1));

	y += height+5;
	XfAddButtonVisual(B, 0, XfCreateVisual(B, 10, y+3, 0, 0,
	    BLACK(display), WHITE(display), 
	    XfTextVisual, "Y", font, 1));

	x = 20;
	y = 5;
	XfAddButtonVisual(B, 0, XfCreateVisual(B, x, y+1, 40, 0,
	    BLACK(display), WHITE(display), 
	    XfTextVisual, "Low", font, 0));

	x += width+10;
	XfAddButtonVisual(B, 0, XfCreateVisual(B, x, y+1, 40, 0,
	    BLACK(display), WHITE(display), 
	    XfTextVisual, "High", font, 0));

	x = 20;
	y = 20;
	B = P->BresizeXlow = XfCreateButton(display, P->Bresize->window,
	    x, y, width,height, 1, BLACK(display), "Xlow", 1);
	XfAddButtonVisual(B, 0, XfCreateVisual(B, 0, 3, 0, 0,
	    BLACK(display), WHITE(display), 
	    XfTextVisual, "0.0", font, 0));
	XfAddButtonCallback(B, 0, getscale, NULL);
	B->member = P;
	B->ext = (char *)1;
	XfActivateButton(B, ExposureMask | ButtonPressMask | KeyPressMask);

	x += width+10;
	B = P->BresizeXhi = XfCreateButton(display, P->Bresize->window,
	    x, y, width, height, 1, BLACK(display), "Xhi", 1);
	XfAddButtonVisual(B, 0, XfCreateVisual(B, 0, 3, 0, 0,
	    BLACK(display), WHITE(display), 
	    XfTextVisual, "1.0", font, 0));
	XfAddButtonCallback(B, 0, getscale, NULL);
	B->member = P;
	B->ext = (char *)2;
	XfActivateButton(B, ExposureMask | ButtonPressMask | KeyPressMask);

	x += width+10;
	B = XfCreateButton(display, P->Bresize->window,
	    x, y, width, height, 1, BLACK(display), "AutoX", 1);
	XfAddButtonVisual(B, 0, XfCreateVisual(B, 0, 3, 0, 0,
	    BLACK(display), Background.pixel,
	    XfTextVisual, "AUTO", font, 0));
	XfAddButtonCallback(B, 0, autoscale, NULL);
	B->member = (void *)P;
	XfActivateButton(B, ExposureMask | ButtonPressMask);

	x = 20;
	y += height+5;
	B = P->BresizeYlow = XfCreateButton(display, P->Bresize->window,
	    x, y, width, height, 1, BLACK(display), "Ylow", 1);
	XfAddButtonVisual(B, 0, XfCreateVisual(B, 0, 3, 0, 0,
	    BLACK(display), WHITE(display), 
	    XfTextVisual, "0.0", font, 0));
	XfAddButtonCallback(B, 0, getscale, NULL);
	B->member = (void *)P;
	B->ext = (char *)3;
	XfActivateButton(B, ExposureMask | ButtonPressMask | KeyPressMask);

	x += width+10;
	B = P->BresizeYhi = XfCreateButton(display, P->Bresize->window,
	    x, y, width, height, 1, BLACK(display), "Yhi", 1);
	XfAddButtonVisual(B, 0, XfCreateVisual(B, 0, 3, 0, 0,
	    BLACK(display), WHITE(display), 
	    XfTextVisual, "1.0", font, 0));
	XfAddButtonCallback(B, 0, getscale, NULL);
	B->member = (void *)P;
	B->ext = (char *)4;
	XfActivateButton(B, ExposureMask | ButtonPressMask | KeyPressMask);

	x += width+10;
	B = XfCreateButton(display, P->Bresize->window,
	    x, y, width, height, 1, BLACK(display), "AutoY", 1);
	XfAddButtonVisual(B, 0, XfCreateVisual(B, 0, 3, 0, 0,
	    BLACK(display), Background.pixel,
	    XfTextVisual, "AUTO", font, 0));
	XfAddButtonCallback(B, 0, autoscale, NULL);
	B->member = (void *)P;
	XfActivateButton(B, ExposureMask | ButtonPressMask);


	x = 20;
	y += height+10;

	B = XfCreateButton(display, P->Bresize->window,
	    x, y, width*3+20, height, 1, BLACK(display), "Done", 1);
	XfAddButtonVisual(B, 0, XfCreateVisual(B, 0, 3, 0, 0,
	    BLACK(display), Background.pixel,
	    XfTextVisual, "Done", font, 0));
	XfAddButtonCallback(B, 0, ResizeDone, NULL);
	B->ext = (char *)2;
	B->member = P;
	XfActivateButton(B, ExposureMask | ButtonPressMask);

	return(P);
}

void
PlotMainCallback(B,E)
Button B;
XEvent *E;
{
	if (E->type == Expose) { /* Only PlotArea can get this one */
		while(XCheckTypedWindowEvent(B->display, B->window, Expose, E))
			;
		replot_all((Plot *)B->member);
	} else if (E->type == ConfigureNotify) {
		resize_all((Plot *)B->member, E->xconfigure.width,E->xconfigure.height);
	} else if (E->type == ButtonPressMask) {
		if (E->xbutton.button == Button3) {
			DestroyPlot((Plot *)B->member);
		}
	}
}

resize_all(P, width,height)
Plot *P;
int width,height;
{
	int w,h;
	int pheight;
	/**
	*** Y-Axis width should be .2*width or
	*** X-Axis height should be .2*height (minimum of the two)
	***
	*** X-Axis height should be 2/3 Y-Axis width.
	**/

	if (width < 40 || height < 40) return;
	if (width != P->Main->width || 
		height != P->Main->height || 
		(int)P->Main->ext == -1) {

		pheight = P->Command->height;

		w = MIN(.2*width, ((height-pheight)*.2)*3/2);

		ResizeButton(P->Axis, 0, 0, width, height-pheight);
		ResizeButton(P->PlotArea, w, 0, width-(w),height-pheight-(w*2/3));
		P->Main->width = width;
		P->Main->height = height;

		/* move command line here */
		P->width = P->PlotArea->width;
		P->height = P->PlotArea->height;

		P->Main->ext == (char *)0;
		XFlush(P->PlotArea->display);
	}
}

clear_plot(P)
Plot *P;
{
	Button B;

	B = P->PlotArea;
	XSetForeground(B->display, gc, WHITE(display));
	XFillRectangle(B->display, B->window, gc, 0, 0, B->width, B->height);
}


replot_all(P)
Plot *P;
{
	int i;

	clear_plot(P);
	for (i = 0 ; i < P->nitems ; i++) {
		plot(P, P->itemlist[i]);
	}
}


plot(P, item)
Plot *P;
PlotItem *item;
{
	XPoint *points;

	if (item->type == PLOT) {
		points = (XPoint *) SpToX(item->plot.x, item->plot.y, 
										item->plot.npts,
										P->xlow, P->xhigh, P->ylow, P->yhigh,
										P->width, P->height);
		XSetForeground(P->display, P->gc, item->plot.color);
		XDrawLines(P->display, P->PlotArea->window, P->gc, points,
					item->plot.npts, CoordModeOrigin);
	}
}


RescaleCallback(B, E)
Button B;
XEvent *E;
{
	if (E->type == ButtonPress) {
		ResizeAxis(B->member);
		return(1);
	}
}


void
redraw_axis(B,E)
Button B;
XEvent *E;
{
	float increment, start, ticstart, ticincr;
	XPoint point;
	char buf[256];
	float loc, tloc;
	int i, j;
	Plot *P = (Plot *)B->member;
	float fsize;
	int factor;
	int ntics;
						
	if (E != NULL && RescaleCallback(B,E)) return;

	XSetForeground(B->display, gc, WHITE(display));
	XFillRectangle(B->display, B->window, gc, 0, 0, B->width, B->height);
	XSetForeground(B->display, gc, BLACK(display));

	inice(P->ylow,P->yhigh,6,&increment,&start);
	inice(start,start+increment,6,&ticincr,&ticstart);

	ntics = floor(increment/ticincr+0.5);

	factor = B->width - P->width;
	fsize = (float)factor/200.0;

	if (start > P->ylow) start -= increment;

	for (i = 0 ; i < 7 ; i++) {
		loc = start + (float)i*increment;
		if (loc > P->yhigh || loc < P->ylow) continue;
		point = SpToPoint(0.0, loc,
						P->xlow, P->xhigh, P->ylow, P->yhigh,
						P->width, P->height);
		
		sprintf(buf,"%.4g",loc);
		XfHersheyString(B->display, B->window, gc, 
						(int)(factor*.75),  point.y, 
						fsize, fsize, 0.0,
						buf, ROMAN_COMPLEX, -1.0);

		/* major tic mark */
		XDrawLine(B->display, B->window, gc, 
					(int)(0.9*factor), point.y,
					(int)(factor), point.y);

		/* minor tics */
		for (j = 0 ; j < ntics ; j++) {
			tloc = loc - j*ticincr;
			if (tloc < P->ylow) continue;
			point = SpToPoint(0.0, tloc,
							P->xlow, P->xhigh, P->ylow, P->yhigh,
							P->width, P->height);
			
			XDrawLine(B->display, B->window, gc, 
							(int)(0.95*factor), point.y,
							(int)(factor), point.y);
		}
	}

	XfHersheyString(B->display, B->window, gc, 
					(int)(.1*factor), P->height/2,
					fsize, fsize,
					acos(-1.0)/2, "Y AXIS", ROMAN_COMPLEX, 0.0);

	inice(P->xlow,P->xhigh,6,&increment,&start);
	inice(start,start+increment,6,&ticincr,&ticstart);

	ntics = floor(increment/ticincr + 0.5);
	if (start > P->xlow) start -= increment;

	for (i = 0 ; i < 7 ; i++) {
		loc = start + i*increment;
		if (loc > P->xhigh || loc < P->xlow) continue;

		point = SpToPoint(loc, 0.0,
						P->xlow, P->xhigh, P->ylow, P->yhigh,
						P->width, P->height);
		sprintf(buf,"%.4g",loc);
		XfHersheyString(B->display, B->window, gc,
						(factor-1) + point.x, (int)(P->height+0.3*factor),
						fsize, fsize, 0.0,
						buf, ROMAN_COMPLEX, 0.0);

		/* major tic mark */
		XDrawLine(B->display, B->window, gc, 
					(factor-1) + point.x, (int)(P->height+0.2*factor),
					(factor-1) + point.x, (int)(P->height));
		for (j = 0 ; j < ntics ; j++) {
			tloc = loc - j*ticincr;
			if (tloc < P->xlow) continue;
			point = SpToPoint(tloc, 0.0,
							P->xlow, P->xhigh, P->ylow, P->yhigh,
							P->width, P->height);
			
			XDrawLine(B->display, B->window, gc, 
							(factor-1) + point.x, (int)(P->height + 0.1*factor),
							(factor-1) + point.x, (int)(P->height));
		}
	}
	XfHersheyString(B->display, B->window, gc, 
					(factor-1) + P->width/2, (int)(P->height + factor*.6),
					fsize, fsize,
					0.0, "X AXIS", ROMAN_COMPLEX, 0.0);
}

PlotItem **
make_item(P)
Plot *P;
{
	PlotItem *item;
	PlotItem **iptr;
	int i;

	P->itemlist = (PlotItem **)malloc(sizeof(PlotItem *));
	P->nitems = 1;
	P->itemlist[0] = item = (PlotItem *)malloc(sizeof(PlotItem));
	item->type = PLOT;
	item->plot.x = (float *)malloc(20*sizeof(float));
	item->plot.y = (float *)malloc(20*sizeof(float));
	item->plot.npts = 20;
	item->plot.color = BLACK(display);
	for (i = 0  ; i < 20 ; i++) {
		item->plot.x[i] = (float)i/20.0;
		item->plot.y[i] = (float)((i%3)+1)/3.0;
	}
}

/*
	Control options:

		* Control Keys w/ popup help.
		* Popup Menu
		* Button operation	
 */

DestroyPlot(P)
Plot *P;
{
	int i;
	for (i = 0 ; i < P->nitems ; i++) {
		free(P->itemlist[i]->plot.x);
		free(P->itemlist[i]->plot.y);
		free(P->itemlist[i]);
	}
	P->itemlist = (PlotItem **)malloc(1);
	P->nitems = 0;
	XSetForeground(P->display, P->gc, WHITE(P->display));
	XFillRectangle(P->display, P->PlotArea->window, gc, 0, 0, 
				P->PlotArea->width, P->PlotArea->height);
}

void
getscale(B,E)
Button B;
XEvent *E;
{
	Plot *P;
	float *fptr = NULL;
	char s[256];
	s[0] = '\0';

	P = (Plot *)B->member;

	if (GetText(B,E,s,256,0) != -1) {
		switch ((int)B->ext) {
			case 1:
				P->xlow = atof(s);
				break;
			case 2:
				P->xhigh = atof(s);
				break;
			case 3:
				P->ylow = atof(s);
				break;
			case 4:
				P->yhigh = atof(s);
				break;
		}
		SetButtonText(B, s);
		replot_all(P);
		redraw_axis(P->Axis, NULL);
	}
	return;
}


ResizeAxis(P)
Plot *P;
{
	XfActivateButton(P->Bresize, ExposureMask);
}


void
ResizeDone(B,E)
Button B;
XEvent *E;
{
	Plot *P;
	P = B->member;
	XfDeactivateButton(P->Bresize);
}


void
autoscale(B,E)
Button B;
XEvent *E;
{

}
