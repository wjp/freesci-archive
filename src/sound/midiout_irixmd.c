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


#undef DEBUG_OUTPUT

static int numinterfaces;
static MDport midiport;
static int fd;
static fd_set fdset;

static char *port = NULL; /* "Software Synth";*/
static int sleeptime = 0;

static int
midiout_sgimd_set_parameter(struct _midiout_driver *drv, char *attribute, char *value)
{
	if (!strcasecmp(attribute, "port")) {
		port = value;
#ifdef DEBUG_OUTPUT
		fprintf(stderr,"MIDI: configured device: %s\n",port);
#endif
	} else if (!strcasecmp(attribute, "sleeptime")) {
		sleeptime = atoi(value);
#ifdef DEBUG_OUTPUT
		fprintf(stderr,"MIDI: sleep %i usecs to flush.\n",sleeptime);
#endif
	} else
		sciprintf("Unknown sgimd option '%s'!\n", attribute);

	return 0;
}

int midiout_sgimd_open()
{
	int i;

	numinterfaces=mdInit();
	if (numinterfaces<=0) {
		fprintf(stderr,"No MIDI interfaces configured.\n");
		perror("Cannot initialize libmd for sound output");
		return -1;
	}

	for (i=0; i<numinterfaces; i++) {
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

	mdSetStampMode(midiport, MD_NOSTAMP); /* don't use Timestamps */
	return 0;
}

int midiout_sgimd_close()
{
	mdClosePort(midiport);
	return 0;
}

int midiout_sgimd_flush()
{
        GTimeVal timeout = {1, 1};

	FD_ZERO(&fdset);
	FD_SET(fd,&fdset);

#ifdef DEBUG_OUTPUT
	fprintf(stderr,"select (fd=%i)...",fd);
#endif
	select(fd+1, NULL, &fdset, NULL, (struct timeval *)&timeout);
	/* _I_ have to make it sleep a bit. My MIDI interface is
           faster than the MIDI device, so SYSEX msgs may get lost... */
	usleep(50000);
#ifdef DEBUG_OUTPUT
	fprintf(stderr," done.\n");
#endif
	return 0;
}

int midiout_sgimd_write(guint8 *buffer, unsigned int count)
{
	MDevent event;
	int i;


	event.stamp=1;
	event.msglen=count;
	if (buffer[0]==MD_SYSEX) {  /* SYSEX */
		event.sysexmsg=buffer;
		event.msg[0]=MD_SYSEX;
		if (buffer[count-1]!=MD_EOX) {
			fprintf(stderr,"broken SYSEX MIDI Event of length %u\n",count);
			return 1;
		}
#ifdef DEBUG_OUTPUT
		fprintf(stderr,"sending SYSEX...");
#endif
	} else {
		event.sysexmsg=NULL;
		if (count>4) { 
			fprintf(stderr,"broken non-SYSEX MIDI Event of length %u\n",count); 
		} else {
			for (i=0; i<count; i++) event.msg[i]=buffer[i];
		}
#ifdef DEBUG_OUTPUT
		fprintf(stderr,"sending MIDI event...");
#endif
	}

	if (mdSend(midiport, &event, 1)!=1) {
		fprintf(stderr,"failed sending MIDI event (dump follows...)\n");
		fprintf(stderr,"MIDI Event (len=%u):",count);
		for (i=0; i<count; i++) fprintf(stderr,"%02x ",(int)buffer[i]);
		fprintf(stderr,"\n");
	}
#ifdef DEBUG_OUTPUT
	fprintf(stderr," done.\n"); 
#endif
}

midiout_driver_t midiout_driver_sgimd = {
	"sgimd",
	"v0.1",
	&midiout_sgimd_set_parameter,
	&midiout_sgimd_open,
	&midiout_sgimd_close,
	&midiout_sgimd_write,
	&midiout_sgimd_flush
};

#endif /* HAVE_DMEDIA_MIDI_H */
