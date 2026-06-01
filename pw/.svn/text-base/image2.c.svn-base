#include <stdio.h>
#include <fcntl.h>
#include <X11/Xos.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "Xfred.h"
#include "image.h"
#include "color.h"

#include "io.h" 

extern XColor Colors[];
extern int NoAutoStretch;

char *lastzip = NULL;



struct vicar_header *
get_image_header(new)
Image new;
{
	FILE *fp;
    struct vicar_header d;
    struct vicar_header *h = &d;
	struct _iheader iheader;
	int i;
	
    memset(&d, 0, sizeof(struct vicar_header));

	if ((fp = LoadHeader(new->filename, &iheader)) == NULL) return(NULL);
	fclose(fp);

	/**
	 ** Convert an iheader into a vicarheader
	 **/
	for (i = 0 ; i < 3 ; i++) {
		if (iheader.suffix[i]) {
			iheader.size[i] += (iheader.suffix[i]/NBYTES(iheader.format));
		}
	}

    h->label_size = iheader.dptr;   /* label size */
    h->lines = 		GetLines(iheader.size, iheader.org);
    h->samples = 	GetSamples(iheader.size, iheader.org);
    h->bands = 		GetBands(iheader.size, iheader.org);

    switch (iheader.org) {        		/* set organization of cube */
    case BSQ:
        h->org = VICAR_BSQ;
        break;
    case BIL: 
        h->org = VICAR_BIL;
        break;
    case BIP:
        h->org = VICAR_BIP;
        break;
    default: 
        break;
    }

	h->format = iheader.format;
	h->bits = NBYTES(h->format)/8;

	h = (struct vicar_header *)malloc(sizeof(struct vicar_header));
	memcpy(h, &d, sizeof(struct vicar_header));
	return(h);
}




/**
 ** get_image_data() - 
 **     Get the actual data for an image that has had its header filled.
 **/
short *
get_image_data(new)
Image new;
{
    int fd;
    short *idata;
    int min,max;
    int width,height;
	char buf[256];
	char tfname[32];
	struct _iheader h;

	/**
	 ** We are going to (erroniously) assume that the user has
	 ** specified any appropriate subset, and that the iheader struct
	 ** is properly filled in.
	 **/

    if ((fp = fopen(new->filename,"r")) == NULL) {
        (void)fprintf(stderr, "Can't open: %s\n",new->filename);
        return(False);
    }
	if (is_compressed(fp)) {
		fp = uncompress(fp, new->filename);
	}
	h = new->iheader;


	/**
	 ** Apply the specified subset to the existing header
	 **/

	h.s_lo[orders[h.org][0]] = new->subset.sample;
	h.s_lo[orders[h.org][1]] = new->subset.line;
	h.s_lo[orders[h.org][2]] = new->band;

	h.s_hi[orders[h.org][0]] = new->subset.sample + new->subset.width;
	h.s_hi[orders[h.org][1]] = new->subset.line + new->subset.height;
	h.s_hi[orders[h.org][2]] = new->band;

	h.s_skip[orders[h.org][0]] = new->subset.sskip;
	h.s_skip[orders[h.org][1]] = new->subset.lskip;
	h.s_skip[orders[h.org][2]] = 0;

	/**
	 ** Put it back?
	 **/
	*iheader = h;

	new->pdata = read_qube_data(fileno(fp), &h);
	fclose(fp);

	{
		char buf[256];
    	sprintf(buf, "Read image: %s", new->filename);
    	SetUserMsg(buf);
    }

	/**
	 ** ATTENTION!!!  new->min and new->max have NOT been set.
	 **/
}

/**
 ** stretch_gray() - convert the pdata data into something 
 ** 				 we can display in 8 bits or less
 **/

stretch_gray(new)
Image new;
{
    int i, j, k, l;

    int low, high, diff;

    int ncolors;
    int max_hist;
    int *table;
    int *table_off;
    int length;
    int npixels;

    char *sdata;
    int *hist;

    low = new->s_low;			/* lowest value when done */
    high = new->s_high;			/* highest value when done */
    ncolors = new->ncolors;
    npixels = new->subset.width * new->subset.height;

    if (new->sdata != NULL) {
        free((char *)new->sdata);
    }
    sdata = (char *)malloc((unsigned int)npixels);
    new->sdata = sdata;

    if (new->histogram != NULL) {
        free((char *)new->histogram);
    }
    hist = (int *)calloc(sizeof(int),(unsigned int)(ncolors));
    new->histogram = hist;

	/**
	 ** compute scaled values, and count up histogram
	 **/
	f = ncolors / (high - low);
	for (i = 0 ; i < npixels ; i++) {
		j = (pdata[i] - low) * f;
		if (j < 0) j = 0;
		if (j >= ncolors) j = ncolors-1;

		sdata[i] = (unsigned char)Colors[j+2].pixel;
		hist[j]++;
	}

    max_hist = 0;
    for (i = 0 ; i < ncolors ; i++) {
        max_hist = (hist[i] > max_hist ? hist[i] : max_hist);
    }
    new->max_hist = max_hist;
}

