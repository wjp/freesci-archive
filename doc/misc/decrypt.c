/*  This will be the least comprehensible of the files.  Parts of it
are more-or-less direct transcriptions of the object code from the Sierra
games, modified enough to avoid copyright infringement.  Such parts
can be identified by the use of variable names like "L14", a sure sign
that I don't understand myself what that variable is for.
Surprisingly enough, however, I have figured out the main decryption
routine. 

This file includes the decryption/decompression routines. */


#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>

extern unsigned int datainfo[4];

/*****************  Decryption  *******************************/

#define MAXBIT 0x2000
static unsigned int whichbit = MAXBIT; /* records our place in bit buffer */

static unsigned int getbits(int numbits, int handle)
/* gets the next numbits bits from file, low bit first */
{
  static unsigned char bits[(MAXBIT>>3)+3];
  int place; /* indicates location within byte */
  unsigned long bitstring;
  if (whichbit >= MAXBIT) {
    whichbit -= MAXBIT;
    read(handle, bits, (MAXBIT >> 3)+3);
    lseek(handle, -3L, SEEK_CUR);
  }
  place = whichbit >> 3;
  bitstring = bits[place+2] | (long)(bits[place+1])<<8
	      | (long)(bits[place])<<16;
  bitstring >>= 24-(whichbit & 7)-numbits;
  bitstring &= 0xffffffffUL >> (32-numbits);
  /* Okay, so this could be made faster with a table lookup.
     It doesn't matter. It's fast enough as it is. */
  whichbit += numbits;
  return bitstring;
}

/* Our first decompression method is a subtitutional coding. I don't know
   name, but it's the one where tokens with high bit 0 are literal,
   and ones with high bit 1 are indexes to a pair of tokens already
   seen. There are some later decryption types that use it, making
   it necessary to leave it and resume where you left off later.
   This made things a little trickier, but I use the global 
   'decryptstart' to indicate where to begin again. */

struct tokenlist {
  unsigned char data;
  int next;
} tokens[0x1004];

static char stak[0x1014], lastchar;
static int stakptr;
static unsigned int numbits, bitstring, lastbits, decryptstart;
static int curtoken, endtoken;

void decryptinit()
{
  lastchar = lastbits = bitstring = whichbit = stakptr = 0;
  numbits = 9;
  whichbit = MAXBIT;
  curtoken = 0x102;
  endtoken = 0x1ff;
  decryptstart = 0;
}

void decrypt(unsigned int length, char  *buffer, int resh)
{
  static int token;
  while(length != 0) switch (decryptstart) {
    case 0:
    case 1:
      bitstring = getbits(numbits, resh);
      if (bitstring == 0x101) { /* found end-of-data signal */
	decryptstart = 4;
	return;
      }
      if (decryptstart == 0) { /* first char */
	decryptstart = 1;
	lastbits = bitstring;
	*(buffer++) = lastchar = (bitstring & 0xff);
	if (--length != 0) continue;
	return;
      }
      if (bitstring == 0x100) { /* start-over signal */
	numbits = 9;
	endtoken = 0x1ff;
	curtoken = 0x102;
	decryptstart = 0;
	continue;
      }
      token = bitstring;
      if (token >= curtoken) { /* index past current point */
	token = lastbits;
	stak[stakptr++] = lastchar;
      }
      while ((token > 0xff)&&(token < 0x1004)) { /* follow links back in data */
	stak[stakptr++] = tokens[token].data;
	token = tokens[token].next;
      }
      lastchar = stak[stakptr++] = token & 0xff;
    case 2:
      while (stakptr > 0) { /* put stack in buffer */
	*(buffer++) = stak[--stakptr];
	length--;
	if (length == 0) {
	  decryptstart = 2;
	  return;
	}
      }
      decryptstart = 1;
      if (curtoken <= endtoken) { /* put token into record */
if (debug) fprintf(stderr,"[%i]", curtoken);
	tokens[curtoken].data = lastchar;
	tokens[curtoken].next = lastbits;
	curtoken++;
	if (curtoken == endtoken && numbits != 12) {
	  numbits++;
	  endtoken *= 2;
	  endtoken++;
	}
      }
      lastbits = bitstring;
      continue; /* When are "break" and "continue" synonymous? */
    case 4:
      return;
  }
}

static unsigned char buf[0x600];

