SUBDIRS = scicore engine include gfx sound tools win32 morphos wince menu
EXTRA_DIST = README yywrap.c config.l sciv dc/3dutils.c dc/Makefile \
	dc/bkg.c dc/bsearch.c dc/config.h.in dc/dc.c dc/dc.h dc/dc_save.c \
	dc/gamemenu.c dc/gp.h dc/mouse1.h dc/selectgame.c dc/selectgame.h \
	dc/sintab.h dc/snd_stream.c dc/snprintf.c dc/stream.h dc/texture.c \
	dc/options.h dc/keyboard.c dc/keyboard.h gp32/wrap.c gp32/wrap.h \
	gp32/config.h gp32/Makefile
INCLUDES = -I$(top_srcdir)/src/include
AM_CFLAGS = $(SDL_CFLAGS)
LDADD = engine/libsciengine.a gfx/libscigraphics.a		   \
	gfx/resource/libsciresources.a gfx/drivers/libscidrivers.a \
	sound/libscisound.a scicore/libscicore.a menu/libscimenu.a \
	@MORPHOS_LIBS@ @ac_readline@ @ac_curses_libraries@         \
	@ac_png_libraries@ @ac_pthread_libraries@ -lm @SDL_LIBS@
bin_PROGRAMS = freesci
bin_SCRIPTS = sciv
freesci_SOURCES = config.l main.c
freesci_LDFLAGS = @SCIV_LDFLAGS@
