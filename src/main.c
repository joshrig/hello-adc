#include <inttypes.h>
#include <string.h>
#include <stdio.h>

#include <atmel_start.h>
#include <peripheral_clk_config.h>
#include <hpl_dma.h>
#include <hpl_adc_dma.h>

#include "adc.h"
#include "display.h"
#include "led.h"
#include "mem.h"
#include "supc.h"
#include "uart.h"


extern double light_sensor_volts;


static bool update_display = true;
static bool update_serial = true;


int main(void)
{
    // Initializes MCU, drivers and middleware
    atmel_start_init();


    uart_init();

    printf("\nJTOS v0.1 copyright 2020 Codeposse Consulting, Inc.\n");
    printf("SAME54 serial: %s\n", mem_get_chip_serial());
    printf("system initializing.\n\n");

    extern uint32_t _sstack;
    extern uint32_t _estack;
    printf("_sstack: 0x%08X\n", (void *)&_sstack);
    printf("_estack: 0x%08X\n\n", (void *)&_estack);

    supc_init();


    led_init(kLEDCountMode);
    printf("initialized LEDs: %s\n", led_get_mode_string());


    // configure the display
    display_init(
        &SPI_0,
        GPIO(GPIO_PORTC, 31), // display reset
        GPIO(GPIO_PORTC,  1), // data/command select
        GPIO(GPIO_PORTC, 14)  // spi slave select
        );
    printf("initialized OLED display\n");

    display_clear();


    printf("get TSENS calibration params.\n");

    mem_tsens_cal_t tsens_cal;
    mem_get_tsens_cal(&tsens_cal);
    
    printf(" TLI: %d, TLD: %d\n", tsens_cal.tli, tsens_cal.tld);
    printf(" THI: %d, THD: %d\n", tsens_cal.thi, tsens_cal.thd);
    printf("  for TL: %0.2f TH: %0.2f\n", tsens_cal.tl, tsens_cal.th);
    printf(" VPL: %d, VPH: %d\n", tsens_cal.vpl, tsens_cal.vph);
    printf(" VCL: %d, VCH: %d\n\n", tsens_cal.vcl, tsens_cal.vch);
    
    
    printf("get ADC calibration params.\n");

    mem_adc_cal_t adc_cal;
    mem_get_adc_cal(&adc_cal);

    printf(" BIASCOMP: 0x%01X\n", adc_cal.biascomp);
    printf(" BIASREFBUF: 0x%01X\n", adc_cal.biasrefbuf);
    printf(" BIASR2R: 0x%01X\n\n", adc_cal.biasr2r);


    //
    // configure the ADC
    //
    printf("Starting ADC: ");

    adc_init(ADC0, &adc_cal);

    printf("done.\n");


    //
    // SysTick is just used, for now, for the LEDs
    //
    uint32_t systick_interval_ticks = 360000;
    SysTick_Config(systick_interval_ticks);
    printf("SysTick interval: %0.3fs\n\n", (double)systick_interval_ticks / (double)CONF_CPU_FREQUENCY);


    printf("system up.\n\n");
    fflush(stdout);

    
    while (1)
    {
        if (update_display)
        {
            char buf[50];


            update_display = false;
    
            led_update();

            display_clear_framebuffer();
            sprintf(buf, "LGHT: %0.3fV", light_sensor_volts);
            display_write_string((const char *)buf, 1, 1);

            extern uint32_t ndma_interrupts;
            sprintf(buf, "%d", ndma_interrupts);
            display_write_string((const char *)buf, 1, 2);

            extern bool dma_error;
            if (dma_error)
            {
                sprintf(buf, "ERROR");
                display_write_string((const char *)buf, 2, 1);
            }

        }
        if (update_serial)
        {
            update_serial = false;
            adc_print_status();
        }            

        
        __WFI();
    }
}


void SysTick_Handler(void)
{
    static int j = 0;


    update_display = true;

    if (j++ > 67)
    {
        update_serial = true;
        j = 0;
    }
}


#if 0
static void recalculate_temp(void)
{
    Temp = TL * VPH * CTAT - VPL * TH * CTAT - TL * VCH * PTAT + TH * VCL * PTAT;
    Temp = Temp / (VCL * PTAT - VCH * PTAT - VPL * CTAT + VPH * CTAT);
    Temp = Temp * 9 / 5 + 32.0;
}

        case 0x1C:
            PTAT = val;
            break;
        case 0x1D:
            CTAT = val;
            break;
#endif
