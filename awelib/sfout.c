/*----------------------------------------------------------------
 * sfout.c:
 *	save to soundfont format
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
#include <string.h>
#include <stdlib.h>
#include "util.h"
#include "sffile.h"
#include "config.h"

/*----------------------------------------------------------------
 * prototypes
 *----------------------------------------------------------------*/

static void write_header(SFInfo *sf, FILE *fin, FILE *fout);
static void write_info(SFInfo *sf, FILE *fin, FILE *fout);
static void write_sdta(SFInfo *sf, FILE *fin, FILE *fout);
static int calc_pdta_size(SFInfo *sf);
static void write_pdta(SFInfo *sf, FILE *fin, FILE *fout);
static void copy_file(int size, FILE *fin, FILE *fout);


#ifndef WORDS_BIGENDIAN
#define READCHUNK(var,fd)	fread(&var, 8, 1, fd)
#define WRITECHUNK(var,fd)	fwrite(&var, 8, 1, fd)
#define WRITEW(var,fd)		fwrite(&var, 2, 1, fd)
#define WRITEDW(var,fd)		fwrite(&var, 4, 1, fd)
#else
#define XCHG_SHORT(x) ((((x)&0xFF)<<8) | (((x)>>8)&0xFF))
#define XCHG_LONG(x) ((((x)&0xFF)<<24) | \
		      (((x)&0xFF00)<<8) | \
		      (((x)&0xFF0000)>>8) | \
		      (((x)>>24)&0xFF))
#define READCHUNK(var,fd)	{uint32 tmp; fread((var).id, 4, 1, fd);\
	fread(&tmp, 4, 1, fd); (var).size = XCHG_LONG(tmp);}
#define WRITECHUNK(var,fd)	{uint32 tmp; fwrite((var).id, 4, 1, fd);\
	tmp = XCHG_LONG((uint32)(var).size); fwrite(&tmp, 8, 1, fd);}
#define WRITEW(var,fd)		{uint16 tmp = XCHG_SHORT((uint16)(var)); fwrite(&tmp, 2, 1, fd);}
#define WRITEDW(var,fd)		{uint32 tmp = XCHG_LONG((uint32)(var)); fwrite(&tmp, 4, 1, fd);}
#endif
#define WRITEB(var,fd)		fwrite(&var, 1, 1, fd)
#define WRITEID(str,fd)		fwrite(str, 4, 1, fd)
#define WRITESTR(str,fd)	fwrite(str, 20, 1, fd)


/*----------------------------------------------------------------
 * save the soundfont info to a file
 *----------------------------------------------------------------*/

void awe_save_soundfont(SFInfo *sf, FILE *fin, FILE *fout)
{
	write_header(sf, fin, fout);
	write_info(sf, fin, fout);
	write_sdta(sf, fin, fout);
	write_pdta(sf, fin, fout);
}

/*----------------------------------------------------------------
 * write RIFF header and sfbk id
 *----------------------------------------------------------------*/

static void write_header(SFInfo *sf, FILE *fin, FILE *fout)
{
	int32 size;

	WRITEID("RIFF", fout);
	size = 4; /* sfbk header */
	size += sf->infosize + 4 + 8; /* info list */
	size += sf->samplesize + 8 + 4 + 8; /* sdta */
	if (sf->version == 1)
		size += sf->nsamples * 20 + 8; /* snam */
	size += calc_pdta_size(sf) + 8; /* pdta */
	WRITEDW(size, fout);
	WRITEID("sfbk", fout);
}


/*----------------------------------------------------------------
 * write info list
 *----------------------------------------------------------------*/

static void write_info(SFInfo *sf, FILE *fin, FILE *fout)
{
	int32 size, left;
	
	WRITEID("LIST", fout);
	size = sf->infosize + 4; /* size with 'INFO' id */
	WRITEDW(size, fout);
	WRITEID("INFO", fout);
	left = sf->infosize;
	fseek(fin, sf->infopos, SEEK_SET);
	while (left > 0) {
		SFChunk chunk;
		READCHUNK(chunk, fin); left -= 8;
		/* replace INAM */
		if (strncmp(chunk.id, "INAM", 4) == 0) {
			char term = 0;
			WRITECHUNK(chunk, fout);
			fwrite(sf->sf_name, chunk.size - 1, 1, fout);
			WRITEB(term, fout); /* write zero terminator */
			fseek(fin, chunk.size, SEEK_CUR);
		} else {
			WRITECHUNK(chunk, fout);
			copy_file(chunk.size, fin, fout);
		}
		left -= chunk.size;
	}
}


