/*
 * Button.c
 *
 * Coding of the support routines for the Button class of X interface
 * controls. Each routine is documented prior to declaration. Naturally,
 * the easy routines will be coded first...
 *
 * $Header$
 *
 * $Log$
 * Revision 1.1  1999/09/09 17:50:38  gorelick
 * Initial revision
 *
 * Revision 0.3  91/10/04  01:34:24  01:34:24  rray (Randy Ray)
 * Altered instances of XEvent being passed to routines to be a pointer rather
 * than the entire structure.
 * 
 * Revision 0.2  91/08/17  17:51:24  17:51:24  rray (Randy Ray)
 * *** empty log message ***
 * 
 * Revision 0.1  91/07/24  18:03:44  18:03:44  rray (Randy Ray)
 * *** empty log message ***
 * 
 *
 */

#include "Xfred.h"
#include "Button.h"
#include "freelist.h"

/* Global list of buttons declared for storage space here */
Button ButtonList = NULL;

/*
 * Consider the passed button as having been selected, and execute the
 * appropriate callback list.
 */
int XfButtonPush(Button B, XEvent *E)
{
  struct CallBackList* execs;  /* List of callbacks to execute */
  XEvent Ev;

  if (B == NULL)
    return(False);
  switch (E->type)
    {
    case Expose:
	  while (XCheckTypedWindowEvent(B->display, B->window, Expose, &Ev));
	  if (!B->noAutoExpose) {
		  (*(B->exposeCallback))(B, E);
		  break;
	  }
	  /* else drop through */
    default:
      execs = (B->States[XfButtonState(B)])->CallBacks;
      while (execs != NULL)
        {
          if (execs->proc != NULL)
            (*(execs->proc))(B, E);
          execs = execs->next;
        }
      break;
    }
  return(True);
}

/*
 * Get the pointer to the button named.
 */
Button XfGetButton(char *name)
{
  Button search;
  
  search = ButtonList;
  
  while (search != NULL)
    {
      if (!strcmp(name, search->name))
        return(search);
      search = search->nextButton;
    }
  
  return(NULL);
}

/*
 * Delete the specified button from the list.
 */
int XfDestroyButton(Button B)
{
  Button search;
  int i;
  struct VisualInfo* vis;
  struct VisualInfo* mm;
  struct VisualInfo* nn;
  struct CallBackList* call;
  
  if ((ButtonList == NULL) || (B == NULL))
    return(False);
  
  XfDeactivateButton(B);
  if (B == ButtonList)
    {
      for (i = 0; i < B->maxstate; i++)
        {
          vis = (B->States[i])->Visuals;
          call = (B->States[i])->CallBacks;
          
          nn = vis;
          while (nn != NULL)
            {
              mm = nn->next;
              if (nn->vtype == XfXImageVisual)
                XFree((char *)nn->visual.i_vis);
              if ((nn->vtype == XfPixmapVisual) ||
                  (nn->vtype == XfTiledVisual) ||
                  (nn->vtype == XfStippledVisual) ||
                  (nn->vtype == XfOpaqueStippledVisual))
                XFreePixmap(B->display, nn->visual.p_vis.map);
              free(nn);
              nn = mm;
            }
          FreeList(struct CallBackList *, call, next);
          free(B->States[i]);
        }
      ButtonList = ButtonList->nextButton;
      free(B);
    }
  else
    {
      search = ButtonList;
      while (search->nextButton != B)
        search = search->nextButton;
      /* search now points to B's "parent" */
      for (i = 0; i < B->maxstate; i++)
        {
          vis = (B->States[i])->Visuals;
          call = (B->States[i])->CallBacks;
          
          nn = vis;
          while (nn != NULL)
            {
              mm = nn->next;
              if (nn->vtype == XfXImageVisual)
                XFree((char *)nn->visual.i_vis);
              if ((nn->vtype == XfPixmapVisual) ||
                  (nn->vtype == XfTiledVisual) ||
                  (nn->vtype == XfStippledVisual) ||
                  (nn->vtype == XfOpaqueStippledVisual))
                XFreePixmap(B->display, nn->visual.p_vis.map);
              free(nn);
              nn = mm;
            }
          FreeList(struct CallBackList *, call, next);
          free(B->States[i]);
        }
      search->nextButton = B->nextButton;
      free(B);
    }
  return(True);
}

/* 
 * Add a callback to the list for the state & button specified. Return
 * an error for any of the many things that could go wrong.
 */
