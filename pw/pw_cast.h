#ifndef PW_CAST_H
#define PW_CAST_H

#include <stdint.h>

/* Store small integers in Button::ext (void *). */
#define PW_CAST_INT(i) ((void *)(intptr_t)(i))
#define PW_CAST_PTR_INT(p) ((int)(intptr_t)(p))

#endif
