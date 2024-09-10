#include "buffer.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

Buffer InitBuffer(usize cap) {
    Buffer b = {
        // .fontPath = "/System/Library/Fonts/Monaco.ttf",
        .fontPath = "assets/IosevkaFixed-Medium.ttf",
        .fontSize = 20,
        .fontSpacing = 3,
        .shader = LoadShader(NULL, "shaders/sdf.fs"),

        .textLineSpacing = 2,
        .textSpacing = 3,

        .buffer = Arraylist_s32_Init(cap),
        .cursorPos = 0,

        .tempAlloc = NewArenaAlloc(SysAlloc, TEMP_ARENA_SIZE),

        .path = Arraylist_char_Init(8),

        .renderTex = LoadRenderTexture(GetScreenWidth(), GetScreenHeight()),

        .msg = Arraylist_char_Init(8),

        .lines = Arraylist_Line_Init(8),
    };

    BufferLoadFont(&b, 20);

    return b;
}

void DeinitBuffer(Buffer *buffer) {
    UnloadFont(buffer->font);
    UnloadShader(buffer->shader);
    DeleteArenaAlloc(SysAlloc, buffer->tempAlloc);
}

void RescanBuffer(Buffer *buffer) {
    buffer->lines.len = 0; // Reseting the lines.

    usize lineStart = 0;
    for (usize i=0; i < buffer->buffer.len;++i) {
        if (buffer->buffer.array[i] == '\n') {
            Arraylist_Line_Push(&buffer->lines, (Line){lineStart, i});
            lineStart = i+1;
        }
    }
}

void InsertBufferBlock(Buffer *buffer, u8 *data, usize dataLen) {
    for (usize i=0;i<dataLen;) {
        s32 cps;
        Arraylist_s32_Insert(&buffer->buffer, GetCodepointNext(data+i, &cps), buffer->cursorPos);
        buffer->cursorPos++;
        i+=cps;
    }

    RescanBuffer(buffer);
}

void InsertBuffer(Buffer *buffer, s32 codepoint) {
    Arraylist_s32_Insert(&buffer->buffer, codepoint, buffer->cursorPos);

    buffer->lines.array[buffer->cursorLine].end++;
    buffer->cursorPos++;

    if (codepoint == '\n') {
        buffer->bufferLines++;
        Arraylist_Line_Push(&buffer->lines,
                              (Line){.start=buffer->cursorPos, .end=buffer->cursorPos});
    }

    // TODO(m1cha1s): Remove, add a line buffer to keep track of lines and so on.
    // BufferFixCursorLineCol(buffer); // NOTE(m1cha1s): This takes 99.99% of the time during loading a file!!!
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

    // BeginShaderMode(buffer->shader);
    for (usize i=0; i < buffer->buffer.len+1; ++i) {
        s32 codepoint = 0;
        s32 index = 0;

        // TODO(m1cha1s): Don't render the whole buffer, just the things that are visible.
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

            DrawTextCodepoint(buffer->font,
                              codepoint,
                              pos,
                              buffer->fontSize,
                              WHITE);
                }

                textOffset.x += newXOff;
        }
    }
    // EndShaderMode();

    // Draw the status bar.
    f32 statusBarHeight = buffer->fontSize + 2*buffer->textLineSpacing;
    DrawRectangle(0, GetScreenHeight()-statusBarHeight, GetScreenWidth(), statusBarHeight, WHITE);

    char *lcText = tfmt(buffer->tempAlloc, "Line: %d Col: %d", buffer->cursorLine+1, buffer->cursorPos+1-buffer->lines.array[buffer->cursorLine].start);

    Vector2 mt = MeasureTextEx(buffer->font, lcText, buffer->fontSize, buffer->textSpacing);

    // DrawFText(&buffer->tempArena,
    //           400, 300, 20, PINK, "mt: (%.4f,%.4f)", mt.x, mt.y);
    // BeginShaderMode(buffer->shader);
    DrawTextEx(buffer->font, lcText,
               (Vector2){buffer->renderTex.texture.width-(mt.x + 2*buffer->textSpacing),
                   buffer->renderTex.texture.height-(buffer->fontSize + buffer->textLineSpacing)},
               buffer->fontSize, buffer->textSpacing, BLACK);
    // EndShaderMode();

    char *pathText = tfmt(buffer->tempAlloc, "<%.*s>", buffer->path.len, buffer->path.array);

    mt = MeasureTextEx(buffer->font, pathText, buffer->fontSize, buffer->textSpacing);

    // BeginShaderMode(buffer->shader);
    DrawTextEx(buffer->font, pathText,
               (Vector2){2*buffer->textSpacing+(buffer->renderTex.texture.width/2)-(mt.x/2),
                   buffer->renderTex.texture.height-(buffer->fontSize + buffer->textLineSpacing)},
               buffer->fontSize, buffer->textSpacing, BLACK);
    // EndShaderMode();

    char *msgText = tfmt(buffer->tempAlloc, "%.*s", buffer->msg.len, buffer->msg.array);

    mt = MeasureTextEx(buffer->font, pathText, buffer->fontSize, buffer->textSpacing);

    // BeginShaderMode(buffer->shader);
    DrawTextEx(buffer->font, msgText,
               (Vector2){2*buffer->textSpacing,
                   buffer->renderTex.texture.height-(buffer->fontSize + buffer->textLineSpacing)},
               buffer->fontSize, buffer->textSpacing, BLACK);
    // EndShaderMode();

    EndTextureMode();

    DrawTextureRec(buffer->renderTex.texture,
                   (Rectangle) {0,0,buffer->renderTex.texture.width,-buffer->renderTex.texture.height},
                   (Vector2){0,0},
                   WHITE);

    memClear(buffer->tempAlloc);
}

