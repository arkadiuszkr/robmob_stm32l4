#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "fonts.h"
#include "freertos_header.h"
#include "lcd_ILI9488.h"
#include "main.h"
#include "spi.h"
#include "stm32l4xx_hal.h"
#include "stm32l4xx_hal_def.h"
#include "stm32l4xx_hal_gpio.h"
#include "stm32l4xx_hal_spi.h"

// ---------- Colors ----------
const static uint8_t _Color_Background[3] = {14, 14, 16};
const static uint8_t _Color_MenuText[3] = {54, 54, 54};
// ---------- LCD Command registers ----------
#define LCD_Cmd_ColumnAddressSet 0x2A
#define LCD_Cmd_PageAddressSet 0x2B
#define LCD_Cmd_MemoryWrite 0x2C
// ---------- Definitions, global variables ----------
#define LCD_Width 320
#define LCD_Height 480
#define LCD_WindowedLineBuffer_PixelLines 40
// static uint8_t frontBuffer_Pixels[LCD_Width * LCD_WindowedLineBuffer_PixelLines][3];
static uint8_t backBuffer_Pixels[LCD_Width * LCD_WindowedLineBuffer_PixelLines][3];

bool lcd_requestedReprint = false;

#define LCD_VisualLine_MAXLength 100
typedef struct {
    char string[LCD_VisualLine_MAXLength];
    uint8_t x_offset;
    Font *font;
    uint16_t y0_bounds;
    uint16_t y1_bounds;
    uint16_t y0_equalSpacing;
    uint16_t y1_equalSpacing;
} LCD_VisualLine;

#define LCD_WindowedLineBuffer_MaxMenuLines 30
struct Snapshot {
    int16_t lineCount;
    int16_t dirtyRegion_y1;
    LCD_VisualLine menuLines[LCD_WindowedLineBuffer_MaxMenuLines];
};
struct Snapshot _mainSnapshot = {.dirtyRegion_y1 = LCD_Height + 100};

struct ScreenPadding {
    uint8_t top;
    uint8_t bottom;
    uint8_t left;
    uint8_t right;
};
static struct ScreenPadding _LCD_ScreenPadding = {.top = 10, .bottom = 10, .left = 5, .right = 5};
#define LCD_LinePadding 10;

// ---------- Forward declarations ----------
static void lcd_Select();
static void lcd_Unselect();
static void LCD_Reset();
static void LCD_SleepOut();
static void LCD_SetPixelFormat();
static void LCD_SetMemoryAccessControl();
static void LCD_DisplayON();
static void LCD_SetPowerControl();
static void LCD_DisplayInversion();
static void LCD_FrameRateControl();
static void LCD_Gamma();
void LCD_PaintRectangle(uint16_t x0, uint16_t y0, uint16_t width, uint16_t height, uint16_t color);
static void LCD_SetWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
static void lcd_writeSPI_command(uint8_t cmd);

void _lcd_initialize() {
    LCD_Reset();
    LCD_SleepOut();

    lcd_Select();

    LCD_SetPixelFormat();
    LCD_SetMemoryAccessControl();

    LCD_SetPowerControl();
    LCD_DisplayInversion();
    LCD_FrameRateControl();
    LCD_Gamma();

    HAL_GPIO_WritePin(SPI2_LCD_BL_GPIO_Port, SPI2_LCD_BL_Pin, GPIO_PIN_SET);
    LCD_DisplayON();
    HAL_GPIO_WritePin(SPI2_LCD_DC_GPIO_Port, SPI2_LCD_DC_Pin, GPIO_PIN_RESET);
    lcd_Unselect();

    calculateMaxFontHeight(&NanoSansMono_CondensedMedium);
}

