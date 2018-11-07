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
void handle_sd(void);
void end_record(void);
void startaudio(void);
void leds_update(void);
void loop_reset(void);

/* globals */
volatile uint32_t tick = 0;		// system tick
const uint8_t tickhz = 5;		// frequency of system tick
const uint8_t norepeat = 4;		// software debounce. [in ticks]
const uint8_t resetloop = 10;		// ticks to hold for reset (+norepeat)
uint8_t heartbeat = 10;		// heartbeat delay [in ticks]
volatile uint16_t chanvol[3] = {0,0,0};	// adc volume values. updated by dma
volatile uint8_t waaaa[8];			// error accumulator

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

struct i2sfreq f48k = {
	// relative error: 0.0198%
	.plln = 215,
	.pllr = 5,
	.div = 3,
	.odd = 1
};

struct i2sfreq f44k = {
	// relative error: 0.1223%
	.plln = 203,
	.pllr = 4,
	.div = 4,
	.odd = 1
};

struct i2sfreq f32k = {
	// relative error: 0.0976%
	.plln = 205,
	.pllr = 5,
	.div = 5,
	.odd = 0
};

struct i2sfreq f22k = {
	// 22k050
	// relative error: 0.1047%
	.plln = 203,
	.pllr = 4,
	.div = 9,
	.odd = 0
};

struct i2sfreq f16k = {
	// relative error: 0.00375%
	.plln = 213,
	.pllr = 4,
	.div = 13,
	.odd = 0
};

enum {
	START = 1,
	STOP = 2,
	MENU = 4
} action = 0;

volatile enum s{
	STANDBY = 0,
	RECORD = 1,
	PLAY = 2,
	OVRDUB = 3
} state = STANDBY;

const enum s trans[][5] = {{0,	PLAY,	STANDBY,	0,	STANDBY},
			{0,	PLAY,	STANDBY,	0,	RECORD},
			{0,	OVRDUB,	STANDBY,	0,	PLAY},
			{0,	PLAY,	STANDBY,	0,	OVRDUB}};

struct {
	uint32_t start;
	uint32_t len;
	int16_t end_idx;
} loop = {10000, 0, 0};

struct {
	/* buffers */
	int16_t in[512];	// from card to codec
	int16_t out[512];	// from codec to card
	/* addresses */
	uint16_t idx;		// in buffers
	uint32_t addr;		// in card
	/* pointers */
	int16_t *txfer;		// from pointer to card
	int16_t *rxfer;		// from card to pointer
	/* flags */
	uint8_t r:1;		// sample read from in buffer
	uint8_t w:1;		// sample written to out buffer
	uint8_t rx:1;
	uint8_t tx:1;
} sd = {
	.addr = 10000
};

int main(void)
{
	///////////////////////// INIT STUFF /////////////////////////
	pll_setup();

	//enable_swo(230400);

	/* TODO increase i2c frequency to 400kHz and test */
	i2c_setup();

	encoder_setup();
	buttons_setup();
	leds_setup();
	sddetect_setup();

	send_codec_cmd(RESET());
	/* for testing purposes */
	/* send_codec_cmd(DAC_C1(1,0,0,1, 0b1001)); */
	/* send_codec_cmd(OMUX(0,1)); */
	send_codec_cmd(DAC_C1(1,0,0,1, 0b1001));
	dma_channel_init(&volumes);
	nvic_set_priority(NVIC_DMA2_STREAM4_IRQ, 0);
	adc_setup();

	if (sddetect()) {
		if (sd_init() != SUCCESS)
			while (1) __asm__("wfi");
	}

	sound_setup(&f32k);
	systick_setup(tickhz);

	startaudio();
	///////////////////////// LOOP STUFF /////////////////////////
	while (1) {
		handle_sd();
		leds_update();
		__asm__("nop");
	}
	return 0;
}

void leds_update(void)
{
	if (state & PLAY) {
		gpio_set(GPIOB, GPIO3); // green
	} else {
		gpio_clear(GPIOB, GPIO3);
	}
	if (state & RECORD) {
		gpio_set(GPIOB, GPIO10); // red
	} else {
		gpio_clear(GPIOB, GPIO10);
	}
	static uint32_t lasttick = 0;
	if (tick > lasttick + heartbeat) {
		lasttick += heartbeat;
		gpio_toggle(GPIOA, GPIO10);
	}
}

int16_t get_sample(void)
{
	static int over = 0;
	static unsigned int atidx[512];
	static unsigned int ataddr[512];
	if (sd.r) {
		waaaa[0]++;
		atidx[over] = sd.idx;
		ataddr[over++] = sd.addr;
		if (over >= 512)
			over = 511;
	}
	sd.r = 1;
	return sd.in[sd.idx];
}

void put_sample(int16_t s)
{
	static int overwritten = 0;
	static unsigned int atidx[512];
	sd.out[sd.idx] = s;
	if (sd.w) {
		waaaa[1]++;
		atidx[overwritten++] = sd.idx;
		if (overwritten >= 512)
			overwritten = 511;
	}
	sd.w = 1;
}

uint8_t sd_dma_done(void)
{
	return !(DMA_SCR(DMA2, DMA_STREAM3) & DMA_SxCR_EN)
		|| SDIO_STA & SDIO_STA_DATAEND;
}

uint8_t card_busy(void)
{
	return !gpio_get(GPIOC, GPIO8);
}

