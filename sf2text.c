/*================================================================
 * sf2text -- print soundfont layer information
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
#include <string.h>
#include "sffile.h"
#include "sfitem.h"
#include "sflayer.h"
#include "awe_parm.h"
#include "util.h"

int seqfd, awe_dev;
static SFInfo sfinfo;

void print_soundfont(FILE *fp, SFInfo *sf);
static void print_name(FILE *fp, char *str);
static void print_layers(FILE *fp, SFInfo *sf, SFHeader *hdr);
static void print_amount(FILE *fp, SFInfo *sf, SFGenRec *gen);


int main(int argc, char  **argv)
{
	FILE *fd, *fout;
	int piped=0;
	if (argc < 2 || strcmp(argv[1],"-") == 0) {
		piped = 1;
		fd = stdin;
	} else {
		if (strcmp(argv[1], "-h") ==0 || strcmp(argv[1], "--help") == 0) {
			fprintf(stderr, "no file is given\n");
			fprintf(stderr, "usage: sf2text soundfont [outputfile]\n");
			return 1;
		}
		if ((fd = fopen(argv[1], "r")) == NULL) {
			fprintf(stderr, "can't open file %s\n", argv[1]);
			return 1;
		}
	}
	if (awe_load_soundfont(&sfinfo, fd, !piped) < 0)
		return 1;
	fclose(fd);

	if (argc < 3 || strcmp(argv[2], "-"))
		fout = stdout;
	else {
		if ((fout = fopen(argv[2], "w")) == NULL) {
			fprintf(stderr, "can't open file %s\n", argv[2]);
			return 1;
		}
	}
	print_soundfont(fout, &sfinfo);
	return 0;
}


void print_soundfont(FILE *fp, SFInfo *sf)
{
	int i;
	SFPresetHdr *preset;
	SFInstHdr *inst;
	SFSampleInfo *sp;

	fprintf(fp, "(Name ");
	print_name(fp, sf->sf_name);
	fprintf(fp, ")\n");

	fprintf(fp, "(SoundFont %d %d)\n", sf->version, sf->minorversion);
	fprintf(fp, "(SamplePos %ld %d)\n", sf->samplepos, sf->samplesize);
	fprintf(fp, "(InfoPos %ld %ld)\n", sf->infopos, sf->infosize);
	fprintf(fp, "(Presets %d (\n", sf->npresets);
	for (preset = sf->preset, i = 0; i < sf->npresets; preset++, i++) {
		fprintf(fp, " (");
		fprintf(fp, "%d ", i);
		print_name(fp, preset->hdr.name);
		fprintf(fp, " (preset %d) (bank %d) (\n",
		       preset->preset, preset->bank);
		print_layers(fp, sf, &preset->hdr);
		fprintf(fp, "  ))\n");
	}
	fprintf(fp, " ))\n");
	
	fprintf(fp, "(Instruments %d (\n", sf->ninsts);
	for (inst = sf->inst, i = 0; i < sf->ninsts; inst++, i++) {
		fprintf(fp, "  (");
		fprintf(fp, "%d ", i);
		print_name(fp, inst->hdr.name);
		fprintf(fp, " (\n");
		print_layers(fp, sf, &inst->hdr);
		fprintf(fp, "  ))\n");
	}
	fprintf(fp, " ))\n");
	
	fprintf(fp, "(SampleInfo %d (\n", sf->nsamples);
	for (sp = sf->sample, i = 0; i < sf->nsamples; sp++, i++) {
		fprintf(fp, " (%d ", i);
		print_name(fp, sp->name);
		fprintf(fp, " (0x%x 0x%x) (0x%x 0x%x)",
		       sp->startsample, sp->endsample,
		       sp->startloop, sp->endloop);
		if (sf->version == 2) {
			fprintf(fp, "\n          (%d %d %d %d %d)",
			       sp->samplerate, sp->originalPitch,
			       sp->pitchCorrection, sp->samplelink,
			       sp->sampletype);
		}
		fprintf(fp, ")\n");
	}
	fprintf(fp, " ))\n");
}

/* print string value. escape or convert the letter to octet if necessary. */
static void print_name(FILE *fp, char *str)
{
	unsigned char *p;
	putc('"', fp);
	int i = 0;
	for (p = str; *p && i < 20; i++, p++) {
		if (!isprint(*p))
			fprintf(fp, "\\%03o", *p);
		else if (*p == '"')
			fprintf(fp, "\\\"");
		else
			putc(*p, fp);
	}
	
	putc('"', fp);
}

/* print layered list */
static void print_layers(FILE *fp, SFInfo *sf, SFHeader *hdr)
{
	SFGenLayer *lay = hdr->layer;
	int j, k;

	for (j = 0; j < hdr->nlayers; lay++, j++) {
		if (lay->nlists == 0) continue;
		fprintf(fp, "  (layer\n");
		for (k = 0; k < lay->nlists; k++) {
			print_amount(fp, sf, &lay->list[k]);
			if (k == lay->nlists-1)
				fprintf(fp, ")\n");
			else
				fprintf(fp, "\n");
		}
	}
}

/* print a layer item together with optional info */
static void print_amount(FILE *fp, SFInfo *sf, SFGenRec *gen)
{
	LayerItem *item = &layer_items[gen->oper];
	int amount;

	fprintf(fp, "   (%s %d", sf_gen_text[gen->oper], gen->amount);

	if (sf->version == 1)
		amount = sbk_to_sf2(gen->oper, gen->amount);
	else
		amount = gen->amount;

	switch (item->type) {
	case T_NOP:
	case T_NOCONV:
	case T_OFFSET:
	case T_HI_OFF:
	case T_SCALE:
		fprintf(fp, " ");
		if (gen->oper == SF_sampleId)
			print_name(fp, sf->sample[amount].name);
		else if (gen->oper == SF_instrument)
			print_name(fp, sf->inst[amount].hdr.name);
		else 
			fprintf(fp, "%d", amount);
		break;
	case T_RANGE:
		fprintf(fp, " (%d %d)", LOWNUM(amount), HIGHNUM(amount));
		break;
	case T_FILTERQ:
	case T_ATTEN:
	case T_TREMOLO:
	case T_VOLSUST:
		fprintf(fp, " %d cB", amount);
		break;
	case T_MODSUST:
		fprintf(fp, " %g %%", (double)amount/10.0);
		break;
	case T_CUTOFF:
		fprintf(fp, " %d Hz", awe_abscent_to_Hz(amount));
		break;
	case T_FREQ:
		fprintf(fp, " %d mHz", awe_abscent_to_mHz(amount));
		break;
	case T_TIME:
		fprintf(fp, " %d msec", awe_timecent_to_msec(amount));
		break;
	case T_TENPCT:
	case T_PANPOS:
		fprintf(fp, " %g %%", (double)amount / 10.0);
		break;
	case T_PSHIFT:
	case T_CSHIFT:
		fprintf(fp, " %g semitone", (double)amount / 100);
		break;
	}
	fprintf(fp, ")");
}
