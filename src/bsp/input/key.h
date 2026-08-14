/**
 * @file        key.h
 * @brief       RP2350A 小系统板按键 BSP（KEY0 / GPIO2 + MultiButton）
 *
 * MultiButton 状态机挂在 async_context 的 5ms at_time worker 上。
 */

#ifndef __KEY_H__
#define __KEY_H__

#include "pico/async_context.h"
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
