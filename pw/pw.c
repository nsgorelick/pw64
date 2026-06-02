#include "ColorControls.h"
#include "Xfred.h"
#include "color.h"
#include "composite.h"
#include "config.h"
#include "display_rgb.h"
#include "image.h"
#include "mag.h"
#include "setcolor.h"
#include <X11/Xlib.h>
#include <X11/Xos.h>
#include <X11/Xutil.h>
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <unistd.h>

#ifndef CORE
#include <sys/resource.h>
#include <sys/time.h>
#endif /* CORE */

#include "loguser.h"

Display *display;
int screen;
int depth;
Window w;
Window ControlWindow;
GC gc;
Button b[200];

XColor Colors[256];
int NColors;

char *cnames[6] = {"red", "green", "blue", "yellow", "magenta", "cyan"};

XColor pwRed, pwGreen, pwBlue, pwYellow, pwCyan, pwMagenta;
XColor pwBackground, pwHilite;

unsigned long PlaneMask[1];

XFontStruct *font;
Colormap ColorMap;
Image Images[NIMAGE];

Button CCWindow;

struct ColorControls *CC;
struct magnify *Mag;
Requestor requestor;

int IWidth = 300, IHeight = 300;
int IXPos = 0, IYPos = 0;
Window IWindow;
Pixmap IPixmap;
static GC IPixmapGC;
XImage *IImage;
int NoIPixmap = 0;

static void free_ipixmap_gc(void)
{
    if (IPixmapGC != NULL) {
        XFreeGC(display, IPixmapGC);
        IPixmapGC = NULL;
    }
}

static void create_ipixmap(Drawable drawable, int width, int height)
{
    int pd = pw_display_depth();

    free_ipixmap_gc();
    if (IPixmap != (Pixmap)NULL) {
        XFreePixmap(display, IPixmap);
        IPixmap = (Pixmap)NULL;
    }
    IPixmap = XCreatePixmap(display, drawable, width, height, pd);
    IPixmapGC = XCreateGC(display, IPixmap, 0, NULL);
    XSetForeground(display, IPixmapGC, BLACK(display));
    XFillRectangle(display, IPixmap, IPixmapGC, 0, 0, width, height);
}

int NoAutoStretch = 0;

void SelectImage(XEvent *E);

extern int load_pan(void);
void LoadImageWindow(Image new);
extern void SetMagnifyScale(struct magnify *Mag, XEvent *E);
extern int load_image(int i);
extern int ActivateMagnify(Display *display, struct magnify *Mag);
void MagBoxClear(void);
extern void UpdateReadout(int x, int y, Image new);
extern void loguser(char *log, char *text);
int init(void);
extern int set_errors(void);
void events(XEvent *E);
void alloc_colors(int recurse);
void MakePrivateColormap(void);
void MagBoxSet(void);
void CommandLineArgs(int ac, char **av, Requestor R);
extern int gadget_buttons(Display *display, Window w, XFontStruct *font);
extern int scroll_button(Display *display, Window w);
extern struct ColorControls *InitColorControls(Display *d, Window w, XFontStruct *font, Colormap ColorMap, int x, int y,
                                               char *name, XColor RedSliderColor, XColor GreenSliderColor,
                                               XColor BlueSliderColor);
extern int CreateAMap(struct ColorControls *AC, Display *display, Window w, XFontStruct *font, int x, int y,
                      long unsigned int hilite, long unsigned int shade);
extern int InitColorSpread(Display *display, Window w, XFontStruct *font, Colormap ColorMap, int x, int y,
                           struct ColorControls *CC);
extern int LoadColorSpread(struct ColorControls *CC, XColor *low, XColor *high, int ncolors, XColor *colors);
extern void CreateReadout(Display *display, Window parent, XFontStruct *font);
extern void CreateBlockPanel(Display *display, Window parent, XFontStruct *font);
extern void CreateRose(Display *display, XFontStruct *font);
extern void CreateSpecprRequestor(Display *display, XFontStruct *font);
extern int MagnifyCallback(struct magnify *Mag, XEvent *E);
extern char *get_version(void);
extern void read_restart(Display *display, char *path);
extern int UpdatePushButtons(Requestor R, int n);
extern int compute_subset(Requestor R, int i);
extern int create_image(Display *display, Image new);
extern int off_load_filename(int i, char *buf, Requestor R, int stretch, int colors, int band);

