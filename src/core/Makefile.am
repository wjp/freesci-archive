INCLUDES = -I$(top_srcdir)/src/include @ac_glib_includes@
LDADD = @ac_glib_libraries@
lib_LIBRARIES = libscicore.a
libscicore_a_SOURCES = resource.c decompress0.c decompress1.c vocabulary.c script.c kernel.c vm.c scriptdebug.c
bin_PROGRAMS = heap
check_PROGRAMS = resourcecheck
resourcecheck_SOURCES = resource.c decompress0.c decompress1.c resourcecheck.c
heap_SOURCES = heap.c
