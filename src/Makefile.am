SUBDIRS = scicore engine include gfx sfx tools win32
EXTRA_DIST = README yywrap.c config.l sciv
INCLUDES = -I$(top_srcdir)/src/include
AM_CFLAGS = $(SDL_CFLAGS)
LDADD = engine/libsciengine.a gfx/libscigraphics.a \
	gfx/resource/libsciresources.a gfx/drivers/libscidrivers.a \
	sfx/libscisound.a scicore/libscicore.a \
	sfx/player/libsciplayer.a \
	sfx/device/libscisounddevice.a \
	sfx/seq/libsciseq.a sfx/timer/libscitimer.a sfx/libscisoundlib.a \
	@ac_readline@ @ac_curses_libraries@ @ac_png_libraries@ -lm @SDL_LIBS@
bin_PROGRAMS = freesci
bin_SCRIPTS = sciv
freesci_SOURCES = config.l main.c
freesci_LDFLAGS = @SCIV_LDFLAGS@
