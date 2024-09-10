#ifndef _UTILS_H
#define _UTILS_H

#include <stddef.h>

typedef unsigned char u8;
typedef unsigned int u32;
typedef int s32;

typedef size_t usize;
// typedef signed size_t ssize;

typedef float f32;
typedef double f64;

typedef u8 b8;

#define true 1
#define false 0

#define _GLUE(a,b) a##b
#define GLUE(a,b) _GLUE(a,b)

#define KB(x) (1024*(x))
#define MB(x) (1024*KB(x))
#define GB(x) (1024*MB(x))

#define TEMP_ARENA_SIZE KB(1)

#define max(a,b) (a)<(b)?(b):(a)
#define min(a,b) (a)<(b)?(a):(b)
#define clamp(a,l,h) max(l, min(a, h))

#include "memory.h"
#include <raylib.h>

char *tfmt(Alloc alloc, const char * format, ...);

s32 DrawFText(Alloc alloc, u32 x, u32 y, u32 size, Color c, const char * format, ...);

#if defined(IMPLS)

#include <stdarg.h>

char *tfmt(Alloc alloc, const char * format, ...) {
    va_list ptr;
    va_start(ptr, format);
    usize len = vsnprintf(NULL, 0, format, ptr);
    va_end(ptr);

    char *buffer = memAlloc(alloc, len+1);

    if (!buffer) return NULL;

    va_start(ptr, format);
    vsnprintf(buffer, len+1, format, ptr);
    va_end(ptr);

    return buffer;
}

#include <raylib.h>

s32 DrawFText(Alloc alloc, u32 x, u32 y, u32 size, Color c, const char * format, ...) {
    va_list ptr;
    va_start(ptr, format);
    usize len = vsnprintf(NULL, 0, format, ptr);
    va_end(ptr);

    char *buffer = memAlloc(alloc, len+1);

    if (!buffer) return -1;

    va_start(ptr, format);
    vsnprintf(buffer, len+1, format, ptr);
    va_end(ptr);

    DrawText(buffer, x, y, size, c);
}

#endif

#endif
