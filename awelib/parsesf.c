/*================================================================
 * parsesf.c
 *	parse SoundFonr layers and convert it to AWE driver patch
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

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#ifdef linux
#include <linux/soundcard.h>
#include <linux/awe_voice.h>
#else
#include <machine/soundcard.h>
#include <awe_voice.h>
#endif
#include "awe_parm.h"
#include "itypes.h"
#include "sffile.h"
#include "sflayer.h"
#include "sfitem.h"
#include "aweseq.h"
#include "util.h"
#include "sfopts.h"


/*----------------------------------------------------------------
 * prototypes
 *----------------------------------------------------------------*/

#define P_GLOBAL	1
#define P_LAYER		2

typedef int (*Loader)(AWEOps *ops, SFInfo *sf, LayerTable *tbl, LoadList *request);

static int awe_make_unique_name(char *given, FILE *fp, char *dst);

static int probe_sample(AWEOps *ops, int sample_id);
static int load_matched_font(AWEOps *ops, SFInfo *sf, LoadList *request);
static int load_samples(AWEOps *ops, SFInfo *sf, int layer, LoadList *request, LoadList *exlist);
static int sample_loader(AWEOps *ops, SFInfo *sf, LayerTable *tbl, LoadList *request);
static int load_infos(AWEOps *ops, SFInfo *sf, int layer, LoadList *request, LoadList *exlist);
static int info_loader(AWEOps *ops, SFInfo *sf, LayerTable *tbl, LoadList *request);
static void set_sample_info(SFInfo *sf, awe_voice_info *vp, LayerTable *tbl);
static void set_init_info(SFInfo *sf, awe_voice_info *vp, LayerTable *tbl);
static void set_rootkey(SFInfo *sf, awe_voice_info *vp, LayerTable *tbl);
static void set_modenv(SFInfo *sf, awe_voice_info *vp, LayerTable *tbl);
static void set_volenv(SFInfo *sf, awe_voice_info *vp, LayerTable *tbl);
static void set_lfo1(SFInfo *sf, awe_voice_info *vp, LayerTable *tbl);
static void set_lfo2(SFInfo *sf, awe_voice_info *vp, LayerTable *tbl);
static int parse_preset_layers(AWEOps *ops, SFInfo *sf, SFPresetHdr *preset,
			       LoadList *request, LoadList *exlist,
			       Loader loader);
static void search_def_drum_inst(SFInfo *sf);
static int parse_inst_layers(AWEOps *ops, SFInfo *sf, LayerTable *tbl, LoadList *request,
			     LoadList *exlist, Loader loader, int level);
static int find_inst(SFGenLayer *layer);
static int is_global(SFGenLayer *layer);
static LoadList *is_excluded_preset(LoadList *list, SFPatchRec *pat);
static void init_layer_items(SFInfo *sf);
static void clear_table(LayerTable *tbl);
static void set_to_table(SFInfo *sf, LayerTable *tbl, SFGenLayer *lay, int level);
static void add_item_to_table(LayerTable *tbl, int oper, int amount, int level);
static void merge_table(SFInfo *sf, LayerTable *dst, LayerTable *src);
static void init_and_merge_table(SFInfo *sf, LayerTable *dst, LayerTable *src);
static int sanity_range(LayerTable *tbl);
static int in_range(LayerTable *tbl, int key);

static void awe_init_marks(void);
static void awe_free_marks(void);
static void add_sample(int sample);
static int search_sample(int id);

/*----------------------------------------------------------------
 * local common variables
 *----------------------------------------------------------------*/

static int def_drum_inst;
static FILE *sample_fd;
static int mem_avail;


/*----------------------------------------------------------------
 * manage instrument counters in hash
 *----------------------------------------------------------------*/

#define INFO_COUNT_MAPS	64

struct info_count_table {
	unsigned short instr;
	unsigned short count;
	struct info_count_table *next;
};
static struct info_count_table *info_count[INFO_COUNT_MAPS];

static void info_write_count_clear(void)
{
	int i;
	struct info_count_table *p;
	for (i = 0; i < INFO_COUNT_MAPS; i++) {
		if ((p = info_count[i]) != NULL) {
			info_count[i] = p->next;
			safe_free(p);
		}
	}
}

static int info_write_count_inc(unsigned short instr)
{
	int key = instr % INFO_COUNT_MAPS;
	struct info_count_table *val;
	for (val = info_count[key]; val; val = val->next) {
		if (val->instr == instr)
			return ++val->count;
	}
	val = safe_malloc(sizeof(*val));
	val->instr = instr;
	val->count = 1;
	val->next = info_count[key];
	info_count[key] = val;
	return 1;
}


/*================================================================
 * open / close patch
 *================================================================*/

static int open_patch_priv(AWEOps *ops, char *name, int type, int locked, int shared)
{
	struct open_patch_rec {
		awe_patch_info head;
		awe_open_parm parm;
	} rec;

	rec.head.type = AWE_OPEN_PATCH;
	rec.head.len = AWE_OPEN_PARM_SIZE;
	rec.parm.type = type;
	if (locked)
		rec.parm.type |= AWE_PAT_LOCKED;
	if (shared)
		rec.parm.type |= AWE_PAT_SHARED;
	memcpy(rec.parm.name, name, AWE_PATCH_NAME_LEN);
	return ops->load_patch(&rec, sizeof(rec));
}

