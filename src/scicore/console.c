/***************************************************************************
 console.c Copyright (C) 1999 Christoph Reichenbach, TU Darmstadt


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

    Christoph Reichenbach (CJR) [creichen@rbg.informatik.tu-darmstadt.de]

***************************************************************************/
/* hooks for debug commands (mainly for the sci console) */

#include <sci_memory.h>
#include <engine.h>
#include <console.h>
#ifdef SCI_CONSOLE
#include <sciresource.h>
#include <sound.h>
#include <script.h>

state_t *con_gamestate = NULL;

typedef struct
{
	int (*command) ();
	char *name;
	char *param;
	char *description;
}
cmd_command_t;

typedef struct
{
	union
	{
		int *intp;
		char **charpp;
	}
	var;
	char *name;
	char *description;
}
cmd_var_t;

unsigned int _cmd_command_mem = 0;
unsigned int _cmd_command_count = 0;
cmd_command_t *_cmd_commands;

unsigned int _cmd_var_mem = 0;
unsigned int _cmd_var_count = 0;
cmd_var_t *_cmd_vars = 0;


int con_passthrough = 0;
FILE *con_file = NULL;

static int _lists_need_sorting = 0;

unsigned int cmd_paramlength;
cmd_param_t *cmd_params;

static void(*_con_string_callback)(char*) = NULL;
static void (*_con_pixmap_callback)(gfx_pixmap_t *) = NULL;


/********** dmalloc functions **********/

#ifdef WITH_DMALLOC
int
c_dm_log_heap (state_t * s)
{
	dmalloc_log_heap_map ();
	return 0;
}

int
c_dm_stats (state_t * s)
{
	dmalloc_log_stats ();
	return 0;
}

int
c_dm_log_unfreed (state_t * s)
{
	dmalloc_log_unfreed ();
	return 0;
}

int
c_dm_verify (state_t * s)
{
	unsigned long pointer_var;
	void *ptr;

	pointer_var = strtoul (cmd_params[0].str, NULL, 0);
	ptr = (void *) pointer_var;

	dmalloc_verify (ptr);

	return 0;
}

int
c_dm_debug (state_t * s)
{
	if (cmd_paramlength) {
		long newval = strtol (cmd_params[0].str, NULL, 0);

		sciprintf ("Setting dmalloc_debug(%ld)\n", newval);
		dmalloc_debug (newval);
	}
	else
		sciprintf ("dmalloc_debug is at 0x%lx\n", dmalloc_debug_current ());
	return 0;
}

int
c_dm_mark (state_t * s)
{
	unsigned long mark = dmalloc_mark ();

	dmalloc_message ("------------- MARK 0x%lx ---------------\n", mark);
	sciprintf ("mark 0x%lx\n", mark);
	return 0;
}

int
c_dm_chmark (state_t * s)
{
	unsigned long mark = strtoul (cmd_params[0].str, NULL, 0);
	sciprintf ("Checking mark 0x%lx\n", mark);
	dmalloc_message ("--- Mark 0x%lx:\n", mark);
	dmalloc_log_changed (mark, 1, 1, 1);
	return 0;
}

int
c_dm_print (state_t * s)
{
	int i;
	for (i = 0; i < cmd_paramlength; i++)
		dmalloc_message ("%s\n", cmd_params[i].str);
	return 0;
}