void handle_sd(void)
{
	if (sd.r || sd.w) {
	/* if (state && state == ((uint8_t)(sd.r << 1) | sd.w)) { */
		sd.r = 0;
		sd.w = 0;
		sd.idx %= 512;
		if (sd.idx == 0) {
			if (state & PLAY) {
				if (sd.rxfer)
					waaaa[2]++;
				sd.rxfer = sd.in + 256;
			}
			if (state & RECORD) {
				if (sd.txfer)
					waaaa[3]++;
				sd.txfer = sd.out + 256;
			}
			/* sd.txfer = (int16_t*)((int32_t)(sd.out + 256) * !!(state & RECORD)); */
		}
		if (sd.idx == 256) {
			if (state & PLAY) {
				if (sd.rxfer)
					waaaa[2]++;
				sd.rxfer = sd.in;
			}
			/* sd.txfer = (int16_t*)((int32_t)(sd.out) * !!(state & RECORD)); */
			if (state & RECORD) {
				if (sd.txfer)
					waaaa[3]++;
				sd.txfer = sd.out;
		/* handle copying data to inbuffer on first recording */
				/* if (sd.addr == loop.start) { */
				/* 	for (int i = 0; i < 256; ++i) */
				/* 		sd.in[i] = sd.out[i]; */
				/* } */
			}
		}
	}
	if (sd.rxfer && sd_dma_done() && !card_busy()) {
		read_single_block((uint32_t*)sd.rxfer, sd.addr);
		sd.rxfer = NULL;
		sd.rx = 1;
	}
	if (sd.txfer && sd_dma_done() && !card_busy()) {
		write_single_block((uint32_t*)sd.txfer, sd.addr);
		sd.txfer = NULL;
		sd.tx = 1;
	}
	if (state && state == ((uint8_t)(sd.rx << 1) | sd.tx)) {
		/* TODO this block sometimes... */
		sd.tx = 0;
		sd.rx = 0;
		sd.addr++;
	}
	if (loop.len && sd.addr == loop.start + loop.len){
		if (sd.idx % 256 >= loop.end_idx) {
			sd.addr = loop.start;
			sd.idx = 0;
			/* TODO probably send last tx buffer */
		}
	}
}

void startaudio(void)
{
	spi_enable_rx_buffer_not_empty_interrupt(I2S2ext);
	spi_enable_tx_buffer_empty_interrupt(I2S2);
}

void loop_reset(void)
{
	loop.len = 0;
	loop.end_idx = 0;
	heartbeat = 10;
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
		action |= START;
	}
}
if (exti_get_flag_status(EXTI12)){
	static uint32_t laststop = 0;
	if (gpio_get(GPIOA, GPIO12)){	// rising
	if (tick >= laststop + norepeat + resetloop) {
		loop_reset();
	}
	} else {				// falling
	if (tick >= laststop + norepeat) {
		laststop = tick;
		/* stop stomped! */
		dprintf(0, "stop stomped!\n");
		action |= STOP;
	}
	}
	exti_reset_request(EXTI12);
}
}

void exti2_isr(void)		// MENU BUTTON
{
	exti_reset_request(EXTI2);
	static uint32_t lastmenu = 0;
	if (tick >= lastmenu + norepeat) {
		lastmenu = tick;
		/* menu button pressed */
		dprintf(0, "menu pressed!\n");
		action |= MENU;
	}
}

void spi2_isr(void)		// I2S data interrupt
{
	static int16_t ldata = 0, rdata = 0, data = 0;
	// TX
	uint32_t sr = SPI_SR(I2S2);
	if (sr & (SPI_SR_UDR | SPI_SR_OVR))
		waaaa[4]++;	// i2s error! should never happen!
	if (sr & SPI_SR_TXE) {			// I2S DATA TX
		SPI_DR(I2S2) = data;
		/* if (sr & SPI_SR_CHSIDE) { */
		/* 	SPI_DR(I2S2) = rdata; */
		/* } else { */
		/* 	SPI_DR(I2S2) = ldata; */
		/* } */
	}
	// RX
	sr = SPI_SR(I2S2ext);
	if (sr & (SPI_SR_UDR | SPI_SR_OVR))
		waaaa[4]++;	// i2s error! should never happen!
	if (sr & SPI_SR_RXNE) {			// I2S has data
		int16_t audio = SPI_DR(I2S2ext);
		if (sr & SPI_SR_CHSIDE) {
			int32_t temp = audio + ldata;
			__asm__("ssat %[dst], #16, %[src]"
					:[dst] "=r" (audio)
					:[src] "r" (temp));
			if (state & PLAY) {
				temp = audio + get_sample();
				__asm__("ssat %[dst], #16, %[src]" //SIGNED SATURATE
						: [dst] "=r" (audio)
						: [src] "r" (temp));
			}
			if (state & RECORD)
				put_sample(audio);
			if (state)
				sd.idx++;
			data = audio;
			enum s nextstate = 0;
			if (action && (nextstate = trans[state][action]) != state) {
				action = 0;
				// initial recording ends here
				if (!loop.len && state & RECORD) {
					loop.end_idx = sd.idx % 256;
				/* TODO addr should be set in handle_sd */
				/* 	when it's sure we handled the */
				/* 	last block */
					loop.len = sd.addr  - loop.start;
					heartbeat = 2;
				}
				if (nextstate == STANDBY) {
					sd.addr = loop.start;
					sd.idx = 0;
				}
				if (state & RECORD && !(nextstate & RECORD)) {
					// TODO this is a hack to get around
					// loop sometimes blocking upon ending
					// recording
					sd.txfer = 0;
					sd.tx = 0;
				}
				state = nextstate;
				if (state == PLAY && !loop.len)
					state = RECORD;
			} else {
				// action doesn't change state
				// remove it either way
				action = 0;
			}
		} else {
			ldata = audio;
		}
	}
}
