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
#include <libopencm3/cm3/nvic.h>

/* this driver supports Physical Layer Version 2.00 and High Capacity SD Memory Cards */
/* high voltage cards only */

#define byte_swap(word) \
	__asm__("rev %[swap], %[swap]" : [swap] "=r" (word) : "0" (word));
const char *sdiohoststat[] = {
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

const char *cardstat[] = {
	"", "", "",
	"AKE_SEQ_ERROR",
	"",
	"APP_CMD",
	"", "",
	"READY_FOR_DATA",
	"", "", "", "", // curr_state[]
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

const char *curr_state[] = {
	"idle",
	"ready",
	"ident",
	"stby",
	"tran",
	"data",
	"rcv",
	"prg",
	"dis",
	"reserved", "reserved", "reserved", "reserved", "reserved",
	"reserved", "reserved for I/O"
};

struct {
	uint16_t rca;
	bool sdv2;		// physical spec 2 card?
	bool high_capacity;
	uint32_t memcap;	// calculated memory capacity
	/* uint16_t voltages;	// this is actually just ocr << 15 & 0x1ff */
	uint32_t last_status;	// store for latest R1 response

	uint32_t ocr;		// operating conditions register
	uint32_t csd[4];	// card specific data register
	uint32_t sdstatus[16];
	uint32_t taac;		// time dependent factor of access time
	uint32_t nsac;		// clock rate dependent factor of access time
	uint16_t block_len;	// read=write block length. in bytes
	uint16_t c_size;	// used to calculate memory capacity
	uint16_t mult;		// used to calculate memory capacity
	uint16_t sector_size;	// erasable sector, measured in write blocks
} sdcard;

#include "sdio_help.c"

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
	.mburst = DMA_SxCR_MBURST_SINGLE,
	.numberofdata = 0
};

struct {
	uint8_t err;
	const char* msg;
} sdio_error[] = {
	{0, ""},
	{ECTIMEOUT,	"CTIMEOUT"},
	{ECCRCFAIL,	"CCRCFAIL"},
	{EUNKNOWN,	"UNKNOWN"}
};


uint32_t sdio_get_host_pwr()
{
	return SDIO_POWER;
}
uint32_t sdio_get_host_clkcr()
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
	dprintf(1,"sending cmd %d with arg %08lx", cmd, arg);
	/* clear flags */
	SDIO_ICR = (3 << 22) | (2047);
	uint8_t acmd = (SDIO_RESPCMD & SDIO_RESPCMD_MASK) == 55;
	/* check response type */
	/* according to SD Spec */
	uint8_t resp;
	uint8_t waitresp;
	switch(cmd){
	case 3:
		resp = 6;
		waitresp = 1;
		break;
	case 2:
	case 9:
	case 10:
		resp = 2;
		waitresp = 3;
		break;
	case 8:
		resp = 7;
		waitresp = 1;
		break;
	case 0:
	case 4:
	case 15:
		waitresp = resp = 0;
		break;
	case 41:
		if (acmd) {
			resp = 3;
			waitresp = 1;
			break;
		}
		/* fallthrough */
	/* according to
	 * http://users.ece.utexas.edu/~valvano/EE345M/SD_Physical_Layer_Spec.pdf
	 * acmd13 has a response R1, not as thought R3 */
	default:
		waitresp = resp = 1;
	}

	/* set arg and send cmd */
	SDIO_ARG = arg;
	uint32_t tmp;
	tmp = (SDIO_CMD_CMDINDEX_MASK & (cmd << SDIO_CMD_CMDINDEX_SHIFT))
				| (waitresp << SDIO_CMD_WAITRESP_SHIFT)
				| SDIO_CMD_CPSMEN;
	SDIO_CMD = tmp;
	/* wait till sent */
	while (SDIO_STA & SDIO_STA_CMDACT) {
		dprintf(1,".");
	}
	dprintf(1,"\nsent\n");
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
	print_response_raw();
	if (resp == 1) {
		sdcard.last_status = SDIO_RESP1;
		print_card_stat();
	}
	print_host_stat();
	/* dprintf(0,"sending command %d returned %s\n", cmd, sdio_error[ret].msg); */
	if (ret)
	{
		if (resp == 3)	// ignore CCRCFAIL for response type R3
			return 0;
		if (cmd == 7 && arg != (uint32_t)sdcard.rca << 16) {// CTIMEOUT is a successful deselect of card
			sdcard.last_status = 0;
			return 0;
		}
		/* TODO: something's happened. probably want to let the program */
		/* know instead of blocking it. */
		dprintf(0, "sending command has returned error. stopping\n");
		while(1) __asm__("nop");
	}
	return ret;
}

