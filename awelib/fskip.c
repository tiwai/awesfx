/*----------------------------------------------------------------
 * skip file position
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

void fskip(int size, FILE *fd, int seekable)
{
	if (seekable)
		fseek(fd, size, SEEK_CUR);
	else {
		char tmp[1024];
		while (size >= (int)sizeof(tmp)) {
			size -= fread(tmp, 1, sizeof(tmp), fd);

		}
		while (size > 0)
			size -= fread(tmp, 1, size, fd);
	}
}


