/* SDV.C - the living pulsating heart of the program
   I should probably split this one up a bit more. Maybe later.
*/

#include "readmap.h"

/* defined in map.c : */
extern struct mapel *map, *mapptr, *mapend;
extern unsigned long maplen;
/* map is the index indicating locations of data 
   mapend is its last element, mapptr is our current location in it
   maplen is the number of entries */

unsigned int datainfo[5] = {0};
   /* DATAINFO ELEMENTS:
       dtype, dnum, compressed length, uncompressed length, coding method */
#define DATALEN datainfo[3]
/* This is the information put at the beginning of each piece of data in
   the resource files. dtype and dnum are there to confirm that you've
   found the right thing, the rest are there to help uncompress it. */

int version = 2;
/* Despite its general stability, SCI had several minor changes over the
   years. Any code that branches on version is a result of those
   minor changes. Most changes were improvements in the indexing
   and data headers. */

#include "sd.c"
/* If I compiled them seperately, the list of externs would be huge.
   That's what you get for seperating code on such an arbitrary basis,
   I guess. */

char *datafilename(dtype, dnum)
/*** WARNING: Static String - Results only saved until next call! */
{
  static char fname[16];
  if (version < 2) sprintf(fname, "%s.%03d", types[dtype], dnum);
  else sprintf(fname, "%d.%s", dnum, types3[dtype]);
  return fname;
}

/********************** viewers and such ********************/

char *readdata(unsigned long entry)
/* This uncompresses and reads data from the resource files,
   given the number of the entry in resource.map that refers to it. */
{
  static char resname[16];
  static int resh = -1, lastresnum = 1000;
    /* resh is resource file handle
       lastresnum is the resource file we read in the last call */
  int resnum = map[entry].location >> ((version > 0)? 28 : 26);
  char *data; /* This is where we put it all. */

  if (entry == 0xffffffff) return 0; /* end-of-index tag */
  if (resnum != lastresnum) { /* don't close a file only to open it again */
    close(resh);
    sprintf(resname, "%s\\resource.%03d", gamedir, resnum);
    resh = open(resname, O_RDONLY | O_BINARY);
    if (resh == -1) {
      textcolor(4);
      sprintf(err, "Could not open %s", resname);
      message(err);
      lastresnum = 1000;
      return NULL;
    }
    lastresnum = resnum;
  }
  /* Now that the proper resource is open, go to the proper point in it. */
  lseek(resh, map[entry].location & ((version > 0)? 0x0ffffffful : 0x03fffffful),
	  SEEK_SET);
  if (version < 2) {
  /* Old style: dtype and dnum are encoded as 5 bits/11 bits of a single
     word. */
    unsigned int id;
    read(resh, &id, 2);
    datainfo[0] = (id >> 11) | 0x80;
    datainfo[1] = id & 0x7ff;
    read(resh, datainfo+2, 6);
  }
  else {
    read(resh, datainfo, 1); /* The first field is a byte, not a word. */
    read(resh, datainfo+1, 8);
  }
  /* Check to see that we're really reading the right thing */
  if (datainfo[0] != (map[entry].dtype | 0x80) ||
      datainfo[1] != map[entry].dnum) {
    sprintf(err, "WARNING: data file does not confirm presence of %s\n"
		 "Press ESC to abort, any other key to continue",
		 datafilename(map[entry].dtype, map[entry].dnum));
    textcolor(4);
    message(err);
    if (getch() == 27) {
      message("");
      return NULL;
    }
  }
  /* allocate the space */
  data = farmalloc(datainfo[3]);
  if (data == NULL) {
    textcolor(4);
    message("Not enough memory.");
    return NULL;
  }
  /* Call different decompression routines based on datainfo. 
     This should probably be in decrypt.c, but done is done. */
  switch (datainfo[4]) {
    case 0: /* Case 0: no compression */
      read(resh, data, datainfo[3]);
      break;
    /* Case 1 is unused. Perhaps it's an in-house thing. */
    case 2:
      decryptinit();
      decrypt(datainfo[3], data, resh);
      break;
    case 3:
      decrypt3(data, resh);
      break;
    case 4:
      decrypt4(data, resh);
      break;
    default:
      textcolor(4);
      sprintf(err, "Unknown encryption method: %d", datainfo[4]);
      message(err);
      farfree(data);
      return 0;
  }
  return data;
}

