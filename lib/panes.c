#include "Xfred.h"
#include <X11/keysym.h>
#include <stdlib.h>

#define MAX_X 12 /* Number of Buttons across */
#define MAX_Y 8 /* Number of Buttons down */
#define SIZE_X 30 /* Size of button in X direction */
#define SIZE_Y 30 /* Size of button in Y direction */
#define BORDER 1 /* Size of border on buttons */

#define BADMOVE XBell(display, 50)
#define Primary(i) (i == 1 || i == 2 || i == 4)
#define Secondary(i) (i == 3 || i == 5 || i == 6)

Display *display;
int screen;
XColor Colors[8];
GC gc;
XFontStruct *font;

Button Case;
Button(B[MAX_X][MAX_Y]);
static char *cross_bitmap;
int cx = 0;
int cy = 0;
int pushed = 0;
int moves = 0;
int panes;
static Button destroy_pending;
int action(Button b, XEvent *E);
void removeButton(Button B, XEvent *E);

struct stack {
    int x1, y1;
    int x2, y2;
    int a;
    int b;
    int c;
    struct stack *next;
} *head;

// Function prototypes
int Next(int i, int j);
int Land(int i, int j);
int InitColors(void);
void CreateWindow(void);
void CreateVisuals(void);
void AddCellVisuals(Button b);
Button CreateButton(int i, int j);
void Set(int i, int j);
void Clear(int i, int j);
void Update(Button B, int i);
void HandleKeyPress(XEvent *E);
void undo(void);
int more_moves(void);
void GoodBye(void);
void GetFont(void);
void Intro(void);
static int text_pixel_width(const char *s);
static Button create_overlay(const char *line1, const char *line2, const char *line3);
void push(int x1, int y1, int x2, int y2, int a, int b, int c);
int pop(int *x1, int *y1, int *x2, int *y2, int *a, int *b, int *c);

/* This checks the NEXT block for valid jump.
    Returns value of NEXT block after jump
    Returns -1 if not a valid jump,

    Okay to move    if next block is same,
                    if we are primary and next is primary,
                    or next is secondary and we are in it.
*/
int Next(int i, int j)
{
    int current;
    int next;

    if (i == 0 && j == 0)
        return (-1);
    if ((cx + i) < 0 || (cx + i) >= MAX_X)
        return (-1);
    if ((cy + j) < 0 || (cy + j) >= MAX_Y)
        return (-1);

    current = (B[cx][cy])->state % 7;
    next = (B[cx + i][cy + j])->state % 7;

    if (next == current || (Primary(current) && Primary(next)))
        return (0);
    if (Primary(current) && Secondary(next) && (next & current))
        return (next - current);

    return (-1);
}

/* This checks landing block for valid land
    Returns value of landing block after jump
    Returns -1 if illegal jump

    Okay to land        if Empty,
                        if Same color,
                        if Primary to Primary (makes secondary)
*/
int Land(int i, int j)
{
    int current;
    int land;

    if (i == 0 && j == 0)
        return (-1);
    if ((cx + i) < 0 || (cx + i) >= MAX_X)
        return (-1);
    if ((cy + j) < 0 || (cy + j) >= MAX_Y)
        return (-1);

    current = (B[cx][cy])->state % 7;
    land = (B[cx + i][cy + j])->state % 7;

    if (land == 0 || current == land)
        return (current);
    if (Primary(current) && Primary(land))
        return (current + land);

    return (-1);
}

char *ColorName[] = {"White", "Red", "Blue", "Magenta", "Yellow", "#ff8d00", "Green", "Black"};

int InitColors(void)
{
    int i;
    Colormap cmap = DefaultColormap(display, screen);

    for (i = 0; i < 8; i++) {
        if (!XParseColor(display, cmap, ColorName[i], &Colors[i]))
            return (0);
        if (!XAllocColor(display, cmap, &Colors[i]))
            return (0);
    }
    return 1;
}

void CreateWindow(void)
{
    Case = XfCreateButton(display, RootWindow(display, screen), 10, 10, (SIZE_X + BORDER) * MAX_X + 1,
                          (SIZE_Y + BORDER) * MAX_Y + 1, (unsigned long)1, BLACK(display), "Caseing", 1);
    XfActivateButton(Case, ExposureMask);
}

