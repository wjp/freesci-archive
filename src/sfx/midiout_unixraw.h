#ifndef _MIDIOUT_UNIXRAW_H_
#define _MIDIOUT_UNIXRAW_H_

int midiout_unixraw_open(char *devicename);
int midiout_unixraw_close();
int midiout_unixraw_write(guint8 *buffer, unsigned int count);

#endif /* _MIDIOUT_UNIXRAW_H_ */
