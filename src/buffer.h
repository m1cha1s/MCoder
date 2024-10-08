#ifndef _BUFFER_H
#define _BUFFER_H

#include "utils.h"
#include "memory.h"

#include <stdio.h>

typedef struct _Line {
    usize start;
    usize end;
} Line;

#ifndef ARRAYLIST
#define ARRAYLIST

#define T s32
#include "arraylist.h"
#define T char
#include "arraylist.h"
#define T Line
#include "arraylist.h"

#endif

typedef enum _BufferMode {
    BMode_Norm,
    BMode_Open,
} BufferMode;

typedef struct _Buffer {
    char *fontPath;
    Font font;
    f32 fontSize;
    f32 fontSpacing;
    Shader shader;

    s32 textLineSpacing;
    f32 textSpacing;

    Arraylist_s32 buffer;

    usize cursorPos;
    usize cursorLine;

    Alloc tempAlloc;

    FILE *file;
    Arraylist_char path;

    RenderTexture2D renderTex;
    f32 viewLoc;

    BufferMode mode;

    Arraylist_char msg;

    Arraylist_Line lines;
} Buffer;

Buffer InitBuffer(usize cap);
void DeinitBuffer(Buffer *buffer);
void InsertBuffer(Buffer *buffer, s32 codepoint);
void BackspaceBuffer(Buffer *buffer);
void DrawBuffer(Buffer *buffer);
s32 BufferOpenFile(Buffer *buffer);// s32 *buffer;
    // usize bufferLen;
    // usize bufferCap;
s32 BufferSave(Buffer *buffer);
void BufferLoadFont(Buffer *buffer, s32 size);

void BufferFixCursorPos(Buffer *buffer);
void BufferFixCursorLineCol(Buffer *buffer);

#endif
