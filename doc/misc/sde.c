struct mapel {
  unsigned int id; /* 5 bit data type, 11 bit file number */
  unsigned long location; /* 6 bit resource number, 26 bit location */
} *map, *mapptr;

unsigned int datainfo[4];
  /* DATAINFO ELEMENTS:
      id, ?, length, ? */
#define DATALEN datainfo[2]
unsigned long maplen;

#include "sd.c"


void setdirvid() { directvideo = 0; }
#pragma startup setdirvid


/********* read and sort resource.map ***********/

int partition(m, p)
{
  struct mapel v = map[m], temp;
  int i = m;
  while (1) {
    while (map[++i].id < v.id);
    while (map[--p].id > v.id);
    if (i >= p) break;
    temp = map[i];
    map[i] = map[p];
    map[p] = temp;
  }
  map[m] = map[p];
  map[p] = v;
  return p;
}

void quicksort(p, q)
{
  int j;
  if (p < q) {
    j = partition(p, q+1);
    quicksort(p, j-1);
    quicksort(j+1, q);
  }
}

struct mapel *readmap()
{
  int maph;
  long len;
  char mapname[80];

  sprintf(mapname, "%s\\resource.map", gamedir);
  maph = open(mapname, O_RDONLY | O_BINARY);
  if (maph == -1) return 0;
  len = filelength(maph);
  map = (struct mapel *)(malloc(len));
  read(maph, map, len);
  close(maph);
  for (maplen = 0; map[maplen].id != 0xffff; maplen++);
  quicksort(0, maplen-1);
  return map;
}

char *datafilename(datatype, fnum)
{
  static char fname[16];
  sprintf(fname, "%s.%03d", types[datatype], fnum);
  return fname;
}


/* The following are used in both decryption algorithms: */
#define MAXBIT 0x2000
unsigned int whichbit = 0;
unsigned char bits[(MAXBIT>>3)+3];

/*****************  Decryption Method 1  *******************************/

unsigned int getbits1(int numbits, int handle) /* handles bitstrings length 9-12 */
{
  int place;
  unsigned long bitstring;
  if (whichbit >= MAXBIT) {
    whichbit -= MAXBIT;
    read(handle, bits, (MAXBIT >> 3)+3);
    lseek(handle, -3L, SEEK_CUR);
  }
  place = whichbit >> 3;
  bitstring = bits[place] | (long)(bits[place+1])<<8
	      | (long)(bits[place+2])<<16;
  bitstring >>= (whichbit & 7);
  bitstring &= 0xffffffffUL >> (32-numbits);
  whichbit += numbits;
  return bitstring;
}

#define STACKSIZE 1024
char stack[STACKSIZE];
int stackptr;
void push(char c) {
  stack[stackptr++] = c;
  if (stackptr >= STACKSIZE) {
    message("Stack overflow");
    exit(1);
  }
}
char *popall(char *dest) {
  while (stackptr-- > 0) *(dest++) = stack[stackptr];
  stackptr++;
  return dest;
}

void decryp1(int handle, char *data)
{
  unsigned int field, linkindex = 0x102, nextindex = 0, newindex = 0,
      maxindex = 0x200, width = 9;
  unsigned char *links, lastbyte=0, linkbyte=0;
  links = malloc(12300);
  whichbit = MAXBIT;
  stackptr = 0;
  while ((field = getbits1(width, handle)) != 0x101) { /* exit on 0x101 */
    if (field == 0x100) { /* start over from beginning of table */
      linkindex = 0x102;
      width = 9;
      maxindex = 0x200;
      nextindex = field = getbits1(width, handle);
      *(data++) = lastbyte = linkbyte = (field & 0xff);
    }
    else {
      newindex = field;
      if (field >= linkindex) { /* if it's a forward reference, */
	push(lastbyte);         /* repeat the last thing */
	field = nextindex;
      }
      while (field > 0xff) { /* trace links back */
	push(links[field*3+2]);
	field = (int)(links[field*3]) | (int)(links[field*3+1]) << 8;
      }
      lastbyte = linkbyte = field;
      push(lastbyte);
      data = popall(data);
      links[linkindex*3+2] = linkbyte;
      links[linkindex*3+1] = nextindex >> 8;
      links[linkindex*3+0] = nextindex & 0xff;
      linkindex++;
      nextindex = newindex;
      if (linkindex >= maxindex && width != 12) {
	width++;
	maxindex <<= 1;
      }
    }
  }
  free(links);
}

