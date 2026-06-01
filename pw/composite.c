/*
 * composite.c
 *
 * Definitions of the routines to manipulate Image Requestor Boxes
 *
 * $Header$
 *
 * $Log$
 * Revision 1.6  2002/05/07 02:47:00  gorelick
 * Version 5.23 changes
 *
 * Revision 1.5  2002/03/12 01:24:56  gorelick
 * Added -all flag
 *
 * Revision 1.4  2001/02/23 01:10:33  gorelick
 * Modifications to use a private colormap if we can't allocate colors
 *
 * Revision 1.3  2001/02/13 17:52:29  asbms
 * io_pw.c is a replacement for io_load.c.  io_pw.c is now an interface between
 * pw and the iomedley library.  No more file-type I/O needs to be done for
 * PW.  Any changes/additions/etc are done in iomedley and will be transperent
 * to PW.  The iomedley library's integration with PW caused some symbol conflicts
 * in name space.  Thus, color names (Red, Blue, Yellow, etc) had "pw" added
 * as a prefix.  This affected several files.
 *
 * Revision 1.2  1999/11/04 22:05:31  asbms
 * Bug updates
 *
 * Revision 1.1.1.1  1999/09/09 17:50:35  gorelick
 * Initial import
 *
 * Revision 1.2  91/09/30  17:22:14  17:22:14  ngorelic (Noel S. Gorelick)
 * *** empty log message ***
 * 
 * Revision 1.1  91/09/23  17:49:20  17:49:20  ngorelic (Noel S. Gorelick)
 * Initial revision
 * 
 *
 */

#include <fcntl.h>
#include <X11/keysym.h>
#include <math.h>
#include "composite.h"
#include <stdio.h>
#include <X11/Xos.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "Xfred.h"
#include "image.h"
#include "vicar.h"
#include "composite_fixes.h" /* Added function prototypes */

/* Function prototypes */
int is_file(char *filename);

void RGB_pushbutton(Button B, XEvent *E);
void overlay_pushbutton(Button B, XEvent *E);
void load_filename(Button B, XEvent *E);
void load_band(Button B, XEvent *E);
void load_ncolors(Button B, XEvent *E);
void toggle_mem(Button B, XEvent *E);
void LoadImage(Button B, XEvent *E);
Image allocate_image(void);
char *trim_filename(char *s, int n);

void GetInput_Samples(Button B, XEvent *E);
void GetInput_Lines(Button B, XEvent *E);
void GetInput_Bands(Button B, XEvent *E);
void GetInput_Format(Button B, XEvent *E);
void GetInput_Org(Button B, XEvent *E);
void GetInput_Label(Button B, XEvent *E);
void GetInput_Order(Button B, XEvent *E);

void GetInput_SubsetLine(Button B, XEvent *E);
void GetInput_SubsetSample(Button B, XEvent *E);
void GetInput_SubsetWidth(Button B, XEvent *E);
void GetInput_SubsetHeight(Button B, XEvent *E);
void GetInput_SubsetLSkip(Button B, XEvent *E);
void GetInput_SubsetSSkip(Button B, XEvent *E);

void ActivateRestartRequestor(Button B, XEvent *E);
void SaveRestart(Button B, XEvent *E);
void ReadRestart(Button B, XEvent *E);

void DeleteImage(Button B, XEvent *E);

extern XColor pwRed,pwGreen,pwBlue,pwYellow;
extern XColor Colors[];
extern int NColors;
extern int IWidth,IHeight;
extern int AllocError;
extern Colormap ColorMap ;

Button UserMsg;

/*
 * Create a Requestor, and all it's associated toys.
 */

int UpdatePushButtons (Requestor R, int n);
int ButtonHilite (Button B, unsigned int fg, unsigned int bg);
int load_info (Image new, Requestor R);
int free_image (Display *display, Image *new);
int UnFlagForLoad (Requestor R, int i);
int FlagForLoad (Requestor R, int i);
int set_no_header (Image new);
int load_fast_mem (Display *display, Image new);
extern int stretch_color (Image new);
extern int stretch_gray (Image new);
int create_hist (Display *display, Image new, int size, int type, int scale);
int create_pan (Display *display, Image new);
int update_display (Display *display, Image new);
int SetUserMsg (char *buf);
int make_hist_dist (Image new, int *C, int size, int scale_in);
int make_hist_freq (Image new, int *C, int size, int scale_in);
extern int quantize (char *data, int ncolors, int height, int width, struct quant_data **quant, int base);
int default_map (Image new);
int compute_subset (Requestor R, int i);

