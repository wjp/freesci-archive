/***************************************************************************
 kfile.c Copyright (C) 1999 Christoph Reichenbach


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

#include <kernel.h>


/* This assumes modern stream implementations. It may break on DOS. */

#define _K_FILE_MODE_OPEN_OR_FAIL 0
#define _K_FILE_MODE_OPEN_OR_CREATE 1
#define _K_FILE_MODE_CREATE 2

void
kFOpen(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  char *filename = s->heap + UPARAM(0);
  int mode = PARAM(1);
  int retval = 1; /* Ignore file_handles[0] */
  FILE *file = NULL;

  if ((mode == _K_FILE_MODE_OPEN_OR_FAIL) || (mode == _K_FILE_MODE_OPEN_OR_CREATE))
    file = fopen(filename, "r+"); /* Attempt to open existing file */

  if ((!file) && ((mode == _K_FILE_MODE_OPEN_OR_CREATE) || (mode == _K_FILE_MODE_CREATE)))
    file = fopen(filename, "w+"); /* Attempt to create file */

  if (!file) { /* Failed */
    s->acc = 0;
    return;
  }

  while (s->file_handles[retval] && (retval < s->file_handles_nr))
    retval++;

  if (retval == s->file_handles_nr) /* Hit size limit => Allocate more space */
    s->file_handles = g_realloc(s->file_handles, sizeof(FILE *) * ++(s->file_handles_nr));

  s->file_handles[retval] = file;

  s->acc = retval;

}

void
kFClose(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  int handle = UPARAM(0);

  if (handle == 0) {
    SCIkwarn(SCIkERROR, "Attempt to close file handle 0\n");
    return;
  }

  if ((handle >= s->file_handles_nr) || (s->file_handles[handle] == NULL)) {
    SCIkwarn(SCIkERROR, "Attempt to close invalid/unused file handle %d\n", handle);
    return;
  }

  fclose(s->file_handles[handle]);

  s->file_handles[handle] = NULL;
}


void kFPuts(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  int handle = UPARAM(0);
  char *data = UPARAM(1) + s->heap;

  if (handle == 0) {
    SCIkwarn(SCIkERROR, "Attempt to write to file handle 0\n");
    return;
  }

  if ((handle >= s->file_handles_nr) || (s->file_handles[handle] == NULL)) {
    SCIkwarn(SCIkERROR, "Attempt to write to invalid/unused file handle %d\n", handle);
    return;
  }

  fputs(data, s->file_handles[handle]);

}

void
kFGets(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  char *dest = UPARAM(0) + s->heap;
  int maxsize = UPARAM(1);
  int handle = UPARAM(2);

  if (handle == 0) {
    SCIkwarn(SCIkERROR, "Attempt to read from file handle 0\n");
    return;
  }

  if ((handle >= s->file_handles_nr) || (s->file_handles[handle] == NULL)) {
    SCIkwarn(SCIkERROR, "Attempt to read from invalid/unused file handle %d\n", handle);
    return;
  }

  fgets(dest, maxsize, s->file_handles[handle]);

}


/* kGetCWD(address):
** Writes the cwd to the supplied address and returns the address in acc.
** This implementation tries to use a game-specific directory in the user's
** home directory first.
*/
void
kGetCWD(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  char *homedir = getenv("HOME");
  char _cwd[256];
  char *cwd = &(_cwd[0]);
  char *savedir;
  char *targetaddr = UPARAM(0) + s->heap;

  s->acc = PARAM(0);

  if (!homedir) { /* We're probably not under UNIX if this happens */

    if (!getcwd(cwd, 255))
      cwd = "."; /* This might suffice */

    memcpy(targetaddr, cwd, strlen(cwd) + 1);
    return;
  }

  /* So we've got a home directory */

  if (chdir(homedir)) {
    fprintf(stderr,"Error: Could not enter home directory %s.\n", homedir);
    perror("Reason");
    exit(1); /* If we get here, something really bad is happening */
  }

  if (strlen(homedir) > MAX_HOMEDIR_SIZE) {
    fprintf(stderr, "Your home directory path is too long. Re-compile FreeSCI with "
	    "MAX_HOMEDIR_SIZE set to at least %i and try again.\n", strlen(homedir));
    exit(1);
  }

  memcpy(targetaddr, homedir, strlen(homedir));
  targetaddr += strlen(homedir); /* Target end of string for concatenation */
  *targetaddr++ = '/';
  *(targetaddr + 1) = 0;

  if (chdir(FREESCI_GAMEDIR))
    if (scimkdir(FREESCI_GAMEDIR, 0700)) {

      SCIkwarn(SCIkWARNING, "Warning: Could not enter ~/"FREESCI_GAMEDIR"; save files"
	      " will be written to ~/\n");

      return;

    }
    else /* mkdir() succeeded */
      chdir(FREESCI_GAMEDIR);

  memcpy(targetaddr, FREESCI_GAMEDIR, strlen(FREESCI_GAMEDIR));
  targetaddr += strlen(FREESCI_GAMEDIR);
  *targetaddr++ = '/';
  *targetaddr = 0;

  if (chdir(s->game_name))
    if (scimkdir(s->game_name, 0700)) {

      fprintf(stderr,"Warning: Could not enter ~/"FREESCI_GAMEDIR"/%s; "
	      "save files will be written to ~/"FREESCI_GAMEDIR"\n", s->game_name);

      return;
    }
    else /* mkdir() succeeded */
      chdir(s->game_name);

  memcpy(targetaddr, s->game_name, strlen(s->game_name));
  targetaddr += strlen(s->game_name);
  *targetaddr++ = '/';
  *targetaddr++ = 0;

}

