/*
 * Stub Xfred.h for unit tests — pw sources include this instead of lib/Xfred.h
 * when built with -Itests/compat before other include paths.
 */
#ifndef XFRED_H
#define XFRED_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#endif                          /* XFRED_H */
