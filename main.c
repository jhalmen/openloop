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
#include <math.h>
#include "wm8778.h"
#include "hardware.h"
#include "sine.h"

#define AUDIOBUFFERSIZE (64)

uint32_t tick = 0;
int main(void)
{
	/* void initialise_monitor_handles(void); */
	/* initialise_monitor_handles(); */

	/* dma memory locations for sound in */
	int16_t outstream[AUDIOBUFFERSIZE];
	int16_t instream[AUDIOBUFFERSIZE];

	pll_setup();
	led_setup();
	led_on();
	gpio_setup();
	systick_setup(100);

	i2c_setup();
	get_i2c_stat1();
	get_i2c_stat2();

	/* configure codec */
	uint16_t disable = DAC_C1(0, 0, 0, 0, 0b0000);
	uint16_t enable = DAC_C1(0, 0, 0, 0, 0b1001);

	/* take care of i2s configuration */
	int div, odd, mckoe;
	/* 48?kHz i think */
	/* plli2s_setup(258,3); */
	/* div = 3; */
	/* odd = 1; */
	/* mckoe = 1; */

	/* TODO temporary this sets the interface to only 2*8kHz! */
	plli2s_setup(256,5);
	div = 12;
	odd = 1;
	mckoe = 1;
	/* enable SPI2/I2S2 peripheral clock */
	rcc_periph_clock_enable(RCC_SPI2);
	/* use dma */
	rcc_periph_clock_enable(RCC_DMA1);

	struct dma_channel audioout = {
		.dma = DMA1,
		.stream = DMA_STREAM4,
		.direction = DMA_SxCR_DIR_MEM_TO_PERIPHERAL,
		.channel = DMA_SxCR_CHSEL_0,
		.psize = DMA_SxCR_PSIZE_16BIT,
		.paddress = (uint32_t) &SPI_DR(I2S2),
		.msize = DMA_SxCR_MSIZE_16BIT,
		.maddress = (uint32_t) outstream,
		.doublebuf = 0,
		.minc = 1,
		.circ = 1,
		.pinc = 0,
		.prio = DMA_SxCR_PL_HIGH,
		.numberofdata = AUDIOBUFFERSIZE
	};

	struct dma_channel audioin = {
		.dma = DMA1,
		.stream = DMA_STREAM3,
		.direction = DMA_SxCR_DIR_PERIPHERAL_TO_MEM,
		.channel = DMA_SxCR_CHSEL_3,
		.psize = DMA_SxCR_PSIZE_16BIT,
		.paddress = (uint32_t) &SPI_DR(I2S2ext),
		.msize = DMA_SxCR_MSIZE_16BIT,
		.maddress = (uint32_t) instream,
		.doublebuf = 0,
		.minc = 1,
		.circ = 1,
		.pinc = 0,
		.prio = DMA_SxCR_PL_HIGH,
		.numberofdata = AUDIOBUFFERSIZE
	};

	//play ( buffer of single audio, number of points)
	uint16_t *address = sine_32;
	uint16_t n = AUDIOBUFFERSIZE/2;
	for (int i = 0; i < n; ++i){
		outstream[2*i] = address[i] - 0x8000;
		outstream[2*i+1] = address[i] - 0x8000;
	}

	init_dma_channel(&audioin);
	init_dma_channel(&audioout);

	i2s_init_slave_receive(I2S2ext);
	i2s_init_master_transmit(I2S2, div, odd, mckoe);
	spi_enable_rx_dma(I2S2ext);
	spi_enable_tx_dma(I2S2);
	/* slave has to be enabled before the master! */
	enable_i2s(I2S2ext);
	enable_i2s(I2S2);

	while (1) {
			/* __asm__("wfi"); */
			/* __asm__("wfe"); */
			__asm__("nop");
	}
	return 0;
}

void sys_tick_handler(void)
{
	++tick;
}

