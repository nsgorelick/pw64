#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>


/* BSQ: Band sequential.   
        Read an entire plane at once, then parse down to subset 
 */

void *
read_bsq(int fd, int x, int y, int z, int label, int a, int b, int c, int w, int l, int d, int skip_x, int skip_y, int skip_z, int size, int order, int *omax, int *omin, int verbose)
                /* File descriptor */
                   /* size of cube */
                    /* Initial offset */
                    /* width, length and depth of subset */
                                /* skip factors */
                /* number of bytes per pixel */
                /* reverse byte order? */
                 
            
{
    register int    i, j, k;
    void    *data;
    short   *sdata;
	int		*idata;
    unsigned char   *cbuf;
    unsigned char   *cdata;
    int width, length, depth;
    int plane;
    int side;
    int pos;
    int min, max;
    char buf[64];

    max = 0;
    min = 0xFFFF;

    if (skip_x == 0) skip_x = 1;
    if (skip_y == 0) skip_y = 1;
    if (skip_z == 0) skip_z = 1;

    if (w == 0 || a+w > x)  w = x - a;
    if (l == 0 || b+l > y)  l = y - b;
    if (d == 0 || c+d > z)  d = z - c;

    depth = c + d;
    length = b + l ;
    width = a + w;

/*
 *  Side has to be WIDTH*length because we must read all the way across
 *  to get the whole plane.
 */

    side = x * y * size;
    plane = x * l * size;
    cbuf = (unsigned char *)malloc(plane);

    data = (void *)malloc(w * l * d * size / skip_x / skip_y);
    cdata = (unsigned char *)data;
    sdata = (short *)data;
	idata = (int *)data;

    pos = 0;
    for (k = c ; k < depth ; k += skip_z) {
        if (verbose) write(verbose, ".", 1);
        (void)lseek(fd, (label + side * k + b * x * size), 0);
        if (!read(fd, cbuf, plane)) {
            if (verbose) {
                sprintf(buf, "Premature EOF\n");
                write(verbose, buf, strlen(buf));
            }
        } else {
            if (size == 1) {
                for (j = 0 ; j < l ; j += skip_y) {
                    for (i = a ; i < width; i += skip_x) {
                        cdata[pos] = cbuf[j*x + i];
                        if (cdata[pos] > (unsigned char)max) max = cdata[pos];
                        if (cdata[pos] < (unsigned char)min) min = cdata[pos];
                        pos++;
                    }
                }
            } else if (size == 2) {
                for (j = 0 ; j < l ; j += skip_y) {
                    for (i = a ; i < width ; i += skip_x) {
                        sdata[pos] = ((short)(cbuf[j*x*2 + i*2+order]) << 8) + 
                            ((short)cbuf[j*x*2 + i*2+(1-order)]);
                        if (sdata[pos] != -32767) {
                            if (sdata[pos] > (short)max) max= sdata[pos];
                            if (sdata[pos] < (short)min) min= sdata[pos];
                        }
                        pos++;
                    }
                }
            } else if (size == 4) {
                for (j = 0 ; j < l ; j += skip_y) {
                    for (i = a ; i < width ; i += skip_x) {
                        idata[pos] = ((int *)cbuf)[j*x + i];
                        if (idata[pos] != -32767) {
                            if (idata[pos] > (int)max) max= idata[pos];
                            if (idata[pos] < (int)min) min= idata[pos];
                        }
                        pos++;
                    }
                }
            }
        }
    }
    free(cbuf);
    *omax = (short)max;
    *omin = (short)min;
    if (verbose) write(verbose, "\n", 1);
    return((void * )data);
}

/* BIL: Band by line.   
        
 */

