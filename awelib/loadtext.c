/*================================================================
 * loadtext.c:
 *	load textized soundfont information
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
#include "slist.h"
#include "util.h"
#include "sffile.h"
#include "sflayer.h"

/*----------------------------------------------------------------
 * prototypes
 *----------------------------------------------------------------*/

static void load_sflist(SFInfo *sf, SList list);
static void load_preset(SFInfo *sf, SList at);
static void load_inst(SFInfo *lp, SList at);
static void load_layers(SFHeader *hdr, SList at);
static void load_lists(SFGenLayer *lay, SList at);
static int sf_index(char *str);
static void load_sample(SFInfo *sf, SList at);


/*----------------------------------------------------------------
 * load the text file
 *----------------------------------------------------------------*/

void awe_load_textinfo(SFInfo *sf, FILE *fp)
{
	SList list;
	while ((list = SReadFile(fp)) != NIL) {
		/*print_list(list);*/
		load_sflist(sf, list);
		SFree(list);
	}
}

/*----------------------------------------------------------------
 * parse a list
 *----------------------------------------------------------------*/

static void load_sflist(SFInfo *sf, SList list)
{
	if (! SListP(list))
		return;

	list = SCar(list);
	if (! SFunP(list))
		return;

	if (SFunIs(list, "Name")) {
		sf->sf_name = safe_strdup(SStr(SCdr(list)));
	} else if (SFunIs(list, "SamplePos")) {
		sf->samplepos = SInt(SCdr(list));
		sf->samplesize = SInt(SCdr(SCdr(list)));
	} else if (SFunIs(list, "SoundFont")) {
		sf->version = SInt(SCdr(list));
		sf->minorversion = SInt(SCdr(SCdr(list)));
	} else if (SFunIs(list, "InfoPos")) {
		sf->infopos = SInt(SCdr(list));
		sf->infosize = SInt(SCdr(SCdr(list)));
	} else if (SFunIs(list, "Presets")) {
		load_preset(sf, SCdr(list));
	} else if (SFunIs(list, "Instruments")) {
		load_inst(sf, SCdr(list));
	} else if (SFunIs(list, "SampleInfo")) {
		load_sample(sf, SCdr(list));
	} else {
		fprintf(stderr, "unknown tag %s\n", SFun(list));
	}
}


/*----------------------------------------------------------------
 * parse preset list
 *----------------------------------------------------------------*/

static void load_preset(SFInfo *sf, SList at)
{
	int i;
	SFPresetHdr *p;

	for (; at != NIL && !SListP(at); at = SCdr(at))
		;
	if (at == NIL) return;
	if ((sf->npresets = SIndex(at)) <= 0) {
		sf->npresets = 0;
		return;
	}
	sf->preset = (SFPresetHdr*)safe_malloc
		(sizeof(SFPresetHdr) * sf->npresets);

	at = SCar(at);
	p = sf->preset;
	for (i = 0; i < sf->npresets && at != NIL; i++) {
		SList list = SCar(at);
		list = SCdr(list); /* skip index */
		strncpy(p->hdr.name, SStr(list), 20); list = SCdr(list);
		p->preset = SInt(SCdr(SCar(list))); list = SCdr(list);
		p->bank = SInt(SCdr(SCar(list))); list = SCdr(list);
		load_layers(&p->hdr, list);
		at = SCdr(at); p++;
	}
}


/*----------------------------------------------------------------
 * parse instrument list
 *----------------------------------------------------------------*/

static void load_inst(SFInfo *sf, SList at)
{
	int i;
	SFInstHdr *p;

	for (; at != NIL && !SListP(at); at = SCdr(at))
		;
	if (at == NIL) return;
	if ((sf->ninsts = SIndex(at)) <= 0) {
		sf->ninsts = 0;
		return;
	}
	sf->inst = (SFInstHdr*)safe_malloc(sizeof(SFInstHdr) * sf->ninsts);

	at = SCar(at);
	p = sf->inst;
	for (i = 0; i < sf->ninsts && at != NIL; i++) {
		SList list = SCar(at);
		list = SCdr(list); /* skip index */
		strncpy(p->hdr.name, SStr(list), 20); list = SCdr(list);
		load_layers(&p->hdr, list);
		at = SCdr(at); p++;
	}
}


