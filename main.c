#include <inttypes.h>
#include <string.h>
#include <stdio.h>

#include <atmel_start.h>

#include "led.h"
#include "display.h"



static uint32_t Tick = 0;


int main(void)
{
    char buf[50];

    

    /* Initializes MCU, drivers and middleware */
    atmel_start_init();

    led_init();

    display_init(
        &SPI_0,
        GPIO(GPIO_PORTC, 31), // display reset
        GPIO(GPIO_PORTC,  1), // data/command select
        GPIO(GPIO_PORTC, 14)  // spi slave select
        );

    // 1ms interrupts @ 48MHz MCLK
    SysTick_Config(48000);



    // wait for an interrupt.
    while (1)
    {
        sprintf(buf, "Hello, World!\ntick: %ld", Tick);
        display_clear_framebuffer();
        display_write_string((const char *)buf, 1, 1);

        __WFI();
    }
}


void SysTick_Handler(void)
{
    Tick++;

    led_update();
}
