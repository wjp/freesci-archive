#include <script.h>
#include <vm.h>



/*Here comes implementations of some kernel functions. I haven't found any
 *documentation on these, except for their names in vocab.999.
 */

#if 0

int kClone(state* s)
{
  instance* old=instance_map[getInt16(s->heap->data+s->acc)];
  instance* new=instantiate(s->heap, old->class);
  int i;
  if(new==0) return 1;

  /*copy the selector values...*/
  for(i=0; i<old->class->selector_count*2; i++)
    new->heap[new->offset+i+2]=old->heap[old->offset+i+2];
  /*...but use the correct id...*/
  putInt16(new->heap+new->offset, new->id);
  /*...mark as clone (ie set bit 0 in -info-)...*/
  putInt16(new->heap+new->offset+6,
	   (getInt16(new->heap+new->offset+6)|1));
  /*...and use old's id as superclass*/
  putInt16(new->heap+new->offset+4, old->id);

  /*set acc to point to the new instance*/
  s->acc=new->offset;
  s->sp-=4;
  return 0;
}

int kDisposeClone(state* s)
{
  int id=getInt16(s->heap->data+s->acc);
  instance* i;
  /*FIXME: check acc for sanity*/

  i=instance_map[id];
  if(h_free(s->heap, i->offset, (i->class->selector_count+1)*2)) return 1;

  /*shuffle the instance_map*/
  instance_map[id]=instance_map[max_instance--];
  putInt16(s->heap->data+instance_map[id]->offset, id);

  s->sp-=4;
  return 0;
}

int kIsObject(state* s)
{
  int id=getInt16(s->heap->data+s->acc);

  if(id<=0x4000 && instance_map[id]->offset==s->acc) s->acc=1;
  else s->acc=0;

  s->sp-=4;
  return 0;
}

#endif /* false */


#define PARAM(x) getInt16(s->heap + argv + ((x)*2))


void
kstub(state_t *s, int funct_nr, int argc, heap_ptr argv)
{
  int i;

  sciprintf("Stub: %s[%x](", s->kernel_names[funct_nr], funct_nr);

  for (i = 0; i < argc; i++) {
    sciprintf("%04x", PARAM(i));
    if (i+1 < argc) sciprintf(", ");
  }
  sciprintf(")\n");
}



struct {
  char *functname; /* String representation of the kernel function as in script.999 */
  kfunct *kernel_function; /* The actual function */
} kfunct_mappers[] = {
  {0,0} /* Terminator */
};


void
script_map_kernel(state_t *s)
{
  int functnr;

  s->kfunct_table = malloc(sizeof(kfunct *) * s->kernel_names_nr);

  for (functnr = 0; functnr < s->kernel_names_nr; functnr++) {
    int seeker, found = -1;

    for (seeker = 0; (found == -1) && kfunct_mappers[seeker].functname; seeker++)
      if (strcmp(kfunct_mappers[seeker].functname, s->kernel_names[functnr]) == 0)
	found = seeker; /* Found a kernel function with the same name! */

    if (found == -1) {

      sciprintf("Warning: Kernel function %s[%x] unmapped\n", s->kernel_names[functnr], functnr);
      s->kfunct_table[functnr] = kstub;

    } else s->kfunct_table[functnr] = kfunct_mappers[found].kernel_function;

  } /* for all functions requesting to be mapped */

}
