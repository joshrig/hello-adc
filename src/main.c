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


extern double light_sensor_counts;





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

    

    // wait for an interrupt.
    while (1)
    {
        __WFI();
    }
}


void SysTick_Handler(void)
{
    char buf[50];
    static int j = 0;

    
    led_update();

    display_clear_framebuffer();
    sprintf(buf, "0x0C: %0.3f", light_sensor_counts);
    display_write_string((const char *)buf, 1, 1);

    extern uint32_t ndma_interrupts;
    sprintf(buf, "nint: %d", ndma_interrupts);
    display_write_string((const char *)buf, 1, 2);

    extern bool dma_error;
    if (dma_error)
    {
        sprintf(buf, "ERROR");
        display_write_string((const char *)buf, 2, 1);
    }

    if (j++ > 67)
    {
        adc_print_status();
        j = 0;
    }
}






#if 0
void SysTick_Handler(void)
{
    char strbuf[50];

    
    led_update();


    // sprintf(strbuf, "VREF: %0.2fV\nTEMP: %0.2fF", Vref, Temp);
    sprintf(strbuf, "LGHT: %0.6fV\nTEMP: %0.2fF", Light, Temp);

    display_clear_framebuffer();
    display_write_string((const char *)strbuf, 1, 1);

    CurrentMUX++;
    if (CurrentMUX == NMUX)
        CurrentMUX = 0;
    adc_async_set_inputs(&ADC_0, Muxes[CurrentMUX], 0x18, 0);

    printf("VREF: %0.2fV, ", Vref);
    printf("LGHT: %0.4fV, ", Light);
    printf("TEMP: %0.2fF\n", Temp);
}





    // configure the light sensor pin
    gpio_set_pin_direction(GPIO(GPIO_PORTB, 0), GPIO_DIRECTION_OFF);
    gpio_set_pin_function (GPIO(GPIO_PORTB, 0), PINMUX_PB00B_ADC0_AIN12);

    
    // set the muxbits POSMUX - PTAT, NEGMUX - GND, channel 0
    adc_async_set_inputs(&ADC_0, Muxes[CurrentMUX], 0x18, 0);

    // set the ADC to freerun (this doesn't work for some reason)
    adc_async_set_conversion_mode(&ADC_0, ADC_CONVERSION_MODE_FREERUN);

    // NOTE: all this does is turn on the ADC
    adc_async_enable_channel(&ADC_0, 0);

    // register the callbacks when we have some samples to read
    adc_async_register_callback(&ADC_0, 0, ADC_ASYNC_CONVERT_CB, convert_cb_ADC_0);



#define NMUX 4
static uint8_t Muxes[NMUX] = { 0x1B, 0x0C, 0x1C, 0x1D };
static uint8_t CurrentMUX = 0;

// temperature calibration parameters
static double   TL;
static double   TH;
static uint16_t VPL;
static uint16_t VPH;
static uint16_t VCL;
static uint16_t VCH;
// temperature counts
static uint16_t PTAT = 0;
static uint16_t CTAT = 0;

// measured values
static double Vref  = 0.0;
static double Temp  = 0.0;
static double Light = 0.0;



static void recalculate_temp(void)
{
    Temp = TL * VPH * CTAT - VPL * TH * CTAT - TL * VCH * PTAT + TH * VCL * PTAT;
    Temp = Temp / (VCL * PTAT - VCH * PTAT - VPL * CTAT + VPH * CTAT);
    Temp = Temp * 9 / 5 + 32.0;
}

static void convert_cb_ADC_0
(
    const struct adc_async_descriptor *const descr,
    const uint8_t channel
)
{
    uint8_t buf[16];
    int32_t nbytes;

    nbytes = adc_async_read_channel(&ADC_0, channel, buf, 16);
    if (nbytes > 0)
    {
        uint16_t val = buf[0] | (buf[1] << 8);

        switch (Muxes[CurrentMUX])
        {
        case 0x0C:
            Light = (double)val / 4096.0;
            break;
        case 0x1B:
            // with a bandgap voltage of 1V and 12bit conversions
            Vref = (double)val / 4096.0;
            break;
        case 0x1C:
            PTAT = val;
            break;
        case 0x1D:
            CTAT = val;
            break;
        }

        recalculate_temp();
    }
}
#endif
