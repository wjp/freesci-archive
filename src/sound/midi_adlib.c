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

/* TODO: 
          Global volume.
	  Pitch bend.
	  Timing issues.
	  ALSA?
*/

#include <stdio.h>
#ifdef HAVE_UNISTD_H
  #include <unistd.h>
#endif
#include <string.h>
#include <midi_device.h>
#include <midiout.h>
#include <soundserver.h>

#ifdef HAVE_SYS_SOUNDCARD_H

#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/soundcard.h>

#define ADLIB_VOICES 9

#if 0
SEQ_DEFINEBUF(2048);
static int seqfd;
#else
extern unsigned char _seqbuf[2048];
extern int _seqbuflen;
extern int _seqbufptr;
extern int seqfd;
#endif

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
  guint8 algorithm1;      /* 0,1 REVERSED? */
  guint8 keyscale2;       /* 0-3 !*/
  guint8 freqmod2;        /* 0-15 !*/
  guint8 feedback2;       /* 0-7 UNUSED? */
  guint8 attackrate2;     /* 0-15 !*/
  guint8 sustainvol2;     /* 0-15 !*/
  guint8 envelope2;       /* t/f !*/
  guint8 decayrate2;      /* 0-15 !*/
  guint8 releaserate2;    /* 0-15 !*/
  guint8 volume2;         /* 0-63 !*/
  guint8 ampmod2;         /* t/f !*/
  guint8 vibrato2;        /* t/f !*/
  guint8 keybdscale2;     /* t/f !*/
  guint8 algorithm2;      /* 0,1 UNUSED? */
  guint8 waveform1;       /* 0-3 !*/
  guint8 waveform2;       /* 0-3 !*/
} adlib_def;

static sbi_instr_data adlib_sbi[96];
static guint8 instr[MIDI_CHANNELS];
static guint8 pitch[MIDI_CHANNELS];
static int polyphony[MIDI_CHANNELS];
static int dev;
static int free_voices = ADLIB_VOICES;
static long note_time[ADLIB_VOICES];
static unsigned char oper_note[ADLIB_VOICES];
static unsigned char oper_chn[ADLIB_VOICES];

#if 0
void seqbuf_dump()
{
  if (_seqbufptr)
    if (write(seqfd, _seqbuf, _seqbufptr) == -1) {
      perror("ADLIB write ");
      exit(-1);
    }
  _seqbufptr = 0;
}
#endif

void make_sbi(adlib_def *one, guint8 *buffer)
{
  memset(buffer, 0, sizeof(sbi_instr_data));

#if 0
 printf ("%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x ", one->keyscale1, one->freqmod1, one->feedback1, one->attackrate1, one->sustainvol1, one->envelope1, one->decayrate1, one->releaserate1, one->volume1, one->ampmod1, one->vibrato1, one->keybdscale1, one->algorithm1, one->waveform1);

  printf (" %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x ", one->keyscale2, one->freqmod2, one->feedback2, one->attackrate2, one->sustainvol2, one->envelope2, one->decayrate2, one->releaserate2, one->volume2, one->ampmod2, one->vibrato2, one->keybdscale2, one->algorithm2, one->waveform2);

  printf("\n");
#endif

  buffer[0] |= ((one->ampmod1 & 0x1) << 7);
  buffer[0] |= ((one->vibrato1 & 0x1) << 6);
  buffer[0] |= ((one->envelope1 & 0x1) << 5);
  buffer[0] |= ((one->keybdscale1 & 0x1) << 4);
  buffer[0] |= (one->freqmod1 & 0xf);
  buffer[1] |= ((one->ampmod2 & 0x1) << 7);
  buffer[1] |= ((one->vibrato2 & 0x1) << 6);
  buffer[1] |= ((one->envelope2 & 0x1) << 5);
  buffer[1] |= ((one->keybdscale2 & 0x1) << 4);
  buffer[1] |= (one->freqmod2 & 0xf);
  buffer[2] |= ((one->keyscale1 & 0x3) << 6);
  buffer[2] |= (one->volume1 & 0x3f);
  buffer[3] |= ((one->keyscale2 & 0x3) << 6); 
  buffer[3] |= (one->volume2 & 0x3f);
  buffer[4] |= ((one->attackrate1 & 0xf) << 4);
  buffer[4] |= (one->decayrate1 & 0xf);  
  buffer[5] |= ((one->attackrate2 & 0xf) << 4);
  buffer[5] |= (one->decayrate2 & 0xf);  
  buffer[6] |= ((one->sustainvol1 & 0xf) << 4);
  buffer[6] |= (one->releaserate1 & 0xf);
  buffer[7] |= ((one->sustainvol2 & 0xf) << 4);
  buffer[7] |= (one->releaserate2 & 0xf);
  buffer[8] |= (one->waveform1 & 0x3);
  buffer[9] |= (one->waveform2 & 0x3);

  buffer[10] |= ((one->feedback1 & 0x7) << 1);
  buffer[10] |= (1-(one->algorithm1 & 0x1));

  return;
}

/* initialise note/operator lists, etc. */
void adlib_init_lists()
{
  int i;
  for(i = 0 ; i < ADLIB_VOICES ; i++) {
    oper_note[i] = 255;
    oper_chn[i] = 255;
    note_time[i] = 0;
  }
}

int adlib_stop_note(int chn, int note, int velocity)
{
  int i, op=255;
  
  for (i=0;i<ADLIB_VOICES && op==255;i++) {
    if (oper_chn[i] == chn)
      if (oper_note[i] == note)
	op=i;
  }

  if (op==255) {
    printf ("can't stop.. chn %d %d %d\n", chn, note, velocity);
    return 255;	/* not playing */
  }
  
  SEQ_STOP_NOTE(dev, op, note, velocity);
  SEQ_DUMPBUF();

  oper_chn[op] = 255;
  oper_note[op] = 255;
  note_time[op] = 0;
  
  free_voices++;

  return op;
}