int awe_open_patch(AWEOps *ops, char *name, int type, int locked)
{
	char tmpname[AWE_PATCH_NAME_LEN];
	strncpy(tmpname, name, AWE_PATCH_NAME_LEN);
	return open_patch_priv(ops, tmpname, type, locked, FALSE);
}

int awe_open_font(AWEOps *ops, SFInfo *sf, FILE *fp, int locked)
{
	unsigned char uname[AWE_PATCH_NAME_LEN];
	int atten;

	def_drum_inst = -1;
	sample_fd = fp;
	mem_avail = ops->mem_avail();
	info_write_count_clear();

	if (awe_option.compatible)
		atten = awe_option.default_atten;
	else
		atten = 0;
	ops->set_zero_atten(atten);

	init_layer_items(sf);
	awe_init_marks();

	if (awe_make_unique_name(sf->sf_name, fp, uname) != AWE_RET_OK)
		return -1;

	return open_patch_priv(ops, uname, AWE_PAT_TYPE_GS, locked, TRUE);
}

/* create a unique name from the given name and file date/size
 * for sample sharing
 *
 * name format = KEY(3bytes) + VERSION(3bytes) + NAME(18bytes)
 *               DATE(4bytes) + SIZE(4bytes) = total 32bytes
 * KEY = 0x01/0x17/0x05
 * VERSION = 0x00/0x04/0x03
 * NAME = copied from given
 * DATE = stat->st_mtime
 * SIZE = stat->st_size
 */

#define ASC_TO_KEY(c) ((c) - 'A' + 1)
static int awe_make_unique_name(char *given, FILE *fp, char *dst)
{
	struct stat st;

	if (fstat(fileno(fp), &st) < 0) {
		if (awe_verbose)
			fprintf(stderr, "parsesf: can't get stat of font file\n");
		return AWE_RET_ERR;
	}

	dst[0] = ASC_TO_KEY('A');
	dst[1] = ASC_TO_KEY('W');
	dst[2] = ASC_TO_KEY('E');
	dst[3] = 0x00;
	dst[4] = 0x04;
	dst[5] = 0x03;
	sprintf(dst + 6, "%-18.18s", given);

	*(int*)(dst + 24) = (int)st.st_mtime;
	*(int*)(dst + 28) = (int)st.st_size;

	return AWE_RET_OK;
}


int awe_close_patch(AWEOps *ops)
{
	awe_patch_info head;

	head.key = AWE_PATCH;
	head.type = AWE_CLOSE_PATCH;
	head.len = 0;
	return ops->load_patch(&head, sizeof(head));
}

void awe_close_font(AWEOps *ops, SFInfo *sf)
{
	def_drum_inst = -1;
	sample_fd = NULL;
	awe_free_marks();
	awe_close_patch(ops);
}


/*----------------------------------------------------------------*/

int awe_is_ram_fonts(SFInfo *sf)
{
	int i;
	for (i = 0; i < sf->nsamples; i++) {
		if (sf->sample[i].size > 0)
			return TRUE;
	}
	return FALSE;
}

/*----------------------------------------------------------------
 * check if the sample is loade on driver
 * -- using the new feature of awedrv-0.4.3p4
 *----------------------------------------------------------------*/

static int probe_sample(AWEOps *ops, int sample_id)
{
#ifdef AWE_PROBE_DATA
	awe_patch_info patch;
	patch.type = AWE_PROBE_DATA;
	patch.len = 0;
	patch.optarg = sample_id;
	if (ops->load_patch(&patch, sizeof(patch)) >= 0)
		return TRUE;
#endif
	return FALSE;
}

/*================================================================
 * load a preset link
 *================================================================*/

int awe_load_map(AWEOps *ops, LoadList *lp)
{
	struct open_patch_rec {
		awe_patch_info head;
		awe_voice_map parm;
	} rec;

	rec.head.type = AWE_MAP_PRESET;
	rec.head.len = sizeof(awe_voice_map);
	rec.parm.src_bank = lp->pat.bank;
	rec.parm.src_instr = lp->pat.preset;
	rec.parm.src_key = lp->pat.keynote;
	rec.parm.map_bank = lp->map.bank;
	rec.parm.map_instr = lp->map.preset;
	rec.parm.map_key = lp->map.keynote;
	if (ops->load_patch(&rec, sizeof(rec)) < 0) {
		if (awe_verbose)
			fprintf(stderr, "awe: can't load preset mapping\n");
		return AWE_RET_ERR;
	}
	return AWE_RET_OK;
}


/*================================================================
 * load the matched fonts with the given list
 *----------------------------------------------------------------
 * plist = requests list
 * load_alt = true if loading alternatives for missing fonts
 *================================================================*/

