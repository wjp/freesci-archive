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

//#define CHECK_MIDI_DATA

#define DECLARE_MIDIEVENT(x)\
	typedef struct {\
		DWORD dwDeltaTime;\
		DWORD dwStreamID;\
		DWORD dwEvent;\
		DWORD dwParms[x];\
	} MIDIEVENT##x

DECLARE_MIDIEVENT(0);
DECLARE_MIDIEVENT(2);
//MIDIPROPTIMEDIV prop;		/* used to set tempo */

static HANDLE midi_canplay_event;	/* if MIDI allowed to play then signalled */
static MIDIHDR midiOutHdr;			/* currently used device header */
static HMIDISTRM midiStream;		/* currently playing stream */
static int midiDeviceNum = -1;

static unsigned int buffered_ticks = 15;	/* number of ticks to store in a buffer */

static guint8 end_of_track = 0;	/* HACK to tell if end of track */

struct song_buffer {
	LPDWORD data;		/* the buffer */
	unsigned int size;	/* location in buffer of where next MCI event will go
						** (doubles as size of buffer) */
};

static struct song_buffer *waiting_buffer, *playing_buffer;
/* the two song buffers */


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
	UINT loop;					/* temporary */
	int *device_scores;			/* array to score each device based on MIDI capabilities */

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
		fprintf(stderr, "Successfully opened for MCI MIDI device #%02d and buffering %i ticks\n", midiDeviceNum, buffered_ticks);
	}

#if 0
	/* Set tempo */
	prop.cbStruct = sizeof(MIDIPROPTIMEDIV);
	prop.dwTimeDiv = 24;
	ret = midiStreamProperty((HMIDIOUT)midiStream, (LPBYTE)&prop, MIDIPROP_SET|MIDIPROP_TIMEDIV);
    if (ret != MMSYSERR_NOERROR)
    {
        fprintf(stderr, "midiStreamProperty (time): ");
        _win32mci_stream_print_error(ret);
		return -1;
    }
#endif

	/* Set up the two buffers */
	waiting_buffer = sci_malloc(sizeof(struct song_buffer));
	waiting_buffer->data = NULL;
	waiting_buffer->size = 0;

	playing_buffer = sci_malloc(sizeof(struct song_buffer));
	playing_buffer->data = NULL;
	playing_buffer->size = 0;

	/* set up midi_canplay_event */
	midi_canplay_event = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (midi_canplay_event == NULL)
	{
        fprintf(stderr, "midiout_win32mci_stream_open(): CreateEvent failed\n");
		return -1;
	}

	return 0;
}

int midiout_win32mci_stream_close()
{
	MMRESULT ret;

	ret = midiStreamClose(midiStream);
	if (ret != MMSYSERR_NOERROR)
	{
		printf("midiStreamClose() failed: ");
		_win32mci_stream_print_error(ret);
	}

	/* Free buffer memory */
	if (waiting_buffer->data)
		sci_free(waiting_buffer->data);
	if (playing_buffer->data)
		sci_free(playing_buffer->data);
	sci_free(waiting_buffer);
	sci_free(playing_buffer);

	if (CloseHandle(midi_canplay_event) == 0)
	{
		fprintf(debug_stream, "add_to_buffer(): CloseHandle() failed, GetLastError() returned %u\n", GetLastError());
		return -1;
	}

	return 0;
}

void CALLBACK _streamCallback(HMIDIOUT hmo, UINT wMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2)
{
	switch (wMsg)
	{
	case MOM_DONE:

		/* HACK for non-song iterator-based sound */
		if (end_of_track) {
			do_end_of_track();
			end_of_track = 0;
		}
		/* HACK END */

		/* Ready for a new one */
		if (SetEvent(midi_canplay_event) == 0)
		{
			fprintf(stderr, "_streamCallback(): SetEvent failed\n");
		}

		break;

	case MOM_OPEN:
		fprintf(stderr, "Opened MIDI device\n");
		break;

	case MOM_CLOSE:
		fprintf(stderr, "Closed MIDI device\n");
		break;
    }
}

