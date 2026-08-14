/**
 * @file        key.c
 * @brief       KEY0 异步适配：GPIO IRQ -> when_pending -> 5ms MultiButton tick
 */

#include "bsp/input/key.h"

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"

static const uint8_t key_pins[KEY_COUNT] = {
    [KEY_ID_0] = KEY0_GPIO_PIN,
};

static Button s_btn[KEY_COUNT];
static async_context_t *s_ctx;
static async_when_pending_worker_t s_pending_worker;
static async_at_time_worker_t s_tick_worker;
static bool s_tick_armed;

static uint8_t key_read_level(uint8_t button_id)
{
    if (button_id >= KEY_COUNT) {
        return (uint8_t)!KEY0_ACTIVE_LEVEL;
    }
    return (uint8_t)gpio_get(key_pins[button_id]);
}

static bool key_all_idle(void)
{
    uint8_t i;

    for (i = 0; i < KEY_COUNT; i++) {
        if (s_btn[i].state != BTN_STATE_IDLE) {
            return false;
        }
    }
    return true;
}

static void key_arm_ticks(async_context_t *context)
{
    if (s_tick_armed) {
        return;
    }
    s_tick_armed = true;
    async_context_add_at_time_worker_in_ms(context, &s_tick_worker, 0);
}

static void key_pending_work(async_context_t *context, async_when_pending_worker_t *worker)
{
    (void)worker;
    key_arm_ticks(context);
}

static void key_tick_work(async_context_t *context, async_at_time_worker_t *worker)
{
    absolute_time_t next;

    button_ticks();
    if (key_all_idle()) {
        s_tick_armed = false;
        return;
    }

    next = delayed_by_us(worker->next_time, (uint64_t)TICKS_INTERVAL * 1000u);
    if (absolute_time_diff_us(get_absolute_time(), next) < 0) {
        next = get_absolute_time();
    }
    async_context_add_at_time_worker_at(context, worker, next);
}

static void key_gpio_irq(uint gpio, uint32_t events)
{
    (void)gpio;
    (void)events;
    if (s_ctx) {
        async_context_set_work_pending(s_ctx, &s_pending_worker);
    }
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
    uint8_t i;

    if (!ctx || s_ctx) {
        return false;
    }

    s_ctx = ctx;
    s_pending_worker.do_work = key_pending_work;
    s_tick_worker.do_work = key_tick_work;
    async_context_add_when_pending_worker(ctx, &s_pending_worker);

    gpio_set_irq_callback(key_gpio_irq);
    irq_set_enabled(IO_IRQ_BANK0, true);
    for (i = 0; i < KEY_COUNT; i++) {
        gpio_set_irq_enabled(key_pins[i],
                             GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE,
                             true);
    }

    /* 上电已按下，或补采一次当前电平 */
    key_arm_ticks(ctx);
    return true;
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

Button *key_handle(key_id_t id)
{
    if (id >= KEY_COUNT) {
        return NULL;
    }
    return &s_btn[id];
}
