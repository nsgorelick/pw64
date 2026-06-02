#include <X11/Xlib.h>
#include <math.h>
#include <stdio.h>
#include "Xfred.h"
#include "image.h"
#include "rose.h"
#include "block.h"
#include "pw_cast.h"

extern Display *display;
extern XColor pwRed, pwGreen, pwBlue, pwYellow, pwCyan, pwMagenta;
extern XColor pwBackground, pwHilite;
extern int Pixels[6];
extern Image images[NIMAGE];
extern char *cnames[6];
extern int ButtonIsDown(Display *display, Window window);

/*
 * Init sets up colors and fonts.
 * Colors are loaded into shared cells, so you CAN run out of colors,
 * but there is never any screen flash.
 */

void do_plot(Button B, XEvent *E);
float pi = 3.1415927;
int anchor = 0;
void switches(Button B, XEvent *E);
void add_bins(Button B, XEvent *E);
void SetRoseImage(Button B, XEvent *E);

void Rose_draw_plot(void);

FILE *RoseFP;
char RoseFile[256];
int RoseFWidth, RoseFHeight;
struct roseplot *rose;
struct PlotStruct RosePS;
int RoseImage = -1;

void do_expose(void);
extern int GetBlockState(int i);
extern void SetBlockState(int i, int state);
extern int SetCurrentBlock(int i);
extern void ActivateBlockID(int i);
extern int GetNBlocks(void);
extern int GetBlockCount(int i);
void plot_mean(float start, float length, int color);
void plot_arc(float start, float length, int color, float magnitude);
extern int SetPlotStruct(struct PlotStruct *plotstruct);
extern int EnableBlock(void);

float cvt_arc(float f) { return (360.0 - f + 90.0); }

void Rose_draw_all(void)
{
    Rose_draw_plot();
    do_expose();
}

