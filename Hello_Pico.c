#include <stdio.h>
#include "pico/stdlib.h"
#include "bsp/led/led.h"
#include "bsp/lcd/spi.h"
#include "bsp/lcd/lcd.h"

int main()
{
    stdio_init_all();
    led_init();
    spi1_init();
    lcd_init();

    lcd_show_string(10, 20, 220, 32, 16, "Hello Pico", RED);
    lcd_show_string(10, 50, 220, 32, 16, "RP2350A LCD", BLUE);
    lcd_flush();

    printf("Hello Pico started\n");

    while (true) {
        LED_TOGGLE();
        sleep_ms(500);
    }
}
