#ifndef _BUFFER_H
#define _BUFFER_H

#include "utils.h"

#ifndef ARRAYLIST
#define ARRAYLIST

#define T s32
#include "arraylist.h"
#define T char
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

    // s32 *buffer;
    // usize bufferLen;
    // usize bufferCap;
    Arraylist_s32 buffer;

    usize bufferLines;

    usize cursorPos;

    usize cursorLine;
    usize cursorCol;

    Arena tempArena;

    FILE *file;
    Arraylist_char path;

    RenderTexture2D renderTex;
    f32 viewLoc;

    BufferMode mode;

    Arraylist_char msg;
} Buffer;

Buffer InitBuffer(usize cap);
void DeinitBuffer(Buffer *buffer);
void InsertBuffer(Buffer *buffer, s32 codepoint);
void BackspaceBuffer(Buffer *buffer);
void DrawBuffer(Buffer *buffer);
s32 BufferOpenFile(Buffer *buffer);
s32 BufferSave(Buffer *buffer);
void BufferLoadFont(Buffer *buffer, s32 size);

void BufferFixCursorPos(Buffer *buffer);
void BufferFixCursorLineCol(Buffer *buffer);

#endif
