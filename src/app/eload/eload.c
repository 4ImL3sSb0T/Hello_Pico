/**
 * @file        eload.c
 * @brief       电子负载应用状态：占空比、采样、绘制。不碰 lcd_fb()
 */

#include "app/eload/eload.h"

#include <stdio.h>
#include <wchar.h>

#include "bsp/adc/adc_in.h"
#include "bsp/input/key.h"
#include "bsp/pwm/pwm_out.h"
#include "font6x9-ISO8859-1.h"

#define ELOAD_DUTY_STEP_PERCENT     1u
#define ELOAD_DUTY_MAX_PERCENT      20u
#define ELOAD_ADC_AVG               8u

#define WHITE                       0xFFFFu
#define RED                         0xF800u
#define GREEN                       0x07E0u
#define YELLOW                      0xFFE0u
#define GRAY                        0x8430u
#define DARK                        0x18E3u
#define CYAN                        0x07FFu

static uint8_t s_duty_percent;
static uint16_t s_adc_raw;
static uint32_t s_adc_mv;
static char s_key_event[12] = "-";

static void ascii_to_wchar(const char *src, wchar_t *dst, size_t n)
{
    size_t i;

    if (n == 0) {
        return;
    }
    for (i = 0; (i + 1) < n && src[i] != '\0'; i++) {
        dst[i] = (wchar_t)(unsigned char)src[i];
    }
    dst[i] = 0;
}

static void apply_duty(uint8_t percent)
{
    if (percent > ELOAD_DUTY_MAX_PERCENT) {
        percent = ELOAD_DUTY_MAX_PERCENT;
    }
    s_duty_percent = percent;
    pwm_out_set_duty_permille((uint16_t)percent * 10u);
    printf("eload duty %u%% (max %u%%)\n",
           (unsigned)s_duty_percent, (unsigned)ELOAD_DUTY_MAX_PERCENT);
}

static void on_key0(Button *btn)
{
    switch (button_get_event(btn)) {
    case BTN_PRESS_DOWN:
        /* 只更新状态，不改占空比。否则单击/双击/长按都会先 +1%。 */
        snprintf(s_key_event, sizeof(s_key_event), "DOWN");
        break;
    case BTN_SINGLE_CLICK:
        snprintf(s_key_event, sizeof(s_key_event), "CLICK");
        if (s_duty_percent + ELOAD_DUTY_STEP_PERCENT <= ELOAD_DUTY_MAX_PERCENT) {
            apply_duty((uint8_t)(s_duty_percent + ELOAD_DUTY_STEP_PERCENT));
        } else {
            apply_duty(ELOAD_DUTY_MAX_PERCENT);
        }
        break;
    case BTN_DOUBLE_CLICK:
        snprintf(s_key_event, sizeof(s_key_event), "DBL");
        if (s_duty_percent >= ELOAD_DUTY_STEP_PERCENT) {
            apply_duty((uint8_t)(s_duty_percent - ELOAD_DUTY_STEP_PERCENT));
        } else {
            apply_duty(0);
        }
        break;
    case BTN_LONG_PRESS_START:
        snprintf(s_key_event, sizeof(s_key_event), "LONG");
        apply_duty(0);
        break;
    default:
        return;
    }
    printf("KEY0 %s duty %u%%\n", s_key_event, (unsigned)s_duty_percent);
}

void eload_init(void)
{
    s_duty_percent = 0;
    s_adc_raw = 0;
    s_adc_mv = 0;
    snprintf(s_key_event, sizeof(s_key_event), "-");
    pwm_out_set_duty_permille(0);
}

void eload_bind_keys(void)
{
    key_attach(KEY_ID_0, BTN_PRESS_DOWN, on_key0);
    key_attach(KEY_ID_0, BTN_SINGLE_CLICK, on_key0);
    key_attach(KEY_ID_0, BTN_DOUBLE_CLICK, on_key0);
    key_attach(KEY_ID_0, BTN_LONG_PRESS_START, on_key0);
}

void eload_sample(void)
{
    s_adc_raw = adc_in_read_raw_avg(ELOAD_ADC_AVG);
    s_adc_mv = adc_in_raw_to_mv(s_adc_raw);
}

