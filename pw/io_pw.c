#include <stdio.h>
#include <stdarg.h>
#include <pwd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "io.h"
#include "iomedley.h"

#define max(a,b) (a > b ? a : b)

/* This file serves as the interface between PW's I/O needs and
** the io_medley library which can serve those needs.
** The functions of the io_medley library are not called directly
** for reasons of code-retrofitting.  Minimal impact was made to the 
** the original PW code to use this library and only wrappers are necessary
** to integrate the two.  All functions in this file WERE original code
** functions used by PW in a file called: io_load.c.  They are now wrapper
** functions used to called the functions in io_medley.c
*/

void parse_error(char *fmt, ...);

int VERBOSE = 3;

int orders[3][3] = {
	{ 0,1,2 },
	{ 0,2,1 },
	{ 1,2,0 }
};
/*
char *FORMAT2STR[] = {
    0,
    "byte",
    "short",
    "int",
    "float",
    "vax float",
    "",
    "",
    "double"
};
 
char *ORG2STR[] = {
    "bsq",
    "bil",
    "bip"
};
*/

/*
FILE * iom_uncompress(FILE * fp, char *fname);
char * iom_expand_filename(char *s);
*/
char * expand_filename(char *s);

static char     vfmt[] = "FORMAT='BYTE'  TYPE='IMAGE'  BUFSIZ=%d  DIM=2\
  EOL=0  RECSIZE=%d  ORG='BSQ'  NL=%d  NS=%d  NB=1  N1=0  N2=0  N3=0\
  N4=0  NBB=0  NLB=0  TASK='conversion'  USER='%s'  DAT_TIM='%s'";


WriteVicarHeader(f, x, y)
    FILE           *f;
    int             x, y;
{

    char            tbuf[512];
    char            lbuf[20];
    long            c;
    int             i;
    int             label;

    c = time(0);
    sprintf(tbuf, vfmt,
        x * 2 ,   /* bufsize */
        x ,   /* recsize */
        y,      /* lines */
        x,      /* samples */
        getenv("USER"),
        ctime(&c));

    label = max(strlen(tbuf) + 20, x);
    sprintf(lbuf, "LBLSIZE=%d ", label);

    fwrite(lbuf, 1, strlen(lbuf), f);
    fwrite(tbuf, 1, strlen(tbuf), f);
    for (i = strlen(tbuf) + strlen(lbuf); i < label; i++) {
        fwrite(" ", 1, 1, f);
    }

}

/*
** iom_iheader.org <-> pw_ORG
*/
int
ihorg2vorg(int org)
{
    int vorder = -1;

    switch(org){
    case iom_BSQ: vorder = BSQ; break;
    case iom_BIL: vorder = BIL; break;
    case iom_BIP: vorder = BIP; break;
    }

    return vorder;
}

int
vorg2ihorg(int vorder)
{
    int org = -1;

    switch(vorder){
    case BSQ: org = iom_BSQ; break;
    case BIL: org = iom_BIL; break;
    case BIP: org = iom_BIP; break;
    }

    return org;
}

/*
** iom_iheader.format <-> pw_FORMAT
*/

int
ihfmt2vfmt(int ifmt)
{
    int vfmt = -1;

    switch(ifmt){
    case iom_BYTE:   vfmt = BYTE;   break;
    case iom_SHORT:  vfmt = SHORT;  break;
    case iom_INT:    vfmt = INT;    break;
    case iom_FLOAT:  vfmt = FLOAT;  break;
    case iom_DOUBLE: vfmt = DOUBLE; break;
        /* VAX_INT & VAX_FLOAT are deprecated */
    }

    return vfmt;
}

int
vfmt2ihfmt(int vfmt)
{
    int ifmt = -1;

    switch(vfmt){
    case BYTE:   ifmt = iom_BYTE;   break;
    case SHORT:  ifmt = iom_SHORT;  break;
    case INT:    ifmt = iom_INT;    break;
    case FLOAT:  ifmt = iom_FLOAT;  break;
    case DOUBLE: ifmt = iom_DOUBLE; break;
    }

    return ifmt;
}


void
iomheader2iheader(struct iom_iheader *iomh, struct _iheader *h)
{
    memset(h, 0, sizeof(struct _iheader));
    h->dptr = iomh->dptr;
    memcpy(h->prefix, iomh->prefix, sizeof(int)*3);
    memcpy(h->suffix, iomh->suffix, sizeof(int)*3);
    memcpy(h->size, iomh->size, sizeof(int)*3);
    memcpy(h->s_lo, iomh->s_lo, sizeof(int)*3);
    memcpy(h->s_hi, iomh->s_hi, sizeof(int)*3);
    memcpy(h->s_skip, iomh->s_skip, sizeof(int)*3);
    memcpy(h->dim, iomh->dim, sizeof(int)*3);
    h->byte_order = iomh->byte_order;
    h->corner = iomh->corner;
    h->format = ihfmt2vfmt(iomh->format);
	 h->eformat = iomh->eformat;
    h->org = ihorg2vorg(iomh->org);
    h->gain = iomh->gain;
    h->offset = iomh->offset;
	 if (h->iom_h!=NULL)
		free(h->iom_h);
	 h->iom_h = (void *)iomh; /*Stash the iomf here for needed later use*/
}

