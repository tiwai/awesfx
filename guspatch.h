/*================================================================
 * guspatch.h
 *	patch structure of GUS compatible patch file
 *================================================================*/

#ifndef GUSPATCH_H_DEF
#define GUSPATCH_H_DEF

#include "itypes.h"

#define GUS_ENVELOPES	6	
 
/* This is the definition for what FORTE's patch format is. All .PAT */
/* files will have this format. */
 
#define GUS_HEADER_SIZE		12
#define GUS_ID_SIZE		10
#define GUS_DESC_SIZE		60
#define GUS_RESERVED_SIZE	40
#define GUS_PATCH_HEADER_RESERVED_SIZE 36
#define GUS_LAYER_RESERVED_SIZE	40
#define GUS_PATCH_DATA_RESERVED_SIZE	36
#define GUS_GF1_HEADER_TEXT	"GF1PATCH110"
 
typedef struct
{
	char	header[ GUS_HEADER_SIZE ];	
	char	gravis_id[ GUS_ID_SIZE ];	/* Id = "ID#000002" */
	char	description[ GUS_DESC_SIZE ];
	byte	instruments;
	char	voices;
	char	channels;
	uint16	wave_forms;
	uint16	master_volume;
	uint32	data_size;
	char	reserved[ GUS_PATCH_HEADER_RESERVED_SIZE ];
} GusPatchHeader;
 
typedef struct
{
	uint16	instrument;
	char	instrument_name[ 16 ];
	int32	instrument_size;
	char	layers;
	char	reserved[ GUS_RESERVED_SIZE ];	
} GusInstrument;
 
typedef struct
{
	char	layer_duplicate;
	char	layer;
	int32	layer_size;
	char	samples;
	char	reserved[ GUS_LAYER_RESERVED_SIZE ];	
} GusLayerData;
 
typedef struct
{
	char	wave_name[7];
 
	byte	fractions;
	int32	wave_size;
	int32	start_loop;
	int32	end_loop;
 
	uint16	sample_rate;
	int32	low_frequency;
	int32	high_frequency;
	int32	root_frequency;
	int16	tune;
	
	byte	balance;
 
	byte	envelope_rate[ GUS_ENVELOPES ];
	byte	envelope_offset[ GUS_ENVELOPES ];	
 
	byte	tremolo_sweep;
	byte	tremolo_rate;
	byte	tremolo_depth;
	
	byte	vibrato_sweep;
	byte	vibrato_rate;
	byte	vibrato_depth;
	
	char	modes;
	#define GUS_MODE_16BIT		1
	#define GUS_MODE_UNSIGNED	2
	#define GUS_MODE_LOOP		4
	#define GUS_MODE_LOOP_BIDIR	8
	#define GUS_MODE_LOOP_BACK	16
	#define GUS_MODE_SUSTAIN	32
	#define GUS_MODE_ENVELOPE	64
 
	int16	scale_frequency;
	uint16	scale_factor;		/* from 0 to 2048 or 0 to 2 */
	
	char	reserved[ GUS_PATCH_DATA_RESERVED_SIZE ];
} GusPatchData;


#endif