void dumpfile(unsigned long entry)
/* Reads compressed data, then writes it to disk in uncompressed form */
{
  char *fname, *data;
  int dump, typenum, fnum;
  textcolor(7);
  textbackground(0);
  message("Reading...");
  data = readdata(entry);
  if (data == NULL) return; /* the error message is printed by readdata */
  /* Produce the filename: */
  typenum = map[entry].dtype;
  fnum = map[entry].dnum;
  fname = datafilename(typenum, fnum);
  dump = open(fname, O_WRONLY | O_BINARY | O_TRUNC);
  if (dump == -1) dump = open(fname, O_CREAT | O_BINARY | O_TRUNC, 0x80);
  message("Writing...");
  typenum |= 0x80;
  write(dump, &typenum, 2); /* Putting this here makes the resulting
			       file readable by the game */
  write(dump, data, datainfo[3]); /* datainfo[3] is data length */
  close(dump);
  message("Done");
  farfree(data);
}

/************ graphics routines ************/
/* Colormap routines are done by BIOS interrupts.
   Actually altering the screen is done by writing to video memory,
     often using the mem.h functions. */

#define SCREENBASE (char far *)0xa0000000
/* start of video memory for VGA graphics mode */

static char colormap[3*256];
/* We save the colormap here when we go back to text mode.
   Necessary because sometimes two pictures share colors. */

void savecmap()
{
  union REGS r;
  struct SREGS segs;
  r.x.ax = 0x1017; /* interrupt subfunction #s for 'save colormap' */
  r.x.bx = 0;
  r.x.cx = 256;
  r.x.dx = (long int)colormap & 0xffff;
  segs.es = (long int)colormap >> 16;
  int86x(0x10, &r, &r, &segs);
}

void restorecmap()
{
  union REGS r;
  struct SREGS segs;
  r.x.ax = 0x1012; /* interrupt subfunction #'s for 'write colormap' */
  r.x.bx = 0;
  r.x.cx = 256;
  r.x.dx = (long int)colormap & 0xffff;
  segs.es = (long int)colormap >> 16;
  int86x(0x10, &r, &r, &segs);
}

void graphmode() /* enters 320x20x256 mode, the mode used by Sierra games */
{
  union REGS regs;
  regs.x.ax = 0x0013;
  int86(0x10, &regs, &regs);
  regs.x.ax = 0x0500;
  int86(0x10, &regs, &regs);
  restorecmap();
}

void endgraph() /* goes back to text mode */
{
  static union REGS regs = {0x0003};
  savecmap();
  int86(0x10, &regs, &regs);
  _setcursortype(_NOCURSOR);
}

