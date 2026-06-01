#include <fcntl.h>
#include <math.h>
#include <values.h>
#include "Xfred.h"
#include <X11/keysym.h>
#include "bitmaps/bitmaps.h"
#include "mag.h"
#include "block.h"

static int      anchor_x = -1, anchor_y = -1;
static int      anchor_w = 0, anchor_h = 0;
static int      anchor = 0;
static int      last_x1, last_y1;
static int      last_x2, last_y2;

int NBlocks = 6;

struct block_node *new_block(int x, int y, struct stack_node *s);
struct stack_node *stack = NULL;
struct block *Blocks[7];
int CurrentBlock=-1;
int BlockAppendReplace = 0;
int BlockCycle = 0;

int             BlockEnabled = 0;
extern int      Pixels[6];

struct PlotStruct *ps;


int GetBlockType (int i);
int set_anchor (int x, int y);
int RubberBox (Display *display, int x, int y);
int AddBlock (int x, int y);
int ToggleBlock (int x, int y);
int DeleteBlock (int x, int y);
int ClearRubberBox (Display *display);
extern int DoAutoCycle (void);
extern int MagUnconvert (int xin, int yin, int *xout, int *yout);
extern int MagConvert (int xin, int yin, int *xout, int *yout);
extern void LocalUpdateMagnify (Display *display, struct magnify *Mag, int x1, int y1, int x2, int y2);
int SetBlockState (int i, int state);
extern int ColorMag (int x, int y, int color, int type);
extern int UncolorMag (int x, int y);
int delete_all (int i);
int TogglePoint (int x, int y);

BlockMovement(Display *display, int x, int y, int type, int buttons)
{
    int             x1, y1, x2, y2;

    if (CurrentBlock < 0 || CurrentBlock == NBlocks)
        return;
    if (BlockEnabled == 0)
        return;
	if (GetBlockType(CurrentBlock) != BK_EXTRACTED)
		return;

    switch (type) {
    case ButtonPress:
        {
            set_anchor(x, y);
            anchor = 1;
            break;
        }
    case MotionNotify:
        {
            if (!anchor)
                return;
            RubberBox(display, x, y);
            break;
        }
    case ButtonRelease:
        {
            if (buttons == Button1) {
                AddBlock(x, y);
            } else if (buttons == Button2) {
                ToggleBlock(x, y);
            } else if (buttons == Button3) {
                DeleteBlock(x, y);
            }
            ClearRubberBox(display);
			if (BlockCycle) DoAutoCycle();
            anchor = 0;
            break;
        }
    }
}
set_anchor(int x, int y)
{
    /*
     * This rounds the anchor points to be exactly on the UL corner
     */
    MagUnconvert(x, y, &anchor_x, &anchor_y);
    MagConvert(anchor_x, anchor_y, &x, &y);

    anchor_x = x;
    anchor_y = y;
    anchor_w = 0;
    anchor_h = 0;
    last_x2 = -1;
    last_y2 = -1;
}
/*
 * Draw box from anchor_x, anchor_y to x,y
 */
