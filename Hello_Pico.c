#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/async_context_poll.h"
#include "bsp/led/led.h"
#include "bsp/lcd/spi.h"
#include "bsp/lcd/lcd.h"
#include "bsp/input/key.h"

#define DISPLAY_HZ              60
#define DISPLAY_INTERVAL_US     (1000000u / DISPLAY_HZ)
#define STATS_INTERVAL_MS       1000u

typedef struct
{
    int16_t x;
    int16_t y;
    int16_t vx;
    int16_t vy;
    int16_t r;
    uint16_t color;
} ball_t;

static ball_t balls[] = {
    { 30,  40,  3,  2, 12, RED },
    { 120, 70, -2,  3, 10, GREEN },
    { 200, 50,  4, -3,  8, BLUE },
    { 80, 100, -3, -2,  9, YELLOW },
};
static const int ball_count = (int)(sizeof(balls) / sizeof(balls[0]));

static int16_t bar_x = 0;
static int16_t bar_vx = 5;
static uint32_t frames;
static uint32_t fps;
static char fps_text[24];
static char key_text[12] = "KEY0";
static uint16_t key_bar_color = 0x18E3;

static async_context_poll_t loop;
static async_at_time_worker_t display_worker;
static async_at_time_worker_t stats_worker;

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

static void app_simulate(void)
{
    int i;
    int16_t max_x = (int16_t)lcd_self.width;
    int16_t max_y = (int16_t)lcd_self.height;

    bar_x += bar_vx;
    if (bar_x < 0 || bar_x > max_x - 40) {
        bar_vx = (int16_t)-bar_vx;
        bar_x += bar_vx;
    }

    for (i = 0; i < ball_count; i++) {
        ball_t *b = &balls[i];

        b->x += b->vx;
        b->y += b->vy;
        if (b->x < b->r || b->x > max_x - 1 - b->r) {
            b->vx = (int16_t)-b->vx;
            b->x += b->vx;
        }
        if (b->y < 32 + b->r || b->y > max_y - 1 - b->r) {
            b->vy = (int16_t)-b->vy;
            b->y += b->vy;
        }
    }
}

static void app_draw(void)
{
    int i;
    uint16_t max_x = lcd_self.width;

    lcd_clear(BLACK);
    lcd_fill((uint16_t)bar_x, 22, (uint16_t)(bar_x + 39), 28, CYAN);
    for (i = 0; i < ball_count; i++) {
        fill_circle(balls[i].x, balls[i].y, balls[i].r, balls[i].color);
    }
    lcd_fill(0, 0, (uint16_t)(max_x - 1), 18, key_bar_color);
    snprintf(fps_text, sizeof(fps_text), "FPS:%lu %s %u",
             (unsigned long)fps, key_text, (unsigned)key_raw_level(KEY_ID_0));
    lcd_show_string(4, 2, 232, 16, 16, fps_text, WHITE);
    lcd_draw_hline(0, 19, max_x, GRAY);
}

static void display_work(async_context_t *context, async_at_time_worker_t *worker)
{
    absolute_time_t next;

    app_simulate();
    app_draw();
    lcd_flush();
    frames++;

    /* 按 60Hz 相位重挂；超时则从现在再走，不追帧。 */
    next = delayed_by_us(worker->next_time, DISPLAY_INTERVAL_US);
    if (absolute_time_diff_us(get_absolute_time(), next) < 0) {
        next = get_absolute_time();
    }
    async_context_add_at_time_worker_at(context, worker, next);
}

static void stats_work(async_context_t *context, async_at_time_worker_t *worker)
{
    fps = frames;
    frames = 0;
    LED_TOGGLE();
    async_context_add_at_time_worker_in_ms(context, worker, STATS_INTERVAL_MS);
}

static void on_key0(Button *btn)
{
    switch (button_get_event(btn)) {
    case BTN_PRESS_DOWN:
        key_bar_color = BLUE;
        snprintf(key_text, sizeof(key_text), "DOWN");
        break;
    case BTN_SINGLE_CLICK:
        key_bar_color = GREEN;
        snprintf(key_text, sizeof(key_text), "CLICK");
        break;
    case BTN_DOUBLE_CLICK:
        key_bar_color = YELLOW;
        snprintf(key_text, sizeof(key_text), "DBL");
        break;
    case BTN_LONG_PRESS_START:
        key_bar_color = RED;
        snprintf(key_text, sizeof(key_text), "LONG");
        break;
    default:
        break;
    }
    printf("KEY0 %s\n", key_text);
}

int main(void)
{
    stdio_init_all();
    led_init();
    spi1_init();
    lcd_init();
    key_init();

    if (!async_context_poll_init_with_defaults(&loop)) {
        return 1;
    }

    key_bind(&loop.core);
    key_attach(KEY_ID_0, BTN_PRESS_DOWN, on_key0);
    key_attach(KEY_ID_0, BTN_SINGLE_CLICK, on_key0);
    key_attach(KEY_ID_0, BTN_DOUBLE_CLICK, on_key0);
    key_attach(KEY_ID_0, BTN_LONG_PRESS_START, on_key0);

    display_worker.do_work = display_work;
    stats_worker.do_work = stats_work;
    async_context_add_at_time_worker_in_ms(&loop.core, &display_worker, 0);
    async_context_add_at_time_worker_in_ms(&loop.core, &stats_worker, STATS_INTERVAL_MS);

    while (true) {
        absolute_time_t next;

        async_context_poll(&loop.core);
        next = loop.core.next_time;
        /* SDK 2.3.0 + RP2350：wait_for_work_until / sleep_until 可能 WFE
         * 死等（硬件 alarm 未 armed）。busy_wait 只看 timer。 */
        if (is_at_the_end_of_time(next)) {
            next = make_timeout_time_ms(5);
        }
        if (!time_reached(next)) {
            busy_wait_until(next);
        }
    }
}