void
con_init_dmalloc ()
{
	con_hook_command (c_dm_log_heap, "dm_log_heap", "",
			  "Dumps the heap state to\n  the dmalloc output file\n\nUSAGE\n\n  dm_log_heap");
	con_hook_command (c_dm_stats, "dm_stats", "",
			  "Prints memory usage stats\n  to the dmalloc output file\n\n  dm_stats");
	con_hook_command (c_dm_log_unfreed, "dm_log_unfreed", "",
			  "Prints unfreed pointer\n  information to the dmalloc\n  output file\n\n"
			  "USAGE\n\n  dm_log_unfreed");
	con_hook_command (c_dm_verify, "dm_verify", "s",
			  "Verifies one pointer,\n  prints output to dmalloc file\n\nUSAGE\n\n"
			  "  dm_verify <ptr>\n  dm_verify 0\n\n  'dm_verify 0' will verify\n  ALL current pointers.\n");
	con_hook_command (c_dm_debug, "dm_debug", "s*",
			  "Sets the dmalloc debug\n  state or displays it\n\nUSAGE\n\n  dm_debug <mode>\n  dm_debug");
	con_hook_command (c_dm_mark, "dm_mark", "",
			  "Gets a mark describing\n  the current heap state\n\nUSAGE\n\n  dm_mark\n\n"
			  "  The mark is written to the\n  dmalloc output file and\n  to sci output.\n\nSEE ALSO\n\n  cm_chmark");
	con_hook_command (c_dm_chmark, "dm_chmark", "s",
			  "Checks changes in the\n  heap state since a certain\n  mark was retreived\n\n"
			  "USAGE\n\n  c_dm_chmark <mark>\n\n  Output is written to the\n  dmalloc output file.\n\n  Use dm_mark to retreive a\n"
			  "  mark.\n\nSEE ALSO\n\n  c_dm_mark");
	con_hook_command (c_dm_print, "dm_print", "s*",
			  "Prints something to the\n  dmalloc output.\n\nUSAGE\n\n  dm_print <text>");
}
#else /* WITH_DMALLOC */

void
con_init_dmalloc ()
{
}

#endif /* WITH_DMALLOC */


void
_cmd_exit (void)
{
	if (_cmd_commands)
		free (_cmd_commands);

	if (_cmd_vars)
		free (_cmd_vars);
}

static int
_comp_command (const void *a, const void *b)
{
	return strcmp (((cmd_command_t *) a)->name, ((cmd_command_t *) b)->name);
}

static int
_comp_var (const void *a, const void *b)
{
	return strcmp (((cmd_var_t *) a)->name, ((cmd_var_t *) b)->name);
}

void
con_sort_all (void)
{
	if (_cmd_command_count)
		qsort (_cmd_commands, _cmd_command_count, sizeof (cmd_command_t),
		       _comp_command);
	if (_cmd_var_count)
		qsort (_cmd_vars, _cmd_var_count, sizeof (cmd_var_t), _comp_var);
	_lists_need_sorting = 0;
}

void
con_init (void)
{
	if (!_cmd_command_mem) {
		_cmd_commands =
			sci_realloc(_cmd_commands, sizeof (cmd_command_t)
				    * (_cmd_command_mem = 32));
		atexit (_cmd_exit);

		/* Hook up some commands */
		con_hook_command (&c_version, "version", "",
				  "Displays the version number");
		con_hook_command (&c_list, "list", "s*",
				  "Lists various things (try 'list')");
		con_hook_command (&c_man, "man", "s",
				  "Gives a short description of something");
		con_hook_command (&c_print, "print", "s", "Prints an int variable");
		con_hook_command (&c_set, "set", "si", "Sets an int variable");
		con_hook_command (&c_size, "size", "si",
				  "Displays the size of a resource");
		con_hook_command (&c_dump, "dump", "si", "HexDumps a resource");
		con_hook_command (&c_hexgrep, "hexgrep", "shh*",
				  "Searches some resources for a\n"
				  "  particular sequence of bytes, re-\n  presented"
				  " as hexadecimal numbers.\n\n"
				  "EXAMPLES:\n  hexgrep script e8 03 c8 00\n"
				  "  hexgrep pic.042 fe");
		con_hook_command (&c_dissectscript, "dissectscript", "i",
				  "Examines a script.");

		con_init_dmalloc ();

		con_hook_int (&con_passthrough, "con_passthrough",
			      "scicon->stdout passthrough");
	}
}

