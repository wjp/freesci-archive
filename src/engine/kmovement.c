/***************************************************************************
 kmovement.c Copyright (C) 2001 Christoph Reichenbach


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

    Christoph Reichenbach (CR) <jameson@linuxgames.com>

***************************************************************************/

#include <sciresource.h>
#include <engine.h>

void
kSetJump(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
	heap_ptr object = UPARAM(0);
	int dx = PARAM(1);
	int dy = PARAM(2);
	int gy = PARAM(3);
	int t1 = 1;
	int t2;
	int x = 0;
	int y = 0;


	if ((dx)&&(abs(dy)>dx)) t1=(2*abs(dy))/dx;

	SCIkdebug(SCIkBRESEN, "t1: %d\n", t1);

	t1--;

	do {
		t1++;
		t2 = t1 * abs(dx) + dy;
	} while (abs(2 * t2) < abs(dx));

	SCIkdebug(SCIkBRESEN, "t1: %d, t2: %d\n", t1, t2);

	if (t2) {
		x = (int)sqrt(abs((gy * dx * dx) / (2 * t2)));
		if (t2 *dx < 0)
			x = -x;
	}
	y = abs(t1 * y);
	if (dx >= 0)
		y=-y;
	if ((dy < 0) && (!y))
		y = -(int)sqrt(-2 * gy * dy);

	SCIkdebug(SCIkBRESEN, "SetJump for object at %x", object);
	SCIkdebug(SCIkBRESEN, "xStep: %d, yStep: %d\n", x, y);

	PUT_SELECTOR(object, xStep, x);
	PUT_SELECTOR(object, yStep, y);

}


#define _K_BRESEN_AXIS_X 0
#define _K_BRESEN_AXIS_Y 1

void
initialize_bresen(state_t *s, int funct_nr, int argc, reg_t *argv, reg_t mover, int step_factor,
		  int deltax, int deltay)
{
	reg_t client = GET_SEL32(mover, client);
	int stepx = GET_SEL32SV(client, xStep) * step_factor;
	int stepy = GET_SEL32SV(client, yStep) * step_factor;
	int numsteps_x = stepx? (abs(deltax) + stepx-1) / stepx : 0;
	int numsteps_y = stepy? (abs(deltay) + stepy-1) / stepy : 0;
	int bdi, i1;
	int numsteps;
	int deltax_step;
	int deltay_step;

	if (numsteps_x > numsteps_y) {
		numsteps = numsteps_x;
		deltax_step = (deltax < 0)? -stepx : stepx;
		deltay_step = numsteps? deltay / numsteps : deltay;
	} else { /* numsteps_x <= numsteps_y */
		numsteps = numsteps_y;
		deltay_step = (deltay < 0)? -stepy : stepy;
		deltax_step = numsteps? deltax / numsteps : deltax;
	}

	/*  if (abs(deltax) > abs(deltay)) {*/ /* Bresenham on y */
	if (numsteps_y < numsteps_x) {

		PUT_SEL32V(mover, b_xAxis, _K_BRESEN_AXIS_Y);
		PUT_SEL32V(mover, b_incr, (deltay < 0)? -1 : 1);
		/*
		i1 = 2 * (abs(deltay) - abs(deltay_step * numsteps)) * abs(deltax_step);
		bdi = -abs(deltax);
		*/
		i1 = 2*(abs(deltay) - abs(deltay_step * (numsteps - 1))) * abs(deltax_step);
		bdi = -abs(deltax);

	} else { /* Bresenham on x */

		PUT_SEL32V(mover, b_xAxis, _K_BRESEN_AXIS_X);
		PUT_SEL32V(mover, b_incr, (deltax < 0)? -1 : 1);
		/*
		i1= 2 * (abs(deltax) - abs(deltax_step * numsteps)) * abs(deltay_step);
		bdi = -abs(deltay);
		*/
		i1 = 2*(abs(deltax) - abs(deltax_step * (numsteps - 1))) * abs(deltay_step);
		bdi = -abs(deltay);

	}

	PUT_SEL32V(mover, dx, deltax_step);
	PUT_SEL32V(mover, dy, deltay_step);

	SCIkdebug(SCIkBRESEN, "Init bresen for mover "PREG": d=(%d,%d)\n", PRINT_REG(mover), deltax, deltay);
	SCIkdebug(SCIkBRESEN, "    steps=%d, mv=(%d, %d), i1= %d, i2=%d\n",
		  numsteps, deltax_step, deltay_step, i1, bdi*2);

/*	PUT_SEL32V(mover, b_movCnt, numsteps); *//* Needed for HQ1/Ogre? */
	PUT_SEL32V(mover, b_di, bdi);
	PUT_SEL32V(mover, b_i1, i1);
	PUT_SEL32V(mover, b_i2, bdi * 2);

}

