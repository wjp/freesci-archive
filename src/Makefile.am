SUBDIRS = scicore engine include gfx sound tools win32
EXTRA_DIST = README yywrap.c config.l python.tar.gz
INCLUDES = -I$(top_srcdir)/src/include
LDFLAGS = -rdynamic
LDADD = engine/libsciengine.a gfx/libscigraphics.a \
	gfx/resource/libsciresources.a gfx/drivers/libscidrivers.a \
	scicore/libscicore.a sound/libscisound.a \
	@ac_readline@ @ac_curses_libraries@ @ac_png_libraries@ -lm \
	@SDL_LIBS@
bin_PROGRAMS = sciv
sciv_SOURCES = config.l main.c
