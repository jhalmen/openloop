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

#include "dma.h"

void dma_init_channel(struct dma_channel *chan)
{
	rcc_periph_clock_enable(chan->rcc);

	dma_set_transfer_mode(chan->dma, chan->stream, chan->direction);
	dma_set_peripheral_size(chan->dma, chan->stream, chan->psize);
	dma_set_memory_size(chan->dma, chan->stream, chan->msize);
	if (chan->pinc) {
		dma_enable_peripheral_increment_mode(chan->dma, chan->stream);
	}
	if (chan->minc) {
		dma_enable_memory_increment_mode(chan->dma, chan->stream);
	}
	dma_channel_select(chan->dma, chan->stream, chan->channel);
	dma_set_memory_burst(chan->dma, chan->stream, chan->mburst);
	dma_set_peripheral_burst(chan->dma, chan->stream, chan->pburst);
	if (chan->periphflwctrl) {
		dma_set_peripheral_flow_control(chan->dma, chan->stream);
	}
	dma_set_priority(chan->dma, chan->stream, chan->prio);
	/* DMA_SCR(chan->dma, chan->stream) = */

	dma_set_peripheral_address(chan->dma, chan->stream, chan->paddress);
	if(chan->doublebuf) {
		dma_enable_double_buffer_mode(chan->dma, chan->stream);
		dma_set_memory_address_1(chan->dma, chan->stream, chan->maddress1);
	} else if (chan->circ) {
		dma_enable_circular_mode(chan->dma, chan->stream);
	}
	dma_set_memory_address(chan->dma, chan->stream, chan->maddress);
	if (!chan->periphflwctrl) {
		dma_set_number_of_data(chan->dma, chan->stream, chan->numberofdata);
	}
	dma_enable_stream(chan->dma, chan->stream);
}

void dma_enable_channel(struct dma_channel *chan)
{
	dma_enable_stream(chan->dma, chan->stream);
}

void dma_disable_channel(struct dma_channel *chan)
{
	dma_disable_stream(chan->dma, chan->stream);
}
