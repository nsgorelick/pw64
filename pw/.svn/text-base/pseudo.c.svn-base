/*
 *
 * Convert N B&W planes to Color compsite
 *
 * Credit where due:
 *   Original version:   FullColor.c   by Alan Mazer
 *   Converted to VICAR: rtp.c         by Dan Stanfill
 *   stolen from VICAR:  pseudo.c      by Noel Gorelick
 *
 */

/*
 * 1) Using M bit array, scan image and count colors used in it.
 *    a) Each pixel scanned is kept with the color it is.
 *
 * 2) combine identical colors and sort by frequency
 *    a) Since we are using an association map, some colors can
 *       be duplicated, and need to be added togther.
 *
 * 3) Map colors to their nearest frequent neighbor
 * 4) Draw composite image
 */

#include <stdio.h>
#include <math.h>
#include "Xfred.h"
#include "pseudo.h"
#include "color.h"

#define RD 1
#define BL 2
#define GR 3

struct partition {
    int lr,lg,lb;
    int hr,hg,hb;
    int top, bot;
    int sorted;
    int count;
    struct partition *next;
};


int compare_freq(struct table_entry *a, struct table_entry *b);
int compare_red(struct table_entry *a, struct table_entry *b);
int compare_green(struct table_entry *a, struct table_entry *b);
int compare_blue(struct table_entry *a, struct table_entry *b);

int allocate_table (struct table_entry **table, int n_bits);
int allocate_pixels (struct pixel_entry **pixels, int length);
int build_table (unsigned char *red, unsigned char *green,
                 unsigned char *blue, struct table_entry *table,
                 struct pixel_entry *pixels, int length,
                 int n_pixels, unsigned char *red_lut,
                 unsigned char *green_lut, unsigned char *blue_lut,
                 int n_red, int n_blue, int n_green, int n_bits, char base);
int merge_partition (struct table_entry *table, int ncolors, int n_out, int npixels);
void make_composite (struct table_entry *table, unsigned char *out, unsigned char *out_map, int n_out, int size);
void make_lookups (struct table_entry *table, int n_out, unsigned char *red, unsigned char *green, unsigned char *blue);
void max_bounds (struct partition *node, struct table_entry *table);
void sort_by_node (struct partition *node, struct table_entry *table);
void make_split (struct partition *node, struct table_entry *table,
                 struct partition *a, struct partition *b);
void replace (struct partition *node, struct partition *a, struct partition *b);
int average_nodes (struct partition *node, struct table_entry *table, int ncolors);
void free_nodes (struct partition *node);

int
pseudocolor(unsigned char *red, unsigned char *green, unsigned char *blue, unsigned char *out, int n_red, int n_green, int n_blue, int n_bits, int n_out, unsigned char *red_lut, unsigned char *green_lut, unsigned char *blue_lut, unsigned char *out_map, char base, int size)
{
    struct table_entry *table;
    struct pixel_entry *pixels;
    int length, n_pixels;
    int ncolors;
    int max_error = 0;

    length =   allocate_table(&table, n_bits);
    n_pixels = allocate_pixels(&pixels, size);
    if (n_pixels == 0) {
        return(0);
    }

    ncolors = build_table(red, green, blue,
        table, pixels,
        length, n_pixels,
        red_lut, green_lut, blue_lut,
        n_red, n_blue, n_green,
        n_bits,
        base);

    if (n_out > ncolors) n_out = ncolors;

    /*
    qsort((char *)table, ncolors, sizeof(struct table_entry ), compare_freq);
    max_error = merge_frequency(table, ncolors, n_out);
    (void)printf("max error of %d\n", max_error);
    */

    n_out = merge_partition(table, ncolors, n_out, n_pixels);
    printf("using %d colors\n",n_out);

    printf("making composite image\n");
    make_composite(table, out, out_map, n_out, size);

    make_lookups(table, n_out, red_lut,green_lut,blue_lut);

    free((char *)table);
    free((char *)pixels);

    return(n_out);
}


/*
 * Allocate X*Y*Z table array.  X is ncolors of red rounded to next ^2
 */

