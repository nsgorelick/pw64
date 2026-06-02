#include "ColorControls.h"
#include "Xfred.h"
#include "bitmaps/bitmaps.h"
#include "block.h"
#include "composite.h"
#include "image.h"
#include "mag.h"
#include "specpr.h" /* Addition 9/10/99 */
#include "util.h"
#include <X11/keysym.h>
#include <fcntl.h>
#include <math.h>

#define NO_FILENAME "<no filename>"
#define NO_DIRECTORY "<no directory>"
#define NO_SUFFIX "<none>"

Button RestartRequestor = NULL;
Button RestartFilename;
Button AutoStretchFilename;
Button AutoStretchDirectory;

void GetSomeText(Button B, XEvent *E);
void ReadRestart(Button B, XEvent *E);
void SaveRestart(Button B, XEvent *E);

void DeactivateRestartRequestor(Button B, XEvent *E);

extern XColor pwBackground;
/* The following was all added: 9/10/99 */
extern char *SP_XFilenames[8];
extern float *waves[7];
extern float *def_waves;
extern int ApplyX;

void ApplySpecpr(int itchan, float *data);
void read_specpr(int fd, int i, struct _label *label, char **data);
int GetNBlocks(void);

extern int free_image(Display *display, Image *new);
extern int UpdatePushButtons(Requestor R, int n);
extern Image allocate_image(void);
extern FILE *LoadHeader(char *, struct _iheader *);
extern void set_delete_points(char **V, char *TR, int Index, int Ci);
extern int compute_subset(Requestor R, int i);
extern int create_image(Display *display, Image new);
extern int load_image(int i);
extern int stretch_color(Image new);
extern int create_pan(Display *display, Image new);
extern int update_display(Display *display, Image new);
extern void SetCubeImage(int i);
extern void set_plot_scale(float x1, float x2, float y1, float y2);
extern void set_cube_correction(char *buf, Button b1, Button t1, float **fptr);
extern void set_scale_factor(float f);
extern int SetCurrentBlock(int i);

void build_iheader(struct _iheader *ihead, struct vicar_header *head) {}

void CreateRestartRequestor(Display *display, XFontStruct *font)
{
    Window parent = RootWindow(display, DefaultScreen(display));
    Button B;
    int x, y, w, h;
    int fg, bg, gray;

    fg = BLACK(display);
    bg = WHITE(display);
    gray = pwBackground.pixel;

    B = XFCreateButton(display, parent, 10, 10, 250, 130, 1, bg, bg, "Restart", 1);
    parent = B->window;
    RestartRequestor = B;

    x = 10;
    y = 15;
    h = 20;
    w = 230;

    MakeTextButton(display, parent, x, y - 10, w, h, font, "Restart Filename", 1);
    B = XFCreateButton(display, parent, x, y, w, h, 1, fg, bg, "Restart Filename", 1);
    XfAddButtonVisual(B, 0, XfCreateVisual(B, 0, 7, 0, 0, fg, gray, XfTextVisual, NO_FILENAME, font, 1));
    XfAddButtonCallback(B, 0, XF_CALLBACK(GetSomeText), NULL);
    XfActivateButton(B, ExposureMask | ButtonPressMask | KeyPressMask);
    RestartFilename = B;

    y += 40;
    w -= 80;
    MakeTextButton(display, parent, x, y - 10, w, h, font, "Auto-Stretch Directory", 1);
    B = XFCreateButton(display, parent, x, y, w, h, 1, fg, bg, "AutoStretch", 1);
    XfAddButtonVisual(B, 0, XfCreateVisual(B, 0, 7, 0, 0, fg, gray, XfTextVisual, NO_DIRECTORY, font, 1));
    XfAddButtonCallback(B, 0, XF_CALLBACK(GetSomeText), NULL);
    XfActivateButton(B, ExposureMask | ButtonPressMask | KeyPressMask);
    AutoStretchDirectory = B;

    x += w + 5;
    w = 70;

    MakeTextButton(display, parent, x, y - 10, w, h, font, "Suffix", 1);
    B = XFCreateButton(display, parent, x, y, w, h, 1, fg, bg, "AutoStretch", 1);
    XfAddButtonVisual(B, 0, XfCreateVisual(B, 0, 7, 0, 0, fg, gray, XfTextVisual, NO_SUFFIX, font, 1));
    XfAddButtonCallback(B, 0, XF_CALLBACK(GetSomeText), NULL);
    XfActivateButton(B, ExposureMask | ButtonPressMask | KeyPressMask);
    AutoStretchFilename = B;

    w = 40;
    h = 20;
    y += 40;
    x = 10;

    B = Make2State3D(display, parent, font, x, y, w, h, 1, fg, fg, bg, "READ");
    XfAddButtonCallback(B, 0, XF_CALLBACK(ReadRestart), NULL);

    x += w + 10;
    B = Make2State3D(display, parent, font, x, y, w, h, 1, fg, fg, bg, "SAVE");
    XfAddButtonCallback(B, 0, XF_CALLBACK(SaveRestart), NULL);

    x += w + 10;
    Make2State3D(display, parent, font, x, y, 50, h, 1, fg, fg, bg, "STRETCH");

    x += w + 50;
    B = Make2State3D(display, parent, font, x, y, w, h, 1, fg, fg, bg, "DONE");
    XfAddButtonCallback(B, 0, XF_CALLBACK(DeactivateRestartRequestor), NULL);
}