void iwindow_callback(XEvent *E)
{
    extern int AllocError;
    int width;
    int height;
    switch (E->type) {
    case Expose: {
        XRectangle rect;
        Region region;
        region = XCreateRegion();
        do {
            rect.x = (short)E->xexpose.x;
            rect.y = (short)E->xexpose.y;
            rect.width = (unsigned short)E->xexpose.width;
            rect.height = (unsigned short)E->xexpose.height;
            XUnionRectWithRegion(&rect, region, region);
        } while (XCheckTypedWindowEvent(E->xexpose.display, IWindow, Expose, E));

        XClipBox(region, &rect);
        if (IPixmap != (Pixmap)NULL) {
            XCopyArea(E->xexpose.display, IPixmap, IWindow, gc, rect.x, rect.y, rect.width, rect.height, rect.x,
                      rect.y);
        } else if (IImage != (XImage *)NULL) {
            XPutImage(E->xexpose.display, IWindow, gc, IImage, rect.x, rect.y, rect.x, rect.y, rect.width, rect.height);
        }
        XDestroyRegion(region);
        MagBoxSet();
        break;
    }
    case ConfigureNotify: {
        width = E->xconfigure.width;
        height = E->xconfigure.height;
        if (IWidth == width && IHeight == height)
            return;
        if (!NoIPixmap) {
            XCopyArea(E->xany.display, IPixmap, IWindow, gc, IXPos, IYPos, width, height, 0, 0);
            free_ipixmap_gc();
            XFreePixmap(E->xany.display, IPixmap);
            IPixmap = (Pixmap)NULL;
        }
        AllocError = 0;
        IPixmap = XCreatePixmap(E->xany.display, IWindow, width, height, pw_display_depth());
        XSync(E->xany.display, False);
        if (AllocError) {
            IPixmap = (Pixmap)NULL;
            NoIPixmap = 1;
        } else {
            free_ipixmap_gc();
            IPixmapGC = XCreateGC(display, IPixmap, 0, NULL);
            XSetForeground(display, IPixmapGC, BLACK(display));
            XFillRectangle(display, IPixmap, IPixmapGC, 0, 0, width, height);
            if (IImage != (XImage *)NULL) {
                XPutImage(display, IPixmap, IPixmapGC, IImage, IXPos, IYPos, 0, 0, width, height);
            }
        }
        IWidth = width;
        IHeight = height;
        if (IImage != (XImage *)NULL) {
            if ((width = CC->image->subset.width - IWidth) < IXPos)
                IXPos = (width < 0 ? 0 : width);
            if ((height = CC->image->subset.height - IHeight) < IYPos)
                IYPos = (height < 0 ? 0 : height);

            load_pan();
            XClearWindow(display, IWindow);
            LoadImageWindow(CC->image);
        }
        break;
    }
    case KeyPress: {
        int i;
        char b[2] = {0, 0};
        KeySym keysym = 0;
        XLookupString(&(E->xkey), b, 2, &keysym, NULL);

        if (b[0] == '+' || b[0] == '-' || b[0] == '*' || b[0] == '/' || (b[0] - '0' > 0 && b[0] - '0' <= 9)) {
            SetMagnifyScale(Mag, E);
        } else {
            i = b[0] - 'a';
            if (i < 0 || i >= NIMAGE)
                i = b[0] - 'A';
            if (i < 0 || i >= NIMAGE || Images[i] == NULL || Images[i]->sdata == NULL) {
                return;
            }
            load_image(i);
        }
        break;
    }
    case MapNotify:
    case UnmapNotify:
    case ReparentNotify:
        break;
    case ButtonPress: {
        int x, y;
        x = E->xbutton.x + IXPos;
        y = E->xbutton.y + IYPos;
        if (E->xbutton.button == Button1) {
            if (Mag->state == 0) {
                ActivateMagnify(E->xany.display, Mag);
            }
            if (CC->image != NULL) {
                MagBoxClear();
                SetMagnify(E->xany.display, CC->image, Mag, x, y);
                MagBoxSet();
            }
        } else if (E->xbutton.button == Button3) {
            if (CC->image != NULL) {
                int width, height, max;
                extern Joystick JS;

                width = CC->image->subset.width;
                height = CC->image->subset.height;
                max = (width > height ? width : height);

                IXPos = x - IWidth / 2;
                IYPos = y - IHeight / 2;
                IXPos = MAX(MIN(IXPos, width - IWidth), 0);
                IYPos = MAX(MIN(IYPos, height - IHeight), 0);
                LoadImageWindow(CC->image);

                JS->thumb_cur_x = JS->new_x = IXPos * JS->width / max;
                JS->thumb_cur_y = JS->new_y = IYPos * JS->height / max;
                (*(JS->exposeCallback))(JS, NULL);
            }
        }
        break;
    }
    case MotionNotify: {
        int x, y;

        if (CC->image == NULL)
            return;
        while (XCheckMaskEvent(display, PointerMotionMask, E))
            ;
        x = E->xbutton.x + IXPos;
        y = E->xbutton.y + IYPos;

        UpdateReadout(x, y, CC->image);
        break;
    }
    default: {
        if (E->type != NoExpose && E->type != GraphicsExpose)
            printf("whats this?  %d\n", E->type);
    }
    }
}

