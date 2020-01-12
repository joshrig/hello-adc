#include <inttypes.h>
#include <string.h>
#include <stdio.h>

#include <atmel_start.h>

#include "display.h"
#include "led.h"
#include "uart.h"


static uint8_t Muxes[] = { 0x1B, 0x1C, 0x1D  };
static uint8_t CurrentMUX = 0;

static uint16_t TP = 0;
static uint16_t TC = 0;
static double   Vref = 0.0;
static double   Temp = 0.0;


static uint16_t TLI;
static uint16_t TLD;
static uint16_t THI;
static uint16_t THD;
static double   TL;
static double   TH;
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

    // initialize the UART
    uart_init();
    printf("initialized UART\n");

    // initialize the LEDs
    led_init();
    printf("initialized LEDs\n");

    // configure the display
    display_init(
        &SPI_0,
        GPIO(GPIO_PORTC, 31), // display reset
        GPIO(GPIO_PORTC,  1), // data/command select
        GPIO(GPIO_PORTC, 14)  // spi slave select
        );
    printf("initialized OLED display\n");

    display_clear();


    //
    // configure the SUPC
    //
    printf("configuring SUPC: ");

    // enable on-demand mode
    hri_supc_set_VREF_ONDEMAND_bit(SUPC);
    printf("ONDEMAND ");

    // enable the temperature sensors
    hri_supc_set_VREF_TSEN_bit(SUPC);
    printf("TSENS ");

    // enable the voltage referance
    hri_supc_set_VREF_VREFOE_bit(SUPC);
    printf("VREF ");

    printf("done.\n");
        

    //
    // grab the temperature calibration parameters
    //

    printf("grab TSENS calibration params.\n");

    uint32_t *p = ((uint32_t *)NVMCTRL_TEMP_LOG);

    TLI = (*p >> 0 ) & 0xFF;
    TLD = (*p >> 8 ) & 0xF;
    THI = (*p >> 12) & 0xFF;
    THD = (*p >> 16) & 0xF;

    TL = (double)TLI + (double)TLD / 16.0;
    TH = (double)THI + (double)THD / 16.0;

    p += 1;
    VPL = (*p >> 8 ) & 0xFFF;
    VPH = (*p >> 20) & 0xFFF;

    p += 1;
    VCL = (*p >> 0 ) & 0xFFF;
    VCH = (*p >> 12) & 0xFFF;

    printf(" TLI: %d, TLD: %d\n", TLI, TLD);
    printf(" THI: %d, THD: %d\n", THI, THD);
    printf("  for TL: %0.2f TH: %0.2f\n", TL, TH);
    printf(" VPL: %d, VPH: %d\n", VPL, VPH);
    printf(" VCL: %d, VCH: %d\n\n", VCL, VCH);

    
    //
    // grab the ADC calibration parameters
    //

    printf("grab ADC calibration params.\n");

    p = ((uint32_t *)0x00800080);
    uint8_t biascomp   = (*p >> 2) & 0x7;
    uint8_t biasrefbuf = (*p >> 5) & 0x7;
    uint8_t biasr2r    = (*p >> 8) & 0x7;
    
    hri_adc_write_CALIB_BIASCOMP_bf(ADC0, biascomp);
    hri_adc_write_CALIB_BIASREFBUF_bf(ADC0, biasrefbuf);
    hri_adc_write_CALIB_BIASR2R_bf(ADC0, biasr2r);

    printf(" BIASCOMP: 0x%01X\n", biascomp);
    printf(" BIASREFBUF: 0x%01X\n", biasrefbuf);
    printf(" BIASR2R: 0x%01X\n\n", biasr2r);


    //
    // configure the ADC
    //
    printf("configuring ADC: ");
    
    // set the muxbits POSMUX - PTAT, NEGMUX - GND, channel 0
    adc_async_set_inputs(&ADC_0, Muxes[CurrentMUX], 0x18, 0);

    // set the ADC to freerun (this doesn't work for some reason)
    //adc_async_set_conversion_mode(&ADC_0, ADC_CONVERSION_MODE_FREERUN);

    // NOTE: all this does is turn on the ADC
    adc_async_enable_channel(&ADC_0, 0);

    // register the callbacks when we have some samples to read
    adc_async_register_callback(&ADC_0, 0, ADC_ASYNC_CONVERT_CB, convert_cb_ADC_0);

    printf("done.\n");


    
    SysTick_Config(4800000);
    printf("enabled SysTick: 4800000\n\n");


    printf("system up.\n\n");





    // wait for an interrupt.
    while (1)
    {
        adc_async_start_conversion(&ADC_0);

        // printf("cmd$ ");
        // scanf("%s", buf);
        // printf("\n  executing: %s\n", buf);

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

    CurrentMUX++;
    if (CurrentMUX == 3)
        CurrentMUX = 0;

    adc_async_set_inputs(&ADC_0, Muxes[CurrentMUX], 0x18, 0);
}
