#ifndef _ARENA_H
#define _ARENA_H

#include <stdio.h>
#include <stdlib.h>

#include "utils.h"

typedef struct _Arena {
    u8 *arena;
    u8 *end;
    usize capacity;
} Arena;

Arena InitArena(usize cap);
void ResetArena(Arena *a);
void *ArenaAlloc(Arena *a, usize size);
void DeinitArena(Arena *a);

#if defined(IMPLS)

inline Arena InitArena(usize cap) {
    Arena a = {
        .arena = malloc(cap),
        .capacity = cap,
    };
    a.end = a.arena;
    return a;
}

inline void ResetArena(Arena *a) {
    a->end = a->arena;
}

void *ArenaAlloc(Arena *a, usize size) {
    if (a->end+size > a->arena+a->capacity) {
        printf("WARNING: Arena overflow!\n");
        ResetArena(a);
    }
    
    if (size > a->capacity) {
        printf("ERROR: Arena too small for allocation!\n");
        return NULL;
    }
    
    void *ptr = a->end;
    a->end += size;
    return ptr;
}

inline void DeinitArena(Arena *a) {
    free(a->arena);
}

#endif

#endif