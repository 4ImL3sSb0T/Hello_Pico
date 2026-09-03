/**
 * @file        eload.h
 * @brief       简易电子负载：KEY0 调 PWM 占空比，ADC0 采样，屏上显示
 *
 * 占空比步进 1%，上限 20%（BSP 再钳一次）。
 * 仍用板上 KEY0：按下立刻 +1%（到 20% 再按回 0%），长按关断。
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