void CreateVisuals(void)
{
    int i, j;
    int rowbytes = SIZE_X / 8 + 1;

    /*
     * Cross bitmap for valid-move markers. Built once; each cell gets its own
     * pixmap copy via XfCreateVisual (pixmaps must not be shared across buttons).
     */
    cross_bitmap = (char *)calloc((size_t)rowbytes * (SIZE_Y + 1), 1);
    if (cross_bitmap == NULL) {
        fprintf(stderr, "panes: out of memory\n");
        exit(1);
    }

    for (j = 0; j < SIZE_Y; j++) {
        for (i = 0; i < SIZE_X; i++) {
            if (j == SIZE_Y / 2 || i == SIZE_X / 2)
                cross_bitmap[j * rowbytes + (i / 8)] |= (1 << (i % 8));
        }
    }
}

void AddCellVisuals(Button b)
{
    int k;

    for (k = 0; k < 7; k++) {
        XfAddButtonVisual(b, k,
                          XfCreateVisual(b, 0, 0, SIZE_X, SIZE_Y, Colors[k].pixel, (unsigned long)0, XfSolidVisual));
        XfAddButtonVisual(
            b, k + 7,
            XfCreateVisual(b, 0, 0, SIZE_X, SIZE_Y, Colors[7].pixel, Colors[k].pixel, XfPixmapVisual, 1, cross_bitmap));
        XfAddButtonVisual(
            b, k + 14,
            XfCreateVisual(b, 0, 0, SIZE_X, SIZE_Y, Colors[0].pixel, Colors[k].pixel, XfPixmapVisual, 1, cross_bitmap));
    }
    /* Jump-target outline on primary-color states (each state needs its own struct) */
    XfAddButtonVisual(
        b, 3, XfCreateVisual(b, 1, 1, SIZE_X - 3, SIZE_Y - 3, BLACK(display), (unsigned long)0, XfOutlineVisual));
    XfAddButtonVisual(
        b, 3 + 7, XfCreateVisual(b, 1, 1, SIZE_X - 3, SIZE_Y - 3, BLACK(display), (unsigned long)0, XfOutlineVisual));
    XfAddButtonVisual(
        b, 3 + 14, XfCreateVisual(b, 1, 1, SIZE_X - 3, SIZE_Y - 3, BLACK(display), (unsigned long)0, XfOutlineVisual));
    XfAddButtonVisual(
        b, 5, XfCreateVisual(b, 1, 1, SIZE_X - 3, SIZE_Y - 3, BLACK(display), (unsigned long)0, XfOutlineVisual));
    XfAddButtonVisual(
        b, 5 + 7, XfCreateVisual(b, 1, 1, SIZE_X - 3, SIZE_Y - 3, BLACK(display), (unsigned long)0, XfOutlineVisual));
    XfAddButtonVisual(
        b, 5 + 14, XfCreateVisual(b, 1, 1, SIZE_X - 3, SIZE_Y - 3, BLACK(display), (unsigned long)0, XfOutlineVisual));
    XfAddButtonVisual(
        b, 6, XfCreateVisual(b, 1, 1, SIZE_X - 3, SIZE_Y - 3, BLACK(display), (unsigned long)0, XfOutlineVisual));
    XfAddButtonVisual(
        b, 6 + 7, XfCreateVisual(b, 1, 1, SIZE_X - 3, SIZE_Y - 3, BLACK(display), (unsigned long)0, XfOutlineVisual));
    XfAddButtonVisual(
        b, 6 + 14, XfCreateVisual(b, 1, 1, SIZE_X - 3, SIZE_Y - 3, BLACK(display), (unsigned long)0, XfOutlineVisual));
}

// Create a wrapper function for action
void actionWrapper(Button b, XEvent *E) { action(b, E); }

Button CreateButton(int i, int j)
{
    Button b;
    char buf[10];
    int k;

    snprintf(buf, sizeof(buf), "%d,%d", i, j);

    b = XfCreateButton(display, Case->window, i * (SIZE_X + BORDER), j * (SIZE_Y + BORDER), SIZE_X, SIZE_Y,
                       (unsigned long)BORDER, BLACK(display), buf, 21);

    AddCellVisuals(b);
    for (k = 0; k < 21; k++)
        XfAddButtonCallback(b, k, XF_CALLBACK(actionWrapper), NULL);

    XfActivateButtonState(b, (rand() % 6) + 1, (ExposureMask | ButtonPressMask | KeyPressMask));
    (B[i][j]) = b;
    return b;
}

