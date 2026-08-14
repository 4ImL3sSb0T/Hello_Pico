/**
 * @file        lcd.h
 * @brief       ST7789 1.14" (240x135) 单缓冲驱动
 *
 * 应用只写帧缓冲，再 lcd_flush / lcd_flush_rect。
 * 单缓冲：flush 启动 DMA 后立刻返回；下一次绘制会等 DMA 结束。
 */

#ifndef __LCD_H__
#define __LCD_H__

#include "pico/stdlib.h"

/* 常用颜色 (RGB565) */
#define WHITE           0xFFFF
#define BLACK           0x0000
#define RED             0xF800
#define GREEN           0x07E0
#define BLUE            0x001F
#define MAGENTA         0xF81F
#define YELLOW          0xFFE0
#define CYAN            0x07FF

#define BROWN           0xBC40
#define BRRED           0xFC07
#define GRAY            0x8430
#define DARKBLUE        0x01CF
#define LIGHTBLUE       0x7D7C
#define GRAYBLUE        0x5458
#define LIGHTGREEN      0x841F
#define LGRAY           0xC618
#define LGRAYBLUE       0xA651
#define LBBLUE          0x2B12

typedef struct
{
    uint16_t width;
    uint16_t height;
    uint8_t  dir;       /* 0 竖屏 135x240，1 横屏 240x135 */
} lcd_obj_t;

extern lcd_obj_t lcd_self;

void lcd_init(void);
void lcd_on(void);
void lcd_off(void);
void lcd_display_dir(uint8_t dir);

void lcd_clear(uint16_t color);
void lcd_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint16_t color);
void lcd_draw_pixel(uint16_t x, uint16_t y, uint16_t color);
void lcd_draw_hline(uint16_t x, uint16_t y, uint16_t len, uint16_t color);
void lcd_draw_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void lcd_draw_rectangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color);
void lcd_draw_circle(uint16_t x0, uint16_t y0, uint16_t r, uint16_t color);

void lcd_show_char(uint16_t x, uint16_t y, uint8_t chr, uint8_t size, uint8_t mode, uint16_t color);
void lcd_show_num(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size, uint16_t color);
void lcd_show_xnum(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size, uint8_t mode, uint16_t color);
void lcd_show_string(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t size, char *p, uint16_t color);

void lcd_wait_idle(void);
bool lcd_is_busy(void);
void lcd_flush(void);
void lcd_flush_rect(uint16_t x, uint16_t y, uint16_t xend, uint16_t yend);

#endif
