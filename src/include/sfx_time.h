/***************************************************************************
 sfx_time.h  Copyright (C) 2002 Christoph Reichenbach


 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public Licence as
 published by the Free Software Foundaton; either version 2 of the
 Licence, or (at your option) any later version.

 It is distributed in the hope that it will be useful, but
 WITHOUT ANY WARRANTY; without even the implied warranty of
 merchantibility or fitness for a particular purpose. See the
 GNU General Public Licence for more details.

 You should have received a copy of the GNU General Public Licence
 along with this program; see the file COPYING. If not, write to
 the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 Boston, MA 02111-1307, USA.


 Please contact the maintainer for any program-related bug reports or
 inquiries.

 Current Maintainer:

    Christoph Reichenbach (CR) <jameson@linuxgames.com>

***************************************************************************/

#ifndef _SFX_TIMER_H_
#define _SFX_TIMER_H_

typedef struct {
	long secs;
	long usecs;
	int sample_rate;
	int sample_offset;
	/* Total time: secs + usecs + sample_offset/sample_rate */
} sfx_timestamp_t;


sfx_timestamp_t
sfx_new_timestamp(long secs, long usecs, int sample_rate);
/* Creates a new mutable timestamp
** Parameters: (long x long) (secs, usecs): Initial timestamp
**             (int) sample_rate: Sample rate, for increasing the time stamp
*/

sfx_timestamp_t
sfx_timestamp_add(sfx_timestamp_t timestamp, int samples);
/* Adds a number of samples to a timestamp
** Parameters: (sfx_timestampt_t *) timestamp: The timestamp to update
**             (int) samples: Number of samples to add
** Returns   : (sfx_timestamp_t) The increased timestamp
*/

void
sfx_timestamp_gettime(sfx_timestamp_t *timestamp, long *secs, long *usecs);
/* Determines the time described by a given timestamp
** Parameters: (sfx_timestamp_t *) timestamp: Timestamp to read from
** Returns   : (int * x int *) (secs, usecs): Seconds and microseconds since
**                                            the epoch described there
*/



#endif /* !defined(_SFX_TIMER_H_) */
