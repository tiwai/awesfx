/*================================================================
 * cmpopen.c:
 *	search / open a compressed file
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
#include <unistd.h>
#include "util.h"

typedef struct _DecompRec {
	char *ext;
	char *format;
} DecompRec;

typedef struct _DecompList {
	DecompRec v;
	struct _DecompList *next;
} DecompList;

static DecompRec declist[] = {
	{".gz", "gunzip -c"},
	{".z", "gunzip -c"},
	{".Z", "zcat"},
	{".zip", "unzip -p"},
	{".lha", "lha -pq"},
	{".lzh", "lha -pq"},
	{".bz2", "bzip2 -d -c"},
};

static DecompList *decopts = NULL;


void CmpAddList(char *ext, char *format)
{
	DecompList *rec;

	rec = (DecompList*)safe_malloc(sizeof(DecompList));
	if (*ext != '.') {
		rec->v.ext = (char*)safe_malloc(strlen(ext) + 2);
		rec->v.ext[0] = '.';
		strcpy(rec->v.ext + 1, ext);
	} else
		rec->v.ext = safe_strdup(ext);
	rec->v.format = safe_strdup(format);
	rec->next = decopts;
	decopts = rec;
}

static int CheckExt(char *name, int len, DecompRec *p)
{
	int exlen = strlen(p->ext);
	if (len > exlen && strcmp(name + (len - exlen), p->ext) == 0)
		return TRUE;
	else
		return FALSE;
}

static DecompRec *CmpSearchFile(char *name)
{
	int i, len;
	DecompList *p;

	if (access(name, R_OK) != 0)
		return NULL;

	len = strlen(name);
	for (p = decopts; p; p = p->next) {
		if (CheckExt(name, len, &p->v))
			return &p->v;
	}
	for (i = 0; i < numberof(declist); i++) {
		if (CheckExt(name, len, &declist[i]))
			return &declist[i];
	}
	return NULL;
}

char *CmpGetExtension(char *name)
{
	DecompRec *rec;

	if ((rec = CmpSearchFile(name)) != NULL) {
		return name + strlen(name) - strlen(rec->ext);
	}
	return strrchr(name, '.');
}

FILE *CmpOpenFile(char *name, int *flag)
{
	FILE *fp;
	DecompRec *rec;
	char str[256];

	*flag = 0;
	if (strcmp(name, "-") == 0) {
		/* use standard input */
		*flag = 2;
		return stdin;
	}
	if ((rec = CmpSearchFile(name)) != NULL) {
		if (strstr(rec->format, "%s") != NULL)
			sprintf(str, rec->format, name);
		else
			sprintf(str, "%s \"%s\"", rec->format, name);
		if ((fp = popen(str, "r")) != NULL) {
			*flag = 1;
			return fp;
		}
	}

	return fopen(name, "r");
}

void CmpCloseFile(FILE *fp, int flag)
{
	switch (flag) {
	case 0:
		fclose(fp); break;
	case 1:
		pclose(fp); break;
	}
}
