#include <console.h>
#include <script.h>
#include <vocabulary.h>
#include <stdio.h>
#include <stdlib.h>
#include <util.h>
#include <vm.h>
#include <assert.h>


#ifdef SCI_CONSOLE
#define printf sciprintf
/* Yeah, I shouldn't be doing this ;-) [CJR] */
#endif

FLEXARRAY_NOEXTRA(object*) fobjects;


static opcode* opcodes;
static int knames_count;
static char** knames;
static char** snames;

char* globals[] = {
  /*00*/
  "ego",
  "GAMEID",
  "roomXX",
  "speed",
  /*04*/
  "quitFlag",
  "cast",
  "regions",
  "timer",
  /*08*/
  "sounds",
  "inv",
  "eventHandler",
  "roomNumberExit",
  /*0C*/
  "previousRoomNumber",
  "roomNumber",
  "enterDebugModeOnRoomExit",
  "score",
  /*10*/
  "maximumScore",
  "11",
  "speed",
  "13",
  /*14*/
  "14",
  "loadCursor",
  "normalFont",
  "restoreSaveFont", /*dialogFont*/
  /*18*/
  "18",
  "19",
  "defaultFont",
  "1B",
  /*1C*/
  "pointerToVersionNumber",
  "locales",
  "pointerToSaveGameDirectory",
  "1F"
};

opcode_format formats[128][4]={
  /*00*/
  {Script_None}, {Script_None}, {Script_None}, {Script_None},
  /*04*/
  {Script_None}, {Script_None}, {Script_None}, {Script_None},
  /*08*/
  {Script_None}, {Script_None}, {Script_None}, {Script_None},
  /*0C*/
  {Script_None}, {Script_None}, {Script_None}, {Script_None},
  /*10*/
  {Script_None}, {Script_None}, {Script_None}, {Script_None},
  /*14*/
  {Script_None}, {Script_None}, {Script_None}, {Script_SVariable, 0},
  /*18*/
  {Script_SVariable, 0}, {Script_SVariable, 0}, {Script_SVariable, 0}, {Script_None},
  /*1C*/
  {Script_SVariable, 0}, {Script_None}, {Script_None}, {Script_Variable, 0},
  /*20*/
  {Script_SVariable, Script_SByte, 0}, {Script_SVariable, Script_SByte, 0}, {Script_SVariable, Script_SByte, 0}, {Script_Variable, Script_SVariable, Script_SByte, 0},
  /*24 (24=ret)*/
  {Script_End}, {Script_SByte, 0}, {Script_Invalid}, {Script_Invalid},
  /*28*/
  {Script_SVariable, 0}, {Script_Invalid}, {Script_SByte, 0}, {Script_SVariable, Script_SByte, 0},
  /*2C*/
  {Script_SVariable, 0}, {Script_SVariable, Script_Variable, 0}, {Script_None}, {Script_Invalid},
  /*30*/
  {Script_None}, {Script_SVariable, 0}, {Script_SVariable, 0}, {Script_SVariable, 0},
  /*34*/
  {Script_SVariable, 0}, {Script_SVariable, 0}, {Script_SVariable, 0}, {Script_SVariable, 0},
  /*38*/
  {Script_SVariable, 0}, {Script_SVariable, 0}, {Script_SVariable, 0}, {Script_None},
  /*3C*/
  {Script_None}, {Script_None}, {Script_None}, {Script_Invalid},
  /*40-4F*/
  {Script_Variable, 0}, {Script_Variable, 0}, {Script_Variable, 0}, {Script_Variable, 0},
  {Script_Variable, 0}, {Script_Variable, 0}, {Script_Variable, 0}, {Script_Variable, 0},
  {Script_Variable, 0}, {Script_Variable, 0}, {Script_Variable, 0}, {Script_Variable, 0},
  {Script_Variable, 0}, {Script_Variable, 0}, {Script_Variable, 0}, {Script_Variable, 0},
  /*50-5F*/
  {Script_Variable, 0}, {Script_Variable, 0}, {Script_Variable, 0}, {Script_Variable, 0},
  {Script_Variable, 0}, {Script_Variable, 0}, {Script_Variable, 0}, {Script_Variable, 0},
  {Script_Variable, 0}, {Script_Variable, 0}, {Script_Variable, 0}, {Script_Variable, 0},
  {Script_Variable, 0}, {Script_Variable, 0}, {Script_Variable, 0}, {Script_Variable, 0},
  /*60-6F*/
  {Script_Variable, 0}, {Script_Variable, 0}, {Script_Variable, 0}, {Script_Variable, 0},
  {Script_Variable, 0}, {Script_Variable, 0}, {Script_Variable, 0}, {Script_Variable, 0},
  {Script_Variable, 0}, {Script_Variable, 0}, {Script_Variable, 0}, {Script_Variable, 0},
  {Script_Variable, 0}, {Script_Variable, 0}, {Script_Variable, 0}, {Script_Variable, 0},
  /*70-7F*/
  {Script_Variable, 0}, {Script_Variable, 0}, {Script_Variable, 0}, {Script_Variable, 0},
  {Script_Variable, 0}, {Script_Variable, 0}, {Script_Variable, 0}, {Script_Variable, 0},
  {Script_Variable, 0}, {Script_Variable, 0}, {Script_Variable, 0}, {Script_Variable, 0},
  {Script_Variable, 0}, {Script_Variable, 0}, {Script_Variable, 0}, {Script_Variable, 0}
};

