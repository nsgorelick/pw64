#include <stdio.h>
#include <fcntl.h>
#include <X11/keysym.h>
#include "Xfred.h"
#include "image.h"
#include "ColorControls.h"
#include "util.h"
#include "mag.h"
#include "color.h"

#define NO_FILENAME "      Click Here To Enter Filename"

int PopupActive=0;

int WriteHeader;
int WriteType;
char    WriteFilename[3][256];

extern struct ColorControls *CC;
extern XColor pwBackground;
extern XColor Colors[];
extern Display *display;
extern Colormap ColorMap;
extern int IWidth,IHeight;
extern int IXPos,IYPos;
extern XImage *IImage;
extern struct magnify *Mag;
extern GC gc;
extern XFontStruct *font;

void WriteGetFilename(Button B, XEvent *E);
void CancelWrite(Button B, XEvent *E);
void DoWrite(Button B, XEvent *E);
void ToggleWriteHeader(Button B, XEvent *E);
void ToggleWriteType(Button B, XEvent *E);

FILE *InitWriteFile(char *filename, Window parent);
int  WriteFileHeader(FILE *f);
int  WriteFile(FILE *f, int i, int nfiles);

#define FORMAT_VICAR 0
#define FORMAT_RAW 1
#define FORMAT_PGM 2
#define FORMAT_XPM 3
#define FORMAT_PPM 4



extern int events (XEvent *E);
extern char *trim_filename (char *s, int n);
extern int WriteVicarHeader (FILE *f, int x, int y);

