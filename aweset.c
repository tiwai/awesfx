/*================================================================
 * aweset -- set controlling parameters of awe sound driver
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
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#ifdef __FreeBSD__
#  include <machine/soundcard.h>
#  include <awe_voice.h>
#elif defined(linux)
#  include <linux/soundcard.h>
#  include <linux/awe_voice.h>
#endif
#include <getopt.h>
#include "util.h"
#include "seq.h"
#include "awe_version.h"

int verbose = 0;

/*----------------------------------------------------------------
 * prototypes
 *----------------------------------------------------------------*/

static void seq_setmode(int mode, int val);
static void seq_init_chip(void);
static void usage(void);
static int search_cmd(char *arg);
static void parse_cmd(int argc, char **argv);
static void do_cmd(int c, char *arg);
static void read_from_file(char *fname);


/*----------------------------------------------------------------
 * sequencer part
 *----------------------------------------------------------------*/

SEQ_USE_EXTBUF();

static void seq_setmode(int mode, int val)
{
	AWE_MISC_MODE(awe_dev, mode, val);
	seqbuf_dump();
}

static void seq_init_chip(void)
{
	AWE_INITIALIZE_CHIP(seqfd, awe_dev);
}


/*----------------------------------------------------------------
 * main routine
 *----------------------------------------------------------------*/

static struct option long_options[] = {
	{"help", 0, 0, 'h'},
	{"verbose", 0, 0, 'v'},
	{"file", 1, 0, 'f'},
	{"device", 1, 0, 'F'},
	{"index", 1, 0, 'D'},
	{0,0,0,0},
};

#define OPTIONS "hvf:F:D:"
static int file_passed = 0;

int main(int argc, char **argv)
{
	int c;
	char *seq_devname = NULL;
	int seq_devidx = -1;

	while ((c = getopt_long(argc, argv, OPTIONS, long_options, NULL)) != -1) {
		switch (c) {
		case 'F':
			seq_devname = optarg;
			break;
		case 'D':
			seq_devidx = atoi(optarg);
			break;
		case 'v':
			verbose = TRUE;
			break;
		case 'f':
			read_from_file(optarg);
			file_passed = 1;
			break;
		default:
			usage();
			return 1;
			break;
		}
	}

	seq_init(seq_devname, seq_devidx);
	
	if (optind < argc)
		parse_cmd(argc - optind, argv + optind);
	else if (! file_passed) {
		usage();
		return 1;
	}

	seq_end();
	return 0;
}


/*----------------------------------------------------------------
 * read commands from the given file
 *----------------------------------------------------------------*/

#define MAX_LINES	256
#define MAX_ARGS	256
#define DELIMITER	" \t\n\r"

static void read_from_file(char *fname)
{
	FILE *fp;
	char line[MAX_LINES];
	char *argv[MAX_ARGS];
	int argc;

	if ((fp = fopen(fname, "r")) == NULL) {
		fprintf(stderr, "error: can't open file '%s'\n", fname);
		exit(1);
	}

	argc = 0;
	while (fgets(line, sizeof(line), fp)) {
		char *p = strtok(line, DELIMITER);
		if (p) {
			argv[0] = p;
			argc = 1;
			while ((p = strtok(NULL, DELIMITER)) != NULL) {
				argv[argc++] = p;
				if (argc >= MAX_ARGS)
					break;
			}
			parse_cmd(argc, argv);
		}
	}

	fclose(fp);
}


/*----------------------------------------------------------------
 * command definitions
 *----------------------------------------------------------------*/

enum { CT_NONE, CT_BOOL, CT_BYTE, CT_WORD, };

typedef struct CtrlParmDefs {
	char *longcmd;
	int shortcmd;
	char *desc;
	int mode;
	int type;
	int defval;
} CtrlParmDefs;

static CtrlParmDefs ctrl_parms[] = {
	{ "init", 'i', "initialize AWE chip and state",
	  -1, CT_NONE, TRUE },
	{ "exclusive", 'e', "exclusive note mode",
	  AWE_MD_EXCLUSIVE_SOUND, CT_BOOL, TRUE },
	{ "realpan", 'p', "realtime panning change",
	  AWE_MD_REALTIME_PAN, CT_BOOL, TRUE },
	{ "gusbank", 'g', "GUS-instrument bank number",
	  AWE_MD_GUS_BANK, CT_BYTE, 0 },
	{ "keepeffect", 'k', "keep effect controls after clearing voices",
	  AWE_MD_KEEP_EFFECT, CT_BOOL, FALSE },
	{ "zeroatten", 'z', "zero attenuation level",
	  AWE_MD_ZERO_ATTEN, CT_BYTE, 32 },
	{ "chnprior", 'C', "channel priority mode",
	  AWE_MD_CHN_PRIOR, CT_BOOL, FALSE },
	{ "modsense", 'm', "modulation wheel sense",
	  AWE_MD_MOD_SENSE, CT_BYTE, 18 },
	{ "defpreset", 'P', "default preset number",
	  AWE_MD_DEF_PRESET, CT_BYTE, 0 },
	{ "defbank", 'B', "default bank number",
	  AWE_MD_DEF_BANK, CT_BYTE, 0 },
	{ "defdrum", 'D', "default drumset number",
	  AWE_MD_DEF_DRUM, CT_BYTE, 0 },
	{ "toggledrum", 'a', "accept to change bank on drum channels",
	  AWE_MD_TOGGLE_DRUM_BANK, CT_BOOL, FALSE },
	{ "newvolume", 'n', "Win-like volume calculation method",
	  AWE_MD_NEW_VOLUME_CALC, CT_BOOL, TRUE },
	{ "chorus", 'c', "chorus mode",
	  AWE_MD_CHORUS_MODE, CT_BYTE, 2 },
	{ "reverb", 'r', "reverb mode",
	  AWE_MD_REVERB_MODE, CT_BYTE, 4 },
	{ "bass", 'b', "equalizer bass level",
	  AWE_MD_BASS_LEVEL, CT_BYTE, 5 },
	{ "treble", 't', "equalizer treble level",
	  AWE_MD_TREBLE_LEVEL, CT_BYTE, 9 },
	{ "debug", 'd', "debug mode",
	  AWE_MD_DEBUG_MODE, CT_BYTE, 0 },
	{ "panexchange", 'x', "exchange panning direction",
	  AWE_MD_PAN_EXCHANGE, CT_BOOL, FALSE },
};

