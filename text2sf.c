/*================================================================
 * text2sf -- convert text to soundfont formmat
 *
 * Copyright (C) 1996-2000 Takashi Iwai
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
#include "util.h"
#include "sffile.h"
#include "awe_version.h"

int seqfd, awe_dev;
static SFInfo sfinfo;

static void usage()
{
	fprintf(stderr, "text2sf -- convert text file to soundfont format\n");
	fprintf(stderr, VERSION_NOTE);
	fprintf(stderr, "usage: text2sf text-file original-file output-file\n");
	fprintf(stderr, "    text-file: s-list text file via sf2text\n");
	fprintf(stderr, "    original-file: original soundfont file\n");
	fprintf(stderr, "    output-file: output soundfont file\n");
	exit(1);
}
	

int main(int argc, char **argv)
{
	FILE *fp, *fout;
	int piped;

	if (argc < 4) {
		usage();
		return 1;
	}

	if ((fp = CmpOpenFile(argv[1], &piped)) == NULL) {
		fprintf(stderr, "can't open text file %s\n", argv[1]);
		return 1;
	}
	awe_load_textinfo(&sfinfo, fp);
	CmpCloseFile(fp, piped);

	if ((fp = fopen(argv[2], "r")) == NULL) {
		fprintf(stderr, "can't open origianl file %s\n", argv[2]);
		return 1;
	}
	if ((fout = fopen(argv[3], "w")) == NULL) {
		fprintf(stderr, "can't open output file %s\n", argv[3]);
		return 1;
	}

	awe_save_soundfont(&sfinfo, fp, fout);

	fclose(fp);
	fclose(fout);

	/*awe_free_soundfont(&sfinfo);*/
	return 0;
}