int action(Button b, XEvent *E)
{
    int i, j;
    int x, y;
    int count;
    Button current;
    Button next;
    Button land;

    if (E->type == ButtonPress && E->xbutton.button == Button3) {
        printf("Total moves: %d\n", moves);
        exit(1);
    } else if (E->type == KeyPress) {
        HandleKeyPress(E);
        return 1;
    } else if (E->type == ButtonPress && E->xbutton.button == Button2) {
        undo();
        return 1;
    }

    if (sscanf(b->name, "%d,%d", &i, &j) != 2)
        return 0;
    if (i < 0 || j < 0 || i >= MAX_X || j >= MAX_Y)
        return 0;

    if (pushed == 0) {
        if (b->state == 0) {
            BADMOVE;
            return 0;
        }
        cx = i;
        cy = j;
        pushed = 1;
        count = 0;
        for (x = -1; x < 2; x++) {
            for (y = -1; y < 2; y++) {
                if (Next(x, y) != -1 && Land(x * 2, y * 2) != -1) {
                    Set(i + x * 2, j + y * 2);
                    count++;
                }
            }
        }
        if (count == 0) {
            pushed = 0;
            BADMOVE;
            return 0;
        }

        Set(i, j);
        Set(i, j);
    } else {
        if (b != B[cx][cy]) {
            int dx = i - cx;
            int dy = j - cy;
            int land_state;
            int next_state;

            current = (B[cx][cy]);
            if (((i + cx) & 1) != 0 || ((j + cy) & 1) != 0) {
                BADMOVE;
                return 0;
            }
            next = (B[(i + cx) / 2][(j + cy) / 2]);
            land = b;
            if (current == NULL || next == NULL || land == NULL) {
                BADMOVE;
                return 0;
            }

            if (land->state < 7) {
                BADMOVE;
                return 0;
            }
            land_state = Land(dx, dy);
            next_state = Next(dx / 2, dy / 2);
            if (land_state < 0 || next_state < 0) {
                BADMOVE;
                return 0;
            }
            push(cx, cy, i, j, current->state, next->state, land->state);
            moves++;

            Update(land, land_state);
            Update(next, next_state);
            Update(current, 0);
        }
        for (x = -1; x < 2; x++) {
            for (y = -1; y < 2; y++) {
                Clear(cx + x * 2, cy + y * 2);
            }
        }
        pushed = 0;
    }
    if (!more_moves()) {
        GoodBye();
    }
    return 1;
}

void Set(int i, int j)
{
    int k;

    if (i < 0 || j < 0 || i >= MAX_X || j >= MAX_Y)
        return;
    k = (B[i][j])->state + 7;
    Update((B[i][j]), k);
}

void Clear(int i, int j)
{
    int k;
    if (i < 0 || j < 0 || i >= MAX_X || j >= MAX_Y)
        return;
    k = (B[i][j])->state % 7;
    Update((B[i][j]), k);
}

void Update(Button B, int i)
{
    if (B == NULL || i < 0 || i >= B->maxstate)
        return;
    B->state = i;
    (*(B->updateCallback))(B, NULL);
}

int main(int argc, char *argv[])
{
    int i;
    int j;

    XEvent E;
    int depth;

    (void)argc;
    (void)argv;

    if (!initx(NULL, &display, &screen, &depth, &gc)) {
        (void)fprintf(stderr, "Could not initialize X server\n");
        exit(1);
    }
    if (depth < 3) {
        (void)fprintf(stderr, "Cannot run on a %d bit display\n", depth);
        exit(1);
    }
    if (InitColors() == 0) {
        (void)fprintf(stderr, "Could not init colors\n");
        exit(1);
    }
    (void)srand(getpid());
    GetFont();
    XfSetDefaultFont(display, font);
    CreateWindow();
    CreateVisuals();

    panes = MAX_X * MAX_Y;
    for (j = 0; j < MAX_Y; j++) {
        for (i = 0; i < MAX_X; i++) {
            CreateButton(i, j);
        }
    }
    Intro();
    while (1) {
        XNextEvent(display, &E);
        destroy_pending = NULL;
        XfButtonPush(XfEventButton(&E), &E);
        if (destroy_pending != NULL) {
            XfDestroyButton(destroy_pending);
            destroy_pending = NULL;
        }
    }
}

void push(int x1, int y1, int x2, int y2, int a, int b, int c)
{
    struct stack *n;
    n = (struct stack *)malloc(sizeof(struct stack));
    n->x2 = x2;
    n->y2 = y2;
    n->x1 = x1;
    n->y1 = y1;
    n->a = a;
    n->b = b;
    n->c = c;
    n->next = head;
    head = n;
}

int pop(int *x1, int *y1, int *x2, int *y2, int *a, int *b, int *c)
{
    struct stack *n = head;

    if (n == NULL)
        return 0;

    *a = n->a;
    *b = n->b;
    *c = n->c;
    *x1 = n->x1;
    *y1 = n->y1;
    *x2 = n->x2;
    *y2 = n->y2;

    head = head->next;
    free(n);
    return 1;
}

