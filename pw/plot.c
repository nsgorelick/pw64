/*
 * Data block selection routines.
 *
 * CreateBlock()
 *
 * MovementCallback Plot()
 *
 */
#include "Xfred.h"
#include "bitmaps/bitmaps.h"
#include "block.h"
#include "config.h"
#include "image.h"
#include "mag.h"
#include "pw_cast.h"
#include "specpr.h"
#include "util.h"
#include <X11/keysym.h>
#include <fcntl.h>
#include <float.h>
#include <math.h>

void set_delete_box_text(int);
void set_delete_points(char **V, char *TR, int Index, int Ci);

#define NO_FILENAME "<NO FILE>"
#define NO_TITLE "No Title"

extern XColor pwRed, pwBlue, pwGreen, pwYellow, pwCyan, pwMagenta, pwHilite, pwBackground;
Button PlotsWindow;

Button CBlocks[7];
Button SwitchCase;
Button ScaleCase;
Button AxisCase;
Button PlotCase;
Button ReadoutCase;
Button CubeSelector;
Button PlotXPos;
Button PlotYPos;
Button XMax, XMin, YMax, YMin;
Button XB, YB;
Button ChanSpaceButton;
int ChanSpace = 1;
Button CreateCubeControls(Display *display, XFontStruct *fs);
int PlotWidth, PlotHeight;
float PlotXMin = 0.0, PlotXMax = 250.0;
float PlotYMin = 0.0, PlotYMax = 20000.0;

float PlotScaleFactor = 1.0;

int Pixels[7];
void BlockButtonCallback(Button B, XEvent *E);
void SpecprCancelWrite(Button B, XEvent *E);
void Toggle_Cycle(Button B, XEvent *E);
void SetScaleFactor(Button B, XEvent *E);
void SetDelFactor(Button B, XEvent *E);
void ReadXCallback(Button B, XEvent *E);
void ReadYCallback(Button B, XEvent *E);
void ActivateBlockIDCallback(Button B, XEvent *E);
void GetScale(Button B, XEvent *E);
void PlotCallback(Button B, XEvent *E);
void ShiftScale(Button B, XEvent *E);
void AxisCallback(Button B, XEvent *E);
void toggle_state(Button B, XEvent *E);
void DeleteAll(Button B, XEvent *E);
void SetCube(Button B, XEvent *E);
void resize(Button B, XEvent *E);
void Toggle_Append(Button B, XEvent *E);
void Toggle_Rotate(Button B, XEvent *E);
void DoCopy(Button B, XEvent *E);
void BlockMath(Button B, XEvent *E);
void BlockAverage(Button B, XEvent *E);
void AutoScale(Button B, XEvent *E);
void Toggle_Cycle(Button B, XEvent *E);
void SelectChanSpace(Button B, XEvent *E);
void set_delete_text_box(void);
void toggle_SpecprType(Button B, XEvent *E);
void toggle_SpecprScaled(Button B, XEvent *E);
void CopyExtendedBlock(Button B, XEvent *E);
void UnZoom(Button B, XEvent *E);

void GetCubeFnames(Button B, XEvent *E);
char *trim_filename(char *s, int n);

int CubeImage = -1;
Time ClickTime = 0;
float *waves[7] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL};

float *def_waves = NULL;
int nwaves[7], def_nwaves;
int ApplyX = 0;

Button SpecprWrite;
Button SpecprWriteMain;
Button SpecprWriteFilename;
Button SpecprWriteTitle;
Button SpecprWriteRecord;
Button SpecprWriteType;
Button SpecprWriteScaled;
Button SpecprWriteGo;
Button SpecprWriteCancel;
int Specpr_DoTitle = 1;

Button CubeControls = NULL;
Button Cube_OffFname;
Button Cube_MultFname;
Button Cube_OffTitle;
Button Cube_MultTitle;
Button Cube_Scale;
Button Cube_Deleted_Points;

void ActivateSpecprWrite(Button B, XEvent *E);
void DeactivateCubeControls(Button B, XEvent *E);

float *SpecprMultSpectra = NULL;
float *SpecprOffSpectra = NULL;

struct block_node *GetFirstBlock(int i);
PointData *get_block_data(int, int);
PointData *ConvertPoints(int, PointData *, float);
void RedrawAll(void); /* void */
void DrawPoints(int color, int n, XPoint *p); /* int, int, XPoint * */