main(int argc, char **argv)
{
    struct VisualInfo *v[5];
    Button b;
    char *prog;
    XEvent Ev;
    extern int _Xdebug;

    _Xdebug = False;

    prog = argv[0];

    loguser(LOGFILE, NEWSFILE); /* loguser.h */

#ifndef CORE
#ifndef RLIMIT_CORE
#define RLIMIT_CORE 4
#endif /* RLIMIT_CORE */
    if (access("core", F_OK)) {
        struct rlimit r;
        r.rlim_cur = 0;
        r.rlim_max = 0;
        setrlimit(RLIMIT_CORE, &r);
    }
#endif /* CORE */

    init();
    set_errors();

    IWindow = XCreateSimpleWindow(display, RootWindow(display, screen), 100, 10, IWidth, IHeight, (unsigned long)1,
                                  BLACK(display), WHITE(display));
    XSetWindowColormap(display, IWindow, ColorMap);
    create_ipixmap(IWindow, IWidth, IHeight);
    NoIPixmap = 0;
    XSelectInput(display, IWindow,
                 (KeyPressMask | StructureNotifyMask | ExposureMask | ButtonPressMask | PointerMotionMask));

    gadget_buttons(display, w, font);
    scroll_button(display, w);

    CCWindow =
        XfCreateButton(display, w, 0, 150, IWidth, IHeight + 20, (unsigned long)0, WHITE(display), "CCWindow", 1);
    XfActivateButton(CCWindow, ExposureMask);

    CC = (struct ColorControls *)InitColorControls(display, CCWindow->window, font, ColorMap, 10, 10, "1", pwRed,
                                                   pwGreen, pwBlue);
    CreateAMap(CC, display, CCWindow->window, font, 10, 85, pwHilite.pixel, pwBackground.pixel);
    InitColorSpread(display, CCWindow->window, font, ColorMap, 10, 65, CC);

    LoadColorSpread(CC, &Colors[0], &Colors[1], 0, Colors + 2);

    requestor = CreateRequestor(display, RootWindow(display, screen), 10, 10, 1, BLACK(display), "Req", font);

    ActivateRequestor(requestor);

    CreateReadout(display, w, font);
    Mag = CreateMagnify(display, (Window)0);
    CreateBlockPanel(display, (Window)0, font);
    CreateRose(display, font);

#ifdef INTERNAL_SP
    CreateSpecprRequestor(display, font);
#endif

    CommandLineArgs(argc, argv, requestor);

    /* Sliders/lists may leave FillStippled on the screen default GC. */
    XSetFillStyle(display, DefaultGC(display, screen), FillSolid);

    while (1) {
        XNextEvent(display, &Ev);
        events(&Ev);
    }
}

void events(XEvent *E)
{
    XfButtonPush(XfEventButton(E), E);
    XfSliderResponse(XfEventSlider(E), E);
    XfAMapAction(XfEventAMap(E), E);
    XfJoystickResponse(XfEventJoystick(E), E);
    if (E->xany.window == IWindow)
        iwindow_callback(E);
    else if (E->xany.window == Mag->window)
        MagnifyCallback(Mag, E);
    else if (E->xany.window == ControlWindow)
        SelectImage(E);
}

#define PW_SW_NCOLORS 220

static int alloc_ui_color(unsigned short r, unsigned short g, unsigned short b)
{
    XColor c;

    c.red = r;
    c.green = g;
    c.blue = b;
    c.flags = DoRed | DoGreen | DoBlue;
    c.pixel = 0;
    if (!XAllocColor(display, ColorMap, &c))
        return 0;
    return (int)c.pixel;
}

