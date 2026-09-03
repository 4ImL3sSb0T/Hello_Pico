/**
 * @file        pwm_out.c
 * @brief       GPIO4 高频 PWM：clk_sys 1 分频，占空比钳位 20%
 */

#include "bsp/pwm/pwm_out.h"

#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"

static uint16_t s_wrap;
static uint32_t s_period;
static uint32_t s_freq_hz;
static uint16_t s_duty_permille;

void pwm_out_init(void)
{
    pwm_config cfg;
    uint slice;
    uint32_t sys_hz;
    uint32_t period;

    sys_hz = clock_get_hz(clk_sys);
    period = sys_hz / PWM_OUT_FREQ_HZ;
    if (period < 2u) {
        period = 2u;
    }
    if (period > 65536u) {
        period = 65536u;
    }

    s_period = period;
    s_wrap = (uint16_t)(period - 1u);
    s_freq_hz = sys_hz / period;
    s_duty_permille = 0;

    gpio_set_function(PWM_OUT_GPIO_PIN, GPIO_FUNC_PWM);
    slice = pwm_gpio_to_slice_num(PWM_OUT_GPIO_PIN);

    cfg = pwm_get_default_config();
    pwm_config_set_clkdiv_int(&cfg, 1);
    pwm_config_set_wrap(&cfg, s_wrap);
    pwm_init(slice, &cfg, true);

    pwm_out_set_duty_permille(0);
}

void pwm_out_set_duty_permille(uint16_t permille)
{
    uint32_t level;

    if (permille > PWM_OUT_DUTY_MAX_PERMILLE) {
        permille = PWM_OUT_DUTY_MAX_PERMILLE;
    }
    s_duty_permille = permille;

    /* 高电平时间 = period * permille / 1000；period = wrap + 1 */
    level = (s_period * (uint32_t)permille) / 1000u;
    if (level > s_wrap) {
        level = s_wrap;
    }
    pwm_set_gpio_level(PWM_OUT_GPIO_PIN, (uint16_t)level);
}

uint16_t pwm_out_get_duty_permille(void)
{
    return s_duty_permille;
}

uint32_t pwm_out_get_freq_hz(void)
{
    return s_freq_hz;
}
