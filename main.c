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

/* function prototypes */
int16_t get_sample(void);
void put_sample(int16_t s);
uint8_t card_busy(void);
uint8_t sd_dma_done(void);
void handle_play(void);
void handle_record(void);
void end_record(void);
void startaudio(void);

/* globals */
volatile uint32_t tick = 0;		// system tick
const uint8_t norepeat = 4;		// software debounce. [in ticks]
volatile uint16_t chanvol[3] = {0,0,0};	// adc volume values. updated by dma
volatile uint8_t waaaa;			// error accumulator

struct dma_channel volumes = {
	.rcc = RCC_DMA2,
	.dma = DMA2,
	.stream = DMA_STREAM4,
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
	.nvic = NVIC_DMA2_STREAM4_IRQ
};

struct i2sfreq f96k = {
	.plln = 258,
	.pllr = 3,
	.div = 3,
	.odd = 1
};

struct i2sfreq f44k1 = {
	.plln = 429,
	.pllr = 4,
	.div = 9,
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

uint8_t enablerecord;
uint8_t disablerecord;
uint8_t enableplayback;
uint8_t disableplayback;

struct {
	uint32_t start;
	uint32_t len;
	uint32_t end_idx;
	uint8_t has_undo:1;
	uint8_t has_redo:1;
} loop = { 2000, 0, 255, 0, 0};

volatile struct _sd {
	uint32_t addr;
	int16_t buffer[512];
	int16_t *xfer;
	int16_t idx;
	int16_t prvidx;
} sdin, sdout;


int16_t get_sample(void)
{
	if (sdin.idx % 512 != sdin.idx) {waaaa |= 1;}
	return sdin.buffer[sdin.idx++];
	/* return *gs++; */
}

void put_sample(int16_t s)
{
	if (sdout.idx % 512 != sdout.idx) {waaaa |= 2;}
	sdout.buffer[sdout.idx++] = s;
	/* *ps++ = s; */
}

void init_play(void)
{
	sdin.addr = loop.start;
	sdin.prvidx = -1;
	sdin.idx = 0;
}

void init_record(void)
{
	sdout.addr = loop.start;
	sdout.prvidx = 0;
	sdout.idx = 0;
}

uint8_t card_busy(void) {return !gpio_get(GPIOC, GPIO8);}

void handle_play(void)
{
	/* static buffer */
	/* gs = buffer; */
	/* TODO make sure that we play to the end idx at the end addr!! */
	if (loop.len && sdin.addr == loop.start + loop.len) {
		if (sdin.idx % 256 == loop.end_idx) {
			init_play();
		}
	}
	if (sdin.prvidx != sdin.idx) {
		sdin.idx %= 512;
		sdin.prvidx = sdin.idx;
		if (sdin.idx == 0) { sdin.xfer = sdin.buffer + 256; }
		if (sdin.idx == 256) { sdin.xfer = sdin.buffer; }
		if (sdin.prvidx == -1) sdin.xfer = sdin.buffer; //TODO this should actually
							// only be handled after the
							// first data was written
	}
	if (sdin.xfer && !(DMA_SCR(DMA2, DMA_STREAM3) & DMA_SxCR_EN)
		&& !card_busy()){
		read_single_block((uint32_t *)sdin.xfer, sdin.addr++);
		sdin.xfer = NULL;
	}
}

void handle_record(void)
{
	if (loop.len && sdout.addr == loop.start + loop.len) {
		// can only get here if already overdubbing
		if (sdout.idx % 256 == loop.end_idx) {
			init_record();
		}
	}
	if (sdout.idx != sdout.prvidx) {
		// once after every buffer write
		sdout.idx %= 512;
		sdout.prvidx = sdout.idx;
		if (sdout.idx == 0) {
			if (sdout.xfer) waaaa |= 16;
			sdout.xfer = sdout.buffer + 256;
		}
		if (sdout.idx == 256) {
			if (sdout.xfer) waaaa |= 16;
			sdout.xfer = sdout.buffer;
		}
	}
	if (sdout.xfer && !(DMA_SCR(DMA2, DMA_STREAM3) & DMA_SxCR_EN)
			&& !card_busy()) {
		write_single_block((uint32_t *)sdout.xfer, sdout.addr++);
		sdout.xfer = NULL;
	}
}

void end_record(void)
{
	if (!loop.len) {
		loop.end_idx = sdout.idx % 256;
		loop.len = sdout.addr - loop.start + 1;
	}
	init_record();
}

void startaudio(void);

int main(void)
{
	///////////////////////// INIT STUFF /////////////////////////
	pll_setup();

	//enable_swo(230400);

	/* TODO increase i2c frequency to 400kHz and test */
	i2c_setup();

	encoder_setup();
	buttons_setup();
	sddetect_setup();

	send_codec_cmd(RESET());
	send_codec_cmd(DAC_C1(1,0,0,1, 0b1001));
	dma_channel_init(&volumes);
	adc_setup();

	if (sddetect()) {
		if (sd_init() != SUCCESS)
			while (1) __asm__("wfi");
	}

	sound_setup(&f16k);
	systick_setup(5);
	init_play();
	init_record();

	startaudio();
	///////////////////////// LOOP STUFF /////////////////////////
	while (1) {
		handle_record();
		handle_play();
		__asm__("nop");
	}
	return 0;
}


void startaudio(void)
{
	spi_enable_rx_buffer_not_empty_interrupt(I2S2ext);
	spi_enable_tx_buffer_empty_interrupt(I2S2);
}

void sys_tick_handler(void)
{
	++tick;
	// update volumes
	adc_start_conversion_regular(ADC1);
}

void dma2_stream4_isr(void)	// VOLUMES
{
	if (!dma_get_interrupt_flag(volumes.dma, volumes.stream, DMA_TCIF))
		return;
	dma_clear_interrupt_flags(volumes.dma, volumes.stream, DMA_TCIF);
	static uint16_t oldvol[3] = {0xffff, 0xffff, 0xffff};
	/* input gains and output volume */
	if ((chanvol[2] - oldvol[2] > 5) ||
		(oldvol[2] - chanvol[2] > 5)) {
		send_codec_cmd(MASTDA(chanvol[2] >> 2,1));
		oldvol[2] = (chanvol[2] >> 2) << 2;
	}
	/* TODO check to use PGA here */
	if ((chanvol[1] - oldvol[1] > 5) ||
		(oldvol[1] - chanvol[1] > 5)) {
		send_codec_cmd(ADCR(chanvol[1] >> 2,1));
		oldvol[1] = (chanvol[1] >> 2) << 2;
	}
	if ((chanvol[0] - oldvol[0] > 5) ||
		(oldvol[0] - chanvol[0] > 5)) {
		send_codec_cmd(ADCL(chanvol[0] >> 2,1));
		oldvol[0] = (chanvol[0] >> 2) << 2;
	}
}

void exti15_10_isr(void)	// START & STOP BUTTONS
{
if (exti_get_flag_status(EXTI11)){
	exti_reset_request(EXTI11);
	static uint32_t laststart = 0;
	if (tick >= laststart + norepeat) {
		laststart = tick;
	/* start stomped! */
	dprintf(0, "start stomped!\n");
	/* TODO */
	}
}
if (exti_get_flag_status(EXTI12)){
	exti_reset_request(EXTI12);
	static uint32_t laststop = 0;
	if (tick >= laststop + norepeat) {
		laststop = tick;
	/* stop stomped! */
	dprintf(0, "stop stomped!\n");
	/* TODO */
	if (state & RECORD) {
		disablerecord = 1;
	} else {
		enablerecord = 1;
	}
	}
}
}

void exti2_isr(void)		// MENU BUTTON
{
	exti_reset_request(EXTI2);
	static uint32_t lastmenu = 0;
	if (tick >= lastmenu + norepeat) {
		lastmenu = tick;
		/* menu button pressed */
		/* send_codec_cmd(enable); */
		dprintf(0, "menu pressed!\n");
		/* statechange = MENU; */
		if (state & PLAY) {
			disableplayback = 1;
		} else {
			enableplayback = 1;
		}
	}
}

void spi2_isr(void)		// I2S data interrupt
{
	static int16_t ldata = 0, rdata = 0;
	// TX
	uint32_t sr = SPI_SR(I2S2);
	if (sr & (SPI_SR_UDR | SPI_SR_OVR))
		waaaa |= 8;	// i2s error! should never happen!
	if (sr & SPI_SR_TXE) {			// I2S DATA TX
		if (sr & SPI_SR_CHSIDE) {
			SPI_DR(I2S2) = rdata;
		} else {
			SPI_DR(I2S2) = ldata;
		}
	}
	// RX
	sr = SPI_SR(I2S2ext);
	if (sr & (SPI_SR_UDR | SPI_SR_OVR))
		waaaa |= 8;	// i2s error! should never happen!
	if (sr & SPI_SR_RXNE) {			// I2S has data
		int16_t audio = SPI_DR(I2S2ext);
		if (state & PLAY) {
			if (sr & SPI_SR_CHSIDE && !(sdin.idx % 2)) {
				/* shouldn't happen */
				waaaa |= 4;
			}
			int32_t temp = audio + get_sample();
			__asm__("ssat %[dst], #16, %[src]" //SIGNED SATURATE
					: [dst] "=r" (audio)
					: [src] "r" (temp));
		}
		if (state & RECORD)
			put_sample(audio);
		if (sr & SPI_SR_CHSIDE) {
			rdata = audio;
			// next time will be left channel -> handle state changes
			if (enablerecord) {
				state |= RECORD;
				enablerecord = 0;
			} else if (disablerecord) {
				state &= ~RECORD;
				disablerecord = 0;
				end_record();
			}
			if (enableplayback) {
				state |= PLAY;
				enableplayback = 0;
			} else if (disableplayback) {
				state &= ~PLAY;
				disableplayback = 0;
				init_play();
			}
		} else {
			ldata = audio;
		}
	}
}
