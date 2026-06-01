#include <stdint.h>

#include "Xfred.h"
#include "XB.h"

static int depth(char *str);
void MBActivatePopup(MButton MB);
void RedrawMB(MButton MB);
void MBDeactivatePopup(MButton MB);

/**
 ** XfAddMenuItem:
 **
 ** Append an item to a menu
 **/

int HitTest(MButton MB, MenuItem * menu, int x, int y);
extern int Xf3DHeight(void);
extern void Draw3DBox(Display * disp, Window win, GC gc, int x, int y, int w, int h, int r, short int hi, short int lo,
                      short int fg, short int bg, int state, int options);

MenuItem *XfAddMenuItem(MenuItem *parent, char *str, int align, XFontStruct *font, Pixmap pixmap, int p_width,
                        int p_height, void *id)
{
    MenuItem *new;

    if (parent == NULL) {
        new = calloc(1, sizeof(MenuItem));
    } else {
        if (parent->items != NULL) {
            parent->items = realloc(parent->items, sizeof(MenuItem) * (parent->nitems + 1));
        } else {
            parent->items = calloc(1, sizeof(MenuItem));
        }
        new = &(parent->items[parent->nitems]);
        parent->nitems++;
    }

    new->str = strdup(str);
    new->align = align;
    new->font = font;
    /* if (font == NULL && str != NULL) font = _xfFontStruct; */
    new->pixmap = pixmap;
    new->p_width = p_width;
    new->p_height = p_height;
    new->id = id;

    return (new);
}

/**
 ** Put the current item into the beginning of a menuItem
 ** Put any children into ->items
 **/

MenuItem *StringToMenu(char *string, MenuItem *parent, int idval)
{
    int level;
    char *p;
    char *q;
    int top;
    static int id = 0;

    if (idval != 0)
        id = idval;
    if (string == NULL)
        return (NULL);
    if (parent == NULL) {
        p = string;
        if ((q = strchr(p, '\n')) != NULL) {
            *q = '\0';
            q++;
        }
        parent = calloc(1, sizeof(MenuItem));
        parent->str = strdup(p);
        parent->id = (void *) (intptr_t) id++;
        StringToMenu(q, parent, 0);
        return (parent);
    } else {
        p = string;
        top = depth(p);

        while (p && *p && (level = depth(p)) >= top) {
            if (level > top) {
                q = p;
                while (q && depth(q) > top) {
                    if ((q = strchr(q, '\n')) != NULL)
                        q++;
                }
                StringToMenu(p, &(parent->items[parent->nitems - 1]), 0);
            } else if (level == top) {
                if ((q = strchr(p, '\n')) != NULL) {
                    *q = '\0';
                    q++;
                }
                if (parent->items != NULL) {
                    parent->items = realloc(parent->items, sizeof(MenuItem) * (parent->nitems + 1));
                } else {
                    parent->items = calloc(1, sizeof(MenuItem));
                }
                parent->items[parent->nitems].str = strdup(p + top);
                parent->items[parent->nitems].id = (void *) (intptr_t) id++;
                parent->nitems++;
            } else {
                printf("something bad\n");
                if ((q = strchr(p, '\n')) != NULL) {
                    q++;
                }
            }
            p = q;
        }
    }
    return (parent);
}

static int depth(char *str)
{
    int i;
    for (i = 0; str && *str && isspace(*str); str++) {
        i++;
    }
    return (i);
}

/**
 ** print a text equivalent, showing the menu hierarchy
 **/

void print_menu(MenuItem *parent, int level)
{
    int i;
    printf("%*s%s\t%d\n", level * 4, "", parent->str, (int) (intptr_t) parent->id);
    for (i = 0; i < parent->nitems; i++) {
        print_menu(&(parent->items[i]), level + 1);
    }
}

MButton
XfCreateMB(Display *d, Window win, int x, int y, int w, int h, int hi, int lo, int fg, int bg, MenuItem *menu,
           CallBack func)
{
    MButton new;

    new = (MButton) XfCreateXB(d, win, x, y, w, h, hi, lo, fg, bg, func, XF_MB);
    new->menu = menu;
    return (new);
}

