# hello-adc

Simple project to learn how to use the ADC and the SPI interfaces
asynchronously using the Atmel START/ASF4 frameworks.

The SPI interface will be used to talk the OLED Xplained board
connected via EXT3 to the SAME54 Xplained board. The pins assignments
are as follows:

- PC04 MOSI
- PC05 SCLK
- PC07 MISO
- PC14 Slave Select
- PC31 Display Reset
- PC01 Data/Command Select

I'm also driving the LEDs via SysTick:

- PC18 LED0
- PC10 LED1
- PD11 LED2
- PD10 LED3

The ADC is configured to use the internal temperature sensors on the
die and display the temps on the OLED display.

**NOTE**

- The display code for the OLED is utter garbage. It effectively
  turns the bit-addressable display into a 16x2 display.

To build and flash the application:

% make -f gcc/Makefile && edbg -b -t same54 -pv -f AtmelStart.bin


EDBG can be found here:

[EDBG](https://github.com/ataradov/edbg)

I'm using macOS 10.15 and MacPorts, so I installed the arm-none-eabi
toolchain from there:

% sudo port install arm-none-eabi-binutils arm-none-eabi-gcc arm-none-eabi-gdb