void ActivateRestartRequestor(Button B, XEvent *E)
{
    extern XFontStruct *font;

    if (RestartRequestor == NULL)
        CreateRestartRequestor(B->display, font);

    if (RestartRequestor->state == 0)
        XfActivateButton(RestartRequestor, ExposureMask);
}

void DeactivateRestartRequestor(Button B, XEvent *E)
{
    XfDeactivateButton(RestartRequestor);
    toggle_state(B, E);
}

void GetSomeText(Button B, XEvent *E)
{
    char buf[256];
    if (GetText(B, E, buf, 256, 0) == -1)
        return;
    SetButtonText(B, buf);
}

/* This routine references the global CC to save the current images histogram. */

void write_restart(char *path)
{
    FILE *fp;
    int i;
    int j;
    Image new;
    char buf[256];
    char *p;
    extern struct ColorControls *CC;
    extern Button Cube_Deleted_Points;

    if ((fp = fopen(path, "w")) == NULL) {
        fprintf(stderr, "cannot open %s\n", path);
        return;
    }
    for (i = 0; i < NIMAGE; i++) {
        new = Images[i];
        if (new != NULL) {
            if (new->composite) {
                fprintf(fp, "FILE %s\n", "<composite>");
            } else {
                fprintf(fp, "FILE %s\n", new->filename);
            }
            fprintf(fp, "HEAD %d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n", new->header.label_size,
                    new->header.format, new->header.org, new->header.lines, new->header.samples, new->header.bands,
                    new->header.bits, new->byte_order, new->band, new->subset.sample, new->subset.line,
                    new->subset.width, new->subset.height, new->subset.sskip, new->subset.lskip);

            fprintf(fp, "IHEAD %d,%d,%d,%d,%d,", new->iheader.dptr, new->iheader.corner, new->iheader.byte_order,
                    new->iheader.format, new->iheader.org);
            fprintf(fp, "%d,%d,%d,", new->iheader.prefix[0], new->iheader.prefix[1], new->iheader.prefix[2]);
            fprintf(fp, "%d,%d,%d,", new->iheader.suffix[0], new->iheader.suffix[1], new->iheader.suffix[2]);
            fprintf(fp, "%d,%d,%d,", new->iheader.size[0], new->iheader.size[1], new->iheader.size[2]);
            fprintf(fp, "%d,%d,%d,", new->iheader.s_lo[0], new->iheader.s_lo[1], new->iheader.s_lo[2]);
            fprintf(fp, "%d,%d,%d,", new->iheader.s_hi[0], new->iheader.s_hi[1], new->iheader.s_hi[2]);
            fprintf(fp, "%d,%d,%d,", new->iheader.s_skip[0], new->iheader.s_skip[1], new->iheader.s_skip[2]);
            fprintf(fp, "%d,%d,%d,", new->iheader.dim[0], new->iheader.dim[1], new->iheader.dim[2]);
            fprintf(fp, "%f,%f\n", new->iheader.gain, new->iheader.offset);

            fprintf(fp, "HIST %d,%d\n", new->hist_type, new->hist_scale);
            fprintf(fp, "CUT %f,%f\n", new->c_low, new->c_high);
            fprintf(fp, "STRETCH %f,%f\n", new->s_low, new->s_high);
            fprintf(fp, "DATA %f,%f\n", new->d_low, new->d_high);
            fprintf(fp, "NCOLOR %d\n", new->ncolors);
            if (!new->composite) {
                fprintf(fp, "%d %d %d\n", new->C1.red, new->C1.blue, new->C1.green);
                fprintf(fp, "%d %d %d\n", new->C2.red, new->C2.blue, new->C2.green);
            }
            if (new == CC->image) {
                if (new->MapPhoto != NULL) {
                    free((char *)new->MapPhoto);
                }
                new->MapPhoto = XfPhotographAMap(CC->Map);
            }
            if (new->MapPhoto != NULL) {
                for (j = 0; j < 130; j++) {
                    if (new->MapPhoto[j].flag) {
                        fprintf(fp, "\t%d,%d\n", j, new->MapPhoto[j].v);
                    }
                }
            }
            fprintf(fp, "ENDMAP\n");
            if (new->composite) {
                for (j = 0; j < NIMAGE; j++) {
                    if (new->rgb[j]) {
                        fprintf(fp, "%c%d,", (new->rgb[j] == 1 ? 'R' : (new->rgb[j] == 2 ? 'G' : 'B')), j);
                    }
                }
                fprintf(fp, "\n");
            }

            for (j = 0; j < NIMAGE; j++) {
                if (new->overlays[j])
                    fprintf(fp, "overlay %d\n", j);
            }

            fprintf(fp, "DELETED:\n%d\n", new->NumRanges);
            if (new->NumRanges && new->Dranges != NULL) {
                for (j = 0; j < new->NumRanges; j++) {
                    fprintf(fp, "%8.6g %8.6g\n", new->Dranges[j].Start, new->Dranges[j].End);
                }
            }

            fprintf(fp, "SCALE %d\n", new->scale);

        } else {
            fprintf(fp, "0\n");
        }
    }
    /*
     * Write out non-image stuff here.
     * Spectral extraction image number, multplier and offset
     * plot scales
     * wavelengths?
     */
    {
        extern int CubeImage;
        extern Button Cube_MultFname;
        extern Button Cube_OffFname;
        extern float PlotXMin, PlotXMax;
        extern float PlotYMin, PlotYMax;
        extern float PlotScaleFactor;

        fprintf(fp, "Cube_Image=%d\n", CubeImage);

        fprintf(fp, "Plot_Scale=%f,%f,%f,%f\n", PlotXMin, PlotXMax, PlotYMin, PlotYMax);

        if (Cube_MultFname == NULL || Cube_MultFname->ext == NULL) {
            fprintf(fp, "Cube_Multiplier=NONE\n");
        } else {
            fprintf(fp, "Cube_Multiplier=%s\n", (char *)Cube_MultFname->ext);
        }

        if (Cube_OffFname == NULL || Cube_OffFname->ext == NULL) {
            fprintf(fp, "Cube_Offset=NONE\n");
        } else {
            fprintf(fp, "Cube_Offset=%s\n", (char *)Cube_OffFname->ext);
        }
        fprintf(fp, "Cube_ScaleFactor=%f\n", PlotScaleFactor);
        fprintf(fp, "WaveFiles: %d\n", ApplyX);
        for (i = 0; i <= GetNBlocks(); i++) {
            if (SP_XFilenames[i] == NULL || waves[i] == NULL) {
                fprintf(fp, "NULL\n");
            }

            else {
                fprintf(fp, "%s", SP_XFilenames[i]);
            }
        }

        if (SP_XFilenames[i] == NULL || def_waves == NULL) {
            fprintf(fp, "NULL\n");
        } else {
            fprintf(fp, "%s\n", SP_XFilenames[i]);
        }
    }

    fclose(fp);
}

