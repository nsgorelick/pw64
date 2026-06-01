#include "Xfred.h"
#include <X11/keysym.h>
#include <stdlib.h>

#define MAX_X   12      /* Number of Buttons across */
#define MAX_Y   8       /* Number of Buttons down */
#define SIZE_X  30      /* Size of button in X direction */
#define SIZE_Y  30      /* Size of button in Y direction */
#define BORDER  1       /* Size of border on buttons */

#define BADMOVE XBell(display, 50)
#define Primary(i)      (i == 1 || i == 2 || i == 4)
#define Secondary(i)    (i == 3 || i == 5 || i == 6)

Display *display;
int screen;
XColor Colors[8];
GC gc;
Font font;

Button Case;
Button (B[MAX_X][MAX_Y]);
struct VisualInfo *V[22];
int cx = 0;
int cy = 0;
int pushed = 0;
int moves=0;
int panes;
int action(Button b, XEvent *E);
void removeButton(Button B, XEvent *E);

struct stack {
    int x1,y1;
    int x2,y2;
    int a;
    int b;
    int c;
    struct stack *next;
} *head;

// Function prototypes
int Next(int i, int j);
int Land(int i, int j);
int InitColors();
void CreateWindow();
void CreateVisuals();
Button CreateButton(int i, int j);
void Set(int i, int j);
void Clear(int i, int j);
void Update(Button B,int i);
void HandleKeyPress(XEvent *E);
void undo();
int more_moves();
void GoodBye();
void GetFont();
void Intro();
void push(int x1, int y1, int x2, int y2, int a, int b, int c);
int pop(int *x1, int *y1, int *x2, int *y2, int *a, int *b, int *c);

/* This checks the NEXT block for valid jump.
    Returns value of NEXT block after jump
    Returns -1 if not a valid jump,

    Okay to move    if next block is same,
                    if we are primary and next is primary,
                    or next is secondary and we are in it.
*/
int
Next(int i, int j)
{
    int current;
    int next;

    if (i == 0 && j == 0) return(-1);
    if ((cx + i) < 0 || (cx + i) >= MAX_X) return(-1);
    if ((cy + j) < 0 || (cy + j) >= MAX_Y) return(-1);

    current = (B[cx][cy])->state % 7;
    next = (B[cx+i][cy+j])->state % 7;

    if (next == current || (Primary(current) && Primary(next))) return(0);
    if (Primary(current) && Secondary(next) && (next & current))
        return(next - current);

    return(-1);
}



/* This checks landing block for valid land
    Returns value of landing block after jump
    Returns -1 if illegal jump

    Okay to land        if Empty,
                        if Same color,
                        if Primary to Primary (makes secondary)
*/
int
Land(int i, int j)
{
    int current;
    int land;

    if (i == 0 && j == 0) return(-1);
    if ((cx + i) < 0 || (cx + i) >= MAX_X) return(-1);
    if ((cy + j) < 0 || (cy + j) >= MAX_Y) return(-1);

    current = (B[cx][cy])->state % 7;
    land = (B[cx+i][cy+j])->state % 7;

    if (land == 0 || current == land) return(current);
    if (Primary(current) && Primary(land)) return(current + land);

    return(-1);
}



char *ColorName[] = {
    "White",
    "Red",
    "Blue",
    "Magenta",
    "Yellow",
    "#ff8d00",
    "Green",
    "Black"
};



int
InitColors()
{
    int i;
    Colormap cmap = DefaultColormap(display, screen);

    for (i = 0 ; i < 8 ; i++) {
        if (!XParseColor(display, cmap, ColorName[i], &Colors[i])) return(0);
        if (!XAllocColor(display, cmap, &Colors[i])) return(0);
    }
    return 1;
}

void
CreateWindow()
{
    Case = XfCreateButton(display, RootWindow(display, screen),
        10, 10, (SIZE_X+BORDER) * MAX_X+ 1, (SIZE_Y+BORDER) * MAX_Y + 1, 
        (unsigned long)1, BLACK(display), "Caseing", 1);
    XfActivateButton(Case, ExposureMask);
}


