/* GhettoPlay: an Ogg Vorbis browser and playback util
   (c)2000-2002 Dan Potter
   (c)2001 Thorsten Titze

   Distributed as an example for libdream 0.7
   Ported up to libdream 0.95
   Ported up to KOS 1.1.x
   Converted to OggVorbis

   Historical note: this is _really_ 2.0. There was an internally
   created Ghetto Play that I made to browse and test various S3Ms
   with early versions of the player. It _really_ deserved the name
   GHETTO Play =). This one, like Ghetto Pong, is a 2.0 that is more
   like "Pimpin' Play" ;-). However, sticking with tradition...   
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the KOS License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   KOS License (README.KOS) for more details.

   You should have received a copy of the KOS License along with this
   program; if not, please visit Cryptic Allusion DCDev at:

     http://dcdev.allusion.net/

*/

/* Modified by Walter van Niftrik <w.f.b.w.v.niftrik@stud.tue.nl> */

#include <stdio.h>
#include "gp.h"

/* Render the mouse if they have one attached */
static int mx = 320, my = 240;
static int lmx[5] = {320, 320, 320, 320, 320},
	lmy[5] = {240, 240, 240, 240, 240};
void mouse_render() {
	int i;
	int atall = 0;
	
	MAPLE_FOREACH_BEGIN(MAPLE_FUNC_MOUSE, mouse_state_t, st)
		atall = 1;
		if (st->dx || st->dy) {
		
			mx += st->dx;
			my += st->dy;
		
			if (mx < 0) mx = 0;
			if (mx > 640) mx = 640;
			if (my < 0) my = 0;
			if (my > 480) my = 480;

			lmx[0] = mx;
			lmy[0] = my;
		}
	MAPLE_FOREACH_END()

	if (atall) {
		for (i=4; i>0; i--) {
			lmx[i] = lmx[i-1];
			lmy[i] = lmy[i-1];
		}

		draw_poly_mouse(mx, my, 1.0f);
		for (i=1; i<5; i++)
			draw_poly_mouse(lmx[i], lmy[i], 0.8f * (5-i) / 6.0f);
	}
}

/* This function is called from main()  */
void choose_game() {
	int fexit = 0;

	/* Do basic setup */
	pvr_init_defaults();

	/* Setup the mouse/font texture */
	setup_util_texture();

	/* Setup background display */
	bkg_setup();

	while (!fexit) {
		pvr_wait_ready();
		pvr_scene_begin();
		pvr_list_begin(PVR_LIST_OP_POLY);

		/* Opaque list *************************************/
		bkg_render();

		/* End of opaque list */
		pvr_list_finish();
		pvr_list_begin(PVR_LIST_TR_POLY);

		/* Translucent list ********************************/

		/* Top Banner */
		draw_poly_box(0.0f, 10.0f, 640.0f, 20.0f+(24.0f*2.0f)+10.0f, 90.0f, 
			0.3f, 0.2f, 0.5f, 0.0f, 0.5f, 0.1f, 0.8f, 0.2f);
		draw_poly_strf(5.0f, 20.0f, 100.0f, 1.0f, 1.0f, 1.0f, 1.0f,
			"                     FreeSCI                      ");
		draw_poly_strf(5.0f, 48.0f, 100.0f, 1.0f, 1.0f, 1.0f, 1.0f,
			"          http://freesci.linuxgames.com           ");

		/* Game menu */
		fexit = game_menu_render();
		
		/* File Information */
		draw_poly_box(20.0f, 440.0f-96.0f+4, 640.0f-20.0f, 440.0f, 90.0f, 
			0.3f, 0.2f, 0.5f, 0.0f, 0.5f, 0.1f, 0.8f, 0.2f);
		
		draw_poly_strf(30.0f,440.0f-96.0f+6+10.0f,100.0f,1.0f,1.0f,1.0f,1.0f,"D-PAD : Select game              L : Page up");
		draw_poly_strf(30.0f,440.0f-96.0f+6+24.0f+10.0f,100.0f,1.0f,1.0f,1.0f,1.0f,"    A : Start game               R : Page down");
		draw_poly_strf(30.0f,440.0f-96.0f+6+48.0f+10.0f,100.0f,1.0f,1.0f,1.0f,1.0f,"    Y : Rescan cd");

		/* Render the mouse if they move it.. it doesn't do anything
		   but it's cool looking ^_^ */
		mouse_render();

		/* End of translucent list */
		pvr_list_finish();

		/* Finish the frame *******************************/
		pvr_scene_finish();
		
	}
	
	return;
}
