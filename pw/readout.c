#include "Xfred.h"
#include "util.h"
#include "image.h"

/*
 * Data readout.
 *
 * Includes control panel readout and popup readout.
 *
 * CreateReadout
 *
 * Update Readout
 *
 * CreatePopup
 * ActivatePopup
 * DeactivatePopup
 *
 */
struct readout {
    Button parent;
    Button Lines;
    Button Samples;
    Button Dn;
} *Readout;

struct popup_readout {
    XFontStruct *font;
    Button parent;
    Button Line;
    Button Sample;
    Button UTMN;
    Button UTME;
    Button Dn[NIMAGE][4];
	Button Names[NIMAGE];
    int NDn;
    int DnSize;

	Button ReadoutToggle;
	Button ColorKeyToggle;

	Button ColorKeyCover;
	Button ReadoutCover;
	Button PlaneCover;
} *PopupReadout;

extern Colormap ColorMap;

char *ColorKeyNames[NIMAGE] = { NULL, NULL, NULL, NULL, 
                            NULL, NULL, NULL, NULL, 
                            NULL, NULL, NULL, NULL, 
                            NULL, NULL, NULL, NULL,
                            NULL, NULL, NULL, NULL,
                            NULL, NULL, NULL, NULL,
                            NULL, NULL };

void RaisePopup(Button B, XEvent *E);
void ChangeImageDnScaleFactor(Button B, XEvent *E);
void ExpandColorKey(Button B, XEvent *E);
void WriteColorKey(Button B, XEvent *E);
void ChangeColorKeyName(Button B, XEvent *E);
void SetColorKeyFilename(Button B, XEvent *E);

extern XColor pwBackground;
char * trim_filename(char *s, int n);
char *ColorKeyFilename = NULL;




int CreatePopupReadout (Display *display, XFontStruct *font);
int UpdatePopupReadout (int x, int y);
int DeactivatePopupReadout (void);
int ActivatePopupReadout (void);
extern int GetNewText (Button B, XEvent *E, char *s, int n, char *str);
int RescalePopupReadout (void);

CreateReadout(Display *display, Window parent, XFontStruct *font)
{
    Window w;
    Button B;
    int x,y,width,height;

    Readout = (struct readout *)malloc(sizeof(struct readout));

    B = XfCreateButton(display, parent, 10,110,97,45,1,BLACK(display),"W",1);
    XfAddButtonVisual(B,0,XfCreateVisual(B, 0,3,0,0,
        WHITE(display),WHITE(display),
        XfSolidVisual));

    XfAddButtonVisual(B,0,XfCreateVisual(B, 0,3,0,0,
        WHITE(display),WHITE(display),
        XfSolidVisual));
    XfAddButtonCallback(B, 0, RaisePopup, NULL);
    Readout->parent = B;
    w = B->window;

    x = 2;
    y = 2;
    width = 50;
    height = 12;

    B = XfCreateButton(display, w, x, y, width, height, 
        0, BLACK(display), "SlowMem", 1);
    XfAddButtonVisual(B, 0, XfCreateVisual(B, 0, 3, 0, 0, 
        BLACK(display),WHITE(display), 
        XfTextVisual, "L 00000", font, 1));
    XfActivateButton(B,ExposureMask);
    Readout->Lines = B;

	y += height+2;

    B = XfCreateButton(display, w, x, y, width, height, 
        0, BLACK(display), "SlowMem", 1);
    XfAddButtonVisual(B, 0, XfCreateVisual(B, 0, 3, 0, 0, 
        BLACK(display),WHITE(display), 
        XfTextVisual, "S 00000", font, 1));
    XfActivateButton(B,ExposureMask);
    Readout->Samples = B;

	y += height+2;
	width = 55;

    B = XfCreateButton(display, w, x, y, width, height, 
        0, BLACK(display), "SlowMem", 1);
    XfAddButtonVisual(B, 0, XfCreateVisual(B, 0, 3, 0, 0, 
        BLACK(display),WHITE(display), 
        XfTextVisual, "D 0123456", font, 1));
    XfActivateButton(B, ExposureMask);
    Readout->Dn = B;

    y += height;

    XfActivateButton(Readout->parent,ExposureMask | ButtonPressMask);
    CreatePopupReadout(display,font);
}
UpdateReadout(int x, int y, Image new)
{
    Button B;
    short *data;
    int width,height;

    char xbuf[16];
    char ybuf[16];
    char zbuf[16];

    if (new != NULL) {

        if (x < 0) x = 0;
        if (y < 0) y = 0;
        if (x >= new->subset.width) x = new->subset.width-1;
        if (y >= new->subset.height) y = new->subset.height-1;

        sprintf(xbuf,"S %d",x+1);
        sprintf(ybuf,"L %d",y+1);

        SetButtonText(Readout->Lines, ybuf);
        SetButtonText(Readout->Samples, xbuf);

        if (new->composite == 0) {
            width = new->subset.width;
/*			sprintf(zbuf, "D %.3g", get_data(new, y*width+x));	***ORIGINAL***/
			sprintf(zbuf, "D %f", get_data(new, y*width+x));	/**Modified 9/13/99**/
        } else {
            sprintf(zbuf,"D --");
        }
        SetButtonText(Readout->Dn, zbuf);
    }

    if (PopupReadout->parent->active) {
        UpdatePopupReadout(x,y);
    }
}

