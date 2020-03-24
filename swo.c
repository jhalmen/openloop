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

int _write(int chan, char *ptr, int len)
{
	for (int i = 0; i < len; ++i) {
		while (!(ITM_STIM32(chan) & ITM_STIM_FIFOREADY));
		ITM_STIM32(chan) = *ptr++;
	}
	return len;
}

void enable_swo(long int ahb_freq, long int swo_freq)
{
	/* enable all features configured and controlled by DWT, ITM, ETM, TPIU */
	SCS_DEMCR = SCS_DEMCR_TRCENA;
	DBGMCU_CR = DBGMCU_CR_TRACE_IOEN;
	TPIU_CSPSR = 1;

	/* trace port protocol = Manchester */
	TPIU_SPPR = TPIU_SPPR_ASYNC_MANCHESTER;
	/* set prescaler */
	TPIU_ACPR = (ahb_freq / swo_freq) - 1;
	/* turn off formatter (TPIU_FFCR_ENFCONT) */
	TPIU_FFCR = TPIU_FFCR_TRIGIN;

	ITM_LAR = 0xC5ACCE55;
	ITM_TCR = ITM_TCR_SWOENA | ITM_TCR_SYNCENA | ITM_TCR_ITMENA;
	/* all ports accessible unprivileged */
	ITM_TPR = 0;
	/* enable 3 stimulus channels, used with ITM_putc() */
	ITM_TER[0] = 0x7;
}
