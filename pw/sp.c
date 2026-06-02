#include "config.h"

#ifdef INTERNAL_SP

#include "Xfred.h"
#include "bitmaps/bitmaps.h"
#include "pw_cast.h"
#include "specpr.h"
#include <X11/keysym.h>
extern int is_file(char *filename);
extern void PushdownComposite(Composite C);

extern XColor pwBackground;

/*
 * Eww, sp guts all over the place.
 */

#define NITEMS 9

struct fields {
    char *name;
    int width;
    int lines;
} Fields[NITEMS] = {"Rec #",   5, 0, "Title", 40, 0, "Chans",    5,  0, "Date",    8,  0, "Time", 8, 0,
                    "Airmass", 4, 0, "User",  8,  0, "AutoHist", 60, 1, "ManHist", 74, 4};

struct filedata {
    int rec;
    char title[41];
    int chans;
    char date[9];
    char time[9];
    int airmass;
    char user[9];
    char autohist[61];
    char manhist[297];
};

int Order[NITEMS] = {1, 2, 3, -4, -5, -6, -7, -8, -9};

Button SPCase;
Button ControlCase;
Button ScrollWindow;
Button ScrollHeader;
Button TopOfScroll;
Button ShowCase;
Button SelectedShow;

Composite SpecprFilename;

int NScroll = 8;
int NLines = 1;
int SelectedScroll = -1;
int SelectedData = -1;
Time SelectedTime = 0;
int FontHeight;

void GetSpecprFilename(Composite C, XEvent *E);
void HeaderCallback(Button B, XEvent *E);
void MoveScroll(Button B, XEvent *E);
void NewScroll(Button B, XEvent *E);
void MoveShow(Button B, XEvent *E);
void ToggleShow(Button B, XEvent *E);
void ToggleShowCase(Button B, XEvent *E);
void ScrollCallback(Button B, XEvent *E);
void select_none(int B, int E);
void tmpquit(Button B, XEvent *E);
int maxrec;
void RequestorResizeCallback(Button B, XEvent *E);

FILE *SPFile = NULL;

void CreateShowPanel(Display *display, XFontStruct *font);
int NextRecord(int i);
void SetScroll(int i);
int ReadRecord(int i, struct filedata *d);
void PutRecord(int i, struct filedata *d, int hilite);
extern int max_rec(int fd);
void MakeOutputString(struct filedata *d, char (*buf)[256]);
void UpdateHeader(void);
void move_scroll(int off);
void FinalSelection(int i);
extern int read_specpr(int fd, int i, struct _label *label, char **data);
extern void ApplySpecpr(int itchan, float *data);
int specpr_text_parse(struct _tlabel *lbl, struct filedata *d);
int specpr_data_parse(struct _label *lbl, struct filedata *d);
extern void DeleteWaves(void);