/*****************  Decryption Method 2  *******************************/
/*This is basically byte-token huffman coding, except that a prefix of
  1 signals a literal byte. In fact, most bytes are literal; only extremely
  common ones are put in the huffman tree. */

char getbits2(int numbits, int handle) /* handles bitstrings length 1-8 */
{
  int bitstring, place;
  if (whichbit >= MAXBIT) {
    whichbit -= MAXBIT;
    read(handle, bits, (MAXBIT >> 3)+3);
    lseek(handle, -3L, SEEK_CUR);
  }
  place = whichbit >> 3;
  bitstring = bits[place] << 8;
  bitstring |= bits[place+1] & 0x00ff;
  bitstring >>= 16-numbits-(whichbit & 7);
  bitstring &= 0xffffu >> (16-numbits);
  whichbit += numbits;
  return bitstring;
}

int getc2(unsigned char *node, int handle)
{
  int next;
  while (node[1] != 0) {
    if (getbits2(1, handle)) {
      next = node[1] & 0xf; /* low 4 bits */
      if (next == 0) return (getbits2(8, handle) | 0x100);
    }
    else next = node[1] >> 4; /* high 4 bits */
    node += next*2;
  }
  return (int)*node;
}

void decryp2(int handle, char *data)
{
  unsigned char numnodes, terminator;
  char *nodes;
  int c;
  read(handle, &numnodes, 1);
  read(handle, &terminator, 1);
  nodes = malloc(2*numnodes);
  read(handle, nodes, 2*numnodes);
  whichbit = MAXBIT;
  while ((c = getc2(nodes, handle)) != (0x0100 | terminator)) {
    *data = (char)c;
    data++;
  }
  free(nodes);
}

#define ANYRES -1

unsigned long findentry(unsigned dattype, unsigned fnum, int resnum)
{
  unsigned int id = (dattype << 11) | fnum;

  for (mapptr = map; mapptr->id != 0xffffu; mapptr++)
    if (mapptr->id == id &&
       (resnum == ANYRES || resnum == mapptr->location >> 26))
      return (mapptr - map);
  sprintf(err, "%s.%03d not found.", types[dattype], fnum);
  message(err);
  return 0xffffffff;
}
/********************** viewers and such ********************/

char *readdata(unsigned long entry)
{
  static char resname[16];
  static int resh = -1, lastresnum = 1000;
  int resnum = map[entry].location >> 26;
  char far *data;

  if (entry == 0xffffffff) return 0;
  if (resnum != lastresnum) {
    close(resh);
    sprintf(resname, "%s\\resource.%03d", gamedir, resnum);
    resh = open(resname, O_RDONLY | O_BINARY);
    if (resh == -1) {
      textcolor(4);
      sprintf(err, "Could not open %s", resname);
      message(err);
      lastresnum = 1000;
      return 0;
    }
    lastresnum = resnum;
  }
  lseek(resh, map[entry].location & 0x03fffffful, SEEK_SET);
  read(resh, datainfo, 8);
  if (datainfo[0] != map[entry].id) {
    sprintf(err, "WARNING: data file does not confirm presence of %s\n"
		 "Press ESC to abort, any other key to continue",
		 datafilename(map[entry].id >>11, map[entry].id & 0x7f));
    textcolor(4);
    message(err);
    if (getch() == 27) return NULL;
  }
  data = farmalloc(datainfo[2]);
  if (data == NULL) {
    textcolor(4);
    message("Not enough memory.");
    return NULL;
  }
  if (datainfo[3] == 0) read(resh, data, datainfo[2]);
  else if (datainfo[3] == 1) decryp1(resh, data);
  else if (datainfo[3] == 2) decryp2(resh, data);
  else {
    message("Unknown encryption type");
    return 0;
  }
  return data;
}

