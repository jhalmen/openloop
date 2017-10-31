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
#include "sdio.h"

uint32_t sdio_pwr()
{
	return SDIO_POWER;
}
uint32_t sdio_clkcr()
{
	return SDIO_CLKCR;
}
void sdio_power_on(void)
{
	SDIO_POWER |= SDIO_POWER_PWRCTRL_PWRON;
}

void sdio_power_off(void)
{
	SDIO_POWER &= ~SDIO_POWER_PWRCTRL_PWRON;
}

void sdio_enable_hw_flow_control(void)
{
	SDIO_CLKCR |= SDIO_CLKCR_HWFC_EN;
}

void sdio_disable_hw_flow_control(void)
{
	SDIO_CLKCR &= ~SDIO_CLKCR_HWFC_EN;
}

void sdio_enable_dephase(void)
{
	SDIO_CLKCR |= SDIO_CLKCR_NEGEDGE;
}

void sdio_disable_dephase(void)
{
	SDIO_CLKCR &= ~SDIO_CLKCR_NEGEDGE;
}

void sdio_set_bus_width(enum sdio_bus_width w)
{
	//send SELECT/DESELECT_CARD (CMD7)
	//send SET_BUS_WIDTH(ACMD6)
	//put pins in respective states
		// enable PullUps on DAT lines
	SDIO_CLKCR |= w << SDIO_CLKCR_WIDBUS_SHIFT;
}

void sdio_enable_clk_bypass(void)
{
	SDIO_CLKCR |= SDIO_CLKCR_BYPASS;
}

void sdio_disable_clk_bypass(void)
{
	SDIO_CLKCR &= ~SDIO_CLKCR_BYPASS;
}

void sdio_enable_pwrsave(void)
{
	SDIO_CLKCR |= SDIO_CLKCR_PWRSAV;
}

void sdio_disable_pwrsave(void)
{
	SDIO_CLKCR &= ~SDIO_CLKCR_PWRSAV;
}

void sdio_enable_clock(void)
{
	SDIO_CLKCR |= SDIO_CLKCR_CLKEN;
}

void sdio_disable_clock(void)
{
	SDIO_CLKCR &= ~SDIO_CLKCR_CLKEN;
}

void sdio_set_clk_div(uint8_t div)
{
	/* SDIO_CK = SDIOCLK / (div + 2) */
	SDIO_CLKCR = (SDIO_CLKCR & 0xFFFFFF00) | div;
}

uint32_t sdio_get_status(void)
{
	return SDIO_STA;
}

uint8_t sdio_get_flag(enum sdio_status_flags flag)
{
	return !!(SDIO_STA & flag);
}

uint32_t sdio_get_resp(int n)
{
	switch (n){
	case 1:
		return SDIO_RESP1;
	case 2:
		return SDIO_RESP2;
	case 3:
		return SDIO_RESP3;
	case 4:
		return SDIO_RESP4;
	default:
		return 0xFFFFFFFF;
	}
}

uint8_t sdio_get_respcmd(void)
{
	return SDIO_RESPCMD & SDIO_RESPCMD_MSK;
}

void sdio_clear_flag(enum sdio_status_flags flag)
{
	SDIO_ICR = flag;
}
	
void sdio_enable_interrupts(uint32_t interrupts)
{
	SDIO_MASK |= interrupts;
}

void sdio_disable_interrupts(uint32_t interrupts)
{
	SDIO_MASK &= ~interrupts;
}

uint8_t sdio_send_cmd_blocking(uint8_t cmd, uint32_t arg)
{
	/* clear flags */
	SDIO_ICR = (3 << 22) | (2047);
	/* check response type */
	uint8_t resp;
	static uint8_t acmd;
	switch(cmd){
	case 2:
	case 9:
	case 10:
		resp = 3;

		break;
	case 0:
		resp = 0;
		break;
	case 13:
		if (acmd){
			resp = 3;
			break;
		}
	default:
		resp = 1;
	}
	
	/* set arg and send cmd */
	SDIO_ARG = arg;
	uint32_t tmp;
	tmp = (SDIO_CMD_CMDINDEX_MSK & (cmd << SDIO_CMD_CMDINDEX_SHIFT))
				| (resp << SDIO_CMD_WAITRESP_SHIFT)
				| SDIO_CMD_CPSMEN;
	SDIO_CMD = tmp;
	/* wait till finished */
	while (SDIO_STA & SDIO_STA_CMDACT) {}
	
	acmd = (cmd == 55);
	
	if (resp &&	(SDIO_STA & SDIO_STA_CMDREND))
		return 0;
	if (!resp && (SDIO_STA & SDIO_STA_CMDSENT))
		return 0;
	return -1;	
}

