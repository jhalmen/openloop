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
#include "swo.h"
#include <stdio.h>


#define OUTBUFFERSIZE (512)
#define INBUFFERSIZE (32)

uint32_t tick = 0;

/* dma memory locations for sound in */
int16_t outstream[OUTBUFFERSIZE];
int16_t instream[INBUFFERSIZE];

/* dma memory location for volume potentiometers */
uint16_t chanvol[3] = {0,0,0};

struct dma_channel audioout = {
	.rcc = RCC_DMA1,
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
	.numberofdata = OUTBUFFERSIZE
};

struct dma_channel audioin = {
	.rcc = RCC_DMA1,
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
	.numberofdata = INBUFFERSIZE
};

struct dma_channel volumes = {
	.rcc = RCC_DMA2,
	.dma = DMA2,
	.stream = DMA_STREAM0,
	.direction = DMA_SxCR_DIR_PERIPHERAL_TO_MEM,
	.channel = DMA_SxCR_CHSEL_0,
	.psize = DMA_SxCR_PSIZE_16BIT,
	.paddress = (uint32_t) &ADC_DR(ADC1),
	.msize = DMA_SxCR_MSIZE_16BIT,
	.maddress = (uint32_t) chanvol,
	.doublebuf = 0,
	.minc = 1,
	.circ = 1,
	.pinc = 0,
	.prio = DMA_SxCR_PL_LOW,
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

int main(void)
{
	/* void initialise_monitor_handles(void); */
	/* initialise_monitor_handles(); */
	pll_setup();
	systick_setup(1);
	enable_swo(115200);

	//play ( buffer of single audio, number of points)
	uint16_t *address = sine_256;
	uint16_t n = OUTBUFFERSIZE/2;
	for (int i = 0; i < n; ++i){
		outstream[2*i] = address[i] - 0x8000;
		outstream[2*i+1] = address[i] - 0x8000;
	}

	i2c_setup();
	get_i2c_stat1();
	get_i2c_stat2();

	/* configure codec */
	disable = DAC_C1(0, 0, 0, 0, 0b0000);
	enable = DAC_C1(0, 0, 0, 0, 0b1001);
	vol_full = MASTDA(255, 1);
	vol_low = MASTDA(222,1);

	init_dma_channel(&audioin);
	init_dma_channel(&audioout);
	setup_sound(&f96k);
	init_dma_channel(&volumes);
	setup_adc();
	setup_encoder();
	setup_buttons();
	setup_sddetect();

	sdio_get_resp(1);
	sdio_get_respcmd();
	sdio_get_status();
	sdio_clkcr();
	sdio_pwr();
	
	enable_swo();
	
	uint16_t oldvol[3] = {0,0,0};
	uint8_t enc_pos = 0;
	uint16_t sdcard;
	if (sddetect())
		setup_sdcard(&sdcard);
	while (1) {
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
		/* get encoder position */
		enc_pos = encpos();
		enc_pos = enc_pos;
		uint32_t type = TPIU_TYPE;
		printf("encoder at position %d\n", enc_pos);
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

void exti15_10_isr(void)
{
	if (exti_get_flag_status(EXTI11)){
		exti_reset_request(EXTI11);
		/* start stomped! */
	}
	if (exti_get_flag_status(EXTI12)){
		exti_reset_request(EXTI12);
		/* stop stomped! */
		send_codec_cmd(disable);
	}
}

void exti2_isr(void)
{
	exti_reset_request(EXTI2);
	/* menu button pressed */
	send_codec_cmd(enable);

}

