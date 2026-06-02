#include "ColorControls.h"
#include "Xfred.h"
#include "bitmaps/bitmaps.h"
#include "composite.h"
#include "image.h"
#include "mag.h"
#include <X11/Xlib.h>
#include <X11/Xos.h>
#include <X11/Xutil.h>

/* Additional function prototypes */
int ConfirmRequestor(Display *display, Window parent, GC gc, XFontStruct *font, int x, int y, int width, int height,
                     int header, int warp, int warp_opt, ...);
int XfJoystickLimitThumb(Joystick J, int x_min, int y_min, int x_max, int y_max);
int XfJoystickResizeThumb(Joystick J, int width, int height);

extern struct ColorControls *CC;
extern Requestor requestor;
extern struct magnify *Mag;
extern Button CCWindow;
extern Window IWindow;
extern XColor Colors[];
extern XColor pwRed, pwBlue, pwGreen, pwYellow, pwHilite;
extern int NColors;

extern int IWidth, IHeight;
extern int IXPos, IYPos;
extern GC gc;

Button MagnifyButton;
Joystick JS;
void Redisplay(Button B, XEvent *E);

extern void WritePopup(Display *display, Window parent, XFontStruct *font);
extern int ActivateMagnify(Display *display, struct magnify *Mag);
extern int DeactivateMagnify(Display *display, struct magnify *Mag);
extern void ActivatePlots(Display *d);
extern void ActivateRose(Display *display);
extern void DeactivatePlots(Display *d);
extern void DeactivateRose(Display *display);
extern void LoadImageWindow(Image new);
int unload_pallate(void);
int load_pallate(int i);
int load_histogram(void);
int load_pan(void);
int load_stretch(void);
extern char *trim_filename(char *s, int n);
int LoadDnList(int i);
extern int LoadColorSpread(struct ColorControls *CC, XColor *low, XColor *high, int ncolors, XColor *colors);
extern int default_map(Image new);
extern int RGB_CS(Display *display, Colormap CMap, XColor *start, XColor *end, int ncolors, XColor *colors, int rgb,
                  int *map);
int MaximizeImage(Display *display);
extern int update_display(Display *display, Image new);
extern int ClearDnList(void);
extern void AddToDnList(int plane, XColor *color);
extern int ActivateDnList(void);
extern int colorspread(struct ColorControls *CC);

void quit(Button B, XEvent *E)
{
    int i;
    XFontStruct *font;

    if (B->ext)
        return;
    font = (B->States[0])->Visuals->visual.t_vis.font;

    B->ext = PW_CAST_INT(1);
    i = ConfirmRequestor(B->display, RootWindow(B->display, DefaultScreen(B->display)), gc, font,
                         (int)E->xbutton.x_root, (int)E->xbutton.y_root, 120, 60, 0, 1, 2, 1, "REALLY QUIT?", 2, "NO",
                         "YES");

    B->ext = 0;
    if (i == 2) {
        exit(1);
    }
}

void ActivateWrite(Button B, XEvent *E)
{
    B->state = 1;
    UpdateButton(B);

    WritePopup(B->display, RootWindow(B->display, DefaultScreen(B->display)), (XFontStruct *)B->ext);

    B->state = 0;
    UpdateButton(B);
}

void toggle_mag(Button B, XEvent *E)
{
    toggle_state(B, E);
    Mag->state = B->state;
    if (Mag->state) {
        ActivateMagnify(B->display, Mag);
    } else {
        DeactivateMagnify(B->display, Mag);
    }
}

void toggle_plot(Button B, XEvent *E)
{
    toggle_state(B, E);
    if (B->state) {
        if (E->xbutton.button == Button1) {
            ActivatePlots(B->display);
        } else {
            ActivateRose(B->display);
        }

    } else {
        if (E->xbutton.button == Button1) {
            DeactivatePlots(B->display);
        } else {
            DeactivateRose(B->display);
        }
    }
}

void pan_image(Joystick J, XEvent *E)
{
    int width, height, max;
    Image new;

    new = CC->image;
    if (new == NULL)
        return;

    width = new->subset.width;
    height = new->subset.height;
    max = (width > height ? width : height);

    IXPos = ((J->thumb_cur_x) * max / JS->width);
    IYPos = ((J->thumb_cur_y) * max / JS->height);

    LoadImageWindow(CC->image);
}

