/*================================================================
 * sfxtest -- example program to control awe sound driver
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
#include <string.h>
#include <stdlib.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#ifdef __FreeBSD__
#  include <machine/soundcard.h>
#  include <awe_voice.h>
#elif defined(linux)
#  include <linux/soundcard.h>
#  include <linux/awe_voice.h>
#endif
#include "seq.h"
#include "awe_version.h"

SEQ_USE_EXTBUF();

static int buffering = 0;

/*----------------------------------------------------------------*/

static void seq_set_bank(int v, int bank)
{
	fprintf(stderr, "bank=%d\n", bank);
	SEQ_CONTROL(awe_dev, v, CTL_BANK_SELECT, bank);
	if (!buffering) seqbuf_dump();
}

static void seq_set_program(int v, int pgm)
{
	fprintf(stderr, "program=%d\n", pgm);
	SEQ_SET_PATCH(awe_dev, v, pgm);
	if (!buffering) seqbuf_dump();
}

static void seq_start_note(int v, int note, int vel)
{
	fprintf(stderr, "note on = %d %d\n", note, vel);
	SEQ_START_NOTE(awe_dev, v, note, vel);
	if (!buffering) seqbuf_dump();
}

static void seq_stop_note(int v, int note)
{
	fprintf(stderr, "note off\n");
	SEQ_STOP_NOTE(awe_dev, v, note, 0);
	if (!buffering) seqbuf_dump();
}

static void seq_pitchsense(int v, int val)
{
	fprintf(stderr, "bender range %d\n", val);
	SEQ_BENDER_RANGE(awe_dev, v, val);
	if (!buffering) seqbuf_dump();
}

static void seq_pitchbend(int v, int val)
{
	fprintf(stderr, "bender %d\n", val);
	SEQ_BENDER(awe_dev, v, val);
	if (!buffering) seqbuf_dump();
}

static void seq_panning(int v, int val)
{
	fprintf(stderr, "panning %d\n", val);
	SEQ_CONTROL(awe_dev, v, CTL_PAN, val);
	if (!buffering) seqbuf_dump();
}

static void seq_key_press(int v, int note, int vel)
{
	fprintf(stderr, "key pressure %d %d\n", note, vel);
	SEQ_KEY_PRESSURE(awe_dev, v, note, vel);
	if (!buffering) seqbuf_dump();
}

static void seq_send_effect(int v, int type, int val)
{
	fprintf(stderr, "send effect %d %d\n", type, val);
	AWE_SEND_EFFECT(awe_dev, v, type, val);
	if (!buffering) seqbuf_dump();
}	

static void seq_add_effect(int v, int type, int val)
{
	fprintf(stderr, "add effect %d %d\n", type, val);
	AWE_ADD_EFFECT(awe_dev, v, type, val);
	if (!buffering) seqbuf_dump();
}	

static void seq_send_control(int v, int type, int val)
{
	fprintf(stderr, "send control %d %d\n", type, val);
	SEQ_CONTROL(awe_dev, v, type, val);
	if (!buffering) seqbuf_dump();
}	

static void seq_wait(int time)
{
	fprintf(stderr, "wait %d\n", time);
	SEQ_DELTA_TIME(time);
	if (!buffering) seqbuf_dump();
}

static void seq_set_debug(int mode)
{
	fprintf(stderr, "debug %d\n", mode);
	AWE_DEBUG_MODE(awe_dev, mode);
	if (!buffering) seqbuf_dump();
}

static void seq_set_reverb(int mode)
{
	fprintf(stderr, "reverb %d\n", mode);
	AWE_REVERB_MODE(awe_dev, mode);
	if (!buffering) seqbuf_dump();
}

static void seq_set_chorus(int mode)
{
	fprintf(stderr, "chorus %d\n", mode);
	AWE_CHORUS_MODE(awe_dev, mode);
	if (!buffering) seqbuf_dump();
}

static void seq_init_chip()
{
	fprintf(stderr, "initialize chip\n");
	AWE_INITIALIZE_CHIP(seqfd, awe_dev);
	if (!buffering) seqbuf_dump();
}
	
static void seq_set_channel_mode(int mode)
{
	fprintf(stderr, "channel mode = %d\n", mode);
	AWE_SET_CHANNEL_MODE(awe_dev, mode);
	if (!buffering) seqbuf_dump();
}	

static void seq_equalizer(int bass, int treble)
{
	fprintf(stderr, "equalizer = %d %d\n", bass, treble);
	AWE_EQUALIZER(awe_dev, bass, treble);
	if (!buffering) seqbuf_dump();
}

static void seq_drum_channels(int channels)
{
	fprintf(stderr, "drum_channels = 0x%x\n", channels);
	AWE_DRUM_CHANNELS(awe_dev, channels);
	if (!buffering) seqbuf_dump();
}

/*----------------------------------------------------------------*/

static void usage();

