/*================================================================
 * safe malloc routine
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
#include <unistd.h>

void *safe_malloc(int size)
{
	void *p;
	p = (void*)calloc(size, 1);
	if (p == NULL) {
		fprintf(stderr, "can't malloc buffer for size %d!!\n", size);
		exit(1);
	}
	return p;
}

void safe_free(void *buf)
{
	if (buf)
		free(buf);
}

char *safe_strdup(char *src)
{
	char *p;
	if ((p = strdup(src)) == NULL) {
		fprintf(stderr, "can't strdup\n");
		exit(1);
	}
	return p;
}

