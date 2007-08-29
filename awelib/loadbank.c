/*================================================================
 * load soundfont and virtual bank
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
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#ifdef linux
#include <linux/soundcard.h>
#else
#include <machine/soundcard.h>
#endif
#include <awe_voice.h>
#include "util.h"
#include "awebank.h"
#include "aweseq.h"
#include "sfopts.h"
#include "config.h"


/*----------------------------------------------------------------
 * prototypes
 *----------------------------------------------------------------*/

/* virtual bank record */
typedef struct _VBank {
	char *name;
	LoadList *list;
	struct _VBank *next;
} VBank;

static int is_virtual_bank(char *path);
static int load_virtual_bank(AWEOps *ops, char *path, LoadList *part_list, int locked);
static int load_patch(AWEOps *ops, char *path, LoadList *lp, LoadList *exlp, int locked, int load_alt);
static int load_map(AWEOps *ops, LoadList *lp, int locked);

static LoadList *make_virtual_list(VBank *v, LoadList *part_list);
static void make_bank_table(FILE *fp);
static void include_bank_table(char *name);
static void free_bank_table(void);

static int do_load_all_banks(AWEOps *ops, int locked);
static int do_load_banks(AWEOps *ops, int locked);
static void mark_loaded_presets(LoadList *pat, LoadList *list);
static LoadList *find_list(LoadList *list, SFPatchRec *pat);
static int find_matching_map(SFPatchRec *pat, int level);
static int find_matching_in_list(SFPatchRec *pat, LoadList *vlist, int level);



/*----------------------------------------------------------------*/
/* extensions to be searched */

static char *path_ext[] = {
	".sf2", ".SF2", ".sbk", ".SBK", NULL,
};
static char *path_ext_all[] = {
	".bnk", ".sf2", ".SF2", ".sbk", ".SBK", NULL,
};
static char *path_ext_bank[] = {
	".bnk", NULL,
};

/* search path for font and bank files */
static char *search_path;

/*----------------------------------------------------------------
 * load arbitrary file with specified loading list
 *----------------------------------------------------------------*/

int awe_load_bank(AWEOps *ops, char *name, LoadList *list, int locked)
{
	int rc;
	char sfpath[256];

	/* set default search path */
	if (awe_option.search_path)
		search_path = safe_strdup(awe_option.search_path);
	else {
		char *p = getenv("SFBANKDIR");
		if (p == NULL || *p == 0)
			p = DEFAULT_SF_PATH;
		search_path = safe_strdup(p);
	}

	if (awe_search_file_name(sfpath, sizeof(sfpath), name, search_path, path_ext_all)) {
		if (is_virtual_bank(sfpath))
			rc = load_virtual_bank(ops, sfpath, list, locked);
		else
			rc = load_patch(ops, name, list, NULL, locked, TRUE);
	} else
		rc = AWE_RET_NOT_FOUND;

	/* release search path */
	free(search_path);
	return rc;
}

/*----------------------------------------------------------------
 * virtual bank record handlers
 *----------------------------------------------------------------*/

static VBank *vbanks, *vmap, *vmapkey;
static char *default_font;
static LoadList *bank_list, *excl_list;


/* check the extension */
static int is_virtual_bank(char *path)
{
	char *p;
	if ((p = strrchr(path, '.')) == NULL)
		return FALSE;
	if (strcmp(p + 1, "bnk") == 0)
		return TRUE;
	return FALSE;
}

/* read a virtual bank config file and load the specified fonts */
static int load_virtual_bank(AWEOps *ops, char *path, LoadList *part_list, int locked)
{
	int rc;
	FILE *fp;

	if ((fp = fopen(path, "r")) == NULL) {
		fprintf(stderr, "awe: can't open virtual bank %s\n", path);
		return AWE_RET_ERR;
	}
	vbanks = NULL;
	vmap = vmapkey = NULL;
	default_font = NULL;
	bank_list = awe_merge_loadlist(NULL, part_list); /* copy the list */
	excl_list = NULL;

	make_bank_table(fp);
	fclose(fp);

	if (bank_list)
		rc = do_load_banks(ops, locked);  /* loading partial fonts */
	else
		rc = do_load_all_banks(ops, locked); /* loading all fonts */

	if (default_font)
		free(default_font);
	if (bank_list)
		awe_free_loadlist(bank_list);
	if (excl_list)
		awe_free_loadlist(excl_list);

	free_bank_table();

	return rc;
}
		