int awe_load_font_list(AWEOps *ops, SFInfo *sf, LoadList *plist, int load_alt)
{
	int rc;
	LoadList *p;

	for (p = plist; p; p = p->next) {
		if (p->loaded)
			continue;
		if ((rc = load_matched_font(ops, sf, p)) == AWE_RET_OK)
			p->loaded = TRUE;
		else if (rc == AWE_RET_ERR || rc == AWE_RET_NOMEM)
			return rc;
	}

	if (load_alt) {
		/* load alternative fonts if the fonts not exist */
		for (p = plist; p; p = p->next) {
			if (p->loaded)
				continue;
			rc = AWE_RET_SKIP;
			if (p->pat.bank == 128) {
				if (p->pat.preset != 0) {
					p->pat.preset = 0;
					rc = load_matched_font(ops, sf, p);
				}
			} else if (p->pat.bank != 0) {
				p->pat.bank = 0;
				rc = load_matched_font(ops, sf, p);
			}
			if (rc == AWE_RET_OK)
				p->loaded = TRUE;
			else if (rc == AWE_RET_ERR || AWE_RET_NOMEM)
				return rc;
		}
	}

	return AWE_RET_OK;
}

/* search preset list and find the mathing layer */
static int load_matched_font(AWEOps *ops, SFInfo *sf, LoadList *request)
{
	int i, rc;
	int found = 0;
	LoadList req;

	for (i = 0; i < sf->npresets; i++) {
		if (request->pat.preset != -1 &&
		    sf->preset[i].preset != request->pat.preset)
			continue;
		if (request->pat.bank != -1 &&
		    sf->preset[i].bank != request->pat.bank)
			continue;
		req = *request;
		if (req.pat.preset == -1)
			req.pat.preset = sf->preset[i].preset;
		if (req.pat.bank == -1)
			req.pat.bank = sf->preset[i].bank;
		/* remap the destination preset */
		awe_merge_keys(&request->map, &req.pat, &req.map);
		rc = load_samples(ops, sf, i, &req, NULL);
		if (rc == AWE_RET_ERR || rc == AWE_RET_NOMEM)
			return rc;
		else if (rc == AWE_RET_SKIP)
			continue;
		rc = load_infos(ops, sf, i, &req, NULL);
		if (rc == AWE_RET_ERR || rc == AWE_RET_NOMEM)
			return rc;
		found++;
	}
	if (found)
		return AWE_RET_OK;
	else
		return AWE_RET_SKIP;
}


/*================================================================
 * load the whole file at once
 *----------------------------------------------------------------
 * exlist = fonts to be excluded (only map is referred)
 *================================================================*/

int awe_load_all_fonts(AWEOps *ops, SFInfo *sf, LoadList *exlist)
{
	int rc, i;
	LoadList *p;

	search_def_drum_inst(sf);

	for (i = 0; i < sf->npresets; i++) {
		LoadList req;
		req.pat.preset = sf->preset[i].preset;
		req.pat.bank = sf->preset[i].bank;
		req.pat.keynote = -1;
		if ((p = is_excluded_preset(exlist, &req.pat)) != NULL) {
			if (p->pat.keynote == -1)
				continue; /* exclude this preset */
		}
		req.map = req.pat;
		req.loaded = FALSE;
		rc = load_samples(ops, sf, i, &req, exlist);
		if (rc == AWE_RET_NOMEM || rc == AWE_RET_ERR)
			return rc;
		rc = load_infos(ops, sf, i, &req, exlist);
		if (rc == AWE_RET_NOMEM || rc == AWE_RET_ERR)
			return rc;
	}

	return AWE_RET_OK;
}


/*================================================================
 * load a specified font
 *================================================================*/

/* load each preset */
int awe_load_font(AWEOps *ops, SFInfo *sf, SFPatchRec *pat, SFPatchRec *map)
{
	int rc;
	LoadList *plist = NULL;

	plist = awe_add_loadlist(plist, pat, map);
	rc = awe_load_font_list(ops, sf, plist, FALSE);
	awe_free_loadlist(plist);

	return rc;
}


/*================================================================
 * load sample data
 *================================================================*/

static int load_samples(AWEOps *ops, SFInfo *sf, int layer, LoadList *request, LoadList *exlist)
{
	return parse_preset_layers(ops, sf, &sf->preset[layer], request, exlist,
				   sample_loader);
}

/* sample loader */

typedef struct patch_rec {
	awe_patch_info patch;
	awe_sample_info hdr;
	unsigned short data[1];
} patch_rec;

