/***************************************************************************
 midi_adlibemu.c Copyright (C) 2002 Solomon Peachy

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
#include <pcmout.h>
#include <soundserver.h>

#include "fmopl.h"

/* #define DEBUG_ADLIB */
/* #define ADLIB_MONO */

/* shamelessly lifted from claudio's XMP */

static int register_base[11] = {
    0x20, 0x23, 0x40, 0x43,
    0x60, 0x63, 0x80, 0x83,
    0xe0, 0xe3, 0xc0
};

static int register_offset[12] = {
  /* Channel           1     2     3     4     5     6     7     8     9  */
  /* Operator 1 */   0x00, 0x01, 0x02, 0x08, 0x09, 0x0A, 0x10, 0x11, 0x12, 0x18, 0x19, 0x1A

};

static int ym3812_note[13] = {
    0x157, 0x16b, 0x181, 0x198, 0x1b0, 0x1ca,
    0x1e5, 0x202, 0x220, 0x241, 0x263, 0x287,
    0x2ae
};

/* logarithmic relationship between midi and FM volumes */
static int my_midi_fm_vol_table[128] = {
   0,  11, 16, 19, 22, 25, 27, 29, 32, 33, 35, 37, 39, 40, 42, 43,
   45, 46, 48, 49, 50, 51, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62,
   64, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 75, 76, 77,
   78, 79, 80, 80, 81, 82, 83, 83, 84, 85, 86, 86, 87, 88, 89, 89,
   90, 91, 91, 92, 93, 93, 94, 95, 96, 96, 97, 97, 98, 99, 99, 100,
   101, 101, 102, 103, 103, 104, 104, 105, 106, 106, 107, 107, 108,
   109, 109, 110, 110, 111, 112, 112, 113, 113, 114, 114, 115, 115,
   116, 117, 117, 118, 118, 119, 119, 120, 120, 121, 121, 122, 122,
   123, 123, 124, 124, 125, 125, 126, 126, 127
};

/* back to your regularly scheduled definitions */

static guint8 instr[MIDI_CHANNELS];
static guint8 pitch[MIDI_CHANNELS];
static guint8 vol[MIDI_CHANNELS];
static guint8 pan[MIDI_CHANNELS];
static int free_voices = ADLIB_VOICES;
static guint8 oper_note[ADLIB_VOICES];
static guint8 oper_chn[ADLIB_VOICES];

static FM_OPL *ym3812_L = NULL;
static FM_OPL *ym3812_R = NULL;

static guint8 adlib_reg_L[256];
static guint8 adlib_reg_R[256];

/* initialise note/operator lists, etc. */
void adlibemu_init_lists()
{
  int i;

  free_voices = ADLIB_VOICES;

  memset(instr, 0, sizeof(instr));
  memset(pitch, 0, sizeof(instr));
  memset(vol, 0x7f, sizeof(instr));
  memset(pan, 0x3f, sizeof(instr));
  memset(adlib_reg_L, 0, sizeof(instr));
  memset(adlib_reg_R, 0, sizeof(instr));
  memset(oper_chn, 0xff, sizeof(oper_chn));
  memset(oper_note, 0xff, sizeof(oper_note));
}

/* more shamelessly lifted from xmp and adplug.  And altered.  :) */

static inline int opl_write_L (int a, int v)
{
  adlib_reg_L[a] = v;
  OPLWrite (ym3812_L, 0x388, a);
  return OPLWrite (ym3812_L, 0x389, v);
}

static inline int opl_write_R (int a, int v)
{
  adlib_reg_R[a] = v;
  OPLWrite (ym3812_R, 0x388, a);
  return OPLWrite (ym3812_R, 0x389, v);
}

static inline int opl_write(int a, int v)
{
  opl_write_L(a,v);
  return opl_write_R(a,v);
}

/*
static inline guint8 opl_read (int a)
{
  OPLWrite (ym3812_L, 0x388, a);
  return OPLRead (ym3812_L, 0x389);
}
*/

