/***************************************************************************
 midiout_ossopl3.c Copyright (C) 2001 Rune Orsval

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

/**************************************************************************
This file contains some code taken from 'fmplay' by Antoine Lefebvre.
Fmplay includes the following notice:

 fmplay.c     (Antoine Lefebvre -  gamma_orion@iname.com)
 The major part of the code come from Andrew Bulhak jam.c which
 contains code from Hannu Savolainen midithru.c (VoxWare utilities)

 There is also code from Nathan Laredo playmidi 2.2

 Copyright (c) Antoine Lefebvre, 1998  (gamma_orion@iname.com)
 Portions copyright (c) Andrew Bulhak, 1995
 Portions copyright (c) Nathan Laredo, 1995, 1996
 Portions copyright (c) Hannu Savolainen, 1993
 Copyright notice from VoxWare code below:
 -----------------------------------------
 Copyright by Hannu Savolainen 1993

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions
 are met:
 1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 SUCH DAMAGE.

***************************************************************************/

/*  This module does not use the patch files for the opl2. Instead the sound */
/*  output is based on the general MIDI data derived from the MT32 patches.  */

#include <midiout.h>

#ifdef HAVE_SYS_SOUNDCARD_H
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/soundcard.h>

#ifndef O_SYNC
#  define O_SYNC 0
#endif

#define SEQUENCER_PATH	"/dev/sequencer"
#define MIDI_PATH       "/dev/midi"
#define O3MELODIC	"std.o3"
#define O3DRUMS		"drums.o3"
#define SBMELODIC	"std.sb"
#define SBDRUMS		"drums.sb"

#define FALSE  0
#define TRUE   1


int fmloaded[256]; /* sound blaster sound data */
int wantopl3 = FALSE;
int reverb = FALSE;
int csound = TRUE;
struct synth_info info;


int seqfd;
int sb_dev=-1;
unsigned char buf[100];
int bufp;

int pgm=0, num_voices, bender=0;
int free_voices;

/* LRU list for free operators */
unsigned char free_list[256];
int fhead=0, ftail=0, flen=0;

/* LRU list for still playing notes */
unsigned char note_list[256];

double notetime[256];          /* second */
int nhead=0, ntail=0, nlen=0;
unsigned char oper_note[32];

double notetime[256];          /* beginning of the note */
double begtime;                /*time at the beginning */

/*the precision on the time (1000 mean 1/1000s, 100 mean 1/100s) */
int precision = 1e6/100;
/*the instrument number in csound: can be change by program change*/
int instr = 1;

/* all those function came from jam.c */
void init_lists();
void init_sequencer();
void stop_note(int note, int velocity);
void kill_one_note();
void start_note(int note, int velocity);
void channel_pressure(int ch, int pressure);
void pitch_bender(int ch, int value);
void do_buf();
void read_midi(int fd);

/* those function came from playmidi 2.2 */
void loadfm(void);
void adjustfm(char *buf, int key);

/* those are mine */
long sec(void);
long usec(void);
void (*printcsound)(int note);

SEQ_DEFINEBUF (2048);
SEQ_PM_DEFINES; /* patch manager stuff */

static char *devicename = "/dev/sequencer";
static char *patchpath = "/etc/midi";

static int ossopl3_lastwrote = 0;

/*
 * The function seqbuf_dump() must always be provided
 */
void seqbuf_dump ()
{
  if (_seqbufptr)
    if (write (seqfd, _seqbuf, _seqbufptr) == -1)
      {
	perror ("write /dev/sequencer");
	exit (-1);
      }
  _seqbufptr = 0;
}

static int
midiout_ossopl3_set_parameter(struct _midiout_driver *drv, char *attribute, char *value)
{
	if (!strcasecmp(attribute, "device")) {
	  devicename = value;
	} else if (!strcasecmp(attribute, "patchpath")) {
	  patchpath = value;
	} else
		sciprintf("Unknown ossopl3 option '%s'!\n", attribute);

	return 0;
}