void BufferFixCursorPos(Buffer *buffer) {
    buffer->cursorPos = min(buffer->cursorPos, buffer->lines.array[buffer->cursorLine].end);
}

void BufferFixCursorLineCol(Buffer *buffer) {
    b8 foundLine = false;
    usize i = 0;
    for (i = 0; i < buffer->lines.len; ++i) {
        usize start = buffer->lines.array[i].start;
        usize end   = buffer->lines.array[i].end;
        if (buffer->cursorPos >= start && buffer->cursorPos <= end) {
            foundLine = true;
            break;
        }
    }

    if (foundLine) {
        buffer->cursorLine=i;
    } else {
        buffer->cursorPos = buffer->lines.array[i].end;
    }
}

s32 BufferOpenFile(Buffer *buffer) {
    f64 startTime = GetTime();

    char *path = tfmt(buffer->tempAlloc, "%.*s", buffer->path.len, buffer->path.array);
    buffer->file = fopen(path, "r");

    if (!buffer->file) {
        buffer->path.len = 0;
        char * msg = tfmt(buffer->tempAlloc, "%s not found", path);
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

    char * msg = tfmt(buffer->tempAlloc, "Opened (%.2fs)", elapsedTime);
    buffer->msg.len=0;
    for (usize i=0;i<strlen(msg);++i) Arraylist_char_Push(&buffer->msg, msg[i]);
}

s32 BufferSave(Buffer *buffer) {
    f64 startTime = GetTime();

    char *path = tfmt(buffer->tempAlloc, "%.*s", buffer->path.len, buffer->path.array);
    buffer->file = fopen(path, "w");

    if (!buffer->file) {
        char * msg = tfmt(buffer->tempAlloc, "%s not found", path);
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

    char * msg = tfmt(buffer->tempAlloc, "Saved (%.2fs)", elapsedTime);
    buffer->msg.len=0;
    for (usize i=0;i<strlen(msg);++i) Arraylist_char_Push(&buffer->msg, msg[i]);
}

void BufferLoadFont(Buffer *buffer, s32 size) {
    s32 fdSize = 0;
    // char *fd = LoadFileData("assets/IosevkaFixed-Medium.ttf", &fdSize);
    char *fd = LoadFileData(buffer->fontPath, &fdSize);

    buffer->font.baseSize = size;
    buffer->font.glyphCount = 95;
    // buffer->font.glyphs = LoadFontData(fd, fdSize, size, 0, 0, FONT_SDF);
    buffer->font.glyphs = LoadFontData(fd, fdSize, size, 0, 0, FONT_DEFAULT);

    Image atlas = GenImageFontAtlas(buffer->font.glyphs, &buffer->font.recs, 95, size, 0, 1);
    buffer->font.texture = LoadTextureFromImage(atlas);
    UnloadImage(atlas);
    UnloadFileData(fd);

    SetTextureFilter(buffer->font.texture, TEXTURE_FILTER_BILINEAR);
}
