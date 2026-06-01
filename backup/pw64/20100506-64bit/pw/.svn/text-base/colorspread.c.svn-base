#include <X11/Xos.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "color.h"


extern int RGBToXColor (RGB r, XColor *x);

RGB_CS(Display *display, Colormap CMap,
       XColor *start, XColor *end,
       int ncolors, XColor *colors, int rgb, int *map)
  /*
   * Spread colors from <start> to <end> in <ncolors> steps.
   * resulting color values are stored in <colors> array.
   * rgb specifies rgb <1> space or hsv <0> space.
   */
{
  int i;
  float v, n;
  RGB s,e,t;

  s = XColorToRGB(start);
  e = XColorToRGB(end);

  n = ncolors -1;

  for (i = 0 ; i < ncolors ; i++)
  {
    v = map[i];
    if (rgb)
      t = MixRGB(e,(v/n),s,((n-v)/n));
    else
      t = MixHSV(e,(v/n),s,((n-v)/n));
    RGBToXColor(t,&colors[i]);
  }
  XStoreColors(display, CMap, colors, ncolors);
}

CreateCSData(Display *display, int ncolors,
             XColor *colors, int width, int height, char *CSData)
  /*
   * Create ColorSpread Image data: Image is <res> pixels wide.
   */
{
  int i, j, k;

  for (i = 0 ; i < width ; i++) {
    k = i*ncolors/width;
    for (j = 0 ; j < height ; j++) {
      CSData[i+j*width] = colors[k].pixel;
    }
  }
}
