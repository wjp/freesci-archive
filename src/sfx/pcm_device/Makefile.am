noinst_LIBRARIES = libscipcm.a
INCLUDES = -I$(top_srcdir)/src/include
AM_CFLAGS = $(SDL_CFLAGS)
EXTRA_DIST = 
libscipcm_a_SOURCES = audiobuf.c pcm_devices.c sdl.c alsa.c
audbuf_test_LDADD = libscipcm.a ../../scicore/libscicore.a
check_PROGRAMS = audbuf_test
