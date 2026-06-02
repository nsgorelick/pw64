/*
 * sampler.c — interactive demo of libXfred widgets.
 *
 * Build: make -C lib sampler
 * Run:   ./lib/sampler   (requires DISPLAY)
 */

#include "Xfred.h"
#include <stdarg.h>
#include <stdint.h>
#include <X11/keysym.h>

#define PANEL_W 700
#define PANEL_H 520

static Display *display;
static int screen;
static GC gc;
static XFontStruct *font;
static Colormap cmap;
static unsigned long fg, bg, gray, hilite;
static Button shell;
static Button status_btn;
static Button destroy_pending;
static int quitting;
static AMap sampler_amap;
#define N_AMAP_MODES 3
static Button amap_mode_btn[N_AMAP_MODES];

static char *list_items[] = {
    "alpha",
    "beta",
    "gamma",
    "delta",
};

static void set_status(const char *fmt, ...)
{
    char buf[256];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    SetButtonText(status_btn, buf);
}

static void quit_cb(Button B, XEvent *E)
{
    (void)B;
    (void)E;
    quitting = 1;
}

static void confirm_cb(Button B, XEvent *E)
{
    int pick;

    (void)B;
    pick = ConfirmRequestor(display, RootWindow(display, screen), gc, font, (int)E->xbutton.x_root,
                            (int)E->xbutton.y_root, 200, 90, 0, 1, 2, 1, "Confirm dialog OK?", 2, "Cancel", "OK");
    set_status("Confirm returned %d", pick);
}

static void gettext_cb(Button B, XEvent *E)
{
    char buf[256];

    if (GetText(B, E, buf, (int)sizeof(buf), 1) == 1)
        set_status("GetText: \"%s\"", buf);
    else
        set_status("GetText cancelled");
}

static void pb_cb(void *widget, XEvent *E)
{
    (void)widget;
    (void)E;
    set_status("PushButton clicked");
}

static void cb_cb(void *widget, XEvent *E)
{
    CButton cb = (CButton)widget;

    (void)E;
    set_status("CheckButton %s", XfActiveCB(cb) ? "on" : "off");
}

static void rb_cb(void *widget, XEvent *E)
{
    RButton rb = (RButton)widget;

    (void)E;
    set_status("RadioButton #%d", XfWhichRB(rb));
}

static void mb_cb(void *widget, XEvent *E)
{
    MButton mb = (MButton)widget;
    char *text;

    (void)E;
    text = XfMBSelectedText(mb);
    set_status("Menu: %s", text ? text : "(none)");
}

static void toggle_cb(Button B, XEvent *E)
{
    (void)E;
    set_status("%s state %d", B->name, B->state);
}

static void slider_cb(Slider S, XEvent *E)
{
    (void)E;
    set_status("Slider \"%s\" = %.2f", S->name, XfGetSliderValue(S));
}

static void joystick_cb(Joystick J, XEvent *E)
{
    (void)E;
    set_status("Joystick %d,%d - %d,%d", XfJoystickUpperX(J), XfJoystickUpperY(J), XfJoystickLowerX(J),
               XfJoystickLowerY(J));
}

static void list_cb(void *widget, XEvent *E)
{
    List list = (List)widget;

    (void)E;
    if (list->selected >= 0 && list->selected < list->nitems)
        set_status("List selected: %s", list->items[list->selected]);
}

static void composite_load_cb(void *widget, XEvent *E)
{
    Composite C = (Composite)widget;

    (void)E;
    set_status("Composite load: %s", C->current_text ? C->current_text : "(empty)");
}

static void amap_mode_cb(Button B, XEvent *E)
{
    static const char *mode_name[] = {"off", "add", "delete", "move", "slide"};
    int mode = (int)(intptr_t)B->ext;
    int i;

    (void)E;
    if (sampler_amap == NULL)
        return;

    for (i = 0; i < N_AMAP_MODES; i++) {
        if (amap_mode_btn[i] != NULL && amap_mode_btn[i] != B && amap_mode_btn[i]->state != 0) {
            set_state(amap_mode_btn[i], 0);
        }
    }
    if (B->state != 1)
        set_state(B, 1);

    XfSetAMapAction(sampler_amap, mode);
    if (mode >= 0 && mode <= XfAMapSlide)
        set_status("AMap: %s mode - drag in map", mode_name[mode]);
}

