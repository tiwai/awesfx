/*================================================================
 * Load AWE patch data
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

#ifndef AWESEQ_H_DEF
#define AWESEQ_H_DEF

#include "sffile.h"
#include "awebank.h"

/*----------------------------------------------------------------
 * initialize and finish the loading
 *----------------------------------------------------------------*/

int awe_open_font(AWEOps *ops, SFInfo *sf, FILE *fp, int locked);
void awe_close_font(AWEOps *ops, SFInfo *sf);
int awe_is_ram_fonts(SFInfo *sf);

int awe_open_patch(AWEOps *ops, char *name, int type, int locked);
int awe_close_patch(AWEOps *ops);
int awe_load_map(AWEOps *ops, LoadList *lp);

/*----------------------------------------------------------------
 * load each preset: load a preset "pat" on "map"
 *----------------------------------------------------------------*/

int awe_load_font(AWEOps *ops, SFInfo *sf, SFPatchRec *pat, SFPatchRec *map);
int awe_load_font_list(AWEOps *ops, SFInfo *sf, LoadList *part_list, int load_alt);


/*----------------------------------------------------------------
 * load the whole file (with checking loadlist)
 *----------------------------------------------------------------*/

int awe_load_all_fonts(AWEOps *ops, SFInfo *sf, LoadList *exclude);


#endif	/* AWESEQ_H_DEF */
