/***************************************************************************
 midi_adlib.c Copyright (C) 2001 Solomon Peachy

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

#include <stdio.h>
#ifdef HAVE_UNISTD_H
  #include <unistd.h>
#endif
#include <string.h>
#include <midi_device.h>
#include <midiout.h>
#include <soundserver.h>

#include <sys/ioctl.h>
#include <sys/soundcard.h>

SEQ_DEFINEBUF(1024);

typedef struct _sci_adlib_def {
  guint8 keyscale;       /* 0-3 !*/
  guint8 freqmod;        /* 0-15 !*/
  guint8 feedback;       /* 0-7 !*/
  guint8 attackrate;     /* 0-15 !*/
  guint8 sustainvol;     /* 0-15 !*/
  guint8 envelope;       /* t/f !*/
  guint8 decayrate;      /* 0-15 !*/
  guint8 releaserate;    /* 0-15 !*/
  guint8 volume;         /* 0-63 !*/
  guint8 ampmod;         /* t/f !*/
  guint8 vibrato;        /* t/f !*/
  guint8 keybdscale;       /* t/f !*/
  guint8 algorithm;       /* 0,1 !*/
  guint8 waveform;       /* 0-3 !*/
} adlib_def;

static adlib_def adlib_sounds[96][2];
static sbi_instr_data adlib_sbi[96];
static guint8 instr[MIDI_CHANNELS];
static int seqfd, dev;

void seqbuf_dump()
{
  if (_seqbufptr)
    if (write(seqfd, _seqbuf, _seqbufptr) == -1) {
      perror("ADLIB write ");
      exit(-1);
    }
  _seqbufptr = 0;
}

void make_sbi(adlib_def *one, adlib_def *two, guint8 *buffer)
{
  memset(buffer, 0, sizeof(sbi_instr_data));

  buffer[0] |= one->ampmod << 7;
  buffer[0] |= one->vibrato << 6;
  buffer[0] |= (one->envelope << 5) & 0x1;
  buffer[0] |= one->keybdscale << 4;
  buffer[0] |= one->freqmod & 0xf;
  buffer[1] |= two->ampmod << 7;
  buffer[1] |= two->vibrato << 6;
  buffer[1] |= (two->envelope << 5) & 0x1;
  buffer[1] |= two->keybdscale << 4;
  buffer[1] |= two->freqmod & 0xf;
  buffer[2] |= one->keyscale << 6;
  buffer[2] |= one->volume & 0x3f;
  buffer[3] |= two->keyscale << 6; 
  buffer[3] |= two->volume & 0x3f;
  buffer[4] |= one->attackrate << 4;
  buffer[4] |= one->decayrate & 0xf;  
  buffer[5] |= two->attackrate << 4;
  buffer[5] |= two->decayrate & 0xf;  
  buffer[6] |= one->sustainvol << 4;
  buffer[6] |= one->releaserate & 0xf;
  buffer[7] |= two->sustainvol << 4;
  buffer[7] |= two->releaserate & 0xf;
  buffer[8] |= one->waveform & 0x3;
  buffer[9] |= two->waveform & 0x3;
  buffer[10] |= one->feedback << 1;
  buffer[10] |= one->algorithm & 0x1;
  buffer[11] |= two->feedback << 1;
  buffer[11] |= two->algorithm & 0x1;

  return;
}

