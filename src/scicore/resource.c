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


#include <sciresource.h>
#include <assert.h>

#include <sys/types.h>
#include <sys/stat.h>

#ifndef O_BINARY
#define O_BINARY 0
/* Not defined on most systems */
#endif

#undef SCI_REQUIRE_RESOURCE_FILES


const char* SCI_Version_Types[] = {
	"SCI version undetermined (Autodetect failed / not run)",
	"SCI version 0.xxx",
	"SCI version 0.xxx w/ 1.000 compression",
	"SCI version 1.000 (early)",
	"SCI version 1.000 (late)",
	"SCI version 1.001",
	"SCI WIN/32"
};

const int sci_max_resource_nr[] = {65536, 1000, 1000, 1000, 8192};

const char* SCI_Error_Types[] = {
	"No error",
	"I/O error",
	"Resource is empty (size 0)",
	"resource.000 or resource.001 not found",
	"Unknown compression method",
	"Decompression failed: Decompression buffer overflow",
	"Decompression failed: Sanity check failed",
	"Decompression failed: Resource too big",
	"SCI version is unsupported"};

const char* Resource_Types[] = {"view","pic","script","text","sound",
				"memory","vocab","font","cursor",
				"patch","bitmap","palette","cdaudio",
				"audio","sync","message","map","heap"};
/* These are the 18 resource types supported by SCI1 */

const char *resource_type_suffixes[] = {"v56","p56","scr","tex","snd",
					"   ","voc","fon","cur","pat",
					"bit","pal","cda","aud","syn",
					"msg","map","hep"};


int sci_version = 0;

int max_resource = 0;
resource_t *resource_map;

struct singly_linked_resources_struct {
	resource_t *resource;
	struct singly_linked_resources_struct *next;
};

int resourceLoader(int decompress(resource_t *result, int resh), int autodetect, int allow_patches);

void killlist(struct singly_linked_resources_struct *rs);

int loadResourcePatches(struct singly_linked_resources_struct *resourcelist);

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


int loadResources(int version, int allow_patches)
{
	int autodetect = (version == SCI_VERSION_AUTODETECT);
	int retval;

	max_resource = 0;

	if ((version < SCI_VERSION_AUTODETECT) || (version > SCI_VERSION_LAST))
		return SCI_ERROR_UNSUPPORTED_VERSION;

	sci_version = (autodetect)? SCI_VERSION_0 : version;

	do {
#ifdef _SCI_RESOURCE_DEBUG
		if (autodetect) fprintf(stderr, "Autodetect: Trying %s\n", SCI_Version_Types[sci_version]);
#endif
		if ((retval = resourceLoader(decompressors[sci_version], autodetect, allow_patches))) {
			freeResources();
			if (autodetect == ((retval == SCI_ERROR_UNKNOWN_COMPRESSION)
					   || (retval == SCI_ERROR_DECOMPRESSION_OVERFLOW)
					   || (retval == SCI_ERROR_DECOMPRESSION_INSANE))) {
				++sci_version;

			} else return retval;
		} else return 0;
	} while (autodetect && sci_version <= SCI_VERSION_LAST);

	return SCI_ERROR_UNSUPPORTED_VERSION;
}



void
_addResource(struct singly_linked_resources_struct *base, resource_t *resource, int priority)
/* Tries to add [resource] to the [base] list. If an element with the same id already
** exists, [resource] will be discarded and free()d if ([priority]==0), otherwise
** the other resource will be free()d and replaced by the new [resource].
*/
{
	struct singly_linked_resources_struct *seeker;

	if (!base->resource) {
		base->resource = resource;
		max_resource++;
	} else {
		seeker = base;

		while (seeker && seeker->next) {
			if (seeker->resource->number == resource->number
			    && seeker->resource->type == resource->type) {
				if (priority) { /* replace the old resource */
					free(seeker->resource->data);
					free(seeker->resource);
					seeker->resource = resource;
					return;
				} else seeker = 0;
			} else seeker = seeker->next;
		}

		if (seeker) {

			seeker->next = malloc(sizeof(struct singly_linked_resources_struct));
			seeker->next->resource = resource;
			seeker->next->next = 0;
			max_resource++;

		} else {

			free(resource->data);
			free(resource);

		}
	}
}


