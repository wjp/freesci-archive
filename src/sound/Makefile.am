INCLUDES = -I$(top_srcdir)/src/include
LDADD = -lc
EXTRA_DIST = sfx_save.cfsml soundserver_dos.c
noinst_LIBRARIES = libscisound.a
libscisound_a_SOURCES = oldmidi.c sound.c sfx_save.c soundserver_null.c midi_mt32.c midiout.c midiout_unixraw.c midiout_alsaraw.c midi_device.c midi_mt32gm.c midiout_ossseq.c soundserver_sdl.c soundserver.c
sfx_save.c: sfx_save.cfsml
	cat sfx_save.cfsml | ../engine/cfsml.pl > sfx_save.c