int midiout_ossopl3_open()
{
   int i, n;

  if((seqfd = open(devicename, O_RDWR, 0)) == -1) {
    sciprintf("Cannot open %s.", devicename);
    exit(-1);
  };

  if (ioctl (seqfd, SNDCTL_SEQ_NRSYNTHS, &n) == -1) {
    sciprintf("Cannot get %s info.", devicename);
    exit(-1);
  };

  for (i = 0; i < n; i++) {
    info.device = i;
    if (ioctl (seqfd, SNDCTL_SYNTH_INFO, &info) == -1) {
      sciprintf("Cannot get %s info.", devicename);
      exit(-1);
    }

    if (info.synth_type == SYNTH_TYPE_FM
	&& info.synth_subtype == FM_TYPE_OPL3)
      sb_dev = i;
    free_voices = num_voices = (info.nr_voices>999)?999:info.nr_voices;
  }
  if(sb_dev == -1) {
    sciprintf("Sound Blaster not available.");
       exit(-1);
  }

  if (ioctl (seqfd, SNDCTL_SEQ_RESET, &sb_dev) == -1)
       printf("Could not reset sequencer.");
/*    init_sequencer(); */
  init_lists();
  loadfm();
/*    start_note(64, 164); */

}

/* initialise note/operator lists, etc. */
void init_lists()
{
  int i;
  for(i=0; i<16;i++) oper_note[i] = 255; /* free */
  /* this asumes that num_voices, set in init_sequencer(), is set. */
  flen = num_voices;
  for(i=0; i<num_voices; i++) {
    free_list[i]=i;
  };
  bufp = 0;
};

int midiout_ossopl3_close()
{
  if (close(seqfd) < 0)
    return -1;
  return 0;
}

int midiout_ossopl3_flush(guint8 code)
{
  /* opened with O_SYNC; already flushed.. */
  usleep (320 * ossopl3_lastwrote);  /* delay to make sure all was written */
  return 0;
}

int midiout_ossopl3_write(guint8 *buffer, unsigned int count, guint32 other_data)
{
 int i;

/*   printf("count %i\n", count); */
 for (i=0; i<count; i++)
   {
   buf[i]=buffer[i];
/*     printf("byte %x, %i\n", buffer[i], buffer[i]);  */
   }
 bufp=count;
 do_buf();
 return 0;

}

midiout_driver_t midiout_driver_ossopl3 = {
  "ossopl3",
  "v0.01",
  &midiout_ossopl3_set_parameter,
  &midiout_ossopl3_open,
  &midiout_ossopl3_close,
  &midiout_ossopl3_write,
  &midiout_ossopl3_flush
};

void loadfm()
{
    int sbfd;
    int i;
    int n;
    int voice_size;
    int data_size;

    char buf[160];
    struct sbi_instrument instr;

    for (i = 0; i < 256; i++)
	fmloaded[i] = 0;
    srand(getpid());         /* why ? */

    if (wantopl3)
    {
	 voice_size = 60;
	 sprintf(buf,"%s/%s", patchpath, O3MELODIC);
    }
    else
    {
	 voice_size = 52;
	 sprintf(buf,"%s/%s", patchpath, SBMELODIC);
    }

    sbfd = open(buf, O_RDONLY, 0);

    if (sbfd == -1)
      {
	 printf("Instruments definition files absent\n");
	 exit(1);
      }
    instr.device = sb_dev;

    for (i = 0; i < 128; i++)
    {
	 if (read(sbfd, buf, voice_size) != voice_size)
	      printf("Read error \n");
	 instr.channel = i;

	 if (strncmp(buf, "4OP", 3) == 0)
	 {
	      instr.key = OPL3_PATCH;
	      data_size = 22;
	 }
	 else
	 {
	      instr.key = FM_PATCH;
	      data_size = 11;
	 }

	 fmloaded[i] = instr.key;

	 adjustfm(buf, instr.key);
	 for (n = 0; n < 32; n++)
	      instr.operators[n] = (n < data_size) ? buf[36 + n] : 0;

	 SEQ_WRPATCH(&instr, sizeof(instr));
    }
    close(sbfd);

    if (wantopl3)
	 sprintf(buf,"%s/%s", patchpath, O3DRUMS);
    else
	 sprintf(buf,"%s/%s", patchpath, SBDRUMS);

    sbfd = open(buf, O_RDONLY, 0);

    for (i = 128; i < 175; i++)    {
	 if (read(sbfd, buf, voice_size) != voice_size)
	      printf("Read error\n");
	 instr.channel = i;

	 if (strncmp(buf, "4OP", 3) == 0)
	 {
	      instr.key = OPL3_PATCH;
	      data_size = 22;
	 }
	 else
	 {
	      instr.key = FM_PATCH;
	      data_size = 11;
	 }
	 fmloaded[i] = instr.key;

	 adjustfm(buf, instr.key);
	 for (n = 0; n < 32; n++)
	      instr.operators[n] = (n < data_size) ? buf[n + 36] : 0;

	 SEQ_WRPATCH(&instr, sizeof(instr));
    }
    close(sbfd);
}


