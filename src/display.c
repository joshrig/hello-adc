/*
  Basic asynchronous driver code for Atmel's OLED Xplained module.
 */
#include <string.h>

#include <atmel_start.h>

#include "display.h"
#include "font.h"


typedef struct
{
    uint8_t framebuf[512];

    struct spi_m_async_descriptor *spi;
    struct io_descriptor          *io;

    // various gpio control pins
    uint32_t reset_line;
    uint32_t data_cmd_sel_line;
    uint32_t slave_sel_line;
} display_state_t;

enum line_level {
    LOW = false,
    HIGH = true,
};

enum data_cmd_select {
    COMMAND = false,
    DATA = true,
};



static display_state_t display_state;



static void xfer_cb(const struct spi_m_async_descriptor *const io)
{
    // nop
}


void display_init
(
    struct spi_m_async_descriptor *spi,
    uint32_t                       reset_gpio,
    uint32_t                       data_cmd_sel_gpio,
    uint32_t                       slave_sel_gpio
)
{
    display_state_t *me = &display_state;

    struct io_descriptor *io;

    
    // grab the io descriptor
    spi_m_async_get_io_descriptor(spi, &io);

    // register a callback when the transfer is complete
    spi_m_async_register_callback(spi, SPI_M_ASYNC_CB_XFER, (FUNC_PTR)xfer_cb);

    // enable SPI in asynchronous mode
    spi_m_async_enable(spi);


    me->spi               = spi;
    me->io                = io;
    me->reset_line        = reset_gpio;
    me->data_cmd_sel_line = data_cmd_sel_gpio;
    me->slave_sel_line    = slave_sel_gpio;


    // this'll stay low for now since we only talk to one SPI device.
    gpio_set_pin_level    (me->slave_sel_line, LOW); 
    gpio_set_pin_direction(me->slave_sel_line, GPIO_DIRECTION_OUT);
    gpio_set_pin_function (me->slave_sel_line, GPIO_PIN_FUNCTION_OFF);


    // configure the OLED reset line, active low.
    gpio_set_pin_level    (me->reset_line, LOW); 
    gpio_set_pin_direction(me->reset_line, GPIO_DIRECTION_OUT);
    gpio_set_pin_function (me->reset_line, GPIO_PIN_FUNCTION_OFF);

    // stay low for a bit (datasheet says 3us)
    delay_us(5);

    // and come back high again.
    gpio_set_pin_level    (me->reset_line, HIGH); 


    // configure the OLED data/command select line.
    // DATA is active high, COMMAND is active low.
    gpio_set_pin_level    (me->data_cmd_sel_line, COMMAND);
    gpio_set_pin_direction(me->data_cmd_sel_line, GPIO_DIRECTION_OUT);
    gpio_set_pin_function (me->data_cmd_sel_line, GPIO_PIN_FUNCTION_OFF);


    uint8_t init_command[] = {
        // flip the display in software since the hardware is
        // upsidedown from the board silkscreen.
        0xDA, 0x22,       // Alt. COM config, En. COM L/R remap
        0xC8,             // COM Remap. Scan from COM[N-1] -> COM0
        0xA1,             // SEG Remap. Column addr. 127 -> SEG0
            
        0x81, 0xFF,       // Set Contrast to 256
            
        0x20, 0x00,       // Horizontal address mode
        0x22, 0x00, 0x03, // Set the end page to 3 (our display has 4 pages)

        0xD5, 0xF0,       // Set max oscillator frequency
        
        0x8D, 0x14,       // Enable charge pump

        0xAF              // Set Display On in normal mode
    };

    // initialize the display
    io_write(io, init_command, sizeof(init_command));
}


void display_clear(void)
{
    display_state_t *me = &display_state;


    display_clear_framebuffer();

    gpio_set_pin_level(me->data_cmd_sel_line, DATA);
    io_write(me->io, me->framebuf, sizeof(me->framebuf));
}


void display_clear_framebuffer(void)
{
    display_state_t *me = &display_state;


    memset(me->framebuf, 0, sizeof(me->framebuf));
}


void display_write_string(const char *str, uint32_t x, uint32_t y)
{
    display_state_t *me = &display_state;


    // print some text to the screen
    // this code basically makes the bit-addressable OLED display into
    // a 16x2 character display. DUMB.
    for (int i = 0; i < strlen(str); i++)
    {
        uint8_t topbuf[8], botbuf[8];
        int index = str[i] - 32;

        if (str[i] == '\r\n')
        {
            y++;
            x = 1;
            continue;
        }

        memcpy(topbuf, fonttbl[index]    , 8);
        memcpy(botbuf, fonttbl[index] + 8, 8);

        // XXX hacky hack hack, clamp
        if (x > 16) x = 16;
        if (y > 2) y = 2;
        
        memcpy(me->framebuf +  ((x - 1) * 8)        + ((y - 1) * 256), topbuf, 8);
        memcpy(me->framebuf + (((x - 1) * 8) + 128) + ((y - 1) * 256), botbuf, 8);

        x++;
    }

    gpio_set_pin_level(me->data_cmd_sel_line, DATA);
    io_write(me->io, me->framebuf, sizeof(me->framebuf));
}
