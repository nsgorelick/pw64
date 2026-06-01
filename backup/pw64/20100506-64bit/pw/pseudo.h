#ifndef PSEUDO_H
#define PSEUDO_H

struct pixel_entry {
    int pixel;
    struct pixel_entry *next;
};

struct table_entry {
    unsigned char r;
	unsigned char g;
	unsigned char b;
    unsigned int color;
    int count;
    struct pixel_entry head;
    struct pixel_entry *last;
};

#endif 