void viewview(unsigned char *data)
/* views VIEW data: the animation cells. Animates them, too.
   View data starts with a table of all the loops of animation in
   that view. Loops start with a table of all the cells.
   More details in the code. */
{
  register unsigned char far *screen; /* we write to the screen here */
  register unsigned int x, y; /* screen coordinates */
  unsigned char *viewptr, c; /* viewptr is our location in the file */
  register unsigned char far *endrow;
  char far *erase; /* point on the screen to erase */
  unsigned int *intptr, /* used to point to words in data */
     nloops, ncells, /* total number of loops and cells */ 
     loop, cell, /* current loop and cell */ 
     left, top, /* where we should start drawing */
     lasttop = 200, lastbottom = 0, lastleft=320, lastright = 0,
	     /* the dimensions of the last cell we drew */
     erasewidth, /* width of region to erase on screen */
     mode=0; /* indicates whether we're animating or not */
  static unsigned int wait = 64;
  char viewback; /* background color */
  union REGS r;
  if (data == NULL) return;
	   /* read the colormap */
  for (viewptr=data; viewptr < data+DATALEN; viewptr++)
    /* Colormap is preceded in file by the string 'PAL' */
    if (viewptr[0]=='P' && viewptr[1]=='A' && viewptr[2]=='L') {
      /* I don't know what the first 0x100 bytes are. They never change. */
      viewptr += 3+0x100;
      viewptr += 3;
      viewback = *(viewptr++); /* First entry indicates background color */
      r.x.ax = 0x1010; /* interrupt subfunction for 'set colormap entry' */
      for (r.x.bx=0; r.x.bx < 0x100; r.x.bx++) {
	/* The first byte of each entry tells whether to change it or not. */
	if (*(viewptr++)) {
	  r.h.dh = *(viewptr++)>>2;
	  r.h.ch = *(viewptr++)>>2;
	  r.h.cl = *(viewptr++)>>2;
	  int86(0x10, &r, &r);
	}
	else viewptr += 3;
      }
      break;
    }
  /* Fill the screen with the background color */
  memset(SCREENBASE+(320*10), viewback, 320*(200-10));
  nloops = data[0]; /* first byte: number of loops */
  loop = cell = 1;
  while (1) {
    intptr = (unsigned int *)data+3+loop; /* get starting location of loop */
    intptr = (unsigned int *)(data+*intptr);
    ncells = *intptr; /* loop starts with number of cells */
    intptr += cell+1; /* get starting location of cell */
    intptr = (unsigned int *)(data+*intptr);
    viewptr = (char *)(intptr+4); /* first 4 bytes of cell indicate dimensions */
    left = (320-intptr[0])/2; /* adjust top and left to center cell on screen */
    top = (200-intptr[1])/2;
    screen = SCREENBASE+(320*top)+left;
    erasewidth = lastright-lastleft;
    erase = SCREENBASE+(320*lasttop)+lastleft;
    /* We now erase the last cell and write the new one at the same time.
       This reduces flicker. */
    /* erase the portion above the top of the new cell */
    if (lasttop < top) for (y=top-lasttop; y != 0; y--) {
      memset(erase, viewback, erasewidth);
      erase += 320;
    }
    erase = SCREENBASE+(320*(top+intptr[1]))+lastleft;
    /* erase the portion below the bottom of the new cell */
    if (lastbottom > top+intptr[1])
      for (y=lastbottom-(top+intptr[1]); y != 0; y--) {
	memset(erase, viewback, erasewidth);
	erase += 320;
      }
    /* erase the portion to the left of the new cell */
    if (lastleft < left) {
      erase = SCREENBASE+(320*top)+lastleft;
      erasewidth = left-lastleft;
      for (y=intptr[1]; y != 0; y--) {
	memset(erase, viewback, erasewidth);
	erase += 320;
      }
    }
    /* erase the portion to the right of the new cell */
    if (lastright > left+intptr[0]) {
      erase = SCREENBASE+(320*top)+left+intptr[0];
      erasewidth = lastright-(left+intptr[0]);
      for (y=intptr[1]; y != 0; y--) {
	memset(erase, viewback, erasewidth);
	erase += 320;
      }
    }
    /* draw the new cell */
    for (y=intptr[1]; y!=0; y--) { /* loop by height */
      endrow = screen+intptr[0]; /* loop by width efficiently */
      erase += 320;
      while (screen < endrow) {
	/* The data is run-length encoded. For each byte, there
	   are three cases: */
	/* Case 1: byte >= C0. Low 6 bits indicates number of pixels
	   to be set to the background color. */
	if (*viewptr >= 0xc0) for (x=*(viewptr++)&0x3f; x != 0; x--)
	   *(screen++) = viewback;
	/* Case 2: C0 > byte > 80. Low 6 bits indicates number of
	   pixels that should be set to the next byte. */
	else if (*viewptr >= 0x80) {
	  for (x = *(viewptr++)& 0x7f; x>0; x--) *(screen++) = *viewptr;
	  viewptr++;
	}
	/* Case 3: 80 > byte. Byte indicates number of following bytes
	   to copy to the screen. */
	else for (x=*(viewptr++); x>0; x--) *(screen++) = *(viewptr++);
      }
      screen += 320-intptr[0]; /* Go to next row. */
    }
    lasttop = top;
    lastbottom = top+intptr[1];
    lastleft = left;
    lastright = left+intptr[0];
    gotoxy(1,1); /* Print some stats */
    printf("cell %2d of %2d in loop %2d of %2d", cell, ncells, loop, nloops);
    switch (mode) { /* Behavior depends on mode. */
      case 2: /* Mode 2: continuous looping animation until keypress */
      case 1: /* Mode 1: animate loop once, then stop. */
	if (kbhit()) {
	  c = getch();
	  switch (c) { /* Special keys: + and - adjust animation speed. */
	    case '+':
	      if (wait > 1) wait >>= 1;
	      break;
	    case '-':
	      if (wait < 0x8000) wait <<= 1;
	      break;
	    case 0:
	      getch();
	    default:
	      mode = 0;
	  }
	}
	if (mode != 0) {
	  cell++;
	  if (cell > ncells) {
	    if (mode == 2) cell = 1;
	    else {
	      mode = 0;
	      cell--;
	    }
	  }
	  delay(wait);
	}
	if (mode) break;
      case 0: /* Mode 0: choose cells interactively through arrow keys */
	c = getch();
	switch (c) {
	  case 0:
	    c = getch();
	    switch (c) {
	      case 0x48: /* up arrow */
		loop++;
		if (loop > nloops) loop--;
		else cell = 1;
		break;
	      case 0x4b: /* left arrow */
		cell--;
		if (cell == 0) cell++;
		break;
	      case 0x4d: /* right arrow */
		cell++;
		if (cell > ncells) cell--;
		break;
	      case 0x50: /* down arrow */
		loop--;
		if (loop == 0) loop++;
		else cell = 1;
		break;
	      case 0x3b: /* f1 */
		cell = 1;
		mode = 1;
		break;
	      case 0x3c: /* f2 */
		mode = 2;
		break;
	    }
	    break;
	  case 27:  /* ESC exits, as always */
	    return;
	  case '+':
	    if (wait > 1) wait >>= 1;
	    break;
	  case '-':
	    if (wait < 0x8000) wait <<= 1;
	    break;
	  default: /* If the key isn't special, cycle through cells and loops. */
	    cell++;
	    if (cell > ncells) {
	      loop++;
	      cell = 1;
	    }
	    if (loop > nloops) return;
	    break;
	}
	break;
    }
  }
}

   /* Routines for background stuff */