/**
 ** XfPushMB    - Handle user events
 **/

int XfPushMB(MButton MB, XEvent *E)
{
    Display *display;
    XEvent Ev;
    MButton parent;
    int i;

    if (MB == NULL)
        return (False);
    display = MB->display;

    switch (E->type) {
    case Expose:
        while (XCheckTypedWindowEvent(display, MB->window, Expose, &Ev));
        RedrawMB(MB);
        break;

    case ButtonPress:
        if (MB->state >= 0) {
            MBActivatePopup(MB);
        }
        break;

    case ButtonRelease:        /* parent never sees release */
        if (MB->state >= 0) {
            parent = MB->root;
            parent->selected = HitTest(MB, MB->menu, E->xbutton.x, E->xbutton.y);
            MBDeactivatePopup(MB);
            if (parent->selected != -1 && parent->function) {
                (*(parent->function)) (parent, E);
            }
        }
        break;

    case EnterNotify:
        break;

    case LeaveNotify:
        break;

    case ConfigureNotify:
        break;

    case MotionNotify:
        while (XCheckTypedWindowEvent(display, MB->window, MotionNotify, &Ev));
        i = HitTest(MB, MB->menu, E->xmotion.x, E->xmotion.y);
        if (i != MB->selected) {
            MB->selected = -1;
            if (i >= 0) {
                MB->selected = i;
            }
            XClearWindow(MB->display, MB->window);
            RedrawMB(MB);
        }
        break;
    }
    return 1;
}

int HitTest(MButton MB, MenuItem *menu, int x, int y)
{
    (void) x;
    int i;
    int last;
    int height;

    height = MB->root->height + 2;
    last = height;
    for (i = 0; i < menu->nitems; i++) {
        if (menu->items[i].str)
            height += menu->font->ascent + menu->font->descent + 2;
        else
            height += 5;

        if (y <= height && y > last) {
            if (menu->items[i].str)
                return (i);
            else
                break;
        }
    }
    return (-1);
}

/**
 **
 **/
void MBActivatePopup(MButton MB)
{
    Display *d = MB->display;
    XSetWindowAttributes xwa;
    int x, y, width, height;
    Window child;
    int screen;
    Window grab;
    int i;

    /**
     ** figure out where this button is located on the screen 
     **/
    XTranslateCoordinates(d, MB->window, RootWindow(d, DefaultScreen(d)), MB->width / 2, 0, &x, &y, &child);
    width = MB->width;
    height = MB->height + 2;

    /**
     ** calculate height 
     **/

    for (i = 0; i < MB->menu->nitems; i++) {
        if (MB->menu->items[i].str)
            height += MB->menu->font->ascent + MB->menu->font->descent + 2;
        else
            height += 5;
    }

    /**
     ** modify our position to fit on the screen
     **/
    x -= MAX(width / 2, MB->width / 2);
    if (x >= 0) {
        int scr_width = DisplayWidth(d, DefaultScreen(d));
        if (x + width > scr_width)
            x = scr_width - width;
    }
    if (x < 0)
        x = 0;

    if (y >= 0) {
        int scr_height = DisplayHeight(d, DefaultScreen(d));
        if (y + height > scr_height)
            y = scr_height - height;
    }
    if (y < 0)
        y = 0;

    /**
     ** create a window on the root.
     **/

    screen = DefaultScreen(d);
    xwa.override_redirect = True;
    xwa.background_pixmap = None;
    xwa.border_pixel = BLACK(d);
    xwa.save_under = True;
    grab = XCreateWindow(d, RootWindow(d, screen),
                         x - 1, y - 1, width - 3, height,
                         2, 0, InputOutput, CopyFromParent,
                         CWBackPixmap | CWOverrideRedirect | CWSaveUnder | CWBorderPixel, &xwa);
    XDefineCursor(d, grab, XCreateFontCursor(d, 132));

    MB->grab = grab;

    MB->popup = XfCreateMB(d, grab, 0, 0, width - 3, height, DEFCOLORS, MB->menu, NULL);
    MB->popup->root = MB;
    MB->popup->selected = -1;
    XSelectInput(d, MB->popup->window,
                 ExposureMask | ButtonPressMask | ButtonReleaseMask |
                 EnterWindowMask | LeaveWindowMask | StructureNotifyMask | ButtonMotionMask);
    XfActivateMB(MB->popup, 4);

    XMapRaised(d, grab);
    XFlush(d);

    XUngrabPointer(d, CurrentTime);
    XGrabPointer(d, MB->popup->window, False,
                 ButtonPressMask | ButtonReleaseMask |
                 EnterWindowMask | LeaveWindowMask |
                 ButtonMotionMask, GrabModeAsync, GrabModeAsync, None, None, CurrentTime);
}

