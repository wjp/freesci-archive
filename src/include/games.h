/***************************************************************************
 games.h Copyright (C) 2002 Solomon Peachy

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

    Solomon Peachy [pizza@shaftnet.org]

***************************************************************************/

/* Game identification */

#ifndef _SCI_GAMES_H_
#define _SCI_GAMES_H_

#include <versions.h>

typedef struct _sci_game {
  int id; /* currently CRC of resource.001 */
  sci_version_t version;
  char *name;
} sci_game_t;

#define SCI_GAMES_COUNT 23

sci_game_t sci_games[] = {
  { 0x01684e20, SCI_VERSION(0,000,685), "ARTHUR" },
  { 0x00a9a8d0, SCI_VERSION(0,000,631), "CB1" },
  { 0x010a46ae, SCI_VERSION(0,000,519), "cardGames" },
  { 0x009f17fb, SCI_VERSION(0,000,572), "solitare" },
  { 0x00d441c7, SCI_VERSION(0,000,668), "iceMan" },
  { 0x02bd7be8, SCI_VERSION(0,000,502), "KQ4" },
  { 0x013249ee, SCI_VERSION(0,000,274), "KQ4" },
  { 0x00e8b382, SCI_VERSION(0,000,409), "LSL2" },
  { 0x03082081, SCI_VERSION(0,000,572), "LSL3" },
  { 0x00e2e755, SCI_VERSION(0,000,572), "LSL3" },
  { 0x032e674b, SCI_VERSION(0,000,395), "PQ2" },
  { 0x032eaeff, SCI_VERSION(0,000,490), "PQ2" },
  { 0x009b0cd2, SCI_VERSION(0,000,566), "HQ" },
  { 0x02e5c05f, SCI_VERSION(0,000,629), "HQ" },
  { 0x0117a04f, SCI_VERSION(0,000,453), "SQ3" },
  { 0x033bc19e, SCI_VERSION(0,000,453), "SQ3" }, /* v1.0 */
  { 0x00cb0bf2, SCI_VERSION(0,000,453), "SQ3-Astro" },
  { 0x05a8efaf, SCI_VERSION(1,000,172), "Trial" }, /* unknown as yet v1.102 */
  { 0x05a71cde, SCI_VERSION(1,000,172), "Trial" }, /* unknown as yet */
  { 0x0223e42c, SCI_VERSION(1,000,172), "Trial" }, /* also 1.000.072 v1.105 */
  { 0x05a8efaf, SCI_VERSION(1,000,172), "Trial" }, /* unknown as yet */
  { 0x014291d1, SCI_VERSION(0,000,999), "KQ1E" }, /* S.old.010 1.000.051 */
  { 0x03aff799, SCI_VERSION(0,000,999), "KQ1E" }, /* S.old.010 */
};

#endif /* _SCI_GAMES_H_ */
