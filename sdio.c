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
#include <stdio.h>

char *sdiohoststat[] = {
	"CCRCFAIL",
	"DCRCFAIL",
	"CTIMEOUT",
	"DTIMEOUT",
	"TXUNDERR",
	"RXOVERR",
	"CMDREND",
	"CMDSENT",
	"DATAEND",
	"STBITERR",
	"DBCKEND",
	"CMDACT",
	"TXACT",
	"RXACT",
	"TXFIFOHE",
	"RXFIFOHF",
	"TXFIFOF",
	"RXFIFOF",
	"TXFIFOE",
	"RXFIFOE",
	"TXDAVL",
	"RXDAVL",
	"SDIOIT",
	"CEATAEND"
};

char *cardstat[] = {
	"", "", "",
	"AKE_SEQ_ERROR",
	"",
	"APP_CMD",
	"", "",
	"READY_FOR_DATA",
	"", "", "", "",
	"ERASE_RESET",
	"CARD_ECC_DISABLED",
	"WP_ERASE_SKIP",
	"CSD_OVERWRITE",
	"", "",
	"ERROR",
	"CC_ERROR",
	"CARD_ECC_FAILED",
	"ILLEGAL_COMMAND",
	"COM_CRC_ERROR",
	"LOCK_UNLOCK_FAILED",
	"CARD_IS_LOCKED",
	"WP_VIOLATION",
	"ERASE_PARAM",
	"ERASE_SEQ_ERROR",
	"BLOCK_LEN_ERROR",
	"ADDRESS_ERROR",
	"OUT_OF_RANGE"
};

char *curr_state[] = {
	"idle",
	"ready",
	"ident",
	"stby",
	"tran",
	"data",
	"rcv",
	"prg",
	"dis",
	"reserved",
	"reserved",
	"reserved",
	"reserved",
	"reserved",
	"reserved",
	"reserved for I/O"
};

static uint16_t sd_rca;
static struct dma_channel sd_dma = {
	.rcc = RCC_DMA2,
	.dma = DMA2,
	.stream = DMA_STREAM3,
	/* .direction = DMA_SxCR_DIR_PERIPHERAL_TO_MEM, */
	.channel = DMA_SxCR_CHSEL_4,
	.psize = DMA_SxCR_PSIZE_32BIT,
	.paddress = (uint32_t)&SDIO_FIFO,
	.msize = DMA_SxCR_MSIZE_32BIT,
	/* .maddress = data, */
	.doublebuf = 0,
	.minc = 1,
	.circ = 0,
	.pinc = 0,
	.prio = DMA_SxCR_PL_HIGH,
	.periphflwctrl = 1,
	.pburst = DMA_SxCR_PBURST_INCR4,
	.mburst = DMA_SxCR_MBURST_INCR4,
	.numberofdata = 0
};

uint32_t sdio_pwr()
{
	return SDIO_POWER;
}
uint32_t sdio_clkcr()
{
	return SDIO_CLKCR;
}
uint32_t sdio_get_host_status(void)
{
	return SDIO_STA;
}

uint8_t sdio_get_host_flag(enum sdio_status_flags flag)
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
	return SDIO_RESPCMD & SDIO_RESPCMD_MASK;
}

void sdio_clear_host_flag(enum sdio_status_flags flag)
{
	SDIO_ICR = flag;
}

