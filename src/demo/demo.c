/***************************************************************************
 demo.c Copyright (C) 1999 Christoph Reichenbach, TU Darmstadt


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

    Christoph Reichenbach (CJR) [creichen@rbg.informatik.tu-darmstadt.de]

***************************************************************************/

#include <resource.h>
#include <graphics_ggi.h>
#include <sound.h>
#include <uinput.h>
#include <console.h>

static int small = 0;

int quit = 0;
int redraw = 0;
int drawflags = 1;
int drawpalette = 0;

picture_t pic, bgpic;

int
c_quit()
{
  quit = 1;
}

int
c_redraw()
{
  redraw = 1;
}

int
c_drawadd()
{
  resource_t *rsc = findResource(sci_pic, cmd_params[0].val);

  if (!rsc)
    sciprintf("Resource not found!\n");
  else
    drawPicture0(bgpic, drawflags, drawpalette, rsc->data);
}

int main(int argc, char** argv)
{
  ggi_visual_t bigvis;
  int noobj = 0;
  resource_t *resource, *font;
  port_t clipframe = {150, 120, 180, 200};
  port_t winframe = {12, 110, 46, 209};
  int boxcol = 15, boxpri = 12, boxflags = 0;

  int pointernr = 999; /* mouse pointer ID */
  int bgpicnr = 10; /* background picture number */

  int view_nr = 4, view_loop = 4, view_cell = 1;

  int conmode = 0;
  /*  char* mytext = "You pause for a moment to reflect\n"
      "upon the new support for sound resources."; */
  int i;

  for (i = 1; i< argc; i++)
    if ((strcmp(argv[i], "--noobj")==0) || (strcmp(argv[i], "-n")==0))
      noobj = 1;

  for (i = 1; i< argc; i++)
    if ((strcmp(argv[i], "--small")==0) || (strcmp(argv[i], "-s")==0))
      small = 1;

  printf("demo.c Copyright (C) 1999 Christoph Reichenbach\n"
	 "This program is free software. You can copy and/or modify it freely\n"
	 "according to the terms of the GNU general public license, v2.0\n"
	 "or any later version, at your option.\n"
	 "It comes with ABSOLUTELY NO WARRANTY.\n");

  ggiInit();

  sci_color_mode = SCI_COLOR_DITHER256;

  if (i = loadResources(SCI_VERSION_AUTODETECT, 1)) {
    fprintf(stderr,"SCI Error: %s!\n", SCI_Error_Types[i]);
    ggiExit();
    exit(-1);
  };
  printf("SCI resources loaded.\n");

  if (!noobj)
  if (loadObjects()) {
    fprintf(stderr,"Could not load objects\n");
    sciprintf("Could not load objects\n");
  }

  printf("Sound output interface: %s\n",
	 SCI_sound_interfaces[initSound(SCI_SOUND_INTERFACE_AUTODETECT)]);
  if (mapMIDIInstruments())
    fprintf(stderr, "Could not map MIDI instruments; defaulting to GM map\n");

  if (small==0) {
    if (!(bigvis = openDoubleVisual())) {
      fprintf(stderr, "Could not open 640x400 GGI visual!\n");
      exit(-1);
    }
  } else {
    if (!(bigvis = openVisual())) {
      fprintf(stderr, "Could not open 320x200 GGI visual!\n");
      exit(-1);
    }
  }

  pic = allocEmptyPicture();
  bgpic = allocEmptyPicture();

  resource = findResource(sci_pic, 10);
  if (resource == NULL) fprintf(stderr,"Picture not found!\n");
  else drawPicture0(pic, 1, 0, resource->data);

  copyPicture(bgpic, pic);

  /*  font = findResource(sci_font, 300);
  if (font == NULL) fprintf(stderr, "Font not found!\n");
  else {
    drawTextboxy0(pic, 30, mytext, font->data, 15);
    drawText0(pic, mytext, font->data, 0);
    } */

  cmdInit();
  cmdHookInt(&pointernr, "pointer_nr", "demo: cursor res# for the mouse pointer");
  cmdHookInt(&bgpicnr, "bgpic_nr", "demo: background picture res#");
  cmdHookInt(&view_nr, "view_nr", "demo: view resource number");
  cmdHookInt(&view_loop, "view_loop", "demo: view loop number");
  cmdHookInt(&view_cell, "view_cell", "demo: view loop frame number");
  cmdHookInt(&drawflags, "bgpic_flags", "demo: Flags for pic drawing");
  cmdHookInt(&drawpalette, "bgpic_palette", "demo: Default palette for pic");
  cmdHookInt(&boxcol, "box_color", "demo: Text box FG color");
  cmdHookInt(&boxpri, "box_priority", "demo: Text box priority");
  cmdHookInt(&boxflags, "box_flags", "demo: Text box flags");
  cmdHook(&c_quit, "quit", "", "demo: Quits");
  cmdHook(&c_redraw, "redraw", "", "demo: Redraw background picture");
  cmdHook(&c_drawadd, "drawadd", "i", "demo: Add to background picture");

  //  con_passthrough = 1; /* enables all sciprintf data to be sent to stdout */
  sciprintf("FreeSCI, version "VERSION"\n");

  while (!quit) {
    char *command;

    sci_event_t event = getEvent();
    if (command = consoleInput(&event)) {
      sciprintf(" >%s\n", command);
      cmdParse(command);
      sciprintf("\n");
    }

    switch (event.type) {
    case SCI_EV_MOUSE_CLICK:
      sciprintf("click %d at (%d, %d)\n", event.key, sci_pointer_x, sci_pointer_y);
      break;
    case SCI_EV_KEY:
      if (event.key == 'Q') quit = 1;
      break;
    case SCI_EV_CTRL_KEY:
      break;
    case SCI_EV_ALT_KEY:
      sciprintf("ALT-%c\n", event.key);
      if (event.key == 'X') quit = 1;
      break;
    case SCI_EV_SPECIAL_KEY:
      if (event.key == SCI_K_CONSOLE)
	conmode = 1-conmode;
      break;
    case SCI_EV_CLOCK: break;
    case SCI_EV_REDRAW:

      if (redraw) {
	clearPicture(bgpic, drawpalette? 0 : 15); /* Palette 0 is night; clear black */
        resource = findResource(sci_pic, bgpicnr);
        if (resource == NULL) sciprintf("Picture not found!\n");
        else drawPicture0(bgpic, drawflags, drawpalette, resource->data);
	redraw = 0;
      }

      copyPicture(pic, bgpic);

      resource = findResource(sci_font, 0);
      if (resource)
	drawWindow(pic, &winframe, boxcol, boxpri, "Title", resource->data, boxflags);

      resource = findResource(sci_view, view_nr); /* Hero */
      if (resource)
	drawView0(pic, &winframe,
		  100-(sci_pointer_x/2), -3, 13, view_loop, view_cell, resource->data);

      if (pointernr != -1) {
	resource = findResource(sci_cursor, pointernr);
	if (resource == NULL) {
	  sciprintf("Pointer not found!\n");
	  pointernr = -1;
	} else
	  drawMouseCursor(pic, sci_pointer_x, sci_pointer_y, resource->data);
      }

      if (conmode) {
	if (con_visible_rows < 20)
	  con_visible_rows++;
      } else if (con_visible_rows > 0)
	con_visible_rows--;

      drawConsole(pic);

      displayPicture(bigvis, pic, 0);
      break;
    }
  }


  freePicture(pic);
  freePicture(bgpic);
  freeResources();
  ggiExit();
  return 0;
}
