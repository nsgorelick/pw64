#include <X11/Xlib.h>
#include <X11/Xos.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include <stdlib.h> /* For exit() */

int errors(Display *d, XErrorEvent *e);
int AllocError;

int set_errors(void)
{
    XSetErrorHandler(errors);
    return (0);
}

int errors(Display *d, XErrorEvent *e)
{
    char buf[256];

    if (e->error_code == BadAlloc) {
        AllocError = 1;
        printf("memory allocation error\n");
    } else {
        XGetErrorText(d, e->error_code, buf, 256);
        fprintf(stderr, "%s (major=%d minor=%d resource=0x%lx)\n", buf, e->request_code, e->minor_code, e->resourceid);
        exit(1);
    }
    return (0);
}