void _lcd_clear() {
    // Take binary semaphore for writing/reading menu snapshot
    osSemaphoreAcquire(semBin_menuSnapshotHandle, HAL_MAX_DELAY);

    _mainSnapshot.lineCount = 0;
}
void _lcd_addToSnapshot_print() {
    // Add to last line before '\n' (if exists)
}
static void snapshot_addNewLine(const char *string, Font *font, size_t length, uint8_t x_offset) {
    LCD_VisualLine newLine;

    if (length > LCD_VisualLine_MAXLength - 3) length = LCD_VisualLine_MAXLength - 3;
    memcpy(newLine.string, string, length);
    newLine.string[length] = '\r';
    newLine.string[length + 1] = '\n';
    newLine.string[length + 2] = '\0';
    newLine.x_offset = x_offset;

    newLine.font = font;
    uint16_t y_lineStart = _LCD_ScreenPadding.top;
    if (_mainSnapshot.lineCount > 0) {
        y_lineStart = _mainSnapshot.menuLines[_mainSnapshot.lineCount - 1].y1_equalSpacing;
        y_lineStart += LCD_LinePadding;
    }
    uint8_t fontHeight = font->line_height;
    float extraFloat = font->maxHeight_forClipping - fontHeight;
    uint8_t extra = (uint8_t)(extraFloat * 0.5 + 0.5);
    newLine.y1_equalSpacing = y_lineStart + fontHeight - 1;
    newLine.y0_equalSpacing = y_lineStart;
    int16_t y0 = y_lineStart - extra;
    if (y0 < 0) y0 = 0;
    newLine.y0_bounds = (uint16_t)y0;
    newLine.y1_bounds = newLine.y1_equalSpacing + extra;

    _mainSnapshot.menuLines[_mainSnapshot.lineCount] = newLine;
    _mainSnapshot.lineCount++;
}
static void snapshot_addLineWithWordWrapping(const char *str, Font *currentFont) {
    const char *string_ptr = str;
    char character = *string_ptr++;

    uint8_t advance = (currentFont->glyphs[1].adv_w >> 4) + 1;
    uint16_t bitmapEndPosition_x = _LCD_ScreenPadding.left - 1;
    const char *lineStart = str;
    const char *lastSpace = NULL;
    uint8_t x_offset = 0;
    for (const char *p = str; *p && *p != '\r' && *p != '\n'; ++p) {
        bitmapEndPosition_x += advance;
        if (*p == ' ') lastSpace = p;

        if (bitmapEndPosition_x < LCD_Width - _LCD_ScreenPadding.right) continue;

        if (lineStart != str) {
            x_offset = 3 * advance;
        } else {
            x_offset = 0;
        }
        if (lastSpace != NULL) {
            // Wrap line start to last space
            snapshot_addNewLine(lineStart, currentFont, (size_t)(lastSpace - lineStart), x_offset);
            lineStart = lastSpace + 1;
            lastSpace = NULL;
        } else {
            // Cut the word at the character
            snapshot_addNewLine(lineStart, currentFont, (size_t)(p - lineStart), x_offset);
            lineStart = p + 1;
        }
        bitmapEndPosition_x = _LCD_ScreenPadding.left - 1;
    }
    if (lineStart != str) {
        x_offset = 3 * advance;
    } else {
        x_offset = 0;
    }
    snapshot_addNewLine(lineStart, currentFont, strlen(lineStart), x_offset);
}
void _lcd_addToSnapshot_printLine(const char *str) {
    // Add a line with normal font
    if (_mainSnapshot.lineCount > LCD_WindowedLineBuffer_MaxMenuLines - 1) return;

    snapshot_addLineWithWordWrapping(str, &NanoSansMono_CondensedMedium);
}
void _lcd_addToSnapshot_printBold(const char *str) { _lcd_addToSnapshot_printLine(str); }
void _lcd_drawMenu() {
    // Set trigger for LCD transmit
    lcd_requestedReprint = true;

    // Give binary semaphore for writing/reading menu snapshot
    osSemaphoreRelease(semBin_menuSnapshotHandle);
}