int XfAddButtonCallback(Button B, int st, CallBack new, CallBack old)
                /* The button */
                /* The state */
                  
                  
{
  struct CallBackList* new_ptr;
  struct CallBackList* loop;
  struct CallBackList* loopn;
  
  if ((B == NULL) || (st >= B->maxstate))
    return(False);
  
  new_ptr = (struct CallBackList *)malloc(sizeof(struct CallBackList));
  if (new_ptr == NULL)
    return(False);
  new_ptr->proc = new;
  loop = (B->States[st])->CallBacks;
  
  if (loop == NULL || loop->proc == old)
    {
      /* Must insert at head of list */
      new_ptr->next = loop;
      (B->States[st])->CallBacks = new_ptr;
    }
  else
    {
      loopn = loop->next;
      while (loopn != NULL)
        {
          if (loopn->proc == old)
            break;
          else
            loop = loopn, loopn = loop->next;
        }
      if (loopn == NULL)
        {
          /* Not found. Unless old is NULL (add at end), error */
          if (old == NULL)
            {
              new_ptr->next = NULL;
              loop->next = new_ptr;
            }
          else
            return(False);
        }
      else
        {
          /* Found it. Insert new callback before */
          new_ptr->next = loop->next;
          loop->next = new_ptr;
        }
    }
  return(True);
}

/*
 * Delete the named callback from the list in st. Error if st is invalid
 * or callback not a member of that list.
 */
int XfDelButtonCallback(Button B, int st, CallBack cb)
{
  struct CallBackList* loop;
  struct CallBackList* loopn;
  
  if ((B == NULL) || (st >= B->maxstate))
    return(False);
  
  loop = (B->States[st])->CallBacks;
  if (loop == NULL)
    return(False);
  if (loop->proc == cb)
    {
      (B->States[st])->CallBacks = (B->States[st])->CallBacks->next;
      free(loop);
      return(True);
    }
  loopn = loop->next;
  while (loopn != NULL)
    {
      if (loopn->proc == cb)
        {
          loop = loopn->next;
          free(loopn);
          return(True);
        }
      loop = loopn;
      loopn = loop->next;
    }
  return(False);        /* Callback was not found */
}

/*
 * Add the passed Visual structure to the END of the list maintained
 * for state st of button B. Error if button or state is invalid.
 */
int XfAddButtonVisual(Button B, int st, struct VisualInfo *vis)
{
  struct VisualInfo* loop;
  
  if ((B == NULL) || (st >= B->maxstate))
    return(False);
  
  vis->next = NULL;
  loop = (B->States[st])->Visuals;
  if (loop == NULL)
    (B->States[st])->Visuals = vis;
  else
    {
      while (loop->next != NULL)
        loop = loop->next;
      loop->next = vis;
    }
  return(True);
}

/*
 * Create a new button with the passed information. This new button will
 * contain no visual information and no callback lists. The mandatory
 * callbacks for expose and update will be set to defaults. Returns NULL
 * if any allocation fails, otherwise returns the address of the new
 * Button.
 */
Button XfCreateButton(Display *display, Window parent, int x, int y, int width, int height, int border_width, long unsigned int border_color, char *name, int num_states)
{
  Button new;
  Button search;
  int i;

  if (num_states == 0)
    return(NULL);
  /*
   * Daddy?
   *
   * Yes, son?
   *
   * What does regret mean?
   *
   * Well, son, regret is a funny thing. Sometimes we regret things we 
   * haven't done more than the things we have done. Oh, and when you
   * see your mother, tell her SATAN SATAN SATAN!!!
   *
   */
  new = (Button)calloc(1,sizeof(struct _Button));
  if (new == NULL)
    return(NULL);
  
  new->parent = parent;
  new->display = display;
  new->x = x;
  new->y = y;
  new->height = height;
  new->width = width;
  new->border_width = border_width;
  new->border_color = border_color;
  strcpy(new->name, name);
  new->maxstate = num_states;
  new->state = 0;
  new->States = (struct ButtonState **)calloc(num_states,
                                              sizeof(struct ButtonState *));
  if (new->States == NULL)
    {
      free(new);
      return(NULL);
    }
  else
    for (i = 0; i < new->maxstate; i++)
      {
        new->States[i] = 
          (struct ButtonState *)calloc(1,sizeof(struct ButtonState));
        if (new->States[i] == NULL)
          {
            free(new);
            return(NULL);
          }
        else
          {
            (new->States[i])->CallBacks = (struct CallBackList *)NULL;
            (new->States[i])->Visuals = (struct VisualInfo *)NULL;
          }
      }
  new->window = XCreateSimpleWindow(display, parent, x, y, width, height,
                                    border_width, border_color, 0);
  XStoreName(display, new->window, name);
  new->exposeCallback = defaultButtonCallback;
  new->updateCallback = defaultButtonCallback;
  new->active = False;
  new->nextButton = NULL;
  new->noAutoExpose = 0;
  search = ButtonList;
  if (search == NULL)
    ButtonList = new;
  else 
    {
      while (search->nextButton != NULL)
        search = search->nextButton;
      search->nextButton = new;
    }
  return(new);
}

