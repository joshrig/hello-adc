#include <inttypes.h>
#include <string.h>
#include <stdio.h>

#include <atmel_start.h>

#include "led.h"
#include "display.h"


static uint8_t Muxes[] = { 0x1B, 0x1C, 0x1D  };
static uint8_t CurrentMUX = 0;

static uint16_t TP = 0;
static uint16_t TC = 0;
static double   Vref = 0.0;
static double   Temp = 0.0;


static double TL;
static double TH;
static uint16_t VPL;
static uint16_t VPH;
static uint16_t VCL;
static uint16_t VCH;


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
        case 0x1B:
            // with a bandgap voltage of 1V and 12bit conversions
            Vref = (double)val / 4096.0;
            break;
        case 0x1C:
            TP = val;
            break;
        case 0x1D:
            TC = val;
            break;
        }

        Temp = TL * VPH * TC - VPL * TH * TC - TL * VCH * TP + TH * VCL * TP;
        Temp = Temp / (VCL * TP - VCH * TP - VPL * TC + VPH * TC);
        Temp = Temp * 1.8 + 32.0;
    }
}


int main(void)
{
    /* Initializes MCU, drivers and middleware */
    atmel_start_init();

    // initialize the LEDs
    led_init();

    // configure the display
    display_init(
        &SPI_0,
        GPIO(GPIO_PORTC, 31), // display reset
        GPIO(GPIO_PORTC,  1), // data/command select
        GPIO(GPIO_PORTC, 14)  // spi slave select
        );

    display_clear();


    //
    // grab the temperature calibration parameters
    //
    
    uint8_t *p = ((uint8_t *)NVMCTRL_TEMP_LOG);

    uint16_t TLI = p[0];
    uint16_t TLD = p[1] >> 4;
    uint16_t THI = (p[1] & 0xF) << 4 | (p[2] >> 4);
    uint16_t THD = p[2] & 0xF;

    TL = (double)TLI + (double)TLD / 16.0;
    TH = (double)THI + (double)THD / 16.0;

    VPL = p[5] << 4 | (p[6] >> 4);
    VPH = p[6] << 8 | p[7];
    VCL = p[8] << 4 | (p[9] >> 4);
    VCH = p[9] << 8 | p[10];


    //
    // configure the SUPC
    //

    // enable on-demand mode
    hri_supc_set_VREF_ONDEMAND_bit(SUPC);

    // enable the temperature sensors
    hri_supc_set_VREF_TSEN_bit(SUPC);

    // enable the voltage referance
    hri_supc_set_VREF_VREFOE_bit(SUPC);


    //
    // grab the ADC calibration parameters
    //

    p = ((uint8_t *)0x00800080);
    uint8_t biascomp   = (p[0] >> 2) & 0x7;
    uint8_t biasrefbuf = (p[0] >> 5) & 0x7;
    uint8_t biasr2r    =  p[1] & 0x7;
    
    hri_adc_write_CALIB_BIASCOMP_bf(ADC0, biascomp);
    hri_adc_write_CALIB_BIASREFBUF_bf(ADC0, biasrefbuf);
    hri_adc_write_CALIB_BIASR2R_bf(ADC0, biasr2r);


    //
    // configure the ADC
    //
    
    // set the muxbits POSMUX - PTAT, NEGMUX - GND, channel 0
    adc_async_set_inputs(&ADC_0, Muxes[CurrentMUX], 0x18, 0);

    // set the ADC to freerun (this doesn't work for some reason)
    //adc_async_set_conversion_mode(&ADC_0, ADC_CONVERSION_MODE_FREERUN);

    // NOTE: all this does is turn on the ADC
    adc_async_enable_channel(&ADC_0, 0);

    // register the callbacks when we have some samples to read
    adc_async_register_callback(&ADC_0, 0, ADC_ASYNC_CONVERT_CB, convert_cb_ADC_0);


    
    SysTick_Config(4800000);



    // wait for an interrupt.
    while (1)
    {
        adc_async_start_conversion(&ADC_0);

        __WFI();
    }
}


void SysTick_Handler(void)
{
    char strbuf[50];


    led_update();


    sprintf(strbuf, "VREF: %0.2fV\nTEMP: %0.2fF", Vref, Temp);
        
    display_clear_framebuffer();
    display_write_string((const char *)strbuf, 1, 1);

#if 1
    CurrentMUX++;
    if (CurrentMUX == 3)
        CurrentMUX = 0;

    adc_async_set_inputs(&ADC_0, Muxes[CurrentMUX], 0x18, 0);
#endif
}
