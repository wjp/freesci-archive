/***************************************************************************
 tools.c Copyright (C) 1999 Christoph Reichenbach


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

    Christoph Reichenbach (CJR) [jameson@linuxgames.com]

***************************************************************************/


#include <resource.h>


#ifdef WORDS_BIGENDIAN
gint16
getInt16(unsigned char* address)
{
  return (gint16)((((guint16)address[1]) << 8) | (address[0]));
}
#endif WORDS_BIGENDIAN



int
memtest(char *where, ...)
{
  va_list argp;
  int i;
  void *blocks[32];
  fprintf(stderr,"Memtesting ");

  va_start(argp, where);
  vfprintf(stderr, where, argp);
  va_end(argp);

  fprintf(stderr,"\n");
  for (i = 0; i < 31; i++) {
    blocks[i] = malloc(1 + i);
#ifdef HAVE_MEMFROB
    memfrob(blocks[i], 1 + i);
#else
    memset(blocks[i], 42, 1 + i);
#endif
  }
  for (i = 0; i < 31; i++)
    free(blocks[i]);

  for (i = 0; i < 31; i++) {
    blocks[i] = malloc(5 + i*5);
#ifdef HAVE_MEMFROB
    memfrob(blocks[i], 5 + i*5);
#else
    memset(blocks[i], 42, 5 + i*5);
#endif
  }
  for (i = 0; i < 31; i++)
    free(blocks[i]);

  for (i = 0; i < 31; i++) {
    blocks[i] = malloc(5 + i*100);
#ifdef HAVE_MEMFROB
    memfrob(blocks[i], 5 + i*100);
#else
    memset(blocks[i], 42, 5 + i*100);
#endif
  }
  for (i = 0; i < 31; i++)
    free(blocks[i]);

  for (i = 0; i < 31; i++) {
    blocks[i] = malloc(5 + i*1000);
#ifdef HAVE_MEMFROB
    memfrob(blocks[i], 5 + i * 1000);
#else
    memset(blocks[i], 42, 5 + i * 1000);
#endif
  }
  for (i = 0; i < 31; i++)
    free(blocks[i]);
fprintf(stderr,"Memtest succeeded!\n");
return 0;
}