/* load the whole virtual banks */
static int do_load_all_banks(AWEOps *ops, int locked)
{
	int rc;
	VBank *v;

	if (vmap || vmapkey) {
		/* load preset mapping to the driver */
		if (vmap) {
			rc = load_map(ops, vmap->list, locked);
			if (rc == AWE_RET_ERR || rc == AWE_RET_NOMEM)
				return rc;
			/* add the loaded presets to exclusive list */
			excl_list = awe_merge_loadlist(excl_list, vmap->list);
		}
		if (vmapkey) {
			rc = load_map(ops, vmapkey->list, locked);
			if (rc == AWE_RET_ERR || rc == AWE_RET_NOMEM)
				return rc;
			/* add the loaded presets to exclusive list */
			excl_list = awe_merge_loadlist(excl_list, vmapkey->list);
		}
	}

	/* load each soundfont file */
	for (v = vbanks; v; v = v->next) {
		if (v->list == NULL)
			continue;
		/* append the defined instruments to exclusive list */
		excl_list = awe_merge_loadlist(excl_list, v->list);
		/* load this file */
		rc = load_patch(ops, v->name, v->list, NULL, locked, FALSE);
		if (rc == AWE_RET_ERR && rc == AWE_RET_NOMEM)
			return rc;
	}

	if (default_font)
		/* load all the fonts except for pre-loaded fonts */
		return load_patch(ops, default_font, NULL, excl_list, locked, TRUE);

	return AWE_RET_OK;
}

/* load banks by dynamic loading */
static int do_load_banks(AWEOps *ops, int locked)
{
	int rc;
	LoadList *vlist, *p;
	VBank *v;

	/* load preset mapping if any.. */
	if (vmap) {
		rc = load_map(ops, vmap->list, locked);
		if (rc == AWE_RET_ERR || rc == AWE_RET_NOMEM)
			return rc;
	}
	if (vmapkey) {
		rc = load_map(ops, vmapkey->list, locked);
		if (rc == AWE_RET_ERR || rc == AWE_RET_NOMEM)
			return rc;
	}

	/* mark the matching presets in the list */
	for (p = bank_list; p; p = p->next) {
		if (! p->loaded) {
			if (find_matching_map(&p->map, 0))
				p->loaded = TRUE;
		}
	}

	/* load each soundfont file */
	for (v = vbanks; v; v = v->next) {
		vlist = make_virtual_list(v, bank_list);
		if (vlist == NULL)
			continue;

		rc = load_patch(ops, v->name, vlist, NULL, locked, TRUE);
		awe_free_loadlist(vlist);
		if (rc == AWE_RET_ERR && rc == AWE_RET_NOMEM)
			return rc;

		mark_loaded_presets(vlist, bank_list);
	}

	if (default_font) {
		/* load rest of font lists */

		/* make the list of fonts not loaded */
		vlist = NULL;
		for (p = bank_list; p; p = p->next) {
			if (!p->loaded)
				vlist = awe_add_loadlist(vlist, &p->pat, &p->map);
		}
			
		if (vlist == NULL) /* all fonts have been loaded */
			return AWE_RET_OK;

		/* load them */
		rc = load_patch(ops, default_font, vlist, NULL, locked, TRUE);
		awe_free_loadlist(vlist);

		return rc;
	}

	return AWE_RET_OK;
}


/* mark the presets as actually loaded */
static void mark_loaded_presets(LoadList *pat, LoadList *list)
{
	LoadList *p, *q;
	for (p = pat; p; p = p->next) {
		if (! p->loaded)
			continue;
		for (q = list; q; q = q->next) {
			if (!q->loaded && awe_match_preset(&p->map, &q->map))
				q->loaded = TRUE;
		}
	}
}


