/*
 * VisInfo.c
 *
 * Coding of any routines specific to creation and/or manipulation of
 * struct VisualInfo data.
 *
 */

#include <stdarg.h>
#include "Xfred.h"

/*
 * Create a VisualInfo structure from the passed data, for adding to a
 * button.
 */
struct VisualInfo *XfCreateVisual(void *input_widget,
                                  int x, int y,
                                  int width, int height, unsigned long fg, unsigned long bg, int type, ...)
{
    va_list args;
    Pixmap tmp_map;
    char *data;
    XFontStruct *new_font;
    int depth, format;
    struct VisualInfo *new;
    Visual *def_vis;
    XImage *xim;
    struct XfAnyWidget *gen = input_widget;

    va_start(args, type);
    new = (struct VisualInfo *) calloc(1, sizeof(struct VisualInfo));
    if (new == NULL)
        return (NULL);
    new->next = NULL;
    new->x_pos = x;
    new->y_pos = y;
    if ((width == 0) && (type == XfOutlineVisual))
        width = gen->width - 1;
    if (width)
        new->width = width;
    else
        new->width = gen->width - x;
    if ((height == 0) && (type == XfOutlineVisual))
        height = gen->height - 1;
    if (height)
        new->height = height;
    else
        new->height = gen->height - y;
    new->foreground = fg;
    new->background = bg;
    new->vtype = type;
    switch (type) {
    case XfOutlineVisual:
    case XfSolidVisual:
        break;
    case XfTiledVisual:
        new->visual.p_vis.depth = 8;
        new->visual.p_vis.map = va_arg(args, Pixmap);
        break;
    case XfStippledVisual:
    case XfOpaqueStippledVisual:
        new->visual.p_vis.depth = 1;
        data = va_arg(args, char *);
        depth = va_arg(args, int);      /* Actually width */
        format = va_arg(args, int);     /* Actually height */
        new->visual.p_vis.map = XCreateBitmapFromData(gen->display, gen->window, data, depth, format);
        break;
    case XfTextVisual:
        data = va_arg(args, char *);
        XF_STRNCPY(new->visual.t_vis.text, data);
        new_font = va_arg(args, XFontStruct *);
        depth = va_arg(args, int);
        new->visual.t_vis.font = new_font;
        new->visual.t_vis.align = depth;
        break;
    case XfPixmapVisual:
        depth = va_arg(args, int);
        if (depth == 1) {
            data = va_arg(args, char *);
            tmp_map = XCreateBitmapFromData(gen->display, gen->window, data, width, height);
        } else
            tmp_map = va_arg(args, Pixmap);
        new->visual.p_vis.depth = depth;
        new->visual.p_vis.map = tmp_map;
        break;
    case XfXImageVisual:
        def_vis = DefaultVisual(gen->display, DefaultScreen(gen->display));
        depth = va_arg(args, int);
        format = va_arg(args, int);
        data = va_arg(args, char *);
        xim = XCreateImage(gen->display, def_vis, depth, format, 0, data, width, height, 8, 0);
        new->visual.i_vis = xim;
        break;
    case XfHersheyVisual:
        /* XfHersheyVisual, text, cset, scale, angle, align */
        XF_STRNCPY(new->visual.h_vis.text, va_arg(args, char *));
        new->visual.h_vis.cset = va_arg(args, int);
        new->visual.h_vis.align = va_arg(args, int);
        new->visual.h_vis.scale = va_arg(args, double);
        new->visual.h_vis.angle = va_arg(args, double);
        break;
    }
    va_end(args);
    return (new);
}

void XfFreeVisual(Display *display, struct VisualInfo *nn)
{
    if (nn->vtype == XfXImageVisual)
        XFree((char *) nn->visual.i_vis);
    if ((nn->vtype == XfPixmapVisual) ||
        (nn->vtype == XfTiledVisual) || (nn->vtype == XfStippledVisual) || (nn->vtype == XfOpaqueStippledVisual))
        XFreePixmap(display, nn->visual.p_vis.map);
    free(nn);
    nn = NULL;
}
