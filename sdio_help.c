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

uint32_t getbits(uint32_t *data, uint8_t blocklen, uint16_t start, uint16_t end);
/* kinda beautiful but accesses the memory up to 32 times... */
/* uint32_t getbits(uint32_t *a, uint8_t blocklen, uint16_t lsb, uint16_t msb) */
/* { */
/* 	int bits = 8 * blocklen; */
/* 	uint32_t t; */
/* 	int i; */

/* 	t = 0; */
/* 	for (i = msb; i > lsb; i--) { */
/* 		t |= (a[((bits-1) - i)/32] >> (i % 32)) & 0x1; */
/* 		t <<= 1; */
/* 	} */
/* 	t |= (a[((bits-1) - lsb)/32] >> (lsb % 32)) & 0x1; */
/* 	return t & ((1 << (msb - lsb+1))-1); */
/* } */

/* less nice but quicker */
uint32_t getbits(uint32_t *data, uint8_t blocklen, uint16_t start, uint16_t end)
{
	int mask = (1<< (end-start+1)) - 1;
	int startword = start/32;
	int startbit = start%32;
	uint32_t word  = *(data + blocklen/4 - 1 - startword) >> startbit;
	// goes over word boundary?
	if (end / 32 - startword) {
		uint32_t hiword = *(data + blocklen/4 - 2 - startword);
		word = word | hiword << (32 - startbit);
	}
	return word & mask;
}

void getbool(uint32_t data, char *buf);
void getbuswidth(uint32_t data, char *buf);
void get_e_offset(uint32_t data, char *buf);
void get_e_timeout(uint32_t data, char *buf);
void get_au_size(uint32_t data, char *buf);
void get_performance_move(uint32_t data, char *buf);
void getclass(uint32_t data, char *buf);
void get_prot_area(uint32_t data, char *buf);
void getcardtype(uint32_t data, char *buf);
void get_e_size(uint32_t data, char *buf);

void get_csd_v(uint32_t data, char *buf);
void get_taac(uint32_t data, char *buf);
void get_nsac(uint32_t data, char *buf);
void get_tran_speed(uint32_t data, char *buf);
void get_ccc(uint32_t data, char *buf);
void get_bl_len(uint32_t data, char *buf);
void get_c_size(uint32_t data, char *buf);
void get_curr_min(uint32_t data, char *buf);
void get_curr_max(uint32_t data, char *buf);
void get_c_size_mult(uint32_t data, char *buf);
void get_sector_size(uint32_t data, char *buf);
void get_wp_grp_size(uint32_t data, char *buf);
void get_r2w_factor(uint32_t data, char *buf);
void get_scr(uint32_t data, char *buf);
void get_spec(uint32_t data, char *buf);
void get_security(uint32_t data, char *buf);
void get_busw(uint32_t data, char *buf);

struct sdslice{
	char *identifier;
	uint16_t endbit;
	uint16_t startbit;
	void (*print)(uint32_t data, char *buf);
};

struct sdslice SD_status[] = {
	{"DAT_BUS_WIDTH", 511,510, getbuswidth},
	{"SECURED_MODE", 509,509, getbool},
	{"SD_CARD_TYPE", 495,480, getcardtype},
	{"SIZE_OF_PROTECTED_AREA", 479,448, get_prot_area},
	{"SPEED_CLASS", 447,440, getclass},
	{"PERFORMANCE_MOVE", 439,432, get_performance_move},
	{"AU_SIZE", 431,428, get_au_size},
	{"ERASE_SIZE", 423,408, get_e_size},
	{"ERASE_TIMEOUT", 407,402, get_e_timeout},
	{"ERASE_OFFSET", 401,400, get_e_offset},
	{NULL,0,0,NULL}
};