static int sample_loader(AWEOps *ops, SFInfo *sf, LayerTable *tbl, LoadList *request)
{
	SFSampleInfo *sp;
	patch_rec *rec;

	if (search_sample(tbl->val[SF_sampleId]))
		return AWE_RET_OK;

	if (probe_sample(ops, tbl->val[SF_sampleId]))
		return AWE_RET_OK;

	sp = &sf->sample[tbl->val[SF_sampleId]];
	if (mem_avail < sp->size * 2)
		return AWE_RET_NOMEM;
	mem_avail -= sp->size * 2;

	/* allocate temporary buffer for all the sample */
	rec = (patch_rec*)safe_malloc(sizeof(*rec) + sp->size * 2);

	/* set sample info */
	rec->hdr.sf_id = 0;
	rec->hdr.sample = tbl->val[SF_sampleId];
	rec->hdr.start = sp->startsample;
	rec->hdr.end = sp->endsample;
	rec->hdr.loopstart = sp->startloop;
	rec->hdr.loopend = sp->endloop;
	rec->hdr.size = sp->size;
	rec->hdr.checksum_flag = 0;
	rec->hdr.mode_flags = 0;
	rec->hdr.checksum = 0;
		
	/* if this is not ROM sample, load it */
	if (rec->hdr.size > 0) {
		int pos = rec->hdr.start * 2;
		if (sample_fd == NULL) {
			if (awe_verbose)
				fprintf(stderr, "awe: no file is opend\n");
			safe_free(rec);
			return AWE_RET_ERR;
		}
		if (pos < 0 || pos > sf->samplesize) {
			if (awe_verbose)
				fprintf(stderr, "awe: illegal file pos %d\n", pos);
			safe_free(rec);
			return AWE_RET_ERR;
		}
		pos += sf->samplepos;
		fseek(sample_fd, pos, SEEK_SET);
		fread(rec->data, rec->hdr.size, 2, sample_fd);
		/* clear the blank data at tail */
		memset(rec->data + rec->hdr.end - rec->hdr.start, 0,
		       (rec->hdr.size - rec->hdr.end + rec->hdr.start) * 2);
	}
	/* ok, loading the patch.. */
	rec->patch.optarg = 0;
	rec->patch.len = AWE_SAMPLE_INFO_SIZE + rec->hdr.size * 2;
	/* not including the patch header size */
	rec->patch.type = AWE_LOAD_DATA;
	rec->patch.reserved = 0;
	if (ops->load_patch(rec, AWE_PATCH_INFO_SIZE + rec->patch.len) < 0) {
		safe_free(rec);
		if (errno == ENOSPC)
			return AWE_RET_NOMEM;
		else if (awe_verbose)
			fprintf(stderr, "awe: error in writing samples\n");
		return AWE_RET_ERR;
	}

	safe_free(rec);

	add_sample(tbl->val[SF_sampleId]);

	return AWE_RET_OK;
}


/*================================================================
 * load instrument data
 *================================================================*/

static int load_infos(AWEOps *ops, SFInfo *sf, int layer, LoadList *request, LoadList *exlist)
{
	return parse_preset_layers(ops, sf, &sf->preset[layer], request, exlist,
				   info_loader);
}

/* instrument patch loader */

static int info_loader(AWEOps *ops, SFInfo *sf, LayerTable *tbl, LoadList *request)
{
	static awe_voice_rec_patch vrec;
	awe_voice_info *vp = &vrec.info;
	
	/* set voice header */
	if (request->map.bank > 0 || awe_option.default_bank < 0)
		vrec.hdr.bank = request->map.bank;
	else
		vrec.hdr.bank = awe_option.default_bank;
	vrec.hdr.instr = request->map.preset;
	vrec.hdr.nvoices = 1;
	if (info_write_count_inc(vrec.hdr.instr) == 1)
		vrec.hdr.write_mode = AWE_WR_REPLACE; /* first time.. */
	else
		vrec.hdr.write_mode = AWE_WR_APPEND;

	/* set voice info parameters */
	set_sample_info(sf, vp, tbl);
	set_init_info(sf, vp, tbl);
	set_rootkey(sf, vp, tbl);
	set_modenv(sf, vp, tbl);
	set_volenv(sf, vp, tbl);
	set_lfo1(sf, vp, tbl);
	set_lfo2(sf, vp, tbl);

	memset(vp->parm.reserved, 0, sizeof(vp->parm.reserved));

	/* if key note is specified, replace the key range */
	if (request->map.keynote != -1 &&
	    request->pat.keynote != request->map.keynote) {
		vp->low = vp->high = request->map.keynote;
		vp->fixkey = request->map.keynote;
	}

	/* set patch header */
	vrec.patch.optarg = 0;
	vrec.patch.len = AWE_VOICE_REC_SIZE + AWE_VOICE_INFO_SIZE;
	vrec.patch.type = AWE_LOAD_INFO;
	vrec.patch.reserved = 0;

	/* then, put it to sequencer */
	if (ops->load_patch(&vrec, sizeof(vrec)) < 0) {
		if (awe_verbose)
			fprintf(stderr, "awe: can't load voice info\n");
		return AWE_RET_ERR;
	}

	return AWE_RET_OK;
}


/*----------------------------------------------------------------*/

