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

static const char *SCIk_Debug_Names[] = {
  "Stub",
  "Nodes",
  "Graphics",
  "String Handling",
  "Memory management",
  "Generic function checks",
};
/* The various debug areas */

#define SCIkERROR_NR -2
#define SCIkWARNING_NR -1
#define SCIkSTUB_NR 0
#define SCIkFUNCCHK_NR 5

#define SCIkERROR    s, __LINE__, SCIkERROR_NR
#define SCIkWARNING  s, __LINE__, SCIkWARNING_NR
#define SCIkSTUB     s, __LINE__, SCIkSTUB_NR
#define SCIkNODES    s, __LINE__, 1
#define SCIkGRAPHICS s, __LINE__, 2
#define SCIkSTRINGS  s, __LINE__, 3
#define SCIkMEM      s, __LINE__, 4
#define SCIkFUNCCHK  s, __LINE__, SCIkFUNCCHK_NR