/**
 ** Deactivate popup window.  Destroy all the buttons in it.
 **/
void MBDeactivatePopup(MButton MB)
{
    if (MB->root)
        MB = MB->root;

    XUngrabPointer(MB->display, CurrentTime);

    XfDeactivateMB(MB->popup);
    XfDestroyMB(MB->popup);
    XUnmapWindow(MB->display, MB->grab);
    XDestroyWindow(MB->display, MB->grab);

}

/**
 ** Execute the specified user callback.
 **/
void MBUserCallback(MButton MB)
{
    (void) MB;
}

/**
 ** Set MB to be redrawn as raised, and call MBActivatePopup for submenus
 **/
void RaiseMB(MButton MB)
{
    (void) MB;
}

/**
 ** Set MB to be redrawn as lowered, and call MBDeactivatePopup for submenus
 **/
void LowerMB(MButton MB)
{
    (void) MB;
}

void draw_MenuItem(MButton MB, MenuItem *menu, int xoff, int yoff, int height, int r, int invert)
{
    int dir, ascent, descent;
    int x = 0, y = 0;
    XCharStruct extents;
    char *str = menu->str;
    XFontStruct *font = menu->font;
    Display *display = MB->display;
    Window window = MB->window;
    GC gc = DefaultGC(display, DefaultScreen(display));

    if (height == 0)
        height = MB->height;

    if (menu->str != NULL) {
        XTextExtents(font, str, strlen(str), &dir, &ascent, &descent, &extents);

        y = (height + ascent - 2) / 2;

        if (menu->align == 0)
            x = (MB->width - extents.width + menu->p_width) / 2;
        else if (menu->align > 0)
            x = r + 2 + menu->p_width;
        else if (menu->align < 0)
            x = MB->width - (r + 2) - extents.width;

        XSetFont(MB->display, gc, font->fid);
        /**
        *** If inactive, its grayed out
        **/
        if (menu->status == -1) {
            XSetStipple(MB->display, gc, XfStipple(display, window, "gray"));
            XSetFillStyle(MB->display, gc, FillOpaqueStippled);
            XSetBackground(MB->display, gc, MB->bg);
        }
        if (invert && menu->status != -1) {
            XSetForeground(MB->display, gc, MB->fg);
            XFillRectangle(MB->display, window, gc, xoff, yoff, MB->width, height);
            XSetForeground(MB->display, gc, MB->bg);
        } else {
            XSetForeground(MB->display, gc, MB->fg);
        }
        XDrawString(MB->display, window, gc, x + xoff, y + yoff, str, strlen(str));
    }
    XSetFillStyle(MB->display, gc, FillSolid);

#if 0
    if (MB->menu->pixmap != 0) {
        int x, y;

        x = (MB->width - MB->pix_w) / 2;
        y = (height - MB->pix_h) / 2;

        XCopyPlane(display, MB->pixmap, window, gc, 0, 0, MB->pix_w, MB->pix_h, x, y, 1);
        if (MB->state == -1) {
            XSetStipple(display, gc, XfStipple(display, window, "gray"));
            XSetFillStyle(display, gc, FillStippled);
            XSetForeground(display, gc, MB->bg);
            XFillRectangle(display, window, gc, x, y, MB->pix_w, MB->pix_h);
        }
    }
#endif
}

