#include <stdio.h>
#include <hpl_dma.h>
#include <hpl_adc_dma.h>
#include <string.h>

#include "adc.h"


#define ADC_BUFLEN 2048

static uint16_t samp_buf_a[ADC_BUFLEN];
static uint16_t samp_buf_b[ADC_BUFLEN];


double bandgap_voltage = 42;


static void dma_err_cb(struct _dma_resource *resource)
{
    // XXX yikes! in a handler?
    printf("DMAC transfer ERROR!\n");
}

static void dma_transfer_done_cb(struct _dma_resource *resource)
{
    _dma_enable_transaction(0, false);
    _dma_enable_transaction(1, false);

    uint32_t sum_a = 0;
    uint32_t sum_b = 0;
    uint32_t sum;
    double avg_counts;

    for (int i = 0; i < ADC_BUFLEN; i++)
    {
        sum_a += samp_buf_a[i];
        sum_b += samp_buf_b[i];
    }
    sum = (sum_a + sum_b) / 2;
    avg_counts = (double)sum / (double)ADC_BUFLEN;

    bandgap_voltage = avg_counts / (double)4096.;

    // XXX how dowe know the current buffer?
}


void adc_init(const void * const adc, mem_adc_cal_t *cal)
{
    ASSERT(adc);
    ASSERT(cal);


    // write the calibration values
    hri_adc_write_CALIB_BIASCOMP_bf(adc, cal->biascomp);
    hri_adc_write_CALIB_BIASREFBUF_bf(adc, cal->biasrefbuf);
    hri_adc_write_CALIB_BIASR2R_bf(adc, cal->biasr2r);



    // dma callbacks
    struct _dma_resource *resource;
    _dma_get_channel_resource(&resource, 0);
    resource->dma_cb.error = dma_err_cb;
    resource->dma_cb.transfer_done = dma_transfer_done_cb;
    

    // XXX
    // so, this is kind of fucked. the ASF API (hpl_dma.h) doesn't
    // have provisions for using linked descriptor lists, well, it
    // does have a function for linking another descriptor on a list,
    // but it just links descriptor slots in BASEADDR, i guess?
    // there's a really weird call, _dma_set_next_descriptor, that
    // links descriptors in two different channel slots. I don't even
    // see the point of this function? 

    
    // dmac descriptor 0, channel 0
    
    // NOTE part of descriptor 0's setup is completed by
    // _dma_init() using the parameters configured in START.
    // 
    _dma_set_source_address(0, (const void *)&REG_ADC0_RESULT);
    _dma_set_destination_address(0, samp_buf_a);
    _dma_set_data_amount(0, ADC_BUFLEN);

    // dmac descriptor 1, channel 0 (a.k.a. descriptor 0 channel 1,
    // read above)

    // since channel 1's descriptor hasn't been setup in _dma_init(),
    // we'll copy the common fields from descriptor 0.
    extern DmacDescriptor _descriptor_section[];

    _descriptor_section[1].BTCTRL = _descriptor_section[0].BTCTRL;
    _descriptor_section[1].BTCNT = _descriptor_section[0].BTCNT;
    _descriptor_section[1].SRCADDR = _descriptor_section[0].SRCADDR;

    // ok, now we can configure the unique parts of descriptor 1
    _dma_set_destination_address(1, samp_buf_b);

    // now circularly link the descriptors (the dumb way, the ASF way)
    _dma_set_next_descriptor(0, 1);
    _dma_set_next_descriptor(1, 0);

    // enable the descriptors
    _dma_enable_transaction(0, false);
    _dma_enable_transaction(1, false);


    // now setup the adc
    _adc_dma_set_inputs(&ADC_0, 0x1B, 0x18, 0);
    _adc_dma_set_conversion_mode(&ADC_0, ADC_CONVERSION_MODE_FREERUN);
    _adc_dma_enable_channel(&ADC_0, 0);
}


void adc_start(void)
{
    _dma_set_irq_state(0, DMA_TRANSFER_COMPLETE_CB, true);
    _dma_set_irq_state(0, DMA_TRANSFER_ERROR_CB, true);

    hri_adc_write_INTEN_RESRDY_bit(ADC_0.hw, true);
    
    // kick off the first conversion
    _adc_dma_convert(&ADC_0);
}
