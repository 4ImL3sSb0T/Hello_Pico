/**
 * @file        lcd_priv.h
 * @brief       LCD 驱动内部接口（总线 / 面板 / 帧缓冲）
 */

#ifndef LCD_PRIV_H
#define LCD_PRIV_H

#include "bsp/lcd/lcd.h"
#include "bsp/lcd/spi.h"

#define LCD_PIN_BL              25
#define LCD_PIN_DC              8
#define LCD_PIN_CS              9

#define LCD_DC(x)               gpio_put(LCD_PIN_DC, (x))
#define LCD_CS(x)               gpio_put(LCD_PIN_CS, (x))
#define LCD_BL(x)               gpio_put(LCD_PIN_BL, (x))

#define LCD_PIXEL_MAX           (240 * 135)

extern uint16_t lcd_buf[LCD_PIXEL_MAX];

void lcd_bus_init(void);
void lcd_spi_8bit(void);
void lcd_spi_16bit(void);
void lcd_dma_start(const uint16_t *src, size_t count);
void lcd_write_cmd(uint8_t cmd);
void lcd_write_data(const uint8_t *data, int len);

void lcd_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);

/* 单缓冲：改 lcd_buf 前必须等 DMA 发完。高层绘制入口调一次即可。 */
static inline void lcd_lock_fb(void)
{
    lcd_wait_idle();
}

static inline void lcd_put_pixel(uint16_t x, uint16_t y, uint16_t color)
{
    if (x >= lcd_self.width || y >= lcd_self.height)
    {
        return;
    }
    lcd_buf[(uint32_t)y * lcd_self.width + x] = color;
}

void lcd_fill_span(uint16_t *dst, uint32_t count, uint16_t color);

#endif