char far *vscreen, *pscreen, *cscreen;
/* Backgrounds come in three screens:
    Video shows what you see on the screen
    Priority governs what covers what
    Control indicates "special areas" (walls, water, tripwires, etc.) 
   In the game, you can only see V, but I let you see all three. 
   For this reason, I need line-drawing and flood-fill routines.
   These I leave largely uncommented because they are such well-known
   algorithms. */
unsigned int curx, cury;

void line(int curx, int cury, int x1, int y1, char far *base, int color)
/* Bresenham. Need I say more? */
/* (If so, I'll say this: Foley & Van Dam, p. 78.) */
{
  int temp, dx, dy, incrE, incrNE, d, x, y, finalx, finaly;
  finalx = x1;
  finaly = y1;
  dx = (x1 > curx)? x1-curx : curx-x1;
  dy = (y1 > cury)? y1-cury : cury-y1;
  if (dx > dy) {
    if (curx > x1) {
      temp = curx;
      curx = x1;
      x1 = temp;
      temp = cury;
      cury = y1;
      y1 = temp;
    }
    dy = y1 - cury;
    x = curx;
    y = cury;
    base[320*y+x] = color;
    d = (dy > 0)? 2*dy-dx : 2*dy+dx;
    incrE = 2*dy;
    incrNE = (dy > 0)? 2*(dy-dx) : 2*(dy+dx);
    if (dy > 0) while (x < x1) {
      if (d < 0) d += incrE;
      else {
	d += incrNE;
	y++;
      }
      x++;
      base[320*y+x] = color;
    }
    else while (x < x1) {
      if (d > 0) d += incrE;
      else {
	d += incrNE;
	y--;
      }
      x++;
      base[320*y+x] = color;
    }
  }
  else {
    if (cury > y1) {
      temp = curx;
      curx = x1;
      x1 = temp;
      temp = cury;
      cury = y1;
      y1 = temp;
    }
    dx = x1 - curx;
    x = curx;
    y = cury;
    base[320*y+x] = color;
    d = (dx > 0)? 2*dx-dy : 2*dx+dy;
    incrE = 2*dx;
    incrNE = (dx > 0)? 2*(dx-dy) : 2*(dx+dy);
    if (dx > 0) while (y<y1) {
      if (d < 0) d += incrE;
      else {
	d += incrNE;
	x++;
      }
      y++;
      base[320*y+x] = color;
    }
    else while (y<y1) {
      if (d > 0) d += incrE;
      else {
	d += incrNE;
	x--;
      }
      y++;
      base[320*y+x] = color;
    }
  }
  curx = finalx;
  cury = finaly;
}

#define BACKGROUND 0
#define fillboundary(x, y) (fillbase[320*y+x] != BACKGROUND)
#define plot(x, y) (fillbase[320*y+x] = fillcolor)
unsigned char far *fillbase;
int fillcolor, fillheight;