/**
 ** Identical to XfCreateButton, but with a background color
 **/

Button XFCreateButton(Display *display, Window parent, int x, int y, int width, int height, int border_width, long unsigned int border_color, long unsigned int bg, char *name, int num_states)
{
  Button new;
  Button search;
  int i;

  if (num_states == 0)
    return(NULL);
  /*
   * Daddy?
   *
   * Yes, son?
   *
   * What does regret mean?
   *
   * Well, son, regret is a funny thing. Sometimes we regret things we 
   * haven't done more than the things we have done. Oh, and when you
   * see your mother, tell her SATAN SATAN SATAN!!!
   *
   */
  new = (Button)calloc(1,sizeof(struct _Button));
  if (new == NULL)
    return(NULL);
  
  new->parent = parent;
  new->display = display;
  new->x = x;
  new->y = y;
  new->height = height;
  new->width = width;
  new->border_width = border_width;
  new->border_color = border_color;
  strcpy(new->name, name);
  new->maxstate = num_states;
  new->state = 0;
  new->States = (struct ButtonState **)calloc(num_states,
                                              sizeof(struct ButtonState *));
  if (new->States == NULL)
    {
      free(new);
      return(NULL);
    }
  else
    for (i = 0; i < new->maxstate; i++)
      {
        new->States[i] = 
          (struct ButtonState *)calloc(1,sizeof(struct ButtonState));
        if (new->States[i] == NULL)
          {
            free(new);
            return(NULL);
          }
        else
          {
            (new->States[i])->CallBacks = (struct CallBackList *)NULL;
            (new->States[i])->Visuals = (struct VisualInfo *)NULL;
          }
      }
  new->window = XCreateSimpleWindow(display, parent, x, y, width, height,
                                    border_width, border_color, bg);
  XStoreName(display, new->window, name);
  new->exposeCallback = defaultButtonCallback;
  new->updateCallback = defaultButtonCallback;
  new->active = False;
  new->nextButton = NULL;
  new->noAutoExpose = 0;
  search = ButtonList;
  if (search == NULL)
    ButtonList = new;
  else 
    {
      while (search->nextButton != NULL)
        search = search->nextButton;
      search->nextButton = new;
    }
  return(new);
}

/*
 * Move the specified button to the front of the list, to improve search
 * time. No reason for this to fail.
 */
int XfMoveButton(Button B)
{
  Button search;
  
  if ((B == NULL) || (B == ButtonList))
    return(True);
  search = ButtonList;
  while (search->nextButton != B)
    {
      if (search->nextButton == NULL)
        return(False);
      search = search->nextButton;
    }
  /* Now that search->nextButton == B, start shuffling */
  search->nextButton = B->nextButton;
  B->nextButton = ButtonList;
  ButtonList = B;
  return(True);
}

/* 
 * Map the button's window, set it's event mask, and mark it active.
 */
int XfActivateButtonState(Button B, int st, long unsigned int mask)
{
  if (B == NULL)
    return(False);
  B->active = True;
  if ((st < 0) || (st >= B->maxstate))
    B->state = 0;
  else
    B->state = st;
  XSelectInput(B->display, B->window, mask);
  XMapRaised(B->display, B->window);
  XFlush(B->display);
  return(True);
}

/*
 * Un-map and select input to zero.
 */
int XfDeactivateButton(Button B)
{
  if (B == NULL)
    return(False);
  B->active = False;
  XSelectInput(B->display, B->window, 0);
  XUnmapWindow(B->display, B->window);
  return(True);
}

/*
 * The default callback for expose and update is to cycle through all visuals,
 * drawing them in the button space.
 */
