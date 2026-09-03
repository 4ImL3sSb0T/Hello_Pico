/**
 * @file        pwm_out.h
 * @brief       电子负载 PWM 输出（GPIO4 / slice2A）
 *
 * 目标约 150 kHz。占空比以千分比设置，硬件侧钳位不超过 20%。
 * 上电默认 0%，避免 MOSFET 误导通。
 */

#ifndef PWM_OUT_H
#define PWM_OUT_H

#include "pico/stdlib.h"

/* 引脚表：GPIO4 完全独立，J1 已引出。PWM slice 2 通道 A。 */
#define PWM_OUT_GPIO_PIN            4
#define PWM_OUT_FREQ_HZ             150000u
#define PWM_OUT_DUTY_MAX_PERMILLE   200u    /* 20.0% */

void pwm_out_init(void);
void pwm_out_set_duty_permille(uint16_t permille);
uint16_t pwm_out_get_duty_permille(void);
uint32_t pwm_out_get_freq_hz(void);

#endif
