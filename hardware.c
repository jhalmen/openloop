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

void codec_send_cmd(uint16_t cmd)
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
	/* finally enable pll */
	rcc_osc_on(RCC_PLL);
	rcc_wait_for_osc_ready(RCC_PLL);
	/* use pll as system clock */
	rcc_set_sysclk_source(RCC_CFGR_SW_PLL);
	rcc_wait_for_sysclk_status(RCC_PLL);
	/* in case this is needed by the library: */
	rcc_ahb_frequency  = 84000000;
	rcc_apb1_frequency = 42000000;
	rcc_apb2_frequency = 84000000;
	rcc_osc_off(RCC_HSI);
}

void systick_setup(uint32_t tick_frequency)
{
	/* returns false if frequency can't be set */
	while (!systick_set_frequency(tick_frequency, rcc_ahb_frequency));
	systick_counter_enable();
	systick_interrupt_enable();
}

void plli2s_setup(uint16_t n, uint8_t r)
{
	rcc_plli2s_config(n, r);
	rcc_osc_on(RCC_PLLI2S);
	rcc_wait_for_osc_ready(RCC_PLLI2S);
}

void i2s_init_master(uint32_t i2s,
		uint8_t chlen,
		uint8_t div,
		uint8_t odd,
		uint8_t mckoe,
		uint8_t i2smode)
{
	/* set prescaler register */
	uint16_t pre = (((mckoe & 0x1) << 9) |
		((odd & 0x1) << 8) | div);
	SPI_I2SPR(i2s) = pre;
	/* set configuration register */
	uint32_t cfg = SPI_I2SCFGR_I2SMOD |
		(i2smode << SPI_I2SCFGR_I2SCFG_LSB) |
		(SPI_I2SCFGR_I2SSTD_I2S_PHILIPS << SPI_I2SCFGR_I2SSTD_LSB) |
		(SPI_I2SCFGR_DATLEN_16BIT << SPI_I2SCFGR_DATLEN_LSB) |
		(chlen==32?SPI_I2SCFGR_CHLEN:0);
	SPI_I2SCFGR(i2s) |= cfg;
}

void i2s_init_master_receive(uint32_t i2s, uint8_t div, uint8_t odd, uint8_t mckoe)
{
	i2s_init_master(i2s, 32, div, odd, mckoe,SPI_I2SCFGR_I2SCFG_MASTER_RECEIVE);
	/* /1* set prescaler register *1/ */
	/* uint16_t pre = (((mckoe & 0x1) << 9) | */
	/* 	((odd & 0x1) << 8) | div); */
	/* SPI_I2SPR(i2s) = pre; */
	/* /1* set configuration register *1/ */
	/* uint32_t cfg = SPI_I2SCFGR_I2SMOD | */
	/* 		SPI_I2SCFGR_I2SCFG_MASTER_RECEIVE | */
	/* 		SPI_I2SCFGR_I2SSTD_I2S_PHILIPS | */
	/* 		SPI_I2SCFGR_DATLEN_16BIT | */
	/* 		SPI_I2SCFGR_CHLEN;  */
	/* SPI_I2SCFGR(i2s) |= cfg; */
}

void i2s_init_master_transmit(uint32_t i2s, uint8_t div, uint8_t odd, uint8_t mckoe)
{
	i2s_init_master(i2s, 32, div, odd, mckoe,SPI_I2SCFGR_I2SCFG_MASTER_TRANSMIT);
	/* /1* set prescaler register *1/ */
	/* uint16_t pre = (((mckoe & 0x1) << 9) | */
	/* 	((odd & 0x1) << 8) | div); */
	/* SPI_I2SPR(i2s) = pre; */
	/* /1* set configuration register *1/ */
	/* uint32_t cfg = SPI_I2SCFGR_I2SMOD | */
	/* 		SPI_I2SCFGR_I2SCFG_MASTER_TRANSMIT | */
	/* 		SPI_I2SCFGR_I2SSTD_I2S_PHILIPS | */
	/* 		SPI_I2SCFGR_DATLEN_16BIT | */
	/* 		SPI_I2SCFGR_CHLEN;  */
	/* SPI_I2SCFGR(i2s) |= cfg; */
}

