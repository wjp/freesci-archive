INCLUDES = -I$(top_srcdir)/src/include @ac_glib_includes@
LDADD = @ac_glib_libraries@
EXTRA_DIST = sci_dos.c
lib_LIBRARIES = libscicore.a
libscicore_a_SOURCES = console.c tools.c resource.c decompress0.c decompress01.c decompress1.c script.c vocab.c vocab_debug.c old_objects.c
check_PROGRAMS = resourcecheck
resourcecheck_SOURCES = resource.c decompress0.c decompress01.c decompress1.c resourcecheck.c