static void amap_mode_btn_setup(Button B)
{
    XfDelButtonCallback(B, 0, XF_CALLBACK(toggle_state));
    XfDelButtonCallback(B, 1, XF_CALLBACK(toggle_state));
    XfAddButtonCallback(B, 0, XF_CALLBACK(amap_mode_cb), NULL);
    XfAddButtonCallback(B, 1, XF_CALLBACK(amap_mode_cb), NULL);
}

static Slider add_slider(Display *d, Window parent, int x, int y, int w, int h, int orient, char *name, CallBack cb)
{
    Slider S;
    int thumb_m = (orient == XfSliderUpDown) ? w : h;

    S = XfCreateSlider(d, parent, x, y, w, h, 1, BLACK(d), name, orient, 0.0f, 100.0f, 1.0f, thumb_m, thumb_m);
    XfAddSliderBarVisual(S, XfCreateVisual(S, 0, 0, 0, 0, BLACK(d), WHITE(d), XfStippledVisual, "\252\125", 2, 2));
    XfAddSliderThumbVisual(S, XfCreateVisual(S, 0, 0, thumb_m, thumb_m, hilite, WHITE(d), XfSolidVisual));
    XfAddSliderCallback(S, cb, NULL);
    XfActivateSliderValue(S, 50.0f, ExposureMask | ButtonPressMask | ButtonMotionMask);
    return S;
}

static int init_display(void)
{
    XColor c;

    if (!initx(NULL, &display, &screen, NULL, &gc))
        return 0;

    cmap = DefaultColormap(display, screen);
    fg = BLACK(display);
    bg = WHITE(display);
    gray = XfColor(display, "#C0C0C0");
    hilite = XfColor(display, "#0000FF");

    font = XLoadQueryFont(display, "7x13");
    if (font == NULL)
        font = XLoadQueryFont(display, "fixed");
    if (font == NULL) {
        fprintf(stderr, "sampler: no font\n");
        return 0;
    }
    XSetFont(display, gc, font->fid);
    XfSetDefaultFont(display, font);
    /* Prime XV_COLORS cache (macros call XfColor(NULL, ...)) */
    (void)XfColor(display, "#C6D5E2");
    (void)XfColor(display, "#8B99B5");
    (void)XfColor(display, "#B2C0DC");

    c.flags = DoRed | DoGreen | DoBlue;
    if (XParseColor(display, cmap, "#E8E8E8", &c) && XAllocColor(display, cmap, &c))
        bg = c.pixel;

    return 1;
}

