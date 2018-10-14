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


#define OUTBUFFERSIZE	(512)
#define INBUFFERSIZE	(512)

uint32_t tick = 0;

/* dma memory locations for sound in */
int16_t outstream[OUTBUFFERSIZE];
int16_t instream[INBUFFERSIZE];

int16_t *insoutb;
uint8_t fastenough = 1;

int16_t outblock[256];
int16_t inblock[256];

/* dma memory location for volume potentiometers */
uint16_t chanvol[3] = {0,0,0};

struct dma_channel audioout = {
	.rcc = RCC_DMA1,
	.dma = DMA1,
	.stream = DMA_STREAM4,
	.direction = DMA_SxCR_DIR_MEM_TO_PERIPHERAL,
	.channel = DMA_SxCR_CHSEL_0,
	.psize = DMA_SxCR_PSIZE_16BIT,
	.paddress = (uint32_t)&SPI_DR(I2S2),
	.msize = DMA_SxCR_MSIZE_16BIT,
	.maddress = (uint32_t)outstream,
	.doublebuf = 0,
	.minc = 1,
	.circ = 1,
	.pinc = 0,
	.prio = DMA_SxCR_PL_HIGH,
	.pburst = DMA_SxCR_PBURST_SINGLE,
	.periphflwctrl = 0,
	.numberofdata = OUTBUFFERSIZE
};

struct dma_channel audioin = {
	.rcc = RCC_DMA1,
	.dma = DMA1,
	.stream = DMA_STREAM3,
	.direction = DMA_SxCR_DIR_PERIPHERAL_TO_MEM,
	.channel = DMA_SxCR_CHSEL_3,
	.psize = DMA_SxCR_PSIZE_16BIT,
	.paddress = (uint32_t)&SPI_DR(I2S2ext),
	.msize = DMA_SxCR_MSIZE_16BIT,
	.maddress = (uint32_t)instream,
	.doublebuf = 0,
	.minc = 1,
	.circ = 1,
	.pinc = 0,
	.prio = DMA_SxCR_PL_HIGH,
	.periphflwctrl = 0,
	.pburst = DMA_SxCR_PBURST_SINGLE,
	/* TODO consider bursting memory sides of audio DMAs */
	/* .mburst = DMA_SxCR_MBURST_ */
	.numberofdata = INBUFFERSIZE,
	.interrupts = DMA_SxCR_TCIE | DMA_SxCR_HTIE,
	.nvic =  NVIC_DMA1_STREAM3_IRQ
};

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

uint16_t disable;
uint16_t enable;
uint16_t vol_full;
uint16_t vol_low;


void copybufferstep(void);
void copybufferstepblind(void);
void copybufferblind(int16_t *dst, int16_t *src);

void printfifocnt(void);
void printfifo(void);
void printdcount(void);
void printfifocnt(void)
{
	printf("fifocount: %ld\n", SDIO_FIFOCNT);
}
void printfifo(void)
{
	printf("fifo: ");
	while (SDIO_STA & SDIO_STA_RXDAVL) {
		printf("gotit");
		printf("%08lx ", SDIO_FIFO);
	}
	printf("\n");
}
void printdcount(void)
{
	if ((SDIO_STA & SDIO_STA_RXACT) ||
	   (SDIO_STA & SDIO_STA_TXACT)) {
		printf("dcount: n/a\n");
	} else {
		printf("dcount: %ld\n", SDIO_DCOUNT);
	}
}

enum {
	START = 1,
	STOP = 2,
	MENU = 4
} statechange = 0;
volatile enum {
	STANDBY,
	RECORD,
	PLAY,
	OVRDUB
} state = STANDBY;