WritePopup(Display *display, Window parent, XFontStruct *font)
{

    XEvent E;
    Button B;
    int i,x,y,width,height,count,twidth;
    Button b[8];
    char *text;

    PopupActive = 1 - PopupActive;
    if (PopupActive == 0) return;

    WriteFilename[0][0] = '\0';
    WriteFilename[1][0] = '\0';
    WriteFilename[2][0] = '\0';

    count = 0;
    
    B = XfCreateButton(display, parent, 100, 100, 300, 140, 1, 
                       BLACK(display), "PW Write", 1);
    XfAddButtonVisual(B, 0, XfCreateVisual(B, 0, 3, 0, 0,
        WHITE(display), WHITE(display), XfSolidVisual));
    b[count++] = B;

    MapAndWait(display, b[0]->window);
    XfActivateButton(b[0], ExposureMask);
    UpdateButton(b[0]);

    x = 50;
    y = 10;
    width = 235;
    height = 20;
    text = "Red";
    twidth = 40;

    MakeTextButton(display, ((b[0])->window), x-twidth,y+10,
                    twidth,height,font, text, 1);

    B = XfCreateButton(display, (b[0])->window, x, y, width, 20, 2, 
        BLACK(display), "filename", 1);
    XfAddButtonVisual(B, 0, XfCreateVisual(B, 0, 7, 0, 0,
        BLACK(display), pwBackground.pixel,
        XfTextVisual, NO_FILENAME, font, 1));
    XfAddButtonCallback(B, 0, WriteGetFilename, NULL);
    XfActivateButton(B, KeyPressMask | ExposureMask | ButtonPressMask);
    B->ext = (char *)count;
    b[count++] = B;

    y += 25;
    text = "Green";
    MakeTextButton(display, ((b[0])->window), x-twidth,y+10,
                    twidth,height,font, text, 1);

    B = XfCreateButton(display, (b[0])->window, x, y, width, 20, 2, 
        BLACK(display), "filename", 1);
    XfAddButtonVisual(B, 0, XfCreateVisual(B, 0, 7, 0, 0,
        BLACK(display), pwBackground.pixel,
        XfTextVisual, NO_FILENAME, font, 1));
    XfAddButtonCallback(B, 0, WriteGetFilename, NULL);
    XfActivateButton(B, KeyPressMask | ExposureMask | ButtonPressMask);
    B->ext = (char *)count;
    b[count++] = B;

    y += 25;
    text = "Blue";
    MakeTextButton(display, ((b[0])->window), x-twidth,y+10,
                    twidth,height,font, text, 1);

    B = XfCreateButton(display, (b[0])->window, x, y, width, 20, 2, 
        BLACK(display), "filename", 1);
    XfAddButtonVisual(B, 0, XfCreateVisual(B, 0, 7, 0, 0,
        BLACK(display), pwBackground.pixel,
        XfTextVisual, NO_FILENAME, font, 1));
    XfAddButtonCallback(B, 0, WriteGetFilename, NULL);
    XfActivateButton(B, KeyPressMask | ExposureMask | ButtonPressMask);
    B->ext = (char *)count;
    b[count++] = B;

    x = 50;
    y += 40;
    width = 50;
    text = "Header";

    MakeTextButton(display, ((b[0])->window), x,y-10,
                    width,height,font, text, 0);

#ifdef HAVE_XPM
    /* Add state for XPM */
    B = XfCreateButton(display, (b[0])->window, x, y, width, 20, 2, 
        BLACK(display), "write", 4);
    XfAddButtonVisual(B, 3, XfCreateVisual(B, 0, 7, 0, 0,
        BLACK(display), WHITE(display), 
        XfTextVisual, "XPM", font, 0));
    XfAddButtonCallback(B, 3, ToggleWriteHeader , NULL);

#else

    B = XfCreateButton(display, (b[0])->window, x, y, width, 20, 2, 
        BLACK(display), "write", 3);

#endif /* HAVE_XPM */
    
    XfAddButtonVisual(B, 0, XfCreateVisual(B, 0, 7, 0, 0,
        BLACK(display), WHITE(display), 
        XfTextVisual, "VICAR", font, 0));
    XfAddButtonVisual(B, 1, XfCreateVisual(B, 0, 7, 0, 0,
        BLACK(display), WHITE(display), 
        XfTextVisual, "RAW", font, 0));
    XfAddButtonVisual(B, 2, XfCreateVisual(B, 0, 7, 0, 0,
        BLACK(display), WHITE(display), 
        XfTextVisual, "PGM", font, 0));
    XfAddButtonCallback(B, 0, ToggleWriteHeader , NULL);
    XfAddButtonCallback(B, 1, ToggleWriteHeader , NULL);
    XfAddButtonCallback(B, 2, ToggleWriteHeader , NULL);
    XfActivateButton(B, ExposureMask | ButtonPressMask);
    b[count++] = B;

    x = 110;
    text = "Write";

    MakeTextButton(display, ((b[0])->window), x,y-10,
                    width,height,font, text, 0);

    B = XfCreateButton(display, (b[0])->window, x, y, 50, 20, 2, 
        BLACK(display), "write", 3);
    XfAddButtonVisual(B, 0, XfCreateVisual(B, 0, 7, 0, 0,
        BLACK(display), WHITE(display), 
        XfTextVisual, "IMAGE", font, 0));
    XfAddButtonVisual(B, 1, XfCreateVisual(B, 0, 7, 0, 0,
        BLACK(display), WHITE(display), 
        XfTextVisual, "WINDOW", font, 0));
    XfAddButtonVisual(B, 2, XfCreateVisual(B, 0, 7, 0, 0,
        BLACK(display), WHITE(display), 
        XfTextVisual, "MAGNIFY", font, 0));
    XfAddButtonCallback(B, 0, ToggleWriteType, NULL);
    XfAddButtonCallback(B, 1, ToggleWriteType, NULL);
    XfAddButtonCallback(B, 2, ToggleWriteType, NULL);
    XfActivateButton(B, ExposureMask | ButtonPressMask);
    b[count++] = B;

    x = 170;

    B = XfCreateButton(display, (b[0])->window, x, y, 30, 20, 2, 
        BLACK(display), "write", 2);
    XfAddButtonVisual(B, 0, XfCreateVisual(B, 0, 7, 0, 0,
        BLACK(display), WHITE(display), 
        XfTextVisual, "GO", font, 0));
    XfAddButtonVisual(B, 1, XfCreateVisual(B, 0, 7, 0, 0,
        BLACK(display), pwBackground.pixel,
        XfTextVisual, "GO", font, 0));
    XfAddButtonCallback(B, 0, DoWrite, NULL);
    XfActivateButton(B, ExposureMask | ButtonPressMask);
    b[count++] = B;

    x = 235;

    B = XfCreateButton(display, (b[0])->window, x, y, 50, 20, 2, 
        BLACK(display), "cancel", 1);
    XfAddButtonVisual(B, 0, XfCreateVisual(B, 0, 7, 0, 0,
        BLACK(display), WHITE(display), 
        XfTextVisual, "CANCEL", font, 0));
    XfAddButtonCallback(B, 0, CancelWrite, NULL);
    XfActivateButton(B, ExposureMask | ButtonPressMask);
    b[count++] = B;


    PopupActive = 1;
    XFlush(display);

    while (XCheckMaskEvent(display, ButtonPressMask, &E))
        ;

    while (PopupActive == 1) {
        XNextEvent(display, &E);
        events(&E);
    }

    XfDeactivateButton(b[0]);
    XFlush(display);
    PopupActive = 0;
}