static inline void assignPixel_18bit(const uint8_t *RGB, uint8_t *pixelStart) {
    pixelStart[0] = RGB[0];
    pixelStart[1] = RGB[1];
    pixelStart[2] = RGB[2];
}
void generatePixelBuffer(int16_t window_y0, int16_t window_y1) {
    // Set background color for the whole buffer
    uint16_t bufferSize = LCD_Width * LCD_WindowedLineBuffer_PixelLines;
    for (int i = 0; i < bufferSize; i++) {
        assignPixel_18bit(_Color_Background, backBuffer_Pixels[i]);
    }

    // Process glyphs and modify value for each character in the buffer
    LCD_VisualLine *currentLine;
    for (int i = 0; i < _mainSnapshot.lineCount; i++) {
        currentLine = &_mainSnapshot.menuLines[i];
        if (currentLine->y1_bounds < window_y0) continue;
        if (currentLine->y0_bounds > window_y1) break;

        const Font *currentFont = currentLine->font;
        const char *ch_ptr = currentLine->string;
        const Glyph *glyph;
        const uint8_t *bitmap;
        uint16_t bitmapStartPosition_x = _LCD_ScreenPadding.left - 1 + currentLine->x_offset;
        int16_t baseline_y;
        uint8_t advance = (currentFont->glyphs[1].adv_w >> 4) + 1;
        while (*ch_ptr && *ch_ptr != '\r' && *ch_ptr != '\n') {
            if (bitmapStartPosition_x + advance > LCD_Width - _LCD_ScreenPadding.right) break;
            char character = *ch_ptr;
            ch_ptr++;
            glyph = getGlyphFromChar(character, currentFont);
            bitmap = &currentFont->bitmapArray[glyph->bitmap_index];

            bool debugged = false;

            uint8_t currentByte = *bitmap++;
            uint8_t mask = 0b10000000;
            int16_t pixelAbsolutePosition_x = 0;
            int16_t pixelAbsolutePosition_y = 0;
            int16_t pixelBufferPosition_y = 0;
            uint8_t *pixelStartBuffer;
            baseline_y = currentLine->y0_equalSpacing + currentFont->line_height - currentFont->base_line;
            for (uint8_t row = 0; row < glyph->box_h; row++) {
                for (uint8_t col = 0; col < glyph->box_w; col++) {
                    if (mask == 0) {
                        currentByte = *bitmap++;
                        mask = 0b10000000;
                    }

                    bool pixelOn = currentByte & mask;
                    mask >>= 1;
                    if (!pixelOn) continue;

                    pixelAbsolutePosition_y = baseline_y - glyph->ofs_y - (int16_t)glyph->box_h + (int16_t)row - 1;
                    if (pixelAbsolutePosition_y < window_y0) continue;
                    if (pixelAbsolutePosition_y > window_y1) continue;

                    pixelAbsolutePosition_x = bitmapStartPosition_x + glyph->ofs_x + col;
                    pixelBufferPosition_y = pixelAbsolutePosition_y - window_y0;

                    if (character == '>' && !debugged) {
                        debugged = true;
                        // debug
                    }

                    if (pixelAbsolutePosition_x > LCD_Width - 1) continue;
                    // if (pixelBufferPosition_y < 0) continue;
                    if (pixelBufferPosition_y > LCD_WindowedLineBuffer_PixelLines - 1) continue;
                    uint16_t index = pixelBufferPosition_y * LCD_Width + pixelAbsolutePosition_x;
                    if (index > LCD_Width * LCD_WindowedLineBuffer_PixelLines - 1)
                        index = LCD_Width * LCD_WindowedLineBuffer_PixelLines - 1;
                    pixelStartBuffer = backBuffer_Pixels[index];
                    assignPixel_18bit(_Color_MenuText, pixelStartBuffer);
                }
            }

            // Add advance to starting pixel x
            bitmapStartPosition_x += advance;
        }
    }
}
void transmitSPI_PixelBuffer() {
    // Wait for DMA release (through task notify / pooling)

    // Change front and back buffer pointers

    // Transmits the whole buffer
    lcd_writeSPI_command(LCD_Cmd_MemoryWrite);
    HAL_GPIO_WritePin(SPI2_LCD_DC_GPIO_Port, SPI2_LCD_DC_Pin, GPIO_PIN_SET);
    HAL_SPI_Transmit(&hspi2, (uint8_t *)backBuffer_Pixels, LCD_Width * LCD_WindowedLineBuffer_PixelLines * 3,
                     HAL_MAX_DELAY);
    HAL_GPIO_WritePin(SPI2_LCD_DC_GPIO_Port, SPI2_LCD_DC_Pin, GPIO_PIN_RESET);
}
void _lcd_drawMenuThroughSPI() {
    osSemaphoreAcquire(semBin_menuSnapshotHandle, HAL_MAX_DELAY);
    lcd_requestedReprint = false;

    // Convert created line snapshot to pixels for window in a loop
    // first test without DMA
    lcd_Select();
    int16_t startPixel = 0;
    int16_t lastLine_y1 = _mainSnapshot.menuLines[_mainSnapshot.lineCount - 1].y1_bounds;
    _mainSnapshot.dirtyRegion_y1 =
        _mainSnapshot.dirtyRegion_y1 > lastLine_y1 ? _mainSnapshot.dirtyRegion_y1 : lastLine_y1;
    while (startPixel < LCD_Height && startPixel < _mainSnapshot.dirtyRegion_y1 + 1) {
        int16_t endPixel = startPixel + LCD_WindowedLineBuffer_PixelLines - 1;
        if (endPixel > LCD_Height - 1) endPixel = LCD_Height - 1;

        // if (!dma_finished) osDelay(2);
        // or better wait for task notification from dma isr
        generatePixelBuffer(startPixel, endPixel);
        LCD_SetWindow(0, startPixel, LCD_Width - 1, endPixel);
        transmitSPI_PixelBuffer();
        // osDelay(500);

        startPixel = endPixel + 1;
    }
    lcd_Unselect();
    _mainSnapshot.dirtyRegion_y1 = lastLine_y1;

    osSemaphoreRelease(semBin_menuSnapshotHandle);
}

