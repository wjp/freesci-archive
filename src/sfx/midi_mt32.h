#ifndef _MIDI_MT32_H_
#define _MIDI_MT32_H_

#include "fake_glib.h"

int midi_mt32_open(guint8 *data_ptr, unsigned int data_length);
int midi_mt32_close();

#endif /* _MIDI_MT32_H_ */
