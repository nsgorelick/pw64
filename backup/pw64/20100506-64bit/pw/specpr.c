#include <stdio.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include <fcntl.h>
#include "specpr.h"


extern void *realloc (void *, size_t);
extern void free (void *);
extern char *getenv (const char *);
int julian_date (int *secs, int *date);
extern time_t time (time_t *);

char *
decode_time(int s, char *buf)
{
    int hour;
    int minutes;
    int sec;
    int tsec;

    
    tsec = s/24000;
    hour = tsec/3600;
    minutes = (tsec - hour*3600)/60;
	sec = tsec - hour*3600 - minutes*60;

    sprintf(buf, "%2.2d:%2.2d:%2.2d", hour, minutes, sec%60);
    buf[8] = '\0';
    return(buf);
}


char *
decode_date(int jday, char *buf)
{
    double ab,a,b,c,d,e,m,y,f;
    double day;
    int iday;

    day = (double)jday/10.0;
    iday = jday/10;
    jday = iday;

    f = day - (double)(iday) + 0.5;

    if (f >= 1.0){
        jday = jday + 1;
    }

    ab = floor((double)(jday/36524.25)-51.12264);
    a = (double)jday + 1.0 + ab - floor(ab/4.0);
    b = a + 1524.0;
    c = floor((b/365.25)-0.3343);
    d = floor(365.25*c);
    e = floor((b-d)/30.61);
    d = b - d - floor(30.61*e);
    m = e-1.0;
    y = c-4716.0;

    if (e > 13.5){
        m = m-12;
    }
    if (m < 2.5) {
        y=y+1;
    }
    if (y > 1900) y -= 1900;
    if (y < 0) y = 0;

    sprintf(buf, "%2.2d/%2.2d/%2.2d", (int) m, (int) d, (int) y);
    buf[8] = '\0';
    return(buf);
}

max_rec(int fd)
{
/*
    struct stat buf;
    stat(path, &buf);
*/
    int i;
    int f;
    f = dup(fd);
    i = lseek(f, 0, SEEK_END);
    close(f);
    return(i / LABELSIZE -1);
}

/* RED 03/15/2004 Added byteorder and checkendian routines derived             */
/*                from specpr code for byteswapping in the read_specpr and     */
/*                write_specpr routines if executing on little endian          */
/*                (i.e. Intel) system                                          */

int chkendian ()
{
  /* Return 0 if Big Endian (HPUX, SUN Solaris,..) */
  /* Return 1 then we are Little Endian (Intel)    */
  union
  {
    long l;
    char c[sizeof (long)];
  } u;
  u.l = 1;
  if (u.c[sizeof (long) - 1] == 1) return(0);
  return (1);
}

static char bytetemp;
typedef char *byteptr;
#define swap1byte(c1, c2) (bytetemp = (c1) , (c1) = (c2) , (c2) = bytetemp)
#define swap4byte(s)     (swap1byte(((byteptr)(s))[0], ((byteptr)(s))[3]), \
                         swap1byte(((byteptr)(s))[1], ((byteptr)(s))[2]),(s))
void
byteorder(a,iflag)
int *a,iflag;
{
        int itmp,i,j;

        itmp=0;

        if (iflag == 1) swap4byte(&((int *)a)[0]);  /* swap byte order on bitflags */
        i=(int)a[0];  /* RED Moved after possible above swap */

        if (check_bit(i,0) == 0 && check_bit(i,1) == 1) {
                /* 1st text data record */
                swap4byte(&((int *)a)[13]);  /* swap byte order on text pointer */
                swap4byte(&((int *)a)[14]);  /* swap byte order on text size    */

        } else if (check_bit(i,0) == 1 && check_bit(i,1) == 1) {
                /* Continuation text data record */
                        /* do something in the block but really nothing to do. */
                itmp =0;
        } else if (check_bit(i,0) == 0 && check_bit(i,1) == 0) {
                /* 1st data record */
                for (j = 13; j <= 28; j++) {
                        swap4byte(&((int *)a)[j]);
                }
                for (j = 118; j <= 383; j++) {
                        swap4byte(&((int *)a)[j]);
                }
        } else if (check_bit(i,0) == 1 && check_bit(i,1) == 0) {
                /* Continuation data record */
                for (j = 1; j <= 383; j++) {
                        swap4byte(&((int *)a)[j]);
                }
        }

        /* RED Corrected to following to refer to offset 0 */
        if (iflag == 2) swap4byte(&((int *)a)[0]);  /* swap byte order on bitflags */

        return;
}



read_record(int fd, int i, char *label)
{
    if (lseek(fd, LABELSIZE * i, 0) == -1) {
        /* some error */
        return(-1);
    }
    if (read(fd, label, LABELSIZE) != LABELSIZE) {
        return(-2);
    }
    if (chkendian()) {
        /* RED 03/15/2004                               */
        /* Running on little endian system (i.e. Intel) */
        /* Always assume input is big endian            */
        byteorder(label,1);
    }
    return(check_bit(((int*)label)[0], 0));
}


read_specpr(int fd, int i, struct _label *label, char **data)
{
    struct _label label2;
    int count = 0;
    int j;
    int size;
    struct _tlabel *tlabel;
    

    if ((j = read_record(fd, i, label)) == 0) {
        tlabel = (struct _tlabel *)label;
	/* RED Change following to include == 1 for consisentcy */
        if (check_bit(label->icflag,1) == 1) {
            /* text */
            size = 1476;
            *data = (char *)malloc(size);
            memcpy(*data,tlabel->itext, size);
        } else {
            size = 256*sizeof(float);
            *data = (char *)malloc(size);
            memcpy(*data, label->data, size);
        }
        while(read_record(fd, ++i, &label2)==1) {
            count++;
            *data = (char *)realloc(*data,(size+count*1532));
            memcpy(*data+(size+(count-1)*1532), ((char *)&label2)+4, 1532);
        }

        return(count+1);
    }
    return((j == 1 ? 0 : j));
}




