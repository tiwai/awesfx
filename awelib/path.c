/*----------------------------------------------------------------
 * search a file from path list
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
#include <unistd.h>
#include <string.h>
#include <sys/fcntl.h>
#include "util.h"

static int file_exists(char *path, char **ext)
{
	char *lastp;
	
	if (access(path, R_OK) == 0)
		return 1;
	if (ext == NULL) return 0;
	lastp = path + strlen(path);
	for (; *ext; ext++) {
		strcpy(lastp, *ext);
		if (access(path, R_OK) == 0)
			return 1;
	}
	return 0;
}

int awe_search_file_name(char *fresult, int maxlen, char *fname, char *pathlist, char **ext)
{
	char *tok;
	char *path;

	if (strlen(fname) >= maxlen)
		return 0;
	/* search the current path at first */
	strcpy(fresult, fname);
	if (file_exists(fresult, ext))
		return 1;

	/* then search along path list */
	if (fname[0] != '/' && pathlist && *pathlist) {
		path = safe_strdup(pathlist);
		for (tok = strtok(path, ":"); tok; tok = strtok(NULL, ":")) {
			if (*tok && tok[strlen(tok)-1] != '/')
				snprintf(fresult, maxlen, "%s/%s", tok, fname);
			else
				snprintf(fresult, maxlen, "%s%s", tok, fname);
			if (file_exists(fresult, ext)) {
				safe_free(path);
				return 1;
			}
		}
	}
	return 0;
}

