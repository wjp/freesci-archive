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


##
## Customizing functions for libggi: Based on similar functions for qt
##
AC_DEFUN(AC_PATH_GGI,
[
AC_MSG_CHECKING([for ggi])

_ac_ggi_includes="/usr/include /usr/local/include"
_ac_ggi_libraries="/usr/lib /usr/local/lib /usr/local/ggi/lib"


AC_ARG_WITH(ggi-dir,
    [  --with-ggi-dir           where the root of ggi is installed ],
    [  _ac_ggi_includes="$_ac_ggi_includes $withval"/include
       _ac_ggi_libraries="$_ac_ggi_libraries $withval"/lib
    ])

AC_ARG_WITH(ggi-includes,
    [  --with-ggi-includes      where the ggi includes are. ],
    [  
       _ac_ggi_includes="$_ac_ggi_includes $withval"
    ])
AC_ARG_WITH(ggi-libraries,
    [  --with-ggi-libraries     where the ggi library is installed.],
    [  _ac_ggi_libraries="$_ac_ggi_libraries $withval"
    ])

AC_FIND_FILE(ggi/ggi.h,$_ac_ggi_includes,ac_ggi_includes)
AC_FIND_FILE(libggi.so,$_ac_ggi_libraries,ac_ggi_libraries)

if test "$ac_ggi_includes" = NO || test "$ac_ggi_libraries" = NO; then

	AC_MSG_RESULT([failed])
	AC_MSG_WARN([ggi is required for graphics support!])
	ac_ggi_libraries=""
	ac_ggi_includes=""
else
	AC_MSG_RESULT([found]);

	ac_ggi_includes=-I"$ac_ggi_includes"
	ac_ggi_libraries=-L"$ac_ggi_libraries -lggi -lgii"
	ac_graphics_ggi_libfile="graphics_ggi.c"
	AC_SUBST(ac_graphics_ggi_libfile)
	ac_graphics_ggi_libobjects="graphics_ggi.o"
	AC_SUBST(ac_graphics_ggi_libobjects)
	AC_DEFINE(HAVE_LIBGGI)
fi
AC_SUBST(ac_ggi_includes)
AC_SUBST(ac_ggi_libraries)
])

##
## Customizing functions for libpng: Based on similar functions for qt
##
AC_DEFUN(AC_PATH_PNG,
[
AC_MSG_CHECKING([for libpng])
## libpng must be statically linked against libz for this to work... :-/

_ac_png_includes="/usr/include /usr/local/include"
_ac_png_libraries="/usr/lib /usr/local/lib"


AC_ARG_WITH(png-dir,
    [  --with-png-dir           where the root of png is installed ],
    [  _ac_png_includes="$_ac_png_includes $withval"/include
       _ac_png_libraries="$_ac_png_libraries $withval"/lib
    ])

AC_ARG_WITH(png-includes,
    [  --with-png-includes      where the png includes are. ],
    [  
       _ac_png_includes="$_ac_png_includes $withval"
    ])
AC_ARG_WITH(png-libraries,
    [  --with-png-libraries     where the png library is installed.],
    [  _ac_png_libraries="$_ac_png_libraries $withval"
    ])

AC_FIND_FILE(png.h,$_ac_png_includes,ac_png_includes)
AC_FIND_FILE(libpng.so.1,$_ac_png_libraries,ac_png_libraries)

if test "$ac_png_includes" = NO || test "$ac_png_libraries" = NO; then

	AC_MSG_RESULT([failed])
	ac_png_libraries=""
	ac_png_includes=""
else
	AC_MSG_RESULT([found]);

	ac_png_includes=-I"$ac_png_includes"
	ac_png_libraries=-L"$ac_png_libraries -lpng"
	AC_DEFINE(HAVE_LIBPNG)
fi
AC_SUBST(ac_png_includes)
AC_SUBST(ac_png_libraries)
])


##
## Customizing functions for libglib: Based on similar functions for qt
##
AC_DEFUN(AC_PATH_GLIB,
[
AC_MSG_CHECKING([for glib])

_ac_glib_includes="/usr/include /usr/local/include"
_ac_glib_libraries="/usr/lib /usr/local/lib /usr/local/glib/lib"

AC_ARG_WITH(glib-dir,
    [  --with-glib-dir           where the root of glib is installed ],
    [  _ac_glib_includes="$_ac_glib_includes $withval"/include
       _ac_glib_libraries="$_ac_glib_libraries $withval"/lib
    ])

AC_ARG_WITH(glib-includes,
    [  --with-glib-includes      where the glib includes are. ],
    [  
       _ac_glib_includes="$_ac_glib_includes $withval"
    ])
AC_ARG_WITH(glib-libraries,
    [  --with-glib-libraries     where the glib library is installed.],
    [  _ac_glib_libraries="$_ac_glib_libraries $withval"
    ])
AC_FIND_FILE(glib.h,$_ac_glib_includes,ac_glib_includes)
AC_FIND_FILE(libglib.a,$_ac_glib_libraries,ac_glib_libraries)

if test "$ac_glib_includes" = NO || test "$ac_glib_libraries" = NO; then
	AC_MSG_RESULT([failed])
	AC_MSG_ERROR([You must install glib to compile this package!])
else
	AC_MSG_RESULT([found]);

	ac_glib_includes=-I"$ac_glib_includes"
	ac_glib_libraries=-L"$ac_glib_libraries"
	AC_SUBST(ac_glib_includes)
	AC_SUBST(ac_glib_libraries)
fi
])

