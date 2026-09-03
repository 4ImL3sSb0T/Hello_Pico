#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/async_context_poll.h"
#include "bsp/led/led.h"
#include "bsp/lcd/spi.h"
#include "bsp/lcd/lcd.h"
#include "bsp/input/key.h"
#include "bsp/pwm/pwm_out.h"
#include "bsp/adc/adc_in.h"
#include "app/eload/eload.h"
#include "hagl.h"
#include "service/fs/fs.h"

#define DISPLAY_HZ              60
#define DISPLAY_INTERVAL_US     (1000000u / DISPLAY_HZ)
#define STATS_INTERVAL_MS       1000u

static uint32_t frames;
static uint32_t fps;
static async_context_poll_t loop;
static async_at_time_worker_t display_worker;
static async_at_time_worker_t stats_worker;
static hagl_backend_t *display;
uint32_t boot_count = 0;

static void display_work(async_context_t *context, async_at_time_worker_t *worker)
{
    absolute_time_t next;

    eload_sample();
    eload_draw(display);
    hagl_flush(display);
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
    printf("eload fps=%lu duty=%u%% adc=%lu mV raw=%u\n",
           (unsigned long)fps,
           (unsigned)eload_duty_percent(),
           (unsigned long)eload_adc_mv(),
           (unsigned)eload_adc_raw());
    async_context_add_at_time_worker_in_ms(context, worker, STATS_INTERVAL_MS);
}

int main(void)
{
    stdio_init_all();
    led_init();
    spi1_init();
    lcd_init();
    display = hagl_init();
    if (!display || display->width == 0 || display->height == 0) {
        return 1;
    }
    key_init();
    pwm_out_init();
    adc_in_init();
    eload_init();

    int fs_ret = fs_init();

    if (fs_ret != LFS_ERR_OK) {
        printf("fs_init failed: %d\n", fs_ret);
        return 1;
    }

    lfs_t *lfs_handle = fs_get_handle();
    lfs_file_t file;

    lfs_mkdir(lfs_handle, "/log");

    int ret = lfs_file_open(lfs_handle, &file, "/log/bootcount.txt", LFS_O_RDWR | LFS_O_CREAT);

    if (ret == LFS_ERR_OK) {

        lfs_file_read(lfs_handle, &file, &boot_count, sizeof(boot_count));
        boot_count++;

        lfs_file_rewind(lfs_handle, &file);
        lfs_file_write(lfs_handle, &file, &boot_count, sizeof(boot_count));
        lfs_file_close(lfs_handle, &file);

        printf("Boot count: %u\n", boot_count);
    } else {
        printf("Failed to open bootcount.txt: %d\n", ret);
    }

    printf("eload PWM GP%u @ %lu Hz, ADC GP%u, duty max 20%%\n",
           (unsigned)PWM_OUT_GPIO_PIN,
           (unsigned long)pwm_out_get_freq_hz(),
           (unsigned)ADC_IN_GPIO_PIN);

    if (!async_context_poll_init_with_defaults(&loop)) {
        return 1;
    }

    key_bind(&loop.core);
    eload_bind_keys();

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
