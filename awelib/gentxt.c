/*================================================================
 * generator label table
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

#include "sflayer.h"

char *sf_gen_text[SF_EOF] = {
	"startAddrs",         /* sample start address -4 (0 to * 0xffffff) */
        "endAddrs",
        "startloopAddrs",     /* loop start address -4 (0 to * 0xffffff) */
        "endloopAddrs",       /* loop end address -3 (0 to * 0xffffff) */
        "startAddrsHi",       /* high word of startAddrs */
        "lfo1ToPitch",        /* main fm: lfo1-> pitch */
        "lfo2ToPitch",        /* aux fm:  lfo2-> pitch */
        "env1ToPitch",        /* pitch env: env1(aux)-> pitch */
        "initialFilterFc",    /* initial filter cutoff */
        "initialFilterQ",     /* filter Q */
        "lfo1ToFilterFc",     /* filter modulation: lfo1 -> filter * cutoff */
        "env1ToFilterFc",     /* filter env: env1(aux)-> filter * cutoff */
        "endAddrsHi",         /* high word of endAddrs */
        "lfo1ToVolume",       /* tremolo: lfo1-> volume */
        "env2ToVolume",       /* Env2Depth: env2-> volume */
        "chorusEffectsSend",  /* chorus */
        "reverbEffectsSend",  /* reverb */
        "panEffectsSend",     /* pan */
        "auxEffectsSend",     /* pan auxdata (internal) */
        "sampleVolume",       /* used internally */
        "unused3",
        "delayLfo1",          /* delay 0x8000-n*(725us) */
        "freqLfo1",           /* frequency */
        "delayLfo2",          /* delay 0x8000-n*(725us) */
        "freqLfo2",           /* frequency */
        "delayEnv1",          /* delay 0x8000 - n(725us) */
        "attackEnv1",         /* attack */
        "holdEnv1",             /* hold */
        "decayEnv1",            /* decay */
        "sustainEnv1",          /* sustain */
        "releaseEnv1",          /* release */
        "autoHoldEnv1",
        "autoDecayEnv1",
        "delayEnv2",            /* delay 0x8000 - n(725us) */
        "attackEnv2",           /* attack */
        "holdEnv2",             /* hold */
        "decayEnv2",            /* decay */
        "sustainEnv2",          /* sustain */
        "releaseEnv2",          /* release */
        "autoHoldEnv2",
        "autoDecayEnv2",
        "instrument",           /* */
        "nop",
        "keyRange",             /* */
        "velRange",             /* */
        "startloopAddrsHi",     /* high word of startloopAddrs */
        "keynum",               /* */
        "velocity",             /* */
        "initAtten",              /* */
        "keyTuning",
        "endloopAddrsHi",       /* high word of endloopAddrs */
        "coarseTune",
        "fineTune",
        "sampleId",
        "sampleFlags",
        "samplePitch",          /* SF1 only */
        "scaleTuning",
        "keyExclusiveClass",
        "rootKey",
};
