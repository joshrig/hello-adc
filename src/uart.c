#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <atmel_start.h>

#include "uart.h"



#define RX_BUFLEN 256

#define CMD_HISTLEN 4
#define CMD_HISTMASK 0x03



typedef struct
{
    struct usart_async_descriptor *uart;
    struct io_descriptor          *io;
    bool                           tx_complete;

    // ring buffer used for getting data from the driver
    uint8_t                        rx_buf[RX_BUFLEN];
    uint8_t                        rx_wc;
    uint8_t                        rx_rc;

    // command buffer
    uint8_t                        cmd_buf[RX_BUFLEN];
    uint8_t                        cmd_wc;

    // history buffer
    uint8_t                        hist[CMD_HISTLEN][RX_BUFLEN];
    uint8_t                        hist_wc;

    // counters
    uint32_t                       nerr;
    uint32_t                       ntx;
    uint32_t                       ntx_retry;
    uint32_t                       nrx;
} uart_state_t;



static void execute_command(const char *);
static void add_to_history(const char *);



static uart_state_t uart_state;



static void tx_cb_USART_0(const struct usart_async_descriptor *const io_descr)
{
    uart_state_t *me = &uart_state;

    me->tx_complete = true;
}


static void rx_cb_USART_0(const struct usart_async_descriptor *const io_descr)
{
    uart_state_t *me = &uart_state;
    
    // we READ from the UART and WRITE to the READ ring buffer
    io_read(me->io, &me->rx_buf[me->rx_wc++], 1);

    me->nrx++;
}


static void err_cb_USART_0(const struct usart_async_descriptor *const io_descr)
{
    uart_state_t *me = &uart_state;

    me->nerr++;
}


void uart_init(void)
{
    uart_state_t *me = &uart_state;

    struct io_descriptor *io;

    
    usart_async_register_callback(&USART_0, USART_ASYNC_TXC_CB, tx_cb_USART_0);
    usart_async_register_callback(&USART_0, USART_ASYNC_RXC_CB, rx_cb_USART_0);
    usart_async_register_callback(&USART_0, USART_ASYNC_ERROR_CB, err_cb_USART_0);
    usart_async_get_io_descriptor(&USART_0, &io);
    usart_async_enable(&USART_0);

    me->uart      = &USART_0;
    me->io        = io;
    me->rx_wc     = 0;
    me->rx_rc     = 0;
    me->cmd_wc    = 0;
    me->nerr      = 0;
    me->ntx       = 0;
    me->nrx       = 0;
    me->ntx_retry = 0;

    for (int i = 0; i < CMD_HISTLEN; i++)
        me->hist[i][0] = '\0';
    me->hist_wc = 0;
}


void uart_start_shell(void)
{
    printf("%% ");
    fflush(stdout);
}


void uart_do_shell(void)
{
#define RESET_CMDBUF() {                        \
    me->cmd_wc     = 0;                         \
    me->cmd_buf[0] = '\0';                      \
    printf("%% ");                              \
    }

    uart_state_t *me = &uart_state;


    while ((uint8_t)(me->rx_wc - me->rx_rc) > 0)
    {
        // copy to command buffer
        me->cmd_buf[me->cmd_wc] = me->rx_buf[me->rx_rc++];

        // shorthand
        char c  = me->cmd_buf[me->cmd_wc];


        // check for carriage return
        if (c == '\r')
        {
            me->cmd_buf[me->cmd_wc] = '\0';

            printf("\r\n");

            if (strlen((const char *)me->cmd_buf) > 0)
            {
                add_to_history((const char *)me->cmd_buf);
                execute_command((const char *)me->cmd_buf);
            }

            RESET_CMDBUF();
        }
        else if (c == '\b')
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
        else if (c == 0x03)
        {
            printf("\r\n");
            RESET_CMDBUF();
        }
        else if (c == 0x10)
        {
            // ctrl-p
            
            // clear line
            while (me->cmd_wc--)
                printf("\b \b");

            // copy from the history buffer to the command buffer
            me->hist_wc--;
            me->cmd_buf[0] = '\0';
            strcpy(me->cmd_buf, me->hist[me->hist_wc & CMD_HISTMASK]);
            me->cmd_wc = strlen(me->cmd_buf);
            
            printf("%s", me->cmd_buf);
        }
        else if (c < 0x20)
        {
            printf("[0x%X] ", c);
            ;//NOP
        }
        else
        {
            // echo back character
            printf("%c", c);

            // XXX check for overflow
            me->cmd_wc++;
        }
        
        fflush(stdout);
    }
}


