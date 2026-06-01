/*
 * Joystick.c
 *
 * The library support routines for joysticks. See Joystick.h
 *
 */

#include "Xfred.h"
#include "Joystick.h"
#include "freelist.h"

Joystick JoystickList = NULL;

void displayJoystick(Joystick J, GC localGC);
int XfSetJoystickValue(Joystick J, int x, int y);
/*
 * Delete the passed joystick
 */
int XfDestroyJoystick(Joystick J)
{
    Joystick search;
    struct VisualInfo *vis;
    struct VisualInfo *mm;
    struct VisualInfo *nn;
    struct CallBackList *call;

    if ((JoystickList == NULL) || (J == NULL))
        return (False);

    XfDeactivateJoystick(J);
    if (J == JoystickList) {
        vis = J->field;
        nn = vis;
        while (nn != NULL) {
            mm = nn->next;
            if (nn->vtype == XfXImageVisual)
                XFree((char *) nn->visual.i_vis);
            if ((nn->vtype == XfPixmapVisual) ||
                (nn->vtype == XfTiledVisual) ||
                (nn->vtype == XfStippledVisual) || (nn->vtype == XfOpaqueStippledVisual))
                XFreePixmap(J->display, nn->visual.p_vis.map);
            free(nn);
            nn = mm;
        }
        vis = J->thumb;
        nn = vis;
        while (nn != NULL) {
            mm = nn->next;
            if (nn->vtype == XfXImageVisual)
                XFree((char *) nn->visual.i_vis);
            if ((nn->vtype == XfPixmapVisual) ||
                (nn->vtype == XfTiledVisual) ||
                (nn->vtype == XfStippledVisual) || (nn->vtype == XfOpaqueStippledVisual))
                XFreePixmap(J->display, nn->visual.p_vis.map);
            free(nn);
            nn = mm;
        }
        call = J->CallBacks;
        FreeList(struct CallBackList *, call, next);
        JoystickList = JoystickList->nextJoystick;
        free(J);
    } else {
        search = JoystickList;
        while (search->nextJoystick != J)
            search = search->nextJoystick;
        /* Search now points to the parent of J */
        vis = J->field;
        nn = vis;
        while (nn != NULL) {
            mm = nn->next;
            if (nn->vtype == XfXImageVisual)
                XFree((char *) nn->visual.i_vis);
            if ((nn->vtype == XfPixmapVisual) ||
                (nn->vtype == XfTiledVisual) ||
                (nn->vtype == XfStippledVisual) || (nn->vtype == XfOpaqueStippledVisual))
                XFreePixmap(J->display, nn->visual.p_vis.map);
            free(nn);
            nn = mm;
        }
        vis = J->thumb;
        nn = vis;
        while (nn != NULL) {
            mm = nn->next;
            if (nn->vtype == XfXImageVisual)
                XFree((char *) nn->visual.i_vis);
            if ((nn->vtype == XfPixmapVisual) ||
                (nn->vtype == XfTiledVisual) ||
                (nn->vtype == XfStippledVisual) || (nn->vtype == XfOpaqueStippledVisual))
                XFreePixmap(J->display, nn->visual.p_vis.map);
            free(nn);
            nn = mm;
        }
        call = J->CallBacks;
        FreeList(struct CallBackList *, call, next);
        search->nextJoystick = J->nextJoystick;
        free(J);
    }
    return (True);
}

/*
 * Get the pointer to the named joystick.
 */
Joystick XfGetJoystick(char *name)
{
    Joystick search;

    search = JoystickList;

    while (search != NULL) {
        if (!strcmp(name, search->name))
            return (search);
        search = search->nextJoystick;
    }

    return (NULL);
}

/*
 * Add the passed VisualInfo structure to the linked list maintained for
 * the joystick field or thumb, based on flag.
 */
