#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <raylib.h>

#include "utils.h"

typedef struct _Buffer {
    Font font;
    f32 fontSize;
    f32 fontSpacing;
    
    s32 textLineSpacing;
    f32 textSpacing;
    
    s32 *buffer;
    usize bufferLen;
    usize bufferCap;
    
    usize cursorPos;
} Buffer;

Buffer InitBuffer(usize cap);
void InsertBuffer(Buffer *buffer, s32 codepoint);
void BackspaceBuffer(Buffer *buffer);
void DrawBuffer(Buffer *buffer);

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
        
        s32 key;
        if (key = GetKeyPressed()) {
            printf("Key:  %d\n", key);
            switch (key) {
                case KEY_ENTER: InsertBuffer(&buffer, '\n'); break;
                case KEY_LEFT: if (buffer.cursorPos) buffer.cursorPos--; break;
                case KEY_RIGHT: if (buffer.cursorPos < buffer.bufferLen) buffer.cursorPos++; break;
                case KEY_BACKSPACE: BackspaceBuffer(&buffer); break;
                case KEY_DELETE: if (buffer.cursorPos < buffer.bufferLen) buffer.cursorPos++; BackspaceBuffer(&buffer); break;
            }
        }
        
        s32 c;
        if (c = GetCharPressed()) {
            printf("Char: %d\n", c);
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
        .font = GetFontDefault(),
        .fontSize = 20,
        .fontSpacing = 3,
        
        .textLineSpacing = 2,
        .textSpacing = 3,
        
        .buffer = malloc(cap*sizeof(s32)),
        .bufferLen = 0,
        .bufferCap = cap,
        .cursorPos = 0,
    };
}

void InsertBuffer(Buffer *buffer, s32 codepoint) {
    if (buffer->bufferLen+1 > buffer->bufferCap) {
        // TODO(m1cha1s): Expand buffer.
    }
    
    memmove(buffer->buffer+buffer->cursorPos+1,
            buffer->buffer+buffer->cursorPos,
            (buffer->bufferLen-buffer->cursorPos)*sizeof(s32));
            
    buffer->buffer[buffer->cursorPos++] = codepoint;
    buffer->bufferLen++;
}

void BackspaceBuffer(Buffer *buffer) {
    if ((!buffer->cursorPos) || (!buffer->bufferLen)) return;
    
    memmove(buffer->buffer+buffer->cursorPos-1,
            buffer->buffer+buffer->cursorPos,
            (buffer->bufferLen-buffer->cursorPos)*sizeof(s32));
    
    buffer->cursorPos--;
    buffer->bufferLen--;
}

void DrawBuffer(Buffer *buffer) {
    Vector2 textOffset = {0};
    
    f32 scaleFactor = buffer->fontSize/buffer->font.baseSize;
    
    for (usize i=0; i < buffer->bufferLen; ++i) {
        s32 codepoint = buffer->buffer[i];
        s32 index = GetGlyphIndex(buffer->font, codepoint);
        
        if (codepoint == '\n') {
            textOffset.y += (buffer->fontSize + buffer->textLineSpacing);
            textOffset.x = 0.0f;
        } else {
            if (buffer->cursorPos == i) {
                // TODO(m1cha1s): Fix not visible cursor on spaces and end of line.
                DrawRectangleV(textOffset,
                               (Vector2){buffer->font.recs[index].width *scaleFactor,
                                         buffer->font.recs[index].height*scaleFactor},
                               PINK);
            }
        
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
}
