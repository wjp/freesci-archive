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

#include <sciresource.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>


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

resource_t *
sci0_read_resource_map(int *resource_nr_p)
{
	struct stat fd_stat;
	int fd = sci_open("resource.map", O_RDONLY);
	resource_t *resources;
	int resources_nr;
	int resource_index = 0;
	int next_entry;
	byte buf[SCI0_RESMAP_ENTRIES_SIZE];

	if (!fd)
		return NULL;

	if (fstat(fd, &fd_stat)) {
		perror("Error occured while trying to stat resource.map");
		return NULL;
	}

	resources_nr = fd_stat.st_size / SCI0_RESMAP_ENTRIES_SIZE;

	resources = calloc(resources_nr, sizeof(resource_t));
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
			sci0_read_entry(buf, resources + resource_index++);

			if (resource_index >= resources_nr) {
				sciprintf("Warning: After %d entries, resource.map"
					  " is not terminated!\n", resource_index);
				next_entry = 0;
			}
		}

	} while (next_entry);

	if (!resource_index) {
		sciprintf("resource.map was empty!\n");
		return NULL;
	}

	if (resource_index < resources_nr)
		resources = realloc(resources, sizeof(resource_t) * resource_index);

	*resource_nr_p = resource_index;

	return resources;
}
