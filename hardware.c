/*
 * This file is part of the openloop hardware looper project.
 *
 * Copyright (C) 2018  Jonathan Halmen <jonathan@halmen.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#include "hardware.h"

/* TODO: this should probably go into wm8778.c */
void send_codec_cmd(uint16_t cmd)
{
	uint8_t data[2] = {cmd>>8, cmd};
	i2c_transfer7(I2C1, 0x34>>1, data, 2, 0, 0);
}

void i2s2_pin_setup(void)
{
	/* i2s2 pins */
	rcc_periph_clock_enable(RCC_GPIOC);
	gpio_set_af(GPIOC, GPIO_AF5, GPIO6);
	gpio_mode_setup(GPIOC, GPIO_MODE_AF,
			GPIO_PUPD_PULLDOWN, GPIO6);
	rcc_periph_clock_enable(RCC_GPIOB);
	gpio_set_af(GPIOB, GPIO_AF5, GPIO15 | GPIO13 | GPIO12);
	gpio_set_output_options(GPIOB, GPIO_OTYPE_PP,
			GPIO_OSPEED_25MHZ, GPIO13);
	gpio_mode_setup(GPIOB, GPIO_MODE_AF,
			GPIO_PUPD_PULLDOWN, GPIO15 | GPIO13 | GPIO12);
	/* i2s2ext pin */
	gpio_set_af(GPIOB, GPIO_AF6, GPIO14);
	gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_PULLDOWN, GPIO14);
}

void pll_setup(void)
{
	/* enable external oscillator first */
	rcc_osc_on(RCC_HSE);
	rcc_wait_for_osc_ready(RCC_HSE);
	/* set main pll 84MHz */
	uint8_t pllm = 2;
	uint16_t plln = 336;
	uint8_t pllp = 8;
	uint8_t pllq = 14;
	uint8_t pllr = 0;
	/* set flash waitstates! */
	flash_set_ws(FLASH_ACR_PRFTEN | FLASH_ACR_DCEN |
		FLASH_ACR_ICEN | FLASH_ACR_LATENCY_2WS);
	rcc_set_main_pll_hse(pllm, plln, pllp, pllq, pllr);
	/* set prescalers for the different domains */
	rcc_set_hpre(RCC_CFGR_HPRE_DIV_NONE);
	rcc_set_ppre1(RCC_CFGR_PPRE_DIV_2);
	rcc_set_ppre2(RCC_CFGR_PPRE_DIV_NONE);
	/* in case this is needed by the library: */
	rcc_ahb_frequency  = 84000000;
	rcc_apb1_frequency = 42000000;
	rcc_apb2_frequency = 84000000;
	/* finally enable pll */
	rcc_osc_on(RCC_PLL);
	rcc_wait_for_osc_ready(RCC_PLL);
	/* use pll as system clock */
	rcc_set_sysclk_source(RCC_CFGR_SW_PLL);
	rcc_wait_for_sysclk_status(RCC_PLL);
}

void systick_setup(uint32_t tick_frequency)
{
	/* systick_set_frequency (desired_systick_freq, current_freq_ahb) */
	/* returns false if frequency can't be set */
	if (!systick_set_frequency(tick_frequency,rcc_ahb_frequency))
		while (1);
	systick_counter_enable();
	systick_interrupt_enable();
}

void plli2s_setup(uint16_t n, uint8_t r)
{
	/* configure plli2s */
	/* void rcc_plli2s_config(uint16_t n, uint8_t r) */
	RCC_PLLI2SCFGR = (
		((n & RCC_PLLI2SCFGR_PLLI2SN_MASK) << RCC_PLLI2SCFGR_PLLI2SN_SHIFT) |
		((r & RCC_PLLI2SCFGR_PLLI2SR_MASK) << RCC_PLLI2SCFGR_PLLI2SR_SHIFT));
	/* enable pll */
	rcc_osc_on(RCC_PLLI2S);
	rcc_wait_for_osc_ready(RCC_PLLI2S);
}

/* TODO: unite these 4 functions into one? */
/* this might help? */
/* enum i2smode{ */
/* 	I2S_SLAVE_TRANSMIT, */
/* 	I2S_SLAVE_RECEIVE, */
/* 	I2S_MASTER_TRANSMIT, */
/* 	I2S_MASTER_RECEIVE */
/* }; */
void i2s_init_master_receive(uint32_t i2s, uint8_t div, uint8_t odd, uint8_t mckoe)
{
	/* set prescaler register */
	uint16_t pre = (((mckoe & 0x1) << 9) |
		((odd & 0x1) << 8) | div);
	SPI_I2SPR(i2s) = pre;
	/* set configuration register */
	uint32_t cfg = SPI_I2SCFGR_I2SMOD |
			SPI_I2SCFGR_I2SCFG_MASTER_RECEIVE |
			SPI_I2SCFGR_I2SSTD_I2S_PHILIPS |
			SPI_I2SCFGR_DATLEN_16BIT |
			SPI_I2SCFGR_CHLEN;  /*only if chlen=32bit*/
	SPI_I2SCFGR(i2s) |= cfg;
}

void i2s_init_master_transmit(uint32_t i2s, uint8_t div, uint8_t odd, uint8_t mckoe)
{
	/* set prescaler register */
	uint16_t pre = (((mckoe & 0x1) << 9) |
		((odd & 0x1) << 8) | div);
	SPI_I2SPR(i2s) = pre;
	/* set configuration register */
	uint32_t cfg = SPI_I2SCFGR_I2SMOD |
			SPI_I2SCFGR_I2SCFG_MASTER_TRANSMIT |
			SPI_I2SCFGR_I2SSTD_I2S_PHILIPS |
			SPI_I2SCFGR_DATLEN_16BIT |
			SPI_I2SCFGR_CHLEN;  /*only if chlen=32bit*/
	SPI_I2SCFGR(i2s) |= cfg;
}

