/***************************************************************************
 resource_map.c Copyright (C) 2001 Christoph Reichenbach


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

    Christoph Reichenbach (CR) <jameson@linuxgames.com>

***************************************************************************/

#include <sci_memory.h>
#include <sciresource.h>
#include <resource.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif

#define RESOURCE_MAP_FILENAME "resource.map"

#define SCI0_RESMAP_ENTRIES_SIZE 6


int
sci0_read_entry(byte *buf, resource_t *res)
{
	res->id = buf[0] | (buf[1] << 8);
	res->type = SCI0_RESID_GET_TYPE(buf);
	res->number = SCI0_RESID_GET_NUMBER(buf);
	res->status = SCI_STATUS_NOMALLOC;
	res->file = SCI0_RESFILE_GET_FILE(buf + 2);
	res->file_offset = SCI0_RESFILE_GET_OFFSET(buf + 2);

	return 0;
}

int
sci0_read_resource_map(char *path, resource_t **resource_p, int *resource_nr_p)
{
	struct stat fd_stat;
	int fd;
	resource_t *resources;
	int resources_nr;
	int resource_index = 0;
	int resources_total_read = 0;
	int next_entry;
	int max_resfile_nr = 0;
	byte buf[SCI0_RESMAP_ENTRIES_SIZE];
	fd = sci_open(RESOURCE_MAP_FILENAME, O_RDONLY | O_BINARY);

	if (!IS_VALID_FD(fd))
		return SCI_ERROR_RESMAP_NOT_FOUND;

	if (fstat(fd, &fd_stat)) {
		perror("Error occured while trying to stat resource.map");
		return SCI_ERROR_RESMAP_NOT_FOUND;
	}

	resources_nr = fd_stat.st_size / SCI0_RESMAP_ENTRIES_SIZE;

	resources = sci_calloc(resources_nr, sizeof(resource_t));
	/* Sets valid default values for most entries */

	do {
		int read_ok = read(fd, &buf, SCI0_RESMAP_ENTRIES_SIZE);
		next_entry = 1;

		if (read_ok < 0 ) {
			perror("While reading from resource.map");
			next_entry = 0;
		} else if (read_ok != 6) {
			next_entry = 0;
		} else if (buf[5] == 0xff) /* Most significant offset byte */
			next_entry = 0;

		if (next_entry) {
			int fresh = 1;
			int addto = resource_index;
			int i;

			sci0_read_entry(buf, resources + resource_index);

			if (resources[resource_index].file > max_resfile_nr)
				max_resfile_nr =
					resources[resource_index].file;

			for (i = 0; i < resource_index; i++)
				if (resources[resource_index].id ==
				    resources[i].id) {
					addto = i;
					fresh = 0;
				}

			_scir_add_altsource(resources + addto,
					    resources[resource_index].file,
					    resources[resource_index].file_offset);

			if (fresh)
				++resource_index;

			if (++resources_total_read >= resources_nr) {
				sciprintf("Warning: After %d entries, resource.map"
					  " is not terminated!\n", resource_index);
				next_entry = 0;
			}

		}

	} while (next_entry);

	close(fd);

	if (!resource_index) {
		sciprintf("resource.map was empty!\n");
		_scir_free_resources(resources, resources_nr);
		return SCI_ERROR_RESMAP_NOT_FOUND;
	}

	if (max_resfile_nr > 999) {
		_scir_free_resources(resources, resources_nr);
		return SCI_ERROR_INVALID_RESMAP_ENTRY;
	} else {
#if 0
/* Check disabled, Mac SQ3 thinks it has resource.004 but doesn't need it -- CR */
		/* Check whether the highest resfile used exists */
		char filename_buf[14];
		sprintf(filename_buf, "resource.%03d", max_resfile_nr);
		fd = sci_open(filename_buf, O_RDONLY);

		if (!IS_VALID_FD(fd)) {
			_scir_free_resources(resources, resources_nr);
			sciprintf("'%s' requested by resource.map, but not found\n", filename_buf);
			return SCI_ERROR_INVALID_RESMAP_ENTRY;
		} else
			close(fd);
#endif
	}

	if (resource_index < resources_nr)
		resources = sci_realloc(resources, sizeof(resource_t) * resource_index);

	*resource_p = resources;
	*resource_nr_p = resource_index;

	return 0;
}



#ifdef TEST_RESOURCE_MAP
int
main(int argc, char **argv)
{
	int resources_nr;
	resource_t *resources;
	int notok = sci0_read_resource_map(".", &resources, &resources_nr);

	if (notok) {
		fprintf(stderr,"Failed: Error code %d\n",notok);
		return 1;
	}

	if (resources) {
		int i;

		printf("Found %d resources:\n", resources_nr);

		for (i = 0; i < resources_nr; i++) {
			resource_t *res = resources + i;

			printf("#%04d:\tRESOURCE.%03d:%8d\t%s.%03d\n",
			       i, res->file, res->file_offset,
			       sci_resource_types[res->type],
			       res->number);
		}
	} else
		fprintf(stderr, "Found no resources.\n");

	return 0;
}
#endif
