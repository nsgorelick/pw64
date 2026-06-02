/*
 * VisInfo.c
 *
 * Coding of any routines specific to creation and/or manipulation of
 * struct VisualInfo data.
 *
 */

#include <stdarg.h>
#include <string.h>
#include "Xfred.h"

unsigned long XfDefaultWidgetBackground(Display *d) { return WhitePixel(d, DefaultScreen(d)); }

void XfDrawTextVisual(Display *d, Window w, GC gc, int widget_w, int widget_h, struct VisualInfo *vis, int x_off,
                      int y_off)
{
    int dir, asc, des;
    XCharStruct xcs;
    int bg_x, bg_y, bg_w, bg_h;
    int text_x, text_y;

    if (vis == NULL || vis->visual.t_vis.font == NULL)
        return;

    XSetFont(d, gc, vis->visual.t_vis.font->fid);
    XTextExtents(vis->visual.t_vis.font, vis->visual.t_vis.text, strlen(vis->visual.t_vis.text), &dir, &asc, &des,
                 &xcs);
    switch (vis->visual.t_vis.align) {
    case 0: /* Center */
        dir = xcs.width / 2;
        if (vis->width == 0)
            text_x = (vis->x_pos + x_off) + widget_w / 2;
        else
            text_x = (vis->x_pos + x_off) + vis->width / 2;
        text_x -= dir;
        text_y = (vis->y_pos + y_off) + xcs.ascent;
        break;
    case 1: /* Left justify */
        text_x = vis->x_pos + x_off;
        text_y = (vis->y_pos + y_off) + xcs.ascent;
        break;
    case 2: /* Right justify */
        text_x = vis->width;
        if (text_x == 0)
            text_x = widget_w;
        text_x = (vis->x_pos + x_off) + text_x - xcs.width;
        text_y = (vis->y_pos + y_off) + xcs.ascent;
        break;
    default:
        text_x = vis->x_pos + x_off;
        text_y = (vis->y_pos + y_off) + xcs.ascent;
        break;
    }

    if (vis->width > 0 && vis->height > 0) {
        bg_x = vis->x_pos + x_off;
        bg_y = vis->y_pos + y_off;
        bg_w = vis->width;
        bg_h = vis->height;
    } else {
        bg_x = x_off;
        bg_y = y_off;
        bg_w = widget_w;
        bg_h = widget_h;
    }
    XSetForeground(d, gc, vis->background);
    XFillRectangle(d, w, gc, bg_x, bg_y, bg_w, bg_h);

    XSetForeground(d, gc, vis->foreground);
    XDrawString(d, w, gc, text_x, text_y, vis->visual.t_vis.text, strlen(vis->visual.t_vis.text));
}

/*
 * Create a VisualInfo structure from the passed data, for adding to a
 * button.
 */
struct VisualInfo *XfCreateVisual(void *input_widget, int x, int y, int width, int height, unsigned long fg,
                                  unsigned long bg, int type, ...)
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
    new = (struct VisualInfo *)calloc(1, sizeof(struct VisualInfo));
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
        depth = va_arg(args, int); /* Actually width */
        format = va_arg(args, int); /* Actually height */
        new->visual.p_vis.map = XCreateBitmapFromData(gen->display, gen->window, data, depth, format);
        break;
    case XfTextVisual:
        data = va_arg(args, char *);
        XF_STRNCPY(new->visual.t_vis.text, data);
        new_font = va_arg(args, XFontStruct *);
        depth = va_arg(args, int);
        new->visual.t_vis.font = new_font;
        new->visual.t_vis.align = depth;
        /*
         * For text visuals, width/height == 0 means "no explicit text background box".
         * Keep these at 0 so XfDrawTextVisual can fall back to widget-sized background.
         */
        if (width == 0)
            new->width = 0;
        if (height == 0)
            new->height = 0;
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
        {
            int pad = (depth > 8 ? 32 : 8);
            xim = XCreateImage(gen->display, def_vis, depth, format, 0, data, width, height, pad, 0);
        }
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
        XFree((char *)nn->visual.i_vis);
    if ((nn->vtype == XfPixmapVisual) || (nn->vtype == XfTiledVisual) || (nn->vtype == XfStippledVisual) ||
        (nn->vtype == XfOpaqueStippledVisual))
        XFreePixmap(display, nn->visual.p_vis.map);
    free(nn);
    nn = NULL;
}