Requestor CreateRequestor(Display *display, Window parent, int x, int y, int border_width, long unsigned int border_color, char *name, XFontStruct *font)
{
    int i,mask;
    char newname[256];
    struct VisualInfo* vis;
    struct VisualInfo* button_vis_1;
    struct VisualInfo* button_vis_2;
    struct VisualInfo* button_vis_3;
    struct VisualInfo* button_vis_4;
    struct VisualInfo* button_vis_5;
    struct VisualInfo* button_vis_6;
    int new_y;
    Requestor new;
    Button B;
    Button bg;
    int width,height;

    new = (Requestor)malloc(sizeof(struct _Requestor));
    if (new == NULL)
        return(NULL);

    new->display = display;
    new->parent = parent;
    new->x = x;
    new->y = y;
    new->width = 350;
    new->height = 570;
    new->border_width = border_width;
    new->border_color = border_color;
    strcpy(new->name, name);

    new->window = XCreateSimpleWindow(display, parent, x, y, new->width,
        new->height, 0, border_color,
        WHITE(display));
	XSetWindowColormap(display, new->window, ColorMap);
/*
 * Now create the tools needed
 */
    B = XfCreateButton(display, new->window, 15, 15, 15, 390, border_width,
        border_color, "", 1);
    B->member = (int *)new;
    XfActivateButton(B, ExposureMask);
    new->letters = B;

    sprintf(newname, "%s fnames", name);
    B = XfCreateButton(display, new->window, 40, 15, 220, 390, border_width,
        border_color, newname, 1);
    B->member = (int *)new;
    XfActivateButton(B, ExposureMask);
    new->fnames = B;

    sprintf(newname, "%s bands", name);
    B = XfCreateButton(display, new->window, 265, 15, 25, 390, border_width,
        border_color, newname, 1);
    B->member = (int *)new;
    XfAddButtonCallback(B, 0, load_band, NULL);
    XfActivateButton(B, ButtonPressMask | ExposureMask);
    new->bands = B;

    new_y = 0;
    for (i = 0; i < NIMAGE; i++)
    {
        sprintf(newname, "%s RGB %d", name, i);
        B = XfCreateButton(display, new->window, 300, new_y+15, 15, 15,
            border_width, border_color, newname, 5);
        B->member = (int *)new;
        if (i == 0)
        {
            button_vis_1 = XfCreateVisual(B, 0, 0, 0, 0,
                WHITE(display),
                WHITE(display),
                XfSolidVisual);
            button_vis_2 = XfCreateVisual(B, 0, 0, 0, 0,
                pwRed.pixel,
                WHITE(display),
                XfSolidVisual);
            button_vis_3 = XfCreateVisual(B, 0, 0, 0, 0,
                pwGreen.pixel,
                WHITE(display),
                XfSolidVisual);
            button_vis_4 = XfCreateVisual(B, 0, 0, 0, 0,
                pwBlue.pixel,
                WHITE(display),
                XfSolidVisual);
            button_vis_5 = XfCreateVisual(B, 0, 0, 0, 0,
                pwYellow.pixel,
                WHITE(display),
                XfSolidVisual);
            button_vis_6 = XfCreateVisual(B, 0, 0, 0, 0,
                pwBackground.pixel,
                WHITE(display),
                XfSolidVisual);
        }
        B->ext = (char *)i;
        XfAddButtonVisual(B, 0, button_vis_1);
        XfAddButtonVisual(B, 1, button_vis_2);
        XfAddButtonVisual(B, 2, button_vis_3);
        XfAddButtonVisual(B, 3, button_vis_4);
        XfAddButtonVisual(B, 4, button_vis_6);
        XfAddButtonCallback(B, 0, RGB_pushbutton, NULL);
        XfAddButtonCallback(B, 1, RGB_pushbutton, NULL);
        XfAddButtonCallback(B, 2, RGB_pushbutton, NULL);
        XfAddButtonCallback(B, 3, RGB_pushbutton, NULL);
        XfActivateButton(B, (ExposureMask | ButtonPressMask));
        new->rgbs[i] = B;

        sprintf(newname, "%s COMP %d", name, i);
        B = XfCreateButton(display, new->window, 315, new_y+15, 15, 15,
            border_width, border_color, newname, 3);
        if (B == NULL)
            return(NULL);
        B->member = (int *)new;
        B->ext = (char *)i;
        XfAddButtonVisual(B, 0, button_vis_1);
        XfAddButtonVisual(B, 1, button_vis_5);
        XfAddButtonVisual(B, 2, button_vis_6);
        XfAddButtonCallback(B, 0, overlay_pushbutton, NULL);
        XfAddButtonCallback(B, 1, overlay_pushbutton, NULL);
        XfActivateButton(B, (ExposureMask | ButtonPressMask));
        new->comps[i] = B;

        sprintf(newname, "%c", ('A' + (char)i));
        B = XfCreateButton(display, new->letters->window, 0, 15*i, 15, 15, 0,
            WHITE(display), newname, 3);
        new->letter[i] = B;

        vis = XfCreateVisual(new->letter[i], 0, 3, 15, 15,
            BLACK(display), WHITE(display), XfTextVisual, newname, font, 0);
        XfAddButtonVisual(new->letter[i], 0, vis);
        vis = XfCreateVisual(new->letter[i], 0, 3, 15, 15,
            BLACK(display), pwHilite.pixel, XfTextVisual, newname, font, 0);
        XfAddButtonVisual(new->letter[i], 1, vis);
        vis = XfCreateVisual(new->letter[i], 0, 3, 15, 15,
            BLACK(display), pwBackground.pixel, XfTextVisual, newname, font, 0);
        XfAddButtonVisual(new->letter[i], 2, vis);

        XfAddButtonCallback(new->letter[i], 1, LoadImage, NULL);
        new->letter[i]->member = (int *)new;
        new->letter[i]->ext = (char *)i;
        XfActivateButton(B, ExposureMask | ButtonPressMask);

        if ((Images[i] == NULL))
            sprintf(newname, "--");
        else if (Images[i]->composite != 0)
            sprintf(newname, "%d", Images[i]->band);
        else
            sprintf(newname, "%d", Images[i]->band + 1);
        vis = XfCreateVisual(new->bands, 0, (new_y + 3), 25, 15,
            BLACK(display), WHITE(display), XfTextVisual,
            newname, font, 0);
        XfAddButtonVisual(new->bands, 0, vis);
        new->bnd[i] = vis;

        sprintf(newname, "%s FNAME %d", name, i);
        B = XfCreateButton(display, new->fnames->window, 0, new_y, 220, 15,
            0, border_color, newname, 3);
        if (B == NULL)
            return(NULL);
        B->member = (int *)new;
        B->ext = (char *)i;

        if (Images[i] == NULL)
            strcpy(newname, "<NA>");
        else if (Images[i]->composite > 0)
            strcpy(newname, "<composite>");
        else {
            strcpy(newname, trim_filename(Images[i]->filename,27));
			if (Images[i]->is_stretched) {
				strcat(newname, " <S>");
			}
		}
        vis = XfCreateVisual(B, 0, 3, 0, 0,
            BLACK(display), WHITE(display), XfTextVisual,
            newname, font, 1);
        XfAddButtonVisual(B, 0, vis);
        XfAddButtonCallback(B, 0, load_filename, NULL);
        XfActivateButton(B, ExposureMask | ButtonPressMask | KeyPressMask);
        new->fname[i] = B;
        new_y += 15;
    }
/* 
 *
 *  Here on out, its the info buttons 
 *
 */
    x = 15;
    y = 410;
    width = 315;
    height = 15;


    B = XfCreateButton(display, new->window, x, y, width, height, 
        1, BLACK(display), "UserMsg", 1);
    XfAddButtonVisual(B, 0, XfCreateVisual(B, 0, 3, 0, 0, 
        BLACK(display),WHITE(display), 
        XfTextVisual, "", font, 1));
    XfActivateButton(B, ExposureMask);
    new->UserMsg = B;
    UserMsg = B;
    B->member = (int *)new;

/*
 * Action buttons 
 */
    x = 15;
    y += height+10;
    width = 35;
    height = 15;

    B = XfCreateButton(display, new->window, x, y, width, height, 
        1, BLACK(display), "DeleteImage", 1);
    XfAddButtonVisual(B, 0, XfCreateVisual(B, 0, 3, 0, 0, 
        BLACK(display),WHITE(display), 
        XfTextVisual, "DEL", font, 0));
    XfAddButtonCallback(B, 0, DeleteImage, NULL);
    XfActivateButton(B, ExposureMask | ButtonPressMask);
    new->DeleteImage = B;
    B->member = (int *)new;

    x += width + 5;
    B = XfCreateButton(display, new->window, x, y, width*2+5, height, 
        1, BLACK(display), "ReadRestart", 1);
    XfAddButtonVisual(B, 0, XfCreateVisual(B, 0, 3, 0, 0, 
        BLACK(display),WHITE(display), 
        XfTextVisual, "RESTART", font, 0));
    XfAddButtonCallback(B, 0, ActivateRestartRequestor, NULL);
    XfActivateButton(B, ExposureMask | ButtonPressMask);
    new->RestartRead = B;
    B->member = (int *)new;
    
/*
 * throw out some backgrounds for Header info
 */

    B = XfCreateButton(display, new->window, 145, y, 185, 70, 1, 
            BLACK(display), "bg", 1);
    XfAddButtonVisual(B, 0, XfCreateVisual(B, 0,0,0,0,
            pwBackground.pixel,pwBackground.pixel, XfSolidVisual));
    XfActivateButton(B, ExposureMask);
    bg = B;

/* Now header info */

    x = 50;
    y = 15;

    width = 40;
    height = 15;


    MakeTextButtonBg(display,bg->window,x-width-5,y,width,height, 
        pwBackground.pixel,font,"Header", 0);

    MakeTextButtonBg(display,bg->window,x,y-10,width,height, 
        pwBackground.pixel,font,"Format", 0);
    new->HeaderFormat = XfCreateButton(display, bg->window, x,y,width,height,
        1, BLACK(display), "Format",1);
    x += width + 5;
    MakeTextButtonBg(display,bg->window,x,y-10,width+4,height,
        pwBackground.pixel, font,"Org", 0);
    new->HeaderOrg = XfCreateButton(display, bg->window, x,y,width,height,
        1, BLACK(display), "Org",1);
    x += width + 5;
    MakeTextButtonBg(display,bg->window,x,y-10,width,height,
        pwBackground.pixel, font,"Label", 0);
    new->HeaderLabel = XfCreateButton(display, bg->window, x,y,width,height,
        1, BLACK(display), "Label",1);

    x = 50 - width - 5;
    y += height*2+5;

    MakeTextButtonBg(display,bg->window,x,y-10,width,height,
        pwBackground.pixel, font,"Lines", 0);
    new->HeaderLines = XfCreateButton(display, bg->window, x,y,width,height,
        1, BLACK(display), "Lines",1);
    x += width + 5;
    MakeTextButtonBg(display,bg->window,x,y-10,width,height,
        pwBackground.pixel, font,"Samples", 0);
    new->HeaderSamples = XfCreateButton(display, bg->window, x,y,width,height,
        1, BLACK(display), "Samples",1);
    x += width + 5;
    MakeTextButtonBg(display,bg->window,x,y-10,width,height,
        pwBackground.pixel, font,"Bands", 0);
    new->HeaderBands = XfCreateButton(display, bg->window, x,y,width,height,
        1, BLACK(display), "Bands",1);
    x += width + 5;
    MakeTextButtonBg(display,bg->window,x,y-10,width,height,
        pwBackground.pixel, font,"Order", 0);
    new->HeaderOrder = XfCreateButton(display, bg->window, x,y,width,height,
        1, BLACK(display), "Order",1);

    AddTextVisual(new->HeaderSamples,0,
        BLACK(display),WHITE(display),"--",font,0);
    AddTextVisual(new->HeaderLines, 0,
        BLACK(display),WHITE(display),"--",font,0);
    AddTextVisual(new->HeaderBands, 0,
        BLACK(display),WHITE(display),"--",font,0);
    AddTextVisual(new->HeaderFormat,0,
        BLACK(display),WHITE(display),"--",font,0);
    AddTextVisual(new->HeaderOrg,   0,
        BLACK(display),WHITE(display),"--",font,0);
    AddTextVisual(new->HeaderLabel, 0,
        BLACK(display),WHITE(display),"--",font,0);
    AddTextVisual(new->HeaderOrder, 0,
        BLACK(display),WHITE(display),"--",font,0);

    XfAddButtonCallback(new->HeaderSamples, 0, GetInput_Samples, NULL);
    XfAddButtonCallback(new->HeaderLines, 0, GetInput_Lines, NULL);
    XfAddButtonCallback(new->HeaderBands, 0, GetInput_Bands, NULL);
    XfAddButtonCallback(new->HeaderFormat, 0, GetInput_Format, NULL);
    XfAddButtonCallback(new->HeaderOrg, 0, GetInput_Org, NULL);
    XfAddButtonCallback(new->HeaderLabel, 0, GetInput_Label, NULL);
    XfAddButtonCallback(new->HeaderOrder, 0, GetInput_Order, NULL);

    mask = ExposureMask | ButtonPressMask | KeyPressMask;
    XfActivateButton(new->HeaderSamples, mask);
    XfActivateButton(new->HeaderLines,  mask);
    XfActivateButton(new->HeaderBands,  mask);
    XfActivateButton(new->HeaderFormat, mask);
    XfActivateButton(new->HeaderOrg,    mask);
    XfActivateButton(new->HeaderLabel,  mask);
    XfActivateButton(new->HeaderOrder,  mask);

    new->HeaderSamples->member = (int *)new;
    new->HeaderLines->member   = (int *)new;
    new->HeaderBands->member   = (int *)new;
    new->HeaderFormat->member  = (int *)new;
    new->HeaderOrg->member     = (int *)new;
    new->HeaderLabel->member   = (int *)new;
    new->HeaderOrder->member   = (int *)new;

/*
 * Subset background
 */

    
    B = XfCreateButton(display, new->window, 15, 510, 315, 45, 1, 
            BLACK(display), "bg", 1);
    XfAddButtonVisual(B, 0, XfCreateVisual(B, 0,0,0,0,
            pwBackground.pixel,pwBackground.pixel, XfSolidVisual));
    XfActivateButton(B, ExposureMask);
    bg = B;

    y = 20;
    x = 0;

    MakeTextButtonBg(display,bg->window,x,y+3,width,height, 
        pwBackground.pixel,font,"Subset", 0);
    x += width+3;
    MakeTextButtonBg(display,bg->window,x,y-10,width,height, 
        pwBackground.pixel,font,"Start L", 0);
    new->SubsetLine = XfCreateButton(display, bg->window, x,y,width,height,
        1, BLACK(display), "StartLine",1);
    x += width+5;
    MakeTextButtonBg(display,bg->window,x,y-10,width+3,height, 
        pwBackground.pixel,font,"Start S", 0);
    new->SubsetSample = XfCreateButton(display, bg->window, x,y,width,height,
        1, BLACK(display), "StartSample",1);
    x += width+5;
    MakeTextButtonBg(display,bg->window,x,y-10,width,height, 
        pwBackground.pixel,font,"Height", 0);
    new->SubsetHeight = XfCreateButton(display, bg->window, x,y,width,height,
        1, BLACK(display), "Height",1);
    x += width+5;
    MakeTextButtonBg(display,bg->window,x,y-10,width,height, 
        pwBackground.pixel,font,"Width", 0);
    new->SubsetWidth = XfCreateButton(display, bg->window, x,y,width,height,
        1, BLACK(display), "Width",1);
    x += width+5;
    MakeTextButtonBg(display,bg->window,x,y-10,width,height, 
        pwBackground.pixel,font,"L Skip", 0);
    new->SubsetLSkip = XfCreateButton(display, bg->window, x,y,width,height,
        1, BLACK(display), "LineSkip",1);
    x += width+5;
    MakeTextButtonBg(display,bg->window,x,y-10,width,height, 
        pwBackground.pixel,font,"S Skip", 0);
    new->SubsetSSkip = XfCreateButton(display, bg->window, x,y,width,height,
        1, BLACK(display), "SampleSkip",1);


    AddTextVisual(new->SubsetLine,0,
        BLACK(display),WHITE(display),"--",font,0);
    AddTextVisual(new->SubsetSample,0,
        BLACK(display),WHITE(display),"--",font,0);
    AddTextVisual(new->SubsetWidth,0,
        BLACK(display),WHITE(display),"--",font,0);
    AddTextVisual(new->SubsetHeight,0,
        BLACK(display),WHITE(display),"--",font,0);
    AddTextVisual(new->SubsetLSkip,0,
        BLACK(display),WHITE(display),"--",font,0);
    AddTextVisual(new->SubsetSSkip,0,
        BLACK(display),WHITE(display),"--",font,0);

    XfAddButtonCallback(new->SubsetLine, 0, GetInput_SubsetLine, NULL);
    XfAddButtonCallback(new->SubsetSample, 0, GetInput_SubsetSample, NULL);
    XfAddButtonCallback(new->SubsetWidth, 0, GetInput_SubsetWidth, NULL);
    XfAddButtonCallback(new->SubsetHeight, 0, GetInput_SubsetHeight, NULL);
    XfAddButtonCallback(new->SubsetLSkip, 0, GetInput_SubsetLSkip, NULL);
    XfAddButtonCallback(new->SubsetSSkip, 0, GetInput_SubsetSSkip, NULL);

    mask = ExposureMask | ButtonPressMask | KeyPressMask;
    XfActivateButton(new->SubsetLine, mask);
    XfActivateButton(new->SubsetSample, mask);
    XfActivateButton(new->SubsetWidth, mask);
    XfActivateButton(new->SubsetHeight, mask);
    XfActivateButton(new->SubsetLSkip, mask);
    XfActivateButton(new->SubsetSSkip, mask);

    new->SubsetLine->member = (int *)new;
    new->SubsetSample->member = (int *)new;
    new->SubsetWidth->member = (int *)new;
    new->SubsetHeight->member = (int *)new;
    new->SubsetLSkip->member = (int *)new;
    new->SubsetSSkip->member = (int *)new;

/* Memory buttons */

    x = 85;
    y = 470;
    width = 15;
    height = 15;

    MakeTextButton(display,new->window,x,y-10,45,height,font,"Memory", 0);
    B = XfCreateButton(display, new->window, x, y, width, height, 
        1, BLACK(display), "SlowMem", 2);
    XfAddButtonVisual(B, 0, XfCreateVisual(B, 0, 3, 0, 0, 
        BLACK(display),WHITE(display), 
        XfTextVisual, "S", font, 0));
    XfAddButtonVisual(B, 1, XfCreateVisual(B, 0, 3, 0, 0, 
        BLACK(display),pwBackground.pixel,
        XfTextVisual, "S", font, 0));
    XfAddButtonCallback(B, 0, toggle_mem, NULL);
    XfAddButtonCallback(B, 1, toggle_mem, NULL);
    XfActivateButton(B, ButtonPressMask | ExposureMask);
    new->SlowMem = B;
    B->member = (int *)new;
    B->ext = 0;

    B = XfCreateButton(display, new->window, x+width, y, width, height, 
        1, BLACK(display), "MediumMem", 2);
    XfAddButtonVisual(B, 0, XfCreateVisual(B, 0, 3, 0, 0, 
        BLACK(display),WHITE(display), 
        XfTextVisual, "M", font, 0));
    XfAddButtonVisual(B, 1, XfCreateVisual(B, 0, 3, 0, 0, 
        BLACK(display),pwBackground.pixel,
        XfTextVisual, "M", font, 0));
    XfAddButtonCallback(B, 0, toggle_mem, NULL);
    XfAddButtonCallback(B, 1, toggle_mem, NULL);
    XfActivateButton(B, ButtonPressMask | ExposureMask);
    new->MediumMem = B;
    B->member = (int *)new;
    B->ext = (char *)1;

    B = XfCreateButton(display, new->window, x+width*2, y, width, height, 
        1, BLACK(display), "FastMem", 2);
    XfAddButtonVisual(B, 0, XfCreateVisual(B, 0, 3, 0, 0, 
        BLACK(display),WHITE(display), 
        XfTextVisual, "F", font, 0));
    XfAddButtonVisual(B, 1, XfCreateVisual(B, 0, 3, 0, 0, 
        BLACK(display),pwBackground.pixel,
        XfTextVisual, "F", font, 0));
    XfAddButtonCallback(B, 0, toggle_mem, NULL);
    XfAddButtonCallback(B, 1, toggle_mem, NULL);
    XfActivateButton(B, ButtonPressMask | ExposureMask);
    new->FastMem = B;
    B->member = (int *)new;
    B->ext = (char *)2;

/* Number of colors */

    x = 45;
    y = 470;
    width = 35;
    height = 15;

    MakeTextButton(display,new->window,x-30,y+3,width,height,font,"Used", 1);
    MakeTextButton(display,new->window,x,y-10,width,height,font,"Colors", 0);
    B = XfCreateButton(display, new->window, x, y, width, height, 
        1, BLACK(display), "UsedColors", 1);
    XfAddButtonVisual(B, 0, XfCreateVisual(B, 0, 3, 0, 0, 
        BLACK(display),WHITE(display), 
        XfTextVisual, "0", font, 0));
    XfAddButtonCallback(B, 0, load_ncolors, NULL);
    XfActivateButton(B, ButtonPressMask | ExposureMask | KeyPressMask);
    new->UsedColors = B;
    B->member = (int *)new;

    y += height+3;

    MakeTextButton(display,new->window,x-30,y+3,width,height,font,"Avail", 1);
    B = XfCreateButton(display, new->window, x, y, width, height, 
        0, BLACK(display), "AvailColors", 1);
    XfAddButtonVisual(B, 0, XfCreateVisual(B, 0, 3, 0, 0, 
        BLACK(display),WHITE(display), 
        XfTextVisual, "0", font, 0));
    XfActivateButton(B, ExposureMask);
    new->AvailColors = B;
    B->member = (int *)new;

    UpdatePushButtons(new,0);
    ButtonHilite(new->fname[0], BLACK(display), pwBackground.pixel);
    load_info(Images[0], new);

    return(new);
}

