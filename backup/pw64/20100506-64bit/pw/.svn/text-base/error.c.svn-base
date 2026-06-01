#include <stdio.h>
#include <X11/Xos.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

int errors(Display *d, XErrorEvent *e);
int AllocError;

set_errors(void)
{
    XSetErrorHandler(errors);
}

errors(Display *d, XErrorEvent *e)
{
    char buf[256];

    if (e->error_code == BadAlloc) {
        AllocError = 1;
        printf("memory allocation error\n");
    } else {
        XGetErrorText(d, e->error_code, buf, 256);
        printf("%s\n",buf);
        exit(1);
    }
}
