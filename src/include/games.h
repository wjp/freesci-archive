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

#ifndef NEED_SCI_VERSIONS
#  error "You shouldn't be including this header file."
#endif

#include <versions.h>

typedef struct _sci_game {
  int id; /* currently CRC of resource.001 */
  sci_version_t version;
  char *name;
} sci_game_t;

sci_game_t sci_games[] = {
	{ 0x94EA377B, SCI_VERSION(0,000,685), "CB1" },
	{ 0xFD9EE7BD, SCI_VERSION(0,000,685), "Camelot" },
	{ 0x2829987F, SCI_VERSION(0,000,685), "Camelot" },
	{ 0x980CEAD3, SCI_VERSION(0,000,629), "Demo Quest" },
	{ 0x3DB972CA, SCI_VERSION(0,000,572), "Hoyle 2" },
	{ 0xC0B37651, SCI_VERSION(0,000,685), "Iceman" },
	{ 0xDABA6B8A, SCI_VERSION(0,000,999), "KQ1" }, /* S.old.010 */
	{ 0x270E37F3, SCI_VERSION(0,000,274), "KQ4" },
	{ 0x685F1205, SCI_VERSION(0,000,502), "KQ4" },
	{ 0x13DD3CD2, SCI_VERSION(0,000,343), "LSL2" },
	{ 0x0C848403, SCI_VERSION(0,000,409), "LSL2" },
	{ 0xC48FE83A, SCI_VERSION(0,000,572), "LSL3" },
	{ 0xC14E3A2A, SCI_VERSION(0,000,395), "PQ2" },
	{ 0x4BD66036, SCI_VERSION(0,000,490), "PQ2" },
	{ 0x7132D6D8, SCI_VERSION(0,000,629), "QfG1" },
	{ 0xF8F4913F, SCI_VERSION(0,000,685), "SQ3" },
	{ 0x34FBC324, SCI_VERSION(0,000,999), "SQ3/DE" }, /* S.old.114 */
	{ 0xE4A3234D, SCI_VERSION(0,000,506), "Fun Seekers Guide v1.02"},
	{ 0x85AFE241, SCI_VERSION(0,000,519), "Hoyle 1 v1.000.104"},
	{ 0xE0E070C3, SCI_VERSION(0,000,572), "Hoyle 2 v1.000.011"},
	{ 0xD0B8794E, SCI_VERSION(0,000,668), "Iceman v1.023"},
	{ 0x94EA377B, SCI_VERSION(0,000,631), "The Colonel's Bequest v1.000.046"},
	{ 0x28543FDF, SCI_VERSION(0,000,453), "Astro Chicken"},
	{ 0x31F46F7D, SCI_VERSION(0,000,453), "Space Quest III v1.0V int"},
	{ 0xAA2C94B9, SCI_VERSION(0,000,685), "Mixed-Up Mother Goose v1.011 Int.#8.2.90"},

	/* Undetermined Amiga versions: */
/*	{ 0x8AE5F854, SCI_VERSION(), "ARTHUR" }, */
/*	{ 0x9FB7015B, SCI_VERSION(), "CB1" }, */
/*	{ 0x560CEDD5, SCI_VERSION(), "iceMan" }, */

	{ 0, 0, NULL } /* terminator */
};

#endif /* _SCI_GAMES_H_ */