void CreateSpecprRequestor(Display *display, XFontStruct *font)
{
    Button B;
    Window w;

    FontHeight = font->ascent + font->descent + 8;

    /*
     * This needs to be a button for a resize callback.
     */
    B = XfCreateButton(display, RootWindow(display, DefaultScreen(display)), 300, 300, 400, 340, 1, BLACK(display),
                       "Case", 1);
    XfAddButtonVisual(B, 0, XfCreateVisual(B, 0, 0, 0, 0, WHITE(display), WHITE(display), XfSolidVisual));
    XfAddButtonCallback(B, 0, XF_CALLBACK(RequestorResizeCallback), NULL);
    SPCase = B;

    w = B->window;
    /*
     *
     */

    SpecprFilename = (Composite)CreateComposite(display, w, font, 15, 5, 350, 20, pwBackground.pixel, "<NO FILE>");
    AddCompositeCallback(SpecprFilename, XF_CALLBACK(GetSpecprFilename));
    ActivateComposite(SpecprFilename);

    B = XfCreateButton(display, w, 15, 35, 370, 20, 1, BLACK(display), "Header", 1);
    XfAddButtonVisual(B, 0,
                      XfCreateVisual(B, 0, 5, 0, 0, BLACK(display), WHITE(display), XfTextVisual, "Header", font, 0));
    ScrollHeader = B;
    XfAddButtonCallback(B, 0, XF_CALLBACK(HeaderCallback), NULL);
    XfActivateButton(B, ExposureMask | ButtonPressMask);

    B = XfCreateButton(display, w, 15, 60, 370, 230, 1, BLACK(display), "ScrollWindow", 1);
    XfAddButtonVisual(B, 0, XfCreateVisual(B, 0, 0, 0, 0, WHITE(display), WHITE(display), XfSolidVisual));
    XfNoAutoExposeButton(B);
    XfAddButtonCallback(B, 0, XF_CALLBACK(ScrollCallback), NULL);
    XfActivateButton(B, ExposureMask | ButtonPressMask | KeyPressMask);
    ScrollWindow = B;

    NScroll = 230 / FontHeight;

    B = XfCreateButton(display, w, 0, 295, 400, 50, 0, BLACK(display), "Caseing", 1);
    XfAddButtonVisual(B, 0, XfCreateVisual(B, 0, 0, 0, 0, WHITE(display), WHITE(display), XfSolidVisual));
    XfActivateButton(B, ExposureMask);
    ControlCase = B;

    B = XfCreateButton(display, ControlCase->window, 15, 0, 45, 30, 1, BLACK(display), "SHOW", 2);
    XfAddButtonVisual(B, 0,
                      XfCreateVisual(B, 0, 11, 0, 0, BLACK(display), WHITE(display), XfTextVisual, "SHOW", font, 0));
    XfAddButtonVisual(B, 1,
                      XfCreateVisual(B, 0, 11, 0, 0, WHITE(display), BLACK(display), XfTextVisual, "SHOW", font, 0));
    XfAddButtonCallback(B, 0, XF_CALLBACK(ToggleShowCase), NULL);
    XfAddButtonCallback(B, 1, XF_CALLBACK(ToggleShowCase), NULL);
    XfActivateButton(B, ExposureMask | ButtonPressMask);

    B = XfCreateButton(display, ControlCase->window, 65, 0, 45, 30, 1, BLACK(display), "SEARCH", 1);
    XfAddButtonVisual(B, 0,
                      XfCreateVisual(B, 1, 11, 0, 0, BLACK(display), WHITE(display), XfTextVisual, "SEARCH", font, 0));
    XfActivateButton(B, ExposureMask);

    B = XfCreateButton(display, ControlCase->window, 120, 0, 20, 30, 1, BLACK(display), "PageUp", 1);
    XfAddButtonVisual(
        B, 0,
        XfCreateVisual(B, 0, 0, 20, 30, BLACK(display), WHITE(display), XfPixmapVisual, 1, Bitmap_Page_Down_bits));
    XfAddButtonCallback(B, 0, XF_CALLBACK(MoveScroll), NULL);
    XfActivateButton(B, ExposureMask | ButtonPressMask);

    B = XfCreateButton(display, ControlCase->window, 145, 0, 20, 30, 1, BLACK(display), "ScrollUp", 1);
    XfAddButtonVisual(
        B, 0,
        XfCreateVisual(B, 0, 0, 20, 30, BLACK(display), WHITE(display), XfPixmapVisual, 1, Bitmap_Scroll_Down_bits));
    XfAddButtonCallback(B, 0, XF_CALLBACK(MoveScroll), NULL);
    XfActivateButton(B, ExposureMask | ButtonPressMask);

    B = XfCreateButton(display, ControlCase->window, 175, 0, 50, 30, 1, BLACK(display), "Readout", 1);
    XfAddButtonVisual(B, 0, XfCreateVisual(B, 0, 11, 0, 0, BLACK(display), WHITE(display), XfTextVisual, "1", font, 0));
    XfAddButtonCallback(B, 0, XF_CALLBACK(NewScroll), NULL);
    XfActivateButton(B, ExposureMask | ButtonPressMask | KeyPressMask);
    TopOfScroll = B;
    B->ext = PW_CAST_INT(1);

    B = XfCreateButton(display, ControlCase->window, 235, 0, 20, 30, 1, BLACK(display), "ScrollDown", 1);
    XfAddButtonVisual(
        B, 0,
        XfCreateVisual(B, 0, 0, 20, 30, BLACK(display), WHITE(display), XfPixmapVisual, 1, Bitmap_Scroll_Up_bits));
    XfAddButtonCallback(B, 0, XF_CALLBACK(MoveScroll), NULL);
    XfActivateButton(B, ExposureMask | ButtonPressMask);

    B = XfCreateButton(display, ControlCase->window, 260, 0, 20, 30, 1, BLACK(display), "PageDown", 1);
    XfAddButtonVisual(
        B, 0, XfCreateVisual(B, 0, 0, 20, 30, BLACK(display), WHITE(display), XfPixmapVisual, 1, Bitmap_Page_Up_bits));
    XfAddButtonCallback(B, 0, XF_CALLBACK(MoveScroll), NULL);
    XfActivateButton(B, ExposureMask | ButtonPressMask);

    B = XfCreateButton(display, ControlCase->window, 290, 0, 45, 30, 1, BLACK(display), "PRINT", 1);
    XfAddButtonVisual(B, 0,
                      XfCreateVisual(B, 0, 11, 0, 0, BLACK(display), WHITE(display), XfTextVisual, "NONE", font, 0));
    XfAddButtonCallback(B, 0, XF_CALLBACK(select_none), NULL);
    XfActivateButton(B, ExposureMask | ButtonPressMask);

    B = XfCreateButton(display, ControlCase->window, 340, 0, 45, 30, 1, BLACK(display), "CANCEL", 1);
    XfAddButtonVisual(B, 0,
                      XfCreateVisual(B, 1, 11, 0, 0, BLACK(display), WHITE(display), XfTextVisual, "CANCEL", font, 0));
    XfAddButtonCallback(B, 0, XF_CALLBACK(tmpquit), NULL);
    XfActivateButton(B, ExposureMask | ButtonPressMask);

    CreateShowPanel(display, font);
}

