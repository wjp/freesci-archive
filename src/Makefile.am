SUBDIRS = scicore engine include gfx sound tools win32
EXTRA_DIST = README yywrap.c config.l python.tar.gz
INCLUDES = -I$(top_srcdir)/src/include
LDADD = engine/libsciengine.a scicore/libscicore.a gfx/libscigraphics.a gfx/resource/libsciresources.a gfx/drivers/libscidrivers.a sound/libscisound.a @X_LIBS@ @ac_ggi_libraries@ @ac_readline@ @ac_curses_libraries@ @ac_png_libraries@ -lm @SDL_LIBS@
bin_PROGRAMS = sciv
sciv_SOURCES = config.l main.c
