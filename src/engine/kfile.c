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

#include <engine.h>

#ifdef WIN32
#define PATH_MAX 255
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

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
  char dir_buffer [MAX_PATH], dir_buffer2 [MAX_PATH];
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

    GetFullPathName (input_s, sizeof (dir_buffer)-1, dir_buffer, NULL);
    strncpy(output_s, dir_buffer, 2);
    output_s [2] = 0;
  }
  break;

  case K_DEVICE_INFO_GET_CURRENT_DEVICE: {
    heap_ptr output = UPARAM(2);
    char *output_s = s->heap + output;

    SCIkASSERT(output >= HEAP_MIN);

    _getcwd (dir_buffer, sizeof (dir_buffer)-1);
    strncpy(output_s, dir_buffer, 2);
    output_s [2] = 0;
  }
  break;

  case K_DEVICE_INFO_PATHS_EQUAL: {
    heap_ptr path1 = UPARAM(1);
    heap_ptr path2 = UPARAM(2);
    char *path1_s = s->heap + path1;
    char *path2_s = s->heap + path2;

    SCIkASSERT(path1 >= HEAP_MIN);
    SCIkASSERT(path2 >= HEAP_MIN);

    GetFullPathName (path1_s, sizeof (dir_buffer)-1, dir_buffer, NULL);
    GetFullPathName (path2_s, sizeof (dir_buffer2)-1, dir_buffer2, NULL);
    
    s->acc = !stricmp (path1_s, path2_s);
  }
  break;

  case K_DEVICE_INFO_IS_FLOPPY: {
    heap_ptr input = UPARAM(1);
    char *input_s = s->heap + input;

    GetFullPathName (input_s, sizeof (dir_buffer)-1, dir_buffer, NULL);
    dir_buffer [3] = 0;  /* leave X:\ */
    
    s->acc = (GetDriveType (dir_buffer) == DRIVE_REMOVABLE);
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
kCheckFreeSpace(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  char *path = (char *) s->heap + UPARAM(0);
  char *testpath = malloc(strlen(path) + 15);
  char buf[1024];
  int i;
  int fd;
  int failed = 0;
  int pathlen;

  strcpy(testpath, path);
  strcat(testpath, "freesci.foo");
  pathlen = strlen(testpath);

  while ((fd = open(testpath, O_RDONLY)) > -1) {
    close(fd);
    if (testpath[pathlen - 2] == 'z') { /* Failed. */
      SCIkwarn(SCIkWARNING, "Failed to find non-existing file for free space test\n");
      free(testpath);
      s->acc = 0;
      return;
    }

    /* If this file couldn't be created, try freesci.fop, freesci.foq etc.,
    ** then freesci.fpa, freesci.fpb. Stop at freesci.fza.
    ** Yes, this is extremely arbitrary and very strange.
    */
    if (testpath[pathlen - 1] == 'z') {
      testpath[pathlen - 1] = 'a';
      ++testpath[pathlen - 2];
    }
    else
      ++testpath[pathlen - 1];
  }

  fd = creat(testpath, O_WRONLY);

  if (fd == -1) {
    SCIkwarn(SCIkWARNING,"Could not test for disk space: %s\n", strerror(errno));
    free(testpath);
    s->acc = 0;
    return;
  }

  for (i = 0; i < 1024; i++) /* Check for 1 MB */
    if (write(fd, buf, 1024) < 1024)
      failed = 1;

  close(fd);

  remove(testpath);

  s->acc = !failed;

  free(testpath);
}


/* Returns a dynamically allocated pointer to the name of the requested save dir */
char *
_k_get_savedir_name(int nr)
{
  char suffices[] = "0123456789abcdef";
  char *savedir_name = malloc(strlen(FREESCI_SAVEDIR_PREFIX) + 2);
  assert(nr >= 0);
  assert(nr < 16);
  strcpy(savedir_name, FREESCI_SAVEDIR_PREFIX);
  savedir_name[strlen(FREESCI_SAVEDIR_PREFIX)] = suffices[nr];
  savedir_name[strlen(FREESCI_SAVEDIR_PREFIX) + 1] = 0;

  return savedir_name;
}


int
_k_check_file(char *filename, int minfilesize)
     /* Returns 0 if the file exists and is big enough */
{
  struct stat state;

  if (stat(filename, &state))
    return 1;

  return (state.st_size < minfilesize);
}

int
_k_find_savegame_by_name(char *game_id_file, char *name)
{
  int savedir_nr = -1;
  int i;

  for (i = 0; i < 16; i++) {
    if (!chdir(_k_get_savedir_name(i))) {
      char namebuf[32]; /* Save game name buffer */
      FILE *idfile = fopen(game_id_file, "r");

      if (idfile) {
	fgets(namebuf, 31, idfile);
	if (strlen(namebuf) > 0)
	  if (namebuf[strlen(namebuf) - 1] == '\n')
	    namebuf[strlen(namebuf) - 1] = 0; /* Remove trailing newlines */

	if (strcmp(name, namebuf) == 0) {
	  sciprintf("Save game name matched entry %d\n", i);
	  savedir_nr = i;
	}

	fclose(idfile);
      }

      chdir("..");
    }
  }
}

void
kCheckSaveGame(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  char *game_id = UPARAM(0) + s->heap;
  char *game_id_file = malloc(strlen(game_id) + strlen(FREESCI_ID_SUFFIX) + 1);
  int savedir_nr = UPARAM(1);

  CHECK_THIS_KERNEL_FUNCTION;

  strcpy(game_id_file, game_id);
  strcat(game_id_file, FREESCI_ID_SUFFIX);

  if (savedir_nr > 15) {
    s->acc = 0;
    free(game_id_file);
    return;
  }

  s->acc = 1;

  if (chdir(_k_get_savedir_name(savedir_nr))) {
    s->acc = 0; /* Couldn't enter savedir */
}  else {
    int fh;

    if (_k_check_file(FREESCI_FILE_HEAP, SCI_HEAP_SIZE))
      s->acc = 0;
    if (_k_check_file(FREESCI_FILE_STATE, 1))
      s->acc = 0;
    if (_k_check_file(game_id_file, 1))
      s->acc = 0;

    chdir ("..");
  }

  free(game_id_file);
}


void
kRestoreGame(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  char *game_id = UPARAM(0) + s->heap;
  int savedir_nr = UPARAM(1);

  CHECK_THIS_KERNEL_FUNCTION;

  if (savedir_nr > -1) {
    state_t *newstate = gamestate_restore(s, _k_get_savedir_name(savedir_nr));

    if (newstate) {

      s->successor = newstate;
      script_abort_flag = 1; /* Abort current game */

    } else {
      sciprintf("Restoring failed.\n");
    }

  } else {
    s->acc = 1;
    sciprintf("Savegame #%d not found!\n", savedir_nr);
  }
}


void
kGetSaveFiles(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  char *game_id = UPARAM(0) + s->heap;
  heap_ptr nametarget = UPARAM(1);
  heap_ptr nameoffsets = UPARAM(2);
  int gfname_len = strlen(game_id) + strlen(FREESCI_ID_SUFFIX) + 1;
  char *gfname = malloc(gfname_len);
  int i;

  strcpy(gfname, game_id);
  strcat(gfname, FREESCI_ID_SUFFIX); /* This file is used to identify in-game savegames */

  CHECK_THIS_KERNEL_FUNCTION;

  SCIkASSERT(UPARAM(0) >= 800);
  SCIkASSERT(nametarget >= 800);
  SCIkASSERT(nameoffsets >= 800);

  s->acc = 0;

  for (i = 0; i < 16; i++) {
    char *gidf_name = malloc(gfname_len + 3 + strlen(FREESCI_SAVEDIR_PREFIX));
    char *savedir_name = _k_get_savedir_name(i);
    FILE *idfile;

    strcpy(gidf_name, savedir_name);
    strcat(gidf_name, G_DIR_SEPARATOR_S);
    strcat(gidf_name, gfname);

    free(savedir_name);

    if (idfile = fopen(gidf_name, "r")) { /* Valid game ID file: Assume valid game */
      char namebuf[32]; /* Save game name buffer */
      fgets(namebuf, 31, idfile);
      if (strlen(namebuf) > 0) {

	if (namebuf[strlen(namebuf) - 1] == '\n')
	  namebuf[strlen(namebuf) - 1] = 0; /* Remove trailing newline */

	++s->acc; /* Increase number of files found */

	PUT_HEAP(nameoffsets, i); /* Write down the savegame number */
	nameoffsets += 2; /* Make sure the next ID string address is written to the next pointer */
	strncpy(s->heap + nametarget, namebuf, SCI_MAX_SAVENAME_LENGTH); /* Copy identifier string */
	s->heap[nametarget + SCI_MAX_SAVENAME_LENGTH - 1] = 0; /* Make sure it's terminated */
	nametarget += SCI_MAX_SAVENAME_LENGTH; /* Increase name offset pointer accordingly */

	fclose(idfile);
      }
    }

    free(gidf_name);
  }

  free(gfname);
  s->heap[nametarget] = 0; /* Terminate list */
}


void
kSaveGame(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  char *game_id = UPARAM(0) + s->heap;
  char *savegame_dir = _k_get_savedir_name(UPARAM(1));
  char *game_description = UPARAM(2) + s->heap;

  CHECK_THIS_KERNEL_FUNCTION;
  s->acc = 1;

  if (gamestate_save(s, savegame_dir)) {
    sciprintf("Saving the game failed.\n");
    s->acc = 0;
  } else {
    FILE *idfile;
    char *game_id_file_name = malloc(strlen(game_id) + strlen(FREESCI_ID_SUFFIX) + 1);

    strcpy(game_id_file_name, game_id);
    strcat(game_id_file_name, FREESCI_ID_SUFFIX);

    chdir(savegame_dir);

    if ((idfile = fopen(game_id_file_name, "w"))) {

      fprintf(idfile, game_description);
      fclose(idfile);

    } else {
      sciprintf("Creating the game ID file failed.\n");
      sciprintf("You can still restore from inside the debugger with \"restore_game %s\"\n", savegame_dir);
      s->acc = 0;
    }

    chdir ("..");
  }
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

