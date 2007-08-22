AC_PREREQ(2.52)
AC_INIT
AC_CONFIG_SRCDIR([acinclude.m4])

AC_CANONICAL_TARGET([])
AM_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE(freesci, 0.3.6-devel)

AC_PROG_CC
#AC_LIBTOOL_DLOPEN
AC_PROG_RANLIB
#AM_PROG_LIBTOOL
#AC_PROG_LIBTOOL
AC_CHECK_HEADERS(limits.h)
AC_CHECK_HEADERS(sys/uio.h)
AC_CHECK_HEADERS(sys/soundcard.h)
AC_CHECK_HEADERS(sys/time.h)
AC_CHECK_HEADERS(obstack.h)
AC_CHECK_HEADERS(dirent.h)
AC_CHECK_HEADERS(unistd.h)
AC_CHECK_HEADERS(fnmatch.h)
AC_CHECK_HEADERS(getopt.h setjmp.h)
AC_CHECK_HEADERS(fcntl.h)
AC_C_INLINE

if test "$cross_compiling" != "yes"; then
	AC_CHECK_SIZEOF(short)
	AC_CHECK_SIZEOF(int)
	AC_CHECK_SIZEOF(long)
else
	case "$target_cpu" in
	alpha*)
		AC_DEFINE(SIZEOF_SHORT, 2)
		AC_DEFINE(SIZEOF_INT, 4)
		AC_DEFINE(SIZEOF_LONG, 8)
	;;


	i*86*|powerpc|arm)
		AC_DEFINE(SIZEOF_SHORT, 2)
		AC_DEFINE(SIZEOF_INT, 4)
		AC_DEFINE(SIZEOF_LONG, 4)
	;;

	m68k*)
		AC_DEFINE(SIZEOF_SHORT, 2)
		AC_DEFINE(SIZEOF_LONG, 4)
	;;


	*)
		echo "Your CPU type is $target_cpu."
		AC_MSG_ERROR([
			" There are no reliable default sizes for short,"
			" int and long on your target processor known to us,"
			" so we we cannot support cross-compiling for it."
			" Please contact the FreeSCI development mailing list,"
			" so that we can work this out..."])
	;;

	esac
fi

case "${target_os}" in
cygwin*)
	AC_CHECK_LIB(cygwin, usleep)
	LIBS="-lwinmm $LIBS"
	;;

darwin*)
	AC_DEFINE([MACOSX], [1], [Define to 1 if you use Mac OS X.])
	CFLAGS="-no-cpp-precomp $CFLAGS"
	# Disable Apple's broken preprocessor
	;;

esac

AC_CHECK_FUNCS(memchr fnmatch pipe socketpair fork sched_yield memf	rob ffs getopt_long isblank usleep)
AM_PATH_ALSA(0.5.0, [AC_DEFINE([HAVE_ALSA], [1], [Define to 1 if you have ALSA.])], [echo >/dev/null])
AC_CHECK_HEADERS(sys/asoundlib.h alsa/asoundlib.h)
AM_WITHOUT_SOUND
AC_PATH_GGI
AC_PATH_PTHREAD
AC_PATH_DIRECTFB
#AC_CHECK_FSCI_DLOPEN
SCIV_LDFLAGS=""
AC_SUBST(SCIV_LDFLAGS)

AC_CHECK_HEADERS(dmedia/midi.h)

if test "${ac_cv_header_dmedia_midi_h}" = "yes"; then
	case ${host_os} in
	irix*)
		found_sound=yes
		LIBS="-lmd $LIBS"
	esac
fi

AC_CHECK_HEADERS(dmedia/audio.h)

if test "${ac_cv_header_dmedia_audio_h}" = "yes"; then
	case ${host_os} in
	irix*)
		found_sound=yes
		LIBS="-laudio -lpthread $LIBS"
	esac
fi

AC_CHECK_HEADERS(fluidsynth.h)

if test "${ac_cv_header_fluidsynth_h}" = "yes"; then
	LIBS="-lfluidsynth $LIBS"
fi

