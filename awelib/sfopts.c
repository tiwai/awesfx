/*================================================================
 * global flags
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
#include "sfopts.h"
#include "config.h"

/* master volume */
#ifndef DEFAULT_VOLUME
#define DEFAULT_VOLUME	70	/* 70% */
#endif

/* default attenuation sense */
#ifndef DEF_ATTEN_SENSE
#define DEF_ATTEN_SENSE	  10
#endif

sf_options awe_option = {
	0,	/* add blank */
	-1,	/* bank */
	0, 0,	/* chorus, reverb */
	DEFAULT_VOLUME,		/* volume */
	DEF_ATTEN_SENSE,	/* atten sense */
	32,			/* default atten */
	50.0,			/* decay sense */
	NULL,			/* search path */
};

/*----------------------------------------------------------------
 * estimate default attenuation
 *----------------------------------------------------------------*/

void awe_init_option(void)
{
	awe_option.default_atten = awe_calc_def_atten(awe_option.atten_sense);
}

int awe_calc_def_atten(double sense)
{
	if (sense <= 0) {
		fprintf(stderr, "calc_def_atten: illegal sense %g\n", sense);
		return 0;
	}
	return (int)(35.55 - 35.55/sense);
}


