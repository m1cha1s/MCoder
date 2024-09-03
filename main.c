
#include "raylib/src/raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <raylib.h>
#include <math.h>

#define IMPLS

#include "utils.h"
#include "arena.h"

#define ARRAYLIST_T s32
#include "arraylist.h"

#define ARRAYLIST_T char
#include "arraylist.h"


#define FONT_BASE_SIZE 128

#define WIDTH  800
#define HEIGHT 600

#define BUFFER_COUNT 2
#define TEMP_ARENA_SIZE KB(1)

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

typedef enum _EditorMode {
    EMode_Normal,
    EMode_Open,
} EditorMode;

typedef struct _Editor {
    Buffer buffers[BUFFER_COUNT];

    usize selectedBuffer;

    f32 width, height;

    Arena tempArena;
} Editor;

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

void HandleInput(Editor *ed);

void UpdateViewport(Editor *ed, f32 width, f32 height);

s32 main() {
    InitWindow(WIDTH, HEIGHT, "MCoder");
    SetWindowState(FLAG_WINDOW_RESIZABLE);

    SetTargetFPS(60);

    Editor ed = {
        .buffers = {
            InitBuffer(KB(1)),
            InitBuffer(KB(1)),
        },
        .tempArena = InitArena(TEMP_ARENA_SIZE),
    };

    // BufferOpenFile(&buffer, "main.c");

    while (!WindowShouldClose()) {
        if (IsWindowResized()) UpdateViewport(&ed, GetScreenWidth(), GetScreenHeight());

        HandleInput(&ed);

        BeginDrawing();

        ClearBackground(BLACK);

        DrawBuffer(ed.buffers+ed.selectedBuffer);

        EndDrawing();

        ResetArena(&ed.tempArena);
    }

    for (usize i=0;i<BUFFER_COUNT;i++)
        DeinitBuffer(&ed.buffers[i]);

    CloseWindow();
}

Buffer InitBuffer(usize cap) {
    Buffer b = {
        // .fontPath = "/System/Library/Fonts/Monaco.ttf",
        .fontPath = "assets/IosevkaFixed-Medium.ttf",
        .fontSize = 20,
        .fontSpacing = 3,
        .shader = LoadShader(NULL, "sdf.fs"),

        .textLineSpacing = 2,
        .textSpacing = 3,

        .buffer = Arraylist_s32_Init(cap),
        .cursorPos = 0,

        .tempArena = InitArena(TEMP_ARENA_SIZE),

        .path = Arraylist_char_Init(8),

        .renderTex = LoadRenderTexture(GetScreenWidth(), GetScreenHeight()),

        .msg = Arraylist_char_Init(8),
    };

    BufferLoadFont(&b, 20);

    return b;
}

void DeinitBuffer(Buffer *buffer) {
    UnloadFont(buffer->font);
    UnloadShader(buffer->shader);
    DeinitArena(&buffer->tempArena);
}

void InsertBuffer(Buffer *buffer, s32 codepoint) {
    if (codepoint == '\n') buffer->bufferLines++;

    Arraylist_s32_Insert(&buffer->buffer, codepoint, buffer->cursorPos);

    buffer->cursorPos++;

    BufferFixCursorLineCol(buffer);
}

void BackspaceBuffer(Buffer *buffer) {
    if ((!buffer->cursorPos) || (!buffer->buffer.len)) return;

    if (buffer->buffer.array[buffer->cursorPos-1] == '\n') buffer->bufferLines--;

    buffer->cursorPos--;

    Arraylist_s32_Remove(&buffer->buffer, buffer->cursorPos);

    BufferFixCursorLineCol(buffer);
}

