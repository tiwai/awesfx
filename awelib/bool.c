/*================================================================
 * check boolean string
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
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include "util.h"

/* search delimiers */
char *strschr(char *str, char *dels)
{
	char *p;
	for (p = str; *p; p++) {
		if (strchr(dels, *p))
			return p;
	}
	return NULL;
}

/* case insensitive comparison */
int strlcmp(char *ap, char *bp)
{
	int a, b;
	for (;;) {
		a = tolower(*ap);
		b = tolower(*bp);
		if (a != b)
			return (a - b);
		if (a == 0)
			return 0;
		ap++;
		bp++;
	}
}

/* check boolean string; on, true, yes */
int bool_val(char *val)
{
	if (strlcmp(val, "on") == 0 || strlcmp(val, "true") == 0 ||
	    strlcmp(val, "yes") == 0)
		return 1;
	else if (strlcmp(val, "off") == 0 || strlcmp(val, "false") == 0 ||
		 strlcmp(val, "no") == 0)
		return 0;
	else
		return (int)strtol(val, NULL, 0);
}

/*================================================================
 * get a token separated by space letters
 * almost equal with strtok(str, spaces) but this checks quote
 * letters.
 *================================================================*/

#define DELIM	" \t\n\r"

static void remove_letter(char *p)
{
	if (! *p)
		return;
	for (;; p++) {
		*p = p[1];
		if (! *p) return;
	}
}

char *strtoken(char *src)
{
	static char *buf = NULL, *vptr;
	char *retptr;

	if (src) {
		if (buf) free(buf);
		buf = safe_strdup(src);
		vptr = buf;
	} else if (buf == NULL) {
		fprintf(stderr, "illegal strtoken call\n");
		exit(1);
	}

	/* skip delimiters at head */
	while (*vptr && strchr(DELIM, *vptr))
		vptr++;
	if (!*vptr)
		return NULL;

	retptr = vptr;
	while (*vptr && strchr(DELIM, *vptr) == NULL) {
		if (*vptr == '\\') {
			remove_letter(vptr);
			if (!*vptr)
				break;
			vptr++;
		} else if (*vptr == '"' || *vptr == '\'') {
			int prev, quote;
			prev = quote = *vptr;
			remove_letter(vptr);
			for (; *vptr; prev = *vptr, vptr++) {
				if (*vptr == '\\') {
					remove_letter(vptr);
					if (*vptr)
						break;
				} else if (*vptr == quote) {
					remove_letter(vptr);
					break;
				}
			}
		} else
			vptr++;
	}
	if (*vptr)
		*vptr++ = 0;
	return retptr;
}

