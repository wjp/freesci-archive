INCLUDES = -I$(top_srcdir)/src/include
EXTRA_DIST = sci_dos.c treedef.1 treedef.2 treedef.3 hufftree.1 \
			 hufftree.2 hufftree.3 huffmake.pl
noinst_LIBRARIES = libscicore.a
libscicore_a_SOURCES = console.c tools.c resource.c decompress0.c     \
		 decompress01.c decompress1.c decompress11.c script.c \
		 vocab.c vocab_debug.c old_objects.c modules.c        \
		 sci_memory.c resource_map.c

check_PROGRAMS = resmap_test

resmap_test:
	$(COMPILE) -I../.. -I../include -DHAVE_CONFIG_H -DTEST_RESOURCE_MAP $(top_srcdir)/src/scicore/resource_map.c libscicore.a -o $(top_builddir)/src/scicore/resmap_test
