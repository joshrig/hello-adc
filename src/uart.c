#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <atmel_start.h>

#include "uart.h"



#define RX_BUFLEN 256

typedef struct
{
    struct usart_async_descriptor *uart;
    struct io_descriptor          *io;
    bool                           tx_complete;

    // ring buffer used for getting data from the driver
    uint8_t                        rx_buf[RX_BUFLEN];
    uint8_t                        rx_wc;
    uint8_t                        rx_rc;

    uint8_t                        cmd_buf[RX_BUFLEN];
    uint8_t                        cmd_wc;
} uart_state_t;


static uart_state_t   uart_state;


static void execute_command(const char *);


static void tx_cb_USART_0(const struct usart_async_descriptor *const io_descr)
{
    uart_state_t *me = &uart_state;

    me->tx_complete = true;
}

static void rx_cb_USART_0(const struct usart_async_descriptor *const io_descr)
{
    uart_state_t *me = &uart_state;
    
    // we READ from the UART and WRITE to the READ buffer
    io_read(me->io, &me->rx_buf[me->rx_wc++], 1);
}




void uart_init(void)
{
    uart_state_t *me = &uart_state;

    struct io_descriptor *io;

    
    usart_async_register_callback(&USART_0, USART_ASYNC_TXC_CB, tx_cb_USART_0);
    usart_async_register_callback(&USART_0, USART_ASYNC_RXC_CB, rx_cb_USART_0);
    usart_async_get_io_descriptor(&USART_0, &io);
    usart_async_enable(&USART_0);

    me->uart   = &USART_0;
    me->io     = io;
    me->rx_wc  = 0;
    me->rx_rc  = (uint8_t)RX_BUFLEN;
    me->cmd_wc = 0;
}


void uart_start_shell(void)
{
    printf("%% ");
    fflush(stdout);
}


void uart_do_shell(void)
{
    uart_state_t *me = &uart_state;


    while (me->rx_wc - me->rx_rc > 0)
    {
        // copy to command buffer
        me->cmd_buf[me->cmd_wc] = me->rx_buf[me->rx_rc++];

        // check for carriage return
        if (me->cmd_buf[me->cmd_wc] == '\r')
        {
            me->cmd_buf[me->cmd_wc] = '\0';

            printf("\r\n");

            if (strlen((const char *)me->cmd_buf) > 0)
                execute_command((const char *)me->cmd_buf);

            me->cmd_wc = 0;
            me->cmd_buf[0] = '\0';

            printf("%% ");
        }
        else if (me->cmd_buf[me->cmd_wc] == '\b')
        {
            if (me->cmd_wc > 0)
            {
                me->cmd_wc--;
                printf("\b \b");
            }
            else
            {
                printf("\b ");
            }
        }
        else if (me->cmd_buf[me->cmd_wc] < 0x20)
        {
            ;//NOP
        }
        else
        {
            // echo back character (XXX check for overflow???)
            printf("%c", me->cmd_buf[me->cmd_wc++]);
        }
        fflush(stdout);
    }
}


int _write(int file, char *ptr, int len)
{
    uart_state_t *me = &uart_state;
    int32_t ret;


    me->tx_complete = false;
retry:
    ret = io_write(me->io, (uint8_t *)ptr, len);
    if (ret == ERR_NO_RESOURCE)
        goto retry;
    
    while (!me->tx_complete)
        __WFI();

    
    return ret;
}



int _read(int file, char *ptr, int len)
{
#if 0
    uart_state_t *me = &uart_state;
    int nrem = len;

    
    while (nrem-- > 0)
    {
        while (me->rx_wc - me->rx_rc == 0)
            __WFI();

        *ptr++ = me->rx_buf[me->rx_rc++];
    }

    return len;
#else
    return -1;
#endif
}


static void execute_command(const char *cmd)
{
    char *pch;

    pch = strtok((char *)cmd, " ");
    if (pch && strcmp(pch, "b") == 0)
    {
        uint32_t addr = 0x00000000;
        uint32_t len = 1;
        
        // print memory command
        pch = strtok(NULL, " ");
        if (pch)
            addr = (uint32_t)strtol(pch, NULL, 16);

        pch = strtok(NULL, " ");
        if (pch)
            len = (uint32_t)strtol(pch, NULL, 16);

        for (int i = 0; i < len; i++)
            printf("0x%08X: 0x%02X\r\n", addr + i, *((uint8_t *)addr + i));
    }
}
