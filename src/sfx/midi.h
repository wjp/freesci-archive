#ifndef _MIDI_H_
#define _MIDI_H_

int midi_open(guint8 *data_ptr, unsigned int data_length);
int midi_close();

int midi_noteoff(guint8 channel, guint8 note);
int midi_noteon(guint8 channel, guint8 note, guint8 velocity);

#endif /* _MIDI_H_ */