void adjustfm(char *buf, int key)
{
    unsigned char pan = ((rand() % 3) + 1) << 4;

    if (key == FM_PATCH)
    {
	 buf[39] &= 0xc0;
	 if (buf[46] & 1)
	      buf[38] &= 0xc0;
	 buf[46] = (buf[46] & 0xcf) | pan;
	 if (reverb)
	 {
	      unsigned val;
	      val = buf[43] & 0x0f;
	      if (val > 0)
		   val--;
	      buf[43] = (buf[43] & 0xf0) | val;
	 }
    }
    else
    {
	 int mode;
	 if (buf[46] & 1)
	      mode = 2;
	 else
	      mode = 0;
	 if (buf[57] & 1)
	      mode++;
	 buf[50] &= 0xc0;
	 if (mode == 3)
	      buf[49] &= 0xc0;
	 if (mode == 1)
	      buf[39] &= 0xc0;
	 if (mode == 2 || mode == 3)
	      buf[38] &= 0xc0;
	 buf[46] = (buf[46] & 0xcf) | pan;
	 buf[57] = (buf[57] & 0xcf) | pan;
	 if (mode == 1 && reverb) {
	      unsigned val;
	      val = buf[43] & 0x0f;
	      if (val > 0)
		   val--;
	      buf[43] = (buf[43] & 0xf0) | val;
	      val = buf[54] & 0x0f;
	      if (val > 0)
		   val--;
	      buf[54] = (buf[54] & 0xf0) | val;
	 }
    }
}

/*
 * code adapted from midithru.c
 */

void stop_note(int note, int velocity)
{
  int i, op=255;

  for (i=0;i<num_voices && op==255;i++) {
    if (oper_note[i]== note) op=i;
  }

  if (op==255)
    {
/*        printf("Note %d off, note not started\n", note); */
/*        printf("%d, %d\n", flen, nlen); */
      return;	/* Has already been killed ??? */
    }

  SEQ_STOP_NOTE(sb_dev, op, note, velocity);
  SEQ_DUMPBUF();

  oper_note[op] = 255;

  free_list[ftail]=op;
  flen++;
  ftail = (ftail+1) % num_voices;

  for (i=0;i<16;i++)
    if (note_list[i] == op) note_list[i] = 255;

  while (nlen && note_list[nhead] == 255) {
      nlen--;
      nhead = (nhead+1) % 256;
    }
  free_voices++;
}

void kill_one_note()
{
     int oldest;

     if (!nlen) {
	  printf("Free list empty but no notes playing\n");
	  return;
     }	/* No notes playing */

     oldest = note_list[nhead];
     nlen--;
     nhead = (nhead+1) % 256;

     printf("Killing oper %d, note %d\n", oldest, oper_note[oldest]);

     if (oldest== 255)
	  return;	/* Was already stopped. Why? */

     stop_note(oper_note[oldest], 127);
}

void start_note(int note, int velocity)
{
     int free;
     /* for the csound output */
     notetime[note] = sec() + rint(usec()/precision)*(precision/1e6);

     if (!flen) kill_one_note();

     if (!flen) {printf("** no free voices\n");return;}	/* Panic??? */

     free = free_list[fhead];
     flen--;
     fhead = (fhead+1) % num_voices;

     note_list[ntail] = free;

     if (nlen>255) {
	  nlen=0;	/* Overflow -> hard reset */
     }
     nlen++;
     ntail = (ntail+1) % 256;

     oper_note[free] = note;

     SEQ_SET_PATCH(sb_dev, free, pgm);
     SEQ_PITCHBEND(sb_dev, free, bender);
     SEQ_START_NOTE(sb_dev, free, note, velocity);
     SEQ_DUMPBUF();
     free_voices--;
}

