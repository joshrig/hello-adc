#ifndef _MEM_H
#define _MEM_H

#include <atmel_start.h>


typedef struct mem_adc_cal
{
    uint8_t biascomp;
    uint8_t biasrefbuf;
    uint8_t biasr2r;
} mem_adc_cal_t;


typedef struct mem_tsens_cal
{
    double   tl;
    double   th;
    uint16_t vpl;
    uint16_t vph;
    uint16_t vcl;
    uint16_t vch;

    // for debug / reference only
    uint8_t tli;
    uint8_t tld;
    uint8_t thi;
    uint8_t thd;
} mem_tsens_cal_t;


void mem_get_adc_cal(mem_adc_cal_t *);
void mem_get_tsens_cal(mem_tsens_cal_t *);
const char *mem_get_chip_serial(void);


#endif 
