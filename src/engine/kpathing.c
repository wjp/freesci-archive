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
#include <kernel_compat.h>

#define POLY_LAST_POINT 0x7777
#define POLY_POINT_SIZE 4

#define POLY_GET_POINT(p,i,x,y) x=GET_HEAP(p+i*POLY_POINT_SIZE); \
                                y=GET_HEAP(p+2+i*POLY_POINT_SIZE);
#define POLY_SET_POINT(p,i,x,y) PUT_HEAP(p+i*POLY_POINT_SIZE, x); \
                                PUT_HEAP(p+2+i*POLY_POINT_SIZE, y);

/* 

Polygon data format:

The poly_list parameter to the pathing part of this call points to a
regular SCI list structure (see klists.c). Each element of the list is
an object of class Polygon, with selectors points, size, and type
being relevant to the algorithm. Points is a linear list of coordinate
pairs _without_ a termination marker. Type indicates the polygon type,
as detailed in US patent 5,287,446 (exact mapping of the constants to
the listed types still unknown).

The polygon parameter to the query part is a single object of class
Polygon.

The inside-polygon query simply returns a boolean value. The pather
returns a list of points, which is to be allocated implicitly on the
heap and its address placed in s->acc (which happens through output
ATM). The caller is responsible for freeing the memory. Remember the
termination, as shown in the trivial case!

The polygons can _not_ be expected to be convex, but they can be
expected to be closed and not to self-intersect. 

*/

void
kAvoidPath(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
	int startx = UPARAM(1);
	int starty = UPARAM(2);

	switch (argc) {
	case 3 :
	{
		heap_ptr polygon = UPARAM(3);

		s->acc = 0; /* Is (startx,starty) inside the polygon? */
		break;
	}		
	case 6 :
	case 7 :
	{
		int endx = UPARAM(3);
		int endy = UPARAM(4);
		heap_ptr poly_list = UPARAM(5);
		heap_ptr poly_list_size = UPARAM(6);
		int unknown = UPARAM_OR_ALT(7, 1);
		heap_ptr output;

		if (!poly_list)
		{
			/* Generate a straight line. */
			output=heap_allocate(s->_heap, POLY_POINT_SIZE*3);

			POLY_SET_POINT(output, 0, startx, starty);
			POLY_SET_POINT(output, 1, endx, endy);
			POLY_SET_POINT(output, 2, POLY_LAST_POINT, POLY_LAST_POINT);
		} else
		{
			SCIkwarn(SCIkSTUB, "The polygon avoidance algorithm is not "
			                   "implemented for non-trivial cases yet.\n"
				           "A crash is imminent.\n");
			output = 0; /* This will fail quickly and let
				       people know when the need
				       arises */ 
		}

		s->acc=output;
		break;
	}
	}
}