void DrawBuffer(Buffer *buffer) {
    BeginTextureMode(buffer->renderTex);
    ClearBackground(BLACK);
    Vector2 textOffset = {0,-buffer->viewLoc};

    f32 scaleFactor = buffer->fontSize/buffer->font.baseSize;

    for (usize i=0; i < buffer->buffer.len+1; ++i) {
        s32 codepoint = 0;
        s32 index = 0;

        if (i < buffer->buffer.len) {
            codepoint = buffer->buffer.array[i];
            index = GetGlyphIndex(buffer->font, codepoint);
        }

        if (buffer->cursorPos == i) {
            DrawRectangleV(textOffset,
                           (Vector2){buffer->font.recs[index].width *scaleFactor + buffer->textSpacing,
                                     buffer->fontSize + buffer->textLineSpacing},
                           PINK);
        }

        if (i >= buffer->buffer.len) continue;

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
                (textOffset.y >= -buffer->fontSize) &&
                ((textOffset.y+buffer->fontSize) <= buffer->renderTex.texture.height)) {
                Vector2 pos = {floor(textOffset.x), floor(textOffset.y)};

                BeginShaderMode(buffer->shader);
                DrawTextCodepoint(buffer->font,
                                  codepoint,
                                  pos,
                                  buffer->fontSize,
                                  WHITE);
                EndShaderMode();
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
    BeginShaderMode(buffer->shader);
    DrawTextEx(buffer->font, lcText,
             (Vector2){buffer->renderTex.texture.width-(mt.x + 2*buffer->textSpacing),
             buffer->renderTex.texture.height-(buffer->fontSize + buffer->textLineSpacing)},
             buffer->fontSize, buffer->textSpacing, BLACK);
    EndShaderMode();

    char *pathText = tfmt(&buffer->tempArena, "<%.*s>", buffer->path.len, buffer->path.array);

    mt = MeasureTextEx(buffer->font, pathText, buffer->fontSize, buffer->textSpacing);

    BeginShaderMode(buffer->shader);
    DrawTextEx(buffer->font, pathText,
             (Vector2){2*buffer->textSpacing+(buffer->renderTex.texture.width/2)-(mt.x/2),
             buffer->renderTex.texture.height-(buffer->fontSize + buffer->textLineSpacing)},
             buffer->fontSize, buffer->textSpacing, BLACK);
    EndShaderMode();

    char *msgText = tfmt(&buffer->tempArena, "%.*s", buffer->msg.len, buffer->msg.array);

    mt = MeasureTextEx(buffer->font, pathText, buffer->fontSize, buffer->textSpacing);

    BeginShaderMode(buffer->shader);
    DrawTextEx(buffer->font, msgText,
             (Vector2){2*buffer->textSpacing,
             buffer->renderTex.texture.height-(buffer->fontSize + buffer->textLineSpacing)},
             buffer->fontSize, buffer->textSpacing, BLACK);
    EndShaderMode();

    EndTextureMode();

    DrawTextureRec(buffer->renderTex.texture,
                           (Rectangle) {0,0,buffer->renderTex.texture.width,-buffer->renderTex.texture.height},
                           (Vector2){0,0},
                           WHITE);

    ResetArena(&buffer->tempArena);
}

void BufferFixCursorPos(Buffer *buffer) {
    usize l=0, c=0;

    for (usize i=0; i < buffer->buffer.len+1; ++i) {
        if (l==buffer->cursorLine && c==buffer->cursorCol) {
            buffer->cursorPos = i;
            return;
        }

        if (i < buffer->buffer.len) {
            if (buffer->buffer.array[i] == '\n' && l==buffer->cursorLine) {
                buffer->cursorPos = i;
                return;
            }

            if (buffer->buffer.array[i] == '\n' && l!=buffer->cursorLine) {
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

        if (buffer->buffer.array[i] == '\n' && i!=buffer->cursorPos) {
            buffer->cursorLine++;
            buffer->cursorCol=0;
        } else buffer->cursorCol++;
    }
}

s32 BufferOpenFile(Buffer *buffer) {
    f64 startTime = GetTime();

    char *path = tfmt(&buffer->tempArena, "%.*s", buffer->path.len, buffer->path.array);
    buffer->file = fopen(path, "r");

    if (!buffer->file) {
        buffer->path.len = 0;
        char * msg = tfmt(&buffer->tempArena, "%s not found", path);
        buffer->msg.len=0;
        for (usize i=0;i<strlen(msg);++i) Arraylist_char_Push(&buffer->msg, msg[i]);
        return -1;
    }

    fseek(buffer->file, 0L, SEEK_END);
    usize fileSize = ftell(buffer->file);
    rewind(buffer->file);

    char *fileContents = malloc(fileSize+1);

    fread(fileContents, fileSize, 1, buffer->file);

    buffer->buffer.len = buffer->cursorPos = 0;
    for (usize i=0;i<fileSize; ) {
        s32 cps;
        InsertBuffer(buffer, GetCodepointNext(fileContents+i, &cps));
        i+=cps;
    }

    fclose(buffer->file);

    buffer->cursorPos = 0;
    BufferFixCursorLineCol(buffer);

    f64 elapsedTime = GetTime()-startTime;

    char * msg = tfmt(&buffer->tempArena, "Opened (%.2fs)", elapsedTime);
    buffer->msg.len=0;
    for (usize i=0;i<strlen(msg);++i) Arraylist_char_Push(&buffer->msg, msg[i]);
}

s32 BufferSave(Buffer *buffer) {
    f64 startTime = GetTime();

    char *path = tfmt(&buffer->tempArena, "%.*s", buffer->path.len, buffer->path.array);
    buffer->file = fopen(path, "w");

    if (!buffer->file) {
        char * msg = tfmt(&buffer->tempArena, "%s not found", path);
        buffer->msg.len=0;
        for (usize i=0;i<strlen(msg);++i) Arraylist_char_Push(&buffer->msg, msg[i]);
        return -1;
    }

    for (usize i=0;i<buffer->buffer.len;++i) {
        s32 size = 0;
        char *utf8 = CodepointToUTF8(buffer->buffer.array[i], &size);
        usize wsize = fwrite(utf8, size, 1, buffer->file);
    }

    fclose(buffer->file);

    f64 elapsedTime = GetTime()-startTime;

    char * msg = tfmt(&buffer->tempArena, "Saved (%.2fs)", elapsedTime);
    buffer->msg.len=0;
    for (usize i=0;i<strlen(msg);++i) Arraylist_char_Push(&buffer->msg, msg[i]);
}

void BufferLoadFont(Buffer *buffer, s32 size) {
    s32 fdSize = 0;
    // char *fd = LoadFileData("assets/IosevkaFixed-Medium.ttf", &fdSize);
    char *fd = LoadFileData(buffer->fontPath, &fdSize);

    buffer->font.baseSize = size;
    buffer->font.glyphCount = 95;
    buffer->font.glyphs = LoadFontData(fd, fdSize, size, 0, 0, FONT_SDF);

    Image atlas = GenImageFontAtlas(buffer->font.glyphs, &buffer->font.recs, 95, size, 0, 1);
    buffer->font.texture = LoadTextureFromImage(atlas);
    UnloadImage(atlas);
    UnloadFileData(fd);

    SetTextureFilter(buffer->font.texture, TEXTURE_FILTER_BILINEAR);
}

void HandleInput(Editor *ed) {
    Buffer *buffer = ed->buffers+ed->selectedBuffer;

    if (IsKeyPressed(KEY_LEFT) || IsKeyPressedRepeat(KEY_LEFT)) {
        if (buffer->cursorPos) {
            buffer->cursorPos--;
            BufferFixCursorLineCol(buffer);
        }
    }
    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressedRepeat(KEY_RIGHT)) {
        if (buffer->cursorPos < buffer->buffer.len) {
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
        switch (buffer->mode) {
            case BMode_Norm: BackspaceBuffer(buffer); break;
            case BMode_Open: {
                if (buffer->path.len) buffer->path.len--;
            } break;
        }
    }

    if (IsKeyPressed(KEY_DELETE) || IsKeyPressedRepeat(KEY_DELETE)) {
        if (buffer->cursorPos < buffer->buffer.len) buffer->cursorPos++;
        BackspaceBuffer(buffer);
    }

    if (IsKeyPressed(KEY_ENTER) || IsKeyPressedRepeat(KEY_ENTER)) {
        switch (buffer->mode) {
            case BMode_Norm: InsertBuffer(buffer, '\n'); break;
            case BMode_Open: {
                buffer->buffer.len = 0;
                BufferOpenFile(buffer);
                buffer->mode = BMode_Norm;
            } break;
        }
    }

    if (IsKeyPressed(KEY_TAB) || IsKeyPressedRepeat(KEY_TAB)) {
        s32 spacesToInsert = 4-(buffer->cursorCol % 4);
        for (s32 i=0;i<spacesToInsert;++i) InsertBuffer(buffer, ' ');
    }

    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        Vector2 mPos = GetMousePosition();

        f32 scaleFactor = buffer->fontSize/buffer->font.baseSize;

        usize l = (usize)(mPos.y+buffer->viewLoc) / (buffer->fontSize+buffer->textLineSpacing);
        usize c = (usize)mPos.x / ((f32)buffer->font.glyphs[0].advanceX*scaleFactor + buffer->textSpacing);

        buffer->cursorLine = l;
        buffer->cursorCol = c;

        BufferFixCursorPos(buffer);
        BufferFixCursorLineCol(buffer);
    }

    if (IsKeyDown(KEY_LEFT_SUPER) || IsKeyDown(KEY_RIGHT_SUPER)) {
        s32 key;
        if ((key = GetKeyPressed())) {
            if (key == KEY_S) {
                BufferSave(buffer);
            }
            if (key == KEY_EQUAL) {
                buffer->fontSize+=4;
                UnloadFont(buffer->font);
                BufferLoadFont(buffer, buffer->fontSize);
            }
            if (key == KEY_MINUS) {
                buffer->fontSize-=4;
                UnloadFont(buffer->font);
                BufferLoadFont(buffer, buffer->fontSize);
            }
            if (key == KEY_O) {
                buffer->mode = BMode_Open;
                buffer->path.len = 0;

                char * msg = tfmt(&buffer->tempArena, "Specify path");
                buffer->msg.len=0;
                for (usize i=0;i<strlen(msg);++i) Arraylist_char_Push(&buffer->msg, msg[i]);
            }
        }
    } else {
        s32 c;
        if ((c = GetCharPressed())) {
            switch (buffer->mode) {
                case BMode_Norm: InsertBuffer(buffer, c); break;
                case BMode_Open: {
                    Arraylist_char_Push(&buffer->path, c);
                } break;
            }

        }
    }

    Vector2 movement = GetMouseWheelMoveV();
    buffer->viewLoc += -movement.y*10;
    if (buffer->viewLoc > buffer->bufferLines * (buffer->fontSize+buffer->textLineSpacing))
        buffer->viewLoc=buffer->bufferLines * (buffer->fontSize+buffer->textLineSpacing);
    if (buffer->viewLoc < 0) buffer->viewLoc=0;
}

void UpdateViewport(Editor *ed, f32 width, f32 height) {
    for (usize i=0;i<BUFFER_COUNT;i++) {
        UnloadRenderTexture(ed->buffers[i].renderTex);
        ed->buffers[i].renderTex = LoadRenderTexture(width, height);
        // TODO(m1cha1s): Add some kind of layouts here. Will need a rework.
    }
}