stretch_color(new)
Image new;
{
#if 0
    int nplanes,i,j,m,n;
    Image rgb[NIMAGE];
    char *out_map;
    char *lut[3];
    RGB start[3],end[3],final,comp,**colors;
    int size;
    

    nplanes = 0;
    for (i = 0 ; i < NIMAGE ; i++) {
        if (new->rgb[i]) { 
            if (Images[i] == NULL || Images[i]->sdata == NULL) {
                printf("image %d not valid\n",i+1);
                return 0;
            }
            rgb[new->rgb[i]-1] = Images[i];
            nplanes++;
        }
    }
    if (nplanes != 3) {
        printf("3 planes only\n");
        return;
    }
    memcpy(&(new->header), &(rgb[0]->header), sizeof(struct vicar_header));
    memcpy(&(new->subset), &(rgb[0]->subset), sizeof(struct _subset));

    if (new->sdata != NULL) {
        free((char *)new->sdata);
    }
    size = new->subset.width*new->subset.height;
    new->sdata = (char *) malloc(size);

/*
 * Load reverse lookup tables (the association maps)
 */
    for (i = 0 ; i < nplanes ; i++) {
        lut[i] = (char *) malloc(MAX(new->ncolors,rgb[i]->ncolors));
        if (rgb[i]->map == NULL) {
            for (j = 0 ; j < rgb[i]->ncolors ; j++) {
                lut[i][j] = j;
            }
        } else {
            for (j = 0 ; j < rgb[i]->ncolors ; j++) {
                lut[i][j] = rgb[i]->map[j];
            }
        }
    }
    lut[0];
    lut[1];
    lut[2];

    out_map = (char *) malloc(new->ncolors);
    for (i = 0 ; i < new->ncolors ; i++) {
        out_map[i] = Colors[i+2].pixel;
    }

    new->ncolors = pseudocolor(rgb[0]->sdata, rgb[1]->sdata, rgb[2]->sdata,   
                new->sdata, 
                rgb[0]->ncolors, rgb[1]->ncolors, rgb[2]->ncolors,
                new->band, new->ncolors,
                lut[0], lut[1], lut[2], out_map,
                (char)Colors[2].pixel, size);

    if (new->Colors != NULL) {
        free((char *)new->Colors);
    }

    colors = (RGB *(*))malloc(sizeof(RGB *)*nplanes);
    new->Colors = (XColor *)malloc(sizeof(XColor)*new->ncolors);

    for ( i = 0 ; i < nplanes ; i++) {
        end[i].r = 0xFFFF * (i == 0 ? 1 : 0);
        end[i].g = 0xFFFF * (i == 1 ? 1 : 0);
        end[i].b = 0xFFFF * (i == 2 ? 1 : 0);
        start[i].r = start[i].g = start[i].b = 0;
        colors[i] = (RGB *)malloc(sizeof(RGB)*rgb[i]->ncolors);
        n = rgb[i]->ncolors-1;
        for (j = 0 ; j < rgb[i]->ncolors ; j++) {
            colors[i][j] = MixRGB(end[i],((float)j/(float)n),
                          start[i],((float)(n-j)/(float)n));
        }
    }

    for (i = 0 ; i < new->ncolors ; i++) {
        final.r = final.g = final.b = 0;
        for (j = 0 ; j < nplanes; j++) {
            m = ((lut[j][i]) * (rgb[0]->ncolors >> new->band));
            comp = colors[j][m];
            final.r = MAX(0, MIN(MAX_INTENSITY, (int)(final.r + comp.r)));
            final.g = MAX(0, MIN(MAX_INTENSITY, (int)(final.g + comp.g)));
            final.b = MAX(0, MIN(MAX_INTENSITY, (int)(final.b + comp.b)));
        }
        new->Colors[i].red = final.r;
        new->Colors[i].green = final.g;
        new->Colors[i].blue = final.b;
        new->Colors[i].pixel = Colors[i+2].pixel;
        new->Colors[i].flags = DoRed | DoBlue | DoGreen;
    }
    new->color_offsets[0] = new->color_offsets[1] = new->color_offsets[2] = 0;
    free((char *)lut[0]);
    free((char *)lut[1]);
    free((char *)lut[2]);
    for (i = 0 ; i < nplanes ; i++) {
        free((char *)colors[i]);
    }
    free((char *)colors);

#endif
}