RubberBox(Display *display, int x, int y)
{
    extern struct magnify *Mag;
    extern GC       gc;
    int             x1, x2, y1, y2;
    int             a_x, a_y;

    if (CurrentBlock == -1)
        return;

    a_x = anchor_x;
    a_y = anchor_y;
    if (x >= anchor_x) {
        x++;
    } else {
        a_x++;
    }
    if (y >= anchor_y) {
        y++;
    } else {
        a_y++;
    }
    MagUnconvert(a_x, a_y, &x1, &y1);
    MagUnconvert(x, y, &x2, &y2);

    if (x2 == last_x2 && y2 == last_y2)
        return;


    if (last_x2 != -1 || last_y2 != -1) {
        int             x1, y1;
        int             x2, y2;

        x1 = last_x1;
        y1 = last_y1;
        x2 = last_x2;
        y2 = last_y2;
        LocalUpdateMagnify(display, Mag, x1, y1, x1 + 1, y2 + 1);
        LocalUpdateMagnify(display, Mag, x1, y2, x2 + 1, y2 + 1);
        LocalUpdateMagnify(display, Mag, x2, y2, x2 + 1, y1 + 1);
        LocalUpdateMagnify(display, Mag, x2, y1, x1 + 1, y1 + 1);
    }
    XSetForeground(display, gc, Pixels[CurrentBlock % 6]);

    XDrawLine(display, Mag->window, gc, x1, y1, x1, y2);
    XDrawLine(display, Mag->window, gc, x1, y2, x2, y2);
    XDrawLine(display, Mag->window, gc, x2, y2, x2, y1);
    XDrawLine(display, Mag->window, gc, x2, y1, x1, y1);

    last_x1 = x1;
    last_y1 = y1;
    last_x2 = x2;
    last_y2 = y2;
}
ClearRubberBox(Display *display)
{
    int             x1, y1, x2, y2, x3, y3, x4, y4;
    extern struct magnify *Mag;

    MagUnconvert(anchor_x, anchor_y, &x1, &y1);
    x1 = last_x1;
    y1 = last_y1;
    x2 = last_x2;
    y2 = last_y2;
    if (x1 > x2) {
        int             t;
        t = x1;
        x1 = x2;
        x2 = t;
    }
    if (y1 > y2) {
        int             t;
        t = y1;
        y1 = y2;
        y2 = t;
    }
    LocalUpdateMagnify(display, Mag, x1 - 1, y1 - 1, x1 + 1, y2 + 1);
    LocalUpdateMagnify(display, Mag, x1 - 1, y1 - 1, x2 + 1, y1 + 1);
    LocalUpdateMagnify(display, Mag, x1 - 1, y2 - 1, x2 + 1, y2 + 1);
    LocalUpdateMagnify(display, Mag, x2 - 1, y1 - 1, x2 + 1, y2 + 1);
}
struct stack_node *
new_stack(int x, int y)
{
    struct stack_node *s;
    s = (struct stack_node *) calloc(1,(unsigned int) sizeof(struct stack_node));
    s->x = x;
    s->y = y;
    s->next = NULL;
    s->colors = NULL;
    return (s);
}


struct block_node *
new_block(int x, int y, struct stack_node *s)
{
    struct stack_node *t;
    struct block_node *n;
    struct color_node *c, *d;
    int npts;

    /*
     * * special case: * move color to top of colors stack
     */
    if (s != NULL && s->y == y && s->x == x) {
        c = s->colors;
        if (c->color != CurrentBlock) {
            while (c->next != NULL && c->next->color != CurrentBlock) {
                c = c->next;
            }
            if (c->next != NULL) {
                d = c->next;
                c->next = d->next;
                d->next = s->colors;
                s->colors = d;
            }
        }
        return (NULL);
    }
    /*
     * * Find either existing stack entry or the place where * new one is
     * to go.
     */
    if (s == NULL) {
        s = stack;
    }
    if (s == NULL || s->y > y || (s->y == y && s->x > x)) {
        t = new_stack(x, y);
        t->next = stack;
        stack = t;
    } else {
        t = s;
        if (!(s->y == y && s->x == x)) {
            while (s->next != NULL) {
                t = s->next;
                if (t->y == y && t->x == x)
                    break;
                if (t->y > y || (t->y == y && t->x > x)) {
                    break;
                }
                s = s->next;
            }
            if (!(t->y == y && t->x == x)) {
                t = new_stack(x, y);
                t->next = s->next;
                s->next = t;
            }
        }
    }

    c = (struct color_node *) calloc(1,(unsigned int) sizeof(struct color_node));
    c->color = CurrentBlock;
    c->next = t->colors;
    t->colors = c;

    n = (struct block_node *) calloc(1,(unsigned int) sizeof(struct block_node));
    n->stack = t;
    n->next = NULL;
    n->type = BK_EXTRACTED;
    n->color = -1;

    if (ps->get) 
		n->pdata = (*(ps->get))(x, y);

    /** 
    ** This scales and draws this data, so we dont have to redraw 
    ** the whole plot
    **/
    if (ps->scale) n->xpoints = (*(ps->scale))(CurrentBlock, n->pdata, 1.0);
    if (ps->draw) (*(ps->draw))(Pixels[CurrentBlock % 6],
                                n->xpoints->npoints,
                                n->xpoints->data);

    return (n);
}

