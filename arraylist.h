#include "utils.h"

#define _AL(x) GLUE(_Arraylist_,x)
#define AL(x) GLUE(Arraylist_,x)

typedef struct AL(T) {
    T *array;
    usize cap;
    usize len;
} AL(T);

AL(T) GLUE(AL(T),_Init)(usize cap);
void GLUE(AL(T),_Insert)(AL(T) *al, T val, usize i);
void GLUE(AL(T),_Remove)(AL(T) *al, usize i);
void GLUE(AL(T),_Push)(AL(T) *al, T val);
T GLUE(AL(T),_Pop)(AL(T) *al);
void GLUE(AL(T),_Deinit)(AL(T) *al);

void GLUE(AL(T),_Grow)(AL(T) *al);
void GLUE(AL(T),_Reserve)(AL(T) *al, usize len);

#ifdef IMPLS

#include <stdlib.h>

AL(T) GLUE(AL(T),_Init)(usize cap) {
    return (AL(T)){
        .array = malloc(cap*sizeof(T)),
        .cap = cap,
    };
}

void GLUE(AL(T),_Insert)(AL(T) *al, T val, usize i) {
    if (i >= al->cap) GLUE(AL(T),_Reserve)(al, i);
    if (al->len+1 > al->cap) GLUE(AL(T),_Grow)(al);

    memmove(al->array+i+1,
            al->array+i,
            (al->len-i)*sizeof(T));

    al->array[i] = val;
    al->len++;
}

void GLUE(AL(T),_Remove)(AL(T) *al, usize i) {
    if (!al->len) return;

    memmove(al->array+i,
            al->array+i+1,
            (al->len-i)*sizeof(T));

    al->len--;
}

void GLUE(AL(T),_Push)(AL(T) *al, T val) {
    GLUE(AL(T),_Insert)(al, val, al->len);
}

T GLUE(AL(T),_Pop)(AL(T) *al) {
    if (!al->len) return (T)0;
    T val = al->array[al->len-1];
    GLUE(AL(T),_Remove)(al, al->len-1);
    return val;
}

void GLUE(AL(T),_Deinit)(AL(T) *al) {
    free(al->array);
}

void GLUE(AL(T),_Grow)(AL(T) *al) {
    T *newAl = malloc(al->cap*2*sizeof(T));
    memmove(newAl, al->array, al->len*sizeof(T));
    free(al->array);
    al->array = newAl;
    al->cap *= 2;
}

void GLUE(AL(T),_Reserve)(AL(T) *al, usize len) {
    for (;al->len < len;al->len++) {
        if (al->len > al->cap) GLUE(AL(T),_Grow)(al);
    }
}

#endif

#undef T
#undef _AL
#undef AL
