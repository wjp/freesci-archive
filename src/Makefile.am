SUBDIRS = core include graphics sound console tools win32
EXTRA_DIST = README yywrap.c config.l python.tar.gz
INCLUDES = -I$(top_srcdir)/src/include @ac_glib_includes@ @ac_ggi_includes@
LDADD = -lfl -lglib -lm -lc @ac_ggi_libraries@ -lreadline -lpng -lcurses core/libscicore.a graphics/libscigraphics.a sound/libscisound.a console/libsciconsole.a
bin_PROGRAMS = sciv
sciv_SOURCES = config.l main.c
