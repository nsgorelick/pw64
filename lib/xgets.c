#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "Xfred.h"

#define min(a,b) (a < b ? a : b)
#define max(a,b) (a > b ? a : b)

static int xgets_clamp_anchor(int anchor, const char *str)
{
    int len = (int) strlen(str);

    if (anchor < 0)
        return 0;
    if (anchor > len)
        return len;
    return anchor;
}

#include <X11/Xos.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

int anchor;
int anchor2;

void insert_str(char *s, char *t, int at);
void file_completion(Display * display, char *str);
void decode_control(Display * display, KeySym k, char *str);

int do_keysym(Display * display, XEvent * event, char *str, int nchars);
extern void free(void *);
extern int complete_dir(char *str);

char *xgets(Display *display, Window window, int x, int y, int width, int height, long unsigned int fg,
            long unsigned int bg, XFontStruct *font, char *input, XEvent *e)
{
    Window w;
    int max_width;
    int offset;
    int done;
    int shift_width;
    int start_x, start_y;
    static char str[256];
    char s[256];
    GC gc;
    XWMHints wmhints;

    gc = DefaultGC(display, DefaultScreen(display));

    XSetFont(display, gc, font->fid);
    max_width = (width - 4) / font->max_bounds.width;

    shift_width = max_width / 10;
    if (shift_width > 5)
        shift_width = 5;
    if (shift_width < 1)
        shift_width = 1;

    if (input == NULL) {
        str[0] = 0;
    } else {
        XF_STRNCPY(str, input);
    }
    s[0] = 0;

    start_x = 1;
    start_y = font->max_bounds.ascent / 2 + height / 2;

    w = XCreateSimpleWindow(display, window, x, y, width, height, 0, fg, bg);
    XMapRaised(display, w);

    wmhints.flags = InputHint;
    wmhints.input = True;
    XSetWMHints(display, w, &wmhints);

    XSelectInput(display, w, Button1MotionMask | ButtonPressMask | KeyPressMask | ExposureMask);

    XGrabPointer(display, window, True,
                 (ButtonPressMask | ButtonReleaseMask | Button1MotionMask), GrabModeAsync,
                 GrabModeAsync, w, None, CurrentTime);

    XSetForeground(display, gc, fg);
    XSetBackground(display, gc, bg);

    anchor = 0;
    anchor2 = -1;
    offset = 0;
    done = 0;

    while (done == 0) {
        switch (e->type) {
        case KeyPress:
            {
                if (do_keysym(display, e, str, 256) == 1) {
                    done = 1;
                }
                break;
            }
        case MotionNotify:
            anchor = e->xbutton.x / font->max_bounds.width + offset;
            anchor = xgets_clamp_anchor(anchor, str);
            break;
        case ButtonPress:
            switch (e->xbutton.button) {
            case Button1:
            case Button2:
                {
                    anchor = e->xbutton.x / font->max_bounds.width + offset;
                    anchor = xgets_clamp_anchor(anchor, str);
                    anchor2 = anchor;
                }
                if (e->type == ButtonPress && e->xbutton.button == Button2) {
                    /* paste buffer at anchor */
                    char *p;
                    int nbytes;
                    p = XFetchBuffer(display, &nbytes, 0);
                    (void) strncpy(s, p, (nbytes > 255 ? 255 : nbytes));
                    s[nbytes] = 0;
                    free((char *) p);
                    insert_str(str, s, anchor);
                    anchor += nbytes;
                }
                break;
            case Button3:
                done = -1;
                break;
            }
            break;
        case ButtonRelease:
            if (anchor2 != anchor) {
                int x1 = min(anchor, anchor2);
                int x2 = max(anchor, anchor2);
                XStoreBuffer(display, str + x1, x2 - x1, 0);
            }
            anchor2 = -1;
            break;
        case Expose:
        default:
            break;
        }

        if (done)
            break;

        if (anchor - offset >= max_width - 1) {
            offset += (anchor - offset) - (max_width - 1);
        } else if (anchor - offset < shift_width && offset > 0) {
            offset += (anchor - offset) - shift_width;
        }
        if (offset < 0)
            offset = 0;

        XClearWindow(display, w);
        XDrawString(display, w, gc, start_x, start_y, str + offset, strlen(str + offset));
        XDrawLine(display, w, gc,
                  (anchor - offset) * font->max_bounds.width - 1, start_y + 2,
                  (anchor - offset) * font->max_bounds.width + 1, start_y);
        XDrawLine(display, w, gc,
                  (anchor - offset) * font->max_bounds.width + 1, start_y,
                  (anchor - offset) * font->max_bounds.width + 3, start_y + 2);

        if (anchor2 >= 0 && anchor2 != anchor) {
            int x1 = min(anchor, anchor2);
            int x2 = max(anchor, anchor2);
            XFillRectangle(display, w, gc,
                           start_x + (x1 - offset) * font->max_bounds.width,
                           0, (x2 - x1) * font->max_bounds.width, height);
            XSetForeground(display, gc, bg);
            XSetBackground(display, gc, fg);
            XDrawString(display, w, gc, start_x + (x1 - offset) * font->max_bounds.width, start_y, str + x1, x2 - x1);
            XSetForeground(display, gc, fg);
            XSetBackground(display, gc, bg);
        }

        XNextEvent(display, e);
    }

    XUnmapWindow(display, w);
    XDestroyWindow(display, w);

    XFlush(display);
    if (done != -1)
        return (str);
    return (NULL);
}

