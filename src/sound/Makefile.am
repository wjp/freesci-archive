INCLUDES = -I$(top_srcdir)/src/include
AM_CFLAGS = $(SDL_CFLAGS)
LDADD = -lc @ac_pthread_libraries@
EXTRA_DIST = sfx_save.cfsml soundserver_dos.c polled_ss_dc.c pcmout_dc.c
noinst_LIBRARIES = libscisound.a
libscisound_a_SOURCES = song_iterator.c oldmidi.c sound.c sfx_save.c	\
		midi_mt32.c midiout.c midiout_unixraw.c 		\
		midiout_alsaraw.c midi_device.c midi_mt32gm.c		\
		midiout_ossseq.c midiout_ossopl3.c thread_ss_sdl.c	\
		soundserver.c midi_adlib.c midi_adlibemu.c fmopl.c	\
		polled_ss_unix.c polled_ss_win32.c midiout_win32mci.c	\
		polled_ss.c pcmout.c pcmout_sdl.c pcmout_alsa.c		\
		midiout_irixmd.c pcmout_al.c pcmout_oss.c fmopl.h	\
		midiout_morphos.c
sfx_save.c: sfx_save.cfsml
	cat sfx_save.cfsml | ../engine/cfsml.pl @CFSML_FLAGS@ -f sfx_save.cfsml > sfx_save.c

datadir = $(prefix)/share/games/freesci
data_DATA =

