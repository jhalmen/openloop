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

#ifndef DMA_H
#define DMA_H
#include <libopencm3/stm32/dma.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/cm3/nvic.h>
#define dprintf(...)

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
	uint8_t periphflwctrl;
	uint32_t pburst;
	uint32_t mburst;
	uint32_t interrupts;
	uint32_t nvic;
};

void dma_print_status(struct dma_channel *chan);
void dma_channel_init(struct dma_channel* chan);
void dma_channel_enable(struct dma_channel *chan);
void dma_channel_disable(struct dma_channel *chan);
#endif