/* set sample address */
static void set_sample_info(SFInfo *sf, awe_voice_info *vp, LayerTable *tbl)
{
	SFSampleInfo *sp;

	vp->sf_id = 0;
	vp->sample = tbl->val[SF_sampleId];
	sp = &sf->sample[vp->sample];

	vp->start = (tbl->val[SF_startAddrsHi] << 15)
		+ tbl->val[SF_startAddrs];
	vp->end = (tbl->val[SF_endAddrsHi] << 15)
		+ tbl->val[SF_endAddrs];
	vp->loopstart = (tbl->val[SF_startloopAddrsHi] << 15)
		+ tbl->val[SF_startloopAddrs];
	vp->loopend = (tbl->val[SF_endloopAddrsHi] << 15)
		+ tbl->val[SF_endloopAddrs];

	vp->rate_offset = awe_calc_rate_offset(sp->samplerate);

	/* sample mode */
	vp->mode = 0;
	if (sp->sampletype & 0x8000)
		vp->mode |= AWE_MODE_ROMSOUND;
	if (tbl->val[SF_sampleFlags] == 1 || tbl->val[SF_sampleFlags] == 3)
		/* looping */
		vp->mode |= AWE_MODE_LOOPING;
	else {
		/* short-shot; set a small blank loop at the tail */
		if (sp->loopshot > 8) {
			vp->loopstart = sp->endsample + 8 - sp->startloop;
			vp->loopend = sp->endsample + sp->loopshot - 8 - sp->endloop;
		} else {
			fprintf(stderr, "loop size is too short: %d\n", sp->loopshot);
			exit(1);
		}
	}
}

/*----------------------------------------------------------------*/

/* set global information */
static void set_init_info(SFInfo *sf, awe_voice_info *vp, LayerTable *tbl)
{
	/* key range */
	vp->low = LOWNUM(tbl->val[SF_keyRange]);
	vp->high = HIGHNUM(tbl->val[SF_keyRange]);

	/* velocity range */
	vp->vellow = LOWNUM(tbl->val[SF_velRange]);
	vp->velhigh = HIGHNUM(tbl->val[SF_velRange]);

	/* fixed key & velocity */
	vp->fixkey = tbl->val[SF_keynum];
	vp->fixvel = tbl->val[SF_velocity];
	
	/* panning position */
	vp->pan = awe_calc_pan(tbl->val[SF_panEffectsSend]);
	vp->fixpan = -1;

	/* initial volume */

	vp->amplitude = awe_option.default_volume * 127 / 100;
	vp->attenuation = awe_calc_attenuation(tbl->val[SF_initAtten]);
#if 0
	if (vp->mode & AWE_MODE_ROMSOUND && !awe_option.compatible) {
		if (vp->attenuation < 0xf0)
			vp->attenuation += 0x10;
		else
			vp->attenuation = 0xff;
	}
#endif
	
	/* chorus & reverb effects */
	if (tbl->set[SF_chorusEffectsSend])
		vp->parm.chorus = awe_calc_chorus(tbl->val[SF_chorusEffectsSend]);
	else
		vp->parm.chorus = awe_calc_chorus(awe_option.default_chorus * 10);
	if (tbl->set[SF_reverbEffectsSend])
		vp->parm.reverb = awe_calc_reverb(tbl->val[SF_reverbEffectsSend]);
	else
		vp->parm.reverb = awe_calc_reverb(awe_option.default_reverb * 10);

	/* initial cutoff & resonance */
	vp->parm.cutoff = awe_calc_cutoff(tbl->val[SF_initialFilterFc]);
	vp->parm.filterQ = awe_calc_filterQ(tbl->val[SF_initialFilterQ]);

	/* exclusive class key */
	vp->exclusiveClass = tbl->val[SF_keyExclusiveClass];
}

/*----------------------------------------------------------------*/

/* calculate root key & fine tune */
static void set_rootkey(SFInfo *sf, awe_voice_info *vp, LayerTable *tbl)
{
	SFSampleInfo *sp = &sf->sample[vp->sample];

	/* scale tuning */
	vp->scaleTuning = tbl->val[SF_scaleTuning];

	/* set initial root key & fine tune */
	if (sf->version == 1 && tbl->set[SF_samplePitch]) {
		/* set from sample pitch */
		vp->root = tbl->val[SF_samplePitch] / 100;
		vp->tune = -tbl->val[SF_samplePitch] % 100;
		if (vp->tune <= -50) {
			vp->root++;
			vp->tune = 100 + vp->tune;
		}
		if (vp->scaleTuning == 50)
			vp->tune /= 2;
	} else {
		/* from sample info */
		vp->root = sp->originalPitch;
		vp->tune = sp->pitchCorrection;
	}

	/* orverride root key */
	if (tbl->set[SF_rootKey])
		vp->root += tbl->val[SF_rootKey] - sp->originalPitch;

	/* tuning */
	if (sf->version == 1)
		vp->tune += tbl->val[SF_coarseTune] * vp->scaleTuning +
			(int)tbl->val[SF_fineTune] * (int)vp->scaleTuning / 100;
	else
		vp->tune += tbl->val[SF_coarseTune] * 100 + tbl->val[SF_fineTune];

	/* correct too high pitch */
	if (vp->root >= vp->high + 60)
		vp->root -= 60;
}

/*----------------------------------------------------------------*/

#define TO_WORD(hi,lo) (((unsigned short)(hi) << 8) | (unsigned short)(lo))

