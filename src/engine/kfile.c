/***************************************************************************
 Kfile.c Copyright (C) 1999 Christoph Reichenbach


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

#ifdef _WIN32
#  ifndef PATH_MAX
#    define PATH_MAX 255
#  endif
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#else
#  include <unistd.h>
#  include <dirent.h>
#endif

#ifndef O_BINARY
#define O_BINARY 0
#endif

static int _savegame_indices_nr = -1; /* means 'uninitialized' */

static struct _savegame_index_struct {
	int id;
	long timestamp;
} _savegame_indices[MAX_SAVEGAME_NR];

/* This assumes modern stream implementations. It may break on DOS. */


/* Attempts to mirror a file by copying it from the resource firectory
** to the working directory. Returns NULL if the file didn't exist.
** Otherwise, the new file is then opened for reading or writing.
*/
static FILE *
f_open_mirrored(state_t *s, char *fname)
{
	int fd;
	char *buf = NULL;
	struct stat fstate;

	chdir(s->resource_dir);
	fd = sci_open(fname, O_RDONLY | O_BINARY);
	if (!IS_VALID_FD(fd)) {
		chdir(s->work_dir);
		return NULL;
	}

	fstat(fd, &fstate);
	if (fstate.st_size) {
		buf = sci_malloc(fstate.st_size);
		read(fd, buf, fstate.st_size);
	}

	close(fd);

	chdir(s->work_dir);

	/* Visual C++ doesn't allow to specify O_BINARY with creat() */
#ifdef _MSC_VER
	fd = _open(fname, _O_CREAT | _O_BINARY | _O_RDWR, _S_IREAD | _S_IWRITE);
#else
	fd = creat(fname, 0600);
#endif

	if (!IS_VALID_FD(fd) && buf) {
		free(buf);
		sciprintf("kfile.c: f_open_mirrored(): Warning: Could not create '%s' in '%s' (%d bytes to copy)\n",
			  fname, s->work_dir, fstate.st_size);
		return NULL;
	}

	if (fstate.st_size) {
		int ret;
		ret = write(fd, buf, fstate.st_size);
		if (ret < fstate.st_size) {
			sciprintf("kfile.c: f_open_mirrored(): Warning: Could not write all %ld bytes to '%s' in '%s' (only wrote %ld)\n",
				  (long)fstate.st_size, fname, s->work_dir, ret);
		}

		free(buf);
	}

	close(fd);

	return sci_fopen(fname, "r" FO_BINARY "+");
}


#define _K_FILE_MODE_OPEN_OR_FAIL 0
#define _K_FILE_MODE_OPEN_OR_CREATE 1
#define _K_FILE_MODE_CREATE 2


void
file_open(state_t *s, char *filename, int mode)
{
	int retval = 1; /* Ignore file_handles[0] */
	FILE *file = NULL;

	SCIkdebug(SCIkFILE, "Opening file %s with mode %d\n", filename, mode);
	if ((mode == _K_FILE_MODE_OPEN_OR_FAIL) || (mode == _K_FILE_MODE_OPEN_OR_CREATE)) {
		file = sci_fopen(filename, "r" FO_BINARY "+"); /* Attempt to open existing file */
		SCIkdebug(SCIkFILE, "Opening file %s with mode %d\n", filename, mode);
		if (!file) {
			SCIkdebug(SCIkFILE, "Failed. Attempting to copy from resource dir...\n");
			file = f_open_mirrored(s, filename);
			if (file)
				SCIkdebug(SCIkFILE, "Success!\n");
			else
				SCIkdebug(SCIkFILE, "Not found.\n");
		}
	}

	if ((!file) && ((mode == _K_FILE_MODE_OPEN_OR_CREATE) || (mode == _K_FILE_MODE_CREATE))) {
			file = sci_fopen(filename, "w" FO_BINARY "+"); /* Attempt to create file */
			SCIkdebug(SCIkFILE, "Creating file %s with mode %d\n", filename, mode);
	}
	if (!file) { /* Failed */
		SCIkdebug(SCIkFILE, "file_open() failed\n");
		s->acc = 0;
		return;
	}

	while (s->file_handles[retval] && (retval < s->file_handles_nr))
		retval++;

	if (retval == s->file_handles_nr) /* Hit size limit => Allocate more space */
		s->file_handles = sci_realloc(s->file_handles, sizeof(FILE *) * ++(s->file_handles_nr));

	s->file_handles[retval] = file;

	s->acc = retval;
}

