/*================================================================
 * setfx -- load user defined chorus / reverb mode effects
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
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#ifdef __FreeBSD__
#  include <machine/soundcard.h>
#  include <awe_voice.h>
#elif defined(linux)
#  include <linux/soundcard.h>
#  include <linux/awe_voice.h>
#endif
#include "util.h"
#include "seq.h"
#include "aweseq.h"
#include "awe_version.h"

/*----------------------------------------------------------------*/

static void usage(void);
static int getline(FILE *fp);
static int nextline(FILE *fp);
static char *gettok(FILE *fp);
static char *divtok(char *src, char *divs, int only_one);
static int htoi(char *p);

static void read_chorus(int mode, char *name, int incl, FILE *fp);
static void read_reverb(int mode, char *name, int incl, FILE *fp);

/*----------------------------------------------------------------*/

static char chorus_defined[AWE_CHORUS_NUMBERS] = {1,1,1,1,1,1,1,1,};
static char reverb_defined[AWE_CHORUS_NUMBERS] = {1,1,1,1,1,1,1,1,};

char *progname;
int verbose = 0;

/* file buffer */
static int curline;
static char line[1024];
static int connected;

/*----------------------------------------------------------------*/

static void usage(void)
{
	fprintf(stderr, "setfx -- load user defined chorus / reverb mode effects\n");
	fprintf(stderr, VERSION_NOTE);
	fprintf(stderr, "usage:	setfx config-file\n");
#ifdef DEFAULT_SF_PATH
	fprintf(stderr, "   system default path is %s\n", DEFAULT_SF_PATH);
#endif
	exit(1);
}

int main(int argc, char **argv)
{
	char *default_sf_path;
	char sfname[500];
	FILE *fp;
	int c;
	char *seq_devname = NULL;
	int seq_devidx = -1;

	/* set program name */
	progname = strrchr(argv[0], '/');
	if (progname == NULL)
		progname = argv[0];
	else
		progname++;

	while ((c = getopt(argc, argv, "F:D:")) != -1) {
		switch (c) {
		case 'F':
			seq_devname = optarg;
			break;
		case 'D':
			seq_devidx = atoi(optarg);
			break;
		default:
			usage();
			exit(1);
		}
	}

	if (optind >= argc) {
		usage();
		return 1;
	}

	/* search the config file */
	default_sf_path = getenv("SFBANKDIR");
#ifdef DEFAULT_SF_PATH
	if (default_sf_path == NULL || *default_sf_path == 0)
		default_sf_path = safe_strdup(DEFAULT_SF_PATH);
#endif

	if (! awe_search_file_name(sfname, sizeof(sfname), argv[optind], default_sf_path, NULL)) {
		fprintf(stderr, "%s: can't find such a file %s\n",
			progname, argv[optind]);
		return 1;
	}

	if ((fp = fopen(sfname, "r")) == NULL) {
		fprintf(stderr, "%s: can't open %s\n", progname, sfname);
		return 1;
	}

	curline = 0;
	if (!getline(fp))
		return 0;

	seq_init(seq_devname, seq_devidx);

	do {
		int chorus, mode, incl;
		char *tok, *name;

		/* chorus/reverb header */
		tok = divtok(line, ":", TRUE);
		if (*tok == 'c' || *tok == 'C')
			chorus = 1;
		else if (*tok == 'r' || *tok == 'R')
			chorus = 0;
		else
			continue;

		/* mode index */
		tok = divtok(NULL, ":", TRUE);
		if (tok == NULL || *tok == 0) {
			fprintf(stderr, "%s: illegal line %d\n",
				progname, curline);
			continue;
		}
		mode = atoi(tok);
		if (mode < 8 || mode >= 32) {
			fprintf(stderr, "%s: illegal mode %d in line %d\n",
				progname, mode, curline);
			continue;
		}

		/* name of the mode */
		name = divtok(NULL, ":", TRUE);
		if (name == NULL || *name == 0) {
			fprintf(stderr, "%s: illegal line %d\n",
				progname, curline);
			continue;
		}

		/* include mode index */
		tok = divtok(NULL, ":", TRUE);
		if (tok == NULL) {
			fprintf(stderr, "%s: illegal line %d\n",
				progname, curline);
			continue;
		}
		if (*tok) {
			incl = atoi(tok);
			if (incl < 0 || incl >= 31 ||
			    (chorus && !chorus_defined[incl]) ||
			    (!chorus && !reverb_defined[incl])) {
				fprintf(stderr, "%s: illegal include %d in line %d\n",
					progname, incl, curline);
				continue;
			}
		} else
			incl = -1;

		/* parse parameters */
		if (chorus)
			read_chorus(mode, name, incl, fp);
		else
			read_reverb(mode, name, incl, fp);

	} while (nextline(fp));

	fclose(fp);
	seq_end();
	return 0;
}


