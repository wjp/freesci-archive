#include <script.h>
#include <vm.h>




int script_debug_flag = 0; /* Defaulting to no debug mode */
int script_abort_flag = 0; /* Set to 1 to abort execution */
int script_exec_stackpos = -1; /* 0 means that one script is active */

script_exec_stack_t script_exec_stack[SCRIPT_MAX_EXEC_STACK];

#if 0
instance* instantiate(heap_t* h, object* o)
{
  instance* i;
  int offset=h_malloc(h, (o->selector_count+1)*2);
  if(offset==-1) return 0;

  i=malloc(sizeof(instance));
  if(i==0) return 0;
  i->id=max_instance++;
  i->class=o;
  i->heap=h->data;
  i->offset=offset;
  instance_map[i->id]=i;
  return i;
}
#endif /* false */

int
script_error(state *s, heap_ptr *pc, heap_ptr *sp, heap_ptr *pp, heap_ptr *objp, char *reason,
	     int nonfatal) /* set nonfatal to 1 if returning should be allowed */
{
  sciprintf("Script error: %s at 0x%04x\n", reason, *pc);
  script_debug(s, pc, sp, pp, objp, nonfatal);
  return 0;
}

#define putInt16(bytes, value) do{\
(bytes)[0]=(value>>8)&0xFF;\
(bytes)[1]=value&0xFF;\
} while(0)


/* Operating on the steck */
#define PUSH(v) putInt16(s->heap + (sp += 2), (v))
#define POP() ((gint16)(getInt16(s->heap + (sp-=2) + 2)))

/* Getting instruction parameters */
#define GET_OP_BYTE() (s->heap[pc++])
#define GET_OP_WORD() (getInt16(s->heap + (pc += 2) - 2))
#define GET_OP_FLEX() ((opcode & 1)? GET_OP_BYTE() : GET_OP_WORD())
#define GET_OP_SIGNED_BYTE() ((gint8)(s->heap[pc++]))
#define GET_OP_SIGNED_WORD() ((gint16)(getInt16(s->heap + (pc += 2) - 2)))
#define GET_OP_SIGNED_FLEX() ((opcode & 1)? GET_OP_SIGNED_BYTE() : GET_OP_SIGNED_WORD())

#define GET_HEAP(address) ((address) < 800)? \
script_error(s, &pc, &sp, &pp, &objp, "Heap address space violation on read", 0) \
: getInt16(s->heap + (address))

#define PUT_HEAP(address, value) if ((address) < 800) \
script_error(s, &pc, &sp, &pp, &objp, "Heap address space violation on write", 0); \
else { s->heap[address] = (value) &0xff; s->heap[(address) + 1] = ((value) >> 8) & 0xff;}


void
executeMethod(state *s, word script, word pubfunct, heap_ptr sp, word argc, heap_ptr argp)
{
  /* FIXME */
}

