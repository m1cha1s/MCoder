#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <raylib.h>

#define IMPLS

#include "utils.h"
#include "arena.h"

#define TEMP_ARENA_SIZE KB(1)

typedef struct _Buffer {
    Font font;
    f32 fontSize;
    f32 fontSpacing;
    
    s32 textLineSpacing;
    f32 textSpacing;
    
    s32 *buffer;
    usize bufferLen;
    usize bufferCap;
    usize bufferLines;
    
    usize cursorPos;
    
    usize cursorLine;
    usize cursorCol;
    
    Arena tempArena;
} Buffer;

Buffer InitBuffer(usize cap);
void DeinitBuffer(Buffer *buffer);
void InsertBuffer(Buffer *buffer, s32 codepoint);
void BackspaceBuffer(Buffer *buffer);
void DrawBuffer(Buffer *buffer);

void BufferFixCursorPos(Buffer *buffer);
void BufferFixCursorLineCol(Buffer *buffer);

s32 main() {
    InitWindow(800, 600, "MCoder");
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    
    SetTargetFPS(60);
    
    Buffer buffer = InitBuffer(KB(1));
    
    // char *msg = "Hello \nWorld!";
    
    // for (usize i=0;i<strlen(msg);) {
    //     s32 codepointByteCount = 0;
    //     s32 codepoint = GetCodepointNext(&msg[i], &codepointByteCount);
    //     InsertBuffer(&buffer, codepoint);
    //     i += codepointByteCount;
    // }
    
    while (!WindowShouldClose()) {
        if (IsWindowResized()) { /* Update the window size. */ }
        
        if (IsKeyPressed(KEY_LEFT) || IsKeyPressedRepeat(KEY_LEFT)) {
            if (buffer.cursorPos) {
                buffer.cursorPos--;
                BufferFixCursorLineCol(&buffer);
            }
        }
        if (IsKeyPressed(KEY_RIGHT) || IsKeyPressedRepeat(KEY_RIGHT)) {
            if (buffer.cursorPos < buffer.bufferLen) {
                buffer.cursorPos++;
                BufferFixCursorLineCol(&buffer);
            }
        }
        if (IsKeyPressed(KEY_UP) || IsKeyPressedRepeat(KEY_UP)) {
            if (buffer.cursorLine) {
                buffer.cursorLine--;
                BufferFixCursorPos(&buffer);
            }
        }
        if (IsKeyPressed(KEY_DOWN) || IsKeyPressedRepeat(KEY_DOWN)) {
            if (buffer.cursorLine < buffer.bufferLines) {
                buffer.cursorLine++;
                BufferFixCursorPos(&buffer);
            }
        }
        
        s32 key;
        if (key = GetKeyPressed()) {
            switch (key) {
                case KEY_ENTER: InsertBuffer(&buffer, '\n'); break;
                case KEY_BACKSPACE: BackspaceBuffer(&buffer); break;
                case KEY_DELETE: if (buffer.cursorPos < buffer.bufferLen) buffer.cursorPos++; BackspaceBuffer(&buffer); break;
                
                case KEY_TAB: for (s32 i=0;i<4;++i) InsertBuffer(&buffer, ' '); break;
            }
        }
        
        s32 c;
        if (c = GetCharPressed()) {
            InsertBuffer(&buffer, c);
        }
        
        BeginDrawing();
        
        ClearBackground(BLACK);
               
        DrawBuffer(&buffer);
        
        EndDrawing();
    }
    
    UnloadFont(buffer.font);
    CloseWindow();
}

Buffer InitBuffer(usize cap) {
    return (Buffer){
        // .font = GetFontDefault(),
        .font = LoadFont("assets/IosevkaFixed-Medium.ttf"),
        .fontSize = 20,
        .fontSpacing = 3,
        
        .textLineSpacing = 2,
        .textSpacing = 3,
        
        .buffer = malloc(cap*sizeof(s32)),
        .bufferLen = 0,
        .bufferCap = cap,
        .cursorPos = 0,
        
        .tempArena = InitArena(TEMP_ARENA_SIZE),
    };
}

void DeinitBuffer(Buffer *buffer) {
    UnloadFont(buffer->font);
    DeinitArena(&buffer->tempArena);
}

