INCLUDES = -I$(top_srcdir)/src/include
EXTRA_DIST = cfsml.pl savegame.cfsml
lib_LIBRARIES = libsciengine.a
libsciengine_a_SOURCES = savegame.c kernel.c kscripts.c klists.c kfile.c kgraphics.c kmath.c kevent.c kstring.c kmenu.c ksound.c vm.c game.c scriptdebug.c heap.c simplesaid.c said.y grammar.c

savegame.c: savegame.cfsml
	cat savegame.cfsml | ./cfsml.pl > savegame.c