void ActivateSP(void) { XfActivateButton(SPCase, ExposureMask | StructureNotifyMask); }

void DeactivateSP(void) { XfDeactivateButton(SPCase); }

void UpdateRecords(int hilite)
{
    int i, j;
    struct filedata fdata;
    Display *dpy;
    XEvent E;

    /* We are doing a complete refill, lose any Exposures */
    while (XCheckTypedWindowEvent(ScrollWindow->display, ScrollWindow->window, Expose, &E))
        ;

    UpdateButton(ScrollWindow);

    if (SPFile == NULL)
        return;

    j = PW_CAST_PTR_INT(TopOfScroll->ext);

    if (j < 1) {
        j = NextRecord(0);
        SetScroll(j);
        return; /* return here, SetScroll calls this routine again */
    }
    for (i = 0; i < NScroll; i++) {
        ReadRecord(j, &fdata);
        PutRecord(i, &fdata, (int)(i == hilite || j == SelectedData));
        j = NextRecord(j);
        if (j == -1)
            return;
    }
}

int NextRecord(int i)
{
    int j, k;
    struct filedata fdata;

    maxrec = max_rec(fileno(SPFile));

    if (i < 0)
        i = 0;
    if (i >= maxrec) {
        return (-1);
    }
    j = i;
    while ((k = ReadRecord(++j, &fdata)) == 0) {
        if (k == -1)
            return (-1);
        if (j >= maxrec)
            return (-1);
    }
    return (j);
}

int PrevRecord(int i)
{
    struct filedata fdata;
    int j, k;

    maxrec = max_rec(fileno(SPFile));
    if (i > maxrec)
        return (-1);
    if (i < 0)
        return (-1);

    j = i;
    while ((k = ReadRecord(--j, &fdata)) == 0) {
        if (k == -1)
            return (-1);
        if (j <= 0)
            return (-1);
    }
    return (j);
}

void PutRecord(int i, struct filedata *d, int hilite)
{
    /* Output a record to the listbox. */
    /* NOTE: size of a record may be more than 1 line */

    char buf[6][256];
    int lines;
    Display *display;
    GC gc;
    int j;

    display = ScrollWindow->display;
    gc = DefaultGC(display, DefaultScreen(display));

    MakeOutputString(d, buf);

    if (hilite) {
        XSetForeground(display, gc, pwBackground.pixel);
        XFillRectangle(display, ScrollWindow->window, gc, 0, (int)(i * NLines * FontHeight), ScrollWindow->width,
                       NLines * FontHeight);
        SelectedData = d->rec;
    }

    XSetForeground(display, gc, BLACK(display));
    for (j = 0; j < NLines; j++) {
        if (buf[j][0] != '\0') {
            XDrawString(display, ScrollWindow->window, DefaultGC(display, DefaultScreen(display)), 0,
                        ((i * NLines) + (j + 1)) * FontHeight - ((FontHeight - 8) / 2), buf[j], strlen(buf[j]));
        }
    }
}