void
kFOpen(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  file_open(s, (char *) s->heap + UPARAM(0), UPARAM(1));
}

void file_close(state_t *s, int handle)
{
  SCIkdebug(SCIkFILE, "Closing file %d\n", handle);

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

void
kFClose(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  file_close(s, UPARAM(0));
}

void fputs_wrapper(state_t *s, int handle, char *data)
{
  SCIkdebug(SCIkFILE, "FPuts'ing \"%s\" to handle %d\n", data, handle);

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

void fwrite_wrapper(state_t *s, int handle, char *data, int length)
{
  SCIkdebug(SCIkFILE, "fwrite()'ing \"%s\" to handle %d\n", data, handle);

  if (handle == 0) {
    SCIkwarn(SCIkERROR, "Attempt to write to file handle 0\n");
    return;
  }

  if ((handle >= s->file_handles_nr) || (s->file_handles[handle] == NULL)) {
    SCIkwarn(SCIkERROR, "Attempt to write to invalid/unused file handle %d\n", handle);
    return;
  }

  fwrite(data, 1, length, s->file_handles[handle]);
}


void kFPuts(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  int handle = UPARAM(0);
  char *data = (char *) (UPARAM(1) + s->heap);

  fputs_wrapper(s, handle, data);
}

static void
fgets_wrapper(state_t *s, char *dest, int maxsize, int handle)
{
  SCIkdebug(SCIkFILE, "FGets'ing %d bytes from handle %d\n", maxsize, handle);


  if (handle == 0) {
    SCIkwarn(SCIkERROR, "Attempt to read from file handle 0\n");
    return;
  }

  if ((handle >= s->file_handles_nr) || (s->file_handles[handle] == NULL)) {
    SCIkwarn(SCIkERROR, "Attempt to read from invalid/unused file handle %d\n", handle);
    return;
  }

  fgets(dest, maxsize, s->file_handles[handle]);

  SCIkdebug(SCIkFILE, "FGets'ed \"%s\"\n", dest);
}


static void
fread_wrapper(state_t *s, char *dest, int bytes, int handle)
{
  SCIkdebug(SCIkFILE, "fread()'ing %d bytes from handle %d\n", bytes, handle);

  if (handle == 0) {
    SCIkwarn(SCIkERROR, "Attempt to read from file handle 0\n");
    return;
  }

  if ((handle >= s->file_handles_nr) || (s->file_handles[handle] == NULL)) {
    SCIkwarn(SCIkERROR, "Attempt to read from invalid/unused file handle %d\n", handle);
    return;
  }

  s->acc=fread(dest, 1, bytes, s->file_handles[handle]);
}


static void
fseek_wrapper(state_t *s, int handle, int offset, int whence)
{

  if (handle == 0) {
    SCIkwarn(SCIkERROR, "Attempt seek on file handle 0\n");
    return;
  }

  if ((handle >= s->file_handles_nr) || (s->file_handles[handle] == NULL)) {
    SCIkwarn(SCIkERROR, "Attempt seek on invalid/unused file handle %d\n", handle);
    return;
  }

  s->acc=fseek(s->file_handles[handle], offset, whence);
}


static char *
_chdir_savedir(state_t *s)
{
	char *cwd = sci_getcwd();
	char *save_dir = (char *) s->heap + s->save_dir + 2;

	if (chdir(save_dir) && sci_mkpath(save_dir)) {

		sciprintf(__FILE__": Can't chdir to savegame dir '%s' or "
			  "create it\n", save_dir);

		sci_free(cwd);
		return NULL;
	}

	if (!cwd)
		cwd = strdup(s->work_dir);

	return cwd; /* Potentially try again */
}

static void
_chdir_restoredir(char *dir)
{
	if (chdir(dir)) {
		sciprintf(__FILE__": Can't seemt to return to previous homedir '%s'\n",
			  dir);
	}
	free(dir);
}

#define TEST_DIR_OR_QUIT(dir) if (!dir) { s->acc = 0; return; }


void
kFGets(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  char *dest = (char *) (UPARAM(0) + s->heap);
  int maxsize = UPARAM(1);
  int handle = UPARAM(2);

  fgets_wrapper(s, dest, maxsize, handle);
  s->acc = UPARAM(0);
}


/* kGetCWD(address):
** Writes the cwd to the supplied address and returns the address in acc.
*/
void
kGetCWD(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
	char *wd = sci_getcwd();
	heap_ptr offset = UPARAM(0);
	char *targetaddr = (char *) s->heap + offset;

	s->acc = offset;
	strncpy(targetaddr, wd, MAX_SAVE_DIR_SIZE - 1);
	targetaddr[MAX_SAVE_DIR_SIZE - 1] = 0; /* Terminate */

	SCIkdebug(SCIkFILE, "Copying cwd='%s'(%d chars) to %p"
		  " (heap starts at %p, offset=%04x)\n",
		  wd, strlen(wd), targetaddr, s->heap, offset);

	free(wd);
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

#ifdef _MSC_VER
    s->acc = !stricmp (path1_s, path2_s);
#else
    s->acc = !strcasecmp (path1_s, path2_s);
#endif
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


  switch(mode) {

  case K_DEVICE_INFO_GET_DEVICE: {
    heap_ptr output = UPARAM(2);
    char *output_s = (char *) s->heap + output;

    SCIkASSERT(output >= HEAP_MIN);

    strcpy(output_s, "/");
  }
  break;

  case K_DEVICE_INFO_GET_CURRENT_DEVICE: {
    heap_ptr output = UPARAM(1);
    char *output_s = (char *) s->heap + output;

    SCIkASSERT(output >= HEAP_MIN);

    strcpy(output_s, "/");
  }
  break;

  case K_DEVICE_INFO_PATHS_EQUAL: {
    heap_ptr path1 = UPARAM(1);
    heap_ptr path2 = UPARAM(2);
    char *path1_s = (char *) s->heap + path1;
    char *path2_s = (char *) s->heap + path2;

    SCIkASSERT(path1 >= HEAP_MIN);
    SCIkASSERT(path2 >= HEAP_MIN);

#ifndef HAVE_FNMATCH_H
#ifndef _DOS
#  warning "File matches will be unprecise!"
#endif
    s->acc = !strcmp(path1_s, path2_s);
#else
    s->acc = fnmatch(path1_s, path2_s, FNM_PATHNAME); /* POSIX.2 */
#endif
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
  char *testpath = sci_malloc(strlen(path) + 15);
  char buf[1024];
  int i;
  int fd;
  int failed = 0;
  int pathlen;

  strcpy(testpath, path);
  strcat(testpath, "freesci.foo");
  pathlen = strlen(testpath);

  while (IS_VALID_FD(fd = open(testpath, O_RDONLY))) {
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

  fd = creat(testpath, 0600);

  if (!IS_VALID_FD(fd)) {
    SCIkwarn(SCIkWARNING,"Could not test for disk space: %s\n", strerror(errno));
    SCIkwarn(SCIkWARNING,"Test path was '%s'\n", testpath);
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
	char suffices[] = "0123456789abcdefghijklmnopqrstuvwxyz";
	char *savedir_name = sci_malloc(strlen(FREESCI_SAVEDIR_PREFIX) + 2);
	assert(nr >= 0);
	assert(nr < MAX_SAVEGAME_NR);
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
	char *buf = NULL;

	for (i = 0; i < MAX_SAVEGAME_NR; i++) {
		if (!chdir((buf = _k_get_savedir_name(i)))) {
			char namebuf[32]; /* Save game name buffer */
			FILE *idfile = sci_fopen(game_id_file, "r");

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
		free(buf);
	}
	return 0;
}


int
test_savegame(state_t *s, char *savegame_id, char *savegame_name, int savegame_name_length)
{
	int retval = 1;
	char *game_id = (char *) s->game_name;
	char *game_id_file = (char *) sci_malloc(strlen(game_id) + strlen(FREESCI_ID_SUFFIX) + 1);

	strcpy(game_id_file, game_id);
	strcat(game_id_file, FREESCI_ID_SUFFIX);

	if (chdir(savegame_id)) {
		retval = 0; /* Couldn't enter savedir */
	}  else {

		if (_k_check_file(FREESCI_FILE_HEAP, SCI_HEAP_SIZE))
			retval = 0;
		if (_k_check_file(FREESCI_FILE_STATE, 1))
			retval = 0;
		if (savegame_name) {
			if (_k_check_file(game_id_file, 1)) {
				*savegame_name = 0;
			} else {
				int foo = open(game_id_file, O_RDONLY);
				int bytes_read = read(foo, savegame_name, savegame_name_length);
				savegame_name[MIN(bytes_read, savegame_name_length)] = 0; /* terminate */
				close(foo);
			}
		} else if (_k_check_file(game_id_file, 1))
			retval = 0;

		chdir ("..");
	}
	free(game_id_file);
	return retval;
}

void
kCheckSaveGame(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
#if 0
	int savedir_nr = UPARAM(1);
	char *buf = NULL;
	char *workdir = _chdir_savedir(s);
	TEST_DIR_OR_QUIT(workdir);



	if (soundserver_dead) {
		sciprintf("Soundserver is dead- cannot save game state!");
		_chdir_restoredir(workdir);
		s->acc = 0;
		return;
	}

	if (savedir_nr > MAX_SAVEGAME_NR-1) {
		_chdir_restoredir(workdir);
		s->acc = 0;
		return;
	}

	s->acc = test_savegame(s, (buf = _k_get_savedir_name(savedir_nr)), NULL, 0);

	_chdir_restoredir(workdir);
	free(buf);
#else
	s->acc = 0;
#endif
}


#define get_file_mtime(fd) get_file_mtime_Unix(fd)
/* Returns the time of the specified file's last modification
** Parameters: (int) fd: The file descriptor of the file in question
** Returns   : (long) An integer value describing the time of the
**                    file's last modification.
** The only thing that must be ensured is that
** get_file_mtime(f1) > get_file_mtime(f2)
**   <=>
** if f1 was modified at a later point in time than the last time
** f2 was modified.
*/

static long
get_file_mtime_Unix(int fd) /* returns the  */
{
	struct stat fd_stat;
	fstat(fd, &fd_stat);

	return fd_stat.st_ctime;
}


static int
_savegame_index_struct_compare(const void *a, const void *b)
{
	return ((struct _savegame_index_struct *)b)->timestamp
		- ((struct _savegame_index_struct *)a)->timestamp;
}

static void
update_savegame_indices(char *gfname)
{
	int i;
	int gfname_len = strlen(gfname);

	_savegame_indices_nr = 0;

	for (i = 0; i < MAX_SAVEGAME_NR; i++) {
		char *dirname = _k_get_savedir_name(i);
		int fd;

		if (!chdir(dirname)) {

			if (IS_VALID_FD(fd = sci_open(gfname, O_RDONLY))) {
				_savegame_indices[_savegame_indices_nr].id = i;
				_savegame_indices[_savegame_indices_nr++].timestamp = get_file_mtime(fd);
				close(fd);
			}
			chdir("..");
		}

		free(dirname);
	}

	qsort(_savegame_indices, _savegame_indices_nr, sizeof(struct _savegame_index_struct),
	      _savegame_index_struct_compare);

}


void
kGetSaveFiles(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
	char *game_id = (char *) (UPARAM(0) + s->heap);
	heap_ptr nametarget = UPARAM(1);
	heap_ptr nameoffsets = UPARAM(2);
	int gfname_len = strlen(game_id) + strlen(FREESCI_ID_SUFFIX) + 1;
	char *gfname = sci_malloc(gfname_len);
	int i;
	char *workdir = _chdir_savedir(s);
	TEST_DIR_OR_QUIT(workdir);

	strcpy(gfname, game_id);
	strcat(gfname, FREESCI_ID_SUFFIX); /* This file is used to identify in-game savegames */

	update_savegame_indices(gfname);


	SCIkASSERT(UPARAM(0) >= 800);
	SCIkASSERT(nametarget >= 800);
	SCIkASSERT(nameoffsets >= 800);

	s->acc = 0;

	for (i = 0; i < _savegame_indices_nr; i++) {
		char *savedir_name = _k_get_savedir_name(_savegame_indices[i].id);
		FILE *idfile;

		if (!chdir(savedir_name)) {


			if ((idfile = sci_fopen(gfname, "r"))) { /* Valid game ID file: Assume valid game */
				char namebuf[SCI_MAX_SAVENAME_LENGTH]; /* Save game name buffer */
				fgets(namebuf, SCI_MAX_SAVENAME_LENGTH-1, idfile);
				if (strlen(namebuf) > 0) {

					if (namebuf[strlen(namebuf) - 1] == '\n')
						namebuf[strlen(namebuf) - 1] = 0; /* Remove trailing newline */

					++s->acc; /* Increase number of files found */

					PUT_HEAP(nameoffsets, i); /* Write down the savegame number */
					nameoffsets += 2; /* Make sure the next ID string address is written to the next pointer */
					strncpy((char *) s->heap + nametarget, namebuf, SCI_MAX_SAVENAME_LENGTH); /* Copy identifier string */
					s->heap[nametarget + SCI_MAX_SAVENAME_LENGTH - 1] = 0; /* Make sure it's terminated */
					nametarget += SCI_MAX_SAVENAME_LENGTH; /* Increase name offset pointer accordingly */

					fclose(idfile);
				}
			}
			chdir("..");
		}
		free(savedir_name);
	}

	free(gfname);
	s->heap[nametarget] = 0; /* Terminate list */

	_chdir_restoredir(workdir);
}


void
kSaveGame(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
#if 0
	char *game_id = (char *) (UPARAM(0) + s->heap);
	char *savegame_dir;
	int savedir_nr = UPARAM(1);
	int savedir_id; /* Savegame ID, derived from savedir_nr and the savegame ID list */
	char *game_id_file_name = sci_malloc(strlen(game_id) + strlen(FREESCI_ID_SUFFIX) + 1);
	char *game_description = (char *) (UPARAM(2) + s->heap);
	char *workdir = _chdir_savedir(s);
	TEST_DIR_OR_QUIT(workdir);

	if (soundserver_dead) {
		sciprintf("Soundserver is dead- cannot save game state!");
		s->acc = 0;
		_chdir_restoredir(workdir);
		return;
	}


	strcpy(game_id_file_name, game_id);
	strcat(game_id_file_name, FREESCI_ID_SUFFIX);

	update_savegame_indices(game_id_file_name);

	if (savedir_nr >= 0 && savedir_nr < _savegame_indices_nr)
		/* Overwrite */
		savedir_id = _savegame_indices[savedir_nr].id;
	else if (savedir_nr >= 0 && savedir_nr < MAX_SAVEGAME_NR) {
		int i = 0;

		savedir_id = 0;

		/* First, look for holes */
		while (i < _savegame_indices_nr)
			if (_savegame_indices[i].id == savedir_id) {
				++savedir_id;
				i = 0;
			} else ++i;

		if (savedir_id >= MAX_SAVEGAME_NR) {
			sciprintf("Internal error: Free savegame ID is %d, shouldn't happen!\n",
				  savedir_id);
			s->acc = 0;
			return;
		}

		/* This loop terminates when savedir_id is not in [x | ex. n. _savegame_indices[n].id = x] */
	} else {
		sciprintf("Savegame ID %d is not allowed!\n", savedir_nr);
		s->acc = 0;
		return;
	}

	savegame_dir = _k_get_savedir_name(savedir_id);

	s->acc = 1;

	if (gamestate_save(s, savegame_dir)) {
		sciprintf("Saving the game failed.\n");
		s->acc = 0;
	} else {
		FILE *idfile;

		chdir(savegame_dir);

		if ((idfile = sci_fopen(game_id_file_name, "w"))) {

			fprintf(idfile, game_description);
			fclose(idfile);

		} else {
			sciprintf("Creating the game ID file failed.\n");
			sciprintf("You can still restore from inside the debugger with \"restore_game %s\"\n", savegame_dir);
			s->acc = 0;
		}

		chdir ("..");
	}
	free(game_id_file_name);
	_chdir_restoredir(workdir);
#else
	s->acc = 0;
#endif
}


void
kRestoreGame(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
#if 0
	char *game_id = (char *) (UPARAM(0) + s->heap);
	int savedir_nr = UPARAM(1);
	char *workdir = _chdir_savedir(s);
	TEST_DIR_OR_QUIT(workdir);

	if (_savegame_indices_nr < 0) {
		char *game_id_file_name = sci_malloc(strlen(game_id) + strlen(FREESCI_ID_SUFFIX) + 1);

		strcpy(game_id_file_name, game_id);
		strcat(game_id_file_name, FREESCI_ID_SUFFIX);
		SCIkwarn(SCIkWARNING, "Savegame index list not initialized!\n");
		update_savegame_indices(game_id_file_name);
	}

	savedir_nr = _savegame_indices[savedir_nr].id;


	if (savedir_nr > -1) {
		char *savedir_name = _k_get_savedir_name(savedir_nr);
		state_t *newstate = gamestate_restore(s, savedir_name);

		free(savedir_name);

		if (newstate) {

			s->successor = newstate;
			script_abort_flag = SCRIPT_ABORT_WITH_REPLAY; /* Abort current game */
			s->execution_stack_pos = s->execution_stack_base;

		} else {
			sciprintf("Restoring failed (game_id = '%s').\n", game_id);
		}

	} else {
		s->acc = 1;
		sciprintf("Savegame #%d not found!\n", savedir_nr);
	}

	_chdir_restoredir(workdir);
#else
	s->acc = -1;
#endif
}


void
kValidPath(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
	heap_ptr offset = UPARAM(0);
	char *pathname = (char *) s->heap + offset;
	char cpath[PATH_MAX + 1];
	getcwd(cpath, PATH_MAX + 1);

	if (offset == s->save_dir_copy) {
		pathname = s->save_dir_copy_buf;
		SCIkdebug(SCIkFILE, "Magic: ValidPath(savedir_tmp)\n");
	}

	s->acc = !chdir(pathname); /* Try to go there. If it works, return 1, 0 otherwise. */

	chdir(cpath);
}

#define K_FILEIO_OPEN 		0
#define K_FILEIO_CLOSE		1
#define K_FILEIO_READ_RAW	2
#define K_FILEIO_WRITE_RAW	3
#define K_FILEIO_UNLINK		4
#define K_FILEIO_READ_STRING	5
#define K_FILEIO_WRITE_STRING	6
#define K_FILEIO_SEEK		7
#define K_FILEIO_FIND_FIRST	8
#define K_FILEIO_FIND_NEXT	9
#define K_FILEIO_STAT		10


char *
write_filename_to_mem(state_t *s, heap_ptr address, char *string)
{
	if (string) {
		memset(s->heap + address, 0, 12);
		strncpy((char *) s->heap + address, string, 11);
	}

	return string;
}

void
next_file(state_t *s)
{
	if (write_filename_to_mem(s, s->dirseeker_outbuffer,
				  sci_find_next(&(s->dirseeker))))
		s->acc = s->dirseeker_outbuffer;
	else
		s->acc = 0;
}

void
first_file(state_t *s, char *dir, char *mask, heap_ptr buffer)
{
	if (buffer == 0) {
		sciprintf("Warning: first_file(state,\"%s\",\"%s\", 0) invoked!\n",
			  dir, mask);
		s->acc = 0;
		return;
	}

	if (strcmp(dir, ".")) {
		sciprintf("%s L%d: Non-local first_file: Not implemented yet\n",
			  __FILE__, __LINE__);
		s->acc = 0;
		return;
	}

	if (s->dirseeker_outbuffer)
		sci_finish_find(&(s->dirseeker));

	s->dirseeker_outbuffer = buffer;

	if (write_filename_to_mem(s, s->dirseeker_outbuffer,
				  sci_find_first(&(s->dirseeker), mask)))
		s->acc = s->dirseeker_outbuffer;
	else
		s->acc = 0;
}

void
kFileIO(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  int func_nr = UPARAM(0);


  switch (func_nr) {

    case K_FILEIO_OPEN :
    {
	char *name = (char *) s->heap + UPARAM(1);
	int mode = UPARAM(2);

	file_open(s, name, mode);
	break;
    }
    case K_FILEIO_CLOSE :
    {
	int handle = UPARAM(1);

	file_close(s, handle);
	break;
    }
    case K_FILEIO_READ_RAW :
    {
	char *dest = (char *) s->heap + UPARAM(2);
	int size = UPARAM(3);
	int handle = UPARAM(1);

	fread_wrapper(s, dest, size, handle);
	break;
    }
    case K_FILEIO_WRITE_RAW :
    {
	char *buf = (char *) s->heap + UPARAM(2);
	int size = UPARAM(3);
	int handle = UPARAM(1);

	fwrite_wrapper(s, handle, buf, size);
	break;
    }
    case K_FILEIO_UNLINK :
    {
	char *name = (char *) (s->heap + UPARAM(1));

	unlink(name);
	break;
    }
    case K_FILEIO_READ_STRING :
    {
	char *dest = (char *) (s->heap + UPARAM(1));
	int size = UPARAM(2);
	int handle = UPARAM(3);

	fgets_wrapper(s, dest, size, handle);
	break;
    }
    case K_FILEIO_WRITE_STRING :
    {
	char *buf = (char *) (s->heap + UPARAM(1));
	int size = UPARAM(2);
	int handle = UPARAM(3);

	fputs_wrapper(s, handle, buf);
	break;
    }
    case K_FILEIO_SEEK :
    {
	int handle = UPARAM(1);
	int offset = UPARAM(2);
	int whence = UPARAM(3);

	fseek_wrapper(s, handle, offset, whence);
	break;
    }
    case K_FILEIO_FIND_FIRST :
    {
	char *mask = (char *) (s->heap + UPARAM(1));
	heap_ptr buf = UPARAM(2);
	int attr = UPARAM(3); /* We won't use this, Win32 might, though... */

#ifndef _WIN32
	if (strcmp(mask, "*.*")==0) strcpy(mask, "*"); /* For UNIX */
#endif
	first_file(s, ".", mask, buf);

	break;
    }
    case K_FILEIO_FIND_NEXT :
    {
	next_file(s);
	break;
    }
    case K_FILEIO_STAT :
    {
	char *name = (char *) (s->heap + UPARAM(1));
	s->acc=1-_k_check_file(name, 0);
	break;
    }
    default :
        SCIkwarn(SCIkERROR, "Unknown FileIO() sub-command: %d\n", func_nr);
  }
}
