## acinclude.m4 Copyright (C) 1999 Christoph Reichenbach, TU Darmstadt
##
##
## This program may be modified and copied freely according to the terms of
## the GNU general public license (GPL), as long as the above copyright
## notice and the licensing information contained herein are preserved.
##
## Please refer to www.gnu.org for licensing details.
##
## This work is provided AS IS, without warranty of any kind, expressed or
## implied, including but not limited to the warranties of merchantibility,
## noninfringement, and fitness for a specific purpose. The author will not
## be held liable for any damage caused by this work or derivatives of it.
##
## By using this source code, you agree to the licensing terms as stated
## above.
##
##
## Please contact the maintainer for bug reports or inquiries.
##
## Current Maintainer:
##
##    Christoph Reichenbach (CJR) [creichen@rbg.informatik.tu-darmstadt.de]
##
##
##
## Contrary to the usual convention, this file contains few publically
## useable macros; instead, it is used to keep the config.in file clean.
##


## ------------------------------------------------------------------------
## Find a file (or one of more files in a list of dirs)
## ------------------------------------------------------------------------
## copied from kvoice's acinclude.m4
##
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




##
## Customizing functions for libggi: Based on similar functions for qt
##
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

##
## Searching for curses/ncurses: Based on similar functions for qt
##
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

##
## Customizing functions for libpng: Based on similar functions for qt
##
AC_DEFUN(AC_PATH_PNG,
[
AC_MSG_CHECKING([for libpng])
## libpng must be statically linked against libz for this to work... :-/

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




##
## Checks for glx
##
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

## ----------------------------------- ##
## Check if --with-dmalloc was given.  ##
## From Franc,ois Pinard               ##
## ----------------------------------- ##

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
  LDFLAGS="$LDFLAGS -g"
else
  AC_MSG_RESULT(no)
fi], [AC_MSG_RESULT(no)])
])

## ------------------------------------------- ##
## Check if --with-malloc-debug was given.     ##
## From Alex Angas                             ##
## ------------------------------------------- ##

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

## -------------------------------------------- ##
## Check if --without-malloc-checks was given.  ##
## From Alex Angas                              ##
## -------------------------------------------- ##

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

## -------------------------------------------- ##
## Check if --without-sound was given.          ##
## From Alex Angas                              ##
## -------------------------------------------- ##

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
