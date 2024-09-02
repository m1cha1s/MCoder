#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <raylib.h>

#define IMPLS

#include "utils.h"
#include "arena.h"

#define WIDTH  800
#define HEIGHT 600

#define BUFFER_COUNT 2
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
    
    FILE *file;
    char *path;
    
    RenderTexture2D renderTex;
    f32 viewLoc;
    
    Vector2 pos;
} Buffer;

typedef enum _EditorMode {
    EMode_Normal,
    EMode_Open,
} EditorMode;

typedef struct _Editor {
    Buffer buffers[BUFFER_COUNT];
    
    Buffer *selectedBuffer;
    
    f32 width, height;
    
    Arena tempArena;
    Arena actionArena;
    
    EditorMode mode;
} Editor;

Buffer InitBuffer(usize cap, f32 x, f32 y, f32 width, f32 height);
void DeinitBuffer(Buffer *buffer);
void InsertBuffer(Buffer *buffer, s32 codepoint);
void BackspaceBuffer(Buffer *buffer);
void DrawBuffer(Buffer *buffer);
s32 BufferOpenFile(Buffer *buffer, char *path);
s32 BufferSave(Buffer *buffer);

void BufferFixCursorPos(Buffer *buffer);
void BufferFixCursorLineCol(Buffer *buffer);

void HandleInput(Editor *ed);

void UpdateViewport(Editor *ed, f32 width, f32 height);

s32 main() {
    InitWindow(WIDTH, HEIGHT, "MCoder");
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    
    SetTargetFPS(60);
    
    Editor ed = {
        .buffers = {
            InitBuffer(KB(1), 0, 0, 400, 600),
            InitBuffer(KB(1), 400, 0, 400, 600),
        },
        .tempArena = InitArena(TEMP_ARENA_SIZE),
        .actionArena = InitArena(TEMP_ARENA_SIZE),
    };
    ed.selectedBuffer = &ed.buffers[0];
    
    // BufferOpenFile(&buffer, "main.c");
    
    while (!WindowShouldClose()) {
        if (IsWindowResized()) UpdateViewport(&ed, GetScreenWidth(), GetScreenHeight());
        
        HandleInput(&ed);
        
        BeginDrawing();
        
        ClearBackground(BLACK);
        
        for (usize i=0;i<BUFFER_COUNT;i++) {  
            DrawBuffer(ed.buffers+i);
        }
        
        DrawBuffer(ed.selectedBuffer);
        
        EndDrawing();
        
        ResetArena(&ed.tempArena);
    }
    
    for (usize i=0;i<BUFFER_COUNT;i++)
        DeinitBuffer(&ed.buffers[i]);
    
    CloseWindow();
}

Buffer InitBuffer(usize cap, f32 x, f32 y, f32 width, f32 height) {
    Buffer b = {
        // .font = GetFontDefault(),
        .font = LoadFont("assets/IosevkaFixed-Medium.ttf"),
        // .font = LoadFont("/System/Library/Fonts/Monaco.ttf"),
        // .font = LoadFontEx("/System/Library/Fonts/Monaco.ttf", 32, NULL, 0),
        .fontSize = 20,
        .fontSpacing = 3,
        
        .textLineSpacing = 2,
        .textSpacing = 3,
        
        .buffer = malloc(cap*sizeof(s32)),
        .bufferLen = 0,
        .bufferCap = cap,
        .cursorPos = 0,
        
        .tempArena = InitArena(TEMP_ARENA_SIZE),
        
        .path = "",
        .renderTex = LoadRenderTexture(width, height),
        
        .pos = (Vector2){x,y},
    };
    
    SetTextureFilter(b.font.texture, TEXTURE_FILTER_ANISOTROPIC_8X);
    
    return b;
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
    BeginTextureMode(buffer->renderTex);
    ClearBackground(BLACK);
    Vector2 textOffset = {0,-buffer->viewLoc};
    
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
            f32 newXOff = 0;
            
            if (buffer->font.glyphs[index].advanceX == 0) 
                newXOff = ((f32)buffer->font.recs[index].width*scaleFactor + buffer->textSpacing);
            else
                newXOff = ((f32)buffer->font.glyphs[index].advanceX*scaleFactor + buffer->textSpacing);
            
            if (textOffset.x+newXOff > buffer->renderTex.texture.width) {
                textOffset.x = 0;
                textOffset.y += (buffer->fontSize + buffer->textLineSpacing);
            }
            
            if ((codepoint != ' ') && 
                (codepoint != '\t') && 
                (textOffset.y >= 0) && 
                ((textOffset.y+buffer->fontSize) <= buffer->renderTex.texture.height)) {
                DrawTextCodepoint(buffer->font,
                                  codepoint,
                                  textOffset,
                                  buffer->fontSize,
                                  WHITE);
            }
            
            textOffset.x += newXOff;
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
             (Vector2){buffer->renderTex.texture.width-(mt.x + 2*buffer->textSpacing), 
             buffer->renderTex.texture.height-(buffer->fontSize + buffer->textLineSpacing)}, 
             buffer->fontSize, buffer->textSpacing, BLACK);
             
    char *pathText = tfmt(&buffer->tempArena, "Path: %s", buffer->path);
    
    mt = MeasureTextEx(buffer->font, pathText, buffer->fontSize, buffer->textSpacing);
      
    DrawTextEx(buffer->font, pathText, 
             (Vector2){2*buffer->textSpacing, 
             buffer->renderTex.texture.height-(buffer->fontSize + buffer->textLineSpacing)}, 
             buffer->fontSize, buffer->textSpacing, BLACK);
    
    EndTextureMode();
    
    DrawTextureRec(buffer->renderTex.texture, 
                           (Rectangle) {0,0,buffer->renderTex.texture.width,-buffer->renderTex.texture.height}, 
                           buffer->pos, 
                           WHITE);
    
    ResetArena(&buffer->tempArena);
}