void *
read_bil(int fd, int x, int y, int z, int label, int a, int b, int c, int w, int l, int d, int skip_x, int skip_y, int skip_z, int size, int order, int *omax, int *omin, int verbose)
                /* File descriptor */
                   /* size of cube */
                    /* Initial offset */
                    /* width, length and depth of subset */
                                /* skip factors */
                /* number of bytes per pixel */
                /* reverse byte order? */
                 
            
{
    register int    i, j, k;
    short   *sdata;
    short   *data;
    unsigned char   *cbuf;
    unsigned char   *cdata;
    int width, length, depth;
    int plane;
    int side;
    int pos;
    int min, max;
    char buf[64];

    max = 0;    /* its an int...  this is cool */
    min = 0xFFFF;

    if (w == 0) 
        w = x - a;
    if (l == 0) 
        l = y - b;
    if (d == 0) 
        d = z - c;

    depth = c + d;
    length = b + l;
    width = a + w;

/*
 *  This has to be WIDTH*length because we must read all the way across
 *  to get the whole plane.
 */

    side = x * z * size;
    plane = x * d * size;
    cbuf = (unsigned char *)malloc(plane);

    data = (short *)malloc(w * l * d * size / skip_x / skip_y / skip_z);
    cdata = (unsigned char *)data;
    sdata = (short *)data;

    pos = 0;
    for (j = b ; j < length ; j += skip_y) {
        if (verbose) write(verbose, ".", 1);
        (void)lseek(fd, (label + side * j + c * x * size), 0);
        if (!read(fd, cbuf, plane)) {
            if (verbose) {
                sprintf(buf, "Premature EOF\n");
                write(verbose, buf, strlen(buf));
            }
        } else {
            if (size == 1) {
                for (k = 0 ; k < d ; k += skip_z) {
                    for (i = a ; i < width ; i += skip_x) {
                        cdata[pos] = cbuf[k*x + i];
                        if (cdata[pos] > (unsigned char)max) max = cdata[pos];
                        if (cdata[pos] < (unsigned char)min) min = cdata[pos];
                        pos++;
                    }
                }
            } else if (size == 2) {
                for (k = 0 ; k < d ; k += skip_z) {
                    for (i = a ; i < width ; i += skip_x) {
                        sdata[pos] = ((short)(cbuf[k*x*2 + i*2+order]) << 8) + 
                            ((short)cbuf[k*x*2 + i*2+(1-order)]);
                        if (sdata[pos] != -32767) {
                            if (sdata[pos] > (short)max) max= sdata[pos];
                            if (sdata[pos] < (short)min) min= sdata[pos];
                        }
                        pos++;
                    }
                }
            }
        }
    }
    free(cbuf);
    *omax = (short)max;
    *omin = (short)min;
    if (verbose) write(verbose, "\n", 1);
    return((void * )data);
}

/* BIP: Band by pixel.   
        
 */

void *
read_bip(int fd, int x, int y, int z, int label, int a, int b, int c, int w, int l, int d, int skip_x, int skip_y, int skip_z, int size, int order, int *omax, int *omin, int verbose)
                /* File descriptor */
                   /* size of cube */
                    /* Initial offset */
                    /* width, length and depth of subset */
                                /* skip factors */
                /* number of bytes per pixel */
                /* reverse byte order? */
                 
            
{
    register int    i, j, k;
    short   *sdata;
    short   *data;
    unsigned char   *cbuf;
    unsigned char   *cdata;
    int width, length, depth;
    int plane;
    int side;
    int pos;
    int min, max;
    char buf[64];

    max = 0;    /* its an int...  this is cool */
    min = 0xFFFF;

    if (w == 0) 
        w = x - a;
    if (l == 0) 
        l = y - b;
    if (d == 0) 
        d = z - c;

    depth = c + d;
    length = b + l;
    width = a + w;

    side = z * x * size;        /* how big is a side of cube */
    plane = z * w * size;       /* how big is side of cube we want */
    cbuf = (unsigned char *)malloc(plane);

    data = (short *)malloc(w * l * d * size / skip_x / skip_y / skip_z);
    cdata = (unsigned char *)data;
    sdata = (short *)data;

    pos = 0;
    for (j = b ; j < length ; j += skip_y) {
        if (verbose) write(verbose, ".", 1);
        (void)lseek(fd, (label + side * j + a * z * size), 0);
        if (!read(fd, cbuf, plane)) {
            if (verbose) {
                sprintf(buf, "Premature EOF\n");
                write(verbose, buf, strlen(buf));
            }
        } else {
            if (size == 1) {
                for (i = 0 ; i < w ; i += skip_x) {
                    for (k = c ; k < depth ; k += skip_z) {
                        cdata[pos] = cbuf[i*z + k];
                        if (cdata[pos] > (unsigned char)max) max = cdata[pos];
                        if (cdata[pos] < (unsigned char)min) min = cdata[pos];
                        pos++;
                    }
                }
            } else if (size == 2) {
                for (i = 0 ; i < w ; i += skip_x) {
                    for (k = c ; k < depth ; k += skip_z) {
                        sdata[pos] = ((short)(cbuf[i*z*2 + k*2+order]) << 8) + 
                            ((short)cbuf[i*z*2 + k*2+(1-order)]);
                        if (sdata[pos] != -32767) {
                            if (sdata[pos] > (short)max) max= sdata[pos];
                            if (sdata[pos] < (short)min) min= sdata[pos];
                        }
                        pos++;
                    }
                }
            }
        }
    }
    free(cbuf);
    *omax = (short)max;
    *omin = (short)min;
    if (verbose) write(verbose, "\n", 1);
    return((void * )data);
}
