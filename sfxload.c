/*================================================================
 * sfxload -- load soundfont info onto awe sound driver
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
#include <ctype.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/fcntl.h>
#include "util.h"
#include "seq.h"
#include "awebank.h"
#include "sfopts.h"

static char *get_fontname(int argc, char **argv);
static int parse_options(int argc, char **argv);
static void add_part_list(char *arg);

extern int awe_verbose;

/*----------------------------------------------------------------
 * print usage and exit
 *----------------------------------------------------------------*/

static void usage()
{
	fprintf(stderr, "sfxload -- load SoundFont on AWE32 sound driver\n");
	fprintf(stderr, "   version 0.4.3   copyright (c) 1996-1999 by Takashi Iwai\n");
	fprintf(stderr, "usage:	sfxload [-options] [soundfont[.sf2|.sbk|.bnk]]\n");
	fprintf(stderr, "\n");

	fprintf(stderr, " options:\n");
	fprintf(stderr, " -F, --device=file        specify the device file\n");
	fprintf(stderr, " -D, --index=number       specify the device index (-1=autoprobe)\n");
	fprintf(stderr, " -i, --clear[=bool]       clear all samples\n");
	fprintf(stderr, " -x, --remove[=bool]      remove additional samples\n");
	fprintf(stderr, " -N, --increment[=bool]   incremental loading\n");
	fprintf(stderr, " -b, --bank=number        append font to the specified bank\n");
	fprintf(stderr, " -l, --lock[=bool]        lock the loading fonts\n");
	fprintf(stderr, " -d, --device=file        use the given device file\n");
	fprintf(stderr, " -C, --compat[=bool]      use v0.4.2 compatible sounds\n");
	fprintf(stderr, " -A, --sense=digit        (compat) set attenuation sensitivity (default=%g)\n", awe_option.atten_sense);
	fprintf(stderr, " -a, --atten=digit        (compat) set default attenuattion (default=%d)\n", awe_option.default_atten);
	fprintf(stderr, " -d, --decay=scale        (compat) set decay time scale (default=%g)\n", awe_option.decay_sense);
	fprintf(stderr, " -M, --memory[=bool]      display available memory on DRAM\n");
	fprintf(stderr, " -B, --addblank[=bool]    add 12 words blank loop on each sample\n");
	fprintf(stderr, " -c, --chorus=percent     set chorus effect (0-100)\n");
	fprintf(stderr, " -r, --reverb=percent     set reverb effect (0-100)\n");
	fprintf(stderr, " -V, --volume=percent     set total volume (0-100) (default=%d)\n", awe_option.default_volume);
	fprintf(stderr, " -L, --extract=preset/bank/note\n");
	fprintf(stderr, "                          do partial loading\n");
	fprintf(stderr, " -P, --path=dir           set SoundFont file search path\n");
	if (awe_option.search_path)
		fprintf(stderr, "   system default path is %s\n", awe_option.search_path);
	exit(1);
}


static int sample_mode, remove_samples, dispmem, lock_sf;
enum { CLEAR_SAMPLE, INCREMENT_SAMPLE, ADD_SAMPLE };
static char *seq_devname = NULL;
static int seq_devidx = -1;

static LoadList *part_list = NULL;

