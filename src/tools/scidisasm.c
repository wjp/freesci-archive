/***************************************************************************
 scidisasm.c Copyright (C) 1999 Christoph Reichenbach, TU Darmstadt


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

    Dmitry Jemerov (DJ) [yole@nnz.ru]

 History:

   991213 - created (DJ)

***************************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include <resource.h>
#include <script.h>
#include <console.h>

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif /* HAVE_GETOPT_H */

#ifdef HAVE_GETOPT_H
static struct option options[] = {
  {"version", no_argument, 0, 256},
  {"help", no_argument, 0, 'h'},
  {"hexdump", no_argument, 0, 'x'},
  {0, 0, 0, 0}};
#endif /* HAVE_GETOPT_H */

static int hexdump=0;

void
graph_update_box(state_t *s, int x,int y,int z,int w)
{}; /* Dummy because of braindead library design */

typedef struct name_s {
  int offset;
  char *name;
  int class_no;
  struct name_s *next;
} name_t;

typedef struct area_s {
  int start_offset;
  int end_offset;
  struct area_s *next;
} area_t;

enum area_type { area_said, area_string };

typedef struct script_state_s {
  int script_no;
  name_t *names;
  area_t *areas [2];

  struct script_state_s *next;
} script_state_t;

typedef struct disasm_state_s {
  char **snames;
  opcode *opcodes;
  int kernel_names_nr;
  char **kernel_names;
  word_t **words;
  int word_count;

  char **class_names;
  short **class_selectors;
  int class_count;

  script_state_t *scripts;
} disasm_state_t;

void 
disassemble_script(disasm_state_t *d, int res_no, int pass_no);

script_state_t *
find_script_state (disasm_state_t *d, int script_no);

void
script_free_names (script_state_t *s);

void
script_add_name (script_state_t *s, int aoffset, char *aname, int aclass_no);

char *
script_find_name (script_state_t *s, int offset, int *class_no);

void
script_add_area (script_state_t *s, int start_offset, int end_offset, int type);

void
script_free_areas (script_state_t *s);

int
script_get_area_type (script_state_t *s, int offset);

void
disasm_init (disasm_state_t *d);

void
disasm_free_state (disasm_state_t *d);

