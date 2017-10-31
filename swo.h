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

#include <libopencm3/stm32/dbgmcu.h>


#define SWO_FREQ 115200
#define HCLK_FREQ 84000000

/* Pelican TPIU
 * cut down TPIU implemented in STM32F7 series
 * see en.DM00224583.pdf page 1882
 */
#define TPIU_CURRENT_PORT_SIZE 				*((volatile unsigned *)(0xE0040004))
#define TPIU_ASYNC_CLOCK_PRESCALER 			*((volatile unsigned *)(0xE0040010))
#define TPIU_SELECTED_PIN_PROTOCOL 			*((volatile unsigned *)(0xE00400F0))
#define TPIU_FORMATTER_AND_FLUSH_CONTROL 	*((volatile unsigned *)(0xE0040304))
#define ITM_LA								*((volatile unsigned *)(0xE0000FB0))
#define ITM_TPR								*((volatile unsigned *)(0xE0000E40))
#define ITM_TER								*((volatile unsigned *)(0xE0000E00))
#define ITM_TCR								*((volatile unsigned *)(0xE0000E80))
#define ITM_TCR_BUSY							(1 << 23)
#define ITM_TCR_TBID_MSK						(0x7F << 16)
#define ITM_TCR_SWOENA	(1 << 4)
#define ITM_TCR_SYNCENA	(1 << 2)
#define ITM_TCR_ITMENA	(1 << 0)
#define ITM_STIM(x)							*((volatile unsigned *)(0xE0000000 + x*0x4))
#define DWT_CTRL	*((volatile unsigned *)(0xE0001000))
#define DWT_CTRL_SYNCTAP_MSK		(3 << 10)
#define TPIU_TYPE				*((volatile unsigned *)(0xE0040FC8))

int _write(int file, char *ptr, int len);
void ITM_SendChar(int chan, char c);
void enable_swo(void);