void
CreateVisuals()
{
    int i, j;
    char    *bitmap;

/*
 * This creates a cross to mark valid moves.  Creation is at runtime
 * so SIZE can be changed without messing anything up.
 *
 * (Note:   I really don't the way this looks on the board)
 */

    bitmap = (char *)malloc((unsigned)((SIZE_X/8+1) * (SIZE_Y+1)));

    for (j = 0 ; j < SIZE_Y ; j++) {
        for (i = 0 ; i <= SIZE_X ; i++) {
            if (j == SIZE_Y / 2 || i == SIZE_X / 2) {
                bitmap[j*(SIZE_X/8+1)+(i/8)] |= (1 << (i%8));
            } else {
                bitmap[j*(SIZE_X/8+1)+(i/8)] = 0;
            }
        }
    }

    for (i = 0 ; i < 7 ; i++) {
        V[i] = XfCreateVisual(Case, 0, 0, SIZE_X, SIZE_Y, Colors[i].pixel,
            (unsigned long)0, XfSolidVisual);
        V[i+7] = XfCreateVisual(Case, 0, 0, SIZE_X, SIZE_Y, Colors[7].pixel,
            Colors[i].pixel, XfPixmapVisual, 1, bitmap);
        V[i+14] = XfCreateVisual(Case, 0, 0, SIZE_X, SIZE_Y, Colors[0].pixel,
            Colors[i].pixel, XfPixmapVisual, 1, bitmap);
    }
    V[21] = XfCreateVisual(Case, 1, 1, SIZE_X-3, SIZE_Y-3, BLACK(display),
            (unsigned long)0, XfOutlineVisual);

    free(bitmap);
}


// Create a wrapper function for action
void actionWrapper(Button b, XEvent *E) {
    action(b, E);
}

Button
CreateButton(int i, int j)
{
    Button b;
    char buf[10];
    int k;

    sprintf(buf, "%d,%d", i, j);

    b = XfCreateButton(display, Case->window,
        i * (SIZE_X + BORDER), j * (SIZE_Y + BORDER), 
        SIZE_X, SIZE_Y, (unsigned long)BORDER, 
        BLACK(display), buf, 21);

    for (k = 0; k < 21; k++) {
        XfAddButtonVisual(b, k, V[k]);
        XfAddButtonCallback(b, k, actionWrapper, NULL);
    }
    XfAddButtonVisual(b, 3,   V[21]);
    XfAddButtonVisual(b, 3+7, V[21]);
    XfAddButtonVisual(b, 3+14,V[21]);
    XfAddButtonVisual(b, 5,   V[21]);
    XfAddButtonVisual(b, 5+7, V[21]);
    XfAddButtonVisual(b, 5+14,V[21]);
    XfAddButtonVisual(b, 6,   V[21]);
    XfAddButtonVisual(b, 6+7, V[21]);
    XfAddButtonVisual(b, 6+14,V[21]);

    XfActivateButtonState(b, (rand()%6)+1, 
        (ExposureMask | ButtonPressMask | KeyPressMask));
    (B[i][j]) = b;
}