delete_block(struct block_node *n)
{
    struct stack_node *s, *t;
    struct color_node *c, *d;
    int             ret;

    ret = -1;
    if (n == NULL)
        return;
    s = n->stack;
    c = s->colors;
    if (c->color == CurrentBlock) {
        s->colors = c->next;
        free((char *) c);
        if (s->colors == NULL) {
            if (stack == s) {
                stack = s->next;
            } else {
                t = stack;
                while (t->next != s)
                    t = t->next;
                t->next = t->next->next;
            }
            free((char *) s);
            ret = -1;
        } else {
            ret = s->colors->color;
        }
    } else {
        while (c->next != NULL) {
            if (c->next->color == CurrentBlock) {
                d = c->next;
                c->next = c->next->next;
                free((char *) d);
                break;
            }
            c = c->next;
        }
        ret = s->colors->color;
    }
    /* FREE */
    if (n->pdata) free_pdata(n->pdata);
    free(n);
    return (ret);
}


AddPoint(int x, int y)
{
    struct block_node *n, *m;
    struct stack_node *s;

    if (Blocks[CurrentBlock] == NULL) {
		/**
		 ** This will malloc the memory for the block
		 **/
		SetBlockState(CurrentBlock, 1);
    }
    n = Blocks[CurrentBlock]->block;

    if (n == NULL || n->stack->y > y || (n->stack->y == y && n->stack->x > x)) {
        n = new_block(x, y, (struct stack_node *) NULL);
        n->next = Blocks[CurrentBlock]->block;
        Blocks[CurrentBlock]->block = n;
        Blocks[CurrentBlock]->nblocks++;
    } else {
        if (n->stack->y == y && n->stack->x == x) {
            (void) new_block(x, y, n->stack);   /* just moves to top */
        } else {
            while (n->next != NULL) {
                s = n->next->stack;
                if ((s->y > y) || (s->y == y && s->x > x)) {
                    break;
                } else if (s->y == y && s->x == x) {
                    (void) new_block(x, y, s);  /* just moves to top */
                    ColorMag(x, y, Pixels[CurrentBlock % 6], 1);
                    return;
                }
                n = n->next;
            }
            m = new_block(x, y, n->stack);
            m->next = n->next;
            n->next = m;
            Blocks[CurrentBlock]->nblocks++;
        }
    }
    ColorMag(x, y, Pixels[CurrentBlock % 6], 1);
}


DeletePoint(int x, int y)
{
    struct block_node *n, *m;
    struct stack_node *s;
    int             uncolor = -1;

    if (Blocks[CurrentBlock] == NULL) {
        return;
    }
    n = Blocks[CurrentBlock]->block;

    if (n == NULL) {
        return 0;
    } else {
        s = n->stack;
        if (s->y > y || (s->y == y && s->x > x))
            return 0;
        else if (s->x == x && s->y == y) {
            Blocks[CurrentBlock]->block = n->next;
            uncolor = delete_block(n);
            Blocks[CurrentBlock]->nblocks--;
        } else {
            while (n->next != NULL) {
                s = n->next->stack;
                if (s->y > y || (s->y == y && s->x > x))
                    return 0;
                if (s->y == y && s->x == x) {
                    m = n->next;
                    n->next = m->next;
                    uncolor = delete_block(m);
                    Blocks[CurrentBlock]->nblocks--;
                    break;
                }
                n = n->next;
                if (n->next == NULL)
                    return 0;
            }
        }
    }
    if (uncolor != -1)
        ColorMag(x, y, Pixels[uncolor % 6], 1);
    else
        UncolorMag(x, y);

    return 1;
}