static int add_object(object* obj)
{
	FLEXARRAY_APPEND(fobjects, obj, return 1);
	return 0;
}

static void dump(byte* data, int len)
{
	int i=0;
	while(i<len)
	{
		printf("%02X ", data[i++]);
		if(i%8==0) printf("   ");
		if(i%16==0) printf("\n");
	}
	if(i%16) printf("\n");
}

static void printMethod(object* obj, int meth, int indent)
{
	script_method* m=obj->methods[meth];
	int i, j;

	for(j=0; j<indent*2-1; j++) printf(" ");
	printf("Method %s\n", snames[m->number]);

	for(i=0; i<m->used; i++)
	{
		script_opcode op=m->data[i];

		for(j=0; j<indent; j++) printf("  ");
		printf("%s ", opcodes[op.opcode].name);

		switch(op.opcode)
		{
			case 0x21: /*callk*/
			{
				if(op.arg1>knames_count) printf("<no such kernel function %02X> ", op.arg1);
				else printf("%s ", knames[op.arg1]);
				printf("%02X", op.arg2);
			} break;
			case 0x28: /*class*/
			{
				if(op.arg1>max_object) printf("<no such class %02X>", op.arg1);
				else
				{
					if(fobjects.data[op.arg1]==0) printf("<null object>");
					else printf("%s", fobjects.data[op.arg1]->name);
				}
			} break;
			case 0x44:
			{
				if(op.arg1>0x20) printf("<no such global %02X> ", op.arg1);
				else printf("%s ", globals[op.arg1]);
			} break;
			default:
			{
				int args[3];
				args[0]=op.arg1;
				args[1]=op.arg2;
				args[2]=op.arg3;
				for(j=0; j<3; j++)
				{
					switch(formats[op.opcode][j])
					{
						case Script_Invalid:
						{
								printf("<invalid> ");
						} break;
						case Script_None:
						{
							j=3;
						} break;
					        case Script_SByte:
						case Script_Byte:
						{
							printf("%02X ", args[j]);
						} break;
						case Script_Word:
					        case Script_SVariable:
						case Script_Variable:
						{
							printf("%04X ", args[j]);
						} break;
						case Script_SWord:
						{
							if(args[j]<0) printf("-%04X", -args[j]);
							else printf("%04X", args[j]);
						} break;
						case Script_End:
						{
							printf("\n");
							return;
						} break;
						default:
						{
							printf("<unknown arg type %d> ", formats[op.opcode][j]);
						}
					}
				}
			} break;
		}
		printf("\n");
	}
}