/* modulation envelope parameters */
static void set_modenv(SFInfo *sf, awe_voice_info *vp, LayerTable *tbl)
{
	/* delay */
	vp->parm.moddelay = awe_calc_delay(tbl->val[SF_delayEnv1]);

	/* attack & hold */
	vp->parm.modatkhld = awe_calc_atkhld(tbl->val[SF_attackEnv1],
					     tbl->val[SF_holdEnv1]);


	/* decay & sustain */
	vp->parm.moddcysus =
		TO_WORD(awe_calc_mod_sustain(tbl->val[SF_sustainEnv1]),
			awe_calc_decay(tbl->val[SF_decayEnv1]));

	/* release */
	vp->parm.modrelease =
		TO_WORD(0x80, awe_calc_decay(tbl->val[SF_releaseEnv1]));

	/* key hold/decay */
	vp->parm.modkeyhold = tbl->val[SF_autoHoldEnv1];
	vp->parm.modkeydecay = tbl->val[SF_autoDecayEnv1];

	/* pitch / cutoff shift */
	vp->parm.pefe =
		TO_WORD(awe_calc_pitch_shift(tbl->val[SF_env1ToPitch]),
			awe_calc_cutoff_shift(tbl->val[SF_env1ToFilterFc], 6));
}

/* volume envelope parameters */
static void set_volenv(SFInfo *sf, awe_voice_info *vp, LayerTable *tbl)
{
	/* delay */
	vp->parm.voldelay = awe_calc_delay(tbl->val[SF_delayEnv2]);

	/* attack & hold */
	vp->parm.volatkhld = awe_calc_atkhld(tbl->val[SF_attackEnv2],
					     tbl->val[SF_holdEnv2]);
	/* decay & sustain */
	vp->parm.voldcysus =
		TO_WORD(awe_calc_sustain(tbl->val[SF_sustainEnv2]),
			awe_calc_decay(tbl->val[SF_decayEnv2]));

	/* release */
	vp->parm.volrelease =
		TO_WORD(0x80, awe_calc_decay(tbl->val[SF_releaseEnv2]));

	/* key hold/decay */
	vp->parm.volkeyhold = tbl->val[SF_autoHoldEnv2];
	vp->parm.volkeydecay = tbl->val[SF_autoDecayEnv2];
}

/* lfo1 parameters (tremolo & vibrato) */
static void set_lfo1(SFInfo *sf, awe_voice_info *vp, LayerTable *tbl)
{
	vp->parm.lfo1delay = awe_calc_delay(tbl->val[SF_delayLfo1]);
	vp->parm.fmmod =
		TO_WORD(awe_calc_pitch_shift(tbl->val[SF_lfo1ToPitch]),
			awe_calc_cutoff_shift(tbl->val[SF_lfo1ToFilterFc], 3));
	vp->parm.tremfrq =
		TO_WORD(awe_calc_tremolo(tbl->val[SF_lfo1ToVolume]),
			awe_calc_freq(tbl->val[SF_freqLfo1]));
}

/* lfo2 parameters (vibrato only) */
static void set_lfo2(SFInfo *sf, awe_voice_info *vp, LayerTable *tbl)
{
	vp->parm.lfo2delay = awe_calc_delay(tbl->val[SF_delayLfo2]);
	vp->parm.fm2frq2 =
		TO_WORD(awe_calc_pitch_shift(tbl->val[SF_lfo2ToPitch]),
			awe_calc_freq(tbl->val[SF_freqLfo2]));
}


/*================================================================
 * parse preset and instrument layers and find matching instrument;
 * call loader with the parameter
 *----------------------------------------------------------------
 * preset = preset record to be parsed
 * request = requested instrument (must be specified)
 * exlist = excluded instruments (null = nothing)
 * loader = loading function
 *================================================================*/

static int parse_preset_layers(AWEOps *ops, SFInfo *sf, SFPresetHdr *preset,
			       LoadList *request, LoadList *exlist,
			       Loader loader)
{
	int rc, j, nlayers;
	SFGenLayer *layp, *globalp;

	/* if layer is empty, skip it */
	if ((nlayers = preset->hdr.nlayers) <= 0 ||
	    (layp = preset->hdr.layer) == NULL)
		return AWE_RET_SKIP;
	/* check global layer */
	globalp = NULL;
	if (is_global(layp)) {
		globalp = layp;
		layp++; /* start from next layer */
		nlayers--;
	}
	/* parse for each preset layer */
	for (j = 0; j < nlayers; j++, layp++) {
		LayerTable tbl;

		/* set up table */
		clear_table(&tbl);
		if (globalp)
			set_to_table(sf, &tbl, globalp, P_GLOBAL);
		set_to_table(sf, &tbl, layp, P_LAYER);
		
		/* parse the instrument layers */
		rc = parse_inst_layers(ops, sf, &tbl, request, exlist, loader, 0);

		/* fatal error */
		if (rc == AWE_RET_ERR || rc == AWE_RET_NOMEM)
			return rc;
	}

	return AWE_RET_OK;
}


/* find instrument id of default drumset #0:
 * the standard drumset is often included in other drumsets.
 */
