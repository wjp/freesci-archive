SUBDIRS = seq timer player device mixer pcm_device
INCLUDES = -I$(top_srcdir)/src/include
noinst_LIBRARIES = libscisound.a libscisoundlib.a
libscisound_a_SOURCES = iterator.c songlib.c core.c
libscisoundlib_a_SOURCES = time.c adlib.c
timetest_LDADD = libscisound.a ../scicore/libscicore.a
check_PROGRAMS = timetest

