#include "buffer.h"

#include <math.h>
#include <string.h>

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
