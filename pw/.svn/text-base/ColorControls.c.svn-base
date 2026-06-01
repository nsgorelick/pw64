#include <X11/Xos.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "Xfred.h"
#include "color.h"
#include "setcolor.h"
#include "ColorControls.h"
#include "image.h"
#include <math.h>
#include "bitmaps/bitmaps.h"
#include "composite.h"

extern Display *display;
extern XColor Colors[], pwBackground;

/*
 * load color:
 *    Take pixel value from B->ext, and load it into ColorSliders
 */

static char    buf[256];
static char    tbuf[256];
void AMapReadoutUpdate(AMap A, XEvent *E);
void ActivateHistList(Button B, XEvent *E);
void DeactivateHistList(Button B, XEvent *E);
void color_sliders(Slider s, XEvent *e);


extern int FlagForLoad (Requestor R, int i);
int colorspread (struct ColorControls *CC);
extern int RGBToXColor (RGB r, XColor *x);
extern int CreateCSData (Display *display, int ncolors, XColor *colors, int width, int height, char *CSData);
extern int create_hist (Display *display, Image new, int size, int type, int scale);
extern int load_histogram (void);
extern int RGB_CS (Display *display, Colormap CMap, XColor *start, XColor *end, int ncolors, XColor *colors, int rgb, int *map);
int CompositeCS (struct ColorControls *CC);

void
GetLowScale(Button B, XEvent *E)
{
    extern Requestor requestor;
    struct ColorControls *CC = (struct ColorControls *)B->member;

    if (CC->image == NULL) 
        return;
    if (GetText(B, E, buf, 256, 0) == -1)
        return;
/*    sprintf(tbuf, "%.3g", atof(buf));		****ORIGINAL*****/
    sprintf(tbuf, "%f", atof(buf));		/**Modified 9/13/99***/

    if (atof(buf) == CC->image->s_high || atof(buf) == CC->image->s_low) 
        return;

    SetButtonText(B, tbuf);
    CC->image->c_low = CC->image->s_low = atof(tbuf);
    FlagForLoad(requestor, CC->image_index);
}


void
GetHighScale(Button B, XEvent *E)
{
    extern Requestor requestor;
    struct ColorControls *CC = (struct ColorControls *)B->member;

    if (CC->image == NULL) 
        return;

    if (GetText(B, E, buf, 256, 0) == -1)
        return;
/*  sprintf(tbuf, "%.3g", atof(buf));		****ORIGINAL****/	
    sprintf(tbuf, "%f", atof(buf));		/****Modified 9/13/99***/

    if (atof(buf) == CC->image->s_high || atof(buf) == CC->image->s_low) 
        return;

    SetButtonText(B, tbuf);
    CC->image->c_high = CC->image->s_high = atof(tbuf);
    FlagForLoad(requestor, CC->image_index);
}


void
SetLowScale(Button B, XEvent *E)
{

}


void
SetHighScale(Button B, XEvent *E)
{

}

void
change_space(Button b, XEvent *e)
{
    /* 
 * Change slider values to new space.  
 * ('HSV s' is used for final floats cuz its already there.)
 */
    RGB r;
    HSV s;
    struct ColorControls *CC = (struct ColorControls *)b->member;

    if (b->state) {     /* change to HSV space*/
        r.r = XfGetSliderValue(CC->RedSlider);
        r.g = XfGetSliderValue(CC->GreenSlider);
        r.b = XfGetSliderValue(CC->BlueSlider);
        s = RGBToHSV(r);
    } else {            /* change To RGB space */
        s.h = CC->RedSlider->value;
        s.s = CC->GreenSlider->value;
        s.v = CC->BlueSlider->value;
        r = HSVToRGB(s);
        s.h = (float) r.r / (float)MAX_INTENSITY;
        s.s = (float) r.g / (float)MAX_INTENSITY;
        s.v = (float) r.b / (float)MAX_INTENSITY;
    }

    XfSetSliderValue(CC->RedSlider, s.h);
    XfSetSliderValue(CC->GreenSlider, s.s);
    XfSetSliderValue(CC->BlueSlider, s.v);

    color_sliders(CC->RedSlider, NULL);
}


void
load_color(Button B, XEvent *e)
{
    XColor * xc;
    RGB p;
    HSV q;

    struct ColorControls *CC = (struct ColorControls *)B->member;

    if (B == CC->CSLow) {
        xc = CC->C1;
    } else {
        xc = CC->C2;
    }

    if (CC->image == NULL)
        return;
    if (CC->image->composite != 0) {
        q.h = (float)(CC->image->color_offsets[0] + 0xFFFF) / (float)(2 * 0xFFFF);
        q.s = (float)(CC->image->color_offsets[1] + 0xFFFF) / (float)(2 * 0xFFFF);
        q.v = (float)(CC->image->color_offsets[2] + 0xFFFF) / (float)(2 * 0xFFFF);
    } else {
        CC->MixBox->ext = (char *)xc->pixel;
        (CC->MixBox->States[0])->Visuals->foreground = xc->pixel;
        UpdateButton(CC->MixBox);

        if (CC->SliderSpace->state) {       /* In HSV space */
            p.r = xc->red;
            p.g = xc->green;
            p.b = xc->blue;
            q = RGBToHSV(p);
        } else {
            q.h = (float) xc->red / (float)MAX_INTENSITY;
            q.s = (float) xc->green / (float)MAX_INTENSITY;
            q.v = (float) xc->blue / (float)MAX_INTENSITY;
        }
    }

    XfSetSliderValue(CC->RedSlider, q.h);
    XfSetSliderValue(CC->GreenSlider, q.s);
    XfSetSliderValue(CC->BlueSlider, q.v);

    color_sliders(CC->RedSlider, NULL);
}