void dumpfile(unsigned long entry)
{
  char *fname, *data;
  int dump, typenum, fnum;
  textcolor(7);
  textbackground(0);
  message("Reading...");
  data = readdata(entry);
  if (data == 0) {
    textcolor(4);
    message("Could not write file");
    return;
  }
  typenum = map[entry].id >> 11;
  fnum = map[entry].id & 0x7ff;
  fname = datafilename(typenum, fnum);
  dump = open(fname, O_WRONLY | O_BINARY | O_TRUNC);
  if (dump == -1) dump = open(fname, O_CREAT | O_BINARY | O_TRUNC, 0x80);
  typenum |= 0x80;
  message("Writing...");
  write(dump, &typenum, 2);
  write(dump, data, datainfo[2]);
  close(dump);
  message("Done");
  farfree(data);
}

/************ graphics routines ************/

void graphmode()
{
  union REGS out, in;
  in.x.ax = 0x000d;
  int86(0x10, &in, &out);
  in.x.ax = 0x0500;
  int86(0x10, &in, &out);
}

void endgraph()
{
  static union REGS out, in = {0x0003};
  int86(0x10, &in, &out);
  _setcursortype(_NOCURSOR);
}

void blankscreen()
{
  register unsigned int c;
  register char far *screen;
  screen = (char far *)(0xa0000000+400);
  for (c=8000-400; c > 0; c--) *(screen++) = 0xff;
}

void clearpage(p)
{
  register unsigned int c;
  register char far *screen;
  screen = (char far *)(0xa0000000+(p*0x2000000));
  outp(0x3ce, 1);
  outp(0x3cf, 0x0f);
  outp(0x3ce, 0); /* set color to 0 */
  outp(0x3cf, 0x00);
  for (c=8000; c > 0; c--) *(screen++) = 0xff;
  outp(0x3cf, 0x0f);
  outp(0x3ce, 1);
  outp(0x3cf, 0);
}

void switchpage(page)
{
  union REGS r = {0x0500};
  r.h.al = page;
  int86(0x10, &r, &r);
}

void putpix(x, y, color, page)
{
  union REGS regs;
  regs.h.ah = 0x0c;
  regs.h.al = color;
  regs.h.bh = page;
  regs.x.cx = x;
  regs.x.dx = y;
  int86(0x10, &regs, &regs);
}

char getpix(x, y, page)
{
  union REGS regs;
  regs.h.ah = 0x0d;
  regs.h.bh = page;
  regs.x.cx = x;
  regs.x.dx = y;
  int86(0x10, &regs, &regs);
  return regs.h.al;
}

int curx, cury, col1 = 0, col2 = 0;
char priority = 0, special=0;
char drawenable;

void plot(x, y)
{
  y += 10;
  if (drawenable & 1) putpix(x, y, ((x^y)&1)? col1 : col2, 0);
  if (drawenable & 2) putpix(x, y, priority, 1);
  if (drawenable & 4) putpix(x, y, special, 2);
  putpix(x, y, drawenable | getpix(x, y, 3), 3);
}

void ditherto(x1, y1)
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
    plot(x, y);
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
      plot(x, y);
    }
    else while (x < x1) {
      if (d > 0) d += incrE;
      else {
	d += incrNE;
	y--;
      }
      x++;
      plot(x, y);
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
    plot(x, y);
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
      plot(x, y);
    }
    else while (y<y1) {
      if (d > 0) d += incrE;
      else {
	d += incrNE;
	x--;
      }
      y++;
      plot(x, y);
    }
  }
  curx = finalx;
  cury = finaly;
}

#define fillboundary(fx, fy) (drawenable & getpix(fx, fy, 3) && \
			      !(drawenable&1 && getpix(fx, fy, 0)==15))