void
CancelPopup(Button B, XEvent *E)
{
	toggle_state(B,NULL);
	RaisePopup(B,E);
}

void
RaisePopup(Button B, XEvent *E)
{
    if (PopupReadout->parent->active) 
        DeactivatePopupReadout();
    else 
        ActivatePopupReadout();
}

ActivatePopupReadout(void)
{
    XfActivateButton(PopupReadout->parent, ExposureMask);
}
DeactivatePopupReadout(void)
{
    XfDeactivateButton(PopupReadout->parent);
}

void
ActivateColorKey(Button B, XEvent *E)
{
	SetButtonState(PopupReadout->ReadoutToggle, 0);	
	SetButtonState(PopupReadout->ColorKeyToggle, 1);	
	XfActivateButton(PopupReadout->ColorKeyCover, ExposureMask);
	XfDeactivateButton(PopupReadout->ReadoutCover);
	XfActivateButton(PopupReadout->PlaneCover, ExposureMask);
}

void
UnActivateColorKey(Button B, XEvent *E)
{
	SetButtonState(PopupReadout->ReadoutToggle, 1);	
	SetButtonState(PopupReadout->ColorKeyToggle, 0);	

	XfDeactivateButton(PopupReadout->ColorKeyCover);
	XfActivateButton(PopupReadout->ReadoutCover, ExposureMask);
	XfActivateButton(PopupReadout->PlaneCover, ExposureMask);
}

