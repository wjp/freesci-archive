SUBDIRS = scicore engine include gfx sound tools win32
EXTRA_DIST = README yywrap.c config.l python.tar.gz sciv
INCLUDES = -I$(top_srcdir)/src/include
LDADD = engine/libsciengine.a gfx/libscigraphics.a \
	gfx/resource/libsciresources.a gfx/drivers/libscidrivers.a \
	sound/libscisound.a scicore/libscicore.a \
	@ac_readline@ @ac_curses_libraries@ @ac_png_libraries@ -lm \
	@SDL_LIBS@
bin_PROGRAMS = freesci
bin_SCRIPTS = sciv
freesci_SOURCES = config.l main.c
freesci_LDFLAGS = @SCIV_LDFLAGS@