reg_t
kInitBresen(state_t *s, int funct_nr, int argc, reg_t *argv)
{
	reg_t mover = argv[0];
	reg_t client = GET_SEL32(mover, client);

	int deltax = GET_SEL32SV(mover, x) - GET_SEL32SV(client, x);
	int deltay = GET_SEL32SV(mover, y) - GET_SEL32SV(client, y);

	initialize_bresen(s, funct_nr, argc, argv, mover, KP_UINT(KP_ALT(1, make_reg(0, 1))), deltax, deltay);

	return s->r_acc;
}


#define MOVING_ON_X (((axis == _K_BRESEN_AXIS_X)&&bi1) || dx)
#define MOVING_ON_Y (((axis == _K_BRESEN_AXIS_Y)&&bi1) || dy)

reg_t
kDoBresen(state_t *s, int funct_nr, int argc, reg_t *argv)
{
	reg_t mover = argv[0];
	reg_t client = GET_SEL32(mover, client);

	int x = GET_SEL32SV(client, x);
	int y = GET_SEL32SV(client, y);
	int oldx, oldy, destx, desty, dx, dy, bdi, bi1, bi2, movcnt, bdelta, axis;
	word signal = GET_SEL32V(client, signal);
	int completed = 0;

	if (SCI_VERSION_MAJOR(s->version)>0)
		signal&=~_K_VIEW_SIG_FLAG_HIT_OBSTACLE;

	PUT_SEL32(client, signal, make_reg(0, signal)); /* This is a NOP for SCI0 */
	oldx = x;
	oldy = y;
	destx = GET_SEL32SV(mover, x);
	desty = GET_SEL32SV(mover, y);
	dx = GET_SEL32SV(mover, dx);
	dy = GET_SEL32SV(mover, dy);
	bdi = GET_SEL32SV(mover, b_di);
	bi1 = GET_SEL32SV(mover, b_i1);
	bi2 = GET_SEL32SV(mover, b_i2);
	movcnt = GET_SEL32V(mover, b_movCnt);
	bdelta = GET_SEL32SV(mover, b_incr);
	axis = GET_SEL32SV(mover, b_xAxis);

	if ((bdi += bi1) > 0) {
		bdi += bi2;

		if (axis == _K_BRESEN_AXIS_X)
			dx += bdelta;
		else
			dy += bdelta;
	}

	PUT_SEL32V(mover, b_di, bdi);
/*	PUT_SELECTOR(mover, b_movCnt, movcnt - 1); *//* Needed for HQ1/Ogre? */

	x += dx;
	y += dy;


 	if ((MOVING_ON_X
	     && (((x < destx) && (oldx >= destx)) /* Moving left, exceeded? */
		 ||
		 ((x > destx) && (oldx <= destx)) /* Moving right, exceeded? */
		 ||
		 ((x == destx) && (abs(dx) > abs(dy))) /* Moving fast, reached? */
		 /* Treat this last case specially- when doing sub-pixel movements
		 ** on the other axis, we could still be far away from the destination  */
		 )
	     )
	    || (MOVING_ON_Y
		&& (((y < desty) && (oldy >= desty)) /* Moving upwards, exceeded? */
		    ||
		    ((y > desty) && (oldy <= desty)) /* Moving downwards, exceeded? */
		    ||
		    ((y == desty) && (abs(dy) >= abs(dx))) /* Moving fast, reached? */
		    )
		)
	    )
		/* Whew... in short: If we have reached or passed our target position */
		{
			x = destx;
			y = desty;
			completed = 1;

			SCIkdebug(SCIkBRESEN, "Finished mover "PREG"\n", PRINT_REG(mover));
		}


	PUT_SEL32V(client, x, x);
	PUT_SEL32V(client, y, y);

	SCIkdebug(SCIkBRESEN, "New data: (x,y)=(%d,%d), di=%d\n", x, y, bdi);

	invoke_selector(INV_SEL(client, canBeHere, 0), 0);

	if (!s->r_acc.offset) { /* Contains the return value */

		signal = GET_SEL32V(client, signal);

		PUT_SEL32V(client, x, oldx);
		PUT_SEL32V(client, y, oldy);

		PUT_SEL32V(client, signal, (signal | _K_VIEW_SIG_FLAG_HIT_OBSTACLE));

		SCIkdebug(SCIkBRESEN, "Finished mover "PREG" by collision\n", PRINT_REG(mover));
		completed = 1;
	}

	if (SCI_VERSION_MAJOR(s->version)>0)
		if (completed)
			invoke_selector(INV_SEL(mover, moveDone, 0), 0);

	return make_reg(0, completed);
}

extern void
_k_dirloop(heap_ptr obj, word angle, state_t *s, int funct_nr,
	   int argc, heap_ptr argp);
