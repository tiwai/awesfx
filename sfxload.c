/*================================================================
 * sfxload -- load soundfont info onto awe sound driver
 *
 * Copyright (C) 1996-2000 Takashi Iwai
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
#include <util.h>
#ifdef BUILD_ASFXLOAD
#include <alsa/asoundlib.h>
#endif
#include <awebank.h>
#include <sfopts.h>
#include <awe_version.h>
#include "seq.h"

#ifdef BUILD_ASFXLOAD
#define PROGNAME "asfxload"
#else
#define PROGNAME "sfxload"
#endif

static char *get_fontname(int argc, char **argv);
static int parse_options(int argc, char **argv);
static void add_part_list(char *arg);

extern int awe_verbose;

static AWEOps load_ops = {
	seq_load_patch,
	seq_mem_avail,
	seq_reset_samples,
	seq_remove_samples,
	seq_zero_atten
};



/*----------------------------------------------------------------
 * print usage and exit
 *----------------------------------------------------------------*/

static void usage()
{
	fputs(
#ifdef BUILD_ASFXLOAD
	      "asfxload -- load SoundFont on ALSA Emux WaveTable\n"
#else
	      "sfxload -- load SoundFont on OSS AWE32 sound driver\n"
#endif
	      VERSION_NOTE
	      "usage:	" PROGNAME " [-options] [soundfont[.sf2|.sbk|.bnk]]\n"
	      "\n"
	      " options:\n"
#ifdef BUILD_ASFXLOAD
	      " -D, --hwdep=name        specify the hwdep name\n"
#else
	      " -F, --device=file        specify the device file\n"
	      " -D, --index=number       specify the device index (-1=autoprobe)\n"
#endif
	      " -i, --clear[=bool]       clear all samples\n"
	      " -x, --remove[=bool]      remove additional samples\n"
	      " -N, --increment[=bool]   incremental loading\n"
	      " -b, --bank=number        append font to the specified bank\n"
	      " -l, --lock[=bool]        lock the loading fonts\n"
	      " -C, --compat[=bool]      use v0.4.2 compatible sounds\n",
	      stderr);
	fprintf(stderr, " -A, --sense=digit        (compat) set attenuation sensitivity (default=%g)\n", awe_option.atten_sense);
	fprintf(stderr, " -a, --atten=digit        (compat) set default attenuattion (default=%d)\n", awe_option.default_atten);
	fprintf(stderr, " -d, --decay=scale        (compat) set decay time scale (default=%g)\n", awe_option.decay_sense);
	fputs(" -M, --memory[=bool]      display available memory on DRAM\n"
	      " -B, --addblank[=bool]    add 12 words blank loop on each sample\n"
	      " -c, --chorus=percent     set chorus effect (0-100)\n"
	      " -r, --reverb=percent     set reverb effect (0-100)\n",
	      stderr);
	fprintf(stderr, " -V, --volume=percent     set total volume (0-100) (default=%d)\n", awe_option.default_volume);
	fputs(" -L, --extract=preset/bank/note\n"
	      "                          do partial loading\n"
	      " -P, --path=dir           set SoundFont file search path\n",
	      stderr);
	if (awe_option.search_path)
		fprintf(stderr, "   system default path is %s\n", awe_option.search_path);
	exit(1);
}


static int sample_mode, remove_samples, dispmem, lock_sf;
enum { CLEAR_SAMPLE, INCREMENT_SAMPLE, ADD_SAMPLE };
#ifdef BUILD_ASFXLOAD
static char *hwdep_name = NULL;
#else
static char *seq_devname = NULL;
static int seq_devidx = -1;
#endif

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
#ifdef BUILD_ASFXLOAD
	seq_alsa_init(hwdep_name);
#else
	/* reset samples if necessary */
	seq_init(seq_devname, seq_devidx);
#endif

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
#ifdef BUILD_ASFXLOAD
		seq_alsa_end();
#else
		seq_end();
#endif
		return 0;
	}

	/* if lock option is not specified, lockinig depends on bank mode */
	if (lock_sf < 0) {
		if (awe_option.default_bank < 0)
			lock_sf = TRUE;
		else
			lock_sf = FALSE;
	}

	rc = awe_load_bank(&load_ops, sffile, part_list, lock_sf);
	if (sample_mode == INCREMENT_SAMPLE && remove_samples) {
		if (rc == AWE_RET_NOMEM) {
			seq_remove_samples();
			rc = awe_load_bank(&load_ops, sffile, part_list, lock_sf);
		}
	}

	if (rc == AWE_RET_OK && dispmem)
		printf("DRAM memory left = %d kB\n", seq_mem_avail()/1024);

#ifdef BUILD_ASFXLOAD
		seq_alsa_end();
#else
	seq_end();
#endif
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

static struct option long_options[] = {
	{"memory", 2, 0, 'M'},
	{"remove", 2, 0, 'x'},
	{"increment", 2, 0, 'N'},
	{"clear", 2, 0, 'i'},
	{"verbose", 2, 0, 'v'},
	{"extract", 1, 0, 'L'},
	{"lock", 2, 0, 'l'},
#ifdef BUILD_ASFXLOAD
	{"hwdep", 1, 0, 'D'},
#else
	{"device", 1, 0, 'F'},
	{"index", 1, 0, 'D'},
#endif
	{0, 0, 0, 0},
};
static int option_index;

#ifdef BUILD_ASFXLOAD
#define OPTION_FLAGS	"MxNivL:lD:"
#else
#define OPTION_FLAGS	"MxNivL:lF:D:"
#endif

int awe_get_argument(int argc, char **argv, char *optstr, struct option *args)
{
	int c, dummy;
	int optind_saved;
	optind_saved = optind;
	optind = 0;
	while ((c = getopt_long(argc, argv, optstr, args, &dummy)) != -1)
		;
	c = optind;
	optind = optind_saved;
	return c;
}

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

#ifdef BUILD_ASFXLOAD
		case 'D':
			hwdep_name = optarg;
			break;
#else
		case 'F':
			seq_devname = optarg;
			break;
		case 'D':
			seq_devidx = atoi(optarg);
			break;
#endif
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
		part_list = awe_add_loadlist(part_list, &pat, &map);
}


