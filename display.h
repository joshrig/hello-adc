#ifndef _DISPLAY_H
#define _DISPLAY_H

#include <atmel_start.h>


void display_init(struct spi_m_async_descriptor *, uint32_t, uint32_t, uint32_t);

void display_clear(void);

void display_clear_framebuffer(void);

void display_write_string(const char *, uint32_t, uint32_t);

#endif