struct PlotStruct SpecprPlot;
char *SP_XFilenames[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

extern Colormap ColorMap;

void fixfloat(char *str);
extern void CreateBlockID(Display *display, XFontStruct *font);
void set_plot_scale(float x1, float x2, float y1, float y2);
extern int SetPlotStruct(struct PlotStruct *plotstruct);
void do_resize(int width, int height);
extern int GetCurrentBlock(void);
extern void SetBlockState(int i, int state);
extern int SetCurrentBlock(int i);
extern int GetBlockCount(int i);
extern void *read_qube_data(int, struct _iheader *);
void SetCubeImage(int i);
extern int EnableBlock(void);
extern int GetNewText(Button B, XEvent *E, char *s, int n, char *str);
void set_scale_factor(float f);
void DrawAxis(void);
extern int GetExtendedBlock(void);
extern int GetBlockState(int i);
void zoom(int x, int y, int state);
void UpdatePlotReadout(XEvent *E);
extern void inice(float alow, float ahigh, float aamax, float *bint, float *astrt);
void CreateSpecprWritePopup(Display *display, XFontStruct *font);
void SetSpecprTitle(void);
extern struct _label *make_header(char *filename, int npixels, int waves, char *title, char *ahist, char *mhist);
extern void write_specpr(int fd, int i, struct _label *label, char *data);
extern int SetBlockData(int i, int type, PointData *pdata, int color);
extern int AddBlockData(int i, int type, PointData *pdata, int color);
void SetExtendedWaves(int i);
extern void ActivateBlockID(int i);
extern int GetNBlocks(void);
extern void ActivateSP(void);
extern void delete_all(int i);
void set_cube_correction(char *buf, Button b1, Button t1, float **fptr);
extern int read_specpr(int fd, int i, struct _label *label, char **data);

void CreateBlockPanel(Display *display, Window parent, XFontStruct *font)
{
    Button B;
    int width, height, x, y;
    int i, j;
    Window w;
    char buf[64];
    int fg, bg;

    x = 5;
    y = 35;
    width = height = 20;

    Pixels[0] = pwRed.pixel;
    Pixels[1] = pwGreen.pixel;
    Pixels[2] = pwBlue.pixel;
    Pixels[3] = pwYellow.pixel;
    Pixels[4] = pwMagenta.pixel;
    Pixels[5] = pwCyan.pixel;
    Pixels[6] = BLACK(display);

    if (parent == (Window)NULL)
        parent = RootWindow(display, DefaultScreen(display));

    B = XfCreateButton(display, parent, 275, 100, 345, 280, 1, BLACK(display), "PlotsWindow", 1);

    XSetWindowColormap(display, B->window, ColorMap);

    XfAddButtonVisual(B, 0, XfCreateVisual(B, 0, 0, 0, 0, WHITE(display), WHITE(display), XfSolidVisual));
    XfAddButtonCallback(B, 0, XF_CALLBACK(resize), NULL);
    PlotsWindow = B;
    PlotsWindow->ext = NULL;
    w = PlotsWindow->window;

    x = 120;
    y = 5;

    for (i = 0; i < 7; i++) {
        int width = 20;

        B = XfCreateButton(display, w, x, y, width, height, 1, BLACK(display), "Colors", 3);
        XfAddButtonVisual(B, 0,
                          XfCreateVisual(B, 2, 2, width - 4, height - 4, Pixels[i], WHITE(display), XfSolidVisual));
        XfAddButtonVisual(
            B, 0, XfCreateVisual(B, 1, 1, width - 3, height - 3, BLACK(display), WHITE(display), XfOutlineVisual));

        XfAddButtonVisual(B, 1, XfCreateVisual(B, 0, 0, 0, 0, Pixels[i], WHITE(display), XfSolidVisual));
        XfAddButtonVisual(
            B, 1, XfCreateVisual(B, 1, 1, width - 3, height - 3, BLACK(display), WHITE(display), XfOutlineVisual));
        XfAddButtonVisual(
            B, 1, XfCreateVisual(B, 0, 0, width - 1, height - 1, pwHilite.pixel, WHITE(display), XfOutlineVisual));

        XfAddButtonVisual(
            B, 2, XfCreateVisual(B, 2, 2, width - 4, height - 4, WHITE(display), WHITE(display), XfSolidVisual));
        XfAddButtonVisual(
            B, 2, XfCreateVisual(B, 1, 1, width - 3, height - 3, BLACK(display), WHITE(display), XfOutlineVisual));

        XfAddButtonCallback(B, 0, XF_CALLBACK(BlockButtonCallback), NULL);
        XfAddButtonCallback(B, 1, XF_CALLBACK(BlockButtonCallback), NULL);
        XfAddButtonCallback(B, 2, XF_CALLBACK(BlockButtonCallback), NULL);

        XfActivateButton(B, ExposureMask | ButtonPressMask);
        B->ext = PW_CAST_INT(i);
        y += height + 1;

        CBlocks[i] = B;
    }

    /* Control buttons */

    x = 5;
    y = 5;

    bg = BLACK(display);
    fg = WHITE(display);

    YB = B = Make2State3D(display, w, font, x, y, 50, 20, 1, bg, bg, fg, "READ Y");
    XfAddButtonCallback(B, 0, XF_CALLBACK(ReadYCallback), NULL);
    XfActivateButton(B, ExposureMask | ButtonPressMask);

    y += 25;
    XB = B = Make2State3D(display, w, font, x, y, 50, 20, 1, bg, bg, fg, "READ X");
    XfAddButtonCallback(B, 0, XF_CALLBACK(ReadXCallback), NULL);
    XfActivateButton(B, ExposureMask | ButtonPressMask);

    y += 25;
    B = Make2State3D(display, w, font, x, y, 50, 20, 1, bg, bg, fg, "LIST");
    XfAddButtonCallback(B, 0, XF_CALLBACK(ActivateBlockIDCallback), NULL);
    XfActivateButton(B, ExposureMask | ButtonPressMask);

    y += 25;
    B = Make2State3D(display, w, font, x, y, 50, 20, 1, bg, bg, fg, "WRITE");
    XfAddButtonCallback(B, 0, XF_CALLBACK(ActivateSpecprWrite), NULL);
    XfAddButtonCallback(B, 1, XF_CALLBACK(SpecprCancelWrite), NULL);
    B->ext = font;
    XfActivateButton(B, ExposureMask | ButtonPressMask);

    SpecprWrite = B;

    y += 25;
    B = Make2State3D(display, w, font, x, y, 50, 20, 1, bg, bg, fg, "COPY");
    XfAddButtonCallback(B, 0, XF_CALLBACK(CopyExtendedBlock), NULL);
    XfActivateButton(B, ExposureMask | ButtonPressMask);

    y += 25;
    B = XfCreateButton(display, w, x, y, 49, 20, 2, bg, "CHAN", 2);
    XfAddButtonVisual(B, 0, XfCreateVisual(B, 0, 7, 0, 0, bg, fg, XfTextVisual, "WAVES", font, 0));
    XfAddButtonVisual(B, 1, XfCreateVisual(B, 0, 7, 0, 0, bg, fg, XfTextVisual, "CHAN", font, 0));
    XfAddButtonCallback(B, 0, XF_CALLBACK(toggle_state), NULL);
    XfAddButtonCallback(B, 0, XF_CALLBACK(SelectChanSpace), NULL);
    XfAddButtonCallback(B, 1, XF_CALLBACK(toggle_state), NULL);
    XfAddButtonCallback(B, 1, XF_CALLBACK(SelectChanSpace), NULL);
    XfActivateButtonState(B, 1, ExposureMask | ButtonPressMask);

    ChanSpaceButton = B;

    /* start with append active */
    y = 5;
    x += 55;

    B = Make2State3D(display, w, font, x, y, 50, 20, 1, bg, bg, fg, "APPEND");
    XfAddButtonCallback(B, 0, XF_CALLBACK(Toggle_Append), NULL);
    XfAddButtonCallback(B, 1, XF_CALLBACK(Toggle_Append), NULL);
    XfActivateButtonState(B, 1, ExposureMask | ButtonPressMask);

    y += 25;
    B = Make2State3D(display, w, font, x, y, 50, 20, 1, bg, bg, fg, "CYCLE");
    XfAddButtonCallback(B, 0, XF_CALLBACK(Toggle_Cycle), NULL);
    XfAddButtonCallback(B, 1, XF_CALLBACK(Toggle_Cycle), NULL);
    XfActivateButton(B, ExposureMask | ButtonPressMask);

    /* Only 1 callback */

    y += 25;
    B = Make2State3D(display, w, font, x, y, 50, 20, 1, bg, bg, fg, "CLEAR");
    XfAddButtonCallback(B, 0, XF_CALLBACK(DeleteAll), NULL);
    XfAddButtonCallback(B, 0, XF_CALLBACK(toggle_state), NULL);
    XfActivateButton(B, ExposureMask | ButtonPressMask);

    /* Only 1 callback */

    y += 25;
    B = Make2State3D(display, w, font, x, y, 50, 20, 1, bg, bg, fg, "AVERAGE");
    XfAddButtonCallback(B, 0, XF_CALLBACK(BlockAverage), NULL);
    XfActivateButton(B, ExposureMask | ButtonPressMask);

    y += 25;

    /* Cube selection */
    y += 20;
    x += width / 2;
    width = 25;
    height = 20;
    MakeTextButton(display, w, x, y - 10, width, height, font, "Cube", 0);

    B = Make2State3D(display, w, font, x, y, width, height, 1, bg, bg, fg, "-");
    XfAddButtonCallback(B, 0, XF_CALLBACK(SetCube), NULL);
    XfAddButtonCallback(B, 1, XF_CALLBACK(SetCube), NULL);
    XfActivateButton(B, ExposureMask | ButtonPressMask | KeyPressMask);
    CubeSelector = B;

    x += width + 5;
    width = 50;

    /*
        MakeTextButton(display,w,x,y-10,width,height,font,"Scale",0);
        B = XfCreateButton(display, w, x, y, width, height,
                   1, BLACK(display), "SCALE", 1);
        XfAddButtonVisual(B, 0, XfCreateVisual(B, 0, 7, 0, 0,
                             BLACK(display), WHITE(display),
                               XfTextVisual, "1", font, 0));
        XfAddButtonVisual(B, 0, XfCreateVisual(B, -1, -1, width, height,
                             BLACK(display), WHITE(display), XfOutlineVisual));
        XfAddButtonCallback(B, 0, XF_CALLBACK(SetScaleFactor), NULL);
        XfActivateButton(B, ExposureMask | ButtonPressMask | KeyPressMask);
    */

    /* Readout */

    x = 10;
    y += 35;
    B = XfCreateButton(display, w, x, y, 260, 34, 0, BLACK(display), "ReadoutCase", 1);
    XfAddButtonVisual(B, 0, XfCreateVisual(B, x, 0, 0, 0, WHITE(display), WHITE(display), XfSolidVisual));
    ReadoutCase = B;

    sprintf(buf, "%s", "----");
    B = XFCreateButton(display, ReadoutCase->window, 0, 5, 40, 20, 1, BLACK(display), pwBackground.pixel, "XPos", 1);
    XfAddButtonVisual(B, 0,
                      XfCreateVisual(B, 0, 7, 0, 0, BLACK(display), pwBackground.pixel, XfTextVisual, buf, font, 0));
    XfActivateButton(B, ExposureMask);
    PlotXPos = B;

    sprintf(buf, "%s", "----");
    B = XFCreateButton(display, ReadoutCase->window, 50, 5, 40, 20, 1, BLACK(display), pwBackground.pixel, "YPos", 1);
    XfAddButtonVisual(B, 0,
                      XfCreateVisual(B, 0, 7, 0, 0, BLACK(display), pwBackground.pixel, XfTextVisual, buf, font, 0));
    XfActivateButton(B, ExposureMask);
    PlotYPos = B;

    XfActivateButton(ReadoutCase, ExposureMask);

    /* Axis window */

    B = XfCreateButton(display, w, 10, 10, 10, 10, 1, BLACK(display), " ", 1);
    XfAddButtonVisual(B, 0, XfCreateVisual(B, 0, 0, 0, 0, WHITE(display), WHITE(display), XfTextVisual, " ", font, 1));
    XfNoAutoExposeButton(B);
    XfAddButtonCallback(B, 0, XF_CALLBACK(AxisCallback), NULL);
    XfActivateButton(B, ExposureMask);
    AxisCase = B;

    /* Plot window */

    B = XfCreateButton(display, w, 190, 5, 190, 170, 1, BLACK(display), " ", 1);
    PlotCase = B;
    XfNoAutoExposeButton(B);
    /*
     * j = 260*200; PlotCase->ext = (char *)malloc((unsigned int)j); for
     * (i = 0 ; i < j ; i++) { PlotCase->ext[i] = WHITE(display); }
     *
     * XfAddButtonVisual(B, 0, XfCreateVisual(B, 0, 0, 0, 0, WHITE(display),
     * WHITE(display), XfXImageVisual, 8, ZPixmap, PlotCase->ext));
     */
    XfAddButtonVisual(B, 0, XfCreateVisual(B, 0, 0, 0, 0, WHITE(display), WHITE(display), XfSolidVisual));
    XfAddButtonCallback(B, 0, XF_CALLBACK(PlotCallback), NULL);
    XfActivateButton(B, ExposureMask | PointerMotionMask | ButtonMotionMask | ButtonPressMask | ButtonReleaseMask);

    /* Sizing */
    x = 10;
    y = 0;

    B = XfCreateButton(display, w, 160, 240, 220, 50, 0, BLACK(display), " ", 1);
    XfAddButtonVisual(B, 0, XfCreateVisual(B, x, 0, 0, 0, WHITE(display), WHITE(display), XfSolidVisual));
    ScaleCase = B;

    B = XfCreateButton(display, ScaleCase->window, 0, 0, 20, 20, 1, BLACK(display), " ", 2);
    XfAddButtonVisual(
        B, 0, XfCreateVisual(B, 0, 0, 20, 20, BLACK(display), WHITE(display), XfPixmapVisual, 1, Bitmap_Zoom_bits));
    XfAddButtonVisual(
        B, 1, XfCreateVisual(B, 0, 0, 20, 20, WHITE(display), BLACK(display), XfPixmapVisual, 1, Bitmap_Zoom_bits));
    XfAddButtonCallback(B, 0, XF_CALLBACK(AutoScale), NULL);
    XfActivateButton(B, ExposureMask | ButtonPressMask);

    B = XfCreateButton(display, ScaleCase->window, 0, 25, 20, 20, 1, BLACK(display), " ", 1);
    XfAddButtonVisual(B, 0,
                      XfCreateVisual(B, 1, 7, 10, 20, BLACK(display), WHITE(display), XfTextVisual, "U", font, 0));
    XfAddButtonCallback(B, 0, XF_CALLBACK(UnZoom), NULL);
    XfActivateButton(B, ExposureMask | ButtonPressMask);

    B = XfCreateButton(display, ScaleCase->window, x + 25, 0, 25, 20, 1, BLACK(display), "X_Up", 1);
    XfAddButtonVisual(B, 0,
                      XfCreateVisual(B, 1, 7, 10, 20, BLACK(display), WHITE(display), XfTextVisual, "X", font, 0));
    XfAddButtonVisual(
        B, 0,
        XfCreateVisual(B, 12, 2, 10, 15, BLACK(display), WHITE(display), XfPixmapVisual, 1, Bitmap_Arrow_Up_15_bits));
    XfAddButtonCallback(B, 0, XF_CALLBACK(ShiftScale), NULL);
    XfActivateButton(B, ExposureMask | ButtonPressMask);

    B = XfCreateButton(display, ScaleCase->window, x + 25, 25, 25, 20, 1, BLACK(display), "X_Down", 1);
    XfAddButtonVisual(B, 0,
                      XfCreateVisual(B, 1, 7, 10, 20, BLACK(display), WHITE(display), XfTextVisual, "X", font, 0));
    XfAddButtonVisual(
        B, 0,
        XfCreateVisual(B, 12, 2, 10, 15, BLACK(display), WHITE(display), XfPixmapVisual, 1, Bitmap_Arrow_Down_15_bits));
    XfAddButtonCallback(B, 0, XF_CALLBACK(ShiftScale), NULL);
    XfActivateButton(B, ExposureMask | ButtonPressMask);

    sprintf(buf, "%-.5g", PlotXMax);
    fixfloat(buf);
    B = XfCreateButton(display, ScaleCase->window, x + 55, 0, 40, 20, 1, BLACK(display), "XMax", 1);
    XfAddButtonVisual(B, 0, XfCreateVisual(B, 0, 7, 0, 0, BLACK(display), WHITE(display), XfTextVisual, buf, font, 0));
    XfAddButtonCallback(B, 0, XF_CALLBACK(GetScale), NULL);
    XfActivateButton(B, ExposureMask | ButtonPressMask | KeyPressMask);
    XMax = B;

    sprintf(buf, "%-.5g", PlotXMin);
    fixfloat(buf);
    B = XfCreateButton(display, ScaleCase->window, x + 55, 25, 40, 20, 1, BLACK(display), "XMin", 1);
    XfAddButtonVisual(B, 0, XfCreateVisual(B, 0, 7, 0, 0, BLACK(display), WHITE(display), XfTextVisual, buf, font, 0));
    XfAddButtonCallback(B, 0, XF_CALLBACK(GetScale), NULL);
    XfActivateButton(B, ExposureMask | ButtonPressMask | KeyPressMask);
    XMin = B;

    /* Y sizing */

    x = 90;
    y = 0;

    B = XfCreateButton(display, ScaleCase->window, x + 25, 0, 25, 20, 1, BLACK(display), "Y_Up", 1);
    XfAddButtonVisual(B, 0,
                      XfCreateVisual(B, 1, 7, 10, 20, BLACK(display), WHITE(display), XfTextVisual, "Y", font, 0));
    XfAddButtonVisual(
        B, 0,
        XfCreateVisual(B, 12, 2, 10, 15, BLACK(display), WHITE(display), XfPixmapVisual, 1, Bitmap_Arrow_Up_15_bits));
    XfAddButtonCallback(B, 0, XF_CALLBACK(ShiftScale), NULL);
    XfActivateButton(B, ExposureMask | ButtonPressMask);

    B = XfCreateButton(display, ScaleCase->window, x + 25, 25, 25, 20, 1, BLACK(display), "Y_Down", 1);
    XfAddButtonVisual(B, 0,
                      XfCreateVisual(B, 1, 7, 10, 20, BLACK(display), WHITE(display), XfTextVisual, "Y", font, 0));
    XfAddButtonVisual(
        B, 0,
        XfCreateVisual(B, 12, 2, 10, 15, BLACK(display), WHITE(display), XfPixmapVisual, 1, Bitmap_Arrow_Down_15_bits));
    XfAddButtonCallback(B, 0, XF_CALLBACK(ShiftScale), NULL);
    XfActivateButton(B, ExposureMask | ButtonPressMask);

    sprintf(buf, "%-.5g", PlotYMax);
    fixfloat(buf);
    B = XfCreateButton(display, ScaleCase->window, x + 55, 0, 40, 20, 1, BLACK(display), "YMax", 1);
    XfAddButtonVisual(B, 0, XfCreateVisual(B, 0, 7, 0, 0, BLACK(display), WHITE(display), XfTextVisual, buf, font, 0));
    XfAddButtonCallback(B, 0, XF_CALLBACK(GetScale), NULL);
    XfActivateButton(B, ExposureMask | ButtonPressMask | KeyPressMask);
    YMax = B;

    sprintf(buf, "%-.5g", PlotYMin);
    fixfloat(buf);
    B = XfCreateButton(display, ScaleCase->window, x + 55, 25, 40, 20, 1, BLACK(display), "YMin", 1);
    XfAddButtonVisual(B, 0, XfCreateVisual(B, 0, 7, 0, 0, BLACK(display), WHITE(display), XfTextVisual, buf, font, 0));
    XfAddButtonCallback(B, 0, XF_CALLBACK(GetScale), NULL);
    XfActivateButton(B, ExposureMask | ButtonPressMask | KeyPressMask);
    YMin = B;

    XfActivateButton(ScaleCase, ExposureMask | ButtonPressMask);

    CreateBlockID(display, font);

    SpecprPlot.get = get_block_data;
    SpecprPlot.scale = ConvertPoints;
    SpecprPlot.draw = DrawPoints;
    SpecprPlot.drawall = RedrawAll;

    set_plot_scale(PlotXMin, PlotXMax, PlotYMin, PlotYMax);

    CreateCubeControls(B->display, (B->States[0]->Visuals[0]).visual.t_vis.font);
}

void ActivatePlots(Display *d)
{
    XWindowAttributes xaw;
    XEvent E;

    SetPlotStruct(&SpecprPlot);
    XfActivateButton(PlotsWindow, ExposureMask | StructureNotifyMask);
    do_resize(PlotsWindow->width, PlotsWindow->height);
}

void DeactivatePlots(Display *d) { XfDeactivateButton(PlotsWindow); }

void BlockButtonCallback(Button B, XEvent *E)
{
    Button b;
    int current;

    int i = GetCurrentBlock();
    current = PW_CAST_PTR_INT(B->ext);

    if (i != -1) {
        if (i != current) {
            b = CBlocks[i];
            b->state = 0;
            UpdateButton(b);
        }
    }

    if (B->state == 1 && E->xbutton.time - ClickTime < 500) {
        B->state = 2;
        SetBlockState(current, 0);
        SetCurrentBlock(-1);
        if (GetBlockCount(current) != 0) {
            RedrawAll();
        }
        ClickTime = 0;
    } else if (B->state == 0 || B->state == 2) {
        SetBlockState(current, 1);
        SetCurrentBlock(current);
        if (B->state == 2 && GetBlockCount(current) != 0) {
            RedrawAll();
        }
        B->state = 1;
        ClickTime = E->xbutton.time;
    } else {
        ClickTime = E->xbutton.time;
    }
    UpdateButton(B);
}

void *idata = NULL; /* static data cache */
int lr_x = -1, lr_y = -1; /* last position read */
int CubeImageFd = -1; /* File pointer */

/*
 *  This routine gets a spectra from the location x,y, out of the currently
 *  selected cube.
 */

PointData *get_block_data(int x, int y)
/* position to read from */
{
    int fd;
    void *data;
    int min, max;
    Image new;
    int i;
    struct _iheader h;

    /* Make sure a cube has been selected. */
    if (CubeImage == -1) {
        return NULL;
    }
    new = Images[CubeImage];
    h = new->iheader;

    data = (void *)malloc(new->header.bands * NBYTES(h.format));

    /*
     * If we didn't read this line (y value) the last time, get the data
     * from the file.  Otherwise, just get it out of the cached memory.
     */

    if (y != lr_y) {
        if (idata != NULL) {
            free(idata);
        }
        if (CubeImageFd < 0)
            CubeImageFd = open(new->filename, O_RDONLY);

        fd = CubeImageFd;

        if (fd < 0) {
            /* This shouldn't happen unless someone removed the file */
            (void)printf("Can't open: %s\n", new->filename);
            free(data);
            return NULL;
        }
        /* Read the data, according to the file type */

        h.s_lo[orders[h.org][1]] = y + 1;
        h.s_hi[orders[h.org][1]] = y + 1;

        idata = (void *)read_qube_data(fd, &h);

        lr_y = y;
    }
    /* Copy the desired spectra from the extracted line into malloc'd memory */
    for (i = 0; i < new->header.bands; i++) {
        switch (new->header.format) {
        case BYTE:
            ((unsigned char *)data)[i] = ((unsigned char *)idata)[i * new->header.samples + x];
            break;
        case SHORT:
            ((short *)data)[i] = ((short *)idata)[i * new->header.samples + x];
            break;
        case INT:
            ((int *)data)[i] = ((int *)idata)[i * new->header.samples + x];
            break;
        case FLOAT:
            ((float *)data)[i] = ((float *)idata)[i * new->header.samples + x];
            break;
        case DOUBLE:
            ((double *)data)[i] = ((double *)idata)[i * new->header.samples + x];
            break;
        }
    }
    return (make_PointData(new->header.bands, new->header.format, data));
}

void SetCube(Button B, XEvent *E)
{
    int i;
    char buf[2];
    char ptr[256];

    toggle_state(B, E);
    if (E->type == KeyPress) {
        char b[2];
        KeySym keysym;
        XComposeStatus status;

        XLookupString(&(E->xkey), b, 2, &keysym, &status);

        buf[0] = b[0];
        buf[1] = 0;

        i = buf[0] - 'A';
        if (i < 0 || i >= NIMAGE) {
            i = buf[0] - 'a';
            if (i < 0 || i >= NIMAGE) {
                return;
            }
        }
        SetCubeImage(i);
    } else {
        XfActivateButton(CubeControls, ExposureMask);
    }
    lr_y = -1;
}

void SetCubeImage(int i)
{
    char buf[2];

    if (Images[i] == NULL)
        return;

    CubeImage = i;

    sprintf(buf, "%c", 'A' + i);
    SetButtonText(CubeSelector, buf);
    set_delete_box_text(i);

    if (CubeImageFd >= 0) {
        close(CubeImageFd);
        CubeImageFd = -1;
    }
    /* If we are in channel space, resize the plot to fit the cube. */
    if (ChanSpace == 1) {
        set_plot_scale(1.0, (float)Images[i]->header.bands, PlotYMin, PlotYMax);
        RedrawAll();
    }
    EnableBlock();
}

void SetDelFactor(Button B, XEvent *E)
{
    char buf[1024];
    int len;
    int i = 0;
    int ptr = 0;
    char *V[20];
    char TR[20];
    int Index = 0;
    int Flag = 0;

    if (GetNewText(B, E, buf, 256, NULL) == -1) {
        if (Images[CubeImage]->Dranges != NULL) {
            free(Images[CubeImage]->Dranges);
            Images[CubeImage]->NumRanges = 0;
            Images[CubeImage]->Dranges = NULL;
        }
        set_delete_box_text((int)CubeImage);
        return;
    }

    len = strlen(buf);
    i = 0;
    V[Index] = (char *)calloc(64, sizeof(char));
    TR[Index] = 0;
    while (i < len) {
        if (buf[i] == ' ') {
            if (Flag) {
                V[Index][ptr] = '\0';
                Index++;
                ptr = 0;
                V[Index] = (char *)calloc(64, sizeof(char));
                TR[Index] = 0;
            }
            while (buf[i] == ' ' && i < len) {
                i++;
            }
        }

        if (i < len) {
            if (isdigit(buf[i]) || buf[i] == 'e' || buf[i] == '.' || buf[i] == '-' || buf[i] == ':') {

                V[Index][ptr++] = buf[i];
                if (buf[i] == ':')
                    TR[Index] = ptr - 1;
                i++;
                Flag = 1;
            }

            else
                return;
        }
    }

    if (isdigit(buf[i - 1])) {
        V[Index][ptr] = '\0';
        Index++;
        ptr = 0;
    }

    set_delete_points(V, TR, Index, CubeImage);

    for (i = 0; i < Index; i++) {
        free(V[i]);
    }
}

void set_delete_box_text(int Ci)
{
    int i, Index;
    char buf[1024];
    char tmp[256];
    memset(buf, 0, 1024);

    if (Images[Ci]->Dranges != NULL) {
        Index = Images[Ci]->NumRanges;
        for (i = 0; i < Index; i++) {
            sprintf(tmp, "%.6g", Images[Ci]->Dranges[i].Start);
            strcpy(buf, tmp);
            if (Images[Ci]->Dranges[i].Start != Images[Ci]->Dranges[i].End) {
                strcat(buf, ":");
                sprintf(tmp, "%.6g", Images[Ci]->Dranges[i].End);
                strcat(buf, tmp);
            }
            strcat(buf, " ");
        }
    }

    else
        strcpy(buf, "<NONE>");

    SetButtonText(Cube_Deleted_Points, buf);
}

void set_delete_points(char **V, char *TR, int Index, int Ci)
{

    int i;
    char buf[512];
    buf[0] = '\0';

    if ((!Index) || Ci < 0)
        return;

    if (Images[Ci]->Dranges != NULL) {
        free(Images[Ci]->Dranges);
    }

    Images[Ci]->Dranges = (DelRange *)calloc(Index, sizeof(DelRange)); /*New set of ranges */

    for (i = 0; i < Index; i++) {
        float tmp;
        if (TR[i]) {
            V[i][TR[i]] = '\0';
            Images[Ci]->Dranges[i].Start = atof(V[i]);
            Images[Ci]->Dranges[i].End = atof(&V[i][TR[i] + 1]);
            V[i][TR[i]] = ':';
        } else {
            Images[Ci]->Dranges[i].Start = atof(V[i]);
            Images[Ci]->Dranges[i].End = Images[Ci]->Dranges[i].Start;
        }
        if (Images[Ci]->Dranges[i].End < Images[Ci]->Dranges[i].Start) {
            tmp = Images[Ci]->Dranges[i].Start;
            Images[Ci]->Dranges[i].Start = Images[Ci]->Dranges[i].End;
            Images[Ci]->Dranges[i].End = tmp;
        }
    }

    Images[Ci]->NumRanges = Index;

    set_delete_box_text(Ci);
    /*
            for (i=0;i<Index;i++){
                    strcat(buf,V[i]);
                    strcat(buf," ");
            }
            buf[strlen(buf)-1]='\0';

            SetButtonText(Cube_Deleted_Points,buf);

            for (i=0;i<Index;i++){
                    printf("Index %d: Start:%48.42f  End:%48.42f\n",i,Images[Ci]->Dranges[i].Start,
                                                            Images[Ci]->Dranges[i].End);
            }

    */
}

void SetScaleFactor(Button B, XEvent *E)
{
    char buf[256];

    if (GetNewText(B, E, buf, 256, NULL) == -1)
        return;

    set_scale_factor(atof(buf));
    RedrawAll();
}

void set_scale_factor(float f)
{
    char buf[256];

    PlotScaleFactor = f;
    sprintf(buf, "%.6g", PlotScaleFactor);
    SetButtonText(Cube_Scale, buf);
}

void resize(Button B, XEvent *E)
{
    int width, height;

    if (E->type != ConfigureNotify) {
        RedrawAll();
        return;
    }
    width = E->xconfigure.width;
    height = E->xconfigure.height;

    if (PlotsWindow->States[0]->Visuals->width == width && PlotsWindow->States[0]->Visuals->height == height)
        return;

    do_resize(width, height);
}

void do_resize(int width, int height)
{
    XEvent E;
    PlotsWindow->States[0]->Visuals->width = width;
    PlotsWindow->States[0]->Visuals->height = height;

    PlotWidth = width - 150 - 10 - 40;
    PlotHeight = height - 5 - 65 - 30;

    ScaleCase->x = 150;
    XMoveWindow(ScaleCase->display, ScaleCase->window, ScaleCase->x, height - 60);

    ResizeButton(AxisCase, PlotCase->x - 40, PlotCase->y, PlotWidth + 40, PlotHeight + 25);
    ResizeButton(PlotCase, PlotCase->x, PlotCase->y, PlotWidth, PlotHeight);

    while (XCheckWindowEvent(PlotCase->display, PlotCase->window, ExposureMask, &E))
        ;
    while (XCheckWindowEvent(AxisCase->display, AxisCase->window, ExposureMask, &E))
        ;

    UpdateButton(PlotsWindow);
    UpdateButton(ScaleCase);
    UpdateButton(AxisCase);
    UpdateButton(PlotCase);
}

void RedrawAll(void)
{
    int i, j;
    int color;
    struct block_node *n;
    PointData *pdata;

    int Cnt, k, l, Range;
    PointData *tmpPacket;
    int Flag = 1;
    int np;
    double *pts;

    XSetForeground(PlotCase->display, DefaultGC(PlotCase->display, DefaultScreen(PlotCase->display)),
                   WHITE(PlotCase->display));
    XFillRectangle(PlotCase->display, PlotCase->window, DefaultGC(PlotCase->display, DefaultScreen(PlotCase->display)),
                   0, 0, PlotWidth, PlotHeight);
    DrawAxis();
    for (i = 0; i <= GetExtendedBlock(); i++) {
        if (GetBlockCount(i) != 0 && GetBlockState(i) == 1) {
            for (n = GetFirstBlock(i); n != NULL; n = n->next) {
                if (n->type == BK_LIBRARY) {
                    pdata = ConvertPoints(i, n->pdata, 1.0);
                } else {
                    pdata = ConvertPoints(i, n->pdata, PlotScaleFactor);
                }

                color = (n->color == -1 ? Pixels[i % 6] : n->color);
                DrawPoints(color, pdata->npoints, pdata->data);
            }
        }
    }
}

double YScale(float f)
{
    double d;

    d = (double)PlotHeight - (((double)f - (double)PlotYMin) * (double)PlotHeight / (double)(PlotYMax - PlotYMin));
    return (d);
}

double XScale(float f)
{
    double d;

    d = (((double)f - (double)PlotXMin) * (double)PlotWidth / (double)(PlotXMax - PlotXMin));
    return (d);
}

double YUnScale(int i) { return ((float)(PlotHeight - i) * (PlotYMax - PlotYMin) / (float)PlotHeight + PlotYMin); }

float XUnScale(int i) { return ((float)i * (PlotXMax - PlotXMin) / (float)PlotWidth + PlotXMin); }

/* Build XPoint list from block data (float samples, optional scale divisor). */
PointData *ConvertPoints(int block, PointData *pdata, float scale)
{
    XPoint *p;
    int i, j;
    int count = 0;
    double d;
    float x, y;
    int n = pdata->npoints;

    int Cnt;
    int Skip = 0;

    p = (XPoint *)malloc((unsigned int)sizeof(XPoint) * (n));
    if (Images[CubeImage]->Dranges != NULL)
        Cnt = Images[CubeImage]->NumRanges;
    else
        Cnt = -1;

    for (j = 0; j < n; j++) {
        if (block < 0 || ChanSpace == 1) {
            x = j + 1;
        } else if (waves[block] != NULL) {
            x = waves[block][j];
        } else if (def_waves != NULL) {
            x = def_waves[j];
        } else {
            x = j + 1;
        }
        //        y = get_PointData(pdata, j)/scale;
        y = get_PointData(pdata, j);
        Skip = 0;
        for (i = 0; (i < Cnt && !(Skip)); i++) {
            if ((Images[CubeImage]->Dranges[i].Start <= x && x <= Images[CubeImage]->Dranges[i].End) ||
                (Images[CubeImage]->Dranges[i].Start <= y && y <= Images[CubeImage]->Dranges[i].End))
                Skip = 1;
        }

        if (Skip)
            continue;
        else
            y /= scale;

        p[count].x = (short)XScale(x);
        p[count].y = (short)YScale(y);

        count++;
    }
    return (make_PointData(count, XPOINTS, p));
}

void DrawPoints(int color, int n, XPoint *p)
{
    extern Display *display;
    extern GC gc;
    XSetForeground(display, gc, color);
    XDrawLines(display, PlotCase->window, gc, p, n, CoordModeOrigin);
}

float zoom_list[10][4];
int zl_ptr = -1;
int zcount = -1;

void set_plot_scale(float x1, float x2, float y1, float y2)
{
    char buf[256];

    if (x1 < x2 && y1 < y2) {
        zcount = MIN(zcount + 1, 10);
        zl_ptr = (zl_ptr + 1) % 10;

        zoom_list[zl_ptr][0] = PlotXMin = x1;
        zoom_list[zl_ptr][1] = PlotXMax = x2;
        zoom_list[zl_ptr][2] = PlotYMin = y1;
        zoom_list[zl_ptr][3] = PlotYMax = y2;

        sprintf(buf, "%-.5g", x1);
        fixfloat(buf);
        SetButtonText(XMin, buf);

        sprintf(buf, "%-.5g", x2);
        fixfloat(buf);
        SetButtonText(XMax, buf);

        sprintf(buf, "%-.5g", y1);
        fixfloat(buf);
        SetButtonText(YMin, buf);

        sprintf(buf, "%-.5g", y2);
        fixfloat(buf);
        SetButtonText(YMax, buf);
    }
}

void UnZoom(Button B, XEvent *E)
{
    int z;
    if (zcount <= 0)
        return;

    z = (zl_ptr - 1) % 10;
    zl_ptr = (zl_ptr - 2) % 10;
    zcount -= 2;
    set_plot_scale(zoom_list[z][0], zoom_list[z][1], zoom_list[z][2], zoom_list[z][3]);
    RedrawAll();
}

void GetScale(Button B, XEvent *E)
{
    Display *display;
    XFontStruct *font;
    char buf[256], *s;
    double f;

    display = B->display;
    font = (B->States[0])->Visuals->visual.t_vis.font;

    s = xgets(display, B->window, 0, 0, B->width, B->height, WHITE(display), BLACK(display), font, NULL, E);

    /*
     * Do nothing if the user supplied the same value again.
     * Otherwise, update the buttons and redraw.
     * Note: This calls set_plot_scale, which updates all the buttons so we
     * can easily do that from the restart routines.
     */
    if (s != NULL && s[0] != 0) {
        f = atof(s);
        if (!strcmp(B->name, "XMin") && f != PlotXMin) {
            set_plot_scale(f, PlotXMax, PlotYMin, PlotYMax);
            RedrawAll();
        } else if (!strcmp(B->name, "XMax") && f != PlotXMax) {
            set_plot_scale(PlotXMin, f, PlotYMin, PlotYMax);
            RedrawAll();
        } else if (!strcmp(B->name, "YMin") && f != PlotYMin) {
            set_plot_scale(PlotXMin, PlotXMax, f, PlotYMax);
            RedrawAll();
        } else if (!strcmp(B->name, "YMax") && f != PlotYMax) {
            set_plot_scale(PlotXMin, PlotXMax, PlotYMin, f);
            RedrawAll();
        }
    }
}

void PlotCallback(Button B, XEvent *E)
{
    switch (E->type) {
    case Expose:
        RedrawAll();
        break;
    case MotionNotify:
        while (XCheckMaskEvent(E->xany.display, PointerMotionMask, E))
            ;
        if (E->xmotion.state & (Button1Mask | Button2Mask | Button3Mask)) {
            /* rubber band goes here */

            zoom(E->xmotion.x, E->xmotion.y, 1);
        }
        UpdatePlotReadout(E);
        break;
    case ButtonPress:
        zoom(E->xbutton.x, E->xbutton.y, 0);
        break;
    case ButtonRelease:
        /* perform zoom */
        zoom(E->xbutton.x, E->xbutton.y, 2);
        break;
    default:
        return;
    }
}

void UpdatePlotReadout(XEvent *E)
{
    int x, y;
    char buf[256];

    x = E->xmotion.x;
    y = E->xmotion.y;

    if (x < 0 || x > PlotWidth)
        return;
    if (y < 0 || y > PlotHeight)
        return;

    sprintf(buf, "%-.5g", XUnScale(x));
    fixfloat(buf);
    buf[6] = '\0';
    SetButtonText(PlotXPos, buf);

    sprintf(buf, "%.5g", YUnScale(y));
    fixfloat(buf);
    buf[6] = '\0';
    SetButtonText(PlotYPos, buf);
}

void fixfloat(char *str)
{
    char *p;
    p = strchr(str, ' ');
    if (p)
        *p = '\0';
}

void DrawAxis(void)
{
    float x_tick, y_tick, x_start, y_start;
    int i;
    float f, point;
    Display *display;
    char buf[256];
    int width;
    XFontStruct *fs;

    display = AxisCase->display;
    fs = AxisCase->States[0]->Visuals->visual.t_vis.font;

    UpdateButton(AxisCase);
    XSetForeground(display, DefaultGC(display, DefaultScreen(display)), BLACK(display));
    inice(PlotXMin, PlotXMax, (float)(PlotWidth / 50), &x_tick, &x_start);
    inice(PlotYMin, PlotYMax, (float)(PlotHeight / 50), &y_tick, &y_start);
    for (f = y_start; f < PlotYMax; f += y_tick) {
        point = (float)YScale(f);
        sprintf(buf, "%.5g", f);
        fixfloat(buf);
        XDrawLine(display, AxisCase->window, DefaultGC(display, DefaultScreen(display)), 30, (int)point, 40,
                  (int)point);

        width = XTextWidth(fs, buf, strlen(buf));

        XDrawString(display, AxisCase->window, DefaultGC(display, DefaultScreen(display)), (int)(20 - width / 2),
                    (int)(point + fs->max_bounds.ascent + 3), buf, strlen(buf));
    }
    for (f = x_start; f < PlotXMax; f += x_tick) {
        point = (float)XScale(f);
        XDrawLine(display, AxisCase->window, DefaultGC(display, DefaultScreen(display)), (int)point + 39,
                  (int)PlotHeight, (int)point + 39, (int)(PlotHeight + 10));
        sprintf(buf, "%.5g", f);
        fixfloat(buf);
        width = XTextWidth(fs, buf, strlen(buf));
        XDrawString(display, AxisCase->window, DefaultGC(display, DefaultScreen(display)), (int)(point + 40 - width),
                    (int)(PlotHeight + 20), buf, strlen(buf));
    }
    /* Minor ticks. */

    inice(PlotXMin, PlotXMax, (float)(PlotWidth / 60 * 5), &x_tick, &x_start);
    inice(PlotYMin, PlotYMax, (float)(PlotHeight / 60 * 5), &y_tick, &y_start);
    for (f = y_start; f < PlotYMax; f += y_tick) {
        point = (float)YScale(f);
        XDrawLine(display, AxisCase->window, DefaultGC(display, DefaultScreen(display)), 35, (int)point, 40,
                  (int)point);
    }
    for (f = x_start; f < PlotXMax; f += x_tick) {
        point = (float)XScale(f);
        XDrawLine(display, AxisCase->window, DefaultGC(display, DefaultScreen(display)), (int)point + 39,
                  (int)PlotHeight, (int)point + 39, (int)(PlotHeight + 5));
    }
}

void AxisCallback(Button B, XEvent *E)
{
    (*(B->updateCallback))(B, E);
    DrawAxis();
}

void ShiftScale(Button B, XEvent *E)
{
    float f;
    char buf[256];

    if (!strcmp(B->name, "X_Up")) {
        PlotXMax += (PlotXMax - PlotXMin);
        f = PlotXMax;
        B = XMax;
    } else if (!strcmp(B->name, "Y_Up")) {
        PlotYMax += (PlotYMax - PlotYMin);
        f = PlotYMax;
        B = YMax;
    } else if (!strcmp(B->name, "X_Down")) {
        PlotXMin -= (PlotXMax - PlotXMin);
        f = PlotXMin;
        B = XMin;
    } else if (!strcmp(B->name, "Y_Down")) {
        PlotYMin -= (PlotYMax - PlotYMin);
        f = PlotYMin;
        B = YMin;
    }
    sprintf(buf, "%.5g", f);
    fixfloat(buf);
    strcpy(B->States[0]->Visuals->visual.t_vis.text, buf);
    UpdateButton(B);
    RedrawAll();
}

void Toggle_Rotate(Button B, XEvent *E) {}

void DoCopy(Button B, XEvent *E) {}

void BlockMath(Button B, XEvent *E) {}

void ActivateSpecprWrite(Button B, XEvent *E)
{
    if (SpecprWriteMain == NULL) {
        CreateSpecprWritePopup(B->display, B->ext);
    }
    SetSpecprTitle();
    XfActivateButton(SpecprWriteMain, ExposureMask);
}

void SpecprWriteGetRecord(Button B, XEvent *E) {}

void SpecprWriteGetFilename(Button B, XEvent *E)
{
    char buf[256];
    if (GetText(B, E, buf, 256, strcmp(B->States[0]->Visuals->visual.t_vis.text, NO_FILENAME)) == -1)
        return;
    SetButtonText(B, buf);
}

void SpecprWriteGetTitle(Button B, XEvent *E)
{
    char buf[256];
    if (GetText(B, E, buf, 256, 1) == -1)
        return;
    SetButtonText(B, buf);
    Specpr_DoTitle = 0;
}

void SpecprGo(Button B, XEvent *E)
{
    struct block_node *n;
    float *avg;
    int i, j;
    struct _label *lbl;
    char *ptr;
    int npoints;
    char buf[256];
    int count;
    float scale = 1.0;
    int fd;
    char *title, ahist[1024], mhist[1024];
    char *blocks;

    ptr = GetButtonText(SpecprWriteFilename);
    if (!strcmp(ptr, NO_FILENAME)) {
        XBell(B->display, 50);
        return;
    }

    B->state = 1;
    UpdateButton(B);
    XFlush(B->display);

    if (SpecprWriteScaled->state == 0) {
        scale = PlotScaleFactor;
    }

    if ((i = GetCurrentBlock()) != -1) {
        n = GetFirstBlock(i);
        npoints = n->pdata->npoints;
        avg = (float *)calloc(sizeof(float), ((npoints + 255) / 256) * 256);
        fd = open(ptr, O_APPEND | O_CREAT | O_WRONLY, 0777);

        title = GetButtonText(SpecprWriteTitle);
        if (SpecprWriteType->state == 0) {
            /* Write average */
            count = GetBlockCount(GetCurrentBlock());
            for (; n != NULL; n = n->next) {
                for (j = 0; j < npoints; j++) {
                    avg[j] += get_PointData(n->pdata, j) / count;
                }
            }

            for (j = 0; j < npoints; j++) {
                if (SpecprOffSpectra)
                    avg[j] -= SpecprOffSpectra[j];
                if (SpecprMultSpectra)
                    avg[j] *= SpecprMultSpectra[j];
                avg[j] /= scale;
            }

            sprintf(ahist, "pw:%s, %d pixels avg", trim_filename(Images[CubeImage]->filename, 20), count);

            sprintf(mhist, "pw: %s, %d pixels avg", trim_filename(Images[CubeImage]->filename, 20), count);

            /* there was a bug here.  EncodeBlock() didn't do anything */

            lbl = make_header(".", npoints, -1, title, ahist, mhist);
            write_specpr(fd, -1, lbl, (char *)avg);
            free(lbl);
        } else {
            for (; n != NULL; n = n->next) {
                sprintf(ahist, "pw: %s pixel (%d,%d)", trim_filename(Images[CubeImage]->filename, 20), n->stack->y + 1,
                        n->stack->x + 1);
                sprintf(mhist, "pw: %s pixel (%d,%d)", trim_filename(Images[CubeImage]->filename, 20), n->stack->y + 1,
                        n->stack->x + 1);
                sprintf(buf, "%s pixel (%d,%d)", title, n->stack->y + 1, n->stack->x + 1);

                lbl = make_header(".", npoints, -1, buf, ahist, mhist);
                for (j = 0; j < npoints; j++) {
                    avg[j] = get_PointData(n->pdata, j);
                    if (SpecprOffSpectra)
                        avg[j] -= SpecprOffSpectra[j];
                    if (SpecprMultSpectra)
                        avg[j] *= SpecprMultSpectra[j];
                    avg[j] /= scale;
                }
                write_specpr(fd, -1, lbl, (char *)avg);
                free(lbl);
            }
        }
        close(fd);
        free(avg);
    }

    B->state = 0;
    UpdateButton(B);
}

void SpecprCancelWrite(Button B, XEvent *E)
{
    XfDeactivateButton(SpecprWriteMain);
    Specpr_DoTitle = 1;
    SetButtonState(SpecprWrite, 0);
}

void CreateSpecprWritePopup(Display *display, XFontStruct *font)
{
    Button B;
    int i, x, y, width, height, count, twidth;
    char *text;
    Window parent;
    Button b[1];
    Window win;

    parent = RootWindow(display, DefaultScreen(display));

    B = XfCreateButton(display, parent, 100, 100, 255, 140, 1, BLACK(display), "SpecprWrite", 1);
    XfAddButtonVisual(B, 0, XfCreateVisual(B, 0, 3, 0, 0, WHITE(display), WHITE(display), XfSolidVisual));
    XfActivateButton(B, ExposureMask);
    b[0] = B;
    win = B->window;
    SpecprWriteMain = B;

    x = 10;
    y = 15;
    width = 185;
    height = 20;

    MakeTextButton(display, win, x, y - 10, width, height, font, "Filename", 1);

    B = XfCreateButton(display, win, x, y, width, 20, 2, BLACK(display), "filename", 1);
    XfAddButtonVisual(
        B, 0, XfCreateVisual(B, 0, 7, 0, 0, BLACK(display), pwBackground.pixel, XfTextVisual, NO_FILENAME, font, 1));
    XfAddButtonCallback(B, 0, XF_CALLBACK(SpecprWriteGetFilename), NULL);
    XfActivateButton(B, KeyPressMask | ExposureMask | ButtonPressMask);
    SpecprWriteFilename = B;

    x += width + 10;

    MakeTextButton(display, win, x, y - 10, width, height, font, "Record", 1);

    B = XfCreateButton(display, win, x, y, 40, 20, 2, BLACK(display), "record", 1);
    XfAddButtonVisual(
        B, 0, XfCreateVisual(B, 0, 7, 0, 0, BLACK(display), pwBackground.pixel, XfTextVisual, "APPEND", font, 1));
    XfAddButtonCallback(B, 0, XF_CALLBACK(SpecprWriteGetRecord), NULL);
    XfActivateButton(B, KeyPressMask | ExposureMask | ButtonPressMask);
    SpecprWriteRecord = B;

    x = 10;
    y += 40;
    width = 185 + 10 + 40;
    height = 20;

    MakeTextButton(display, win, x, y - 10, width, height, font, "Title", 1);

    B = XfCreateButton(display, win, x, y, width, 20, 2, BLACK(display), "title", 1);
    XfAddButtonVisual(
        B, 0, XfCreateVisual(B, 0, 7, 0, 0, BLACK(display), pwBackground.pixel, XfTextVisual, NO_TITLE, font, 1));
    XfAddButtonCallback(B, 0, XF_CALLBACK(SpecprWriteGetTitle), NULL);
    XfActivateButton(B, KeyPressMask | ExposureMask | ButtonPressMask);
    SpecprWriteTitle = B;

    x = 10;
    y += 40;
    width = 50;
    text = "Scaling";

    MakeTextButton(display, win, x, y - 10, width, height, font, text, 0);

    B = XfCreateButton(display, win, x, y, width, 20, 2, BLACK(display), "write", 2);
    XfAddButtonVisual(B, 0,
                      XfCreateVisual(B, 0, 7, 0, 0, BLACK(display), WHITE(display), XfTextVisual, "SCALED", font, 0));
    XfAddButtonVisual(B, 1,
                      XfCreateVisual(B, 0, 7, 0, 0, BLACK(display), WHITE(display), XfTextVisual, "RAW", font, 0));
    XfAddButtonCallback(B, 0, XF_CALLBACK(toggle_SpecprScaled), NULL);
    XfAddButtonCallback(B, 1, XF_CALLBACK(toggle_SpecprScaled), NULL);
    XfActivateButton(B, ExposureMask | ButtonPressMask);
    SpecprWriteScaled = B;

    x += width + 10;
    text = "Write";

    MakeTextButton(display, win, x, y - 10, width, height, font, text, 0);

    B = XfCreateButton(display, win, x, y, 50, 20, 2, BLACK(display), "write", 2);
    XfAddButtonVisual(B, 0,
                      XfCreateVisual(B, 0, 7, 0, 0, BLACK(display), WHITE(display), XfTextVisual, "AVERAGE", font, 0));
    XfAddButtonVisual(B, 1,
                      XfCreateVisual(B, 0, 7, 0, 0, BLACK(display), WHITE(display), XfTextVisual, "ALL", font, 0));
    XfAddButtonCallback(B, 0, XF_CALLBACK(toggle_SpecprType), NULL);
    XfAddButtonCallback(B, 1, XF_CALLBACK(toggle_SpecprType), NULL);
    XfActivateButton(B, ExposureMask | ButtonPressMask);
    SpecprWriteType = B;

    x += width + 10;

    B = XfCreateButton(display, win, x, y, 30, 20, 2, BLACK(display), "go", 2);
    XfAddButtonVisual(B, 0, XfCreateVisual(B, 0, 7, 0, 0, BLACK(display), WHITE(display), XfTextVisual, "GO", font, 0));
    XfAddButtonVisual(B, 1, XfCreateVisual(B, 0, 7, 0, 0, WHITE(display), BLACK(display), XfTextVisual, "GO", font, 0));
    XfAddButtonCallback(B, 0, XF_CALLBACK(SpecprGo), NULL);
    XfActivateButton(B, ExposureMask | ButtonPressMask);
    SpecprWriteGo = B;

    x += width + 15;

    B = XfCreateButton(display, win, x, y, 50, 20, 2, BLACK(display), "cancel", 1);
    XfAddButtonVisual(B, 0,
                      XfCreateVisual(B, 0, 7, 0, 0, BLACK(display), WHITE(display), XfTextVisual, "CANCEL", font, 0));
    XfAddButtonCallback(B, 0, XF_CALLBACK(SpecprCancelWrite), NULL);
    XfActivateButton(B, ExposureMask | ButtonPressMask);
    SpecprWriteCancel = B;
}

/* Apply a specpr record to the plot either as waves or as Y data */

void ApplySpecpr(int itchan, float *data)
{
    int i;
    PointData *pdata;

    i = GetCurrentBlock();
    if (ApplyX) {
        ChanSpace = 0;
        SetButtonState(ChanSpaceButton, 0);
        if (i < 0) {
            if (def_waves != NULL)
                free(def_waves);
            def_waves = data;
            def_nwaves = itchan;
        } else {
            if (waves[i] != NULL)
                free(waves[i]);
            waves[i] = data;
            nwaves[i] = itchan;
        }
    } else {
        if (i < 0) {
            free(data);
            return;
        } else {
            pdata = make_PointData(itchan, FLOAT, data);
            SetBlockData(i, BK_LIBRARY, pdata, -1);
        }
    }
    RedrawAll();
}

void DeleteWaves(void)
{
    int i;

    i = GetCurrentBlock();

    if (i < 0) {
        if (def_waves != NULL)
            free(def_waves);
        def_waves = NULL;
        def_nwaves = -1;
    } else {
        if (waves[i] != NULL)
            free(waves[i]);
        waves[i] = NULL;
        nwaves[i] = -1;
    }
}

void AutoScale(Button B, XEvent *E)
{
    /*
     * Figure out the maximum extents of all the data in the plot.
     * Gotta check with waves for each
     */

    float xlo = FLT_MAX, xhi = -FLT_MAX, ylo = FLT_MAX, yhi = -FLT_MAX;
    struct block_node *n;
    float val, *w = NULL;
    int i, j, nw;
    char buf[256];

    toggle_state(B, E);

    for (i = 0; i < 6; i++) {
        if (waves[i] == NULL) {
            w = def_waves;
            nw = def_nwaves;
        } else {
            w = waves[i];
            nw = nwaves[i];
        }
        for (n = GetFirstBlock(i); n != NULL; n = n->next) {
            for (j = 0; j < n->pdata->npoints; j++) {
                if (n->type == BK_LIBRARY) {
                    val = get_PointData(n->pdata, j);
                } else {
                    val = get_PointData(n->pdata, j) / PlotScaleFactor;
                }

                if (val == -1.23e34)
                    continue;

                /* Check waves, or default to channels. */
                if (w != NULL) {
                    if (j < nw) {
                        if (w[j] == -1.23e34)
                            continue;
                        if (w[j] < xlo)
                            xlo = w[j];
                        if (w[j] > xhi)
                            xhi = w[j];
                    }
                } else {
                    if (j < xlo)
                        xlo = j;
                    if (j > xhi)
                        xhi = j;
                }

                if (val < ylo)
                    ylo = val;
                if (val > yhi)
                    yhi = val;
            }
        }
    }

    if (xlo == FLT_MAX || ylo == FLT_MAX) {
        toggle_state(B, E);
        return;
    }
    set_plot_scale(xlo, xhi, ylo, yhi);

    RedrawAll();
    toggle_state(B, E);
}

void BlockAverage(Button B, XEvent *E)
{
    struct block_node *n;
    float *avg, *sigma, *s1, *s2;
    int i, j, current;
    int npoints;
    int count;
    int ext;
    float v;
    PointData *pdata1, *pdata2, *pdata3;

    int Cnt, k;
    int *Modifier;
    float Val;
    int Skip;

    /* Calculate avg, sigma */
    i = current = GetCurrentBlock();

    if (current != -1 && current != GetExtendedBlock()) {
        if ((n = GetFirstBlock(current)) == NULL) {
            toggle_state(B, E);
            return;
        }
        npoints = n->pdata->npoints;
        avg = (float *)calloc(sizeof(float), npoints);
        sigma = (float *)calloc(sizeof(float), npoints);
        Modifier = (int *)calloc(sizeof(int), npoints);
        memset(Modifier, 0, npoints * sizeof(int));

        /* Get average */
        count = GetBlockCount(current);
        if (Images[CubeImage]->Dranges != NULL)
            Cnt = Images[CubeImage]->NumRanges;
        else
            Cnt = -1;

        for (; n != NULL; n = n->next) {
            for (j = 0; j < npoints; j++) {
                Skip = 1;
                Val = get_PointData(n->pdata, j);
                for (k = 0; (k < Cnt && Skip); k++) {
                    if (Images[CubeImage]->Dranges[k].Start <= Val && Val <= Images[CubeImage]->Dranges[k].End) {
                        Modifier[j]++;
                        Skip = 0;
                    }
                }
                if (Skip)
                    avg[j] += Val;
            }
        }

        for (j = 0; j < npoints; j++) {
            if ((count - Modifier[j]) == 0)
                avg[j] = 0;
            else
                avg[j] /= ((float)(count - Modifier[j]));
        }

        /* apply correction spectra */
        for (j = 0; j < npoints; j++) {
            if (SpecprOffSpectra)
                avg[j] = avg[j] - SpecprOffSpectra[j];
            if (SpecprMultSpectra)
                avg[j] = avg[j] * SpecprMultSpectra[j];
        }

        /* std deviation of the mean */
        if (count > 2) {
            n = GetFirstBlock(current);
            for (; n != NULL; n = n->next) {
                for (j = 0; j < npoints; j++) {
                    v = get_PointData(n->pdata, j);
                    Skip = 1;
                    for (k = 0; (k < Cnt && Skip); k++) {
                        if (Images[CubeImage]->Dranges[k].Start <= v && v <= Images[CubeImage]->Dranges[k].End) {
                            Skip = 0;
                        }
                    }
                    if (Skip) {
                        if (SpecprOffSpectra)
                            v = v - SpecprOffSpectra[j];
                        if (SpecprMultSpectra)
                            v = v * SpecprMultSpectra[j];
                        v = v - avg[j];
                        sigma[j] += v * v;
                    }
                }
            }
            for (j = 0; j < npoints; j++) {
                sigma[j] =
                    sqrt((double)(sigma[j] / (float)(count - 1 - Modifier[j]))) / sqrt((double)(count - Modifier[j]));
            }
        }

        s1 = (float *)calloc(npoints, sizeof(float));
        s2 = (float *)calloc(npoints, sizeof(float));

        for (i = 0; i < npoints; i++) {
            //                      avg[i] = avg[i];
            s1[i] = avg[i] - sigma[i];
            s2[i] = avg[i] + sigma[i];
        }
        free(sigma);
        free(Modifier);

        ext = GetExtendedBlock();
        pdata1 = make_PointData(npoints, FLOAT, s1);
        pdata2 = make_PointData(npoints, FLOAT, s2);
        pdata3 = make_PointData(npoints, FLOAT, avg);
        SetBlockData(ext, BK_AVG, pdata1, (int)pwBackground.pixel);
        AddBlockData(ext, BK_AVG, pdata2, (int)pwBackground.pixel);
        AddBlockData(ext, BK_AVG, pdata3, (int)BLACK(B->display));

        /* auto-set waves for this color here */
        SetExtendedWaves(current);

        RedrawAll();
    }
    toggle_state(B, E);
}

void SetSpecprTitle(void)
{
    Image new;
    char buf[256];
    int i;
    int count;

    i = GetCurrentBlock();
    if (CubeImage == -1 || i < 0 || !Specpr_DoTitle) {
        return;
    }

    new = Images[CubeImage];
    count = GetBlockCount(i);
    sprintf(buf, "pw:%s, ", trim_filename(new->filename, 20));

    if (SpecprWriteType->state == 0) { /* avg */
        sprintf(buf + strlen(buf), "%d pixels avg", count);
    }
    buf[40] = '\0';

    SetButtonText(SpecprWriteTitle, buf);
}

void toggle_SpecprType(Button B, XEvent *E)
{
    toggle_state(B, E);
    SetSpecprTitle();
}

void toggle_SpecprScaled(Button B, XEvent *E)
{
    toggle_state(B, E);
    SetSpecprTitle();
}

void ActivateBlockIDCallback(Button B, XEvent *E)
{
    ActivateBlockID(GetCurrentBlock());
    toggle_state(B, E);
}

void DoAutoCycle(void)
{
    Button b;
    int i = GetCurrentBlock();

    if (i != -1) {
        b = CBlocks[i];
        b->state = 0;
        UpdateButton(b);
    }
    i = (i + 1) % (GetNBlocks());
    b = CBlocks[i];
    b->state = 1;
    UpdateButton(b);
    SetCurrentBlock(i);
}

void ReadXCallback(Button B, XEvent *E)
{
    int i;
    ApplyX = 1;

    XSync(B->display, False);

    i = GetCurrentBlock();
    if (i == -1)
        i = GetNBlocks();

#ifdef INTERNAL_SP
    ActivateSP();
#else
    ActivateSP(SP_XFilenames[i]);
#endif
    toggle_state(B, E);
}

void ReadYCallback(Button B, XEvent *E)
{
    ApplyX = 0;
    XSync(B->display, False);

#ifdef INTERNAL_SP
    ActivateSP();
#else
    ActivateSP(NULL);
#endif
    toggle_state(B, E);
}

/* This is a hokey routine to copy waves[i] into the waves[ExtendedBlock] */
void SetExtendedWaves(int i)
{
    waves[GetExtendedBlock()] = waves[i];
    nwaves[GetExtendedBlock()] = nwaves[i];
}

/* This copies the extendedblock (average only) to the currently selected color */

void CopyExtendedBlock(Button B, XEvent *E)
{
    struct block_node *n;
    int current;
    float *fptr;
    PointData *pdata;

    if (GetBlockCount(GetExtendedBlock()) == 0) {
        toggle_state(B, E);
        return;
    }
    current = GetCurrentBlock();
    if (current < 0 || current == GetExtendedBlock()) {
        toggle_state(B, E);
        return;
    }

    delete_all(current);
    /* Get average (its third in the block, after 2 sigma's) */
    n = GetFirstBlock(GetExtendedBlock());
    while (n && n->next)
        n = n->next;

    pdata = copy_PointData(n->pdata);
    SetBlockData(current, BK_AVG, pdata, -1);
    delete_all(GetExtendedBlock());

    RedrawAll();

    toggle_state(B, E);
}

#ifndef INTERNAL_SP

ActivateSP(str) char *str;
{
    char buf[256];
    int i, fd;
    FILE *pfp;
    struct _label label;
    char *data;
    char *p;

    sprintf(buf, "sp -bg %lu -q", (unsigned long)pwBackground.pixel);
    i = GetCurrentBlock();
    if (i == -1)
        i = GetNBlocks();

    if (str) {
        sprintf(buf + strlen(buf), " %s", str);
    }

    pfp = popen(buf, "r");
    if (fgets(buf, 256, pfp) == NULL)
        return;
    pclose(pfp);

    if (!strcmp(buf, "CANCEL\n")) {
        return;
    } else if (!strcmp(buf, "NONE\n")) {
        DeleteWaves();
    } else {
        /* Copy name for posterity */
        if (SP_XFilenames[i])
            free(SP_XFilenames[i]);
        SP_XFilenames[i] = strdup(buf);
        /* find filename and load data */
        p = strchr(buf, '#');
        *p = '\0';
        if ((fd = open(buf, O_RDONLY)) < 0) {
            fprintf(stderr, "Error opening file: %s\n", buf);
            return;
        }
        fprintf(stderr, "%d, %d\n", fd, atoi(p + 1));
        read_specpr(fd, atoi(p + 1), &label, &data);
        ApplySpecpr(label.itchan, (float *)data);
    }
}

#endif

Button CreateCubeControls(Display *display, XFontStruct *fs)
{
    Window win;
    Button B;
    int x, y, width, height;
    int fg, bg;

    bg = BLACK(display);
    fg = WHITE(display);

    if (CubeControls != NULL)
        return (NULL);

    win = RootWindow(display, DefaultScreen(display));
    B = XfCreateButton(display, win, 275, 100, 450, 180, 1, BLACK(display), "Cube Controls", 1);
    XfAddButtonVisual(B, 0, XfCreateVisual(B, 0, 0, 0, 0, WHITE(display), WHITE(display), XfSolidVisual));
    CubeControls = B;
    win = B->window;

    /* Create labels */
    MakeTextButton(display, win, 100, 10, 150, 15, fs, "Cube Calibration Spectra", 1);

    MakeTextButton(display, win, 70, 45, 50, 15, fs, "Filename#rec", 1);
    MakeTextButton(display, win, 155, 25, 50, 15, fs, "Title", 1);

    /* Create spectra buttons */

    x = 70;
    y = 35;
    width = 80;
    height = 20;
    MakeTextButton(display, win, 5, y + 7, 60, 15, fs, "Multiplier", 2);
    B = XFCreateButton(display, win, x, y, width, height, 1, BLACK(display), WHITE(display), "MultFname", 2);
    XfAddButtonVisual(B, 0,
                      XfCreateVisual(B, 0, 7, 0, 0, BLACK(display), pwBackground.pixel, XfTextVisual, "<NONE>", fs, 1));
    XfAddButtonVisual(B, 1,
                      XfCreateVisual(B, 0, 7, 0, 0, WHITE(display), BLACK(display), XfTextVisual, "working", fs, 1));
    XfAddButtonCallback(B, 0, XF_CALLBACK(toggle_state), NULL);
    XfAddButtonCallback(B, 0, XF_CALLBACK(GetCubeFnames), NULL);
    XfActivateButton(B, ExposureMask | ButtonPressMask);
    Cube_MultFname = B;
    B->ext = NULL;
    B->member = PW_CAST_INT(1);

    x += width + 5;
    B = XFCreateButton(display, win, x, y, 250, height, 1, BLACK(display), WHITE(display), "MultTitle", 2);
    XfAddButtonVisual(B, 0,
                      XfCreateVisual(B, 0, 7, 0, 0, BLACK(display), pwBackground.pixel, XfTextVisual, "<NONE>", fs, 1));
    XfAddButtonVisual(B, 1,
                      XfCreateVisual(B, 0, 7, 0, 0, WHITE(display), BLACK(display), XfTextVisual, "working", fs, 1));
    XfAddButtonCallback(B, 0, XF_CALLBACK(toggle_state), NULL);
    XfAddButtonCallback(B, 0, XF_CALLBACK(GetCubeFnames), NULL);
    XfActivateButton(B, ExposureMask | ButtonPressMask);
    Cube_MultTitle = B;
    B->member = (char *)1;

    x = 70;
    y += height + 5;
    MakeTextButton(display, win, 5, y + 7, 60, 15, fs, "Offset", 2);
    B = XFCreateButton(display, win, x, y, width, height, 1, BLACK(display), WHITE(display), "OffsetFname", 2);
    XfAddButtonVisual(B, 0,
                      XfCreateVisual(B, 0, 7, 0, 0, BLACK(display), pwBackground.pixel, XfTextVisual, "<NONE>", fs, 1));
    XfAddButtonVisual(B, 1,
                      XfCreateVisual(B, 0, 7, 0, 0, WHITE(display), BLACK(display), XfTextVisual, "working", fs, 1));
    XfAddButtonCallback(B, 0, XF_CALLBACK(toggle_state), NULL);
    XfAddButtonCallback(B, 0, XF_CALLBACK(GetCubeFnames), NULL);
    XfActivateButton(B, ExposureMask | ButtonPressMask);
    Cube_OffFname = B;
    B->ext = NULL;
    B->member = (char *)2;

    x += width + 5;
    B = XFCreateButton(display, win, x, y, 250, height, 1, BLACK(display), WHITE(display), "OffsetTitle", 2);
    XfAddButtonVisual(B, 0,
                      XfCreateVisual(B, 0, 7, 0, 0, BLACK(display), pwBackground.pixel, XfTextVisual, "<NONE>", fs, 1));
    XfAddButtonVisual(B, 1,
                      XfCreateVisual(B, 0, 7, 0, 0, WHITE(display), BLACK(display), XfTextVisual, "working", fs, 1));
    XfAddButtonCallback(B, 0, XF_CALLBACK(toggle_state), NULL);
    XfAddButtonCallback(B, 0, XF_CALLBACK(GetCubeFnames), NULL);
    XfActivateButton(B, ExposureMask | ButtonPressMask);
    Cube_OffTitle = B;
    B->member = (char *)2;

    x = 70;
    y += height + 5;
    width = 40;
    MakeTextButton(display, win, 5, y + 7, 60, 15, fs, "Scale", 2);
    B = XFCreateButton(display, win, x, y, width, height, 1, BLACK(display), WHITE(display), "CubeScale", 1);
    XfAddButtonVisual(B, 0, XfCreateVisual(B, 0, 7, 0, 0, BLACK(display), WHITE(display), XfTextVisual, "1", fs, 1));
    XfAddButtonCallback(B, 0, XF_CALLBACK(SetScaleFactor), NULL);
    XfActivateButton(B, ExposureMask | ButtonPressMask | KeyPressMask);
    Cube_Scale = B;

    x = 70;
    y += height + 5;
    width = 180;
    MakeTextButton(display, win, x, y + 5, 140, 15, fs, "Deleted Point Value(s)", 2);
    B = XFCreateButton(display, win, x, y + 15, width, height, 1, BLACK(display), WHITE(display), "CubeDelete", 1);
    XfAddButtonVisual(B, 0,
                      XfCreateVisual(B, 0, 7, 0, 0, BLACK(display), WHITE(display), XfTextVisual, "<NONE>", fs, 1));
    XfAddButtonCallback(B, 0, XF_CALLBACK(SetDelFactor), NULL);
    XfActivateButton(B, ExposureMask | ButtonPressMask | KeyPressMask);
    Cube_Deleted_Points = B;

    B = Make2State3D(display, win, fs, x + 60 + 250 + 25 - 50, y, 50, 20, 1, bg, bg, fg, "DONE");
    XfAddButtonCallback(B, 0, XF_CALLBACK(DeactivateCubeControls), NULL);
    XfActivateButton(B, ExposureMask | ButtonPressMask);

    return (CubeControls);
}

void GetCubeFnames(Button B, XEvent *E)
{
    Button b1, b2, t1, t2;
    char *fname = NULL;
    char buf[256];
    char buf2[256];
    char *p;
    FILE *pfp;
    float **fptr;
    char *ptr;

    if (PW_CAST_PTR_INT(B->member) == 1) {
        b1 = Cube_MultFname;
        t1 = Cube_MultTitle;
        b2 = Cube_OffFname;
        fptr = &SpecprMultSpectra;
    } else {
        b1 = Cube_OffFname;
        t1 = Cube_OffTitle;
        b2 = Cube_MultFname;
        fptr = &SpecprOffSpectra;
    }

    /* setup command line */

    sprintf(buf, "sp -bg %lu -q", (unsigned long)pwBackground.pixel);
    /* get last filename from this button, or other if null. */
    fname = b1->ext;
    if (fname == NULL)
        fname = b2->ext;

    if (fname != NULL) {
        sprintf(buf + strlen(buf), " %s", fname);
    }

    /* Put button state back. */
    XFlush(b1->display);

    pfp = popen(buf, "r");
    if (fgets(buf, 256, pfp) == NULL) {
        toggle_state(B, E);
        return;
    }
    pclose(pfp);

    if ((p = strrchr(buf, '\n')) != NULL)
        *p = '\0';
    set_cube_correction(buf, b1, t1, fptr);

    /** pick up title, because it is given by sp */
    fgets(buf, 256, pfp);

    RedrawAll();
    toggle_state(B, E);
}

void set_cube_correction(char *buf, Button b1, Button t1, float **fptr)
{
    int fd;
    char buf2[256];
    char *ptr;
    struct _label label;
    int j;

    if (b1->ext)
        free(b1->ext);
    if (!strncmp(buf, "NONE", 4)) {
        b1->ext = NULL;
        SetButtonText(b1, "<NONE>");
        SetButtonText(t1, "<NONE>");
        free(*fptr);
        *fptr = NULL;
    } else {
        /* copy filename#rec, and get title too */

        b1->ext = strdup(buf);
        SetButtonText(b1, trim_filename(buf, 20));

        ptr = strrchr(buf, '#');
        *ptr = '\0';
        ptr++;

        if ((fd = open(buf, O_RDONLY)) < 0) {
            fprintf(stderr, "Unable to open specpr file: %s\n", buf);
        } else {
            j = read_specpr(fd, atoi(ptr), &label, (char **)fptr);
            if (j <= 0) {
                *fptr = NULL;
                fprintf(stderr, "Unable to access specpr record: %s#%s\n", buf, ptr);
            }
            close(fd);
            strncpy(buf2, label.ititl, 40);
            buf2[40] = '\0';
            SetButtonText(t1, buf2);
        }
    }
}

void DeactivateCubeControls(Button B, XEvent *E)
{
    toggle_state(B, E);
    XfDeactivateButton(CubeControls);
}

void SelectChanSpace(Button B, XEvent *E)
{
    ChanSpace = B->state;
    RedrawAll();
}

int zoom_anchor;
int zoom_anchor_x;
int zoom_anchor_y;
int zoom_a, zoom_b, zoom_w, zoom_h;

void zoom(int x, int y, int state)
{
    Button B = PlotCase;
    extern GC gc;

    if (state == 0) {
        zoom_anchor_x = x;
        zoom_anchor_y = y;
        zoom_a = x;
        zoom_b = y;
        zoom_w = zoom_h = 0;
        zoom_anchor = 1;
        XSetFunction(B->display, gc, GXxor);
        XSetForeground(B->display, gc, 255);
    } else if (state == 1) {
        /* undraw old one */
        if (zoom_anchor == 0)
            return;

        XDrawRectangle(B->display, B->window, gc, zoom_a, zoom_b, zoom_w, zoom_h);

        zoom_a = MIN(x, zoom_anchor_x);
        zoom_b = MIN(y, zoom_anchor_y);
        zoom_w = abs(x - zoom_anchor_x);
        zoom_h = abs(y - zoom_anchor_y);

        /* draw new one */
        XDrawRectangle(B->display, B->window, gc, zoom_a, zoom_b, zoom_w, zoom_h);
    } else {
        /* undraw old one */
        if (zoom_anchor == 0)
            return;
        XDrawRectangle(B->display, B->window, gc, zoom_a, zoom_b, zoom_w, zoom_h);
        XSetFunction(B->display, gc, GXcopy);
        set_plot_scale(XUnScale(MIN(x, zoom_anchor_x)), XUnScale(MAX(x, zoom_anchor_x)),
                       YUnScale(MAX(y, zoom_anchor_y)), YUnScale(MIN(y, zoom_anchor_y)));
        RedrawAll();
        zoom_anchor = 0;
    }
}