void CreateRose(Display *display, XFontStruct *font)
{
    Button B;
    Window parent;
    int i;
    int height;
    int x, width;
    struct VisualInfo *v[3];

    Pixels[0] = pwRed.pixel;
    Pixels[1] = pwGreen.pixel;
    Pixels[2] = pwBlue.pixel;
    Pixels[3] = pwYellow.pixel;
    Pixels[4] = pwMagenta.pixel;
    Pixels[5] = pwCyan.pixel;

    rose = (struct roseplot *)malloc(sizeof(struct roseplot));
    rose->width = 200;
    rose->height = 200;
    rose->nseg = 12;

    rose->x = rose->width / 2;
    rose->y = rose->height / 2;
    rose->radial_offset = ((float)360 / (float)rose->nseg) / 2.0;

    /* create outer case */
    parent = RootWindow(display, DefaultScreen(display));
    B = XfCreateButton(display, parent, 100, 100, 300, 300, 1, WHITE(display), "Main", 1);
    XfAddButtonVisual(B, 0, XfCreateVisual(B, 0, 0, 0, 0, WHITE(display), WHITE(display), XfSolidVisual));
    rose->parent = B;
    parent = B->window;

    x = 10;
    width = 30;
    B = XfCreateButton(display, parent, x, 40, 30, 20, 1, WHITE(display), "text", 1);
    XfAddButtonVisual(B, 0,
                      XfCreateVisual(B, 0, 3, 0, 0, BLACK(display), WHITE(display), XfTextVisual, "Bins", font, 1));
    XfActivateButton(B, ExposureMask);

    x += width + 2 + 15;
    width = 40;

    B = XfCreateButton(display, parent, x, 40, width, 20, 1, BLACK(display), "nbins", 1);
    XfAddButtonVisual(B, 0, XfCreateVisual(B, 0, 5, 0, 0, BLACK(display), WHITE(display), XfTextVisual, "12", font, 0));
    XfActivateButton(B, ExposureMask | ButtonPressMask | KeyPressMask);
    rose->Segs = B;

    x += width + 1;
    width = 15;

    B = XfCreateButton(display, parent, x, 40, width, 10, 1, BLACK(display), "+bins", 1);
    XfAddButtonVisual(B, 0, XfCreateVisual(B, 0, 1, 0, 0, BLACK(display), WHITE(display), XfTextVisual, "+", font, 0));
    XfAddButtonCallback(B, 0, XF_CALLBACK(add_bins), NULL);
    XfActivateButton(B, ExposureMask | ButtonPressMask);
    B->ext = PW_CAST_INT(1);

    B = XfCreateButton(display, parent, x, 50, width, 10, 1, BLACK(display), "-bins", 1);
    XfAddButtonVisual(B, 0, XfCreateVisual(B, 0, 3, 0, 0, BLACK(display), WHITE(display), XfTextVisual, "-", font, 0));
    XfAddButtonCallback(B, 0, XF_CALLBACK(add_bins), NULL);
    XfActivateButton(B, ExposureMask | ButtonPressMask);
    B->ext = PW_CAST_INT(-1);

    x += width;
    width = 40;
    B = XfCreateButton(display, parent, x, 40, width, 20, 1, BLACK(display), "size_bins", 1);
    XfAddButtonVisual(B, 0,
                      XfCreateVisual(B, 0, 5, 0, 0, BLACK(display), pwBackground.pixel, XfTextVisual, "15.0", font, 0));
    XfActivateButton(B, ExposureMask);
    rose->SegSize = B;

    /* Put in color selection buttons */

    height = 31;
    rose->Switches = (Button *)malloc(sizeof(Button) * 6);
    for (i = 0; i < 6; i++) {
        B = XfCreateButton(display, parent, 10, 80 + i * (height + 1), height, height, 1, BLACK(display), "Switch", 3);

        v[0] = XfCreateVisual(B, 2, 2, height - 5, height - 5, Pixels[i], WHITE(display), XfSolidVisual);

        v[1] = XfCreateVisual(B, 2, 2, height - 5, height - 5, Pixels[i], BLACK(display), XfSolidVisual);

        if (i == 0) {
            v[2] = XfCreateVisual(B, 2, 2, height - 5, height - 5, BLACK(display), WHITE(display), XfOutlineVisual);
        }

        XfAddButtonVisual(B, 0, v[0]);
        XfAddButtonVisual(B, 0, v[2]);

        XfAddButtonVisual(B, 1, v[1]);
        XfAddButtonVisual(B, 1, v[2]);

        XfAddButtonVisual(B, 2, v[2]);

        XfAddButtonCallback(B, 0, XF_CALLBACK(switches), NULL);
        XfAddButtonCallback(B, 1, XF_CALLBACK(switches), NULL);
        XfAddButtonCallback(B, 2, XF_CALLBACK(switches), NULL);
        XfActivateButton(B, ExposureMask | ButtonPressMask);
        rose->Switches[i] = B;
        B->ext = PW_CAST_INT(i);
    }

    B = XfCreateButton(display, parent, 60, 80, 200, 200, 1, WHITE(display), "Plot", 1);
    rose->B = B;
    {
        int rd = DefaultDepth(display, DefaultScreen(display));

        rose->pixmap = XCreatePixmap(display, parent, 200, 200, rd);
        rose->pm_gc = XCreateGC(display, rose->pixmap, 0, NULL);
        rose->win_gc = XCreateGC(display, B->window, 0, NULL);
    }
    Rose_draw_plot();
    /*
       XfAddButtonVisual(B, 0, XfCreateVisual(B, 0, 0, 200, 200, WHITE(display),
       WHITE(display), XfPixmapVisual, 8, rose->pixmap));
     */
    XfAddButtonCallback(B, 0, XF_CALLBACK(do_plot), NULL);
    XfNoAutoExposeButton(B);
    XfActivateButton(B, ExposureMask | ButtonPressMask | ButtonReleaseMask | ButtonMotionMask);

    x = 10;
    width = 45;
    B = XfCreateButton(display, parent, x, 10, width, 20, 1, WHITE(display), "text", 1);
    XfAddButtonVisual(B, 0,
                      XfCreateVisual(B, 0, 7, 0, 0, BLACK(display), WHITE(display), XfTextVisual, "Vectors", font, 1));
    XfActivateButton(B, ExposureMask);

    x += width + 5;
    width = 20;
    B = XfCreateButton(display, parent, x, 10, width, 20, 1, BLACK(display), "text", 2);
    XfAddButtonVisual(B, 0,
                      XfCreateVisual(B, 0, 7, 0, 0, BLACK(display), pwBackground.pixel, XfTextVisual, " ", font, 0));
    XfAddButtonVisual(B, 1, XfCreateVisual(B, 0, 7, 0, 0, BLACK(display), WHITE(display), XfTextVisual, " ", font, 0));
    XfAddButtonCallback(B, 0, XF_CALLBACK(SetRoseImage), NULL);
    XfAddButtonCallback(B, 1, XF_CALLBACK(SetRoseImage), NULL);
    XfActivateButton(B, ExposureMask | KeyPressMask);
    rose->ImageSel = B;

    RosePS.get = NULL;
    RosePS.scale = NULL;
    RosePS.draw = NULL;
    RosePS.drawall = Rose_draw_all;
}

