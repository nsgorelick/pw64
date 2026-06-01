#include <stdio.h>
#include <math.h>
#include "Xfred.h"
#include "ColorControls.h"
#include <X11/keysym.h>

#define NOFILE "<Enter Filename>"


extern Display *display;
extern XFontStruct *font;
extern XColor pwBackground;
extern XColor pwHilite;

Button HistWindow = NULL;
Composite HistFilename;
List HistList;
int GetHistFilename(Composite C, XEvent *E);
char HFile[256];
static char **HistValues = NULL;
int HistCount;

void DoHistRead(Button B, XEvent *E);
void DoHistDone(Button B, XEvent *E);
void DoHistWrite(Button B, XEvent *E);
void GetPhoto(Button B, XEvent *E);

CreateHistogramManager(Display *display, XFontStruct *font)
{
    Window w;
    Button B;
    List list;

    HistWindow = XfCreateButton(display, 
                            RootWindow(display, DefaultScreen(display)),
                            100,100,230,200,1,WHITE(display),"hist-case",1);
    w = HistWindow->window;

    HistFilename = (Composite)CreateComposite(display, w, font, 15,5, 180, 20,
                     pwBackground.pixel, NOFILE);
    AddCompositeCallback(HistFilename,  GetHistFilename);
    HistFilename->ext = 0;
    ActivateComposite(HistFilename);

    B = Make2State(display, w, font, 15, 40, 60, 20, 1, BLACK(display),
                    BLACK(display), WHITE(display), "READ");
    XfAddButtonCallback(B, 0, DoHistRead, NULL);

    B = Make2State(display, w, font, 85, 40, 60, 20, 1, BLACK(display),
                    BLACK(display), WHITE(display), "WRITE");
    XfAddButtonCallback(B, 0, DoHistWrite, NULL);

    B = Make2State(display, w, font, 155, 40, 60, 20, 1, BLACK(display),
                    BLACK(display), WHITE(display), "DONE");
    XfAddButtonCallback(B, 0, DoHistDone, NULL);

    B = Make2State(display, w, font, 155, 70, 60, 20, 1, BLACK(display),
                    BLACK(display), pwBackground.pixel, "ADD");

    B = Make2State(display, w, font, 155, 100, 60, 20, 1, BLACK(display),
                    BLACK(display), pwBackground.pixel, "DELETE");

    B = Make2State(display, w, font, 155, 130, 60, 20, 1, BLACK(display),
                    BLACK(display), pwBackground.pixel, "APPLY");

    B = Make2State(display, w, font, 155, 160, 60, 20, 1, BLACK(display),
                    BLACK(display), WHITE(display), "RE-READ");
    XfAddButtonCallback(B, 0, GetPhoto, NULL);
    XfAddButtonCallback(B, 0, toggle_state, NULL);

    HistList = CreateList(display, w, font, 15, 70, 
        130, 80, 10, 0, pwBackground.pixel, 0, 0);
    ActivateList(HistList);

    B = XfCreateButton(display, w, 15, 160, 60, 20, 1, BLACK(display),
                        "Edit1", 1);
    XfAddButtonVisual(B, 0, XfCreateVisual(B, 0, 7, 0, 0, BLACK(display),
            WHITE(display), XfTextVisual, "-", font, 0));
    XfActivateButton(B, ExposureMask | ButtonPressMask | KeyPressMask);

    B = XfCreateButton(display, w, 85, 160, 60, 20, 1, BLACK(display),
                        "Edit2", 1);
    XfAddButtonVisual(B, 0, XfCreateVisual(B, 0, 7, 0, 0, BLACK(display),
            WHITE(display), XfTextVisual, "-", font, 0));

    XfActivateButton(B, ExposureMask | ButtonPressMask | KeyPressMask);

}


GetHistFilename(Composite C, XEvent *E)
{
    C->ext = 1;
    strcpy(HFile,C->current_text);
    SetButtonText(HistFilename->Edit, C->current_text);
    PushdownComposite(C);
}