void
con_parse (state_t * s, char *command)
{
	int quote = 0;		/* quoting? */
	int done = 0;			/* are we done yet? */
	int cdone = 0;		/* Done with the current command? */
	char *paramt;			/* parameter types */
	char *cmd = (char *) sci_strdup (command);
	char *_cmd = cmd;
	int pos = 0;

	if (!_cmd_command_mem)
		con_init ();

	while (!done) {
		int cmdnum = -1;		/* command number */
		int onvar = 1;		/* currently working on a variable? */
		unsigned int parammem = 0;
		unsigned int i;
		cdone = 0;
		pos = 0;

		/*    cmd_params = sci_realloc(cmd_params, parammem); */
		cmd_paramlength = 0;

		while (*cmd == ' ')
			cmd++;

		while (!cdone) {
			switch (cmd[pos]) {
			case 0:
				done = 1;
			case ';':
				if (!quote)
					cdone = 1;
			case ' ':
				if (!quote)
					cmd[pos] = onvar = 0;
				break;
			case '\\':		/* don't check next char for special meaning */
				memmove (cmd + pos, cmd + pos + 1, strlen (cmd + pos) - 1);
				break;
			case '"':
				quote ^= 1;
				memmove (cmd + pos, cmd + pos + 1, strlen (cmd + pos));
				pos--;
				break;
			default:
				if (!onvar) {
					onvar = 1;
					if (cmd_paramlength == parammem)
						cmd_params = sci_realloc(cmd_params,
									 sizeof (cmd_param_t)
									 * (parammem += 8));
			    
					cmd_params[cmd_paramlength].str = cmd + pos;

					cmd_paramlength++;
				}
				break;
			}
			pos++;
		}

		if (quote)
			sciprintf ("unbalanced quotes\n");
		else if (strcmp (cmd, "") != 0) {

			for (i = 0; i < _cmd_command_count; i++)
				if (strcmp (_cmd_commands[i].name, cmd) == 0)
					cmdnum = i;

			if (cmdnum == -1)
				sciprintf ("%s: not found\n", cmd);
			else {
				unsigned int minparams;

				paramt = _cmd_commands[cmdnum].param;

				minparams = strlen (paramt);
				if ((paramt[0] != 0) && (paramt[strlen (paramt) - 1] == '*'))
					minparams -= 2;

				if (cmd_paramlength < minparams)
					sciprintf ("%s: needs more than %d parameters\n",
						   cmd, cmd_paramlength);

				else if ((cmd_paramlength > strlen (paramt))
					 && ((strlen (paramt) == 0)
					     || paramt[strlen (paramt) - 1] != '*'))
					sciprintf ("%s: too many parameters", cmd);
				else {
					int carry = 1;
					char paramtype;
					int paramtypepos = 0;
					char *endptr;

					for (i = 0; i < cmd_paramlength; i++) {
						paramtype = paramt[paramtypepos];

						if ((paramt[paramtypepos + 1])
						    && (paramt[paramtypepos + 1] != '*'))
							paramtypepos++;
						/* seek next param type unless end of string or '*						   ' */

						switch (paramtype) {
							/* Now turn the parameters into variables of the appropriate types,
							** unless they're strings, and store them in the global cmd_params[]
							** structure  */
						case 'i': {
							char *orgstr = cmd_params[i].str;

							cmd_params[i].val = strtol (orgstr, &endptr, 0);
							if (*endptr != '\0') {
								carry = 0;
								sciprintf ("%s: '%s' is not an int\n", cmd, orgstr);
							}
						}
							break;

						case 'h': {
							char *orgstr = cmd_params[i].str;

							cmd_params[i].val = strtol (orgstr, &endptr, 16);

							if (*endptr != '\0') {
								carry = 0;
								sciprintf ("%s: '%s' is not a hex number\n", cmd, orgstr);
							}

							cmd_params[i].val &= 0xff;	/* Clip hex numbers to 0x00 ... 0xff */
						}
							break;

						case 's':
							break;

						default:
							fprintf(stderr, "Internal error: Heap corruption or prior assertion failed:\n"
								"Unknown parameter type '%c' for funtion\n", paramtype);

						}
					}

					if (carry == 1)
						_cmd_commands[cmdnum].command (s);
				}
			}
		}
		cmd += pos;
	}

	free (_cmd);
	if (cmd_params)
		free (cmd_params);
	cmd_params = NULL;

}