void fillhelp(int xstart, int xend, int y, int direction)
/* Floodfill, recursive, line by line. Direction indicates whether
   the last line we filled was above or below the current one. */
{
  register xright = xstart, xleft = xstart;
  y += direction;
  if (y >= 0 && y < fillheight) {
    if (!fillboundary(xleft, y)) {
      while (xleft > 0 && !fillboundary(xleft-1, y)) xleft--;
      for (xright = xleft; xright < 319 && !fillboundary(xright, y); xright++)
	plot(xright, y);
      if (xleft < xstart) fillhelp(xleft, xstart, y, -direction);
      fillhelp(xleft, xright-1, y, direction);
    }
    while (xright <= xend) {
      while (fillboundary(xright, y)) {
	xright++;
	if (xright > xend) return;
      }
      xleft = xright;
      while (xright<319 && !fillboundary(xright, y)) {
	plot(xright, y);
	xright++;
      }
      fillhelp(xleft, xright-1, y, direction);
    }
    xright--;
    if (xright > xend) fillhelp(xend, xright, y, -direction);
  }
}

void fill(int x, int y, char far *base, int color, int height)
/* base is the start of the screen buffer to which you are writing.
   height is used because not all PIC files are as tall as the screen. */
{
  register xstart, xend, xmid;
  fillbase = base;
  if (fillboundary(x, y)) return;
  fillheight = height;
  fillcolor = color;
  for (xstart = x; xstart >= 0 && !fillboundary(xstart, y); xstart--)
    plot(xstart, y);
  xstart++;
  for (xend = x+1; xend < 319 && !fillboundary(xend, y); xend++)
    plot(xend, y);
  xend--;
  fillhelp(xstart, xend, y, 1);
  fillhelp(xstart, xend, y, -1);
}


#undef plot
#undef fillboundary