AddBlock(int x, int y)
{
    /*
     * Repeatly call add_block for all points between here and anchor.
     */
    int             x1, y1;
    int             a_x, a_y;
    a_x = anchor_x;
    a_y = anchor_y;
    if (x < a_x) {
        int             t;
        t = x;
        x = a_x;
        a_x = t;
    }
    if (y < a_y) {
        int             t;
        t = y;
        y = a_y;
        a_y = t;
    }
    y1 = y;
    if (BlockAppendReplace)
        delete_all(CurrentBlock);
    while (y1 >= a_y) {
        x1 = x;
        while (x1 >= a_x) {
            AddPoint(x1, y1);
            x1--;
        }
        y1--;
    }
    if (ps->draw == NULL || BlockAppendReplace) {
        (*(ps->drawall))();
    }
}


ToggleBlock(int x, int y)
{
    /*
     * Repeatly call toggle_block for all points between here and anchor.
     */
    int             x1, y1;
    int             a_x, a_y;
    a_x = anchor_x;
    a_y = anchor_y;
    if (x < a_x) {
        int             t;
        t = x;
        x = a_x;
        a_x = t;
    }
    if (y < a_y) {
        int             t;
        t = y;
        y = a_y;
        a_y = t;
    }
    y1 = y;
    while (y1 >= a_y) {
        x1 = x;
        while (x1 >= a_x) {
            TogglePoint(x1, y1);
            x1--;
        }
        y1--;
    }
    (*(ps->drawall))();
}


DeleteBlock(int x, int y)
{
    /*
     * Repeatly call delete_block for all points between here and anchor.
     */
    int             x1, y1;
    int             a_x, a_y;
    a_x = anchor_x;
    a_y = anchor_y;
    if (x > a_x) {
        int             t;
        t = x;
        x = a_x;
        a_x = t;
    }
    if (y > a_y) {
        int             t;
        t = y;
        y = a_y;
        a_y = t;
    }
    y1 = y;
    while (y1 <= a_y) {
        x1 = x;
        while (x1 <= a_x) {
            DeletePoint(x1, y1);
            x1++;
        }
        y1++;
    }
    (*(ps->drawall))();
}
TogglePoint(int x, int y)
{
    struct block_node *n, *m;
    struct stack_node *s;
    struct color_node *c, *d;

    if (DeletePoint(x, y) == 0) {
        AddPoint(x, y);
    }
}

ColorBlocks(char *data, int x, int y, int width, int height)
{
    struct stack_node *s;
    struct stack_node *t;
    int             i;

    if (stack == NULL)
        return;
    t = stack;
    while (t != NULL) {
        if (t->y < y || (t->y == y && t->x < x)) {
            t = t->next;
            continue;
        }
        if (t->y > y + height || (t->y == y + height && t->x > x + width)) {
            return;
        }
        i = (((t->y - y) * width) + (t->x - x));
        if (i >= 0 && i <= width * height) {
            ColorMag(t->x, t->y, Pixels[t->colors->color % 8], 2);
        }
        t = t->next;
    }
}

/**
 ** This routine needs to recognize the block type and call the appropriate
 ** deletion routine.  It does not currently do so.
 **/

DeleteAll(Button B, XEvent *E)
{
    delete_all(CurrentBlock);
    (*(ps->drawall))();
}

delete_all(int i)
          	/* block to delete */
{
    int             uncolor;
    struct block_node *n;
    int             x, y;

    if (i == -1 || Blocks[i] == NULL) 
        return;

    if (GetBlockType(i) == BK_EXTRACTED) {
        while (Blocks[i]->block != NULL) {
            n = Blocks[i]->block;
            Blocks[i]->block = n->next;
            uncolor = -1;
            x = n->stack->x;
            y = n->stack->y;
            uncolor = delete_block(n);
            Blocks[i]->nblocks--;
            if (uncolor != -1)
                ColorMag(x, y, Pixels[uncolor % 6], 1);
            else
                UncolorMag(x, y);
        }
    } else {
        while(Blocks[i]->block != NULL) {
            n = Blocks[i]->block;
            Blocks[i]->block = n->next;
            Blocks[i]->nblocks--;
            if (n->pdata) free_pdata(n->pdata);
            free(n);
        }
    }
}


