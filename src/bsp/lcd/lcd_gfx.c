/**
 * @file        lcd_gfx.c
 * @brief       单缓冲上的绘制与文字
 */

#include "bsp/lcd/lcd_priv.h"
#include "lcdfont.h"

void lcd_fill_span(uint16_t *dst, uint32_t count, uint16_t color)
{
    uint32_t pair;
    uint32_t *p32;

    if (dst == NULL || count == 0)
    {
        return;
    }

    if (((uintptr_t)dst & 2u) != 0u)
    {
        *dst++ = color;
        count--;
    }

    pair = ((uint32_t)color << 16) | color;
    p32 = (uint32_t *)(void *)dst;
    while (count >= 2)
    {
        *p32++ = pair;
        count -= 2;
    }
    if (count != 0)
    {
        *(uint16_t *)(void *)p32 = color;
    }
}

void lcd_clear(uint16_t color)
{
    uint32_t n = (uint32_t)lcd_self.width * lcd_self.height;

    lcd_lock_fb();
    if (n > LCD_PIXEL_MAX)
    {
        n = LCD_PIXEL_MAX;
    }
    lcd_fill_span(lcd_buf, n, color);
}

void lcd_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint16_t color)
{
    uint16_t y;
    uint16_t span;

    lcd_lock_fb();

    if (sx > ex)
    {
        uint16_t t = sx;
        sx = ex;
        ex = t;
    }
    if (sy > ey)
    {
        uint16_t t = sy;
        sy = ey;
        ey = t;
    }
    if (sx >= lcd_self.width || sy >= lcd_self.height)
    {
        return;
    }
    if (ex >= lcd_self.width)
    {
        ex = (uint16_t)(lcd_self.width - 1);
    }
    if (ey >= lcd_self.height)
    {
        ey = (uint16_t)(lcd_self.height - 1);
    }

    span = (uint16_t)(ex - sx + 1);
    if (sx == 0 && ex == (uint16_t)(lcd_self.width - 1))
    {
        lcd_fill_span(&lcd_buf[(uint32_t)sy * lcd_self.width],
                      (uint32_t)span * (uint32_t)(ey - sy + 1), color);
        return;
    }

    for (y = sy; y <= ey; y++)
    {
        lcd_fill_span(&lcd_buf[(uint32_t)y * lcd_self.width + sx], span, color);
    }
}

void lcd_draw_pixel(uint16_t x, uint16_t y, uint16_t color)
{
    lcd_lock_fb();
    lcd_put_pixel(x, y, color);
}

uint16_t lcd_get_pixel(uint16_t x, uint16_t y)
{
    lcd_lock_fb();
    if (x >= lcd_self.width || y >= lcd_self.height)
    {
        return 0;
    }
    return lcd_buf[(uint32_t)y * lcd_self.width + x];
}

void lcd_draw_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
    int16_t x = (int16_t)x1;
    int16_t y = (int16_t)y1;
    int16_t dx = (int16_t)((int16_t)x2 - (int16_t)x1);
    int16_t dy = (int16_t)((int16_t)y2 - (int16_t)y1);
    int16_t sx = (dx >= 0) ? 1 : -1;
    int16_t sy = (dy >= 0) ? 1 : -1;
    int16_t err;

    if (dx < 0)
    {
        dx = (int16_t)-dx;
    }
    if (dy < 0)
    {
        dy = (int16_t)-dy;
    }
    err = (int16_t)((dx > dy) ? (dx / 2) : -(dy / 2));

    lcd_lock_fb();
    for (;;)
    {
        lcd_put_pixel((uint16_t)x, (uint16_t)y, color);
        if (x == (int16_t)x2 && y == (int16_t)y2)
        {
            break;
        }
        {
            int16_t e2 = err;
            if (e2 > -dx)
            {
                err = (int16_t)(err - dy);
                x = (int16_t)(x + sx);
            }
            if (e2 < dy)
            {
                err = (int16_t)(err + dx);
                y = (int16_t)(y + sy);
            }
        }
    }
}

void lcd_draw_hline(uint16_t x, uint16_t y, uint16_t len, uint16_t color)
{
    if ((len == 0) || (x >= lcd_self.width) || (y >= lcd_self.height))
    {
        return;
    }
    lcd_fill(x, y, (uint16_t)(x + len - 1), y, color);
}

void lcd_draw_rectangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color)
{
    uint16_t w;

    if (x0 > x1)
    {
        uint16_t t = x0;
        x0 = x1;
        x1 = t;
    }
    if (y0 > y1)
    {
        uint16_t t = y0;
        y0 = y1;
        y1 = t;
    }

    w = (uint16_t)(x1 - x0 + 1);
    lcd_draw_hline(x0, y0, w, color);
    lcd_draw_hline(x0, y1, w, color);
    if (y1 > (uint16_t)(y0 + 1))
    {
        lcd_fill(x0, (uint16_t)(y0 + 1), x0, (uint16_t)(y1 - 1), color);
        lcd_fill(x1, (uint16_t)(y0 + 1), x1, (uint16_t)(y1 - 1), color);
    }
}

