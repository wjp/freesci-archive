/***************************************************************************
 resource.c Copyright (C) 1999 Christoph Reichenbach, TU Darmstadt


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

 History:

   990327 - created (CJR)

***************************************************************************/
/* Resource library */

#include <sci_memory.h>
#include <sciresource.h>
#include <assert.h>
#include <vocabulary.h> /* For SCI version auto-detection */

#include <sys/types.h>
#include <sys/stat.h>

#ifndef O_BINARY
#define O_BINARY 0
/* Not defined on most systems */
#endif

#undef SCI_REQUIRE_RESOURCE_FILES
#define SCI_VERBOSE_RESMGR 1


const char* sci_version_types[] = {
	"SCI version undetermined (Autodetect failed / not run)",
	"SCI version 0.xxx",
	"SCI version 0.xxx w/ 1.000 compression",
	"SCI version 1.000 (early)",
	"SCI version 1.000 (late)",
	"SCI version 1.001",
	"SCI WIN/32"
};

const int sci_max_resource_nr[] = {65536, 1000, 1000, 1000, 8192};

const char* sci_error_types[] = {
	"No error",
	"I/O error",
	"Resource is empty (size 0)",
	"resource.map entry is invalid",
	"resource.map file not found",
	"No resource files found",
	"Unknown compression method",
	"Decompression failed: Decompression buffer overflow",
	"Decompression failed: Sanity check failed",
	"Decompression failed: Resource too big",
	"SCI version is unsupported"};

const char* sci_resource_types[] = {"view","pic","script","text","sound",
				    "memory","vocab","font","cursor",
				    "patch","bitmap","palette","cdaudio",
				    "audio","sync","message","map","heap"};
/* These are the 18 resource types supported by SCI1 */

const char *sci_resource_type_suffixes[] = {"v56","p56","scr","tex","snd",
					    "   ","voc","fon","cur","pat",
					    "bit","pal","cda","aud","syn",
					    "msg","map","hep"};


int resourcecmp(const void *first, const void *second);


typedef int decomp_funct(resource_t *result, int resh);

static decomp_funct *decompressors[] = {
	NULL,
	&decompress0,
	&decompress01,
	&decompress1e,
	&decompress1l,
	&decompress11,
	NULL
};

#if 0
int loadResourcePatches(struct singly_linked_resources_struct *resourcelist)
     /* Adds external patch files to an unprepared list of resources
     ** (as used internally by loadResources). It will override any
     ** resources from the original resource files, freeing their memory
     ** in the process.
     */
{
	sci_dir_t dir;
	char *entry;
	int counter = 0;

	sci_init_dir(&dir);
	entry = sci_find_first(&dir, "*.???");
	while (entry) {
		int restype = sci_invalid_resource;
		int resnumber = -1;
		int i;
		unsigned int resname_len;
		char *endptr;

		for (i = sci_view; i < sci_invalid_resource; i++)
			if (strncasecmp(sci_resource_types[i], entry,
					strlen(sci_resource_types[i])) == 0)
				restype = i;

		if (restype != sci_invalid_resource) {

			resname_len = strlen(sci_resource_types[restype]);
			if (entry[resname_len] != '.')
				restype = sci_invalid_resource;
			else {
				resnumber = strtol(entry + 1 + resname_len,
						   &endptr, 10); /* Get resource number */
				if ((*endptr != '\0') || (resname_len+1 == strlen(entry)))
					restype = sci_invalid_resource;

				if ((resnumber < 0) || (resnumber > 1000))
					restype = sci_invalid_resource;
			}
		}

		if (restype != sci_invalid_resource) {
			struct stat filestat;

			printf("Patching \"%s\": ", entry);

			if (stat(entry, &filestat))
				perror("""__FILE__"": (""__LINE__""): stat()");
			else {
				int file;
				guint8 filehdr[2];
				resource_t *newrsc;

				if (filestat.st_size < 3) {
					printf("File too small\n");
					entry = sci_find_next(&dir);
					continue; /* next file */
				}

				file = open(entry, O_RDONLY);
				if (!file)
					perror("""__FILE__"": (""__LINE__""): open()");
				else {

					read(file, filehdr, 2);
					if ((filehdr[0] & 0x7f) != restype) {
						printf("Resource type mismatch\n");
						close(file);
					} else {

						newrsc = sci_malloc(sizeof(resource_t));
#ifdef SATISFY_PURIFY
						memset(newrsc, 0, sizeof(resource_t));
#endif

						newrsc->size = filestat.st_size - 2;
						newrsc->id = restype << 11 | resnumber;
						newrsc->number = resnumber;
						newrsc->status = SCI_STATUS_ALLOCATED;
						newrsc->type = restype;

						newrsc->data = sci_malloc(newrsc->size);
#ifdef SATISFY_PURIFY
						memset(newrsc->data, 0, newrsc->size);
#endif

						read(file, newrsc->data, newrsc->size);
						close(file);

						_addResource(resourcelist, newrsc, 1); /* Add and overwrite old stuff */

						printf("OK\n");

						counter++;
					}
				}
			}
		}
		entry = sci_find_next(&dir);
	}

	return counter;
}
#endif


