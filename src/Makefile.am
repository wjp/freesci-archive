SUBDIRS = core include graphics sound console tools win32
EXTRA_DIST = README yywrap.c config.l python.tar.gz
INCLUDES = -I$(top_srcdir)/src/include @ac_glib_includes@ @ac_ggi_includes@ $(GLIB_CFLAGS) @ac_png_includes@
LDADD = core/libscicore.a graphics/libscigraphics.a sound/libscisound.a console/libsciconsole.a -lfl -lm @ac_ggi_libraries@ -lreadline @ac_png_libraries@ $(GLIB_LIBS) @ac_curses_libraries@ 
bin_PROGRAMS = sciv
sciv_SOURCES = config.l main.c
