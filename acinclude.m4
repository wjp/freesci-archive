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

