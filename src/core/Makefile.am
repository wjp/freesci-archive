INCLUDES = -I$(top_srcdir)/src/include @ac_glib_includes@ @ac_ggi_includes@ @ac_png_includes@
LDADD = @ac_glib_libraries@
EXTRA_DIST = cfsml.pl savegame.cfsml
lib_LIBRARIES = libscicore.a
libscicore_a_SOURCES = savegame.c vocab.c resource.c decompress0.c decompress1.c vocab_debug.c script.c kernel.c vm.c scriptdebug.c heap.c
check_PROGRAMS = resourcecheck
resourcecheck_SOURCES = resource.c decompress0.c decompress1.c resourcecheck.c