void
hsv_spread(Button b, XEvent *e)
{

    /* this is the callback for the SpreadSpace controls */

    struct ColorControls *CC = (struct ColorControls *)b->member;

    colorspread(CC);
}


void
color_sliders(Slider s, XEvent *e)
{
    int i;
    float   f[3];
    Button b[3];
    unsigned short  color[3];
    XColor * xc, t;
    struct ColorControls *CC = (struct ColorControls *)s->member;


    b[0] = CC->RedVal;
    b[1] = CC->GreenVal;
    b[2] = CC->BlueVal;

    if (CC->image == NULL)
        return;
    if (CC->image->composite == 0) {
        if ((unsigned long)CC->MixBox->ext == Colors[0].pixel) {
            xc = &(CC->image->C1);
        } else {
            xc = &(CC->image->C2);
        }

        if (CC->SliderSpace->state) {
            HSV r;
            r.h = CC->RedSlider->value;
            r.s = CC->GreenSlider->value;
            r.v = CC->BlueSlider->value;
            RGBToXColor(HSVToRGB(r), xc);

            f[0] = r.h;
            f[1] = r.s;
            f[2] = r.v;

            for (i = 0 ; i < 3 ; i++) {
              sprintf((b[i]->States[0])->Visuals->visual.t_vis.text,
                      "%-1.2f", f[i]);
              UpdateButton(b[i]);
            }
        } else {
          xc->red  = (unsigned short) XfGetSliderValue(CC->RedSlider);
          xc->blue  = (unsigned short) XfGetSliderValue(CC->BlueSlider);
          xc->green  = (unsigned short) XfGetSliderValue(CC->GreenSlider);

          color[0] = xc->red >> 8;
          color[1] = xc->green >> 8;
          color[2] = xc->blue >> 8;

          for (i = 0 ; i < 3 ; i++) {
            sprintf((b[i]->States[0])->Visuals->visual.t_vis.text,
                    "%d", color[i]);
            UpdateButton(b[i]);
          }
        }

        /*
        SEtcolor(CC->display,CC->ColorMap, xc->red,0,0, CC->RedSliderColor);
        setcolor(CC->display,CC->ColorMap, 0,xc->green,0, CC->GreenSliderColor);
        setcolor(CC->display,CC->ColorMap, 0,0,xc->blue, CC->BlueSliderColor);
*/
        setcolor(CC->display, CC->ColorMap, xc->red, xc->green, xc->blue, (*xc));
    } else {
        f[0] = CC->RedSlider->value;
        f[1] = CC->GreenSlider->value;
        f[2] = CC->BlueSlider->value;


        for (i = 0 ; i < 3 ; i++) {
          CC->image->color_offsets[i] = (int)
              (f[i] * (float)(0xFFFF * 2)) - 0xFFFF;

          sprintf((b[i]->States[0])->Visuals->visual.t_vis.text,
                  "%-1.2f", (f[i] * 2 - 1.0));
          UpdateButton(b[i]);
        }
    }
    colorspread(CC);
}


char    *color_names[5] = {
    "Black",
    "White",
    "Red",
    "Green",
    "Blue"
};


void
selected_color(Button B, XEvent *E)
{
    int i;
    XColor xc;
    struct ColorControls *CC;
    CC = (struct ColorControls *)B->member;


    sscanf(B->name, "PICK%d", &i);


    xc.pixel = (B->States[0])->Visuals->foreground;

    XQueryColor(CC->display, CC->ColorMap, &xc);

    if (i % 2) {
        xc.pixel = (unsigned long)CC->C2->pixel;
        XStoreColor(CC->display, CC->ColorMap, &xc);
        load_color(CC->CSHigh, NULL);
    } else {
        xc.pixel = (unsigned long)CC->C1->pixel;
        XStoreColor(CC->display, CC->ColorMap, &xc);
        load_color(CC->CSLow, NULL);
    }

    colorspread(CC);
}


void
select_window(Button B, XEvent *E)
{
    Button b[11];
    XEvent e;
    int i;
    unsigned long   pixels[5];
    struct ColorControls *CC;

    CC = (struct ColorControls *)B->member;

    if (!XAllocColorCells(CC->display, CC->ColorMap, False, 0, 0, pixels, 5))
        return;

    b[0] = XfCreateButton(CC->display, B->parent, (B->x + 10), (B->y) + 10, 40, 100,
        1, BLACK(display), "Selection", 1);

    XfAddButtonVisual(  b[0], 0, XfCreateVisual(
        b[0], 0, 0, 0, 0, WHITE(display), BLACK(display),
        XfSolidVisual));

    for (i = 0 ; i < 10 ; i++) {
        char    buf[256];
        XStoreNamedColor(CC->display, CC->ColorMap, color_names[i/2],
            pixels[i/2], DoRed | DoBlue | DoGreen);

        sprintf(buf, "PICK%d", i);
        b[i+1] = XfCreateButton(CC->display, b[0]->window, 
            (i % 2) * 20 + 5, (i / 2) * 20 + 5, 10, 10, 1, BLACK(display), buf, 1);

        XfAddButtonCallback(b[i+1], 0, selected_color, NULL);
        XfAddButtonVisual(  b[i+1], 0, XfCreateVisual(
            b[i+1], 0, 0, 0, 0, pixels[i/2], BLACK(display),
            XfSolidVisual));
        b[i+1]->member = (int *)CC;
    }

    XfActivateButton(b[0], (ButtonPressMask | ExposureMask | ButtonReleaseMask) );

    for (i = 0 ; i < 10 ; i++)
        XfActivateButton(b[i+1], 
            (ButtonPressMask | ExposureMask | ButtonReleaseMask));

    XGrabPointer(CC->display, b[0]->window, True, 
        (ButtonPressMask | ButtonReleaseMask), GrabModeAsync, GrabModeAsync, 
        b[0]->window, None, CurrentTime);

    while (1) {
        XNextEvent(CC->display, &e);
        XfButtonPush(XfEventButton(&e), &e);
        if (e.type == ButtonPress || e.type == ButtonRelease)
            break;
    }
    for (i = 1 ; i < 11 ; i++) {
        XfDeactivateButton(b[i]);
        XfDestroyButton(b[i]);
    }
    XFreeColors(CC->display, CC->ColorMap, pixels, 5, 0);
    XfDeactivateButton(b[0]);
    XfDestroyButton(b[0]);
    XUngrabPointer(CC->display, CurrentTime);
}


