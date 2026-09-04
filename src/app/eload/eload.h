/**
 * @file        eload.h
 * @brief       简易电子负载：KEY0 调 PWM 占空比，ADC0 采样，屏上显示
 *
 * 占空比步进 1%，上限 20%（BSP 再钳一次）。
 * 仍用板上 KEY0：单击 +1%，双击 -1%，长按关断（0%）。
 * 单击要松手后再等 KEY_SHORT_MS（现 100ms）才能和双击区分。
 */

#ifndef ELOAD_H
#define ELOAD_H

#include <stdint.h>
#include "hagl.h"

void eload_init(void);
void eload_bind_keys(void);
void eload_sample(void);
void eload_draw(hagl_backend_t *display);
uint8_t eload_duty_percent(void);
uint16_t eload_adc_raw(void);
uint32_t eload_adc_mv(void);

#endif
