/***************************************************************************
 versions.h Copyright (C) 1999,2000,01 Christoph Reichenbach


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

#define SCI_VERSION_MAJOR(_version_) ((_version_) >> 20)
#define SCI_VERSION_MINOR(_version_) (((_version_) >> 10) & 0x3ff)
#define SCI_VERSION_PATCHLEVEL(_version_) ((_version_) & 0x3ff)

/* Version number guide:
** - Always use the version number of the first known version to have a special feature.
** - Don't assume that special feature changes are linked just because they appeared to change
**   simultaneously.
** - Put all the magic version numbers here, into THIS file.
** - "FTU" means "First To Use"
*/

#define SCI_VERSION_LAST_SCI0 SCI_VERSION(0,000,685)

#define SCI_VERSION_DEFAULT_SCI0 SCI_VERSION_LAST_SCI0
/* AFAIK this is the last published SCI0 version */
#define SCI_VERSION_DEFAULT_SCI01 SCI_VERSION(1,000,72)
/* The version used by my implementation of QfG2 */


#define SCI_VERSION_FTU_CENTERED_TEXT_AS_DEFAULT SCI_VERSION(0,000,629)
/* Last version known not to do this: 0.000.502 */

#define SCI_VERSION_FTU_NEW_GETTIME SCI_VERSION(0,000,629)

#define SCI_VERSION_FTU_NEWER_DRAWPIC_PARAMETERS SCI_VERSION(0,000,502)
/* Last version known not to do this: 0.000.435
** Old SCI versions used to interpret the third DrawPic() parameter inversely,
** with the opposite default value (obviously)
*/

#define SCI_VERSION_LTU_PRIORITY_OB1 SCI_VERSION(0,000,590)
/* First version not to have this: 0.000.602
** Old versions change the priority map one pixel earlier
*/

#define SCI_VERSION_FTU_NEW_SCRIPT_HEADER SCI_VERSION(0,000,397)
/* Last version known not to do this: 0.000.343
** Old SCI versions used two word header for script blocks (first word equal
** to 0x82, meaning of the second one unknown). New SCI versions used one
** word header.
*/

#define SCI_VERSION_LTU_BASE_OB1 SCI_VERSION(0,000,256)
/* First version version known not to have this bug: ?
** When doing CanBeHere(), augment y offset by 1
*/

#define SCI_VERSION_FTU_2ND_ANGLES SCI_VERSION(0,000,572)
/* Last version known not to use this: ?
** Earlier versions assign 120 degrees to left & right , and 60 to up and down.
** Later versions use an even 90 degree distribution.
*/

#define SCI_VERSION_RESUME_SUSPENDED_SONG SCI_VERSION(0,000,490)
/* First version (PQ2-new) known to use the different song resumption 
   mechanism -- When a new song is initialized, we store its state and
   resume it when the new one finishes.  Older versions completely
   clobbered the old songs.
*/

typedef int sci_version_t;

struct _state;

void
version_require_earlier_than(struct _state *s, sci_version_t version);
/* Function used in autodetection
** Parameters: (state_t *) s: state_t containing the version
**             (sci_version_t) version: The version that we're earlier than
*/

void
version_require_later_than(struct _state *s, sci_version_t version);
/* Function used in autodetection (read this function "version_require_later_than_or_equal_to")
** Parameters: (state_t *) s: state_t containing the version
**             (sci_version_t) version: The version that we're later than
*/

sci_version_t
version_parse(char *vn);
/* Parse a string containing an SCI version number
** Parameters: (char *) vn: The string to parse
** Returns   : (sci_version_t): The resulting version number
*/

#endif /* !_SCI_VERSIONS_H_ */