void
DoHistRead(Button B, XEvent *E)
{
    extern struct ColorControls *CC;
    struct Point *points;
    int i, count = 0;
    float xlow, xhigh;
    float ylow, yhigh;
	float v;
    float x, y;
	float lastx, lasty;
    char buf[256];
    FILE *fp;

    if (strlen(HFile) == 0) {
        XBell(B->display,50);
    } else {
        if (CC->image != NULL) {
            xlow = CC->image->s_low;
            xhigh = CC->image->s_high;
            
			lastx = lasty = -1000000;	/* large negative value */

            if ((fp = fopen(HFile,"r")) == NULL) {
                XBell(B->display,50);
                return;
            }
            points =(struct Point *)calloc(CC->Map->width,sizeof(struct Point));
            while(fgets(buf,256,fp) != NULL) {
                sscanf(buf, "%f %f", &x, &y);
				/**
				 ** If currrent is less, or last if greater, we can skip this
				 ** one.
				 **/
				if (x <= xlow || lastx >= xhigh) {
					lastx = x;
					lasty = y;
					count++;
					continue;
				}
				if (x >= xlow && lastx <= xlow && count) {
					/**
					 ** Points have crossed lower boundary,
					 ** Interpolate a lower bound value.
					 **/
					v = (y-lasty)/((x-lastx)/(xlow-lastx))+lasty;
					points[0].v = v/((1 << CC->image->header.bits) -1) *
                                (CC->Map->height -1);
					points[0].flag = 1;
				}
				if (x >= xhigh && lastx <= xhigh && count) {
					/**
					 ** Points have crossed upper boundary,
					 ** Interpolate an upper bound value.
					 **/
					v = (y-lasty)/((x-lastx)/(xhigh-lastx))+lasty;
					i = CC->Map->width-1;
					points[i].v = v/((1 << CC->image->header.bits) -1) *
                                (CC->Map->height -1);
					points[i].flag = 1;
				}
				if (x >= xlow && x <= xhigh) {
					/**
					 ** This point is inside the map.
					 **/
					i = (x - xlow)/(xhigh-xlow)*(CC->Map->width-1);
					points[i].v = y/((1 << CC->image->header.bits) -1) *
                                (CC->Map->height -1);
					points[i].flag = 1;
				}
				lastx = x;
				lasty = y;
				count++;
            }
            fclose(fp);
            XfRestoreAMap(CC->Map, points);
            GetPhoto(B,E);
            free(points);
        }
    }
    toggle_state(B,E);
}
void
DoHistDone(Button B, XEvent *E)
{
    toggle_state(B,E);
    XfDeactivateButton(HistWindow);
}
void 
DoHistWrite(Button B, XEvent *E)
{
    extern struct ColorControls *CC;
    struct Point *points;
    int i, count;
    float xlow, xhigh;
    float ylow, yhigh;
    float x, y;
    char buf1[256];
    char buf2[256];
    char buf[256];
    FILE *fp;

    if (strlen(HFile) == 0) {
        XBell(B->display, 50);
    } else {
        if (CC->image != NULL) {
            points = XfPhotographAMap(CC->Map);

            fp = fopen(HFile,"w");

            xlow = CC->image->s_low;
            xhigh = CC->image->s_high;

            for (i = 0 ; i < CC->Map->width ; i++) {
                if (points[i].flag) {
					x = (float)i/(float)(CC->Map->width-1)
									* (xhigh - xlow) + xlow;
                    y = (float)(points[i].v) / (float)(CC->Map->height -1)
                                    * ((1 << CC->image->header.bits) - 1);
                    fprintf(fp, "%9.3f %9.3f\n" ,x, y);
                }
            }
            fclose(fp);
            free(points);
        }
    }
    toggle_state(B,E);
}


void
ActivateHistList(Button B, XEvent *E)
{
    if (HistWindow == NULL) {
        CreateHistogramManager(display, font);
    }
    XfActivateButton(HistWindow, ExposureMask);
}

void
DeactivateHistList(Button B, XEvent *E)
{
    XfDeactivateButton(HistWindow);
}

void
GetPhoto(Button B, XEvent *E)
{
    extern struct ColorControls *CC;
    struct Point *points;
    int i, count;
    float xlow, xhigh;
    float ylow, yhigh;
    float x, y;
    char buf1[256];
    char buf2[256];
    char buf[256];

    if (CC->image != NULL) {
        xlow = CC->image->s_low;
        xhigh = CC->image->s_high;
        
        points = XfPhotographAMap(CC->Map);
        count = 0;
        for (i = 0 ; i < CC->Map->width ; i++) {
            if (points[i].flag) count++;
        }

        if (HistValues != NULL) {
            for (i = 0 ; i < HistCount ; i++) {
                free(HistValues[i]);
            }
            free(HistValues);
        }
        HistValues = (char **)malloc(sizeof(char *)*count);

        HistCount = count;

        count = 0;
        for (i = 0 ; i < CC->Map->width ; i++) {
            if (points[i].flag) {
                x = (float)i/(float)(CC->Map->width-1) * (xhigh - xlow) + xlow;
                y = (float)(points[i].v) / (float)(CC->Map->height -1 )
                                * ((1 << CC->image->header.bits) - 1);
                sprintf(buf,"%9.3f %9.3f" ,x, y);

                HistValues[count++] = strdup(buf);
            }
        }
        ReCreateList(HistList, HistCount, HistValues);
    }
}