int
resourceLoader(int decompress(resource_t *result, int resh), int autodetect, int allow_patches)
{
	int resourceFile = 0;
	int resourceFileCounter = 0;
	resource_t *resource;
	char filename[13];
	int resourceCounter;
	struct singly_linked_resources_struct *seeker;
	struct singly_linked_resources_struct base;
	int found_resfiles = 0;

	base.next = 0;
	base.resource = 0;

	do {
		if (resourceFileCounter > 0 && resourceFile > 0) {
			int decomperr;
			resource = malloc(sizeof(resource_t));
			while (!(decomperr = (*decompress)(resource, resourceFile))) {

				_addResource(&base, resource, 0);
				found_resfiles = 1;

				resource = malloc(sizeof(resource_t));
			}
			free(resource);
			close(resourceFile);
			if (decomperr >= SCI_ERROR_CRITICAL) {
#ifdef _SCI_RESOURCE_DEBUG
				fprintf(stderr,"SCI Error: %s in '%s'!\n",
					SCI_Error_Types[decomperr], filename);
#endif	
				killlist(&base); /* Free resources */
				max_resource = 0;
				return decomperr;
			}
		}

		sprintf(filename, "resource.%03i", resourceFileCounter);
		resourceFile = open(filename, O_RDONLY|O_BINARY);

		if (resourceFile <= 0) {
			sprintf(filename, "RESOURCE.%03i", resourceFileCounter);
			resourceFile = open(filename, O_RDONLY|O_BINARY);
		}    /* Try alternative valid file name */

		resourceFileCounter++;
#ifdef _SCI_RESOURCE_DEBUG
		if (resourceFile > 0) fprintf(stderr, "Reading %s...\n", filename);
		else if (resourceFileCounter > 1) fprintf(stderr, "Completed.\n");
#endif
	} while ((resourceFile > 0) || (resourceFileCounter == 1));

#ifndef SCI_REQUIRE_RESOURCE_FILES
	if (!found_resfiles) {
		return SCI_ERROR_NO_RESOURCE_FILES_FOUND;
	}
#endif

#ifdef _SCI_RESOURCE_DEBUG
	fprintf(stderr,"%i unique resources have been read.\n", max_resource);
#endif

	if (allow_patches) {
		int pcount = loadResourcePatches(&base);
		if (pcount == 1)
			printf("One patch was applied.\n");
		else if (pcount)
			printf("%d patches were applied.\n", pcount);
		else {
			printf("No patches found.\n");
			if (!found_resfiles)
				return SCI_ERROR_NO_RESOURCE_FILES_FOUND;
		}
	} else if (!found_resfiles)
		return SCI_ERROR_NO_RESOURCE_FILES_FOUND;

	else printf("Ignoring any patches.\n");

	resource_map = malloc(max_resource * sizeof(resource_t));

	seeker = &base;

	resourceCounter = 0;

	if (base.resource)
		while (seeker) {
			memcpy(resource_map + resourceCounter, seeker->resource,
			       sizeof(resource_t));
			resourceCounter++;
			seeker = seeker->next;
		}

	if(resourceCounter != max_resource) {
		fprintf(stderr,"Internal error: resourceCounter=%d != max_resource=%d!\n",
			resourceCounter, max_resource);
		exit(1);
	}
	qsort(resource_map, max_resource, sizeof(resource_t),
	      resourcecmp);

	killlist(&base);

	return 0;
}


void killlist(struct singly_linked_resources_struct *rs)
{
	if (rs->next) {
		killlist(rs->next);
		free(rs->next);
		rs->next = NULL;
	}
	free(rs->resource);
}


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
		int resname_len;
		char *endptr;

		for (i = sci_view; i < sci_invalid_resource; i++)
			if (strncasecmp(Resource_Types[i], entry, strlen(Resource_Types[i])) == 0)
				restype = i;

		if (restype != sci_invalid_resource) {

			resname_len = strlen(Resource_Types[restype]);
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

						newrsc = malloc(sizeof(resource_t));
						newrsc->length = filestat.st_size - 2;
						newrsc->id = restype << 11 | resnumber;
						newrsc->number = resnumber;
						newrsc->status = SCI_STATUS_OK;
						newrsc->type = restype;

						newrsc->data = malloc(newrsc->length);
						read(file, newrsc->data, newrsc->length);
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

resource_t *findResource(unsigned short type, unsigned short number)
{
	resource_t binseeker, *retval;
	binseeker.type = type;
	binseeker.number = number;
	retval = (resource_t *)
		bsearch(&binseeker, resource_map, max_resource, sizeof(resource_t),
			resourcecmp);

	if (retval && retval->status == SCI_STATUS_NOMALLOC) {
		fprintf(stderr,"Warning: Dereferenced unallocated resource!\n");
		return NULL;
	} else
		return retval;
}

void freeResources()
{
	if (resource_map) {
		int i;

		for (i=0; i < max_resource; i++) {
			if (!resource_map[i].status) free(resource_map[i].data);
		}
		free(resource_map);
		max_resource = 0;
		return;
	}
}


void *
_XALLOC(size_t size, char *file, int line, char *funct)
{
	void *memblock;

	if (!(memblock = malloc(size))) {
		fprintf(stderr,"file %s %d (%s): XALLOC(%ld) failed.\n",
			file, line, funct, (long) size);
		exit(-1);
	}

	return memblock;
}