void MakeOutputString(struct filedata *d, char (*buf)[256])
{
    /*
     * This routine is very data structure dependent.
     */
    char out[256];
    int i, j, k;
    int width;
    char *p, *q;
    int len;
    char tmp[16];

    strcpy(buf[0], "  ");
    for (i = 1; i < 6; i++) {
        buf[i][0] = '\0';
    }

    for (i = 0; i < NITEMS; i++) {
        j = Order[i];
        if (j > 0) {
            width = Fields[j - 1].width;
            if (j < 8) {
                switch (j) {
                case 1: /* rec # */
                    sprintf(out, "%*d", width, d->rec);
                    break;
                case 2: /* title */
                    sprintf(out, "%-*s", width, d->title);
                    break;
                case 3: /* chans */
                    sprintf(out, "%*d", width, d->chans);
                    break;
                case 4: /* date */
                    sprintf(out, "%*s", width, d->date);
                    break;
                case 5: /* time */
                    sprintf(out, "%*s", width, d->time);
                    break;
                case 6: /* airmass */
                    sprintf(out, "%4.4f", (float)d->airmass / 1000.0);
                    break;
                case 7: /* user */
                    sprintf(out, "%*s", width, d->user);
                    break;
                }
                out[width] = '\0';
                strcat(buf[0], out);
                strcat(buf[0], "   ");
            } else {
                switch (j) {
                case 8: /* auto hist */
                    strcpy(buf[1], "          ");
                    strncat(buf[1], d->autohist, width);
                    break;
                case 9: /* man hist */
                    for (k = 2; k < 6; k++) {
                        strcpy(buf[k], "          ");
                    }
                    len = strlen(d->manhist);

                    q = d->manhist;
                    strncat(buf[2], q, width);

                    p = (len > width * 2 ? d->manhist + 2 * width : q + len);
                    strncat(buf[3], p, width);

                    p = (len > width * 2 ? d->manhist + 2 * width : q + len);
                    strncat(buf[4], p, width);

                    p = (len > width * 3 ? d->manhist + 2 * width : q + len);
                    strncat(buf[5], p, width);
                    break;
                }
            }
        }
    }
}

void HeaderCallback(Button B, XEvent *E) { UpdateHeader(); }

void UpdateHeader(void)
{
    int i, j;
    char *s;
    int lines;

    char buf[256];
    s = ScrollHeader->States[0]->Visuals->visual.t_vis.text;

    s[0] = 0;
    strcat(s, "  ");

    lines = 1;
    for (i = 0; i < 9; i++) {
        j = Order[i];
        if (j > 0) {
            if (j < 8) {
                sprintf(buf, "%-*s   ", Fields[j - 1].width, Fields[j - 1].name);
                strcat(s, buf);
            }
            lines += Fields[j - 1].lines;
        }
    }
    NLines = lines;
    NScroll = ScrollWindow->height / (FontHeight * NLines);
    ScrollHeader->States[0]->Visuals->visual.t_vis.align = 1;
    UpdateButton(ScrollHeader);
}

void GetSpecprFilename(Composite C, XEvent *E)
{
    if (is_file(C->current_text) == 0) {
        printf("No such file\n");
        SPFile = NULL;
    } else {
        if (SPFile != NULL)
            fclose(SPFile);
        SPFile = fopen(C->current_text, "r");
        maxrec = max_rec(fileno(SPFile));
        SetButtonText(SpecprFilename->Edit, C->current_text);
        UpdateHeader();
        SetScroll(0);
        PushdownComposite(C);
    }
}

void set_filenames(int ac, char **av)
{
    char *p;
    int i, j;

    for (i = ac; i > 0; i--) {
        if (is_file(av[i]) != 0) {
            AddToComposite(SpecprFilename, av[i]);
            j = i;
        }
    }
    SetButtonText(SpecprFilename->Edit, av[j]);
    SPFile = fopen(av[j], "r");
    maxrec = max_rec(fileno(SPFile));
    UpdateHeader();
    UpdateRecords(-1);
}