/* From kgraphics.c, used as alternative looper */

extern int
get_angle(int xrel, int yrel);
/* from kmath.c, used for calculating angles */


void
kDoAvoider(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
#warning "Fix DoAvoider() selector invocations"
#if 0
	heap_ptr avoider = UPARAM(0);
	heap_ptr client, looper, mover;
	int angle;
	int dx, dy;
	int destx, desty;


	s->acc = -1;

	if (!is_object(s, avoider)) {
		SCIkwarn(SCIkWARNING, "DoAvoider() where avoider %04x is not an object\n", avoider);
		return;
	}

	client = GET_SELECTOR(avoider, client);

	if (!is_object(s, client)) {
		SCIkwarn(SCIkWARNING, "DoAvoider() where client %04x is not an object\n", client);
		return;
	}

	looper = GET_SELECTOR(client, looper);

	mover = GET_SELECTOR(client, mover);

	if (!is_object(s, mover)) {
		if (mover) {
			SCIkwarn(SCIkWARNING, "DoAvoider() where mover %04x is not an object\n", mover);
		}
		return;
	}

	destx = GET_SELECTOR(mover, x);
	desty = GET_SELECTOR(mover, y);

	SCIkdebug(SCIkBRESEN, "Doing avoider %04x (dest=%d,%d)\n", avoider, destx, desty);

	if (!mover)
		return;

	if (invoke_selector(INV_SEL(mover, doit, 1) , 0)) {
		SCIkwarn(SCIkERROR, "Mover %04x of avoider %04x"
			 " doesn't have a doit() funcselector\n", mover, avoider);
		return;
	}

	mover = GET_SELECTOR(client, mover);
	if (!mover) /* Mover has been disposed? */
		return; /* Return gracefully. */

	if (invoke_selector(INV_SEL(client, isBlocked, 1) , 0)) {
		SCIkwarn(SCIkERROR, "Client %04x of avoider %04x doesn't"
			 " have an isBlocked() funcselector\n", client, avoider);
		return;
	}

	dx = destx - GET_SELECTOR(client, x);
	dy = desty - GET_SELECTOR(client, y);
	angle = get_angle(dx, dy);

	SCIkdebug(SCIkBRESEN, "Movement (%d,%d), angle %d is %s blocked\n",
		  dx, dy, angle, (s->acc)? "": "not");

	if (s->acc) { /* isBlocked() returned non-zero */
		int rotation = (rand() & 1)? 45 : (360-45); /* Clockwise/counterclockwise */
		int oldx = GET_SELECTOR(client, x);
		int oldy = GET_SELECTOR(client, y);
		int xstep = GET_SELECTOR(client, xStep);
		int ystep = GET_SELECTOR(client, yStep);
		int moves;

		SCIkdebug(SCIkBRESEN, " avoider %04x\n", avoider);

		for (moves = 0; moves < 8; moves++) {
			int move_x = (int) (sin(angle * PI / 180.0) * (xstep));
			int move_y = (int) (-cos(angle * PI / 180.0) * (ystep));

			PUT_SELECTOR(client, x, oldx + move_x);
			PUT_SELECTOR(client, y, oldy + move_y);

			SCIkdebug(SCIkBRESEN, "Pos (%d,%d): Trying angle %d; delta=(%d,%d)\n",
				  oldx, oldy, angle, move_x, move_y);

			if (invoke_selector(INV_SEL(client, canBeHere, 1) , 0)) {
				SCIkwarn(SCIkERROR, "Client %04x of avoider %04x doesn't"
					 " have a canBeHere() funcselector\n", client, avoider);
				return;
			}

			PUT_SELECTOR(client, x, oldx);
			PUT_SELECTOR(client, y, oldy);

			if (s->acc) { /* We can be here */
				SCIkdebug(SCIkBRESEN, "Success\n");
				PUT_SELECTOR(client, heading, angle);
				s->acc = angle;
				return;
			}

			angle += rotation;

			if (angle > 360)
				angle -= 360;
		}

		SCIkwarn(SCIkWARNING, "DoAvoider failed for avoider %04x\n",
			 avoider);

	} else {
		int heading = GET_SELECTOR(client, heading);

		if (heading == -1)
			return; /* No change */

		PUT_SELECTOR(client, heading, angle);

		s->acc = angle;

		if (looper) {
			if (invoke_selector(INV_SEL(looper, doit, 1), 2, angle, client)) {
				SCIkwarn(SCIkERROR, "Looper %04x of avoider %04x doesn't"
					 " have a doit() funcselector\n", looper, avoider);
			} else return;
		}
		/* No looper? Fall back to DirLoop */

		_k_dirloop(client, (word)angle, s, funct_nr, argc, argp);
	}
#endif
}