allocate_table(struct table_entry **table, int n_bits)
{
    int length = ((1 << n_bits) *(1 << n_bits) *(1 << n_bits));
    *table = (struct table_entry *)calloc(sizeof(struct table_entry ),
        (unsigned int)length);
    if (*table == NULL) {
        (void)printf("Can not allocate table space (out of memory)\n");
        return 0;
    }
    (void)printf("allocated %d table entries\n",length);
    return length;
}


/*
 * Allocate size # pixels.
 */

allocate_pixels(struct pixel_entry **pixels, int length)
{
    *pixels = (struct pixel_entry *)calloc(sizeof(struct pixel_entry ),
        (unsigned int)length);
    if (*pixels == NULL) {
        (void)printf("Can not allocate pixels space (out of memory)\n");
        return 0;
    }
    return(length);
}


int
build_table(unsigned char *red, unsigned char *green, unsigned char *blue, 
            struct table_entry *table, struct pixel_entry *pixels, 
            int length, int n_pixels, 
            unsigned char *red_lut, 
            unsigned char *green_lut, 
            unsigned char *blue_lut, 
            int n_red, int n_blue, int n_green, int n_bits, char base)
{
    int i;
    int shift_red[256], shift_green[256], shift_blue[256];
    int color, source;
    int    r, g, b, next_entry;
    unsigned char  *red_ptr, *blue_ptr, *green_ptr;
    struct pixel_entry *next_pixel;
    struct table_entry *t;
    int red_mask,green_mask, blue_mask;
    float l;

/*
 * create lookup tables for shifts (faster this way)
 * (dont actually need to go to 256, but its cheaper)
 */
    l = (log((double)n_red)/log((double)2.0));
    red_mask =  ((l != floor(l)) ? (int)ceil(l) : (int) floor(l));
    red_mask -= n_bits;

    l = (log((double)n_green)/log((double)2.0));
    green_mask =  ((l != floor(l)) ? (int)ceil(l) : (int) floor(l));
    green_mask -= n_bits;

    l = (log((double)n_blue)/log((double)2.0));
    blue_mask =  ((l != floor(l)) ? (int)ceil(l) : (int) floor(l));
    blue_mask -= n_bits;


    /* changed, 9/17.  Was i< 256, but only n_pixels were allocated. NsG */
    /* 20091119: The previous comment was apparently wrong. */
    for (i = 0 ; i < 256 ; i++) {
        shift_red[i] = (red_lut[MAX(i-base,0)] >> red_mask) << (n_bits *2);
        shift_green[i] = (green_lut[MAX(i-base,0)] >> green_mask) << (n_bits);
        shift_blue[i] = (blue_lut[MAX(i-base,0)] >> blue_mask);
    }
/*
 * Initialize table
 * (how much of this is necesary)
 */

    for (r = 0 ; r < (1 << n_bits) ; r++) {
        for (g = 0 ; g < (1 << n_bits) ; g++) {
            for (b = 0 ; b < (1 << n_bits) ; b++) {
                i = (r << (n_bits*2)) | (g << (n_bits)) | b;
                (table + i)->color = i;
                (table + i)->r = r;
                (table + i)->g = g;
                (table + i)->b = b;
                (table + i)->last = &((table + i)->head);
                (table + i)->count = 0;
            }
        }
    }

/*
 * Decremented from start, 'cause of pre-autoincrement later on
 * (again, necessary?)
 */

    red_ptr = red - 1;
    green_ptr = green - 1;
    blue_ptr = blue - 1;
    next_pixel = pixels - 1;

    for (i = 0 ; i < n_pixels ; i++) {
        color = *(shift_red + (*++red_ptr)) |
            *(shift_green + (*++green_ptr)) |
            *(shift_blue + (*++blue_ptr));
        t= table + color;
        t->count++;
        (++next_pixel)->pixel = i;
        next_pixel->next = NULL;
        t->last->next = next_pixel;
        t->last = next_pixel;
    }

/*
 * compress table
 */
    source = 0;
    for (next_entry = 0 ; ; next_entry++) {
        while ((table + source)->count == 0 && source < length)
            source++;
        if (source == length)
            break;
        if (source != next_entry) {
            memcpy((char *)(table + next_entry),
                (char *)(table + source),
                sizeof(struct table_entry ));
        }
        source++;
    }
    (void)printf("found %d/%d colors\n", next_entry, length);

    return(next_entry);
}


