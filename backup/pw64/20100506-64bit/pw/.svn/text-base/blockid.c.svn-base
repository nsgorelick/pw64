/* ----------  blockid ---------------- */
#include <fcntl.h>
#include "Xfred.h"
#include <X11/keysym.h>
#include "bitmaps/bitmaps.h"
#include "mag.h"
#include <math.h>
#include <values.h>
#include "block.h"

extern struct block *Blocks[7];
extern XColor pwRed, pwBlue, pwGreen, pwYellow, pwCyan, pwMagenta, pwHilite, pwBackground;
extern int Pixels[6];
extern int CurrentBlock;
extern struct PlotStruct *ps;

Button ColorBox;
Button NPixels;
Button BlockID = NULL;
List PixelList;

void BlockIDWrite(Button B, XEvent *E);
void BlockIDDelete(Button B, XEvent *E);
void BlockIDUnDel(Button B, XEvent *E);
void BlockIDBlock(Button B, XEvent *E);
void BlockIDDone(Button B, XEvent *E);

void GetBlockIDWriteFilename(Button B, XEvent *E);
void BlockIDWriteDoRead(Button B, XEvent *E);
void BlockIDWriteDoWrite(Button B, XEvent *E);
void BlockIDWriteDoCancel(Button B, XEvent *E);

int do_hilite(List L, XEvent *E);

char    BlockIDFilename[256];


int CreateTextButton (Display *d, XFontStruct *f, Button *b, Window w, int x, int y, int width, int height, int border, char *text, int align, void *proc);
int CreateBlockIDWrite (Display *display, XFontStruct *font);
int UpdateBlockID (int color);
int DeactivateBlockIDWrite (void);
int ActivateBlockIDWrite (void);
int DecodeBlock (char *s, int *xi, int *yi, int *wi, int *hi);
extern int DeletePoint (int x, int y);
int MarkDeleted (void);
extern int delete_all (int i);
extern int GetExtendedBlock (void);
extern int AddPoint (int x, int y);
int UnMarkDeleted (void);
int ReadPixels (char *f);
int WritePixels (char *f, Button B);
extern int GetCurrentBlock (void);
extern int SetBlockData (int i, int type, PointData *pdata, int color);
extern int SetExtendedWaves (int i);

CreateBlockID(Display *display, XFontStruct *font)
{
    Button B;
    List list;
    Window parent;
    int x, y, width, height;

    parent = RootWindow(display, DefaultScreen(display));

    B = XfCreateButton(display, parent, 300, 300, 185, 185,
        1, BLACK(display), "BlockIDList", 1);
    XfAddButtonVisual(B, 0, XfCreateVisual(B, 0, 0, 0, 0,
        WHITE(display), WHITE(display),
        XfSolidVisual));
    BlockID = B;

    parent = B->window;

    x = 5;
    y = 5;
    width = 20;
    height = 20;

    B = XfCreateButton(display, parent, x, y, width, height,
        1, BLACK(display), "ColorBox", 1);
    XfAddButtonVisual(B, 0, XfCreateVisual(B, 0, 0, width - 1, height - 1,
        BLACK(display),  pwBlue.pixel,
        XfOutlineVisual));
    XfActivateButton(B, ExposureMask);

    ColorBox = B;
    x += width + 1;
    width = 90;

    B = XfCreateButton(display, parent, x, y, width, height,
        1, BLACK(display), "NPixels", 1);
    XfAddButtonVisual(B, 0, XfCreateVisual(B, 0, 7, 0, 0,
        BLACK(display), WHITE(display),
        XfTextVisual, "0 pixels", font, 0));
    XfActivateButton(B, ExposureMask);
    NPixels = B;

    x -= 20 + 1;
    y += height + 10;
    width = 90 + 20 + 1;

    list = CreateList(display, parent, font, x, y, 
        width, 140, 10, 0, pwHilite.pixel, 0, 0);
	AddListCallback(list, do_hilite);
    ActivateList(list);
    PixelList = list;

    y -= height + 10;
    x += width + 10;

    width = 50;
    height = 20;

    CreateTextButton(display, font, &B, parent, x, y, width, height, 1, 
                    "WRITE", 0, BlockIDWrite);
    y += height + 10;
    CreateTextButton(display, font, &B, parent, x, y, width, height, 1, 
                    "DELETE", 0, BlockIDDelete);
    y += height + 10;
    CreateTextButton(display, font, &B, parent, x, y, width, height, 1, 
                    "UNDELETE", 0, BlockIDUnDel);
    y += height + 10;
    CreateTextButton(display, font, &B, parent, x, y, width, height, 1, 
                    "DONE", 0, BlockIDDone);
    CreateBlockIDWrite(display, font);
}




ActivateBlockID(int i)
{
    UpdateBlockID(i);
    XfActivateButton(BlockID, ExposureMask);
}


DeactivateBlockID(void)
{
    XfDeactivateButton(BlockID);
    DeactivateBlockIDWrite();
}


void
BlockIDWrite(Button B, XEvent *E)
{
    ActivateBlockIDWrite();
}


