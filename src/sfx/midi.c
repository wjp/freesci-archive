#ifndef _MIDI_MT32_H_
#define _MIDI_MT32_H_

int midi_mt32_open(guint8 *data_ptr, unsigned int data_length);
int midi_mt32_close();

int midi_mt32_noteoff(guint8 channel, guint8 note);
int midi_mt32_noteon(guint8 channel, guint8 note, guint8 velocity);

#endif /* _MIDI_MT32_H_ */
