#include <atmel_start.h>

#include "led.h"



typedef struct
{
    uint8_t  val;

#define NLEDS 4
    uint32_t leds[NLEDS];

} led_state_t;



static led_state_t led_state = {
    .val = 0, 
    .leds = {
        GPIO(GPIO_PORTC, 18),
        GPIO(GPIO_PORTC, 10),
        GPIO(GPIO_PORTD, 11),
        GPIO(GPIO_PORTD, 10),
    }
};




void led_init(void)
{
    led_state_t *me = &led_state;


    // initialize the LEDS
    for (int i = 0; i < NLEDS; i++)
    {
        gpio_set_pin_level    (me->leds[i], true);
        gpio_set_pin_direction(me->leds[i], GPIO_DIRECTION_OUT);
        gpio_set_pin_function (me->leds[i], GPIO_PIN_FUNCTION_OFF);
    }
}


void led_update(void)
{
    led_state_t *me = &led_state;


    for (int i = 0; i < NLEDS; i++)
        gpio_set_pin_level(me->leds[i], !((me->val >> i) & 0x01));

    me->val++;
}
