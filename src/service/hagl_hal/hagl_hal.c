/**
 * @file        hagl_hal.c
 * @brief       把 HAGL 接到本板单缓冲：像素进 lcd_buf，flush 进 DMA
 */

#include "hagl_hal.h"

#include "bsp/lcd/lcd.h"
#include "rgb565.h"

static void put_pixel(void *self, int16_t x0, int16_t y0, hagl_color_t color)
{
    (void)self;
    if ((x0 < 0) || (y0 < 0)) {
        return;
    }
    lcd_draw_pixel((uint16_t)x0, (uint16_t)y0, color);
}

static hagl_color_t get_pixel(void *self, int16_t x0, int16_t y0)
{
    (void)self;
    if ((x0 < 0) || (y0 < 0)) {
        return 0;
    }
    return lcd_get_pixel((uint16_t)x0, (uint16_t)y0);
}

static void hline(void *self, int16_t x0, int16_t y0, uint16_t width, hagl_color_t color)
{
    (void)self;
    if ((width == 0) || (x0 < 0) || (y0 < 0)) {
        return;
    }
    lcd_draw_hline((uint16_t)x0, (uint16_t)y0, width, color);
}

static void vline(void *self, int16_t x0, int16_t y0, uint16_t height, hagl_color_t color)
{
    (void)self;
    if ((height == 0) || (x0 < 0) || (y0 < 0)) {
        return;
    }
    lcd_fill((uint16_t)x0, (uint16_t)y0, (uint16_t)x0,
             (uint16_t)(y0 + (int16_t)height - 1), color);
}

static size_t flush(void *self)
{
    (void)self;
    lcd_flush();
    return (size_t)lcd_self.width * (size_t)lcd_self.height * sizeof(uint16_t);
}

static hagl_color_t make_color(void *self, uint8_t r, uint8_t g, uint8_t b)
{
    (void)self;
    return rgb565(r, g, b);
}

void hagl_hal_init(hagl_backend_t *backend)
{
    if ((lcd_self.width == 0) || (lcd_self.height == 0)) {
        return;
    }

    backend->width = (int16_t)lcd_self.width;
    backend->height = (int16_t)lcd_self.height;
    backend->depth = 16;
    backend->put_pixel = put_pixel;
    backend->get_pixel = get_pixel;
    backend->color = make_color;
    backend->hline = hline;
    backend->vline = vline;
    backend->flush = flush;
}