void synth_setpatch (int voice, guint8 *data)
{
  int i, x;

  opl_write(0xBD, 0);

  for (i = 0; i < 10; i++)
    opl_write(register_base[i] + register_offset[voice], data[i]);

  opl_write(register_base[10] + voice, data[10]);

  /* mute voice after patch change */
  opl_write_L(0xb0 + voice, adlib_reg_L[0xb0+voice] & 0xdf);
  opl_write_R(0xb0 + voice, adlib_reg_R[0xb0+voice] & 0xdf);

#ifdef DEBUG_ADLIB
  for (i = 0; i < 10; i++)
    printf("%02x ", adlib_reg_L[register_base[i]+register_offset[voice]]);
    printf("%02x ", adlib_reg_L[register_base[10]+voice]);
#endif

}

void synth_setvolume_L (int voice, int volume)
{
  volume = volume >> 1;  /* adlib is 6-bit, midi is 7-bit */

  /* algorithm-dependent; we may need to set both operators. */
  if (adlib_reg_L[register_base[10]+voice] & 1)
    opl_write_L(register_base[2]+register_offset[voice],
	      ((63-volume) |
	       (adlib_reg_L[register_base[2]+register_offset[voice]]&0xc0)));

  opl_write_L(register_base[3]+register_offset[voice],
	      ((63-volume) |
	       (adlib_reg_L[register_base[3]+register_offset[voice]]&0xc0)));

}

void synth_setvolume_R (int voice, int volume)
{
  volume = volume >> 1;  /* adlib is 6-bit, midi is 7-bit */

  /* now for the other side. */
  if (adlib_reg_R[register_base[10]+voice] & 1)
    opl_write_R(register_base[2]+register_offset[voice],
		((63-volume) |
		 (adlib_reg_R[register_base[2]+register_offset[voice]]&0xc0)));
  
  opl_write_R(register_base[3]+register_offset[voice],
	      ((63-volume) |
	       (adlib_reg_R[register_base[3]+register_offset[voice]]&0xc0)));

}

void synth_setvolume (int voice, int volume)
{
  synth_setvolume_L(voice, volume);
  synth_setvolume_R(voice, volume);
}

void synth_setnote (int voice, int note, int bend)
{
    int n, fre, oct;

    n = note % 12;
    fre = ym3812_note[n] + (ym3812_note[n + 1] - ym3812_note[n]) * bend / 100;
    oct = note / 12 - 1;

    if (oct < 0)
        oct = 0;

    opl_write(0xa0 + voice, fre & 0xff);
    opl_write(0xb0 + voice,
        0x20 | ((oct << 2) & 0x1c) | ((fre >> 8) & 0x03));

#ifdef DEBUG_ADLIB
    printf("-- %02x %02x\n", adlib_reg_L[0xa0+voice], adlib_reg_L[0xb0+voice]);
#endif

}


/* back to your regularly scheduled driver */

int adlibemu_stop_note(int chn, int note, int velocity)
{
  int i, op=255;

  for (i=0;i<ADLIB_VOICES && op==255;i++) {
    if (oper_chn[i] == chn)
      if (oper_note[i] == note)
	op=i;
  }

  if (op==255) {
#ifdef DEBUG_ADLIB
    printf ("ADLIB: can't stop note: C%02x N%02x V%02x\n", chn, note, velocity);
    printf ("C ");
    for (i = 0; i < ADLIB_VOICES ; i++ ) {
      printf ("%02x ", oper_chn[i]);
    }
    printf ("\n");
    printf ("N ");
    for (i = 0; i < ADLIB_VOICES ; i++ ) {
      printf ("%02x ", oper_note[i]);
    }
    printf ("\n");
#endif
    return -1; /* that note isn't playing.. */
  }

  opl_write_L(0xb0+op,(adlib_reg_L[0xb0+op] & 0xdf));
  opl_write_R(0xb0+op,(adlib_reg_R[0xb0+op] & 0xdf));

  /*   synth_setnote(op, note, pitch[chn]);  */

  oper_chn[op] = 255;
  oper_note[op] = 255;

  free_voices++;

#ifdef DEBUG_ADLIB
  printf("stop voice %d (%d rem):  C%02x N%02x V%02x\n", op, free_voices, chn, note, velocity);
#endif

  return 0;
}

