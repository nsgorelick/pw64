#include "Xfred.h"

/*
 * What we want here is a text composite widget that keeps track of items
 * that were successfully input into it.  Those items should be retriveable
 * in a popdown list.
 *
 * Action List:
 *      Click or type on edit box, enter edit mode on last element.
 *           (sucess, store current element in popdown list.)
 *      Click on popdown, get list of elements.
 *      Click on popdown element, push list from current to here down by 1
 *			 and move this element to current, activating load callback.
 */


void Cedit_callback(Button B, XEvent *E);
void Cpopdown_callback(Button B, XEvent *E);
void Clist_callback(List L, XEvent *E);


extern int GetText (Button B, XEvent *E, char *s, int n, int copy);
extern void DeactivateList (List list);
extern void AddListCallback (List list, CallBack proc);
extern void ActivateList (List list);
extern void ReCreateList (List list, int nitems, char **items);
extern int ListIsDoubleClick (List list, XEvent *E);

Composite
CreateComposite(Display *display, Window parent, XFontStruct *font, int x, int y, int width, int height, short int hilite, char *instr)
{
	Composite new;
	int font_height = font->ascent;

	new = (Composite)malloc(sizeof(struct _composite));

	new->display = display;
	new->parent = parent;
	new->font = font;
	new->x = x;
	new->y = y;
	new->width = width;
	new->height = height;
	new->hilite = hilite;

	new->items = NULL;
	new->list = NULL;
	new->nitems = 0;
	new->current_text = NULL;
	new->Edit = XfCreateButton(display, parent, x, y, width, height,
	                           1, BLACK(display), "editbox", 1);
	XfAddButtonVisual(new->Edit, 0,
				      XfCreateVisual(new->Edit, 0, height/2-font_height/2, 0, 0,
									 BLACK(display), WHITE(display), 
									 XfTextVisual, instr, font, 1));
	XfAddButtonCallback(new->Edit, 0, Cedit_callback, NULL);
	new->Edit->member = (int *)new;

	new->Popdown = XfCreateButton(display, parent, x+width+2, y, height, height,
                                  1, BLACK(display), "popdown", 1);
	XfAddButtonVisual(new->Popdown, 0,
				      XfCreateVisual(new->Popdown, 0, 0, 0, 0,
									 hilite, hilite, XfSolidVisual));
	XfAddButtonCallback(new->Popdown, 0, Cpopdown_callback, NULL);
	new->Popdown->member = (int *)new;

	new->font_height = font->ascent + font->descent;
	new->ext = 1;
	return(new);
}


void
AddCompositeCallback(Composite C, CallBack callback)
{
	C->load_proc = callback;
}


void
ActivateComposite(Composite C)
{
	XfActivateButton(C->Edit, ExposureMask | ButtonPressMask | KeyPressMask);
	XfActivateButton(C->Popdown, ExposureMask | ButtonPressMask);
}


void
DeactivateComposite(Composite C)
{
	XfDeactivateButton(C->Edit);
	XfDeactivateButton(C->Popdown);
}


void
AddToComposite(Composite C, char *str)
{
	int i;

	C->nitems++;

	if (C->items == NULL) {
		C->items = (char **)malloc(sizeof(char *));
	} else {
		C->items = (char **)realloc(C->items, C->nitems * sizeof(char *));
	}

	for (i = C->nitems-1 ; i > 0 ; i--) {
		C->items[i] = C->items[i-1];
	}
	C->items[0] = strdup(str);
}


void
PushdownComposite(Composite C)
{
	int i;

/* 
	Can't pushdown a text that came from selection (or for that matter,
	the same peice of text twice)
*/

	if (C->current_text == NULL) return;
	for (i = 0 ; i < C->nitems ; i++) {
		if (C->items[i] == C->current_text) return;
	}
	AddToComposite(C,C->current_text);
}

/*
RemoveFromComposite();
ClearComposite()
*/

void
Cedit_callback(Button B, XEvent *E)
{
	char buf[256];
	Composite C = (Composite)B->member;

	if (GetText(B, E, buf, 256, B->ext) == -1) return;

	SetButtonText(B, buf);
	B->ext = (char *)1;
	C->current_text = B->States[0]->Visuals->visual.t_vis.text;
	if (C->list != NULL) DeactivateList(C->list);
	(*(C->load_proc))(C,NULL);
}


void
Cpopdown_callback(Button B, XEvent *E)
{
/* 
	this should just put up a list with the callback of the list
	setting current_text to the appropriate pointer selection.
*/
	Composite C = (Composite)B->member;


	if (C->list == NULL) {
		C->list = CreateList(C->display, C->parent, C->font, 
							 C->x, C->y+C->height, 
							 C->width+C->height+2, (int)(C->font_height*2*1.2), 
							 C->height, 0, C->hilite, C->nitems, C->items);
		AddListCallback(C->list, Clist_callback);
		C->list->member = (int *)B->member;
		ActivateList(C->list);
	} else {
		if (C->list->view->active == 0) {
			ReCreateList(C->list, C->nitems, C->items);
			ActivateList(C->list);
		} else {
			DeactivateList(C->list);
		}
	}
}

static int old_select=-1;

void
Clist_callback(List L, XEvent *E)
{
	Composite C = (Composite)L->member;

	if (C->list->selected == old_select && ListIsDoubleClick(L,E)) {
		C->current_text = C->items[C->list->selected];
		DeactivateList(C->list);
		(*(C->load_proc))(C,NULL);
		old_select=-1;
	}
	old_select = C->list->selected;
}