int CurImage = -1;

int load_image(int i)
{
    int j, ncolors;
    char buf[256];
    int width, height;
    /*
     * unload and save previous data
     */

    if (Images[i]->data == NULL && Images[i]->composite == 0)
        return (0);

    if ((width = Images[i]->subset.width - IWidth) < IXPos)
        IXPos = width;
    if ((height = Images[i]->subset.height - IHeight) < IYPos)
        IYPos = height;

    if (IXPos < 0)
        IXPos = 0;
    if (IYPos < 0)
        IYPos = 0;

    unload_pallate();

    LoadImageWindow(Images[i]);

    CC->image = Images[i];
    CC->image_index = i;

    load_pallate(i);
    load_histogram();
    load_pan();
    load_stretch();
    SetMagnify(CC->display, CC->image, Mag, Mag->x, Mag->y);

    if (CC->image->composite == 0) {
        sprintf(buf, "%c %s", ('A' + i), trim_filename(CC->image->filename, 30));
    } else {
        sprintf(buf, "%c %s", ('A' + i), "<composite>");
    }
    LoadDnList(i);
    XStoreName(CC->display, IWindow, buf);
    XStoreName(CC->display, Mag->window, buf);

    return (0);
}

int load_pan(void)
{
    int max;
    int x_max, y_max;
    int width, height;
    int t_width, t_height;
    Image new;

    new = CC->image;

    if (new != NULL) {
        JS->field->vtype = XfXImageVisual;
        JS->field->visual.i_vis = new->pan_image;

        width = new->subset.width;
        height = new->subset.height;
        max = (width > height ? width : height);

        x_max = (JS->width) * width / max;
        y_max = (JS->height) * height / max;

        t_width = x_max * (IWidth) / width - 1;
        t_height = y_max * (IHeight) / height - 1;

        if (t_width > x_max)
            t_width = x_max;
        if (t_height > y_max)
            t_height = y_max;

        JS->thumb->width = t_width;
        JS->thumb->height = t_height;

        XfJoystickLimitThumb(JS, 0, 0, x_max, y_max);
        XfJoystickResizeThumb(JS, t_width, t_height);
    } else {
        JS->field->vtype = XfSolidVisual;
        JS->field->visual.i_vis = NULL;
    }
    (*(JS->exposeCallback))(JS, NULL);

    return (0);
}

int load_histogram(void)
{
    AMap A = CC->Map;
    if (CC->image != NULL && CC->image->composite == 0) {
        if (CC->image->hist_type != 2) {
            A->visual->vtype = XfXImageVisual;
            A->visual->visual.i_vis = CC->image->hist_image;
            CC->HistType->state = CC->image->hist_type;
            if (CC->image->hist_scale > 1) {
                ((CC->HistScale->States[0])->Visuals[0]).background = pwHilite.pixel;
            } else {
                ((CC->HistScale->States[0])->Visuals[0]).background = WHITE(CC->display);
            }
            UpdateButton(CC->HistScale);
        } else {
            A->visual->vtype = XfSolidVisual;
            A->visual->visual.i_vis = NULL;
            CC->HistType->state = 2;
        }
    } else {
        A->visual->vtype = XfSolidVisual;
        A->visual->visual.i_vis = NULL;
        CC->HistType->state = 2;
    }
    UpdateButton(CC->HistType);
    (*(A->exposeCallback))(A, NULL);

    return (0);
}

int unload_pallate(void)
{
    XColor *g1, *g2;

    if (CC->image != NULL) {
        if (CC->image->composite) {
            return (0);
        }
        if (CC->image->MapPhoto != NULL) {
            free((char *)CC->image->MapPhoto);
        }
        CC->image->MapPhoto = XfPhotographAMap(CC->Map);
    }

    return (0);
}

