#include <script.h>
#include <vm.h>




void (*VM_debug_function)(state *s, heap_ptr pc, heap_ptr sp, heap_ptr pp, heap_ptr objp) = NULL;
/* The VM single step debug function */

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

void
scriptdebug(state *s, heap_ptr pc, heap_ptr sp, heap_ptr pp, heap_ptr objp)
{
  if (VM_debug_function)
    VM_debug_function(s, pc, sp, pp, objp);
  else {
    fprintf(stderr,"Scriptdebug requested: STUB\n"); /* FIXME */
    exit (-1);
  }
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
  heap_ptr local_vars = objp + SCRIPT_SELECTOR_OFFSET;
  heap_ptr variables[4] =
  { s->global_vars, local_vars, pp, argp }; /* Offsets of global, local, temp, and param variables */

  while (1) {
    byte opcode = GET_OP_BYTE(); /* Get opcode */
    int var_number; /* See description below */

    if (VM_debug_function)
      VM_debug_function(s, pc, sp, pp, objp);
    /* Debug if this has been requested */

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
      break;
    case 0x2c: /* &rest */
      break;
    case 0x2d: /* lea */
      break;
    case 0x2e: /* selfID */
      s->acc = objp;
      break;
    case 0x30: /* pprev */
      PUSH(s->prev);
      break;
    case 0x31: /* pToa */
      break;
    case 0x32: /* aTop */
      break;
    case 0x33: /* pTos */
      break;
    case 0x34: /* sTop */
      break;
    case 0x35: /* ipToa */
      break;
    case 0x36: /* dpToa */
      break;
    case 0x37: /* ipTos */
      break;
    case 0x38: /* dpTos */
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
      break;
    case 0x44: /* lsg */
    case 0x45: /* lsl */
    case 0x46: /* lst */
    case 0x47: /* lsp */
      var_number = (opcode >> 1) & 0x3; /* Gets the type of variable: g, l, t or p */
      temp = variables[var_number] + (GET_OP_FLEX() << 1);
      break;
    case 0x48: /* lagi */
    case 0x49: /* lali */
    case 0x4a: /* lati */
    case 0x4b: /* lapi */
      var_number = (opcode >> 1) & 0x3; /* Gets the type of variable: g, l, t or p */
      temp = variables[var_number] + (GET_OP_FLEX() << 1);
      break;
    case 0x4c: /* lsgi */
    case 0x4d: /* lsli */
    case 0x4e: /* lsti */
    case 0x4f: /* lspi */
      var_number = (opcode >> 1) & 0x3; /* Gets the type of variable: g, l, t or p */
      temp = variables[var_number] + (GET_OP_FLEX() << 1);
      break;
    case 0x50: /* sag */
    case 0x51: /* sal */
    case 0x52: /* sat */
    case 0x53: /* sap */
      var_number = (opcode >> 1) & 0x3; /* Gets the type of variable: g, l, t or p */
      temp = variables[var_number] + (GET_OP_FLEX() << 1);
      break;
    case 0x54: /* ssg */
    case 0x55: /* ssl */
    case 0x56: /* sst */
    case 0x57: /* ssp */
      var_number = (opcode >> 1) & 0x3; /* Gets the type of variable: g, l, t or p */
      temp = variables[var_number] + (GET_OP_FLEX() << 1);
      break;
    case 0x58: /* sagi */
    case 0x59: /* sali */
    case 0x5a: /* sati */
    case 0x5b: /* sapi */
      var_number = (opcode >> 1) & 0x3; /* Gets the type of variable: g, l, t or p */
      temp = variables[var_number] + (GET_OP_FLEX() << 1);
      break;
    case 0x5c: /* ssgi */
    case 0x5d: /* ssli */
    case 0x5e: /* ssti */
    case 0x5f: /* sspi */
      var_number = (opcode >> 1) & 0x3; /* Gets the type of variable: g, l, t or p */
      temp = variables[var_number] + (GET_OP_FLEX() << 1);
      break;
    case 0x60: /* +ag */
    case 0x61: /* +al */
    case 0x62: /* +at */
    case 0x63: /* +ap */
      var_number = (opcode >> 1) & 0x3; /* Gets the type of variable: g, l, t or p */
      temp = variables[var_number] + (GET_OP_FLEX() << 1);
      break;
    case 0x64: /* +sg */
    case 0x65: /* +sl */
    case 0x66: /* +st */
    case 0x67: /* +sp */
      var_number = (opcode >> 1) & 0x3; /* Gets the type of variable: g, l, t or p */
      temp = variables[var_number] + (GET_OP_FLEX() << 1);
      break;
    case 0x68: /* +agi */
    case 0x69: /* +ali */
    case 0x6a: /* +ati */
    case 0x6b: /* +api */
      var_number = (opcode >> 1) & 0x3; /* Gets the type of variable: g, l, t or p */
      temp = variables[var_number] + (GET_OP_FLEX() << 1);
      break;
    case 0x6c: /* +sgi */
    case 0x6d: /* +sli */
    case 0x6e: /* +sti */
    case 0x6f: /* +spi */
      var_number = (opcode >> 1) & 0x3; /* Gets the type of variable: g, l, t or p */
      temp = variables[var_number] + (GET_OP_FLEX() << 1);
      break;
    case 0x70: /* -ag */
    case 0x71: /* -al */
    case 0x72: /* -at */
    case 0x73: /* -ap */
      var_number = (opcode >> 1) & 0x3; /* Gets the type of variable: g, l, t or p */
      temp = variables[var_number] + (GET_OP_FLEX() << 1);
      break;
    case 0x74: /* -sg */
    case 0x75: /* -sl */
    case 0x76: /* -st */
    case 0x77: /* -sp */
      var_number = (opcode >> 1) & 0x3; /* Gets the type of variable: g, l, t or p */
      temp = variables[var_number] + (GET_OP_FLEX() << 1);
      break;
    case 0x78: /* -agi */
    case 0x79: /* -ali */
    case 0x7a: /* -ati */
    case 0x7b: /* -api */
      var_number = (opcode >> 1) & 0x3; /* Gets the type of variable: g, l, t or p */
      temp = variables[var_number] + (GET_OP_FLEX() << 1);
      break;
    case 0x7c: /* -sgi */
    case 0x7d: /* -sli */
    case 0x7e: /* -sti */
    case 0x7f: /* -spi */
      var_number = (opcode >> 1) & 0x3; /* Gets the type of variable: g, l, t or p */
      temp = variables[var_number] + (GET_OP_FLEX() << 1);
      break;
    default:
      pc--;
      sciprintf("Illegal opcode: %x\n at %x!\n", opcode & 0xff, pc);
      scriptdebug(s, pc, sp, pp, objp);
    }
  }
}




int emulate(state* s, object* obj, int method)
{
  int pc=0;
  script_method* m=obj->methods[method];
}