/* search the matching element with pat */
static LoadList *find_list(LoadList *list, SFPatchRec *pat)
{
	LoadList *p;
	for (p = list; p; p = p->next) {
		if (awe_match_preset(&p->map, pat))
			return p;
	}
	return NULL;
}

/* if the given pattern matches to an element in the given list,
 * reutrn TRUE.  the mapped pattern is recursively searched.
 * if additional instrument is necessary to be loaded, prepend it
 * to the loading list.
 */
static int find_matching_in_list(SFPatchRec *pat, LoadList *vlist, int level)
{
	LoadList *p, *curp;
	SFPatchRec tmp;
	for (p = vlist; p; p = p->next) {
		if (! p->loaded)
			continue;
		if (awe_match_preset(&p->map, pat)) {
			if (find_list(bank_list, &p->pat))
				return TRUE;
			tmp = p->pat;
			if (tmp.keynote == -1)
				tmp.keynote = pat->keynote;
			bank_list = curp = awe_add_loadlist(bank_list, &tmp, NULL);
			if (find_matching_map(&curp->map, level+1))
				curp->loaded = TRUE;
			return TRUE;
		}
	}
	return FALSE;
}


#define MAX_RECURSIVE_LEVEL	10

/* if the given pattern matches to an element in mapping list,
 * return TRUE.  the mapped pattern will be further searched
 * in the mapping list.
 */
static int find_matching_map(SFPatchRec *pat, int level)
{
	if (level >= MAX_RECURSIVE_LEVEL)
		return TRUE;

	if (vmapkey) {
		if (find_matching_in_list(pat, vmapkey->list, level))
			return TRUE;
	}
	if (vmap) {
		if (find_matching_in_list(pat, vmap->list, level))
			return TRUE;
	}
	return FALSE;
}


/* make part list for each virtual bank */
static LoadList *make_virtual_list(VBank *v, LoadList *part_list)
{
	LoadList *p, *q, *list;
	list = NULL;
	for (p = part_list; p; p = p->next) {
		if (p->loaded)
			continue;
		for (q = v->list; q; q = q->next) {
			if (awe_match_preset(&p->pat, &q->map)) {
				SFPatchRec src, map;
				awe_merge_keys(&q->pat, &p->pat, &src);
				awe_merge_keys(&q->map, &p->map, &map);
				list = awe_add_loadlist(list, &src, &map);
			}
		}
	}
	return list;
}


/* read a virtual bank config file and build bank table */
static void make_bank_table(FILE *fp)
{
	char line[256], *p, *name;
	SFPatchRec pat, map;
	VBank *v;
	int len;

	while (fgets(line, sizeof(line), fp)) {
		/* discard the linefeed */
		len = strlen(line);
		if (len > 0 && line[len-1] == '\n') line[len-1] = 0; 
		/* skip spaces & comments */
		for (p = line; isspace(*p); p++)
			;
		if (!*p || *p == '#' || *p == '!' || *p == '%')
			continue;

		if (isalpha(*p)) {
			char *arg;

			arg = strschr(p, " \t\r\n");
			if (arg) {
				char *tmp = strschr(arg, " \t\r\n");
				if (tmp) *tmp = 0;
				arg++;
			}
			
			switch (*p) {
			case 'i':
			case 'I':
				/* include other bank file */
				include_bank_table(arg);
				break;
			case 'd':
			case 'D':
				/* set default font file */
				if (default_font) free(default_font);
				default_font = safe_strdup(arg);
				break;
			default:
				fprintf(stderr, "awe: illegal bank command %s", line);
				break;
			}
			continue;
		} else if (*p == '*' || *p == '-' || isdigit(*p)) {
			if (! awe_parse_loadlist(p, &pat, &map, &name))
				continue;
		} else
			continue;

		if (name && *name) {
			/* discard garbage letters */
			if ((p = strschr(name, " \t\n#")) != NULL)
				*p = 0;
		}

		if (name == NULL || !*name) {
			/* without font name -- this is a preset link */
			if (map.keynote != -1)
				v = vmapkey;
			else
				v = vmap;
			if (v == NULL) {
				/* the list is not created.. */
				v = (VBank*)safe_malloc(sizeof(VBank));
				v->list = NULL;
				v->name = NULL;
				if (map.keynote != -1)
					vmapkey = v;
				else
					vmap = v;
			}

		} else {
			/* search the list matching the font name */
			for (v = vbanks; v; v = v->next) {
				if (strcmp(v->name, name) == 0)
					break;
			}
			if (v == NULL) {
				/* not found -- create a new list */
				v = (VBank*)safe_malloc(sizeof(VBank));
				v->list = NULL;
				v->name = safe_strdup(name);
				v->next = vbanks;
				vbanks = v;
			}
		}

		/* append the current record */
		v->list = awe_add_loadlist(v->list, &pat, &map);
	}
}


