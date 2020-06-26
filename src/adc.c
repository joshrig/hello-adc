#include <stdio.h>
#include <stdlib.h>

#include <hpl_dma.h>
#include <hpl_adc_dma.h>
#include <string.h>

#include "adc.h"


#define ADC_BUFLEN 4

uint16_t samp_buf_a[ADC_BUFLEN];
uint16_t samp_buf_b[ADC_BUFLEN];
uint16_t *current_buf = samp_buf_a;


double light_sensor_counts = 0;


extern DmacDescriptor _descriptor_section[];


static void dma_err_cb(struct _dma_resource *resource)
{
}

static void dma_transfer_done_cb(struct _dma_resource *resource)
{
    uint32_t sum = 0;
    double avg_counts;

    for (int i = 0; i < ADC_BUFLEN; i++)
        sum += current_buf[i];

    avg_counts = (double)sum / (double)ADC_BUFLEN;

    light_sensor_counts = avg_counts / (double)4096.;

    current_buf = samp_buf_a == current_buf ? samp_buf_b : samp_buf_a;
}


void adc_init(const void * const adc, mem_adc_cal_t *cal)
{
    ASSERT(adc);
    ASSERT(cal);


    // write the calibration values
    hri_adc_write_CALIB_BIASCOMP_bf(adc, cal->biascomp);
    hri_adc_write_CALIB_BIASREFBUF_bf(adc, cal->biasrefbuf);
    hri_adc_write_CALIB_BIASR2R_bf(adc, cal->biasr2r);


    // XXX we do this ourselves now since ASF is fucking broken...
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

    _descriptor_section[1].BTCTRL = _descriptor_section[0].BTCTRL;
    _descriptor_section[1].BTCNT = _descriptor_section[0].BTCNT;
    _descriptor_section[1].SRCADDR = _descriptor_section[0].SRCADDR;

    // ok, now we can configure the unique parts of descriptor 1
    _dma_set_destination_address(1, samp_buf_b + 4);

    // now circularly link the descriptors (the dumb way, the ASF way)
    _dma_set_next_descriptor(0, 1);
    _dma_set_next_descriptor(1, 0);

    // enable the descriptors
    hri_dmacdescriptor_set_BTCTRL_VALID_bit(&_descriptor_section[0]);
    hri_dmacdescriptor_set_BTCTRL_VALID_bit(&_descriptor_section[1]);

    hri_dmac_set_CHCTRLA_ENABLE_bit(DMAC, 0);
    
    _dma_set_irq_state(0, DMA_TRANSFER_COMPLETE_CB, true);
    _dma_set_irq_state(0, DMA_TRANSFER_ERROR_CB, true);

    // now setup the adc
    _adc_dma_enable_channel(&ADC_0, 0);
}

void adc_print_status(void)
{
    printf("dmac enable: %d\n", hri_dmac_get_CTRL_DMAENABLE_bit(ADC0));
    printf("dmac ch0 enable: %d\n", hri_dmacchannel_get_CHCTRLA_ENABLE_bit(&DMAC->Channel[0]));
    printf("dmac ch1 enable: %d\n", hri_dmacchannel_get_CHCTRLA_ENABLE_bit(&DMAC->Channel[1]));

    printf("desc0 valid: %d\n", hri_dmacdescriptor_get_BTCTRL_VALID_bit(&_descriptor_section[0]));
    printf("desc1 valid: %d\n", hri_dmacdescriptor_get_BTCTRL_VALID_bit(&_descriptor_section[1]));
    printf("desc0 @ 0x%08X\n", (void *)&_descriptor_section[0]);
    printf("desc1 @ 0x%08X\n", (void *)&_descriptor_section[1]);
    printf("desc0 descaddr: 0x%08X\n", hri_dmacdescriptor_get_DESCADDR_reg(&_descriptor_section[0], 0xFFFFFFFF));
    printf("desc1 descaddr: 0x%08X\n", hri_dmacdescriptor_get_DESCADDR_reg(&_descriptor_section[1], 0xFFFFFFFF));

    printf("samp_buf_a @ 0x%08X\n", (void *)samp_buf_a);
    printf("samp_buf_b @ 0x%08X\n", (void *)samp_buf_b);
    printf("desc0 dstaddr: 0x%08X\n", hri_dmacdescriptor_get_DSTADDR_reg(&_descriptor_section[0], 0xFFFFFFFF));
    printf("desc1 dstaddr: 0x%08X\n", hri_dmacdescriptor_get_DSTADDR_reg(&_descriptor_section[1], 0xFFFFFFFF));

    printf("\n\n");
}




    // int a = 0;
    // for (int i = 0; i < 4; i++)
    // {
    //     if (samp_buf_a[i] == 0)
    //         continue;
    //     a++;
    // }
    // int b = 0;
    // for (int i = 0; i < 4; i++)
    // {
    //     if (samp_buf_b[i] == 0)
    //         continue;
    //     b++;
    // }
    // printf("samp_buf_a: %d nonzero samples\n", a);
    // printf("samp_buf_b: %d nonzero samples\n", b);

    // if (a == 4)
    //     for (int i = 0; i < 4; i++)
    //         samp_buf_a[i] = 0;

    // if (b == 4)
    //     for (int i = 0; i < 4; i++)
    //         samp_buf_b[i] = 0;

