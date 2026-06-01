/*
 * freelist.h
 *
 * A define that allows recursive-like freeing of memory in a linked
 * list.
 *
 * $Header$
 * 
 * $Log$
 * Revision 1.1  1999/09/09 17:50:39  gorelick
 * Initial revision
 *
 * Revision 0.1  91/07/24  18:04:17  18:04:17  rray (Randy Ray)
 * *** empty log message ***
 * 
 */

/*
 * This incredibly unelegant macro allows for freeing up memory that
 * has been slaved to a linked list.
 */

#define FreeList(type, head, next) \
{ \
  type n; \
  type m; \
  n = head; \
  while (n != NULL) \
    { \
      m = n->next; \
      free(n); \
      n = m; \
    } \
}