int XfAddJoystickVisual(Joystick J, struct VisualInfo *vis, int flag)
                           /* 0 for field, 1 for thumb */
{
    struct VisualInfo *loop;

    if ((J == NULL) || (vis == NULL))
        return (False);

    vis->next = NULL;
    if (flag)
        loop = J->thumb;
    else
        loop = J->field;
    if (loop == NULL) {
        if (flag)
            J->thumb = vis;
        else
            J->field = vis;
    } else {
        while (loop->next != NULL)
            loop = loop->next;
        loop->next = vis;
    }
    return (True);
}

/*
 * Move the specified joystick to the front of the list, to improve search
 * time. No reason for this to fail.
 */
int XfMoveJoystick(Joystick J)
{
    Joystick search;

    if ((J == NULL) || (J == JoystickList))
        return (True);
    search = JoystickList;
    while (search->nextJoystick != J) {
        if (search->nextJoystick == NULL)
            return (False);
        search = search->nextJoystick;
    }
    /* Now that search->nextJoystick == J, start shuffling */
    search->nextJoystick = J->nextJoystick;
    J->nextJoystick = JoystickList;
    JoystickList = J;
    return (True);
}

/*
 * EventJoystick returns which joystick, if any, the passed event occured in.
 */
Joystick XfEventJoystick(XEvent *E)
{
    Joystick search;

    search = JoystickList;
    while (search != NULL) {
        switch (E->type) {
        case NoExpose:
        case GraphicsExpose:
            break;
        default:
            if (search->window == E->xany.window)
                return (search);
            break;
        }
        search = search->nextJoystick;
    }
    return (NULL);
}

/* 
 * Add a callback to the list for the joystick specified. Return
 * an error for any of the many things that could go wrong.
 */
int XfAddJoystickCallback(Joystick J, CallBack new, CallBack old)
             /* The joystick */
{
    struct CallBackList *new_ptr;
    struct CallBackList *loop;
    struct CallBackList *loopn;

    if (J == NULL)
        return (False);

    new_ptr = (struct CallBackList *) malloc(sizeof(struct CallBackList));
    if (new_ptr == NULL)
        return (False);
    new_ptr->proc = new;
    loop = J->CallBacks;

    if ((loop == NULL) || (loop->proc == old)) {
        /* Must insert at head of list */
        new_ptr->next = loop;
        J->CallBacks = new_ptr;
    } else {
        loopn = loop->next;
        while (loopn != NULL) {
            if (loopn->proc == old)
                break;
            else
                loop = loopn, loopn = loop->next;
        }
        if (loopn == NULL) {
            /* Not found. Unless old is NULL (add at end), error */
            if (old == NULL) {
                new_ptr->next = NULL;
                loop->next = new_ptr;
            } else {
                free(new_ptr);
                return (False);
            }
        } else {
            /* Found it. Insert new callback before */
            new_ptr->next = loop->next;
            loop->next = new_ptr;
        }
    }
    return (True);
}

/*
 * Delete the named callback from the list. Error if cb not a member of list.
 */
int XfDelJoystickCallback(Joystick J, CallBack cb)
{
    struct CallBackList *loop;
    struct CallBackList *loopn;

    if ((J == NULL) || (cb == NULL))
        return (False);

    loop = J->CallBacks;
    if (loop == NULL)
        return (False);
    if (loop->proc == cb) {
        J->CallBacks = J->CallBacks->next;
        free(loop);
        return (True);
    }
    loopn = loop->next;
    while (loopn != NULL) {
        if (loopn->proc == cb) {
            loop = loopn->next;
            free(loopn);
            return (True);
        }
        loop = loopn;
        loopn = loop->next;
    }
    return (False);             /* Callback was not found */
}

/*
 * Un-map and select input to zero.
 */
int XfDeactivateJoystick(Joystick J)
{
    if (J == NULL)
        return (False);
    J->active = False;
    XSelectInput(J->display, J->window, 0);
    XUnmapWindow(J->display, J->window);
    return (True);
}

/* 
 * Map the joystick's window, set it's event mask, and mark it active.
 */
