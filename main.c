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
#include "wm8778.h"
#include "swo.h"
#include <stdlib.h>

uint8_t printall = 1;

#define OUTBUFFERSIZE (8192)
#define INBUFFERSIZE (8192)

uint32_t tick = 0;

/* dma memory locations for sound in */
int16_t outstream[OUTBUFFERSIZE];
int16_t instream[INBUFFERSIZE];

uint32_t data[512];

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
	.numberofdata = INBUFFERSIZE
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
	.numberofdata = 3
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

void updatevolumes(void);

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

int main(void)
{
	///////////////////////// INIT STUFF /////////////////////////
	pll_setup();
	systick_setup(1);
	/* enable_swo(115200); */
	enable_swo(230400);
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

	for (int i = 0; i < 512; ++i)
		data[i] = 1;
	if (sddetect()) {
		sdio_periph_setup();
		read_status();
		read_scr();
	}
	while (1) { __asm__("nop"); }

	sound_setup(&f96k);

	///////////////////////// LOOP STUFF /////////////////////////
	volatile enum {
		STANDBY,
		RECORD,
		PLAY,
		OVRDUB
	} state = STANDBY;
	bool statechange = true; // TODO this should be global, set in button interrupt
	while (1) {
	if (statechange) { // statechange!!
		statechange = false;
		switch (state) {
		case STANDBY:
			send_codec_cmd(OMUX(1,1));
			break;
		case RECORD:
			send_codec_cmd(OMUX(1, 0));
			break;
		case PLAY:
			break;
		case OVRDUB:
			break;
		}
	}
	while (!statechange && (state == STANDBY || state == RECORD)) {
		/* donothing for the standby case*/
		/* once DMAs are set up correctly,
		 * enev in record there's nothing to do than to set volumes */
		updatevolumes();
		__asm__("nop");
	}

	while (!statechange && (state == PLAY || state == OVRDUB)) {
		/* dacbuffer = adcbuffer >> 1 + sdbuffer >> 1; */
		updatevolumes(); // from time to time...
		__asm__("nop");
	}
			__asm__("nop");
			/* __asm__("wfi"); */
		updatevolumes();
		dprintf(0, "encoder at position %d\n", encpos());
	}
	return 0;
}

void updatevolumes(void)
{
	static uint16_t oldvol[3] = {0,0,0};
	/* input gains and output volume */
	if ((abs(oldvol[2] - chanvol[2]) > 10) &&
		((chanvol[2] >> 2) != (oldvol[2] >> 2))) {
		send_codec_cmd(MASTDA(chanvol[2] >> 2,1));
		oldvol[2] = chanvol[2];
	}
	/* uncomment this after soldering potis */
	/* if ((abs(oldvol[1] - chanvol[1]) > 10) && */
	/* 	((chanvol[1] >> 2) != (oldvol[1] >> 2))) { */
	/* 	send_codec_cmd(ADCR(chanvol[1] >> 2,1)); */
	/* 	oldvol[1] = chanvol[1]; */
	/* } */
	/* if ((abs(oldvol[0] - chanvol[0]) > 10) && */
	/* 	((chanvol[0] >> 2) != (oldvol[0] >> 2))) { */
	/* 	send_codec_cmd(ADCL(chanvol[0] >> 2,1)); */
	/* 	oldvol[0] = chanvol[0]; */
	/* } */
}

void sys_tick_handler(void)
{
	++tick;
}

void exti15_10_isr(void)
{
	if (exti_get_flag_status(EXTI11)){
		exti_reset_request(EXTI11);
		/* start stomped! */
		dprintf(0, "start stomped!\n");
	}
	if (exti_get_flag_status(EXTI12)){
		exti_reset_request(EXTI12);
		/* stop stomped! */
		/* consider stopping data transfer */
		send_codec_cmd(disable);
		dprintf(0, "stop stomped!\n");
	}
}

void exti2_isr(void)
{
	exti_reset_request(EXTI2);
	/* menu button pressed */
	send_codec_cmd(enable);
	dprintf(0, "menu pressed!\n");

}