void BufferFixCursorPos(Buffer *buffer) {
    usize l=0, c=0;
    
    for (usize i=0; i < buffer->bufferLen+1; ++i) {
        if (l==buffer->cursorLine && c==buffer->cursorCol) {
            buffer->cursorPos = i;
            return;
        }
        
        if (i < buffer->bufferLen) {
            if (buffer->buffer[i] == '\n' && l==buffer->cursorLine) {
                buffer->cursorPos = i;
                return;
            }
            
            if (buffer->buffer[i] == '\n' && l!=buffer->cursorLine) {
                l++;
                c=0;
                continue;
            }
        } else {
            if (l==buffer->cursorLine) {
                buffer->cursorPos = i;
                return;
            }
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

s32 BufferOpenFile(Buffer *buffer, char *path) {
    buffer->path = path;
    buffer->file = fopen(path, "r");
    
    if (!buffer->file) {
        buffer->path = "";
        return -1;
    }
    
    fseek(buffer->file, 0L, SEEK_END);
    usize fileSize = ftell(buffer->file);
    rewind(buffer->file);
    
    char *fileContents = malloc(fileSize+1);
    
    fread(fileContents, fileSize, 1, buffer->file);
    
    for (usize i=0;i<fileSize;) {
        s32 codepointSize = 0;
        InsertBuffer(buffer, GetCodepointNext(fileContents+i, &codepointSize));
        i += codepointSize;
    }
    
    fclose(buffer->file);
    
    buffer->cursorPos = 0;
    BufferFixCursorLineCol(buffer);
}

s32 BufferSave(Buffer *buffer) {
    f64 startTime = GetTime();
    buffer->file = fopen(buffer->path, "w");
    
    for (usize i=0;i<buffer->bufferLen;++i) {
        s32 size = 0;
        char *utf8 = CodepointToUTF8(buffer->buffer[i], &size);
        usize wsize = fwrite(utf8, size, 1, buffer->file);
        
        // printf("%d\n", wsize);
    }
    
    fclose(buffer->file);
    
    f64 elapsedTime = GetTime()-startTime;
    printf("Saved in %.2f seconds.\n", elapsedTime);
}

void HandleInput(Editor *ed) {
    Buffer *buffer = ed->selectedBuffer;
    
    if (IsKeyPressed(KEY_LEFT) || IsKeyPressedRepeat(KEY_LEFT)) {
        if (buffer->cursorPos) {
            buffer->cursorPos--;
            BufferFixCursorLineCol(buffer);
        }
    }
    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressedRepeat(KEY_RIGHT)) {
        if (buffer->cursorPos < buffer->bufferLen) {
            buffer->cursorPos++;
            BufferFixCursorLineCol(buffer);
        }
    }
    if (IsKeyPressed(KEY_UP) || IsKeyPressedRepeat(KEY_UP)) {
        if (buffer->cursorLine) {
            buffer->cursorLine--;
            BufferFixCursorPos(buffer);
        }
    }
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressedRepeat(KEY_DOWN)) {
        if (buffer->cursorLine < buffer->bufferLines) {
            buffer->cursorLine++;
            BufferFixCursorPos(buffer);
        }
    }
    
    if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) {
        BackspaceBuffer(buffer);
    }
    
    if (IsKeyPressed(KEY_DELETE) || IsKeyPressedRepeat(KEY_DELETE)) {
        if (buffer->cursorPos < buffer->bufferLen) buffer->cursorPos++; 
        BackspaceBuffer(buffer);
    }
    
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressedRepeat(KEY_ENTER)) {
        if (ed->mode == EMode_Open) {
            char *filename = ArenaAlloc(&ed->tempArena, buffer->bufferLen+1);
            for (usize i=0;i<buffer->bufferLen;++i) {
                filename[i] = (char)buffer->buffer[i];
            }
            filename[buffer->bufferLen] = '\0';
        } else InsertBuffer(buffer, '\n');
    }
    
    if (IsKeyPressed(KEY_TAB) || IsKeyPressedRepeat(KEY_TAB)) {
        s32 spacesToInsert = 4-(buffer->cursorCol % 4);
        for (s32 i=0;i<spacesToInsert;++i) InsertBuffer(buffer, ' ');
    }
    
    if (IsKeyDown(KEY_LEFT_SUPER) || IsKeyDown(KEY_RIGHT_SUPER)) {
        s32 key;
        if (key = GetKeyPressed()) {
            if (key == KEY_S) {
                BufferSave(buffer);
            }
            if (key == KEY_EQUAL) {
                buffer->fontSize+=4;
            }
            if (key == KEY_MINUS) {
                buffer->fontSize-=4;
            }
            if (key == KEY_O) {
                ed->mode = EMode_Open;
                
                Buffer *tb = ArenaAlloc(&ed->actionArena, sizeof(Buffer));
                *tb = InitBuffer(128, 200, 150, 400, 300);
                
                ed->selectedBuffer = tb;
            }
        }
    } else {
        s32 c;
        if (c = GetCharPressed()) {
            InsertBuffer(buffer, c);
        }
    }
    
    Vector2 movement = GetMouseWheelMoveV();
    ed->selectedBuffer->viewLoc += -movement.y*10;
}

void UpdateViewport(Editor *ed, f32 width, f32 height) {
    for (usize i=0;i<BUFFER_COUNT;i++) {
        UnloadRenderTexture(ed->buffers[i].renderTex);
        ed->buffers[i].renderTex = LoadRenderTexture(width, height);
        // TODO(m1cha1s): Add some kind of layouts here. Will need a rework.
    }
}