/*
 * "Activate" the Requestor by mapping it
 */
int ActivateRequestor(Requestor C)
{
    if (C == NULL)
        return(False);

    XMapRaised(C->display, C->window);
    return(True);
}

int DeactivateRequestor(Requestor C)
{
    if (C == NULL)
        return(False);

    XUnmapWindow(C->display, C->window);
    return(True);
}

/*
 * Update the visuals for the fnames button
 */
int UpdateRequestor(Requestor R, int n)
{
    int i;
    char newt[256];

    if (R == NULL)
        return(False);

    for (i = 0; i < NIMAGE; i++)
    {
        if (Images[i] == NULL)
            strcpy(newt, "<NA>");
        else if (Images[i]->composite > 0)
            strcpy(newt, "<composite>");
        else {
            strcpy(newt,trim_filename(Images[i]->filename,27));
			if (Images[i]->is_stretched) {
				strcat(newt, " <S>");
			}
		}
        strcpy(((R->fname[i])->States[0])->Visuals->visual.t_vis.text,newt);

        if (i != n) {
            ((R->fname[i])->States[0])->Visuals->background = 
                WHITE(R->fname[i]->display);
        } else {
            ((R->fname[i])->States[0])->Visuals->background = 
                pwBackground.pixel;
        }

        if (Images[i] == NULL)
            strcpy(newt, "--");
        else if (Images[i]->composite > 0)
            sprintf(newt, "%d", Images[i]->band);
        else
            sprintf(newt, "%d", Images[i]->band + 1);

        strcpy((R->bnd[i])->visual.t_vis.text, newt);

        UpdateButton(R->fname[i]);
    }
    UpdateButton(R->bands);
    return(True);
}