void InsertBuffer(Buffer *buffer, s32 codepoint) {
    if (buffer->bufferLen+1 > buffer->bufferCap) {
        printf("Growing buffer...\n");
        s32 *newBuffer = malloc(buffer->bufferCap*2*sizeof(s32));
        if (!newBuffer) {
            printf("Failed to resize buffer, out of memory!\n");
            exit(-1);
        }
        memcpy(newBuffer, buffer->buffer, sizeof(s32)*buffer->bufferLen);
        free(buffer->buffer);
        buffer->buffer = newBuffer;
        buffer->bufferCap *= 2;
    }
    
    if (codepoint == '\n') buffer->bufferLines++;
    
    memmove(buffer->buffer+buffer->cursorPos+1,
            buffer->buffer+buffer->cursorPos,
            (buffer->bufferLen-buffer->cursorPos)*sizeof(s32));
            
    buffer->buffer[buffer->cursorPos++] = codepoint;
    buffer->bufferLen++;
    
    BufferFixCursorLineCol(buffer);
}

void BackspaceBuffer(Buffer *buffer) {
    if ((!buffer->cursorPos) || (!buffer->bufferLen)) return;
    
    if (buffer->buffer[buffer->cursorPos-1] == '\n') buffer->bufferLines--;
    
    memmove(buffer->buffer+buffer->cursorPos-1,
            buffer->buffer+buffer->cursorPos,
            (buffer->bufferLen-buffer->cursorPos)*sizeof(s32));
    
    buffer->cursorPos--;
    buffer->bufferLen--;
    
    BufferFixCursorLineCol(buffer);
}

void DrawBuffer(Buffer *buffer) {
    Vector2 textOffset = {0};
    
    f32 scaleFactor = buffer->fontSize/buffer->font.baseSize;
    
    for (usize i=0; i < buffer->bufferLen+1; ++i) {
        s32 codepoint = 0;
        s32 index = 0;
    
        if (i < buffer->bufferLen) {
            codepoint = buffer->buffer[i];
            index = GetGlyphIndex(buffer->font, codepoint);
        }
    
        if (buffer->cursorPos == i) {
            DrawRectangleV(textOffset,
                           (Vector2){buffer->font.recs[index].width *scaleFactor + buffer->textSpacing,
                                     buffer->fontSize + buffer->textLineSpacing},
                           PINK);
        }
        
        if (i >= buffer->bufferLen) continue;
        
        if (codepoint == '\n') {
            textOffset.y += (buffer->fontSize + buffer->textLineSpacing);
            textOffset.x = 0.0f;
        } else {
            if ((codepoint != ' ') && (codepoint != '\t')) {
                DrawTextCodepoint(buffer->font,
                                  codepoint,
                                  textOffset,
                                  buffer->fontSize,
                                  WHITE);
            }
            
            if (buffer->font.glyphs[index].advanceX == 0) 
                textOffset.x += ((f32)buffer->font.recs[index].width*scaleFactor + buffer->textSpacing);
            else
                textOffset.x += ((f32)buffer->font.glyphs[index].advanceX*scaleFactor + buffer->textSpacing);
        }
    }
    
    // Draw the status bar.
    f32 statusBarHeight = buffer->fontSize + 2*buffer->textLineSpacing;
    DrawRectangle(0, GetScreenHeight()-statusBarHeight, GetScreenWidth(), statusBarHeight, WHITE);
    
    char *lcText = tfmt(&buffer->tempArena, "Line: %d Col: %d", buffer->cursorLine+1, buffer->cursorCol+1);
    
    Vector2 mt = MeasureTextEx(buffer->font, lcText, buffer->fontSize, buffer->textSpacing);
    
    // DrawFText(&buffer->tempArena,
    //           400, 300, 20, PINK, "mt: (%.4f,%.4f)", mt.x, mt.y);
    
    DrawTextEx(buffer->font, lcText, 
             (Vector2){GetScreenWidth()-(mt.x + 2*buffer->textSpacing), 
             GetScreenHeight()-(buffer->fontSize + buffer->textLineSpacing)}, 
             buffer->fontSize, buffer->textSpacing, BLACK);
    
    ResetArena(&buffer->tempArena);
}

void BufferFixCursorPos(Buffer *buffer) {
    usize l=0, c=0;
    
    for (usize i=0; i < buffer->bufferLen; ++i) {
        if (l==buffer->cursorLine && c==buffer->cursorCol) {
            buffer->cursorPos = i;
            return;
        }
        
        if (buffer->buffer[i] == '\n' && l==buffer->cursorLine) {
            buffer->cursorPos = i;
            return;
        }
        
        if (buffer->buffer[i] == '\n' && l!=buffer->cursorLine) {
            l++;
            c=0;
            continue;
        }
        
        c++;
    }
}

void BufferFixCursorLineCol(Buffer *buffer) {
    buffer->cursorLine = 0;
    buffer->cursorCol  = 0;
    for (usize i=0; i < buffer->cursorPos; ++i) {
        
        if (buffer->buffer[i] == '\n' && i!=buffer->cursorPos) {
            buffer->cursorLine++;
            buffer->cursorCol=0;
        } else buffer->cursorCol++;
    }
}