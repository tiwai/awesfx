/*================================================================
 * awe_parm.c
 *	convert Emu8000 parameters
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

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "awe_parm.h"
#include "sfopts.h"

/* #define LOOKUP_TABLE */

/*================================================================
 * unit conversion
 *================================================================*/

/*
 * convert timecents to msec
 */
int awe_timecent_to_msec(int timecent)
{
	return (int)(1000 * pow(2.0, (double)timecent / 1200.0));
}


/*
 * convert msec to timecents
 */
int awe_msec_to_timecent(int msec)
{
	if (msec <= 0) msec = 1;
	return (int)(log((double)msec / 1000.0) / log(2.0) * 1200.0);
}


/*
 * convert abstract cents to mHz
 */
int awe_abscent_to_mHz(int abscents)
{
	return (int)(8176.0 * pow(2.0, (double)abscents / 1200.0));
}


/*
 * convert from mHz to abstract cents
 */
int awe_mHz_to_abscent(int mHz)
{
	return (int)(log((double)mHz / 8176.0) / log(2.0) * 1200.0);
}


/*
 * convert abstract cents to Hz
 */
int awe_abscent_to_Hz(int abscents)
{
	return (int)(8.176 * pow(2.0, (double)abscents / 1200.0));
}


/*
 * convert from Hz to abstract cents
 */
int awe_Hz_to_abscent(int Hz)
{
	return (int)(log((double)Hz / 8.176) / log(2.0) * 1200.0);
}


/*================================================================
 * Emu8000 pitch offset conversion
 *================================================================*/

/*
 * Sample pitch offset for the specified sample rate
 * rate=44100 is no offset, each 4096 is 1 octave (twice).
 * eg, when rate is 22050, this offset becomes -4096.
 */
short awe_calc_rate_offset(int Hz)
{
	if (Hz == 44100) return 0;
	return (short)(log((double)Hz / 44100) / log(2.0) * 4096.0);
}


/*================================================================
 * initialize Emu8000 parameters
 *================================================================*/

void awe_init_voice(awe_voice_info *vp)
{
	vp->sf_id = 0;
	vp->sample = 0;
	vp->start = 0;
	vp->end = 0;
	vp->loopstart = 0;
	vp->loopend = 0;
	vp->rate_offset = 0;

	vp->mode = 0;
	vp->root = 60;
	vp->tune = 0;

	vp->low = 0;
	vp->high = 127;
	vp->vellow = 0;
	vp->velhigh = 127;

	vp->fixkey = -1;
	vp->fixvel = -1;
	vp->fixpan = -1;
	vp->pan = -1;

	vp->exclusiveClass = 0;
	vp->amplitude = 127;
	vp->attenuation = 0;
	vp->scaleTuning = 100;

	awe_init_parm(&vp->parm);
}


void awe_init_parm(awe_voice_parm *pp)
{
	pp->moddelay = 0x8000;
	pp->modatkhld = 0x7f7f;
	pp->moddcysus = 0x7f7f;
	pp->modrelease = 0x807f;
	pp->modkeyhold = 0;
	pp->modkeydecay = 0;

	pp->voldelay = 0x8000;
	pp->volatkhld = 0x7f7f;
	pp->voldcysus = 0x7f7f;
	pp->volrelease = 0x807f;
	pp->volkeyhold = 0;
	pp->volkeydecay = 0;

	pp->lfo1delay = 0x8000;
	pp->lfo2delay = 0x8000;
	pp->pefe = 0;

	pp->fmmod = 0;
	pp->tremfrq = 0;
	pp->fm2frq2 = 0;

	pp->cutoff = 0xff;
	pp->filterQ = 0;

	pp->chorus = 0;
	pp->reverb = 0;
}


/*================================================================
 * Emu8000 parameters conversion
 *================================================================*/

/*
 * Delay time
 * sf: timecents
 * parm: 0x8000 - msec * 0.725
 */
static int calc_delay_adip(int amount)
{
	/* completely lost here! */
	int delay,temp;

	if (amount <= -12000) return 0x8000; /* minimum delay */ 
	delay = (amount+0x30E4)<<16;
	delay /= 1200;
	temp = 0x10000|(delay&0xffff); /* SI:BX */
	delay >>= 16;
	temp >>= 16-(delay&0xff);
	delay = 0x8000 - temp;
	if (delay < -32768) delay = -32768;
	return delay;
}

static int calc_delay_trad(int tcents)
{
	int msec = awe_timecent_to_msec(tcents);
	return (unsigned short)(0x8000 - msec * 1000 / 725);
}