int
UpdatePushButtons(Requestor R, int n)
{
    int i;
    if (R == NULL)
        return(False);
    for (i = 0 ; i < NIMAGE ; i++) {
        if (Images[n] == NULL || Images[n]->rgb[i] == 0)
            R->rgbs[i]->state = 0;
        else
            R->rgbs[i]->state = Images[n]->rgb[i];

        if (Images[n] == NULL || Images[n]->overlays[i]== 0)
            R->comps[i]->state = 0;
        else
            R->comps[i]->state = 1;

        if (Images[i] == NULL || Images[i]->composite || i == n) {
            R->rgbs[i]->state = 4;
            R->comps[i]->state = 2;
        }
        UpdateButton(R->rgbs[i]);
        UpdateButton(R->comps[i]);
    }
    return(True);
}


/*
 * The callback for the user-interactive buttons
 */
void RGB_pushbutton(Button B, XEvent *E)
{
    Requestor R;
    Button BB;
    int i,j,k;
    int ncolors;

    if (B == NULL)
        return;

    R = (Requestor)B->member;
    i = (int)B->ext;
    j = (int)R->fnames->ext;

    if (Images[i] == NULL || (Images[i])->composite == 1) {
        XBell(B->display, 50);
        return;
    }

    if (Images[j] != NULL && Images[j]->composite == 0) {
        XBell(B->display,50);
        return;
    }

    if (Images[j] == NULL) {
        Images[j] = allocate_image();
        Images[j]->composite = 1;
        Images[j]->band = 4;
        Images[j]->ncolors = NColors;
        UpdateRequestor(R,j);
    }

    B->state = (B->state+1) % (B->maxstate-1);
    {
        int flags[4];
        int count=0;
        flags[1] = flags[2] = flags[3] = 0;
        for (k = 0 ; k < NIMAGE ; k++) {
            if (Images[j]->rgb[k] == 1) flags[1]++;
            if (Images[j]->rgb[k] == 2) flags[2]++;
            if (Images[j]->rgb[k] == 3) flags[3]++;
        }
        while(B->state && flags[B->state])
            B->state = (B->state +1) % (B->maxstate -1);
    }

    BB = R->comps[(int)B->ext];
    Images[j]->overlays[i] = 0;
    BB->state = 0;

    Images[j]->rgb[(int)B->ext] = B->state;

    UpdateButton(B);
    UpdateButton(BB);

    for (k=0 ; k < NIMAGE ; k++) {
        if (Images[j]->rgb[k] > 0) break;
    }
    if (k == NIMAGE) {
        free_image(B->display,&Images[j]);
        UnFlagForLoad(R,j);
        UpdateRequestor(R,j);
    } else {
        FlagForLoad(R,j);
    }
    load_info(Images[j],R);
}

void overlay_pushbutton(Button B, XEvent *E)
{
    Requestor R;
    Button BB;
    int i,j,k;
    int ncolors;

    if (B == NULL)
        return;

    R = (Requestor)B->member;
    i = (int)B->ext;
    j = (int)R->fnames->ext;

    if (Images[j] == NULL || 
        Images[i] == NULL || 
        (Images[i])->composite == 1) {
        XBell(B->display, 50);
        return;
    }

    BB = R->rgbs[i];
    Images[j]->rgb[i] = 0;
    BB->state = 0;

    B->state = 1-B->state;
    Images[j]->overlays[(int)B->ext] = B->state;

    ncolors = Images[j]->ncolors;
    for (i = 0 ; i < NIMAGE ; i++) {
        ncolors += (Images[j]->overlays[i] ? Images[i]->ncolors : 0);
    }
    if (ncolors > NColors) {
        B->state = 0;
        Images[j]->overlays[(int)B->ext] = B->state;
    }

    UpdateButton(B);
    UpdateButton(BB);

    UpdateRequestor(R,j);
}

void load_filename(Button B, XEvent *E)
{
    Requestor R;
    int i;
    Display *display;
    XFontStruct *font;
    char *s,buf[256],*p;
    int fp;
    struct vicar_header *header;

    R = (Requestor)B->member;
    i = (int)R->fnames->ext;
    display = B->display;
    font = (B->States[0])->Visuals->visual.t_vis.font;

    p = (B->States[0])->Visuals->visual.t_vis.text;
    if (p != NULL && *p == '<') p = NULL;

    if ((int)B->ext == i) {
/* 
 *
 *   Get new filename.  Remove any old data.  Load new header 
 *
 */
        s = xgets(display, B->window, 0, 0, B->width, B->height, 
            WHITE(display), BLACK(display), font, p, E);

        if (s != NULL && s[0] != 0) {
            if (p != NULL && !strcmp(p,s)) return;
            strcpy(buf,s);

/*
 *  Check existance
 */
            if (!is_file(buf)) {
                fprintf(stderr,"%s: not a file\n",buf);
                return;
            }
/*
 *  Load header & buttons
 */
/*
 * Clear out old data
 */
            if (Images[i] != NULL) {
                free_image(B->display,&Images[i]);
            }
            Images[i] = allocate_image();
            (Images[i])->filename = strdup(buf);

            if ((header = get_image_header(Images[i])) == NULL) {
                set_no_header(Images[i]);
                UpdateRequestor(R,i);
                UpdatePushButtons(R,i);
                FlagForLoad(R,i);
                return;
            }
            memcpy(&(Images[i]->header),header, sizeof(struct vicar_header));
            Images[i]->subset.width = Images[i]->header.samples;
            Images[i]->subset.height = Images[i]->header.lines;
/*
 *  If single band image, load data.
 */
            if (header->bands == 1) {
                Images[i]->band = 0;
            } else {
                Images[i]->band = -1;
            }
            free(header);

            UpdateRequestor(R,i);
            UpdatePushButtons(R,i);
            FlagForLoad(R,i);
        }
    } else {
        ButtonHilite(R->fname[i],BLACK(display),WHITE(display));
        i =  (int)B->ext;
        R->fnames->ext = B->ext;
        ButtonHilite(R->fname[i], BLACK(display), pwBackground.pixel);
        UpdatePushButtons(R,i);
    }
    load_info(Images[(int)R->fnames->ext], R);
}