static void printObject_r(object* obj, int flags, int level)
{
	int i;
	for(i=0; i<level; i++) printf("  ");
	if(obj==0) printf("(null)\n");
	else
	{
		printf("%s\n", obj->name);
		if(flags&SCRIPT_PRINT_METHODS)
		{
			for(i=0; i<obj->method_count; i++)
			{
				printMethod(obj, i, level+1);
			}
		}
		if(flags&SCRIPT_PRINT_CHILDREN)
		{
			for(i=0; i<obj->children.used; i++)
			{
				printObject_r(obj->children.data[i], flags, level+1);
			}
		}
	}
}

void printObject(object* obj, int flags)
{
	printf("pO(%p, %d)\n", obj, flags);
	printObject_r(obj, flags, 0);
}

static object* object_new()
{
	object* obj=malloc(sizeof(object));
	if(obj==0) return 0;

	obj->parent=0;
	FLEXARRAY_INIT(object*, obj->children);
	obj->name=0;
	obj->selector_count=0;
	obj->selector_numbers=0;
	obj->methods=0;
	obj->method_count=0;

	return obj;
}

static int add_child(object* parent, object* child)
{
	FLEXARRAY_APPEND(parent->children, child, return 1);
	return 0;
}

static object* fake_object(char* reason)
{
	object* obj=object_new();
	if(obj==0)
	{
		#ifdef SCRIPT_DEBUG
		printf("object_new failed during fake for %s\n", reason);
		#endif
		free(obj);
		return 0;
	}
	if(add_child(object_root, obj))
	{
		#ifdef SCRIPT_DEBUG
		printf("add_child failed during fake for %s\n", reason);
		#endif
		free(obj);
		return 0;
	}
	obj->name=reason;
	if(add_object(obj))
	{
		#ifdef SCRIPT_DEBUG
		printf("add_object failed during fake for %s\n", reason);
		#endif
		/*FIXME: clean up parent*/
		return 0;
	}
	return obj;
}

static script_method* decode_method(byte* data)
{
  script_method* m;
  int done=0;
  int pos=0;
  static count=0;
  
  count++;

  if((m=malloc(sizeof(script_method)))==0) return 0;
  FLEXARRAY_INIT(script_opcode, *m);

  while(!done)
    {
      int op=data[pos]>>1;
      int size=2-(data[pos]&1);
      int* args[3];
      int arg;
      int old_pos;

      FLEXARRAY_ADD_SPACE(*m, 1, return 0);
      old_pos=pos;
      m->data[m->used-1].pos=pos;
      m->data[m->used-1].opcode=op;

      /*Copy the adresses of the args to an array for convenience*/
      args[0]=&m->data[m->used-1].arg1;
      args[1]=&m->data[m->used-1].arg2;
      args[2]=&m->data[m->used-1].arg3;

      /*Skip past the opcode*/
      pos++;

      for(arg=0; arg<4; arg++)
	{
	  switch(formats[op][arg])
	    {
	    case Script_Invalid: /*Can't happen(tm)*/
	      {
	      	int i;
		printf("Invalid opcode %02X at %04X in method %d\n", op, pos, count);
		for(i=m->used-9; i<m->used-1; i++)
		{
			printf("%s[%02X] ", opcodes[m->data[i].opcode].name, m->data[i].opcode);
			dump(data+m->data[i].pos, m->data[i].size);
		}
		printf("Dump from %04X-%04X\n", pos-16, pos+16);
		dump(data + pos - 16, 32);
	      } break;
	    case Script_None: /*No more args*/
	      {
		arg=4;
	      } break;
	    case Script_Byte: /*Just a one byte arg*/
	    case Script_SByte:
	      {
		*args[arg]=data[pos++]; 
	      } break;
	    case Script_Word: /*A two byte arg*/
	      {
		*args[arg]=getInt16(data+pos);
		pos+=2;
	      } break;
	    case Script_SWord: /*A signed two-byte arg*/
	      {
		int t=getInt16(data+pos);
		if(t&0x8000) *args[arg]=-(t&0x7FFF);
		else *args[arg]=t;
		pos+=2;
	      } break;
	    case Script_Variable: /*Size of arg depends on LSB in opcode*/
            case Script_SVariable:
	      {
		if(size==1) *args[arg]=data[pos++];
		else
		{
			*args[arg]=getInt16(data+pos);
			pos+=2;
		}
	      } break;
	    case Script_End: /*Special tag for ret*/
	      {
		done=1;
		arg=4;
	      } break;
	    default: /*Can't happen(tm)*/
	      {
		printf("Unknown argument format %d for op %02X\n", formats[op][arg], op);
	      } break;
	    }
	}
      fflush(stdout);
      if (m->used) m->data[m->used-1].size=pos-old_pos;
    }

  return m;
}