unsigned short awe_calc_delay(int tcents)
{
	if (awe_option.compatible)
		return calc_delay_trad(tcents);
	else
		return calc_delay_adip(tcents);
}


#ifdef LOOKUP_TABLE
/* search an index for specified time from given time table */
static int
calc_parm_search(int msec, short *table)
{
	int left = 1, right = 127, mid;
	while (left < right) {
		mid = (left + right) / 2;
		if (msec < (int)table[mid])
			left = mid + 1;
		else
			right = mid;
	}
	return left;
}
#endif

/*
 * Attack and Hold time
 * This parameter is difficult...
 *
 * ADIP says:
 * bit15 = always 0
 * upper byte = 0x7f - hold time / 92, max 11.68sec at 0, no hold at 0x7f.
 * bit7 = always 0
 * lower byte = encoded attack time, 0 = never attack,
 *      1 = max 11.68sec, 0x7f = min 6msec.
 *
 * In VVSG, 
 *        if AttackTime_ms >= 360ms:
 *          RegisterValue = 11878/AttackTime_ms - 1
 *        if AttackTime_ms < 360ms and AttackTime != 0:
 *          RegisterValue = 32 + [16/log(2)] * log(360_ms/AttackTime_ms)
 *        if AttackTime_ms == 0
 *          RegisterValue = 0x7F
 */
/* attack & decay/release time table (msec) */

#ifdef LOOKUP_TABLE
static short attack_time_tbl[128] = {
32767, 32767, 5989, 4235, 2994, 2518, 2117, 1780, 1497, 1373, 1259, 1154, 1058, 970, 890, 816,
707, 691, 662, 634, 607, 581, 557, 533, 510, 489, 468, 448, 429, 411, 393, 377,
361, 345, 331, 317, 303, 290, 278, 266, 255, 244, 234, 224, 214, 205, 196, 188,
180, 172, 165, 158, 151, 145, 139, 133, 127, 122, 117, 112, 107, 102, 98, 94,
90, 86, 82, 79, 75, 72, 69, 66, 63, 61, 58, 56, 53, 51, 49, 47,
45, 43, 41, 39, 37, 36, 34, 33, 31, 30, 29, 28, 26, 25, 24, 23,
22, 21, 20, 19, 19, 18, 17, 16, 16, 15, 15, 14, 13, 13, 12, 12,
11, 11, 10, 10, 10, 9, 9, 8, 8, 8, 8, 7, 7, 7, 6, 0,
};
static int calc_attack_adip(int amount)
{
	int atkmsec = awe_timecent_to_msec(amount);
	return calc_parm_search(atkmsec, attack_time_tbl);
}
#else
static int calc_attack_adip(int amount)
{
	int attack,temp1,temp2;

	if (amount >= 4300)
		return 1;
	if (amount > -600) {
		attack = (amount + 500) / 150;
		temp1 = ((~attack) & 7) | 8;
		temp2 = attack >> 16;
		attack += temp2 & 7;
		attack >>= 3;
		temp1 >>= attack;
		if (temp1 < 1) temp1 = 1;
		return temp1;
	} 
	attack = (37 - amount) / 75 + 8;
	if (attack >= 128)
		attack = 127; /* 128 */
	return attack;
}
#endif

static int calc_attack_trad(int atktime)
{
	int atkmsec = awe_timecent_to_msec(atktime);
	int lw;

	if (atkmsec == 0)
		lw = 0x7f;
	else if (atkmsec >= 360)
		lw = (unsigned char)(11878 / atkmsec);
	else if (atkmsec < 360)
		lw = (unsigned char)(32 + 53.15085 * log10(360.0/atkmsec));
	else
		lw = 0x7f;
	if (lw < 1) lw = 1;
	if (lw > 127) lw = 127;

	return lw;
}

static int calc_attack(int amount)
{
	if (awe_option.compatible)
		return calc_attack_trad(amount);
	else
		return calc_attack_adip(amount);
}

static int calc_hold_adip(int amount)
{
	int hold, temp;

	if (amount < -5368) return 127; /* maximum hold */
	hold = (amount+4130)<<16;
	hold /= 1200;
	temp = 0x10000|(hold&0xffff); /* SI:BX */
	hold >>= 16;
	temp >>= 16-(hold&0xff);
	hold = 127-temp;
	if (hold < 0) hold = 0;
	return hold;
}

static int calc_hold_trad(int hldtime)
{
	int up, hldmsec = awe_timecent_to_msec(hldtime);

	up = (int)((0x7f * 92 - hldmsec) / 92);
	if (up < 1) up = 1;
	if (up > 127) up = 127;

	return up;
}

