#include <stdio.h>
#include <fcntl.h>
#include <X11/Xos.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "Xfred.h"
#include "image.h"
#include "color.h"
#include "vicar.h"

extern XColor Colors[];
extern int NoAutoStretch;

char *lastzip = NULL;


FILE *LoadHeader(char *filename, struct _iheader *header);
int is_compressed(FILE * fp);
FILE * uncompress(FILE * fp, char *fname);
void * read_qube_data(int fd, struct _iheader *h);
void header2iheader(struct vicar_header *h, struct _iheader *iheader);


extern int SetUserMsg (char *buf);
extern int pseudocolor (unsigned char *red, unsigned char *green, unsigned char *blue, unsigned char *out, int n_red, int n_green, int n_blue, int n_bits, int n_out, unsigned char *red_lut, unsigned char *green_lut, unsigned char *blue_lut, unsigned char *out_map, char base, int size);

  double 
get_data(Image new, int i)
{
  double d;
  switch (new->header.format) {
    case BYTE:
      d = ((unsigned char *)new->data)[i];
      break; 
    case SHORT:
      d = ((short *)new->data)[i];
      break; 
    case INT:
      d = ((int *)new->data)[i];
      break; 
    case FLOAT:
      d = ((float *)new->data)[i];
      break; 
    case DOUBLE:
      d = ((double *)new->data)[i];
      break; 
  }
  return(d);
}

  struct vicar_header *
get_image_header(Image new)
{
  FILE *fp;
  struct vicar_header d;
  struct vicar_header *h = &d;
  struct _iheader iheader;
  int i;

  memset(&d, 0, sizeof(struct vicar_header));

  if ((fp = LoadHeader(new->filename, &iheader)) == NULL) return(NULL);
  fclose(fp);

  header2iheader(&d, &iheader);
  new->iheader = iheader;

  h = (struct vicar_header *)malloc(sizeof(struct vicar_header));
  memcpy(h, &d, sizeof(struct vicar_header));
  return(h);
}


  void
header2iheader(struct vicar_header *h, struct _iheader *iheader)
{
  int i;

  h->label_size = iheader->dptr;   /* label size */
  h->lines = 		GetLines(iheader->size, iheader->org);
  h->samples = 	GetSamples(iheader->size, iheader->org);
  h->bands = 		GetBands(iheader->size, iheader->org);

  switch (iheader->org) {        		/* set organization of cube */
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

  h->format = iheader->format;
  h->bits = NBYTES(h->format)/8;
}

/**
 ** get_image_data() - 
 **     Get the actual data for an image that has had its header filled.
 **/
  void *
get_image_data(Image new)
{
  int fd;
  double d;
  int i, len;
  int width,height;
  char buf[256];
  struct _iheader h;
  FILE *fp;

  /**
   ** We are going to (erroniously) assume that the user has
   ** specified any appropriate subset, and that the iheader struct
   ** is properly filled in.
   **/

  if ((fp = fopen(new->filename,"r")) == NULL) {
    (void)fprintf(stderr, "Can't open: %s\n",new->filename);
    return(NULL);
  }
  if (is_compressed(fp)) {
    fp = uncompress(fp, new->filename);
  }
  h = new->iheader;

  /**
   ** Apply the specified subset to the existing header
   **/

  /*	h.s_lo[orders[h.org][0]] = new->subset.sample+1;
        h.s_lo[orders[h.org][1]] = new->subset.line+1;	
        h.s_hi[orders[h.org][0]] = new->subset.sample + new->subset.width;
        h.s_hi[orders[h.org][1]] = new->subset.line + new->subset.height;***ORIGINAL***/

  h.s_lo[orders[h.org][0]] = ((new->subset.sample) ? (new->subset.sample): (1));
  h.s_lo[orders[h.org][1]] = ((new->subset.line) ? (new->subset.line): (1));
  h.s_lo[orders[h.org][2]] = new->band +1;

  h.s_hi[orders[h.org][0]] = ((new->subset.sample) ? (new->subset.sample + new->subset.uwidth-1) :
                              (new->subset.sample + new->subset.uwidth));

  h.s_hi[orders[h.org][1]] = ((new->subset.line) ? (new->subset.line + new->subset.uheight-1):
                              (new->subset.line + new->subset.uheight)); /***Modified 9/14/99**/

  h.s_hi[orders[h.org][2]] = new->band +1;

  h.s_skip[orders[h.org][0]] = new->subset.sskip+1;
  h.s_skip[orders[h.org][1]] = new->subset.lskip+1;
  h.s_skip[orders[h.org][2]] = 1;

  new->data = read_qube_data(fileno(fp), &h);
  fclose(fp);

  if(new->iheader.format==VAX_FLOAT) new->iheader.format=FLOAT;
  if(new->iheader.format==VAX_INTEGER) new->iheader.format=INT;
  if(new->header.format==VAX_FLOAT) new->header.format=FLOAT;
  if(new->header.format==VAX_INTEGER) new->header.format=INT;

  {
    char buf[256];
    sprintf(buf, "Read image: %s", new->filename);
    SetUserMsg(buf);
  }


  /* whats up with this? s_low should be d_low */

  if (new->s_low == new->s_high) {
    /*		len = (new->header.samples  * new->header.lines);   ***ORIGINAL****/
    len = (new->subset.width  * new->subset.height);  /*Modified 9/15/99*/
    new->s_low = get_data(new, 0);
    new->s_high = get_data(new, 0);

    /* this finds the min and max of the image */
    for (i = 1 ; i < len ; i++) {
      d = get_data(new, i);	
      new->s_low = min(new->s_low, d);
      new->s_high = max(new->s_high, d);
    }
  }
  
  return new->data;
}

/**
 ** stretch_gray() - convert the pdata data into something 
 ** 				 we can display in 8 bits or less
 **/

int
stretch_gray(Image new)
{
  int i, j, k, l;
  double low, high;

  int ncolors;
  int max_hist;
  int npixels;
  char *sdata;
  int *hist;

  double f;


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
  f = (double)ncolors / (double)(high - low);
  for (i = 0 ; i < npixels ; i++) {
    j = f * (get_data(new, i) - low);
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
  
  return(0);
}

int
stretch_color(Image new)
{
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
        return(0);
      }
      rgb[new->rgb[i]-1] = Images[i];
      nplanes++;
    }
  }
  if (nplanes != 3) {
    printf("3 planes only\n");
    return(0);
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
  /* Make sure lut array is allocated */
  if(lut[0] == NULL || lut[1] == NULL || lut[2] == NULL) {
    printf("Error allocating lut arrays\n");
    return(0);
  }

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
  
  return(0);
}
