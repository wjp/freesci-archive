/***************************************************************************
 midiout_ossseq.c Copyright (C) 2001 Christoph Reichenbach

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

#ifdef HAVE_SYS_SOUNDCARD_H
#include <sys/soundcard.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#undef DEBUG_OUTPUT

static FILE *recorder = 0;
static int recorder_count = 0;
static int fd;

static byte last_op = 0;
static char *recorder_file;
static char *sequencer_dev = "/dev/sequencer";
static int device_nr = 1;

static int
midiout_ossseq_set_parameter(struct _midiout_driver *drv, char *attribute, char *value)
{
	char *testptr;

	if (!strcasecmp(attribute, "recorder")) {
		recorder_file = value;
		recorder = fopen(recorder_file, "w");
	} else
	if (!strcasecmp(attribute, "device")) {
		device_nr = strtol(value, &testptr, 0);
		if (*testptr) {
			sciprintf("Warning: invalid OSS sequencer device '%s'!\n", value);
			return 1;
		}
	} else
		sciprintf("Unknown ossseq option '%s'!\n", attribute);

	return 0;
}

int midiout_ossseq_open()
{
	fd = open(sequencer_dev, O_WRONLY);
	if (!fd) {
		fprintf(stderr,"While trying to open '%s':\n", sequencer_dev);
		perror("Failed to open for sound output");
		return -1;
	}
	return 0;
}

int midiout_ossseq_close()
{
	close(fd);
	if (recorder)
		fclose(recorder);
	return 0;
}

int midiout_ossseq_flush(guint8 code)
{
	return 0;
}

int midiout_ossseq_write(guint8 *buffer, unsigned int count)
{
	int written;
	int i;
	int src = 1;
	char buf[4];
	byte input;

	if (sequencer_dev == 0)
		return 0;

	buf[0] = SEQ_MIDIPUTC;
	buf[2] = device_nr;
	buf[3] = 0;

	if (buffer[0] >= 0x80) {
#ifdef DEBUG_OUTPUT
		fprintf(stderr, "C ");
#endif
		input = last_op = buffer[0]; /* Copy operation */
	}	else {
#ifdef DEBUG_OUTPUT
		fprintf(stderr, "  ");
#endif
		src = 0;
		count++;
		input = last_op;
	}


	for (i = 0; i < count; i++) {
#ifdef DEBUG_OUTPUT
		fprintf(stderr,"%02x ", input);
#endif
		buf[1] = input;
		written = write(fd, buf, 4);
		if (recorder) {
			fprintf(recorder, "%05d: %02x:  %02x %02x %02x\n",
				recorder_count++, buf[0], buf[1], buf[2], buf[3]);
		}


		if (i+1 < count)
			input = buffer[src++];

		if (written == -1) {
			fprintf(stderr,"While trying to write to '%s':\n", sequencer_dev);
			perror("Sequencer error");
			sequencer_dev = 0;
			return -1;
		}
	}
#ifdef DEBUG_OUTPUT
	fprintf(stderr, "\n");
#endif

	return written;
}

midiout_driver_t midiout_driver_ossseq = {
	"ossseq",
	"v0.1",
	&midiout_ossseq_set_parameter,
	&midiout_ossseq_open,
	&midiout_ossseq_close,
	&midiout_ossseq_write,
	&midiout_ossseq_flush
};

#endif /* HAVE_SYS_SOUNDCARD_H */
