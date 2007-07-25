/*================================================================
 * seq.c
 *	sequencer control routine for awe sound driver
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

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <math.h>
#include <fcntl.h>
#ifdef __FreeBSD__
#  include <machine/soundcard.h>
#elif defined(linux)
#  include <linux/soundcard.h>
#endif
#include <awe_voice.h>
#include <util.h>
#include "seq.h"

SEQ_DEFINEBUF(128);
int seqfd;

void seqbuf_dump()
{
	if (_seqbufptr)
		if (write(seqfd, _seqbuf, _seqbufptr) == -1) {
			perror("write device");
			exit(-1);
		}
	_seqbufptr = 0;
}


#define MAX_CARDS	16
int awe_dev;
/*int max_synth_voices;*/

static void seq_init_priv(char *devname, int devidx);

void seq_init(char *devname, int devidx)
{
	if (devname == NULL)
		seq_init_priv(OSS_SEQUENCER_DEV, devidx);
	else
		seq_init_priv(devname, devidx);
}

static void seq_init_priv(char *devname, int devidx)
{
	int i;
	int nrsynths;
	struct synth_info card_info;

	if ((seqfd = open(devname, O_WRONLY, 0)) < 0) {
		perror(devname);
		exit(1);
	}

	if (ioctl(seqfd, SNDCTL_SEQ_NRSYNTHS, &nrsynths) == -1) {
		fprintf(stderr, "there is no soundcard\n");
		exit(1);
	}

	if (devidx >= 0) {
		card_info.device = devidx;
		if (ioctl(seqfd, SNDCTL_SYNTH_INFO, &card_info) < 0 ||
		    card_info.synth_type != SYNTH_TYPE_SAMPLE ||
		    card_info.synth_subtype != SAMPLE_TYPE_AWE32) {
			fprintf(stderr, "invalid soundcard (device = %s, index = %d)\n", devname, devidx);
			exit(1);
		}
		awe_dev = devidx;
	} else {
		/* auto probe */
		awe_dev = -1;
		for (i = 0; i < nrsynths; i++) {
			card_info.device = i;
			if (ioctl(seqfd, SNDCTL_SYNTH_INFO, &card_info) == -1) {
				fprintf(stderr, "cannot get info on soundcard\n");
				perror(devname);
				exit(1);
			}
			if (card_info.synth_type == SYNTH_TYPE_SAMPLE
			    && card_info.synth_subtype == SAMPLE_TYPE_AWE32) {
				awe_dev = i;
				/*max_synth_voices = card_info.nr_voices;*/
				break;
			}
		}
		if (awe_dev < 0) {
			fprintf(stderr, "No AWE synth device is found\n");
			exit(1);
		}
	}
}

int seq_reset_samples(void)
{
	if (ioctl(seqfd, SNDCTL_SEQ_RESETSAMPLES, &awe_dev) == -1) {
		perror("Sample reset");
		exit(1);
	}
	return 0;
}

void seq_end(void)
{
	SEQ_DUMPBUF();
	/*ioctl(seqfd, SNDCTL_SEQ_SYNC);*/
	close(seqfd);
}

int seq_remove_samples(void)
{
	AWE_REMOVE_LAST_SAMPLES(seqfd, awe_dev);
	return 0;
}

void seq_default_atten(int val)
{
	AWE_MISC_MODE(awe_dev, AWE_MD_ZERO_ATTEN, val);
	SEQ_DUMPBUF();
}

void seq_set_gus_bank(int bank)
{
	AWE_SET_GUS_BANK(awe_dev, bank);
	SEQ_DUMPBUF();
}

int seq_load_patch(void *patch, int len)
{
	awe_patch_info *p;
	SEQ_DUMPBUF();
	p = (awe_patch_info*)patch;
	p->key = AWE_PATCH;
	p->device_no = awe_dev;
	p->sf_id = 0;
	return write(seqfd, patch, len);
}

int seq_mem_avail(void)
{
	int mem_avail = awe_dev;
	ioctl(seqfd, SNDCTL_SYNTH_MEMAVL, &mem_avail);
	return mem_avail;
}

int seq_zero_atten(int atten)
{
	/* set zero attenuation level; this doesn't use seqbuf */
	/* flag 0x40 is the ossseq global flag */
	_AWE_CMD_NOW(seqfd, awe_dev, 0, _AWE_MISC_MODE|0x40,
		     AWE_MD_ZERO_ATTEN, atten);
	return 0;
}
