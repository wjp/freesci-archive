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
int midi_close(void);

int midi_noteoff(guint8 channel, guint8 note, guint8 velocity);
int midi_noteon(guint8 channel, guint8 note, guint8 velocity);
int midi_event(guint8 command, guint8 note, guint8 velocity);
int midi_event2(guint8 command, guint8 param);

int midi_volume(guint8 volume);
int midi_allstop(void);
int midi_reverb(short param);

/* the struct */

typedef struct _midi_device {
  char *name;
  char *version;

  int (*open)(guint8 *data_ptr, unsigned int data_length);
  int (*close)(void);

  int (*event)(guint8 command, guint8 param, guint8 param2);
  int (*event2)(guint8 command, guint8 param);
  int (*allstop)(void);

  int (*volume)(guint8 volume);
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

extern DLLEXTERN midi_device_t *midi_devices[];

struct _midi_device *midi_find_device(char *name);

#endif /* _MIDI_DEVICE_H_ */



