/*================================================================
 * gusload -- load a GUS patch file on AWE32 sound driver
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

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/fcntl.h>
#ifdef __FreeBSD__
#  include <machine/soundcard.h>
#elif defined(linux)
#  include <linux/soundcard.h>
#endif
#include "guspatch.h"
#include "seq.h"
#include "util.h"
#include "awe_version.h"

void seq_load_gus(FILE *fd);


static void usage()
{
	fprintf(stderr, "gusload -- load GUS patch file on AWE32 sound driver\n");
	fprintf(stderr, VERSION_NOTE);
	fprintf(stderr, "usage: gusload [-options] GUSpatch\n");
	fprintf(stderr, " -v          verbose mode\n");
	fprintf(stderr, " -p number   set instrument number (default is internal value)\n");
	fprintf(stderr, " -b number   set bank number (default is 0)\n");
	fprintf(stderr, " -k keynote  set fixed keynote\n");
	exit(1);
}

static int clear_sample = FALSE;
static int preset = -1;
static int bankchange = -1;
static int keynote = -1;
int awe_verbose;

#define OPTION_FLAGS	"b:p:k:viF:D:"

int main(int argc, char **argv)
{
	FILE *fd;
	char *gusfile;
	int c;
	char *seq_devname = NULL;
	int seq_devidx = -1;

	while ((c = getopt(argc, argv, OPTION_FLAGS)) != -1) {
		switch (c) {
		case 'F':
			seq_devname = optarg;
			break;
		case 'D':
			seq_devidx = atoi(optarg);
			break;
		case 'v':
			if (optarg)
				awe_verbose = atoi(optarg);
			else
				awe_verbose++;
			break;
		case 'b':
			bankchange = atoi(optarg);
			break;
		case 'p':
			preset = atoi(optarg);
			break;
		case 'k':
			keynote = atoi(optarg);
			break;
		case 'i':
			clear_sample = TRUE;
			break;
		default:
			usage();
			return 1;
		}
	}
		
	if (argc - optind < 1) {
		usage();
		return 1;
	}

	gusfile = argv[optind]; optind++;
	if ((fd = fopen(gusfile, "r")) == NULL) {
		fprintf(stderr, "can't open GUS patch file %s\n", gusfile);
		return 1;
	}

	/* open awe sequencer device */
	seq_init(seq_devname, seq_devidx);
	if (clear_sample)
		seq_reset_samples();

	DEBUG(0,fprintf(stderr, "uploading samples..\n"));
	seq_load_gus(fd);

	DEBUG(0,printf("DRAM memory left = %d kB\n", seq_mem_avail()/1024));

	/* close sequencer */
	seq_end();
	fclose(fd);

	return 0;
}

static GusPatchHeader header;
static GusInstrument ins;
static GusLayerData layer;
static GusPatchData sample;

#define freq_to_note(mhz)	(int)(log((double)mhz / 8176.0) / log(2.0) * 1200.0)

/*
 * load voice record to the awe driver
 */