static int calc_hold(int hldtime)
{
	if (awe_option.compatible)
		return calc_hold_trad(hldtime);
	else
		return calc_hold_adip(hldtime);
}

unsigned short awe_calc_atkhld(int atktime, int hldtime)
{
	return (unsigned short)(calc_attack(atktime) | (calc_hold(hldtime) << 8));
}


/*
 * Sustain level
 * sf: centibels
 * parm: 0x7f - sustain_level(dB) * 0.75
 */
static int calc_sustain_adip(int amount)
{
	int val = 127 - (amount * 127) / 1000;
	if (val < 0) return 0;
	else if (val > 127) return 127;
	else return val;
}

static int calc_sustain_trad(int sust_cB)
{
	int up;

	/* sustain level */
	if (sust_cB <= 0)
		return 0x7f;
	else if (sust_cB >= 1000)
		return 1;
	up = (0x7f * 40 - sust_cB * 3) / 40;
	/* sustain level must be greater than zero to be audible.. */
	if (up < 1)
		up = 1;
	return up;
}

unsigned char awe_calc_sustain(int sust_cB)
{
	if (awe_option.compatible)
		return calc_sustain_trad(sust_cB);
	else
		return calc_sustain_adip(sust_cB);
}

/*
 * Modulation sustain level
 * sf: 0.1%
 * parm: 0x7f - sustain_level(dB) * 0.75
 */
static int calc_mod_sustain_adip(int amount)
{
	int val = 127 - (amount * 127) / 1000;
	if (val < 0) return 0;
	else if (val > 127) return 127;
	else return val;
}

static int calc_mod_sustain_trad(int tenth_percent)
{
	int up;

	if (tenth_percent < 0) tenth_percent = 0;
	else if (tenth_percent > 1000) tenth_percent = 1000;
	if (tenth_percent == 1000)
		up = 1;
	else {
		/*up = (int)(0x7f + 20.0 * 0.75 * log10((double)(1000 - tenth_percent) / 1000.0));*/
		up = (0x7f * 40 - tenth_percent * 3) / 40;
		if (up < 1) up = 1;
	}
	return up;
}

unsigned char awe_calc_mod_sustain(int tenth_percent)
{
	if (awe_option.compatible)
		return calc_mod_sustain_trad(tenth_percent);
	else
		return calc_mod_sustain_adip(tenth_percent);
}

/*
 * This parameter is also difficult to understand...
 *
 * ADIP says the value means decay rate, 0x7f minimum time is of 240usec/dB,
 * 0x01 being the max time of 470msec/dB, and 0 begin no decay.
 *
 * In VVSG, 2 * log(0.5) * log(23756/[ms]) (0x7F...0ms), but this is
 * obviously incorrect. But, the max time 23756 seems to be correct.
 * (actually, in NRPN control, decay time is within 0.023 and 23.7 secs.)
 * 
 */ 

#ifdef LOOKUP_TABLE
static short decay_time_tbl[128] = {
32767, 32767, 22614, 15990, 11307, 9508, 7995, 6723, 5653, 5184, 4754, 4359, 3997, 3665, 3361, 3082,
2828, 2765, 2648, 2535, 2428, 2325, 2226, 2132, 2042, 1955, 1872, 1793, 1717, 1644, 1574, 1507,
1443, 1382, 1324, 1267, 1214, 1162, 1113, 1066, 978, 936, 897, 859, 822, 787, 754, 722,
691, 662, 634, 607, 581, 557, 533, 510, 489, 468, 448, 429, 411, 393, 377, 361,
345, 331, 317, 303, 290, 278, 266, 255, 244, 234, 224, 214, 205, 196, 188, 180,
172, 165, 158, 151, 145, 139, 133, 127, 122, 117, 112, 107, 102, 98, 94, 90,
86, 82, 79, 75, 72, 69, 66, 63, 61, 58, 56, 53, 51, 49, 47, 45,
43, 41, 39, 37, 36, 34, 33, 31, 30, 29, 28, 26, 25, 24, 23, 22,
};

static int calc_decay_adip(int amount)
{
	int dcymsec = awe_timecent_to_msec(amount);
	return calc_parm_search(dcymsec, decay_time_tbl);
}
#else

static int calc_decay_adip(int amount)
{
	int decay, temp1, temp2;

	if (amount > 1800) {
		decay = (amount - 1800) / 150;
		temp1 = ((~decay) & 7) | 8;
		/* here, ASM code has high part of decay & 0x7 too,
		   but we know that this is always clear since amount
		   is between 1800 & 32767, therefore decay is 
		   0..206
		   */
		temp2 = decay >> 3;
		temp1 >>= temp2;
		decay = temp1;
		if (decay < 1) decay = 1;
		return decay;
	}

	/* here, amount < 1800 */
	decay = (37 - amount) / 75 + 39;
	if (decay >= 128)
		decay = 127;
	return decay;
}
#endif