merge_frequency(struct table_entry *table, int ncolors, int n_out)
{
    int square[513];
    register int    *square_zero;
    int i, j;
    int index, best_index;
    int max_error;
    int dist, best_dist = 0;
    int red_diff, blue_diff, green_diff;
    int red, blue, green;

/*
 * lookup table for squares (no jokes, please)
 */
    max_error = 0;
    if (ncolors < n_out) return(0);

    for (i = -256; i <= 256 ; i++) {
        square[i+256] = i * i;
    }
    square_zero = square + 256;

/*
 * Calcualte distance for every color, appending pixels to closest match
 */
    for (index = n_out ; index < ncolors ; index++) {
        red = (table + index)->r;
        green = (table + index)->g;
        blue = (table + index)->b;
        for (j = 0 ; j < n_out ; j++) {
            red_diff = red - (table+j)->r;
            green_diff = green - (table+j)->g;
            blue_diff = blue - (table+j)->b;
            dist =  *(square_zero + red_diff) +
                *(square_zero + green_diff) +
                *(square_zero + blue_diff);
            if (dist < best_dist || j == 0) {
                best_dist = dist;
                best_index = j;
            }
        }
        (table + best_index)->last->next = (table + index)->head.next;
        (table + best_index)->last = (table + index)->last;
        (table + best_index)->count += (table + index)->count;
        if (best_dist > max_error)
            max_error = best_dist;
    }
    return (max_error);
}
/*
 *  Local version of Heckbert's adaptive partitioning algorithm
 *  (partly stolen from fbquant.c R1.0 by Michael Mauldin but
 *   hideously optimised in the process.  No more recursion)
 *
 *  Not sure this works exactly the same:
 *    The original fbquant divided the colorspace into half, and then
 *    divided each half into N/2 colors.  This means that each half MUST
 *    have N/2 divisions in it.  We don't assume any such thing.
 */
struct partition *largest_node(struct partition *node);

int
merge_partition(struct table_entry *table, int ncolors, int n_out, int npixels)
{
    struct partition head,*largest,a,b;
    int i;
    /*
        If nout had not been set to be equal to ncolors, this would trigger
        us out of here to do no partitions.  For some reason it is making
        less than n_out partitons.
    */
    if (ncolors < n_out) {
        return(0);
    }
    head.top = 0; head.bot = ncolors-1;
    head.next = NULL;
    head.sorted = 0;
    head.count = npixels;

    max_bounds(&head,table);
    for (i = 0 ; i < n_out-1 ; i++) {
        largest = largest_node(&head);
        if (largest == NULL) {
            break;
        }
        sort_by_node(largest,table);
        make_split(largest,table,&a,&b);
        replace(largest,&a,&b);
        max_bounds(largest,table);
        max_bounds(largest->next,table);
    }
    i = average_nodes(&head,table,ncolors);
    free_nodes(head.next);
    return(i);
}
/*
 * Find max color boundaries
 */
