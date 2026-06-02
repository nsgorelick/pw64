#include "image.h"

struct magnify {
    Display *display;
    Window window;
    Window parent;
    XImage *mag_image;
    char *mag_data; /* RGB tile at display depth */
    char *mag_slots; /* LUT slot indices (parallel to mag_data pixels) */
    int width, height; /* Width and height of Window */
    int _width, _height; /* rounded width and height (size of image) */
    int x, y; /* X and Y location of MagWindow (in dataspace) */
    int scale; /* Scale being used */
    float fact; /* Scale being used */
    char last_key; /* USed for [-]N scaling */
    Image image;
    int state;
} *CreateMagnify(Display *display, Window parent);

void SetMagnify(Display *display, Image new, struct magnify *Mag, int x, int y);
void SetMagnifyScale(struct magnify *Mag, XEvent *E);