static int calc_decay_trad(int dcytime)
{
	int dcymsec = awe_timecent_to_msec(dcytime);
	int lw;

	/* decay time */
	if (dcymsec == 0)
		lw = 127;
	else {
		/*lw = 0x7f - (int)((double)0x7f / 2.2 * log10(dcymsec / 23.04));*/
		/* it looks almost ok.. */
		lw = (int)(0x7f - awe_option.decay_sense * log10(dcymsec / 23.04));
		/* ADIP way */
		/* linear aprox; lw = 0x7f - ((dcymsec * 1000 / 100) * 0x7e) / (470000 - 240);*/
		/* log aprox; lw = (int)(0x7f - 38.2759 * log10(dcymsec / 24.0)); */
		/* same as above; lw = (int)(0x7f - 38.2759 * (dcytime * 0.30103 / 1200 + 1.61979));*/
	}
	if (lw < 1) lw = 1;
	if (lw > 127) lw = 127;
	return lw;
}

unsigned char awe_calc_decay(int dcytime)
{
	if (awe_option.compatible)
		return (unsigned char)calc_decay_trad(dcytime);
	else
		return (unsigned char)calc_decay_adip(dcytime);
}


/*
 * Cutoff frequency; return (0-255)
 * sf: abs cents (cents above 8.176Hz)
 * parm: quarter semitone; 0 = 125Hz, 0xff=8kHz?
 * (in VVS, cutoff(Hz) = value * 31.25 + 100)
 */

static int calc_cutoff_adip(int amount)
{
	int cutoff = (amount+0xf)/0x1d-0x99;
	if (cutoff < 0) return 0;
	else if (cutoff > 255) return 255;
	else return cutoff;
}

static int calc_cutoff_trad(int abscents)
{
	int cutoff;
	if (abscents == 0) /* no cutoff */
		return 0xff;

	/*cutoff = abscents / 25 - 189;*/
	cutoff = (abscents - 4721) / 25; /* 4721=125Hz */

	if (cutoff < 0) cutoff = 0;
	if (cutoff > 255) cutoff = 255;
	return cutoff;
}

unsigned char awe_calc_cutoff(int abscents)
{
	if (awe_option.compatible)
		return calc_cutoff_trad(abscents);
	else
		return calc_cutoff_adip(abscents);
}


/*
 * Initial filter Q; return (0-15)
 * sf: centibels above DC gain.
 * parm: 0 = no attenuation, 15 = 24dB
 */
static int calc_filterQ_adip(int amount)
{
	int Q = amount/12;
	if (Q < 0) return 0;
	else if (Q > 15) return 15;
	else return Q;
}

static int calc_filterQ_trad(int gain_cB)
{
	int Q;
	Q = (gain_cB * 2) / 30;
	if (Q < 0)
		Q = 0;
	else if (Q > 15)
		Q = 15;
	return (unsigned char)Q;
}

unsigned char awe_calc_filterQ(int gain_cB)
{
	if (awe_option.compatible)
		return calc_filterQ_trad(gain_cB);
	else
		return calc_filterQ_adip(gain_cB);
}

/*
 * Pitch modulation height (0-255)
 * sf: cents, 100 = 1 semitone
 * parm: signed char, 0x80 = 1 octave
 */
static int calc_pitch_shift_adip(int amount)
{
	int val = (amount * 0x1b4f) >> 16;
	if (val < -128) val = -128;
	else if (val > 127) val = 127;
	if (val < 0)
		return 0x100 + val;
	else
		return val;
}

static int calc_pitch_shift_trad(int cents)
{
	int val;
	val = (cents * 0x80) / 1200;
	if (val < -128) val = -128;
	if (val > 127) val = 127;
	if (val < 0)
		return 0x100 + val;
	else
		return val;
}

unsigned char awe_calc_pitch_shift(int cents)
{
	if (awe_option.compatible)
		return calc_pitch_shift_trad(cents);
	else
		return calc_pitch_shift_adip(cents);
}

/*
 * Filter cutoff modulation height (0-255)
 * sf: 1200 = +1 octave
 * par: 0x80 = +3(lfo1) or +6(modenv) octave
 */
static int calc_cutoff_shift_adip(int amount, int octave_shift)
{
	int val;
	if (octave_shift == 3)
		val = (amount*0x0919)>>16;
	else
		val = (amount*0x048D)>>16;
	if (val < -128) val = -128;
	if (val > 127) val = 127;
	if (val < 0)
		return (unsigned char)(0x100 + val);
	else
		return (unsigned char)val;
}		

