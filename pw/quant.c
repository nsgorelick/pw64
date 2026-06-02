#include "quant.h"
#include "Xfred.h"

extern int allocate_pixels(struct pixel_entry **pixels, int length);

void quantize(char *data, int ncolors, int height, int width, struct quant_data **quant, int base)
{
    int i, j;
    int n_pixels;
    int count;
    int tcount;
    struct quant_entry *table, *t;
    struct pixel_entry *pixels, *next_pixel;
    struct quant_data *q;
    int *pixel_array;

    n_pixels = allocate_pixels(&pixels, height * width);
    table = (struct quant_entry *)malloc((unsigned int)ncolors * sizeof(struct quant_entry));

    for (i = 0; i < ncolors; i++) {
        (table + i)->last = &((table + i)->head);
        (table + i)->last->next = NULL;
        (table + i)->count = 0;
    }

    next_pixel = pixels;

    for (i = 0; i < n_pixels; i++) {
        t = (table + (unsigned char)data[i] - base);
        t->last->next = next_pixel;
        t->last = next_pixel;
        next_pixel->pixel = i;
        next_pixel->next = NULL;
        next_pixel++;
        t->count++;
    }

    *quant = (struct quant_data *)malloc(sizeof(struct quant_data) * ncolors);

    /*
     * This puts all the pixels for a color into an array.
     */

    pixel_array = (int *)malloc(sizeof(int) * n_pixels);

    q = *quant;
    tcount = 0;
    for (i = 0; i < ncolors; i++) {
        count = (q + i)->count = (table + i)->count;
        q[i].pixels = pixel_array + tcount;
        next_pixel = (table + i)->head.next;
        for (j = 0; j < count; j++) {
            q[i].pixels[j] = next_pixel->pixel;
            next_pixel = next_pixel->next;
        }
        tcount += count;
    }
    free(pixels);
    free(table);
}
