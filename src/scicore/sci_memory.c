/***************************************************************************
 sci_memory.c Copyright (C) 2001 Alexander R Angas


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

    Alexander R Angas (Alex Angas) <wgd@internode.on.net>

 History:

   20010815 - assembled from the various memory allocation functions lying
              about, namely console.c (extra dmalloc stuff), menubar.c
              (for malloc_cpy -> strdup, malloc_ncpy -> strndup).
                -- Alex Angas

***************************************************************************/

#include "sci_memory.h"

/* set optimisations for Win32: */
/* g on: enable global optimizations */
/* t on: use fast code */
/* y on: suppress creation of frame pointers on stack */
/* s off: disable minimize size code */

#ifdef _WIN32
#include <crtdbg.h>
#	ifdef NDEBUG
#		pragma optimize( "s", off )
#		pragma optimize( "gty", on )
#		pragma intrinsic( memcpy, strlen )
#	endif
#endif


inline void *
_SCI_MALLOC(size_t size, char *file, int line, char *funct, int debug)
{
	void *res;
	ALLOC_MEM((res = malloc(size)), size, file, line, funct, debug)
	return res;
}


inline void *
_SCI_CALLOC(size_t num, size_t size, char *file, int line, char *funct, int debug)
{
	void *res;
	ALLOC_MEM((res = calloc(num, size)), num * size, file, line, funct, debug)
	return res;
}


inline void *
_SCI_REALLOC(void *ptr, size_t size, char *file, int line, char *funct, int debug)
{
	void *res;
	ALLOC_MEM((res = realloc(ptr, size)), size, file, line, funct, debug)
	return res;
}


inline void
_SCI_FREE(void *ptr, char *file, int line, char *funct, int debug)
{
	if (debug)
		fprintf(stderr, "_SCI_FREE() [%s:(%s)%u]\n",
			file, funct, line);
	assert(ptr);
	free(ptr);
}


inline void *
_SCI_MEMDUP(void *ptr, size_t size, char *file, int line, char *funct, int debug)
{
	void *res;
	ALLOC_MEM((res = malloc(size)), size, file, line, funct, debug)
	memcpy(res, ptr, size);
	return res;
}


inline char *
_SCI_STRDUP(const char *src, char *file, int line, char *funct, int debug)
{
	void *res;
	ALLOC_MEM((res = strdup(src)), strlen(src), file, line, funct, debug)
	return res;
}


inline char *
_SCI_STRNDUP(const char *src, size_t length, char *file, int line, char *funct, int debug)
{
	void *res;
	char *strres;
	int rlen = MIN(strlen(src), length) + 1;
	ALLOC_MEM((res = malloc(rlen)), rlen, file, line, funct, debug)

	strres = res;
	strncpy(strres, src, rlen);
	strres[rlen - 1] = 0;

	return strres;
}


/********** Win32 functions **********/

#ifdef _WIN32
void
debug_win32_memory(int dbg_setting)
{
	int tmpFlag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);

#ifdef NDEBUG
	fprintf(stderr,
		"WARNING: Cannot debug Win32 memory when not in debug mode.\n");
#endif

	if (dbg_setting > 0)
		tmpFlag |= _CRTDBG_CHECK_ALWAYS_DF;
		/* call _CrtCheckMemory at every request */

	if (dbg_setting > 1)
        tmpFlag |= _CRTDBG_LEAK_CHECK_DF;
		/* perform automatic leak checking at program exit */

	if (dbg_setting > 2)
        tmpFlag |= _CRTDBG_DELAY_FREE_MEM_DF;
		/* enable debug heap allocations */

	if (dbg_setting > 3)
	{
		PANIC((stderr, "Invalid value for debug_win32_memory!\n"));
		BREAKPOINT();
	}

	if (dbg_setting <= 0)
	{
		/* turn off above */
		tmpFlag &= ~_CRTDBG_CHECK_ALWAYS_DF;
        tmpFlag &= ~_CRTDBG_DELAY_FREE_MEM_DF;
        tmpFlag &= ~_CRTDBG_LEAK_CHECK_DF;
	}

	/* set new state for flag */
	_CrtSetDbgFlag( tmpFlag );
}
#endif