int main(int argc, char **argv)
{
	char *sffile;
	int rc;

	awe_init_option();

	sample_mode = ADD_SAMPLE;
	remove_samples = FALSE;
	dispmem = FALSE;
	lock_sf = -1;
	awe_verbose = 0;
	part_list = NULL;

	awe_read_option_file(NULL);
	if ((sffile = get_fontname(argc, argv)) != NULL) {
		awe_read_option_file(sffile);
	}

	parse_options(argc, argv);

	DEBUG(0,fprintf(stderr, "default bank = %d\n", awe_option.default_bank));
	DEBUG(0,fprintf(stderr, "default chorus = %d\n", awe_option.default_chorus));
	DEBUG(0,fprintf(stderr, "default reverb = %d\n", awe_option.default_reverb));
	if (awe_option.compatible) {
		DEBUG(0,fprintf(stderr, "v0.4.2-compatible mode\n"));
		DEBUG(0,fprintf(stderr, "minimum attenuation = %d\n", awe_option.default_atten));
		DEBUG(0,fprintf(stderr, "attenuation sense = %g\n", awe_option.atten_sense));
		DEBUG(0,fprintf(stderr, "decay sense = %g\n", awe_option.decay_sense));
	} else {
		DEBUG(0,fprintf(stderr, "use new calculation\n"));
	}

	/*----------------------------------------------------------------*/
	/* reset samples if necessary */
	seq_init(seq_devname, seq_devidx);

	/* clear or remove samples */

	if (sample_mode == ADD_SAMPLE && awe_option.default_bank < 0 && sffile)
		sample_mode = CLEAR_SAMPLE;

	if (sample_mode == CLEAR_SAMPLE)
		seq_reset_samples();
	else if (sample_mode != INCREMENT_SAMPLE && remove_samples)
		seq_remove_samples();

	if (sffile == NULL && dispmem) {
		if (dispmem)
			printf("DRAM memory left = %d kB\n", seq_mem_avail()/1024);
	}
	if (sffile == NULL) {
		seq_end();
		return 0;
	}

	/* if lock option is not specified, lockinig depends on bank mode */
	if (lock_sf < 0) {
		if (awe_option.default_bank < 0)
			lock_sf = TRUE;
		else
			lock_sf = FALSE;
	}

	rc = awe_load_bank(sffile, part_list, lock_sf);
	if (sample_mode == INCREMENT_SAMPLE && remove_samples) {
		if (rc == AWE_RET_NOMEM) {
			seq_remove_samples();
			rc = awe_load_bank(sffile, part_list, lock_sf);
		}
	}

	if (rc == AWE_RET_OK && dispmem)
		printf("DRAM memory left = %d kB\n", seq_mem_avail()/1024);

	seq_end();
	if (rc == AWE_RET_NOMEM) {
		if (awe_verbose)
			fprintf(stderr, "sfxload: no memory left\n");
		return 1;
	} else if (rc == AWE_RET_NOT_FOUND) {
		fprintf(stderr, "sfxload: can't find font file %s\n", sffile);
		return 1;
	} else if (rc == AWE_RET_ERR) {
		if (awe_verbose)
			fprintf(stderr, "sfxload: stopped by error\n");
	}

	return 0;
}


/*----------------------------------------------------------------
 * long options
 *----------------------------------------------------------------*/

static awe_option_args long_options[] = {
	{"memory", 2, 0, 'M'},
	{"remove", 2, 0, 'x'},
	{"increment", 2, 0, 'N'},
	{"clear", 2, 0, 'i'},
	{"verbose", 2, 0, 'v'},
	{"extract", 1, 0, 'L'},
	{"lock", 2, 0, 'l'},
	{"device", 1, 0, 'F'},
	{"index", 1, 0, 'D'},
	{0, 0, 0, 0},
};
static int option_index;

#define OPTION_FLAGS	"MxNivL:lF:D:"

static char *get_fontname(int argc, char **argv)
{
	int rc;
	rc = awe_get_argument(argc, argv, OPTION_FLAGS, long_options);
	if (rc < argc)
		return argv[rc];
	return NULL;
}

#define set_bool()	(optarg ? bool_val(optarg) : TRUE)

static int parse_options(int argc, char **argv)
{
	int c;
	while ((c = awe_parse_options(argc, argv, OPTION_FLAGS, long_options, &option_index)) != -1) {
		if (c == 0)
			continue;
		switch (c) {
		case 'x':
			remove_samples = set_bool();
			break;
		case 'N':
			if (set_bool())
				sample_mode = INCREMENT_SAMPLE;
			break;
		case 'i':
			if (set_bool())
				sample_mode = CLEAR_SAMPLE;
			break;
		case 'M':
			dispmem = set_bool();
			break;
		case 'v':
			if (optarg)
				awe_verbose = atoi(optarg);
			else
				awe_verbose++;
			break;
		case 'L':
			add_part_list(optarg);
			break;
		case 'l':
			lock_sf = set_bool();
			break;

		case 'F':
			seq_devname = optarg;
			break;
		case 'D':
			seq_devidx = atoi(optarg);
			break;

		case 'm':
		case 's':
		case 'I':
			break;

		default:
			usage();
			exit(1);
		}
	}
	return -1;
}

/* make a preset list from comand line options */

static void add_part_list(char *arg)
{
	char tmp[100];
	SFPatchRec pat, map;
	if (strlen(arg) > sizeof(tmp)-1) {
		fprintf(stderr, "sfxload: illegal argument %s\n", arg);
		return;
	}
	strcpy(tmp, arg);
	if (awe_parse_loadlist(tmp, &pat, &map, NULL))
		part_list = add_loadlist(part_list, &pat, &map);
}