void i2s_init_slave_transmit(uint32_t i2s)
{
	/* set configuration register */
	SPI_I2SCFGR(i2s) |= SPI_I2SCFGR_I2SMOD |
			SPI_I2SCFGR_I2SCFG_SLAVE_TRANSMIT |
			SPI_I2SCFGR_I2SSTD_I2S_PHILIPS |
			SPI_I2SCFGR_DATLEN_16BIT |
			SPI_I2SCFGR_CHLEN;  /*only if chlen=32bit*/
}

void i2s_init_slave_receive(uint32_t i2s)
{
	/* set configuration register */
	SPI_I2SCFGR(i2s) |= SPI_I2SCFGR_I2SMOD |
			SPI_I2SCFGR_I2SCFG_SLAVE_RECEIVE |
			SPI_I2SCFGR_I2SSTD_I2S_PHILIPS |
			SPI_I2SCFGR_DATLEN_16BIT |
			SPI_I2SCFGR_CHLEN;  /*only if chlen=32bit*/
}

void enable_i2s(uint32_t i2s)
{
	SPI_I2SCFGR(i2s) |= SPI_I2SCFGR_I2SE;
}

uint32_t i2s_read(uint32_t i2s)
{
	if (SPI_I2SCFGR(i2s) & SPI_I2SCFGR_DATLEN_32BIT) {
		uint16_t dat = SPI_DR(i2s);
		return dat << 16 | SPI_DR(i2s);
	}
	return SPI_DR(i2s);
}

uint8_t chkside(uint32_t i2s)
{
	return SPI_SR(i2s) & BIT2;
}

void i2c_setup(void)
{
	/* i2c pins */
	/* pb6 & pb7 */
	rcc_periph_clock_enable(RCC_GPIOB);
	gpio_set_af(GPIOB, GPIO_AF4, GPIO6 | GPIO7);
	gpio_set_output_options(GPIOB, GPIO_OTYPE_OD,
			GPIO_OSPEED_50MHZ, GPIO6 | GPIO7);
	gpio_mode_setup(GPIOB, GPIO_MODE_AF,
			GPIO_PUPD_NONE, GPIO6 | GPIO7);
	/* i2c periph*/
	rcc_periph_clock_enable(RCC_I2C1);
	i2c_set_clock_frequency(I2C1, I2C_CR2_FREQ_42MHZ);
	i2c_set_fast_mode(I2C1);
	/* ~ 200kHz freq: */
	/* i2c_set_ccr(I2C1, 38); */
	/* 100kHz freq: */
	/* i2c_set_ccr(I2C1, 42); */
	/* following results in approx 40kHz freq */
	i2c_set_ccr(I2C1, 336);
	/* max rise time = 300ns */
	i2c_set_trise(I2C1, 13);
	i2c_peripheral_enable(I2C1);
}
 /* delete this debug function */
uint32_t get_i2c_stat1(void)
{
	return I2C_SR1(I2C1);
}
 /* delete this debug function */
uint32_t get_i2c_stat2(void)
{
	return I2C_SR2(I2C1);
}

void init_dma_channel(struct dma_channel *chan)
{
	rcc_periph_clock_enable(chan->rcc);
	dma_set_transfer_mode(chan->dma, chan->stream, chan->direction);
	dma_set_peripheral_size(chan->dma, chan->stream, chan->psize);
	dma_set_peripheral_address(chan->dma, chan->stream, chan->paddress);
	if (chan->pinc)
		dma_enable_peripheral_increment_mode(chan->dma, chan->stream);
	dma_channel_select(chan->dma, chan->stream, chan->channel);
	if(chan->doublebuf) {
		dma_enable_double_buffer_mode(chan->dma, chan->stream);
		dma_set_memory_address_1(chan->dma, chan->stream, chan->maddress1);
	} else if (chan->circ) {
		dma_enable_circular_mode(chan->dma, chan->stream);
	}
	dma_set_memory_size(chan->dma, chan->stream, chan->msize);
	if (chan->minc)
		dma_enable_memory_increment_mode(chan->dma, chan->stream);
	dma_set_memory_address(chan->dma, chan->stream, chan->maddress);
	dma_set_number_of_data(chan->dma, chan->stream, chan->numberofdata);
	dma_set_priority(chan->dma, chan->stream, chan->prio);
	dma_enable_stream(chan->dma, chan->stream);
}

void setup_adc(void)
{
	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_ADC1);

	gpio_mode_setup(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO5);
	gpio_mode_setup(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO6);
	gpio_mode_setup(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO7);

	adc_set_resolution(ADC1, ADC_CR1_RES_10BIT);
	adc_set_sample_time_on_all_channels(ADC1, ADC_SMPR_SMP_480CYC);
	adc_set_right_aligned(ADC1);
	adc_enable_scan_mode(ADC1);
	adc_set_continuous_conversion_mode(ADC1);
	uint8_t channels[3] = {5,6,7};
	adc_set_regular_sequence(ADC1, 3, channels);

	adc_enable_dma(ADC1);
	adc_set_dma_continue(ADC1);
	adc_power_on(ADC1);
	/* Wait for ADC starting up. */
	int i;
	for (i = 0; i < 800000; i++)
		__asm__("nop");
	adc_start_conversion_regular(ADC1);
}
