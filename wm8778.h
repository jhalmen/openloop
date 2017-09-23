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

#ifndef LIBWM8778_H
#define LIBWM8778_H

#include <stdint.h>
/*Digital Attenuation DACL*/
/* #define LDA(val) 	((uint16_t) ((((0x03 << 1) + 1) << 8) + (uint8_t) (val))) */
uint16_t LDA(uint8_t att, uint8_t update);

/*Digital Attenutaion DACR*/
/* #define RDA(val) 	((uint16_t) ((((0x04 << 1) + 1) << 8) + (uint8_t) (val))) */
uint16_t RDA(uint8_t att, uint8_t update);

/*Master Digital Attenuation (All Channels)*/
/* #define MASTDA(val) 	((uint16_t) ((((0x05 << 1) + 1) << 8) + (uint8_t) (val))) */
uint16_t MASTDA(uint8_t att, uint8_t update);

/*Phase Swaps*/
/* #define PHASE(val)	((uint16_t) ((0x06 << 9) + ( 0x03 & (uint8_t) (val)))) */
uint16_t PHASE(uint8_t inv_left, uint8_t inv_right);

/*DAC Control*/
uint16_t DAC_C1(uint8_t DZCEN, uint8_t ATC, uint8_t IZD, uint8_t TOD, uint8_t PL);

/*DAC Mute*/
uint16_t DMUTE(uint8_t mute);

/*DAC Control*/
uint16_t DAC_C2(uint8_t DEEMP, uint8_t DZFM);

/*DAC Interface Control*/
uint16_t DAC_IC(uint8_t DACFMT, uint8_t DACLRP, uint8_t DACBCP, uint8_t DACWL);

/*ADC Interface Control*/
uint16_t ADC_IC(uint8_t ADCFMT, uint8_t ADCLRP, uint8_t ADCBCP, uint8_t ADCWL, uint8_t ADCMCLK, uint8_t ADCHPD);

/*Master Mode Control*/
uint16_t MMC(uint8_t ADCRATE, uint8_t ADCOSR, uint8_t DACRATE, uint8_t DACMS, uint8_t ADCMS);

/*PWR Down Control*/
uint16_t PWR_C(uint8_t PDWN, uint8_t ADCPD, uint8_t DACPD, uint8_t AINPD);

/*Attenuation ADCL*/
uint16_t ADCL(uint8_t LAG, uint8_t ZCLA);

/*Attenutaion ADCR*/
uint16_t ADCR(uint16_t RAG, uint8_t ZCRA);

/*ALC Control 1*/
uint16_t ALC_C1(uint8_t LCT, uint8_t MAXGAIN, uint8_t LCSEL);

/*ALC Control 2*/
uint16_t ALC_C2(uint8_t HLD, uint8_t ALCZC, uint8_t LCEN);

/*ALC Control 3*/
uint16_t ALC_C3(uint8_t ATK, uint8_t DCY);

/*Noise Gate Control*/
uint16_t NGC(uint8_t NGAT, uint8_t NGTH);

/*Limiter Control*/
uint16_t LIM_C(uint8_t MAXATTEN, uint8_t TRANWIN);

/*ADC Mux Control*/
uint16_t AMUX(uint8_t MUTERA, uint8_t MUTELA, uint8_t LRBOTH);

/*Output Mux*/
uint16_t OMUX(uint8_t MXDAC, uint8_t MXBYP);

/*Software Reset*/
uint16_t RESET(void);
#endif /* LIBWM8778_H */
