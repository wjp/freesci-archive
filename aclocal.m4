dnl aclocal.m4 generated automatically by aclocal 1.4

dnl Copyright (C) 1994, 1995-8, 1999 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY, to the extent permitted by law; without
dnl even the implied warranty of MERCHANTABILITY or FITNESS FOR A
dnl PARTICULAR PURPOSE.



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

AC_DEFUN(AC_PATH_PNG,
[
AC_MSG_CHECKING([for libpng])

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
dnl Redhat 6 install libpng 1.0.3 in /usr/lib/libpng.2.1.0.3
AC_FIND_FILE(libpng.so,$_ac_png_libraries,ac_png_libraries)

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


# Like AC_CONFIG_HEADER, but automatically create stamp file.

AC_DEFUN(AM_CONFIG_HEADER,
[AC_PREREQ([2.12])
AC_CONFIG_HEADER([$1])
dnl When config.status generates a header, we must update the stamp-h file.
dnl This file resides in the same directory as the config header
dnl that is generated.  We must strip everything past the first ":",
dnl and everything past the last "/".
AC_OUTPUT_COMMANDS(changequote(<<,>>)dnl
ifelse(patsubst(<<$1>>, <<[^ ]>>, <<>>), <<>>,
<<test -z "<<$>>CONFIG_HEADERS" || echo timestamp > patsubst(<<$1>>, <<^\([^:]*/\)?.*>>, <<\1>>)stamp-h<<>>dnl>>,
<<am_indx=1
for am_file in <<$1>>; do
  case " <<$>>CONFIG_HEADERS " in
  *" <<$>>am_file "*<<)>>
    echo timestamp > `echo <<$>>am_file | sed -e 's%:.*%%' -e 's%[^/]*$%%'`stamp-h$am_indx
    ;;
  esac
  am_indx=`expr "<<$>>am_indx" + 1`
done<<>>dnl>>)
changequote([,]))])

# Do all the work for Automake.  This macro actually does too much --
# some checks are only needed if your package does certain things.
# But this isn't really a big deal.

# serial 1

dnl Usage:
dnl AM_INIT_AUTOMAKE(package,version, [no-define])

AC_DEFUN(AM_INIT_AUTOMAKE,
[AC_REQUIRE([AC_PROG_INSTALL])
PACKAGE=[$1]
AC_SUBST(PACKAGE)
VERSION=[$2]
AC_SUBST(VERSION)
dnl test to see if srcdir already configured
if test "`cd $srcdir && pwd`" != "`pwd`" && test -f $srcdir/config.status; then
  AC_MSG_ERROR([source directory already configured; run "make distclean" there first])
fi
ifelse([$3],,
AC_DEFINE_UNQUOTED(PACKAGE, "$PACKAGE", [Name of package])
AC_DEFINE_UNQUOTED(VERSION, "$VERSION", [Version number of package]))
AC_REQUIRE([AM_SANITY_CHECK])
AC_REQUIRE([AC_ARG_PROGRAM])
dnl FIXME This is truly gross.
missing_dir=`cd $ac_aux_dir && pwd`
AM_MISSING_PROG(ACLOCAL, aclocal, $missing_dir)
AM_MISSING_PROG(AUTOCONF, autoconf, $missing_dir)
AM_MISSING_PROG(AUTOMAKE, automake, $missing_dir)
AM_MISSING_PROG(AUTOHEADER, autoheader, $missing_dir)
AM_MISSING_PROG(MAKEINFO, makeinfo, $missing_dir)
AC_REQUIRE([AC_PROG_MAKE_SET])])

#
# Check to make sure that the build environment is sane.
#

AC_DEFUN(AM_SANITY_CHECK,
[AC_MSG_CHECKING([whether build environment is sane])
# Just in case
sleep 1
echo timestamp > conftestfile
# Do `set' in a subshell so we don't clobber the current shell's
# arguments.  Must try -L first in case configure is actually a
# symlink; some systems play weird games with the mod time of symlinks
# (eg FreeBSD returns the mod time of the symlink's containing
# directory).
if (
   set X `ls -Lt $srcdir/configure conftestfile 2> /dev/null`
   if test "[$]*" = "X"; then
      # -L didn't work.
      set X `ls -t $srcdir/configure conftestfile`
   fi
   if test "[$]*" != "X $srcdir/configure conftestfile" \
      && test "[$]*" != "X conftestfile $srcdir/configure"; then

      # If neither matched, then we have a broken ls.  This can happen
      # if, for instance, CONFIG_SHELL is bash and it inherits a
      # broken ls alias from the environment.  This has actually
      # happened.  Such a system could not be considered "sane".
      AC_MSG_ERROR([ls -t appears to fail.  Make sure there is not a broken
alias in your environment])
   fi

   test "[$]2" = conftestfile
   )
then
   # Ok.
   :
else
   AC_MSG_ERROR([newly created file is older than distributed files!
Check your system clock])
fi
rm -f conftest*
AC_MSG_RESULT(yes)])

dnl AM_MISSING_PROG(NAME, PROGRAM, DIRECTORY)
dnl The program must properly implement --version.
AC_DEFUN(AM_MISSING_PROG,
[AC_MSG_CHECKING(for working $2)
# Run test in a subshell; some versions of sh will print an error if
# an executable is not found, even if stderr is redirected.
# Redirect stdin to placate older versions of autoconf.  Sigh.
if ($2 --version) < /dev/null > /dev/null 2>&1; then
   $1=$2
   AC_MSG_RESULT(found)
else
   $1="$3/missing $2"
   AC_MSG_RESULT(missing)
fi
AC_SUBST($1)])


dnl AM_PROG_LEX
dnl Look for flex, lex or missing, then run AC_PROG_LEX and AC_DECL_YYTEXT
AC_DEFUN(AM_PROG_LEX,
[missing_dir=ifelse([$1],,`cd $ac_aux_dir && pwd`,$1)
AC_CHECK_PROGS(LEX, flex lex, "$missing_dir/missing flex")
AC_PROG_LEX
AC_DECL_YYTEXT])

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