int do_keysym(Display *display, XEvent *event, char *str, int nchars)
{
    int count;
    char buffer[128];
    KeySym ksym;

    count = XLookupString(&(event->xkey), buffer, 10, &ksym, NULL);

    buffer[count] = '\0';

    switch (ksym) {
    case XK_Return:
    case XK_KP_Enter:
    case XK_Linefeed:
        return (1);
    case XK_BackSpace:
        if (strlen(str) > 0 && anchor > 0) {
            (void) strcpy(str + anchor - 1, str + anchor);
            anchor--;
        } else {
            XBell(display, 50);
        }
        break;
    case XK_Delete:
        if (strlen(str) && anchor < (int) strlen(str)) {
            strcpy(str + anchor, str + anchor + 1);
            anchor = xgets_clamp_anchor(anchor, str);
        } else {
            XBell(display, 50);
        }
        break;
    case XK_Left:
        anchor = max(anchor - 1, 0);
        break;
    case XK_Right:
        anchor = xgets_clamp_anchor(anchor + 1, str);
        break;
    case XK_Up:
        anchor = 0;
        break;
    case XK_Down:
        anchor = strlen(str);
        break;
    case XK_Escape:
    case XK_Tab:
        file_completion(display, str);
        break;
    default:
        if (((ksym >= XK_KP_Space) && (ksym <= XK_KP_9)) || ((ksym >= XK_space) && (ksym <= XK_asciitilde))) {
            if (event->xkey.state & ControlMask) {
                decode_control(display, ksym, str);
            } else {
                if ((int) (strlen(buffer) + strlen(str)) > nchars) {
                    XBell(display, 50);
                } else {
                    insert_str(str, buffer, anchor);
                    anchor++;
                }
            }
        } else if ((ksym >= XK_Shift_L) && (ksym <= XK_Hyper_R)) {
            ;
        } else {
            XBell(display, 100);
        }
    }

    /* clear buffer so multi-character popup_strs are properly supported, 
     * even though they can only be deleted on char at a time */
    buffer[1] = '\0';
    return (0);
}

void decode_control(Display *display, KeySym k, char *str)
{
    char buf[8];
    int a;

    (void) strcpy(buf, XKeysymToString(k));

    if (str == NULL || strlen(str) == 0)
        return;

    switch (buf[0]) {
                /**
		 ** some important vt100 control keys
		 **/
    case 'u':
        anchor = 0;
        str[0] = '\0';
        break;
    case 'w':
        a = anchor;
        anchor--;
        while (anchor >= 0 && isspace(str[anchor])) {
            anchor--;
        }
        while (anchor >= 0 && !isspace(str[anchor])) {
            anchor--;
        }

        anchor++;
        (void) strcpy(str + anchor, str + a);
        break;

                /**
		 ** some important emacs keys.
		 **/
    case 'a':
        anchor = 0;
        break;
    case 'e':
        anchor = strlen(str);
        break;
    case 'k':
        str[anchor] = '\0';
        break;
    case 'f':
        anchor = xgets_clamp_anchor(anchor + 1, str);
        break;
    case 'b':
        anchor = max(anchor - 1, 0);
        break;
    case 'd':
        if (strlen(str) && anchor < (int) strlen(str)) {
            strcpy(str + anchor, str + anchor + 1);
            anchor = xgets_clamp_anchor(anchor, str);
        } else {
            XBell(display, 50);
        }
        break;
    }
}

void insert_str(char *s, char *t, int at)
{
    /* put t into s at 'at' */
    (void) strcat(t, s + at);
    (void) strcpy(s + at, t);
}

void directory_requestor(Display *display, char *str)
{
    (void) str;
    printf("directory requestor not implemented yet.\n");
    printf("freeing pointer though.\n");

    XUngrabPointer(display, CurrentTime);
}

void file_completion(Display *display, char *str)
{
    int i;
    char buf[256];

    strcpy(buf, str);
    i = complete_dir(buf);
    if (i >= 0)
        XBell(display, 100);
    if (i > 0)
        anchor = i;
    if (i == -1 || i > (int) strlen(str))
        strcpy(str, buf);
    if (i == -1)
        anchor = strlen(str);
}
