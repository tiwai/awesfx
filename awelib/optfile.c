/*----------------------------------------------------------------
 * parse options
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "util.h"
#include "sfopts.h"
#include "config.h"


#define SYSTEM_RCFILE		"/etc/sfxloadrc"
#define RCFILE			".sfxloadrc"

#define DEFAULT_ID	"default"

typedef struct OptionFile {
	int argc;
	char **argv;
	struct OptionFile *next;
} OptionFile;

static OptionFile *optlist = NULL;

#define MAX_ARGC	100

static void read_option_file(void)
{
	char *p, rcfile[256];
	char line[256];
	FILE *fp;
	OptionFile *rec;

	*rcfile = 0;
	if ((p = getenv("HOME")) != NULL && *p) {
		snprintf(rcfile, sizeof(rcfile), "%s/%s", p, RCFILE);
		if (access(rcfile, R_OK) != 0)
			rcfile[0] = 0;
	}
	if (! *rcfile) {
#ifdef SYSTEM_RCFILE
		strcpy(rcfile, SYSTEM_RCFILE);
		if (access(rcfile, R_OK) != 0)
			return;
#else
		return;
#endif
	}

	optlist = NULL;

	if ((fp = fopen(rcfile, "r")) == NULL)
		return;

	while (fgets(line, sizeof(line), fp)) {
		char *argv[MAX_ARGC];
		int i, argc;
		if ((argv[0] = strtok(line, " \t\n")) == NULL) continue;
		if (*argv[0] == '#') continue; /* skip comments */
		for (argc = 1; argc < MAX_ARGC; argc++) {
			argv[argc] = strtok(NULL,  " \t\n");
			if (argv[argc] == NULL)
				break;
		}
		rec = (OptionFile*)safe_malloc(sizeof(OptionFile));
		rec->argv = (char**)safe_malloc(sizeof(char*) * argc);
		rec->argc = argc;
		for (i = 0; i < argc; i++)
			rec->argv[i] = safe_strdup(argv[i]);
		rec->next = optlist;
		optlist = rec;
	}

	fclose (fp);
}

static void parse_named_option(char *fname)
{
	OptionFile *p;
	for (p = optlist; p; p = p->next) {
		int optind_save = optind;
		optind = 0;
		if (strcmp(p->argv[0], fname) == 0) {
			while (awe_parse_options(p->argc, p->argv, 0, 0, 0) == 0)
				;
		}
		optind = optind_save;
	}
}

void awe_read_option_file(char *fname)
{
	char tmp[256], *base, *ep;

	if (optlist == NULL) {
		read_option_file();
		if (optlist == NULL)
			return;
	}

	parse_named_option(DEFAULT_ID);
	if (fname) {
		strncpy(tmp, fname, sizeof(tmp));
		fname[sizeof(tmp) - 1] = 0;
		if ((base = strrchr(tmp, '/')) == NULL)
			base = tmp; /* no directory path is attached */
		else
			base++; /* next of slash letter */
		/* remove extension */
		if ((ep = strchr(base, '.')) != NULL)
			*ep = 0;
		if (strcmp(base, DEFAULT_ID) != 0)
			parse_named_option(base);
	}
}

#define DEFAULT_OPTION_NUM	10

static struct option long_options[40] = {
	{"addblank", 2, 0, 'B'},
	{"bank", 2, 0, 'b'},
	{"chorus", 1, 0, 'c'},
	{"reverb", 1, 0, 'r'},
	{"path", 1, 0, 'P'},
	{"sense", 1, 0, 'A'},
	{"atten", 1, 0, 'a'},
	{"decay", 1, 0, 'd'},
	{"volume", 1, 0, 'V'},
	{"compat", 2, 0, 'C'},
};
#define OPTION_FLAGS	"b:c:r:P:A:a:d:V:BC"

#define set_bool()	(optarg ? bool_val(optarg) : TRUE)

int awe_parse_options(int argc, char **argv, char *optflags,
		      struct option *long_opts, int *optidx)
{
	int c;
	int ival;
	double dval;
	static char options[100];

	if (optflags) {
		if (strlen(OPTION_FLAGS) + strlen(optflags) >= sizeof(options))
			return -1;
	}
	strcpy(options, OPTION_FLAGS);
	if (optflags)
		strcat(options, optflags);
	c = DEFAULT_OPTION_NUM;
	if (long_opts) {
		struct option *p;
		for (p = long_opts; p->name; p++) {
			if (c >= numberof(long_options))
				return -1;
			long_options[c++] = *p;
		}
	}
	long_options[c].name = 0;

	if ((c = getopt_long(argc, argv, options, long_options, optidx)) == -1)
		return -1;

	switch (c) {
	case 'b':
		ival = atoi(optarg);
		if (ival > 127)
			fprintf(stderr, "awe: illegal bank number %d\n", ival);
		else
			awe_option.default_bank = ival;
		break;
	case 'B':
		awe_option.auto_add_blank = set_bool();
		break;
	case 'C':
		awe_option.compatible = set_bool();
		break;
	case 'V':
		ival = atoi(optarg);
		if (ival < 0 || ival > 100)
			fprintf(stderr, "awe: illegal default volume value %d\n", ival);
		else
			awe_option.default_volume = ival;
		break;
	case 'c':
		ival = atoi(optarg);
		if (ival < 0 || ival > 100)
			fprintf(stderr, "awe: illegal default chorus value %d\n", ival);
		else
			awe_option.default_chorus = ival;
		break;
	case 'r':
		ival = atoi(optarg);
		if (ival < 0 || ival > 100)
			fprintf(stderr, "awe: illegal default reverb value %d\n", ival);
		else
			awe_option.default_reverb = ival;
		break;
	case 'P':
		if (awe_option.search_path)
			free(awe_option.search_path);

		awe_option.search_path = safe_strdup(optarg);
		break;
	case 'A':
		dval = atof(optarg);
		if (dval <= 0)
			fprintf(stderr, "awe: illegal atten sense parameter %g\n", dval);
		else {
			awe_option.atten_sense = dval;
			awe_option.default_atten = awe_calc_def_atten(dval);
		}
		break;
	case 'a':
		ival = atoi(optarg);
		if (ival < 0 || ival > 255)
			fprintf(stderr, "awe: illegal attenuation parameter %d\n", ival);
		else
			awe_option.default_atten = ival;
		break;
	case 'd':
		awe_option.decay_sense = atof(optarg);
		break;

	default:
		return c;
	}

	return 0;
}