void fillhelp(xstart, xend, y, direction)
{
  register xright = xstart, xleft = xstart;
  y += direction;
  if (y >= 0 && y < 190) {
    if (!fillboundary(xleft, y+10)) {
      while (xleft > 0 && !fillboundary(xleft-1, y+10)) xleft--;
      for (xright = xleft; xright < 320 && !fillboundary(xright, y+10); xright++)
	plot(xright, y);
      if (xleft < xstart) fillhelp(xleft, xstart, y, -direction);
      fillhelp(xleft, xright-1, y, direction);
    }
    while (xright <= xend) {
      while (fillboundary(xright, y+10)) {
	xright++;
	if (xright > xend) return;
      }
      xleft = xright;
      while (xright<320 && !fillboundary(xright, y+10)) {
	plot(xright, y);
	xright++;
      }
      fillhelp(xleft, xright-1, y, direction);
    }
    xright--;
    if (xright > xend) fillhelp(xend, xright, y, -direction);
  }
}

void ditherfill(x, y)
{
  register xstart, xend, xmid;
  if (fillboundary(x, y+10)) return;
  for (xstart = x; xstart >= 0 && !fillboundary(xstart, y+10); xstart--)
    plot(xstart, y);
  xstart++;
  for (xend = x+1; xend < 320 && !fillboundary(xend, y+10); xend++)
    plot(xend, y);
  xend--;
  fillhelp(xstart, xend, y, 1);
  fillhelp(xstart, xend, y, -1);
}