void set_filename(char *str)
{
    char *p;
    int i, j;

    if (is_file(str) != 0) {
        AddToComposite(SpecprFilename, str);
    }
    SetButtonText(SpecprFilename->Edit, str);
    SPFile = fopen(str, "r");
    maxrec = max_rec(fileno(SPFile));
    UpdateHeader();
    UpdateRecords(-1);
}

void MoveScroll(Button B, XEvent *E)
{
    int off;

    if (!strcmp(B->name, "PageUp"))
        off = NScroll;
    else if (!strcmp(B->name, "PageDown"))
        off = -NScroll;
    else if (!strcmp(B->name, "ScrollUp"))
        off = 1;
    else if (!strcmp(B->name, "ScrollDown"))
        off = -1;
    move_scroll(off);
}

void move_scroll(int off)
{
    int i, j;

    i = PW_CAST_PTR_INT(TopOfScroll->ext);
    while (off) {
        if (off < 0) {
            j = PrevRecord(i);
            off++;
        } else {
            j = NextRecord(i);
            off--;
        }
        if (j == -1) {
            SetScroll(i);
            return;
        }
        i = j;
    }
    SetScroll(i);
}

void NewScroll(Button B, XEvent *E)
{
    XFontStruct *font;
    char *s;
    int i;

    font = B->States[0]->Visuals->visual.t_vis.font;

    s = xgets(B->display, B->window, 0, 0, B->width, B->height, WHITE(B->display), BLACK(B->display), font, NULL, E);
    if (s != NULL && *s != '\0') {
        maxrec = max_rec(fileno(SPFile));
        i = atoi(s);
        if (i > maxrec) {
            i = PrevRecord(maxrec);
        } else {
            i = NextRecord(i - 1);
        }
        SetScroll(i);
    }
}

void SetScroll(int i)
{
    char *s;

    TopOfScroll->ext = PW_CAST_INT(i);
    s = TopOfScroll->States[0]->Visuals->visual.t_vis.text;
    sprintf(s, "%d", PW_CAST_PTR_INT(TopOfScroll->ext));
    UpdateButton(TopOfScroll);

    UpdateRecords(-1);
}

void ActivateShowCase(void) { XfActivateButton(ShowCase, ExposureMask); }

void DeactivateShowCase(void) { XfDeactivateButton(ShowCase); }

void ToggleShowCase(Button B, XEvent *E)
{
    toggle_state(B, E);
    if (B->state) {
        ActivateShowCase();
    } else {
        DeactivateShowCase();
    }
}

void CreateShowPanel(Display *display, XFontStruct *font)
{
    Button B;
    Button b;
    struct VisualInfo *V[4];
    char buf[256];
    int i, j;
    int y, height;

    B = XfCreateButton(display, RootWindow(display, DefaultScreen(display)), 350, 350, 101, 300, 1, BLACK(display),
                       "SHOW", 1);
    XfAddButtonVisual(B, 0, XfCreateVisual(B, 0, 0, 0, 0, WHITE(display), WHITE(display), XfSolidVisual));
    ShowCase = B;

    height = 25;
    y = 25;

    XfAddButtonVisual(
        B, 0,
        XfCreateVisual(B, 0, height / 2 - 3, 0, 0, BLACK(display), WHITE(display), XfTextVisual, "SHOW", font, 0));

    for (i = 0; i < NITEMS; i++) {
        sprintf(buf, "%s", Fields[i].name);
        B = XfCreateButton(display, ShowCase->window, 10, y, 80, height, 1, BLACK(display), buf, 2);
        XfAddButtonVisual(
            B, 0,
            XfCreateVisual(B, 30, height / 2 - 3, 0, 0, BLACK(display), WHITE(display), XfTextVisual, buf, font, 1));
        XfAddButtonVisual(
            B, 1,
            XfCreateVisual(B, 30, height / 2 - 3, 0, 0, WHITE(display), BLACK(display), XfTextVisual, buf, font, 1));
        XfAddButtonCallback(B, 0, XF_CALLBACK(MoveShow), NULL);
        XfAddButtonCallback(B, 1, XF_CALLBACK(MoveShow), NULL);
        XfActivateButton(B, ExposureMask | ButtonPressMask);

        y += height + 5;

        b = XfCreateButton(display, B->window, 2, 2, height - 6, height - 6, 0, BLACK(display), buf, 4);

        if (i == 0) {
            V[0] = XfCreateVisual(b, 2, 2, 16, 16, BLACK(display), WHITE(display), XfOutlineVisual);
            V[1] = XfCreateVisual(b, 2, 2, 17, 17, BLACK(display), WHITE(display), XfPixmapVisual, 1,
                                  Bitmap_CheckMark_bits);
            V[2] = XfCreateVisual(b, 2, 2, 16, 16, WHITE(display), BLACK(display), XfOutlineVisual);
            V[3] = XfCreateVisual(b, 2, 2, 17, 17, WHITE(display), BLACK(display), XfPixmapVisual, 1,
                                  Bitmap_CheckMark_bits);
        }
        for (j = 0; j < 4; j++) {
            XfAddButtonVisual(b, j, V[j]);
            XfAddButtonCallback(b, j, XF_CALLBACK(ToggleShow), NULL);
        }
        if (i < 3) {
            b->state = 1;
        }
        XfActivateButtonState(b, b->state, ExposureMask | ButtonPressMask);
        B->member = (int *)b;
        b->ext = B->ext = PW_CAST_INT(i);
    }
}