void switches(Button B, XEvent *E)
{
    int i;
    switch (E->xbutton.button) {
    case Button1:
        i = GetBlockState(PW_CAST_PTR_INT(B->ext));
        if (i == 0 || i == 1)
            i = 1 - i;
        else
            i = 1;
        set_state(B, i);
        SetBlockState(PW_CAST_PTR_INT(B->ext), i);
        if (i == 1) {
            SetCurrentBlock(PW_CAST_PTR_INT(B->ext));
        }
        break;
    case Button2:
        i = B->state;
        i = (i == 2 ? 0 : 2);
        set_state(B, i);
        SetBlockState(PW_CAST_PTR_INT(B->ext), i);
        break;
    case Button3:
        ActivateBlockID(PW_CAST_PTR_INT(B->ext));
        break;
    }
    if (B->state == 1) {
        for (i = 0; i < GetNBlocks(); i++) {
            if (PW_CAST_PTR_INT(B->ext) != i) {
                if (GetBlockState(i) == 1) {
                    set_state(rose->Switches[i], 0);
                    SetBlockState(i, 0);
                }
            }
        }
    }
    Rose_draw_plot();
    do_expose();
}

void do_plot(Button B, XEvent *E)
{
    int x, y, R, r;
    float f;
    char buf[16];

    switch (E->type) {
    case Expose:
        do_expose();
        break;

    case ButtonPress:
        /* Figure out if its in the circle  */
        x = E->xbutton.x - rose->width / 2;
        y = rose->height / 2 - E->xbutton.y;
        R = sqrt((double)x * x + y * y);
        r = R - (rose->width / 2 - 10);
        if (r >= 0 && r <= 10) {
            f = atan2((double)y, (double)x) * 180 / pi;
            f = cvt_arc(f);
            f = fmod(f, (float)360 / rose->nseg);
            rose->radial_offset = f;
            anchor++;
            sprintf(buf, "%f", rose->radial_offset);
            buf[4] = '\0';
            SetButtonText(rose->SegSize, buf);
        }
        break;

    case MotionNotify:
        if (!anchor)
            return;
        while (XCheckMaskEvent(E->xany.display, PointerMotionMask, E))
            ;
        x = E->xmotion.x - rose->width / 2;
        y = rose->height / 2 - E->xmotion.y;
        f = atan2((double)y, (double)x) * 180 / pi;
        f = cvt_arc(f);
        f = fmod(f, (float)360 / rose->nseg);
        rose->radial_offset = f;
        sprintf(buf, "%f", rose->radial_offset);
        buf[4] = '\0';
        SetButtonText(rose->SegSize, buf);
        do_expose();
        break;

    case ButtonRelease:
        if (!anchor)
            return;
        anchor = 0;
        x = E->xbutton.x - rose->width / 2;
        y = rose->height / 2 - E->xbutton.y;
        f = atan2((double)y, (double)x) * 180 / pi;
        f = cvt_arc(f);
        f = fmod(f, (float)360 / rose->nseg);
        rose->radial_offset = f;
        sprintf(buf, "%f", rose->radial_offset);
        buf[4] = '\0';
        SetButtonText(rose->SegSize, buf);
        Rose_draw_plot();
        do_expose();
        break;
    }
}