void
iheader2iomheader(struct iom_iheader *iomh, struct _iheader *h)
{

/*	 memcpy(iomh,(struct iom_header *)h->iom_h,sizeof(struct iom_iheader));*/
	

    memset(iomh, 0, sizeof(struct iom_iheader));
    iomh->dptr = h->dptr;
    memcpy(iomh->prefix, h->prefix, sizeof(int)*3);
    memcpy(iomh->suffix, h->suffix, sizeof(int)*3);
    memcpy(iomh->size, h->size, sizeof(int)*3);
    memcpy(iomh->s_lo, h->s_lo, sizeof(int)*3);
    memcpy(iomh->s_hi, h->s_hi, sizeof(int)*3);
    memcpy(iomh->s_skip, h->s_skip, sizeof(int)*3);
    memcpy(iomh->dim, h->dim, sizeof(int)*3);
    iomh->byte_order = h->byte_order;
    iomh->corner = h->corner;
    iomh->format = vfmt2ihfmt(h->format);
    iomh->eformat = h->eformat;

	if (iomh->eformat == iom_EDF_INVALID){
		switch(iomh->format){
		case iom_BYTE:   iomh->eformat = iom_NATIVE_INT_1;        break;
		case iom_SHORT:  iomh->eformat = iom_NATIVE_INT_2;        break;
		case iom_INT:    iomh->eformat = iom_NATIVE_INT_4;        break;
		case iom_FLOAT:  iomh->eformat = iom_NATIVE_IEEE_REAL_4;  break;
		case iom_DOUBLE: iomh->eformat = iom_NATIVE_IEEE_REAL_8;  break;
		default:
			iomh->eformat = iom_EDF_INVALID;
			fprintf(stderr, "Error! Internal format %d not recognized.\n",
				iomh->format);
			break;
		}
	}

    iomh->transposed = 0;
    iomh->org = vorg2ihorg(h->org);
    iomh->gain = h->gain;
    iomh->offset = h->offset;
    iomh->data = ((struct iom_iheader *)h->iom_h)->data;
    iomh->ddfname = ((struct iom_iheader *)h->iom_h)->ddfname;

}




/**
 **  LoadHeader() - Try to load a file's header.
 **
 **  returns a pointer to the file, or NULL if there is no file.
 **/

FILE *
LoadHeader(char *filename, struct _iheader *header)
{
	FILE *fp;
	char fname[256];
	int input;

	struct 	iom_iheader	*iom_header;

	iom_header=(struct iom_iheader *)malloc(sizeof(struct iom_iheader));

	strcpy(fname, filename);
	expand_filename(fname);

	if ((fp = fopen(fname, "r")) == NULL) return(NULL);

	if (iom_is_compressed(fp)) {
		fp = iom_uncompress(fp, fname);
	}
	
	input=iom_LoadHeader(fp,fname,iom_header);

	if (input == 0) {
		/**
		 ** unable to determine file type
		 **/
		fprintf(stderr, "Unable to determine file type: %s\n", fname);
		fclose(fp);
		free(iom_header);
		return(NULL);
	} else {
		/*Need to convert from external format to internal*/
		iom_header->format=iom_Eformat2Iformat(iom_header->eformat);
		printf("Image is %s, %s (%s) %dx%dx%d\n", 
			iom_Org2Str(iom_header->org),
			iom_Format2Str(iom_header->format),
			iom_EFormat2Str(iom_header->eformat),
			iom_GetSamples(iom_header->size,iom_header->org),
			iom_GetLines(iom_header->size,iom_header->org),
			iom_GetBands(iom_header->size,iom_header->org));
	}
	iomheader2iheader(iom_header,header);
//	free(iom_header);
	return(fp);
}


/**
 ** read_qube_data() - generalized cube input routines
 ** replaced with iom_read_qube_data();
 **/

void *
read_qube_data(int fd, struct _iheader *h)
{
	void *data;
	int nfd;
	struct iom_iheader iom_header;

	nfd=fd;

	iheader2iomheader(&iom_header,h); 

	if (iom_header.ddfname!=NULL){
		nfd=open(iom_header.ddfname,O_RDONLY);	
	}

	data=iom_read_qube_data(nfd,&iom_header);

	if (nfd!=fd)
		close(nfd);

	iomheader2iheader(&iom_header,h);

	return(data);
}

int
is_compressed(FILE * fp)
{
	return(iom_is_compressed(fp));
}

FILE *
uncompress(FILE * fp, char *fname)
{
	return(iom_uncompress(fp,fname));
}

char *
get_env_var(char *name)
{
	return(getenv(name));
}

/**
 ** Try to expand environment variables and ~
 ** puts answer back into argument.  Make sure its big enough...
 **/

char *
expand_filename(char *s)
{
	return(iom_expand_filename(s));
}


void
parse_error(char *fmt, ...)
{
	va_list ap;

	if (VERBOSE == 0) return;

	va_start(ap, fmt);
	(void) vfprintf(stderr, fmt, ap);
	va_end(ap);
	fprintf(stderr, "\n");
}