int
off_load_filename(int i, char *buf, Requestor R, int stretch, int colors, int band)
{
    struct vicar_header *header;

    int fp;
    fp = open(buf, O_RDONLY);
    if (fp < 0) {
        printf("file not found\n");
        XBell(R->display, 50);
        return 0;
    }
    close(fp);

    Images[i] = allocate_image();

	if (colors != 0) {
		Images[i]->ncolors = colors;
	}

	if (stretch != 0 && stretch != 255)  {
		Images[i]->c_low = 0;
		Images[i]->c_high = stretch;
	}

    (Images[i])->filename = (char *)malloc((unsigned int)(strlen(buf) + 1));
    strcpy((Images[i])->filename, buf);
    if ((header = get_image_header(Images[i])) == NULL) {
        set_no_header(Images[i]);
        UpdateRequestor(R,i);
        UpdatePushButtons(R,i);
        return 0;
    }
    memcpy((char *) & (Images[i]->header),(char *)header,
        sizeof(struct vicar_header ));

    Images[i]->subset.width = Images[i]->header.samples;
    Images[i]->subset.height = Images[i]->header.lines;



/*
 *  If single band image, load data.
 */
    if (header->bands == 1) {
        Images[i]->band = 0;
		Images[i]->data = NULL;
    } else {
        Images[i]->band = -1;
    }

	if (band >= 0 && band < header->bands) {
		Images[i]->band = band;
		Images[i]->data = NULL;
	}

    R->fnames->ext = (char *)i;
    UpdateRequestor(R, i);
    UpdatePushButtons(R, i);
    FlagForLoad(R,i);
    load_info(Images[(int)R->fnames->ext], R);
    
    return 0;
}

void load_band(Button B, XEvent *E)
{
    Requestor R;
    int i, j;
    Display * display;
    XFontStruct *font;
    char    *s, buf[256];

    R = (Requestor)B->member;
    i = (int)R->fnames->ext;
    display = B->display;
    font = (B->States[0])->Visuals->visual.t_vis.font;

/* 
 *
 */

    s = xgets(display, B->window, 
            R->bnd[i]->x_pos, R->bnd[i]->y_pos - 3, 
            R->bnd[i]->width, R->bnd[i]->height,
            WHITE(display), BLACK(display), 
            font, NULL, E);
    if (s != NULL && s[0] != 0) {
        j = atoi(s);
        if (Images[i] != NULL && Images[i]->composite == 0) {
            if (j <= 0 || j > Images[i]->header.bands) {
                XBell(display, 50);
                return;
            }
            if (Images[i]->data != NULL) {
                free((char *)Images[i]->data);
                Images[i]->data = NULL;
            }
            Images[i]->band = j - 1;
            FlagForLoad(R,i);
        } else {
            if (j < 2 || j > 8) {
                XBell(display, 50);
                return;
            } else {
                Images[i]->band = j;
            }
        }
    }
    UpdateRequestor(R,i);
}


void load_ncolors(Button B, XEvent *E)
{
    Requestor R;
    int i,j;
    Display *display;
    XFontStruct *font;
    char *s,buf[256];

    R = (Requestor)B->member;
    i = (int)R->fnames->ext;
    display = B->display;
    font = (B->States[0])->Visuals->visual.t_vis.font;

/* 
 *
 */

    if (Images[i] != NULL) {
        s = xgets(display, B->window, 0,0,B->width, B->height,
            WHITE(display), BLACK(display), font, NULL, E);
        if (s != NULL && s[0] != 0) {
            j = atoi(s);
            if (j <= 0 || j > NColors) {
                XBell(display, 50);
                return;
            }
            Images[i]->ncolors = j;
            FlagForLoad(R,i);
            load_info(Images[i],R);
        }
    } else {
        XBell(display, 50);
        return;
    }

}


char *
trim_filename(char *s, int n)
{
    char *p;
	if ((p=strchr(s, '\n')) != NULL) *p = '\0';

    if (strlen(s) > n) {
        p = strrchr(s,'/');
        if (p != NULL) {
            return(p+1);
        }
    }
    return(s);
}

Image
allocate_image(void)
{
    Image new;
    int i;

    new = (Image) calloc(1,sizeof(struct _image));
    new->data = NULL;
    new->sdata = NULL;
    new->histogram = NULL;
    new->ncolors = 128;
    new->composite = 0;
    new->ximage = NULL;
    for (i = 0 ; i < NIMAGE ; i++) {
        new->rgb[i] = 0;
        new->overlays[i] = 0;
    }
    new->MapPhoto = NULL;
    new->map = NULL;
    new->hist_image = NULL;
    new->hist_data = NULL;
    new->pan_data = NULL;
    new->pan_image = NULL;
    new->scale = 0;
    new->byte_order = 0;
    new->Dranges=NULL;
    new->NumRanges=0;
    new->c_low = new->c_high = 0;
    new->s_low = new->s_high = 0;
    new->d_low = new->d_high = 0;
	new->is_stretched = 0;
    memset(&(new->subset), 0, sizeof(struct _subset));

    memcpy(&(new->C1),&Colors[0],sizeof(XColor));
    memcpy(&(new->C2),&Colors[1],sizeof(XColor));
    return(new);
}

int
free_image(Display *display, Image *new)
{
    Image old;
    old = *new;
    if (old->data)
        free((char *)old->data);
    old->data = NULL;
    if (old->sdata)
        free((char *)old->sdata);
    old->sdata = NULL;
    if (old->image_hold)
        free((char *)old->image_hold);
    old->image_hold = NULL;
    if (old->histogram)
        free((char *)old->histogram);
    old->histogram = NULL;
    if (old->filename)
        free((char *)old->filename);
    old->filename = NULL;
    if (old->ximage)
        XFree((char *)old->ximage);
    old->ximage = NULL;
    if (old->pixmap)
        XFreePixmap(display,old->pixmap);
    old->pixmap = (Pixmap)NULL;
    if (old->MapPhoto)
        free((char *)old->MapPhoto);
    old->MapPhoto = NULL;

    free((char *)*new);
    *new = NULL;
    
    return 0;
}

void toggle_mem(Button B, XEvent *E)
{
    Requestor R;

    R = (Requestor)B->member;
    if (Images[(int)R->fnames->ext] == NULL) return;

    /* 
     * Memory modes:
     *   0: slow
     *   1: medium
     *   2: fast
     */

    R->SlowMem->state = 0;
    R->MediumMem->state = 0;
    R->FastMem->state = 0;

    B->state = 1;

    Images[(int)R->fnames->ext]->MemMode = (int)B->ext;

    UpdateButton(R->SlowMem);
    UpdateButton(R->MediumMem);
    UpdateButton(R->FastMem);

    if (load_fast_mem(B->display,Images[(int)R->fnames->ext]) == 0) {
        B->state = 0;
        R->SlowMem->state = 1;
        Images[(int)R->fnames->ext]->MemMode = 0;
        UpdateButton(R->SlowMem);
        UpdateButton(R->MediumMem);
        UpdateButton(R->FastMem);
    }
}

