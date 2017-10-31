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
#include "swo.h"

int _write(int file, char *ptr, int len)
{
	for (int i = 0; i < len; ++i)
		ITM_SendChar(file, *ptr++);
	return len;
}

void ITM_SendChar(int chan, char c)
{
	while (ITM_STIM(chan) == 0);
	ITM_STIM(chan) = c;
}
	
void enable_swo(void)
{
		/*************************/
	//uint32_t SWOPrescaler;
	//uint32_t SWOSpeed;
	//SWOSpeed = 6000000;
	//*((volatile unsigned *)0xE000EDFC) |= 0x01000000;   // "Debug Exception and Monitor Control Register (DEMCR)"
	//*((volatile unsigned *)0xE0042004) = 0x00000027;
	//*((volatile unsigned *)0xE00400F0) = 0x00000002;   // "Selected PIN Protocol Register": Select which protocol to use for trace output (2: SWO)
	//SWOPrescaler = (82000000 / SWOSpeed) - 1;  // SWOSpeed in Hz
	//*((volatile unsigned *)0xE0040010) = SWOPrescaler; // "Async Clock Prescaler Register". Scale the baud rate of the asynchronous output
	//*((volatile unsigned *)0xE0000FB0) = 0xC5ACCE55;   // ITM Lock Access Register, C5ACCE55 enables more write access to Control Register 0xE00 :: 0xFFC
	//*((volatile unsigned *)0xE0000E80) = 0x0001000D;   // ITM Trace Control Register
	//*((volatile unsigned *)0xE0000E40) = 0x0000000F;   // ITM Trace Privilege Register
	//*((volatile unsigned *)0xE0000E00) = 0x00000001;   // ITM Trace Enable Register. Enabled tracing on stimulus ports. One bit per stimulus port.
	//*((volatile unsigned *)0xE0001000) = 0x400003FE;   // DWT_CTRL
	//*((volatile unsigned *)0xE0040304) = 0x00000100;   // Formatter and Flush Control Register
	//#define TRACE_PORT0 *(volatile unsigned *)0xE0000000
	
	*((volatile unsigned *)0xE000EDFC) |= 0x01000000;   // "Debug Exception and Monitor Control Register (DEMCR)"
	DBGMCU_CR = DBGMCU_CR_SLEEP | DBGMCU_CR_STOP | DBGMCU_CR_STANDBY | DBGMCU_CR_TRACE_IOEN;
  TPIU_CURRENT_PORT_SIZE = 1; /* port size = 1 bit */
  TPIU_SELECTED_PIN_PROTOCOL = 1; /* trace port protocol = Manchester */
  TPIU_ASYNC_CLOCK_PRESCALER = (HCLK_FREQ / SWO_FREQ) - 1;
  TPIU_FORMATTER_AND_FLUSH_CONTROL = 0x100; /* turn off formatter (0x02 bit) */

  ITM_LA = 0xC5ACCE55;
  ITM_TCR = ITM_TCR_TBID_MSK | ITM_TCR_SWOENA | ITM_TCR_SYNCENA | ITM_TCR_ITMENA;
  ITM_TPR = 0; /* all ports accessible unprivileged */
  ITM_TER = 3; /* enable stimulus channel 0, used with ITM_SendChar() */

  /* this apparently turns off sync packets, see SYNCTAP in DDI0403D pdf: */
  DWT_CTRL &= ~DWT_CTRL_SYNCTAP_MSK;
}
