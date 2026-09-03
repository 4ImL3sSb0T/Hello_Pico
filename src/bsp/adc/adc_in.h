/**
 * @file        adc_in.h
 * @brief       片内 SAR ADC：GPIO26 / ADC0 做负载电压采样
 *
 * 模拟脚由 adc_gpio_init 关掉数字 IE（避开 RP2350-E9）。
 * GPIO29 板上已接 5V 分压监测，不要拿来当负载采样。
 * 输入不得超过 ADC_AVDD（3.3 V），更高电压必须外部分压。
 */

#ifndef ADC_IN_H
#define ADC_IN_H

#include "pico/stdlib.h"

#define ADC_IN_GPIO_PIN     26
#define ADC_IN_CHANNEL      0
#define ADC_IN_VREF_MV      3300u
#define ADC_IN_MAX_RAW      4095u

void adc_in_init(void);
uint16_t adc_in_read_raw(void);
uint16_t adc_in_read_raw_avg(uint8_t n);
uint32_t adc_in_raw_to_mv(uint16_t raw);

#endif
