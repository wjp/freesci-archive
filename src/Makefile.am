SUBDIRS = scicore engine include gfx sfx tools win32
EXTRA_DIST = README yywrap.c config.l sciv dc/3dutils.c dc/LICENSE.txt \
	dc/Makefile dc/README.KOS dc/aica_cmd_iface.h dc/bkg.c \
	dc/bsearch.c dc/config.h dc/dc.c dc/dc.h dc/dc_save.c \
	dc/gamemenu.c dc/gp.h dc/mouse1.h dc/selectgame.c dc/selectgame.h \
	dc/sintab.h dc/snd_stream.c dc/snprintf.c dc/stdio.c dc/texture.c \
	dc/vmu_pkg.c dc/fs_iso9660.c
INCLUDES = -I$(top_srcdir)/src/include
AM_CFLAGS = $(SDL_CFLAGS)
LDADD = engine/libsciengine.a gfx/libscigraphics.a \
	gfx/resource/libsciresources.a gfx/drivers/libscidrivers.a \
	sfx/libscisound.a scicore/libscicore.a \
	sfx/player/libsciplayer.a \
	sfx/seq/libsciseq.a sfx/timer/libscitimer.a \
	sfx/pcm_device/libscipcm.a sfx/mixer/libscimixer.a \
	sfx/softseq/libscisoftseq.a \
	sfx/libscisoundlib.a \
	sfx/device/libscisounddevice.a \
	@ac_readline@ @ac_curses_libraries@ @ac_png_libraries@ -lm @SDL_LIBS@
bin_PROGRAMS = freesci
bin_SCRIPTS = sciv
freesci_SOURCES = config.l main.c
freesci_LDFLAGS = @SCIV_LDFLAGS@