int resourcecmp (const void *first, const void *second)
{
	if (((resource_t *)first)->type ==
	    ((resource_t *)second)->type)
		return (((resource_t *)first)->number <
			((resource_t *)second)->number)? -1 :
		!(((resource_t *)first)->number ==
		  ((resource_t *)second)->number);
	else
		return (((resource_t *)first)->type <
			((resource_t *)second)->type)? -1 : 1;
}

/*------------------------------------------------*/
/** Resource manager constructors and operations **/
/*------------------------------------------------*/

void
_scir_add_altsource(resource_t *res, int file, unsigned int file_offset)
{
	resource_source_t *rsrc = sci_malloc(sizeof(resource_source_t));

	rsrc->next = res->alt_sources;
	rsrc->file = file;
	rsrc->file_offset = file;
	res->alt_sources = rsrc;
}

resource_mgr_t *
scir_new_resource_manager(char *dir, int version,
			  char allow_patches, int max_memory)
{
	int resource_error = 0;
	resource_mgr_t *mgr = sci_malloc(sizeof(resource_mgr_t));
	char *caller_cwd = sci_getcwd();
	int resmap_version = version;

	if (chdir(dir)) {
		sciprintf("Resmgr: Directory '%s' is invalid!\n", dir);
		free(caller_cwd);
		return NULL;
	}

	mgr->max_memory = max_memory;

	mgr->memory_locked = 0;
	mgr->memory_lru = 0;

	mgr->resource_path = dir;

	mgr->resources = NULL;

	if (version <= SCI_VERSION_01) {
		resource_error =
			sci0_read_resource_map(dir,
					       &mgr->resources,
					       &mgr->resources_nr);

		if (resource_error >= SCI_ERROR_CRITICAL) {
			sciprintf("Resmgr: Error while loading resource map: %s\n",
				  sci_error_types[resource_error]);
			sci_free(mgr);
			chdir(caller_cwd);
			free(caller_cwd);
			return NULL;
		}

		resmap_version = SCI_VERSION_0;
	}

	/* FIXME: Check for certain vocab files to determine
	**        SCI version  */

	/* ADDME: Try again with sci1_read_resource_map() */

	if (!mgr->resources) {
		sciprintf("Resmgr: Could not retreive a resource list!\n");
		sci_free(mgr);
		chdir(caller_cwd);
		free(caller_cwd);
		return NULL;
	}

	mgr->lru_first = NULL;
	mgr->lru_last = NULL;

	qsort(mgr->resources, mgr->resources_nr, sizeof(resource_t),
	      resourcecmp); /* Sort resources */

	mgr->allow_patches = allow_patches;

	if (version == SCI_VERSION_AUTODETECT)
		switch (resmap_version) {
		case SCI_VERSION_0:
			if (scir_test_resource(mgr, sci_vocab,
					       VOCAB_RESOURCE_SCI0_MAIN_VOCAB)) {
				sciprintf("Resmgr: Detected SCI0\n");
				version = SCI_VERSION_0;
			} else {
				sciprintf("Resmgr: Detected SCI01\n");
				version = SCI_VERSION_01;
			} break;

		default:
			sciprintf("Resmgr: Warning: While autodetecting: Couldn't"
				  " determine SCI version!\n");
		}

	mgr->sci_version = version;

	chdir(caller_cwd);
	free(caller_cwd);

	return mgr;
}