void max_bounds(struct partition *node, struct table_entry *table)
{
    int i;

/*
 * if this node is sorted, then that saves us two comparisons in that
 * color.  Probably worth the extra code.
 */
 /*
    if (node->sorted) {
        switch(node->sorted) {
        case RD:
            {
                for (i = node->top+1 ; i < node->bot ; i++) {
                    if (node->lg > (table+i)->g) node->lg = (table+i)->g;
                    if (node->hg < (table+i)->g) node->hg = (table+i)->g;
                    if (node->lb > (table+i)->b) node->lb = (table+i)->b;
                    if (node->hb < (table+i)->b) node->hb = (table+i)->b;
                }
                node->lr = (table+node->top)->r;
                node->hr = (table+node->bot)->r;
                break;
            }
        case GR:
            {
                for (i = node->top+1 ; i < node->bot ; i++) {
                    if (node->lr > (table+i)->r) node->lr = (table+i)->r;
                    if (node->hr < (table+i)->r) node->hr = (table+i)->r;
                    if (node->lb > (table+i)->b) node->lb = (table+i)->b;
                    if (node->hb < (table+i)->b) node->hb = (table+i)->b;
                }
                node->lg = (table+node->top)->g;
                node->hg = (table+node->bot)->g;
                break;
            }
        case BL:
            {
                for (i = node->top+1 ; i < node->bot ; i++) {
                    if (node->lr > (table+i)->r) node->lr = (table+i)->r;
                    if (node->hr < (table+i)->r) node->hr = (table+i)->r;
                    if (node->lg > (table+i)->g) node->lg = (table+i)->g;
                    if (node->hg < (table+i)->g) node->hg = (table+i)->g;
                }
                node->lb = (table+node->top)->b;
                node->hb = (table+node->bot)->b;
                break;
            }
        }
    } else {
    */
        node->lr = node->hr = (table+node->top)->r;
        node->lg = node->hg = (table+node->top)->g;
        node->lb = node->hb = (table+node->top)->b;

        for (i = node->top+1 ; i <= node->bot ; i++) {
            if (node->lr > (table+i)->r) node->lr = (table+i)->r;
            if (node->hr < (table+i)->r) node->hr = (table+i)->r;
            if (node->lg > (table+i)->g) node->lg = (table+i)->g;
            if (node->hg < (table+i)->g) node->hg = (table+i)->g;
            if (node->lb > (table+i)->b) node->lb = (table+i)->b;
            if (node->hb < (table+i)->b) node->hb = (table+i)->b;
        }
/*
    }
*/
}
/*
 * Select node with largest difference in color
 * (orginal says: use variance instead of difference.  If I only knew how...)
 *
 * This routine should return NULL if all the partitions have a diff of 0.
 * (that should mean that we are partitioned out, so quit)
 */
struct partition *
largest_node(struct partition *node)
{
    struct partition *tmp,*largest=node;
    int rdiff,gdiff,bdiff;
    int max_diff;

    max_diff = 0;

    for (tmp = node ; tmp != NULL ; tmp = tmp->next) {
        rdiff = tmp->hr - tmp->lr;
        gdiff = tmp->hg - tmp->lg;
        bdiff = tmp->hb - tmp->lb;
        if (rdiff > max_diff) {
            largest = tmp;
            max_diff = rdiff;
        }
        if (gdiff > max_diff) {
            largest = tmp;
            max_diff = gdiff;
        }
        if (bdiff > max_diff) {
            largest = tmp;
            max_diff = bdiff;
        }
    }


    if (max_diff == 0) {
        return(NULL);
    }
    return(largest);
}
/*
 * Sort the entries defined by a node
 * (if already sorted in that color, dont bother.)
 */
void
sort_by_node(struct partition *node, struct table_entry *table)
{
    int rdiff,gdiff,bdiff;

    rdiff = node->hr - node->lr;
    gdiff = node->hg - node->lg;
    bdiff = node->hb - node->lb;

    if (rdiff >= gdiff && rdiff >= bdiff) {
        if (node->sorted != RD) {
            qsort((char *)(table+node->top), (node->bot - node->top +1),
                   sizeof(struct table_entry ), compare_red);
            node->sorted = RD;
        }
    } else if (gdiff >= rdiff && gdiff >= bdiff) {
        if (node->sorted != GR) {
            qsort((char *)(table+node->top), (node->bot - node->top +1),
                   sizeof(struct table_entry ), compare_green);
            node->sorted = GR;
        }
    } else {
        if (node->sorted != BL) {
            qsort((char *)(table+node->top), (node->bot - node->top +1),
                   sizeof(struct table_entry ), compare_blue);
            node->sorted = BL;
        }
    }
}
/*
 * Split node into a and b
 */
void
make_split(struct partition *node, struct table_entry *table,
           struct partition *a, struct partition *b)
{
    int count,diff;
    int min_diff,half ;
    int i;

