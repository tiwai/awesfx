/*================================================================
 * awe_parm.h  --  convert to Emu8000 native parameters
 *
 * Copyright (C) 1996-1999 Takashi Iwai
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *================================================================*/

#ifndef AWE_PARM_H_DEF
#define AWE_PARM_H_DEF

#include <awe_voice.h>

void awe_init_parm(awe_voice_parm *pp);
void awe_init_voice(awe_voice_info *vp);

int awe_timecent_to_msec(int timecent);
int awe_msec_to_timecent(int msec);
int awe_abscent_to_mHz(int abscents);
int awe_mHz_to_abscent(int mHz);
int awe_Hz_to_abscent(int Hz);
int awe_abscent_to_Hz(int abscents);

short awe_calc_rate_offset(int Hz);
unsigned short awe_calc_delay(int tcent);
unsigned short awe_calc_atkhld(int atk_tcent, int hld_tcent);
unsigned char awe_calc_sustain(int sust_cB);
unsigned char awe_calc_mod_sustain(int sust_cB);
unsigned char awe_calc_decay(int dcy_tcent);
unsigned char awe_calc_cutoff(int abscents);
unsigned char awe_calc_filterQ(int gain_cB);
unsigned char awe_calc_pitch_shift(int cents);
unsigned char awe_calc_cutoff_shift(int cents, int octave_shift);
unsigned short awe_calc_tremolo(int vol_cB);
unsigned char awe_calc_freq(int abscents);
char awe_calc_pan(int val);
unsigned char awe_calc_chorus(int val);
unsigned char awe_calc_reverb(int val);
unsigned char awe_calc_attenuation(int att_cB);

#endif