static void
_scir_free_resource_sources(resource_source_t *rss)
{
	if (rss) {
		_scir_free_resource_sources(rss->next);
		free(rss);
	}
}

void
_scir_free_resources(resource_t *resources, int resources_nr)
{
	int i;

	for (i = 0; i < resources_nr; i++) {
		resource_t *res = resources + i;

		if (res->status != SCI_STATUS_NOMALLOC)
			sci_free(res->data);
	}

	sci_free(resources);
}

void
scir_free_resource_manager(resource_mgr_t *mgr)
{
	_scir_free_resources(mgr->resources, mgr->resources_nr);

	mgr->resources = NULL;

	sci_free(mgr);
}

static void
_scir_load_resource(resource_mgr_t *mgr, resource_t *res)
{
	char *cwd = sci_getcwd();
	char filename[14];
	int fh;

	/* Enter resource directory */
	chdir(mgr->resource_path);

	/* First try lower-case name */
	sprintf(filename, "resource.%03i", res->file);
	fh = open(filename, O_RDONLY|O_BINARY);

	if (fh <= 0) {
		sprintf(filename, "RESOURCE.%03i", res->file);
		fh = sci_open(filename, O_RDONLY|O_BINARY);
	}    /* Try case-insensitively name */

	if (fh <= 0) {
		sciprintf("Failed to open %s/resource.%03d!\n",
			  mgr->resource_path, res->file);
		res->data = NULL;
		res->status = SCI_STATUS_NOMALLOC;
		res->size = 0;
		return;
	}

	lseek(fh, res->file_offset, SEEK_SET);

	if (!decompressors[mgr->sci_version])
		sciprintf("Resource manager's SCI version (%d) is invalid!\n",
			  mgr->sci_version);
	else {
		int error =
			decompressors[mgr->sci_version](res, fh);

		if (error) {
			sciprintf("Error %d occured while reading %s.%03d"
				  " from resource file: %s\n",
				  error, sci_resource_types[res->type], res->number,
				  sci_error_types[error]);
			res->data = NULL;
			res->status = SCI_STATUS_NOMALLOC;
			res->size = 0;
			return;
		}
	}

	close(fh);
	chdir(cwd);
	free(cwd);
}

void
_scir_unalloc(resource_t *res)
{
	sci_free(res->data);
	res->status = SCI_STATUS_NOMALLOC;
}


void
_scir_remove_from_lru(resource_mgr_t *mgr, resource_t *res)
{
	if (res->status != SCI_STATUS_ENQUEUED) {
		sciprintf("Resmgr: Oops: trying to remove resource that isn't"
			  " enqueued\n");
		return;
	}

	if (res->next)
		res->next->prev = res->prev;
	if (res->prev)
		res->prev->next = res->next;
	if (mgr->lru_first == res)
		mgr->lru_first = res->next;
	if (mgr->lru_last == res)
		mgr->lru_last = res->prev;

	mgr->memory_lru -= res->size;

	res->status = SCI_STATUS_ALLOCATED;
}

void
_scir_add_to_lru(resource_mgr_t *mgr, resource_t *res)
{
	if (res->status != SCI_STATUS_ALLOCATED) {
		sciprintf("Resmgr: Oops: trying to enqueue resource with state"
			  " %d\n", res->status);
		return;
	}

	res->prev = NULL;
	res->next = mgr->lru_first;
	mgr->lru_first = res;
	if (!mgr->lru_last)
		mgr->lru_last = res;
	if (res->next)
		res->next->prev = res;

	mgr->memory_lru += res->size;
#if (SCI_VERBOSE_RESMGR > 1)
	fprintf(stderr, "Adding %s.%03d (%d bytes) to lru control: %d bytes total\n",
		sci_resource_types[res->type], res->number, res->size,
		mgr->memory_lru);
	
#endif

	res->status = SCI_STATUS_ENQUEUED;
}

