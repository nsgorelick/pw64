/*
 * freelist.h
 *
 * A define that allows recursive-like freeing of memory in a linked
 * list.
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