/* Okay, now we're finally ready to tackle: */
void viewpic(char *data)
/* Displays PIC data, which is the backgrounds. 
   The visual portion is drawn as a big run-length encoded bitmap.
   The priority and control screens are stroked in using a special
   byte code. */
{
  unsigned char *ptr, /* pointer into data */
  code, /* current byte code */
  priority=15, control=15;
  unsigned char pridraw=1, condraw=1; /* indicates whether P and V are active */
  unsigned int x, y, curx, cury, k, screenheight=190, sdiff=3200;
  union REGS r = {0x0013};
  memset(vscreen, BACKGROUND, 64000); /* clear all three screens */
  memset(pscreen, BACKGROUND, 64000);
  memset(cscreen, BACKGROUND, 64000);
  ptr = data;
  if (data == NULL) return;
  while (ptr < data+DATALEN) {
    code = *(ptr++);
    switch (code) {
    /* All the byte codes are > 0xF0. */
      case 0xf0: /* set color */
	break; /* This is vestigial from the old EGA SCI. */
      case 0xf1: /* turn off color draw */
	break; /* So is this. Colors are no longer stroked in. */
      case 0xf2: /* set priority */
	priority = *(ptr++);
	pridraw = 1;
	break;
      case 0xf3: /* turn off priority draw */
	pridraw = 0;
	break;
      case 0xf5: /* draw relative lines: sector x y [dx dy]... */
	  /* figure sector x y */
	curx = ((*ptr & 0xf0) << 4) | (0xff & ptr[1]);
	cury = ((*ptr & 0x0f) << 8) | (0xff & ptr[2]);
	ptr += 3;
	  /* draw lines */
	while (*ptr < 0xf0) {  
	  if (ptr[0] & 0x80) y = cury - (ptr[0] & 0x7f);
	  else y = cury + ptr[0];
	  x = curx + (signed char)(ptr[1]);
	  if (pridraw) line(curx, cury, x, y, pscreen, priority);
	  if (condraw) line(curx, cury, x, y, cscreen, control);
	  curx = x;
	  cury = y;
	  ptr +=2;
	}
	break;
      case 0xf6: /* draw long lines: sector x y [sector x y]... */
	  /* figure sector x y */
	curx = ((*ptr & 0xf0) << 4) | (0xff & ptr[1]);
	cury = ((*ptr & 0x0f) << 8) | (0xff & ptr[2]);
	ptr += 3;
	  /* draw lines */
	while (*ptr < 0xf0) {
	  x = ((*ptr & 0xf0) << 4) | (0xff & ptr[1]);
	  y = ((*ptr & 0x0f) << 8) | (0xff & ptr[2]);
	  if (pridraw) line(curx, cury, x, y, pscreen, priority);
	  if (condraw) line(curx, cury, x, y, cscreen, control);
	  curx = x;
	  cury = y;
	  ptr += 3;
	}
	break;
      case 0xf7: /* draw short lines: sector x y [d]... */
	  /* figure sector x y */
	curx = ((*ptr & 0xf0) << 4) | (0xff & ptr[1]);
	cury = ((*ptr & 0x0f) << 8) | (0xff & ptr[2]);
	ptr += 3;
	  /* draw lines */
	while (*ptr < 0xf0) {
	  if (*ptr & 0x80) x = curx - ((*ptr >> 4) & 7);
	  else x = curx + ((*ptr >> 4) & 7);
	  if (*ptr & 0x08) y = cury - (*ptr & 7);
	  else y = cury + (*ptr & 7);
	  if (pridraw) line(curx, cury, x, y, pscreen, priority);
	  if (condraw) line(curx, cury, x, y, cscreen, control);
	  curx = x;
	  cury = y;
	  ptr++;
	}
	break;
      case 0xf8: /* fill: sector x y */
	while (*ptr < 0xf0) {
	curx = ((*ptr & 0xf0) << 4) | (0xff & ptr[1]);
	cury = ((*ptr & 0x0f) << 8) | (0xff & ptr[2]);
	  if (pridraw) fill(curx, cury, pscreen, priority, screenheight);
	  if (condraw) fill(curx, cury, cscreen, control, screenheight);
	  ptr += 3;
	}
	break;
      case 0xfb: /* set control */
	control = *(ptr++);
	condraw = 1;
	break;
      case 0xfc: /* turn off control draw */
	condraw = 0;
	break;
      case 0xfe: /* define palette and draw bitmap */
	code = *(ptr++); /* Yes, we take a sub-code here. */
	switch (code) {
	  register char far *screen;
	  case 1: /* Draw bitmap */
	    ptr += 5; /* skip header */
	    screenheight = *(int *)(ptr+2);
	    sdiff = 320*(200-screenheight);
	    ptr += 7;
	    screen = vscreen;
	    while (screen < vscreen+(320*screenheight)) {
	      /* The run-length encoding is just like that in VIEW data. */
	      if (*ptr >= 0xc1) /* skip */
		screen += *(ptr++)-0xc0;
	      else if (*ptr == 0xc0) {
		ptr++;
		printf("%d", *(ptr++));
	      }
	      else if (*ptr >= 0x80) { /* run */
		for (k=*(ptr++); k>0x80; k--) *(screen++) = *ptr;
		ptr++;
	      }
	      else { /* dump */
		for (k=*(ptr++); k>0; k--) *(screen++) = *(ptr++);
	      }
	    }
	    break;
	  case 2: /* set color map. Again, see the VIEW code. */
	    ptr += 0x100;
	    ptr += 4;
	    k = 0;
	    while (*ptr < 0xf0) {
	      if (*(ptr++)) {
		colormap[k++] = *(ptr++)>>2;
		colormap[k++] = *(ptr++)>>2;
		colormap[k++] = *(ptr++)>>2;
	      }
	      else {
		ptr += 3;
		k+=3;
	      }
	    }
	    break;
	  case 4: /* I'm not sure what this does, but it takes 16 bytes.
		     Perhaps it defines the 16 colors for the Priority and
		     Control views in debug mode or something. */
	    ptr += 16;
	    break;
	  default: /* If we hit this, we're lost. Try to find home again. */
	    while (*ptr < 0xf0) {
	      ptr++;
	    }
	    break;
	}
	break;
      case 0xff: /* end */
	break;
      default: ;
    }
  }
  /* Okay, now put it on the screen! */
  graphmode();
  memcpy(SCREENBASE+sdiff, vscreen, 64000-sdiff);
  savecmap();
  while ((code = getch()) != 27) switch (code) {
    case '1':
      restorecmap();
      memcpy(SCREENBASE+sdiff, vscreen, 64000-sdiff);
      break;
    case '2':
      int86(0x10, &r, &r);
      memcpy(SCREENBASE+sdiff, pscreen, 64000-sdiff);
      break;
    case '3':
      int86(0x10, &r, &r);
      memcpy(SCREENBASE+sdiff, cscreen, 64000-sdiff);
      break;
    default: ;
  }
  endgraph();
}

