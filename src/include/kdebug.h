/***************************************************************************
 kdebug.h Copyright (C) 1999 Christoph Reichenbach, TU Darmstadt


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

    Christoph Reichenbach (CJR) [jameson@linuxgames.com]

***************************************************************************/
/* Kernel debug defines */

#ifndef _SCI_KDEBUG_H_
#define _SCI_KDEBUG_H_

struct _state;
#define SCIk_DEBUG_MODES 13

static const char *SCIk_Debug_Names[SCIk_DEBUG_MODES] = {
  "Stubs",
  "Lists and nodes",
  "Graphics",
  "Character handling",
  "Memory management",
  "Function parameter checks",
  "Bresenham algorithms",
  "Audio subsystem",
  "System graphics driver",
  "Base setter results",
  "Parser",
  "Menu handling",
  "Said specs"
};
/* The various debug areas */

#define SCIkERROR_NR -2
#define SCIkWARNING_NR -1
#define SCIkSTUB_NR 0
#define SCIkFUNCCHK_NR 5
#define SCIkSOUNDCHK_NR 7
#define SCIkGFXDRIVER_NR 8
#define SCIkBASESETTER_NR 9
#define SCIkPARSER_NR 10

#define SCIkERROR      s, __FILE__, __LINE__, SCIkERROR_NR
#define SCIkWARNING    s, __FILE__, __LINE__, SCIkWARNING_NR
#define SCIkSTUB       s, __FILE__, __LINE__, SCIkSTUB_NR
#define SCIkNODES      s, __FILE__, __LINE__, 1
#define SCIkGRAPHICS   s, __FILE__, __LINE__, 2
#define SCIkSTRINGS    s, __FILE__, __LINE__, 3
#define SCIkMEM        s, __FILE__, __LINE__, 4
#define SCIkFUNCCHK    s, __FILE__, __LINE__, SCIkFUNCCHK_NR
#define SCIkBRESEN     s, __FILE__, __LINE__, 6
#define SCIkSOUND      s, __FILE__, __LINE__, SCIkSOUNDCHK_NR
#define SCIkGFXDRIVER  s, __FILE__, __LINE__, SCIkGFXDRIVER_NR
#define SCIkBASESETTER s, __FILE__, __LINE__, SCIkBASESETTER_NR
#define SCIkPARSER     s, __FILE__, __LINE__, SCIkPARSER_NR
#define SCIkMENU       s, __FILE__, __LINE__, 11
#define SCIkSAID       s, __FILE__, __LINE__, 12


#define SCI_KERNEL_DEBUG

#ifdef SCI_KERNEL_DEBUG

#ifdef __GNUC__

#define SCIkdebug(arguments...) _SCIGNUkdebug(__PRETTY_FUNCTION__,  ## arguments)

#else /* !__GNUC__ */

#define SCIkdebug _SCIkdebug

#endif /* !__GNUC__ */

#else /* !SCI_KERNEL_DEBUG */

#define SCIkdebug 1? (void)0 : _SCIkdebug

#endif /* !SCI_KERNEL_DEBUG */



#ifdef __GNUC__

#define SCIkwarn(arguments...) _SCIGNUkdebug(__PRETTY_FUNCTION__, ## arguments)

#else /* !__GNUC__ */

#define SCIkwarn _SCIkwarn

#endif /* !__GNUC__ */

/* Internal functions */
void
_SCIkwarn(struct _state *s, char *file, int line, int area, char *format, ...);
void
_SCIkdebug(struct _state *s, char *file, int line, int area, char *format, ...);
void
_SCIGNUkdebug(char *funcname, struct _state *s, char *file, int line, int area, char *format, ...);

/* If mode=1, enables debugging for specified areas. If mode=0, disables
** debugging for specified areas.
** Valid area characters: ulgcmfbad
*/

void
set_debug_mode (struct _state *s, int mode, char *areas);

extern int sci_debug_flags;

/* Debug flags */
#define _DEBUG_FLAG_LOGGING 1 /* Log each command executed */
#define _DEBUG_FLAG_BREAK_ON_WARNINGS 2 /* Break on warnings */


#endif
