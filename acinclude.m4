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
	AC_MSG_WARN([ggi is required for graphics support!])
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

AC_CHECK_INCLUDE_PATH([png.h],[$_ac_png_includes],[], ac_png_includes)
AC_CHECK_LINK_PATH([png_info_init((png_infop)0);],$_ac_png_libraries,["-lpng"],
		 [$ac_png_includes],[#include <png.h>], ac_png_libraries)

if test "$ac_png_includes" = no || test "$ac_png_libraries" = no; then

	AC_MSG_RESULT([failed])
	AC_MSG_ERROR([libpng is required to compile FreeSCI.])
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
	[  --with-endianness        whether big or little endianness should be used.],
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