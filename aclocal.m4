# aclocal.m4 generated automatically by aclocal 1.5

# Copyright 1996, 1997, 1998, 1999, 2000, 2001
# Free Software Foundation, Inc.
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY, to the extent permitted by law; without
# even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE.



AC_DEFUN(AC_FIND_FILE,
[
$3=NO
for i in $2;
do
  for j in $1;
  do
    if test -r "$i/$j"; then
      $3=$i
      break 2
    fi
  done
done
])


AC_DEFUN(AC_CHECK_FSCI_DLOPEN,
[
dlopen="no"
SCIV_LDFLAGS=""
AC_ARG_WITH(modules,
	[  --without-modules	  Statically link all modules to the executable ])

if test x"$with_modules" != xno; then
	AC_CHECK_FUNC(dlopen, [
				SCIV_LDFLAGS="-rdynamic"
				AC_DEFINE(HAVE_DLOPEN)
				dlopen="yes"
			 ])
fi
AC_SUBST(SCIV_LDFLAGS)
])

# AC_CHECK_INCLUDE_PATH(header.h, "-Ipath1 -Ipath2 -Ipath3", "#include <foo.h>"
#                        result_var)
# Tries to preprocess a file including header.h (and <foo.h>) while trying out "",
# "-Ipath1", "-Ipath2" etc. as include paths. Writes the result to result_var
# (any element of {"no", "-Ipath1", "-Ipath2", "-Ipath3"}
AC_DEFUN(AC_CHECK_INCLUDE_PATH,
[
OLDCPPFLAGS="$CPPFLAGS"
$4=no
wasfound=no

for i in "" $2;
do
	CPPFLAGS="$OLDCPPFLAGS $i"

	AC_TRY_CPP([$3
#include <$1>], [wasfound=$i])

	if test "$wasfound" != no; then

		$4=$wasfound
		CPPFLAGS=$OLDCPPFLAGS
		break
	fi

done

CPPFLAGS=$OLDCPPFLAGS
])



# AC_CHECK_LINK_PATH("x_fork();", "-L/usr/lib -L/usr/foo/lib", "-lxyzzy -lgnuxyzzy",
#                    "-lfoo", "#include <xfork.h>", result_var)
# Checks for the function x_fork in -lxyzzy and -lgnuxyzzy, both of which are looked
# for in the default link path, then /usr/lib, and /usr/foo/lib. To compile, xfork.h is
# included, and libfoo is linked appropriately. The resulting valid -L/-l combination
# would be stored in result_var. If none is found, result_var is set to "no".
AC_DEFUN(AC_CHECK_LINK_PATH,
[
OLDCFLAGS=$CFLAGS
$6=no
wasfound=no

for i in "" $2;
do
	for j in "" $3;
	do
		CFLAGS="$OLDCFLAGS $i $j $4"
		AC_TRY_LINK([$5], [$1], [wasfound="$i $j"])

		if test "$wasfound" != no; then

			CFLAGS=$OLDCFLAGS
			$6=$wasfound
			break 2

		fi
	done
done

CFLAGS=$OLDCFLAGS
])




AC_DEFUN(AC_PATH_GGI,
[
AC_MSG_CHECKING([for ggi])

ac_ggi_so=""

_ac_ggi_includes="-I/usr/include -I/usr/local/include"
_ac_ggi_libraries="-L/usr/lib -L/usr/local/lib -L/usr/local/ggi/lib"


AC_ARG_WITH(ggi,
    [  --without-ggi           Don't build the ggi driver])

AC_ARG_WITH(ggi-dir,
    [  --with-ggi-dir          where the root of ggi is installed ],
    [  _ac_ggi_includes="-I$withval"/include
       _ac_ggi_libraries="-L$withval"/lib
    ])

AC_ARG_WITH(ggi-includes,
    [  --with-ggi-includes     where the ggi includes are. ],
    [  
       _ac_ggi_includes="-I$withval"
    ])
AC_ARG_WITH(ggi-libraries,
    [  --with-ggi-libraries    where the ggi library is installed.],
    [  _ac_ggi_libraries="-L$withval"
    ])

if test x"$with_ggi" = xno; then
	AC_MSG_RESULT([disabled]);
	ac_ggi_libraries=""
	ac_ggi_includes=""
else

	AC_CHECK_INCLUDE_PATH([ggi/ggi.h],[$_ac_ggi_includes],[#include <ggi/gii.h>], ac_ggi_includes)
	AC_CHECK_LINK_PATH([ggiInit();],$_ac_ggi_libraries,["-lggi"],
			 [$ac_ggi_includes -lgii -lgg],[#include <ggi/ggi.h>
#include <ggi/gii.h>], ac_ggi_libraries)

	if test "$ac_ggi_includes" = no || test "$ac_ggi_libraries" = no; then

		AC_MSG_RESULT([failed])
		ac_ggi_libraries=""
		ac_ggi_includes=""
	else
		AC_MSG_RESULT([found]);

		ac_ggi_libraries="$ac_ggi_libraries -lgii -lgg"
		ac_graphics_ggi_libfile="graphics_ggi.c"
		AC_SUBST(ac_graphics_ggi_libfile)
		ac_graphics_ggi_libobjects="graphics_ggi.o"
		AC_SUBST(ac_graphics_ggi_libobjects)
		AC_DEFINE(HAVE_LIBGGI)
		fsci_ggi_driver="yes"
	fi
fi

AC_SUBST(ac_ggi_includes)
AC_SUBST(ac_ggi_libraries)
])


AC_DEFUN(AC_CHECK_XSHM,
[
AC_MSG_CHECKING([for the X11 MIT-SHM extension])

AC_LANG_SAVE
AC_LANG_C
oldLIBS="$X_LIBS"
X_LIBS="$X_LIBS -lXext"
AC_TRY_COMPILE([
#include <X11/Xlib.h>
#include <X11/extensions/XShm.h>
], [
	Display *display;
	XShmSegmentInfo foo;

	XShmAttach(display, &foo);
],[
	AC_MSG_RESULT(found.)
	AC_DEFINE(HAVE_MITSHM)
],[
	AC_MSG_RESULT(not present.)
	X_LIBS="$oldLIBS"
])
AC_LANG_RESTORE
])

AC_DEFUN(AC_PATH_CURSES,
[
AC_MSG_CHECKING([for (n)curses])

_ac_curses_includes="-I/usr/include -I/usr/local/include"
_ac_curses_libraries="-L/usr/lib -L/usr/local/lib"
_ac_curses_libnames="-lncurses -lcurses"


AC_ARG_WITH(curses-dir,
    [  --with-curses-dir       where the root of lib(n)curses is installed ],
    [  _ac_curses_includes="-I$withval"/include
       _ac_curses_libraries="-L$withval"/lib
    ])

AC_ARG_WITH(curses-includes,
    [  --with-curses-includes  where the (n)curses includes are. ],
    [  
       _ac_curses_includes="-I$withval"
    ])
AC_ARG_WITH(curses-library,
    [  --with-curses-libraries where the (n)curses library is installed.],
    [  _ac_curses_libraries="-L$withval"
    ])
AC_ARG_WITH(curses-libname,
    [  --with-curses-libraries whether to use curses or ncurses.],
    [  _ac_curses_libnames="-l$withval"
    ])

CURSHEADER=ncurses.h
AC_CHECK_INCLUDE_PATH([ncurses.h],[$_ac_curses_includes],[], ac_curses_includes)

if test x"$ac_curses_includes" = x; then
	CURSHEADER=curses.h
	AC_CHECK_INCLUDE_PATH([curses.h],[$_ac_curses_includes],[], ac_curses_includes)
fi

AC_CHECK_LINK_PATH([noecho();],$_ac_curses_libraries,$_ac_curses_libnames,
		 [$ac_curses_includes],[#include <$CURSHEADER>], ac_curses_libraries)

if test "$ac_curses_includes" = no || test "$ac_curses_libraries" = no; then

	AC_MSG_RESULT([failed])
	ac_curses_libraries=""
	ac_curses_includes=""

else

	AC_MSG_RESULT([found $ac_curses_libraries]);
	AC_DEFINE(HAVE_CURSES)

fi

AC_SUBST(ac_curses_includes)
AC_SUBST(ac_curses_libraries)
])

AC_DEFUN(AC_PATH_PNG,
[
AC_MSG_CHECKING([for libpng])

_ac_png_includes="-I/usr/include -I/usr/local/include"
_ac_png_libraries="-L/usr/lib -L/usr/local/lib"


AC_ARG_WITH(png-dir,
    [  --with-png-dir          where the root of png is installed ],
    [  _ac_png_includes="-I$withval"/include
       _ac_png_libraries="-L$withval"/lib
    ])

AC_ARG_WITH(png-includes,
    [  --with-png-includes     where the png includes are. ],
    [  
       _ac_png_includes="-I$withval"
    ])
AC_ARG_WITH(png-libraries,
    [  --with-png-libraries    where the png library is installed.],
    [  _ac_png_libraries="-L$withval"
    ])

OLDLIBS="$LIBS"
LIBS="$LIBS -lpng"
AC_CHECK_INCLUDE_PATH([png.h],[$_ac_png_includes],[], ac_png_includes)
AC_CHECK_LINK_PATH([png_info_init(0);],$_ac_png_libraries,["-lpng"],
		 [$ac_png_includes],[#include <png.h>], ac_png_libraries)


if test "$ac_png_libraries" = no; then
# Try again with -lz
	LIBS="$LIBS -lz"
	AC_CHECK_LINK_PATH([png_read_end(NULL, NULL);],$_ac_png_libraries,["-lpng"],
			 [$ac_png_includes],[#include <png.h>], ac_png_libraries)
fi

if test "$ac_png_includes" = no || test "$ac_png_libraries" = no; then

	AC_MSG_RESULT([failed])
	LIBS="$OLDLIBS"
	ac_png_libraries=""
	ac_png_includes=""
else
	AC_MSG_RESULT([found]);
	AC_DEFINE(HAVE_LIBPNG)
fi
AC_SUBST(ac_png_includes)
AC_SUBST(ac_png_libraries)
])



# Checks for endianness, unless endianness is specified by a parameter
AC_DEFUN(AC_C_PARAMETRIZED_BIGENDIAN,
[
force_endian=no

AC_ARG_WITH(endianness,
	[  --with-endianness=ARG   whether big or little endianness should be used.],
	[
		force_endian="$withval"
	])


if test "$force_endian" = no; then

	AC_C_BIGENDIAN
else


	if test "$force_endian" = big; then

		AC_DEFINE(WORDS_BIGENDIAN)
	else

		if test "$force_endian" != little; then

			AC_MSG_ERROR(["Endianness may only be 'big' or 'little', not $force_endian."])

		fi 
	fi
fi
])




AC_DEFUN(AC_PATH_GLX,
[
AC_MSG_CHECKING([for glx])

oldCFLAGS=$CFLAGS
oldLIBS=$LIBS

if test "$x_libraries"; then
	ac_glx_libraries="$LDFLAGS -L$x_libraries"
	CFLAGS="$CFLAGS -I$x_includes"
fi

found_glx=no
AC_TRY_CPP([#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include <GL/gl.h>
#include <GL/glx.h>], [found_glx=yes])


if test "$found_glx" = yes; then

	found_glx=no

	ac_glx_libraries="$ac_glx_libraries -lGL -lX11 -lXmu -lXi"
	LIBS="$oldLIBS $ac_glx_libraries"
	AC_TRY_LINK([#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include <GL/gl.h>
#include <GL/glx.h>], [glBegin(GL_TRIANGLE_STRIP);], [found_glx=yes])

	if test "$found_glx" = yes; then

		AC_MSG_RESULT([yes])
		AC_DEFINE(HAVE_GLX)

	else

		AC_MSG_RESULT([no])
		ac_glx_libraries=""
		CFLAGS=$oldCFLAGS

	fi
else

	AC_MSG_RESULT([no])
	ac_glx_libraries=""
	CFLAGS=$oldCFLAGS

fi

LIBS="$oldLIBS"

])





dnl Configure Paths for Alsa
dnl Some modifications by Richard Boulton <richard-alsa@tartarus.org>
dnl Christopher Lansdown <lansdoct@cs.alfred.edu>
dnl Jaroslav Kysela <perex@suse.cz>
dnl Last modification: 07/01/2001
dnl AM_PATH_ALSA([MINIMUM-VERSION [, ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]]])
dnl Test for libasound, and define ALSA_CFLAGS and ALSA_LIBS as appropriate.
dnl enables arguments --with-alsa-prefix=
dnl                   --with-alsa-enc-prefix=
dnl                   --disable-alsatest  (this has no effect, as yet)
dnl
dnl For backwards compatibility, if ACTION_IF_NOT_FOUND is not specified,
dnl and the alsa libraries are not found, a fatal AC_MSG_ERROR() will result.
dnl
AC_DEFUN(AM_PATH_ALSA,
[dnl Save the original CFLAGS, LDFLAGS, and LIBS
alsa_save_CFLAGS="$CFLAGS"
alsa_save_LDFLAGS="$LDFLAGS"
alsa_save_LIBS="$LIBS"
alsa_found=yes

dnl
dnl Get the cflags and libraries for alsa
dnl
AC_ARG_WITH(alsa-prefix,
[  --with-alsa-prefix=PFX  Prefix where Alsa library is installed(optional)],
[alsa_prefix="$withval"], [alsa_prefix=""])

AC_ARG_WITH(alsa-inc-prefix,
[  --with-alsa-inc-prefix=PFX  Prefix where include libraries are (optional)],
[alsa_inc_prefix="$withval"], [alsa_inc_prefix=""])

dnl FIXME: this is not yet implemented
AC_ARG_ENABLE(alsatest,
[  --disable-alsatest      Do not try to compile and run a test Alsa program],
[enable_alsatest=no],
[enable_alsatest=yes])

dnl Add any special include directories
AC_MSG_CHECKING(for ALSA CFLAGS)
if test "$alsa_inc_prefix" != "" ; then
        ALSA_CFLAGS="$ALSA_CFLAGS -I$alsa_inc_prefix"
        CFLAGS="$CFLAGS -I$alsa_inc_prefix"
fi
AC_MSG_RESULT($ALSA_CFLAGS)

dnl add any special lib dirs
AC_MSG_CHECKING(for ALSA LDFLAGS)
if test "$alsa_prefix" != "" ; then
        ALSA_LIBS="$ALSA_LIBS -L$alsa_prefix"
        LDFLAGS="$LDFLAGS $ALSA_LIBS"
fi

dnl add the alsa library
ALSA_LIBS="$ALSA_LIBS -lasound -lm -ldl"
LIBS="$ALSA_LIBS $LIBS"
AC_MSG_RESULT($ALSA_LIBS)

dnl Check for a working version of libasound that is of the right version.
min_alsa_version=ifelse([$1], ,0.1.1,$1)
AC_MSG_CHECKING(for libasound headers version >= $min_alsa_version)
no_alsa=""
    alsa_min_major_version=`echo $min_alsa_version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    alsa_min_minor_version=`echo $min_alsa_version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    alsa_min_micro_version=`echo $min_alsa_version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`

AC_LANG_SAVE
AC_LANG_C
AC_TRY_COMPILE([
#include <sys/asoundlib.h>
], [
/* ensure backward compatibility */
#if !defined(SND_LIB_MAJOR) && defined(SOUNDLIB_VERSION_MAJOR)
#define SND_LIB_MAJOR SOUNDLIB_VERSION_MAJOR
#endif
#if !defined(SND_LIB_MINOR) && defined(SOUNDLIB_VERSION_MINOR)
#define SND_LIB_MINOR SOUNDLIB_VERSION_MINOR
#endif
#if !defined(SND_LIB_SUBMINOR) && defined(SOUNDLIB_VERSION_SUBMINOR)
#define SND_LIB_SUBMINOR SOUNDLIB_VERSION_SUBMINOR
#endif

#  if(SND_LIB_MAJOR > $alsa_min_major_version)
  exit(0);
#  else
#    if(SND_LIB_MAJOR < $alsa_min_major_version)
#       error not present
#    endif

#   if(SND_LIB_MINOR > $alsa_min_minor_version)
  exit(0);
#   else
#     if(SND_LIB_MINOR < $alsa_min_minor_version)
#          error not present
#      endif

#      if(SND_LIB_SUBMINOR < $alsa_min_micro_version)
#        error not present
#      endif
#    endif
#  endif
exit(0);
],
  [AC_MSG_RESULT(found.)],
  [AC_MSG_RESULT(not present.)
   ifelse([$3], , [AC_MSG_ERROR(Sufficiently new version of libasound not found.)])
   alsa_found=no]
)
AC_LANG_RESTORE

dnl Now that we know that we have the right version, let's see if we have the library and not just the headers.
AC_CHECK_LIB([asound], [snd_defaults_card],,
        [ifelse([$3], , [AC_MSG_ERROR(No linkable libasound was found.)])
         alsa_found=no]
)

if test "x$alsa_found" = "xyes" ; then
   ifelse([$2], , :, [$2])
fi
if test "x$alsa_found" = "xno" ; then
   ifelse([$3], , :, [$3])
   CFLAGS="$alsa_save_CFLAGS"
   LDFLAGS="$alsa_save_LDFLAGS"
   LIBS="$alsa_save_LIBS"
   ALSA_CFLAGS=""
   ALSA_LIBS=""
fi

dnl That should be it.  Now just export out symbols:
AC_SUBST(ALSA_CFLAGS)
AC_SUBST(ALSA_LIBS)
])

# Configure paths for SDL
# Sam Lantinga 9/21/99
# stolen from Manish Singh
# stolen back from Frank Belew
# stolen from Manish Singh
# Shamelessly stolen from Owen Taylor

dnl AM_PATH_SDL([MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]]])
dnl Test for SDL, and define SDL_CFLAGS and SDL_LIBS
dnl
AC_DEFUN(AM_PATH_SDL,
[dnl 
dnl Get the cflags and libraries from the sdl-config script
dnl
AC_ARG_WITH(sdl-prefix,[  --with-sdl-prefix=PFX   Prefix where SDL is installed (optional)],
            sdl_prefix="$withval", sdl_prefix="")
AC_ARG_WITH(sdl-exec-prefix,[  --with-sdl-exec-prefix=PFX Exec prefix where SDL is installed (optional)],
            sdl_exec_prefix="$withval", sdl_exec_prefix="")
AC_ARG_ENABLE(sdltest, [  --disable-sdltest       Do not try to compile and run a test SDL program],
		    , enable_sdltest=yes)

  if test x$sdl_exec_prefix != x ; then
     sdl_args="$sdl_args --exec-prefix=$sdl_exec_prefix"
     if test x${SDL_CONFIG+set} != xset ; then
        SDL_CONFIG=$sdl_exec_prefix/bin/sdl-config
     fi
  fi
  if test x$sdl_prefix != x ; then
     sdl_args="$sdl_args --prefix=$sdl_prefix"
     if test x${SDL_CONFIG+set} != xset ; then
        SDL_CONFIG=$sdl_prefix/bin/sdl-config
     fi
  fi

  AC_PATH_PROG(SDL_CONFIG, sdl-config, no)
  min_sdl_version=ifelse([$1], ,0.11.0,$1)
  AC_MSG_CHECKING(for SDL - version >= $min_sdl_version)
  no_sdl=""
  if test "$SDL_CONFIG" = "no" ; then
    no_sdl=yes
  else
    SDL_CFLAGS=`$SDL_CONFIG $sdlconf_args --cflags`
    SDL_LIBS=`$SDL_CONFIG $sdlconf_args --libs`

    sdl_major_version=`$SDL_CONFIG $sdl_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    sdl_minor_version=`$SDL_CONFIG $sdl_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    sdl_micro_version=`$SDL_CONFIG $sdl_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`
    if test "x$enable_sdltest" = "xyes" ; then
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LIBS="$LIBS"
      CFLAGS="$CFLAGS $SDL_CFLAGS"
      LIBS="$LIBS $SDL_LIBS"

dnl
dnl Now check if the installed SDL is sufficiently new. (Also sanity
dnl checks the results of sdl-config to some extent
dnl
      rm -f conf.sdltest
      AC_TRY_RUN([
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SDL.h"

char*
my_strdup (char *str)
{
  char *new_str;
  
  if (str)
    {
      new_str = (char *)malloc ((strlen (str) + 1) * sizeof(char));
      strcpy (new_str, str);
    }
  else
    new_str = NULL;
  
  return new_str;
}

int main (int argc, char *argv[])
{
  int major, minor, micro;
  char *tmp_version;

  /* This hangs on some systems (?)
  system ("touch conf.sdltest");
  */
  { FILE *fp = fopen("conf.sdltest", "a"); if ( fp ) fclose(fp); }

  /* HP/UX 9 (%@#!) writes to sscanf strings */
  tmp_version = my_strdup("$min_sdl_version");
  if (sscanf(tmp_version, "%d.%d.%d", &major, &minor, &micro) != 3) {
     printf("%s, bad version string\n", "$min_sdl_version");
     exit(1);
   }

   if (($sdl_major_version > major) ||
      (($sdl_major_version == major) && ($sdl_minor_version > minor)) ||
      (($sdl_major_version == major) && ($sdl_minor_version == minor) && ($sdl_micro_version >= micro)))
    {
      return 0;
    }
  else
    {
      printf("\n*** 'sdl-config --version' returned %d.%d.%d, but the minimum version\n", $sdl_major_version, $sdl_minor_version, $sdl_micro_version);
      printf("*** of SDL required is %d.%d.%d. If sdl-config is correct, then it is\n", major, minor, micro);
      printf("*** best to upgrade to the required version.\n");
      printf("*** If sdl-config was wrong, set the environment variable SDL_CONFIG\n");
      printf("*** to point to the correct copy of sdl-config, and remove the file\n");
      printf("*** config.cache before re-running configure\n");
      return 1;
    }
}

],, no_sdl=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
     fi
  fi
  if test "x$no_sdl" = x ; then
     AC_MSG_RESULT(yes)
     ifelse([$2], , :, [$2])     
  else
     AC_MSG_RESULT(no)
     if test "$SDL_CONFIG" = "no" ; then
       echo "*** The sdl-config script installed by SDL could not be found"
       echo "*** If SDL was installed in PREFIX, make sure PREFIX/bin is in"
       echo "*** your path, or set the SDL_CONFIG environment variable to the"
       echo "*** full path to sdl-config."
     else
       if test -f conf.sdltest ; then
        :
       else
          echo "*** Could not run SDL test program, checking why..."
          CFLAGS="$CFLAGS $SDL_CFLAGS"
          LIBS="$LIBS $SDL_LIBS"
          AC_TRY_LINK([
#include <stdio.h>
#include "SDL.h"
],      [ return 0; ],
        [ echo "*** The test program compiled, but did not run. This usually means"
          echo "*** that the run-time linker is not finding SDL or finding the wrong"
          echo "*** version of SDL. If it is not finding SDL, you'll need to set your"
          echo "*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point"
          echo "*** to the installed location  Also, make sure you have run ldconfig if that"
          echo "*** is required on your system"
	  echo "***"
          echo "*** If you have an old version installed, it is best to remove it, although"
          echo "*** you may also be able to get things to work by modifying LD_LIBRARY_PATH"],
        [ echo "*** The test program failed to compile or link. See the file config.log for the"
          echo "*** exact error that occured. This usually means SDL was incorrectly installed"
          echo "*** or that you have moved SDL since it was installed. In the latter case, you"
          echo "*** may want to edit the sdl-config script: $SDL_CONFIG" ])
          CFLAGS="$ac_save_CFLAGS"
          LIBS="$ac_save_LIBS"
       fi
     fi
     SDL_CFLAGS=""
     SDL_LIBS=""
     ifelse([$3], , :, [$3])
  fi
  AC_SUBST(SDL_CFLAGS)
  AC_SUBST(SDL_LIBS)
  rm -f conf.sdltest
])


# serial 1

AC_DEFUN(AM_WITH_DMALLOC,
[AC_MSG_CHECKING(if malloc debugging is wanted)
AC_ARG_WITH(dmalloc,
[  --with-dmalloc          use dmalloc, available at http://dmalloc.com],
[if test "$withval" = yes; then
  AC_MSG_RESULT(yes)
  AC_DEFINE(WITH_DMALLOC,1,
            [Define if using the dmalloc debugging malloc package])
  LIBS="$LIBS -ldmallocth"
  CFSML_FLAGS="-l"
  LDFLAGS="$LDFLAGS -g"
else
  AC_MSG_RESULT(no)
fi], [AC_MSG_RESULT(no)])
])


AC_DEFUN(AM_WITH_MALLOC_DEBUG,
[AC_MSG_CHECKING(whether to output debug info at every memory (de)allocation)
AC_ARG_WITH(malloc-debug,
[  --with-malloc-debug     output debug information at every memory
                          (de)allocation.],
[if test x"$with_malloc_debug" = xyes; then
  AC_MSG_RESULT(yes)
  AC_DEFINE(MALLOC_DEBUG,,
            [Define if want to output debug info for every memory allocation])
else
  AC_MSG_RESULT(no)
fi], [AC_MSG_RESULT(no)])
])


AC_DEFUN(AM_WITHOUT_MALLOC_CHECKS,
[AC_MSG_CHECKING(whether to check memory allocations for NULL)
AC_ARG_WITH(malloc-checks,
[  --without-malloc-checks do not check memory allocations for NULL.],
[if test x"$with_malloc_checks" = xno; then
  AC_MSG_RESULT(no);
  AC_DEFINE(UNCHECKED_MALLOCS,,
            [Defined if not checking memory allocations])
else
  AC_MSG_RESULT(yes)
fi], [AC_MSG_RESULT(yes)])
])


AC_DEFUN(AM_WITHOUT_SOUND,
[AC_MSG_CHECKING(whether to compile with sound)
AC_ARG_WITH(sound,
[  --without-sound         Do not compile with sound.],
[if test x"$with_sound" = xno; then
  AC_MSG_RESULT(no);
  AC_DEFINE(NO_SOUND,,
            [Defined if not compiling a sound server])
else
  AC_MSG_RESULT(yes)
fi], [AC_MSG_RESULT(yes)])
])

# Like AC_CONFIG_HEADER, but automatically create stamp file.

# serial 3

# When config.status generates a header, we must update the stamp-h file.
# This file resides in the same directory as the config header
# that is generated.  We must strip everything past the first ":",
# and everything past the last "/".

AC_PREREQ([2.12])

AC_DEFUN([AM_CONFIG_HEADER],
[ifdef([AC_FOREACH],dnl
	 [dnl init our file count if it isn't already
	 m4_ifndef([_AM_Config_Header_Index], m4_define([_AM_Config_Header_Index], [0]))
	 dnl prepare to store our destination file list for use in config.status
	 AC_FOREACH([_AM_File], [$1],
		    [m4_pushdef([_AM_Dest], m4_patsubst(_AM_File, [:.*]))
		    m4_define([_AM_Config_Header_Index], m4_incr(_AM_Config_Header_Index))
		    dnl and add it to the list of files AC keeps track of, along
		    dnl with our hook
		    AC_CONFIG_HEADERS(_AM_File,
dnl COMMANDS, [, INIT-CMDS]
[# update the timestamp
echo timestamp >"AS_ESCAPE(_AM_DIRNAME(]_AM_Dest[))/stamp-h]_AM_Config_Header_Index["
][$2]m4_ifval([$3], [, [$3]]))dnl AC_CONFIG_HEADERS
		    m4_popdef([_AM_Dest])])],dnl
[AC_CONFIG_HEADER([$1])
  AC_OUTPUT_COMMANDS(
   ifelse(patsubst([$1], [[^ ]], []),
	  [],
	  [test -z "$CONFIG_HEADERS" || echo timestamp >dnl
	   patsubst([$1], [^\([^:]*/\)?.*], [\1])stamp-h]),dnl
[am_indx=1
for am_file in $1; do
  case " \$CONFIG_HEADERS " in
  *" \$am_file "*)
    am_dir=\`echo \$am_file |sed 's%:.*%%;s%[^/]*\$%%'\`
    if test -n "\$am_dir"; then
      am_tmpdir=\`echo \$am_dir |sed 's%^\(/*\).*\$%\1%'\`
      for am_subdir in \`echo \$am_dir |sed 's%/% %'\`; do
        am_tmpdir=\$am_tmpdir\$am_subdir/
        if test ! -d \$am_tmpdir; then
          mkdir \$am_tmpdir
        fi
      done
    fi
    echo timestamp > "\$am_dir"stamp-h\$am_indx
    ;;
  esac
  am_indx=\`expr \$am_indx + 1\`
done])
])]) # AM_CONFIG_HEADER

# _AM_DIRNAME(PATH)
# -----------------
# Like AS_DIRNAME, only do it during macro expansion
AC_DEFUN([_AM_DIRNAME],
       [m4_if(m4_regexp([$1], [^.*[^/]//*[^/][^/]*/*$]), -1,
	      m4_if(m4_regexp([$1], [^//\([^/]\|$\)]), -1,
		    m4_if(m4_regexp([$1], [^/.*]), -1,
			  [.],
			  m4_patsubst([$1], [^\(/\).*], [\1])),
		    m4_patsubst([$1], [^\(//\)\([^/].*\|$\)], [\1])),
	      m4_patsubst([$1], [^\(.*[^/]\)//*[^/][^/]*/*$], [\1]))[]dnl
]) # _AM_DIRNAME

# Do all the work for Automake.  This macro actually does too much --
# some checks are only needed if your package does certain things.
# But this isn't really a big deal.

# serial 5

# There are a few dirty hacks below to avoid letting `AC_PROG_CC' be
# written in clear, in which case automake, when reading aclocal.m4,
# will think it sees a *use*, and therefore will trigger all it's
# C support machinery.  Also note that it means that autoscan, seeing
# CC etc. in the Makefile, will ask for an AC_PROG_CC use...


# We require 2.13 because we rely on SHELL being computed by configure.
AC_PREREQ([2.13])

# AC_PROVIDE_IFELSE(MACRO-NAME, IF-PROVIDED, IF-NOT-PROVIDED)
# -----------------------------------------------------------
# If MACRO-NAME is provided do IF-PROVIDED, else IF-NOT-PROVIDED.
# The purpose of this macro is to provide the user with a means to
# check macros which are provided without letting her know how the
# information is coded.
# If this macro is not defined by Autoconf, define it here.
ifdef([AC_PROVIDE_IFELSE],
      [],
      [define([AC_PROVIDE_IFELSE],
              [ifdef([AC_PROVIDE_$1],
                     [$2], [$3])])])


# AM_INIT_AUTOMAKE(PACKAGE,VERSION, [NO-DEFINE])
# ----------------------------------------------
AC_DEFUN([AM_INIT_AUTOMAKE],
[AC_REQUIRE([AC_PROG_INSTALL])dnl
# test to see if srcdir already configured
if test "`CDPATH=:; cd $srcdir && pwd`" != "`pwd`" &&
   test -f $srcdir/config.status; then
  AC_MSG_ERROR([source directory already configured; run \"make distclean\" there first])
fi

# Define the identity of the package.
PACKAGE=$1
AC_SUBST(PACKAGE)dnl
VERSION=$2
AC_SUBST(VERSION)dnl
ifelse([$3],,
[AC_DEFINE_UNQUOTED(PACKAGE, "$PACKAGE", [Name of package])
AC_DEFINE_UNQUOTED(VERSION, "$VERSION", [Version number of package])])

# Autoconf 2.50 wants to disallow AM_ names.  We explicitly allow
# the ones we care about.
ifdef([m4_pattern_allow],
      [m4_pattern_allow([^AM_[A-Z]+FLAGS])])dnl

# Autoconf 2.50 always computes EXEEXT.  However we need to be
# compatible with 2.13, for now.  So we always define EXEEXT, but we
# don't compute it.
AC_SUBST(EXEEXT)
# Similar for OBJEXT -- only we only use OBJEXT if the user actually
# requests that it be used.  This is a bit dumb.
: ${OBJEXT=o}
AC_SUBST(OBJEXT)

# Some tools Automake needs.
AC_REQUIRE([AM_SANITY_CHECK])dnl
AC_REQUIRE([AC_ARG_PROGRAM])dnl
AM_MISSING_PROG(ACLOCAL, aclocal)
AM_MISSING_PROG(AUTOCONF, autoconf)
AM_MISSING_PROG(AUTOMAKE, automake)
AM_MISSING_PROG(AUTOHEADER, autoheader)
AM_MISSING_PROG(MAKEINFO, makeinfo)
AM_MISSING_PROG(AMTAR, tar)
AM_PROG_INSTALL_SH
AM_PROG_INSTALL_STRIP
# We need awk for the "check" target.  The system "awk" is bad on
# some platforms.
AC_REQUIRE([AC_PROG_AWK])dnl
AC_REQUIRE([AC_PROG_MAKE_SET])dnl
AC_REQUIRE([AM_DEP_TRACK])dnl
AC_REQUIRE([AM_SET_DEPDIR])dnl
AC_PROVIDE_IFELSE([AC_PROG_][CC],
                  [_AM_DEPENDENCIES(CC)],
                  [define([AC_PROG_][CC],
                          defn([AC_PROG_][CC])[_AM_DEPENDENCIES(CC)])])dnl
AC_PROVIDE_IFELSE([AC_PROG_][CXX],
                  [_AM_DEPENDENCIES(CXX)],
                  [define([AC_PROG_][CXX],
                          defn([AC_PROG_][CXX])[_AM_DEPENDENCIES(CXX)])])dnl
])

#
# Check to make sure that the build environment is sane.
#

# serial 3

# AM_SANITY_CHECK
# ---------------
AC_DEFUN([AM_SANITY_CHECK],
[AC_MSG_CHECKING([whether build environment is sane])
# Just in case
sleep 1
echo timestamp > conftest.file
# Do `set' in a subshell so we don't clobber the current shell's
# arguments.  Must try -L first in case configure is actually a
# symlink; some systems play weird games with the mod time of symlinks
# (eg FreeBSD returns the mod time of the symlink's containing
# directory).
if (
   set X `ls -Lt $srcdir/configure conftest.file 2> /dev/null`
   if test "$[*]" = "X"; then
      # -L didn't work.
      set X `ls -t $srcdir/configure conftest.file`
   fi
   rm -f conftest.file
   if test "$[*]" != "X $srcdir/configure conftest.file" \
      && test "$[*]" != "X conftest.file $srcdir/configure"; then

      # If neither matched, then we have a broken ls.  This can happen
      # if, for instance, CONFIG_SHELL is bash and it inherits a
      # broken ls alias from the environment.  This has actually
      # happened.  Such a system could not be considered "sane".
      AC_MSG_ERROR([ls -t appears to fail.  Make sure there is not a broken
alias in your environment])
   fi

   test "$[2]" = conftest.file
   )
then
   # Ok.
   :
else
   AC_MSG_ERROR([newly created file is older than distributed files!
Check your system clock])
fi
AC_MSG_RESULT(yes)])


# serial 2

# AM_MISSING_PROG(NAME, PROGRAM)
# ------------------------------
AC_DEFUN([AM_MISSING_PROG],
[AC_REQUIRE([AM_MISSING_HAS_RUN])
$1=${$1-"${am_missing_run}$2"}
AC_SUBST($1)])


# AM_MISSING_HAS_RUN
# ------------------
# Define MISSING if not defined so far and test if it supports --run.
# If it does, set am_missing_run to use it, otherwise, to nothing.
AC_DEFUN([AM_MISSING_HAS_RUN],
[AC_REQUIRE([AM_AUX_DIR_EXPAND])dnl
test x"${MISSING+set}" = xset || MISSING="\${SHELL} $am_aux_dir/missing"
# Use eval to expand $SHELL
if eval "$MISSING --run true"; then
  am_missing_run="$MISSING --run "
else
  am_missing_run=
  am_backtick='`'
  AC_MSG_WARN([${am_backtick}missing' script is too old or missing])
fi
])

# AM_AUX_DIR_EXPAND

# For projects using AC_CONFIG_AUX_DIR([foo]), Autoconf sets
# $ac_aux_dir to `$srcdir/foo'.  In other projects, it is set to
# `$srcdir', `$srcdir/..', or `$srcdir/../..'.
#
# Of course, Automake must honor this variable whenever it calls a
# tool from the auxiliary directory.  The problem is that $srcdir (and
# therefore $ac_aux_dir as well) can be either absolute or relative,
# depending on how configure is run.  This is pretty annoying, since
# it makes $ac_aux_dir quite unusable in subdirectories: in the top
# source directory, any form will work fine, but in subdirectories a
# relative path needs to be adjusted first.
#
# $ac_aux_dir/missing
#    fails when called from a subdirectory if $ac_aux_dir is relative
# $top_srcdir/$ac_aux_dir/missing
#    fails if $ac_aux_dir is absolute,
#    fails when called from a subdirectory in a VPATH build with
#          a relative $ac_aux_dir
#
# The reason of the latter failure is that $top_srcdir and $ac_aux_dir
# are both prefixed by $srcdir.  In an in-source build this is usually
# harmless because $srcdir is `.', but things will broke when you
# start a VPATH build or use an absolute $srcdir.
#
# So we could use something similar to $top_srcdir/$ac_aux_dir/missing,
# iff we strip the leading $srcdir from $ac_aux_dir.  That would be:
#   am_aux_dir='\$(top_srcdir)/'`expr "$ac_aux_dir" : "$srcdir//*\(.*\)"`
# and then we would define $MISSING as
#   MISSING="\${SHELL} $am_aux_dir/missing"
# This will work as long as MISSING is not called from configure, because
# unfortunately $(top_srcdir) has no meaning in configure.
# However there are other variables, like CC, which are often used in
# configure, and could therefore not use this "fixed" $ac_aux_dir.
#
# Another solution, used here, is to always expand $ac_aux_dir to an
# absolute PATH.  The drawback is that using absolute paths prevent a
# configured tree to be moved without reconfiguration.

AC_DEFUN([AM_AUX_DIR_EXPAND], [
# expand $ac_aux_dir to an absolute path
am_aux_dir=`CDPATH=:; cd $ac_aux_dir && pwd`
])

# AM_PROG_INSTALL_SH
# ------------------
# Define $install_sh.
AC_DEFUN([AM_PROG_INSTALL_SH],
[AC_REQUIRE([AM_AUX_DIR_EXPAND])dnl
install_sh=${install_sh-"$am_aux_dir/install-sh"}
AC_SUBST(install_sh)])

# One issue with vendor `install' (even GNU) is that you can't
# specify the program used to strip binaries.  This is especially
# annoying in cross-compiling environments, where the build's strip
# is unlikely to handle the host's binaries.
# Fortunately install-sh will honor a STRIPPROG variable, so we
# always use install-sh in `make install-strip', and initialize
# STRIPPROG with the value of the STRIP variable (set by the user).
AC_DEFUN([AM_PROG_INSTALL_STRIP],
[AC_REQUIRE([AM_PROG_INSTALL_SH])dnl
INSTALL_STRIP_PROGRAM="\${SHELL} \$(install_sh) -c -s"
AC_SUBST([INSTALL_STRIP_PROGRAM])])

# serial 4						-*- Autoconf -*-



# There are a few dirty hacks below to avoid letting `AC_PROG_CC' be
# written in clear, in which case automake, when reading aclocal.m4,
# will think it sees a *use*, and therefore will trigger all it's
# C support machinery.  Also note that it means that autoscan, seeing
# CC etc. in the Makefile, will ask for an AC_PROG_CC use...



# _AM_DEPENDENCIES(NAME)
# ---------------------
# See how the compiler implements dependency checking.
# NAME is "CC", "CXX" or "OBJC".
# We try a few techniques and use that to set a single cache variable.
#
# We don't AC_REQUIRE the corresponding AC_PROG_CC since the latter was
# modified to invoke _AM_DEPENDENCIES(CC); we would have a circular
# dependency, and given that the user is not expected to run this macro,
# just rely on AC_PROG_CC.
AC_DEFUN([_AM_DEPENDENCIES],
[AC_REQUIRE([AM_SET_DEPDIR])dnl
AC_REQUIRE([AM_OUTPUT_DEPENDENCY_COMMANDS])dnl
AC_REQUIRE([AM_MAKE_INCLUDE])dnl
AC_REQUIRE([AM_DEP_TRACK])dnl

ifelse([$1], CC,   [depcc="$CC"   am_compiler_list=],
       [$1], CXX,  [depcc="$CXX"  am_compiler_list=],
       [$1], OBJC, [depcc="$OBJC" am_compiler_list='gcc3 gcc']
       [$1], GCJ,  [depcc="$GCJ"  am_compiler_list='gcc3 gcc'],
                   [depcc="$$1"   am_compiler_list=])

AC_CACHE_CHECK([dependency style of $depcc],
               [am_cv_$1_dependencies_compiler_type],
[if test -z "$AMDEP_TRUE" && test -f "$am_depcomp"; then
  # We make a subdir and do the tests there.  Otherwise we can end up
  # making bogus files that we don't know about and never remove.  For
  # instance it was reported that on HP-UX the gcc test will end up
  # making a dummy file named `D' -- because `-MD' means `put the output
  # in D'.
  mkdir conftest.dir
  # Copy depcomp to subdir because otherwise we won't find it if we're
  # using a relative directory.
  cp "$am_depcomp" conftest.dir
  cd conftest.dir

  am_cv_$1_dependencies_compiler_type=none
  if test "$am_compiler_list" = ""; then
     am_compiler_list=`sed -n ['s/^#*\([a-zA-Z0-9]*\))$/\1/p'] < ./depcomp`
  fi
  for depmode in $am_compiler_list; do
    # We need to recreate these files for each test, as the compiler may
    # overwrite some of them when testing with obscure command lines.
    # This happens at least with the AIX C compiler.
    echo '#include "conftest.h"' > conftest.c
    echo 'int i;' > conftest.h
    echo "${am__include} ${am__quote}conftest.Po${am__quote}" > confmf

    case $depmode in
    nosideeffect)
      # after this tag, mechanisms are not by side-effect, so they'll
      # only be used when explicitly requested
      if test "x$enable_dependency_tracking" = xyes; then
	continue
      else
	break
      fi
      ;;
    none) break ;;
    esac
    # We check with `-c' and `-o' for the sake of the "dashmstdout"
    # mode.  It turns out that the SunPro C++ compiler does not properly
    # handle `-M -o', and we need to detect this.
    if depmode=$depmode \
       source=conftest.c object=conftest.o \
       depfile=conftest.Po tmpdepfile=conftest.TPo \
       $SHELL ./depcomp $depcc -c conftest.c -o conftest.o >/dev/null 2>&1 &&
       grep conftest.h conftest.Po > /dev/null 2>&1 &&
       ${MAKE-make} -s -f confmf > /dev/null 2>&1; then
      am_cv_$1_dependencies_compiler_type=$depmode
      break
    fi
  done

  cd ..
  rm -rf conftest.dir
else
  am_cv_$1_dependencies_compiler_type=none
fi
])
$1DEPMODE="depmode=$am_cv_$1_dependencies_compiler_type"
AC_SUBST([$1DEPMODE])
])


# AM_SET_DEPDIR
# -------------
# Choose a directory name for dependency files.
# This macro is AC_REQUIREd in _AM_DEPENDENCIES
AC_DEFUN([AM_SET_DEPDIR],
[rm -f .deps 2>/dev/null
mkdir .deps 2>/dev/null
if test -d .deps; then
  DEPDIR=.deps
else
  # MS-DOS does not allow filenames that begin with a dot.
  DEPDIR=_deps
fi
rmdir .deps 2>/dev/null
AC_SUBST(DEPDIR)
])


# AM_DEP_TRACK
# ------------
AC_DEFUN([AM_DEP_TRACK],
[AC_ARG_ENABLE(dependency-tracking,
[  --disable-dependency-tracking Speeds up one-time builds
  --enable-dependency-tracking  Do not reject slow dependency extractors])
if test "x$enable_dependency_tracking" != xno; then
  am_depcomp="$ac_aux_dir/depcomp"
  AMDEPBACKSLASH='\'
fi
AM_CONDITIONAL([AMDEP], [test "x$enable_dependency_tracking" != xno])
pushdef([subst], defn([AC_SUBST]))
subst(AMDEPBACKSLASH)
popdef([subst])
])

# Generate code to set up dependency tracking.
# This macro should only be invoked once -- use via AC_REQUIRE.
# Usage:
# AM_OUTPUT_DEPENDENCY_COMMANDS

#
# This code is only required when automatic dependency tracking
# is enabled.  FIXME.  This creates each `.P' file that we will
# need in order to bootstrap the dependency handling code.
AC_DEFUN([AM_OUTPUT_DEPENDENCY_COMMANDS],[
AC_OUTPUT_COMMANDS([
test x"$AMDEP_TRUE" != x"" ||
for mf in $CONFIG_FILES; do
  case "$mf" in
  Makefile) dirpart=.;;
  */Makefile) dirpart=`echo "$mf" | sed -e 's|/[^/]*$||'`;;
  *) continue;;
  esac
  grep '^DEP_FILES *= *[^ #]' < "$mf" > /dev/null || continue
  # Extract the definition of DEP_FILES from the Makefile without
  # running `make'.
  DEPDIR=`sed -n -e '/^DEPDIR = / s///p' < "$mf"`
  test -z "$DEPDIR" && continue
  # When using ansi2knr, U may be empty or an underscore; expand it
  U=`sed -n -e '/^U = / s///p' < "$mf"`
  test -d "$dirpart/$DEPDIR" || mkdir "$dirpart/$DEPDIR"
  # We invoke sed twice because it is the simplest approach to
  # changing $(DEPDIR) to its actual value in the expansion.
  for file in `sed -n -e '
    /^DEP_FILES = .*\\\\$/ {
      s/^DEP_FILES = //
      :loop
	s/\\\\$//
	p
	n
	/\\\\$/ b loop
      p
    }
    /^DEP_FILES = / s/^DEP_FILES = //p' < "$mf" | \
       sed -e 's/\$(DEPDIR)/'"$DEPDIR"'/g' -e 's/\$U/'"$U"'/g'`; do
    # Make sure the directory exists.
    test -f "$dirpart/$file" && continue
    fdir=`echo "$file" | sed -e 's|/[^/]*$||'`
    $ac_aux_dir/mkinstalldirs "$dirpart/$fdir" > /dev/null 2>&1
    # echo "creating $dirpart/$file"
    echo '# dummy' > "$dirpart/$file"
  done
done
], [AMDEP_TRUE="$AMDEP_TRUE"
ac_aux_dir="$ac_aux_dir"])])

# AM_MAKE_INCLUDE()
# -----------------
# Check to see how make treats includes.
AC_DEFUN([AM_MAKE_INCLUDE],
[am_make=${MAKE-make}
cat > confinc << 'END'
doit:
	@echo done
END
# If we don't find an include directive, just comment out the code.
AC_MSG_CHECKING([for style of include used by $am_make])
am__include='#'
am__quote=
_am_result=none
# First try GNU make style include.
echo "include confinc" > confmf
# We grep out `Entering directory' and `Leaving directory'
# messages which can occur if `w' ends up in MAKEFLAGS.
# In particular we don't look at `^make:' because GNU make might
# be invoked under some other name (usually "gmake"), in which
# case it prints its new name instead of `make'.
if test "`$am_make -s -f confmf 2> /dev/null | fgrep -v 'ing directory'`" = "done"; then
   am__include=include
   am__quote=
   _am_result=GNU
fi
# Now try BSD make style include.
if test "$am__include" = "#"; then
   echo '.include "confinc"' > confmf
   if test "`$am_make -s -f confmf 2> /dev/null`" = "done"; then
      am__include=.include
      am__quote='"'
      _am_result=BSD
   fi
fi
AC_SUBST(am__include)
AC_SUBST(am__quote)
AC_MSG_RESULT($_am_result)
rm -f confinc confmf
])

# serial 3

# AM_CONDITIONAL(NAME, SHELL-CONDITION)
# -------------------------------------
# Define a conditional.
#
# FIXME: Once using 2.50, use this:
# m4_match([$1], [^TRUE\|FALSE$], [AC_FATAL([$0: invalid condition: $1])])dnl
AC_DEFUN([AM_CONDITIONAL],
[ifelse([$1], [TRUE],
        [errprint(__file__:__line__: [$0: invalid condition: $1
])dnl
m4exit(1)])dnl
ifelse([$1], [FALSE],
       [errprint(__file__:__line__: [$0: invalid condition: $1
])dnl
m4exit(1)])dnl
AC_SUBST([$1_TRUE])
AC_SUBST([$1_FALSE])
if $2; then
  $1_TRUE=
  $1_FALSE='#'
else
  $1_TRUE='#'
  $1_FALSE=
fi])


# AM_PROG_LEX
# Look for flex, lex or missing, then run AC_PROG_LEX and AC_DECL_YYTEXT
AC_DEFUN([AM_PROG_LEX],
[AC_REQUIRE([AM_MISSING_HAS_RUN])
AC_CHECK_PROGS(LEX, flex lex, [${am_missing_run}flex])
AC_PROG_LEX
AC_DECL_YYTEXT])

