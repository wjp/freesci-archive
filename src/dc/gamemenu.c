/*
 * Copyright 2000, 2001, 2002
 *         Dan Potter. All rights reserved.
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

#include <string.h>
#include "gp.h"

/* Takes care of the game menu */


/* Song menu choices */
typedef struct {
	char	fn[16];
	char	*dir;
	int	size;
} entry;
typedef struct {
	char	fn[64];
	int	size;
} lst_entry;
char curdir[64] = "/";
char playdir[64] = "/";
char loadme[256] = "";
char workstring[256] ="";

static entry entries[200];
static int num_entries = 0, load_queued = 0;
static int selected = 0, top = 0;

static int framecnt = 0;
static float throb = 0.2f, dthrob = 0.01f;

static int menu_state = 0;

static void load_game_list(char *dir) {
	file_t d;

	d = fs_open(dir, O_RDONLY | O_DIR);
	if (!d) return;
	{
		dirent_t *de;
		while ((de = fs_readdir(d)) && num_entries < 200) {
			if (!stricmp(de->name, "resource.map")) {
				strncpy(entries[num_entries].fn, dir, 255);
				entries[num_entries].dir = strdup(dir);
				entries[num_entries].size = 0;
				num_entries++;
			}
			else if (de->size < 0) {
				char *new_dir;
				new_dir = malloc(strlen(dir)+strlen(de->name)+2);
				strcpy(new_dir, dir);
				strcat(new_dir, "/");
				strcat(new_dir, de->name);
				load_game_list(new_dir);
				free(new_dir);
			}
		}
	}
	fs_close(d);
}

/* Draws the game listing */
static void draw_listing() {
	float y = 92.0f;
	int i, esel;

	/* Draw all the game titles */	
	for (i=0; i<10 && (top+i)<num_entries; i++) {
		draw_poly_strf(32.0f, y, 100.0f, 1.0f, 1.0f, 1.0f, 1.0f,
			entries[top+i].fn);
		y += 24.0f;
	}
	
	if (menu_state == 2) {
	/* Put a highlight bar under one of them */
	esel = (selected - top);
	draw_poly_box(31.0f, 92.0f+esel*24.0f - 1.0f,
		609.0f, 92.0f+esel*24.0f + 25.0f, 95.0f,
		throb, throb, 0.2f, 0.2f, throb, throb, 0.2f, 0.2f);
	}
}

/* Handle controller input */
static uint8 mcont = 0;
void check_controller() {
	static int up_moved = 0, down_moved = 0, a_pressed = 0, y_pressed = 0;
	cont_cond_t cond;

	if (!mcont) {
		mcont = maple_first_controller();
		if (!mcont) { return; }
	}
	if (cont_get_cond(mcont, &cond)) { return; }

	if (!(cond.buttons & CONT_DPAD_UP)) {
		if ((framecnt - up_moved) > 10) {
			if (selected > 0) {
				selected--;
				if (selected < top) {
					top = selected;
				}
			}
			up_moved = framecnt;
		}
	}
	if (!(cond.buttons & CONT_DPAD_DOWN)) {
		if ((framecnt - down_moved) > 10) {
			if (selected < (num_entries - 1)) {
				selected++;
				if (selected >= (top+10)) {
					top++;
				}
			}
			down_moved = framecnt;
		}
	}
	if (cond.ltrig > 0) {
		if ((framecnt - up_moved) > 10) {
			selected -= 10;

			if (selected < 0) selected = 0;
			if (selected < top) top = selected;
			up_moved = framecnt;
		}
	}
	if (cond.rtrig > 0) {
		if ((framecnt - down_moved) > 10) {
			selected += 10;
			if (selected > (num_entries - 1))
				selected = num_entries - 1;
			if (selected >= (top+10))
				top = selected;
			down_moved = framecnt;
		}
	}

	if (!(cond.buttons & CONT_Y)) {
		if ((framecnt - y_pressed) > 10)
		{
			num_entries = 0;
			selected = 0;
			menu_state = 1;
		}
		y_pressed=framecnt;
	}

	if (!(cond.buttons & CONT_A)) {
		if ((framecnt - a_pressed) > 10)
		{
			if (menu_state == 0) menu_state = 1;
			else if (!strcmp(entries[selected].fn, "Error!"))
			{
				num_entries = 0;
				menu_state = 1;
			}
			else
			{
				fs_chdir(entries[selected].dir);
				menu_state = 3;
			}
		}
		a_pressed = framecnt;
	}
	return;
}

/* Check maple bus inputs */
void check_inputs() {
	check_controller();
}

/* Main rendering of the game menu */
int game_menu_render() {
	/* Draw a background box */
	draw_poly_box(30.0f, 80.0f, 610.0f, 440.0f-96.0f, 90.0f, 
		0.2f, 0.8f, 0.5f, 0.0f, 0.2f, 0.8f, 0.8f, 0.2f);
		
	if (menu_state == 0) {
		draw_poly_strf(32.0f, 92.0f, 100.0f, 1.0f, 1.0f, 1.0f, 1.0f,
			"Please insert a game cd and press A...");
	}

	else if (menu_state == 1) {
		if (load_queued < 4) {
			draw_poly_strf(32.0f, 92.0f, 100.0f, 1.0f, 1.0f, 1.0f, 1.0f,
				"Scanning cd for SCI games...");
			load_queued++;
			return 0;
		} else {
			iso_ioctl(0,NULL,0);
			load_game_list("/cd");
			load_queued = 0;
			if (num_entries == 0) menu_state = 0;
			else menu_state = 2;
		}
	}
	
	else if (menu_state == 3) {
		if (load_queued < 4) {
			draw_poly_strf(32.0f, 92.0f, 100.0f, 1.0f, 1.0f, 1.0f, 1.0f,
				"Starting game, please wait...");
			load_queued++;
			return 0;
		} else {
			return 1;
		}
	}

	/* Draw the game listing */
	draw_listing();
	
	/* Adjust the throbber */
	throb += dthrob;
	if (throb < 0.2f || throb > 0.8f) {
		dthrob = -dthrob;
		throb += dthrob;
	}
	
	/* Check maple inputs */
	check_inputs();

	framecnt++;

	return 0;
}