int XfActivateJoystickValue(Joystick J, int x, int y, long unsigned int mask)
{
    if (J == NULL)
        return (False);
    J->active = True;

    XfSetJoystickValue(J, x, y);

    XSelectInput(J->display, J->window, mask);
    XMapRaised(J->display, J->window);
    XFlush(J->display);
    return (True);
}

/*
 * Create a new joystick, placing it at the end of the list.
 */
Joystick XfCreateJoystick(Display *display, Window parent, int x, int y, int width, int height, int border_width,
                          long unsigned int border_color, char *name, int thumb_width, int thumb_height)
{
    Joystick new;
    Joystick search;

    if ((thumb_width == 0) || (thumb_height == 0))
        return (NULL);

    new = (Joystick) malloc(sizeof(struct _Joystick));
    if (new == NULL)
        return (NULL);

    new->parent = parent;
    new->display = display;
    new->x = x;
    new->y = y;
    new->height = height;
    new->width = width;
    new->border_width = border_width;
    new->border_color = border_color;
    XF_STRNCPY(new->name, name);
    new->thumb_width = thumb_width;
    new->thumb_height = thumb_height;
    new->thumb_max_x = width - thumb_width - 1;
    new->thumb_max_y = height - thumb_height - 1;
    new->thumb_min_x = 0;
    new->thumb_min_y = 0;
    new->field = (struct VisualInfo *) NULL;
    new->thumb = (struct VisualInfo *) NULL;
    new->CallBacks = (struct CallBackList *) NULL;
    new->window = XCreateSimpleWindow(display, parent, x, y, width, height, border_width, border_color, 0);
    new->exposeCallback = XF_CALLBACK(defaultJoystickCallback);
    new->updateCallback = XF_CALLBACK(defaultJoystickUpdateCallback);
    new->active = False;
    new->nextJoystick = NULL;

    search = JoystickList;
    if (search == NULL)
        JoystickList = new;
    else {
        while (search->nextJoystick != NULL)
            search = search->nextJoystick;
        search->nextJoystick = new;
    }
    return (new);
}

/*
 * Default callback for expose events
 */
void defaultJoystickCallback(Joystick J, XEvent *E)
{
    (void) E;
    struct VisualInfo *vis;     /* For traversing linked lists */
    GC localGC;                 /* Inherited from parent */
    unsigned long bg;           /* Color for filling bar initially */

    localGC = DefaultGC(J->display, DefaultScreen(J->display));
    vis = J->field;
    if (vis != NULL)
        bg = vis->background;
    else
        bg = J->border_color;

    XSetForeground(J->display, localGC, bg);
    XFillRectangle(J->display, J->window, localGC, 0, 0, J->width, J->height);

    displayJoystick(J, localGC);
}

/*
 * Redisplay a joystick without redrawing everything
 */
void defaultJoystickUpdateCallback(Joystick J, XEvent *E)
{
    GC localGC;

    (void) E;

    localGC = DefaultGC(J->display, DefaultScreen(J->display));
    /*
     * Why?
     *
     XSetForeground(J->display, localGC, bg);
     XFillRectangle(J->display, J->window, localGC, J->thumb_cur_x,
     J->thumb_cur_y, J->thumb_width, J->thumb_height);
     */
    J->thumb_cur_x = J->new_x;
    J->thumb_cur_y = J->new_y;

    displayJoystick(J, localGC);
}

