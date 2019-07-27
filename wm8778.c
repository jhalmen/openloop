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

#include "wm8778.h"
/*Digital Attenuation DACL*/
uint16_t WM8778_LDA(uint8_t att, uint8_t update){
	return (0x03 << 9) | (!!update << 8) | att;
}

/*Digital Attenutaion DACR*/
uint16_t WM8778_RDA(uint8_t att, uint8_t update){
	return (0x04 << 9) | (!!update << 8) | att;
}

/*Master Digital Attenuation (All Channels)*/
uint16_t WM8778_MASTDA(uint8_t att, uint8_t update){
	return (0x05 << 9) | (!!update << 8) | att;
}

/*Phase Swaps*/
uint16_t WM8778_PHASE(uint8_t inv_left, uint8_t inv_right){
	return (0x06 << 9) | (!!inv_right << 1) | !!inv_left;
}

/*DAC Control*/
uint16_t WM8778_DAC_C1(uint8_t DZCEN, uint8_t ATC, uint8_t IZD,
		uint8_t TOD, uint8_t PL){
	return 0x07 << 9 | (PL & 0xf) << 4 | !!TOD << 3 | !!IZD << 2 |
		!!ATC << 1 | !!DZCEN;
}

/*DAC Mute*/
uint16_t WM8778_DMUTE(uint8_t mute){
	return (0x08 << 9) | !!mute;
}

/*DAC Control*/
uint16_t WM8778_DAC_C2(uint8_t DEEMP, uint8_t DZFM){
	return (0x09 << 9) | ((DZFM & 0x3) << 1) | !!DEEMP;
}

/*DAC Interface Control*/
uint16_t WM8778_DAC_IC(uint8_t DACFMT, uint8_t DACLRP,
		uint8_t DACBCP, uint8_t DACWL){
	return (0x0A << 9) | ((0x03 & DACWL) << 4) | (!!DACBCP << 3) |
		(!!DACLRP << 2) | (0x03 & DACFMT);
}

/*ADC Interface Control*/
uint16_t WM8778_ADC_IC(uint8_t ADCFMT, uint8_t ADCLRP, uint8_t ADCBCP,
		uint8_t ADCWL, uint8_t ADCMCLK, uint8_t ADCHPD){
	return (0x0B << 9) | (!!ADCHPD << 8) | (!!ADCMCLK << 6) |
		((0x03 & ADCWL) << 4) | (!!ADCBCP << 3) | (!!ADCLRP << 2) |
		(0x03 & ADCFMT);
}

/*Master Mode Control*/
uint16_t WM8778_MMC(uint8_t ADCRATE, uint8_t ADCOSR, uint8_t DACRATE,
		uint8_t DACMS, uint8_t ADCMS){
	return (0x0C << 9) | (!!ADCMS << 8) | (!!DACMS << 7) |
		((0x7 & DACRATE) << 4) | (!!ADCOSR << 3) | (0x7 & ADCRATE);
}

/*PWR Down Control*/
uint16_t WM8778_PWR_C(uint8_t PDWN, uint8_t ADCPD,
		uint8_t DACPD, uint8_t AINPD){
	return (0x0D << 9) | (!!AINPD << 6) | (!!DACPD << 2) |
		(!!ADCPD << 1) | !!PDWN;
}

/*Attenuation ADCL*/
uint16_t WM8778_ADCL(uint8_t LAG, uint8_t ZCLA){
	return (0x0E << 9) | (!!ZCLA << 8) | LAG;
}

/*Attenutaion ADCR*/
uint16_t WM8778_ADCR(uint8_t RAG, uint8_t ZCRA){
	return (0x0F << 9) | (!!ZCRA << 8) | RAG;
}

/*ALC Control 1*/
uint16_t WM8778_ALC_C1(uint8_t LCT, uint8_t MAXGAIN, uint8_t LCSEL){
	return (0x10 << 9) | ((0x03 & LCSEL) << 7) | ((0x07 & MAXGAIN) << 4) |
		(0x0F & LCT);
}

/*ALC Control 2*/
uint16_t WM8778_ALC_C2(uint8_t HLD, uint8_t ALCZC, uint8_t LCEN){
	return (0x11 << 9) | (!!LCEN << 8) | (!!ALCZC << 7) | (0x0F & HLD);
}

/*ALC Control 3*/
uint16_t WM8778_ALC_C3(uint8_t ATK, uint8_t DCY){
	return (0x12 << 9) | ((0x0F & DCY) << 4) | (0x0F & ATK);
}

/*Noise Gate Control*/
uint16_t WM8778_NGC(uint8_t NGAT, uint8_t NGTH){
	return (0x13 << 9) | ((0x07 & NGTH) << 2) | !!NGAT;
}

/*Limiter Control*/
uint16_t WM8778_LIM_C(uint8_t MAXATTEN, uint8_t TRANWIN){
	return (0x14 << 9) | ((0x07 & TRANWIN) << 4) | (0x0F & MAXATTEN);
}

/*ADC Mux Control*/
uint16_t WM8778_AMUX(uint8_t MUTERA, uint8_t MUTELA, uint8_t LRBOTH){
	return (0x15 << 9) | (!!LRBOTH << 8) | (!!MUTELA << 7) |
		(!!MUTERA << 6);
}

/*Output Mux*/
uint16_t WM8778_OMUX(uint8_t MXDAC, uint8_t MXBYP){
	return 0x16 << 9 | !!MXBYP << 2 | !!MXDAC;
}

/*Software Reset*/
uint16_t WM8778_RESET(){
	return 0x17 << 9 | 1;
}