void seq_load_gus(FILE *fp)
{
	int j, len;
	struct patch_info *patch;

	/* Unix based routines, assume big-endian machine */
	/* read header */
	DEBUG(1,fprintf(stderr, "reading header\n"));
	fread(&header.header, sizeof(header.header), 1, fp);
	fread(&header.gravis_id, sizeof(header.gravis_id), 1, fp);
	fread(&header.description, sizeof(header.description), 1, fp);
	fread(&header.instruments, sizeof(header.instruments), 1, fp);
	fread(&header.voices, sizeof(header.voices), 1, fp);
	fread(&header.channels, sizeof(header.channels), 1, fp);
	fread(&header.wave_forms, sizeof(header.wave_forms), 1, fp);
	header.wave_forms = swapi( header.wave_forms );
	fread(&header.master_volume, sizeof(header.master_volume), 1, fp);
	header.master_volume = swapi( header.master_volume );
	fread(&header.data_size, sizeof(header.data_size), 1, fp);
	header.data_size = swapl( header.data_size );
	fread(&header.reserved, sizeof(header.reserved), 1, fp);
	/* read instrument header */
	DEBUG(1,fprintf(stderr, "reading instrument\n"));
	fread(&ins.instrument, sizeof(ins.instrument), 1, fp);
	fread(&ins.instrument_name, sizeof(ins.instrument_name), 1, fp);
	fread(&ins.instrument_size, sizeof(ins.instrument_size), 1, fp);
	ins.instrument_size = swapl( ins.instrument_size );
	fread(&ins.layers, sizeof(ins.layers), 1, fp);
	fread(&ins.reserved, sizeof(ins.reserved), 1, fp);
	/* read layer header */
	DEBUG(1,fprintf(stderr, "reading layer\n"));
	fread(&layer.layer_duplicate, sizeof(layer.layer_duplicate), 1, fp);
	fread(&layer.layer, sizeof(layer.layer), 1, fp);
	fread(&layer.layer_size, sizeof(layer.layer_size), 1, fp);
	layer.layer_size = swapl( layer.layer_size );
	fread(&layer.samples, sizeof(layer.samples), 1, fp);
	fread(&layer.reserved, sizeof(layer.reserved), 1, fp);
	if (strcmp(header.gravis_id, "ID#000002") != 0) {
		fprintf(stderr, "Not a GUS patch file\n");
		exit(1);
	}

	DEBUG(0,fprintf(stderr, "data size = %d\n", (int)header.data_size));

	/* read sample information */
	for (j = 0; j < layer.samples; j++) {
		/* read sample information */
		DEBUG(1,fprintf(stderr, "reading sample(%d)\n", j));
		fread(&sample.wave_name, sizeof(sample.wave_name), 1, fp);
		fread(&sample.fractions, sizeof(sample.fractions), 1, fp);
		fread(&sample.wave_size, sizeof(sample.wave_size), 1, fp);
		sample.wave_size = swapl( sample.wave_size );
		fread(&sample.start_loop, sizeof(sample.start_loop), 1, fp);
		fread(&sample.end_loop, sizeof(sample.end_loop), 1, fp);
		fread(&sample.sample_rate, sizeof(sample.sample_rate), 1, fp);
		sample.sample_rate = swapi( sample.sample_rate );
		fread(&sample.low_frequency, sizeof(sample.low_frequency), 1, fp);
		fread(&sample.high_frequency, sizeof(sample.high_frequency), 1, fp);
		fread(&sample.root_frequency, sizeof(sample.root_frequency), 1, fp);
		fread(&sample.tune, sizeof(sample.tune), 1, fp);
		fread(&sample.balance, sizeof(sample.balance), 1, fp);
		fread(&sample.envelope_rate, sizeof(sample.envelope_rate), 1, fp);
		fread(&sample.envelope_offset, sizeof(sample.envelope_offset), 1, fp);
		fread(&sample.tremolo_sweep, sizeof(sample.tremolo_sweep), 1, fp);
		fread(&sample.tremolo_rate, sizeof(sample.tremolo_rate), 1, fp);
		fread(&sample.tremolo_depth, sizeof(sample.tremolo_depth), 1, fp);
		fread(&sample.vibrato_sweep, sizeof(sample.vibrato_sweep), 1, fp);
		fread(&sample.vibrato_rate, sizeof(sample.vibrato_rate), 1, fp);
		fread(&sample.vibrato_depth, sizeof(sample.vibrato_depth), 1, fp);
		fread(&sample.modes, sizeof(sample.modes), 1, fp);
		fread(&sample.scale_frequency, sizeof(sample.scale_frequency), 1, fp);
		fread(&sample.scale_factor, sizeof(sample.scale_factor), 1, fp);
		sample.scale_factor = swapi( sample.scale_factor );
		fread(&sample.reserved, sizeof(sample.reserved), 1, fp);

		DEBUG(1,fprintf(stderr, "-- sample len = %d\n",
				(int)sample.wave_size));
		/* allocate sound driver patch data */
		len = sizeof(struct patch_info) + sample.wave_size - 1;
		patch = (struct patch_info*)calloc(len, 1);
		if (patch == NULL) {
			fprintf(stderr, "can't allocate patch buffer\n");
			exit(1);
		}
		patch->key = GUS_PATCH;
		patch->device_no = awe_dev;
		if (preset >= 0)
			patch->instr_no = preset;
		else
			patch->instr_no = ins.instrument;
		DEBUG(0,fprintf(stderr,"-- preset=%d\n", patch->instr_no));
		patch->mode = sample.modes;
		DEBUG(0,fprintf(stderr,"-- sample_mode=0x%x\n", patch->mode));
		patch->len = sample.wave_size;
		patch->loop_start = sample.start_loop;
		patch->loop_end = sample.end_loop;
		DEBUG(1,fprintf(stderr,"-- loop position=%d/%d\n", patch->loop_start, patch->loop_end));
		patch->base_freq = sample.sample_rate;
		if (keynote != -1) {
			/*int note = freq_to_note(sample.root_frequency);*/
			int low = freq_to_note(sample.low_frequency);
			int high = freq_to_note(sample.high_frequency);
			if (keynote < low / 100 || high / 100 < keynote)
				continue;
			low = (low / 100) * 100;
			patch->base_note = (int)(pow(2.0, (double)(keynote * 100 - low) / 1200.0) * sample.root_frequency);
			patch->low_note = (int)(pow(2.0, (double)keynote / 12.0) * 8176.0);
			patch->high_note = patch->low_note;
		} else {
			patch->base_note = sample.root_frequency;
			patch->high_note = sample.high_frequency;
			patch->low_note = sample.low_frequency;
		}
		DEBUG(0,fprintf(stderr,"-- base freq=%d, note=%d[%d] (%d[%d]-%d[%d])\n",
				(int)patch->base_freq, (int)patch->base_note,
				freq_to_note(patch->base_note)/100,
				(int)patch->low_note,
				freq_to_note(patch->low_note)/100,
				(int)patch->high_note,
				freq_to_note(patch->high_note)/100));
		patch->panning = sample.balance;
		patch->detuning = sample.tune;
		memcpy(patch->env_rate, sample.envelope_rate, 6);
		memcpy(patch->env_offset, sample.envelope_offset, 6);
		if (sample.tremolo_rate > 0 && sample.tremolo_depth > 0)
			patch->mode |= WAVE_TREMOLO;
		patch->tremolo_sweep = sample.tremolo_sweep;
		patch->tremolo_rate = sample.tremolo_rate;
		patch->tremolo_depth = sample.tremolo_depth;
		DEBUG(0,fprintf(stderr,"-- tremolo rate=%d, depth=%d\n",
		      patch->tremolo_rate, patch->tremolo_depth));
		if (sample.vibrato_rate > 0 && sample.vibrato_depth > 0)
			patch->mode |= WAVE_VIBRATO;
		patch->vibrato_sweep = sample.vibrato_sweep;
		patch->vibrato_rate = sample.vibrato_rate;
		patch->vibrato_depth = sample.vibrato_depth;
		DEBUG(0,fprintf(stderr,"-- vibrato rate=%d, depth=%d\n",
		      patch->vibrato_rate, patch->vibrato_depth));
		patch->scale_frequency = sample.scale_frequency;
		patch->scale_factor = sample.scale_factor;
		patch->volume = header.master_volume;
#if SOUND_VERSION > 301
		patch->fractions = sample.fractions;
#endif
		/* allocate raw sample buffer */
		DEBUG(1,fprintf(stderr, "-- reading wave data\n"));
		fread(patch->data, 1, patch->len, fp);

		/* if -b option is specified, change the bank value */
		if (bankchange >= 0) {
			DEBUG(1,fprintf(stderr, "-- set bank number\n"));
			seq_set_gus_bank(bankchange);
		}

		DEBUG(1,fprintf(stderr, "-- transferring\n"));
		if (write(seqfd, patch, len) == -1) {
			fprintf(stderr, "[Loading GUS %d]\n", j);
			perror("Error in loading info");
			exit(1);
		}

		/* free temporary buffer */
		free(patch);

		/* exit loop if keynote is fixed */
		if (keynote != -1)
			break;
	}
}