int
load_info(Image new, Requestor R)
{
    char *t,buf[256];

    /*
     * Memory mode, Header Info and Colors Used
     */

    if (new == NULL) {
/*
 * DEFAULTS
 */
        R->SlowMem->state = 1;
        R->MediumMem->state = 0;
        R->FastMem->state = 0;

        sprintf(buf,"--");

        strcpy((R->HeaderLines->States[0])->Visuals->visual.t_vis.text,buf);
        strcpy((R->HeaderSamples->States[0])->Visuals->visual.t_vis.text,buf);
        strcpy((R->HeaderBands->States[0])->Visuals->visual.t_vis.text,buf);
        strcpy((R->HeaderFormat->States[0])->Visuals->visual.t_vis.text,buf);
        strcpy((R->HeaderOrg->States[0])->Visuals->visual.t_vis.text,buf);
        strcpy((R->HeaderLabel->States[0])->Visuals->visual.t_vis.text,buf);
        strcpy((R->HeaderOrder->States[0])->Visuals->visual.t_vis.text,buf);

        strcpy((R->SubsetLine->States[0])->Visuals->visual.t_vis.text,buf);
        strcpy((R->SubsetSample->States[0])->Visuals->visual.t_vis.text,buf);
        strcpy((R->SubsetWidth->States[0])->Visuals->visual.t_vis.text,buf);
        strcpy((R->SubsetHeight->States[0])->Visuals->visual.t_vis.text,buf);
        strcpy((R->SubsetLSkip->States[0])->Visuals->visual.t_vis.text,buf);
        strcpy((R->SubsetSSkip->States[0])->Visuals->visual.t_vis.text,buf);

        sprintf(buf,"%d",0);
        t = (R->UsedColors->States[0])->Visuals->visual.t_vis.text;
        strcpy(t,buf);

        sprintf(buf,"%d",NColors);
        t = (R->AvailColors->States[0])->Visuals->visual.t_vis.text;
        strcpy(t,buf);

    } else {
        R->SlowMem->state = 0;
        R->MediumMem->state = 0;
        R->FastMem->state = 0;

        if (new->MemMode == 0) R->SlowMem->state = 1;
        else if (new->MemMode == 1) R->MediumMem->state = 1;
        else if (new->MemMode == 2) R->FastMem->state = 1;


        t = (R->HeaderLines->States[0])->Visuals->visual.t_vis.text;
        if (new->header.lines == 0) sprintf(buf,"--");
        else sprintf(buf,"%d",new->header.lines);
        strcpy(t,buf);

        t = (R->HeaderSamples->States[0])->Visuals->visual.t_vis.text;
        if (new->header.samples == 0) sprintf(buf,"--");
        else sprintf(buf,"%d",new->header.samples);
        strcpy(t,buf);

        t = (R->HeaderBands->States[0])->Visuals->visual.t_vis.text;
        if (new->header.bands == 0) sprintf(buf,"--");
        else sprintf(buf,"%d",new->header.bands);
        strcpy(t,buf);

        t = (R->HeaderFormat->States[0])->Visuals->visual.t_vis.text;
        if (new->header.format == 0) sprintf(buf,"--");
        else {
            switch (new->header.format) {
				case BYTE: sprintf(buf, "%s", "BYTE"); break;
				case SHORT: sprintf(buf, "%s", "SHORT"); break;
				case INT: sprintf(buf, "%s", "INT"); break;
				case FLOAT: sprintf(buf, "%s", "FLOAT"); break;
				case DOUBLE: sprintf(buf, "%s", "DOUBLE"); break;
            }
        }
        strcpy(t,buf);

        t = (R->HeaderOrg->States[0])->Visuals->visual.t_vis.text;
        if (new->header.org == 0) sprintf(buf,"--");
        else {
            switch (new->header.org) {
                case VICAR_BIL: sprintf(buf,"%s","BIL");break;
                case VICAR_BIP: sprintf(buf,"%s","BIP");break;
                case VICAR_BSQ: sprintf(buf,"%s","BSQ");break;
                case FORMAT_GRD: sprintf(buf,"%s","GRD");break;
            }
        }
        strcpy(t,buf);

        t = (R->HeaderLabel->States[0])->Visuals->visual.t_vis.text;
        if (new->header.label_size == 0) sprintf(buf,"--");
        else sprintf(buf,"%d",new->header.label_size);
        strcpy(t,buf);

        t = (R->HeaderOrder->States[0])->Visuals->visual.t_vis.text;
        switch (new->byte_order) {
            case 0: sprintf(buf,"%s","SUN");break;
            case 1: sprintf(buf,"%s","VAX");break;
        }
        strcpy(t,buf);

        t = (R->SubsetSample->States[0])->Visuals->visual.t_vis.text;
        sprintf(buf,"%d",new->subset.sample);
        strcpy(t,buf);

        t = (R->SubsetLine->States[0])->Visuals->visual.t_vis.text;
        sprintf(buf,"%d",new->subset.line);
        strcpy(t,buf);

        t = (R->SubsetWidth->States[0])->Visuals->visual.t_vis.text;
        if (new->subset.uwidth == 0) sprintf(buf,"--");
        else sprintf(buf,"%d",new->subset.uwidth);
        strcpy(t,buf);

        t = (R->SubsetHeight->States[0])->Visuals->visual.t_vis.text;
        if (new->subset.uheight == 0) sprintf(buf,"--");
        else sprintf(buf,"%d",new->subset.uheight);
        strcpy(t,buf);

        t = (R->SubsetSSkip->States[0])->Visuals->visual.t_vis.text;
        sprintf(buf,"%d",new->subset.sskip);
        strcpy(t,buf);

        t = (R->SubsetLSkip->States[0])->Visuals->visual.t_vis.text;
        sprintf(buf,"%d",new->subset.lskip);
        strcpy(t,buf);
/*
Mon Feb 12 16:33:03 MST 2001
Added this color check:
*/

		  if (new->ncolors >= NColors) {
				new->ncolors = NColors-1;
			}


        sprintf(buf,"%d",new->ncolors);
        t = (R->UsedColors->States[0])->Visuals->visual.t_vis.text;
        strcpy(t,buf);

        sprintf(buf,"%d",NColors);
        t = (R->AvailColors->States[0])->Visuals->visual.t_vis.text;
        strcpy(t,buf);
    }
    UpdateButton(R->SlowMem);
    UpdateButton(R->MediumMem);
    UpdateButton(R->FastMem);

    UpdateButton(R->HeaderLines);
    UpdateButton(R->HeaderSamples);
    UpdateButton(R->HeaderBands);
    UpdateButton(R->HeaderFormat);
    UpdateButton(R->HeaderOrg);
    UpdateButton(R->HeaderLabel);
    UpdateButton(R->HeaderOrder);

    UpdateButton(R->SubsetSample);
    UpdateButton(R->SubsetLine);
    UpdateButton(R->SubsetWidth);
    UpdateButton(R->SubsetHeight);
    UpdateButton(R->SubsetSSkip);
    UpdateButton(R->SubsetLSkip);

    UpdateButton(R->UsedColors);
    UpdateButton(R->AvailColors);
    
    return 0;
}

int
create_image(Display *display, Image new)
{
/*
 *
 * Create an image, keeping in mind: memory mode, composites and overlays
 *
 */
    if (new == NULL) return 0;

    if (new->composite) {
        stretch_color(new);
    } else {
        if (new->data == NULL) return 0;
        stretch_gray(new);

        if (new->c_high <= new->c_low) {
            new->c_high = new->s_high;
            new->c_low = new->s_low;
        }
		new->c_low = max(new->c_low, new->s_low);
		new->c_high = min(new->c_high, new->s_high);

        create_hist(display,new,130,0,1);
        if (new->image_hold != NULL) {
            free(new->image_hold);
            new->image_hold = NULL;
        }

/*
 * Free quants:
 *     Can simply free quant[0].pixels cause all of the pixels are in the
 *     same big array, starting with quant[0].pixels.
 */
        if (new->quant) {
            free(new->quant[0].pixels);
            free(new->quant);
            new->quant = NULL;
        }
    }
    if (new->sdata == NULL) {
        return 0;
    }

    create_pan(display,new);
    update_display(display,new);
    
    return 0;
}

void LoadImage(Button B, XEvent *E)
{
    int i;
    Requestor R;
    R = (Requestor)B->member;
    i = (int)B->ext;

    if (Images[i]->band >= 0) {
        B->state = 2;
		UpdateButton(B);

        if ((Images[i]->force_reload || Images[i]->data == NULL) 
               && Images[i]->composite == 0) {
            get_image_data(Images[i]);
        }
        {
        	char buf[256];
			sprintf(buf, "Processing image: %c", 'A' + i);
			SetUserMsg(buf);
		}
        create_image(B->display, Images[i]);

        Images[i]->force_reload=0;
        B->state = 0;
		UpdateButton(B);
    } else {
        XBell(B->display, 50);
    }
}

int
create_hist(Display *display, Image new, int size, int type, int scale)
{
    char *idata = NULL;
    extern XColor pwHilite,pwBackground,pwRed,pwBlue,pwGreen;
    int n,i,k,j;
    int *C=NULL;

    n = size;
    idata = (char *)malloc(sizeof(char)*n*n);
    C = (int *)malloc(sizeof(int)*n);

    /*
     * Clear image to all white
     */
    for (i = 0 ; i < n*n ; i++) {
        idata[i] = WHITE(display);
    }

    if (type) make_hist_dist(new,C,size,scale);
    else make_hist_freq(new,C,size,scale);
    for (i = 0 ; i < n ; i ++) {
        for (j = 0 ; j < (n-C[i]) ; j++) {
            idata[i+j*n] = WHITE(display);
        }
        for (j = (n-C[i]) ; j < n ; j++) {
            idata[i+j*n] = pwBlue.pixel;
        }
    }
    if (new->hist_image != NULL) {
        XFree((char *)new->hist_image);
        free((char *)new->hist_data);
    }
    new->hist_data = idata;
    new->hist_type = type;
    new->hist_scale = scale;

    new->hist_image =
        XCreateImage(display, 
        DefaultVisual(display, DefaultScreen(display)),
        DefaultDepth(display, DefaultScreen(display)), ZPixmap, 0,
        idata, n, n, 8, 0);
    free((char *)C);
    
    return 0;
}

int
make_hist_freq(Image new, int *C, int size, int scale_in)
{
    float step,scale;
    float x,z,lasty;
    float y;
    int *A,B;
    int n,m,count;

    A = new->histogram;
    m = new->ncolors-1;
    n = size-1;

    step = (float)m / (float)n;
    scale = (float)(n)/(float)new->max_hist*scale_in;

    count = 1;

    if (step >= 1.0) {
        lasty = -1;
        for (x = 0.0 ; x <= m ; x++) {
            y = x/step;
            count = (y == lasty ? count+1 : 1);
            C[(int)y] = ((C[(int)y])*(count-1) + (A[(int)x])*scale)/count;
            lasty = y;
        }
    } else {
        x = 0;
        while (x < n) {
            y = x * step;
            z = floor((double)y);
            B = scale *(A[(int)z] * ((z + 1) - y) + A[(int)z+1] * (y - z));
            C[(int)x] = (B > size ? size : B);
            x++;
        }
        B = scale*A[m];
        C[n] = (int) (B > size ? size : B);
    }
    
    return 0;
}