void uart_print_stats(void)
{
    uart_state_t *me = &uart_state;

    printf("USART:\r\n");
    printf(" me->nerr:      %ld\r\n", me->nerr);
    printf(" me->ntx:       %ld\r\n", me->ntx);
    printf(" me->nrx:       %ld\r\n", me->nrx);
    printf(" me->ntx_retry: %ld\r\n", me->ntx_retry);
    printf(" me->rx_wc:     %d\r\n", me->rx_wc);
    printf(" me->rx_rc:     %d\r\n", me->rx_rc);
    printf(" me->hist_wc:   %d\r\n", me->hist_wc & CMD_HISTMASK);
    for (int i = 0; i < CMD_HISTLEN; i++)
        printf("  %d: [%s] %s\r\n", i, me->hist[i], i == (me->hist_wc & CMD_HISTMASK) ? "*" : "");
}


int _write(int file, char *ptr, int len)
{
    uart_state_t *me = &uart_state;
    int32_t ret;


    me->tx_complete = false;
retry:
    ret = io_write(me->io, (uint8_t *)ptr, len);
    if (ret == ERR_NO_RESOURCE)
    {
        me->ntx_retry++;
        goto retry;
    }

    // XXX hacky hacky hack
    while (!me->tx_complete)
        __WFI();


    me->ntx += len;
    
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


static void add_to_history(const char *cmd)
{
    uart_state_t *me = &uart_state;

    strncpy(me->hist[me->hist_wc & CMD_HISTMASK], cmd, RX_BUFLEN);
    me->hist_wc++;
}


static void execute_command(const char *cmd)
{
    char *pch;


    if (!cmd)
        return;
    
    pch = strtok((char *)cmd, " ");
    if (!pch)
        return;
    
    if (*pch == '?')
    {
        printf("commands:\r\n");
        printf("?                         help\r\n");
        printf("^p                        cycle through command history ring buf\r\n");
        printf("s[tats]                   print various module's statistics\r\n");
        printf("w[ord] [0xaddr] [len]     print len words starting at 'addr'\r\n");
        printf("h[alf] [0xaddr] [len]     print len half-words starting at 'addr'\r\n");
        printf("b[yte] [0xaddr] [len]     print len bytes starting at 'addr'\r\n");
    }
    else if (*pch == 'b' || *pch == 'h' || *pch == 'w')
    {
        char     width = *pch;
        uint32_t addr  = 0x00000000;
        uint32_t len   = 1;
        
        // print memory command
        pch = strtok(NULL, " ");
        if (pch)
        {
            addr = (uint32_t)strtol(pch, NULL, 16);
            // XXX
            // errno isn't set in this libc implementation and an
            // address of 0 is valid, so we can't check for conversion
            // error. that's ok since we just default to 0x00000000
            // anyhow.
            if (addr == 0 && errno == EINVAL)
            {
                printf("syntax error\r\n");
                return;
            }
        }

        pch = strtok(NULL, " ");
        if (pch)
        {
            len = (uint32_t)strtol(pch, NULL, 10);
            if (len == 0)
            {
                printf("syntax error\r\n");
                return;
            }
        }

        for (int i = 0; i < len; i++)
        {
            switch (width)
            {
            case 'b':
                printf("0x%08X: 0x%02X\r\n", addr + i, *((uint8_t *)addr + i));
                break;
            case 'h':
                printf("0x%08X: 0x%04X\r\n", addr + i, *((uint16_t *)addr + i));
                break;
            case 'w':
                printf("0x%08X: 0x%08X\r\n", addr + i, *((uint32_t *)addr + i));
                break;
            }
        }
    }
    else if (*pch == 's')
    {
        uart_print_stats();
        extern void adc_print_stats();
        adc_print_stats();
    }
    else
    {
        printf("unknown command\r\n");
    }
}
