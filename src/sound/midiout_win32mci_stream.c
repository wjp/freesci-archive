/***************************************************************************
 midiout_win32mci_stream.c Copyright (C) 2001,2002 Alexander R Angas

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

 Current maintainer: Alexander R Angas <wgd@internode.on.net>

 Number of ticks to hold in buffer is set by set_parameter().
 The sound server writes events at a faster rate than what can be output.
 Each event sent is encoded in MCI format and added to a buffer.
 Once the buffer is full (has enough data for required number of ticks), a
 second buffer is filled. The rest of the song is stored in a third buffer.
 The process then repeats with a new buffer.
 If flush(0) is called, the song is played with each buffer going in
 sequence to the MIDI out driver.
 If flush(1) is called, all buffers are cleared in preparation for new data.

***************************************************************************/

#include <midiout.h>

#ifdef _WIN32

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <windows.h>
#include <mmsystem.h>
#include <sci_memory.h>
#include <engine.h>

#define NUMBER_OF_SONG_BUFFERS 1000
#define PLAY_BUFFER		0
#define CONTINUE_BUFFER	1

//static HANDLE midi_canplay_event;	/* if MIDI allowed to play then signalled */
static MIDIHDR midiOutHdr;			/* currently used device header */
static HMIDISTRM midiStream;		/* currently playing stream */
static int midiDeviceNum = -1;

static unsigned int ticks_in_buffer = 15;	/* number of ticks to store in a buffer */

struct song_buffer {
	LPDWORD data;		/* the buffer */
	unsigned int pos;	/* location in buffer of where next MCI event will go
						** (doubles as size of buffer when completed a buffer) */
};

static struct song_buffer buffered_song[NUMBER_OF_SONG_BUFFERS];
/* array of song buffers */

static unsigned int buffi = 0;
/* total number of actual song buffers stored in buffered_song[] */

static unsigned int unbuffered = 0;
/* most recent point in song that is unbuffered */

static unsigned int next_flush_buff = 0;
/* next buffer to send to MCI */

static word current_handle = 0;
/* local copy of handle for MIDI data currently playing (0 if not associated with song) */

static sound_server_state_t *sss = 0;
/* local copy of sound server state */


void CALLBACK _streamCallback(HMIDIOUT hmo, UINT wMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2);


void _win32mci_stream_print_error(int ret)
{
	char err[MAXERRORLENGTH];

	midiOutGetErrorText(ret, err, MAXERRORLENGTH);
	fprintf(stderr, "\n %s\n", err);
}

static int
midiout_win32mci_stream_set_parameter(struct _midiout_driver *drv, char *attribute, char *value)
{
	if (NULL == value)
	{
		sciprintf("midiout_win32mci_stream_set_parameter(): NULL passed for value.\n");
		return -1;
	}

	if (!strcasecmp(attribute, "device"))
	{
		midiDeviceNum = ((int)*value) - 48;
	}
	else if (!strcasecmp(attribute, "ticks_in_buffer"))
	{
		ticks_in_buffer = ((int)*value) - 48;
	}
	else
	{
		sciprintf("Unknown win32mci_stream option '%s'!\n", attribute);
	}

	return 0;
}

