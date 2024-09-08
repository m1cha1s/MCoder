#ifndef _MEMORY_H
#define _MEMORY_H

#include "utils.h"
#include "arena.h"

typedef enum _AllocMsg {
    ALLOC_ALLOC,
    ALLOC_FREE,
    ALLOC_CLEAR,
} AllocMsg;

typedef void *(*AllocProc)(usize, void *, AllocMsg, void *);

typedef struct _Alloc {
    AllocProc proc;
    void *data;
} Alloc;

void *memAlloc(Alloc alloc, usize size);
void memFree(Alloc alloc, void *p);
void memClear(Alloc alloc);


typedef struct _Arena {
    u8 *arena;
    u8 *end;
    usize capacity;
} Arena;

Arena InitArena(Alloc alloc, usize cap);
void ResetArena(Arena *a);
void *ArenaAlloc(Arena *a, usize size);
void DeinitArena(Alloc alloc, Arena *a);

extern Alloc SysAlloc;

void *SysAllocProc(usize size, void *p, AllocMsg msg, void *_data);
void *ArenaAllocProc(usize size, void *p, AllocMsg msg, void *arena);

Alloc NewArenaAlloc(Alloc alloc, usize size);
void DeleteArenaAlloc(Alloc alloc, Alloc the_alloc);

# if defined(IMPLS)

void *memAlloc(Alloc alloc, usize size) {
    return alloc.proc(size, NULL, ALLOC_ALLOC, alloc.data);
}

void memFree(Alloc alloc, void *p) {
    alloc.proc(0, p, ALLOC_FREE, alloc.data);
}

void memClear(Alloc alloc) {
    alloc.proc(0, NULL, ALLOC_CLEAR, alloc.data);
}

#include <stdlib.h>

Alloc SysAlloc = (Alloc){
    .proc = SysAllocProc,
}; 

void *SysAllocProc(usize size, void *p, AllocMsg msg, void *_data) {
    switch(msg) {
        case ALLOC_ALLOC: {
            return malloc(size);
        } break;
        case ALLOC_FREE: {
            free(p);
        } break;
        case ALLOC_CLEAR: {} break;
    }
    return NULL;
}

Alloc NewArenaAlloc(Alloc alloc, usize size) {
    Arena *arena = memAlloc(alloc, sizeof(Arena));
    *arena = InitArena(alloc, size);
    
    return (Alloc) {
        .proc = ArenaAllocProc,
        .data = (void*)arena,  
    };
}

void DeleteArenaAlloc(Alloc alloc, Alloc the_alloc) {
    DeinitArena(alloc, (Arena*)the_alloc.data);
    memFree(alloc, the_alloc.data);
}

void *ArenaAllocProc(usize size, void *p, AllocMsg msg, void *arena){
    switch(msg) {
        case ALLOC_ALLOC: {
            return ArenaAlloc((Arena*)arena, size);
        } break;
        case ALLOC_FREE: {} break;
        case ALLOC_CLEAR: {
            ResetArena((Arena*)arena);
        } break;
    }
    return NULL;
}

inline Arena InitArena(Alloc alloc, usize cap) {
    Arena a = {
        .arena = memAlloc(alloc, cap),
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

inline void DeinitArena(Alloc alloc, Arena *a) {
    memFree(alloc, a->arena);
}

# endif

#endif // _MEMORY_H