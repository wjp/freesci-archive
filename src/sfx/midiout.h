#ifndef _MIDIOUT_H_
#define _MIDIOUT_H_

int midiout_open();
int midiout_close();
int midiout_write_event(guint8 *buffer, unsigned int count);
int midiout_write_block(guint8 *buffer, unsigned int count);

#endif /* _MIDIOUT_H_ */
