#define MakeTextButton(d, window, x, y, w, h, font, text, align)                                                       \
    {                                                                                                                  \
        Button TexTBuTTon;                                                                                             \
        TexTBuTTon = XfCreateButton(d, window, x, y, w, h, 0, WHITE(d), text, 1);                                      \
        XfAddButtonVisual(                                                                                             \
            TexTBuTTon, 0,                                                                                             \
            XfCreateVisual(TexTBuTTon, 0, 0, 0, 0, BLACK(d), WHITE(d), XfTextVisual, text, font, align));              \
        XfActivateButton(TexTBuTTon, ExposureMask);                                                                    \
    }

#define MakeTextButtonBg(d, window, x, y, w, h, bg, font, text, align)                                                 \
    {                                                                                                                  \
        Button TexTBuTTon;                                                                                             \
        TexTBuTTon = XfCreateButton(d, window, x, y, w, h, 0, WHITE(d), text, 1);                                      \
        XfAddButtonVisual(TexTBuTTon, 0,                                                                               \
                          XfCreateVisual(TexTBuTTon, 0, 0, 0, 0, BLACK(d), bg, XfTextVisual, text, font, align));      \
        XfActivateButton(TexTBuTTon, ExposureMask);                                                                    \
    }