static void
_scir_print_lru_list(resource_mgr_t *mgr)
{
	int mem = 0;
	int entries = 0;
	resource_t *res = mgr->lru_first;

	while (res) {
		fprintf(stderr,"\t%s.%03d: %d bytes\n",
			sci_resource_types[res->type], res->number,
			res->size);
		mem += res->size;
		++entries;
		res = res->next;
	}

	fprintf(stderr,"Total: %d entries, %d bytes (mgr says %d)\n",
		entries, mem, mgr->memory_lru);
}

static void
_scir_free_old_resources(resource_mgr_t *mgr, int last_invulnerable)
{
	while (mgr->max_memory < mgr->memory_lru
	       && (!last_invulnerable || mgr->lru_first != mgr->lru_last)) {
		resource_t *goner = mgr->lru_last;
		if (!goner) {
			fprintf(stderr,"Internal error: mgr->lru_last is NULL!\n");
			fprintf(stderr,"LRU-mem= %d\n", mgr->memory_lru);
			fprintf(stderr,"lru_first = %p\n", mgr->lru_first);
			_scir_print_lru_list(mgr);
		}

		_scir_remove_from_lru(mgr, goner);
		_scir_unalloc(goner);
#ifdef SCI_VERBOSE_RESMGR
		sciprintf("Resmgr-debug: LRU: Freeing %s.%03d (%d bytes)\n",
			  sci_resource_types[goner->type], goner->number,
			  goner->size);
#endif
	}
}

resource_t *
scir_test_resource(resource_mgr_t *mgr, int type, int number)
{
	resource_t binseeker;
	binseeker.type = type;
	binseeker.number = number;
	return (resource_t *)
		bsearch(&binseeker, mgr->resources, mgr->resources_nr,
			sizeof(resource_t), resourcecmp);
}

resource_t *
scir_find_resource(resource_mgr_t *mgr, int type, int number, int lock)
{
	resource_t *retval = scir_test_resource(mgr, type, number);

	if (!retval)
		return NULL;

	if (!retval->status)
		_scir_load_resource(mgr, retval);
	else if (retval->status == SCI_STATUS_ENQUEUED)
			_scir_remove_from_lru(mgr, retval);
	/* Unless an error occured, the resource is now either
	** locked or allocated, but never queued or freed.  */

	if (lock) {
		if (retval->status == SCI_STATUS_ALLOCATED) {
			retval->status = SCI_STATUS_LOCKED;
			retval->lockers = 0;
			mgr->memory_locked += retval->size;
		}

		++retval->lockers;

	} else if (retval->status != SCI_STATUS_LOCKED) { /* Don't lock it */
		if (retval->status == SCI_STATUS_ALLOCATED)
			_scir_add_to_lru(mgr, retval);
	}

	_scir_free_old_resources(mgr, retval->status == SCI_STATUS_ALLOCATED);

	if (retval->data)
		return retval;
	else {
		sciprintf("Resmgr: Failed to read %s.%03d\n",
			  sci_resource_types[retval->type], retval->number);
		return NULL;
	}
}

void
scir_unlock_resource(resource_mgr_t *mgr, resource_t *res)
{
	if (!res) {
		sciprintf("Resmgr: Warning: Attempt to unlock non-existant"
			  " resource %s.%03d\n",
			  sci_resource_types[res->type], res->number);
		return;
	}

	if (res->status != SCI_STATUS_LOCKED) {
		sciprintf("Resmgr: Warning: Attempt to unlock unlocked"
			  " resource %s.%03d\n",
			  sci_resource_types[res->type], res->number);
		return;
	}

	if (!--res->lockers) { /* No more lockers? */
		res->status = SCI_STATUS_ALLOCATED;
		mgr->memory_locked -= res->size;
		_scir_add_to_lru(mgr, res);
	}

	_scir_free_old_resources(mgr, 0);
}

