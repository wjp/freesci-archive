SUBDIRS = scicore engine include gfx sound tools win32
EXTRA_DIST = README yywrap.c config.l python.tar.gz
INCLUDES = -I$(top_srcdir)/src/include @ac_ggi_includes@ @ac_png_includes@
LDADD = engine/libsciengine.a scicore/libscicore.a gfx/libscigraphics.a gfx/resource/libsciresources.a gfx/drivers/libscidrivers.a sound/libscisound.a @ac_glx_libraries@ @ac_ggi_libraries@ @ac_readline@ @ac_curses_libraries@ @ac_png_libraries@ -lm
bin_PROGRAMS = sciv
sciv_SOURCES = config.l main.c