int
make_hist_dist(Image new, int *C, int size, int scale_in)
{
    float   step, scale;
    float   x, z;
    float   y;
    int *A, B;
    int n, m;
    int i, j;
    int total,lasty,count;
	int tmp;

    A = new->histogram;
    m = new->ncolors - 1;
    n = size - 1;

	tmp = A[0];
	A[0] = 0;

    step = (float)m / (float)n;
    total = 0;
    count = 1;
    if (step >= 1.0) {
        lasty = -1;
        for (x = 0.0 ; x <= m ; x++) {
            y = x/step;
            count = (y == lasty ? count+1 : 1);
            C[(int)y] = ((C[(int)y])*(count-1) + (A[(int)x]))/count;
            if ((int)y != lasty) {
                total += (lasty == -1 ? 0 : C[lasty]);
            }
            lasty = y;
        }
        total += C[lasty];
    } else {
        x = 0;
        while (x < n) {
            y = x * step;
            z = floor((double)y);
            B = (A[(int)z] * ((z + 1) - y) + A[(int)z+1] * (y - z));
            C[(int)x] = B;
            total += B;
            x++;
        }
        B = A[m];
        C[n] = (int) B;
    }

    j = 0 ;
    for (i = 0 ; i <= n ; i++) {
        j += C[i];
        x = (total ? (float)j * (float)n / (float)total * scale_in : size);
        C[i] = (int)(x > size ? size : x);
    }

	A[0] = tmp;
	
	return 0;
}





int
overlay(char *data, int start, struct quant_data *quant, int ncolors, int *map)
{
    int i,j;
    if (start > NColors) return 0;

    for (i = 0 ; i < ncolors ; i++) {
        if (map[i] != 0) {
            for (j = 0 ; j < quant[i].count ; j++) {
                data[quant[i].pixels[j]] = Colors[start+i+2].pixel;
            }
        }
    }
    return 0;
}

int
update_display(Display *display, Image new)
{
    int i,j,k,l;
    Image rgb[NIMAGE];
    int nplanes;
    int ncolors[NIMAGE];
    int div;
    char *data;
    int colors;
    int npixels;

    npixels = new->subset.width*new->subset.height;

    for (i = 0 ; i < NIMAGE ; i++) {
        if (new->overlays[i]) {
            if (new->image_hold == NULL) {
                new->image_hold = (char *)malloc(npixels);
                memcpy(new->image_hold,new->sdata,npixels);
            } else {
                memcpy(new->sdata, new->image_hold, npixels);
            }
            break;
        }
    }
    if (i == NIMAGE) {
        if (new->image_hold != NULL) {
            memcpy(new->sdata, new->image_hold, npixels);
            free(new->image_hold);
            new->image_hold = NULL;
        }
    }

    colors = new->ncolors;
    for (i = 0 ; i < NIMAGE ; i++) {
        if (new->overlays[i]) {
            if (Images[i]->quant == NULL) {
            	char buf[256];
            	sprintf(buf, "Quantizing overlay image: %c", 'A' + i);
            	SetUserMsg(buf);

                quantize(Images[i]->sdata,Images[i]->ncolors,
                    Images[i]->subset.width,Images[i]->subset.height,
                    &(Images[i]->quant),Colors[2].pixel);
                default_map(Images[i]);
            }
            overlay(new->sdata,colors,Images[i]->quant,Images[i]->ncolors,Images[i]->map);
            colors += Images[i]->ncolors;
        }
    }

    if (new->sdata == NULL) return 0;
    if (new->ximage != NULL) {
        XFree((char *)new->ximage);
    }

    {
		SetUserMsg("Displaying image");
    }
    
    new->ximage = XCreateImage(display, 
        DefaultVisual(display, DefaultScreen(display)),
        DefaultDepth(display, DefaultScreen(display)),
        ZPixmap, 0, new->sdata, 
        (unsigned int)new->subset.width,
        (unsigned int)new->subset.height, 
        8,0);
    load_fast_mem(display,new);
    return 0;
}

int
load_fast_mem(Display *display, Image new)
{
    /* 
     *  Medium memory
     */
    if (new->pixmap != (Pixmap)NULL)  {
        XFreePixmap(display, new->pixmap);
        new->pixmap = (Pixmap)NULL;
    }

    if (new->ximage == NULL) return 1;
    if (new->MemMode == 1) {
        AllocError = 0;
        new->pixmap =
            XCreatePixmap(display, 
            RootWindow(display, DefaultScreen(display)),
            IWidth, IHeight, 
            DefaultDepth(display, DefaultScreen(display)));
        XSync(display, False);
        if (!AllocError) {
            XPutImage(display, new->pixmap, 
                DefaultGC(display,DefaultScreen(display)), 
                new->ximage, 0, 0, 0, 0, IWidth,IHeight);
        } else {
            new->pixmap = (Pixmap)NULL;
            return 0;
        }
    } else if (new->MemMode == 2) {
        AllocError = 0;
        new->pixmap =
            XCreatePixmap(display, 
            RootWindow(display, DefaultScreen(display)),
            new->subset.width, new->subset.height,
            DefaultDepth(display, DefaultScreen(display)));
        XSync(display, False);
        if (!AllocError) {
            XPutImage(display, new->pixmap, 
                DefaultGC(display,DefaultScreen(display)), 
                new->ximage, 0, 0, 0, 0, 
                new->subset.width, new->subset.height);
        } else {
            new->pixmap = (Pixmap)NULL;
            return 0;
        }
    }
    return 1;
}

int
default_map(Image new)
{
    int j;
    if (new->map == NULL) {
        new->map = (int *)malloc(new->ncolors*sizeof(int));
        for (j = 0 ; j < new->ncolors ; j++) {
            new->map[j] = j;
        }
    }
    return 0;
}

int
create_pan(Display *display, Image new)
{
    int i,x,y;
    int width,height;
    int xsize,ysize;
    float xrat,yrat,rat;
    char *data;
    int size;
    int max;
    extern Joystick JS;

    max = (new->subset.height > new->subset.width ?
            new->subset.height : new->subset.width);

    xsize = JS->width*new->subset.width/max;
    ysize = JS->height*new->subset.height/max;

    xrat = (float)new->subset.width/(float)JS->width;
    yrat = (float)new->subset.height/(float)JS->height;

    rat = (xrat > yrat ? xrat : yrat);

    size = (xsize*ysize);
    data = (char *)malloc((unsigned int)size);

    for (y = 0 ; y < ysize ; y++) {
        for (x = 0 ; x < xsize ; x++) {
            data[y*xsize + x] = 
            new->sdata[((int)(y*rat)*new->subset.width + (int)(x*rat))];
        }
    }
    if (new->pan_data != NULL) {
        XFree((char *)new->pan_image);
        free(new->pan_data);
    }
    new->pan_data = data;
    new->pan_image =
        XCreateImage(display, 
        DefaultVisual(display, DefaultScreen(display)),
        DefaultDepth(display, DefaultScreen(display)), ZPixmap, 0,
        new->pan_data, xsize, ysize, 8, 0);
    return 0;
}

int
set_no_header(Image new)
{
    new->header.label_size =
    new->header.format =
    new->header.record_size =
    new->header.org =
    new->header.lines =
    new->header.samples =
    new->header.bands = 0;
    new->byte_order = 0;
    new->band = -1;
    return 0;
}

static char    buf[256];
static char    tbuf[256];


void GetInput_Samples(Button B, XEvent *E)
{
    int i;
    Requestor R;
    R = (Requestor)B->member;
    i = (int)R->fnames->ext;

    if (GetText(B, E, buf, 256, 0) == -1) 
        return;

    sprintf(tbuf,"%d",atoi(buf));
    SetButtonText(B, buf);
    Images[i]->header.samples = atoi(buf);
    Images[i]->subset.width = atoi(buf);
    FlagForLoad(R,i);
    Images[i]->force_reload=1;
}


void GetInput_Lines(Button B, XEvent *E)
{
    int i;
    Requestor R;
    R = (Requestor)B->member;
    i = (int)R->fnames->ext;

    if (GetText(B, E, buf, 256, 0) == -1) 
        return;

    sprintf(tbuf,"%d",atoi(buf));
    SetButtonText(B, buf);
    Images[i]->header.lines = atoi(buf);
    Images[i]->subset.height = atoi(buf);
    FlagForLoad(R,i);
    Images[i]->force_reload=1;
}


void GetInput_Bands(Button B, XEvent *E)
{
    int i;
    Requestor R;
    R = (Requestor)B->member;
    i = (int)R->fnames->ext;

    if (GetText(B, E, buf, 256, 0) == -1) 
        return;

    sprintf(tbuf,"%d",atoi(buf));
    SetButtonText(B, buf);
    Images[i]->header.bands = atoi(buf);
    FlagForLoad(R,i);
    Images[i]->force_reload=1;

}