char    *ColorControlsText[] = {
    "RED", "GRN", "BLU", "HUE", "SAT", "VAL" };


struct ColorControls *
InitColorControls(Display *d, Window w, XFontStruct *font, Colormap ColorMap, int x, int y, char *name, XColor RedSliderColor, XColor GreenSliderColor, XColor BlueSliderColor)
{
    Button b;
    int i;
    int x_pos = x, y_pos = y;
    int width, height;
    int Border = 1;
    int text_offset = 3;
    char    buf[256];
    struct ColorControls *CC;

    CC = (struct ColorControls *)malloc(sizeof(struct ColorControls ));

    display = d;

    width = 35;
    height = 47;

    b = XfCreateButton(display, w, x_pos, y_pos, width, height,
        Border, BLACK(display), "SliderSpace", 2);

    XfAddButtonCallback(b, 0, toggle_state, NULL);
    XfAddButtonCallback(b, 1, toggle_state, NULL);
    XfAddButtonCallback(b, 0, change_space, NULL);
    XfAddButtonCallback(b, 1, change_space, NULL);

    /* Add Text visuals for RGB/HSV button: 0,1,2 is RGB, 3,4,5 is HSV */

    for (i = 0 ; i < 6 ; i++) {
        XfAddButtonVisual(b, (i / 3), XfCreateVisual( 
            b, 0, (i % 3 * 15) + text_offset, 0, 15, BLACK(display), 
            WHITE(display), 
            XfTextVisual, ColorControlsText[i], font, 0));
    }
    XfActivateButton(b, (ButtonPressMask | ExposureMask) );
    b->member = (int *)CC;
    CC->SliderSpace = b;


    /* color boxes */

    x_pos += width + Border;

    width = 10;
    height = 15;

     {
        Button b[3];

        b[0] = XfCreateButton(display, w, x_pos, y_pos, width, height, Border,
            BLACK(display), "RedColor", 1);
        y_pos += height + Border;
        b[1] = XfCreateButton(display, w, x_pos, y_pos, width, height, Border,
            BLACK(display), "GreenColor", 1);
        y_pos += height + Border;
        b[2] = XfCreateButton(display, w, x_pos, y_pos, width, height, Border,
            BLACK(display), "BlueColor", 1);

         {
            unsigned int    colors[3];

            colors[0] = RedSliderColor.pixel;
            colors[1] = GreenSliderColor.pixel;
            colors[2] = BlueSliderColor.pixel;

            for (i = 0 ; i < 3 ; i++) {
                XfAddButtonVisual(b[i], 0, XfCreateVisual(
                    b[i], 0, 0, 0, 0, colors[i], BLACK(display),
                    XfSolidVisual));
                XfActivateButton(b[i], (ExposureMask) );
                b[i]->member = (int *)CC;
            }
        }
        CC->RedColor = b[0];
        CC->GreenColor = b[1];
        CC->BlueColor = b[2];
    }

    x_pos += width + Border;
    y_pos = y;

    /* Scroll bars */
    width = 130;
    height = 15;

     {
        Slider s[3];
        s[0] = XfCreateSlider(display, w, x_pos, y_pos, width, height, 
            1, BLACK(display),
            "RedSlider", XfSliderLeftRight, 0.0, 65535.0, 255.0, 10, height);
        y_pos += height + Border;
        s[1] = XfCreateSlider(display, w, x_pos, y_pos, width, height, 
            1, BLACK(display),
            "GreenSlider", XfSliderLeftRight, 0.0, 65535.0, 255.0, 10, height);
        y_pos += height + Border;
        s[2] = XfCreateSlider(display, w, x_pos, y_pos, width, height, 
            1, BLACK(display),
            "BlueSlider", XfSliderLeftRight, 0.0, 65535.0, 255.0, 10, height);

        for (i = 0 ; i < 3 ; i++) {
            XfAddSliderBarVisual(s[i],
                XfCreateVisual(s[i], 0, 0, 0, 0, WHITE(display),
                BLACK(display), XfStippledVisual, "\252\125", 2, 2));
            XfAddSliderThumbVisual(s[i],
                XfCreateVisual(s[i], 0, 0, 10, height, BLACK(display),
                (unsigned long)1, XfSolidVisual));
            XfAddSliderThumbVisual(s[i],
                XfCreateVisual(s[i], 1, 0, 8, height, WHITE(display),
                (unsigned long)1, XfSolidVisual));
            XfAddSliderCallback(s[i], color_sliders, NULL);
            XfActivateSlider(s[i], 
                (ExposureMask | ButtonPressMask |  ButtonMotionMask));

            s[i]->member = (int *)CC;
        }
        CC->RedSlider = s[0];
        CC->GreenSlider = s[1];
        CC->BlueSlider = s[2];
    }

    y_pos = y;
    x_pos += width + Border;

    /* mixture bar */

    width = 10;
    height = 47;

    b = XfCreateButton(display, w, x_pos, y_pos, width, height,
        1, BLACK(display), "MixBox", 1);
    XfAddButtonVisual(b, 0,
        XfCreateVisual(b, 0, 0, 0, 0, BLACK(display), BLACK(display), XfSolidVisual));
    XfActivateButton(b, (ButtonPressMask | ExposureMask) );
    b->member = (int *)CC;
    CC->MixBox = b;

    x_pos += width + Border;

    width = 35;
    height = 15;


     {
        Button b[3];

        b[0] = XfCreateButton(display, w, x_pos, y_pos, width, height,
            Border, BLACK(display), "RedVal", 1);
        y_pos += height + Border;
        b[1] = XfCreateButton(display, w, x_pos, y_pos, width, height,
            Border, BLACK(display), "GreenVal", 1);
        y_pos += height + Border;
        b[2] = XfCreateButton(display, w, x_pos, y_pos, width, height,
            Border, BLACK(display), "BlueVal", 1);

        for (i = 0 ; i < 3 ; i++) {
            XfAddButtonVisual(b[i], 0, XfCreateVisual(b[i], 0, 0 + text_offset, 0, 0,
                BLACK(display), WHITE(display), XfTextVisual, "255", font, 0));
            XfActivateButton(b[i], (ButtonPressMask | ExposureMask) );
            b[i]->member = (int *)CC;
        }
        CC->RedVal = b[0];
        CC->GreenVal = b[1];
        CC->BlueVal = b[2];
    }
    x_pos += width + Border;

    CC->display = display;
    CC->ColorMap = ColorMap;
    CC->image = NULL;

    CC->RedSliderColor = RedSliderColor;
    CC->BlueSliderColor = BlueSliderColor;
    CC->GreenSliderColor = GreenSliderColor;
    return(CC);
}