/*----------------------------------------------------------------
 * pre-defined chorus and reverb modes
 *----------------------------------------------------------------*/

static awe_chorus_fx_rec chorus_parm[AWE_CHORUS_NUMBERS] = {
	{0xE600, 0x03F6, 0xBC2C ,0x00000000, 0x0000006D}, /* chorus 1 */
	{0xE608, 0x031A, 0xBC6E, 0x00000000, 0x0000017C}, /* chorus 2 */
	{0xE610, 0x031A, 0xBC84, 0x00000000, 0x00000083}, /* chorus 3 */
	{0xE620, 0x0269, 0xBC6E, 0x00000000, 0x0000017C}, /* chorus 4 */
	{0xE680, 0x04D3, 0xBCA6, 0x00000000, 0x0000005B}, /* feedback */
	{0xE6E0, 0x044E, 0xBC37, 0x00000000, 0x00000026}, /* flanger */
	{0xE600, 0x0B06, 0xBC00, 0x0000E000, 0x00000083}, /* short delay */
	{0xE6C0, 0x0B06, 0xBC00, 0x0000E000, 0x00000083}, /* short delay + feedback */
};

static awe_reverb_fx_rec reverb_parm[AWE_REVERB_NUMBERS] = {
{{  /* room 1 */
	0xB488, 0xA450, 0x9550, 0x84B5, 0x383A, 0x3EB5, 0x72F4,
	0x72A4, 0x7254, 0x7204, 0x7204, 0x7204, 0x4416, 0x4516,
	0xA490, 0xA590, 0x842A, 0x852A, 0x842A, 0x852A, 0x8429,
	0x8529, 0x8429, 0x8529, 0x8428, 0x8528, 0x8428, 0x8528,
}},
{{  /* room 2 */
	0xB488, 0xA458, 0x9558, 0x84B5, 0x383A, 0x3EB5, 0x7284,
	0x7254, 0x7224, 0x7224, 0x7254, 0x7284, 0x4448, 0x4548,
	0xA440, 0xA540, 0x842A, 0x852A, 0x842A, 0x852A, 0x8429,
	0x8529, 0x8429, 0x8529, 0x8428, 0x8528, 0x8428, 0x8528,
}},
{{  /* room 3 */
	0xB488, 0xA460, 0x9560, 0x84B5, 0x383A, 0x3EB5, 0x7284,
	0x7254, 0x7224, 0x7224, 0x7254, 0x7284, 0x4416, 0x4516,
	0xA490, 0xA590, 0x842C, 0x852C, 0x842C, 0x852C, 0x842B,
	0x852B, 0x842B, 0x852B, 0x842A, 0x852A, 0x842A, 0x852A,
}},
{{  /* hall 1 */
	0xB488, 0xA470, 0x9570, 0x84B5, 0x383A, 0x3EB5, 0x7284,
	0x7254, 0x7224, 0x7224, 0x7254, 0x7284, 0x4448, 0x4548,
	0xA440, 0xA540, 0x842B, 0x852B, 0x842B, 0x852B, 0x842A,
	0x852A, 0x842A, 0x852A, 0x8429, 0x8529, 0x8429, 0x8529,
}},
{{  /* hall 2 */
	0xB488, 0xA470, 0x9570, 0x84B5, 0x383A, 0x3EB5, 0x7254,
	0x7234, 0x7224, 0x7254, 0x7264, 0x7294, 0x44C3, 0x45C3,
	0xA404, 0xA504, 0x842A, 0x852A, 0x842A, 0x852A, 0x8429,
	0x8529, 0x8429, 0x8529, 0x8428, 0x8528, 0x8428, 0x8528,
}},
{{  /* plate */
	0xB4FF, 0xA470, 0x9570, 0x84B5, 0x383A, 0x3EB5, 0x7234,
	0x7234, 0x7234, 0x7234, 0x7234, 0x7234, 0x4448, 0x4548,
	0xA440, 0xA540, 0x842A, 0x852A, 0x842A, 0x852A, 0x8429,
	0x8529, 0x8429, 0x8529, 0x8428, 0x8528, 0x8428, 0x8528,
}},
{{  /* delay */
	0xB4FF, 0xA470, 0x9500, 0x84B5, 0x333A, 0x39B5, 0x7204,
	0x7204, 0x7204, 0x7204, 0x7204, 0x72F4, 0x4400, 0x4500,
	0xA4FF, 0xA5FF, 0x8420, 0x8520, 0x8420, 0x8520, 0x8420,
	0x8520, 0x8420, 0x8520, 0x8420, 0x8520, 0x8420, 0x8520,
}},
{{  /* panning delay */
	0xB4FF, 0xA490, 0x9590, 0x8474, 0x333A, 0x39B5, 0x7204,
	0x7204, 0x7204, 0x7204, 0x7204, 0x72F4, 0x4400, 0x4500,
	0xA4FF, 0xA5FF, 0x8420, 0x8520, 0x8420, 0x8520, 0x8420,
	0x8520, 0x8420, 0x8520, 0x8420, 0x8520, 0x8420, 0x8520,
}},
};


