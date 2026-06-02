#include <math.h>
void inice(float alow, float ahigh, float aamax, float *bint, float *astrt)
{
    int ia;
    double rng, amin, amax, a, b, d_bint;

    rng = (double)(ahigh - alow);
    amin = (double)alow;
    amax = (double)aamax;
    if (amax <= (double)0.0)
        amax = (double)1.0;
    a = log10(rng / amax);
    ia = a;
    if (ia > a)
        ia = ia - (double)1.0;
    b = a - ia;
    if (b <= log10((double)2.0))
        b = log10((double)2.0);
    else if (b > log10((double)5.0))
        b = (double)1.0;
    else
        b = log10((double)5.0);
    d_bint = pow((double)10.0, (ia + b));
    a = fabs(amin) + d_bint;
    do {
        a = a - d_bint;
        if (a - (double)10.0 * d_bint > (double)0.0)
            a = a - ((double)10.0 * d_bint);
        if (a - (double)10.0 * (double)10.0 * d_bint > (double)0.0)
            a = a - ((double)10.0 * (double)10.0 * d_bint);
        if (a - d_bint <= (double)0.0 && a >= (double)0.0)
            break;
    } while (a >= (double)0.0);
    if (amin < 0)
        a = amin + a;
    else if (amin == (double)0.0)
        a = amin;
    else
        a = amin - a + d_bint;
    *bint = (float)d_bint;
    *astrt = (float)a;
}
