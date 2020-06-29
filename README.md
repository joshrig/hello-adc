# hello-adc

This is a simple project to learn how to use the USART and SPI
interfaces asynchronously as well as how to gather samples from
the ADC using the DMA controller on the Atmel SAM E54 / D51

I'm using the Atmel START/ASF frameworks. They are a bit buggy
and non-orthogonal. I'll be re-writing the drivers for this
specific µC. 

A SPI interface is used to talk the OLED1 Xplained board
connected via EXT3 to the SAME54 Xplained board. The pin
assignments are as follows:

- PC04 MOSI
- PC05 SCLK
- PC07 MISO
- PC14 Slave Select
- PC31 Display Reset
- PC01 Data/Command Select

I'm also counting with the LEDs as a heartbeat via SysTick:

- PC18 LED0 (main)
- PB14 LED1 (IO1 via EXT2)
- PC10 LED2 (OLED1 via EXT3)
- PD11 LED3 (OLED1 via EXT3)
- PD10 LED4 (OLED1 via EXT3)

The ADC is configured to the use DMAC channel 0 via circularly
linked descriptors, each writing samples from the ADC to a
different buffer; This creates a double-buffer so I can perform
calculations on one while gathering samples on the other.

The ADC is sampling the light sensor on the IO1 Xplained board
via PB00 / AIN12 (EXT2). The values are averaged and displayed
on the OLED.

The USART is configured for 115200 8N1. There is a very simple
command shell on the UART for querying memory locations and
printing some module statistics; very useful for debugging.


To build and flash the application:

% make -f gcc/Makefile && edbg -b -t same54 -pv -f AtmelStart.bin

EDBG can be found here:

[EDBG](https://github.com/ataradov/edbg)

I'm using macOS 10.15 and MacPorts, so I installed the arm-none-eabi
toolchain from there:

% sudo port install arm-none-eabi-binutils arm-none-eabi-gcc arm-none-eabi-gdb

**NOTE**
If you find any of this useful, please let me know. Thanks!
