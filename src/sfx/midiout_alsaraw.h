#ifndef _MIDIOUT_ALSARAW_H_
#define _MIDIOUT_ALSARAW_H_

int midiout_alsaraw_open(int card, int device);
int midiout_alsaraw_close();
int midiout_alsaraw_write(guint8 *buffer, unsigned int count);

#endif /* _MIDIOUT_ALSARAW_H_ */