int midiout_win32mci_stream_open()
{
	UINT numdevs;				/* total number of MIDIout devices */
	MMRESULT ret;				/* return value of MCI calls */
	MIDIOUTCAPS devicecaps;		/* device capabilities structure */
	MIDIPROPTIMEDIV prop;
	UINT loop;
	int *device_scores;

	numdevs = midiOutGetNumDevs();
	if (numdevs == 0)
	{
		fprintf(stderr, "No win32 MCI MIDI output devices found!\n");
		return -1;
	}

	device_scores = (int*)sci_malloc(numdevs * sizeof(int));
	fprintf(stderr, "MCI MIDI output devices found: %d\n", numdevs);

	if (midiDeviceNum == -1)
	{
		int max_score = -10;

		for (loop = 0; loop < numdevs; loop++)
		{
			ret = midiOutGetDevCaps(loop, &devicecaps, sizeof(devicecaps));
			if (MMSYSERR_NOERROR != ret)
			{
				fprintf(stderr, "midiOutGetDevCaps: ");
				_win32mci_stream_print_error(ret);
				sci_free(device_scores);
				return -1;
			}

			fprintf(stderr, "MIDI Out %02d: ", loop);
			fprintf(stderr, "[%s]\n             ", devicecaps.szPname);

			/* which kind of device would be the best to default to? */
			/*   ignore hardware ports because they may not be connected
			**   midi mapper is what is selected in control panel and should be first preference
			**   software synths will take a lot of cpu time so they should be lower
			**   devices supporting MIDICAPS_STREAM should be higher
			**   1. MOD_MAPPER (but doesn't support streaming)
			**   2. MOD_WAVETABLE
			**   3. MOD_SQSYNTH
			**   4. MOD_SYNTH
			**   5. MOD_FMSYNTH
			**   6. MOD_SWSYNTH
			**   7. MOD_MIDIPORT
			*/
			switch (devicecaps.wTechnology)
			{
#ifdef MOD_WAVETABLE
				case MOD_WAVETABLE:
					fprintf(stderr, "Hardware wavetable synth, ");
					device_scores[loop] = 6;
					break;
#endif

				case MOD_SQSYNTH:
					fprintf(stderr, "Square wave synth, ");
					device_scores[loop] = 5;
					break;

				case MOD_SYNTH:
					fprintf(stderr, "Generic synth, ");
					device_scores[loop] = 4;
					break;

				case MOD_FMSYNTH:
					fprintf(stderr, "FM synth, ");
					device_scores[loop] = 3;
					break;

#ifdef MOD_SWSYNTH
				case MOD_SWSYNTH:
					fprintf(stderr, "Software synth, ");
					device_scores[loop] = 2;
					break;
#endif

				case MOD_MIDIPORT:
					fprintf(stderr, "MIDI hardware port, ");
					device_scores[loop] = 1;
					break;

				case MOD_MAPPER:
					fprintf(stderr, "MIDI mapper, ");
					midiDeviceNum = loop;
					device_scores[loop] = -1;	/* cannot be used for streaming */
					break;

				default:
					fprintf(stderr, "Unknown synth, ");
					device_scores[loop] = 0;
					break;
			}

			fprintf(stderr, "%d voices, ", devicecaps.wVoices);
			if (devicecaps.dwSupport & MIDICAPS_STREAM)
			{
				fprintf(stderr, "supports streaming, ");
				device_scores[loop] += 2;
			}
			fprintf(stderr, "score: %i\n", device_scores[loop]);
		}

		/* set device to use based on score */
		for (loop = 0; loop < numdevs; loop++)
		{
			if (device_scores[loop] > max_score)
			{
				midiDeviceNum = loop;
				max_score = device_scores[loop];
			}
		}
		sci_free(device_scores);
	}

	/* set up HMIDISTRM */
	memset(&midiStream, 0, sizeof(HMIDISTRM));
	ret = midiStreamOpen(&midiStream,
		&midiDeviceNum,
		1,
		(DWORD)_streamCallback,
		0,
		CALLBACK_FUNCTION);
    if (ret != MMSYSERR_NOERROR)
    {
        fprintf(stderr, "midiStreamOpen: ");
        _win32mci_stream_print_error(ret);
		return -1;
    }
	else
	{
		fprintf(stderr, "Successfully opened for MCI MIDI device #%02d\n", midiDeviceNum);
	}

	/* Set the stream device's Time Division to 96 PPQN */
#if 0
	prop.cbStruct = sizeof(MIDIPROPTIMEDIV);
	prop.dwTimeDiv = 96;
	ret = midiStreamProperty(midiStream, (LPBYTE)&prop, MIDIPROP_SET | MIDIPROP_TIMEDIV);
    if (ret != MMSYSERR_NOERROR)
    {
        fprintf(stderr, "midiStreamOpen: ");
        _win32mci_stream_print_error(ret);
		return -1;
    }
#endif
	buffered_song[0].data = (LPDWORD)sci_malloc(3 * sizeof(DWORD));

#if 0
	/* set up midi_canplay_event */
	midi_canplay_event = CreateEvent(NULL, FALSE, TRUE, NULL);
	if (midi_canplay_event == NULL)
	{
        fprintf(stderr, "midiout_win32mci_stream_open(): CreateEvent failed\n");
	}
#endif

	return 0;
}

int midiout_win32mci_stream_close()
{
	/* TODO: Check if need to unprepare header */
	MMRESULT ret;

	ret = midiStreamClose(midiStream);
	if (ret != MMSYSERR_NOERROR)
	{
		printf("midiStreamClose() failed: ");
		_win32mci_stream_print_error(ret);
	}

	/* TODO: free buffer memory */

	return 0;
}

