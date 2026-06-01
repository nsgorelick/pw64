#ifndef CALLBACK_H
#define CALLBACK_H
/*
 * Callback.h
 *
 * Typedef and structure def for callback routines and lists of same.
 *
 * $Header$
 *
 * $Log$
 * Revision 1.1  1999/09/09 17:50:38  gorelick
 * Initial revision
 *
 * Revision 0.2  91/09/23  17:55:03  17:55:03  ngorelic (Noel S. Gorelick)
 * *** empty log message ***
 * 
 * Revision 0.1  91/07/24  18:04:07  18:04:07  rray (Randy Ray)
 * *** empty log message ***
 * 
 *
 */
#include "Xfred.h"

/* A function ptr to allow dynamic lists of functions called by tools */
typedef void (*CallBack)();

/* Maintain a linked-list of callback routines */
struct CallBackList
{
  CallBack proc;                /* address of callback */
  struct CallBackList* next;    /* next callback, if any */
};

#endif   // CALLBACK_H