# Configure paths for GLIB
# Owen Taylor     97-11-3

dnl AM_PATH_GLIB([MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND [, MODULES]]]])
dnl Test for GLIB, and define GLIB_CFLAGS and GLIB_LIBS, if "gmodule" or 
dnl gthread is specified in MODULES, pass to glib-config
dnl
AC_DEFUN(AM_PATH_GLIB,
[dnl 
dnl Get the cflags and libraries from the glib-config script
dnl
AC_ARG_WITH(glib-prefix,[  --with-glib-prefix=PFX   Prefix where GLIB is installed (optional)],
            glib_config_prefix="$withval", glib_config_prefix="")
AC_ARG_WITH(glib-exec-prefix,[  --with-glib-exec-prefix=PFX Exec prefix where GLIB is installed (optional)],
            glib_config_exec_prefix="$withval", glib_config_exec_prefix="")
AC_ARG_ENABLE(glibtest, [  --disable-glibtest       Do not try to compile and run a test GLIB program],
		    , enable_glibtest=yes)

  if test x$glib_config_exec_prefix != x ; then
     glib_config_args="$glib_config_args --exec-prefix=$glib_config_exec_prefix"
     if test x${GLIB_CONFIG+set} != xset ; then
        GLIB_CONFIG=$glib_config_exec_prefix/bin/glib-config
     fi
  fi
  if test x$glib_config_prefix != x ; then
     glib_config_args="$glib_config_args --prefix=$glib_config_prefix"
     if test x${GLIB_CONFIG+set} != xset ; then
        GLIB_CONFIG=$glib_config_prefix/bin/glib-config
     fi
  fi

  for module in . $4
  do
      case "$module" in
         gmodule) 
             glib_config_args="$glib_config_args gmodule"
         ;;
         gthread) 
             glib_config_args="$glib_config_args gthread"
         ;;
      esac
  done

  AC_PATH_PROG(GLIB_CONFIG, glib-config, no)
  min_glib_version=ifelse([$1], ,0.99.7,$1)
  AC_MSG_CHECKING(for GLIB - version >= $min_glib_version)
  no_glib=""
  if test "$GLIB_CONFIG" = "no" ; then
    no_glib=yes
  else
    GLIB_CFLAGS=`$GLIB_CONFIG $glib_config_args --cflags`
    GLIB_LIBS=`$GLIB_CONFIG $glib_config_args --libs`
    glib_config_major_version=`$GLIB_CONFIG $glib_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    glib_config_minor_version=`$GLIB_CONFIG $glib_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    glib_config_micro_version=`$GLIB_CONFIG $glib_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`
    if test "x$enable_glibtest" = "xyes" ; then
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LIBS="$LIBS"
      CFLAGS="$CFLAGS $GLIB_CFLAGS"
      LIBS="$GLIB_LIBS $LIBS"