InitColorSpread(Display *display, Window w, XFontStruct *font, Colormap ColorMap, int x, int y, struct ColorControls *CC)
{
    Button b;
    int Border = 1;
    int x_pos = x;
    int y_pos = y;
    int width;
    int height;
    char    *CSData;

    width = 35;
    height = 15;
    b = XfCreateButton(display, w, x_pos, y_pos, width, height, Border, 
        BLACK(display), "ColorSelection", 1);
    XfAddButtonVisual(b, 0,
        XfCreateVisual(b, 0, 3, 0, 0, BLACK(display), WHITE(display),
        XfTextVisual, "PICK", font, 0));
    XfAddButtonCallback(b, 0, select_window, NULL);
    XfActivateButton(b, (ButtonPressMask | ExposureMask) );
    b->member = (int *)CC;
    CC->ColorSelection = b;

    x_pos += width + Border;
    width = 10;
    height = 15;

    b = XfCreateButton(display, w, x_pos, y_pos, width, height, Border,
        BLACK(display), "CSLow", 1);
    XfAddButtonVisual(b, 0,
        XfCreateVisual(b, 1, 1, width - 2, height - 2, BLACK(display),
        BLACK(display), XfSolidVisual));
    XfAddButtonVisual(b, 0,
        XfCreateVisual(b, 0, 0, 0, 0, WHITE(display), BLACK(display),
        XfOutlineVisual));
    XfAddButtonCallback(b, 0, load_color, NULL);
    XfActivateButton(b, (ButtonPressMask | ExposureMask) );
    b->member = (int *)CC;
    CC->CSLow = b;

    x_pos += width + Border;
    width = 130;
    height = 15;

    CSData = (char *)malloc(width * height);
    if (CSData == NULL) {
        printf("GAG\n");
    }
    CC->CSWidth = width;
    CC->CSHeight = height;

    b = XfCreateButton(display, w, x_pos, y_pos, width, height, 1, 
        BLACK(display), "ColorSpread", 1);
    XfAddButtonVisual(b, 0, XfCreateVisual(b, 0, 0, width, height,
        WHITE(display), BLACK(display), XfXImageVisual, 8, ZPixmap, CSData));
    XfActivateButton(b, (ButtonPressMask | ExposureMask) );
    b->member = (int *)CC;
    CC->Colorspread = b;

    x_pos += width + Border;
    width = 10;
    height = 15;

    b = XfCreateButton(display, w, x_pos, y_pos, width, height, Border,
        BLACK(display), "CSHigh", 1);
    XfAddButtonVisual(b, 0, XfCreateVisual(b, 1, 1, width - 2, height - 2,
        BLACK(display), BLACK(display), XfSolidVisual));
    XfAddButtonVisual(b, 0, XfCreateVisual(b, 0, 0, 0, 0,
        WHITE(display), BLACK(display), XfOutlineVisual));
    XfAddButtonCallback(b, 0, load_color, NULL);
    XfActivateButton(b, (ButtonPressMask | ExposureMask) );
    b->member = (int *) CC;
    CC->CSHigh = b;

    x_pos += width + Border;

    width = 35;
    height = 15;
    b = XfCreateButton(display, w, x_pos, y_pos, width, height, Border, 
        BLACK(display), "SpreadSpace", 2);
    XfAddButtonVisual(b, 0, XfCreateVisual(b, 0, 3, 0, 0,
        BLACK(display), WHITE(display), XfTextVisual, "HSV", font, 0));
    XfAddButtonVisual(b, 1, XfCreateVisual(b, 0, 3, 0, 0,
        BLACK(display), WHITE(display), XfTextVisual, "RGB", font, 0));

    XfAddButtonCallback(b, 0, toggle_state, NULL);
    XfAddButtonCallback(b, 1, toggle_state, NULL);
    XfAddButtonCallback(b, 0, hsv_spread, NULL);
    XfAddButtonCallback(b, 1, hsv_spread, NULL);

    XfActivateButton(b, (ButtonPressMask | ExposureMask) );
    b->member = (int *)CC;
    CC->SpreadSpace = b;

    CC->display = display;
    CC->ColorMap = ColorMap;
    CC->CSData = CSData;

}