void Rose_draw_plot(void)
{
    int i, j, k;
    int **s;
    int *sp;
    float t;
    int max;
    float maxval;
    float radial_offset = rose->radial_offset;
    int NSeg = rose->nseg;
    float val = 0, v = 0;
    float mean = 0;
    float sigma = 0;
    struct block_node *n;
    int count = 0;
    int nblocks = GetNBlocks();
    int x, y;
    double R, dbl = 0, summ = 0, suml = 0, Ro;

    /* calculate our bins */
    /* this just cleans up the pixmap */

    XSetForeground(display, rose->pm_gc, WHITE(display));
    XFillRectangle(display, rose->pixmap, rose->pm_gc, 0, 0, rose->width, rose->height);
    XSetForeground(display, rose->pm_gc, WHITE(display));
    XFillArc(display, rose->pixmap, rose->pm_gc, 10, 10, rose->width - 20, rose->height - 20, 0, 360 * 64);
    XSetForeground(display, rose->pm_gc, BLACK(display));

    XDrawArc(display, rose->pixmap, rose->pm_gc, 0, 0, rose->width - 1, rose->height - 1, 0, 360 * 64);
    XDrawArc(display, rose->pixmap, rose->pm_gc, 10, 10, rose->width - 20, rose->height - 20, 0, 360 * 64);

    if (RoseImage == -1) {
        return;
    }

    t = (float)NSeg / 360.0;
    s = (int **)malloc(sizeof(int *) * nblocks);
    for (i = 0; i < nblocks; i++) {
        s[i] = (int *)calloc((NSeg + 1), sizeof(int));
        sp = s[i];
        sp[NSeg] = 0;
        count = 0;
        summ = 0;
        suml = 0;
        if (GetBlockState(i) != 2 && GetBlockCount(i) != 0) {
            for (n = GetFirstBlock(i); n != NULL; n = n->next) {
                x = n->stack->x;
                y = n->stack->y;
                sp[NSeg]++;
                if (RoseImage >= 0) {
                    val = get_data(Images[RoseImage], y * Images[RoseImage]->subset.width + x);
                }
                /* Screwy deleted point stuff */
                if (val > 0.9e30)
                    continue;
                v = val - radial_offset;

                if (v > 360.0)
                    v -= 360.0;
                if (v < 0.0)
                    v += 360.0;

                sp[(int)(v * t)]++;

                if ((v += 180.0) > 360.0)
                    v -= 360.0;
                sp[(int)(v * t)]++;
                /* calculate radial statistics */
                count++;

                dbl = val * 2.0 * pi / 180.0;
                suml += cos(dbl);
                summ += sin(dbl);
            }

            R = sqrt(suml * suml + summ * summ);
            Ro = sqrt((double)(0.5) * (double)count * (double)5.99);

            mean = atan2(summ, suml) * 180.0 / acos(-1.0) / 2.0;
            if (mean < 0)
                mean += 180.0;

            if (count > 0) {
                sigma = 180.0 * sqrt(2.0) / pi * sqrt(1.0 - (R / (double)count));
                /* sigma = sqrt(2.0 - 2.0 * R / (double) count) / 2.0; */
            } else {
                sigma = 0.0;
            }

            printf("%s:\tN=%d", cnames[i], count);
            printf("\tAvg: %f\tsigma: %f", mean, sigma);
            printf("\tstat signif=%s\n", (R > Ro && count > 20) ? "yes" : "no");
            plot_mean(mean, sigma, i);
        }
    }

    /* clean up after plotting mean arcs */

    XSetForeground(display, rose->pm_gc, WHITE(display));
    XFillArc(display, rose->pixmap, rose->pm_gc, 10, 10, rose->width - 20, rose->height - 20, 0, 360 * 64);
    printf("\n");

    /* now draw from most to least in each bin */

    /* first, normalize to the maxval for each color */
    for (j = 0; j < nblocks; j++) {
        max = -1;
        for (i = 0; i < NSeg; i++) {
            if (s[j][i] > max)
                max = s[j][i];
        }
        s[j][NSeg] = max;
    }

    for (i = 0; i < NSeg; i++) {
        for (j = 0; j < nblocks; j++) {
            max = -1;
            maxval = -1;
            for (k = 0; k < nblocks; k++) {
                if (s[k][i] > 0 && (float)s[k][i] / (float)s[k][NSeg] > maxval) {
                    maxval = (float)s[k][i] / (float)s[k][NSeg];
                    max = k;
                }
            }
            if (max != -1) {
                plot_arc(radial_offset + ((1.0 / t) * i), (1.0 / t), max, maxval);
                s[max][i] = 0;
            }
        }
    }

    XSetForeground(display, rose->pm_gc, BLACK(display));

    XDrawArc(display, rose->pixmap, rose->pm_gc, 0, 0, rose->width - 1, rose->height - 1, 0, 360 * 64);
    XDrawArc(display, rose->pixmap, rose->pm_gc, 10, 10, rose->width - 20, rose->height - 20, 0, 360 * 64);

    for (i = 0; i < nblocks; i++) {
        free(s[i]);
    }
    free(s);
}