AC_ARG_WITH(sdl, [  --without-sdl           Do not build the SDL driver])
if test x"$with_sdl" != xno; then
	AM_PATH_SDL(1.1.8, 	[
					AC_DEFINE([HAVE_SDL], [1], [Define to 1 if you have the `SDL' library (-lSDL).])
					fsci_sdl_driver="yes"
				],
				[
					fsci_sdl_driver=""
				])
else
	AC_MSG_CHECKING([for SDL])
	AC_MSG_RESULT([disabled]) # Well, we should give some kind of feedback...
	fsci_sdl_driver=""
fi

AC_PATH_CURSES

if test x"$ac_curses_libraries" != x; then
	AC_CHECK_HEADERS(readline/readline.h, [	ac_readline="-lreadline" ], [ ac_readline="" ])
	AC_SUBST(ac_readline)
	AC_CHECK_HEADERS(readline/history.h)
fi

#--- beginning
AC_PATH_XTRA

if test x"$have_x" = xyes; then
	AC_CHECK_XSHM
	# in Debian, the freetype headers are in a strange place
	if test -d "/usr/include/freetype2"; then
	        CPPFLAGS="$CPPFLAGS -I/usr/include/freetype2"
	        CPFLAGS="$CPFLAGS -I/usr/include/freetype2"
	fi
	AC_CHECK_HEADERS([X11/Xft/Xft.h])
	AC_CHECK_LIB(Xft, XftDisplayInfoGet, [X_LIBS="$XLIBS -lXft"])
	AC_CHECK_HEADERS(X11/extensions/Xrender.h, [X_LIBS="$XLIBS -lXrender"])
	AC_CHECK_LIB(fontconfig, FcInit, [X_LIBS="$X_LIBS -lfontconfig"])
fi

AC_MSG_CHECKING(for Alpha hardware and gcc or ccc style __asm__)
AC_TRY_COMPILE([], [
#ifdef __DECC
#  define my_asm(x) asm (x)
#else
#  define my_asm(x) __asm__ (x)
#endif
#ifndef __alpha__
#error "Not on Alpha"
#endif

	my_asm("zapnot \$31, \$31, \$31");
	return 0;
], [
	AC_MSG_RESULT(found.)
	ev6_cpu="-mcpu=ev6"
	AC_DEFINE([HAVE_ALPHA_EV6_SUPPORT], [1], [Define to 1 if you have Alpha EV6 or PCA56 hardware.])
],
[
	AC_MSG_RESULT(not present.);
	ev6_cpu=""
])
#--- end


AC_SUBST(ev6_cpu)


fsci_xlib_driver=""
if test x"$have_x" = xyes; then
	if test x"$x_libraries" != xNONE; then
		if test x"$x_libraries" != x; then
			LIBS="-L$x_libraries $LIBS"
		fi
		AC_CHECK_LIB(Xi, XGrabDevice, [X_LIBS="$X_LIBS -lXi"])
		AC_CHECK_LIB(X11, XOpenDisplay, [X_LIBS="$X_LIBS -lX11"])
		AC_CHECK_LIB(Xext, XShmAttach, [X_LIBS="$X_LIBS -lXext"])

		fsci_xlib_driver="yes"
	fi
fi

dnl if test "no_x" != yes; then
dnl 	AC_PATH_GLX
dnl fi
AC_SUBST(ac_glx_libraries)


fsci_xlib_dl=""
fsci_sdl_dl=""
fsci_ggi_dl=""
fsci_xlib_st=""
fsci_sdl_st=""
fsci_ggi_st=""

if test x"$dlopen" = xyes; then
	if test x"$fsci_ggi_driver" = xyes; then
		fsci_ggi_dl="ggi_driver.la"
	fi

	if test x"$fsci_sdl_driver" = xyes; then
		fsci_sdl_dl="sdl_driver.la"
	fi

	if test x"$fsci_xlib_driver" = xyes; then
		fsci_xlib_dl="xlib_driver.la"
	fi
else
	if test x"$fsci_ggi_driver" = xyes; then
		fsci_ggi_st="ggi_driver.o"
		LIBS="$LIBS $ac_ggi_libraries"
	fi

	if test x"$fsci_directfb_driver" = xyes; then
		fsci_ggi_st="directfb_driver.o"
		LIBS="$LIBS $ac_directfb_libraries"
	fi

	if test x"$fsci_sdl_driver" = xyes; then
		fsci_sdl_st="sdl_driver.o"
		LIBS="$LIBS $SDL_LIBS"
	fi

	if test x"$fsci_xlib_driver" = xyes; then
		fsci_xlib_st="xlib_driver.o"
		LIBS="$LIBS $X_LIBS"
	fi
fi

AC_SUBST(fsci_xlib_dl)
AC_SUBST(fsci_sdl_dl)
AC_SUBST(fsci_ggi_dl)
AC_SUBST(fsci_xlib_st)
AC_SUBST(fsci_sdl_st)
AC_SUBST(fsci_ggi_st)

dnl AC_SUBST(ac_xlib_o)
dnl AC_SUBST(ac_sdl_o)
dnl AC_SUBST(ac_ggi_o)

AC_CHECK_LIB(m, cos, [LIB_M="-lm"], [LIB_M=""])
dnl AC_CHECK_LIB(c, sprintf, [LIB_C="-lc"], [LIB_C=""])
AC_SUBST(LIB_M)

if test x"$have_x" = xyes; then
   AC_CHECK_HEADERS([Xm/MwmUtil.h],[],[],[#include <X11/X.h>])
fi

#AC_PATH_PNG
AC_SUBST(ac_png_includes)
AC_SUBST(ac_png_libraries)
AM_PROG_LEX
AC_PROG_YACC

AC_C_PARAMETRIZED_BIGENDIAN

AC_ARG_WITH(efence,
	[  --with-efence           link against libefence (for debugging)],
	[LIBS="$LIBS -lefence"],
	[LIBS="$LIBS $LIB_C"])

AC_ARG_WITH(Wall,
	[  --with-Wall		  compile using -Wall],
	[CFLAGS="-Wall $CFLAGS"],
	[])

AC_ARG_WITH(E,
	[  --with-E		  compile using -E (output from pre-processor only)],
	[CFLAGS="-E $CFLAGS"],
	[])

AC_ARG_WITH(console,
	[  --with-console	  Build with on-screen console support],
	[AC_DEFINE(WANT_CONSOLE)],
	[])

AC_ARG_WITH(tools,
	[  --with-tools		  Build freesci development tools],
	[wanttools=yes],
	[wanttools=no])

AC_ARG_WITH(forced-console,
	[  --with-forced-console   Enforce usage of on-screen console on all gfx drivers],
	[
	 AC_DEFINE([WANT_CONSOLE], [1], [Define to 1 if you want to build with on-screen console support.])
	 AC_DEFINE([FORCE_CONSOLE], [1], [Define to 1 if you want to enforce usage of on-screen console on all gfx drivers.])],
	[])

AC_ARG_WITH(pic-scaling, [  --without-pic-scaling	  Disable support for scaled images])
if test x"$with_pic_scaling" != xno; then
	 AC_DEFINE([WITH_PIC_SCALING], [1], [Define to 1 if you want SCI0 pic resource scaling support.])
fi

CFSML_FLAGS=""

AM_WITH_DMALLOC
AM_WITH_MALLOC_DEBUG
AM_WITHOUT_MALLOC_CHECKS

AC_SUBST(CFSML_FLAGS)

RPM_VERSION=`echo $VERSION | sed 's/\-/_/'`
AC_SUBST(RPM_VERSION)

SDL_CFLAGS="$CFLAGS $SDL_CFLAGS"

case $target in
        *morphos*)
		MORPHOS_LIBS="morphos/libmorphos.a"
		AC_SUBST(MORPHOS_LIBS)
		morphos=yes
	;;
esac

AM_CONDITIONAL(MORPHOS, test x$morphos = xyes)
AM_CONDITIONAL(WANT_TOOLS, test x$wanttools = xyes)

AC_CONFIG_FILES([Makefile    \
	debian/Makefile \
	desktop/Makefile \
	doc/Makefile \
        doc/misc/Makefile \
        doc/man/Makefile \
        conf/Makefile \
	custom/Makefile \
	src/Makefile  \
	src/scicore/Makefile \
	src/engine/Makefile \
	src/gfx/Makefile \
	src/gfx/drivers/Makefile \
	src/gfx/resource/Makefile \
	src/include/Makefile \
	src/include/win32/Makefile \
	src/include/beos/Makefile \
	src/tools/Makefile \
	src/win32/Makefile \
	src/sound/Makefile \
	src/morphos/Makefile \
	src/wince/Makefile \
	src/menu/Makefile \
	freesci.spec])

AC_OUTPUT

