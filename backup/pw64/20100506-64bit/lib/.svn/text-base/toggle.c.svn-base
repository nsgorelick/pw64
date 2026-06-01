#include "Xfred.h"

void
toggle_state(Button B, XEvent *E)
{
	B->state = ((B->state+1)%B->maxstate);
	(*(B->updateCallback))(B,E);
}

void
set_state(Button B, int i)
{
	B->state = i;
	(*(B->updateCallback))(B,NULL);
}