CreatePopupReadout(Display *display, XFontStruct *font)
{
    Window w,parent;
    Button B;
    int x,y,width,height;
    struct popup_readout *new;
	int fg,bg;

	bg = BLACK(display);
	fg = WHITE(display);

    new = (struct popup_readout *)malloc(sizeof(struct popup_readout));
    PopupReadout = new;
    new->font = font;
    new->NDn = 0;
    new->DnSize = 0;

    parent = RootWindow(display, DefaultScreen(display));
    B = XfCreateButton(display, parent, 300,300,165,300,1,BLACK(display),"W",1);
    XfAddButtonVisual(B, 0, XfCreateVisual(B, 0,0,0,0,WHITE(display),
        WHITE(display),XfSolidVisual));
    new->parent = B;


	parent = new->parent->window;
	B = Make2State3D(display, parent,font,10,10,70,20,1,bg,bg,fg,"READOUT");
	XfAddButtonCallback(B, 0, UnActivateColorKey, NULL);
	XfAddButtonCallback(B, 1, ActivateColorKey, NULL);
    XfActivateButtonState(B, 1, ExposureMask | ButtonPressMask);
	new->ReadoutToggle = B;
	
	B = Make2State3D(display, parent,font,85,10,70,20,1,bg,bg,fg,"COLOR KEY");
	XfAddButtonCallback(B, 0, ActivateColorKey, NULL);
	XfAddButtonCallback(B, 1, UnActivateColorKey, NULL);
    XfActivateButtonState(B, 0, ExposureMask | ButtonPressMask);
	new->ColorKeyToggle = B;

	B = XfCreateButton(display, new->parent->window, 0,40,165,100,0,
			BLACK(display),"ColorKeyCover",1);
    XfAddButtonVisual(B,0,XfCreateVisual(B,0,7,0,0,
        WHITE(display),WHITE(display),XfSolidVisual));
	new->ColorKeyCover = B;

	B = XfCreateButton(display, new->parent->window, 0,40,165,100,0,
			BLACK(display),"ColorKeyCover",1);
    XfAddButtonVisual(B,0,XfCreateVisual(B,0,7,0,0,
        WHITE(display),WHITE(display),XfSolidVisual));
	XfActivateButton(B, ExposureMask);
	new->ReadoutCover = B;

	B = XfCreateButton(display, new->parent->window, 10,120,35,105,0,
			BLACK(display),"ColorKeyCover1",1);
    XfAddButtonVisual(B,0,XfCreateVisual(B,0,7,0,0,
        WHITE(display),WHITE(display),XfSolidVisual));
	XfActivateButton(B, ExposureMask);
	new->PlaneCover = B;

	y = 10;

	parent = new->ReadoutCover->window;

    B = XfCreateButton(display, parent, 50,y,40,10,0,
        BLACK(display),"line",1);
    XfAddButtonVisual(B,0,XfCreateVisual(B,0,0,0,0,
        BLACK(display),WHITE(display),XfTextVisual,"LINE",font,2));
    XfActivateButton(B, ExposureMask);

    B = XfCreateButton(display, parent, 100,y,40,10,0,
        BLACK(display),"sample",1);
    XfAddButtonVisual(B,0,XfCreateVisual(B,0,0,0,0,
        BLACK(display),WHITE(display),XfTextVisual,"SAMPLE",font,1));
    XfActivateButton(B, ExposureMask);

	y+=20;

    B = XfCreateButton(display, parent, 10,y,35,10,0,
        BLACK(display),"pixel",1);
    XfAddButtonVisual(B,0,XfCreateVisual(B,0,0,0,0,
        BLACK(display),WHITE(display),XfTextVisual,"PIXEL",font,1));
    XfActivateButton(B, ExposureMask);

	y+= 30;
    
    B = XfCreateButton(display, parent, 10,y,25,10,0,
        BLACK(display),"utm",1);
    XfAddButtonVisual(B,0,XfCreateVisual(B,0,0,0,0,
        BLACK(display),WHITE(display),XfTextVisual,"UTM",font,1));
    XfActivateButton(B, ExposureMask);

	y += 30;

    B = XfCreateButton(display, parent, 70,y,20,10,0,
        BLACK(display),"lines",1);
    XfAddButtonVisual(B,0,XfCreateVisual(B,0,0,0,0,
        BLACK(display),WHITE(display),XfTextVisual,"DN",font,1));
    XfActivateButton(B, ExposureMask);

    B = XfCreateButton(display, parent, 113,y,40,10,0,
        BLACK(display),"lines",1);
    XfAddButtonVisual(B,0,XfCreateVisual(B,0,0,0,0,
        BLACK(display),WHITE(display),XfTextVisual,"SCALED",font,1));
    XfActivateButton(B, ExposureMask);

	y -= 65;

    B = XfCreateButton(display, parent, 50,y,40,20,1,
        BLACK(display),"lines",1);
    XfAddButtonVisual(B,0,XfCreateVisual(B,0,7,0,0,
        BLACK(display),WHITE(display),XfTextVisual,"0",font,0));
    XfActivateButton(B, ExposureMask);
    new->Line = B;

    B = XfCreateButton(display, parent, 100,y,40,20,1,
        BLACK(display),"samples",1);
    XfAddButtonVisual(B,0,XfCreateVisual(B,0,7,0,0,
        BLACK(display),WHITE(display),XfTextVisual,"0",font,0));
    XfActivateButton(B, ExposureMask);
    new->Sample = B;

	y += 30;

    B = XfCreateButton(display, parent, 40,y,50,20,1,
        BLACK(display),"utmN",1);
    XfAddButtonVisual(B,0,XfCreateVisual(B,0,7,0,0,
        BLACK(display),WHITE(display),XfTextVisual,"0",font,0));
    XfActivateButton(B, ExposureMask);
    new->UTMN = B;

    B = XfCreateButton(display, parent, 100,y,50,20,1,
        BLACK(display),"utmE",1);
    XfAddButtonVisual(B,0,XfCreateVisual(B,0,7,0,0,
        BLACK(display),WHITE(display),XfTextVisual,"0",font,0));
    XfActivateButton(B, ExposureMask);
    new->UTME = B;

	/**
	 ** setup PlaneCover
	 **/

	parent = new->PlaneCover->window;
    B = XfCreateButton(display, parent, 0,10,30,10,0,
        BLACK(display),"lines",1);
    XfAddButtonVisual(B,0,XfCreateVisual(B,0,0,0,0,
        BLACK(display),WHITE(display),XfTextVisual,"PLANE",font,1));
    XfActivateButton(B, ExposureMask);

	/**
	 ** setup ColorKeyCover
	 **/

	parent = new->ColorKeyCover->window;
    MakeTextButton(display,parent,10,10,120,15,font,"TGIF Output Filename", 1);

    B = XfCreateButton(display, parent, 10,20,145,20,1,
        BLACK(display),"Color Key Filename",1);
    XfAddButtonVisual(B,0,XfCreateVisual(B,0,7,0,0,
        BLACK(display),pwBackground.pixel,XfTextVisual,"<NO FILE>",font,1));
	XfAddButtonCallback(B, 0, SetColorKeyFilename, NULL);
    XfActivateButton(B, ExposureMask | ButtonPressMask | KeyPressMask);

	B = Make2State3D(display, parent,font,10,50,45,20,1,bg,bg,fg,"WRITE");
	XfAddButtonCallback(B, 0, WriteColorKey, NULL);

	B = Make2State3D(display, parent,font,60,50,45,20,1,bg,bg,fg,"EXPAND");
	XfAddButtonCallback(B, 0, ExpandColorKey, NULL);

	B = Make2State3D(display, parent,font,110,50,45,20,1,bg,bg,fg,"DONE");
	XfAddButtonCallback(B, 0, CancelPopup, NULL);

}

