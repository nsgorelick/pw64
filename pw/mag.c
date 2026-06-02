#include "mag.h"
#include "Xfred.h"
#include "display_rgb.h"
#include <X11/Xlib.h>
#include <X11/Xos.h>
#include <X11/Xutil.h>
#include <math.h>
#include <stdio.h>

extern XColor pwRed, pwGreen, pwBlue, pwYellow, pwCyan, pwMagenta;
extern int NColors;

/*
 * CreateMagnify
 * ActivateMagnify
 * DeactivateMagnify
 * SetMagnify
 * MagnifyCallback
 * UpdateMagnify
 * SetMagnifyScale
 *
 *
 * Mag->scale
 * (scale > 1 is positive magnification,
 *  scale < 1 is negative magnification
 *
 */

extern GC gc;
extern Colormap ColorMap;

extern int ColorBlocks(char *data, int x, int y, int width, int height);
void UpdateMagnify(Display *display, struct magnify *Mag);
int MagConvert(int xin, int yin, int *xout, int *yout);
extern void BlockMovement(Display *display, int x, int y, int type, int buttons);
extern void UpdateReadout(int x, int y, Image new);
void SetMagnifyScale(struct magnify *Mag, XEvent *E);
extern void MagBoxClear(void);
extern void MagBoxSet(void);
extern int load_image(int i);

static void mag_rasterize(struct magnify *Mag)
{
    int n;

    if (Mag->mag_slots == NULL || Mag->mag_data == NULL || Mag->image == NULL)
        return;
    n = Mag->_width * Mag->_height;
    if (n > 0)
        pw_lut_index_to_rgb(Mag->image, (unsigned char *)Mag->mag_slots, (unsigned char *)Mag->mag_data, n);
}

static void mag_ui_color(unsigned long pixel, XColor *out)
{
    if (pixel == pwRed.pixel)
        *out = pwRed;
    else if (pixel == pwGreen.pixel)
        *out = pwGreen;
    else if (pixel == pwBlue.pixel)
        *out = pwBlue;
    else if (pixel == pwYellow.pixel)
        *out = pwYellow;
    else if (pixel == pwCyan.pixel)
        *out = pwCyan;
    else if (pixel == pwMagenta.pixel)
        *out = pwMagenta;
    else {
        out->red = out->green = out->blue = 0;
        out->flags = DoRed | DoGreen | DoBlue;
    }
}

char *create_mag(char *data, char *out, int x, int y, int src_width, int src_height, int dest_width, int dest_height,
                 int scale)
{
    double fact, f;
    int i, j, k, l;
    int xpos, ypos;
    int width, height;
    int val;
    int a, b;

    if (scale > 0) {
        width = dest_width / scale;
        height = dest_height / scale;
        for (j = 0; j < height; j++) {
            b = j * scale;
            ypos = y + j;
            for (i = 0; i < width; i++) {
                xpos = x + i;
                if (xpos < src_width && ypos < src_height) {
                    val = data[ypos * src_width + xpos];
                } else {
                    val = 0;
                }
                a = i * scale;
                for (k = 0; k < scale; k++) {
                    for (l = 0; l < scale; l++) {
                        out[(b + k) * dest_width + (a + l)] = val;
                    }
                }
            }
        }
    } else {
        scale = -scale;
        for (i = 0; i < dest_height; i++) {
            ypos = y + i * scale;
            for (j = 0; j < dest_width; j++) {
                xpos = x + j * scale;
                if (xpos > src_width || ypos > src_height) {
                    out[(i * dest_width + j)] = 0;
                } else {
                    out[(i * dest_width + j)] = data[ypos * src_width + xpos];
                }
            }
        }
    }
    return (out);
}

struct magnify *CreateMagnify(Display *display, Window parent)
{
    extern int AllocError;

    /*
     * initialize sturcture,
     * create and map window.
     */
    struct magnify *Mag;
    Mag = (struct magnify *)calloc(1, sizeof(struct magnify));
    Mag->width = 200;
    Mag->height = 200;
    Mag->scale = 2;
    Mag->x = 0;
    Mag->y = 0;
    Mag->mag_image = NULL;
    Mag->mag_data = NULL;
    Mag->mag_slots = NULL;
    if (parent == (Window)NULL) {
        parent = RootWindow(display, DefaultScreen(display));
    }
    Mag->parent = parent;
    Mag->display = display;
    Mag->state = 0;
    Mag->window =
        XCreateSimpleWindow(display, Mag->parent, 300, 300, Mag->width, Mag->height, 1, BLACK(display), BLACK(display));
    XSetWindowColormap(display, Mag->window, ColorMap);