/*----------------------------------------------------------------
 * read reverb definition
 *----------------------------------------------------------------*/

static void read_reverb(int mode, char *name, int incl, FILE *fp)
{
	char *p;
	int i, val;
	struct reverb_pat {
		awe_patch_info patch;
		awe_reverb_fx_rec v;
	} r;

	if (incl >= 0) {
		/* include from pre-defined mode */
		reverb_parm[mode] = reverb_parm[incl];
		while ((p = gettok(fp)) != NULL) {
			int src;
			char *q;
			for (q = p; *q; q++) {
				if (*q == '=') {
					*q++ = 0;
					break;
				}
			}
			if (!*q) {
				fprintf(stderr, "%s: illegal reverb definition: %s\n",
					progname, name);
				return;
			}
			src = atoi(p);
			val = htoi(q);
			if (src < 0 || src >= 28) {
				fprintf(stderr, "%s: illegal reverb parm: %s\n",
					progname, name);
				return;
			}
			reverb_parm[mode].parms[src] = val;
		}
	} else {
		/* define all 28 parameters */
		for (i = 0; i < 28; i++) {
			if ((p = gettok(fp)) == NULL) {
				fprintf(stderr, "%s: too short reverb definition: %s\n",
					progname, name);
				return;
			}
			val = htoi(p);
			reverb_parm[mode].parms[i] = val;
		}
	}
	r.patch.optarg = mode;
	r.patch.len = sizeof(awe_reverb_fx_rec);
	r.patch.type = AWE_LOAD_REVERB_FX;
	r.v = reverb_parm[mode];
	seq_load_patch(&r, sizeof(r));

	reverb_defined[mode] = 1;
}

/*----------------------------------------------------------------
 * read chorus definition
 *----------------------------------------------------------------*/

static void read_chorus(int mode, char *name, int incl, FILE *fp)
{
	char *p;
	int i, val;
	struct chorus_pat {
		awe_patch_info patch;
		awe_chorus_fx_rec v;
	} c;

	/* define all five parameters */
	for (i = 0; i < 5; i++) {
		if ((p = gettok(fp)) == NULL) {
			fprintf(stderr, "%s: illegal chorus definition: %s\n",
				progname, name);
			return;
		}
		val = htoi(p);
		switch (i) {
		case 0: chorus_parm[mode].feedback = val; break;
		case 1: chorus_parm[mode].delay_offset = val; break;
		case 2: chorus_parm[mode].lfo_depth = val; break;
		case 3: chorus_parm[mode].delay = val; break;
		case 4: chorus_parm[mode].lfo_freq = val; break;
		}
	}

	c.patch.optarg = mode;
	c.patch.len = sizeof(awe_chorus_fx_rec);
	c.patch.type = AWE_LOAD_CHORUS_FX;
	c.v = chorus_parm[mode];
	seq_load_patch(&c, sizeof(c));

	chorus_defined[mode] = 1;
}


/*----------------------------------------------------------------
 * read a line and parse tokens
 *----------------------------------------------------------------*/

static int getline(FILE *fp)
{
	char *p;
	curline++;
	connected = FALSE;
	if (fgets(line, sizeof(line), fp) == NULL)
		return FALSE;
	/* strip the linefeed and backslash at the tail */
	for (p = line; *p && *p != '\n'; p++) {
		if (*p == '\\' && p[1] == '\n') {
			*p = 0;
			connected = TRUE;
			break;
		}
	}
	*p = 0;
	return TRUE;
}

static int nextline(FILE *fp)
{
	if (connected) {
		do {
			if (! getline(fp))
				return FALSE;
		} while (connected);
		return TRUE;
	} else {
		return getline(fp);
	}
}

/* hex to integer */
static int htoi(char *p)
{
	return strtol(p, NULL, 16);
}

/* get a token separated by spaces */
static char *gettok(FILE *fp)
{
	char *tok;
	tok = divtok(NULL, " \t\r\n", FALSE);
	while (tok == NULL || *tok == 0) {
		if (! connected) return NULL;
		if (! getline(fp)) return NULL;
		tok = divtok(line, " \t\r\n", FALSE);
	}
	return tok;
}

/* divide a token with specified terminators;
 * this behaves just like strtok.  if only_one is TRUE, divtok checks only
 * one letter as a divider. */
static char *divtok(char *src, char *divs, int only_one)
{
	static char *lastp = NULL;
	char *tok;
	if (src)
		lastp = src;
	if (!only_one) {
		for (; *lastp && strchr(divs, *lastp); lastp++)
			;
	}
	tok = lastp;
	for (; *lastp && !strchr(divs, *lastp); lastp++)
		;
	
	if (lastp)
		*lastp++ = 0;
	return tok;
}