int load_pallate(int i)
{
    XColor *g1, *g2;
    Image new, comp = NULL;
    int j, k;
    int colors;

    new = Images[i];

    g1 = &(new->C1);
    g2 = &(new->C2);

    (void)g1;
    (void)g2;
    if (new->MapPhoto != NULL) {
        XfRestoreAMap(CC->Map, new->MapPhoto);
    } else {
        XfClearAMap(CC->Map);
        (*(CC->Map->exposeCallback))(CC->Map, NULL);
    }
    LoadColorSpread(CC, g1, g2, new->ncolors, Colors + 2);
    colors = new->ncolors;
    for (j = 0; j < NIMAGE; j++) {
        if (new->overlays[j]) {
            default_map(Images[j]);
            RGB_CS(CC->display, CC->ColorMap, &(Images[j]->C1), &(Images[j]->C2), Images[j]->ncolors,
                   Colors + colors + 2, CC->SpreadSpace->state, Images[j]->map);

            colors += Images[j]->ncolors;
        }
    }

    return (0);
}

void toggle_image_window(Button B, XEvent *E)
{

    if (E->xbutton.button != Button2 || B->state == 0) {
        toggle_state(B, NULL);
    } else {
        MaximizeImage(B->display);
    }

    if (B->state == 1) {
        XMapWindow(B->display, IWindow);
    } else {
        XUnmapWindow(B->display, IWindow);
        XSetTransientForHint(B->display, IWindow, DefaultRootWindow(B->display));
        XFlush(B->display);
    }
}

void toggle_requestor_window(Button B, XEvent *E)
{
    toggle_state(B, NULL);
    if (B->state == 1) {
        ActivateRequestor(requestor);
    } else
        DeactivateRequestor(requestor);
}

