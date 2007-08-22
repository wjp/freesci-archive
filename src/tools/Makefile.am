INCLUDES = -I$(top_srcdir)/src/include @ac_png_includes@
LDADD = ../scicore/libscicore.a
EXTRA_DIST = sciunpack.h

if WANT_TOOLS
bin_PROGRAMS = sciunpack sciconsole scidisasm scipack
sciunpack_SOURCES = sciunpack.c listwords.c scriptdump.c vocabdump.c
sciunpack_LDADD = ../sound/libscisound.a ../scicore/libscicore.a @ac_glx_libraries@ @ac_glx_libraries@ @ac_png_libraries@
scidisasm_SOURCES = scidisasm.c
sciconsole_LDADD = ../sound/libscisound.a ../scicore/libscicore.a @ac_curses_libraries@ @ac_readline@ @ac_curses_libraries@ @LIB_M@
sciconsole_SOURCES = sciconsole.c
scipack_LDADD = ../scicore/libscicore.a
scipack_SOURCES = scipack.c
endif