static int calc_cutoff_shift_trad(int cents, int octave_shift)
{
	int val;
	val = (cents * 0x80) / (octave_shift * 1200);
	if (val < -128) val = -128;
	if (val > 127) val = 127;
	if (val < 0)
		return (unsigned char)(0x100 + val);
	else
		return (unsigned char)val;
}

unsigned char awe_calc_cutoff_shift(int cents, int octave_shift)
{
	if (awe_option.compatible)
		return calc_cutoff_shift_trad(cents, octave_shift);
	else
		return calc_cutoff_shift_adip(cents, octave_shift);
}

/*
 * Tremolo volume (0-255)
 * sf: cB, 10 = 1dB
 * parm: 0x7f = +/-12dB, 0x80 = -/+12dB
 */
static int calc_tremolo_adip(int amount)
{
	int val = (amount << 7) / 0x78;
	if (val < -128) val = -128;
	if (val > 127) val = 127;
	if (val < 0)
		val = 0x100 + val;
	return (unsigned char)val;
}

static int calc_tremolo_trad(int vol_cB)
{
	int val;
	val = (vol_cB * 0x80) / 120;
	if (val < -128) val = -128;
	if (val > 127) val = 127;
	if (val < 0)
		val = 0x100 + val;
	return (unsigned char)val;
}

unsigned short awe_calc_tremolo(int vol_cB)
{
	if (awe_option.compatible)
		return calc_tremolo_trad(vol_cB);
	else
		return calc_tremolo_adip(vol_cB);
}

/*
 * Envelope/LFO frequency (0-255)
 * sf: cents 
 * parm: mHz / 42 (42mHz step; 0xff=10.72Hz)
 */
static int calc_freq_adip(int amount)
{
	int freq, temp;

	if (amount <= -16000) return 0; /* minimum freq. shift */

	freq = (amount+0x23A6) << 16;
	freq /= 1200;
	temp = 0x10000|(freq&0xffff); /* DX:AX */
	freq >>= 16; /* SI:BX */
	temp >>= 16-(freq&0xff);
	if (temp > 255) temp = 255;
	return temp;
}

static int calc_freq_trad(int abscents)
{
	int mHz, val;

	mHz = awe_abscent_to_mHz(abscents);
	val = mHz / 42;
	if (val < 0) val = 0;
	if (val > 255) val = 255;
	return val;
}

unsigned char awe_calc_freq(int abscents)
{
	if (awe_option.compatible)
		return calc_freq_trad(abscents);
	else
		return calc_freq_adip(abscents);
}

/*
 * Panning position (0-127)
 * sf: (left) -500 - 500 (right) (0=center)
 * parm: (left) 0 - 127 (right), as same as MIDI parameter.
 *
 * NOTE:
 *   The value above is converted in the driver to the actual emu8000
 *   parameter, 8bit, 0 (right) - 0xff (left).
 */
char awe_calc_pan(int val)
{
	if (val < -500) return 0;
	else if (val > 500) val = 127;
	return (char)((val + 500) * 127 / 1000);
}

/*
 * Chorus strength
 * sf: 0 - 1000 (max)
 * parm: 0 - 255 (max)
 */
unsigned char awe_calc_chorus(int val)
{
	if (val < 0) return 0;
	else if (val > 1000) val = 255;
	return (unsigned char)(val * 255 / 1000);
}

/*
 * Reverb strength
 * sf: 0 - 1000 (max)
 * parm: 0 - 255 (max)
 */
unsigned char awe_calc_reverb(int val)
{
	if (val < 0) return 0;
	else if (val > 1000) val = 255;
	return (unsigned char)(val * 255 / 1000);
}

/*
 * Initial volume attenuation (0-255)
 * sf: centibels, eg. 60 = 6dB below from full scale
 * parm: dB * 8 / 3
 */
static int calc_attenuation_adip(int amount)
{
	int atten;
	atten = (amount + 12) / 24;
	atten = (atten * 8) / 3;
	if (atten > 255) atten = 255;
	if (atten < 0) atten = 0;
	return atten;
}

static int calc_attenuation_trad(int att_cB)
{
	att_cB = (int)(att_cB / awe_option.atten_sense);
	if (att_cB < 0) return 0;
	else if (att_cB > 956) return 255;
	return (unsigned char)(att_cB * 8 / 30);
}

unsigned char awe_calc_attenuation(int att_cB)
{
	if (awe_option.compatible)
		return calc_attenuation_trad(att_cB);
	else
		return calc_attenuation_adip(att_cB);
}