static void build_ui(void)
{
    Window panel;
    Button B;
    PButton pb;
    CButton cb;
    RButton rb;
    LButton lb;
    MButton mb;
    MenuItem *menu;
    Slider sh, sv;
    Joystick joy;
    AMap amap;
    List list;
    Composite comp;
    int y;

    shell =
        XFCreateButton(display, RootWindow(display, screen), 30, 30, PANEL_W + 4, PANEL_H + 4, 2, fg, bg, "sampler", 1);
    XfAddButtonVisual(shell, 0, XfCreateVisual(shell, 0, 0, 0, 0, bg, bg, XfSolidVisual));
    XfActivateButton(shell, ExposureMask);
    panel = shell->window;

    status_btn = XFCreateButton(display, panel, 8, PANEL_H - 36, PANEL_W - 16, 24, 1, fg, bg, "status", 1);
    XfAddButtonVisual(status_btn, 0, XfCreateVisual(status_btn, 0, 0, 0, 0, bg, bg, XfSolidVisual));
    XfAddButtonVisual(
        status_btn, 0,
        XfCreateVisual(status_btn, 4, 4, 0, 0, fg, bg, XfTextVisual, "Ready - click widgets to test.", font, 1));
    XfActivateButton(status_btn, ExposureMask);

    y = 8;
    pb = XfCreatePB(display, panel, 8, y, 72, 26, XV_COLORS, LEFTTEXT("Push"), XF_CALLBACK(pb_cb));
    XfActivatePB(pb, ExposureMask | ButtonPressMask | ButtonReleaseMask);

    cb = XfCreateCB(display, panel, 88, y, 72, 26, XV_COLORS, LEFTTEXT("Check"), XF_CALLBACK(cb_cb));
    XfActivateCB(cb, ExposureMask | ButtonPressMask | ButtonReleaseMask);

    rb = NULL;
    rb = XfCreateRB(display, panel, 168, y + 4, 56, 22, XV_COLORS, LEFTTEXT("1"), XF_CALLBACK(rb_cb), rb);
    rb = XfCreateRB(display, panel, 230, y + 4, 56, 22, XV_COLORS, LEFTTEXT("2"), XF_CALLBACK(rb_cb), rb);
    rb = XfCreateRB(display, panel, 292, y + 4, 56, 22, XV_COLORS, LEFTTEXT("3"), XF_CALLBACK(rb_cb), rb);
    XfActivateRBList(rb, 0);

    lb = XfCreateLB(display, panel, 358, y, 80, 26, XV_COLORS, LEFTTEXT("Label"));
    XfActivateLB(lb, ExposureMask);

    menu = XfAddMenuItem(NULL, "Menu", CENTER, font, (Pixmap)0, 0, 0, NULL);
    XfAddMenuItem(menu, "Open", CENTER, font, (Pixmap)0, 0, 0, NULL);
    XfAddMenuItem(menu, "Save", CENTER, font, (Pixmap)0, 0, 0, NULL);
    XfAddMenuItem(menu, "Close", CENTER, font, (Pixmap)0, 0, 0, NULL);
    mb = XfCreateMB(display, panel, 448, y, 80, 26, XV_COLORS, menu, XF_CALLBACK(mb_cb));
    XfActivateMB(mb, ExposureMask | ButtonPressMask | ButtonReleaseMask);

    y = 44;
    B = Make2State(display, panel, font, 8, y, 70, 24, 1, fg, fg, bg, "Toggle");
    XfAddButtonCallback(B, 0, XF_CALLBACK(toggle_cb), NULL);
    XfAddButtonCallback(B, 1, XF_CALLBACK(toggle_cb), NULL);
    XfActivateButton(B, ExposureMask | ButtonPressMask);

    B = Make2State3D(display, panel, font, 86, y, 70, 24, 1, fg, fg, bg, "3D");
    XfAddButtonCallback(B, 0, XF_CALLBACK(toggle_cb), NULL);
    XfAddButtonCallback(B, 1, XF_CALLBACK(toggle_cb), NULL);
    XfActivateButton(B, ExposureMask | ButtonPressMask);

    B = XFCreateButton(display, panel, 170, y, 160, 36, 1, fg, bg, "hershey", 1);
    XfAddButtonVisual(B, 0, XfCreateVisual(B, 0, 0, 0, 0, bg, bg, XfSolidVisual));
    XfAddButtonVisual(B, 0,
                      XfCreateVisual(B, 12, 28, 0, 0, fg, bg, XfHersheyVisual, "Xfred", ROMAN_SIMPLEX, 0.35, 0.0, 0.0));
    XfActivateButton(B, ExposureMask);

    y = 92;
    sh = add_slider(display, panel, 8, y, 220, 18, XfSliderLeftRight, "H-Slider", XF_CALLBACK(slider_cb));
    (void)sh;
    sv = add_slider(display, panel, 240, y, 18, 100, XfSliderUpDown, "V-Slider", XF_CALLBACK(slider_cb));
    (void)sv;

    joy = XfCreateJoystick(display, panel, 270, y, 110, 110, 1, BLACK(display), "joy", 14, 14);
    XfAddJoystickFieldVisual(joy, XfCreateVisual(joy, 0, 0, 0, 0, bg, bg, XfSolidVisual));
    XfAddJoystickThumbVisual(joy, XfCreateVisual(joy, 0, 0, 14, 14, hilite, hilite, XfSolidVisual));
    XfAddJoystickCallback(joy, XF_CALLBACK(joystick_cb), NULL);
    XfActivateJoystickValue(joy, 40, 40, ExposureMask | ButtonPressMask | ButtonMotionMask | ButtonReleaseMask);

    y = 188;
    amap_mode_btn[0] = Make2State3D(display, panel, font, 8, y, 52, 20, 1, fg, fg, bg, "Move");
    amap_mode_btn[0]->ext = (void *)(intptr_t)XfAMapMove;
    amap_mode_btn_setup(amap_mode_btn[0]);
    XfActivateButton(amap_mode_btn[0], ExposureMask | ButtonPressMask);

    amap_mode_btn[1] = Make2State3D(display, panel, font, 64, y, 52, 20, 1, fg, fg, bg, "Add");
    amap_mode_btn[1]->ext = (void *)(intptr_t)XfAMapAdd;
    amap_mode_btn_setup(amap_mode_btn[1]);
    XfActivateButton(amap_mode_btn[1], ExposureMask | ButtonPressMask);

    amap_mode_btn[2] = Make2State3D(display, panel, font, 120, y, 52, 20, 1, fg, fg, bg, "Slide");
    amap_mode_btn[2]->ext = (void *)(intptr_t)XfAMapSlide;
    amap_mode_btn_setup(amap_mode_btn[2]);
    XfActivateButton(amap_mode_btn[2], ExposureMask | ButtonPressMask);

    set_state(amap_mode_btn[0], 1);

    y = 212;
    amap = XfCreateAMap(display, panel, 8, y, 150, 150, 1, BLACK(display), "amap", hilite, gray, FillSolid, (Pixmap)0);
    sampler_amap = amap;
    XfAddAMapVisual(amap, XfCreateVisual(amap, 0, 0, 0, 0, bg, bg, XfSolidVisual));
    /*
     * XfActivateAMap() forces action_mode to NoAction; use ActivateAMapMode
     * so XfClearAMap runs and the transfer curve is visible/interactive.
     */
    XfActivateAMapMode(amap, XfAMapMove, ExposureMask | ButtonPressMask | ButtonReleaseMask | ButtonMotionMask);

    /* fast=0: draw directly; fast=1 uses depth-8 pixmaps (BadMatch on TrueColor) */
    list = CreateList(display, panel, font, 170, y, 200, 130, 16, 0, hilite, 4, list_items);
    AddListCallback(list, XF_CALLBACK(list_cb));
    ActivateList(list);

    y = 352;
    comp = CreateComposite(display, panel, font, 8, y, 260, 22, (short)hilite, "type filename here");
    AddCompositeCallback(comp, XF_CALLBACK(composite_load_cb));
    ActivateComposite(comp);
    AddToComposite(comp, "sample.txt");
    AddToComposite(comp, "other.dat");

    y = 382;
    B = Make2State3D(display, panel, font, 8, y, 90, 24, 1, fg, fg, bg, "Confirm");
    XfAddButtonCallback(B, 0, XF_CALLBACK(confirm_cb), NULL);
    XfActivateButton(B, ExposureMask | ButtonPressMask);

    B = XFCreateButton(display, panel, 106, y, 90, 24, 1, fg, bg, "gettext", 1);
    XfAddButtonVisual(B, 0, XfCreateVisual(B, 0, 0, 0, 0, bg, bg, XfSolidVisual));
    XfAddButtonVisual(B, 0, XfCreateVisual(B, 4, 4, 0, 0, fg, bg, XfTextVisual, "GetText...", font, 1));
    B->ext = (char *)1;
    XfAddButtonCallback(B, 0, XF_CALLBACK(gettext_cb), NULL);
    XfActivateButton(B, ExposureMask | ButtonPressMask | KeyPressMask);

    B = Make2State3D(display, panel, font, PANEL_W - 98, y, 80, 24, 1, fg, fg, bg, "Quit");
    XfAddButtonCallback(B, 0, XF_CALLBACK(quit_cb), NULL);
    XfActivateButton(B, ExposureMask | ButtonPressMask);
}

int main(int argc, char **argv)
{
    XEvent E;

    (void)argc;
    (void)argv;

    if (!init_display()) {
        fprintf(stderr, "sampler: cannot open display\n");
        return 1;
    }

    build_ui();
    set_status("Sampler ready - AMap: Move mode, drag the blue curve");

    while (!quitting) {
        XNextEvent(display, &E);
        if (destroy_pending != NULL) {
            XfDestroyButton(destroy_pending);
            destroy_pending = NULL;
        }
        XfButtonPush(XfEventButton(&E), &E);
        XfSliderResponse(XfEventSlider(&E), &E);
        XfAMapAction(XfEventAMap(&E), &E);
        XfJoystickResponse(XfEventJoystick(&E), &E);
        {
            XButton xb = XfEventXB(&E);
            if (xb != NULL)
                (void)XfPushXB(xb, &E);
        }
    }

    return 0;
}