    XSelectInput(display, Mag->window,
                 (ExposureMask | ButtonPressMask | ButtonReleaseMask | KeyPressMask | StructureNotifyMask |
                  PointerMotionMask | ButtonMotionMask));
    AllocError = 0;
    /*
     *
        Mag->pixmap = XCreatePixmap(display, Mag->window,
                                    Mag->width, Mag->height,
                                    DefaultDepth(display,DefaultScreen(display)));
        XSync(display, False);
        if (AllocError) {
            Mag->pixmap = NULL;
        }
     *
     */
    return (Mag);
}

void ActivateMagnify(Display *display, struct magnify *Mag)
{
    XMapRaised(display, Mag->window);
    XFlush(display);
    Mag->state = 1;
}

void DeactivateMagnify(Display *display, struct magnify *Mag)
{
    XSetTransientForHint(display, Mag->window, Mag->parent);
    XUnmapWindow(display, Mag->window);
    XFlush(display);
    Mag->state = 0;
}

void SetMagnify(Display *display, Image new, struct magnify *Mag, int x, int y)
/* center of mag square */
{
    int xpos, ypos;
    double fact;
    int scale;
    int width, height;
    int xsize, ysize;

    if (new == NULL)
        return;

    if (Mag->scale > 0) {
        fact = Mag->scale;
    } else {
        fact = -1.0 / Mag->scale;
    }

    xsize = Mag->width / fact + 1;
    ysize = Mag->height / fact + 1;

    xpos = new->subset.width - xsize / 2;
    ypos = new->subset.height - ysize / 2;

    if (x > xpos)
        x = xpos;
    if (y > ypos)
        y = ypos;
    if (x < xsize / 2)
        x = xsize / 2;
    if (y < ysize / 2)
        y = ysize / 2;

    /*
     * Convert back to (rounded) window size
     */
    width = xsize * fact;
    height = ysize * fact;

    Mag->_width = width;
    Mag->_height = height;

    Mag->x = x;
    Mag->y = y;

    Mag->image = new;
    if (Mag->mag_image != NULL) {
        pw_free_rgb_image(Mag->mag_image);
        Mag->mag_image = NULL;
        Mag->mag_data = NULL; /* freed via mag_image->data */
    } else if (Mag->mag_data != NULL) {
        free(Mag->mag_data);
        Mag->mag_data = NULL;
    }
    if (Mag->mag_slots != NULL) {
        free(Mag->mag_slots);
        Mag->mag_slots = NULL;
    }
    Mag->mag_slots = (char *)malloc((unsigned int)(width * height));
    Mag->mag_data = (char *)malloc((unsigned int)(width * height * 4));

    create_mag(new->sdata, Mag->mag_slots, x - xsize / 2, y - ysize / 2, new->subset.width, new->subset.height, width,
               height, Mag->scale);

    mag_rasterize(Mag);
    Mag->mag_image = pw_create_rgb_image(display, (unsigned char *)Mag->mag_data, width, height);

    ColorBlocks(Mag->mag_slots, x - xsize / 2, y - ysize / 2, width, height);

    UpdateMagnify(display, Mag);
}

void MagnifyCallback(struct magnify *Mag, XEvent *E)
{
    int x = 0, y = 0;
    switch (E->type) {
    case Expose: {
        UpdateMagnify(E->xany.display, Mag);
        break;
    }
    case ButtonPress:
    case ButtonRelease: {
        MagConvert((int)E->xbutton.x, (int)E->xbutton.y, &x, &y);
        BlockMovement(E->xbutton.display, x, y, E->type, E->xbutton.button);
        break;
    }
    case MotionNotify: {
        while (XCheckMaskEvent(E->xany.display, PointerMotionMask, E))
            ;
        MagConvert((int)E->xbutton.x, (int)E->xbutton.y, &x, &y);

        if (E->xmotion.state & (Button1Mask | Button2Mask | Button3Mask)) {
            BlockMovement(E->xany.display, x, y, E->type, 0);
        } else {
            UpdateReadout(x, y, Mag->image);
        }
        break;
    }
    case KeyPress: {
        SetMagnifyScale(Mag, E);
        break;
    }
        /*
         * Window size had maybe changed?
         */
    case ConfigureNotify: {
        int width, height;

        width = E->xconfigure.width;
        height = E->xconfigure.height;

        if (Mag->width == width && Mag->height == height)
            return;
        MagBoxClear();

        if (Mag->mag_image != NULL) {
            pw_free_rgb_image(Mag->mag_image);
            Mag->mag_image = NULL;
            Mag->mag_data = NULL;
        } else if (Mag->mag_data != NULL) {
            free(Mag->mag_data);
            Mag->mag_data = NULL;
        }
        if (Mag->mag_slots != NULL) {
            free(Mag->mag_slots);
            Mag->mag_slots = NULL;
        }

        Mag->width = width;
        Mag->height = height;

        SetMagnify(E->xany.display, Mag->image, Mag, Mag->x, Mag->y);
        MagBoxSet();
        break;
    }
        /*
         * Maybe update on default?
         */
    case ReparentNotify:
    case MapNotify:
    case UnmapNotify:
        break;
    default: {
        fprintf(stderr, "Mag: Whats this? %d\n", E->type);
        break;
    }
    }
}