int midiout_win32mci_stream_write_event(guint8 *buffer, unsigned int count, guint32 other_data)
{
	/* Puts buffer into format required by MCI (both long and short messages) */

	static guint8 running_status;	/* last encountered MIDI msg */
	static unsigned int ticks_so_far;		/* ticks in this buffer so far */
	guint8 delta_time;	/* time before status is sent to device */

	guint8 status;		/* MIDI status byte */
	guint8 data1;		/* data byte 1 (if it exists) */
	guint8 data2;		/* data byte 2 (if it exists) */
	guint8 num_params;	/* number of parameters for this MIDI status */

	/* Unpack buffer (for short messages only) */
	status = *buffer;	/* get status byte */

	if (MIDI_RUNNING_STATUS_BYTE(status))
	{
		/* set first data byte when in running status mode */
		data1 = status;
		status = running_status;		/* set status byte to running status byte */
		buffer++;	/* now lined up to next parameter or a new status */
	}
	else
	{
		/* set first data byte when not in running status mode */
		buffer++;
		data1 = *buffer;
		buffer++;
	}

	/* set number of parameters for status byte */
	num_params = (MIDI_PARAMETERS_ONE_BYTE(status) ? 1 : 2);

	if (num_params > 1)
		data2 = *buffer;	/* set second parameter if there is one */

	delta_time = other_data;
//fprintf(stderr, "%i\n", delta_time);

	/* Finished preparing data, now pack it up for MCI */

	/* first DWORD in MCI event - number of ticks */
	buffered_song[buffi].data[
		buffered_song[buffi].pos++
		] = delta_time;
	ticks_so_far += delta_time;	/* increment ticks that have been stored so far stored in buffer */

	/* see if need a new buffer */
	if (ticks_so_far > ticks_in_buffer)
	{
#if 0
		buffi++;	/* move to next buffer in buffered_song[] */

		/* make sure not over number of buffers */
		if (buffi >= NUMBER_OF_SONG_BUFFERS)
		{
			fprintf(stderr, "midiout_win32mci_stream_write_event(): Out of song buffers, please increase NUMBER_OF_SONG_BUFFERS\n");
			return -1;
		}

		/* make room for a MIDI event in the new buffer */
		buffered_song[buffi].data = (LPDWORD)sci_malloc(3 * sizeof(DWORD));

		/* copy the last byte of data from the previous buffer to the new
		** buffer (time code for the first MIDI event of the new buffer) */
		buffered_song[buffi].data[
			buffered_song[buffi].pos
			] = buffered_song[buffi - 1].data[buffered_song[buffi - 1].pos - 1];
		ticks_so_far = buffered_song[buffi].data[
			buffered_song[buffi].pos++
			];		/* reset number of ticks counter */

		/* realloc() to remove that byte from the previous buffer */
		buffered_song[buffi - 1].data =
			(LPDWORD)sci_realloc(buffered_song[buffi - 1].data,
				(buffered_song[buffi - 1].pos - 1) * sizeof(DWORD));

		buffered_song[buffi - 1].pos--;	/* set old pos to be size of old buffer */
#endif
#if 0
		{
			unsigned int i;
			for (i = 0; i < buffered_song[buffi - 1].pos; i++)
fprintf(stderr, "buffi - 1: %03d  code: %08x\n", buffi - 1, buffered_song[buffi - 1].data[i]);
		}
#endif
	}

	buffered_song[buffi].data[
		buffered_song[buffi].pos++
		] = 0;		/* required to be 0 by MCI */

	buffered_song[buffi].data[
		buffered_song[buffi].pos
		] = MEVT_SHORTMSG << 24;	/* high word, high byte */

	if (!(MIDI_RUNNING_STATUS_BYTE(status)))
	{
		buffered_song[buffi].data[
			buffered_song[buffi].pos
			] |= status;	/* low word, low byte */

		buffered_song[buffi].data[
			buffered_song[buffi].pos
			] |= (data1 << 8);	/* low word, high byte */

		if (num_params >= 2)
			buffered_song[buffi].data[
				buffered_song[buffi].pos
				] |= (data2 << 16);	/* high word, low byte */
	}
	else	/* running status mode */
	{
		buffered_song[buffi].data[
			buffered_song[buffi].pos
			] |= data1;			/* low word, low byte */

		if (num_params >= 2)
			buffered_song[buffi].data[
				buffered_song[buffi].pos
				] |= (data2 << 8);	/* high word, low byte */
	}

	buffered_song[buffi].data[
		buffered_song[buffi].pos++
		] |= MEVT_F_SHORT;

//fprintf(stderr, "buffi: %03d  time: %08x  zero: %08x  evnt: %08x\n", buffi, buffered_song[buffi].data[buffered_song[buffi].pos - 3], buffered_song[buffi].data[buffered_song[buffi].pos - 2], buffered_song[buffi].data[buffered_song[buffi].pos - 1]);

	/* resize to make room for next command */
	buffered_song[buffi].data = sci_realloc(
		buffered_song[buffi].data, (buffered_song[buffi].pos + 3) * sizeof(DWORD));

	return count;
}

