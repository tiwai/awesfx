/*================================================================
 * seq.h
 *	sequencer control routine
 *
 * Copyright (C) 1996-2003 Takashi Iwai
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

#ifndef SEQ_H_DEF
#define SEQ_H_DEF

extern int seqfd;
/*extern int nrsynths;*/
extern int awe_dev;
/*extern int max_synth_voices;*/

#define OSS_SEQUENCER_DEV	"/dev/sequencer"

void seq_init(char *devname, int devidx);
int seq_reset_samples(void);
void seq_end(void);
void seq_default_atten(int val);
int seq_zero_atten(int val);
int seq_remove_samples(void);
void seq_initialize_chip(void);
void seq_set_gus_bank(int bank);
int seq_load_patch(void *patch, int len);
int seq_mem_avail(void);

/* alsa.c */
void seq_alsa_init(char *hwdep);
void seq_alsa_end(void);

#endif
