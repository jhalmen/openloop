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
#ifndef HARDWARE_H
#define HARDWARE_H
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/flash.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/dma.h>
#include <libopencm3/stm32/i2c.h>
#include <libopencm3/stm32/adc.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/exti.h>
#include "sdio.h"
void pll_setup(void);
void plli2s_setup(uint16_t n, uint8_t r);
void i2s2_pin_setup(void);
void i2s_init_master_receive(uint32_t i2s, uint8_t div, uint8_t odd, uint8_t mckoe);
void i2s_init_master_transmit(uint32_t i2s, uint8_t div, uint8_t odd, uint8_t mckoe);
void i2s_init_slave_transmit(uint32_t i2s);
void i2s_init_slave_receive(uint32_t i2s);
void systick_setup(uint32_t tick_frequency);
uint32_t i2s_read(uint32_t i2s);
uint8_t chkside(uint32_t i2s);
void enable_i2s(uint32_t i2s);
void setup_adc(void);
void setup_encoder(void);
uint8_t encpos(void);
void setup_buttons(void);
void setup_sddetect(void);
uint8_t sddetect(void);
void setup_sdcard(uint16_t *card);
uint32_t sd_status(void);
void sd_identify(uint16_t *rca);


void send_codec_cmd(uint16_t cmd);
void i2c_setup(void);

struct dma_channel {
	uint32_t rcc;
	uint32_t dma;
	uint32_t stream;
	uint8_t direction;
	uint32_t channel;
	uint32_t paddress;
	uint32_t psize;
	uint8_t pinc;
	uint32_t maddress;
	uint32_t maddress1;
	uint32_t msize;
	uint8_t minc;
	uint8_t circ;
	uint32_t doublebuf;
	uint32_t prio;
	uint16_t numberofdata;
};

struct i2sfreq {
	uint16_t plln;
	uint8_t pllr;
	uint8_t div;
	uint8_t odd;
};

void setup_sound(struct i2sfreq * f);
void init_dma_channel(struct dma_channel* chan);

/* delete this debug function */
uint32_t get_i2c_stat1(void);
/* delete this debug function */
uint32_t get_i2c_stat2(void);

#endif //HARDWARE_H
