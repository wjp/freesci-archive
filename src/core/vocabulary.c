/* Modified 07/18/99 by Christoph Reichenbach:
** - added vocabulary_free_snames();
*/


#include <string.h>
#include "resource.h"
#include "vocabulary.h"

int getInt(unsigned char* d)
{
  return d[0] | (d[1]<<8);
}

int* vocabulary_get_classes(int* count)
{
  resource_t* r;
  int *c, i;

  if((r=findResource(sci_vocab, 996))==0) return 0;

  c=malloc(sizeof(int)*r->length/2);
  for(i=2; i<r->length; i+=4)
    {
      c[i/4]=getInt(r->data+i);
    }
  *count=r->length/4;

  return c;
}

char** vocabulary_get_snames()
{
  char** t;
  int count;
  int i;

  resource_t* r=findResource(sci_vocab, 997);

  count=getInt(r->data);
  t=malloc(sizeof(char*)*(count+1));

  for(i=0; i<count; i++)
    {
      int offset=getInt(r->data+2+i*2);
      int len=getInt(r->data+offset);
      t[i]=malloc(len+1);
      strncpy(t[i], r->data+offset+2, len);
      t[i][len]='\0';
    }

  t[i]=0;
  
  return t;
}

void
vocabulary_free_snames(char **snames_list)
{
  int pos = 0;

  while (snames_list[pos])
    free(snames_list[pos++]);

  free(snames_list);
}

opcode* vocabulary_get_opcodes()
{
  opcode* o;
  int count, i=0;
  resource_t* r=findResource(sci_vocab, 998);

  count=getInt(r->data);

  o=malloc(sizeof(opcode)*256);
  for(i=0; i<count; i++)
    {
      int offset=getInt(r->data+2+i*2);
      int len=getInt(r->data+offset)-2;
      o[i].type=getInt(r->data+offset+2);
      o[i].number=i;
      o[i].name=malloc(len+1);
      strncpy(o[i].name, r->data+offset+4, len);
      o[i].name[len]='\0';
#ifdef VOCABULARY_DEBUG
      printf("Opcode %02X: %s, %d\n", i, o[i].name, o[i].type);
#endif
    }
  for(i=count; i<256; i++)
    {
      o[i].type=0;
      o[i].number=i;
      o[i].name=malloc(strlen("undefined")+1);
      strcpy(o[i].name, "undefined");
    }
  return o;
}


/* Alternative kernel func names retriever. Required for KQ1/SCI (at least). */
static char** _vocabulary_get_knames0alt(int *names, resource_t *r)
{
  int mallocsize = 32;
  char **retval = malloc(sizeof (char *) * mallocsize);
  int i = 0, index = 0;

  while (index < r->length) {

    int slen = strlen(r->data + index) + 1;

    retval[i] = malloc(slen);
    memcpy(retval[i++], r->data + index, slen);
    /* Wouldn't normally read this, but the cleanup code wants to free() this */

    index += slen;

    if (i == mallocsize)
      retval = realloc(retval, sizeof(char *) * (mallocsize <<= 1));

  }
  fprintf(stderr,"Done\n");

  *names = i - 1;
  retval = realloc(retval, sizeof(char *) * i);

  return retval;
}


static char** vocabulary_get_knames0(int* names)
{
  char** t;
  int count, i, index=2;
  resource_t* r=findResource(sci_vocab, 999);
  
  count=getInt(r->data);

  if (count > 1023)
    return _vocabulary_get_knames0alt(names, r);

  t=malloc(sizeof(char*)*(count+1));
  for(i=0; i<count; i++)
    {
      int offset=getInt(r->data+index);
      int len=getInt(r->data+offset);
      //fprintf(stderr,"Getting name %d of %d...\n", i, count);
      index+=2;
      t[i]=malloc(len+1);
      strncpy(t[i], r->data + offset + 2, len);
      t[i][len]='\0';
    }
  t[count]=0;
  *names=count;
  return t;
}

/*NOTE: Untested*/
static char** vocabulary_get_knames1()
{
  char** t;
  int size=128, used=0, pos=0;
  resource_t* r=findResource(sci_vocab, 999);

  while(pos<r->length)
    {
      int len;
      if(used==size-1)
	{
	  size*=2;
	  t=realloc(t, size*sizeof(char*));
	}
      len=strlen(r->data+pos);
      t[used]=malloc(len+1);
      strcpy(t[used], r->data+pos);
      used++;
      pos+=len+1;
    }
  t[used]=0;
  return t;
}

char** vocabulary_get_knames(int* count)
{
	switch(sci_version)
	{
		case SCI_VERSION_0:
		case SCI_VERSION_01: return vocabulary_get_knames0(count);
		case SCI_VERSION_1:
		case SCI_VERSION_32: return vocabulary_get_knames1(count);
		default: return 0;
	}
}

void vocabulary_free_knames(char** names)
{
	int i=0;
	while(names[i]) free(names[i++]);
	free(names);
}