    half = node->count/2;
    min_diff = half;
    count = 0;

/*
 * increment until we start getting larger diff
 */
    for (i = node->top ; i <= node->bot ; i++) {
        count += (table+i)->count;
        diff = abs(half - count);
        if (diff < min_diff) {
            min_diff = diff;
        } else {
            break;
        }
    }

    if (i != node->top) {
        count -= (table+i)->count;
        i--;
    }

    a->top = node->top;
    a->bot = i;
    if (i == node->bot) a->bot = i-1;

    b->top = a->bot + 1;
    b->bot = node->bot;
    a->count = count;
    b->count = node->count - count;
}
/*
 * replace node with a and b;
 * (use node's memory, and allocate one extra)
*/

void
replace(struct partition *node, struct partition *a, struct partition *b)
{
    struct partition *t;

    t = (struct partition *)malloc(sizeof(struct partition));

    node->top = a->top;
    node->bot = a->bot;
    t->top = b->top;
    t->bot = b->bot;
    t->sorted = node->sorted;
    t->count = b->count;
    t->next = node->next;
    node->next = t;
    node->count = a->count;
}

int
average_nodes(struct partition *node, struct table_entry *table, int ncolors)
{
    int r,g,b,i,j;
    int source, next_entry;
    struct partition *t;

    j = 0;
    while (node != NULL) {
        j++;
        r = (table +node->top)->r;
        g = (table +node->top)->g;
        b = (table +node->top)->b;


        for (i = node->top+1 ; i <= node->bot ; i++) {
            r += (table +i)->r;
            g += (table +i)->g;
            b += (table +i)->b;
            (table + node->top)->last->next = (table + i)->head.next;
            (table + node->top)->last = (table + i)->last;
            (table + node->top)->count += (table + i)->count;
            (table+i)->count = 0;
        }
        (table + node->top)->r = r/(node->bot - node->top +1);
        (table + node->top)->g = g/(node->bot - node->top +1);
        (table + node->top)->b = b/(node->bot - node->top +1);
        node = node->next;
    }
/*
 * compress table
 */
    source = 0;
    for (next_entry = 0 ; ; next_entry++) {
        while ((table + source)->count == 0 && source < ncolors)
            source++;
        if (source == ncolors)
            break;
        if (source != next_entry) {
            memcpy((char *)(table + next_entry),
                (char *)(table + source),
                sizeof(struct table_entry ));
        }
        source++;
    }
    return(next_entry);
}

void
free_nodes(struct partition *node)
{
    struct partition *n;

    while(node != NULL) {
        n = node;
        node = node->next;
        free(n);
    }
}

void
make_composite(struct table_entry *table, unsigned char *out, unsigned char *out_map, int n_out, int size)
{
    int i;
    int count=0;
    struct pixel_entry *tmp_ptr;

    memset(out, 0, size);
    for (i = 0 ; i < n_out ; i++) {
        if (i%10==0 && i) {
            printf("\n%d",++count);
        }
        printf("+");
        fflush(stdout);
        tmp_ptr = (table + i)->head.next;
        while (tmp_ptr != NULL) {
            *(out + tmp_ptr->pixel) = out_map[i];
            tmp_ptr = tmp_ptr->next;
        }
    }
    printf("\n");
}


void
make_lookups(struct table_entry *table, int n_out, unsigned char *red, unsigned char *green, unsigned char *blue)
{
    int i;
    int j;
    for (i = 0 ; i < n_out ; i++)  {
        red[i] = (table + i)->r;
        green[i] = (table + i)->g;
        blue[i] = (table + i)->b;
    }

    for (i = 0 ; i < 10 ; i++) {
        j = (n_out-1)*i/9;
        (void)printf("color %d : (%d %d %d) %d pixels\n",
                j+1,red[j],green[j],blue[j],(table+j)->count);
    }
}


int
compare_freq(struct table_entry *a, struct table_entry *b)
{
    return (b->count - a->count);
}
int
compare_red(struct table_entry *a, struct table_entry *b)
{
    return (b->r - a->r);
}
int
compare_green(struct table_entry *a, struct table_entry *b)
{
    return (b->g - a->g);
}
int
compare_blue(struct table_entry *a, struct table_entry *b)
{
    return (b->b - a->b);
}
