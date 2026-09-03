/**
 * @file        adc_in.c
 * @brief       ADC0（GPIO26）单次转换；读前重选通道，避免以后加通道串扰
 */

#include "bsp/adc/adc_in.h"

#include "hardware/adc.h"

void adc_in_init(void)
{
    adc_init();
    adc_gpio_init(ADC_IN_GPIO_PIN);
    adc_select_input(ADC_IN_CHANNEL);
}

uint16_t adc_in_read_raw(void)
{
    adc_select_input(ADC_IN_CHANNEL);
    return adc_read();
}

uint16_t adc_in_read_raw_avg(uint8_t n)
{
    uint32_t sum = 0;
    uint8_t i;

    if (n == 0) {
        n = 1;
    }
    adc_select_input(ADC_IN_CHANNEL);
    for (i = 0; i < n; i++) {
        sum += adc_read();
    }
    return (uint16_t)(sum / n);
}

uint32_t adc_in_raw_to_mv(uint16_t raw)
{
    return ((uint32_t)raw * ADC_IN_VREF_MV) / ADC_IN_MAX_RAW;
}