write_record(int fd, int i, struct _label *label)
{
    if (i < 0) {
        if ((i = lseek(fd, 0, SEEK_END)) == 0) {
			char *p = (char *)malloc(LABELSIZE);
			memset(p, '\0', LABELSIZE);
			memcpy(p, SPECPR_STAMP, strlen(SPECPR_STAMP));
			write(fd, p, LABELSIZE);
			free(p);
            i = lseek(fd, 0, SEEK_END);
        }
        i /= LABELSIZE;
    } else if (lseek(fd, LABELSIZE * i, 0) == -1) {
	    /* some error */
	    return(-1);
    }

    if (check_bit(label->icflag, 0) == 0 && check_bit(label->icflag,1) == 0) {
        label->irecno = i;
    }

    if (chkendian()) {
        /* RED 03/15/2004                               */
        /* Running on little endian system (i.e. Intel) */
        /* Always assume input is big endian            */
        byteorder(label,2);
    }

    if (write(fd, label, LABELSIZE) != LABELSIZE) {
        return(-2);
    }
    return(1);
}


specpr_open(char *path)
{
    int fout;
    char *p;

    if ((fout = open(path, O_RDONLY, 0444)) >= 0) {
        close(fout);
        fout = open(path, O_WRONLY | O_CREAT | O_APPEND, 0444);
        return(fout);
    } else {
        /* it doesn't exist.  Create it. */

        fout = open(path, O_WRONLY | O_CREAT | O_APPEND, 0777);
        if (fout < 0) return(fout);

        p = (char *)malloc(LABELSIZE);
        memset(p, '\0', LABELSIZE);
	    memcpy(p, SPECPR_STAMP, strlen(SPECPR_STAMP));
        write(fout, p, LABELSIZE);
        free(p);
        return(fout);
    }
}

write_specpr(int fd, int i, struct _label *label, char *data)
{
    int type;
    int chans;
    struct _tlabel *tlabel;
    struct _tlabel tmpl;
    int size;
    int offset;
    int count;

    tlabel = (struct _tlabel *)label;
/* RED Zero the flag since cannot guarantee its init value */
    tmpl.icflag = 0;

    switch(check_bit(label->icflag,1)) {
        case 0:
            /* data */
            size = label->itchan*sizeof(float);
            offset = 256*sizeof(float);
            set_bit(tmpl.icflag,1,0);
            if (data != NULL) {
                memcpy(label->data, data, offset);
            }
            break;
        case 1:
            /* text */
            size = tlabel->itxtch;
            offset = 1476;
            set_bit(tmpl.icflag,1,1);
            if (data != NULL) {
                memcpy(tlabel->itext, data, offset);
            }
            break;
    }
    write_record(fd, i, label);

    count = 0;
    size -= offset;
    set_bit(tmpl.icflag,0,1);
    while(size > 0) {
        memcpy(((char *)&tmpl)+4, data+(offset+1532*count), 1532);
        write_record(fd, (i < 0 ? i : i+count+1), &tmpl);
        size -= 1532;
        count++;
    }
}

struct _label *
make_header(char *filename, int npixels, int waves, char *title, char *ahist, char *mhist)
{
    int i;
    struct _label label, *lbl;
    char *buf[256];
    char *p;

    p = strrchr(filename,'/');
    if (p == NULL) p = filename;
    else p++;
    
    label.icflag = 0;
    set_bit(label.icflag, 4, 1);
    set_bit(label.icflag, 5, 1);

    sprintf(label.usernm,"%s",getenv("USER"));
    julian_date(&label.iscta,&label.jdatea);
    label.iscta = label.isctb = label.iscta * 24000;
    label.jdateb = label.jdatea;
    label.istb = 0;
    label.isra = label.isdec = 0;
    label.itchan = npixels;
    label.irmas = label.revs = label.iband[0] = label.iband[1] = 1;
    label.irespt = label.itpntr = 0;
    label.siangl = label.seangl = label.sphase = 0;
    label.itimch = label.xnrm = label.scatin = label.timint = 1;
    label.tempd = 273;

    label.irwav = waves ;/* wavelengths pointer */

    label.irecno = 0; /* record number pointer */
    
    label.ihist[0] = '\0';
    label.mhist[0] = '\0';
    label.ititl[0] = '\0';

    if (title != NULL) 
        for (i = 0 ; i < 40 ; i++)
            label.ititl[i] = (i >= strlen(title) ? ' ' : title[i]);
    if (ahist != NULL) 
        for (i = 0 ; i < 60 ; i++)
            label.ihist[i] = (i >= strlen(ahist) ? ' ' : ahist[i]);
    if (mhist != NULL) 
        for (i = 0 ; i < 296 ; i++)
            label.mhist[i] = (i >= strlen(mhist) ? ' ' : mhist[i]);

    label.iwtrns = label.nruns = npixels; /* number of blocks averaged */
    label.data[0] = 0; /* averaged spectra data */

    lbl = (struct _label *)malloc(sizeof(struct _label));
    memcpy(lbl, &label, sizeof(struct _label));
    return(lbl);
}

julian_date(int *secs, int *date)
{
/* sets date to Julian day * 10 and time to secs since 0:00 hours UT */
    int jda, jsec, nday, isec;

    jda = 24405875;
    jsec = time(0);

    nday = jsec/(3600*24);
    isec = jsec - (nday*3600*24);
    jda = jda+nday*10;

    *date = jda;
    *secs = isec;
}