unsigned char patcode, patnum;
void plotpattern(x, y)
{
  static char circles[][30] = { /* bitmaps for circle patterns */
    {0x80},
    {0x4e, 0x40},
    {0x73, 0xef, 0xbe, 0x70},
    {0x38, 0x7c, 0xfe, 0xfe, 0xfe, 0x7c, 0x38, 0x00},
    {0x1c, 0x1f, 0xcf, 0xfb, 0xfe, 0xff, 0xbf, 0xef, 0xf9, 0xfc, 0x1c},
    {0x0e, 0x03, 0xf8, 0x7f, 0xc7, 0xfc, 0xff, 0xef, 0xfe, 0xff, 0xe7,
	   0xfc, 0x7f, 0xc3, 0xf8, 0x1f, 0x00},
    {0x0f, 0x80, 0xff, 0x87, 0xff, 0x1f, 0xfc, 0xff, 0xfb, 0xff, 0xef,
	   0xff, 0xbf, 0xfe, 0xff, 0xf9, 0xff, 0xc7, 0xff, 0x0f, 0xf8,
	   0x0f, 0x80},
    {0x07, 0xc0, 0x1f, 0xf0, 0x3f, 0xf8, 0x7f, 0xfc, 0x7f, 0xfc, 0xff,
	   0xfe, 0xff, 0xfe, 0xff, 0xfe, 0xff, 0xfe, 0xff, 0xfe, 0x7f,
	   0xfc, 0x7f, 0xfc, 0x3f, 0xf8, 0x1f, 0xf0, 0x07, 0xc0}};
  static unsigned char junq[32] = { /* random-looking fill pattern */
    0x20, 0x94, 0x02, 0x24, 0x90, 0x82, 0xa4, 0xa2, 0x82, 0x09, 0x0a, 0x22,
    0x12, 0x10, 0x42, 0x14, 0x91, 0x4a, 0x91, 0x11, 0x08, 0x12, 0x25, 0x10,
    0x22, 0xa8, 0x14, 0x24, 0x00, 0x50, 0x24, 0x04};
  static unsigned char junqindex[128] = { /* starting points for junq fill */
    0x00, 0x18, 0x30, 0xc4, 0xdc, 0x65, 0xeb, 0x48,
    0x60, 0xbd, 0x89, 0x05, 0x0a, 0xf4, 0x7d, 0x7d,
    0x85, 0xb0, 0x8e, 0x95, 0x1f, 0x22, 0x0d, 0xdf,
    0x2a, 0x78, 0xd5, 0x73, 0x1c, 0xb4, 0x40, 0xa1,
    0xb9, 0x3c, 0xca, 0x58, 0x92, 0x34, 0xcc, 0xce,
    0xd7, 0x42, 0x90, 0x0f, 0x8b, 0x7f, 0x32, 0xed,
    0x5c, 0x9d, 0xc8, 0x99, 0xad, 0x4e, 0x56, 0xa6,
    0xf7, 0x68, 0xb7, 0x25, 0x82, 0x37, 0x3a, 0x51,
    0x69, 0x26, 0x38, 0x52, 0x9e, 0x9a, 0x4f, 0xa7,
    0x43, 0x10, 0x80, 0xee, 0x3d, 0x59, 0x35, 0xcf,
    0x79, 0x74, 0xb5, 0xa2, 0xb1, 0x96, 0x23, 0xe0,
    0xbe, 0x05, 0xf5, 0x6e, 0x19, 0xc5, 0x66, 0x49,
    0xf0, 0xd1, 0x54, 0xa9, 0x70, 0x4b, 0xa4, 0xe2,
    0xe6, 0xe5, 0xab, 0xe4, 0xd2, 0xaa, 0x4c, 0xe3,
    0x06, 0x6f, 0xc6, 0x4a, 0xa4, 0x75, 0x97, 0xe1
    };
  int k, l, size;
  unsigned char junqbit = junqindex[patnum];
  size = (patcode&7);
  if (x<size) x=size;
  else if (x>320-size) x=320-size;
  if (y<size) y = size;
  else if (y>=190-size) y=189-size;
  if (patcode & 0x10) { /* rectangle */
    for (l=y-size; l<=y+size; l++) for (k=x-size; k<=x+size+1; k++) {
      if (patcode & 0x20) {
	if ((junq[junqbit>>3] >> (7-(junqbit & 7))) & 1) plot(k, l);
	junqbit++;
	if (junqbit == 0xff) junqbit=0;
      }
      else plot(k, l);
    }
  }
  else { /* circle */
    int circlebit = 0;
    for (l=y-size; l<=y+size; l++) for (k=x-size; k<=x+size+1; k++) {
      if ((circles[patcode&7][circlebit>>3] >> (7-(circlebit & 7))) & 1) {
	if (patcode & 0x20) {
	  if ((junq[junqbit>>3] >> (7-(junqbit & 7))) & 1) plot(k, l);
	  junqbit++;
	  if (junqbit == 0xff) junqbit=0;
	}
	else plot(k, l);
      }
      circlebit++;
    }
  }
}

void viewview(char far *data)
{
  char *dataptr, c, curpage=0;
  int *lookup, nloops, ncells, loop, cell, x, y, maxx, maxy, minx;
  static union REGS regs = {0x0200, 0, 0, 0};
  unsigned int mode=0, wait=64;
  drawenable = 1;
  if (data == 0) return;
  nloops = *((int *)data);
  graphmode();
  loop = cell = 1;
  while (1) {
    lookup = (int *)(data + ((int *)data)[3+loop]);
		    /* loop table starts at word 4 */
    ncells = *lookup;
    lookup++;
    curpage = !curpage;
    clearpage(curpage);
    dataptr = data+lookup[cell];
    maxx = ((int *)dataptr)[0];
    maxy = ((int *)dataptr) [1];
    minx = x = (320-maxx)/2;
    y = (200-maxy)/2;
    maxx += x;
    maxy += y;
    dataptr += 8;
    while (y < maxy) {
      int color, rep;
      color = *dataptr & 0xf;
      rep = (*dataptr >> 4) & 0xf;
      dataptr++;
      while (rep-- > 0) {
	putpix(x, y, color, curpage);
	if (++x >= maxx) {
	  x = minx;
	  y++;
	}
      }
    }
    switchpage(curpage);
    regs.h.bh = curpage;
    regs.h.ah = 2;
    regs.x.dx = 0;
    int86(0x10, &regs, &regs);
    textcolor(15);
    printf("loop %d of %d    cell %d of %d", loop, nloops, cell, ncells);
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
	      if (loop < nloops) {
		loop++;
		cell = 1;
	      }
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
	      if (loop > 1) {
		loop--;
		cell = 1;
	      }
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
    }
  }
}