void decrypt3(unsigned char *data, int resh)
/* This calls decrypt, but rearranges the resulting data somehow
   that is specific to the data type it's reading. */
{
  static unsigned char buf14[0x14];
  static unsigned int buf200[0x200];
  unsigned char b, *place, *place14 = buf14, *charto, *chartoend;
  unsigned int *placeto;
  unsigned int k, L16, L18, L1a, L1c, L1e;
  unsigned int *L2, *L8 = buf200;
  unsigned char *L14;
  decryptinit();
  decrypt(2, (char *)&k, resh);
  place = data+datainfo[3]-k;
  decrypt(k, place, resh);
  decrypt(*(int *)(place+8)*2, (char *)buf200, resh);
  placeto = (unsigned int *)data;
  L18 = *(place++);
  *(placeto++) = 0x8000 | L18;
  L16 = *(place++);
  *(placeto++) = *(int *)place;
  place+=2;
  *(placeto++) = *(int *)place;
  place+=2;
  *(placeto++) = *(int *)place;
  place += 4;
  L2 = placeto;
  placeto += L18;
  L14 = place+L16;
  L1e = *((int *)(data+2));
  L1a = L18;
  while (L16-- != 0) *(place14++) = *(place++);
  place14 = buf14;
  while (L1a-- != 0) {    /* decode the group indices */
    if (L1e & 1) {
      *L2 = *(L2-1);
      L2++;
    }
    else {
      *(L2++) = (char *)placeto - (char *)data;
      *(placeto++) = L1c = *(place14++);
      *(placeto++) = 0;
      charto = (unsigned char *)(placeto+L1c);
      while (L1c-- != 0) {
	*(placeto++) = charto-data;
	*(charto++) = *(L14++);
	*(charto++) = *(L14++);
	*(charto++) = *(L14++);
	*(charto++) = *(L14++);
	*(charto++) = *(L14++);
	*(charto++) = *(L14++);
	*(charto++) = *(L14++);
	*(charto++) = 0;
	charto += *(L8++);
      }
      placeto = (unsigned int *)charto;
    }
    L1e >>= 1;
  }
  decrypt(0x600, buf, resh);
  k = 0x600;
  place = buf;
  placeto = (unsigned int *)data;
  L1e = *(placeto+1);
  L2 = placeto+4;
  L8 = buf200;
  L1a = L18;
  while (L1a-- != 0) {
    if (L1e & 1) L2++;
    else {
      placeto = (unsigned int *)(data+*(L2++));
      L1c = *placeto;
      placeto += 2;
      while (L1c-- != 0) {
	charto = data+*(placeto++)+8;
	chartoend = charto+*(L8++);
	while (charto < chartoend) {
	  b = *(charto++) = *(place++);
	  if (--k == 0) {
	    decrypt(0x600, buf, resh);
	    k = 0x600;
	    place = buf;
	  }
	  if (b >= 0xc0) continue;
	  else if (b >= 0x80) charto++;
	  else charto += b;
	}
      }
    }
    L1e >>= 1;
  }
  placeto = (unsigned int *)data;
  L1e = *(placeto+1);
  L2 = placeto+4;
  L8 = buf200;
  L1a = L18;
  while (L1a-- != 0) {
    if (L1e & 1) L2++;
    else {
      placeto = (unsigned int *)(data+*(L2++));
      L1c = *placeto;
      placeto += 2;
      while (L1c-- != 0) {
	charto = data+*(placeto++)+8;
	chartoend = charto+*(L8++);
	while (charto < chartoend) {
	  b = *(charto++);
	  if (b >= 0xc0) continue;
	  else if (b >= 0x80) {
	    *(charto++) = *(place++);
	    if (--k == 0) {
	      decrypt(0x600, buf, resh);
	      k = 0x600;
	      place = buf;
	    }
	  }
	  else while (b-- > 0) {
	    *(charto++) = *(place++);
	    if (--k == 0) {
	      decrypt(0x600, buf, resh);
	      k = 0x600;
	      place = buf;
	    }
	  }
	}
      }
    }
  L1e >>= 1;
  }
  if (*(int *)(data+6)) {
    *(charto++) = 'P';
    *(charto++) = 'A';
    *(charto++) = 'L';
    for (k=0; k<0x100; k++) *(charto++) = k;
  }
}

void decrypt4(unsigned char *data, int resh)
{
  unsigned int place, bufplace, after, pal1start, pal1end, pal2end, L10, L12;
  decryptinit();
  decrypt(6, data, resh);
  after = ((int *)data)[0];
  L10 = ((int *)data)[2];
  place = pal1start = ((int *)data)[1];
  data[place++] = 0xfe;
  data[place++] = 0x01;
  data[place++] = 0x00;
  data[place++] = 0x00;
  data[place++] = 0x00;
  data[place++] = (after+8)&0xff;
  data[place++] = (after+8)>>8;
  decrypt(7, data+place, resh);
  place += 7;
  data[place++] = 0;
  pal1end = place;
  data[0] = 0xfe;
  data[1] = 0x02;
  for (place=2; place < 0x102; place++) data[place] = place-2;
  data[place++]=0;
  data[place++]=0;
  data[place++]=0;
  data[place++]=0;
  decrypt(0x400, data+place, resh);
  place += 0x400;
  pal2end = place;
  if (pal2end != pal1start) decrypt(pal1start - pal2end, data+place, resh);
  place = pal1end+after;
  if (place != datainfo[3]) decrypt(datainfo[3]-place, data+place, resh);
  place = L12 = pal1end+after-L10;
  decrypt(L10, data+place, resh);
  decrypt(0x600, buf, resh);
  bufplace = 0;
  place = pal1end;
  while (place < pal1end+after) {
    unsigned int k;
    k = data[place++] = buf[bufplace++];
    if (bufplace == 0x600) {
      decrypt(0x600, buf, resh);
      bufplace = 0;
    }
    if (k>0xc0) continue;
    else if (k>0x80) data[place++]=data[L12++];
    else while (k-- > 0) data[place++]=data[L12++];
  }
}