static void search_def_drum_inst(SFInfo *sf)
{
	int i;
	SFGenLayer *layp;

	def_drum_inst = -1;
	for (i = 0; i < sf->npresets; i++) {
		if (sf->preset[i].bank == 128 && sf->preset[i].preset == 0) {
			/* check the first layer */
			if ((layp = sf->preset[i].hdr.layer) == NULL)
				continue;
			if (!is_global(layp))
				def_drum_inst = find_inst(layp);
			break;
		}
	}
}

/*================================================================
 * parse instrument layers -- called from preset parser
 * level represents the recursive level
 *================================================================*/

static int parse_inst_layers(AWEOps *ops, SFInfo *sf, LayerTable *tbl, LoadList *request,
			     LoadList *exlist, Loader loader, int level)
{
	SFInstHdr *inst;
	int rc, i, nlayers;
	SFGenLayer *lay, *globalp;

	if (level >= 2) {
		fprintf(stderr, "parse_layer: too deep instrument level\n");
		return AWE_RET_ERR;
	}

	/* instrument must be defined */
	if (!tbl->set[SF_instrument])
		return AWE_RET_SKIP;

	/* if non-standard drumset includes standard drum instruments,
	   skip it to avoid duplicate the data */
	inst = &sf->inst[tbl->val[SF_instrument]];
	/*
	if (def_drum_inst >= 0 && request->pat.bank == 128 &&
	    request->pat.preset != 0 &&
	    tbl->val[SF_instrument] == def_drum_inst)
		return AWE_RET_SKIP;
		*/

	/* if layer is empty, skip it */
	if ((nlayers = inst->hdr.nlayers) <= 0 ||
	    (lay = inst->hdr.layer) == NULL)
		return AWE_RET_SKIP;

	/* check global layer */
	globalp = NULL;
	if (is_global(lay)) {
		globalp = lay;
		lay++; /* start from the next layer */
		nlayers--;
	}

	/* parse for each layer */
	for (i = 0; i < nlayers; i++, lay++) {
		LayerTable ctbl;
		clear_table(&ctbl);
		if (globalp)
			set_to_table(sf, &ctbl, globalp, P_GLOBAL);
		set_to_table(sf, &ctbl, lay, P_LAYER);

		if (!ctbl.set[SF_sampleId]) {
			/* recursive loading */
			merge_table(sf, &ctbl, tbl);
			if (! sanity_range(&ctbl))
				continue;
			rc = parse_inst_layers(ops, sf, &ctbl, request, exlist,
					       loader, level+1);
			if (rc == AWE_RET_NOMEM || rc == AWE_RET_ERR)
				return rc;
		} else {
			init_and_merge_table(sf, &ctbl, tbl);
			if (! sanity_range(&ctbl))
				continue;
			if (request->pat.keynote != -1 &&
			    ! in_range(&ctbl, request->pat.keynote))
				continue;
			if (exlist) {
				/* check exclusion list */
				SFPatchRec tmp;
				tmp = request->pat;
				tmp.keynote = LOWNUM(tbl->val[SF_keyRange]);
				if (is_excluded_preset(exlist, &tmp))
					continue;
			}

			rc = loader(ops, sf, &ctbl, request);
			if (rc == AWE_RET_NOMEM || rc == AWE_RET_ERR)
				return rc;
		}
	}
	return AWE_RET_OK;
}


/*----------------------------------------------------------------
 * find instrument in the list
 *----------------------------------------------------------------*/

static int find_inst(SFGenLayer *layer)
{
	int i;
	for (i = 0; i < layer->nlists; i++) {
		if (layer->list[i].oper == SF_instrument)
			return layer->list[i].amount;
	}
	return -1;
}


/*----------------------------------------------------------------
 * check if the layer is global layer
 *----------------------------------------------------------------*/

static int is_global(SFGenLayer *layer)
{
	int i;
	for (i = 0; i < layer->nlists; i++) {
		if (layer->list[i].oper == SF_instrument ||
		    layer->list[i].oper == SF_sampleId)
			return 0;
	}
	return 1;
}


/*----------------------------------------------------------------
 * check if the specified preset matches the excluded list
 *----------------------------------------------------------------*/

static LoadList *is_excluded_preset(LoadList *list, SFPatchRec *pat)
{
	LoadList *p;
	for (p = list; p; p = p->next) {
		if (awe_match_preset(&p->map, pat))
			return p;
	}
	return NULL;
}


/*================================================================
 * layer table handlers
 *================================================================*/

/* initialize layer default values according to SF version */
static void init_layer_items(SFInfo *sf)
{
	/* default value is not zero */
	if (sf->version == 1) {
		layer_items[SF_sustainEnv1].defv = 1000;
		layer_items[SF_sustainEnv2].defv = 1000;
		layer_items[SF_freqLfo1].defv = -725;
		layer_items[SF_freqLfo2].defv = -15600;
	} else {
		layer_items[SF_sustainEnv1].defv = 0;
		layer_items[SF_sustainEnv2].defv = 0;
		layer_items[SF_freqLfo1].defv = 0;
		layer_items[SF_freqLfo2].defv = 0;
	}
}