void defaultButtonCallback(Button B, XEvent *E)
{
  struct VisualInfo* vis;
  GC localGC;
  int dir, asc, des;
  XCharStruct xcs;

  if (B == NULL || B->display == NULL) {
    return;
  }

  localGC = XCreateGC(B->display, B->window, 0, NULL);
  if (localGC == NULL) {
    return;
  }

  vis = B->States[B->state]->Visuals;
  while (vis != NULL) {
    switch (vis->vtype) {
      case XfXImageVisual:
        if (vis->visual.i_vis != NULL) {
          XPutImage(B->display, B->window, localGC, vis->visual.i_vis, 0, 0,
                    vis->x_pos, vis->y_pos, vis->width, vis->height);
        }
        break;
      case XfPixmapVisual:
        XSetForeground(B->display, localGC, vis->foreground);
        XSetBackground(B->display, localGC, vis->background);
        if (vis->visual.p_vis.depth == 1)
          XCopyPlane(B->display, vis->visual.p_vis.map, B->window, localGC,
                     0, 0, vis->width, vis->height, vis->x_pos,vis->y_pos,1);
        else
          XCopyArea(B->display, vis->visual.p_vis.map, B->window, localGC,
                    0, 0, vis->width, vis->height, vis->x_pos, vis->y_pos);
        break;
      case XfTextVisual:
        if (vis->visual.t_vis.font != NULL) {
          XSetFont(B->display, localGC, vis->visual.t_vis.font->fid);
          XSetForeground(B->display, localGC, vis->foreground);
          XTextExtents(vis->visual.t_vis.font, vis->visual.t_vis.text,
                        strlen(vis->visual.t_vis.text), 
                        &dir, &asc, &des, &xcs);
          switch (vis->visual.t_vis.align) {
            case 0: /* Center */
              dir = xcs.width / 2;
              if (vis->width == 0)
                asc = vis->x_pos + B->width/2;
              else
                asc = vis->x_pos + vis->width/2;
              asc -= dir;
              des = vis->y_pos + xcs.ascent;
              break;
            case 1: /* Left justify */
              asc = vis->x_pos;
              des = vis->y_pos + xcs.ascent;
              break;
            case 2: /* Right justify */
              asc = vis->width;
              if (asc == 0)
                asc = B->width;
              asc -= xcs.width;
              des = vis->y_pos + xcs.ascent;
              break;
          }
          XDrawString(B->display, B->window, localGC, asc, des, 
                      vis->visual.t_vis.text, strlen(vis->visual.t_vis.text));
        }
        break;
      case XfOutlineVisual:
      case XfSolidVisual:
        XSetForeground(B->display, localGC, vis->foreground);
        XDrawRectangle(B->display, B->window, localGC, vis->x_pos, vis->y_pos,
                      vis->width, vis->height);
        break;
      case XfStippledVisual:
        XSetForeground(B->display, localGC, vis->foreground);
        XSetBackground(B->display, localGC, vis->background);
        XSetStipple(B->display, localGC, vis->visual.p_vis.map);
        XSetFillStyle(B->display, localGC, FillStippled);
        XFillRectangle(B->display, B->window, localGC, vis->x_pos, vis->y_pos,
                      vis->width, vis->height);
        XSetFillStyle(B->display, localGC, FillSolid);
        break;
      case XfOpaqueStippledVisual:
        XSetForeground(B->display, localGC, vis->foreground);
        XSetBackground(B->display, localGC, vis->background);
        XSetStipple(B->display, localGC, vis->visual.p_vis.map);
        XSetFillStyle(B->display, localGC, FillOpaqueStippled);
        XFillRectangle(B->display, B->window, localGC, vis->x_pos, vis->y_pos,
                      vis->width, vis->height);
        XSetFillStyle(B->display, localGC, FillSolid);
        break;
      case XfTiledVisual:
        XSetForeground(B->display, localGC, vis->foreground);
        XSetBackground(B->display, localGC, vis->background);
        XSetTile(B->display, localGC, vis->visual.p_vis.map);
        XSetFillStyle(B->display, localGC, FillTiled);
        XFillRectangle(B->display, B->window, localGC, vis->x_pos, vis->y_pos,
                      vis->width, vis->height);
        XSetFillStyle(B->display, localGC, FillSolid);
        break;
      case XfHersheyVisual:
        XSetForeground(B->display, localGC, vis->foreground);
        XSetBackground(B->display, localGC, vis->background);
        XfHersheyString(B->display, B->window, localGC,
                        vis->x_pos, vis->y_pos,
                        vis->visual.h_vis.scale, vis->visual.h_vis.scale,
                        vis->visual.h_vis.angle, vis->visual.h_vis.text,
                        vis->visual.h_vis.cset, vis->visual.h_vis.align);
        break;
    }
    vis = vis->next;
  }
  XFreeGC(B->display, localGC);
}