int gadget_buttons(Display *display, Window w, XFontStruct *font)
{
    Button b, B;
    int x, y, width, height;
    char *map;

    B = XfCreateButton(display, w, 10, 10, 97, 97, 1, BLACK(display), "W", 1);
    XfAddButtonVisual(B, 0, XfCreateVisual(B, 0, 3, 0, 0, WHITE(display), WHITE(display), XfSolidVisual));
    XfActivateButton(B, ExposureMask);

    x = 5;
    y = 5;
    width = 25;
    height = 25;

    map = Window_bits;

    b = XfCreateButton(display, B->window, x, y, width, height, 1, BLACK(display), "Image", 2);
    XfAddButtonVisual(b, 0,
                      XfCreateVisual(b, 0, 0, width, height, BLACK(display), WHITE(display), XfPixmapVisual, 1, map));
    XfAddButtonVisual(b, 1,
                      XfCreateVisual(b, 0, 0, width, height, WHITE(display), BLACK(display), XfPixmapVisual, 1, map));
    XfAddButtonCallback(b, 0, XF_CALLBACK(toggle_image_window), NULL);
    XfAddButtonCallback(b, 1, XF_CALLBACK(toggle_image_window), NULL);
    XfActivateButton(b, ButtonPressMask | ExposureMask);

    x += 30;

    map = Magnify_bits;

    b = XfCreateButton(display, B->window, x, y, width, height, 1, BLACK(display), "Magnify", 2);
    XfAddButtonVisual(b, 0,
                      XfCreateVisual(b, 0, 0, width, height, BLACK(display), WHITE(display), XfPixmapVisual, 1, map));
    XfAddButtonVisual(b, 1,
                      XfCreateVisual(b, 0, 0, width, height, WHITE(display), BLACK(display), XfPixmapVisual, 1, map));
    XfAddButtonCallback(b, 0, XF_CALLBACK(toggle_mag), NULL);
    XfAddButtonCallback(b, 1, XF_CALLBACK(toggle_mag), NULL);
    XfActivateButton(b, ExposureMask | ButtonPressMask);
    MagnifyButton = b;

    x += 30;
    map = Plot_bits;

    b = XfCreateButton(display, B->window, x, y, width, height, 1, BLACK(display), "Plot", 2);
    XfAddButtonVisual(b, 0,
                      XfCreateVisual(b, 0, 0, width, height, BLACK(display), WHITE(display), XfPixmapVisual, 1, map));
    XfAddButtonVisual(b, 1,
                      XfCreateVisual(b, 0, 0, width, height, WHITE(display), BLACK(display), XfPixmapVisual, 1, map));
    XfAddButtonCallback(b, 0, XF_CALLBACK(toggle_plot), NULL);
    XfAddButtonCallback(b, 1, XF_CALLBACK(toggle_plot), NULL);
    XfActivateButton(b, ExposureMask | ButtonPressMask);

    x = 5;
    y += 30;
    map = File_bits;

    b = XfCreateButton(display, B->window, x, y, width, height, 1, BLACK(display), "Folder", 2);
    XfAddButtonVisual(b, 0,
                      XfCreateVisual(b, 0, 0, width, height, BLACK(display), WHITE(display), XfPixmapVisual, 1, map));
    XfAddButtonVisual(b, 1,
                      XfCreateVisual(b, 0, 0, width, height, WHITE(display), BLACK(display), XfPixmapVisual, 1, map));
    XfAddButtonCallback(b, 0, XF_CALLBACK(toggle_requestor_window), NULL);
    XfAddButtonCallback(b, 1, XF_CALLBACK(toggle_requestor_window), NULL);
    XfActivateButton(b, ButtonPressMask | ExposureMask);

    x += 30;
    map = Write_bits;

    b = XfCreateButton(display, B->window, x, y, width, height, 1, BLACK(display), "Write", 2);
    XfAddButtonVisual(b, 0,
                      XfCreateVisual(b, 0, 0, width, height, BLACK(display), WHITE(display), XfPixmapVisual, 1, map));
    XfAddButtonVisual(b, 1,
                      XfCreateVisual(b, 0, 0, width, height, WHITE(display), BLACK(display), XfPixmapVisual, 1, map));
    XfAddButtonCallback(b, 0, XF_CALLBACK(ActivateWrite), NULL);
    XfAddButtonCallback(b, 1, XF_CALLBACK(ActivateWrite), NULL);
    XfActivateButton(b, ButtonPressMask | ExposureMask);
    b->ext = font;

    x += 30;
    map = Help_bits;

    b = XfCreateButton(display, B->window, x, y, width, height, 1, BLACK(display), "Help", 1);
    XfAddButtonVisual(b, 0,
                      XfCreateVisual(b, 0, 0, width, height, BLACK(display), WHITE(display), XfPixmapVisual, 1, map));
    XfActivateButton(b, ExposureMask);

    x = 5;
    y += 30;

    width = 40;
    height = 25;
    map = "QUIT";

    b = XfCreateButton(display, B->window, x, y, width, height, 1, BLACK(display), "QUIT", 1);
    XfAddButtonVisual(b, 0, XfCreateVisual(b, 0, 8, 0, 0, BLACK(display), WHITE(display), XfTextVisual, map, font, 0));
    XfAddButtonCallback(b, 0, XF_CALLBACK(quit), NULL);
    XfActivateButton(b, ExposureMask | ButtonPressMask);
    b->ext = 0;

    x += width + 5;
    width = 40;
    height = 25;
    map = "REDRAW";

    b = XfCreateButton(display, B->window, x, y, width, height, 1, BLACK(display), "Redisplay", 2);
    XfAddButtonVisual(b, 0, XfCreateVisual(b, 0, 8, 0, 0, BLACK(display), WHITE(display), XfTextVisual, map, font, 0));
    XfAddButtonVisual(b, 1,
                      XfCreateVisual(b, 0, 8, 0, 0, BLACK(display), pwBackground.pixel, XfTextVisual, "WAIT", font, 0));
    XfAddButtonCallback(b, 0, XF_CALLBACK(Redisplay), NULL);
    XfActivateButton(b, ExposureMask | ButtonPressMask);

    return (0);
}

int scroll_button(Display *display, Window w)
{
    Button b;
    int x, y, width, height;

    x = 115;
    y = 10;
    width = 120;
    height = 120;

    JS = XfCreateJoystick(display, w, x, y, width, height, 1, BLACK(display), "JS1", 15, 15);
    XfAddJoystickFieldVisual(
        JS, XfCreateVisual(JS, 0, 0, 0, 0, (unsigned long)WHITE(display), WHITE(display), XfSolidVisual));
    XfAddJoystickThumbVisual(JS, XfCreateVisual(JS, 0, 0, 15, 15, pwHilite.pixel, WHITE(display), XfOutlineVisual));
    XfAddJoystickCallback(JS, XF_CALLBACK(pan_image), NULL);
    XfActivateJoystick(JS, (ExposureMask | ButtonPressMask | ButtonMotionMask));

    return (0);
}