void viewpic(char far *data)
{
  char *ptr;
  static char startcolors[40] =
    {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
     0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0x88,
     0x88, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x88,
     0x88, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff,
     0x08, 0x91, 0x2a, 0x3b, 0x4c, 0x5d, 0x6e, 0x88};
  char colors[4][40], priorities[4][40];
  unsigned char code;
  int x, y;
  ptr = data;
  if (data == 0) return;
  blankscreen();
  priority = 0;
  col1 = col2 = 0;
  drawenable = 3;
  patcode = patnum = 0;
  for (x=0; x<4; x++) for (y=0; y<40; y++) {
    colors[x][y] = startcolors[y];
    priorities[x][y] = y & 0x0f;
  }
  textattr(15);
  gotoxy(1, 1);
  cprintf("Drawing...");
  while ((code = *(ptr++)) != 0xff) {
    switch (code) {
      case 0xf0: /* set color */
	code = *(ptr++);
	code = colors[code/40][code%40];
	col1 = (code >> 4) & 0x0f;
	col2 = code & 0x0f;
	drawenable |= 1;
	break;
      case 0xf1: /* turn off color draw */
	drawenable &= ~1;
	break;
      case 0xf2: /* set priority */
	code = *(ptr++);
	priority = priorities[code/40][code%40];
	drawenable |= 2;
	break;
      case 0xf3: /* turn off priority draw */
	drawenable &= ~2;
	break;
      case 0xf4: /* short-relative pattern */
	if (patcode & 0x20) patnum = *(ptr++) >> 1 & 0x7f;
	x = ((*ptr & 0xf0) << 4) | (0xff & ptr[1]);
	y = ((*ptr & 0x0f) << 8) | (0xff & ptr[2]);
	plotpattern(x, y);
	ptr += 3;
	while (*((unsigned char *)ptr) < 0xf0) {
	  if (patcode & 0x20) patnum = *(ptr++) >> 1 & 0x7f;
	  if (*ptr & 0x80) x -= ((*ptr & 0x70) >> 4);
	  else x += ((*ptr & 0x70) >> 4);
	  if (*ptr & 0x08) y -= (*ptr & 0x07);
	  else y += (*ptr & 0x07);
	  plotpattern(x, y);
	  ptr++;
	}
	break;
      case 0xf5: /* draw relative lines */
	curx = ((*ptr & 0xf0) << 4) | (0xff & ptr[1]);
	cury = ((*ptr & 0x0f) << 8) | (0xff & ptr[2]);
	ptr += 3;
	while (*((unsigned char *)ptr) < 0xf0) {
	  if (ptr[0] & 0x80) y = cury - (ptr[0] & 0x7f);
	  else y = cury + ptr[0];
	  x = curx+ptr[1];
	  ditherto(x, y);
	  ptr += 2;
	}
	break;
      case 0xf6: /* draw long lines */
	curx = ((*ptr & 0xf0) << 4) | (0xff & ptr[1]);
	cury = ((*ptr & 0x0f) << 8) | (0xff & ptr[2]);
	while (*((unsigned char *)ptr) < 0xf0) {
	  x = ((*ptr & 0xf0) << 4) | (0xff & ptr[1]);
	  y = ((*ptr & 0x0f) << 8) | (0xff & ptr[2]);
	  ditherto(x, y);
	  ptr += 3;
	}
	break;
      case 0xf7: /* draw short lines */
	curx = ((*ptr & 0xf0) << 4) | (0xff & ptr[1]);
	cury = ((*ptr & 0x0f) << 8) | (0xff & ptr[2]);
	ptr += 3;
	while (*((unsigned char *)ptr) < 0xf0) {
	  if (*ptr & 0x80) x = curx - ((*ptr & 0x70) >> 4);
	  else x = curx + ((*ptr & 0x70) >> 4);
	  if (*ptr & 0x08) y = cury - (*ptr & 0x07);
	  else y = cury + (*ptr & 0x07);
	  ditherto(x, y);
	  ptr++;
	}
	break;
      case 0xf8: /* fill */
	while (*((unsigned char *)ptr) < 0xf0) {
	  x = ((*ptr & 0xf0) << 4) | (0xff & ptr[1]);
	  y = ((*ptr & 0x0f) << 8) | (0xff & ptr[2]);
	  ditherfill(x, y);
	  ptr += 3;
	}
	break;
      case 0xf9: /* change pattern */
	patcode = *(ptr++) & 0x37;
	break;
      case 0xfa: /* absolute pattern */
	while (*((unsigned char *)ptr) < 0xf0) {
	  if (patcode & 0x20) patnum = *(ptr++) >> 1 & 0x7f;
	  x = ((*ptr & 0xf0) << 4) | (0xff & ptr[1]);
	  y = ((*ptr & 0x0f) << 8) | (0xff & ptr[2]);
	  plotpattern(x, y);
	  ptr += 3;
	}
	break;
      case 0xfb: /* set floor */
	code = *(ptr++);
	special = priorities[code/40][code%40];
	drawenable |= 4;
	break;
      case 0xfc: /* disable floor draw */
	drawenable &= ~4;
	break;
      case 0xfd: /* relative pattern */
	if (patcode & 0x20) patnum = *(ptr++) >> 1 & 0x7f;
	curx = (*ptr & 0xf0) << 4;
	cury = (*ptr & 0x0f) << 8;
	x = curx | (0xff & *(ptr+1));
	y = cury | (0xff & *(ptr+2));
	plotpattern(x, y);
	ptr += 3;
	while (*((unsigned char *)ptr) < 0xf0) {
	  if (patcode & 0x20) patnum = *(ptr++) >> 1 & 0x7f;
	  if (ptr[0] & 0x80) y -= (ptr[0] & 0x7f);
	  else y += ptr[0];
	  x += ptr[1];
	  plotpattern(x, y);
	  ptr += 2;
	}
	break;
      case 0xfe: /* define palette */
	code = *(ptr++);
	switch (code) {
	  case 0:
	    while ((code = *ptr) < 0xf0) {
	      colors[code/40][code%40] = *(ptr+1);
	      ptr += 2;
	    }
	    break;
	  case 1:
	    x = *(ptr++);
	    for (y=0; y<40; y++) colors[x][y] = *(ptr++);
	    break;
	  case 2:
	    x = *(ptr++);
	    for (y=0; y<40; y++) priorities[x][y] = *(ptr++);
	    break;
	  case 5:
	    ptr++;
	  default:
	    gotoxy(1, 1);
	    printf("Unknown palette %02x", code);
	    break;
	}
	break;
      default: ;
    }
    if (kbhit()) {
      code = getch();
      if (code == 27) return;
      if (code == '1') switchpage(0);
      else if (code == '2') switchpage(1);
      else if (code == '3') switchpage(2);
    }
  }
  gotoxy(1, 1);
  cprintf("<1-3> to select screen, <esc> to exit");
  while ((code = getch()) != 27) {
    if (code == '2') switchpage(1);
    else if (code == '3') switchpage(2);
    else switchpage(0);
  }
}