struct sdslice SD_csd_v1[] = {
	{"CSD_STRUCTURE", 127,126, get_csd_v},
	{"TAAC", 119,112, get_taac},
	{"NSAC", 111,104, get_nsac},
	{"TRAN_SPEED", 103,96, get_tran_speed},
	{"CCC", 95,84, get_ccc},
	{"READ_BL_LEN", 83,80, get_bl_len},
	{"READ_BL_PARTIAL", 79,79, getbool},
	{"WRITE_BLK_MISALIGN", 78,78, getbool},
	{"READ_BLK_MISALIGN", 77,77, getbool},
	{"DSR_IMPL", 76,76, getbool},
	{"C_SIZE", 73,62, get_c_size},
	{"VDD_R_CURR_MIN", 61,59, get_curr_min},
	{"VDD_R_CURR_MAX", 58,56, get_curr_max},
	{"VDD_W_CURR_MIN", 55,53, get_curr_min},
	{"VDD_W_CURR_MAX", 52,50, get_curr_max},
	{"C_SIZE_MULT", 49,47, get_c_size_mult},
	{"ERASE_BLK_EN", 46,46, getbool},
	{"SECTOR_SIZE", 45,39, get_sector_size},
	{"WP_GRP_SIZE", 38,32, get_wp_grp_size},
	{"WP_GRP_ENABLE", 31,31, getbool},
	{"R2W_FACTOR", 28,26, get_r2w_factor},
	{"WRITE_BL_LEN", 25,22, get_bl_len},
	{"WRITE_BL_PARTIAL", 21,21, getbool},
	{"FILE_FORMAT_GRP", 15,15, getbool},
	{"COPY", 14,14, getbool},
	{"PERM_WRITE_PROTECT", 13,13, getbool},
	{"TMP_WRITE_PROTECT", 12,12, getbool},
//	{"FILE_FORMAT", 11,10,func},
	{NULL, 0,0, NULL}
};

struct sdslice SD_csd_v2[] = {
	{"CSD_STRUCTURE", 127,126, get_csd_v},
	{"TAAC", 119,112, get_taac},
	{"NSAC", 111,104, get_nsac},
	{"TRAN_SPEED", 103,96, get_tran_speed},
	{"CCC", 95,84, get_ccc},
	{"READ_BL_LEN", 83,80, get_bl_len},
	{"READ_BL_PARTIAL", 79,79, getbool},
	{"WRITE_BLK_MISALIGN", 78,78, getbool},
	{"READ_BLK_MISALIGN", 77,77, getbool},
	{"DSR_IMPL", 76,76, getbool},
	{"C_SIZE", 69,48, get_c_size},
	{"ERASE_BLK_EN", 46,46, getbool},
	{"SECTOR_SIZE", 45,39, get_sector_size},
	{"WP_GRP_SIZE", 38,32, get_wp_grp_size},
	{"WP_GRP_ENABLE", 31,31, getbool},
	{"R2W_FACTOR", 28,26, get_r2w_factor},
	{"WRITE_BL_LEN", 25,22, get_bl_len},
	{"WRITE_BL_PARTIAL", 21,21, getbool},
	{"FILE_FORMAT_GRP", 15,15, getbool},
	{"COPY", 14,14, getbool},
	{"PERM_WRITE_PROTECT", 13,13, getbool},
	{"TMP_WRITE_PROTECT", 12,12, getbool},
//	{"FILE_FORMAT", 11,10,func},
	{NULL, 0,0, NULL}
};

struct sdslice SD_scr[]={
	{"SCR_STRUCTURE", 63,60, get_scr},
	{"SD_SPEC", 59,56, get_spec},
	{"DATA_STAT_AFTER_ERASE", 55,55, getbool},
	{"SD_SECURITY", 54,52, get_security},
	{"SD_BUS_WIDTHS", 51,48, get_busw},
	{NULL, 0,0, NULL}
};

void get_scr(uint32_t data, char *buf)
{
	snprintf(buf, 16, "%s", data?"reserved":"Version 1.01-2.00");
}
void get_spec(uint32_t data, char *buf)
{
	switch (data) {
	case 0:
		snprintf(buf, 16, "Version 1.0-.01");
		break;
	case 1:
		snprintf(buf, 16, "Version 1.10");
		break;
	case 2:
		snprintf(buf, 16, "Version 2.00");
		break;
	default:
		snprintf(buf, 16, "reserved");
	}
}