void Redisplay(Button B, XEvent *E)
{
    int ncolors, j;

    if (CC->image == NULL)
        return; /*Modified 9/15/99 */

    B->state = 1;
    UpdateButton(B);

    update_display(CC->display, CC->image);
    LoadImageWindow(CC->image);

    ncolors = CC->image->ncolors;
    for (j = 0; j < NIMAGE; j++) {
        if (CC->image->overlays[j]) {
            default_map(Images[j]);
            RGB_CS(CC->display, CC->ColorMap, &(Images[j]->C1), &(Images[j]->C2), Images[j]->ncolors,
                   Colors + ncolors + 2, CC->SpreadSpace->state, Images[j]->map);

            ncolors += Images[j]->ncolors;
        }
    }
    LoadDnList(CC->image_index);
    B->state = 0;
    UpdateButton(B);
}

int LoadDnList(int i)
{
    int ncolors, j;

    ClearDnList();

    if (Images[i]->composite == 0) {
        ncolors = Images[i]->ncolors;
        AddToDnList(i, &(Colors[2 + ncolors - 1]));
        for (j = 0; j < NIMAGE; j++) {
            if (Images[i]->overlays[j]) {
                ncolors += Images[j]->ncolors;
                AddToDnList(j, &(Colors[ncolors + 2 - 1]));
            }
        }
    } else {
        for (j = 0; j < NIMAGE; j++) {
            switch (Images[i]->rgb[j]) {
            case 0:
                continue;
                break;
            case 1:
                AddToDnList(j, &pwRed);
                break;
            case 2:
                AddToDnList(j, &pwGreen);
                break;
            case 3:
                AddToDnList(j, &pwBlue);
                break;
            }
        }
    }
    ActivateDnList();

    return (0);
}

int iwlast = -1;
int ihlast = -1;

int MaximizeImage(Display *display)
{
    unsigned int w, h;
    Window root;
    int x, y;
    unsigned int width, height, bw, depth;
    int iheight, iwidth;
    int dwidth, dheight;

    int screen = DefaultScreen(display);

    if (CC->image != NULL) {
        iwidth = CC->image->subset.width;
        iheight = CC->image->subset.height;

        dwidth = DisplayWidth(display, screen);
        dheight = DisplayHeight(display, screen);

        XGetGeometry(display, IWindow, &root, &x, &y, &w, &h, &bw, &depth);

        if (ihlast != -1 && iwlast != -1) {
            width = iwlast;
            height = ihlast;
            iwlast = -1;
            ihlast = -1;
        } else {
            iwlast = w;
            ihlast = h;

            width = dwidth;
            height = dheight;
            if (width > iwidth)
                width = iwidth;
            if (height > iheight)
                height = iheight;
        }
        XResizeWindow(display, IWindow, width, height);
    }

    return (0);
}

int load_stretch(void)
{
    AMap a;

    char buf[16];
    float s_low, s_high;
    float c_low, c_high;
    float f_low, f_high;

    if (CC->image != NULL && CC->image->composite == 0) {

        s_low = CC->image->s_low;
        s_high = CC->image->s_high;

        c_low = CC->image->c_low;
        c_high = CC->image->c_high;

        sprintf(buf, "%5.5g", s_low);
        SetButtonText(CC->StretchLow, buf);

        sprintf(buf, "%5.5g", s_high);
        SetButtonText(CC->StretchHigh, buf);

        if (s_high != s_low) {
            f_high = (float)(c_high - s_low) / (float)(s_high - s_low);
            f_low = (float)(c_low - s_low) / (float)(s_high - s_low);
        } else {
            f_high = f_low = 0;
        }

        XfSetSliderValue(CC->Top, f_low);
        XfSetSliderValue(CC->Bottom, f_high);

        a = CC->Map;
        a->shade_left = (int)((float)130 * f_low);
        a->shade_right = (int)((float)130 * (1.0 - f_high));
        (*(a->exposeCallback))(a, NULL);

        sprintf(buf, "%5g", c_high);
        SetButtonText(CC->ScrollHigh, buf);

        sprintf(buf, "%5g", c_low);
        SetButtonText(CC->ScrollLow, buf);
        colorspread(CC);
    }

    return (0);
}