uint8_t sdio_send_cmd_blocking(uint8_t cmd, uint32_t arg)
{
	fprintf(stderr,"sending command %d with arg %08lx", cmd, arg);
	/* clear flags */
	SDIO_ICR = (3 << 22) | (2047);
	uint8_t acmd = (SDIO_RESPCMD & SDIO_RESPCMD_MASK) == 55;
	/* check response type */
	uint8_t resp;
	switch(cmd){
	case 3:
		resp = 6;
		break;
	case 2:
	case 9:
	case 10:
		resp = 2;
		break;
	case 8:
		resp = 7;
		break;
	case 0:
	case 4:
	case 15:
		resp = 0;
		break;
	/* according to
	 * http://users.ece.utexas.edu/~valvano/EE345M/SD_Physical_Layer_Spec.pdf
	 * acmd13 has a response R1, not as thought R3 */
	/* case 13: */
	/* 	if (acmd){ */
	/* 		resp = 3; */
	/* 		break; */
	/* 	} */
	case 41:
		if (acmd) {
			resp = 3;
		}
		break;
	default:
		resp = 1;
	}

	/* set arg and send cmd */
	SDIO_ARG = arg;
	uint8_t waitresp;
	switch (resp) {
	case 0:
		waitresp = 0;
		break;
	case 1:
	case 3:
	case 6:
	case 7:
		waitresp = 1;
		break;
	case 2:
		waitresp = 3;
		break;
	};

	uint32_t tmp;
	tmp = (SDIO_CMD_CMDINDEX_MASK & (cmd << SDIO_CMD_CMDINDEX_SHIFT))
				| (waitresp << SDIO_CMD_WAITRESP_SHIFT)
				| SDIO_CMD_CPSMEN;
	SDIO_CMD = tmp;
	/* wait till finished */
	while (SDIO_STA & SDIO_STA_CMDACT) {
		fprintf(stderr,".");
	}
	fprintf(stderr," ");
	int ret;
	if (!resp && SDIO_STA & SDIO_STA_CMDSENT) {
		ret = 0;
	} else if (resp) {
		/* wait for response */
		uint32_t sta;
		while ((sta = SDIO_STA)) {
			if (sta & SDIO_STA_CMDREND) {
				ret = 0;
				break;
			} else if (sta & SDIO_STA_CTIMEOUT) {
				ret = ECTIMEOUT;
				break;
			} else if (sta & SDIO_STA_CCRCFAIL) {
				ret = ECCRCFAIL;
				break;
			}
		}
	} else {
		/* shouldn't happen */
		ret = EUNKNOWN;
	}
	fprintf(stderr,"sent\n");
	printf("RESPONSE: %08lx %08lx %08lx %08lx\n", SDIO_RESP1, SDIO_RESP2, SDIO_RESP3, SDIO_RESP4);
	uint8_t *response =(uint8_t *) &SDIO_RESP1;
	for (int i = 0; i < 16; ++i)
		printf("%02x", *response++);
	printf("\n");
	if (resp == 1) {
		print_card_stat();
	}
	print_host_stat();
	return ret;
}

void print_card_stat(void)
{
	uint32_t resp = sdio_get_resp(1);
	printf("CARD STATUS: #################\n");
	for (int i = 0; i < 32; ++i) {
		if (i >= 9 && i <= 12) {
			printf("##[%2ld] state: %s\n", (resp >> 9) & 0xf,curr_state[(resp >> 9) & 0xf]);
			i = 12;
		}
		else if (resp & (1 << i)) {
			printf("##[%2d] %s\n", i,cardstat[i]);
		}
	}
}

void print_host_stat(void)
{
	uint32_t st = SDIO_STA;
	for (int i = 0; i < 24; ++i)
		if(st & 1<<i)
			printf("[%2d]%s ", i, sdiohoststat[i]);
	printf("\n");
}

uint32_t sdio_get_card_status(void)
{
	// SEND_STATUS (CMD13)
	sdio_send_cmd_blocking(13, sd_rca << 16);
	// read response
	return SDIO_RESP1;
}

