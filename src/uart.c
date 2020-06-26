#include <stdio.h>
#include <string.h>

#include <atmel_start.h>

#include "uart.h"



typedef struct
{
    struct usart_async_descriptor *uart;
    struct io_descriptor          *io;
} uart_state_t;


static uart_state_t   uart_state;



static void tx_cb_USART_0(const struct usart_async_descriptor *const io_descr)
{
}

static void rx_cb_USART_0(const struct usart_async_descriptor *const io_descr)
{
}




void uart_init(void)
{
    uart_state_t *me = &uart_state;

    struct io_descriptor *io;

    
    usart_async_register_callback(&USART_0, USART_ASYNC_TXC_CB, tx_cb_USART_0);
    usart_async_register_callback(&USART_0, USART_ASYNC_RXC_CB, rx_cb_USART_0);
    usart_async_get_io_descriptor(&USART_0, &io);
    usart_async_enable(&USART_0);

    me->uart = &USART_0;
    me->io   = io;
}


int _write(int file, char *ptr, int len)
{
    uart_state_t *me = &uart_state;


    // XXX polling byte-at-a-time, bad bad bad
    for (int todo = 0; todo < len; todo++)
    {
        io_write(me->io, (uint8_t *)ptr, 1);
        while (!usart_async_is_tx_empty(me->uart));
        if (*ptr == '\n')
        {
            uint8_t lf[1] = { '\r' };
            io_write(me->io, lf, 1);
            while (!usart_async_is_tx_empty(me->uart));
        }
        ptr++;
    }

    return len;
}


// XXX borked
int _read(int file, char *ptr, int len)
{
    // uart_state_t *me = &uart_state;
    // int j = 0;

    // uint8_t buf[50];
    // while (1)
    // {
    //     j += io_read(me->io, (uint8_t *)&buf[j], 1);

    //     io_write(me->io, (uint8_t *)&buf[j], 1);
    //     while (!usart_async_is_tx_empty(me->uart));

    //     if (buf[j] == '\r')
    //         break;
    // }


    // memcpy(ptr, buf, j);

    // return j;
    return -1;
}
