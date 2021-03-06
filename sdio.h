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

#ifndef SDIO_H
#define SDIO_H
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/sdio.h>
#include "dma.h"

uint32_t sdio_get_host_status(void);
uint8_t sdio_get_host_flag(enum sdio_status_flags flag);
uint32_t sdio_get_resp(int n);
uint8_t sdio_get_respcmd(void);
void sdio_clear_host_flag(enum sdio_status_flags flag);
enum sdio_status sdio_send_cmd_blocking(uint8_t cmd, uint32_t arg);
enum sd_status sd_identify(void);
uint32_t sdio_get_card_status(void);

uint32_t sdio_get_host_clkcr(void);
uint32_t sdio_get_host_pwr(void);


void sd_enable_wbus(void);
void sd_select_card(void);

void print_card_stat(void);
void print_host_stat(void);
void print_response_raw(void);
void read_status(void);
void read_scr(void);
void read_single_block(uint32_t *dest_buffer, uint32_t sd_address);
void write_single_block(uint32_t *data, uint32_t sd_address);
void erase(uint32_t start, uint32_t nblocks);
void sd_stop_data_transfer(void);

enum sdio_status {
	ECTIMEOUT = 1,
	ECCRCFAIL,
	EUNKNOWN
};
enum sd_status {
	SUCCESS,
	FAILURE,
	BAD_CARD
};
#endif
