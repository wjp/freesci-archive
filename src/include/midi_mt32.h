/***************************************************************************
 midi_mt32.h Copyright (C) 2000 Rickard Lind


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

#ifndef _MIDI_MT32_H_
#define _MIDI_MT32_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include <resource.h>

int midi_mt32_open(guint8 *data_ptr, unsigned int data_length);
int midi_mt32_close(void);

int midi_mt32_noteoff(guint8 channel, guint8 note, guint8 velocity);
int midi_mt32_noteon(guint8 channel, guint8 note, guint8 velocity);
int midi_mt32_event(guint8 command, guint8 note, guint8 velocity);
int midi_mt32_event2(guint8 command, guint8 param);

int midi_mt32_volume(guint8 volume);
int midi_mt32_allstop(void);
int midi_mt32_reverb(short param);

int midi_mt32_poke_gather(guint32 address, guint8 *data1, unsigned int count1,
                          guint8 *data2, unsigned int count2);

int midi_mt32_write_block(guint8 *data, unsigned int count);

extern unsigned short mt32_midi_patch;
extern guint8 mt32_midi_playflag;

#endif /* _MIDI_MT32_H_ */
