/***************************************************************************
 midi_device.h Copyright (C) 2001 Solomon Peachy

 This program may be modified and copied freely according to the terms of
 the GNU general public license (GPL), as long as the above copyright
 notice and the licensing information contained herein are preserved.

 Please refer to www.gnu.org for licensing details.

 This work is provided AS IS, without warranty of any kind, expressed or
 implied, including but not limited to the warranties of merchantibility,
 noninfringement, and fitness for a specific purpose. The author will not
 be held liable for any damage caused by this work or derivatives of it.

 By using this source code, you agree to the licensing terms as stated
 above.

***************************************************************************/


#ifndef _MIDI_DEVICE_H_
#define _MIDI_DEVICE_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include <resource.h>

#define RHYTHM_CHANNEL 9

/* This is the actual API. */

int midi_open(guint8 *data_ptr, unsigned int data_length);
/* Opens MIDI out device.
** Parameters: (guint8 *) data_ptr: Patch data for MIDI synths
**             (unsigned int) data_length: Length of data contained within data_ptr
**             (void *) other_data: Driver-specific data
** Returns   : (int) Success code
*/

int midi_close();
/* Closes MIDI out device.
** Parameters: none
** Returns   : (int) Success code
*/

/*
int midi_noteoff(guint8 channel, guint8 note, guint8 velocity);
int midi_noteon(guint8 channel, guint8 note, guint8 velocity);
*/

int midi_event(guint8 command, guint8 note, guint8 velocity, guint32 delta);
/* Sends MIDI event with two parameters to MIDI device
** Parameters: (guint8) command: MIDI status / command
**             (guint8) note: Note / parameter 1
**             (guint8) velocity: Velocity / parameter 2
**             (guint32) delta:  time since last event.
** Returns   : (int) Success code
*/

int midi_event2(guint8 command, guint8 param, guint32 delta);
/* Sends MIDI event with one parameter to MIDI device
** Parameters: (guint8) command: MIDI status / command
**             (guint8) param: Parameter 1
**             (guint32) delta:  time since last event.
** Returns   : (int) Success code
*/


int midi_volume(guint8 volume);
int midi_allstop();
int midi_reverb(short param);

/* the struct */

typedef struct _midi_device {
	char *name;
	char *version;

	int (*open)(guint8 *data_ptr, unsigned int data_length);
	int (*close)();

	int (*event)(guint8 command, guint8 param, guint8 param2, guint32 other_data);
	int (*event2)(guint8 command, guint8 param, guint32 other_data);
	int (*allstop)();

        int (*volume)(guint8 volume);   /* OPTIONAL -- can be NULL */
	int (*reverb)(short param);

	unsigned short patchfile;
	guint8 playflag;
	guint8 playrhythm;
	gint8 polyphony;

} midi_device_t;

#define midi_playflag (midi_device->playflag)
#define midi_playrhythm (midi_device->playrhythm)
#define midi_patchfile (midi_device->patchfile)
#define midi_polyphony (midi_device->polyphony)

extern DLLEXTERN midi_device_t *midi_device;

/* existing drivers and methods for finding 'em. */

extern DLLEXTERN midi_device_t midi_device_mt32;
extern DLLEXTERN midi_device_t midi_device_mt32gm;

#ifdef HAVE_SYS_SOUNDCARD_H
extern DLLEXTERN midi_device_t midi_device_adlib;
#endif

extern DLLEXTERN midi_device_t midi_device_adlibemu;

extern DLLEXTERN midi_device_t *midi_devices[];

struct _midi_device *midi_find_device(char *name);


/*** MT32->GM mapping API ***/

void
midi_mt32gm_print_instrument(FILE *file, int index);
/* Prints a description of an instrument
** Parameters: (FILE *) file: The file to write to
**             (int) index: Index of the instrument to
**                          inspect
** Returns   : (void)
*/

void
midi_mt32gm_print_all_maps(FILE *file);
/* Prints all instrument mappings
** Parameters: (FILE *) file: The file they should be print to
** Returns   : (void)
*/

/***********  Adlib crap ***************/

typedef struct _sci_adlib_def {
  guint8 keyscale1;       /* 0-3 !*/
  guint8 freqmod1;        /* 0-15 !*/
  guint8 feedback1;       /* 0-7 !*/
  guint8 attackrate1;     /* 0-15 !*/
  guint8 sustainvol1;     /* 0-15 !*/
  guint8 envelope1;       /* t/f !*/
  guint8 decayrate1;      /* 0-15 !*/
  guint8 releaserate1;    /* 0-15 !*/
  guint8 volume1;         /* 0-63 !*/
  guint8 ampmod1;         /* t/f !*/
  guint8 vibrato1;        /* t/f !*/
  guint8 keybdscale1;     /* t/f !*/
  guint8 algorithm1;      /* 0,1 REVERSED */
  guint8 keyscale2;       /* 0-3 !*/
  guint8 freqmod2;        /* 0-15 !*/
  guint8 feedback2;       /* 0-7 UNUSED */
  guint8 attackrate2;     /* 0-15 !*/
  guint8 sustainvol2;     /* 0-15 !*/
  guint8 envelope2;       /* t/f !*/
  guint8 decayrate2;      /* 0-15 !*/
  guint8 releaserate2;    /* 0-15 !*/
  guint8 volume2;         /* 0-63 !*/
  guint8 ampmod2;         /* t/f !*/
  guint8 vibrato2;        /* t/f !*/
  guint8 keybdscale2;     /* t/f !*/
  guint8 algorithm2;      /* 0,1 UNUSED */
  guint8 waveform1;       /* 0-3 !*/
  guint8 waveform2;       /* 0-3 !*/
} adlib_def;

typedef unsigned char adlib_instr[12];

extern adlib_instr adlib_sbi[96];

void make_sbi(adlib_def *one, guint8 *buffer);
/* Converts a raw SCI adlib instrument into the adlib register format. */

#endif /* _MIDI_DEVICE_H_ */
