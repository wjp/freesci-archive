/* This file contains all the routines common to sde and sdv.
   A motley assortment. */

#include <fcntl.h>
#include <stdio.h>

char *gamedir = ".";
void gotodir(char *path) {
/* In a previous version of this program, this function would change the
   current working directory (via dos interrupt). I have since learned
   that this is bad - suppose the user is reading from floppies, and
   wants to save uncompressed data? Anyway, all it does now is alter
   a new global called gamedir. */
  char *ptr;
  for (ptr=path; *ptr != '\0'; ptr++);
  ptr--;
  if (*ptr == '\\') *ptr='\0';
  gamedir = path;
}

struct text_info tinfo; /* TCC structure, saves info about screen size */

char err[80], /* buffer for error messages */
     *types[] = {"view","pic","script","text","sound","memory","vocab",
		"font","cursor","patch", "bitmap", "palette", "cdaudio",
		"audio", "sync", "message", "map", "heap"},
     *types3[] = {"v56","p56","scr","tex","snd","   ","voc","fon","cur",
		  "pat","bit","pal","cda","aud","syn","msg","map","hep"};

#define VIEW 0
#define PIC 1
#define SCRIPT 2
#define TEXT 3
#define SOUND 4
#define MEMORY 5
#define VOCAB 6
#define FONT 7
#define CURSOR 8
#define PATCH 9
#define BITMAP 10
#define PALETTE 11
#define CDAUDIO 12
#define AUDIO 13
#define SYNC 14
#define MESSAGE 15
#define MAP 16
#define HEAP 17
/* oh, you think I should have used an enumerated type? You're probably right. */

#define MESSAGETOP 20    /* dimensions of message window */
#define MESSAGELEFT 22
#define MESSAGERIGHT tinfo.screenwidth-2
#define MESSAGEBOTTOM tinfo.screenheight-1

void drawbox(left, top, right, bottom)
/* makes a box of IBM graphics characters. Fairly straightforward. */
{
  register int k; /* Excuse me. When I wrote this, I was going register crazy. */
  /* draw the top */
  gotoxy(left, top);
  putch(0xda);
  for (k=left+1; k<right; k++) putch(0xc4);
  putch(0xbf);
  /* draw the sides */
  for (k=top+1; k<bottom; k++) {
    gotoxy(left, k);
    putch(0xb3);
    gotoxy(right, k);
    putch(0xb3);
  }
  /* draw the bottom */
  gotoxy(left, bottom);
  putch(0xc0);
  for (k=left+1; k<right; k++) putch(0xc4);
  putch(0xd9);
}

char *boxprint(int let, int top, int right, int bottom, char *text);
/* boxprint will be a routine for writing text in a box, breaking
   lines only at spaces. */

void message(char *m)
/* writes message to message box */
{
  window(MESSAGELEFT, MESSAGETOP, MESSAGERIGHT-1, MESSAGEBOTTOM-1);
  clrscr();
  window(1, 1, tinfo.screenwidth, tinfo.screenheight);
  boxprint(MESSAGELEFT, MESSAGETOP, MESSAGERIGHT, MESSAGEBOTTOM, m);
}

char *inquire(char *m)
/* prompts for input, in the message box. */
{
  static char line[80]; /* input buffer */
  int c=0;
  window(MESSAGELEFT, MESSAGETOP, MESSAGERIGHT-1, MESSAGEBOTTOM-1);
  clrscr();
  window(1, 1, tinfo.screenwidth, tinfo.screenheight);
  boxprint(MESSAGELEFT, MESSAGETOP, MESSAGERIGHT, MESSAGEBOTTOM, m);
  putch('>');
  _setcursortype(_NORMALCURSOR);
  while ((line[c++] = getchar()) != '\n');
  _setcursortype(_NOCURSOR);
  line[--c] = 0;
  return line;
}

char *boxprint(int left, int top, int right, int bottom, char *text)
{  /* print text inside box, return ptr to start of unused portion */
  int row=top, column=left, c, d;
  static char word[81]; /* buffer for single word */
  gotoxy(column, row);
  while (row < bottom && *text != 0) {
    c = 0;
    /* The following puts a single word up, avoiding line breaks. */
    while (*text > ' ') { /* All non-whitespace printable chars are > ' ' */
      word[c++] = *text;
      if (column+c >= right) { /* if attempt to print past edge, */
	row++;                 /* do carriage return */
	column = left;
	gotoxy(column, row);
	if (row >= bottom) {
	  text -= c;
	  return text;
	}
      }
      text++;
    }
    word[c] = 0; /* put the word on the screen */
    cputs(word);
    column += c;
    while (*text <= ' '){ /* Now deal with whitespace. */
      switch (*text) {
	case 0:
	  return text;
	case ' ':
	  putch(' ');
	  column++;
	  if (column < right) break;
	case '\n':
	  column = left;
	  row++;
	  gotoxy(column, row);
	  if (row >= bottom) return text;
	default:
	  break;
      }
      text++;
    }
  }
  return text; /* At this point, text points to the unprinted portion */
}

void viewtext(char *text)
/* View text data from the game. Since this does not depend on graphics,
   we can use it for both sdv and sde. */
/* NOTE: It wouldn't be hard to alter this so you could page backwards,
   but I don't feel like it right now. */
{
  char *next, *end; /* next thing to print and end of text */
  int top = 1; /* top of unused portion of screen */
  if (text == 0) return;
  end = text + DATALEN;
  next = text;
  while (next < end) {
    if (top >= tinfo.screenheight-1) {
       textcolor(1); /* print "more" message in invesre video */
       textbackground(7);
       cputs("\x0d\nPRESS ANY KEY TO CONTINUE...");
       textcolor(7);
       textbackground(0);
       if (getch() == 27) { /* ESC to quit, as always */
	 return;
       }
       clrscr();
       top = 1;
    }
    next = boxprint(1, top, tinfo.screenwidth+1, tinfo.screenheight-1, next)+1;
    top = wherey()+1;
  }
  gotoxy(1, tinfo.screenheight-1);
  textcolor(1);
  textbackground(7);
  cputs("\x0d\nPRESS ANY KEY TO EXIT");
  getch();
}