/*----------------------------------------------------------------
 * write sdta list
 *----------------------------------------------------------------*/

static void write_sdta(SFInfo *sf, FILE *fin, FILE *fout)
{
	int32 size;
	int i;

	WRITEID("LIST", fout);
	size = 4; /* size of 'sdta' id */
	if (sf->version == 1)
		size += sf->nsamples * 20 + 8; /* size of 'snam' chunk */
	if (sf->samplesize > 0)
		size += sf->samplesize + 8; /* size of 'smpl' chunk */
	WRITEDW(size, fout);
	WRITEID("sdta", fout);
	if (sf->version == 1) {
		WRITEID("snam", fout);
		size = sf->nsamples * 20;
		WRITEDW(size, fout);
		for (i = 0; i < sf->nsamples; i++) {
			WRITESTR(sf->sample[i].name, fout);
		}
	}
	if (sf->samplesize > 0) {
		WRITEID("smpl", fout);
		size = sf->samplesize;
		WRITEDW(size, fout);
		fseek(fin, sf->samplepos, SEEK_SET);
		copy_file(sf->samplesize, fin, fout);
	}
}


/*----------------------------------------------------------------
 * calculate pdta list size
 *----------------------------------------------------------------*/

static int calc_pdta_size(SFInfo *sf)
{
	int32 size = 0;
	int i, j;
	uint16 bag, gen;

	/* pdta id */
	size = 4;

	/* phdr chunk */
	size += sf->npresets * 38 + 8;

	/* pbag chunk */
	bag = 0;
	for (i = 0; i < sf->npresets; i++) {
		if (sf->preset[i].hdr.nlayers <= 0)
			bag++;
		else
			bag += sf->preset[i].hdr.nlayers;
	}
	size += bag * 4 + 8;
		
	/* pmod chunk */
	size += 8;

	/* pgen chunk */
	gen = 0;
	for (i = 0; i < sf->npresets; i++) {
		for (j = 0; j < sf->preset[i].hdr.nlayers; j++)
			gen += sf->preset[i].hdr.layer[j].nlists;
	}
	size += gen * 4 + 8;

	/* ihdr chunk */
	size += sf->ninsts * 22 + 8;

	/* ibag chunk */
	bag = 0;
	for (i = 0; i < sf->ninsts; i++) {
		if (sf->inst[i].hdr.nlayers <= 0)
			bag++;
		else
			bag += sf->inst[i].hdr.nlayers;
	}
	size += bag * 4 + 8;
		
	/* imod chunk */
	size += 8;

	/* igen chunk */
	gen = 0;
	for (i = 0; i < sf->ninsts; i++) {
		for (j = 0; j < sf->inst[i].hdr.nlayers; j++)
			gen += sf->inst[i].hdr.layer[j].nlists;
	}
	size += gen * 4 + 8;

	/* shdr chunk */
	if (sf->version == 1)
		size += sf->nsamples * 16 + 8;
	else
		size += sf->nsamples * 46 + 8;

	return size;
}


/*----------------------------------------------------------------
 * write pdta list
 *----------------------------------------------------------------*/

