#include "image.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

static char vfmt[] = "FORMAT='BYTE'  TYPE='IMAGE'  BUFSIZ=%d  DIM=2\
  EOL=0  RECSIZE=%d  ORG='BSQ'  NL=%d  NS=%d  NB=1  N1=0  N2=0  N3=0\
  N4=0  NBB=0  NLB=0  TASK='conversion'  USER='%s'  DAT_TIM='%s'";

WriteVicarHeader(FILE *f, int x, int y)
{
    char tbuf[512];
    char lbuf[20];
    long c;
    int i;
    int label;

    c = time(0);
    sprintf(tbuf, vfmt, x * 2, /* bufsize */
            x, /* recsize */
            y, /* lines */
            x, /* samples */
            getenv("USER"), ctime(&c));

    label = max(strlen(tbuf) + 20, x);
    sprintf(lbuf, "LBLSIZE=%d ", label);

    fwrite(lbuf, 1, strlen(lbuf), f);
    fwrite(tbuf, 1, strlen(tbuf), f);
    for (i = strlen(tbuf) + strlen(lbuf); i < label; i++) {
        fwrite(" ", 1, 1, f);
    }
}