volatile uint32_t *clooook = &SDIO_CLKCR;
int stopabit = 0;
int main(void)
{
	///////////////////////// INIT STUFF /////////////////////////
	pll_setup();
	systick_setup(5);
	/* enable_swo(115200); */

/////////
//	enable_swo(230400);
////////
	/* enable_swo(2250000); */

	i2c_setup();

	/* configure codec */
	disable = DAC_C1(0, 0, 0, 0, 0b0000);
	enable = DAC_C1(0, 0, 0, 0, 0b1001);
	vol_full = MASTDA(255, 1);
	vol_low = MASTDA(222,1);

	dma_channel_init(&audioin);
	dma_channel_init(&audioout);

	dma_channel_init(&volumes);

	adc_setup();
	encoder_setup();
	buttons_setup();
	sddetect_setup();

	send_codec_cmd(enable);

	if (sddetect()) {
		sdio_periph_setup();
		read_status();
		read_scr();
	}
	while (stopabit) {
		__asm__("nop");
	}

	sound_setup(&f96k);

	///////////////////////// LOOP STUFF /////////////////////////
	while (1) {
	dprintf(0, "state STANDBY\n");
	while (state == STANDBY) {
		switch (statechange) {
		case START:
			/* if previous recording available */
			state = PLAY;
			/* else */
			state = RECORD;
			/* until implemented */
			state = STANDBY;
			statechange = 0;
			break;
		case STOP:
			state = PLAY;
			break;
		case MENU:
			state = RECORD;
			/* statechange = 0; */
			break;
		}
		if (statechange) {
			statechange = 0;
			break;
		}
		/* donothing for the standby case */
		copybufferstep();
		__asm__("nop");
	}
	dprintf(0, "state RECORD\n");
	while (state == RECORD) {
		switch (statechange) {
		case START:
			state = PLAY;
			/* or maybe */
			state = OVRDUB;
			/* until implemented */
			state = RECORD;
			statechange = 0;
			break;
		case STOP:
			/* statechange = 0; */
			break;
		case MENU:
			statechange = 0;
			break;
		}
		if (statechange) {
			statechange = 0;
			break;
		}
		/* ///////////// */
#define STARTADDRESS	4000
#define ENDADDRESS	6000
		static int sdaddress = STARTADDRESS;
		static int16_t *lastp;
		static int writing = 1;
		static int lpcounter = 0;
		static int bfcounter = 0;
		if (sdaddress == ENDADDRESS) {
			state = STANDBY;
			break;
		}
		if (		/* buffer ready */
			lastp != insoutb
			&&
			!fastenough
			&&
				/*sd_dma ready*/
			!(DMA_SCR(DMA2, DMA_STREAM3) & DMA_SxCR_EN)
		   ) {
			copybufferblind(outblock, insoutb);
			fastenough = 1;
			lastp = insoutb;
			bfcounter++;
			writing = 0;
		}
		if (	fastenough
			&&
				/* SD card done programming */
			gpio_get(GPIOC, GPIO8)
			&&
			!writing
		   ) {
			write_single_block(outblock, sdaddress++);
			writing = 1;
		}
		lpcounter++;
		/* ///////////// */
		copybufferstepblind();
		__asm__("nop");
	}
	dprintf(0, "state PLAY\n");
	while (state == PLAY) {
		switch (statechange) {
		case START:
			statechange = 0;
			break;
		case STOP:
			statechange = 0;
			break;
		case MENU:
			state=STANDBY;
			break;
		}
		if (statechange) {
			statechange = 0;
			break;
		}
		/* /1* ///////////// *1/ */
/* #define STARTADDRESS	6000 */
/* #define ENDADDRESS	7000 */
		/* static int sdaddress = STARTADDRESS; */
		/* static int16_t *lastp; */
		/* static int writing = 1; */
		/* static int lpcounter = 0; */
		/* static int bfcounter = 0; */
		/* if (sdaddress == ENDADDRESS) { */
		/* 	state = STANDBY; */
		/* 	break; */
		/* } */
		/* if ( */
		/* 		/1* buffer ready *1/ */
		/* 	lastp != insoutb */
		/* 	&& */
		/* 	!fastenough */
		/* 	&& */
		/* 		/1* sd_dma ready *1/ */
		/* 	!(DMA_SCR(DMA2, DMA_STREAM3) & DMA_SxCR_EN) */
		/*    ) { */
		/* 	copybufferblind(outblock, insoutb); */
		/* 	fastenough = 1; */
		/* 	lastp = insoutb; */
		/* 	bfcounter++; */
		/* 	writing = 0; */
		/* } */
		/* if (	fastenough */
		/* 	&& */
		/* 		/1* SD card done programming *1/ */
		/* 	gpio_get(GPIOC, GPIO8) */
		/* 	&& */
		/* 	!writing */
		/*    ) { */
		/* 	write_single_block(outblock, sdaddress++); */
		/* 	writing = 1; */
		/* } */
		/* lpcounter++; */
		/* /1* ///////////// *1/ */

		copybufferstepblind();
		__asm__("nop");
	}
	dprintf(0, "state OVRDUB\n");
	while (state == OVRDUB) {
		/* dacbuffer = adcbuffer >> 1 + sdbuffer >> 1; */
		__asm__("nop");
	}
	}
	return 0;
}