int midi_adlib_open(guint8 *data_ptr, unsigned int data_length)
{
  int nrdevs, i, n;
  struct synth_info info;
  struct sbi_instrument sbi;

  data_ptr += 2; 
  data_length -=2;
  memcpy(adlib_sounds, data_ptr, data_length);

  for (i = 0; i < 96; i++) 
    make_sbi(&adlib_sounds[i][0], &adlib_sounds[i][1], adlib_sbi[i]);

  memset(instr, 0, sizeof(instr));
 
  if ((seqfd=open("/dev/sequencer", O_WRONLY, 0))==-1) {
    perror("/dev/sequencer");
    return(-1);
  }
  if (ioctl(seqfd, SNDCTL_SEQ_NRSYNTHS, &nrdevs) == -1) {
    perror("/dev/sequencer");
    return(-1);
  }
  for (i=0;i<nrdevs && dev==-1;i++) {
    info.device = i;
    if (ioctl(seqfd, SNDCTL_SYNTH_INFO, &info)==-1) {
      perror("info: /dev/sequencer");
      return(-1);
    }
    if (info.synth_type == SYNTH_TYPE_FM)
      dev = i;
  }
  if (dev == -1) {
    fprintf(stderr, "ADLIB: FM synthesizer not detected\n");
    return(-1);
  }

  printf("ADLIB: Loading patches into synthesizer");
  sbi.device = dev;
  sbi.key = FM_PATCH;
  for (i = 0; i < 96; i++) {
    for (n = 0; n < 32; n++)
      memcpy(sbi.operators, &adlib_sbi[i], sizeof(sbi_instr_data));
    sbi.channel=i;
    SEQ_WRPATCH(&sbi, sizeof(sbi));
  }

  SEQ_DUMPBUF();
  return 0;
}


int midi_adlib_close()
{
  /* XXXX flush output?  signal sequencer anything? */
  return close(seqfd);
}

int midi_adlib_volume(guint8 volume)
{
  volume &= 0x7f; /* (make sure it's not over 127) */

  printf("volume NYI %02x \n", volume);
  return 0;

}

int midi_adlib_allstop(void) {
  printf("NYI\n");
  return 0;
}

int midi_adlib_reverb(short param)
{
  printf("reverb NYI %04x \n", param);
  return 0;
}

int midi_adlib_noteoff(guint8 channel, guint8 note, guint8 velocity)
{
  SEQ_STOP_NOTE(dev, channel, note, velocity);

  printf("noteoff NYI %02x %02x %02x\n", channel, note, velocity);
  SEQ_DUMPBUF();
  return 0;
}

int midi_adlib_noteon(guint8 channel, guint8 note, guint8 velocity)
{
  SEQ_START_NOTE(dev, channel, note, velocity);

  printf("noteon NYI %02x %02x %02x\n", channel, note, velocity);
  return 0;
  SEQ_DUMPBUF();
}

int midi_adlib_event(guint8 command, guint8 note, guint8 velocity)
{
  guint8 channel, oper;

  channel = command & 0x0f;
  oper = command & 0xf0;

  switch (oper) {    
  case 0x80:  /* XXXX noteon and noteoff */
    if (velocity != 0)
      return midi_adlib_noteon(channel, note, velocity);
  case 0x90:
    return midi_adlib_noteoff(channel, note, velocity);
  case 0xe0:    /* Pitch bend NYI */
    break;
  case 0xb0:    /* CC changes.  let 'em through... */
    break;
  default:
    printf("ADLIB: Unknown event %02x\n", command);
    return 0;
  }
  
  printf("event NYI %02x %02x %02x\n", command, note, velocity); 
  SEQ_DUMPBUF();
  return 0;
}

int midi_adlib_event2(guint8 command, guint8 param)
{
  guint8 channel;
  guint8 oper;
  int xparam = param;
  
  channel = command & 0x0f;
  oper = command & 0xf0;
  switch (oper) {
  case 0xc0: {  /* change instrument */
    int inst = param;
    instr[channel] = inst; /* XXXX offset? */
    return 0; /* MAGIC */
  }
  default:
    printf("ADLIB: Unknown event %02x\n", command);
  }

  SEQ_DUMPBUF();
  return 0;
}

/* the driver struct */

midi_device_t midi_device_adlib = {
  "adlib",
  "v0.01",
  &midi_adlib_open,
  &midi_adlib_close,
  &midi_adlib_event,
  &midi_adlib_event2,
  &midi_adlib_allstop,
  &midi_adlib_volume,
  &midi_adlib_reverb,
  003,		/* patch.001 */
  0x04,		/* playflag */
  1, 		/* play channel 9 -- rhythm ? */
  9		/* Max polyphony */
};
