/*================================================================
 * AWElib patch loader
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

#ifndef AWEBANK_H_DEF
#define AWEBANK_H_DEF

/* return value */
#define AWE_RET_OK		0	/* successfully loaded */
#define AWE_RET_ERR		1	/* some fatal error occurs */
#define AWE_RET_SKIP		2	/* some fonts are skipped */
#define AWE_RET_NOMEM		3	/* out or memory; not all fonts loaded */
#define AWE_RET_NOT_FOUND	4	/* the file is not found */


/*----------------------------------------------------------------
 * operators
 *----------------------------------------------------------------*/

typedef struct _AWEOps {
	int (*load_patch)(void *buf, int len);
	int (*mem_avail)(void);
	int (*reset_samples)(void);
	int (*remove_samples)(void);
	int (*set_zero_atten)(int val);
} AWEOps;


/*----------------------------------------------------------------
 * preset/bank/keynote bag
 *----------------------------------------------------------------*/

typedef struct _SFPatchRec {
	int preset, bank, keynote; /* -1 = matches all */
} SFPatchRec;

/* compare two patch sets */
int awe_match_preset(SFPatchRec *rec, SFPatchRec *pat);
/* merge two patch sets (p1 + p2 -> rec) */
void awe_merge_keys(SFPatchRec *p1, SFPatchRec *p2, SFPatchRec *rec);

/* parse the text and store source/map patch sets */
int awe_parse_loadlist(char *arg, SFPatchRec *pat, SFPatchRec *map, char **strp);


/* dynamic loading list */
typedef struct _AWELoadList {
	SFPatchRec pat;
	SFPatchRec map;
	int loaded;
	struct _AWELoadList *next;
} LoadList;

/* add a patch set on the list */
LoadList *awe_add_loadlist(LoadList *list, SFPatchRec *pat, SFPatchRec *map);
/* merge the elements in latter list */
LoadList *awe_merge_loadlist(LoadList *list, LoadList *old);
/* free all list elements */
void awe_free_loadlist(LoadList *p);

/*----------------------------------------------------------------
 * load a soundfont or virtual bank file
 *----------------------------------------------------------------
 * 'name' is the file name of the sound font.  The file is searched
 * along the prescribed search path.
 * If 'list' is not NULL, only the presets listed are loaded.
 * 'locked' is boolean value to specify the fonts are locked or not.
 * The locked fonts are not removed by -x option of sfxload.
 *----------------------------------------------------------------*/

int awe_load_bank(AWEOps *ops, char *name, LoadList *list, int locked);


#endif	/* AWEBANK_H_DEF */
