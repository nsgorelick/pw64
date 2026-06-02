#include "Xfred.h"
#include "image.h"
#include <X11/Xlib.h>
#include <X11/Xos.h>
#include <X11/Xutil.h>

struct ColorControls {
    Button RedVal; /* RGB Values (text output) */
    Button BlueVal;
    Button GreenVal;

    Button RedColor; /* RGB Colors (Color box) */
    Button BlueColor;
    Button GreenColor;

    Button MixBox; /* RGB Color boxes mixed */
    Button SliderSpace; /* RGB or HSV space for sliders */

    Slider RedSlider; /* RGB Sliders */
    Slider BlueSlider;
    Slider GreenSlider;

    XColor RedSliderColor;
    XColor GreenSliderColor;
    XColor BlueSliderColor;

    Button ColorSelection; /* Color selection menu (popup) */
    Button CSLow; /* Low Color Spread color box */
    Button Colorspread; /* Color Spread */
    Button CSHigh; /* High Color Spread color box */
    Button SpreadSpace; /* RGB or HSV spread */

    Button StretchLow, StretchHigh; /* actual image stretch values */
    Button ScrollLow, ScrollHigh; /* Image saturation values */

    XColor *C1, *C2;

    XColor *Spread; /* Pointer to pixels array for color spread */

    char *CSData; /* RGB color-spread widget buffer (4 bytes/pixel) */
    int CSWidth, CSHeight;

    /* AMap stuff */
    Slider Top;
    Slider Bottom;

    AMap Map;
    Button HistType;
    Button HistScale;
    /*    Button Modes[5];		****ORIGINAL*****/
    Button Modes[6]; /*Modified 9/10/99* */
    Button Readout;

    Colormap ColorMap;
    Display *display;

    Image image;
    int image_index;
};