int
con_hook_command (int command (state_t *), char *name, char *param,
		  char *description)
{
	unsigned int i;

	if (NULL == name) {
		sciprintf("console.c: con_hook_command(): NULL passed for name\n");
		return -1;
	}

	if (!_cmd_command_mem)
		con_init ();
	if (_cmd_command_mem == _cmd_command_count)
		_cmd_commands = sci_realloc(_cmd_commands,
					    sizeof (cmd_command_t) *
					    (_cmd_command_mem <<= 1));

	if (command == NULL)
		return 1;
	for (i = 0; i < _cmd_command_count; i++)
		if (strcmp (_cmd_commands[i].name, name) == 0)
			return 1;

	if (param == NULL)
		param = "";

	if (description == NULL)
		description = "";

	i = 0;
	while (param[i] != 0) {
		switch (param[i]) {
		case '*':
			if (param[i + 1] != 0)
				return 1;
			if (i == 0)
				return 1;
		case 'h':
		case 'i':
		case 's':
			break;
		default:
			return 1;
		}
		i++;
	}

	_cmd_commands[_cmd_command_count].command = command;
	_cmd_commands[_cmd_command_count].name = name;
	_cmd_commands[_cmd_command_count].param = param;
	_cmd_commands[_cmd_command_count].description = description;

	_cmd_command_count++;

	_lists_need_sorting = 1;
	return 0;
}


int
con_hook_int (int *pointer, char *name, char *description)
{
	unsigned int i;

	if (_cmd_var_mem == _cmd_var_count)
		_cmd_vars = sci_realloc(_cmd_vars,
					sizeof (cmd_var_t) * (_cmd_var_mem += 16));

	if (pointer == NULL)
		return 1;
	for (i = 0; i < _cmd_var_count; i++)
		if (strcmp (_cmd_vars[i].name, name) == 0)
			return 1;

	if (description == NULL)
		description = "";

	_cmd_vars[_cmd_var_count].var.intp = pointer;
	_cmd_vars[_cmd_var_count].name = name;
	_cmd_vars[_cmd_var_count].description = description;

	_cmd_var_count++;

	_lists_need_sorting = 1;
	return 0;
}



/****************************************/
/* sciprintf                            */
/****************************************/


int
sciprintf (char *fmt, ...)
{
	va_list argp;
	size_t bufsize = 256;
	unsigned int i;
	char *buf 	= (char *) sci_malloc (bufsize);

	if (NULL == fmt) {
		fprintf(stderr, "console.c: sciprintf(): NULL passed for parameter fmt\n");
		return -1;
	}

	if (NULL == buf) {
		fprintf(stderr, "console.c: sciprintf(): malloc failed for buf\n");
		return -1;
	}

	va_start (argp, fmt);
	while ((i = vsnprintf (buf, bufsize - 1, fmt, argp)) == -1
	       || (i >= bufsize - 2)) {
		/* while we're out of space... */
		va_end (argp);
		va_start (argp, fmt);	/* reset argp */

		free (buf);
		buf = (char *) sci_malloc (bufsize <<= 1);
	}
	va_end (argp);

	if (con_passthrough)
		printf ("%s", buf);
	if (con_file)
		fprintf (con_file, "%s", buf);

	
	if (_con_string_callback)
		_con_string_callback(buf);
}

void
con_set_string_callback(void(*callback)(char *))
{
	_con_string_callback = callback;
}

void
con_set_pixmap_callback(void(*callback)(gfx_pixmap_t *))
{
	_con_pixmap_callback = callback;
}

int
con_can_handle_pixmaps()
{
	return _con_pixmap_callback != NULL;
}

int
con_insert_pixmap(gfx_pixmap_t *pixmap)
{
	if (_con_pixmap_callback)
		_con_pixmap_callback(pixmap);
	else
		return 1;
	return 0;
}