LoadColorSpread(struct ColorControls *CC, XColor *low, XColor *high, int ncolors, XColor *colors)
{

    CC->C1 = low;
    CC->C2 = high;

    (CC->CSLow->States[0])->Visuals->foreground = low->pixel;
    (CC->CSHigh->States[0])->Visuals->foreground = high->pixel;

	UpdateButton(CC->CSLow);
	UpdateButton(CC->CSHigh);

    CC->Colorspread->ext = (char *)ncolors;
    CC->Spread = colors;

    load_color(CC->CSHigh, NULL);

    if (CC->image != NULL && CC->image->composite == 0) {
        CreateCSData(CC->display, ncolors, colors, CC->CSWidth,
            CC->CSHeight, CC->CSData);
    } else {
        /* compoiste, blank it out */
        int i, j;
        for (i = 0 ; i < CC->CSWidth ; i++) {
            for (j = 0 ; j < CC->CSHeight ; j++) {
                CC->CSData[j*CC->CSWidth + i] = 
                    ((i + j) % 2 ? BLACK(CC->display) : WHITE(CC->display));
            }
        }
    }
    UpdateButton(CC->Colorspread);
}


void
amap_set_action(Button B, XEvent *E)
{
    int i, j;
    Button b;
    struct ColorControls *AC;

    AC = (struct ColorControls *)B->member;
    j = AC->Map->action_mode;
    if (j) {
        for (i = 0 ; i < 4 ; i++) {
            b = AC->Modes[i];
            if ((int)b->ext == j) {
                b->state = 0;
                (*(b->updateCallback))(b, E);
            }
        }
    }
    B->state = 1;
    (*(B->updateCallback))(B, E);
    XfSetAMapAction(AC->Map, B->ext);
}


void
amap_toggle_state(Button B, XEvent *e)
{
    struct ColorControls *CC;

    CC = (struct ColorControls *)B->member;
    B->state = ((B->state + 1) % B->maxstate);
    (*(B->updateCallback))(B, e);
    XfSetAMapAction(CC->Map, XfAMapNoAction);
}


topslider(Slider S, XEvent *E)
{
    AMap a;
    float value;
    float s_low, s_high;
    struct ColorControls *CC;
    int ncolors;
    int colors;
    char    buf[256];

    /*value = (int)XfGetSliderValue(S); ***ORIGINAL***/
    value = XfGetSliderValue(S); /* Modified 9/9/99 */
    CC = (struct ColorControls *)S->member;
    a = CC->Map;

    if (a->shade_left != value) {
        a->shade_left = value;		/***ORIGINAL***/

/*    if (a->shade_left != (int)value) {
        a->shade_left = (int)value;		*/
        (*(a->exposeCallback))(a, E);
        if (CC->image != NULL) {
            s_low = CC->image->s_low;
            s_high = CC->image->s_high;
            value = (value) * (s_high - s_low) / 130.0 + s_low;
            CC->image->c_low = value;
/*          sprintf(buf, "%.3g", value);		***ORIGINAL****/
            sprintf(buf, "%f", value);		/*Modified 9/13/99***/
            SetButtonText(CC->ScrollLow, buf);
        }
        colorspread(CC);
    }
}


bottomslider(Slider S, XEvent *E)
{
    AMap a;
    struct ColorControls *CC;
    int ncolors;
    int colors;
    char    buf[256];
    float value;
    float s_low, s_high;

    value = S->max - (int)XfGetSliderValue(S);
    CC = (struct ColorControls *)S->member;
    a = CC->Map;
    if (a->shade_right != value) {
        a->shade_right = value;
        (*(a->exposeCallback))(a, E);
        if (CC->image != NULL) {
            s_low = CC->image->s_low;
            s_high = CC->image->s_high;
            value = (S->max - value) * (s_high - s_low) / 130 + s_low;
            CC->image->c_high = value;
/*          sprintf(buf, "%.3g", value);		***ORIGINAL***/
            sprintf(buf, "%f", value);		/*Modified 9/13/99****/
            SetButtonText(CC->ScrollHigh, buf);
        }

        colorspread(CC);
    }

}


void
amap_clear(Button B, XEvent *E)
{
    struct ColorControls *CC;
    int ncolors, *colors;

    CC = (struct ColorControls *)B->member;
    XfCenterAMap(CC->Map);
    XfClearAMap(CC->Map);
    (*(CC->Map->exposeCallback))(CC->Map, E);

    colorspread(CC);
}


void
amap_center(Button B, XEvent *E)
{
    struct ColorControls *CC;
    CC = (struct ColorControls *)B->member;
    XfCenterAMap(CC->Map);
    (*(CC->Map->exposeCallback))(CC->Map, E);

}


void
amap_callback(AMap A, XEvent *E)
{
    struct ColorControls *CC;
    int *mapping, *colors;
    int ncolors;


    CC = (struct ColorControls *) A->member;
    colorspread(CC);
}


translate_amap(AMap map, int *B, int n)
{
    int *A;
    int i, m, left, right;
    float   step, scale;
    float   x, z;
    float   y;

    m = map->width;
    A = (int *)malloc(m * sizeof(int));
    XfAMapValue(map, A);
    left  = map->shade_left;
    right = m - map->shade_right;
    for (i = 0 ; i < left ; i++) {
        A[i] = 0;
    }
    for (i = right ; i < m ; i++) {
        A[i] = m-1;
    }

	for (i = 0; i < n; i++) {
		z = (float) i / (float) (n - 1) * (m - 1);
		B[i] = (int) A[(int) z] / (float) (m - 1) * (n - 1);
	}

    free((char *)A);
}