void
WriteGetFilename(Button B, XEvent *E)
{
    int i;
    Display * display;
    XFontStruct *font;
    char    *s, buf[256];
    int fp;
    char *str;

    display = B->display;
    font = (B->States[0])->Visuals->visual.t_vis.font;

    str = WriteFilename[(int)B->ext-1];

/* 
 *   Get filename.
 */
    s = xgets(display, B->window, 0, 0, B->width, B->height, 
        WHITE(display), BLACK(display), font, str, E);
    if (s != NULL && s[0] != '\0') {
        strcpy(str, s);
        strcpy((B->States[0])->Visuals->visual.t_vis.text, 
                (char *)trim_filename(str,45));
        (B->States[0])->Visuals->background = WHITE(display);
        UpdateButton(B);
    }
}

void
CancelWrite(Button B, XEvent *E)
{
    PopupActive = 0;
}

void
ToggleWriteHeader(Button B, XEvent *E)
{
    toggle_state(B,NULL);
    WriteHeader = B->state;
}

void
ToggleWriteType(Button B, XEvent *E)
{
    toggle_state(B,NULL);
    WriteType = B->state;
}


void
DoWrite(Button B, XEvent *E)
{
    FILE *f[3];
    int nfiles;
    int i;
    int lines,samples,label;

    if (CC->image == NULL) return;

    toggle_state(B, NULL);

    for (nfiles = 0 ; nfiles < 3 ; nfiles++) {
        if (WriteFilename[nfiles][0] == '\0') break;
    }
    if (nfiles == 1 || nfiles == 3) {
        for (i = 0 ; i < nfiles ; i++) {
            f[i] = NULL;
            f[i] = InitWriteFile(WriteFilename[i],B->parent);
            if (f[i] == NULL) break;
        }
        if (i == nfiles) {
            for (i = 0 ; i < nfiles ; i++) {
                WriteFileHeader(f[i]);
                WriteFile(f[i],i,nfiles);
            }
            PopupActive = 0;
        }
    }
    toggle_state(B, NULL);
}

FILE *
InitWriteFile(char *filename, Window parent)
{
    int i;
    FILE *f;
    char buf[256];
    int err;



    i = open(filename, O_EXCL | O_CREAT, 0744);
    if (i > 0) {
        close(i);
    } else {
        sprintf(buf,"%s exists",trim_filename(filename, 30));
        err = ConfirmRequestor(display, parent, gc, font, 
                         0, 0, 300, 140, 
                         1, 0, 0,
                         1, buf,
                         3, "OVERWRITE", "APPEND", "CANCEL");

        if (err == 1) {
            unlink(filename);
        } else if (err == 2) {
            if (WriteHeader != FORMAT_RAW) return(NULL);
        } else {
            return(NULL);
        }
    }
    f = fopen(filename, "a");
    return(f);
}