/*
 * EventButton returns which button, if any, the passed event occured in.
 */
Button XfEventButton(XEvent *E)
{
  Button search;
  
  search = ButtonList;
  while (search != NULL)
    {
      switch (E->type)
        {
        case NoExpose:
        case GraphicsExpose:
          break;
        default:
          if (search->window == E->xany.window)
            return(search);
          break;
        }
      search = search->nextButton;
    }
  return(NULL);
}

void
XfNoAutoExposeButton(Button B)
{
	B->noAutoExpose = 1;
}

void
ResizeButton(Button B, int x, int y, int w, int h)
{
	struct VisualInfo *v;
	int i;

	if (x != -1 || y != -1) {
		XMoveWindow(B->display, B->window, x, y);
		B->x = x;
		B->y = y;
	}
	if (w != -1 || h != -1) {
		XResizeWindow(B->display, B->window, w, h);
		B->width = w;
		B->height = h;
		for (i = 0; i < B->maxstate ; i++) {
			for (v = B->States[i]->Visuals ; v != NULL ; v=v->next) {
				v->width = w;
				v->height = h;
			}
		}
	} 
}

void
SetButtonText(Button B, char *text)
{
	struct VisualInfo *Vis;

	int	i;
	for (i = 0 ; i < B->maxstate ; i++) {
		for (Vis = B->States[i]->Visuals ; Vis != NULL ; Vis = Vis->next) {
			if (Vis->vtype == XfTextVisual) {
				if (!strcmp(Vis->visual.t_vis.text, text))
					return;
				strncpy(Vis->visual.t_vis.text, text, 256);
			}
		}
	}
	UpdateButton(B);
}

Button 
Make2State(Display *display, Window window, XFontStruct *font, int x, int y, int w, int h, int bw, unsigned int bc, unsigned int fg, unsigned int bg, char *name)
{
	Button B;
	int a, d, direction;
	XCharStruct overall;

	XTextExtents(font, name, strlen(name), &direction, &a, &d, &overall);

    B = XfCreateButton(display, window, x, y, w, h, bw, bc, name, 2);
	XfAddButtonVisual(B, 0, XfCreateVisual(B, 0, (h-a)/2, 0, 0,
		fg, bg, XfTextVisual, name, font, 0));
	XfAddButtonVisual(B, 1, XfCreateVisual(B, 0, (h-a)/2, 0, 0,
		bg, fg, XfTextVisual, name, font, 0));
	XfAddButtonCallback(B, 0, toggle_state, NULL);
	XfAddButtonCallback(B, 1, toggle_state, NULL);
	XfActivateButton(B, ExposureMask | ButtonPressMask);
	return(B);
}

Button 
Make2State3D(Display *display, Window window, XFontStruct *font, int x, int y, int w, int h, int bw, unsigned int bc, unsigned int fg, unsigned int bg, char *name)
{
	Button B;
	int a, d, direction;
	XCharStruct overall;

	XTextExtents(font, name, strlen(name), &direction, &a, &d, &overall);

    B = XfCreateButton(display, window, x, y, w, h, bw, bc, name, 2);
	XfAddButtonVisual(B, 0, XfCreateVisual(B, 0, (h-a)/2, 0, 0,
		fg, bg, XfTextVisual, name, font, 0));
	XfAddButtonVisual(B, 0, XfCreateVisual(B, -1, -1, w, h,
		bc, bg, XfOutlineVisual));
	XfAddButtonVisual(B, 1, XfCreateVisual(B, 0, (h-a)/2, 0, 0,
		bg, fg, XfTextVisual, name, font, 0));
	XfAddButtonCallback(B, 0, toggle_state, NULL);
	XfAddButtonCallback(B, 1, toggle_state, NULL);
	XfActivateButton(B, ExposureMask | ButtonPressMask);
	return(B);
}


char *
GetButtonText(Button B)
{
	struct VisualInfo *Vis;

	for (Vis = B->States[B->state]->Visuals ; Vis != NULL ; Vis = Vis->next) {
		if (Vis->vtype == XfTextVisual) {
			return(Vis->visual.t_vis.text);
		}
	}
	return(NULL);
}

void
SetButtonState(Button B, int state)
{
	if (state < B->maxstate) {
		B->state = state;
		UpdateButton(B);
	}
}

void
UpdateButton(Button B)
{
	(*(B->updateCallback))(B,NULL);
}
