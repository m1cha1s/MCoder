#ifndef _STRING_H
#define _STRING_H

#include "utils.h"

typedef struct _String {
    usize len;
    u8 *data;
} String;

#endif // _STRING_H