void
BlockIDDelete(Button B, XEvent *E)
{
    int x, y, w, h;
    int i, j;
    int tmp;
    if (PixelList->selected == -1) {
        XBell(B->display, 50);
        return;
    }
    if (DecodeBlock(PixelList->items[PixelList->selected], &x, &y, &w, &h) <= 0) {
        XBell(B->display, 50);
        return;
    }
    tmp = CurrentBlock;
    CurrentBlock = (int)BlockID->ext;
    for (j = y ; j < y + h ; j++) {
        for (i = x ; i < x + w ; i++) {
            DeletePoint(i,j);
        }
    }
    CurrentBlock = tmp;
    MarkDeleted();

	/**
	 ** This is called out of file, but heck, who really cares.
	 **/
	delete_all(GetExtendedBlock());

    (*(ps->drawall))();

}


void
BlockIDUnDel(Button B, XEvent *E)
{
    int x, y, w, h;
    int i, j;
    int tmp;
    if (PixelList->selected == -1) {
        XBell(B->display, 50);
        return;
    }
    if (DecodeBlock(PixelList->items[PixelList->selected], &x, &y, &w, &h) >= 0) {
        XBell(B->display, 50);
        return;
    }
    tmp = CurrentBlock;
    CurrentBlock = (int)BlockID->ext;
    for (j = y ; j < y + h ; j++) {
        for (i = x ; i < x + w ; i++) {
            AddPoint(i,j);
        }
    }
    CurrentBlock = tmp;
    (*(ps->drawall))();
    UnMarkDeleted();

	do_hilite(PixelList,NULL);
}


void
BlockIDBlock(Button B, XEvent *E)
{
}


void BlockIDDone(Button B, XEvent *E)
{
    DeactivateBlockID();
}


Button  BlockIDWriteCase;

ActivateBlockIDWrite(void)
{
    XfActivateButton(BlockIDWriteCase, ExposureMask);
}


DeactivateBlockIDWrite(void)
{
    XfDeactivateButton(BlockIDWriteCase);
}


CreateBlockIDWrite(Display *display, XFontStruct *font)
{
    Window parent;
    Button B;
    int x, y, width, height;

    parent = RootWindow(display, DefaultScreen(display));

    B = XfCreateButton(display, parent, 320, 320, 300, 65,
        2, BLACK(display), "BlockIDWriteCase", 1);
    XfAddButtonVisual(B, 0, XfCreateVisual(B, 0, 0, 0, 0,
        WHITE(display), WHITE(display),
        XfSolidVisual));
    BlockIDWriteCase = B;

    parent = B->window;

    x = 5;
    y = 5;
    width = 55;
    height = 20;


    CreateTextButton(display, font, &B, parent, x, y, width, height, 
        0, "Filename", 0, NULL);

    B = XfCreateButton(display, parent, x + width, y, 235, height,
        1, BLACK(display), "FilenameRead", 1);
    XfAddButtonVisual(B, 0, XfCreateVisual(B, 2, 7, 0, 0,
        BLACK(display), pwBackground.pixel,
        XfTextVisual, "Click Here to Enter Filename", font, 0 ));
    XfAddButtonCallback(B, 0, GetBlockIDWriteFilename, NULL);
    XfActivateButton(B, ExposureMask | ButtonPressMask | KeyPressMask);

    x += width;
    y += height + 10;

    CreateTextButton(display, font, &B, parent, x, y, width, height, 
        2, "READ", 0, BlockIDWriteDoRead);
    x += width + 10;
    CreateTextButton(display, font, &B, parent, x, y, width, height, 
        2, "WRITE", 0, BlockIDWriteDoWrite);

    x = 300 - width - 5 - 2;
    CreateTextButton(display, font, &B, parent, x, y, width, height, 
        2, "CANCEL", 0, BlockIDWriteDoCancel);

}


void
GetBlockIDWriteFilename(Button B, XEvent *E)
{
    char    buf[256];
    int copy;

    copy = (B->States[0]->Visuals->background == pwBackground.pixel ? 0 : 1);

    if (GetText(B, E, buf, 256, copy) == -1) 
        return;

    B->States[0]->Visuals->background = WHITE(B->display);
    B->States[0]->Visuals->visual.t_vis.align = 1;
    SetButtonText(B, buf);

    strcpy(BlockIDFilename, buf);
}

void
BlockIDWriteDoRead(Button B, XEvent *E)
{
    if (!strlen(BlockIDFilename)) {
        XBell(B->display, 50);
        return;
    }
    ReadPixels(BlockIDFilename);
}


void
BlockIDWriteDoWrite(Button B, XEvent *E)
{
    if (!strlen(BlockIDFilename)) {
        XBell(B->display, 50);
        return;
    }
    WritePixels(BlockIDFilename, B);
    DeactivateBlockIDWrite() ;
}


void
BlockIDWriteDoCancel(Button B, XEvent *E)
{
    DeactivateBlockIDWrite();
}


