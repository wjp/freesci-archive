INCLUDES = -I$(top_srcdir)/src/include @ac_glib_includes@ @ac_ggi_includes@ @ac_png_includes@
LDADD = @ac_glib_libraries@
EXTRA_DIST = cfsml.pl savegame.cfsml
lib_LIBRARIES = libscicore.a
libscicore_a_SOURCES = said.y game.c state.c console.c old_objects.c savegame.c vocab.c resource.c decompress0.c decompress1.c vocab_debug.c script.c kernel.c kscripts.c klists.c kfile.c kgraphics.c kmath.c kevent.c kstring.c kmenu.c ksound.c vm.c tools.c scriptdebug.c heap.c
check_PROGRAMS = resourcecheck
resourcecheck_SOURCES = resource.c decompress0.c decompress1.c resourcecheck.c
