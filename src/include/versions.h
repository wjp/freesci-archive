/***************************************************************************
 versions.h Copyright (C) 1999 Christoph Reichenbach, TU Darmstadt


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
/* Versions management */

#ifndef _SCI_VERSIONS_H_
#define _SCI_VERSIONS_H_


struct _state;

#define SCI_VERSION(_major_, _minor_, _patchlevel_) (((_major_)<<20) | ((_minor_)<<10) | _patchlevel_)
/* This allows version numbers to be compared directly */

/* Version number guide:
** - Always use the version number of the first known version to have a special feature.
** - Don't assume that special feature changes are linked just because they appeared to change
**   simultaneously.
** - Put all the magic version numbers here, into THIS file.
** - "FTU" means "First To Use"
*/

#define SCI_VERSION_DEFAULT_SCI0 SCI_VERSION(0,000,685)
/* AFAIK this is the last published SCI0 version */
#define SCI_VERSION_DEFAULT_SCI01 SCI_VERSION(1,000,72)
/* The version used by my implementation of QfG2 */


#define SCI_VERSION_FTU_CENTERED_TEXT_AS_DEFAULT SCI_VERSION(0,000,629)
/* Last version known not to do this: 0.000.502 */

#define SCI_VERSION_FTU_NEWER_DRAWPIC_PARAMETERS SCI_VERSION(0,000,502)
/* Last version known not to do this: 0.000.435
** Old SCI versions used to interpret the third DrawPic() parameter inversely,
** with the opposite default value (obviously)
*/



typedef int sci_version_t;




#endif /* !_SCI_VERSIONS_H_ */
