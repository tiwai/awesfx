/*================================================================
 * optget compatible option parser
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
#include <string.h>
#include "util.h"

static int check_error = 1;
static int argc = 0;
static char **argv = NULL;
static char *next_flag = NULL;

static int do_shortoption(char *p, char *optstr);
static int do_longoption(char *p, awe_option_args *args, int *idxp);

int awe_get_argument(int argc, char **argv, char *optstr, awe_option_args *args)
{
	int c, dummy;
	check_error = 0;
	while ((c = awe_getopt(argc, argv, optstr, args, &dummy)) != -1)
		;
	check_error = 1;
	return optind;
}

int awe_getopt(int new_argc, char **new_argv, char *optstr,
	       awe_option_args *args, int *idxp)
{
	char *p;

	if (new_argc != argc || new_argv != argv) {
		argc = new_argc;
		argv = new_argv;
		optind = 0;
	}

	if (idxp) *idxp = -1;
	optarg = NULL;

	if (next_flag && *next_flag) {
		p = next_flag;
		next_flag = NULL;
	} else {
		next_flag = NULL;
		optind++; /* skip to next */
		if (optind >= argc) {
			argc = 0;
			argv = NULL;
			return -1;
		}
		p = argv[optind];
		/* option terminated */
		if (*p != '-' || p[1] == 0) {
			argc = 0;
			argv = NULL;
			return -1;
		}
		p++;
	}

	if (*p == '-') {
		/* long option */
		if (p[1] == 0) {
		/* only '--'; leave the left arguments */
			optind++;
			argc = 0;
			argv = NULL;
			return -1;
		}
		return do_longoption(p + 1, args, idxp);
	}

	return do_shortoption(p, optstr);
}

/* short option */
static int do_shortoption(char *p, char *optstr)
{
	char *q;
	for (q = optstr; *q; q++) {
		if (*q == *p) {
			if (q[1] == ':') {
				if (p[1])
					optarg = p + 1;
				else {
					optind++;
					if (optind >= argc) {
						if (check_error)
							fprintf(stderr, "option: no argument is given for option -%c\n", *q);
						return '?';
					}
					optarg = argv[optind];
				}
			} else
				next_flag = p + 1;
			return *p;
		}
	}
	if (check_error)
		fprintf(stderr, "unknown option -%c\n", *p);
	return '?';
}

/* long option */
static int do_longoption(char *p, awe_option_args *args, int *idxp)
{
	int i;
	char *q;
	char tmp[512];

	if (args == NULL) return '?';
	if (strlen(p) > sizeof(tmp)-1) {
		if (check_error) fprintf(stderr, "too long options!\n");
		return '?';
	}
	strcpy(tmp, p);
	if ((q = strchr(tmp, '=')) != NULL)
		*q = 0;
	if ((q = strchr(p, '=')) != NULL)
		q++;
	for (i = 0; args[i].str; i++) {
		if (strcmp(args[i].str, tmp) == 0) {
			if (idxp) *idxp = i;
			if (args[i].flag)
				*args[i].flag = args[i].val;
			optarg = q;
			if (args[i].has_arg == 1 && optarg == NULL) {
				if (check_error)
					fprintf(stderr, "option: no argument is given for option --%s\n", args[i].str);
				return '?';
			}
			return args[i].val;
		}
	}
	return '?';
}
