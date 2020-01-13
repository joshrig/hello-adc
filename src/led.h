#ifndef _LED_H
#define _LED_H

typedef enum
{
    kLEDCountMode,
    kLEDBlinkMode,
} led_mode_t;

void led_init(led_mode_t);
void led_update(void);
const char *led_get_mode_string();

#endif
