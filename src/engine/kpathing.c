/***************************************************************************
 kpathing.c Copyright (C) 2002 Lars Skovlund


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

    Lars Skovlund (LS) [lskovlun@image.dk]

***************************************************************************/

#include <engine.h>

#define POLY_LAST_POINT 0x7777
#define POLY_POINT_SIZE 4

#define POLY_GET_POINT(p,i,x,y) x = getInt16(p+i*POLY_POINT_SIZE); \
                                y = getInt16(p+2+i*POLY_POINT_SIZE);
#define POLY_SET_POINT(p,i,x,y) putInt16(p+i*POLY_POINT_SIZE, x); \
                                putInt16(p+2+i*POLY_POINT_SIZE, y);

/* 

Polygon data format:

The poly_list parameter to the pathing part of this call points to a
regular SCI list structure (see klists.c). Each element of the list is
an object of class Polygon, with selectors points, size, and type
being relevant to the algorithm. Points is a linear list of coordinate
pairs _without_ a termination marker. Type indicates the polygon type,
as detailed in US patent 5,287,446 (constants given below).

The polygon parameter to the query part is a single object of class
Polygon.

The inside-polygon query simply returns a boolean value. The pather
returns a list of points, which is to be allocated implicitly on the
heap and its address placed in s->acc (which happens through output
ATM). The caller is responsible for freeing the memory. Remember the
termination, as shown in the trivial case!

The polygons can _not_ be expected to be convex, but they can be
expected to be closed and not to self-intersect. 

[CR]: The three types are:
 - completely blocking
 - blocking unless the start or end point is within the polygon
 - direction inducing: blocking on edges except for vertices (leaving
             (enough room to pass through)
*/

/* SCI-defined polygon types */
#define POLY_TOTAL_ACCESS 0
#define POLY_NEAREST_ACCESS 1
#define POLY_BARRED_ACCESS 2
#define POLY_CONTAINED_ACCESS 3

reg_t
kAvoidPath(state_t *s, int funct_nr, int argc, reg_t *argv)
{
	int startx = SKPV(0);
	int starty = SKPV(1);

	switch (argc) {

	case 3 :
	{
		reg_t polygon = argv[2];

		SCIkwarn(SCIkWARNING, "Fix me: Inside-polygon test needed!\n");
		return NULL_REG; /* Is (startx,starty) inside the polygon? */
		break;
	}
	case 6 :
	case 7 :
	{
		int endx = SKPV(2);
		int endy = SKPV(3);
		reg_t poly_list = argv[4];
		int poly_list_size = UKPV(5);
		int unknown = UKPV_OR_ALT(6, 1);
		reg_t output;
		byte *oref;

		if (poly_list.segment) {
			SCIkwarn(SCIkWARNING, "Fix me: Polygon avoidance algorithm needed!\n");
		}

		/* Generate a straight line. */
		oref = s->seg_manager.alloc_dynmem(&s->seg_manager, POLY_POINT_SIZE*3,
						   "AvoidPath polyline", &output);

		POLY_SET_POINT(oref, 0, startx, starty);
		POLY_SET_POINT(oref, 1, endx, endy);
		POLY_SET_POINT(oref, 2, POLY_LAST_POINT, POLY_LAST_POINT);

		return output; /* Memory is freed by explicit calls to Memory */
	}

	default:
		SCIkwarn(SCIkWARNING, "Unknown AvoidPath subfunction %d\n",
			 argc);
		return NULL_REG;
		break;
	}
}