void viewdata(int dtype, char far *data)
{
  static unsigned char mainscreen[25*80*2];
  gettext(1, 1, tinfo.screenwidth, tinfo.screenheight, mainscreen);
  clrscr();
  switch (dtype) {
    case VIEW:
      graphmode();
      viewview(data);
      endgraph();
      break;
    case PIC:
      graphmode();
      viewpic(data);
      endgraph();
      break;
    case TEXT:
      viewtext(data);
      break;
    default: ;
  }
  textmode(C80);
  puttext(1, 1, tinfo.screenwidth, tinfo.screenheight, mainscreen);
}

void viewentry(unsigned long entry)
{
  char far *data;
  textcolor(7);
  textbackground(0);
  message("Reading...");
  data = readdata(entry);
  if (data == NULL) return;
  message("");
  viewdata(map[entry].id >> 11, data);
  farfree(data);
}

void viewfile(char *filename)
{
  char far *data;
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
  data = farmalloc(filelength(h));
  if (data == NULL) {
    textcolor(4);
    message("Not enough memory");
    return;
  }
  read(h, data, filelength(h));
  close(h);
  message("");
  viewdata(data[0]&15, data+2+data[1]);
  farfree(data);
}

#define menumark textcolor(1); textbackground(7); \
		 gotoxy(MENULEFT, menuptr); \
		 id = map[menuptr+menufirst-1].id; \
		 loc = map[menuptr+menufirst-1].location; \
		 cprintf("%8s.%03d %02d", types[id>>11], id & 0x7ff, loc >> 26)