#define numberof(ary)	(sizeof(ary)/sizeof(ary[0]))


/*----------------------------------------------------------------
 * print usage
 *----------------------------------------------------------------*/

static void usage(void)
{
	int i;

	fprintf(stderr, "aweset -- control awedrv parameters\n");
	fprintf(stderr, VERSION_NOTE);
	fprintf(stderr, "usage: aweset [-options] command [argument] ...\n");
	fprintf(stderr, "  options:\n");
	fprintf(stderr, "    --help, -h: put this message\n");
	fprintf(stderr, "    --verbose, -v: verbose mode\n");
	fprintf(stderr, "    --file=config, -f: read commands from file\n");
	fprintf(stderr, "  commands: name (abbrev) argument : description\n");
	for (i = 0; i < numberof(ctrl_parms); i++) {
		fprintf(stderr, "    %10s (%c)",
			ctrl_parms[i].longcmd, ctrl_parms[i].shortcmd);
		switch (ctrl_parms[i].type) {
		case CT_NONE:
			fprintf(stderr, "  --- : %s\n", ctrl_parms[i].desc);
			break;
		case CT_BOOL:
			fprintf(stderr, " bool : %s (default:%s)\n",
				ctrl_parms[i].desc,
				(ctrl_parms[i].defval ? "on" : "off"));
			break;
		default:
			fprintf(stderr, " value: %s (default:%d)\n",
				ctrl_parms[i].desc, ctrl_parms[i].defval);
			break;
		}
	}
}	


/*----------------------------------------------------------------
 * search the command mathing with given string
 *----------------------------------------------------------------*/

static int search_cmd(char *arg)
{
	int i;

	if (! *arg)
		return -1;

	for (i = 0; i < numberof(ctrl_parms); i++) {
		if (strcmp(arg, ctrl_parms[i].longcmd) == 0)
			return i;
	}
	if (arg[1])
		return -1;

	for (i = 0; i < numberof(ctrl_parms); i++) {
		if (*arg == ctrl_parms[i].shortcmd)
			return i;
	}
	return -1;
}


/*----------------------------------------------------------------
 * parse command line arguments
 *----------------------------------------------------------------*/  

static void parse_cmd(int argc, char **argv)
{
	int c, cmd;
	char *p;

	for (c = 0; c < argc; c++) {
		if ((cmd = search_cmd(argv[c])) >= 0) {
			if (ctrl_parms[cmd].type != CT_NONE && c >= argc - 1) {
				fprintf(stderr, "error: no argument is given for command '%s'\n", ctrl_parms[cmd].longcmd);
				exit(1);
			}
			do_cmd(cmd, argv[c + 1]);
			if (ctrl_parms[cmd].type != CT_NONE)
				c++;
		} else if ((p = strchr(argv[c], '=')) != NULL) {
			*p = 0;
			if ((cmd = search_cmd(argv[c])) < 0) {
				fprintf(stderr, "error: invalid command '%s'\n", argv[c]);
				exit(1);
			}
			do_cmd(cmd, p + 1);
		}
	}
}


/*----------------------------------------------------------------
 * call sequencer routines
 *----------------------------------------------------------------*/

static void do_cmd(int c, char *arg)
{
	int val;
	switch (ctrl_parms[c].type) {
	case CT_NONE:
		if (ctrl_parms[c].mode == -1) {
			if (verbose)
				fprintf(stderr, "initializing AWE chip\n");
			seq_init_chip();
		}
		return;
	case CT_BOOL:
		val = bool_val(arg);
		break;
	default:
		val = (int)strtol(arg, NULL, 0);
		break;
	}
	if (verbose)
		fprintf(stderr, "set %s to %d\n", ctrl_parms[c].longcmd, val);
	seq_setmode(ctrl_parms[c].mode, val);
}
	
