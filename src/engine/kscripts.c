/***************************************************************************
 kscripts.c.c Copyright (C) 1999 Christoph Reichenbach


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

#include <sciresource.h>
#include <engine.h>
#include <kernel_compat.h>

reg_t
read_selector(state_t *s, reg_t object, selector_t selector_id, char *file, int line)
{
	reg_t *address;

	if (lookup_selector(s, object, selector_id, &address, NULL) != SELECTOR_VARIABLE)
		return NULL_REG;
	else
		return *address;
}


void
write_selector(state_t *s, reg_t object, selector_t selector_id, reg_t value,
	       char *fname, int line)
{
	reg_t *address;

	if ((selector_id < 0) || (selector_id > s->selector_names_nr)) {
		SCIkwarn(SCIkWARNING, "Attempt to write to invalid selector %d of"
			 " object at "PREG" (%s L%d).\n", selector_id,
			 PRINT_REG(object), fname, line);
		return;
	}

	if (lookup_selector(s, object, selector_id, &address, NULL) != SELECTOR_VARIABLE)
		SCIkwarn(SCIkWARNING, "Selector '%s' of object at %04x could not be"
			 " written to (%s L%d)\n",
			 s->selector_names[selector_id], object, fname, line);
	else
		*address = value;

}

int
invoke_selector(state_t *s, reg_t object, int selector_id, int noinvalid, int kfunct,
		stack_ptr_t k_argp, int k_argc, /* Kernel function argp/argc */
		char *fname, int line, int argc, ...)
{
	va_list argp;
	int i;
	int framesize = 2 + 1 * argc;
	reg_t address;
	stack_ptr_t stackframe = k_argp + k_argc;

	exec_stack_t *xstack; /* Execution stack */

	stackframe[0] = make_reg(0, selector_id);  /* The selector we want to call */
	stackframe[1] = make_reg(0, argc); /* Argument count */

	if (lookup_selector(s, object, selector_id, NULL, &address) != SELECTOR_METHOD) {
		SCIkwarn(SCIkERROR, "Selector '%s' of object at "PREG" could not be invoked (%s L%d)\n",
			 s->selector_names[selector_id], PRINT_REG(object), fname, line);
		if (noinvalid == 0)
			KERNEL_OOPS("Not recoverable: VM was halted\n");
		return 1;
	}

	va_start(argp, argc);
	for (i = 0; i < argc; i++) {
		reg_t arg = va_arg(argp, reg_t);
		stackframe[2 + i] = arg; /* Write each argument */
	}
	va_end(argp);

	/* Write "kernel" call to the stack, for debugging: */
	xstack = add_exec_stack_entry(s, NULL_REG, NULL, NULL_REG,
				      k_argc, k_argp - 1, 0, NULL_REG,
				      s->execution_stack_pos);
	xstack->selector = -42 - kfunct; /* Evil debugging hack to identify kernel function */
	xstack->type = EXEC_STACK_TYPE_KERNEL;

	/* Now commit the actual function: */
	xstack = send_selector(s, object, object,
			       stackframe, framesize, stackframe);

	run_vm(s, 0); /* Start a new vm */

	--(s->execution_stack_pos); /* Get rid of the extra stack entry */

	return 0;
}


int
is_object(state_t *s, heap_ptr offset)
{
  if (offset & 1) /* objects are always at even addresses */
    return 0;
  else if (offset < 1000)
    return 0;
  else
    return (GET_HEAP(offset + SCRIPT_OBJECT_MAGIC_OFFSET) == SCRIPT_OBJECT_MAGIC_NUMBER);
}


/* kLoad(restype, resnr):
** Loads an arbitrary resource of type 'restype' with resource number 'resnr'
*/
void
kLoad(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
    int restype = UPARAM(0);
    int resnr = UPARAM(1);

    s->acc = ((restype << 11) | resnr); /* Return the resource identifier as handle */

    if (restype == sci_memory)/* Request to dynamically allocate hunk memory for later use */
      s->acc = kalloc(s, HUNK_TYPE_ANY, restype);

}

void
kLock(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
	int restype = UPARAM(0);
	int resnr = UPARAM(1);
	int state = UPARAM(2);
	resource_t *which;

	switch (state)
	{
	case 1 :
		scir_find_resource(s->resmgr, restype, resnr, 1);
		break;
	case 0 :
		which = scir_find_resource(s->resmgr, restype, resnr, 0);
		scir_unlock_resource(s->resmgr, which, resnr, restype);
		break;
	}
}

/* kUnload():
** Unloads an arbitrary resource of type 'restype' with resource numbber 'resnr'
*/
void
kUnLoad(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
    int restype = UPARAM(0);
    int resnr = UPARAM(1);

    if (restype == sci_memory)
      kfree(s, resnr);

}