void
execute(state *s, heap_ptr pc, heap_ptr sp, heap_ptr objp, word argc, heap_ptr argp)
{
  gint16 temp, temp2, temp3;
  gint16 pp = sp;
  heap_ptr local_vars = getInt16(s->heap + SCRIPT_LOCALVARPTR_OFFSET);

  heap_ptr variables[4] =
  { s->global_vars, local_vars, pp, argp }; /* Offsets of global, local, temp, and param variables */

  heap_ptr selector_offset = objp + SCRIPT_SELECTOR_OFFSET;

  /* Start entering debug information into call stack debug blocks */
  ++script_exec_stackpos;
  script_exec_stack[script_exec_stackpos].objpp = &objp;
  script_exec_stack[script_exec_stackpos].pcp = &pc;
  script_exec_stack[script_exec_stackpos].spp = &sp;
  script_exec_stack[script_exec_stackpos].ppp = &pp;
  script_exec_stack[script_exec_stackpos].argcp = &argc;
  script_exec_stack[script_exec_stackpos].argpp = &argp;

  while (1) {
    heap_ptr old_pc = pc;
    byte opcode = GET_OP_BYTE(); /* Get opcode */
    int var_number; /* See description below */

    if (script_abort_flag)
      return; /* Emergency */
    if (script_debug_flag)
      script_debug(s, &pc, &sp, &pp, &objp, 1);
    /* Debug if this has been requested */

    if (sp < s->heap_base)
      script_error(s, &pc, &sp, &pp, &objp, "Absolute stack underflow", 0);

    if (sp < pp)
      script_error(s, &pc, &sp, &pp, &objp, "Relative stack underflow", 1);

    if (pc < 800)
      script_error(s, &pc, &sp, &pp, &objp, "Program Counter gone astray", 0);

    switch (opcode >> 1) {
    case 0x00: /* bnot */
      s->acc ^= 0xffff;
      break;
    case 0x01: /* add */
      s->acc = POP() + s->acc;
      break;
    case 0x02: /* sub */
      s->acc = POP() - s->acc;
      break;
    case 0x03: /* mul */
      s->acc = POP() * s->acc;
      break;
    case 0x04: /* div */
      s->acc = s->acc? POP() / s->acc : 0;
      break;
    case 0x05: /* mod */
      s->acc = (s->acc > 0)? POP() % s->acc : 0;
      break;
    case 0x06: /* shr */
      s->acc = POP() >> s->acc;
      break;
    case 0x07: /* shl */
      s->acc = POP() << s->acc;
      break;
    case 0x08: /* xor */
      s->acc = POP() ^ s->acc;
      break;
    case 0x09: /* and */
      s->acc = POP() & s->acc;
      break;
    case 0x0a: /* or */
      s->acc = POP() | s->acc;
      break;
    case 0x0b: /* neg */
      s->acc = -(s->acc);
      break;
    case 0x0c: /* not */
      s->acc = !s->acc;
      break;
    case 0x0d: /* eq? */
      s->acc = s->prev = (POP() == s->acc);
      break;
    case 0x0e: /* ne? */
      s->acc = s->prev = (POP() != s->acc);
      break;
    case 0x0f: /* gt? */
      s->acc = s->prev = (POP() > s->acc);
      break;
    case 0x10: /* ge? */
      s->acc = s->prev = (POP() >= s->acc);
      break;
    case 0x11: /* lt? */
      s->acc = s->prev = (POP() < s->acc);
      break;
    case 0x12: /* le? */
      s->acc = s->prev = (POP() <= s->acc);
      break;
    case 0x13: /* ugt? */
      s->acc = s->prev = ((guint16)(POP()) > ((guint16)(s->acc)));
      break;
    case 0x14: /* uge? */
      s->acc = s->prev = ((guint16)(POP()) >= ((guint16)(s->acc)));
      break;
    case 0x15: /* ult? */
      s->acc = s->prev = ((guint16)(POP()) < ((guint16)(s->acc)));
      break;
    case 0x16: /* ule? */
      s->acc = s->prev = ((guint16)(POP()) <= ((guint16)(s->acc)));
      break;
    case 0x17: /* bt */
      if (s->acc) pc += GET_OP_FLEX();
      break;
    case 0x18: /* bnt */
      if (!s->acc) pc += GET_OP_FLEX();
      break;
    case 0x19: /* jmp */
      pc += GET_OP_FLEX();
      break;
    case 0x1a: /* ldi */
      s->acc = GET_OP_FLEX();
      break;
    case 0x1b: /* push */
      PUSH(s->acc);
      break;
    case 0x1c: /* pushi */
      PUSH(GET_OP_FLEX());
      break;
    case 0x1d: /* toss */
      POP();
      break;
    case 0x1e: /* dup */
      temp = POP();
      PUSH(temp);
      PUSH(temp); /* Gets compiler-optimized */
      break;
    case 0x1f: /* link */
      sp += (GET_OP_FLEX()) * 2;
      break;
    case 0x20: /* call */
      temp = GET_OP_FLEX(); /* relative position */
      temp2 = GET_OP_BYTE(); /* frame size */
      execute(s, pc + temp, sp, objp, getInt16(s->heap + (sp - temp2 - 2)), sp - temp2);
      break;
    case 0x21: /* callk */
      /* FIXME */
      break;
    case 0x22: /* callb */
      temp = GET_OP_FLEX();
      temp2 = GET_OP_BYTE();
      executeMethod(s, 0, temp, sp, getInt16(s->heap + (sp - temp2 - 2)) , sp - temp2);
      break;
    case 0x23: /* calle */
      temp = GET_OP_FLEX();
      temp2 = GET_OP_BYTE();
      temp3 = GET_OP_BYTE();
      executeMethod(s, temp, temp2, sp, getInt16(s->heap + (sp - temp3 - 2)) , sp - temp3);
      break;
    case 0x24: /* ret */
      --script_exec_stackpos; /* Go back one exec stack level */
      return; /* Hard return */
      break;
    case 0x25: /* send */
      /* FIXME */
      break;
    case 0x28: /* class */
      /* FIXME */
      break;
    case 0x2a: /* self */
      /* FIXME */
      break;
    case 0x2b: /* super */
      /* FIXME */
      break;
    case 0x2c: /* &rest */
      temp = GET_OP_FLEX() - 1; /* First argument (we start counting at 0) */
      temp2 = argp + (temp << 1); /* Pointer to the first argument to &restore */
      for (; temp < argc; temp++) {
	PUSH(getInt16(s->heap + temp2));
	temp2 += 2;
      }
      break;
    case 0x2d: /* lea */
      temp = GET_OP_FLEX() >> 1;
      var_number = temp & 0x03; /* Get variable type */

      temp2 = variables[var_number]; /* Get variable block offset */
      if (temp & 0x08)
	temp2 += s->acc; /* Add accumulator offset if requested */
      s->acc = temp2 + (GET_OP_BYTE() << 1); /* Add index */
      break;
    case 0x2e: /* selfID */
      s->acc = objp;
      break;
    case 0x30: /* pprev */
      PUSH(s->prev);
      break;
    case 0x31: /* pToa */
      s->acc = GET_HEAP(selector_offset + GET_OP_FLEX()); 
      break;
    case 0x32: /* aTop */
      PUT_HEAP(selector_offset + GET_OP_FLEX(), s->acc);
      break;
    case 0x33: /* pTos */
      PUSH(GET_HEAP(selector_offset + GET_OP_FLEX()));
      break;
    case 0x34: /* sTop */
      PUT_HEAP(selector_offset + GETO_OP_FLEX(), POP());
      break;
    case 0x35: /* ipToa */
      s->acc = GET_HEAP(selector_offset + GET_OP_FLEX()); 
      ++(s->acc);
      PUT_HEAP(selector_offset + GET_OP_FLEX(), s->acc);
      break;
    case 0x36: /* dpToa */
      s->acc = GET_HEAP(selector_offset + GET_OP_FLEX()); 
      --(s->acc);
      PUT_HEAP(selector_offset + GET_OP_FLEX(), s->acc);
      break;
    case 0x37: /* ipTos */
      temp = GET_HEAP(selector_offset + GET_OP_FLEX()); 
      PUT_HEAP(selector_offset + GET_OP_FLEX(), temp + 1);
      break;
    case 0x38: /* dpTos */
      temp = GET_HEAP(selector_offset + GET_OP_FLEX()); 
      PUT_HEAP(selector_offset + GET_OP_FLEX(), temp - 1);
      break;
    case 0x39: /* lofsa */
      break;
    case 0x3a: /* lofss */
      break;
    case 0x3b: /* push0 */
      PUSH(0);
      break;
    case 0x3c: /* push1 */
      PUSH(1);
      break;
    case 0x3d: /* push2 */
      PUSH(2);
      break;
    case 0x3e: /* pushSelf */
      PUSH(objp);
      break;
    case 0x40: /* lag */
    case 0x41: /* lal */
    case 0x42: /* lat */
    case 0x43: /* lap */
      var_number = (opcode >> 1) & 0x3; /* Gets the type of variable: g, l, t or p */
      temp = variables[var_number] + (GET_OP_FLEX() << 1);
      s->acc = GET_HEAP(temp);
      break;
    case 0x44: /* lsg */
    case 0x45: /* lsl */
    case 0x46: /* lst */
    case 0x47: /* lsp */
      var_number = (opcode >> 1) & 0x3; /* Gets the type of variable: g, l, t or p */
      temp = variables[var_number] + (GET_OP_FLEX() << 1);
      PUSH(GET_HEAP(temp));
      break;
    case 0x48: /* lagi */
    case 0x49: /* lali */
    case 0x4a: /* lati */
    case 0x4b: /* lapi */
      var_number = (opcode >> 1) & 0x3; /* Gets the type of variable: g, l, t or p */
      temp = variables[var_number] + (GET_OP_FLEX() << 1);
      s->acc = GET_HEAP(temp + s->acc);
      break;
    case 0x4c: /* lsgi */
    case 0x4d: /* lsli */
    case 0x4e: /* lsti */
    case 0x4f: /* lspi */
      var_number = (opcode >> 1) & 0x3; /* Gets the type of variable: g, l, t or p */
      temp = variables[var_number] + (GET_OP_FLEX() << 1);
      PUSH(GET_HEAP(temp + s->acc));
      break;
    case 0x50: /* sag */
    case 0x51: /* sal */
    case 0x52: /* sat */
    case 0x53: /* sap */
      var_number = (opcode >> 1) & 0x3; /* Gets the type of variable: g, l, t or p */
      temp = variables[var_number] + (GET_OP_FLEX() << 1);
      PUT_HEAP(temp, s->acc);
      break;
    case 0x54: /* ssg */
    case 0x55: /* ssl */
    case 0x56: /* sst */
    case 0x57: /* ssp */
      var_number = (opcode >> 1) & 0x3; /* Gets the type of variable: g, l, t or p */
      temp = variables[var_number] + (GET_OP_FLEX() << 1);
      PUT_HEAP(temp, POP());
      break;
    case 0x58: /* sagi */
    case 0x59: /* sali */
    case 0x5a: /* sati */
    case 0x5b: /* sapi */
      var_number = (opcode >> 1) & 0x3; /* Gets the type of variable: g, l, t or p */
      temp = variables[var_number] + (GET_OP_FLEX() << 1);
      PUT_HEAP(temp + s->acc, s->acc);
      break;
    case 0x5c: /* ssgi */
    case 0x5d: /* ssli */
    case 0x5e: /* ssti */
    case 0x5f: /* sspi */
      var_number = (opcode >> 1) & 0x3; /* Gets the type of variable: g, l, t or p */
      temp = variables[var_number] + (GET_OP_FLEX() << 1);
      PUT_HEAP(temp + s->acc, POP());
      break;
    case 0x60: /* +ag */
    case 0x61: /* +al */
    case 0x62: /* +at */
    case 0x63: /* +ap */
      var_number = (opcode >> 1) & 0x3; /* Gets the type of variable: g, l, t or p */
      temp = variables[var_number] + (GET_OP_FLEX() << 1);
      s->acc = GET_HEAP(temp);
      ++(s->acc);
      GET_HEAP(temp);
      break;
    case 0x64: /* +sg */
    case 0x65: /* +sl */
    case 0x66: /* +st */
    case 0x67: /* +sp */
      var_number = (opcode >> 1) & 0x3; /* Gets the type of variable: g, l, t or p */
      temp = variables[var_number] + (GET_OP_FLEX() << 1);
      temp2 = GET_HEAP(temp);
      temp2++;
      PUT_HEAP(temp, temp2);
      PUSH(temp2);
      break;
    case 0x68: /* +agi */
    case 0x69: /* +ali */
    case 0x6a: /* +ati */
    case 0x6b: /* +api */
      var_number = (opcode >> 1) & 0x3; /* Gets the type of variable: g, l, t or p */
      temp = variables[var_number] + (GET_OP_FLEX() << 1);
      s->acc = GET_HEAP(temp + s->acc);
      ++(s->acc);
      PUT_HEAP(temp, s->acc);
      break;
    case 0x6c: /* +sgi */
    case 0x6d: /* +sli */
    case 0x6e: /* +sti */
    case 0x6f: /* +spi */
      var_number = (opcode >> 1) & 0x3; /* Gets the type of variable: g, l, t or p */
      temp = variables[var_number] + (GET_OP_FLEX() << 1);
      temp2 = GET_HEAP(temp + s->acc);
      temp2++;
      PUT_HEAP(temp, temp2);
      PUSH(temp2);
      break;
    case 0x70: /* -ag */
    case 0x71: /* -al */
    case 0x72: /* -at */
    case 0x73: /* -ap */
      var_number = (opcode >> 1) & 0x3; /* Gets the type of variable: g, l, t or p */
      temp = variables[var_number] + (GET_OP_FLEX() << 1);
      s->acc = GET_HEAP(temp);
      --(s->acc);
      PUT_HEAP(temp, s->acc);
      break;
    case 0x74: /* -sg */
    case 0x75: /* -sl */
    case 0x76: /* -st */
    case 0x77: /* -sp */
      var_number = (opcode >> 1) & 0x3; /* Gets the type of variable: g, l, t or p */
      temp = variables[var_number] + (GET_OP_FLEX() << 1);
      temp2 = GET_HEAP(temp);
      temp2--;
      PUT_HEAP(temp, temp2);
      PUSH(temp2);
      break;
    case 0x78: /* -agi */
    case 0x79: /* -ali */
    case 0x7a: /* -ati */
    case 0x7b: /* -api */
      var_number = (opcode >> 1) & 0x3; /* Gets the type of variable: g, l, t or p */
      temp = variables[var_number] + (GET_OP_FLEX() << 1);
      s->acc = GET_HEAP(temp + s->acc);
      --(s->acc);
      PUT_HEAP(temp, s->acc);
      break;
    case 0x7c: /* -sgi */
    case 0x7d: /* -sli */
    case 0x7e: /* -sti */
    case 0x7f: /* -spi */
      var_number = (opcode >> 1) & 0x3; /* Gets the type of variable: g, l, t or p */
      temp = variables[var_number] + (GET_OP_FLEX() << 1);
      temp2 = GET_HEAP(temp + s->acc);
      temp2--;
      PUT_HEAP(temp, temp2);
      PUSH(temp2);
      break;
    default:
      pc = old_pc;
      scriptdebug(s, &pc, &sp, &pp, &objp, "Illegal opcode", 0);
    }
  }
}




int emulate(state* s, object* obj, int method)
{
  int pc=0;
  script_method* m=obj->methods[method];
}
