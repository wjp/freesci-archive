SUBDIRS = scicore engine include graphics sound tools win32
EXTRA_DIST = README yywrap.c config.l python.tar.gz
INCLUDES = -I$(top_srcdir)/src/include @ac_glib_includes@ @ac_ggi_includes@ $(GLIB_CFLAGS) @ac_png_includes@
LDADD = engine/libsciengine.a scicore/libscicore.a graphics/libscigraphics.a sound/libscisound.a @ac_glx_libraries@ @ac_ggi_libraries@ @ac_readline@ $(GLIB_LIBS) @ac_curses_libraries@ @ac_png_libraries@ -lm
bin_PROGRAMS = sciv
sciv_SOURCES = config.l main.c
