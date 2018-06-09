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
#include <libopencm3/cm3/tpiu.h>
#include <libopencm3/cm3/scs.h>
#include <libopencm3/cm3/itm.h>
#include "swo.h"

extern uint32_t rcc_ahb_frequency;
int _write(int file, char *ptr, int len)
{
	for (int i = 0; i < len; ++i)
		ITM_SendChar(file, *ptr++);
	return len;
}

void ITM_SendChar(int chan, char c)
{
	while (!(ITM_STIM32(chan) & ITM_STIM_FIFOREADY));
	ITM_STIM32(chan) = c;
}

void enable_swo(int swo_freq)
{
	SCS_DEMCR = SCS_DEMCR_TRCENA;	// enable all features configured and controlled by DWT, ITM, ETM, TPIU
	DBGMCU_CR = DBGMCU_CR_TRACE_IOEN;
	TPIU_CSPSR = 1;

	TPIU_SPPR = TPIU_SPPR_ASYNC_MANCHESTER;	/* trace port protocol = Manchester */
	TPIU_ACPR = (rcc_ahb_frequency / swo_freq) - 1;	// set prescaler
	TPIU_FFCR = TPIU_FFCR_TRIGIN;	// turn off formatter (TPIU_FFCR_ENFCONT)

	ITM_LAR = 0xC5ACCE55;
	ITM_TCR = ITM_TCR_SWOENA | ITM_TCR_SYNCENA | ITM_TCR_ITMENA;
	ITM_TPR = 0; /* all ports accessible unprivileged */
	ITM_TER[0] = 0x7;/* enable 3 stimulus channels, used with ITM_SendChar() */
}