/* Since so much code was common... */
void displayJoystick(Joystick J, GC localGC)
{
    int x_off, y_off;
    struct VisualInfo *vis;
    int i;
    XCharStruct xcs;            /* Character structure for XQueryTextExtents */
    int dir, asc, des;          /* Used in calls to XQueryTextExtents */

    localGC = DefaultGC(J->display, DefaultScreen(J->display));
    vis = J->field;
    x_off = 0;
    y_off = 0;

    for (i = 0; i < 2; i++) {
        if (i) {
            vis = J->thumb;
            x_off = J->thumb_cur_x;
            y_off = J->thumb_cur_y;
        }
        while (vis != NULL) {
            switch (vis->vtype) {
            case XfXImageVisual:
                XPutImage(J->display, J->window, localGC, vis->visual.i_vis, 0,
                          0, (vis->x_pos + x_off), (vis->y_pos + y_off), vis->width, vis->height);
                break;
            case XfPixmapVisual:
                XSetForeground(J->display, localGC, vis->foreground);
                XSetBackground(J->display, localGC, vis->background);
                if (vis->visual.p_vis.depth == 1)
                    XCopyPlane(J->display, vis->visual.p_vis.map, J->window,
                               localGC, 0, 0, vis->width, vis->height, (vis->x_pos + x_off), (vis->y_pos + y_off), 1);
                else
                    XCopyArea(J->display, vis->visual.p_vis.map, J->window,
                              localGC, 0, 0, vis->width, vis->height, (vis->x_pos + x_off), (vis->y_pos + y_off));
                break;
            case XfTextVisual:
                XSetFont(J->display, localGC, vis->visual.t_vis.font->fid);
                XSetForeground(J->display, localGC, vis->foreground);
                XTextExtents(vis->visual.t_vis.font, vis->visual.t_vis.text,
                             strlen(vis->visual.t_vis.text), &dir, &asc, &des, &xcs);
                switch (vis->visual.t_vis.align) {
                case 0:        /* Center */
                    dir = xcs.width / 2;
                    if (vis->width == 0)
                        asc = (vis->x_pos + x_off) + J->width / 2;
                    else
                        asc = (vis->x_pos + x_off) + vis->width / 2;
                    asc -= dir;
                    des = (vis->y_pos + y_off) + xcs.ascent;
                    break;
                case 1:        /* Left justify */
                    asc = (vis->x_pos + x_off);
                    des = (vis->y_pos + y_off) + xcs.ascent;
                    break;
                case 2:        /* Right justify */
                    asc = vis->width;
                    if (asc == 0)
                        asc = J->width;
                    asc -= xcs.width;
                    des = (vis->y_pos + y_off) + xcs.ascent;
                    break;
                }
                XDrawString(J->display, J->window, localGC, asc, des,
                            vis->visual.t_vis.text, strlen(vis->visual.t_vis.text));
                break;
            case XfOutlineVisual:
            case XfSolidVisual:
                XSetForeground(J->display, localGC, vis->foreground);
                if (vis->vtype == XfSolidVisual)
                    XFillRectangle(J->display, J->window, localGC,
                                   (vis->x_pos + x_off), (vis->y_pos + y_off), vis->width, vis->height);
                else
                    XDrawRectangle(J->display, J->window, localGC,
                                   (vis->x_pos + x_off), (vis->y_pos + y_off), vis->width, vis->height);
                break;
            case XfStippledVisual:
            case XfOpaqueStippledVisual:
                XSetBackground(J->display, localGC, vis->background);
                XSetForeground(J->display, localGC, vis->foreground);
                XSetStipple(J->display, localGC, vis->visual.p_vis.map);
                XSetFillStyle(J->display, localGC,
                              ((vis->vtype == XfStippledVisual) ? FillStippled : FillOpaqueStippled));
                XFillRectangle(J->display, J->window, localGC,
                               (vis->x_pos + x_off), (vis->y_pos + y_off), vis->width, vis->height);
                XSetFillStyle(J->display, localGC, FillSolid);
                break;
            case XfTiledVisual:
                XSetTile(J->display, localGC, vis->visual.p_vis.map);
                XSetFillStyle(J->display, localGC, FillTiled);
                XFillRectangle(J->display, J->window, localGC,
                               (vis->x_pos + x_off), (vis->y_pos + y_off), vis->width, vis->height);
                XSetFillStyle(J->display, localGC, FillSolid);
                break;
            }
            vis = vis->next;
        }
    }
}

/*
 * Lastly, the routine to process events that occur in a joystick tool.
 */