void eload_draw(hagl_backend_t *display)
{
    wchar_t wtext[40];
    char text[40];
    int16_t w = (int16_t)display->width;
    int16_t bar_x0 = 8;
    int16_t bar_x1 = (int16_t)(w - 9);
    int16_t bar_y0 = 54;
    int16_t bar_y1 = 66;
    int16_t bar_span;
    int16_t fill_w;
    uint16_t bar_color;
    uint16_t status_color;
    const char *status;
    uint32_t freq_hz = pwm_out_get_freq_hz();
    size_t n;
    int16_t status_x;

    if (s_duty_percent == 0) {
        status = "STOP";
        status_color = GRAY;
        bar_color = GRAY;
    } else if (s_duty_percent >= 16u) {
        status = "RUN";
        status_color = RED;
        bar_color = RED;
    } else if (s_duty_percent >= 6u) {
        status = "RUN";
        status_color = YELLOW;
        bar_color = YELLOW;
    } else {
        status = "RUN";
        status_color = GREEN;
        bar_color = GREEN;
    }

    hagl_clear(display);
    hagl_fill_rectangle(display, 0, 0, (int16_t)(w - 1), 18, DARK);

    ascii_to_wchar("LOAD", wtext, sizeof(wtext) / sizeof(wtext[0]));
    hagl_put_text(display, wtext, 4, 4, WHITE, font6x9_ISO8859_1);

    ascii_to_wchar(status, wtext, sizeof(wtext) / sizeof(wtext[0]));
    n = wcslen(wtext);
    status_x = (int16_t)(w - 4 - (int16_t)(n * 6));
    if (status_x < 4) {
        status_x = 4;
    }
    hagl_put_text(display, wtext, status_x, 4, status_color, font6x9_ISO8859_1);
    hagl_draw_hline(display, 0, 19, (uint16_t)w, GRAY);

    ascii_to_wchar("V", wtext, sizeof(wtext) / sizeof(wtext[0]));
    hagl_put_text(display, wtext, 8, 26, GRAY, font6x9_ISO8859_1);
    snprintf(text, sizeof(text), "%u.%03u V",
             (unsigned)(s_adc_mv / 1000u),
             (unsigned)(s_adc_mv % 1000u));
    ascii_to_wchar(text, wtext, sizeof(wtext) / sizeof(wtext[0]));
    hagl_put_text(display, wtext, 80, 26, WHITE, font6x9_ISO8859_1);

    ascii_to_wchar("DUTY", wtext, sizeof(wtext) / sizeof(wtext[0]));
    hagl_put_text(display, wtext, 8, 38, GRAY, font6x9_ISO8859_1);
    snprintf(text, sizeof(text), "%u %%  / %u %%",
             (unsigned)s_duty_percent,
             (unsigned)ELOAD_DUTY_MAX_PERCENT);
    ascii_to_wchar(text, wtext, sizeof(wtext) / sizeof(wtext[0]));
    hagl_put_text(display, wtext, 80, 38, CYAN, font6x9_ISO8859_1);

    bar_span = (int16_t)(bar_x1 - bar_x0);
    fill_w = (int16_t)((bar_span * (int16_t)s_duty_percent) / (int16_t)ELOAD_DUTY_MAX_PERCENT);
    hagl_fill_rectangle(display, bar_x0, bar_y0, bar_x1, bar_y1, DARK);
    if (fill_w > 0) {
        hagl_fill_rectangle(display, bar_x0, bar_y0,
                            (int16_t)(bar_x0 + fill_w), bar_y1, bar_color);
    }
    hagl_draw_rectangle(display, bar_x0, bar_y0, bar_x1, bar_y1, GRAY);

    snprintf(text, sizeof(text), "PWM GP%u  %lukHz",
             (unsigned)PWM_OUT_GPIO_PIN,
             (unsigned long)(freq_hz / 1000u));
    ascii_to_wchar(text, wtext, sizeof(wtext) / sizeof(wtext[0]));
    hagl_put_text(display, wtext, 8, 74, WHITE, font6x9_ISO8859_1);

    snprintf(text, sizeof(text), "ADC GP%u  raw %u",
             (unsigned)ADC_IN_GPIO_PIN,
             (unsigned)s_adc_raw);
    ascii_to_wchar(text, wtext, sizeof(wtext) / sizeof(wtext[0]));
    hagl_put_text(display, wtext, 8, 90, WHITE, font6x9_ISO8859_1);

    snprintf(text, sizeof(text), "KEY GP%u  %s",
             (unsigned)KEY0_GPIO_PIN, s_key_event);
    ascii_to_wchar(text, wtext, sizeof(wtext) / sizeof(wtext[0]));
    hagl_put_text(display, wtext, 8, 106, CYAN, font6x9_ISO8859_1);

    ascii_to_wchar("click+1  dbl-1  long0", wtext,
                   sizeof(wtext) / sizeof(wtext[0]));
    hagl_put_text(display, wtext, 8, 120, GRAY, font6x9_ISO8859_1);
}

uint8_t eload_duty_percent(void)
{
    return s_duty_percent;
}

uint16_t eload_adc_raw(void)
{
    return s_adc_raw;
}

uint32_t eload_adc_mv(void)
{
    return s_adc_mv;
}
