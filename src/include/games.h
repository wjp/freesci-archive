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
  int id; /* currently sciv/sierra.exe size */
  sci_version_t version;
  char *name;
} sci_game_t;

#define SCI_GAMES_COUNT 13

sci_game_t sci_games[] = {
  { 44443, SCI_VERSION(0,000,685), "ARTHUR" },
  { 76419, SCI_VERSION(0,000,631), "CB1" },
  { 75107, SCI_VERSION(0,000,519), "cardGames" },
  { 76499, SCI_VERSION(0,000,668), "iceMan" },
  { 75075, SCI_VERSION(0,000,502), "KQ4" },  /* re-release */
  { 73407, SCI_VERSION(0,000,274), "KQ4" },  /* original release */
  { 73697, SCI_VERSION(0,000,409), "LSL2" },
  { 75203, SCI_VERSION(0,000,572), "LSL3/solitare" },
  { 74915, SCI_VERSION(0,000,490), "PQ2" },  /* re-release */
  { 73375, SCI_VERSION(0,000,395), "PQ2" },  /* original release */
  { 75027, SCI_VERSION(0,000,566), "HQ" }, /* original release */
  { 76179, SCI_VERSION(0,000,629), "HQ" }, /* re-release */
  { 74342, SCI_VERSION(0,000,453), "SQ3" }
};

#endif /* _SCI_GAMES_H_ */