void
open_console_file (char *filename)
{
  if (con_file != NULL)
    fclose (con_file);

  if (NULL == filename)
  {
    fprintf(stderr, "console.c: open_console_file(): NULL passed for parameter filename\r\n");
  }
#ifdef WIN32
  con_file = fopen (filename, "wt");
#else
  con_file = fopen (filename, "w");
#endif

  if (NULL == con_file)
    fprintf(stderr, "console.c: open_console_file(): Could not open output file %s\n", filename);

}

void
close_console_file ()
{
  if (con_file != NULL)
  {
    fclose (con_file);
    con_file = NULL;
  }
}


/***************************************************************************
 * Console commands and support functions
 ***************************************************************************/


int
getResourceNumber (char *resid)
/* Gets the resource number of a resource string, or returns -1 */
{
  int i, res = -1;

  for (i = 0; i < sci_invalid_resource; i++)
    if (strcmp (sci_resource_types[i], resid) == 0)
      res = i;
  return res;
}

int
c_version (state_t * s)
{
	if (NULL == s) {
		sciprintf("console.c: c_version: NULL passed for parameter s\n");
		return -1;
	}

	sciprintf ("FreeSCI, version " VERSION "\n");
	sciprintf ("Running %s\n", sci_version_types[s->resmgr->sci_version]);
	return 0;
}

int
c_list_words(state_t *s)
{
	word_t **words;
	int words_nr;
	int i;

	words = vocab_get_words(s->resmgr, &words_nr);

	if (!words) {
		sciprintf("No vocabulary.\n");
		return 1;
	}

	for (i = 0; i < words_nr; i++)
		sciprintf("%4d: %03x [%03x] %s\n",
			  i,
			  words[i]->w_class,
			  words[i]->group,
			  words[i]->word);

	vocab_free_words(words, words_nr);
	return 0;
}

int
c_list_suffices(state_t *s)
{
	suffix_t **suffices;
	int suffices_nr;
	int i;
	char word_buf[256], alt_buf[256];

	suffices = vocab_get_suffices(s->resmgr, &suffices_nr);

	if (!suffices) {
		sciprintf("No suffix vocabulary.\n");
		return 1;
	}

	for (i = 0; i < suffices_nr; i++) {
		suffix_t *suf = suffices[i];

		strncpy(word_buf, suf->word_suffix,
			suf->word_suffix_length);
		word_buf[suf->word_suffix_length] = 0;
		strncpy(alt_buf, suf->alt_suffix,
			suf->alt_suffix_length);
		alt_buf[suf->alt_suffix_length] = 0;

		sciprintf("%4d: (%03x) -%12s  =>  -%12s (%03x)\n",
			  i, suf->class_mask, word_buf,
			  alt_buf, suf->result_class);
	}

	vocab_free_suffices(s->resmgr, suffices, suffices_nr);
	return 0;
}


int
c_list (state_t * s)
{
	if (_lists_need_sorting)
		con_sort_all ();

	if (!s) {
		sciprintf("You need a state to do that!\n");
		return 1;
	}

	if (cmd_paramlength == 0)
		{
			sciprintf ("usage: list [type]\nwhere type is one of the following:\n"
				   "cmds       - lists all commands\n"
				   "vars       - lists all variables\n"
				   "\n"
				   "restypes   - lists all resource types\n"
				   "selectors  - lists all selectors\n"
				   "syscalls   - lists all kernel functions\n"
				   "words      - lists all kernel words\n"
				   "suffixes   - lists all suffix replacements\n"
				   "[resource] - lists all [resource]s");
		}
	else if (cmd_paramlength == 1) {

		if (strcmp ("cmds", cmd_params[0].str) == 0) {
			unsigned int i;
			for (i = 0; i < _cmd_command_count; i++)
				sciprintf ("%s (%s)\n", _cmd_commands[i].name,
					   _cmd_commands[i].param);

		}
		else if (!strcmp("selectors", cmd_params[0].str))
			return c_selectornames(s);
		else if (!strcmp("syscalls", cmd_params[0].str))
			return c_kernelnames(s);
		else if (!strcmp("suffixes", cmd_params[0].str)
			 || !strcmp("suffices", cmd_params[0].str)
			 || !strcmp("sufficos", cmd_params[0].str))
			/* sufficos: Accusative Plural of 'suffix' */
			return c_list_suffices(s);
		else if (!strcmp("words", cmd_params[0].str))
			return c_list_words(s);
		else if (strcmp ("restypes", cmd_params[0].str) == 0) {
			int i;
			for (i = 0; i < sci_invalid_resource; i++)
				sciprintf ("%s\n", sci_resource_types[i]);

		}
		else if (strcmp ("vars", cmd_params[0].str) == 0) {
			unsigned int i;

			for (i = 0; i < _cmd_var_count; i++)
				sciprintf ("%s = %d\n", _cmd_vars[i].name, *(_cmd_vars[i].var.intp));
		}
		else {
			int res = getResourceNumber (cmd_params[0].str);
			if (res == -1)
				sciprintf ("Unknown resource type: '%s'\n", cmd_params[0].str);
			else {
				int i;
				for (i = 0; i < 1000; i++)
					if (scir_test_resource (s->resmgr, res, i))
						sciprintf ("%s.%03d\n", sci_resource_types[res], i);
			}
		}
	}
	else
		sciprintf ("list can only be used with one argument");
	return 0;
}