int XfJoystickResponse(Joystick J, XEvent *E)
{
    struct CallBackList *execs;
    int x, y;
    XEvent EE;

    if (J == NULL)
        return (False);
    else
        execs = J->CallBacks;

    x = J->thumb_cur_x;
    y = J->thumb_cur_y;

    switch (E->type) {
    case Expose:
        (*(J->exposeCallback)) (J, E);
        return (True);
        break;
    case ButtonPress:
        while (XCheckMaskEvent(J->display, ButtonMotionMask, &EE));
        if (E->xbutton.button == Button3) {
            /* Move joystick along Y axis */
            x = J->thumb_cur_x;
            y = E->xbutton.y - (J->thumb_height / 2);
        } else if (E->xbutton.button == Button2) {
            /* Move joystick along X axis */
            y = J->thumb_cur_y;
            x = E->xbutton.x - (J->thumb_width / 2);
        } else {                /* Button1 */
            /* Move joystick in both X and Y */
            x = E->xbutton.x - (J->thumb_width / 2);
            y = E->xbutton.y - (J->thumb_height / 2);
        }
        break;
    case MotionNotify:
        while (XCheckMaskEvent(J->display, ButtonMotionMask, E));
        if (E->xmotion.state & Button1Mask) {
            /* Move joystick in both X and Y */
            x = E->xbutton.x - (J->thumb_width / 2);
            y = E->xbutton.y - (J->thumb_height / 2);
        } else if (E->xmotion.state & Button2Mask) {
            /* Move joystick along X axis */
            y = J->thumb_cur_y;
            x = E->xbutton.x - (J->thumb_width / 2);
        } else if (E->xmotion.state & Button3Mask) {
            /* Move joystick along Y axis */
            x = J->thumb_cur_x;
            y = E->xbutton.y - (J->thumb_height / 2);
        }
        break;
    }

    if (x < J->thumb_min_x)
        x = J->thumb_min_x;
    else if (x > J->thumb_max_x)
        x = J->thumb_max_x;
    if (y < J->thumb_min_y)
        y = J->thumb_min_y;
    else if (y > J->thumb_max_y)
        y = J->thumb_max_y;
    J->new_x = x;
    J->new_y = y;
    (*(J->updateCallback)) (J, E);

    while (execs != NULL) {
        struct CallBackList *next_exec = execs->next;

        if (execs->proc != NULL)
            (*(execs->proc)) (J, E);
        execs = next_exec;
    }
    return (True);
}

int XfJoystickLimitThumb(Joystick J, int x_min, int y_min, int x_max, int y_max)
{
    J->thumb_min_x = x_min;
    J->thumb_max_x = x_max - J->thumb_width - 1;
    J->thumb_min_y = y_min;
    J->thumb_max_y = y_max - J->thumb_height - 1;
    return (1);
}

int XfJoystickResizeThumb(Joystick J, int width, int height)
{
    J->thumb_max_x += J->thumb_width + 1;
    J->thumb_max_y += J->thumb_height + 1;

    J->thumb_width = width;
    J->thumb_height = height;

    J->thumb_max_x -= (J->thumb_width + 1);
    J->thumb_max_y -= (J->thumb_height + 1);

    XfSetJoystickValue(J, J->thumb_cur_x, J->thumb_cur_y);
    return (1);
}

int XfSetJoystickValue(Joystick J, int x, int y)
{
    if (J == NULL)
        return (False);

    if (x < J->thumb_min_x)
        J->thumb_cur_x = J->thumb_min_x;
    else if (x > J->thumb_max_x)
        J->thumb_cur_x = J->thumb_max_x;
    else
        J->thumb_cur_x = x;

    if (y < J->thumb_min_y)
        J->thumb_cur_y = J->thumb_min_y;
    else if (y > J->thumb_max_y)
        J->thumb_cur_y = J->thumb_max_y;
    else
        J->thumb_cur_y = y;

    return (True);
}
