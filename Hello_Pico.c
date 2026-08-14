#include <stdio.h>
#include "pico/stdlib.h"
#include "bsp/led/led.h"
#include "bsp/lcd/spi.h"
#include "bsp/lcd/lcd.h"

typedef struct
{
    int16_t x;
    int16_t y;
    int16_t vx;
    int16_t vy;
    int16_t r;
    uint16_t color;
} ball_t;

static void fill_circle(int16_t cx, int16_t cy, int16_t r, uint16_t color)
{
    int16_t y;

    for (y = -r; y <= r; y++)
    {
        int16_t x;
        int16_t span = 0;

        for (x = 0; x <= r; x++)
        {
            if ((int32_t)x * x + (int32_t)y * y <= (int32_t)r * r)
            {
                span = x;
            }
        }
        {
            int16_t x0 = (int16_t)(cx - span);
            int16_t x1 = (int16_t)(cx + span);
            int16_t yy = (int16_t)(cy + y);

            if (yy < 0) {
                continue;
            }
            if (x0 < 0) {
                x0 = 0;
            }
            if (x1 < x0) {
                continue;
            }
            lcd_fill((uint16_t)x0, (uint16_t)yy, (uint16_t)x1, (uint16_t)yy, color);
        }
    }
}

int main()
{
    ball_t balls[] = {
        { 30,  40,  3,  2, 12, RED },
        { 120, 70, -2,  3, 10, GREEN },
        { 200, 50,  4, -3,  8, BLUE },
        { 80, 100, -3, -2,  9, YELLOW },
    };
    const int ball_count = (int)(sizeof(balls) / sizeof(balls[0]));
    uint32_t frames = 0;
    uint32_t fps = 0;
    uint64_t fps_t0 = time_us_64();
    char fps_text[16];
    int16_t bar_x = 0;
    int16_t bar_vx = 5;

    stdio_init_all();
    led_init();
    spi1_init();
    lcd_init();

    while (true) {
        int i;
        uint16_t max_x = lcd_self.width;
        uint16_t max_y = lcd_self.height;

        lcd_clear(BLACK);
        lcd_fill((uint16_t)bar_x, 22, (uint16_t)(bar_x + 39), 28, CYAN);
        for (i = 0; i < ball_count; i++) {
            fill_circle(balls[i].x, balls[i].y, balls[i].r, balls[i].color);
        }
        lcd_fill(0, 0, max_x - 1, 18, 0x18E3);
        snprintf(fps_text, sizeof(fps_text), "FPS:%lu", (unsigned long)fps);
        lcd_show_string(4, 2, 120, 16, 16, fps_text, WHITE);
        lcd_draw_hline(0, 19, max_x, GRAY);
        lcd_flush();

        bar_x += bar_vx;
        if (bar_x < 0 || bar_x > (int16_t)max_x - 40) {
            bar_vx = -bar_vx;
            bar_x += bar_vx;
        }
        for (i = 0; i < ball_count; i++) {
            ball_t *b = &balls[i];

            b->x += b->vx;
            b->y += b->vy;
            if (b->x < b->r || b->x > (int16_t)max_x - 1 - b->r) {
                b->vx = -b->vx;
                b->x += b->vx;
            }
            if (b->y < 32 + b->r || b->y > (int16_t)max_y - 1 - b->r) {
                b->vy = -b->vy;
                b->y += b->vy;
            }
        }

        frames++;
        {
            uint64_t now = time_us_64();
            if (now - fps_t0 >= 1000000) {
                fps = frames;
                frames = 0;
                fps_t0 = now;
                LED_TOGGLE();
            }
        }
    }
}