Toggle_Append(Button B, XEvent *E)
{
    BlockAppendReplace = 1-B->state;
}
Toggle_Cycle(Button B, XEvent *E)
{
    BlockCycle = B->state;
}
EnableBlock(void)
{
    BlockEnabled = 1;
}
DisableBlock(void)
{
    BlockEnabled = -1;
}
GetBlockState(int i)
{
    if (Blocks != NULL && Blocks[i] != NULL) {
        return(Blocks[i]->state);
    }
    return(0);
}
SetBlockState(int i, int state)
{
    if (Blocks[i] == NULL) {
        Blocks[i] = (struct block *) 
                                calloc(1,(unsigned int) sizeof(struct block));
        Blocks[i]->block = NULL;
        Blocks[i]->nblocks = 0;
    }
    Blocks[i]->state = state;
}
GetCurrentBlock(void)
{
    return(CurrentBlock);
}
SetCurrentBlock(int i)
{
    CurrentBlock = i;
}

GetBlockCount(int i)
{
    if (Blocks != NULL && Blocks[i] != NULL) 
        return(Blocks[i]->nblocks);
    return(0);
}

struct block_node *
GetFirstBlock(int i)
{
    if (Blocks != NULL && Blocks[i] != NULL) {
        return(Blocks[i]->block);
    }
    return(NULL);
}

SetPlotStruct(struct PlotStruct *plotstruct)
{
    ps = plotstruct;    
}

int
GetNBlocks(void)
{
    return(NBlocks);
}

/**
 ** Encode the block of pixels selected as x+w,y+h.
 **/
char *
EncodeBlock(int i)
{
	return(NULL);
}

GetBlockType(int i)
{
	if (Blocks[i] == NULL || Blocks[i]->nblocks == 0)
		return(0);
	return(Blocks[i]->block->type);
}

GetExtendedBlock(void)
{
	return(NBlocks);
}

SetBlockData(int i, int type, PointData *pdata, int color)
{
    struct block_node *n;

    SetBlockState(i,1);
    delete_all(i);

    if (type != -1) {
        Blocks[i]->nblocks = 1;
        n = (struct block_node *)calloc(1, sizeof(struct block_node));
        Blocks[i]->block = n;
		
        n->type = type;
        n->pdata = pdata;
        n->color = color;
        n->next = NULL;
    }
}
 

AddBlockData(int i, int type, PointData *pdata, int color)
{
    struct block_node *n;

    if (Blocks[i] == NULL || Blocks[i]->block == NULL) {
        SetBlockData(i, type, pdata, color);
    } else {
        /**
        ** Find last block
        **/
        n = Blocks[i]->block;
        while(n->next != NULL)  {
            n = n->next;
        }

        /**
        ** Set data
        **/
        n->next = (struct block_node *)calloc(1, sizeof(struct block_node));
        n = n->next;

        n->type = type;
        n->pdata = pdata;
        n->color = color;
        n->next = NULL;

        Blocks[i]->nblocks++;
    }
}

void 
free_pdata(PointData * pdata)
{
	free(pdata->data);
	free(pdata);
}

double 
get_PointData(PointData * pdata, int i)
{
	double d = 0.0;
	switch (pdata->format) {
		case BYTE:
			d = ((unsigned char *)pdata->data)[i]; break;
		case SHORT:
			d = ((short *)pdata->data)[i]; break;
		case INT:
			d = ((int *)pdata->data)[i]; break;
		case FLOAT:
			d = ((float *)pdata->data)[i]; break;
		case DOUBLE:
			d = ((double *)pdata->data)[i]; break;
		case VAX_FLOAT:
		case VAX_INTEGER:
			fprintf(stderr, "VAX format is not supported\n");
	}
	return(d);
}

PointData *
make_PointData(int npoints, int format, void *data)
{
	PointData *p = malloc(sizeof(PointData));

    p->npoints = npoints;
    p->format = format;
    p->data = data;
	return (p);
}

PointData *
copy_PointData(PointData * pdata)
{
	PointData *p = malloc(sizeof(PointData));

    p->npoints = pdata->npoints;
    p->format = pdata->format;
    p->data = malloc(NBYTES(pdata->format)*pdata->npoints);
	memcpy(p->data, pdata->data, NBYTES(pdata->format)*pdata->npoints);
	return (p);
}