/*----------------------------------------------------------------
 * parse preset/inst layer list
 *----------------------------------------------------------------*/

static void load_layers(SFHeader *hdr, SList at)
{
	int i;
	SFGenLayer *p;

	if ((hdr->nlayers = SIndex(at)) <= 0) {
		hdr->nlayers = 0;
		return;
	}
	hdr->layer = (SFGenLayer*)safe_malloc(sizeof(SFGenLayer) * hdr->nlayers);
	p = hdr->layer;
	at = SCar(at);
	for (i = 0; i < hdr->nlayers; i++) {
		load_lists(p, at);
		p++;
		at = SCdr(at);
	}
}


/*----------------------------------------------------------------
 * parse layered elements
 *----------------------------------------------------------------*/

static void load_lists(SFGenLayer *lay, SList at)
{
	int i;
	SFGenRec *p;

	lay->nlists = SIndex(at) - 1;
	if (lay->nlists < 0) {
		lay->nlists = 0;
		lay->list = NULL;
		return;
	}
	lay->list = (SFGenRec*)safe_malloc(sizeof(SFGenRec) * lay->nlists);
	p = lay->list;
	at = SCdr(SCar(at));
	for (i = 0; i < lay->nlists; i++) {
		SList list = SCar(at);
		p->oper = sf_index(SFun(list));
		p->amount = SInt(SCdr(list));
		p++;
		at = SCdr(at);
	}
}


/*----------------------------------------------------------------
 * convert string to SF id number
 *----------------------------------------------------------------*/

static int sf_index(char *str)
{
	int i;
	for (i = 0; i < SF_EOF; i++) {
		if (strcmp(sf_gen_text[i], str) == 0)
			return i;
	}
	return SF_EOF;
}


/*----------------------------------------------------------------
 * parse sample info list
 *----------------------------------------------------------------*/

static void load_sample(SFInfo *sf, SList at)
{
	int i;
	SFSampleInfo *p;
	int in_rom;

	for (; at != NIL && !SListP(at); at = SCdr(at))
		;
	if (at == NIL) return;
	if ((sf->nsamples = SIndex(at)) <= 0) {
		sf->nsamples = 0;
		return;
	}

	sf->sample = (SFSampleInfo*)safe_malloc
		(sizeof(SFSampleInfo) * sf->nsamples);
	at = SCar(at);
	p = sf->sample;
	in_rom = 1;  /* data may start from ROM samples */
	for (i = 0; i < sf->nsamples; i++) {
		SList list = SCar(at);
		list = SCdr(list); /* skip index */
		strncpy(p->name, SStr(list), 20); list = SCdr(list);
		p->startsample = SInt(SCar(list));
		p->endsample = SInt(SCdr(SCar(list))); list = SCdr(list);
		p->startloop = SInt(SCar(list));
		p->endloop = SInt(SCdr(SCar(list))); list = SCdr(list);
		if (list != NIL && sf->version > 1) {
			list = SCar(list);
			p->samplerate = SInt(list); list = SCdr(list);
			p->originalPitch = SInt(list); list = SCdr(list);
			p->pitchCorrection = SInt(list); list = SCdr(list);
			p->samplelink = SInt(list); list = SCdr(list);
			p->sampletype = SInt(list);
		} else {
			p->samplerate = 44100;
			p->originalPitch = 60;
			p->pitchCorrection = 0;
			p->samplelink = 0;
			/* the first RAM data starts from address 0 */
			if (p->startsample == 0)
				in_rom = 0;
			if (in_rom)
				p->sampletype = 0x8001;
			else
				p->sampletype = 1;
		}
		p++;
		at = SCdr(at);
	}

}
