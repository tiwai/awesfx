/*----------------------------------------------------------------
 * general options for AWElib
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
 *----------------------------------------------------------------*/

#ifndef SFOPTS_H_DEF
#define SFOPTS_H_DEF

#include <getopt.h>

/* options */
typedef struct _sf_options {
	int auto_add_blank;	/* bool: add 48 blank samples */
	int default_bank;	/* int: bank map number for bank #0 */
	int default_chorus;	/* int: chorus effects in percent */
	int default_reverb;	/* int: reverb effects in percent */
	int default_volume;	/* int: total volume in percent */
	double atten_sense;	/* attenuation sensitivity (default is 10.0) */
	int default_atten;	/* zero attenuation level (default is 32) */
	double decay_sense;	/* decay sensitivity (default is 50.0) */
	char *search_path;	/* search path for soundfont files */
	int compatible;		/* compatible mode */
} sf_options;

void awe_init_option(void);
int awe_calc_def_atten(double sense);	/* calculate zero atten level */

extern sf_options awe_option;

void awe_read_option_file(char *fname);
int awe_parse_options(int argc, char **argv, char *optflags,
		      struct option *long_opts, int *optidx);

#endif