void sdio_identify(void)
{
	//activate bus
	sdio_send_cmd_blocking(0, 0);
	/* already active */
	/* v.2.0 card? */
	sdio_send_cmd_blocking(8, 0x1aa);
	if (sdio_get_resp(1) != 0x1aa) {
		// card is not v2, this is just a test
		// not actually needed to be v2 probably
		// so i might just remove this if
		while (1) __asm__("nop");
	}

	//send SD_APP_OP_COND (ACMD41)
	/* cards respond with operating condition registers,
	 * incompatible cards are placed in inactive state
	 */
	// repeat ACMD41 until no card responds with busy bit set anymore
	// uint32_t response;
	do {
		sdio_send_cmd_blocking(55, 0);
		sdio_send_cmd_blocking(41, 0xff8000); // argument needed so that card
						// knows what voltages are supported
		//while (!(SDIO_STA & SDIO_STA_CMDREND));
	} while (!(SDIO_RESP1 & 1 << 31));
	//clear response received flag
	//SDIO_ICR |= SDIO_ICR_CMDRENDC;

	//send ALL_SEND_CID (CMD2)
	//cards send back CIDs and enter identification state
	sdio_send_cmd_blocking(2,0);

	//send SET_RELATIVE_ADDR (CMD3) to a specific card
	/* this card enters standby state */
	/* after powerup or CMD0 (GO_IDLE_STATE) cards are initialized
	 * with default RCA = 0x0001, maybe i can just use that
	 */
	sdio_send_cmd_blocking(3, 0);
	/* while (!(SDIO_STA & SDIO_STA_CMDREND)); */
	sd_rca = SDIO_RESP1 >> 16;
	//clear response received flag
	/* SDIO_ICR |= SDIO_ICR_CMDRENDC; */

	/* after detection is done host can send
	 * SET_CLR_CARD_DETECT (ACMD42) to disable card internal PullUp
	 * on DAT3 line
	 */
	sdio_send_cmd_blocking(7, (sd_rca) << 16);
	sdio_send_cmd_blocking(55, (sd_rca)  << 16);
	sdio_send_cmd_blocking(42, (sd_rca)  << 16);
	sdio_send_cmd_blocking(7, 0);
}

void write_block(uint32_t *buffer, uint32_t length, uint32_t sd_address)
{
	sd_dma.direction = DMA_SxCR_DIR_MEM_TO_PERIPHERAL;
	sd_dma.maddress = (uint32_t)buffer;
	//TODO init dma dir to card
	sdio_send_cmd_blocking(24, sd_address);
	dma_init_channel(&sd_dma);
	/* timeout : 250ms */
	SDIO_DTIMER = 6000000;
	SDIO_DLEN = length;
	SDIO_DCTRL = 	(9 << 4)| /* DATA BLOCKSIZE 2^x bits */
			(1 << 3)| /* DMA Enable */
			(0 << 2)| /* DTMODE: Block (0) or Stream (1) */
			(0 << 1)| /* DTDIR: from controller */
			(1 << 0); /* DTEN: enable data state machine */
	/* sdio_send_cmd_blocking(7,0); */
}

void read_status(uint32_t *buffer)
{
	dma_disable_channel(&sd_dma);
	// select card
	sdio_send_cmd_blocking(7, sd_rca<<16);
	// ACMD13
	sdio_send_cmd_blocking(55, sd_rca<<16);
	sdio_send_cmd_blocking(13, sd_rca<<16);
	//TODO init dma dir from card
	sd_dma.direction = DMA_SxCR_DIR_PERIPHERAL_TO_MEM;
	sd_dma.maddress = (uint32_t)buffer;
	dma_init_channel(&sd_dma);
	/* timeout : 100ms */
	SDIO_DTIMER = 2400000;
	SDIO_DLEN = 64;
	SDIO_DCTRL = 	(9 << 4)| /* DATA BLOCKSIZE 2^x bits */
			(1 << 3)| /* DMA Enable */
			(0 << 2)| /* DTMODE: Block (0) or Stream (1) */
			(1 << 1)| /* DTDIR: to controller */
			(1 << 0); /* DTEN: enable data state machine */
}

void read_block(uint32_t length, uint32_t sd_address)
{
	//TODO init dma dir from card
	sdio_send_cmd_blocking(17, sd_address);
	/* timeout : 100ms */
	SDIO_DTIMER = 2400000;
	SDIO_DLEN = length;	// in bytes
	SDIO_DCTRL = 	(9 << 4)| /* DATA BLOCKSIZE 2^x bits */
			(1 << 3)| /* DMA Enable */
			(0 << 2)| /* DTMODE: Block (0) or Stream (1) */
			(1 << 1)| /* DTDIR: to controller */
			(1 << 0); /* DTEN: enable data state machine */
}

void erase(uint32_t start, uint32_t nblocks)
{
	/* timeout: nblocks * 250ms */
	start+=0;
	nblocks+=0;
}
