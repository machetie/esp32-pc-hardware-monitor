#pragma once
#include <Arduino.h>
#include <SPI.h>
// Display native dimensions (portrait mode - hardware level)
// LVGL will handle rotation to landscape in software
#define LCD_WIDTH   172 //LCD width (native portrait)
#define LCD_HEIGHT  320 //LCD height (native portrait)

#define SPIFreq                        80000000
#define EXAMPLE_PIN_NUM_MISO           5
#define EXAMPLE_PIN_NUM_MOSI           6
#define EXAMPLE_PIN_NUM_SCLK           7
#define EXAMPLE_PIN_NUM_LCD_CS         14
#define EXAMPLE_PIN_NUM_LCD_DC         15
#define EXAMPLE_PIN_NUM_LCD_RST        21
#define EXAMPLE_PIN_NUM_BK_LIGHT       22
#define Frequency       1000
#define Resolution      10

#define VERTICAL   0
#define HORIZONTAL 1

// Offsets for portrait orientation (native)
#define Offset_X 34
#define Offset_Y 0


void LCD_SetCursor(uint16_t x1, uint16_t y1, uint16_t x2,uint16_t y2);

void LCD_Init(void);
void LCD_SetCursor(uint16_t Xstart, uint16_t Ystart, uint16_t Xend, uint16_t  Yend);
void LCD_addWindow(uint16_t Xstart, uint16_t Ystart, uint16_t Xend, uint16_t Yend,uint16_t* color);

void Backlight_Init(void);
void Set_Backlight(uint8_t Light);