int adlibemu_start_note(int chn, int note, int velocity)
{
  int op, volume_L, volume_R, inst = 0;

  if (velocity == 0) {
    return adlibemu_stop_note(chn, note, velocity);
  }

  if (free_voices <= 0) {
    printf("ADLIB: All voices full\n");  /* XXX implement overflow code */
    return -1;
  }
   
  for (op = 0; op < ADLIB_VOICES ; op++)
    if (oper_chn[op] == 255)
      break;

  if (op == ADLIB_VOICES) {
    printf("ADLIB:  WTF?  We couldn't find a voice yet it we have %d left.\n", free_voices);
    return -1;
  }

  volume_L = velocity * vol[chn] / 128;     /* Scale channel volume */
  volume_R = velocity * vol[chn] / 128;     /* Scale channel volume */

#ifndef ADLIB_MONO
  if (pan[chn] > 0x3f)  /* pan right; so we scale the left down. */
    volume_L = volume_L / 0x3f * (0x3f - (pan[chn] - 0x3f));
  else if (pan[chn] < 0x3f) /* pan left; so we scale the right down.*/
    volume_R = volume_R / 0x3f * (0x3f - (0x3f-pan[chn]));
#endif 

  volume_R = my_midi_fm_vol_table[volume_R];  /* scale logarithmically */
  volume_L = my_midi_fm_vol_table[volume_L];  /* scale logarithmically */

  inst = instr[chn];

  synth_setpatch(op, adlib_sbi[inst]);
  synth_setvolume_L(op, volume_L);
  synth_setvolume_R(op, volume_R);
  synth_setnote(op, note, pitch[chn]);

  oper_chn[op] = chn;
  oper_note[op] = note;
  free_voices--;

#ifdef DEBUG_ADLIB
  printf("play voice %d (%d rem):  C%02x N%02x V%02x/%02x-%02x P%02x (%02x/%02x)\n", op, free_voices, chn, note, velocity, volume_L, volume_R, inst,
	 adlib_reg_L[register_base[2]+register_offset[op]] & 0x3f,
	 adlib_reg_L[register_base[3]+register_offset[op]] & 0x3f);
#endif

  return 0;
}

void test_adlib () {

  int voice = 0;
#if 0
  guint8 data[] = { 0x25, 0x21, 0x48, 0x48, 0xf0, 0xf2, 0xf0, 0xa5, 0x00, 0x00, 0x06 };
#else
  guint8 *data = adlib_sbi[0x0a];
#endif

#if 1
  opl_write(register_base[0]+register_offset[voice], data[0]);
  opl_write(register_base[1]+register_offset[voice], data[1]);
  opl_write(register_base[2]+register_offset[voice], data[2]);
  opl_write(register_base[3]+register_offset[voice], data[3]);
  opl_write(register_base[4]+register_offset[voice], data[4]);
  opl_write(register_base[5]+register_offset[voice], data[5]);
  opl_write(register_base[6]+register_offset[voice], data[6]);
  opl_write(register_base[7]+register_offset[voice], data[7]);
  opl_write(register_base[8]+register_offset[voice], data[8]);
  opl_write(register_base[9]+register_offset[voice], data[9]);
  opl_write(register_base[10]+register_offset[voice], data[10]);
#else
  synth_setpatch(voice, data);
#endif

#if 0
  opl_write(0xA0 + voice, 0x57);
  opl_write(0xB0 + voice, 0x2d);
#else
  synth_setvolume(voice, 0x50);
  synth_setvolume(voice, 0x50);
  synth_setnote(voice, 0x30, 0);
#endif

  /*
  instr[0x0e] = 0x0a;
  instr[0x03] = 0x26;

  adlibemu_start_note(0x0e, 0x30, 0x40);
  sleep(1);
  adlibemu_start_note(0x03, 0x48, 0x40);
  sleep(1);
  */
}

int midi_adlibemu_open(guint8 *data_ptr, unsigned int data_length)
{
  int i, n;

  /* load up the patch.003 file, parse out the insturments */
  if (data_length < 1344) {
    printf ("ADLIB: invalid patch.003");
    return -1;
  }

  for (i = 0; i < 48; i++)
    make_sbi((adlib_def *)(data_ptr+(28 * i)), adlib_sbi[i]);

  if (data_length > 1344)
    for (i = 48; i < 96; i++)
      make_sbi((adlib_def *)(data_ptr+2+(28 * i)), adlib_sbi[i]);

  ym3812_L = OPLCreate (OPL_TYPE_YM3812, 3579545, pcmout_sample_rate);
  ym3812_R = OPLCreate (OPL_TYPE_YM3812, 3579545, pcmout_sample_rate);

  // XXX register with pcm layer.
  return midi_adlibemu_reset();
}