struct copystruct{
	int16_t *source;
	uint32_t slen;
	uint32_t s_at;
	int16_t *dest;
	uint32_t dlen;
	uint32_t d_at;
	int32_t ncopied;
	int32_t readylast;
};

struct {
	uint32_t earlyout;
	uint32_t through;
	uint32_t amount;
} d = {0,0, 0};

void copybufferstep(void)
{
	static struct copystruct cs = {
		.source = instream,
		.slen = INBUFFERSIZE,
		.s_at = 0,
		.dest = outstream,
		.dlen = OUTBUFFERSIZE,
		.d_at = 0,
		.ncopied = 0,
		.readylast = 0
	};

	int32_t ready = cs.slen - DMA_SNDTR(audioin.dma, audioin.stream);
	if (ready < cs.readylast) {
		/* dprintf(2, "ready wrapped around\n"); */
		cs.ncopied -= cs.slen;
	}
	cs.readylast = ready;
	/* dprintf(2, "d=%d, r=%d  ", cs.ncopied, ready); */
	if (cs.ncopied >= ready) {
		d.earlyout++;
		/* dprintf(2, "ncopied early\n"); */
		return;
	}
	d.through++;
	d.amount = ready - cs.ncopied;
	/* dprintf(2, "copying %d nod\n", ready-cs.ncopied); */
	for (; cs.ncopied <= ready; ++cs.ncopied) {
		/* TODO check for off by one */
		cs.dest[cs.d_at++] = cs.source[cs.s_at++];
		if (cs.d_at == cs.dlen) cs.d_at = 0;
		if (cs.s_at == cs.slen) cs.s_at = 0;
	}
}

void copybufferstepblind(void)
{
	/* TODO check this is fast enough using e.g. oscilloscope */
	static int inp = 0, outp = 0;
	if (inp == INBUFFERSIZE) inp = 0;
	if (outp == OUTBUFFERSIZE) outp = 0;
	outstream[outp++] = instream[inp++];
	outstream[outp++] = instream[inp++];
}

void copybufferblind(int16_t *dst, int16_t *src)
{
	for (int i = 0; i < 256; ++i)
		dst[i] = src[i];
}

void dma2_stream0_isr(void)
{
	if (!dma_get_interrupt_flag(DMA2, DMA_STREAM0, DMA_TCIF))
		return;
	dma_clear_interrupt_flags(DMA2, DMA_STREAM0, DMA_TCIF);
	static uint16_t oldvol[3] = {0,0,0};
	/* input gains and output volume */
	if (chanvol[2] >> 2 != oldvol[2] >> 2) {
		send_codec_cmd(MASTDA(chanvol[2] >> 2,1));
		oldvol[2] = chanvol[2];
	}
	/* uncomment this after soldering potis */
	/* TODO check to use PGA here */
	/* if (chanvol[1] >> 2 != oldvol[1] >> 2) { */
	/* 	send_codec_cmd(ADCR(chanvol[1] >> 2,1)); */
	/* 	oldvol[1] = chanvol[1]; */
	/* } */
	/* if (chanvol[0] >> 2 != oldvol[0] >> 2) { */
	/* 	send_codec_cmd(ADCL(chanvol[0] >> 2,1)); */
	/* 	oldvol[0] = chanvol[0]; */
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

void exti2_isr(void)
{
	exti_reset_request(EXTI2);
	/* menu button pressed */
	/* send_codec_cmd(enable); */
	dprintf(0, "menu pressed!\n");
	statechange = MENU;
}

void dma1_stream3_isr(void)
{
	if (dma_get_interrupt_flag(DMA1, DMA_STREAM3, DMA_HTIF)) {
		dma_clear_interrupt_flags(DMA1, DMA_STREAM3, DMA_HTIF);
		/* audioin transfer half complete */
		insoutb = instream;
		if (state == RECORD && !fastenough) {
			while(1)
				__asm__("nop");
		} else {
			fastenough = 0;
		}
	}
	if (dma_get_interrupt_flag(DMA1, DMA_STREAM3, DMA_TCIF)) {
		dma_clear_interrupt_flags(DMA1, DMA_STREAM3, DMA_TCIF);
		/* audioin transfer complete */
		insoutb = instream + 256;
		if (state == RECORD && !fastenough) {
			while (1)
				__asm__("nop");
		} else {
			fastenough = 0;
		}
	}
}
