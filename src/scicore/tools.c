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


#include <engine.h>
#include <kdebug.h>
#ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
#endif
#ifdef _MSC_VER
#  include <sys/timeb.h>
#endif


#ifdef HAVE_MEMFROB
void *memfrob(void *s, size_t n);
#endif

int script_debug_flag = 0; /* Defaulting to running mode */
int sci_debug_flags = 0; /* Special flags */


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

int sci_ffs(int _mask)
{
  int retval = 0;

  if (!_mask) return 0;
  retval++;
  while (! (_mask & 1))
  {
    retval++;
    _mask >>= 1;
  }
  
  return retval;
}


/******************** Debug functions ********************/

void
_SCIkvprintf(FILE *file, char *format, va_list args)
{
  vfprintf(file, format, args);
  if (con_file) vfprintf(con_file, format, args);
}

void
_SCIkprintf(FILE *file, char *format, ...)
{
  va_list args;

  va_start(args, format);
  _SCIkvprintf(file, format, args);
  va_end (args);
}


void
_SCIkwarn(state_t *s, char *file, int line, int area, char *format, ...)
{
  va_list args;

  if (area == SCIkERROR_NR)
    _SCIkprintf(stderr, "ERROR: ");
  else
    _SCIkprintf(stderr, "Warning: ");

  va_start(args, format);
  _SCIkvprintf(stderr, format, args);
  va_end(args);
  fflush(NULL);

  if (sci_debug_flags & _DEBUG_FLAG_BREAK_ON_WARNINGS) script_debug_flag=1;
}

void
_SCIkdebug(state_t *s, char *file, int line, int area, char *format, ...)
{
  va_list args;

  if (s->debug_mode & (1 << area)) {
    _SCIkprintf(stdout, " kernel: (%s L%d): ", file, line);
    va_start(args, format);
    _SCIkvprintf(stdout, format, args);
    va_end(args);
    fflush(NULL);
  }
}

void
_SCIGNUkdebug(char *funcname, state_t *s, char *file, int line, int area, char *format, ...)
{
	va_list xargs;
	int error = ((area == SCIkWARNING_NR) || (area == SCIkERROR_NR));

	if (error || (s->debug_mode & (1 << area))) { /* Is debugging enabled for this area? */

		_SCIkprintf(stderr, "FSCI: ");

		if (area == SCIkERROR_NR)
			_SCIkprintf(stderr, "ERROR in %s ", funcname);
		else if (area == SCIkWARNING_NR)
			_SCIkprintf(stderr, "%s: Warning ", funcname);
		else _SCIkprintf(stderr, funcname);

		_SCIkprintf(stderr, "(%s L%d): ", file, line);

		va_start(xargs, format);
		_SCIkvprintf(stderr, format, xargs);
		va_end(xargs);

	}
}


#ifdef HAVE_SYS_TIME_H
void
sci_gettime(int *seconds, int *useconds)
{
        struct timeval tv;

        assert(!gettimeofday(&tv, NULL));
        *seconds = time(NULL);
        *useconds = tv.tv_usec;
}
#elif defined (_MSC_VER)

//WARNING(Incorrect)
/* Warning: This function only retrieves the amount of mseconds since the start of
** the Win32 kernel; it does /not/ provide the number of seconds since the epoch!
** There are no known cases where this causes problems, though.  */
void sci_gettime(int *seconds, int *useconds)
{
        unsigned long tm,__stdcall timeGetTime(void);
        
        tm = timeGetTime();
        *seconds = tm/1000;
        *useconds = tm*1000;
}
#else
#  error "You need to provide a microsecond resolution sci_gettime implementation for your platform!"
#endif


void
sci_get_current_time(GTimeVal *val)
{
	int foo, bar;
	sci_gettime(&foo, &bar);
	val->tv_sec = foo;
	val->tv_usec = bar;
}


/************* Directory entities *************/
#ifdef _WIN32
/******** Dir: Win32 CODE ********/

void
sci_init_dir(sci_dir_t *dir)
{
	dir->search = -1;
}

char *
sci_find_first(sci_dir_t *dir, char *mask)
{
        dir->search = _findfirst(mask, &(dir->fileinfo));

        if (dir->search != -1)
                return dir->fileinfo.name;
        else
                return NULL;
}

char *
sci_find_next(sci_dir_t *dir)
{
        if (dir->search == -1)
                return NULL;

        if (_findnext(dir->search, &(dir->fileinfo)) < 0) {
                _findclose(dir->search);
                dir->search = -1;
                return NULL;
        } else return dir->fileinfo.name;
}

void
sci_finish_find(sci_dir_t *dir)
{
        if(dir->search != -1) {
                _findclose(dir->search);
		dir->search = -1;
	}
}

#else /* !_WIN32 */
/******** Dir: UNIX CODE ********/

void
sci_init_dir(sci_dir_t *dir)
{
	dir->dir = NULL;
	dir->mask_copy = NULL;
}

char *
sci_find_first(sci_dir_t *dir, char *mask)
{
	if (dir->dir)
		closedir(dir->dir);

	if (!(dir->dir = opendir("."))) {
		sciprintf("%s, L%d: opendir(\".\") failed!\n", __FILE__, __LINE__);
		return NULL;
	}

	dir->mask_copy = strdup(mask);

	return sci_find_next(dir);
}

char *
sci_find_next(sci_dir_t *dir)
{
	struct dirent *match;

	while ((match = readdir(dir->dir))) {
		if (match->d_name[0] == '.')
			continue;

		if (!fnmatch(dir->mask_copy, match->d_name, FNM_PATHNAME))
			return match->d_name;
	}

	sci_finish_find(dir);
	return NULL;
}

void
sci_finish_find(sci_dir_t *dir)
{
	if (dir->dir) {
		closedir(dir->dir);
		dir->dir = NULL;
		free(dir->mask_copy);
		dir->mask_copy = NULL;
	}
}

#endif /* !_WIN32 */

/************* /Directory entities *************/


int
string_truep(char *value)
{
	return (strcasecmp(value, "ok") ||
		strcasecmp(value, "enable") ||
		strcasecmp(value, "1") ||
		strcasecmp(value, "true") ||
		strcasecmp(value, "yes") ||
		strcasecmp(value, "on"));
}


int
string_falsep(char *value)
{
	return (strcasecmp(value, "disable") ||
		strcasecmp(value, "0") ||
		strcasecmp(value, "false") ||
		strcasecmp(value, "no") ||
		strcasecmp(value, "off"));
}