int main(int argc, char **argv)
{
	int idx, chan;
	int c;
	char *seq_devname = NULL;
	int seq_devidx = -1;

	while ((c = getopt(argc, argv, "F:D:")) != -1) {
		switch (c) {
		case 'F':
			seq_devname = optarg;
			break;
		case 'D':
			seq_devidx = atoi(optarg);
			break;
		default:
			usage();
			exit(1);
		}
	}
	if (optind >= argc) {
		usage();
		return 1;
	}

	seq_init(seq_devname, seq_devidx);
	fprintf(stderr, "init done\n");
	SEQ_START_TIMER();
	if (!buffering) seqbuf_dump();

	fprintf(stderr, "argc=%d\n", argc);

	idx = optind;
	chan = 0;
	while (idx < argc) {
		fprintf(stderr, "[%d] ", idx);
		switch (*argv[idx]) {
		case 'u':
			buffering = !buffering;
			fprintf(stderr, "buffering mode %d\n", buffering);
			idx++;
			break;
		case 'X':
			seq_set_channel_mode(1); idx++;
			break;
		case 'x':
			chan = atoi(argv[idx+1]); idx += 2;
			break;
		case 'b':
			seq_set_bank(chan, atoi(argv[idx+1])); idx += 2;
			break;
		case 'p':
			seq_set_program(chan, atoi(argv[idx+1])); idx += 2;
			break;
		case 'n':
			seq_start_note(chan, atoi(argv[idx+1]),atoi(argv[idx+2]));
			idx += 3;
			break;
		case 'k':
			seq_stop_note(chan, 0); idx++;
			break;
		case 'K':
			seq_stop_note(chan, atoi(argv[idx+1])); idx += 2;
			break;
		case 'r':
			seq_pitchsense(chan, atoi(argv[idx+1])); idx += 2;
			break;
		case 't':
			seq_wait(atoi(argv[idx+1])); idx += 2;
			break;
		case 'T':
			sleep(atoi(argv[idx+1])); idx += 2;
			break;
		case 'w':
			seq_pitchbend(chan, atoi(argv[idx+1])); idx += 2;
			break;
		case 'c':
			seq_panning(chan, atoi(argv[idx+1])); idx += 2;
			break;
		case 'v':
			seq_key_press(chan, atoi(argv[idx+1]), atoi(argv[idx+2]));
			idx += 3;
			break;
		case 'F':
			seq_send_effect(chan, atoi(argv[idx+1]), atoi(argv[idx+2]));
			idx += 3;
			break;
		case 'f':
			seq_add_effect(chan, atoi(argv[idx+1]), atoi(argv[idx+2]));
			idx += 3;
			break;
		case 'm':
			seq_send_control(chan, atoi(argv[idx+1]), atoi(argv[idx+2]));
			idx += 3;
			break;
		case 'D':
			seq_set_debug(atoi(argv[idx+1])); idx += 2;
			break;
		case 'd':
			seq_drum_channels(strtol(argv[idx+1], NULL, 0)); idx += 2;
			break;
		case 'R':
			seq_set_reverb(atoi(argv[idx+1])); idx += 2;
			break;
		case 'C':
			seq_set_chorus(atoi(argv[idx+1])); idx += 2;
			break;
		case 'I':
			seq_init_chip(); idx++;
			break;
		case 'V':
			AWE_INITIAL_VOLUME(awe_dev, atoi(argv[idx+1])); idx += 2;
			break;
		case 'e':
			seq_equalizer(atoi(argv[idx+1]), atoi(argv[idx+2])); idx += 3;
			break;
		case 'M':
			AWE_MISC_MODE(awe_dev, atoi(argv[idx+1]), atoi(argv[idx+2])); idx += 3;
			break;
		default:
			goto loopend;
		}
	}
  loopend:
	if (buffering) seqbuf_dump();
	fprintf(stderr, "finishing..\n");
	seq_end();
	return 0;
}

static void usage()
{
	fprintf(stderr, "sfxtest - a test program for AWE32/64 driver\n");
	fprintf(stderr, VERSION_NOTE);
	fprintf(stderr, "usage: sfxtest cmd pars..\n");
	fprintf(stderr, "commands =\n");
	fprintf(stderr, "X: use channel control mode\n");
	fprintf(stderr, "x channel: change channel\n");
	fprintf(stderr, "b bank: change bank\n");
	fprintf(stderr, "p prg: change program\n");
	fprintf(stderr, "n note vel: start note\n");
	fprintf(stderr, "k : kill a note\n");
	fprintf(stderr, "K note: kill a note (for channel mode)\n");
	fprintf(stderr, "r val: set pitch sense\n");
	fprintf(stderr, "w val: set pitch wheel\n");
	fprintf(stderr, "t time: wait for time csec\n");
	fprintf(stderr, "c val: set panning\n");
	fprintf(stderr, "v note vel: change key pressure\n");
	fprintf(stderr, "D mode: set debug mode\n");
	fprintf(stderr, "C mode: set chorus mode\n");
	fprintf(stderr, "R mode: set reverb mode\n");
	fprintf(stderr, "F parm val: send effect\n");
	fprintf(stderr, "m parm val: send control value\n");
	fprintf(stderr, "I: initialize emu chip\n");
}	