void GetInput_Format(Button B, XEvent *E)
{
    int i;
    Requestor R;
    R = (Requestor)B->member;
    i = (int)R->fnames->ext;

    if (GetText(B, E, buf, 256, 0) == -1) 
        return;

    if (!strcmp(buf, "HALF") || !strcmp(buf, "half") || !strcmp(buf, "16")) {
            Images[i]->header.format = HALF;
			Images[i]->header.bits = 16;
            SetButtonText(B, "HALF");
    } else  if (!strcmp(buf, "BYTE")|| !strcmp(buf, "byte") ||!strcmp(buf, "8")) {
        Images[i]->header.bits = 8;
        Images[i]->header.format = BYTE;
        SetButtonText(B, "BYTE");
    }
    FlagForLoad(R,i);
    Images[i]->force_reload=1;
}


void GetInput_Org(Button B, XEvent *E)
{
    int i;
    Requestor R;
    R = (Requestor)B->member;
    i = (int)R->fnames->ext;

    if (GetText(B, E, buf, 256, 0) == -1) 
        return;

    if (!strcmp(buf, "GRD") ||!strcmp(buf, "grd") || !strcmp(buf, "4")) {
        Images[i]->header.org = 4;
        SetButtonText(B, "GRD");
    } else if (!strcmp(buf, "BSQ") ||!strcmp(buf, "bsq") || !strcmp(buf, "3")) {
        Images[i]->header.org = 3;
        SetButtonText(B, "BSQ");
    } else if (!strcmp(buf, "BIP")||!strcmp(buf, "bip")||!strcmp(buf, "2")) {
        Images[i]->header.org = 2;
        SetButtonText(B, "BIP");
    } else {
        Images[i]->header.org = 1;
        SetButtonText(B, "BIL");
    }
    FlagForLoad(R,i);
    Images[i]->force_reload=1;
}


void GetInput_Order(Button B, XEvent *E)
{
    int i;
    Requestor R;
    R = (Requestor)B->member;
    i = (int)R->fnames->ext;

    if (GetText(B, E, buf, 256, 0) == -1) 
        return;

    sprintf(tbuf,"%d",atoi(buf));
    if (!strcmp(buf, "VAX")||!strcmp(buf, "vax")||!strcmp(buf, "1")) {
        Images[i]->byte_order = 1;
        SetButtonText(B, "VAX");
    } else if (!strcmp(buf, "SUN")|| !strcmp(buf, "sun")|| !strcmp(buf, "0")) {
        Images[i]->byte_order = 0;
        SetButtonText(B, "SUN");
    }
    FlagForLoad(R,i);
    Images[i]->force_reload=1;
}


void GetInput_Label(Button B, XEvent *E)
{
    int i, j;
    Requestor R;
    R = (Requestor)B->member;
    i = (int)R->fnames->ext;

    if (GetText(B, E, buf, 256, 0) == -1) 
        return;

    sprintf(tbuf,"%d",atoi(buf));
    SetButtonText(B, tbuf);
    Images[i]->header.label_size = atoi(buf);
    FlagForLoad(R,i);
    Images[i]->force_reload=1;
}

void GetInput_SubsetLine(Button B, XEvent *E)
{
    int i;
    Requestor R;
    R = (Requestor)B->member;
    i = (int)R->fnames->ext;

    if (GetText(B, E, buf, 256, 0) == -1) 
        return;

    sprintf(buf, "%d", atoi(buf));
    SetButtonText(B, buf);
    Images[i]->subset.line = atoi(buf);
    Images[i]->force_reload = 1;

    compute_subset(R,i);

    FlagForLoad(R,i);
}


void GetInput_SubsetSample(Button B, XEvent *E)
{
    int i;
    Requestor R;
    R = (Requestor)B->member;
    i = (int)R->fnames->ext;

    if (GetText(B, E, buf, 256, 0) == -1) 
        return;

    sprintf(buf, "%d", atoi(buf));
    SetButtonText(B, buf);
    Images[i]->subset.sample = atoi(buf);
    Images[i]->force_reload = 1;
    compute_subset(R,i);
    FlagForLoad(R,i);
}


void GetInput_SubsetWidth(Button B, XEvent *E)
{
    int i;
    Requestor R;
    R = (Requestor)B->member;
    i = (int)R->fnames->ext;

    if (GetText(B, E, buf, 256, 0) == -1) 
        return;

    sprintf(buf, "%d", atoi(buf));
    SetButtonText(B, buf);
    Images[i]->subset.uwidth = atoi(buf);
    Images[i]->force_reload = 1;
    compute_subset(R,i);
    FlagForLoad(R,i);
}


void GetInput_SubsetHeight(Button B, XEvent *E)
{
    int i;
    Requestor R;
    R = (Requestor)B->member;
    i = (int)R->fnames->ext;

    if (GetText(B, E, buf, 256, 0) == -1) 
        return;

    sprintf(buf, "%d", atoi(buf));
    SetButtonText(B, buf);
    Images[i]->subset.uheight = atoi(buf);
    Images[i]->force_reload = 1;
    compute_subset(R,i);
    FlagForLoad(R,i);

}


void GetInput_SubsetLSkip(Button B, XEvent *E)
{
    int i;
    Requestor R;
    R = (Requestor)B->member;
    i = (int)R->fnames->ext;

    if (GetText(B, E, buf, 256, 0) == -1) 
        return;

    sprintf(buf, "%d", atoi(buf));
    SetButtonText(B, buf);
    Images[i]->subset.lskip = atoi(buf);
    Images[i]->force_reload = 1;
    compute_subset(R,i);
    FlagForLoad(R,i);
}


void GetInput_SubsetSSkip(Button B, XEvent *E)
{
    int i;
    Requestor R;
    R = (Requestor)B->member;
    i = (int)R->fnames->ext;

    if (GetText(B, E, buf, 256, 0) == -1) 
        return;

    sprintf(buf, "%d", atoi(buf));
    SetButtonText(B, buf);
    Images[i]->subset.sskip = atoi(buf);
    Images[i]->force_reload = 1;
    compute_subset(R,i);
    FlagForLoad(R,i);
}

int
GetNewText(Button B, XEvent *E, char *s, int n, char *str)
{
    char    *p, *q;
    XFontStruct *font;
    p = str;
    font = (B->States[0])->Visuals->visual.t_vis.font;

    q = xgets(B->display, B->window, 0, 0, B->width, B->height,
        WHITE(B->display), BLACK(B->display), font, p, E);

    if (q == NULL || *q == '\0')
        return(-1);
    strncpy(s, q, n);
	return(1);
}

int
ButtonHilite(Button B, unsigned int fg, unsigned int bg)
{
    (B->States[0])->Visuals->foreground = fg;
    (B->States[0])->Visuals->background = bg;
    UpdateButton(B);
    return 0;
}

int
FlagForLoad(Requestor R, int i)
{
    int j;
    Button B;
    struct VisualInfo* vis;

    B = R->letter[i];
    B->state = 1;
    UpdateButton(B);
    return 0;
}

int
UnFlagForLoad(Requestor R, int i)
{
    int j;
    Button B;
    struct VisualInfo* vis;

    B = R->letter[i];
    B->state = 0;
    UpdateButton(B);
    return 0;
}


void DeleteImage(Button B, XEvent *E)
{
    int i;
    Requestor R;
    R = (Requestor)B->member;
    i = (int)R->fnames->ext;

    if (Images[i] != NULL) {
        free_image(B->display,&Images[i]);
    }
    UpdateRequestor(R, i);
}

int compute_subset(Requestor R, int i)
{
    Image new;
    struct _subset *s;
    int width;
    int height;
    int x;
    int y;


    new = Images[i];
    s = &(new->subset);

    x = s->sample;
    y = s->line;
    width = s->uwidth;
    height = s->uheight;

  if (x == 0) x=1;
  if (y == 0) y=1;

    if (x > new->header.samples) x = new->header.samples;
    if (y > new->header.lines) y = new->header.lines;

    if (width == 0) width = new->header.samples - x + 1;
    if (height == 0) height = new->header.lines - y + 1;

    if (x + width > new->header.samples) width = new->header.samples - x + 1;
    if (y + height > new->header.lines) height = new->header.lines - y + 1;

/*  s->width = width / (s->sskip + 1);
    s->height = height / (s->lskip + 1);	***ORIGINAL***/

    s->width = (width-1) / (s->sskip + 1) + 1;
    s->height = (height-1) / (s->lskip + 1) + 1;	/**Modified 9/14/99***/

    s->sample = x;
    s->line = y;

    s->uwidth = width;
    s->uheight = height;

    load_info(Images[i], R);
    
    return 0;
}

int
SetUserMsg(char *buf)
{
	SetButtonText(UserMsg, buf);
	XFlush(UserMsg->display);
}
