/*
 * Copyright 2000, 2001, 2002
 *         Dan Potter, Thorsten Titze. All rights reserved.
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of Cryptic Allusion nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* Modified by Walter van Niftrik <w.f.b.w.v.niftrik@stud.tue.nl> */

#include <stdio.h>
#include "gp.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

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
	FILE *cfile;

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
		draw_poly_strf(308.0f - (8+sizeof(VERSION)-1)/2.0f*12.0f, 20.0f, 100.0f, 1.0f, 1.0f, 1.0f, 1.0f,
			"FreeSCI " VERSION);
		draw_poly_strf(308.0f - 29.0f/2.0f*12.0f, 48.0f, 100.0f, 1.0f, 1.0f, 1.0f, 1.0f,
			"http://freesci.linuxgames.com");

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
	
	/* Write config file */
	cfile = fopen("/ram/config", "w");
	fputs("gfx.dc.render_mode = vram\n", cfile);
	fputs("midi_device = adlibemu\n", cfile);
	fputs("pcmout_stereo = 0\n", cfile);
	fputs("pcmout_rate = 11025\n", cfile);
	fclose(cfile);

	return;
}
