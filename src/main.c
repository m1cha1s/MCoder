#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <raylib.h>
#include <rlgl.h>

#define IMPLS

#include "buffer.h"

#include "utils.h"
#include "memory.h"


# ifndef ARRAYLIST
# define ARRAYLIST

#  define T s32
#  include "arraylist.h"
#  define T char
#  include "arraylist.h"

# endif

#define FONT_BASE_SIZE 128

#define WIDTH  800
#define HEIGHT 600

#define BUFFER_COUNT 2


typedef enum _EditorMode {
    EMode_Normal,
    EMode_Open,
} EditorMode;

typedef struct _Editor {
    Buffer buffers[BUFFER_COUNT];

    usize selectedBuffer;

    f32 width, height;

    Alloc tempAlloc;
} Editor;

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
        .tempAlloc = NewArenaAlloc(SysAlloc, TEMP_ARENA_SIZE),
    };

    // BufferOpenFile(&buffer, "main.c");

    while (!WindowShouldClose()) {
        if (IsWindowResized()) UpdateViewport(&ed, GetScreenWidth(), GetScreenHeight());

        HandleInput(&ed);

        BeginDrawing();

        ClearBackground(BLACK);

        DrawBuffer(ed.buffers+ed.selectedBuffer);

        EndDrawing();

        memClear(ed.tempAlloc);
    }

    for (usize i=0;i<BUFFER_COUNT;i++)
        DeinitBuffer(&ed.buffers[i]);

    CloseWindow();
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

    if (IsKeyDown(KEY_LEFT_SUPER) || IsKeyDown(KEY_RIGHT_SUPER) || IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) {
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

                char * msg = tfmt(buffer->tempAlloc, "Specify path");
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
    buffer->viewLoc += -movement.y*100;
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