void _lcd_drawTestScreen() {
    return;
    lcd_Select();
    LCD_PaintRectangle(0, 0, 100, 150, 0xF800);
    lcd_Unselect();
}

static void lcd_writeSPI_command(uint8_t cmd) {
    HAL_GPIO_WritePin(SPI2_LCD_DC_GPIO_Port, SPI2_LCD_DC_Pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(&hspi2, &cmd, 1, HAL_MAX_DELAY);
}
static void lcd_writeSPI_data(const void *data, uint32_t length) {
    HAL_GPIO_WritePin(SPI2_LCD_DC_GPIO_Port, SPI2_LCD_DC_Pin, GPIO_PIN_SET);
    HAL_SPI_Transmit(&hspi2, (uint8_t *)data, length, HAL_MAX_DELAY);
}
static void lcd_Select() { HAL_GPIO_WritePin(SPI2_LCD_CS_GPIO_Port, SPI2_LCD_CS_Pin, GPIO_PIN_RESET); }
static void lcd_Unselect() { HAL_GPIO_WritePin(SPI2_LCD_CS_GPIO_Port, SPI2_LCD_CS_Pin, GPIO_PIN_SET); }

// ---------- LCD pixel write functions ----------
static uint8_t LS_BYTE(uint16_t value) { return value & 0xFF; }
static uint8_t MS_BYTE(uint16_t value) { return (value >> 8) & 0xFF; }
static void LCD_SetWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    uint8_t data[4];

    data[0] = MS_BYTE(x0);
    data[1] = LS_BYTE(x0);
    data[2] = MS_BYTE(x1);
    data[3] = LS_BYTE(x1);
    lcd_writeSPI_command(LCD_Cmd_ColumnAddressSet);
    lcd_writeSPI_data(data, 4);

    data[0] = MS_BYTE(y0);
    data[1] = LS_BYTE(y0);
    data[2] = MS_BYTE(y1);
    data[3] = LS_BYTE(y1);
    lcd_writeSPI_command(LCD_Cmd_PageAddressSet);
    lcd_writeSPI_data(data, 4);
}
void LCD_PaintRectangle(uint16_t x0, uint16_t y0, uint16_t width, uint16_t height, uint16_t color) {
    if (x0 > LCD_Width - 1 || y0 > LCD_Height - 1) {
        // perror("LCD_PaintRectangle(): The x0 or y0 is outside of the screen");
        return;
    }

    uint16_t x1 = x0 + width - 1;
    uint16_t y1 = y0 + height - 1;
    if (x1 > LCD_Width - 1) x1 = LCD_Width - 1;
    if (y1 > LCD_Height - 1) y1 = LCD_Height - 1;

    LCD_SetWindow(x0, y0, x1, y1);

    lcd_writeSPI_command(LCD_Cmd_MemoryWrite);

    uint32_t pixelsAmount = (x1 - x0 + 1) * (y1 - y0 + 1);
    uint8_t pixel[3] = {((color >> 11) & 0x1F) << 1, ((color >> 5) & 0x3F), ((color >> 0) & 0x1F) << 1};
    HAL_GPIO_WritePin(SPI2_LCD_DC_GPIO_Port, SPI2_LCD_DC_Pin, GPIO_PIN_SET);
    for (uint32_t i = 0; i < pixelsAmount; i++) {
        HAL_SPI_Transmit(&hspi2, pixel, 3, HAL_MAX_DELAY);
    }
}