void plot_mean(float start, float length, int color)
{
    int offset;
    int size;

    length = fabs(length / 2.0);

    XSetForeground(display, rose->pm_gc, Pixels[color]);
    XFillArc(display, rose->pixmap, rose->pm_gc, 0, 0, rose->width, rose->height, (int)(cvt_arc(start - length) * 64.0),
             (int)-(length * 2 * 64.0));
    XFillArc(display, rose->pixmap, rose->pm_gc, 0, 0, rose->width, rose->height,
             (int)(cvt_arc(start + 180.0 - length) * 64.0), (int)-(length * 2 * 64.0));
}

void plot_arc(float start, float length, int color, float magnitude)
{
    int offset;
    int size;

    size = rose->width - 20;
    offset = (float)(size) / 2.0 * (1.0 - magnitude);

    XSetForeground(display, rose->pm_gc, Pixels[color]);
    XFillArc(display, rose->pixmap, rose->pm_gc, 10 + offset, 10 + offset, size - offset * 2, size - offset * 2,
             (int)(cvt_arc(start) * 64.0), (int)-(length * 64.0));
}

void DrawBackground(void)
{
    float theta;
    float c, s;
    int x1, y1, x2, y2;
    int i, w;

    XSetForeground(display, rose->pm_gc, BLACK(display));

    for (i = 0; i < rose->nseg; i++) {
        theta = cvt_arc(360.0 / rose->nseg * i + rose->radial_offset);
        c = cos(theta * pi / 180.0);
        s = sin(theta * pi / 180.0);
        w = rose->width / 2;
        x1 = c * w + w;
        y1 = w - s * w;
        x2 = c * (w - 10) + w;
        y2 = w - s * (w - 10);
        XDrawLine(display, rose->B->window, rose->win_gc, x1, y1, x2, y2);
    }
}

void do_expose(void)
{
    XCopyArea(display, rose->pixmap, rose->B->window, rose->win_gc, 0, 0, rose->width, rose->height, 0, 0);
    DrawBackground();
    XFlush(display);
}

void add_bins(Button B, XEvent *E)
{
    char buf[16];

    rose->nseg += PW_CAST_PTR_INT(B->ext);
    if (rose->nseg < 2)
        rose->nseg = 2;

    sprintf(buf, "%d", rose->nseg);
    SetButtonText(rose->Segs, buf);
    Rose_draw_plot();
    do_expose();

    if (ButtonIsDown(B->display, B->window)) {
        usleep(300000);
        while (ButtonIsDown(B->display, B->window)) {
            rose->nseg += PW_CAST_PTR_INT(B->ext);
            if (rose->nseg < 2)
                rose->nseg = 2;

            sprintf(buf, "%d", rose->nseg);
            SetButtonText(rose->Segs, buf);
            Rose_draw_plot();
            do_expose();
            XFlush(B->display);
            usleep(50000);
        }
    }
}

void ActivateRose(Display *display)
{
    /* ... */
    SetPlotStruct(&RosePS);
    XfActivateButton(rose->parent, ExposureMask);
    Rose_draw_plot();
    do_expose();
}

void DeactivateRose(Display *display) { XfDeactivateButton(rose->parent); }

void SetRoseImage(Button B, XEvent *E)
{
    int i;
    char buf[2];
    char *ptr;

    if (B->state == 0) {
        set_state(rose->ImageSel, 1);
    }

    if (E->type == KeyPress) {
        char b[2];
        KeySym keysym;
        XComposeStatus status;

        XLookupString(&(E->xkey), b, 2, &keysym, &status);

        buf[0] = b[0];
        buf[1] = 0;

        i = buf[0] - 'A';
        if (i < 0 || i >= NIMAGE) {
            i = buf[0] - 'a';
            if (i < 0 || i >= NIMAGE) {
                return;
            }
        }
        RoseImage = i;
        ptr = (B->States[1]->Visuals[0]).visual.t_vis.text;
        ptr[strlen(ptr) - 1] = 'A' + i;
        UpdateButton(B);
        EnableBlock();
    }
}

IsSignif(int count, float *data)
{
    int i;
    if (count < 20)
        return (0);
    for (i = 0; i < count; i++) {
    }
    return 0;
}
