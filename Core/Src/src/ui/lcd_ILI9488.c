#include <stdint.h>

#include "cli_menu.h"
#include "lcd_ILI9488.h"
#include "main.h"
#include "spi.h"
#include "stm32l4xx_hal.h"
#include "stm32l4xx_hal_def.h"
#include "stm32l4xx_hal_gpio.h"
#include "stm32l4xx_hal_spi.h"

// ---------- Definitions, global variables ----------
#define LCD_Width 320
#define LCD_Height 480
#define LCD_WindowedLineBuffer_PixelLines 12
static uint8_t frameBuffer_Pixels[LCD_Width * LCD_WindowedLineBuffer_PixelLines];

#define LCD_WindowedLineBuffer_MenuLines 2

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
    lcd_Unselect();
}

void _lcd_drawMenu(Menu *menu) {}
void _lcd_drawLine(const char lineString[], Font *font) {}

void _lcd_drawTestScreen() {
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

// ---------- LCD Command registers ----------
#define LCD_Cmd_ColumnAddressSet 0x2A
#define LCD_Cmd_PageAddressSet 0x2B
#define LCD_Cmd_MemoryWrite 0x2C
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
    uint8_t mcuParameter = 0b00001000;
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
