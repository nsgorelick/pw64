#ifndef CALLBACK_H
#define CALLBACK_H
/*
 * Callback.h
 *
 * Typedef and structure def for callback routines and lists of same.
 */

#include <X11/Xlib.h>

/* All Xfred widget callbacks use (widget, event); widget is cast at call sites. */
typedef void (*CallBack)(void *widget, XEvent * event);

/* Cast a widget-specific handler to the generic callback type (Clang -Werror safe). */
#define XF_CALLBACK(fn) ((CallBack)(fn))

struct CallBackList {
    CallBack proc;
    struct CallBackList *next;
};

#endif                          /* CALLBACK_H */