void
ExpandColorKey(Button B, XEvent *E)
{
	return;
}

#define TGIF_HEADER "\
%TGIF 2.16-p5\n\
state(0,32,100,0,0,0,8,1,0,1,1,0,0,1,0,1,1,'Times-Roman',0,20,0,0,0,10,0,0,1,1,0,16,0,0,1,1,1,0,1088,1408).\n\
%\n\
% @(#)$Header$\n\
% %W%\n\
%\n\
page(1,\"\").\n"

#define TGIF_BOX(X,Y,COLOR) "\
box('%s',%d,%d,%d,%d,1,1,1,22,0,0,0,[\n\
]).\n", COLOR, X, Y, X+32, Y+32

#define TGIF_OUTLINE(X,Y) "\
box('black',%d,%d,%d,%d,0,1,1,26,0,0,0,[\n\
]).\n", X, Y, X+32, Y+32


#define TGIF_TEXT(X,Y,TEXT) "\
text('black',%d,%d,'Helvetica',0,20,1,0,0,1,78,22,25,0,18,4,0,0,0,0,[\n\
	\"%s\"]).\n", X+50, Y+8, TEXT


void
WriteColorKey(Button B, XEvent *E)
{
	/**
	 ** This function needs to scan the Dn list, getting colors.
	 ** and create a TGIF object file.
	 **/
	FILE *fp;
	char buf[256];
	Button b;
	XColor xc;
	int i;

	if (ColorKeyFilename == NULL || !strcmp(ColorKeyFilename, "<NO FILE>")) {
		XBell(B->display,50);
	} else {
		sprintf(buf, "%s.obj", ColorKeyFilename);
		if ((fp = fopen(buf,"w")) == NULL) {
			fprintf(stderr, "Cannot open file: %s\n", buf);
			return;
		}
		fprintf(fp, "%s", TGIF_HEADER);
		for (i = 0 ; i < PopupReadout->NDn ; i++) {
			b = PopupReadout->Dn[i][1];
			xc.pixel = b->States[0]->Visuals->foreground;
			XQueryColor(B->display, 
						ColorMap,
						&xc);
			sprintf(buf, "#%4.4x%4.4x%4.4x", xc.red, xc.green, xc.blue);
			fprintf(fp, TGIF_BOX(20,(i+1)*40, buf));
			if (xc.red > 0xF000 && xc.blue > 0xF000 && xc.green > 0xF000) {
				fprintf(fp, TGIF_OUTLINE(20,(i+1)*40));
			}
			fprintf(fp, TGIF_TEXT(20, (i+1)*40, ColorKeyNames[(int)b->ext]));
		}
		fclose(fp);
	}
	toggle_state(B,NULL);
}

void
ChangeColorKeyName(Button B, XEvent *E)
{
	int n;
	char buf[256];
	n = (int)B->ext;

    if (GetText(B, E, buf, 256, 0) == -1) 
        return;

	if (ColorKeyNames[n] != NULL) free(ColorKeyNames[n]);
	ColorKeyNames[n] = strdup(buf);
	SetButtonText(B,buf);
}

void
SetColorKeyFilename(Button B, XEvent *E)
{
	char buf[256];

    if (GetText(B, E, buf, 256, 0) == -1) 
        return;

	SetButtonText(B,buf);
	if (ColorKeyFilename) free(ColorKeyFilename);
	ColorKeyFilename = strdup(buf);
}

AddToDnList(int plane, XColor *color)
{
    int x,y,n;
    Button B;
    char buf[2];
    Display *display = PopupReadout->parent->display;
    char foo[32];

    buf[0] = plane+'A';
    buf[1] = '\0';

    n = PopupReadout->NDn;
    y = n*25;

    if (n < PopupReadout->DnSize) {
        SetButtonText(PopupReadout->Dn[n][0], buf);
        B = PopupReadout->Dn[n][1];
        B->States[0]->Visuals->foreground = color->pixel;
		UpdateButton(B);
        PopupReadout->Dn[n][3]->ext = (char *)plane;
        PopupReadout->Dn[n][1]->ext = (char *)plane;

		if (ColorKeyNames[plane] == NULL) {
			ColorKeyNames[plane] = strdup(trim_filename(Images[plane]->filename,20));
		}
		SetButtonText(PopupReadout->Names[n], ColorKeyNames[plane]);
    } else {
        sprintf(foo, "plane_%d",PopupReadout->NDn);
        B = XfCreateButton(display, PopupReadout->PlaneCover->window,
            0,y+25,11,20,1,BLACK(display),foo,1);
        XfAddButtonVisual(B, 0, XfCreateVisual(B, 0,7,0,0, BLACK(display),
            WHITE(display), XfTextVisual, buf, PopupReadout->font, 0));
        XfActivateButton(B, ExposureMask);
        PopupReadout->Dn[n][0] = B;
        B->ext = (char *)plane;

        B = XfCreateButton(display, PopupReadout->PlaneCover->window,
            15,y+25,11,20,1,BLACK(display),"color",1);
        XfAddButtonVisual(B, 0, XfCreateVisual(B, 0,0,0,0, color->pixel,
            color->pixel, XfSolidVisual));
        XfActivateButton(B, ExposureMask);
        PopupReadout->Dn[n][1] = B;
        B->ext = (char *)plane;

        B = XfCreateButton(display, PopupReadout->ReadoutCover->window,
            60,y+105,40,20,1,BLACK(display),"dn",1);
        XfAddButtonVisual(B, 0, XfCreateVisual(B, 0,7,0,0, BLACK(display),
            WHITE(display), XfTextVisual, "0", PopupReadout->font, 0));
        XfActivateButton(B, ExposureMask);
        PopupReadout->Dn[n][2] = B;
        B->ext = (char *)plane;

        B = XfCreateButton(display, PopupReadout->ReadoutCover->window,
            110,y+105,40,20,1,BLACK(display),"scaled",1);
        XfAddButtonVisual(B, 0, XfCreateVisual(B, 0,7,0,0, BLACK(display),
            WHITE(display), XfTextVisual, "0", PopupReadout->font, 0));
        XfAddButtonCallback(B, 0, ChangeImageDnScaleFactor, NULL);
        XfActivateButton(B, ExposureMask | ButtonPressMask | KeyPressMask);
        PopupReadout->Dn[n][3] = B;
        B->ext = (char *)plane;

        B = XfCreateButton(display, PopupReadout->ColorKeyCover->window,
            50,105+y,100,20,1,BLACK(display),"Names",1);
        XfAddButtonVisual(B, 0, XfCreateVisual(B, 0,7,0,0, BLACK(display),
            pwBackground.pixel, XfTextVisual, " ", PopupReadout->font, 1));
        XfAddButtonCallback(B, 0, ChangeColorKeyName, NULL);
        PopupReadout->Names[n] = B;
        B->ext = (char *)plane;

		if (ColorKeyNames[plane] == NULL) {
			ColorKeyNames[plane] = strdup(trim_filename(Images[plane]->filename,20));
		}
		SetButtonText(B, ColorKeyNames[plane]);
        XfActivateButton(B, ExposureMask | ButtonPressMask | KeyPressMask);

        PopupReadout->DnSize++;
    }
    PopupReadout->NDn++;
}

void
ChangeImageDnScaleFactor(Button B, XEvent *E)
{
    char str[16];
    int n=16;

    sprintf(str,"%d", Images[(int)B->ext]->scale);
    if (GetNewText(B,E,str,n,str) != -1) {
        Images[(int)B->ext]->scale = atoi(str); 
        RescalePopupReadout();
    }
}
UpdatePopupReadout(int x, int y)
{
    Image new;
    int scale,i,j,width;
    double v;
    float f;
    char buf[16];
    char fbuf[16];

    sprintf(buf,"%d",x+1);
    sprintf(fbuf,"%d",y+1);

    SetButtonText(PopupReadout->Sample, buf);
    SetButtonText(PopupReadout->Line, fbuf);

    for(i = 0; i < PopupReadout->NDn ; i ++) {
        j = (int)(PopupReadout->Dn[i][3])->ext;
        new = Images[j];
        if (new == NULL) continue;
        width = new->subset.width;
        scale = new->scale;
        v = get_data(new, y*width+x);
        f = (scale ? (float)v/(float)scale : v);
        sprintf(buf,"%.3g",v);
        sprintf(fbuf,"%f",f);
        buf[6] = fbuf[6] = '\0';
        SetButtonText(PopupReadout->Dn[i][2],buf);
        SetButtonText(PopupReadout->Dn[i][3],fbuf);
    }
}
RescalePopupReadout(void)
{
    char fbuf[16];
    int i,j,scale,v;
    float f;

    for(i = 0; i < PopupReadout->NDn ; i ++) {
        v = atoi(

        (PopupReadout->Dn[i][2])->States[0]->Visuals->visual.t_vis.text);

        scale = Images[(int)(PopupReadout->Dn[i][3]->ext)]->scale;
        f = (scale ? v/scale : v);
        sprintf(fbuf,"%f",f);
        fbuf[6] = '\0';
        SetButtonText(PopupReadout->Dn[i][3],fbuf);
    }
}
ClearDnList(void)
{
    PopupReadout->NDn = 0;
}
ActivateDnList(void)
{
    int i;
    for (i = PopupReadout->NDn ; i < PopupReadout->DnSize ; i++) {
        XfDestroyButton(PopupReadout->Dn[i][0]);
        XfDestroyButton(PopupReadout->Dn[i][1]);
        XfDestroyButton(PopupReadout->Dn[i][2]);
        XfDestroyButton(PopupReadout->Dn[i][3]);
        XfDestroyButton(PopupReadout->Names[i]);
    }
    ResizeButton(PopupReadout->parent, -1, -1,
                 PopupReadout->parent->width, 145 + PopupReadout->NDn * 30);
    ResizeButton(PopupReadout->ReadoutCover, -1, -1,
                 PopupReadout->ReadoutCover->width, 105 + PopupReadout->NDn * 30);
    ResizeButton(PopupReadout->ColorKeyCover, -1, -1,
                 PopupReadout->ColorKeyCover->width, 105 + PopupReadout->NDn * 30);
    ResizeButton(PopupReadout->PlaneCover, -1, -1,
                 PopupReadout->PlaneCover->width, 20 + PopupReadout->NDn * 30);
    PopupReadout->DnSize = PopupReadout->NDn;
}