#define K_DEVICE_INFO_GET_DEVICE 0
#define K_DEVICE_INFO_GET_CURRENT_DEVICE 1
#define K_DEVICE_INFO_PATHS_EQUAL 2
#define K_DEVICE_INFO_IS_FLOPPY 3

#ifdef _WIN32

void
kDeviceInfo_Win32(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  int mode = UPARAM(0);

  CHECK_THIS_KERNEL_FUNCTION;

  switch(mode) {

  case K_DEVICE_INFO_GET_DEVICE: {
    heap_ptr input = UPARAM(1);
    heap_ptr output = UPARAM(2);
    char *input_s = s->heap + output;
    char *output_s = s->heap + output;

    SCIkASSERT(input >= HEAP_MIN);
    SCIkASSERT(output >= HEAP_MIN);

    strncpy(output_s, input_s, 2);
  }
  break;

  case K_DEVICE_INFO_GET_CURRENT_DEVICE: {
    heap_ptr output = UPARAM(2);
    char *output_s = s->heap + output;

    SCIkASSERT(output >= HEAP_MIN);

    strncpy(output_s, "C:"); /* FIXME */
  }
  break;

  case K_DEVICE_INFO_PATHS_EQUAL: {
    heap_ptr path1 = UPARAM(1);
    heap_ptr path2 = UPARAM(2);
    char *path1_s = s->heap + path1;
    char *path2_s = s->heap + path2;

    SCIkASSERT(path1 >= HEAP_MIN);
    SCIkASSERT(path2 >= HEAP_MIN);

    s->acc = fnmatch(path1_s, path2_s, FNM_PATHNAME); /* POSIX.2 */
  }
  break;

  case K_DEVICE_INFO_IS_FLOPPY: {

    s->acc = 0; /* Never? FIXME */
  }
  break;

  default: {
    SCIkwarn(SCIkERROR, "Unknown DeviceInfo() sub-command: %d\n", mode);
  }
  }
}

#else /* !_WIN32 */

void
kDeviceInfo_Unix(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  int mode = UPARAM(0);

  CHECK_THIS_KERNEL_FUNCTION;

  switch(mode) {

  case K_DEVICE_INFO_GET_DEVICE: {
    heap_ptr output = UPARAM(2);
    char *output_s = s->heap + output;

    SCIkASSERT(output >= HEAP_MIN);

    strcpy(output_s, "/");
  }
  break;

  case K_DEVICE_INFO_GET_CURRENT_DEVICE: {
    heap_ptr output = UPARAM(2);
    char *output_s = s->heap + output;

    SCIkASSERT(output >= HEAP_MIN);

    strcpy(output_s, "/");
  }
  break;

  case K_DEVICE_INFO_PATHS_EQUAL: {
    heap_ptr path1 = UPARAM(1);
    heap_ptr path2 = UPARAM(2);
    char *path1_s = s->heap + path1;
    char *path2_s = s->heap + path2;

    SCIkASSERT(path1 >= HEAP_MIN);
    SCIkASSERT(path2 >= HEAP_MIN);

    s->acc = fnmatch(path1_s, path2_s, FNM_PATHNAME); /* POSIX.2 */
  }
  break;

  case K_DEVICE_INFO_IS_FLOPPY: {

    s->acc = 0; /* Never */
  }
  break;

  default: {
    SCIkwarn(SCIkERROR, "Unknown DeviceInfo() sub-command: %d\n", mode);
  }
  }
}

#endif /* !_WIN32 */


void
kGetSaveDir(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  s->acc = s->save_dir + 2; /* +2 to step over heap block size */
}


void
kValidPath(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  char *pathname = s->heap + UPARAM(0);
  char cpath[PATH_MAX + 1];
  getcwd(cpath, PATH_MAX + 1);

  s->acc = !chdir(pathname); /* Try to go there. If it works, return 1, 0 otherwise. */

  chdir(cpath);
}