void
toggle_hist(Button B, XEvent *E)
{
    struct ColorControls *CC;
    CC = (struct ColorControls *)B->member;
    toggle_state(B, E);
    if (CC->image != NULL && CC->image->composite == 0) {
        if (B->state == 2) {
            free(CC->image->hist_data);
            XFree((char *)CC->image->hist_image);
            CC->image->hist_data = NULL;
            CC->image->hist_image = NULL;
        } else {
            create_hist(B->display, CC->image, 130, B->state, 1);
        }
        CC->image->hist_type = B->state;
        load_histogram();
    }
}


void
scale_hist(Button B, XEvent *E)
{
    int scale;
    struct ColorControls *CC;
    CC = (struct ColorControls *)B->member;
    if (CC->image == NULL || CC->image->composite != 0) 
        return;

    scale = CC->image->hist_scale;

    if (E->xbutton.x < B->width / 2) {
        /* up */
        scale *= 2.0;
    } else {
        /* down */
        scale *= 0.5;
    }
    if (scale > 512) 
        scale = 512;
    if (scale < 1) 
        scale = 1;
    if (scale == CC->image->hist_scale) 
        return;

    create_hist(B->display, CC->image, 130, CC->HistType->state, scale);
    CC->image->hist_scale = scale;
    load_histogram();
}


