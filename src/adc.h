#ifndef _ADC_H
#define _ADC_H

#include <atmel_start.h>

#include "mem.h"


extern double bandgap_voltage;


void adc_init(const void * const, mem_adc_cal_t *);
void adc_start(void);
void adc_print_stats(void);

#endif