void read_restart(Display *display, char *path)
{
    FILE *fp;
    int i;
    int j;
    Image new;
    char buf[256];
    char *p;
    char wname[256];
    int wnumber;
    extern Requestor requestor;
    extern struct ColorControls *CC;

    struct _label label;
    char *data;
    int fd;

    int color1, color2, color3;
    float c_low, c_high;
    float s_low, s_high;

    if ((fp = fopen(path, "r")) == NULL) {
        fprintf(stderr, "cannot open %s\n", path);
        return;
    }

    /* Added 9/17, CC->image was hanging onto a free'd image.  NSG */
    if (CC->image)
        CC->image = NULL;
    for (i = 0; i < NIMAGE; i++) {

        if (Images[i] != NULL) {
            free_image(display, &Images[i]);
        }

        fgets(buf, 256, fp);
        if (!strcmp(buf, "0\n")) {
            UpdateRequestor(requestor, i);
            UpdatePushButtons(requestor, i);
            continue;
        }
        Images[i] = (Image)allocate_image();
        new = Images[i];
        if (!strcmp(buf, "FILE <composite>\n")) {
            fprintf(stderr, "FILE <composite>\n");
            new->composite = 1;
        } else {
            *(p = strchr(buf, '\n')) = '\0';
            new->filename = strdup(buf + 5);
            new->composite = 0;
        }

        fgets(buf, 256, fp);

        sscanf(buf, "HEAD %d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n", &(new->header.label_size),
               &(new->header.format), &(new->header.org), &(new->header.lines), &(new->header.samples),
               &(new->header.bands), &(new->header.bits), &(new->byte_order), &(new->band), &(new->subset.sample),
               &(new->subset.line), &(new->subset.width), &(new->subset.height), &(new->subset.sskip),
               &(new->subset.lskip));

        fgets(buf, 256, fp);

        if ((sscanf(buf,
                    "IHEAD %d,%d,%d,%d,%d,%d,%d,"
                    "%d,%d,%d,%d,%d,%d,%d,%d,%d,"
                    "%d,%d,%d,%d,%d,%d,%d,%d,%d,"
                    "%d,%f,%f\n",
                    &new->iheader.dptr, &new->iheader.corner, &new->iheader.byte_order, &new->iheader.format,
                    &new->iheader.org, &new->iheader.prefix[0], &new->iheader.prefix[1], &new->iheader.prefix[2],
                    &new->iheader.suffix[0], &new->iheader.suffix[1], &new->iheader.suffix[2], &new->iheader.size[0],
                    &new->iheader.size[1], &new->iheader.size[2], &new->iheader.s_lo[0], &new->iheader.s_lo[1],
                    &new->iheader.s_lo[2], &new->iheader.s_hi[0], &new->iheader.s_hi[1], &new->iheader.s_hi[2],
                    &new->iheader.s_skip[0], &new->iheader.s_skip[1], &new->iheader.s_skip[2], &new->iheader.dim[0],
                    &new->iheader.dim[1], &new->iheader.dim[2], &new->iheader.gain, &new->iheader.offset)) == 28) {

            fgets(buf, 256, fp);
        }

        else {
            LoadHeader(new->filename, &new->iheader);
            build_iheader(&new->iheader, &new->header);
        }

        sscanf(buf, "HIST %d,%d\n", &new->hist_type, &new->hist_scale);

        fgets(buf, 256, fp);
        sscanf(buf, "CUT %f,%f\n", &c_low, &c_high);

        fgets(buf, 256, fp);
        sscanf(buf, "STRETCH %f,%f\n", &s_low, &s_high);

        fgets(buf, 256, fp);
        sscanf(buf, "DATA %lf,%lf\n", &new->d_low, &new->d_high);

        fgets(buf, 256, fp);
        sscanf(buf, "NCOLOR %d\n", &new->ncolors);

        if (!new->composite) {
            fgets(buf, 256, fp);
            sscanf(buf, "%d %d %d\n", &color1, &color2, &color3);
            new->C1.red = color1;
            new->C1.blue = color2;
            new->C1.green = color3;

            fgets(buf, 256, fp);
            sscanf(buf, "%d %d %d\n", &color1, &color2, &color3);
            new->C2.red = color1;
            new->C2.blue = color2;
            new->C2.green = color3;
        }
        fgets(buf, 256, fp);
        new->MapPhoto = NULL;
        if (strcmp(buf, "ENDMAP\n")) {
            int value;
            new->MapPhoto = (struct Point *)malloc(130 * sizeof(struct Point));
            memset(new->MapPhoto, '\0', 130 * sizeof(struct Point));

            while (strcmp(buf, "ENDMAP\n")) {
                sscanf(buf, "\t%d,%d\n", &j, &value);
                new->MapPhoto[j].v = value;
                new->MapPhoto[j].flag = 1;
                fgets(buf, 256, fp);
            }
        }
        fgets(buf, 256, fp);
        if (new->composite) {
            char c1, c2, c3;
            int v1, v2, v3;

            sscanf(buf, "%c%d,%c%d,%c%d,\n", &c1, &v1, &c2, &v2, &c3, &v3);
            new->rgb[v1] = (c1 == 'R' ? 1 : (c1 == 'G' ? 2 : 3));
            new->rgb[v2] = (c2 == 'R' ? 1 : (c2 == 'G' ? 2 : 3));
            new->rgb[v3] = (c3 == 'R' ? 1 : (c3 == 'G' ? 2 : 3));
            fgets(buf, 256, fp);
        }

        while (!strncmp(buf, "overlay", 7)) {
            sscanf(buf, "overlay %d\n", &j);
            new->overlays[j] = 1;
            fgets(buf, 256, fp);
        }

        if (!strcmp(buf, "DELETED:\n")) {
            char *V[20];
            char TR[20];
            char Start[64], End[64];
            int Index;
            fgets(buf, 256, fp);
            sscanf(buf, "%d\n", &Index);
            for (j = 0; j < Index; j++) {
                V[j] = (char *)calloc(64, sizeof(char));
                fgets(buf, 256, fp);
                sscanf(buf, "%63s %63s", Start, End);
                if (strcmp(Start, End)) {
                    TR[j] = strlen(Start);
                    strcat(V[j], Start);
                    strcat(V[j], ":");
                    strcat(V[j], End);
                } else {
                    strcat(V[j], Start);
                    TR[j] = 0;
                }
            }
            set_delete_points(V, TR, Index, i); // i= current cube;
            for (j = 0; j < Index; j++) {
                free(V[j]);
            }
            fgets(buf, 256, fp);
        }
        sscanf(buf, "SCALE %d\n", &new->scale);

        if (new->composite == 0) {
            compute_subset(requestor, i);
            get_image_data(new);
            new->c_low = c_low;
            new->c_high = c_high;
            new->s_low = s_low;
            new->s_high = s_high;
            create_image(display, new);
            load_image(i);
            UpdateRequestor(requestor, i);
            UpdatePushButtons(requestor, i);
            /*
              stretch_gray(new);
              create_hist(display,new,130,0,1);
              create_pan(display,new);
            */
        }
    }
    for (i = 0; i < NIMAGE; i++) {
        new = Images[i];
        if (new != NULL) {
            if (new->composite) {
                stretch_color(new);
                create_pan(display, new);
            }
            if (new->composite || new->data) {
                update_display(display, new);
            }
        }
    }
    {
        extern struct ColorControls *CC;

        CC->image = NULL;
        CC->image_index = -1;
    }

    {
        extern Button Cube_MultFname, Cube_MultTitle;
        extern Button Cube_OffFname, Cube_OffTitle;
        extern float *SpecprMultSpectra, *SpecprOffSpectra;
        float x1, x2, y1, y2, f;
        int i;

        char b2[256];
        while (fgets(buf, 256, fp) != NULL) {

            if (sscanf(buf, "Cube_Image=%d", &i) == 1 && i >= 0)
                SetCubeImage(i);

            else if (sscanf(buf, "Plot_Scale=%f,%f,%f,%f\n", &x1, &x2, &y1, &y2) == 4)
                set_plot_scale(x1, x2, y1, y2);

            else if (sscanf(buf, "Cube_Multiplier=%s\n", b2) == 1)
                set_cube_correction(b2, Cube_MultFname, Cube_MultTitle, &SpecprMultSpectra);

            else if (sscanf(buf, "Cube_Offset=%s\n", b2) == 1)
                set_cube_correction(b2, Cube_OffFname, Cube_OffTitle, &SpecprOffSpectra);

            else if (sscanf(buf, "Cube_ScaleFactor=%f\n", &f) == 1)
                set_scale_factor(f);

            else if (sscanf(buf, "WaveFiles: %d\n", &ApplyX) == 1) {
                for (i = 0; i <= GetNBlocks(); i++) {
                    SetCurrentBlock(i);
                    fgets(buf, 256, fp);
                    if (strcmp(buf, "NULL\n")) {
                        SP_XFilenames[i] = strdup(buf);
                        p = strchr(buf, '#');
                        *p = '\0';
                        if ((fd = open(buf, O_RDONLY)) < 0) {
                            fprintf(stderr, "Error opening file: %s\n", buf);
                            free(SP_XFilenames[i]);
                            SP_XFilenames[i] = NULL;
                        }
                        read_specpr(fd, atoi(p + 1), &label, &data);
                        ApplySpecpr(label.itchan, (float *)data);
                    }

                    else
                        SP_XFilenames[i] = NULL;
                }
            }
        }
    }
}

