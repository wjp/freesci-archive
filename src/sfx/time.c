/***************************************************************************
 time.c  Copyright (C) 2002 Christoph Reichenbach


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

#include <sfx_time.h>

sfx_timestamp_t
sfx_new_timestamp(long secs, long usecs, int sample_rate)
{
	sfx_timestamp_t r;
	r.secs = secs;
	r.usecs = usecs;
	r.sample_rate = sample_rate;
	r.sample_offset = 0;
}


sfx_timestamp_t
sfx_timestamp_add(sfx_timestamp_t timestamp, int samples)
{
	timestamp.sample_offset += samples;

	timestamp.secs += (timestamp.sample_offset / timestamp.sample_rate);
	timestamp.sample_offset %= timestamp.sample_rate;

	return timestamp;
}

void
sfx_timestamp_gettime(sfx_timestamp_t *timestamp, long *secs, long *usecs)
{
	long ust = timestamp->usecs;
	/* On 64 bit machines, we can do an accurate computation */
#if (SIZEOF_LONG >= 8)
	ust += (timestamp->sample_offset) / (timestamp->sample_rate);
#else
	ust += (timestamp->sample_offset * 1000) / (timestamp->sample_rate / 1000);
#endif

	if (ust > 1000000) {
		ust -= 1000000;
		*secs = timestamp->secs + 1;
	} else
		*secs = timestamp->secs;

	*usecs = ust;
}



#endif /* !defined(_SFX_TIMER_H_) */
