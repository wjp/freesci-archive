INCLUDES = -I$(top_srcdir)/src/include
CFLAGS = $(SDL_CFLAGS)
LDADD = -lc
EXTRA_DIST = sfx_save.cfsml soundserver_dos.c midiout_win32mci.c
noinst_LIBRARIES = libscisound.a
libscisound_a_SOURCES = oldmidi.c sound.c sfx_save.c midi_mt32.c midiout.c midiout_unixraw.c midiout_alsaraw.c midi_device.c midi_mt32gm.c midiout_ossseq.c midiout_ossopl3.c soundserver_sdl.c soundserver.c midi_adlib.c soundserver_unix.c
sfx_save.c: sfx_save.cfsml
	cat sfx_save.cfsml | ../engine/cfsml.pl @CFSML_FLAGS@ -f sfx_save.cfsml > sfx_save.c

datadir = $(prefix)/share/games/freesci
data_DATA =