CreateAMap(struct ColorControls *AC, Display *display, Window w, XFontStruct *font, int x, int y, long unsigned int hilite, long unsigned int shade)
{
    int xin = x, yin = y;
    int width, height;
    Button b;
    int Border = 1;
    int i;
    int yout;
    char    *map;

	yin = y = y + 20;

    y += 45;
    width = 35;
    height = 15;

    AC->Modes[0] = XfCreateButton(display, w, x, y, width, height, 
        Border, BLACK(display), "Add", 2);
    y += height + 8;
    AC->Modes[1] = XfCreateButton(display, w, x, y, width, height, 
        Border, BLACK(display), "Mov", 2);
    y += height + 8;
    AC->Modes[2] = XfCreateButton(display, w, x, y, width, height, 
        Border, BLACK(display), "Del", 2);
    y += height + 8;
    AC->Modes[3] = XfCreateButton(display, w, x, y, width, height, 
        Border, BLACK(display), "Shift", 2);
    y += height + 8;
    /* only one state for clear */
    AC->Modes[4] = XfCreateButton(display, w, x, y, width, height, 
        Border, BLACK(display), "Clear", 1);
    y += height + 8;
    AC->Modes[5] = XfCreateButton(display, w, x, y, width, height, 
        Border, BLACK(display), "Center", 1);

    yout = y + height;

    y = yin + 45;
    x += width + 12;

    XfAddButtonVisual(AC->Modes[0], 0,
        XfCreateVisual(AC->Modes[0], 0, 3, 0, 0, BLACK(display),
        WHITE(display), XfTextVisual, "ADD",
        font, 0));
    XfAddButtonVisual(AC->Modes[1], 0,
        XfCreateVisual(AC->Modes[1], 0, 3, 0, 0, BLACK(display),
        WHITE(display), XfTextVisual, "MOV",
        font, 0));
    XfAddButtonVisual(AC->Modes[2], 0,
        XfCreateVisual(AC->Modes[2], 0, 3, 0, 0, BLACK(display),
        WHITE(display), XfTextVisual, "DEL",
        font, 0));
    XfAddButtonVisual(AC->Modes[3], 0,
        XfCreateVisual(AC->Modes[3], 0, 3, 0, 0, BLACK(display),
        WHITE(display), XfTextVisual, "SHIFT",
        font, 0));
    XfAddButtonVisual(AC->Modes[4], 0,
        XfCreateVisual(AC->Modes[4], 0, 3, 0, 0, BLACK(display),
        WHITE(display), XfTextVisual, "CLEAR",
        font, 0));
    XfAddButtonVisual(AC->Modes[5], 0,
        XfCreateVisual(AC->Modes[5], 0, 3, 0, 0, BLACK(display),
        WHITE(display), XfTextVisual, "ALIGN",
        font, 0));

    XfAddButtonVisual(AC->Modes[0], 1,
        XfCreateVisual(AC->Modes[0], 0, 3, 0, 0, BLACK(display),
        hilite, XfTextVisual, "ADD", font, 0));
    XfAddButtonVisual(AC->Modes[1], 1,
        XfCreateVisual(AC->Modes[1], 0, 3, 0, 0, BLACK(display),
        hilite, XfTextVisual, "MOV", font, 0));
    XfAddButtonVisual(AC->Modes[2], 1,
        XfCreateVisual(AC->Modes[2], 0, 3, 0, 0, BLACK(display),
        hilite, XfTextVisual, "DEL", font, 0));
    XfAddButtonVisual(AC->Modes[3], 1,
        XfCreateVisual(AC->Modes[3], 0, 3, 0, 0, BLACK(display),
        hilite, XfTextVisual, "SHIFT", font, 0));

    AC->Modes[0]->ext = (char *)XfAMapAdd;
    AC->Modes[1]->ext = (char *)XfAMapMove;
    AC->Modes[2]->ext = (char *)XfAMapDel;
    AC->Modes[3]->ext = (char *)XfAMapSlide;

    /* No second visual for clear, its a pushbutton */

    for (i = 0 ; i < 4 ; i++) {
        XfAddButtonCallback(AC->Modes[i], 0, amap_set_action, NULL);
        XfAddButtonCallback(AC->Modes[i], 1, amap_toggle_state, NULL);
        XfActivateButton(AC->Modes[i], (ExposureMask | ButtonPressMask));
        AC->Modes[i]->member = (int *)AC;
    }
    XfAddButtonCallback(AC->Modes[4], 0, amap_clear, NULL);
    XfAddButtonCallback(AC->Modes[5], 0, amap_center, NULL);
    XfActivateButton(AC->Modes[4], (ExposureMask | ButtonPressMask));
    XfActivateButton(AC->Modes[5], (ExposureMask | ButtonPressMask));
    AC->Modes[4]->member = (int *)AC;
    AC->Modes[5]->member = (int *)AC;

    /* Create Map */

    width = 130;
    height = 130;

    AC->Map = XfCreateAMap(display, w, x, y, width, height, 1, BLACK(display),
        "Map", hilite, shade, FillSolid, 0);
    XfAddAMapVisual(AC->Map,
        XfCreateVisual(AC->Map,
        0, 0, 0, 0, WHITE(display), WHITE(display),
        XfSolidVisual));
    XfAddAMapCallback(AC->Map, amap_callback, NULL);
    XfActivateAMap(AC->Map, (ExposureMask | ButtonPressMask | 
        ButtonReleaseMask | ButtonMotionMask));
    AC->Map->member = (int *)AC;

    /* Now, sliders */
    x = xin + 47;
    y = yin + 5;
    width = 130;
    height = 15;

    AC->Top = XfCreateSlider(display, w, x, y, width, height, 1, BLACK(display),
        "Top", XfSliderLeftRight, 0.0, 130.0, 1.0, 10,
        height);
    y += height + 1;
    AC->Bottom = XfCreateSlider(display, w, x, y, width, height, 1, BLACK(display),
        "Bottom", XfSliderLeftRight, 0.0, 130.0, 1.0,
        10, height);

    XfAddSliderCallback(AC->Top, topslider, NULL);
    XfAddSliderCallback(AC->Bottom, bottomslider, NULL);

    XfAddSliderBarVisual(AC->Top,
        XfCreateVisual(AC->Top, 0, 0, 0, 0,
        BLACK(display), WHITE(display),
        XfStippledVisual, "\252\125", 2, 2));
    XfAddSliderBarVisual(AC->Bottom,
        XfCreateVisual(AC->Bottom, 0, 0, 0, 0,
        BLACK(display), WHITE(display),
        XfStippledVisual, "\252\125", 2, 2));

    XfAddSliderThumbVisual(AC->Top,
        XfCreateVisual(AC->Top, 0, 0, 10, height,
        BLACK(display), BLACK(display),
        XfSolidVisual));
    XfAddSliderThumbVisual(AC->Bottom,
        XfCreateVisual(AC->Bottom, 0, 0, 
        10, height, BLACK(display),
        BLACK(display), XfSolidVisual));

    XfAddSliderThumbVisual(AC->Top,
        XfCreateVisual(AC->Top, 1, 0, 8, height,
        WHITE(display), BLACK(display),
        XfSolidVisual));
    XfAddSliderThumbVisual(AC->Bottom,
        XfCreateVisual(AC->Bottom, 1, 0, 
        8, height, WHITE(display),
        BLACK(display), XfSolidVisual));

    XfActivateSlider(AC->Top,
        (ExposureMask | ButtonPressMask | ButtonMotionMask));
    XfActivateSliderValue(AC->Bottom, 1.0,
        (ExposureMask | ButtonPressMask | ButtonMotionMask));

    x = xin;
    y = yin + 5;
    width = 35;
    height = 15;
    b = XfCreateButton(display, w, x, y, width, height, 1, BLACK(display),
        "lowstr", 1);
    XfAddButtonVisual(b, 0, XfCreateVisual(b, 0, 3, 0, 0, BLACK(display), 
        WHITE(display), XfTextVisual, "0", font, 1));
    XfAddButtonCallback(b, 0, GetLowScale, NULL);
    XfActivateButton(b, ExposureMask | ButtonPressMask);
    AC->StretchLow = b;
    b->member = (int *)AC;

    y = y + height;

    b = XfCreateButton(display, w, x, y, width, height, 1, BLACK(display),
        "highstr", 1);
    XfAddButtonVisual(b, 0, XfCreateVisual(b, 0, 3, 0, 0, BLACK(display), 
        WHITE(display), XfTextVisual, "0", font, 1));
    XfAddButtonCallback(b, 0, GetHighScale, NULL);
    XfActivateButton(b, ExposureMask | ButtonPressMask);
    AC->StretchHigh = b;
    b->member = (int *)AC;

    x = xin + 47 + 131 + 11;

    y = yin + 5;
    width = 35;
    height = 15;

    b = XfCreateButton(display, w, x, y, width, height, 1, BLACK(display),
        "lowscroll", 1);
    XfAddButtonVisual(b, 0, XfCreateVisual(b, 0, 3, 0, 0, BLACK(display), 
        pwBackground.pixel, XfTextVisual, "0", font, 1));
    XfAddButtonCallback(b, 0, SetLowScale, NULL);
    XfActivateButton(b, ExposureMask | ButtonPressMask);
    AC->ScrollLow = b;
    b->member = (int *)AC;

    y = y + height;


    b = XfCreateButton(display, w, x, y, width, height, 1, BLACK(display),
        "highscroll", 1);
    XfAddButtonVisual(b, 0, XfCreateVisual(b, 0, 3, 0, 0, BLACK(display), 
        pwBackground.pixel, XfTextVisual, "0", font, 1));
    XfAddButtonCallback(b, 0, SetHighScale, NULL);
    XfActivateButton(b, ExposureMask | ButtonPressMask);
    AC->ScrollHigh = b;
    b->member = (int *)AC;

    AC->Top->member = (int *)AC;
    AC->Bottom->member = (int *)AC;

    y = yin + 45;
    width = 35;
    height = 20;

    b = XfCreateButton(display, w, x, y, width, height, 
        Border, BLACK(display), "Arrows", 1);
    XfAddButtonVisual(b, 0, XfCreateVisual(b, -1, 0, 20, 20,
        BLACK(display), WHITE(display),
        XfPixmapVisual, 1, Bitmap_Button_Up_bits));
    XfAddButtonVisual(b, 0, XfCreateVisual(b, 18, 0, 20, 20,
        BLACK(display), WHITE(display),
        XfPixmapVisual, 1, Bitmap_Button_Down_bits));

    XfAddButtonCallback(b, 0, scale_hist, NULL);
    XfActivateButton(b, ButtonPressMask | ExposureMask);
    b->member = (int *)AC;
    AC->HistScale = b;

    x = xin + 47 + 131 + 11;
    y += height + 8;
    width = 35;
    height = 15;

    b = XfCreateButton(display, w, x, y, width, height, 
        Border, BLACK(display), "Guass", 3);
    XfAddButtonVisual(b, 0,
        XfCreateVisual(b, 0, 3, 0, 0, BLACK(display),
        WHITE(display), XfTextVisual, "FREQ",
        font, 0));
    XfAddButtonVisual(b, 1,
        XfCreateVisual(b, 0, 3, 0, 0, BLACK(display),
        WHITE(display), XfTextVisual, "CUMUL",
        font, 0));
    XfAddButtonVisual(b, 2,
        XfCreateVisual(b, 0, 3, 0, 0, BLACK(display),
        WHITE(display), XfTextVisual, "NONE",
        font, 0));
    XfAddButtonCallback(b, 0, toggle_hist, NULL);
    XfAddButtonCallback(b, 1, toggle_hist, NULL);
    XfAddButtonCallback(b, 2, toggle_hist, NULL);
    XfActivateButton(b, ButtonPressMask | ExposureMask);
    b->member = (int *)AC;
    AC->HistType = b;

    y += height + 8;
    width = 35;
    height = 15;

    b = XfCreateButton(display, w, x, y, width, height, 
        Border, BLACK(display), "ReadHist", 2);
    XfAddButtonVisual(b, 0, XfCreateVisual(b, 0, 3, 0, 0,
        BLACK(display), WHITE(display),
        XfTextVisual, "LIST", font, 0));
    XfAddButtonVisual(b, 1, XfCreateVisual(b, 0, 3, 0, 0,
        WHITE(display), BLACK(display),
        XfTextVisual, "LIST", font, 0));
    XfAddButtonCallback(b, 0, ActivateHistList, NULL);
    XfAddButtonCallback(b, 1, DeactivateHistList, NULL);
    XfActivateButton(b, ButtonPressMask | ExposureMask);

    x = xin + 47 + 131 - 100;
    y = yin + 135 + 47;
    width = 100;

    b = XfCreateButton(display, w, x, y, width, height, 
        Border, BLACK(display), "WriteHist", 1);
    XfAddButtonVisual(b, 0, XfCreateVisual(b, 0, 7, 0, 0,
        BLACK(display), WHITE(display),
        XfTextVisual, "Readout", font, 0));
    XfActivateButton(b, ButtonPressMask | ExposureMask);

    AC->Readout = b;
    XfAddAMapReadoutCallback(AC->Map, AMapReadoutUpdate);
}