void viewpalette(char *data)
/* View Palette data. Just read the palette like we always do and put it
   on the screen in nice blocks. */
{
  union REGS r;
  register char far *screen;
  register int x, y;
  char *ptr;
  ptr = data;
  ptr += 256;
  ptr += 4;
  r.x.ax = 0x1010;
  for (r.x.bx = 0; r.x.bx < 0xff; r.x.bx++) {
    if (*(ptr++)) {
      r.h.dh = *(ptr++)>>2;
      r.h.ch = *(ptr++)>>2;
      r.h.cl = *(ptr++)>>2;
      int86(0x10, &r, &r);
      screen = SCREENBASE + (r.x.bx >> 4)*320*12 + (r.x.bx & 0xf)*16;
      for (y=12; y>0; y--) {
	for (x=16; x>0; x--) *(screen++) = r.x.bx;
	screen += 320-16;
      }
    }
    else ptr += 3;
  }
  getch();
}

void viewdata(int dtype, char *data)
/* View data in general, given its type. */
{
  static unsigned char mainscreen[25*80*2];
  gettext(1, 1, tinfo.screenwidth, tinfo.screenheight, mainscreen);
  switch (dtype) {
    case VIEW:
      graphmode();
      viewview(data);
      endgraph();
      break;
    case PIC:
      message("Drawing...");
      viewpic(data);
      message("");
      break;
    case TEXT:
      clrscr();
      viewtext(data);
      break;
    case PALETTE:
      graphmode();
      viewpalette(data);
      endgraph();
      break;
    default: ;
  }
  textmode(C80);
  puttext(1, 1, tinfo.screenwidth, tinfo.screenheight, mainscreen);
}

void viewentry(unsigned long entry)
/* Reads compressed data and displays it */
{
  char *data;
  textcolor(7);
  textbackground(0);
  message("Reading...");
  data = readdata(entry);
  if (data == NULL) return;
  message(""); /* clears the message box */
  viewdata(map[entry].dtype, data);
  farfree(data);
}

void viewfile(char *filename)
/* Reads uncompressed data from a file and displays it */
{
  char *data;
  int h;
  textcolor(7);
  textbackground(0);
  message("Reading...");
  h = open(filename, O_RDONLY | O_BINARY);
  if (h == -1) {
    sprintf(err, "Could not open %s", filename);
    textcolor(4);
    message(err);
    return;
  }
  DATALEN = filelength(h);
  data = farmalloc(DATALEN);
  if (data == NULL) {
    textcolor(4);
    message("Not enough memory");
    return;
  }
  read(h, data, DATALEN);
  close(h);
  message("");
  DATALEN -= 2; /* skip the 2-byte header */
  viewdata(data[0]&0x7f, data+2+data[1]);
  farfree(data);
}


/* Now for the user interface. This contains some ugly code, but I still
   think it's pretty good considering that I whipped it together from
   scratch in a single afternoon. 
   Basically it's a great big scrolling menu and a box that spits
   messages at you. Parts of this code are in sd.c; I would have put
   all of it there but there are quite a few version-specific details. */

/* First, some macros to hide the full horror */
#define menuline cprintf("%11s %2d", datafilename(mapptr->dtype, mapptr->dnum),\
			       mapptr->location >> (version? 28 : 26))
/* puts a single line of the menu on the screen */

#define menumark textcolor(1); textbackground(7); \
		 gotoxy(MENULEFT, menuptr); \
		 mapptr = map+menuptr+menufirst-1;\
		 menuline;
/* marks the currently selected menu entry */

#define menuunmark textcolor(7); textbackground(0); \
		   gotoxy(MENULEFT, menuptr); \
		   mapptr = map+menuptr+menufirst-1;\
		   menuline;
/* unmarks it */

#define MENUWIDTH 15
#define MENULEFT 1
unsigned long menufirst = 0;
int menuptr = 1;

void drawmenu()
{
  int k;
  textcolor(7);
  textbackground(0);
  for (k=1; k < tinfo.screenheight; k++) {
    gotoxy(MENULEFT, k);
    mapptr = map+k+menufirst-1;
    if (mapptr == mapend && menuptr >= k) menuptr = k-1;
    if (k+menufirst > maplen) cprintf("              ");
    else menuline;
  }
  menumark;
}

