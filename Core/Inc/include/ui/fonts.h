#ifndef FONTS_H
#define FONTS_H

#include <stdint.h>

typedef struct {
    uint16_t bitmap_index;
    uint16_t adv_w;
    uint8_t box_w;
    uint8_t box_h;
    int8_t ofs_x;
    int8_t ofs_y;
} Glyph;

typedef struct {
    char unicode;
    uint16_t glyphsIndex;
} GlyphMap;

typedef struct {
    uint8_t height;
    uint8_t maxHeight_forClipping;
    uint8_t glyphMapCount;
    const uint8_t *bitmapArray;
    const Glyph *glyphs;
    const GlyphMap *glyphMap;
} Font;

extern Font NanoSansMono_CondensedMedium;

const Glyph *getGlyphFromChar(char character, const Font *font);
void calculateMaxFontHeight(Font *font);

#endif
