/*================================================================
 * sample.c
 *	convert SBK parameters and calculate loop sizes
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

#include "sffile.h"
#include "sfitem.h"
#include "util.h"

/* add blank loop for each data */
int awe_auto_add_blank = 0;

void awe_correct_samples(SFInfo *sf)
{
	int i;
	SFSampleInfo *sp;
	int prev_end;

	prev_end = 0;
	for (sp = sf->sample, i = 0; i < sf->nsamples; i++, sp++) {
		/* correct sample positions for SBK file */
		if (sf->version == 1) {
			sp->startloop++;
			sp->endloop += 2;
		}

		/* calculate sample data size */
		if (sp->sampletype & 0x8000)
			sp->size = 0;
		else if (sp->startsample < prev_end && sp->startsample != 0)
			sp->size = 0;
		else {
			sp->size = -1;
			if (!awe_auto_add_blank && i != sf->nsamples-1)
				sp->size = sp[1].startsample - sp->startsample;
			if (sp->size < 0)
				sp->size = sp->endsample - sp->startsample + 48;
		}
		prev_end = sp->endsample;

		/* calculate short-shot loop size */
		if (awe_auto_add_blank || i == sf->nsamples-1)
			sp->loopshot = 48;
		else {
			sp->loopshot = sp[1].startsample - sp->endsample;
			if (sp->loopshot < 0 || sp->loopshot > 48)
				sp->loopshot = 48;
		}
	}
}