void CALLBACK _streamCallback(HMIDIOUT hmo, UINT wMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2)
{
	MMRESULT ret;

	switch (wMsg)
	{
	case MOM_DONE:
		fprintf(stderr, "_streamCallback(): MOM_DONE received\n");

		ret = midiOutUnprepareHeader((HMIDIOUT)midiStream, &midiOutHdr, sizeof(midiOutHdr));
		if (ret != MMSYSERR_NOERROR)
		{
            printf("midiOutUnprepareHeader() failed: ");
            _win32mci_stream_print_error(ret);
        }

#if 0
		if (SetEvent(midi_canplay_event) == 0)
		{
			fprintf(stderr, "_streamCallback(): SetEvent failed\n");
		}
#endif

//		next_flush_buff++;
//		midiout_win32mci_stream_flush(CONTINUE_BUFFER);	/* play next buffer */

		break;

	case MOM_OPEN:
		fprintf(stderr, "_streamCallback(): MOM_OPEN received\n");
		break;

	case MOM_CLOSE:
		fprintf(stderr, "_streamCallback(): MOM_CLOSE received\n");
		break;
    }
}

#if 0
int _clear_buffered_song()
{
	unsigned int i;
	MMRESULT ret;

	/* stop the tunes */
	ret = midiStreamStop(midiStream);
	if (ret != MMSYSERR_NOERROR)
	{
		fprintf(stderr, "midiStreamStop: ");
		_win32mci_stream_print_error(ret);
		return -1;
	}

	/* wipe the buffers */
	for (i = 0; i < buffi; i++)
	{
		sci_free(buffered_song[i].data);
		buffered_song[i].pos = 0;
	}
	buffi = 0;
	next_flush_buff = 0;
}
#endif

int midiout_win32mci_stream_flush(guint8 code)
{
    MMRESULT ret;	/* stores return values from MCI functions */
	static unsigned int play_times = 0;

//fprintf(stderr, "midiout_win32mci_stream_flush(): code %i\n", code);

	if (play_times)
	{
		printf("Sorry, can't play any more\n");
		return -1;
	}

	if (code == PLAY_BUFFER)
	{
		/* buffers should not be empty */
		if (buffered_song[0].pos == 0)
		{
			fprintf(stderr, "midiout_win32mci_stream_flush(): Cannot play empty buffer\n");
			return -1;
		}

		next_flush_buff = 0;	/* start from first buffer */
	}
#if 0
	else if (code == CONTINUE_BUFFER)
	{
		/* see if reached end of buffers */
		if (buffered_song[next_flush_buff].pos == 0)
		{
			if (current_handle == 0)
			{
				/* not a song so clear the buffers */
				_clear_buffered_song();
			}
			else
			{
				/* a song so leave buffers intact but call end_of_track() */
			}
		}
		/* else play next buffer as indicated by next_flush_buff */
	}
#endif

#if 0
	if (WaitForSingleObject(midi_canplay_event, INFINITE) != WAIT_OBJECT_0)
	{
		fprintf(stderr, "midiout_win32mci_stream_write_event(): WFSO failed\n");
		return -1;
	}
#endif

	/* set up MIDIHDR struct */
	memset(&midiOutHdr, 0, sizeof(MIDIHDR));
	midiOutHdr.lpData = (LPBYTE)buffered_song[next_flush_buff].data;
	midiOutHdr.dwBufferLength =
		buffered_song[next_flush_buff].pos * sizeof(DWORD);
	midiOutHdr.dwBytesRecorded =
		buffered_song[next_flush_buff].pos * sizeof(DWORD);

#if 0
	{
		unsigned int i;
		for (i = 0; i < buffered_song[next_flush_buff].pos; i++)
		{
			fprintf(stderr, "next_flush_buff: %03d  code: %08x\n", next_flush_buff, buffered_song[next_flush_buff].data[i]);
		}
	}
#endif

	ret = midiOutPrepareHeader((HMIDIOUT)midiStream,
		&midiOutHdr,
		sizeof(MIDIHDR));
	if (ret != MMSYSERR_NOERROR)
	{
		fprintf(stderr, "midiOutPrepareHeader: ");
		_win32mci_stream_print_error(ret);
		return -1;
	}

	/* play */
	ret = midiStreamOut(midiStream,
		&midiOutHdr,
		sizeof(MIDIHDR));
	if (ret != MMSYSERR_NOERROR)
	{
		fprintf(stderr, "midiStreamOut: ");
		_win32mci_stream_print_error(ret);
		return -1;
	}

	ret = midiStreamRestart(midiStream);
	if (ret != MMSYSERR_NOERROR)
	{
		fprintf(stderr, "midiStreamRestart: ");
		_win32mci_stream_print_error(ret);
		return -1;
	}

	return 0;
}

midiout_driver_t midiout_driver_win32mci_stream = {
  "win32mci_stream",
  "0.3",
  &midiout_win32mci_stream_set_parameter,
  &midiout_win32mci_stream_open,
  &midiout_win32mci_stream_close,
  &midiout_win32mci_stream_write_event,
  &midiout_win32mci_stream_flush
};
#endif