int
c_man (state_t * s)
{
  unsigned int i;
  for (i = 0; i < _cmd_command_count; i++)
    if (strcmp (cmd_params[0].str, _cmd_commands[i].name) == 0)
    {
      char *paramseeker = _cmd_commands[i].param;
      sciprintf ("-- COMMAND: %s\nSYNOPSIS:\n", _cmd_commands[i].name);

      sciprintf ("  %s", _cmd_commands[i].name);
      while (*paramseeker)
      {
	switch (*paramseeker)
	{
	case 'i':
	  sciprintf (" (int)");
	  break;
	case 's':
	  sciprintf (" (string)");
	  break;
	case 'h':
	  sciprintf (" (hexbyte)");
	  break;
	case '*':
	  sciprintf ("*");
	  break;
	default:
	  sciprintf (" (Unknown(%c))", *paramseeker);
	}
	paramseeker++;
      }

      sciprintf ("\n\nDESCRIPTION:\n  %s\n", _cmd_commands[i].description);
    }
  for (i = 0; i < _cmd_var_count; i++)
    if (strcmp (cmd_params[0].str, _cmd_vars[i].name) == 0)
    {
      sciprintf ("-- VARIABLE: (int) %s\n\nDESCRIPTION:\n  ",
		 _cmd_vars[i].name);
      sciprintf (_cmd_vars[i].description);
      sciprintf ("\n");
    }
  return 0;
}

int
c_set (state_t * s)
{
  unsigned int i;

  for (i = 0; i < _cmd_var_count; i++)
    if (strcmp (cmd_params[0].str, _cmd_vars[i].name) == 0)
      *(_cmd_vars[i].var.intp) = cmd_params[1].val;

  return 0;
}

int
c_print (state_t * s)
{
  unsigned int i;

  for (i = 0; i < _cmd_var_count; i++)
    if (strcmp (cmd_params[0].str, _cmd_vars[i].name) == 0)
      sciprintf ("%d", *(_cmd_vars[i].var.intp));

  return 0;
}


int
c_size (state_t * s)
{
  int res = getResourceNumber (cmd_params[0].str);
  if (res == -1)
    sciprintf ("Resource type '%s' is not valid\n", cmd_params[0].str);
  else
  {
    resource_t *resource = scir_find_resource(s->resmgr, res, cmd_params[1].val, 0);
    if (resource)
    {
      sciprintf ("Size: %d\n", resource->size);
    }
    else
      sciprintf ("Resource %s.%03d not found\n", cmd_params[0].str,
		 cmd_params[1].val);
  }
  return 0;
}


