/*
 * Code generated from Atmel Start.
 *
 * This file will be overwritten when reconfiguring your Atmel Start project.
 * Please copy examples or other code you want to keep to a separate file
 * to avoid losing it when reconfiguring.
 */

#include "driver_init.h"
#include <peripheral_clk_config.h>
#include <utils.h>
#include <hal_init.h>

#include <hpl_adc_dma.h>

/*! The buffer size for USART */
#define USART_0_BUFFER_SIZE 16

struct _adc_dma_device         ADC_0;
struct usart_async_descriptor       USART_0;

static uint8_t USART_0_buffer[USART_0_BUFFER_SIZE];

struct spi_m_async_descriptor SPI_0;

/**
 * \brief ADC initialization function
 *
 * Enables ADC peripheral, clocks and initializes ADC driver
 */
void ADC_0_init(void)
{
	hri_mclk_set_APBDMASK_ADC0_bit(MCLK);
	hri_gclk_write_PCHCTRL_reg(GCLK, ADC0_GCLK_ID, CONF_GCLK_ADC0_SRC | (1 << GCLK_PCHCTRL_CHEN_Pos));
        _adc_dma_init(&ADC_0, ADC0);
}

/**
 * \brief USART Clock initialization function
 *
 * Enables register interface and peripheral clock
 */
void USART_0_CLOCK_init()
{

	hri_gclk_write_PCHCTRL_reg(GCLK, SERCOM2_GCLK_ID_CORE, CONF_GCLK_SERCOM2_CORE_SRC | (1 << GCLK_PCHCTRL_CHEN_Pos));
	hri_gclk_write_PCHCTRL_reg(GCLK, SERCOM2_GCLK_ID_SLOW, CONF_GCLK_SERCOM2_SLOW_SRC | (1 << GCLK_PCHCTRL_CHEN_Pos));

	hri_mclk_set_APBBMASK_SERCOM2_bit(MCLK);
}

/**
 * \brief USART pinmux initialization function
 *
 * Set each required pin to USART functionality
 */
void USART_0_PORT_init()
{

	gpio_set_pin_function(PB25, PINMUX_PB25D_SERCOM2_PAD0);

	gpio_set_pin_function(PB24, PINMUX_PB24D_SERCOM2_PAD1);
}

/**
 * \brief USART initialization function
 *
 * Enables USART peripheral, clocks and initializes USART driver
 */
void USART_0_init(void)
{
	USART_0_CLOCK_init();
	usart_async_init(&USART_0, SERCOM2, USART_0_buffer, USART_0_BUFFER_SIZE, (void *)NULL);
	USART_0_PORT_init();
}

void SPI_0_PORT_init(void)
{

	gpio_set_pin_level(PC04,
	                   // <y> Initial level
	                   // <id> pad_initial_level
	                   // <false"> Low
	                   // <true"> High
	                   false);

	// Set pin direction to output
	gpio_set_pin_direction(PC04, GPIO_DIRECTION_OUT);

	gpio_set_pin_function(PC04, PINMUX_PC04C_SERCOM6_PAD0);

	gpio_set_pin_level(PC05,
	                   // <y> Initial level
	                   // <id> pad_initial_level
	                   // <false"> Low
	                   // <true"> High
	                   false);

	// Set pin direction to output
	gpio_set_pin_direction(PC05, GPIO_DIRECTION_OUT);

	gpio_set_pin_function(PC05, PINMUX_PC05C_SERCOM6_PAD1);

	// Set pin direction to input
	gpio_set_pin_direction(PC07, GPIO_DIRECTION_IN);

	gpio_set_pin_pull_mode(PC07,
	                       // <y> Pull configuration
	                       // <id> pad_pull_config
	                       // <GPIO_PULL_OFF"> Off
	                       // <GPIO_PULL_UP"> Pull-up
	                       // <GPIO_PULL_DOWN"> Pull-down
	                       GPIO_PULL_OFF);

	gpio_set_pin_function(PC07, PINMUX_PC07C_SERCOM6_PAD3);
}

void SPI_0_CLOCK_init(void)
{
	hri_gclk_write_PCHCTRL_reg(GCLK, SERCOM6_GCLK_ID_CORE, CONF_GCLK_SERCOM6_CORE_SRC | (1 << GCLK_PCHCTRL_CHEN_Pos));
	hri_gclk_write_PCHCTRL_reg(GCLK, SERCOM6_GCLK_ID_SLOW, CONF_GCLK_SERCOM6_SLOW_SRC | (1 << GCLK_PCHCTRL_CHEN_Pos));

	hri_mclk_set_APBDMASK_SERCOM6_bit(MCLK);
}

void SPI_0_init(void)
{
	SPI_0_CLOCK_init();
	spi_m_async_init(&SPI_0, SERCOM6);
	SPI_0_PORT_init();
}

void system_init(void)
{
	init_mcu();

	ADC_0_init();

	USART_0_init();

	SPI_0_init();
}