#define menuunmark textcolor(7); textbackground(0); \
		   gotoxy(MENULEFT, menuptr); \
		   id = map[menuptr+menufirst-1].id; \
		   loc = map[menuptr+menufirst-1].location; \
		   cprintf("%8s.%03d %02d", types[id>>11], id & 0x7ff, loc >> 26)

#define MENUWIDTH 15
#define MENULEFT 1
unsigned long menufirst = 0;
int menuptr = 1;

void drawmenu()
{
  int k;
  unsigned id;
  unsigned long loc;
  textcolor(7);
  textbackground(0);
  for (k=1; k < tinfo.screenheight; k++) {
    gotoxy(MENULEFT, k);
    id = map[k+menufirst-1].id;
    loc = map[k+menufirst-1].location;
    if (id == 0xffff && menuptr >= k) menuptr = k-1;
    if (k+menufirst-1 >= maplen) cprintf("               ");
    else cprintf("%8s.%03d %02d", types[id>>11], id & 0x7ff, loc >> 26);
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
  textmode(C80);
  _setcursortype(_NOCURSOR);
  textbackground(0);
  textcolor(7);
  clrscr();
  boxprint(30, 2, 80, 24,
      "       Hermetic Software\n"
      "           presents\n"
      "     SCI DECODER/EGA v. 1.01\n\n"
      "    Copyright 1993 Carl Muckenhoupt\n"
      "All rights reserved.\n\n\n\n"
      "Use arrow keys to move through data\n"
      "ENTER to view data\n"
      "D to dump data to unencoded file\n"
      "L to view unencoded file\n"
      "ESC exits program"
    );
  gettextinfo(&tinfo);
  drawbox(MESSAGELEFT-1, MESSAGETOP-1, MESSAGERIGHT, MESSAGEBOTTOM);
  gotoxy(1, 1);
  drawmenu();
  while ((c = getch()) != 27) switch (c) {
    case 0:
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
	    if (map[menufirst+menuptr].id == 0xffff) continue;
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
	      map[menufirst+menuptr-1].id != 0xffff) menuptr++;
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
	  dtype = map[menufirst+menuptr-1].id >> 11;
	  for (k = menufirst; k > 0 && map[k].id >> 11 == dtype; k--);
	  if (k > 0) {
	    dtype = map[k].id >> 11;
	    while (k >= 0 && map[k].id >> 11 == dtype) k--;
	    k++;
	  }
	  else k = 0;
	  menufirst = k;
	  menuptr = 1;
	  drawmenu();
	  break;
	case 0x4d: /* right arrow */
	  dtype = map[menufirst+menuptr-1].id >> 11;
	  for (k = menufirst; map[k].id >> 11 == dtype; k++);
	  if (map[k].id != 0xffff) {
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
  textcolor(7);
  textbackground(0);
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
"Also available: SCI Decoder for Sierra's VGA games\r\n\n");
  textcolor(7);
}
