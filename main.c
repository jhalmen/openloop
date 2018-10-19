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
#define dprintf(...)
#include "hardware.h"
#include "wm8778.h"
#include "swo.h"
#include <stdlib.h>

uint32_t tick = 0;

/* dma memory location for volume potentiometers */
uint16_t chanvol[3] = {0,0,0};

struct dma_channel volumes = {
	.rcc = RCC_DMA2,
	.dma = DMA2,
	.stream = DMA_STREAM0,
	.direction = DMA_SxCR_DIR_PERIPHERAL_TO_MEM,
	.channel = DMA_SxCR_CHSEL_0,
	.psize = DMA_SxCR_PSIZE_16BIT,
	.paddress = (uint32_t)&ADC_DR(ADC1),
	.msize = DMA_SxCR_MSIZE_16BIT,
	.maddress = (uint32_t)chanvol,
	.doublebuf = 0,
	.minc = 1,
	.circ = 1,
	.pinc = 0,
	.prio = DMA_SxCR_PL_LOW,
	.pburst = DMA_SxCR_PBURST_SINGLE,
	.periphflwctrl = 0,
	.numberofdata = 3,
	.interrupts = DMA_SxCR_TCIE,
	.nvic = NVIC_DMA2_STREAM0_IRQ
};

struct i2sfreq f96k = {
	.plln = 258,
	.pllr = 3,
	.div = 3,
	.odd = 1
};

struct i2sfreq f16k = {
	.plln = 256,
	.pllr = 5,
	.div = 12,
	.odd = 1
};

enum {
	START = 1,
	STOP = 2,
	MENU = 4
} statechange = 0;

volatile enum {
	STANDBY = 0,
	RECORD = 1,
	PLAY = 2,
	OVRDUB = 3
} state = STANDBY;

volatile struct _loop {
	uint32_t start;
	uint32_t len;
	uint32_t end_idx;
	uint8_t has_undo:1;
	uint8_t has_redo:1;
} loop;

volatile struct _sd {
	uint32_t addr;
	int16_t buffer[512];
	uint16_t idx;
	uint16_t prvidx;
	uint8_t xferlow:1;
	uint8_t xferhigh:1;
} sdin;

int16_t get_sample(void)
{
	return sdin.buffer[sdin.idx++];
}

void buffer_init()
{
	sdin.addr = 4000;
	sdin.prvidx = -1;
	sdin.idx = 0;
	sdin.xferlow = 1;
}

void handle_buffer(void) {
	static int16_t *b = sdin.buffer;
	if (sdin.addr == 6000) {
		state = STANDBY;
		buffer_init();
	}
	if (sdin.prvidx != sdin.idx) {
	if (sdin.idx == 512) sdin.idx = 0;
	if (sdin.idx == 0) {
		sdin.xferhigh = 1;
		b = sdin.buffer + 256;
	}
	if (sdin.idx == 256) {
		sdin.xferlow = 1;
		b = sdin.buffer;
	}
	if ((sdin.xferlow | sdin.xferhigh) && !(DMA_SCR(DMA2, DMA_STREAM3) & DMA_SxCR_EN)
		&& gpio_get(GPIOC, GPIO8)){
		read_single_block(b, sdin.addr++);
		if (sdin.xferlow) {
			sdin.xferlow = 0;
		} else {
			sdin.xferhigh = 0;
		}
		/* if (sdin.idx == -1) {	// initial samples */
		/* 	sdin.idx = 0; */
		/* } */
	}
	sdin.prvidx = sdin.idx;
	}
}


void startaudio(void);

volatile uint32_t *clooook = &SDIO_CLKCR;
int stopabit = 0;
int main(void)
{
	///////////////////////// INIT STUFF /////////////////////////
	pll_setup();
	systick_setup(5);
/////////
//	enable_swo(230400);
////////

	i2c_setup();

	/* configure codec */
	uint16_t disable = DAC_C1(0, 0, 0, 0, 0b0000);
	uint16_t enable = DAC_C1(0, 0, 0, 0, 0b1001);
	uint16_t vol_full = MASTDA(255, 1);
	uint16_t vol_low = MASTDA(222,1);

	dma_channel_init(&volumes);

	adc_setup();
	encoder_setup();
	buttons_setup();
	sddetect_setup();

	send_codec_cmd(enable);

	if (sddetect()) {
		if (sd_init() != SUCCESS)
			while (1) __asm__("wfi");
	}
	while (stopabit) {
		__asm__("nop");
	}

	sound_setup(&f96k);
	startaudio();
	buffer_init();

	///////////////////////// LOOP STUFF /////////////////////////
	while (1) {
		handle_buffer();
		__asm__("nop");
	}
	return 0;
}