void alloc_colors(int recurse)
{
    int j;
    int g;

    (void)recurse;

    NColors = PW_SW_NCOLORS;
    setcolor_lut(0x0000, 0x0000, 0x0000, Colors[0]);
    setcolor_lut(MAX_INTENSITY, MAX_INTENSITY, MAX_INTENSITY, Colors[1]);
    for (j = 0; j < NColors; j++) {
        g = (NColors > 1) ? (j * MAX_INTENSITY / (NColors - 1)) : 0;
        setcolor_lut((unsigned short)g, (unsigned short)g, (unsigned short)g, Colors[j + 2]);
    }

    pwRed.pixel = (unsigned long)alloc_ui_color(0xFFFF, 0x0000, 0x0000);
    pwGreen.pixel = (unsigned long)alloc_ui_color(0x0000, 0xCC00, 0x0000);
    pwBlue.pixel = (unsigned long)alloc_ui_color(0x3000, 0x3000, 0xFFFF);
    pwCyan.pixel = (unsigned long)alloc_ui_color(0x0000, 0xD800, 0xFFFF);
    pwMagenta.pixel = (unsigned long)alloc_ui_color(0xFFFF, 0x0000, 0xFFFF);
    pwYellow.pixel = (unsigned long)alloc_ui_color(0xFFFF, 0xBBBB, 0x0000);
    pwBackground.pixel = (unsigned long)alloc_ui_color(0xC000, 0xC000, 0xC000);
    pwHilite.pixel = (unsigned long)alloc_ui_color(0xFFFF, 0x4000, 0x4000);

    pwRed.flags = pwGreen.flags = pwBlue.flags = pwYellow.flags = DoRed | DoGreen | DoBlue;
    pwCyan.flags = pwMagenta.flags = pwBackground.flags = pwHilite.flags = DoRed | DoGreen | DoBlue;

    printf("software LUT budget: %d image colors (+ black/white slots)\n", NColors);
}

void MakePrivateColormap(void)
{
    XColor black, white;
    black.pixel = BlackPixel(display, screen);
    white.pixel = WhitePixel(display, screen);

    XQueryColor(display, ColorMap, &black);
    XQueryColor(display, ColorMap, &white);

    ColorMap = XCreateColormap(display, RootWindow(display, screen), DefaultVisual(display, screen), AllocNone);

    (void)XAllocColor(display, ColorMap, &black);
    (void)XAllocColor(display, ColorMap, &white);

    XInstallColormap(display, ColorMap);
}

init(void)
{
    char buf[32];

    if (!initx(NULL, &display, &screen, &depth, &gc)) {
        printf("aborting\n");
        exit(1);
    }
    pw_init_display(display, screen);
    ColorMap = DefaultColormap(display, screen);
    alloc_colors(0);

    font = XLoadQueryFont(display, "6x10");
    if (font == NULL) {
        fprintf(stderr, "Can't find font 6x10\nAborting.\n");
        exit(1);
    }
    XSetFont(display, gc, font->fid);

    w = XCreateSimpleWindow(display, RootWindow(display, screen), 10, 10, (unsigned int)255, (unsigned int)470,
                            (unsigned long)1, BLACK(display), WHITE(display));

    XSetWindowColormap(display, w, ColorMap);

    XSelectInput(display, w, KeyPressMask);
    ControlWindow = w;

    XStoreName(display, w, (char *)get_version());

    MapAndWait(display, w);
    return 0;
}

void LoadImageWindow(Image new)
{
    int width, height;
    XWindowAttributes wa;

    XGetWindowAttributes(display, IWindow, &wa);
    width = wa.width;
    height = wa.height;

    IImage = new->ximage;
    if (IImage == NULL)
        return;
    if (NoIPixmap) {
        if (new->pixmap == (Pixmap)NULL) {
            XPutImage(display, IWindow, gc, new->ximage, IXPos, IYPos, 0, 0, width, height);
        } else {
            XCopyArea(display, new->pixmap, IWindow, gc, IXPos, IYPos, width, height, 0, 0);
            IPixmap = new->pixmap;
        }
    } else {
        if (IPixmapGC != NULL) {
            XSetForeground(display, IPixmapGC, BLACK(display));
            XFillRectangle(display, IPixmap, IPixmapGC, 0, 0, IWidth, IHeight);
        }
        if (new->pixmap == (Pixmap)NULL) {
            XPutImage(display, IPixmap, IPixmapGC != NULL ? IPixmapGC : gc, new->ximage, IXPos, IYPos, 0, 0, width,
                      height);
        } else {
            XCopyArea(display, new->pixmap, IPixmap, IPixmapGC != NULL ? IPixmapGC : gc, IXPos, IYPos, width, height, 0,
                      0);
        }
        XCopyArea(display, IPixmap, IWindow, gc, 0, 0, width, height, 0, 0);
    }
    MagBoxSet();
}