void main(int argc, char **argv)
{
  int dtype;
  unsigned int id;
  unsigned long loc;
  long k;
  unsigned char c;
  if (argc > 1) gotodir(argv[1]);
  if (readmap() == NULL) {
    printf("Could not find RESOURCE.MAP");
    exit(0);
  }
  vscreen = farmalloc(64000);
  pscreen = farmalloc(64000);
  cscreen = farmalloc(64000);
  textmode(C80);
  savecmap();
  _setcursortype(_NOCURSOR);
  textbackground(0);
  textcolor(7);
  clrscr();
  boxprint(30, 2, 80, 24,
      "       Hermetic Software\n"
      "           presents\n"
      "     SCI DECODER/VGA v. 1.0\n\n"
      "(c) Copyright 1993 Carl Muckenhoupt\n"
      "All rights reserved.\n\n\n\n"
      "Use arrow keys to move through menu\n"
      "left and right arrows select data type\n"
      "ENTER to view data\n"
      "D to dump data to unencoded file\n"
      "L to view unencoded data file\n"
      "ESC exits program"
    );
  gettextinfo(&tinfo);
  drawbox(MESSAGELEFT-1, MESSAGETOP-1, MESSAGERIGHT, MESSAGEBOTTOM);
  gotoxy(1, 1);
  drawmenu();
  while ((c = getch()) != 27) switch (c) { /* ESC exits, as always */
    case 0: /* getch returns 0 only as a prefix to a scan code */
      c = getch();
      switch (c) {
	case 0x48: /* up arrow */
	  if (menuptr == 1) {
	    if (menufirst == 0) continue;
	    movetext(MENULEFT, 1, MENULEFT+MENUWIDTH, tinfo.screenheight-2,
		     MENULEFT, 2);
	    menufirst--;
	    menuptr++;
	  }
	  menuunmark;
	  menuptr--;
	  menumark;
	  break;
	case 0x50: /* down arrow */
	    if (map+menufirst+menuptr >= mapend) continue;
	  if (menuptr == tinfo.screenheight-1) {
	    movetext(MENULEFT, 2, MENULEFT+MENUWIDTH, tinfo.screenheight-1,
		     MENULEFT, 1);
	    menufirst++;
	    menuptr--;
	  }
	  menuunmark;
	  menuptr++;
	  menumark;
	  break;
	case 0x49: /* page up */
	  if (menuptr > 1) {
	    menuunmark;
	    menuptr = 1;
	    menumark;
	    break;
	  }
	  if (menufirst < tinfo.screenheight) menufirst = 0;
	  else menufirst -= (tinfo.screenheight-1);
	  drawmenu();
	  break;
	case 0x51: /* page down */
	  if (menuptr < tinfo.screenheight-1) {
	    menuunmark;
	    while (menuptr < tinfo.screenheight &&
	      map+menufirst+menuptr <= mapend) menuptr++;
	    menuptr--;
	    menumark;
	    break;
	  }
	  if (menufirst + tinfo.screenheight >= maplen)
	       menufirst = maplen - tinfo.screenheight + 1;
	  else menufirst += (tinfo.screenheight-1);
	  drawmenu();
	  break;
	case 0x4b: /* left arrow */
	  dtype = map[menufirst+menuptr-1].dtype;
	  for (k = menufirst; k != 0 && map[k].dtype == dtype; k--);
	  if (k > 0) {
	    dtype = map[k].dtype;
	    while (k >= 0 && map[k].dtype == dtype) k--;
	    k++;
	  }
	  else k = 0;
	  menufirst = k;
	  menuptr = 1;
	  drawmenu();
	  break;
	case 0x4d: /* right arrow */
	  dtype = map[menufirst+menuptr-1].dtype;
	  for (k = menufirst; map[k].dtype == dtype; k++);
	  if (map+k < mapend) {
	    menufirst = k;
	    menuptr = 1;
	    drawmenu();
	  }
	  break;
	default: ;
      }
      break;
    case 0x0d: /* return */
      viewentry(menufirst+menuptr-1);
      break;
    case 'd':
    case 'D':
      dumpfile(menufirst+menuptr-1);
      break;
    case 'l':
    case 'L':
      textbackground(0);
      textcolor(7);
      viewfile(inquire("Enter file name\n"));
      break;
    default: ;
  }
  textbackground(0);
  textcolor(7);
  _setcursortype(_NORMALCURSOR);
  clrscr();
  textcolor(10);
  cputs(
"Thank you for using SCI DECODER!\r\n\n"
"Please remember to register your copy! Registration entitles you to the latest\r\n"
"version of SCI Decoder as well as the author's notes on Sierra's encryption\r\n"
"and data storage methods.\r\n\n"
"Send check or money order for $10 payable to Carl Muckenhoupt to:\r\n\n"
"Hermetic Software                 phone: (908) 369-3430\r\n"
"196 Woodfern Rd.                  internet: muckenhoupt@cancer.rutgers.edu\r\n"
"Neshanic Station, NJ 08853\r\n\n"
"Also available: SCI Decoder for Sierra's old EGA games\r\n\n");
  textcolor(7);
}