void lcd_draw_circle(uint16_t x0, uint16_t y0, uint16_t r, uint16_t color)
{
    int16_t x = 0;
    int16_t y = (int16_t)r;
    int16_t d = (int16_t)(1 - (int16_t)r);
    int16_t cx = (int16_t)x0;
    int16_t cy = (int16_t)y0;

    lcd_lock_fb();
    while (x <= y)
    {
        lcd_put_pixel((uint16_t)(cx + x), (uint16_t)(cy + y), color);
        lcd_put_pixel((uint16_t)(cx - x), (uint16_t)(cy + y), color);
        lcd_put_pixel((uint16_t)(cx + x), (uint16_t)(cy - y), color);
        lcd_put_pixel((uint16_t)(cx - x), (uint16_t)(cy - y), color);
        lcd_put_pixel((uint16_t)(cx + y), (uint16_t)(cy + x), color);
        lcd_put_pixel((uint16_t)(cx - y), (uint16_t)(cy + x), color);
        lcd_put_pixel((uint16_t)(cx + y), (uint16_t)(cy - x), color);
        lcd_put_pixel((uint16_t)(cx - y), (uint16_t)(cy - x), color);

        x++;
        if (d < 0)
        {
            d = (int16_t)(d + 2 * x + 1);
        }
        else
        {
            y--;
            d = (int16_t)(d + 2 * (x - y) + 1);
        }
    }
}

void lcd_show_char(uint16_t x, uint16_t y, uint8_t chr, uint8_t size, uint8_t mode, uint16_t color)
{
    uint8_t *pfont;
    uint8_t col;
    uint8_t row;
    uint8_t char_w = (uint8_t)(size / 2);
    uint8_t char_h = size;
    uint8_t bytes_per_row;

    if (chr < ' ' || chr > '~')
    {
        return;
    }
    if ((char_w == 0) || (char_w > lcd_self.width) || (char_h > lcd_self.height))
    {
        return;
    }
    if ((x > (uint16_t)(lcd_self.width - char_w)) || (y > (uint16_t)(lcd_self.height - char_h)))
    {
        return;
    }

    chr = (uint8_t)(chr - ' ');
    switch (size)
    {
        case 12:
            pfont = (uint8_t *)asc2_1206[chr];
            break;
        case 16:
            pfont = (uint8_t *)asc2_1608[chr];
            break;
        case 24:
            pfont = (uint8_t *)asc2_2412[chr];
            break;
        case 32:
            pfont = (uint8_t *)asc2_3216[chr];
            break;
        default:
            return;
    }

    lcd_lock_fb();
    bytes_per_row = (uint8_t)((char_w + 7) / 8);
    for (row = 0; row < char_h; row++)
    {
        for (col = 0; col < char_w; col++)
        {
            uint8_t temp = pfont[row * bytes_per_row + col / 8];

            if (temp & (uint8_t)(0x80 >> (col % 8)))
            {
                lcd_put_pixel((uint16_t)(x + col), (uint16_t)(y + row), color);
            }
            else if (mode == 0)
            {
                lcd_put_pixel((uint16_t)(x + col), (uint16_t)(y + row), WHITE);
            }
        }
    }
}

static uint32_t lcd_pow10(uint8_t n)
{
    static const uint32_t tab[] = {
        1ul, 10ul, 100ul, 1000ul, 10000ul,
        100000ul, 1000000ul, 10000000ul,
        100000000ul, 1000000000ul
    };

    return (n < 10) ? tab[n] : 0;
}

void lcd_show_num(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size, uint16_t color)
{
    uint8_t t;
    uint8_t enshow = 0;

    lcd_lock_fb();
    for (t = 0; t < len; t++)
    {
        uint8_t temp = (uint8_t)((num / lcd_pow10((uint8_t)(len - t - 1))) % 10);

        if (enshow == 0 && t < (uint8_t)(len - 1))
        {
            if (temp == 0)
            {
                lcd_show_char((uint16_t)(x + (size / 2) * t), y, ' ', size, 0, color);
                continue;
            }
            enshow = 1;
        }
        lcd_show_char((uint16_t)(x + (size / 2) * t), y, (uint8_t)(temp + '0'), size, 0, color);
    }
}

void lcd_show_xnum(uint16_t x, uint16_t y, uint32_t num, uint8_t len, uint8_t size, uint8_t mode, uint16_t color)
{
    uint8_t t;
    uint8_t enshow = 0;

    lcd_lock_fb();
    for (t = 0; t < len; t++)
    {
        uint8_t temp = (uint8_t)((num / lcd_pow10((uint8_t)(len - t - 1))) % 10);

        if (enshow == 0 && t < (uint8_t)(len - 1))
        {
            if (temp == 0)
            {
                char ch = (mode & 0x80) ? '0' : ' ';
                lcd_show_char((uint16_t)(x + (size / 2) * t), y, (uint8_t)ch, size, (uint8_t)(mode & 0x01), color);
                continue;
            }
            enshow = 1;
        }
        lcd_show_char((uint16_t)(x + (size / 2) * t), y, (uint8_t)(temp + '0'), size, (uint8_t)(mode & 0x01), color);
    }
}

void lcd_show_string(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t size, char *p, uint16_t color)
{
    uint16_t x0 = x;
    const unsigned char *s = (const unsigned char *)p;

    if (s == NULL)
    {
        return;
    }

    width = (uint16_t)(width + x);
    height = (uint16_t)(height + y);

    lcd_lock_fb();
    while (*s >= ' ' && *s <= '~')
    {
        if (x >= width)
        {
            x = x0;
            y = (uint16_t)(y + size);
        }
        if (y >= height)
        {
            break;
        }
        lcd_show_char(x, y, *s, size, 1, color);
        x = (uint16_t)(x + size / 2);
        s++;
    }
}
