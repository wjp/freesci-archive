SUBDIRS = core include graphics sound console @ac_moresubdirs@ tools
EXTRA_DIST = README python.tar.gz
INCLUDES = -I$(top_srcdir)/src/include @ac_glib_includes@
LDADD = -lefence -lglib -lm -lc -lggi -lgii -lreadline -lcurses core/libscicore.a graphics/libscigraphics.a sound/libscisound.a console/libsciconsole.a
bin_PROGRAMS = sciv
sciv_SOURCES = main.c