WriteFileHeader(FILE *f)
{
    char    *buf;
    char    tbuf[256];
    long c;
    int i;
    int label;
    int lines,samples;
    int x,y;

    if (WriteHeader == FORMAT_RAW) return;

    switch (WriteType) {
        case 0:
            x = CC->image->subset.width;
            y = CC->image->subset.height;
            break;
        case 1:         /* Window */
            x = IWidth;
            y = IHeight;
            break;
        case 2:         /* magnify */
            x = Mag->width;
            y = Mag->height;
            break;
    }

    switch (WriteHeader) {
        case FORMAT_VICAR:
            WriteVicarHeader(f, x, y);
            break;
        case FORMAT_PGM:
            fprintf(f, "P5\n%d %d\n%d\n", x, y, 255);
            break;
        default:
            break;
    }
}


WriteFile(FILE *f, int i, int nfiles)
{
    int xoff, yoff;
    char    *data;
    int lines, samples, width, height;
    int table[256];
    int colors;
    Image new;
    int j;
    int index;
    unsigned char val;
    char itoa[256][4];
    XColor lcolors[256];
    char buf[256];
    XImage *image;

    new = CC->image;


    if (CC->image == NULL) 
        return;

    /* Image */
    if (WriteType == 0) {
        lines = CC->image->subset.height;
        samples = CC->image->subset.width;
        data = CC->image->sdata;
        image = CC->image->ximage;
        width = samples;
        height = lines;
        xoff = 0;
        yoff = 0;
    }
    /* Window */
    if (WriteType == 1) {
        lines = IHeight;
        samples = IWidth;
        data = CC->image->sdata;
        width = CC->image->subset.width;
        height = CC->image->subset.height;
        xoff = IXPos;
        yoff = IYPos;
        image = IImage;
    }
    /* Magnify */
    if (WriteType == 2) {
        lines = Mag->height;
        samples = Mag->width;
        data = Mag->mag_data;
        width = Mag->_width;
        height = Mag->_height;
        xoff = 0;
        yoff = 0;
        image = Mag->mag_image;
    }

#ifdef HAVE_XPM
    if (WriteHeader == FORMAT_XPM) {
        XpmWriteFileFromImage(display, WriteFilename[i], image, NULL, NULL);
        return;
    }
#endif /* HAVE_XPM */

	memset(lcolors, '\0', sizeof(lcolors));
    for (j = 0 ; j < 256; j++) {
        lcolors[j].pixel = (unsigned long)j;
		sprintf(itoa[j], "%d", j);
    }


	XSync(display, False);
    XQueryColors(display, ColorMap, lcolors, 256);

    for (j = 0 ; j < 256 ; j++) {
        if (i == 0)
            table[j] = (255 *(unsigned int)lcolors[j].red / 0x0FFFF);
        else if (i == 1)
            table[j] = (255 *(unsigned int)lcolors[j].green / 0x0FFFF);
        else
            table[j] = (255 *(unsigned int)lcolors[j].blue / 0x0FFFF);
    }

    for (i = 0 ; i < lines ; i++) {
        for (j = 0 ; j < samples ; j++) {
            index = (unsigned char)data[(i+yoff)*width + j+xoff];
            val = (unsigned char)table[index];
			fwrite(&val, 1, 1, f);
			/*
            switch (WriteHeader) {
                case FORMAT_VICAR:
                case FORMAT_RAW:
                    fwrite(&val, 1, 1, f);
                    break;
                case FORMAT_PGM:
                    if (j && j % 20) fputc('\n', f);
                    fputs(itoa[val], f);
                    fputc(' ', f);
            }
			*/
        }
		/*
        if (WriteHeader == FORMAT_PGM) fputc('\n', f);
		*/
    }
    fclose(f);
}


