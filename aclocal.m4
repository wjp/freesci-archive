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

_ac_ggi_includes="-I/usr/include -I/usr/local/include"
_ac_ggi_libraries="-L/usr/lib -L/usr/local/lib -L/usr/local/ggi/lib"


AC_ARG_WITH(ggi-dir,
    [  --with-ggi-dir           where the root of ggi is installed ],
    [  _ac_ggi_includes="-I$withval"/include
       _ac_ggi_libraries="-L$withval"/lib
    ])

AC_ARG_WITH(ggi-includes,
    [  --with-ggi-includes      where the ggi includes are. ],
    [  
       _ac_ggi_includes="-I$withval"
    ])
AC_ARG_WITH(ggi-libraries,
    [  --with-ggi-libraries     where the ggi library is installed.],
    [  _ac_ggi_libraries="-L$withval"
    ])

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
fi

AC_SUBST(ac_ggi_includes)
AC_SUBST(ac_ggi_libraries)
])


AC_DEFUN(AC_PATH_CURSES,
[
AC_MSG_CHECKING([for (n)curses])

_ac_curses_includes="-I/usr/include -I/usr/local/include"
_ac_curses_libraries="-L/usr/lib -L/usr/local/lib"
_ac_curses_libnames="-lncurses -lcurses"


AC_ARG_WITH(curses-dir,
    [  --with-curses-dir        where the root of lib(n)curses is installed ],
    [  _ac_curses_includes="-I$withval"/include
       _ac_curses_libraries="-L$withval"/lib
    ])

AC_ARG_WITH(curses-includes,
    [  --with-curses-includes   where the (n)curses includes are. ],
    [  
       _ac_curses_includes="-I$withval"
    ])
AC_ARG_WITH(curses-library,
    [  --with-curses-libraries  where the (n)curses library is installed.],
    [  _ac_curses_libraries="-L$withval"
    ])
AC_ARG_WITH(curses-libname,
    [  --with-curses-libraries  whether to use curses or ncurses.],
    [  _ac_curses_libnames="-l$withval"
    ])

AC_CHECK_INCLUDE_PATH([curses.h],[$_ac_curses_includes],[], ac_curses_includes)
AC_CHECK_LINK_PATH([initscr();],$_ac_curses_libraries,$_ac_curses_libnames,
		 [$ac_curses_includes],[#include <curses.h>], ac_curses_libraries)

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
    [  --with-png-dir           where the root of png is installed ],
    [  _ac_png_includes="-I$withval"/include
       _ac_png_libraries="-L$withval"/lib
    ])

AC_ARG_WITH(png-includes,
    [  --with-png-includes      where the png includes are. ],
    [  
       _ac_png_includes="-I$withval"
    ])
AC_ARG_WITH(png-libraries,
    [  --with-png-libraries     where the png library is installed.],
    [  _ac_png_libraries="-L$withval"
    ])

LIBS="$LIBS -lpng -lm"
AC_CHECK_INCLUDE_PATH([png.h],[$_ac_png_includes],[], ac_png_includes)
AC_CHECK_LINK_PATH([png_info_init((png_infop)0);],$_ac_png_libraries,["-lpng"],
		 [$ac_png_includes],[#include <png.h>], ac_png_libraries)


if test "$ac_png_libraries" = no; then
# Try again with -lz
	LIBS="$LIBS -lz"
	AC_CHECK_LINK_PATH([png_info_init((png_infop)0);],$_ac_png_libraries,["-lpng"],
			 [$ac_png_includes],[#include <png.h>], ac_png_libraries)
fi

if test "$ac_png_includes" = no || test "$ac_png_libraries" = no; then

	AC_MSG_RESULT([failed])
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
	[  --with-endianness=ARG    whether big or little endianness should be used.],
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