int main(int argc, char** argv)
{
  int i;
  char outfilename [256];
  int optindex = 0;
  int c;
  disasm_state_t disasm_state;

#ifdef HAVE_GETOPT_H
  while ((c = getopt_long(argc, argv, "vhx", options, &optindex)) > -1) {
#else /* !HAVE_GETOPT_H */
  while ((c = getopt(argc, argv, "vhx")) > -1) {
#endif /* !HAVE_GETOPT_H */
      
      switch (c)
	{
	case 256:
	  printf("scidisasm ("PACKAGE") "VERSION"\n");
	  printf("This program is copyright (C) 1999 Christoph Reichenbach.\n"
		 "It comes WITHOUT WARRANTY of any kind.\n"
		 "This is free software, released under the GNU General Public License.\n");
	  exit(0);

        case 'x':
          hexdump=1;
          break;
	  
	case 'h':
	  printf("Usage: scidisasm\n"
		 "\nAvailable options:\n"
		 " --version              Prints the version number\n"
		 " --help        -h       Displays this help message\n"
                 " --hexdump     -x       Hex dump all script resources\n");
	  exit(0);
	  
	case 0: /* getopt_long already did this for us */
	case '?':
	  /* getopt_long already printed an error message. */
	  break;
	  
	default:
	  return -1;
	}
    }

  printf ("Loading resources...\n");
  if (i = loadResources(SCI_VERSION_AUTODETECT, 0)) {
    fprintf(stderr,"SCI Error: %s!\n", SCI_Error_Types[i]);
    return 1;
  }

  disasm_init (&disasm_state);
  
  printf ("Performing first pass...\n");
  for (i=0; i<max_resource; i++)
    if (resource_map[i].type == sci_script)
      disassemble_script(&disasm_state, resource_map [i].number, 1);

  printf ("Performing second pass...\n");
  for (i=0; i<max_resource; i++)
    if (resource_map[i].type == sci_script)
    {
      FILE *f;

      sprintf (outfilename, "%03d.script", resource_map [i].number);
      f=fopen (outfilename, "wt");
      con_file=f;
      disassemble_script(&disasm_state, resource_map [i].number, 2);
      fclose (f);
    }
    
  disasm_free_state (&disasm_state);
    
  freeResources();
  return 0;
}

/* -- General operations on disasm_state_t -------------------------------  */
  
void
disasm_init (disasm_state_t *d)
{
  int *classes;
  
  d->snames = vocabulary_get_snames();
  d->opcodes = vocabulary_get_opcodes();
  d->kernel_names = vocabulary_get_knames (&d->kernel_names_nr);
  d->words = vocab_get_words (&d->word_count);
  d->scripts = NULL;

  classes=vocabulary_get_classes (&d->class_count);
  g_free (classes);
  d->class_names = (char **) malloc (d->class_count * sizeof (char *));
  memset (d->class_names, 0, d->class_count * sizeof (char *));
  d->class_selectors = (short **) malloc (d->class_count * sizeof (short *));
  memset (d->class_selectors, 0, d->class_count * sizeof (short *));
}

void
disasm_free_state (disasm_state_t *d)
{
  script_state_t *s, *next_script;
  int i;

  s=d->scripts;
  while (s)
  {
    next_script=s->next;
    script_free_names (s);
    script_free_areas (s);
    s=next_script;
  }

  for (i=0; i<d->class_count; i++)
  {
    if (d->class_names [i]) free (d->class_names [i]);
    if (d->class_selectors [i]) free (d->class_selectors [i]);
  }
  free (d->class_names);
  free (d->class_selectors);
  
  vocabulary_free_snames (d->snames);
  g_free (d->opcodes);
  vocabulary_free_knames (d->kernel_names);
  vocab_free_words (d->words, d->word_count);
}

script_state_t *
find_script_state (disasm_state_t *d, int script_no)
{
  script_state_t *s;

  for (s=d->scripts; s; s=s->next)
    if (s->script_no == script_no) return s;

  s=(script_state_t *) malloc (sizeof (script_state_t));
  memset (s, 0, sizeof (script_state_t));
  s->script_no = script_no;
  s->next = d->scripts;

  d->scripts=s;
  return s;
}

/* -- Name table operations ----------------------------------------------  */

void
script_free_names (script_state_t *s)
{
  name_t *p=s->names, *next_name;

  while (p)
  {
    next_name=p->next;
    free (p->name);
    free (p);
    p=next_name;
  }

  s->names = NULL;
}

void
script_add_name (script_state_t *s, int aoffset, char *aname, int aclass_no)
{
  name_t *p;
  char *name=script_find_name (s, aoffset, NULL);
  if (name) return;

  p=(name_t *) malloc (sizeof (name_t));
  p->offset=aoffset;
  p->name=strdup (aname);
  p->class_no=aclass_no;
  p->next=s->names;
  s->names=p;
}

char *
script_find_name (script_state_t *s, int offset, int *aclass_no)
{
  name_t *p;

  for (p=s->names; p; p=p->next)
    if (p->offset == offset) 
    {
      if (aclass_no) *aclass_no = p->class_no;
      return p->name;
    }
  
  return NULL;
}

/* -- Area table operations ----------------------------------------------  */

void
script_add_area (script_state_t *s, int start_offset, int end_offset, int type)
{
  area_t *area;

  area=(area_t *) malloc (sizeof (area_t));
  area->start_offset = start_offset;
  area->end_offset = end_offset;
  area->next = s->areas [type];

  s->areas [type] = area;
}

void
script_free_areas (script_state_t *s)
{
  int i;

  for (i=0; i<2; i++)
  {
    area_t *area=s->areas [i], *next_area;
    while (area)
    {
      next_area=area->next;
      free (area);
      area=next_area;
    }
  }
}

int
script_get_area_type (script_state_t *s, int offset)
{
  int i;
  
  for (i=0; i<2; i++)
  {
    area_t *area=s->areas [i];
    while (area)
    {
      if (area->start_offset <= offset && area->end_offset >= offset)
        return i;
      area=area->next;
    }
  }

  return -1;
}

/* -- Code to dump individual script block types -------------------------  */

static void
script_dump_object(disasm_state_t *d, script_state_t *s,
                   unsigned char *data, int seeker, int objsize, int pass_no)
{
  int selectors, overloads, selectorsize;
  int species = getInt16(data + 8 + seeker);
  int superclass = getInt16(data + 10 + seeker);
  int namepos = getInt16(data + 14 + seeker);
  int nameseeker, i = 0;
  short sel;
  char *name;
  char buf [256];
  short *sels;

  selectors = (selectorsize = getInt16(data + seeker + 6));
  name=namepos? ((char *)data + namepos) : "<unknown>";
  
  if (pass_no == 2)
  {
    sciprintf("Name: %s\n", name);
    sciprintf("Superclass: ");
    if (superclass == -1)
      sciprintf ("<none>\n");
    else
      sciprintf ("%s  [%x]\n", 
                 d->class_names [superclass] ? d->class_names [superclass] : "<unknown>",
                 superclass);

    sciprintf("Species: ");
    if (species == -1)
      sciprintf ("<none>\n");
    else
      sciprintf ("%s  [%x]\n", 
                 d->class_names [species] ? d->class_names [species] : "<unknown>",
                 species);

    sciprintf("-info-:%x\n", getInt16(data + 12 + seeker) & 0xffff);

    sciprintf("Function area offset: %x\n", getInt16(data + seeker + 4));
    sciprintf("Selectors [%x]:\n", selectors);
  }

  seeker += 8;

  sels=d->class_selectors [species];
  
  while (selectors--) {
    if (pass_no == 2)
    {
      sel=getInt16(data + seeker) & 0xffff;
      if (sels && sels [i] >= 0)
      {
        sciprintf("  [#%03x] %s = 0x%x\n", i, d->snames [sels [i]], sel);
        i++;
      }
      else
        sciprintf("  [#%03x] <unknown> = 0x%x\n", i++, sel);
    }

    seeker += 2;
  }

  selectors = overloads = getInt16(data + seeker);

  if (pass_no == 2)
    sciprintf("Overloaded functions: %x\n", overloads);

  seeker += 2;

  while (overloads--) {
    int selector = getInt16(data + (seeker));

    if (pass_no == 1) 
    {
      sprintf (buf, "%s::%s", name, (d->snames)? d->snames[selector] : "<?>");
      script_add_name (s, getInt16(data + seeker + selectors*2 + 2) & 0xffff, buf, species);
    } 
    else {
      sciprintf("  [%03x] %s: @", selector & 0xffff, (d->snames)? d->snames[selector] : "<?>");
      sciprintf("%04x\n", getInt16(data + seeker + selectors*2 + 2) & 0xffff);
    }

    seeker += 2;
  }
}

static void 
script_dump_class(disasm_state_t *d, script_state_t *s,
                  unsigned char *data, int seeker, int objsize, int pass_no)
{
  int selectors, overloads, selectorsize;
  int species = getInt16(data + 8 + seeker);
  int superclass = getInt16(data + 10 + seeker);
  int namepos = getInt16(data + 14 + seeker);
  int nameseeker;
  char *name;
  char buf [256];
  int i;

  name=namepos? ((char *)data + namepos) : "<unknown>";
  selectors = (selectorsize = getInt16(data + seeker + 6));

  if (pass_no == 1)
  {
    if (species >= 0 && species < d->class_count)
    {
      d->class_names [species] = strdup (name);
      d->class_selectors [species] = (short *) malloc (sizeof (short) * selectors);
    }
  }
  
  if (pass_no == 2) 
  {
    sciprintf ("Class:\n");
    sciprintf("Name: %s\n", name);
    sciprintf("Superclass: ");
    if (superclass == -1)
      sciprintf ("<none>\n");
    else
      sciprintf ("%s  [%x]\n", 
                 d->class_names [superclass] ? d->class_names [superclass] : "<unknown>",
                 superclass);
    sciprintf("Species: %x\n", species);
    sciprintf("-info-:%x\n", getInt16(data + 12 + seeker) & 0xffff);

    sciprintf("Function area offset: %x\n", getInt16(data + seeker + 4));
    sciprintf("Selectors [%x]:\n", selectors);
  }

  seeker += 8;
  selectorsize <<= 1;

  for (i=0; i<selectors; i++) {
    int selector = 0xffff & getInt16(data + (seeker) + selectorsize);

    if (pass_no == 1)
      (d->class_selectors [species]) [i] = selector;
    else
      sciprintf("  [%03x] %s = 0x%x\n", 0xffff & selector, (d->snames)? d->snames[selector] : "<?>",
	        getInt16(data + seeker) & 0xffff);

    seeker += 2;
  }

  seeker += selectorsize;

  selectors =  overloads = getInt16(data + seeker);

  sciprintf("Overloaded functions: %x\n", overloads);

  seeker += 2;

  while (overloads--) {
    int selector = getInt16(data + (seeker));

    if (pass_no == 1)
    {
      sprintf (buf, "%s::%s", name, (d->snames)? d->snames[selector] : "<?>");
      script_add_name (s, getInt16(data + seeker + selectors*2 + 2) & 0xffff, buf, species);
    }
    else {
      sciprintf("  [%03x] %s: @", selector & 0xffff, (d->snames)? d->snames[selector] : "<?>");
      sciprintf("%04x\n", getInt16(data + seeker + selectors*2 + 2) & 0xffff);
    }

    seeker += 2;
  }
}

static void
script_dump_said(disasm_state_t *d, script_state_t *s,
                 unsigned char *data, int seeker, int objsize, int pass_no)
{
  int _seeker=seeker+objsize-4;
  
  if (pass_no == 1) 
  {
    script_add_area (s, seeker, seeker+objsize-1, area_said);
    return;
  }
  
  sciprintf ("%04x: ", seeker);
  while (seeker < _seeker)
  {
    unsigned short nextitem=(unsigned char) data [seeker++];
    if (nextitem == 0xFF)
      sciprintf ("\n%04x: ", seeker);
    else if (nextitem >= 0xF0)
    {
      switch (nextitem) 
      {
      case 0xf0: sciprintf(", "); break;
      case 0xf1: sciprintf("& "); break;
      case 0xf2: sciprintf("/ "); break;
      case 0xf3: sciprintf("( "); break;
      case 0xf4: sciprintf(") "); break;
      case 0xf5: sciprintf("[ "); break;
      case 0xf6: sciprintf("] "); break;
      case 0xf7: sciprintf("# "); break;
      case 0xf8: sciprintf("< "); break;
      case 0xf9: sciprintf("> "); break;
      }
    }
    else {
      nextitem = nextitem << 8 | (unsigned char) data [seeker++];                    
      sciprintf ("%s[%03x] ", vocab_get_any_group_word (nextitem, d->words, d->word_count), nextitem);
    }
  }
  sciprintf ("\n");
}

static void
script_dump_strings(disasm_state_t *d, script_state_t *s,
                    unsigned char *data, int seeker, int objsize, int pass_no)
{
  if (pass_no == 1) 
  {
    script_add_area (s, seeker, seeker+objsize-1, area_string);
    return;
  }

  sciprintf("Strings\n");
  while (data [seeker])
  {                                                           
    sciprintf ("%04x: %s\n", seeker, data+seeker);
    seeker += strlen (data+seeker)+1;
  }
}

static void
script_dump_exports(disasm_state_t *d, script_state_t *s,
                    unsigned char *data, int seeker, int objsize, int pass_no)
{
  word *pexport=(word *) (data+seeker);
  word export_count=*pexport++;
  int i;
  char buf [256];

  if (pass_no == 2)
    sciprintf ("Exports:\n");

  for (i=0; i<export_count; i++)
  {
    if (pass_no == 1)
    {
      word offset=*pexport;
      sprintf (buf, "exp_%02X", i);
      script_add_name (s, offset, buf, -1);
    }
    else
      sciprintf ("%02X: %04X\n", i, *pexport);
    pexport++;
  }
}

/* -- The disassembly code -----------------------------------------------  */

static void
script_disassemble_code(disasm_state_t *d, script_state_t *s,
                        unsigned char *data, int seeker, int objsize, int pass_no)
{
  int endptr=seeker+objsize-4;
  int i=0;
  int cur_class=-1;
  word dest;
  
  if (pass_no == 1) return;
  
  while (seeker < endptr)
  {
    unsigned char opsize = data [seeker];
    unsigned char opcode = opsize >> 1;
    word param_value;
    char *name;
    i = 0;

    opsize &= 1; /* byte if true, word if false */

    name=script_find_name (s, seeker, &cur_class);
    if (name) sciprintf ("      %s:\n", name);
    sciprintf("%04X: ", seeker);
    sciprintf("[%c] %-9s", opsize? 'B' : 'W', d->opcodes[opcode].name);

    seeker++;

    while (formats[opcode][i])

      switch (formats[opcode][i++]) {

      case Script_Invalid: sciprintf("-Invalid operation-"); break;

      case Script_SByte:
      case Script_Byte: sciprintf(" %02x", data[seeker++]); break;

      case Script_Word:
      case Script_SWord:
        sciprintf(" %04x", 0xffff & (data[seeker] | (data[seeker+1] << 8)));
        seeker += 2;
        break;

      case Script_SVariable:
      case Script_Variable:
        if (opsize)
	  param_value = data [seeker++];
        else {
	  param_value = 0xffff & (data[seeker] | (data[seeker+1] << 8));
	  seeker += 2;
        }

        if (opcode == op_callk)
	  sciprintf(" %s[%x]", (param_value < d->kernel_names_nr)
		    ? d->kernel_names[param_value] : "<invalid>", param_value);
        else if (opcode == op_class || (opcode == op_super && i==1))
          sciprintf (" %s[%x]", (d->class_names && param_value < d->class_count) 
                    ? d->class_names[param_value] : "<invalid>", param_value);
        else sciprintf(opsize? " %02x" : " %04x", param_value);

        break;

      case Script_SRelative:
        if (opsize)
	  param_value = data [seeker++];
        else {
	  param_value = 0xffff & (data[seeker] | (data[seeker+1] << 8));
	  seeker += 2;
        }

        dest=seeker+(short) param_value;
        sciprintf (opsize? " %02x  [%04x]" : " %04x  [%04x]", param_value, dest);
        if (opcode == op_lofsa || opcode == op_lofss)
        {
          int atype=script_get_area_type (s, dest);
          if (atype == area_string)
          {
            char buf [80];
            strncpy (buf, &data [dest], sizeof (buf)-1);
            buf [sizeof (buf)-1] = 0;
            if (strlen (buf) > 40)
            {
              buf [40] = 0;
              strcat (buf, "...");
            }
            sciprintf ("%8s; \"%s\"", "", buf);
          }
        }
        break;

      case Script_Property:
        if (opsize)
	  param_value = data [seeker++];
        else {
	  param_value = 0xffff & (data[seeker] | (data[seeker+1] << 8));
	  seeker += 2;
        }

        if (cur_class != -1)
          sciprintf (" %s[%x]", 
             d->snames [d->class_selectors [cur_class][param_value/2]],
             param_value);
        else
          sciprintf(opsize? " %02x" : " %04x", param_value);

        break;
      
      case Script_End: 
        sciprintf ("\n");
        //return;

    }
    sciprintf ("\n");
  }
 
}

void
disassemble_script_pass (disasm_state_t *d, script_state_t *s,
                         resource_t *script, int pass_no)
{
  int _seeker = 0;
  while (_seeker < script->length) {
    int objtype = getInt16(script->data + _seeker);
    int objsize;
    int seeker = _seeker + 4;

    if (!objtype) return;

    if (pass_no == 2)
      sciprintf("\n");

    objsize = getInt16(script->data + _seeker + 2);

    if (pass_no == 2)
    {
      sciprintf("Obj type #%x, offset 0x%x, size 0x%x:\n", objtype, _seeker, objsize);
      if (hexdump) sci_hexdump(script->data + seeker, objsize -4, seeker);
    }

    _seeker += objsize;

    switch (objtype) {
    case sci_obj_object: 
      script_dump_object (d, s, script->data, seeker, objsize, pass_no);
      break;

    case sci_obj_code: 
      script_disassemble_code (d, s, script->data, seeker, objsize, pass_no);
      break;

    case 3: if (pass_no == 2) {
      sciprintf("<unknown>\n");
      sci_hexdump(script->data + seeker, objsize -4, seeker);
    };
    break;

    case sci_obj_said:
      script_dump_said (d, s, script->data, seeker, objsize, pass_no);
      break;

    case sci_obj_strings: 
      script_dump_strings (d, s, script->data, seeker, objsize, pass_no);
      break;

    case sci_obj_class: 
      script_dump_class (d, s, script->data, seeker, objsize, pass_no);
      break;

    case sci_obj_exports:
      script_dump_exports (d, s, script->data, seeker, objsize, pass_no);
      break;
                         
    case sci_obj_pointers: if (pass_no == 2) {
      sciprintf("Pointers\n");
      sci_hexdump(script->data + seeker, objsize -4, seeker);
    };
    break;

    case 9: if (pass_no == 2) {
      sciprintf("<unknown>\n");
      sci_hexdump(script->data + seeker, objsize -4, seeker);
    };
    break;

    case sci_obj_localvars: if (pass_no == 2) {
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
}

void 
disassemble_script(disasm_state_t *d, int res_no, int pass_no)
{
  resource_t *script = findResource(sci_script, res_no);
  script_state_t *s=find_script_state (d, res_no);

  if (!script) {
    sciprintf("Script not found!\n");
    return;
  }

  disassemble_script_pass (d, s, script, pass_no);
}
