#ifndef _COMPOSITE_FIXES_H
#define _COMPOSITE_FIXES_H

/* Function prototypes for all functions defined in composite.c */
int is_file(char *filename);
int UpdatePushButtons(Requestor R, int n);
int off_load_filename(int i, char *buf, Requestor R, int stretch, int colors, int band);
char *trim_filename(char *s, int n);
Image allocate_image(void);
int free_image(Display *display, Image *new);
int load_info(Image new, Requestor R);
int create_image(Display *display, Image new);
int create_hist(Display *display, Image new, int size, int type, int scale);
int make_hist_freq(Image new, int *C, int size, int scale_in);
int make_hist_dist(Image new, int *C, int size, int scale_in);
int overlay(char *data, int start, struct quant_data *quant, int ncolors, int *map);
int update_display(Display *display, Image new);
int load_fast_mem(Display *display, Image new);
int default_map(Image new);
int create_pan(Display *display, Image new);
int set_no_header(Image new);
int GetNewText(Button B, XEvent *E, char *s, int n, char *str);
int ButtonHilite(Button B, unsigned int fg, unsigned int bg);
int FlagForLoad(Requestor R, int i);
int UnFlagForLoad(Requestor R, int i);
int compute_subset(Requestor R, int i);
int SetUserMsg(char *buf);

#endif /* _COMPOSITE_FIXES_H */ 