/**
 * @file        key.h
 * @brief       RP2350A 小系统板按键 BSP（KEY0 / GPIO2 + MultiButton）
 *
 * MultiButton 状态机挂在 async_context 的 5ms at_time worker 上。
 * 单击/双击区分时间在这里改，不要直接改 multi_button.h。
 */

#ifndef __KEY_H__
#define __KEY_H__

#include "pico/async_context.h"

#define TICKS_INTERVAL          5
#define KEY_SHORT_MS            100     /* 松手后再等这么久才判定单击，以便区分双击 */
#define KEY_LONG_MS             1000
#define SHORT_TICKS             (KEY_SHORT_MS / TICKS_INTERVAL)
#define LONG_TICKS              (KEY_LONG_MS / TICKS_INTERVAL)

#include "bsp/input/multi_button.h"

/* 引脚表 KEY0 / 原理图 KEY1：GPIO2，按下为低，内部上拉 */
#define KEY0_GPIO_PIN           2
#define KEY0_ACTIVE_LEVEL       0

typedef enum {
    KEY_ID_0 = 0,
    KEY_COUNT
} key_id_t;

void key_init(void);
bool key_bind(async_context_t *ctx);
void key_attach(key_id_t id, ButtonEvent event, BtnCallback cb);
void key_detach(key_id_t id, ButtonEvent event);
int  key_is_pressed(key_id_t id);
uint8_t key_raw_level(key_id_t id);
Button *key_handle(key_id_t id);

#endif
