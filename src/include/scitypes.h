/***************************************************************************
 resource.h Copyright (C) 2001 Christoph Reichenbach


 This program may be modified and copied freely according to the terms of
 the GNU general public license (GPL), as long as the above copyright
 notice and the licensing information contained herein are preserved.

 Please refer to www.gnu.org for licensing details.

 This work is provided AS IS, without warranty of any kind, expressed or
 implied, including but not limited to the warranties of merchantibility,
 noninfringement, and fitness for a specific purpose. The author will not
 be held liable for any damage caused by this work or derivatives of it.

 By using this source code, you agree to the licensing terms as stated
 above.


 Please contact the maintainer for bug reports or inquiries.

 Current Maintainer:

    Christoph Reichenbach (CR) [creichen@rbg.informatik.tu-darmstadt.de]

***************************************************************************/

#ifndef SCI_TYPES
#define SCI_TYPES

#ifdef HAVE_DIRENT_H
#  include <sys/types.h>
#  include <dirent.h>
#endif
#ifdef _WIN32
#  include <io.h>
#endif

#if defined(WIN32) && defined(_MSC_VER)
#  define TYPE_16 short
#  define TYPE_32 int
#endif

#if defined(__BEOS__)
#  define TYPE_16 short
#  define TYPE_32 int
#endif

#ifndef TYPE_8
#  define TYPE_8 char /* Guaranteed by ISO */
#endif

#ifndef TYPE_16
#  if (SIZEOF_SHORT == 2)
#    define TYPE_16 short
#  elif (SIZEOF_INT == 2)
#    define TYPE_16 int
#  else
#    error "Could not find a 16 bit data type!"
#  endif
#endif /* !TYPE_16 */

#ifndef TYPE_32
#  if (SIZEOF_INT == 4)
#    define TYPE_32 int
#  elif (SIZEOF_LONG == 4)
#    define TYPE_32 long
#  else
#    error "Could not find a 32 bit data type!"
#  endif
#endif /* !TYPE_32 */

typedef signed TYPE_8 gint8;
typedef unsigned TYPE_8 guint8;

typedef signed TYPE_16 gint16;
typedef unsigned TYPE_16 guint16;

typedef signed TYPE_32 gint32;
typedef unsigned TYPE_32 guint32;

#undef TYPE_8
#undef TYPE_16
#undef TYPE_32

typedef struct {
        long tv_sec;
        long tv_usec;
} GTimeVal;

typedef struct {
#if defined(_WIN32) && defined(_MSC_VER)
	long search;
	struct _finddata_t fileinfo;
#else
	DIR *dir;
	char *mask_copy;
#endif
} sci_dir_t; /* used by sci_find_first and friends */

#endif /* !SCI_TYPES */