/* include another bank file */
static void include_bank_table(char *name)
{
	char path[256];
	FILE *fp;
	if (awe_search_file_name(path, sizeof(path), name, search_path, path_ext_bank) &&
	    (fp = fopen(path, "r")) != NULL) {
		make_bank_table(fp);
		fclose(fp);
	} else {
		fprintf(stderr, "awe: can't include bank file %s\n", name);
	}
}


/* free bank record table */
static void free_bank_table(void)
{
	VBank *v, *next;
	for (v = vbanks; v; v = next) {
		next = v->next;
		if (v->name)
			safe_free(v->name);
		awe_free_loadlist(v->list);
		safe_free(v);
	}
	vbanks = NULL;
	if (vmap) {
		/* this has no name */
		awe_free_loadlist(vmap->list);
		safe_free(vmap);
	}
	if (vmapkey) {
		/* this has no name */
		awe_free_loadlist(vmapkey->list);
		safe_free(vmapkey);
	}
}


/*----------------------------------------------------------------
 * load sample & info on the sound driver
 *----------------------------------------------------------------
 * name = file name of soudnfont file (without path)
 * lp = loading list (NULL = all)
 * exlp = excluding list (NULL = none)
 * locked = flag for remove lock
 * load_alt = load altenatives for missing fonts
 *----------------------------------------------------------------*/

static int load_patch(AWEOps *ops, char *name, LoadList *lp, LoadList *exlp, int locked, int load_alt)
{
	FILE *fd;
	char path[256];
	int rc;
	static SFInfo sfinfo;

	if (! awe_search_file_name(path, sizeof(path), name, search_path, path_ext)) {
		fprintf(stderr, "awe: can't find font file %s\n", name);
		return AWE_RET_SKIP;
	}
	if ((fd = fopen(path, "r")) == NULL) {
		fprintf(stderr, "awe: can't open SoundFont file %s\n", path);
		return AWE_RET_SKIP;
	}
	if (awe_load_soundfont(&sfinfo, fd, TRUE) < 0) {
		fprintf(stderr, "awe: can't load SoundFont %s\n", path);
		return AWE_RET_SKIP;
	}
	awe_correct_samples(&sfinfo);

	awe_open_font(ops, &sfinfo, fd, locked);
	/*rc = awe_load_font_buffered(&sfinfo, lp, exlp, load_alt);*/
	if (lp)
		rc = awe_load_font_list(ops, &sfinfo, lp, load_alt);
	else
		rc = awe_load_all_fonts(ops, &sfinfo, exlp);

	awe_close_font(ops, &sfinfo);
	awe_free_soundfont(&sfinfo);
	if (fd)
		fclose(fd);

	return rc;
}


/*----------------------------------------------------------------
 * load preset links
 *----------------------------------------------------------------*/

static int load_map(AWEOps *ops, LoadList *lp, int locked)
{
	int rc;
	if (awe_open_patch(ops, "*MAP*", AWE_PAT_TYPE_MAP, locked) < 0) {
		fprintf(stderr, "awe: can't access to sequencer\n");
		return AWE_RET_SKIP;
	}

	for (; lp; lp = lp->next) {
		if ((rc = awe_load_map(ops, lp)) != AWE_RET_OK)
			return rc;
		lp->loaded = TRUE;
	}
	awe_close_patch(ops);
	return AWE_RET_OK;
}