int action(Button b, XEvent *E) {
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

    sscanf(b->name, "%d,%d", &i, &j);
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
            current = (B[cx][cy]);
            next = (B[(i + cx) / 2][(j + cy) / 2]);
            land = b;

            if (land->state < 7) {
                BADMOVE;
                return 0;
            }
            push(cx, cy, i, j, current->state, next->state, land->state);
            moves++;

            Update(land, Land(i - cx, j - cy));
            Update(next, Next((i - cx) / 2, (j - cy) / 2));
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


void
Set(int i, int j)
{
    int k = (B[i][j])->state + 7;
    Update((B[i][j]),k);
}


void
Clear(int i, int j)
{
    int k;
    if (i < 0 || j < 0 || i >= MAX_X || j >= MAX_Y) return;
    k = (B[i][j])->state % 7;
    Update((B[i][j]), k);
}


void
Update(Button B,int i)
{
    B->state = i;
    (*(B->updateCallback))(B, NULL);
}

int main(int argc, char *argv[])
{
    int i;
    int j;

    XEvent E;
    int depth;

    if (!initx(NULL, &display, &screen, &depth, &gc)) {
        (void)fprintf(stderr, "Could not initialize X server\n");
        exit(1);
    }
    if (depth < 3) {
        (void)fprintf(stderr, "Cannot run on a %d bit display\n",depth);
        exit(1);
    }
    if (InitColors() == 0) {
        (void)fprintf(stderr, "Could not init colors\n");
        exit(1);
    }
    (void)srand(getpid());
    GetFont();
    CreateWindow();
    CreateVisuals();

    panes = MAX_X*MAX_Y;
    for (j = 0 ; j < MAX_Y ; j++) {
        for (i = 0 ; i < MAX_X ; i++) {
            CreateButton(i, j);
        }
    }
    Intro();
    while (1) {
        XNextEvent(display, &E);
        XfButtonPush(XfEventButton(&E), &E);
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
    n->a = a % 7;
    n->b = b % 7;
    n->c = c % 7;
    n->next = head;
    head = n;
}

int pop(int *x1, int *y1, int *x2, int *y2, int *a, int *b, int *c) {
    struct stack *n = head;

    if (n == NULL) return 0;

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

void undo()
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

int more_moves()
{
    int i;
    int j;
    int x;
    int y;
    int t_x=cx;
    int t_y=cy;

    for (j = 0 ; j < MAX_Y ; j++) {
        for (i = 0 ; i < MAX_X ; i++) {
            if ((B[i][j])->state == 0) continue;

            cx = i;
            cy = j;
            for (x = -1 ; x < 2 ; x++) {
                for (y = -1 ; y < 2 ; y++) {
                    if (Next(x,y) != -1 && Land(x*2,y*2) != -1) {
                        cx = t_x;
                        cy = t_y;
                        return(1);
                    }
                }
            }

        }
    }
    return 0;
}

void GoodBye()
{
    Button B;
    char buf[256];
    int width, height;
    XFontStruct *fs;
    int asc, desc, dir;
    XCharStruct xcs;

    strcpy(buf,"NO MORE MOVES");
    fs = XQueryFont(display, font);
    XTextExtents(fs, buf, strlen(buf), &dir, &asc, &desc, &xcs);

    width = xcs.width*1.4;
    height = (xcs.ascent + xcs.descent)*3.5;

    B = XfCreateButton(display, Case->window, 
        Case->width/2 - width/2, Case->height/2 - height/2, width,height,
        (unsigned long)1, BLACK(display), "Out'o Moves", 1);
    XfAddButtonVisual(B, 0, XfCreateVisual(B, 0,
            (int)(height*0.3), 0, 0, BLACK(display),
            WHITE(display), XfTextVisual, buf, font, 0));
    sprintf(buf, "TOTAL MOVES: %d",moves);
    XfAddButtonVisual(B, 0, XfCreateVisual(B, 0,
            (int)(height*0.6), 0, 0, BLACK(display),
            WHITE(display), XfTextVisual, buf, font, 0));
    XfAddButtonCallback(B, 0, removeButton, NULL);
    XfActivateButton(B, ExposureMask | ButtonPressMask);
}

void removeButton(Button B, XEvent *E)
{
    XfDestroyButton(B);
}

void GetFont()
{
	XFontStruct *fs;

    fs = XLoadQueryFont(display, "terminal14");
    if (fs == NULL)
    fs = XLoadQueryFont(display, "fixed");
    if (fs == NULL) {
        fprintf(stderr, "Cant find fonts\n");
        exit(1);
    }
	font = fs->fid;
    XSetFont(display, gc, font);
}

void Intro()
{
    Button B;
    char buf[256];
    XFontStruct *fs;
    int asc, desc, dir;
    XCharStruct xcs;
    int width,height;

    strcpy(buf,"A Stained Glass Clone");
    fs = XQueryFont(display, font);
    XTextExtents(fs, buf, strlen(buf), &dir, &asc, &desc, &xcs);

    width = xcs.width*1.25;
    height = (xcs.ascent + xcs.descent)*7;

    B = XfCreateButton(display, Case->window, 
        Case->width/2 - width/2, Case->height/2 - height/2, width,height,
        (unsigned long)1, BLACK(display), "Out'o Moves", 1);
    strcpy(buf,"PANES");
    XfAddButtonVisual(B, 0, XfCreateVisual(B, 0,
            (int)(height*0.1), 0, 0, BLACK(display),
            WHITE(display), XfTextVisual, buf, font, 0));
    strcpy(buf,"A Stained Glass Clone");
    XfAddButtonVisual(B, 0, XfCreateVisual(B, 0,
            (int)(height*0.25), 0, 0, BLACK(display),
            WHITE(display), XfTextVisual, buf, font, 0));
    sprintf(buf, "Click Here To Start");
    XfAddButtonVisual(B, 0, XfCreateVisual(B, 0,
            (int)(height*0.8), 0, 0, BLACK(display),
            WHITE(display), XfTextVisual, buf, font, 0));
    XfAddButtonCallback(B, 0, removeButton, NULL);
    XfActivateButton(B, ExposureMask | ButtonPressMask);
}
