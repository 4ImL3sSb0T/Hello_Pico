/**
 * @file        key.c
 * @brief       KEY1/KEY0（GPIO2）异步适配：5ms MultiButton tick
 *
 * MultiButton 消抖发生在 IDLE，必须持续打拍，不能按“空闲就停”。
 */

#include "bsp/input/key.h"

#include "pico/stdlib.h"
#include "hardware/gpio.h"

static const uint8_t key_pins[KEY_COUNT] = {
    [KEY_ID_0] = KEY0_GPIO_PIN,
};

static Button s_btn[KEY_COUNT];
static async_at_time_worker_t s_tick_worker;

static uint8_t key_read_level(uint8_t button_id)
{
    if (button_id >= KEY_COUNT) {
        return (uint8_t)!KEY0_ACTIVE_LEVEL;
    }
    return (uint8_t)gpio_get(key_pins[button_id]);
}

static void key_tick_work(async_context_t *context, async_at_time_worker_t *worker)
{
    absolute_time_t next;

    button_ticks();

    next = delayed_by_us(worker->next_time, (uint64_t)TICKS_INTERVAL * 1000u);
    if (absolute_time_diff_us(get_absolute_time(), next) < 0) {
        next = get_absolute_time();
    }
    async_context_add_at_time_worker_at(context, worker, next);
}

void key_init(void)
{
    uint8_t i;

    for (i = 0; i < KEY_COUNT; i++) {
        gpio_init(key_pins[i]);
        gpio_set_dir(key_pins[i], GPIO_IN);
        gpio_pull_up(key_pins[i]);
        button_init(&s_btn[i], key_read_level, KEY0_ACTIVE_LEVEL, i);
        button_start(&s_btn[i]);
    }
}

bool key_bind(async_context_t *ctx)
{
    if (!ctx || s_tick_worker.do_work) {
        return false;
    }

    s_tick_worker.do_work = key_tick_work;
    return async_context_add_at_time_worker_in_ms(ctx, &s_tick_worker, 0);
}

void key_attach(key_id_t id, ButtonEvent event, BtnCallback cb)
{
    if (id >= KEY_COUNT) {
        return;
    }
    button_attach(&s_btn[id], event, cb);
}

void key_detach(key_id_t id, ButtonEvent event)
{
    if (id >= KEY_COUNT) {
        return;
    }
    button_detach(&s_btn[id], event);
}

int key_is_pressed(key_id_t id)
{
    if (id >= KEY_COUNT) {
        return -1;
    }
    return button_is_pressed(&s_btn[id]);
}

uint8_t key_raw_level(key_id_t id)
{
    return key_read_level((uint8_t)id);
}

Button *key_handle(key_id_t id)
{
    if (id >= KEY_COUNT) {
        return NULL;
    }
    return &s_btn[id];
}
