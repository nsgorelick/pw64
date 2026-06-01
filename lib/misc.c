#include "Xfred.h"
#include <string.h>

int GetText(Button B, XEvent *E, char *s, int n, int copy)
{
    char *p = NULL;
    char *q;
    XFontStruct *font = NULL;
    struct VisualInfo *vis;
    int state;

    if (B == NULL || B->States == NULL || n <= 0)
        return -1;

    state = B->state;
    if (state < 0 || state >= B->maxstate)
        state = 0;

    for (vis = B->States[state]->Visuals; vis != NULL; vis = vis->next) {
        if (vis->vtype == XfTextVisual) {
            font = vis->visual.t_vis.font;
            if (copy)
                p = vis->visual.t_vis.text;
            break;
        }
    }
    if (font == NULL)
        return -1;

    q = xgets(B->display, B->window, 0, 0, B->width, B->height, WHITE(B->display), BLACK(B->display), font, p, E);

    if (q == NULL || *q == '\0')
        return -1;
    strncpy(s, q, (size_t) n);
    s[n - 1] = '\0';

    return 1;
}

#include <stdio.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifndef MIN
#define MIN(a,b)	((a) < (b) ? (a) : (b))
#endif

int getbit(FILE *file)
{
    int ch;

    do {
        ch = fgetc(file);
        if (ch == '#') {
            while ((ch = fgetc(file)) != '\n' && ch != EOF);
        }
    } while (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r');

    if (ch != '0' && ch != '1')
        return (-1);

    return (ch == '1') ? 1 : 0;
}

int get_int(FILE *fp)
{
    int i = 0;
    char buf[256];
    int ch;

    while ((ch = fgetc(fp)) != EOF && !isdigit(ch)) {
        if (ch == '#') {
            do {
                ch = fgetc(fp);
            }
            while (ch != '\n');
        }
    }
    if (ch < '0' || ch > '9')
        return (-1);

    do {
        buf[i++] = ch;
        ch = fgetc(fp);
    } while (ch >= '0' && ch <= '9');

    buf[i] = '\0';
    return (atoi(buf));
}

float get_float(FILE *fp)
{
    int i = 0;
    char buf[256];
    int ch;

    while ((ch = fgetc(fp)) != EOF && isspace(ch)) {
        if (ch == '#') {
            do {
                ch = fgetc(fp);
            }
            while (ch != '\n');
        }
    }
    if (!isdigit(ch) && ch != '.' && ch != 'e' && ch != '-')
        return (-1);

    do {
        buf[i++] = ch;
        ch = fgetc(fp);
    } while (isdigit(ch) || ch == '.' || ch == 'e' || ch == '-');

    buf[i] = '\0';
    return (atof(buf));
}

int is_file(char *path)
{
    int i;
    struct stat sb;

    i = stat(path, &sb);
    if (i == 0) {
        if ((sb.st_mode & S_IFMT) == S_IFDIR) {
            return (0);
        }
        return (1);
    }
    return (0);
}

int is_dir(char *path)
{
    struct stat sb;

    stat(path, &sb);
    if ((sb.st_mode & S_IFMT) == S_IFDIR) {
        return 1;
    }
    return 0;
}

/**
*** The following two routines encode and decode a string using
*** the TIFF packbits encoding scheme.  This encoding method is
*** used by several HP peripherals to reduce the size of incoming
*** raster images.  Both routines convert s1 into s2.  The len parameter
*** indicates the size of the incoming string.  The return value is
*** the size of the string output into s2.
**/

int ToTiff(unsigned char *s1, unsigned char *s2, int len)
{
        /**
	*** Pack s1 using TIFF packbits encoding into s2
	**/
    int count = 0;
    int i;
    int base, newbase, size, outp;

    base = newbase = outp = 0;
    for (i = 1; i < len; i++) {
        if (s1[newbase] == s1[i]) {
            count++;
        } else {
            if (count < 2) {
                newbase = i;
                count = 0;
            } else {
                                /**
				*** Put any backed up literals first.
				**/
                while ((newbase - base) > 0) {
                    size = MIN(127, newbase - base - 1);
                    s2[outp++] = size;
                    memcpy(s2 + outp, s1 + base, size + 1);
                    outp += size + 1;
                    base += size + 1;
                }
                                /**
				*** Now put -count and repeated string.
				**/
                count++;
                while (count > 0) {
                    size = MIN(128, count);
                    s2[outp++] = -(size - 1);
                    s2[outp++] = s1[newbase];
                    count -= size;
                }
                base = newbase = i;
            }
        }
    }
        /**
	*** Output any trailing literals.
	**/
    newbase = i;
    while ((newbase - base) > 0) {
        size = MIN(127, newbase - base - 1);
        s2[outp++] = size;
        memcpy(s2 + outp, s1 + base, size + 1);
        outp += size + 1;
        base += size + 1;
    }
    return (outp);
}

int UnTiff(unsigned char *s1, unsigned char *s2, int len)
{
    int i, j, size;
    int outp;

    outp = 0;
    for (i = 0; i < len; i++) {
        size = ((char *) s1)[i];
        if (size < 0) {
                        /**
			*** repeat next character -N times.
			**/
            size = -size;
            memset(s2 + outp, s1[i + 1], size + 1);
            outp += size + 1;
            i++;
        } else {
                        /**
			*** Copy N characters
			**/
            for (j = 0; j <= size; j++) {
                i++;
                s2[outp] = s1[i];
                outp++;
            }
        }
    }
    return (outp);
}

/**
 ** Check the pointer and see if any mouse buttons are down.
 **/
int ButtonIsDown(Display *display, Window window)
{
    Window root, child;
    int root_x, root_y;
    int win_x, win_y;
    unsigned int state;

    XQueryPointer(display, window, &root, &child, &root_x, &root_y, &win_x, &win_y, &state);

    return ((state == Button1Mask || state == Button2Mask || state == Button3Mask));
}