void print_card_stat(void)
{
	uint32_t resp = sdio_get_resp(1);
	printf("CARD STATUS: #################\n");
	/* print out what every bit means */
	for (int i = 0; i < 32; ++i) {
		if (i >= 9 && i <= 12) {  // 4bit field
			printf("##[%2ld] state: %s\n",
					(resp >> 9) & 0xf,
					curr_state[(resp >> 9) & 0xf]);
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

void print_response_raw(void)
{
	printf("RESPONSE: %08lx %08lx %08lx %08lx\n",
			SDIO_RESP1, SDIO_RESP2, SDIO_RESP3, SDIO_RESP4);
}


uint32_t sdio_get_card_status(void)
{
	// SEND_STATUS (CMD13)
	sdio_send_cmd_blocking(13, sdcard.rca << 16);
	// read response
	return SDIO_RESP1;
}

void sdio_identify(void)
{
	int retries = 3;
	// according to family reference manual,
	// set data timer register at this point (rm0368 p.622 -> 5.a)
	// TODO: -^ or don't if it works

	//activate bus
	sdio_send_cmd_blocking(0, 0);

	/* v.2.0 card? */
	sdio_send_cmd_blocking(8, 0x1aa);

	if (SDIO_RESP1 == 0x1aa) {
		sdcard.sdv2 = true;
	} else {
		sdcard.sdv2 = false;
	}

	//send SD_APP_OP_COND (ACMD41)
	/* cards respond with operating condition registers,
	 * incompatible cards are placed in inactive state
	 */
	/* do commented out part to actually get supported voltages from card
	 * instead of assuming them */
	/* // get supported voltages */
	/* // TODO: do something with these voltages */
	/* sdio_send_cmd_blocking(55, 0); */
	/* sdio_send_cmd_blocking(41, 0); */
	/* sdcard.voltages = SDIO_RESP1 >> 15 & 0x1ff; */
	// repeat ACMD41 until card not busy or number of retries
	do {
		sdio_send_cmd_blocking(55, 0);
		sdio_send_cmd_blocking(41, 0x40ff8000); // argument needed so that card
							// knows we support all except low voltages
							// and high capacity cards
	} while (--retries && !(SDIO_RESP1 & 1 << 31));
	if (!(SDIO_RESP1 & 1 << 31)) {
		dprintf(0, "SD card not supported!\n");
		// clear sd struct
		for (unsigned int i = 0; i < sizeof sdcard; ++i)
			*((uint8_t*)&sdcard + i) = 0;
		return;
	}
	sdcard.high_capacity = SDIO_RESP1 >> 30 & 0x1;
	sdcard.ocr = SDIO_RESP1;
	//clear response received flag
	//SDIO_ICR |= SDIO_ICR_CMDRENDC;

	//send ALL_SEND_CID (CMD2)
	//cards send back CIDs and enter identification state
	sdio_send_cmd_blocking(2,0);

	//send SET_RELATIVE_ADDR (CMD3) to a specific card
	/* this card enters standby state */
	sdio_send_cmd_blocking(3, 0);

	sdcard.rca = SDIO_RESP1 >> 16;

	/* after detection is done host can send
	 * SET_CLR_CARD_DETECT (ACMD42) to disable card internal PullUp
	 * on DAT3 line
	 */
	sdio_send_cmd_blocking(7, sdcard.rca << 16);
	sdio_send_cmd_blocking(55, sdcard.rca << 16);
	sdio_send_cmd_blocking(42, sdcard.rca << 16);
	sdio_send_cmd_blocking(7, 0);

	// get remaining sdcard information
	// CSD Register
	sdio_send_cmd_blocking(13, sdcard.rca << 16);
	sdio_send_cmd_blocking(9, sdcard.rca << 16);
	sdcard.csd[0] = SDIO_RESP1;
	sdcard.csd[1] = SDIO_RESP2;
	sdcard.csd[2] = SDIO_RESP3;
	sdcard.csd[3] = SDIO_RESP4;

	parse_csd();

	// TODO: probably return something to caller in this or in error case
}

/*
 * Memory access commands include block read commands (CMD17, CMD18), block write commands
 * (CMD24, CMD25), and block erase commands (CMD32, CMD33).
 * Following are the functional differences of memory access commands between Standard Capacity and
 * High Capacity SD Memory Cards:
 *
 *  • Command Argument
 *  In High Capacity Cards, the 32-bit argument of memory access commands uses the memory
 *  address in block address format. Block length is fixed to 512 bytes,
 *  In Standard Capacity Cards, the 32-bit argument of memory access commands uses the
 *  memory address in byte address format. Block length is determined by CMD16,
 *  i.e.:
 *  (a) Argument 0001h is byte address 0001h in the Standard Capacity Card and 0001h block in
 *  High Capacity Card
 *  (b) Argument 0200h is byte address 0200h in the Standard Capacity Card and 0200h block in
 *  High Capacity Card
 *
 *  • Partial Access and Misalign Access
 *  Partial access and Misalign access (crossing physical block boundary) are disabled in High
 *  Capacity Card as the block address is used. Access is only granted based on block
 *  addressing.
 *
 *  • Set Block Length
 *  When memory read and write commands are used in block address mode, 512-byte fixed
 *  block length is used regardless of the block length set by CMD16. The setting of the block
 *  length does not affect the memory access commands.  CMD42 is not classified as a memory
 *  access command. The data block size shall be specified by CMD16 and the block length can
 *  be set up to 512 bytes. Setting block length larger than 512 bytes sets the
 *  BLOCK_LEN_ERROR error bit regardless of the card capacity.
 *
 *  • Write Protected Group
 *  High Capacity SD Memory Card does not support write-protected groups. Issuing CMD28,
 *  CMD29 and CMD30 generates the ILLEGAL_COMMAND error.
 * */


void write_block(uint32_t *buffer, uint32_t length, uint32_t sd_address)
{
	sd_dma.direction = DMA_SxCR_DIR_MEM_TO_PERIPHERAL;
	sd_dma.maddress = (uint32_t)buffer;
	//TODO init dma dir to card
	sdio_send_cmd_blocking(24, sd_address);
	dma_channel_init(&sd_dma);
	/* timeout : 250ms */
	SDIO_DTIMER = 6000000;
	SDIO_DLEN = length;
	SDIO_DCTRL = 	(9 << 4)| /* DATA BLOCKSIZE 2^x bytes */
			(1 << 3)| /* DMA Enable */
			(0 << 2)| /* DTMODE: Block (0) or Stream (1) */
			(0 << 1)| /* DTDIR: from controller */
			(1 << 0); /* DTEN: enable data state machine */
	/* sdio_send_cmd_blocking(7,0); */
}

void read_status(uint32_t *buffer)
{
	dma_channel_disable(&sd_dma);
	// select card
	if ((sdcard.last_status & (4 << 9)) == 0) {
		sdio_send_cmd_blocking(7, sdcard.rca<<16);
	}
	// take care of dma
	sd_dma.direction = DMA_SxCR_DIR_PERIPHERAL_TO_MEM;
	sd_dma.maddress = (uint32_t)buffer;
	dma_channel_init(&sd_dma);
	dprintf(0, "dma enabled: %d\n", DMA_SCR(DMA2, DMA_STREAM3) & DMA_SxCR_EN);
	// ACMD13
	sdio_send_cmd_blocking(55, sdcard.rca<<16);
	sdio_send_cmd_blocking(13, sdcard.rca<<16);
	/* timeout : 100ms */
	SDIO_DTIMER = 2400000;
	/* SDIO_DTIMER = 24000000; */
	SDIO_DLEN = 64;
	SDIO_DCTRL = 	(6 << 4)| /* DATA BLOCKSIZE 2^x bytes */
			(1 << 3)| /* DMA Enable */
			(0 << 2)| /* DTMODE: Block (0) or Stream (1) */
			(1 << 1)| /* DTDIR: to controller */
			(1 << 0); /* DTEN: enable data state machine */
	dprintf(0, "dma enabled: %d\n", DMA_SCR(DMA2, DMA_STREAM3) & DMA_SxCR_EN);
	/* sdio_send_cmd_blocking(7, 0); */
}

void read_scr(uint32_t *buffer)
{
	dma_channel_disable(&sd_dma);
	// select card
	if ((sdcard.last_status & (4 << 9)) == 0) {
		sdio_send_cmd_blocking(7, sdcard.rca<<16);
	}
	// take care of dma
	sd_dma.direction = DMA_SxCR_DIR_PERIPHERAL_TO_MEM;
	sd_dma.maddress = (uint32_t)buffer;
	dma_channel_init(&sd_dma);
	// ACMD51
	sdio_send_cmd_blocking(55, sdcard.rca<<16);
	sdio_send_cmd_blocking(51, sdcard.rca<<16);
	/* timeout : 100ms */
	SDIO_DTIMER = 2400000;
	/* SDIO_DTIMER = 0xffffffff; */
	SDIO_DLEN = 8;
	SDIO_DCTRL = 	(3 << 4)| /* DATA BLOCKSIZE 2^x bytes */
			(1 << 3)| /* DMA Enable */
			(0 << 2)| /* DTMODE: Block (0) or Stream (1) */
			(1 << 1)| /* DTDIR: to controller */
			(1 << 0); /* DTEN: enable data state machine */
	dprintf(1, "dma enabled: %d\n", DMA_SCR(DMA2, DMA_STREAM3) & DMA_SxCR_EN);
	/* sdio_send_cmd_blocking(7, 0); */
	//block until reading is done
	// TODO: remove this blocking
	while (DMA_SCR(DMA2, DMA_STREAM3) & DMA_SxCR_EN);
	for (int i = 0; i < 2; ++i)
		byte_swap(buffer[i]);
	dprintf(1, "dma enabled: %d\n->therefore scr reading finished", DMA_SCR(DMA2, DMA_STREAM3) & DMA_SxCR_EN);
	print_host_stat();
}

void read_block(uint32_t length, uint32_t sd_address)
{
	//TODO init dma dir from card
	sdio_send_cmd_blocking(17, sd_address);
	/* timeout : 100ms */
	SDIO_DTIMER = 2400000;
	SDIO_DLEN = length;	// in bytes
	SDIO_DCTRL = 	(9 << 4)| /* DATA BLOCKSIZE 2^x bytes */ //TODO waaa blocksize
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