void i2s_init_slave(uint32_t i2s, uint8_t i2smode)
{
	uint8_t chlen = 32;
	/* set configuration register */
	SPI_I2SCFGR(i2s) = SPI_I2SCFGR_I2SMOD |
		(i2smode << SPI_I2SCFGR_I2SCFG_LSB) |
		(SPI_I2SCFGR_I2SSTD_I2S_PHILIPS << SPI_I2SCFGR_I2SSTD_LSB) |
		(SPI_I2SCFGR_DATLEN_16BIT << SPI_I2SCFGR_DATLEN_LSB) |
		(chlen==32?SPI_I2SCFGR_CHLEN:0);
}

void i2s_init_slave_transmit(uint32_t i2s)
{
	i2s_init_slave(i2s, SPI_I2SCFGR_I2SCFG_SLAVE_TRANSMIT);
	/* /1* set configuration register *1/ */
	/* SPI_I2SCFGR(i2s) |= SPI_I2SCFGR_I2SMOD | */
	/* 		SPI_I2SCFGR_I2SCFG_SLAVE_TRANSMIT | */
	/* 		SPI_I2SCFGR_I2SSTD_I2S_PHILIPS | */
	/* 		SPI_I2SCFGR_DATLEN_16BIT | */
	/* 		SPI_I2SCFGR_CHLEN;  */
}

void i2s_init_slave_receive(uint32_t i2s)
{
	i2s_init_slave(i2s, SPI_I2SCFGR_I2SCFG_SLAVE_RECEIVE);
	/* /1* set configuration register *1/ */
	/* SPI_I2SCFGR(i2s) |= SPI_I2SCFGR_I2SMOD | */
	/* 		SPI_I2SCFGR_I2SCFG_SLAVE_RECEIVE | */
	/* 		SPI_I2SCFGR_I2SSTD_I2S_PHILIPS | */
	/* 		SPI_I2SCFGR_DATLEN_16BIT | */
	/* 		SPI_I2SCFGR_CHLEN;   */
}

void i2s_enable(uint32_t i2s)
{
	SPI_I2SCFGR(i2s) |= SPI_I2SCFGR_I2SE;
}

void i2s_disable(uint32_t i2s)
{
	SPI_I2SCFGR(i2s) &= ~SPI_I2SCFGR_I2SE;
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
	i2c_reset(I2C1);
	/* i2c pins */
	/* pb6 & pb7 */
	rcc_periph_clock_enable(RCC_GPIOB);
	gpio_set_af(GPIOB, GPIO_AF4, GPIO6 | GPIO7);
	gpio_set_output_options(GPIOB, GPIO_OTYPE_OD,
			GPIO_OSPEED_50MHZ, GPIO6 | GPIO7);
	gpio_mode_setup(GPIOB, GPIO_MODE_AF,
			GPIO_PUPD_PULLUP, GPIO6 | GPIO7);
	/* i2c periph*/
	rcc_periph_clock_enable(RCC_I2C1);
	i2c_set_clock_frequency(I2C1, I2C_CR2_FREQ_42MHZ);
	i2c_set_fast_mode(I2C1);
	/* ~ 200kHz freq: */
	i2c_set_ccr(I2C1, 38);
	/* 100kHz freq: */
	/* i2c_set_ccr(I2C1, 42); */
	/* following results in approx 40kHz freq */
	/* i2c_set_ccr(I2C1, 336); */
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

void adc_setup(void)
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
	/* adc_set_continuous_conversion_mode(ADC1); */
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

void sound_setup(struct i2sfreq * f)
{
	spi_reset(I2S2ext);
	spi_reset(I2S2);
	i2s2_pin_setup();
	/* enable SPI2/I2S2 peripheral clock */
	rcc_periph_clock_enable(RCC_SPI2);
	plli2s_setup(f->plln, f->pllr);
	i2s_init_slave_receive(I2S2ext);
	i2s_init_master_transmit(I2S2, f->div, f->odd, 1);
	nvic_set_priority(NVIC_SPI2_IRQ, 0);
	nvic_enable_irq(NVIC_SPI2_IRQ);
	/* slave has to be enabled before the master! */
	i2s_enable(I2S2ext);
	i2s_enable(I2S2);
}

void encoder_setup(void)
{
	/* take care of pins */
	rcc_periph_clock_enable(RCC_GPIOA);
	gpio_set_af(GPIOA, GPIO_AF1, GPIO0 | GPIO1);
	gpio_mode_setup(GPIOA, GPIO_MODE_AF,
			GPIO_PUPD_PULLUP, GPIO0 | GPIO1);
	/* configure timer in encoder mode */
	rcc_periph_clock_enable(RCC_TIM2);
	timer_set_period(TIM2, 1023);
	timer_slave_set_mode(TIM2, 0x3); // encoder
	timer_ic_set_input(TIM2, TIM_IC1, TIM_IC_IN_TI1);
	timer_ic_set_input(TIM2, TIM_IC2, TIM_IC_IN_TI2);
	/* timer_ic_enable(TIM2, TIM_IC1); */
	/* timer_ic_enable(TIM2, TIM_IC2); */
	timer_enable_counter(TIM2);
}

uint8_t encpos(void)
{
	return timer_get_counter(TIM2) >> 2;
}

void leds_setup(void)
{
	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_GPIOB);
	gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT,
			GPIO_PUPD_NONE, GPIO10);
	gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT,
			GPIO_PUPD_NONE, GPIO10 | GPIO3);
}


