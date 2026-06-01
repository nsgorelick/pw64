#include "image.h"

struct magnify {
    Display *display;
    Window window;
    Window parent;
    XImage *mag_image;
    char *mag_data;
    int width,height;       /* Width and height of Window */
    int _width,_height;     /* rounded width and height (size of image) */
    int x,y;                /* X and Y location of MagWindow (in dataspace) */
    int scale;              /* Scale being used */
    float fact;             /* Scale being used */
    char last_key;          /* USed for [-]N scaling */
    Image image;
    int state;
} *CreateMagnify(Display *display, Window parent);