void dma2_stream0_isr(void)	// VOLUMES
{
	if (!dma_get_interrupt_flag(DMA2, DMA_STREAM0, DMA_TCIF))
		return;
	dma_clear_interrupt_flags(DMA2, DMA_STREAM0, DMA_TCIF);
	static uint16_t oldvol[3] = {0,0,0};
	/* input gains and output volume */
	if ((chanvol[2] - oldvol[2] > 5) ||
		(oldvol[2] - chanvol[2] > 5)) {
		send_codec_cmd(MASTDA(chanvol[2] >> 2,1));
		oldvol[2] = (chanvol[2] >> 2) << 2;
	}
	/* uncomment this after soldering potis */
	/* TODO check to use PGA here */
	/* if ((chanvol[1] - oldvol[1] > 5) || */
	/* 	(oldvol[1] - chanvol[1] > 5)) { */
	/* 	send_codec_cmd(ADCR(chanvol[1] >> 2,1)); */
	/* 	oldvol[1] = (chanvol[1] >> 2) << 2; */
	/* } */
	/* if ((chanvol[0] - oldvol[0] > 5) || */
	/* 	(oldvol[0] - chanvol[0] > 5)) { */
	/* 	send_codec_cmd(ADCL(chanvol[0] >> 2,1)); */
	/* 	oldvol[0] = (chanvol[0] >> 2) << 2; */
	/* } */
}

void sys_tick_handler(void)
{
	++tick;
	adc_start_conversion_regular(ADC1);
}

void exti15_10_isr(void)
{
	if (exti_get_flag_status(EXTI11)){
		exti_reset_request(EXTI11);
		/* start stomped! */
		dprintf(0, "start stomped!\n");
		statechange = START;
	}
	if (exti_get_flag_status(EXTI12)){
		exti_reset_request(EXTI12);
		/* stop stomped! */
		/* consider stopping data transfer */
		/* send_codec_cmd(disable); */
		dprintf(0, "stop stomped!\n");
		statechange = STOP;
	}
}
uint8_t enableplayback = 0;

void exti2_isr(void)
{
	exti_reset_request(EXTI2);
	/* menu button pressed */
	/* send_codec_cmd(enable); */
	dprintf(0, "menu pressed!\n");
	/* statechange = MENU; */
	if (state & PLAY) {
		state &= 2;
	} else {
		enableplayback = 1;
	}
}

void put_sample(int16_t sample)
{
	static int16_t buffer[512];
	static int16_t *b;
	static int16_t idx = 0;
	static uint8_t dowrite = 0;
	if (idx == 512) idx = 0;

	buffer[idx++] = sample;
	if (!(idx % 256)) {
		/* write buffer out */
		b = buffer + 256*((idx-1)/256);
		dowrite = 1;
	}
	if (dowrite && !(DMA_SCR(DMA2, DMA_STREAM3) & DMA_SxCR_EN)
			&& gpio_get(GPIOC,GPIO8)) {
		write_single_block(b, 0);// sdaddress++);
		dowrite = 0;
	}
}

void startaudio(void)
{
	spi_enable_rx_buffer_not_empty_interrupt(I2S2ext);
	spi_enable_tx_buffer_empty_interrupt(I2S2);
}

volatile uint32_t *spi = &SPI_SR(I2S2);
volatile uint32_t *ext = &SPI_SR(I2S2ext);

void spi2_isr(void)
{
	static int16_t ldata = 0, rdata = 0;
	// tx
	uint32_t sr = SPI_SR(I2S2);
	if (sr & (SPI_SR_UDR | SPI_SR_OVR))
		while (sr) {__asm__("nop");}	// i2s error! should never happen!
	if (sr & SPI_SR_TXE) {			// i2s needs data
		if (sr & SPI_SR_CHSIDE) {
			SPI_DR(I2S2) = rdata;
		} else {
			SPI_DR(I2S2) = ldata;
		}
	}
	// rx
	sr = SPI_SR(I2S2ext);
	if (sr & (SPI_SR_UDR | SPI_SR_OVR))
		while (sr) {__asm__("nop");}	// i2s error! should never happen!
	if (sr & SPI_SR_RXNE) {			// i2s has data
		int16_t audio = SPI_DR(I2S2ext);
		if (enableplayback && !(sr & SPI_SR_CHSIDE)) {
			state |= PLAY;
			enableplayback = 0;
		}
		if (state & PLAY)
			/* audio = (audio >> 1) + (get_sample() >> 1); */
			audio = get_sample();
		if (state & RECORD)
			put_sample(audio);
		if (sr & SPI_SR_CHSIDE) {
			rdata = audio;
		} else {
			ldata = audio;
		}
	}
}