/* initialize layer table */
static void clear_table(LayerTable *tbl)
{
	memset(tbl->val, 0, sizeof(tbl->val));
	memset(tbl->set, 0, sizeof(tbl->set));
}

/* set items in a layer to the table */
static void set_to_table(SFInfo *sf, LayerTable *tbl, SFGenLayer *lay, int level)
{
	int i;
	for (i = 0; i < lay->nlists; i++) {
		SFGenRec *gen = &lay->list[i];
		/* copy the value regardless of its copy policy */
		tbl->val[gen->oper] = gen->amount;
		tbl->set[gen->oper] = level;
	}
}

/* add an item to the table */
static void add_item_to_table(LayerTable *tbl, int oper, int amount, int level)
{
	LayerItem *item = &layer_items[oper];
	int o_lo, o_hi, lo, hi;

	switch (item->copy) {
	case L_INHRT:
		tbl->val[oper] += amount;
		break;
	case L_OVWRT:
		tbl->val[oper] = amount;
		break;
	case L_PRSET:
	case L_INSTR:
		/* do not overwrite */
		if (!tbl->set[oper])
			tbl->val[oper] = amount;
		break;
	case L_RANGE:
		if (!tbl->set[oper]) {
			tbl->val[oper] = amount;
		} else {
			o_lo = LOWNUM(tbl->val[oper]);
			o_hi = HIGHNUM(tbl->val[oper]);
			lo = LOWNUM(amount);
			hi = HIGHNUM(amount);
			if (lo < o_lo) lo = o_lo;
			if (hi > o_hi) hi = o_hi;
			tbl->val[oper] = RANGE(lo, hi);
		}
		break;
	}
}

/* merge two tables */
static void merge_table(SFInfo *sf, LayerTable *dst, LayerTable *src)
{
	int i;
	for (i = 0; i < SF_EOF; i++) {
		if (src->set[i]) {
			if (sf->version == 1) {
				if (!dst->set[i] ||
				    i == SF_keyRange || i == SF_velRange)
					/* just copy it */
					dst->val[i] = src->val[i];
			}
			else
				add_item_to_table(dst, i, src->val[i], P_GLOBAL);
			dst->set[i] = P_GLOBAL;
		}
	}
}

/* merge and set default values */
static void init_and_merge_table(SFInfo *sf, LayerTable *dst, LayerTable *src)
{
	int i;

	/* set default */
	for (i = 0; i < SF_EOF; i++) {
		if (!dst->set[i])
			dst->val[i] = layer_items[i].defv;
	}
	merge_table(sf, dst, src);
	/* convert from SBK to SF2 */
	if (sf->version == 1) {
		for (i = 0; i < SF_EOF; i++) {
			if (dst->set[i])
				dst->val[i] = sbk_to_sf2(i, dst->val[i]);
		}
	}
}


/*================================================================
 * check key and velocity range
 *================================================================*/

/* check if the table has sanity key/velocity ranges */
static int sanity_range(LayerTable *tbl)
{
	int lo, hi;

	lo = LOWNUM(tbl->val[SF_keyRange]);
	hi = HIGHNUM(tbl->val[SF_keyRange]);
	if (lo < 0 || lo > 127 || hi < 0 || hi > 127 || hi < lo)
		return 0;

	lo = LOWNUM(tbl->val[SF_velRange]);
	hi = HIGHNUM(tbl->val[SF_velRange]);
	if (lo < 0 || lo > 127 || hi < 0 || hi > 127 || hi < lo)
		return 0;

	return 1;
}

/* check if the given key is included in the table */
static int in_range(LayerTable *tbl, int key)
{
	int lo, hi;

	lo = LOWNUM(tbl->val[SF_keyRange]);
	hi = HIGHNUM(tbl->val[SF_keyRange]);
	if (key >= lo && key <= hi)
		return TRUE;
	return FALSE;
}



/*================================================================
 * sample mark up table
 *================================================================*/

/* linked list of sample record */
typedef struct _DynSample {
	int id;
	struct _DynSample *next;
} DynSample;

static int nsamples;
static DynSample *dynsample;

/* initialize lists */
static void awe_init_marks(void)
{
	nsamples = 0;
	dynsample = NULL;
}

/* free allocated lists */
static void awe_free_marks(void)
{
	DynSample *sp, *sp_next;

	for (sp = dynsample; sp; sp = sp_next) {
		sp_next = sp->next;
		safe_free(sp);
	}
}

/* mark the sample */
static void add_sample(int sample)
{
	DynSample *sp;
	for (sp = dynsample; sp; sp = sp->next) {
		if (sp->id == sample)
			return;
	}

	sp = (DynSample*)safe_malloc(sizeof(DynSample));
	sp->id = sample;
	sp->next = dynsample;
	dynsample = sp;
	nsamples++;
}

/* search the sample element with the specified id */
static int search_sample(int id)
{
	DynSample *sp;
	for (sp = dynsample; sp; sp = sp->next) {
		if (sp->id == id)
			return TRUE;
	}
	return FALSE;
}