int
c_dump (state_t * s)
{
  int res = getResourceNumber (cmd_params[0].str);

  if (res == -1)
    sciprintf ("Resource type '%s' is not valid\n", cmd_params[0].str);
  else
  {
    resource_t *resource = scir_find_resource(s->resmgr, res, cmd_params[1].val, 0);
    if (resource)
      sci_hexdump (resource->data, resource->size, 0);
    else
      sciprintf ("Resource %s.%03d not found\n", cmd_params[0].str,
		 cmd_params[1].val);
  }
  return 0;
}


int
c_hexgrep (state_t * s)
{
  int i, seeklen, resnr, restype, resmax;
  unsigned char *seekstr	= NULL;
  resource_t *script		= NULL;
  char *dot = strchr (cmd_params[0].str, '.');

  if (NULL == s)
  {
    fprintf(stderr, "console.c: c_hexgrep(): NULL passed for s\r\n");
    return(-1);
  }

  seekstr = sci_malloc (seeklen = (cmd_paramlength - 1));

  if (NULL == seekstr)
  {
    fprintf(stderr, "console.c: c_hexgrep(): malloc failed for seekstr\r\n");
    return(-1);
  }

  for (i = 0; i < seeklen; i++)
    seekstr[i] = (byte)cmd_params[i + 1].val;

  if (dot)
  {
    *dot = 0;
    resmax = resnr = atoi (dot + 1);
  }
  else
  {
    resnr = 0;
    resmax = 999;
  }

  if ((restype = getResourceNumber (cmd_params[0].str)) == -1)
  {
    sciprintf ("Unknown resource type \"%s\"\n", cmd_params[0].str);
    free(seekstr);
    return 1;
  }

  for (; resnr <= resmax; resnr++)
    if ((script = scir_find_resource(s->resmgr, restype, resnr, 0)))
    {
      int seeker = 0, seekerold = 0;
      int comppos = 0;
      int output_script_name = 0;

      while (seeker < script->size)
      {

	if (script->data[seeker] == seekstr[comppos])
	{
	  if (comppos == 0)
	    seekerold = seeker;

	  comppos++;

	  if (comppos == seeklen)
	  {
	    comppos = 0;
	    seeker = seekerold + 1;

	    if (!output_script_name)
	    {
	      sciprintf ("\nIn %s.%03d:\n", sci_resource_types[restype], resnr);
	      output_script_name = 1;
	    }
	    sciprintf ("   0x%04x\n", seekerold);
	  }
	}
	else
	  comppos = 0;

	seeker++;
      }
    }

  free (seekstr);

  return 0;
}


int
c_selectornames (state_t * s)
{
  int namectr;
  char **snames	= NULL;
  int seeker = 0;

  if (NULL == s)
  {
    sciprintf("console.c: c_selectornames(): NULL passed for parameter s\n");
    return -1;
  }

  snames = vocabulary_get_snames (s->resmgr, &namectr, s ? s->version : 0);

  if (!snames)
  {
    sciprintf ("No selector name table found!\n");
    return 1;
  }

  sciprintf ("Selector names in numeric order:\n");
  while (snames[seeker])
  {
    sciprintf ("%03x: %s\n", seeker, snames[seeker]);
    seeker++;
  }
  vocabulary_free_snames (snames);
  return 0;
}

int
c_kernelnames (state_t * s)
{
  int knamectr;
  char **knames = vocabulary_get_knames (s->resmgr, &knamectr);
  int seeker = 0;

  if (NULL == s) {
    sciprintf("console.c: c_kernelnames NULL passed for parameter s\n");
    return -1;
  }

  if (!knames)
  {
    sciprintf ("No kernel name table found!\n");
    return 1;
  }

  sciprintf ("Syscalls in numeric order:\n");
  for (seeker = 0; seeker < knamectr; seeker++)
    sciprintf ("%03x: %s\n", seeker, knames[seeker]);

  vocabulary_free_knames (knames);
  return 0;
}

int
c_dissectscript (state_t * s)
{
  if (NULL == s)
  {
    sciprintf("console.c: c_dissectscript(): NULL passed for parameter s\n");
    return -1;
  }

  script_dissect (s->resmgr, cmd_params[0].val, s->selector_names, s->selector_names_nr);
  return 0;
}

#endif /* SCI_CONSOLE */