void
kClone(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
#warning "Re-implement kClone()"
#if 0
  heap_ptr old_offs = UPARAM(0);
  heap_ptr new_offs;
  heap_ptr functareaptr;
  word species;
  int selectors;
  int object_size;
  int i;

  if (GET_HEAP(old_offs + SCRIPT_OBJECT_MAGIC_OFFSET) != SCRIPT_OBJECT_MAGIC_NUMBER) {
    SCIkwarn(SCIkERROR, "Attempt to clone non-object/class at %04x failed", old_offs);
    return;
  }

  SCIkdebug(SCIkMEM, "Attempting to clone from %04x\n", old_offs);

  selectors = UGET_HEAP(old_offs + SCRIPT_SELECTORCTR_OFFSET);

  object_size = 8 + 2 + (selectors * 2);
  /* 8: Pre-selector area; 2: Function area (zero overloaded methods) */

  new_offs = heap_allocate(s->_heap, object_size);

  if (!new_offs) {
    KERNEL_OOPS("Out of heap memory while cloning!\n");
    return;
  }

  new_offs += 2; /* Step over heap header */

  SCIkdebug(SCIkMEM, "New clone at %04x, size %04x\n", new_offs, object_size);

  memcpy(s->heap + new_offs, s->heap + old_offs + SCRIPT_OBJECT_MAGIC_OFFSET, object_size);
  new_offs -= SCRIPT_OBJECT_MAGIC_OFFSET; /* Center new_offs on the first selector value */

  PUT_HEAP(new_offs + SCRIPT_INFO_OFFSET, SCRIPT_INFO_CLONE); /* Set this to be a clone */

  functareaptr = selectors * 2 + 2; /* New function area */
  PUT_HEAP(new_offs + functareaptr - 2, 0); /* No functions */
  PUT_HEAP(new_offs + SCRIPT_FUNCTAREAPTR_OFFSET, functareaptr);

  species = GET_HEAP(new_offs + SCRIPT_SPECIES_OFFSET);
  /*  PUT_HEAP(new_offs + SCRIPT_SUPERCLASS_OFFSET, species); */
  PUT_HEAP(new_offs + SCRIPT_SUPERCLASS_OFFSET, old_offs); /* Offset of the original class */

  s->acc = new_offs; /* return the object's address */

  i = 0;
  while ((i < SCRIPT_MAX_CLONES) && (s->clone_list[i])) i++;

  if (i < SCRIPT_MAX_CLONES)
    s->clone_list[i] = new_offs; /* Log this clone */
  else SCIkwarn(SCIkWARNING, "Could not log clone at %04x\n", new_offs);

  /* Now, optionally set new selector values (late SCI0+ functionality) */
  if (argc>1)
    SCIkdebug(SCIkMEM, "Clone() called with extended functionality\n");

  for (i=1;i<argc;i+=2)
  {
    int selector = UPARAM(i);
    int value = UPARAM(i+1);

    write_selector(s, new_offs, selector, value, __FILE__, __LINE__);
  }
#endif
}


void
kDisposeClone(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
#warning "Re-implement kDisposeClone()"
#if 0
	heap_ptr offset = PARAM(0);
	int i;
	word underBits;

	if (GET_HEAP(offset + SCRIPT_OBJECT_MAGIC_OFFSET) != SCRIPT_OBJECT_MAGIC_NUMBER) {
		SCIkwarn(SCIkERROR, "Attempt to dispose non-class/object at %04x\n", offset);
		return;
	}

	if (GET_HEAP(offset + SCRIPT_INFO_OFFSET) != SCRIPT_INFO_CLONE) {
		/*  SCIkwarn("Attempt to dispose something other than a clone at %04x\n", offset); */
		/* SCI silently ignores this behaviour; some games actually depend on this */
		return;
	}

	underBits = GET_SELECTOR(offset, underBits);
	if (underBits) {
		SCIkwarn(SCIkWARNING,"Clone %04x was cleared with underBits set\n", offset);
	}
#if 0
	if (s->dyn_views) {  /* Free any widget associated with the clone */
		gfxw_widget_t *widget = gfxw_set_id(gfxw_remove_ID(s->dyn_views, offset), GFXW_NO_ID);

		if (widget && s->bg_widgets)
			s->bg_widgets->add(GFXWC(s->bg_widgets), widget);
	}
#endif

	i = 0;
	while ((i < SCRIPT_MAX_CLONES) && (s->clone_list[i] != offset)) i++;
	if (i < SCRIPT_MAX_CLONES)
		s->clone_list[i] = 0; /* un-log clone */
	else SCIkwarn(SCIkWARNING, "Could not remove log entry from clone at %04x\n", offset);

	offset += SCRIPT_OBJECT_MAGIC_OFFSET; /* Step back to beginning of object */

	heap_free(s->_heap, offset -2); /* -2 to step back on the heap block size indicator */
#endif
}


/* kScriptID(script, index):
** Returns script dispatch address index in the supplied script
*/
reg_t
kScriptID(state_t *s, int funct_nr, int argc, reg_t *argv)
{
	int script = KP_UINT(argv[0]);
	int index = KP_UINT(KP_ALT(1, NULL_REG));

	seg_id_t scriptid = script_get_segment(s, script, SCRIPT_GET_LOAD);
	script_t *scr;

	if (!scriptid)
		return NULL_REG;

	scr = &(s->seg_manager.heap[scriptid]->data.script);

	if (!scr->exports_nr) {
		SCIkdebug(SCIkERROR, "Script 0x%x does not have a dispatch table\n", script);
		return NULL_REG;
	}

	if (index > scr->exports_nr) {
		SCIkwarn(SCIkERROR, "Dispatch index too big: %d > %d\n",
			 index, scr->exports_nr);
		return NULL_REG;
	}

	return make_reg(scriptid, getUInt16((byte*)(scr->export_table + index)));
}


reg_t
kDisposeScript(state_t *s, int funct_nr, int argc, reg_t *argv)
{
	int script = argv[0].offset;

	script_uninstantiate(s, script);
}

static int
is_heap_object(state_t *s, reg_t pos)
{
#warning "Optimize me!"
	return obj_get(s, pos) != NULL;
}

reg_t
kIsObject(state_t *s, int funct_nr, int argc, reg_t *argv)
{
	if (argv[0].offset == 0xffff) /* Treated specially */
		return NULL_REG;
	else
		s->acc = is_heap_object(s, argv[0]);
}

reg_t
kRespondsTo(state_t *s, int funct_nr, int argc, reg_t *argv)
{
	reg_t obj = argv[0];
	int selector = KP_UINT(argv[1]);

	return make_reg(0, is_heap_object(s, obj)
			&& lookup_selector(s, obj, selector, NULL, NULL) != SELECTOR_NONE);
}