#ifdef SCRIPT_DEBUG
void list_code_blocks(resource_t* r)
{
	int pos=getInt16(r->data+2);
	while(pos<r->length-2)
	{
		int type=getInt16(r->data+pos);
		int len=getInt16(r->data+pos+2);
		if(type==2) printf("%X-%X\n", pos, pos+len);
		pos+=len;
	}
}
#endif


/*These expect the frame, the whole frame, and, well, other stuff too,
 *I guess, as long as it looks like a frame*/
static int get_type(unsigned char* obj)
{
	return getInt16(obj);
}

static int get_length(unsigned char* obj)
{
	return getInt16(obj+2);
}

static int get_selector_count(unsigned char* obj)
{
	return getInt16(obj+10);
}

static int get_selector_value(unsigned char* obj, int sel)
{
	assert(sel<get_selector_count(obj));
	return getInt16(obj + 12 + sel*2);
}

/*Bas things happen if the method offset value is wrong*/
static unsigned char* get_method_area(unsigned char* obj)
{
	return obj+getInt16(obj+8)+10;
}

static int get_method_count(unsigned char* obj)
{
	return getInt16(get_method_area(obj));
}

static int get_method_number(unsigned char* obj, int i)
{
	assert(i<get_method_count(obj));
	return getInt16(get_method_area(obj)+2+2*i);
}

static int get_method_location(unsigned char* obj, int i)
{
	assert(i<get_method_count(obj));
	return getInt16(get_method_area(obj)+4+2*get_method_count(obj)+2*i);
}


/*Returns the position of the first frame of type 'type' in resource 'r',
 *starting from the frame starting at 'start', or -1 on failure.
 */
static int find_frame(resource_t* r, int type, int start)
{
	int t=-1;
	int pos=start;
	unsigned char* frame;

	assert(start<=r->length-4);

	#ifdef SCRIPT_DEBUG
	printf("Searching for frame of type %d in script %03d, starting at %#x\n", type, r->number, start);
	dump(r->data+start, 32);
	#endif

	/*Some times there's an extra byte at the beginning. Christoph?*/
#if 1
	if(pos==0 && r->length>=6 && \
	   !((0<getInt16(r->data)) && (10>getInt16(r->data)))) pos=2;
#else
	if(pos==0) pos=2;
#endif
	frame=r->data+pos;
	while(1)
	{
		#ifdef SCRIPT_DEBUG
		printf("offset = %#x\n", pos);
		dump(frame, 32);
		#endif
		t=get_type(frame);
		if(t==type) break;
		if(t==0) return -1;

		pos+=get_length(frame);
		if(pos>r->length-2) return -1;
		frame+=get_length(frame);
	}

	return pos;
}



/*FIXME: lots of things are identical to read_object and read_class. Some of
 *these would benefit from being put in separate functions.*/

static object* read_object(int script, int positions[1000])
{
	resource_t* r=findResource(sci_script, script);
	unsigned char* raw;
	int pos, length;
	object* obj;