void channel_pressure(int ch, int pressure)
{
     int i;
     for (i=0;i<num_voices;i++) {
	  if (oper_note[i] != 255) {
	       SEQ_CHN_PRESSURE(sb_dev, i, pressure);
	       SEQ_DUMPBUF();
	  }
     }
}

void pitch_bender(int ch, int value)
{
     int i;
     value -= 8192;
     bender = value;
     for (i=0;i<num_voices;i++) {
	  if (oper_note[i] != 255) {
	       bender = value;
	       SEQ_PITCHBEND(sb_dev, i, value);
	       SEQ_DUMPBUF();
	  }
     }
     exit(0);
}

void do_buf()
{
     int ch = buf[0] & 0x0f;
     int value;

     switch (buf[0] & 0xf0) {
     case 0x90:	/* Note on */
/*         printf("note\n"); */
       if (bufp < 3) break;
       if (buf[2])
	 start_note(buf[1], buf[2]);
       else
	 stop_note(buf[1], buf[2]);
       bufp=1;
       break;

     case 0xb0:	/* Control change */
/*         printf("control\n"); */
       if (bufp < 3) break;
       bufp=1;
       break;

     case 0x80:	/* Note off */
/*         printf("off\n"); */
/*    printf("off: bufp = %i\n", bufp); */
       if (bufp < 3) break;
/*    printf("off hej\n"); */
       stop_note(buf[1], buf[2]);
       bufp=1;
/*         printf("off2\n"); */
       break;

     case 0xe0:	/* Pitch bender */
/*         printf("bend\n"); */
       if (bufp < 3) break;
       value = ((buf[2] & 0x7f) << 7) | (buf[1] & 0x7f);
       pitch_bender(ch, value);
       bufp=1;
       break;

     case 0xc0:	/* Pgm change */
/*         printf("pgm\n"); */
       if (bufp < 2) break;
       pgm = buf[1];
       instr = pgm;
       bufp=0;
       break;

     case 0xd0:	/* Channel pressure */
/*         printf("pressure\n"); */
       if (bufp < 2) break;
       channel_pressure(ch, buf[1]);
       bufp=1;
       break;

     default:
/*         printf(".\n"); */
       bufp=0;
     }
}

void read_midi(int fd)
{
  unsigned char ev[4];
  int i,n;

  if((n=read(seqfd, ev, sizeof(ev)))==-1) {
       printf("Error reading " SEQUENCER_PATH);
       exit(-1);
  }
  for(i=0; i<=(n/4); i++) {
    unsigned char *p = &ev[i*4];

    if(p[0]==SEQ_MIDIPUTC && p[2]==0) {
      if(p[1] & 0x80) { /* Status */
	if(bufp) do_buf();
	buf[0] = p[1]; bufp=1;
      } else
	if(bufp) {
	  buf[bufp++] = p[1];
	  if ((buf[0] & 0xf0) == 0x90 || (buf[0] & 0xf0) == 0x80
	      || (buf[0] & 0xf0) == 0xb0 || (buf[0] & 0xf0) == 0xe0) {
	    if(bufp==3) do_buf();
	  } else if ((buf[0] & 0xf0) == 0xc0 || (buf[0] & 0xf0) == 0xd0) {
	    if(bufp==2) do_buf();
	  };

	};
    };
  };
};


long sec(void)
{
     int err;
/*       if (( err = gettimeofday(&timev,&timez)) == -1 ) { */
/*  	  printf("Gettimeofday error\n"); */
/*       }   */
/*       return(timev.tv_sec); */
}

long usec(void)
{
     int err;
/*       if (( err = gettimeofday(&timev,&timez)) == -1 ) { */
/*  	  printf("Gettimeofday error\n"); */
/*       } */
/*       return(timev.tv_usec); */
}

#endif /* HAVE_SYS_SOUNDCARD_H */