void get_security(uint32_t data, char *buf)
{
	switch (data) {
	case 0:
		snprintf(buf, 16, "no security");
		break;
	case 1:
		snprintf(buf, 16, "not used");
		break;
	case 2:
		snprintf(buf, 16, "Version 1.01");
		break;
	case 3:
		snprintf(buf, 16, "Version 2.00");
		break;
	default:
		snprintf(buf, 16, "reserved");
	}
}

void get_busw(uint32_t data, char *buf)
{
	snprintf(buf, 16, "%ld= 0br4r1", data);
}

void getbool(uint32_t data, char *buf)
{
	snprintf(buf, 5, "%s", data ? "YES":"NO");
}

void getbuswidth(uint32_t data, char *buf)
{
	switch (data) {
	case 0:
		snprintf(buf, 5, "1bit");
		break;
	case 1:
		snprintf(buf, 5, "BAD");
		break;
	case 2:
		snprintf(buf, 5, "4bit");
		break;
	case 3:
		snprintf(buf, 5, "BAD");
		break;
	}
}

void getcardtype(uint32_t data, char *buf)
{
	switch (data) {
	case 0:
		snprintf(buf, 16, "Reg. SD RD/WR");
		break;
	case 1:
		snprintf(buf, 16, "SD ROM Card");
		break;
	default:
		snprintf(buf, 16, "BAD:%lx", data);
		break;
	}
}

void get_prot_area(uint32_t data, char *buf)
{
	if (sdcard.high_capacity) {
		snprintf(buf, 16, "%luB", data);
	} else {
		snprintf(buf, 16, "TODO: implement");
	}
}

void getclass(uint32_t data, char *buf)
{
	if (data > 3 && 0) {
		snprintf(buf, 9, "RESERVED");
	} else {
		snprintf(buf, 4, "%ld", 2 * data);
	}
}

void get_performance_move(uint32_t data, char *buf)
{
	if (!data) {
		snprintf(buf, 4, "N/A");
	} else if (data == 0xFF) {
		snprintf(buf, 4, "INF");
	} else {
		snprintf(buf, 7, "%ldM/s", data);
	}
}

void get_au_size(uint32_t data, char *buf)
{
	if (!data) {
		snprintf(buf, 4, "N/A");
	} else if (data > 10) {
		snprintf(buf, 5, "RSVD");
	} else if (data < 7) {
		snprintf(buf, 5, "%ldK", data * 16);
	} else {
		snprintf(buf, 5, "%ldM", data - 6);
	}
	if (data == 9)
		snprintf(buf, 5, "%ldM", data - 5);
}

void get_e_size(uint32_t data, char *buf)
{
	if (data) {
		snprintf(buf, 8, "%ldAU", data);
	} else {
		snprintf(buf, 4, "N/A");
	}
}

void get_e_timeout(uint32_t data, char *buf)
{
	if (data) {
		snprintf(buf, 6, "%ldsec", data);
	} else {
		snprintf(buf, 4, "N/A");
	}

}

void get_e_offset(uint32_t data, char *buf)
{
	snprintf(buf, 5, "%ldsec",data);
}

void get_taac(uint32_t data, char *buf)
{
	int unit = data & 7;
	int val = data >> 3 & 0xF;
	float tv[] = {0,1.0,1.2,1.3,1.5,2.0,2.5,3.0,
		3.5,4.0,4.5,5.0,5.5,6.0,7.0,8.0};
	uint32_t tmp = 10;
	for (int i = 0; i < unit; ++i)
		tmp *= 10;
	sdcard.taac = tv[val]*tmp;
	snprintf(buf, 16, "%.1f*10^%dns",tv[val],unit);
}

void get_nsac(uint32_t data, char *buf)
{
	sdcard.nsac = data * 100;
	snprintf(buf, 16, "%ldclks", data*100);
}