void UpdateMagnify(Display *display, struct magnify *Mag)
{
    if (Mag->mag_image != NULL) {
        XPutImage(display, Mag->window, gc, Mag->mag_image, 0, 0, 0, 0, Mag->width, Mag->height);
    }
}

void LocalUpdateMagnify(Display *display, struct magnify *Mag, int x1, int y1, int x2, int y2)
{
    if (x2 < x1) {
        int t;
        t = x2;
        x2 = x1;
        x1 = t;
    }
    if (y2 < y1) {
        int t;
        t = y2;
        y2 = y1;
        y1 = t;
    }

    if (Mag->mag_image != NULL) {
        XPutImage(display, Mag->window, gc, Mag->mag_image, x1, y1, x1, y1, x2 - x1 + 1, y2 - y1 + 1);
    }
}

void SetMagnifyScale(struct magnify *Mag, XEvent *E)
{
    int i;
    char b[2];
    KeySym keysym;

    b[0] = 0;
    XLookupString(&(E->xkey), b, 2, &keysym, NULL);

    if (b[0] == 0)
        return;
    if (b[0] == '+' || b[0] == '-' || b[0] == '*' || b[0] == '/') {
        Mag->last_key = b[0];
        return;
    }
    i = b[0] - '0';

    if (i <= 0 || i > 9) {
        i = b[0] - 'a';
        if (i < 0 || i >= NIMAGE)
            i = b[0] - 'A';
        if (i < 0 || i >= NIMAGE || Images[i] == NULL || Images[i]->sdata == NULL) {
            return;
        }
        load_image(i);
        return;
    }

    MagBoxClear();
    switch (Mag->last_key) {
    case '-':
        Mag->scale = -i;
        break;
    case '*':
        Mag->scale = 10 * i;
        break;
    case '/':
        Mag->scale = -10 * i;
        break;
    case '+':
    case 0:
    default:
        Mag->scale = i;
        break;
    }
    SetMagnify(E->xany.display, Mag->image, Mag, Mag->x, Mag->y);
    MagBoxSet();
    Mag->last_key = 0;
}

int MagConvert(int xin, int yin, int *xout, int *yout)
{
    extern struct magnify *Mag;
    double fact;

    if (Mag->scale > 0) {
        fact = Mag->scale;
    } else {
        fact = -1.0 / Mag->scale;
    }

    *xout = Mag->x - ((int)(Mag->width / fact + 1)) / 2 + (xin / fact);
    *yout = Mag->y - ((int)(Mag->height / fact + 1)) / 2 + (yin / fact);
    return 0;
}

int MagUnconvert(int xin, int yin, int *xout, int *yout)
{
    extern struct magnify *Mag;
    double fact;

    if (Mag->scale > 0) {
        fact = Mag->scale;
    } else {
        fact = -1.0 / Mag->scale;
    }

    *xout = (xin - (Mag->x - ((int)(Mag->width / fact + 1)) / 2)) * fact;
    *yout = (yin - (Mag->y - ((int)(Mag->height / fact + 1)) / 2)) * fact;
    return 0;
}

void ColorMag(int x, int y, int color, int type)
{
    /*
     * Type: == 0   Color whole block
     *       >= 1:  Color with %2 stipple
     *       >= 2:  Don't update immediatly
     */
    extern struct magnify *Mag;
    int scale;
    int xpos, ypos;
    int i, j;
    int val;
    XColor c;

    if (Mag->mag_data == NULL || Mag->mag_slots == NULL)
        return;

    if (color >= 2 && color <= NColors + 1)
        pw_color_at(Mag->image, (unsigned char)color, &c);
    else
        mag_ui_color((unsigned long)color, &c);

    if (Mag->scale > 0) {
        scale = Mag->scale;
        MagUnconvert(x, y, &xpos, &ypos);
        if (xpos >= 0 && ypos >= 0 && xpos <= Mag->width && ypos <= Mag->width) {
            for (i = 0; i < scale; i++) {
                for (j = 0; j < scale; j++) {
                    if (type == 0 || (i + j) % 2 == 0) {
                        val = xpos + i + (ypos + j) * Mag->_width;
                        if (color >= 2 && color <= NColors + 1)
                            Mag->mag_slots[val] = (unsigned char)color;
                        pw_pack_xcolor(&c, (unsigned char *)Mag->mag_data + val * 4);
                    }
                }
            }
            if (type <= 1) {
                LocalUpdateMagnify(Mag->display, Mag, xpos, ypos, xpos + scale, ypos + scale);
            }
        }
    } else {
        scale = -Mag->scale;
    }
}

void UncolorMag(int x, int y)
{
    extern struct magnify *Mag;
    ColorMag(x, y, (int)Mag->image->sdata[y * Mag->image->subset.width + x], 0);
}
