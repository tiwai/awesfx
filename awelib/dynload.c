/*----------------------------------------------------------------
 * partial loading mechanism
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
#include <ctype.h>
#include "util.h"
#include "aweseq.h"

static void set_preset(SFPatchRec *pat, char *arg);

/*----------------------------------------------------------------*/

LoadList *awe_add_loadlist(LoadList *list, SFPatchRec *pat, SFPatchRec *map)
{
	LoadList *rec;
	rec = (LoadList*)safe_malloc(sizeof(LoadList));
	rec->pat = *pat;
	if (map)
		rec->map = *map;
	else
		rec->map = *pat;
	rec->loaded  = FALSE;
	rec->next = list;
	return rec;
}

/* add the elements in the latter list to the former one */
LoadList *awe_merge_loadlist(LoadList *list, LoadList *old)
{
	LoadList *p;
	for (p = old; p; p = p->next) {
		list = awe_add_loadlist(list, &p->pat, &p->map);
		list->loaded  = p->loaded;
	}
	return list;
}

/* free all the elements in the list */
void awe_free_loadlist(LoadList *p)
{
	LoadList *next;
	for (; p; p = next) {
		next = p->next;
		safe_free(p);
	}
}

/* parse source and mapping presets and return the remaining string;
 * the original argument will be broken
 */
int awe_parse_loadlist(char *arg, SFPatchRec *pat, SFPatchRec *map, char **strp)
{
	char *next;

	if ((next = strschr(arg, ":=")) != NULL)
		*next++ = 0;
	set_preset(pat, arg);
	if (next == NULL) {
		*map = *pat;
		if (strp) *strp = NULL;
		return TRUE;
	}
	arg = next;
	if ((next = strschr(arg, ":=")) != NULL)
		*next++ = 0;
	set_preset(map, arg);
	if (strp) *strp = next;
	return TRUE;
}

/* ascii to digit; accept * and - characters */
static int parse_arg(char *arg)
{
	if (isdigit(*arg))
		return atoi(arg);
	else
		return -1;
}

/* parse preset/bank/keynote */
static void set_preset(SFPatchRec *pat, char *arg)
{
	char *next;
	if ((next = strschr(arg, "/: \t\n")) != NULL)
		*next++ = 0;
	pat->preset = parse_arg(arg);
	pat->bank = 0;
	pat->keynote = -1;
	arg = next;
	if (arg) {
		if ((next = strschr(arg, "/: \t\n")) != NULL)
			 *next++ = 0;
		pat->bank = parse_arg(arg);
		if (next)
			pat->keynote = parse_arg(next);
	}
}

/* check the preset matches to the given pattern (rec) */
int awe_match_preset(SFPatchRec *rec, SFPatchRec *pat)
{
	if (rec->preset != -1 && pat->preset != -1 &&
	    rec->preset != pat->preset)
		return FALSE;
	if (rec->bank != -1 && pat->bank != -1 &&
	    rec->bank != pat->bank)
		return FALSE;
	if (rec->keynote != -1 && pat->keynote != -1 &&
	    rec->keynote != pat->keynote)
		return FALSE;
	return TRUE;
}

/* merge two matching bank/preset/key records */
void awe_merge_keys(SFPatchRec *p1, SFPatchRec *p2, SFPatchRec *rec)
{
	rec->bank = (p1->bank == -1 ? p2->bank : p1->bank);
	rec->preset = (p1->preset == -1 ? p2->preset : p1->preset);
	rec->keynote = (p1->keynote == -1 ? p2->keynote : p1->keynote);
}


