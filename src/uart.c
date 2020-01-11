#include <stdio.h>
#include <string.h>

#include <atmel_start.h>

#include "uart.h"



typedef struct
{
    uint8_t *buf;

    struct usart_async_descriptor *uart;
    struct io_descriptor          *io;
} uart_state_t;


static uart_state_t   uart_state;

#define kMaxBufferSize 1024
static uint8_t        buffer[kMaxBufferSize];



static void tx_cb_USART_0(const struct usart_async_descriptor *const io_descr)
{
}

static void rx_cb_USART_0(const struct usart_async_descriptor *const io_descr)
{
    uart_state_t *me = &uart_state;
}




void uart_init(void)
{
    uart_state_t *me = &uart_state;

    struct io_descriptor *io;

    
    usart_async_register_callback(&USART_0, USART_ASYNC_TXC_CB, tx_cb_USART_0);
    usart_async_register_callback(&USART_0, USART_ASYNC_RXC_CB, rx_cb_USART_0);
    usart_async_get_io_descriptor(&USART_0, &io);
    usart_async_enable(&USART_0);

    me->buf  = &buffer[0];
    me->uart = &USART_0;
    me->io   = io;
}


void uart_write(const char *str)
{
    int len = strlen(str);
    uart_state_t *me = &uart_state;


    // wait for the current write to complete, if any
    while (!usart_async_is_tx_empty(me->uart));
    while (!usart_async_is_tx_empty(me->uart));
    while (!usart_async_is_tx_empty(me->uart));
    while (!usart_async_is_tx_empty(me->uart));
    while (!usart_async_is_tx_empty(me->uart));
    while (!usart_async_is_tx_empty(me->uart));

    memcpy(me->buf, str, len);
    io_write(me->io, me->buf, len);
}