void MoveShow(Button B, XEvent *E)
{
    int x1, y1;
    int x2, y2;
    int t;
    Button b;

    if (B->state == 1) {
        set_state(B, 0);
        b = (Button)B->member;
        set_state(b, b->state & 1);
        SelectedShow = NULL;
    } else {
        if (SelectedShow == NULL) {
            set_state(B, 1);
            b = (Button)B->member;
            set_state(b, b->state | 2);
            SelectedShow = B;
        } else {
            x1 = B->x;
            y1 = B->y;
            x2 = SelectedShow->x;
            y2 = SelectedShow->y;

            B->x = x2;
            B->y = y2;
            SelectedShow->x = x1;
            SelectedShow->y = y1;

            XMoveWindow(B->display, B->window, x2, y2);
            XMoveWindow(SelectedShow->display, SelectedShow->window, x1, y1);

            set_state(B, 0);

            b = (Button)B->member;
            set_state(b, b->state & 1);

            set_state(SelectedShow, 0);

            b = (Button)SelectedShow->member;
            set_state(b, b->state & 1);

            t = PW_CAST_PTR_INT(B->ext);
            B->ext = SelectedShow->ext;
            SelectedShow->ext = PW_CAST_INT(t);

            t = Order[PW_CAST_PTR_INT(B->ext)];
            Order[PW_CAST_PTR_INT(B->ext)] = Order[PW_CAST_PTR_INT(SelectedShow->ext)];
            Order[PW_CAST_PTR_INT(SelectedShow->ext)] = t;

            t = PW_CAST_PTR_INT(((Button)B->member)->ext);
            ((Button)B->member)->ext = ((Button)SelectedShow->member)->ext;
            ((Button)SelectedShow->member)->ext = PW_CAST_INT(t);

            SelectedShow = NULL;
            UpdateHeader();
            UpdateRecords(-1);
        }
    }
}

void ToggleShow(Button B, XEvent *E)
{
    int state;

    state = B->state;
    state = (state & 2) | ((state + 1) & 1);
    set_state(B, state);
    Order[PW_CAST_PTR_INT(B->ext)] *= -1;

    UpdateHeader();
    UpdateRecords(-1);
}

void ScrollCallback(Button B, XEvent *E)
{
    int i, j;

    if (E->type == Expose) {
        UpdateRecords(-1);
        return;
    }

    /* ButtonPress */
    if (E->type == ButtonPress) {

        while (XCheckMaskEvent(B->display, ButtonPressMask, E))
            ;

        /* figure out which one we clicked on. */

        i = E->xbutton.y / (FontHeight * NLines);
        if (i >= NScroll)
            i = NScroll - 1;

        if (i == SelectedScroll && SelectedTime != 0 && (E->xbutton.time - SelectedTime < 500)) {
            /* This is a double click.  Lets go get pizza */
            FinalSelection(SelectedData);
        } else {
            SelectedTime = E->xbutton.time;
            SelectedScroll = i;
            SelectedData = -1;
            UpdateRecords(i);
        }
    } else if (E->type == KeyPress) {

        while (XCheckMaskEvent(B->display, KeyPressMask, E))
            ;

        SelectedData = -1;
        switch (XLookupKeysym(&(E->xkey), 0)) {
        case XK_Up:
            move_scroll(1);
            UpdateRecords(SelectedScroll);
            break;
        case XK_Down:
            move_scroll(-1);
            UpdateRecords(SelectedScroll);
            break;
        case XK_Prior:
            move_scroll(-NScroll);
            UpdateRecords(SelectedScroll);
            break;
        case XK_Next:
            move_scroll(NScroll);
            UpdateRecords(SelectedScroll);
            break;
        case XK_Return:
            FinalSelection(SelectedData);
            break;
        }
    }
}

