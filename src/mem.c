#include <stdio.h>

#include "mem.h"



void mem_get_adc_cal(mem_adc_cal_t *cal)
{
    uint32_t *p = ((uint32_t *)NVMCTRL_SW0);


    ASSERT(cal);


    cal->biascomp   = (*p >> 2) & 0x7;
    cal->biasrefbuf = (*p >> 5) & 0x7;
    cal->biasr2r    = (*p >> 8) & 0x7;
}


void mem_get_tsens_cal(mem_tsens_cal_t *cal)
{
    uint32_t *p = ((uint32_t *)NVMCTRL_TEMP_LOG);


    ASSERT(cal);


    cal->tli = (*p >> 0 ) & 0xFF;
    cal->tld = (*p >> 8 ) & 0xF;
    cal->thi = (*p >> 12) & 0xFF;
    cal->thd = (*p >> 16) & 0xF;

    cal->tl = (double)cal->tli + (double)cal->tld / 16.0;
    cal->th = (double)cal->thi + (double)cal->thd / 16.0;

    p++;
    cal->vpl = (*p >> 8 ) & 0xFFF;
    cal->vph = (*p >> 20) & 0xFFF;

    p++;
    cal->vcl = (*p >> 0 ) & 0xFFF;
    cal->vch = (*p >> 12) & 0xFFF;
}


const char *mem_get_chip_serial(void)
{
    static char buf[16 + 1];
    char *p = buf;
    uint32_t *words[4] = { (uint32_t *)0x008061FC,
                           (uint32_t *)0x00806010,
                           (uint32_t *)0x00806014,
                           (uint32_t *)0x00806018 };


    for (int i = 0; i < 4; i++)
    {
        sprintf(p, "%08X", *words[i]);
        p += 4;
    }
    *p = '\0';
    

    return (const char *)buf;
}