void draw_separator(MButton MB, MenuItem *menu, int xoff, int yoff, int r)
{
    (void) menu;
    (void) r;
    Display *display = MB->display;
    Window window = MB->window;
    GC gc = DefaultGC(display, DefaultScreen(display));
    int width = MB->width;

    XSetForeground(display, gc, MB->lo);
    XDrawLine(display, window, gc, xoff, yoff + 2, width - xoff * 2, yoff + 2);
}

/**
 ** RedrawMB    - Redraw PushButton based on its state
 **
 **             - this function uses Draw3DBox.  If a buttons text or pixmap
 **               ever changes, Draw3DBox won't erase the old text/pixmap 
 **               unless the option (CLEAR) is passed as the last argument.
 **               The safe thing to do is ClearWindow and Expose on change.
 **/

void RedrawMB(MButton MB)
{
    Display *display = MB->display;
    Window window = MB->window;
    GC gc = DefaultGC(display, DefaultScreen(display));
    int width = MB->width;
    int height = MB->height;
    int r = Xf3DHeight();
    int state = MB->state;
    int t_ht;
    MenuItem *item;
    int i, h;

    if (state != 4) {
        Draw3DBox(display, window, gc, 0, 0, width - 2, height - 2, Xf3DHeight(), MB->hi, MB->lo, MB->fg, MB->bg, 0, 0);

        if (state == -1) {
            XSetStipple(display, gc, XfStipple(display, window, "gray"));
            XSetFillStyle(display, gc, FillOpaqueStippled);
        }
        XSetForeground(display, gc, MB->fg);
        XSetBackground(display, gc, MB->bg);

        XDrawLine(display, window, gc, 2, height - 2, width - 2, height - 2);
        XDrawLine(display, window, gc, width - 2, height - 2, width - 2, 2);
        XDrawLine(display, window, gc, 2, height - 1, width - 1, height - 1);
        XDrawLine(display, window, gc, width - 1, height - 1, width - 1, 2);

        XSetForeground(display, gc, MB->fg);
        XSetBackground(display, gc, MB->bg);
        draw_MenuItem(MB, MB->menu, 0, 0, MB->height, r, 0);

        XSetFillStyle(display, gc, FillSolid);
    } else {
        draw_MenuItem(MB, MB->root->menu, 0, 0, MB->root->height - 2, r, 0);
        draw_separator(MB, MB->menu, 0, MB->root->height - 2 - 3, r);
        draw_separator(MB, MB->menu, 0, MB->root->height - 2, r);
        h = MB->root->height + 2;
        for (i = 0; i < MB->menu->nitems; i++) {
            item = (MB->menu->items) + i;
            if (item->str) {
                t_ht = item->font->ascent + item->font->descent;
                if (i == MB->selected && item->status != -1) {
                    draw_MenuItem(MB, item, 0, h, t_ht + 2, r, 1);
                } else {
                    draw_MenuItem(MB, item, 0, h, t_ht + 2, r, 0);
                }
                h += t_ht + 2;
            } else {
                draw_separator(MB, item, 2, h, r);
                h += 5;
            }
        }
    }

    XFlush(display);
}

void *XfMBSelectedValue(MButton MB)
{
    if (MB->selected != -1)
        return (MB->menu->items[MB->selected].id);
    return (NULL);
}

char *XfMBSelectedText(MButton MB)
{
    if (MB->selected != -1)
        return (MB->menu->items[MB->selected].str);
    return (NULL);
}

void SetMenuText(MButton MB, MenuItem *new, char *str)
{
    if (new->str)
        free(new->str);
    new->str = strdup(str);

    XClearWindow(MB->display, MB->window);
    RedrawMB(MB);
}
