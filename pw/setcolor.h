#define setcolor_lut(r, g, b, c)                                                                                       \
    do {                                                                                                               \
        (c).red = (r);                                                                                                 \
        (c).green = (g);                                                                                               \
        (c).blue = (b);                                                                                                \
        (c).flags = DoRed | DoGreen | DoBlue;                                                                          \
    } while (0)

#define setcolor(display, ColorMap, r, g, b, c)                                                                        \
    do {                                                                                                               \
        setcolor_lut(r, g, b, c);                                                                                      \
        XStoreColor((display), (ColorMap), &(c));                                                                      \
    } while (0)