static void write_pdta(SFInfo *sf, FILE *fin, FILE *fout)
{
	int32 size, total_size;
	int i, j, k;
	uint16 bag, gen;
	
	WRITEID("LIST", fout);
	size = calc_pdta_size(sf);
	WRITEDW(size, fout);
	WRITEID("pdta", fout);
	total_size = 4;

	WRITEID("phdr", fout);
	size = sf->npresets * 38;
	WRITEDW(size, fout);
	total_size += size + 8;

	/* calculate bag size */
	bag = 0;
	for (i = 0; i < sf->npresets; i++) {
		int32 dummy = 0;
		WRITESTR(sf->preset[i].hdr.name, fout);
		WRITEW(sf->preset[i].preset, fout);
		WRITEW(sf->preset[i].bank, fout);
		WRITEW(bag, fout);
		WRITEDW(dummy, fout);
		WRITEDW(dummy, fout);
		WRITEDW(dummy, fout);
		if (sf->preset[i].hdr.nlayers <= 0)
			bag++;
		else
			bag += sf->preset[i].hdr.nlayers;
	}

	WRITEID("pbag", fout);
	size = bag * 4;
	WRITEDW(size, fout);
	total_size += size + 8;

	/* calculate generators size */
	gen = 0;
	for (i = 0; i < sf->npresets; i++) {
		SFHeader *p = &sf->preset[i].hdr;
		int16 dummy = 0;
		if (p->nlayers <= 0) {
			WRITEW(gen, fout);
			WRITEW(dummy, fout);
			continue;
		}
		for (j = 0; j < p->nlayers; j++) {
			WRITEW(gen, fout);
			WRITEW(dummy, fout);
			gen += p->layer[j].nlists;
		}
	}
		
	WRITEID("pmod", fout);
	size = 0;
	WRITEDW(size, fout);
	total_size += size + 8;
	
	WRITEID("pgen", fout);
	size = gen * 4;
	WRITEDW(size, fout);
	total_size += size + 8;

	for (i = 0; i < sf->npresets; i++) {
		SFHeader *p = &sf->preset[i].hdr;
		for (j = 0; j < p->nlayers; j++) {
			for (k = 0; k < p->layer[j].nlists; k++) {
				WRITEW(p->layer[j].list[k].oper, fout);
				WRITEW(p->layer[j].list[k].amount, fout);
			}
		}
	}

	WRITEID("inst", fout);
	size = sf->ninsts * 22;
	WRITEDW(size, fout);
	total_size += size + 8;

	bag = 0;
	for (i = 0; i < sf->ninsts; i++) {
		WRITESTR(sf->inst[i].hdr.name, fout);
		WRITEW(bag, fout);
		if (sf->inst[i].hdr.nlayers <= 0)
			bag++;
		else
			bag += sf->inst[i].hdr.nlayers;
	}

	WRITEID("ibag", fout);
	size = bag * 4;
	WRITEDW(size, fout);
	total_size += size + 8;

	gen = 0;
	for (i = 0; i < sf->ninsts; i++) {
		SFHeader *p = &sf->inst[i].hdr;
		int16 dummy = 0;
		if (p->nlayers <= 0) {
			WRITEW(gen, fout);
			WRITEW(dummy, fout);
			continue;
		}
		for (j = 0; j < p->nlayers; j++) {
			WRITEW(gen, fout);
			WRITEW(dummy, fout);
			gen += p->layer[j].nlists;
		}
	}
		
	WRITEID("imod", fout);
	size = 0;
	WRITEDW(size, fout);
	total_size += size + 8;
	
	WRITEID("igen", fout);
	size = gen * 4;
	WRITEDW(size, fout);
	total_size += size + 8;

	for (i = 0; i < sf->ninsts; i++) {
		SFHeader *p = &sf->inst[i].hdr;
		for (j = 0; j < p->nlayers; j++) {
			for (k = 0; k < p->layer[j].nlists; k++) {
				WRITEW(p->layer[j].list[k].oper, fout);
				WRITEW(p->layer[j].list[k].amount, fout);
			}
		}
	}
	
	WRITEID("shdr", fout);
	if (sf->version == 1)
		size = sf->nsamples * 16;
	else
		size = sf->nsamples * 46;
	WRITEDW(size, fout);
	total_size += size + 8;

	for (i = 0; i < sf->nsamples; i++) {
		if (sf->version > 1)
			WRITESTR(sf->sample[i].name, fout);
		WRITEDW(sf->sample[i].startsample, fout);
		WRITEDW(sf->sample[i].endsample, fout);
		WRITEDW(sf->sample[i].startloop, fout);
		WRITEDW(sf->sample[i].endloop, fout);
		if (sf->version > 1) { /* SF2 only */
			WRITEDW(sf->sample[i].samplerate, fout);
			WRITEB(sf->sample[i].originalPitch, fout);
			WRITEB(sf->sample[i].pitchCorrection, fout);
			WRITEW(sf->sample[i].samplelink, fout);
			WRITEW(sf->sample[i].sampletype, fout);
		}
	}

	if ((size = calc_pdta_size(sf)) != total_size) {
		fprintf(stderr, "pdta total size differ: %d %d\n",
			total_size, size);
	}
}


/*----------------------------------------------------------------
 * copy a block from file to file
 *----------------------------------------------------------------*/

static void copy_file(int size, FILE *fin, FILE *fout)
{
	static char buf[1024];
	int s;

	for (s = size; s >= sizeof(buf); s -= sizeof(buf)) {
		fread(buf, sizeof(buf), 1, fin);
		fwrite(buf, sizeof(buf), 1, fout);
	}
	if (s > 0) {
		fread(buf, s, 1, fin);
		fwrite(buf, s, 1, fout);
	}
}
