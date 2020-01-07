#include <stdio.h>
#include <string.h>

#include <atmel_start.h>

#include "uart.h"



typedef struct
{
    uint8_t *buf;
    uint32_t wc;
    uint32_t rc;

    bool inprogress;

    struct usart_async_descriptor *uart;
    struct io_descriptor          *io;
} uart_state_t;


static uart_state_t   uart_state;

#define kMaxBufferSize 1024
static uint8_t        buffer[kMaxBufferSize];



static void tx_cb_USART_0(const struct usart_async_descriptor *const io_descr)
{
    uart_state_t *me = &uart_state;

    me->inprogress = false;

    if (me->wc > me->rc)
    {
        io_write(me->io, &me->buf[me->rc], me->wc - me->rc);
        me->rc = me->wc;
    }
}




void uart_init(void)
{
    uart_state_t *me = &uart_state;

    struct io_descriptor *io;

    
    usart_async_register_callback(&USART_0, USART_ASYNC_TXC_CB, tx_cb_USART_0);
    usart_async_get_io_descriptor(&USART_0, &io);
    usart_async_enable(&USART_0);

    me->buf  = &buffer[0];
    me->wc   = 0;
    me->rc   = 0;
    me->uart = &USART_0;
    me->io   = io;
    me->inprogress = false;
}

void uart_write(const char *str)
{
    int len;
    uart_state_t *me = &uart_state;


    len = strlen(str);
    if (me->wc + len > kMaxBufferSize)
    {
        int ncpy = kMaxBufferSize - (me->wc + len);
        memcpy(&me->buf[me->wc], str, ncpy);

        if (!me->inprogress)
        {
            me->inprogress = true;
            io_write(me->io, &me->buf[me->rc], ncpy);
            me->rc += ncpy;
        }

        memcpy(me->buf, str + ncpy, strlen(str) - ncpy);
        me->wc = strlen(str) - ncpy;

        return;
    }

    memcpy(&me->buf[me->wc], str, strlen(str));
    me->wc += len;
    if (!me->inprogress)
    {
        me->inprogress = true;
        io_write(me->io, &me->buf[me->rc], len);
        me->rc += len;
    }
}