	printf("Searching for object in script %03d\n", script);

	if(r==0) return 0;

	/*Skip to the next object*/
	#ifdef SCRIPT_DEBUG
	printf("pre skip: pos=%#x\n", positions[script]);
	#endif
	pos=find_frame(r, 1, positions[script]);
	#ifdef SCRIPT_DEBUG
	printf("post skip: pos=%#x\n", pos);
	#endif
	if(pos==-1) return 0;
	else positions[script]=pos+get_length(r->data+pos);
	#ifdef SCRIPT_DEBUG
	printf("post post: pos=%#x (len=%#x)\n", positions[script], get_length(r->data+pos));
	#endif

	/*Construct the object*/
	obj=object_new();
	raw=r->data+pos;

	/*Fill in the name*/
	if(get_selector_count(raw)<4) obj->name="<anonymous>";
	else
	{
		if(get_selector_value(raw, 3)) obj->name=r->data + get_selector_value(raw, 3);
		else obj->name="<null>";
	}

	/*Fill in the class*/
	if(get_selector_count(raw)==0) obj->parent=object_root;
	else
	{
		int parent_id=get_selector_value(raw, 1);
		if(parent_id>=fobjects.used)
		{
			free(obj);
			return 0;
		}
		if(parent_id<1) obj->parent=object_root;
		else obj->parent=fobjects.data[parent_id];
	}

	/*Add the object to the class*/
	if(!obj->parent)
	{
		free(obj);
		return 0;
	}
	if(add_child(obj->parent, obj)){
		free(obj);
		return 0;
	}
	if(add_object(obj))
	{
		free(obj);
		return 0;
	}

	/*FIXME: decode selectors here*/

	obj->method_count=get_method_count(raw);
	obj->methods=malloc(obj->method_count*sizeof(script_method));
	if(obj->methods==0)
	{
		free(obj);
		return 0;
	} else {
		int i;
		for(i=0; i<obj->method_count; i++)
		{
			int number=get_method_number(raw, i);
			int position=get_method_location(raw, i);

			if((obj->methods[i]=decode_method(r->data+position))==0)
			{
				obj->method_count=i-1;
				break;
			}
			obj->methods[i]->number=number;
		}
	}

	return obj;
}

static object* read_class(int script, int positions[1000])
{
	resource_t* r=findResource(sci_script, script);
	unsigned char* raw;
	int pos, length;
	object* obj;

	printf("Searching for class in script %03d\n", script);

	if(r==0) return fake_object("<resource not found>");

	/*Skip to the next class*/
	#ifdef SCRIPT_DEBUG
	printf("pre skip: pos=%#x\n", positions[script]);
	#endif
	pos=find_frame(r, 6, positions[script]);
	#ifdef SCRIPT_DEBUG
	printf("post skip: pos=%#x\n", pos);
	#endif
	if(pos==-1) return fake_object("<no more classes in script>");
	else positions[script]=pos+get_length(r->data+pos);
	#ifdef SCRIPT_DEBUG
	printf("post post: pos=%#x (len=%#x)\n", positions[script], get_length(r->data+pos));
	#endif

	/*Construct the object*/
	obj=object_new();
	raw=r->data+pos;

	/*Fill in the name*/
	if(get_selector_count(raw)<4) obj->name="<anonymous>";
	else
	{
		if(get_selector_value(raw, 3)) obj->name=r->data + get_selector_value(raw, 3);
		else obj->name="<null>";
	}

	/*Fill in the parent*/
	if(get_selector_count(raw)==0) obj->parent=object_root;
	else
	{
		int superclass_id=get_selector_value(raw, 1);
		printf("superclass==%d\n", superclass_id);
		if(superclass_id>=fobjects.used)
		{
			free(obj);
			return fake_object("<no such superclass>");
		}
		if(superclass_id<1) obj->parent=object_root;
		else obj->parent=fobjects.data[superclass_id];
	}