int adlib_kill_one_note(int chn)
{
  int oldest, i = 255;
  long time = 0;
     
  if (free_voices >= ADLIB_VOICES) {
    printf("Free list empty but no notes playing\n"); 
    return 255;
  }	/* No notes playing */

  for (i = 0; i < ADLIB_VOICES ; i++) {
    if (oper_chn[i] != chn)
      continue;
    if (note_time[i] == 0)
      continue;
    if (time == 0) {
      time = note_time[i];
      oldest = i;
      continue;
    }
    if (note_time[i] < time) {
      time = note_time[i];
      oldest = i;
    }
  }

  printf("Killing chn %d, oper %d\n", chn, oldest);

  if (oldest == 255) 
    return 255;	/* Was already stopped. Why? */

  SEQ_STOP_NOTE(dev, oldest, oper_note[oldest], 0);
  SEQ_DUMPBUF();

  oper_chn[oldest] = 255;
  oper_note[oldest] = 255;
  note_time[oldest] = 0;
  free_voices++;

  return oldest;
}

void adlib_start_note(int chn, int note, int velocity)
{
  int free;    
  struct timeval now;

  if (velocity == 0) {
    adlib_stop_note(chn, note, velocity);
    return;
  }

  gettimeofday(&now, NULL);  

  if (free_voices <= 0)
    free = adlib_kill_one_note(chn);
  else 
    for (free = 0; free < ADLIB_VOICES ; free++)
      if (oper_chn[free] == 255)
	break;

  printf("play operator %d/%d:  %d %d %d\n", free, free_voices, chn, note, velocity);

  oper_chn[free] = chn;
  oper_note[free] = note;
  note_time[free] = now.tv_sec * 1000000 + now.tv_usec;
  free_voices--;
  
  SEQ_SET_PATCH(dev, free, instr[chn]);
  //XXXX  SEQ_PITCHBEND(dev, free, pitch[chn]);
  SEQ_START_NOTE(dev, free, note, velocity);
  SEQ_DUMPBUF();
}

int midi_adlib_open(guint8 *data_ptr, unsigned int data_length)
{
  int nrdevs, i, n;
  struct synth_info info;
  struct sbi_instrument sbi;

  if (data_length < 1344) {
    printf ("invalid patch.003");
    return -1;
  }

  for (i = 0; i < 48; i++) 
    make_sbi((adlib_def *)(data_ptr+(28 * i)), adlib_sbi[i]);

  if (data_length > 1344)
    for (i = 48; i < 96; i++) 
      make_sbi((adlib_def *)(data_ptr+2+(28 * i)), adlib_sbi[i]);

  memset(instr, 0, sizeof(instr));
  memset(pitch, 0, sizeof(pitch));
 
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

  /*  free_voices = info.nr_voices; */
  adlib_init_lists();

  printf("ADLIB: Loading patches into synthesizer\n");
  sbi.device = dev;
  sbi.key = FM_PATCH;
  for (i = 0; i < 96; i++) {
    for (n = 0; n < 32; n++)
      memcpy(sbi.operators, &adlib_sbi[i], sizeof(sbi_instr_data));
    sbi.channel=i;
    SEQ_WRPATCH(&sbi, sizeof(sbi));
    SEQ_DUMPBUF();
  }

  return 0;
}


int midi_adlib_close()
{
  SEQ_DUMPBUF();
  return close(seqfd);
}

int midi_adlib_volume(guint8 volume)
{
  volume &= 0x7f; /* (make sure it's not over 127) */

  printf("volume NYI %02x \n", volume);
  return 0;

}

int midi_adlib_allstop(void) {
  int i;
  for (i = 0; i < ADLIB_VOICES ; i++) {
    if (oper_chn[i] == 255)
      continue;
    adlib_stop_note(oper_chn[i], oper_note[i], 0);
  }
  printf("end allstop\n");
  return 0;
}

int midi_adlib_reverb(short param)
{
  printf("reverb NYI %04x \n", param);
  return 0;
}

int midi_adlib_noteoff(guint8 channel, guint8 note, guint8 velocity)
{
  adlib_stop_note(channel, note, velocity);
  return 0;
}

int midi_adlib_noteon(guint8 channel, guint8 note, guint8 velocity)
{
  adlib_start_note(channel,note,velocity);
  return 0;
}

int midi_adlib_event(guint8 command, guint8 note, guint8 velocity)
{
  guint8 channel, oper;

  channel = command & 0x0f;
  oper = command & 0xf0;

  switch (oper) {    
  case 0x80:
    return midi_adlib_noteoff(channel, note, velocity);
  case 0x90:  
    return midi_adlib_noteon(channel, note, velocity);
  case 0xe0:    /* Pitch bend NYI */
    break;
  case 0xb0:    /* CC changes.  we ignore. */
    return 0;
  case 0xd0:    /* aftertouch */
    SEQ_CHN_PRESSURE(dev, channel, note);
    SEQ_DUMPBUF();    
    return 0;
  default:
    printf("ADLIB: Unknown event %02x\n", command);
    return 0;
  }
  
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
    //    SEQ_SET_PATCH(dev, channel, inst);
    //    SEQ_DUMPBUF();
    return 0;
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
  003,		/* patch.003 */
  0x04,		/* playflag */
  1, 		/* play channel 9 -- rhythm ? */
  ADLIB_VOICES  /* Max polyphony */
};

#endif /* HAVE_SYS_SOUNDCARD_H */