void MagBoxClear(void)
{
    Image new;
    int x, y;
    int width, height;
    float scale;

    if (Mag->scale > 0) {
        scale = Mag->scale;
    } else {
        scale = -1.0 / Mag->scale;
    }

    width = Mag->width / scale;
    height = Mag->height / scale;
    x = Mag->x - IXPos - width / 2;
    y = Mag->y - IYPos - height / 2;

    if ((new = CC->image) == NULL)
        return;
    if (NoIPixmap) {
        if (new->pixmap == (Pixmap)NULL) {
            XPutImage(display, IWindow, gc, new->ximage, IXPos + x, IYPos + y, x, y, width + 1, height + 1);
        } else {
            XCopyArea(display, new->pixmap, IWindow, gc, IXPos + x, IYPos + y, width + 1, height + 1, x, y);
        }
    } else {
        XCopyArea(display, IPixmap, IWindow, gc, x, y, width + 1, height + 1, x, y);
    }
}

void MagBoxSet(void)
{
    Image new;
    int x, y;
    int width, height;
    float scale;

    if (Mag->state) {
        if (Mag->scale > 0) {
            scale = Mag->scale;
        } else {
            scale = -1.0 / Mag->scale;
        }
        width = Mag->width / scale;
        height = Mag->height / scale;
        x = Mag->x - IXPos - width / 2;
        y = Mag->y - IYPos - height / 2;

        XSetForeground(display, gc, pwHilite.pixel);
        XDrawRectangle(display, IWindow, gc, x, y, width, height);
    }
}

void CommandLineArgs(int ac, char **av, Requestor R)
{
    int i = 0, j;
    int textf;
    char *buffy[256];
    int r;
    int count = 0;
    int hs = 0;
    int colors = 128;
    char *fname;
    int band;
    int bands;

    for (i = 1; i < ac; i++) {
        if (av[i][0] == '-') {
            /* process option */
            if (!strncmp(av[i], "-r", 2)) { /* restart file */
                if (strlen(av[i]) == 2) {
                    i++;
                    fname = av[i];
                } else {
                    fname = av[i] + 2;
                }
                read_restart(display, fname);
                UpdateRequestor(requestor, 0);
                UpdatePushButtons(requestor, 0);
                compute_subset(requestor, 0); /* Added 9/9/99 */
                create_image(display, Images[0]);
                load_image(0);
                break;
            }
            if (!strncmp(av[i], "-s", 2)) { /* hard stretch */
                hs = atoi(av[i] + 2);
                if (strlen(av[i]) == 2) { /* skip to next arg */
                    i++;
                    hs = atoi(av[i]);
                }
                if (hs == 0) {
                    fprintf(stderr, "Error: -s requires a numeric value\n");
                }
            }

            if (!strncmp(av[i], "-c", 2)) { /* set ncolors */
                colors = atoi(av[i] + 2);
                if (strlen(av[i]) == 2) {
                    i++;
                    colors = atoi(av[i]);
                }
            }
            if (!strncmp(av[i], "-all", 4)) { /* load all bands */
                i++;
                if (av[i] != NULL) {
                    /* load once, then get info and load again */
                    if (count < NIMAGE) {
                        off_load_filename(count, av[i], R, hs, colors, 0);
                    } else {
                        printf("\nToo many images.  Ignoring: %s\n", av[i]);
                    }
                    bands = Images[count]->header.bands;
                    count++;
                    for (j = 1; j < bands; j++) {
                        if (count < NIMAGE) {
                            off_load_filename(count++, av[i], R, hs, colors, j);
                        }
                    }
                }
            }
        } else {
            /* process filename */
            if (count < NIMAGE) {
                printf("\nAutoloading %s\n", av[i]);
                off_load_filename(count++, av[i], R, hs, colors, -1);
            } else {
                printf("\nToo many images.  Ignoring: %s\n", av[i]);
            }
        }
    }
}

void SelectImage(XEvent *E)
{
    int i;
    char b[2] = {0, 0};
    KeySym keysym = 0;

    XLookupString(&(E->xkey), b, 2, &keysym, NULL);

    if (b[0] == '+' || b[0] == '-' || b[0] == '*' || b[0] == '/' || (b[0] - '0' > 0 && b[0] - '0' <= 9)) {
        SetMagnifyScale(Mag, E);
    } else {
        i = b[0] - 'a';
        if (i < 0 || i >= NIMAGE)
            i = b[0] - 'A';
        if (i < 0 || i >= NIMAGE || Images[i] == NULL || Images[i]->sdata == NULL) {
            return;
        }
        load_image(i);
    }
}