	/*Add the class to the hierarchy*/
	if(!obj->parent)
	{
		free(obj);
		return fake_object("<null parent>");
	}
	if(add_child(obj->parent, obj)){
		free(obj);
		return fake_object("<add_child failed>");
	}
	if(add_object(obj))
	{
		free(obj);
		return fake_object("<add_object failed>");
	}

	/*FIXME: decode selectors and methods here*/

	return obj;
}

void freeObject(object* obj)
{
	int i;
	for(i=0; i<obj->children.used; i++) freeObject(obj->children.data[i]);
	free(obj);
}

static int objects_init()
{
	FLEXARRAY_INIT(object*, fobjects);
	max_object=0;

	if((object_root=object_new())==0) return 1;
	object_root->name="<root>";
	add_object(object_root);

	opcodes=vocabulary_get_opcodes();
	knames=vocabulary_get_knames(&knames_count);
	snames=vocabulary_get_snames();

	return 0;
}

int loadObjects()
{
	int i;
	int *classes, class_count;
	int positions[1000];

	if(objects_init())
	{
		#ifdef SCRIPT_DEBUG
		perror("objects_init");
		#endif
		return 1;
	}
	classes=vocabulary_get_classes(&class_count);

	for(i=0; i<1000; i++) positions[i]=0;
	for(i=0; i<class_count; i++)
	{
		if(read_class(classes[i], positions)==0)
		{
			#ifdef SCRIPT_DEBUG
			fprintf(stderr, "Failed to load class %d, which is a parent.\n", i);
			#endif
			return 1;
		}
	}

	for(i=0; i<1000; i++) positions[i]=0;
	for(i=0; i<1000; i++) while(read_object(i, positions));

	object_map=fobjects.data;
	max_object=fobjects.used;

	return 0;
}


int
script_find_selector(state_t *s, char *selectorname)
{
  int i;
  for (i = 0; i < s->selector_names_nr; i++)
    if (strcmp(selectorname, s->selector_names[i]) == 0)
      return i;

  sciprintf("Warning: Could not map '%s' to any selector!\n", selectorname);
  return -1;
}

#define FIND_SELECTOR(_slc_, _slcstr_) map->_slc_ = script_find_selector(s, _slcstr_);

void
script_map_selectors(state_t *s, selector_map_t *map)
{
  map->init = script_find_selector(s, "init");
  map->play = script_find_selector(s, "play");
  map->x = script_find_selector(s, "x");
  map->y = script_find_selector(s, "y");
  map->z = script_find_selector(s, "z");
  map->priority = script_find_selector(s, "priority");
  map->view = script_find_selector(s, "view");
  map->loop = script_find_selector(s, "loop");
  map->cel = script_find_selector(s, "cel");
  FIND_SELECTOR(brLeft, "brLeft");
  FIND_SELECTOR(brRight, "brRight");
  FIND_SELECTOR(brTop, "brTop");
  FIND_SELECTOR(brBottom, "brBottom");
  FIND_SELECTOR(xStep, "xStep");
  FIND_SELECTOR(yStep, "yStep");
  FIND_SELECTOR(nsBottom, "nsBottom");
  FIND_SELECTOR(nsTop, "nsTop");
  FIND_SELECTOR(nsLeft, "nsLeft");
  FIND_SELECTOR(nsRight, "nsRight");
  FIND_SELECTOR(font, "font");
  FIND_SELECTOR(text, "text");
  FIND_SELECTOR(type, "type");
  FIND_SELECTOR(state, "state");
  FIND_SELECTOR(doit, "doit");
  FIND_SELECTOR(delete, "delete");
  FIND_SELECTOR(signal, "signal");
  FIND_SELECTOR(underBits, "underBits");
  FIND_SELECTOR(canBeHere, "canBeHere");
  FIND_SELECTOR(client, "client");
  FIND_SELECTOR(dx, "dx");
  FIND_SELECTOR(dy, "dy");
  FIND_SELECTOR(xStep, "xStep");
  FIND_SELECTOR(yStep, "yStep");
  FIND_SELECTOR(b_movCnt, "b-moveCnt");
  FIND_SELECTOR(b_i1, "b-i1");
  FIND_SELECTOR(b_i2, "b-i2");
  FIND_SELECTOR(b_di, "b-di");
  FIND_SELECTOR(b_xAxis, "b-xAxis");
  FIND_SELECTOR(b_incr, "b-incr");
  FIND_SELECTOR(completed, "completed");
  FIND_SELECTOR(illegalBits, "illegalBits");
  FIND_SELECTOR(dispose, "dispose");
  FIND_SELECTOR(prevSignal, "prevSignal");
  FIND_SELECTOR(message, "message");
  FIND_SELECTOR(modifiers, "modifiers");
  FIND_SELECTOR(cue, "cue");
  FIND_SELECTOR(owner, "owner");
  FIND_SELECTOR(handle, "handle");
  FIND_SELECTOR(number, "number");
}