void FinalSelection(int i)
{
    struct _label label;
    char *data;
    int j;

    j = read_specpr(fileno(SPFile), i, &label, &data);

    if (j > 0) {
        ApplySpecpr(label.itchan, (float *)data);
        DeactivateSP();
    }
}

void tmpquit(Button B, XEvent *E) { DeactivateSP(); }

void RequestorResizeCallback(Button B, XEvent *E)
{
    int width, height;
    Display *display;

    width = E->xconfigure.width;
    height = E->xconfigure.height;

    if (width < 100 || height < 100)
        return;
    if (width == B->width && height == B->height)
        return;

    B->width = width;
    B->height = height;

    /*
        ResizeButton(SpecprFilename,
            SpecprFilename->x, SpecprFilename->y, width - 30, 20);
    */

    ResizeButton(ScrollHeader, ScrollHeader->x, ScrollHeader->y, width - 30, 20);

    ResizeButton(ScrollWindow, ScrollWindow->x, ScrollWindow->y, width - 30, height - 110);

    ResizeButton(ControlCase, ControlCase->x, height - 45, 400, 50);

    NScroll = ScrollWindow->height / (FontHeight * NLines);
}

ReadRecord(int i, struct filedata *d)
{
    /* return 0 if record is a partial entry */
    /* return -1 if no such record. */

    struct _label label;
    char *data;
    int j;

    /*
        return(read_specpr(SPFile, i, d));
    */

    j = read_specpr(fileno(SPFile), i, &label, &data);
    if (j > 0) {
        if (check_bit(label.icflag, 1)) {
            specpr_text_parse((struct _tlabel *)&label, d);
        } else {
            specpr_data_parse(&label, d);
        }
        d->rec = i;
        d->title[40] = '\0';
        free(data);
        return (i + j);
    }
    if (j < -1)
        j = -1;
    return (j);
}

/*
r2ead_specpr(fp, i, fdata)
FILE *fp;
int i;
struct filedata *fdata;
{
    static char buf[RECSIZE];

    maxrec = max_rec(fileno(SPFile));
    if (i > maxrec) return(-1);

    if (fseek(fp, RECSIZE * i, 0)) {
        return(-1);
    }
    fread(buf, RECSIZE, 1, fp);

    if (check_bit(buf, 0)) {
        return(0);
    } else {
        if (check_bit(buf, 1)) {
        } else {
        }
    }
    return(i);
}
*/

int specpr_text_parse(struct _tlabel *lbl, struct filedata *d)
{
    strncpy(d->title, lbl->ititl, 40);
    d->chans = lbl->itxtch;
    strcpy(d->date, "");
    strcpy(d->time, "");
    d->airmass = 0;
    strncpy(d->user, lbl->usernm, 8);
    strncpy(d->autohist, " ", 1);
    strncpy(d->manhist, " ", 1);
    return 0;
}

int specpr_data_parse(struct _label *lbl, struct filedata *d)
{
    char tbuf[16];

    strncpy(d->title, lbl->ititl, 40);
    d->chans = lbl->itchan;
    strncpy(d->date, decode_date(lbl->jdateb, tbuf), 8);
    strncpy(d->time, decode_time(lbl->isctb, tbuf), 8);
    d->airmass = lbl->irmas;
    strncpy(d->user, lbl->usernm, 8);
    strncpy(d->autohist, lbl->ihist, 60);
    strncpy(d->manhist, lbl->mhist, 296);
    return 0;
}

void select_none(int B, int E)
{
    DeactivateSP();
    DeleteWaves();
}

#endif
