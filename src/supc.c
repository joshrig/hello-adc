#include <stdio.h>

#include "supc.h"


void supc_init(void)
{
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

    printf("done.\r\n");
}