int midi_adlibemu_close()
{
  FM_OPL *opl = ym3812_L;
  ym3812_L = NULL;
  OPLDestroy (opl);
  opl = ym3812_R;
  ym3812_R = NULL;
  OPLDestroy (opl);

  // XXX deregister with pcm layer.
  return 0;
}

int midi_adlibemu_reset(void)
{
  //  printf("AdlibEmu:  Reset\n");
  if (ym3812_L == NULL)
    return -1;
  if (ym3812_R == NULL)
    return -1;

  adlibemu_init_lists();

  OPLResetChip (ym3812_L);
  OPLResetChip (ym3812_R);

  opl_write(0x01, 0x20);
  opl_write(0xBD, 0xc0);

  //  test_adlib();
  return 0;
}

int midi_adlibemu_reverb(short param)
{
  printf("ADLIB: reverb NYI %04x \n", param);
  return 0;
}

int midi_adlibemu_event(guint8 command, guint8 note, guint8 velocity, guint32 delta)
{
  guint8 channel, oper;

  channel = command & 0x0f;
  oper = command & 0xf0;

  switch (oper) {
  case 0x80:
    return adlibemu_stop_note(channel, note, velocity);
  case 0x90:  /* noteon and noteoff */
    return adlibemu_start_note(channel, note, velocity);
  case 0xe0:    /* XXXX Pitch bend NYI */
    break;
  case 0xb0:    /* CC changes. */
    switch (note) {
    case 0x07:
      vol[channel] = velocity;
      break;
    case 0x0a:
      pan[channel] = velocity;
      break;
    case 0x7b:  { /* all notes off */
      int i = 0;
      for (i=0;i<ADLIB_VOICES;i++)
	if (oper_chn[i] == channel)
	  adlibemu_stop_note(channel, oper_note[i], 0);
      break;
    }
    default:
      ; /* XXXX ignore everything else for now */
    }
    return 0;
  case 0xd0:    /* aftertouch */
    /* XXX Aftertouch in the OPL thing? */
    return 0;
  default:
    printf("ADLIB: Unknown event %02x\n", command);
    return 0;
  }

  return 0;
}

int midi_adlibemu_event2(guint8 command, guint8 param, guint32 delta)
{
  guint8 channel;
  guint8 oper;

  channel = command & 0x0f;
  oper = command & 0xf0;
  switch (oper) {
  case 0xc0:   /* change instrument */
    instr[channel] = param;
    return 0;
  default:
    printf("ADLIB: Unknown event %02x\n", command);
  }

  return 0;
}

/* the driver struct */

midi_device_t midi_device_adlibemu = {
  "adlibemu",
  "v0.01",
  &midi_adlibemu_open,
  &midi_adlibemu_close,
  &midi_adlibemu_event,
  &midi_adlibemu_event2,
  &midi_adlibemu_reset,
  NULL,
  &midi_adlibemu_reverb,
  003,		/* patch.003 */
  0x04,		/* playflag */
  0, 		/* do not play channel 9 */
  ADLIB_VOICES  /* Max polyphony */
};

/* count is # of FRAMES, not bytes.
   We assume 16-bit stereo frames (ie 4 bytes)
*/
void synth_mixer (void* buffer, int count)
{
  int i;
  guint16 *ptr = buffer;
  guint16 *databuf = sci_malloc(count * sizeof(guint16)); // hold half.

  if (!databuf)
    return;
  if (!buffer)
    return;
  if (!ym3812_L)
    return;
  if (!ym3812_R)  /* if either is uninitialized, bad things happen */
    return;
  
  YM3812UpdateOne (ym3812_L, databuf, count); 

  for (i = 0; i < count ; i++) {
    *ptr = databuf[i];
    ptr += 2;
  }

  ptr = buffer;
  ptr++;

  YM3812UpdateOne (ym3812_R, databuf, count);
  for (i = 0; i < count ; i++) {
    *ptr = databuf[i];
    ptr+=2;
  }

  sci_free(databuf);
}