DecodeBlock(char *s, int *xi, int *yi, int *wi, int *hi)
{
    int x, y, w, h;
    if (sscanf(s, "%d%d,%d%d", &y, &h, &x, &w) == 4) {
        w += 1;
        h += 1;
    } else if (sscanf(s, "%d,%d%d", &y, &x, &w) == 3) {
        h += 1;
    } else if (sscanf(s, "%d%d,%d", &y, &h, &x) == 3) {
        w += 1;
    } else if (sscanf(s, "%d,%d", &y, &x) == 2) {
        w = 1;
        h = 1;
    } else {
		return(0);
	}

    *xi = x;
    *yi = y;
    *wi = w;
    *hi = h;

    if (y < 0) {
        *yi = -y;

        *xi -= 1;
        *yi -= 1;
        return (-1);
    } else {
        *xi -= 1;
        *yi -= 1;
        return (1);
    }
}



ReadPixels(char *f)
{

}


WritePixels(char *f, Button B)
{
    int i;
    char buf[256];
    int err;
    FILE *fp;

    i = open(f, O_EXCL | O_CREAT, 0744);
    if (i > 0) {
        close(i);
    } else {
        sprintf(buf,"%s exists",f);
        err = ConfirmRequestor(B->display, B->parent, 
                         DefaultGC(B->display, DefaultScreen(B->display)), 
                         B->States[0]->Visuals->visual.t_vis.font,
                         0, 0, 300, 65, 
                         1, 0, 0,
                         1, buf,
                         3, "OVERWRITE", "APPEND", "CANCEL");

        if (err == 1) {
            unlink(f);
        } else if (err == 3) {
            return;
        }
    }
    fp = fopen(f, "a");
    for (i = 0 ; i < PixelList->nitems ; i++) {
        if (PixelList->items[i][0] != '-') {
            fprintf(fp, "%s\n",PixelList->items[i]+1);
        }
    }
    fclose(fp);
}


MarkDeleted(void)
{
    PixelList->items[PixelList->selected][0] = '-';
    RefreshList(PixelList);
}


UnMarkDeleted(void)
{
    PixelList->items[PixelList->selected][0] = ' ';
    RefreshList(PixelList);
}


UpdateBlockID(int color)
{
    int i;
    char **items;
    struct block_node *n;
    static char buf[32];

    for (i = 0 ; i < PixelList->nitems ; i++) {
        free(PixelList->items[i]);
    }
    if (color < 0 || Blocks[color] == NULL) {
        ReCreateList(PixelList,0,NULL);
        sprintf(buf,"0 pixels");
    } else {
        items = (char *(*))malloc(sizeof(char *) * Blocks[color]->nblocks);
        n = Blocks[color]->block;
        for (i = 0 ; i < Blocks[color]->nblocks ; i++) {
			if (n->type == BK_EXTRACTED) {
				sprintf(buf," %d,%d", n->stack->y+1, n->stack->x+1);
			} else if (n->type == BK_AVG) {
				sprintf(buf," block avg");
			} else if (n->type == BK_LIBRARY) {
				sprintf(buf," library");
			}
            items[i] = strdup(buf);
            n = n->next;
        }
        ReCreateList(PixelList,Blocks[color]->nblocks,items);
        sprintf(buf,"%d pixels",Blocks[color]->nblocks);
    }

    SetButtonText(NPixels, buf);
    ColorBox->States[0]->Visuals->background = Pixels[color];
    UpdateButton(ColorBox);
    BlockID->ext = (char *)color;
}



CreateTextButton(Display *d, XFontStruct *f, Button *b, Window w, int x, int y, int width, int height, int border, char *text, int align, void *proc)
{
    Button B;
    Display * display = d;
    XFontStruct *font = f;

    B = XfCreateButton(display, w, x, y, width, height,
        border, BLACK(display), "Auto", 1);
    XfAddButtonVisual(B, 0, XfCreateVisual(B, 0, 7, 0, 0,
        BLACK(display), WHITE(display),
        XfTextVisual, text, font, align));
    if (proc != NULL)
        XfAddButtonCallback(B, 0, proc, NULL);
    XfActivateButton(B, ExposureMask | ButtonPressMask | KeyPressMask);
    *b = B;
}


do_hilite(List L, XEvent *E)
{
    int i,j;
    struct block_node *n;
    int x,y,w,h;
    int ext = GetExtendedBlock();
    short *sptr;
	PointData *pdata;

    i = GetCurrentBlock();	

    if (i != -1) {
        if (DecodeBlock(L->items[L->selected], &x, &y, &w, &h) == 0) {
            return;
        }
        for (n = GetFirstBlock(i); n != NULL ; n = n->next) {
            if (n->stack->x == x && n->stack->y == y) {
                delete_all(ext);
                pdata = copy_PointData(n->pdata);

/*              SetBlockData(ext, BK_EXTRACTED, pdata, BLACK(L->display)); ***ORIGINAL LINE****/
                SetBlockData(ext, BK_LIST, pdata, BLACK(L->display)); /*Modified 9/9/99 (haha!) */
                SetExtendedWaves(i);
                (*(ps->drawall))();
                return;
            }
        }
    }
}
