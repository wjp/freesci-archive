/***************************************************************************
 resource.h Copyright (C) 1999,2000,01 Christoph Reichenbach


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

 History:

   990327 - created (CR)

***************************************************************************/

#ifndef FREESCI_PRIMARY_RESOURCE_H_
#define FREESCI_PRIMARY_RESOURCE_H_

/*#define _SCI_RESOURCE_DEBUG */
/*#define _SCI_DECOMPRESS_DEBUG*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#ifdef WITH_DMALLOC
#  include <dmalloc.h>
#endif

#include <scitypes.h>

#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif

#ifdef _WIN32
#  include <io.h>
#  undef inline /* just to be sure it is not defined */
#  define inline __inline
#  define strcasecmp stricmp
#  define strncasecmp strnicmp
#  define vsnprintf _vsnprintf
#else /* !_WIN32 */
#  define DLLEXTERN
#endif /* !_WIN32 */

#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#ifdef _DOS
#  include <sci_dos.h>
#endif
#ifdef HAVE_LIMITS_H
#  include <limits.h>
#endif


#ifdef _MSC_VER
#  ifdef FREESCI_EXPORTS
#    define DLLEXTERN
#  else
#    define DLLEXTERN __declspec(dllimport)
#endif
#else
#  define DLLEXTERN
#endif

#if _MSC_VER || _DOS
#  define G_DIR_SEPARATOR_S "\\"
#else
#  define G_DIR_SEPARATOR_S "/"
#endif

#ifndef MIN
#  define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#endif
#ifndef MAX
#  define MAX(a, b)  (((a) > (b)) ? (a) : (b))
#endif /* HP-UX defines both */


#define GUINT16_SWAP_LE_BE_CONSTANT(val) ((((val) & 0x00ff) << 8) | (((val) & 0xff00) >> 8))

#define GUINT32_SWAP_LE_BE_CONSTANT(val)  ( \
                                             (((val) & 0xff000000) >> 24) \
                                           | (((val) & 0x00ff0000) >> 8) \
                                           | (((val) & 0x0000ff00) << 8) \
                                           | (((val) & 0x000000ff) << 24))

#define SCI_MAX_RESOURCE_SIZE 0x0400000
/* The maximum allowed size for a compressed or decompressed resource */


typedef guint8 byte;
typedef guint16 word;


extern DLLEXTERN int sci_version;
/* The version we are currently working with */

extern DLLEXTERN int max_resource;

#define MAX_HOMEDIR_SIZE 255


/**** FUNCTION DECLARATIONS ****/

#ifdef _WIN32
#    define scimkdir(arg1,arg2) mkdir(arg1)
#else
#    define scimkdir(arg1,arg2) mkdir(arg1,arg2)
#endif

#ifndef _GET_INT_16
#define _GET_INT_16

static inline gint16
getInt16(byte *d)
{
  return *d | (d[1] << 8);
}

#define getUInt16(d) (guint16)(getInt16(d))
#endif
/* Turns a little endian 16 bit value into a machine-dependant 16 bit value
** Parameters: d: Pointer to the memory position from which to read
** Returns   : (gint16) The (possibly converted) 16 bit value
** getUInt16 returns the int unsigned.
*/


void *
_XALLOC(size_t size, char *file, int line, char *funct);
#ifdef __GNUC__
#define xalloc(size) \
        _XALLOC((size), __FILE__, __LINE__, __PRETTY_FUNCTION__)
#else /* !__GNUC__ */
#define xalloc(size) \
        _XALLOC((size), __FILE__, __LINE__, "")
#endif /* !__GNUC__ */
/* Tries to allocate memory, prints an error message if not successful.
** Parameters: size: Number of bytes to allocate
** Returns   : (void *) The address of the allocated memory block
*/

#define SCI_MEMTEST memtest(__FILE__ ": line %d", __LINE__)

int
memtest(char *location, ...);
/* Allocates, manipulates, and frees some memory
** Parameters: (char *) location,... : The location to print
** Returns   : (int) 0
** This function calls malloc(), free(), and memfrob() or memset()
** to provocate segmentation faults caused by dynamic allocation bugs
** in previous parts of the code.
*/

void
sci_gettime(int *seconds, int *useconds);
/* Calculates the current time in seconds and microseconds
** Parameters: (int *) seconds: Pointer to the variable the seconds part of the
**                              current time will be stored in
**             (int *) useconds: Pointer to the variable the microseconds part
**                               of the current time will be stored in
** Returns   : (void)
** The resulting values must be relative to an arbitrary fixed point in time
** (typically 01/01/1970 on *NIX systems).
*/

void
sci_get_current_time(GTimeVal *val);
/* GTimeVal version of sci_gettime()
** Parameters: (GTimeVal *) val: Pointer to the structure the values will be stored in
** Returns   : (void)
*/

void
sci_init_dir(sci_dir_t *dirent);
/* Initializes an sci directory search structure
** Parameters: (sci_dir_t *) dirent: The entity to initialize
** Returns   : (void)
** The entity is initialized to "empty" values, meaning that it can be
** used in subsequent sci_find_first/sci_find_next constructs. In no
** event should this function be used upon a structure which has been
** subjected to any of the other dirent calls.
*/

char *
sci_find_first(sci_dir_t *dirent, char *mask);
/* Finds the first file matching the specified file mask
** Parameters: (sci_dir_t *) dirent: Pointer to an unused dirent structure
**             (char *) mask: File mask to apply
** Returns   : (char *) Name of the first matching file found, or NULL
*/

char *
sci_find_next(sci_dir_t *dirent);
/* Finds the next file specified by an sci_dir initialized by sci_find_first()
** Parameters: (sci_dir_t *) dirent: Pointer to SCI dir entity
** Returns   : (char *) Name of the next matching file, or NULL
*/

void
sci_finish_find(sci_dir_t *dirent);
/* Completes an 'sci_find_first/next' procedure
** Parameters: (sci_dir_t *) dirent: Pointer to the dirent used
** Returns   : (void)
** In the operation sequences
**   sci_init_dir(x); sci_finish_find(x);
** and
**   sci_finish_find(x); sci_finish_find(x);
** the second operation is guaranteed to be a no-op.
*/

int
sciprintf(char *fmt, ...);
#define gfxprintf sciprintf
/* Prints a string to the console stack
** Parameters: fmt: a printf-style format string
**             ...: Additional parameters as defined in fmt
** Returns   : (void)
** Implementation is in src/scicore/console.c
*/

/* The following was originally based on glib.h code, which was
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
 */
#if defined (__GNUC__) && __GNUC__ >= 2
#  if defined (__i386__)
#    define BREAKPOINT()          {__asm__ __volatile__ ("int $03"); }
#  elif defined(__alpha__)
#    define BREAKPOINT()          {__asm__ __volatile__ ("bpt"); }
#  endif /* !__i386__ && !__alpha__ */
#elif defined (_MSC_VER)
#  if defined (_M_IX86)
#    define BREAKPOINT()          { __asm { int 03 } }
#  elif defined(_M_ALPHA)
#    define BREAKPOINT()          { __asm { bpt } }
#  endif /* !_M_IX86 && !_M_ALPHA */
#endif
#ifndef BREAKPOINT
#  define BREAKPOINT() { fprintf(stderr, "Missed breakpoint in %s, line %d\n", __FILE__, __LINE__); exit(1); }
#endif  /* !BREAKPOINT() */

#define WARNING(foo) {char i; i = 500;}

#endif