colorspread(struct ColorControls *CC)
{
    if (CC->image == NULL)
        return;
    if (CC->image->composite == 0) {
        int ncolors, *colors;

        ncolors = (int)CC->Colorspread->ext;
        if (CC->image->map != NULL) {
            free((char *)CC->image->map);
        }
        CC->image->map = (int *)malloc(ncolors * sizeof(int));
        translate_amap(CC->Map, CC->image->map, ncolors);

        RGB_CS(CC->display, CC->ColorMap, CC->C1, CC->C2,
            (long)CC->Colorspread->ext, CC->Spread, CC->SpreadSpace->state,
            CC->image->map);

    }  else {
        CompositeCS(CC);
    }
}


CompositeCS(struct ColorControls *CC)
{
    XColor xc, *p;
    Image new = CC->image;
    int result;
    int j;

    for (j = 0 ; j < new->ncolors; j++) {
        p = &(new->Colors[j]);
        p->red = MAX(MIN((int)(p->red + new->color_offsets[0]), 0xFFFF), 0);
        p->green = MAX(MIN((int)(p->green + new->color_offsets[1]), 0xFFFF), 0);
        p->blue = MAX(MIN((int)(p->blue + new->color_offsets[2]), 0xFFFF), 0);
        p->flags = DoRed | DoBlue | DoGreen;
    }
    result = XStoreColors(CC->display, CC->ColorMap, new->Colors, new->ncolors);
    if (result != Success) {
      char error_buf[256];
      XGetErrorText(display, result, error_buf, 256);
      printf("XStoreColors error: %s\n", error_buf);
    }
}


void
AMapReadoutUpdate(AMap A, XEvent *E)
{
    struct ColorControls *CC;
    int x,y;
    int imin, imax;

    CC = (struct ColorControls *)A->member;
    if (CC == NULL || CC->Readout == NULL) return;
    if (CC->image == NULL) return;

    x = E->xbutton.x;
    y = E->xbutton.y;
    
    if (y < 0) y = 0;
    else if (y >= A->height)
        y = A->height - 1;
    
    if (x < 0)  x = 0;
    else if (x >= A->width) x = A->width;

    imin = CC->image->s_low;
    imax = CC->image->s_high;

    x = (float)x / (float)(A->height) * 255;
    y = (float)(A->height - y) / (float)A->height * 255;

    /* this probably wants to be scaled according to the
       stretch on the image already, but doesn't present
       itself in an easy way)
    */
       


    sprintf(buf, "%d,%d", x, y);
    SetButtonText(CC->Readout, buf);    
}
