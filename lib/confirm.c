#include <stdarg.h>
#include <stdint.h>
#include "Xfred.h"

/*
 * This routine puts up a verify requestor.
 * It returns n, being the option selected by the user.
 *
 *    ---------------------------------
 *    |                               |
 *    |           prompt1             |
 *    |           prompt2             |
 *    |             ...               |
 *    |           promptn             |
 *    |                               |
 *    |--------  --------     --------|
 *    || Opt1 |  | Opt2 | ... | Optn ||
 *    ||______|  |______|     |______||
 *    |_______________________________|
 *
 *
 */

int done;

void opt_callback(Button B, XEvent *E)
{
    (void) E;
    done = 1 + (int) (intptr_t) B->ext;
}

int
ConfirmRequestor(Display *display,
                 Window parent,
                 GC gc, XFontStruct *font, int x, int y, int width, int height, int header, int warp, int warp_opt, ...)
{
    va_list args;
    Window w;
    Button Main;
    Button *Opts;
    char **opts;
    char **prompt;
    int nopts, nprompt;
    int i;
    int max_width;
    int Bwidth;
    int step;
    int max_len;
    int xpos, ypos;
    XEvent E;

    va_start(args, warp_opt);
    done = 0;

    Main = XfCreateButton(display, parent, x, y, width, height, (header ? 1 : 3), BLACK(display), "verify", 1);
    XfAddButtonVisual(Main, 0, XfCreateVisual(Main, 0, 3, 0, 0, WHITE(display), WHITE(display), XfSolidVisual));

    w = Main->window;

    nprompt = va_arg(args, int);
    prompt = (char *(*)) malloc(nprompt * sizeof(char *));

    ypos = (height - 30) / 2 - (nprompt / 2 * 15);

    for (i = 0; i < nprompt; i++) {
        prompt[i] = va_arg(args, char *);
        XfAddButtonVisual(Main, 0, XfCreateVisual(Main, 0, ypos, 0, 0,
                                                  BLACK(display), WHITE(display), XfTextVisual, prompt[i], font, 0));
        ypos += 15;
    }

    nopts = va_arg(args, int);
    opts = (char *(*)) malloc(nopts * sizeof(char *));
    Opts = (Button *) malloc(sizeof(Button) * nopts);

    max_len = 0;
    for (i = 0; i < nopts; i++) {
        int len;

        opts[i] = va_arg(args, char *);
        len = (int) strlen(opts[i]);
        if (len > max_len)
            max_len = len;
    }

    XSetFont(display, gc, font->fid);
    max_width = font->max_bounds.width;

    Bwidth = max_width * max_len + 10;
    step = (width - 10) / nopts;

/*
 * Center button at x+step/2;
 */
    ypos = height - 30;
    xpos = 5;
    for (i = 0; i < nopts; i++) {
        Opts[i] = XFCreateButton(display, w, (xpos + step / 2 - Bwidth / 2), ypos,
                                 Bwidth, 20, 1, BLACK(display), WHITE(display), "opt", 1);
        XfAddButtonVisual(Opts[i], 0, XfCreateVisual(Opts[i], 0, 0, 0, 0,
                                                     WHITE(display), WHITE(display), XfSolidVisual));
        XfAddButtonVisual(Opts[i], 0, XfCreateVisual(Opts[i], 0, 7, 0, 0,
                                                     BLACK(display), WHITE(display), XfTextVisual, opts[i], font, 0));
        XfAddButtonCallback(Opts[i], 0, XF_CALLBACK(opt_callback), NULL);
        XfActivateButton(Opts[i], ExposureMask | ButtonPressMask);
        (Opts[i])->ext = (void *) (intptr_t) i;
        xpos += step;
    }
    XSetTransientForHint(display, Main->window, parent);
    if (header == 0) {
        XSetWindowAttributes a;
        a.override_redirect = True;
        XChangeWindowAttributes(display, Main->window, CWOverrideRedirect, &a);
    }
    XfActivateButton(Main, ExposureMask);
    if (warp) {
        XWarpPointer(display, None, Opts[warp_opt - 1]->window, 0, 0, 0, 0, Bwidth / 2, 10);
    }

/*
    XGrabPointer(display, w, True,
		 (ButtonPressMask | ButtonReleaseMask), GrabModeAsync,
		 GrabModeAsync, w, None, CurrentTime);
 */

    while (done == 0) {
        XNextEvent(display, &E);
        XfButtonPush(XfEventButton(&E), &E);
    }

/*
	XUngrabPointer(display, CurrentTime);
*/
    for (i = 0; i < nopts; i++) {
        XfDestroyButton(Opts[i]);
    }
    XfDestroyButton(Main);
    return (done);
}