dnl
dnl Now check if the installed GLIB is sufficiently new. (Also sanity
dnl checks the results of glib-config to some extent
dnl
      rm -f conf.glibtest
      AC_TRY_RUN([
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>

int 
main ()
{
  int major, minor, micro;
  char *tmp_version;

  system ("touch conf.glibtest");

  /* HP/UX 9 (%@#!) writes to sscanf strings */
  tmp_version = g_strdup("$min_glib_version");
  if (sscanf(tmp_version, "%d.%d.%d", &major, &minor, &micro) != 3) {
     printf("%s, bad version string\n", "$min_glib_version");
     exit(1);
   }

  if ((glib_major_version != $glib_config_major_version) ||
      (glib_minor_version != $glib_config_minor_version) ||
      (glib_micro_version != $glib_config_micro_version))
    {
      printf("\n*** 'glib-config --version' returned %d.%d.%d, but GLIB (%d.%d.%d)\n", 
             $glib_config_major_version, $glib_config_minor_version, $glib_config_micro_version,
             glib_major_version, glib_minor_version, glib_micro_version);
      printf ("*** was found! If glib-config was correct, then it is best\n");
      printf ("*** to remove the old version of GLIB. You may also be able to fix the error\n");
      printf("*** by modifying your LD_LIBRARY_PATH enviroment variable, or by editing\n");
      printf("*** /etc/ld.so.conf. Make sure you have run ldconfig if that is\n");
      printf("*** required on your system.\n");
      printf("*** If glib-config was wrong, set the environment variable GLIB_CONFIG\n");
      printf("*** to point to the correct copy of glib-config, and remove the file config.cache\n");
      printf("*** before re-running configure\n");
    } 
  else if ((glib_major_version != GLIB_MAJOR_VERSION) ||
	   (glib_minor_version != GLIB_MINOR_VERSION) ||
           (glib_micro_version != GLIB_MICRO_VERSION))
    {
      printf("*** GLIB header files (version %d.%d.%d) do not match\n",
	     GLIB_MAJOR_VERSION, GLIB_MINOR_VERSION, GLIB_MICRO_VERSION);
      printf("*** library (version %d.%d.%d)\n",
	     glib_major_version, glib_minor_version, glib_micro_version);
    }
  else
    {
      if ((glib_major_version > major) ||
        ((glib_major_version == major) && (glib_minor_version > minor)) ||
        ((glib_major_version == major) && (glib_minor_version == minor) && (glib_micro_version >= micro)))
      {
        return 0;
       }
     else
      {
        printf("\n*** An old version of GLIB (%d.%d.%d) was found.\n",
               glib_major_version, glib_minor_version, glib_micro_version);
        printf("*** You need a version of GLIB newer than %d.%d.%d. The latest version of\n",
	       major, minor, micro);
        printf("*** GLIB is always available from ftp://ftp.gtk.org.\n");
        printf("***\n");
        printf("*** If you have already installed a sufficiently new version, this error\n");
        printf("*** probably means that the wrong copy of the glib-config shell script is\n");
        printf("*** being found. The easiest way to fix this is to remove the old version\n");
        printf("*** of GLIB, but you can also set the GLIB_CONFIG environment to point to the\n");
        printf("*** correct copy of glib-config. (In this case, you will have to\n");
        printf("*** modify your LD_LIBRARY_PATH enviroment variable, or edit /etc/ld.so.conf\n");
        printf("*** so that the correct libraries are found at run-time))\n");
      }
    }
  return 1;
}
],, no_glib=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
     fi
  fi
  if test "x$no_glib" = x ; then
     AC_MSG_RESULT(yes)
     ifelse([$2], , :, [$2])     
  else
     AC_MSG_RESULT(no)
     if test "$GLIB_CONFIG" = "no" ; then
       echo "*** The glib-config script installed by GLIB could not be found"
       echo "*** If GLIB was installed in PREFIX, make sure PREFIX/bin is in"
       echo "*** your path, or set the GLIB_CONFIG environment variable to the"
       echo "*** full path to glib-config."
     else
       if test -f conf.glibtest ; then
        :
       else
          echo "*** Could not run GLIB test program, checking why..."
          CFLAGS="$CFLAGS $GLIB_CFLAGS"
          LIBS="$LIBS $GLIB_LIBS"
          AC_TRY_LINK([
#include <glib.h>
#include <stdio.h>
],      [ return ((glib_major_version) || (glib_minor_version) || (glib_micro_version)); ],
        [ echo "*** The test program compiled, but did not run. This usually means"
          echo "*** that the run-time linker is not finding GLIB or finding the wrong"
          echo "*** version of GLIB. If it is not finding GLIB, you'll need to set your"
          echo "*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point"
          echo "*** to the installed location  Also, make sure you have run ldconfig if that"
          echo "*** is required on your system"
	  echo "***"
          echo "*** If you have an old version installed, it is best to remove it, although"
          echo "*** you may also be able to get things to work by modifying LD_LIBRARY_PATH"
          echo "***"
          echo "*** If you have a RedHat 5.0 system, you should remove the GTK package that"
          echo "*** came with the system with the command"
          echo "***"
          echo "***    rpm --erase --nodeps gtk gtk-devel" ],
        [ echo "*** The test program failed to compile or link. See the file config.log for the"
          echo "*** exact error that occured. This usually means GLIB was incorrectly installed"
          echo "*** or that you have moved GLIB since it was installed. In the latter case, you"
          echo "*** may want to edit the glib-config script: $GLIB_CONFIG" ])
          CFLAGS="$ac_save_CFLAGS"
          LIBS="$ac_save_LIBS"
       fi
     fi
     GLIB_CFLAGS=""
     GLIB_LIBS=""
     ifelse([$3], , :, [$3])
  fi
  AC_SUBST(GLIB_CFLAGS)
  AC_SUBST(GLIB_LIBS)
  rm -f conf.glibtest
])
