/***************************************************************************
 midiout_sgimd.c Copyright (C) 02 Rainer Canavan


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

#include <midiout.h>

#ifdef HAVE_DMEDIA_MIDI_H
#include <dmedia/midi.h>
#include <stdio.h>

#include <unistd.h>
#include <sys/types.h>
#include <bstring.h>
#include <sys/time.h>

#define DEBUG_OUTPUT 1
#undef DEBUG_OUTPUT 

#ifdef DEBUG_OUTPUT
#define dfprintf fprintf
#define mididump  dfprintf(stderr,"  %i(%1x: %3i,%3i) ",count, event.msg[0]&MD_CHANNELMASK, event.msg[1], event.msg[2])
#else 
#define dfprintf if(0)fprintf
#define mididump
#endif

static int numinterfaces;
static MDport midiport;
static int fd;
static byte last_op = 0;

unsigned long long absolutetime;
unsigned long long now = 0;     /* UST of current time */

static fd_set fdset;

static char *port = NULL; /* "Software Synth";*/
static int sleeptime = 50000;

static int
midiout_sgimd_set_parameter(struct _midiout_driver *drv, char *attribute, char *value)
{
    if (!strcasecmp(attribute, "port")) {
        port = value;
        dfprintf(stderr,"MIDI: configured device: %s\n",port);
    } else if (!strcasecmp(attribute, "sleeptime")) {
        sleeptime = atoi(value);
        dfprintf(stderr,"MIDI: sleep %i usecs to flush.\n",sleeptime);
    } else
        sciprintf("Unknown sgimd option '%s'!\n", attribute);

    return 0;
}

int midiout_sgimd_open()
{
    int i;

    numinterfaces = mdInit();
    if (numinterfaces <= 0) {
        fprintf(stderr,"No MIDI interfaces configured.\n");
        perror("Cannot initialize libmd for sound output");
        return -1;
    }

    for (i = 0; i<numinterfaces; i++) {
        fprintf(stderr,"MIDI interface %i: %s\n", i, mdGetName(i));
    }

    midiport = mdOpenOutPort(port);
    if (!midiport) {
        fprintf(stderr,"Unable to open MIDI interfaces %s.\n", port);
        perror("Cannot open sound output.");
        return -1;
    }
    fd = mdGetFd(midiport);
    if (!fd) {
        fprintf(stderr,"While trying to open '%s':\n", port);
        perror("Failed to open for sound output");
        mdClosePort(midiport);
        return -1;
    }

    absolutetime = 0;

    mdSetStampMode(midiport, MD_NOSTAMP);  /* don't use Timestamps */
    /* mdSetStampMode(midiport, MD_RELATIVETICKS);
    mdSetStampMode(midiport, MD_DELTATICKS);
    mdSetDivision(midiport, 1);
    mdSetTempo(midiport, 60000);
    mdSetTemposcale(midiport,1.0);*/

    return 0;
}

int midiout_sgimd_close()
{
    mdClosePort(midiport);
    return 0;
}

int midiout_sgimd_flush(guint8 code)
{
    GTimeVal timeout = {1, 1};

    FD_ZERO(&fdset);
    FD_SET(fd,&fdset);

    dfprintf(stderr,"select (fd=%i)...",fd);
    select(fd+1, NULL, &fdset, NULL, (struct timeval *)&timeout);
    /* _I_ have to make it sleep a bit. My MIDI interface is
       faster than the MIDI device, so long SYSEX msgs may get lost... */
    usleep(sleeptime);

    dfprintf(stderr," done.\n");
    return 0;
}

int midiout_sgimd_write(guint8 *buffer, unsigned int count, guint32 delta)
{
    MDevent event;
    int i;
    GTimeVal timeout = {1, 1};
    byte status;
    int src;

    /* FD_ZERO(&fdset);
    FD_SET(fd,&fdset);
    select(fd+1, NULL, &fdset, NULL, (struct timeval *)&timeout); */

    absolutetime += delta;

    event.stamp = 0;

    if (buffer[0] >= 0x80) {
        last_op = status = buffer[0];
        src = 1;
    } else {
        status = last_op;
        src = 0;
        count++;
    }

    if (status == MD_SYSEX) {  /* SYSEX */
        event.sysexmsg = buffer;
        event.msglen = count;
        event.msg[0] = MD_SYSEX;
        if (buffer[count-1] != MD_EOX) {
            fprintf(stderr,"broken SYSEX MIDI Event of length %u\n",count);
            return 1;
        }
        dfprintf(stderr,"sending SYSEX...");
    } else {
        event.sysexmsg = NULL;
        event.msglen = count;
        if ((status < 0x80) || (count > 4)) { 
            fprintf(stderr,"broken non-SYSEX MIDI Event of length %u\n",count); 
        } else {
            event.msg[0] = status;
            for (i=1; i < count; i++) event.msg[i] = buffer[src++];
            for (; i < 4; i++) event.msg[i] = 0;
        }

#ifdef DEBUG_OUTPUT
        if ((event.msg[0]&MD_CHANNELMASK)!=111) {
            dfprintf(stderr,"MIDI event: ");
            switch(event.msg[0]&MD_STATUSMASK) {
               case MD_NOTEON:              dfprintf(stderr,"NoteON           "); mididump; break;
               case MD_NOTEOFF:             dfprintf(stderr,"NoteOFF          "); event.msg[2]=0; mididump; break;
               case MD_PROGRAMCHANGE:       dfprintf(stderr,"ProgramChange    "); mididump; break;
               case MD_CHANNELMODESELECT:   dfprintf(stderr,"Channelmodeselect");  switch (event.msg[1]){ 
                             case 7:   dfprintf(stderr, " Volume c=%1x, v=%3i", (int)event.msg[0]&MD_CHANNELMASK,event.msg[2]);  break;
                             case 10:  dfprintf(stderr, " Pan:   c=%1x, p=%3i", (int)event.msg[0]&MD_CHANNELMASK,event.msg[2]);  break;
                             case 123: dfprintf(stderr, " ALL Notes OFF c=%1x", (int)event.msg[0]&MD_CHANNELMASK);  break;
                             default: mididump; 
                          } break;
               default: dfprintf(stderr,"%i:unknown(e=%i, c=%i, %i,%i)",count, event.msg[0]&MD_STATUSMASK, 
                                                           event.msg[0]&MD_CHANNELMASK, event.msg[1], event.msg[2]);

            } 
            dfprintf(stderr,"@%i.\n", absolutetime); 
        }
#endif
    }

    /* event.stamp=delta;
    if (now==0) {
        dmGetUST(&now);
        mdSetStartPoint(midiport, (long long) now, 0);
    }*/

    /*if (delta==0) usleep(10000000); 
    dfprintf(stderr," delta: %6i\n", absolutetime);*/

    if (mdSend(midiport, &event, 1) != 1) {
        fprintf(stderr,"failed sending MIDI event (dump follows...)\n");
        fprintf(stderr,"MIDI Event (len=%u):",count);
        for (i=0; i<count; i++) fprintf(stderr,"%02x ",(int)buffer[i]);
        fprintf(stderr,"\n");
        return -1;
    }
    return 1;
}

midiout_driver_t midiout_driver_sgimd = {
    "sgimd",
    "v0.11",
    &midiout_sgimd_set_parameter,
    &midiout_sgimd_open,
    &midiout_sgimd_close,
    &midiout_sgimd_write,
    &midiout_sgimd_flush
};

#endif /* HAVE_DMEDIA_MIDI_H */