void buttons_setup(void)
{
	rcc_periph_clock_enable(RCC_GPIOA);
	gpio_mode_setup(GPIOA, GPIO_MODE_INPUT,
			GPIO_PUPD_PULLUP, GPIO2 | GPIO11 | GPIO12);
	rcc_periph_clock_enable(RCC_SYSCFG);
	exti_select_source(EXTI2 | EXTI11 | EXTI12, GPIOA);
	exti_set_trigger(EXTI2 | EXTI11, EXTI_TRIGGER_FALLING);
	exti_set_trigger(EXTI12, EXTI_TRIGGER_FALLING);
	exti_enable_request(EXTI2 | EXTI11 | EXTI12);
	nvic_set_priority(NVIC_EXTI15_10_IRQ, 0b01110000);
	nvic_set_priority(NVIC_EXTI2_IRQ, 0b01110000);
	nvic_enable_irq(NVIC_EXTI15_10_IRQ);
	nvic_enable_irq(NVIC_EXTI2_IRQ);
}

void sddetect_setup(void)
{
	/* sdcard detect pin */
	rcc_periph_clock_enable(RCC_GPIOA);
	gpio_mode_setup(GPIOA, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, GPIO15);
}

uint8_t sddetect(void)
{
	return !(gpio_get(GPIOA, GPIO15));
}

enum sd_status sd_init(void)
{
	/* take care of the pins */
	//enable PullUps on CMD and DAT lines
	/* SDIO_CMD */
	rcc_periph_clock_enable(RCC_GPIOD);
	gpio_set_af(GPIOD, GPIO_AF12, GPIO2);
	gpio_set_output_options(GPIOD, GPIO_OTYPE_PP,
			GPIO_OSPEED_50MHZ, GPIO2);
	gpio_mode_setup(GPIOD, GPIO_MODE_AF,
			GPIO_PUPD_PULLUP, GPIO2);
	/* SDIO_CLK, DAT*/
	rcc_periph_clock_enable(RCC_GPIOC);
	gpio_set_af(GPIOC, GPIO_AF12, GPIO8 | GPIO9 | GPIO10 | GPIO11 | GPIO12);
	gpio_set_output_options(GPIOC, GPIO_OTYPE_PP,
			GPIO_OSPEED_50MHZ, GPIO8 | GPIO9 | GPIO10 | GPIO11 | GPIO12);
	gpio_mode_setup(GPIOC, GPIO_MODE_AF, GPIO_PUPD_PULLUP,
				GPIO8 | GPIO9 | GPIO10 | GPIO11);
	gpio_mode_setup(GPIOC, GPIO_MODE_AF, GPIO_PUPD_PULLDOWN, GPIO12);
	/* take care of sdio configuration */
	rcc_periph_clock_disable(RCC_SDIO);
	rcc_periph_clock_enable(RCC_SDIO);

	//sdio_enable_hw_flow_control(); /* device limitation on stm32f401 do not use */
	sdio_set_clk_div(118);
	sdio_enable_pwrsave();
	sdio_enable_clock();
	sdio_power_on();

	/* identify card, to be able to use it after */
	enum sd_status s;
	while ((s = sd_identify()) == FAILURE) {}
	if (s == BAD_CARD)
		return s;

	sd_enable_wbus();

	/* speed up communication */
	/* sdio_set_clk_div(2); //logic analyzer speed */
	/* sdio_set_clk_div(0); //max speed */
	sdio_enable_clk_bypass(); //max speed
	read_status();
	read_scr();
	return SUCCESS;
}