void HandleKeyPress(XEvent *E)
{
    KeySym k;
    k = XLookupKeysym(&E->xkey, 0);
    if (k == XK_BackSpace || k == XK_Delete || k == XK_Escape) {
        undo();
    } else {
        XBell(display, 50);
    }
}

void undo(void)
{
    int x, y;
    int x1, y1;
    int x2, y2;
    int current, next, land;

    if (!pop(&x1, &y1, &x2, &y2, &current, &next, &land)) {
        XBell(display, 50);
        return;
    }
    moves--;
    if (pushed == 1) {
        for (x = -1; x < 2; x++) {
            for (y = -1; y < 2; y++) {
                Clear(cx + x * 2, cy + y * 2);
            }
        }
        pushed = 0;
    }
    Update(B[x1][y1], current);
    Update(B[x2][y2], land);
    Update(B[(x1 + x2) / 2][(y1 + y2) / 2], next);
}

int more_moves(void)
{
    int i;
    int j;
    int x;
    int y;
    int t_x = cx;
    int t_y = cy;

    for (j = 0; j < MAX_Y; j++) {
        for (i = 0; i < MAX_X; i++) {
            if ((B[i][j])->state == 0)
                continue;

            cx = i;
            cy = j;
            for (x = -1; x < 2; x++) {
                for (y = -1; y < 2; y++) {
                    if (Next(x, y) != -1 && Land(x * 2, y * 2) != -1) {
                        cx = t_x;
                        cy = t_y;
                        return (1);
                    }
                }
            }
        }
    }
    return 0;
}

static int text_pixel_width(const char *s)
{
    XCharStruct xcs;
    int dir, asc, des;

    XTextExtents(font, s, (int)strlen(s), &dir, &asc, &des, &xcs);
    return xcs.width;
}

static Button create_overlay(const char *line1, const char *line2, const char *line3)
{
    Button B;
    int line_h = font->ascent + font->descent;
    int ypos = line_h;
    int width = line1 ? text_pixel_width(line1) : 0;
    int height;
    int w2, w3;

    if (line2) {
        w2 = text_pixel_width(line2);
        if (w2 > width)
            width = w2;
    }
    if (line3) {
        w3 = text_pixel_width(line3);
        if (w3 > width)
            width = w3;
    }
    width += 24;
    height = line_h * 4 + 24;

    B = XFCreateButton(display, Case->window, Case->width / 2 - width / 2, Case->height / 2 - height / 2, width, height,
                       1, BLACK(display), WHITE(display), "overlay", 1);
    XfAddButtonVisual(B, 0, XfCreateVisual(B, 0, 0, 0, 0, WHITE(display), WHITE(display), XfSolidVisual));
    if (line1) {
        XfAddButtonVisual(
            B, 0, XfCreateVisual(B, 0, ypos, 0, 0, BLACK(display), WHITE(display), XfTextVisual, line1, font, 0));
        ypos += line_h + 6;
    }
    if (line2) {
        XfAddButtonVisual(
            B, 0, XfCreateVisual(B, 0, ypos, 0, 0, BLACK(display), WHITE(display), XfTextVisual, line2, font, 0));
        ypos += line_h + 6;
    }
    if (line3) {
        XfAddButtonVisual(
            B, 0, XfCreateVisual(B, 0, ypos, 0, 0, BLACK(display), WHITE(display), XfTextVisual, line3, font, 0));
    }
    XfAddButtonCallback(B, 0, XF_CALLBACK(removeButton), NULL);
    XfActivateButton(B, ExposureMask | ButtonPressMask);
    return B;
}

void GoodBye(void)
{
    char buf[256];

    snprintf(buf, sizeof(buf), "TOTAL MOVES: %d", moves);
    create_overlay("NO MORE MOVES", buf, NULL);
}

void removeButton(Button B, XEvent *E)
{
    (void)E;
    /* Destroy after XfButtonPush returns; destroying here frees the callback list */
    destroy_pending = B;
}

void GetFont(void)
{
    font = XLoadQueryFont(display, "terminal14");
    if (font == NULL)
        font = XLoadQueryFont(display, "fixed");
    if (font == NULL) {
        fprintf(stderr, "Cant find fonts\n");
        exit(1);
    }
    XSetFont(display, gc, font->fid);
}

void Intro(void) { create_overlay("PANES", "A Stained Glass Clone", "Click Here To Start"); }