// ---------- LCD Init command registers ----------
#define LCD_Cmd_SoftwareReset 0x01
#define LCD_Cmd_SleepIN 0x10
#define LCD_Cmd_SleepOUT 0x11
#define LCD_Cmd_WritePixelFormat 0x3A
#define LCD_Cmd_DisplayON 0x29
#define LCD_Cmd_MemoryAccessControl 0x36

#define LCD_Cmd_PowerControl1 0xC0
#define LCD_Cmd_PowerControl2 0xC1
#define LCD_Cmd_VCOMControl 0xC5

#define LCD_Cmd_DisplayInversionON 0x21
#define LCD_Cmd_FrameRateControl 0xB1
#define LCD_Cmd_DisplayInversionControl 0xB4

#define LCD_Cmd_PGAMCTRL 0xE0
#define LCD_Cmd_NGAMCTRL 0xE1
// ---------- LCD Init functions ----------
static void LCD_Reset() {
    lcd_Select();
    HAL_GPIO_WritePin(SPI2_LCD_RST_GPIO_Port, SPI2_LCD_RST_Pin, GPIO_PIN_RESET);
    HAL_Delay(5);
    HAL_GPIO_WritePin(SPI2_LCD_RST_GPIO_Port, SPI2_LCD_RST_Pin, GPIO_PIN_SET);
    lcd_writeSPI_command(LCD_Cmd_SoftwareReset);
    lcd_Unselect();
    HAL_Delay(120);
}
static void LCD_SleepOut() {
    lcd_Select();
    lcd_writeSPI_command(LCD_Cmd_SleepOUT);
    lcd_Unselect();
    HAL_Delay(5);
}
static void LCD_SetPixelFormat() {
    lcd_writeSPI_command(LCD_Cmd_WritePixelFormat);
    uint8_t pixelFormat = 0b01100110; // using RGB666
    lcd_writeSPI_data(&pixelFormat, 1);
}
static void LCD_DisplayON() {
    lcd_writeSPI_command(LCD_Cmd_DisplayON);
    HAL_Delay(20);

    //   for (int i = 0; i < 8; i++) {
    //       uint8_t cmd = i % 2 == 0 ? 0x22 : 0x23;
    //       lcd_writeSPI_command(cmd);
    //       HAL_Delay(500);
    //   }
}
static void LCD_SetMemoryAccessControl() {
    lcd_writeSPI_command(LCD_Cmd_MemoryAccessControl);
    uint8_t mcuParameter = 0b10001000;
    lcd_writeSPI_data(&mcuParameter, 1);
}
static void LCD_SetPowerControl() {
    // Power Control 1
    lcd_writeSPI_command(LCD_Cmd_PowerControl1);
    uint8_t data[2] = {0x0F, 0x0f};
    lcd_writeSPI_data(data, 2);
    // Power Control 2
    lcd_writeSPI_command(LCD_Cmd_PowerControl2);
    uint8_t value = 0x47;
    lcd_writeSPI_data(&value, 1);
    // VCOM Control
    lcd_writeSPI_command(LCD_Cmd_VCOMControl);
    uint8_t data2[3] = {0x00, 0x4D, 0x80};
    lcd_writeSPI_data(data2, 3);
}
static void LCD_DisplayInversion() {
    lcd_writeSPI_command(LCD_Cmd_DisplayInversionON);
    lcd_writeSPI_command(LCD_Cmd_DisplayInversionControl);
    uint8_t value = 0x02;
    lcd_writeSPI_data(&value, 1);
}
static void LCD_FrameRateControl() {
    lcd_writeSPI_command(LCD_Cmd_FrameRateControl);
    uint8_t data[2] = {0xB0, 0x11};
    lcd_writeSPI_data(data, 2);
}
static void LCD_Gamma() {
    lcd_writeSPI_command(LCD_Cmd_PGAMCTRL);
    uint8_t pGamma[15] = {0x00, 0x07, 0x0B, 0x03, 0x0F, 0x05, 0x30, 0x56, 0x47, 0x04, 0x0B, 0x0A, 0x2D, 0x37, 0x0F};
    lcd_writeSPI_data(pGamma, 15);
    lcd_writeSPI_command(LCD_Cmd_NGAMCTRL);
    uint8_t nGamma[15] = {0x00, 0x0E, 0x13, 0x04, 0x11, 0x07, 0x39, 0x45, 0x50, 0x07, 0x10, 0x0D, 0x32, 0x36, 0x0F};
    lcd_writeSPI_data(nGamma, 15);
}