int
sci_hexdump(byte *data, int length, int offsetplus)
{
  char tempstr[40];
  int i;
  for (i = 0; i < length; i+= 8) {
    int j;

    sprintf(tempstr, "%04x:                                 ", i + offsetplus);
    for (j = 0; j < MIN(8, length - i); j++)
      sprintf(tempstr + 6 + (j*3) + (j > 3), "%02x  ", data[i+j]);
    for (j = 0; j < MIN(8, length - i); j++) {
      int thechar;
      thechar = data[i+j];
      sprintf(tempstr + 31 + j, "%c",
	      ((thechar < ' ') || (thechar > 127))? '.' : thechar );
    }

    for (j = 0; j < 38; j++)
      if (!tempstr[j])
	tempstr[j] = ' '; /* get rid of sprintf's \0s */
	  
    sciprintf("%s\n", tempstr);
  }
  return 0;
}

void script_dissect(int res_no)
{
  char **snames = vocabulary_get_snames();
  int objectctr[11] = {0,0,0,0,0,0,0,0,0,0,0};
  int _seeker = 0;
  resource_t *script = findResource(sci_script, res_no);

  if (!script) {
    sciprintf("Script not found!\n");
    return;
  }

  while (_seeker < script->length) {
    int objtype = getInt16(script->data + _seeker);
    int objsize;
    int seeker = _seeker + 4;

    if (!objtype) {
      sciprintf("End of script object (#0) encountered.\n");
      sciprintf("Classes: %i, Objects: %i, Export: %i,\n Var: %i (all base 10)",
		objectctr[6], objectctr[1], objectctr[7], objectctr[10]);
      vocabulary_free_snames(snames);
      return;
    }

    sciprintf("\n");

    objsize = getInt16(script->data + _seeker + 2);

    sciprintf("Obj type #%x, size 0x%x: ", objtype, objsize);

    _seeker += objsize;

    objectctr[objtype]++;

    switch (objtype) {
    case sci_obj_object: {
      int selectors, overloads, selectorsize;
      int species = getInt16(script->data + 8 + seeker);
      int superclass = getInt16(script->data + 10 + seeker);
      int namepos = getInt16(script->data + 14 + seeker);
      int nameseeker, i = 0;

      sciprintf("Object\n");

      sci_hexdump(script->data + seeker, objsize -4, seeker);
      /*-4 because the size includes the two-word header */

      sciprintf("Name: %s\n", namepos? ((char *)script->data + namepos) : "<unknown>");
      sciprintf("Superclass: %x\n", superclass);
      sciprintf("Species: %x\n", species);
      sciprintf("-info-:%x\n", getInt16(script->data + 12 + seeker) & 0xffff);

      sciprintf("Function area offset: %x\n", getInt16(script->data + seeker + 4));
      sciprintf("Selectors [%x]:\n",
		selectors = (selectorsize = getInt16(script->data + seeker + 6)));

      seeker += 8;

      while (selectors--) {
	sciprintf("  [#%03x] = 0x%x\n", i++, getInt16(script->data + seeker) & 0xffff);

	seeker += 2;
      }


      sciprintf("Overloaded functions: %x\n", selectors = 
		overloads = getInt16(script->data + seeker));

      seeker += 2;

      if (overloads < 100)
      while (overloads--) {
	int selector = getInt16(script->data + (seeker));

	sciprintf("  [%03x] %s: @", selector & 0xffff, (snames)? snames[selector] : "<?>");
	sciprintf("%04x\n", getInt16(script->data + seeker + selectors*2 + 2) & 0xffff);

	seeker += 2;
      }

    };
    break;

    case sci_obj_code: {
      sciprintf("Code\n");
      sci_hexdump(script->data + seeker, objsize -4, seeker);
    };
    break;

    case 3:
    case 4: {
      sciprintf("<unknown>\n");
      sci_hexdump(script->data + seeker, objsize -4, seeker);
    };
    break;

    case sci_obj_strings: {
      sciprintf("Strings\n");
      while (script->data [seeker])
      {
        sciprintf ("%02x  %s\n", seeker, script->data+seeker);
        seeker += strlen (script->data+seeker)+1;
      }
      seeker++; /* the ending zero byte */
    };
    break;

    case sci_obj_class: {
      int selectors, overloads, selectorsize;
      int species = getInt16(script->data + 8 + seeker);
      int superclass = getInt16(script->data + 10 + seeker);
      int namepos = getInt16(script->data + 14 + seeker);
      int nameseeker;

      sciprintf("Class\n");

      sci_hexdump(script->data + seeker, objsize -4, seeker); 

      sciprintf("Name: %s\n", namepos? ((char *)script->data + namepos) : "<unknown>");
      sciprintf("Superclass: %x\n", superclass);
      sciprintf("Species: %x\n", species);
      sciprintf("-info-:%x\n", getInt16(script->data + 12 + seeker) & 0xffff);

      sciprintf("Function area offset: %x\n", getInt16(script->data + seeker + 4));
      sciprintf("Selectors [%x]:\n",
		selectors = (selectorsize = getInt16(script->data + seeker + 6)));

      seeker += 8;
      selectorsize <<= 1;

      while (selectors--) {
	int selector = getInt16(script->data + (seeker) + selectorsize);

	sciprintf("  [%03x] %s = 0x%x\n", 0xffff & selector, (snames)? snames[selector] : "<?>",
		  getInt16(script->data + seeker) & 0xffff);

	seeker += 2;
      }

      seeker += selectorsize;

      sciprintf("Overloaded functions: %x\n", selectors = 
		overloads = getInt16(script->data + seeker));

      seeker += 2;

      while (overloads--) {
	int selector = getInt16(script->data + (seeker));

	sciprintf("  [%03x] %s: @", selector & 0xffff, (snames)? snames[selector] : "<?>");
	sciprintf("%04x\n", getInt16(script->data + seeker + selectors*2 + 2) & 0xffff);

	seeker += 2;
      }

    };
    break;

    case sci_obj_exports: {
      sciprintf("Exports\n");
      sci_hexdump(script->data + seeker, objsize -4, seeker);
    };
    break;

    case sci_obj_pointers: {
      sciprintf("Pointers\n");
      sci_hexdump(script->data + seeker, objsize -4, seeker);
    };
    break;

    case 9: {
      sciprintf("<unknown>\n");
      sci_hexdump(script->data + seeker, objsize -4, seeker);
    };
    break;

    case sci_obj_localvars: {
      sciprintf("Local vars\n");
      sci_hexdump(script->data + seeker, objsize -4, seeker);
    };
    break;

    default:
      sciprintf("Unsupported!\n");
      return;
    }
    
  }

  sciprintf("Script ends without terminator\n");

  vocabulary_free_snames(snames);
}