void get_tran_speed(uint32_t data, char *buf)
{
	int unit = data & 7;
	int val = data >> 3 & 0xF;
	float tv[] = {0,1.0,1.2,1.3,1.5,2.0,2.5,3.0,
		3.5,4.0,4.5,5.0,5.5,6.0,7.0,8.0};
	snprintf(buf, 16, "%.1f*10^%dkb/s",tv[val],unit);
}

void get_csd_v(uint32_t data, char *buf)
{
	if (data > 1) {
		snprintf(buf, 16, "RESERVED: %ld", data);
	} else {
		snprintf(buf, 2, "%ld", data+1);
	}
}

void get_ccc(uint32_t data, char *buf)
{
	for (int i = 0; i < 12; ++i) {
		if (data & 1 << i) {
			buf[11 - i] = 'x';
		} else {
			buf[11 - i] = '-';
		}
	}
}

void get_bl_len(uint32_t data, char *buf)
{
	if (data < 9 || data > 11) {
		snprintf(buf, 16, "BAD: %ld", data);
	} else {
		int tmp = 1;
		for (unsigned int i = 0; i < data; ++i) {
			tmp *= 2;
		}
		sdcard.block_len = tmp;
		snprintf(buf, 6, "2^%ldB", data);
	}
}

void calculate_memory_capacity(void);
void calculate_memory_capacity(void)
{
	if (sdcard.high_capacity) {
		sdcard.memcap = (sdcard.c_size + 1) * 512;
	} else {
		sdcard.memcap = (sdcard.c_size + 1) * sdcard.mult * sdcard.block_len;
	}
}

void get_c_size(uint32_t data, char *buf)
{
	sdcard.c_size = data;
	snprintf(buf, 16, "%ld", data);
}

void get_curr_min(uint32_t data, char *buf)
{
	float c[] = {0.5, 1, 5, 10, 25, 35, 60, 100};
	snprintf(buf, 7, "%.1fmA", c[data]);
}

void get_curr_max(uint32_t data, char *buf)
{
	uint8_t c[] = {1, 5, 10, 25, 35, 45, 80, 200};
	snprintf(buf, 6, "%dmA", c[data]);
}

void get_c_size_mult(uint32_t data, char *buf)
{
	int tmp = 1;
	for (unsigned int i = 0; i < data + 2; ++i)
		tmp *= 2;
	sdcard.mult = tmp;
	snprintf(buf, 4, "%d", tmp);

}

void get_sector_size(uint32_t data, char *buf)
{
	sdcard.sector_size = data + 1;
	snprintf(buf, 4, "%ld", data + 1);
}

void get_wp_grp_size(uint32_t data, char *buf)
{
	snprintf(buf, 4, "%ld", data +1);
}

void get_r2w_factor(uint32_t data, char *buf)
{
	if (data > 5) {
		snprintf(buf, 16, "BAD: %ld", data);
	} else {
		int t = 1;
		for (unsigned int i = 0; i < data; ++i)
			t *= 2;
		snprintf(buf, 4, "%d", t);
	}
}

void printsdslice(uint32_t *data, struct sdslice *slice, uint8_t blocklen, char *message);
void printsdslice(uint32_t *data, struct sdslice *slice, uint8_t blocklen, char *message)
{
	int i = 0;
	dprintf(2, "%s\n", message);
	do {
		uint32_t bits = getbits(data, blocklen, slice[i].startbit, slice[i].endbit);
		char buf[16] = {0};
		slice[i].print(bits,buf);
		dprintf(2,"%s: %s\n", slice[i].identifier, buf);
	} while (slice[++i].identifier);
}
void printsdstatus(void);
inline void printsdstatus(void)
{
	printsdslice(sdcard.sdstatus, SD_status,
			64, "#######  SD_STATUS  #######");
}
void parse_csd(void);
inline void parse_csd(void)
{
	printsdslice(sdcard.csd, sdcard.high_capacity ? SD_csd_v2 : SD_csd_v1,
			16, "#######     CSD     #######");
	calculate_memory_capacity();
}
void printscr(void);
inline void printscr(void)
{
	printsdslice (sdcard.scr, SD_scr, 8, "#######     SCR     #######");
}