void SaveRestart(Button B, XEvent *E)
{
    char *ptr;

    B->state = 1;
    UpdateButton(B);

    ptr = GetButtonText(RestartFilename);

    if (strcmp(ptr, NO_FILENAME)) {
        write_restart(ptr);
    }

    B->state = 0;
    UpdateButton(B);
}

void ReadRestart(Button B, XEvent *E)
{
    extern Requestor requestor;
    char *ptr;
    int i;

    B->state = 1;
    UpdateButton(B);

    XFlush(B->display);
    ptr = GetButtonText(RestartFilename);

    if (strcmp(ptr, NO_FILENAME)) {
        read_restart(B->display, ptr);
        requestor->fnames->ext = 0;
        UpdateRequestor(requestor, 0);
        UpdatePushButtons(requestor, 0);
        compute_subset(requestor, 0);
        create_image(B->display, Images[0]);
        load_image(0);
    }

    B->state = 0;
    UpdateButton(B);
}

int Apply_AutoScale(Button B, XEvent *E)
{
    char *path, *suffix;
    char **dir;
    int ndir, count, i;

    path = GetButtonText(AutoStretchDirectory);
    if (!strcmp(path, NO_DIRECTORY) || is_dir(path) == 0) {
        XBell(B->display, 50);
        return (0);
    }

    suffix = GetButtonText(AutoStretchFilename);
    if (!strcmp(suffix, NO_SUFFIX)) {
        XBell(B->display, 50);
        return (0);
    }

    /*
     * Get directory listing.  Find any files that end in 'suffix'
     * Free everything else
     */
    ndir = get_sorted_dir(path, &dir);
    count = 0;
    for (i = 0; i < ndir; i++) {
        if (!strcmp(dir[i] + strlen(dir[i]) - strlen(suffix), suffix)) {
            dir[count++] = dir[i];
        } else {
            free(dir[i]);
        }
    }

    /*
     * For each match, see if there is a corresponding file loaded, and
     * apply if so.
     */
    if (count != 0) {
        for (i = 0; i < count; i++) {
        }
    }

    /* Free everything */
    for (i = 0; i < count; i++) {
        free(dir[i]);
    }
    free(dir);
    return 0;
}