/*
	addToBuffer():
	Add the given delta time to length of latest buffer
	If delta time exceeds given, then play latest buffer
*/
int add_to_buffer(void *me)
{
	static unsigned int ticks_so_far;		/* ticks in this buffer so far */
	unsigned int old_size = waiting_buffer->size;

	/* HACK that will not be required for song iterator based sound system:
	** check for end of track and if need to flush waiting ticks that are < buffered_ticks */
	if (!me)
	{
		end_of_track = 1;
		if (waiting_buffer->size == 0)
			return 0;
		ticks_so_far = (unsigned int)-1;	/* make check below always true */
	}
	else
	{	/* REQUIRED for song iterator based system */
		/* Add this buffer's ticks to ticks_so_far */
		ticks_so_far += ((MIDIEVENT*)me)->dwDeltaTime;

		/* realloc waiting_buffer and copy over contents of MIDIEVENT */
		waiting_buffer->size += sizeof(MIDIEVENT0)/sizeof(DWORD);
		waiting_buffer->data = sci_realloc(waiting_buffer->data, waiting_buffer->size * sizeof(DWORD));
		memmove(waiting_buffer->data + old_size, me, sizeof(MIDIEVENT0));
	}

	/* time to play? */
	if (ticks_so_far > buffered_ticks)
	{
		MMRESULT ret;
		DWORD dRet;

		/* don't allow anything else to play */
		if (ResetEvent(midi_canplay_event) == 0)
		{
			fprintf(debug_stream, "add_to_buffer(): ResetEvent() failed, GetLastError() returned %u\n", GetLastError());
			return -1;
		}

		/* realloc playing buffer to match size of waiting_buffer, then copy */
		playing_buffer->size = waiting_buffer->size;
		playing_buffer->data = sci_realloc(playing_buffer->data, playing_buffer->size * sizeof(DWORD));
		memmove(playing_buffer->data, waiting_buffer->data, playing_buffer->size * sizeof(DWORD));

		/* Set up and play buffer */
		memset(&midiOutHdr, 0, sizeof(MIDIHDR));
		midiOutHdr.lpData = (LPBYTE)playing_buffer->data;
		midiOutHdr.dwBufferLength = playing_buffer->size * sizeof(DWORD);
		midiOutHdr.dwBytesRecorded = playing_buffer->size * sizeof(DWORD);

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

		dRet = WaitForSingleObject(midi_canplay_event, 0);
		if (dRet == WAIT_OBJECT_0)
		{
			fprintf(debug_stream, "Warning: MIDI buffers not in sync\n");
		}
		else if (dRet == WAIT_TIMEOUT)
		{
			/* wait until midi_canplay */
			if (WaitForSingleObject(midi_canplay_event, INFINITE) != WAIT_OBJECT_0)
			{
				fprintf(debug_stream, "add_to_buffer(): WaitForSingleObject() failed, GetLastError() returned %u\n", GetLastError());
				return -1;
			}
		}
		else
		{
			fprintf(debug_stream, "add_to_buffer(): WaitForSingleObject() failed, GetLastError() returned %u\n", GetLastError());
			return -1;
		}

		/* Finished this buffer */
		ret = midiOutUnprepareHeader((HMIDIOUT)midiStream, &midiOutHdr, sizeof(midiOutHdr));
		if (ret != MMSYSERR_NOERROR)
		{
            printf("midiOutUnprepareHeader() failed: ");
            _win32mci_stream_print_error(ret);
        }

		/* Reset waiting_buffer */
		waiting_buffer->size = 0;
		ticks_so_far = 0;
	}

	return 0;
}

/* Puts buffer into format required by MCI (both long and short messages) */
int midiout_win32mci_stream_write_event(guint8 *buffer, unsigned int count, guint32 other_data)
{
	MIDIEVENT short_me;	/* this MIDI event */
	static guint8 running_status;	/* last encountered MIDI msg */
	guint8 status_code;	/* MIDI status byte */
	guint8 offset;		/* Bit offset for packing parameters */

	/* HACK that will not be required for song iterator based sound system:
	** check for end of track */
	if (other_data == (guint32)-1)
	{
		add_to_buffer(NULL);
		return 0;
	}
	/* HACK END */

	/* Store delta time value in dwDeltaTime */
	short_me.dwDeltaTime = other_data;

	/* Set dwStreamID to 0 */
	short_me.dwStreamID = 0;

	/* Save copy of status byte and set parameter offset for bit packing */
	status_code = *buffer;
	if (!MIDI_RUNNING_STATUS_BYTE(status_code))
	{
		running_status = status_code;
		offset = 8;
		short_me.dwEvent = status_code | MEVT_F_SHORT;
		buffer++;	/* Move along */
	} else if (!MIDI_SYSTEM_BYTE(status_code)) {
		status_code = running_status;
		offset = 0;
		short_me.dwEvent = MEVT_F_SHORT;
	} else {
		offset = 0;
		short_me.dwEvent = MEVT_F_LONG;
		buffer++;
	}

	/* Process short message */
	if (!MIDI_SYSTEM_BYTE(status_code))
	{
#ifdef CHECK_MIDI_DATA
		/* Check contents of buffer */
		if (status_code < 0xE0)
			if (! ( (*buffer >=0) && (*buffer <= 127) ) )
			{
				fprintf(stderr, "MIDI parameter %04x out of range\n", *buffer);
				return -1;
			}
#endif

		/* Add parameters */
		short_me.dwEvent |= (*buffer) << (0 + offset);
		buffer++;
		if (MIDI_PARAMETERS_TWO_BYTE(status_code))
		{
#ifdef CHECK_MIDI_DATA
			/* Check contents of buffer */
			if (status_code < 0xE0)
				if (! ( (*buffer >=0) && (*buffer <= 127) ) )
				{
					fprintf(stderr, "MIDI parameter %04x out of range\n", *buffer);
					return -1;
				}
#endif

			short_me.dwEvent |= (*buffer) << (8 + offset);
			buffer++;
		}

		add_to_buffer(&short_me);
	}
	else
	/* Process long message - example */
	{
		fprintf(stderr, "************************* Long message %04x\n", status_code);
		BREAKPOINT();
#if 0
		/* Depends on what size the message is */
		if (status_code == MIDI_SYSTEM_QFRAME)
		{
			MIDIEVENT2 long_me;
			memmove(&long_me, &short_me, sizeof(short_me));
			long_me.dwEvent |= sizeof(MIDIEVENT2);
			long_me.dwParms[0] = status_code;
			long_me.dwParms[1] = *buffer;
			add_to_buffer(&long_me);
		}
		buffer++;
#endif
	}

	return count;
}

int midiout_win32mci_stream_flush(guint8 code)
{
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
