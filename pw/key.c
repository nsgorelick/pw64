
CreateColorKey(display, font) Display *display;
XFontStruct *font;
{
    Window parent;

    parent = RootWindow(display, DefaultScreen(display));

    if (ColorKeyMain == NULL) {
        ColorKeyMain = XfCreateButton(display, parent, 10, 110, 200, 50, 1, BLACK(display), "ColorKey", 1);
    } else {
        for (i = 0; i < NKeys; i++) {
            DestroyColorKey(i);
        }
    }
}
