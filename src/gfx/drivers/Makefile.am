INCLUDES = -I$(top_srcdir)/src/include @ac_ggi_includes@ @ac_png_includes@ @X_CFLAGS@
LDADD =  $(LDADD_ALL)

gfxdriverdir = $(libdir)/freesci/gfx

EXTRA_LTLIBRARIES = xlib_driver.la ggi_driver.la sdl_driver.la
gfxdriver_LTLIBRARIES = @ac_xlib_o@ @ac_ggi_o@ @ac_sdl_o@

noinst_LIBRARIES = libscidrivers.a
libscidrivers_a_SOURCES = gfx_drivers.c


xlib_driver_la_LDFLAGS = -rpath $(gfxdriverdir) -module -no-undefined \
			 -export-symbols $(srcdir)/EXPORTS_xlib
xlib_driver_la_LIBADD = @X_LIBS@
xlib_driver_la_SOURCES = xlib_driver.c

ggi_driver_la_LDFLAGS = -rpath $(gfxdriverdir) -module -no-undefined \
			-export-symbols $(srcdir)/EXPORTS_ggi
ggi_driver_la_LIBADD = @X_LIBS@ @ac_ggi_libraries@
ggi_driver_la_SOURCES = ggi_driver.c

sdl_driver_la_LDFLAGS = -rpath $(gfxdriverdir) -module -no-undefined \
			-export-symbols $(srcdir)/EXPORTS_sdl
sdl_driver_la_LIBADD =  @SDL_LIBS@
sdl_driver_la_SOURCES = sdl_driver.c


