#include "fonts.h"
#include <stdio.h>
#include <stdlib.h>

const Glyph *getGlyphFromChar(char character, const Font *font) {
    for (int i = 0; i < font->glyphMapCount; i++) {
        if (font->glyphMap[i].unicode == character) {
            return &font->glyphs[font->glyphMap[i].glyphsIndex];
        }
    }
    return NULL;
}

void calculateMaxFontHeight(Font *font) {
    uint8_t maxHeight = font->line_height;
    uint8_t currentHeight = 0;
    for (int i = 1; i <= font->glyphMapCount; i++) {
        currentHeight = (int)(font->glyphs[i].box_h * 0.5 + 0.5) + abs(font->glyphs[i].ofs_y);
        maxHeight = currentHeight > maxHeight ? currentHeight : maxHeight;
    }
    font->maxHeight_forClipping = maxHeight;
}
