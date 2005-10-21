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
#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif

#define RESOURCE_MAP_FILENAME "resource.map"

#define SCI0_RESMAP_ENTRIES_SIZE 6
#define SCI1_RESMAP_ENTRIES_SIZE 6
#define SCI11_RESMAP_ENTRIES_SIZE 5


static int
sci_res_read_entry(byte *buf, resource_t *res, int sci_version)
{
	res->id = buf[0] | (buf[1] << 8);
	res->type = SCI0_RESID_GET_TYPE(buf);
	res->number = SCI0_RESID_GET_NUMBER(buf);
	res->status = SCI_STATUS_NOMALLOC;

	if (sci_version == SCI_VERSION_01_VGA_ODD) {
		res->file = SCI01V_RESFILE_GET_FILE(buf + 2);
		res->file_offset = SCI01V_RESFILE_GET_OFFSET(buf + 2);

#if 0
		if (res->type < 0 || res->type > sci1_last_resource)
			return 1;
#endif
	} else {
		res->file = SCI0_RESFILE_GET_FILE(buf + 2);
		res->file_offset = SCI0_RESFILE_GET_OFFSET(buf + 2);

#if 0
		if (res->type < 0 || res->type > sci0_last_resource)
			return 1;
#endif
	}

#if 0
	fprintf(stderr, "Read [%04x] %6d.%s\tresource.%03d, %08x\n",
		res->id, res->number,
		sci_resource_type_suffixes[res->type],
		res->file, res->file_offset);
#endif

	return 0;
}

inline int sci1_res_type(int ofs, int *types, int lastrt)
{
	int i, last = -1;

	for (i=0;i<sci1_last_resource;i++)
		if (types[i])
		{
			if (types[i]>ofs)
				return last;
			last=i;
		}

	return lastrt;
}

int sci1_parse_header(int fd, int *types, int *lastrt)
{
	unsigned char rtype;
	unsigned char offset[2];
	int read_ok;
	int size = 0;

	do
	{
		read_ok = read(fd, &rtype, 1);
		if (!read_ok) break;
		read_ok = read(fd, &offset, 2);
		if (read_ok<2) 
			read_ok=0;
		if (rtype!=0xff)
		{
			types[rtype&0x7f]=(offset[1]<<8)|(offset[0]);
			*lastrt = rtype&0x7f;
		}
		size+=3;
	} while (read_ok && (rtype != 0xFF));

	if (!read_ok) return 0;
	
	return size;
}


int
sci0_read_resource_map(char *path, resource_t **resource_p, int *resource_nr_p, int *sci_version)
{
	int fsize;
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

	read(fd, &buf, 4);

	if (buf[0] == 0x80 ||
	    buf[1] % 3 == 0 ||
	    buf[3] == 0x81)
	{
		close(fd);
		return SCI_ERROR_INVALID_RESMAP_ENTRY;
	}

	lseek(fd, 0, SEEK_SET);

	if ((fsize = sci_fd_size(fd)) < 0) {
		perror("Error occured while trying to get filesize of resource.map");
		return SCI_ERROR_RESMAP_NOT_FOUND;
	}

	resources_nr = fsize / SCI0_RESMAP_ENTRIES_SIZE;

	resources = sci_calloc(resources_nr, sizeof(resource_t));
	/* Sets valid default values for most entries */

	do {
		int read_ok = read(fd, &buf, SCI0_RESMAP_ENTRIES_SIZE);
		next_entry = 1;

		if (read_ok < 0 ) {
			perror("While reading from resource.map");
			next_entry = 0;
		} else if (read_ok != SCI0_RESMAP_ENTRIES_SIZE) {
			next_entry = 0;
		} else if (buf[5] == 0xff) /* Most significant offset byte */
			next_entry = 0;

		if (next_entry) {
			int fresh = 1;
			int addto = resource_index;
			int i;

			if (sci_res_read_entry(buf, resources + resource_index, sci_version)) {
				sci_free(resources);
				close(fd);
				return SCI_ERROR_RESMAP_NOT_FOUND;
			}

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

#define TEST fprintf(stderr, "OK in line %d\n", __LINE__);

static int sci10_or_11(int *types)
{
	int this_restype = 0;
	int next_restype = 1;

	while (next_restype <= sci_heap)
	{
		int could_be_10 = 0;
		int could_be_11 = 0;

		while (types[this_restype] == 0) 
		{
			this_restype++;
			next_restype++;
		}

		while (types[next_restype] == 0) 
			next_restype++;

		could_be_10 = ((types[next_restype] - types[this_restype])
				 % SCI1_RESMAP_ENTRIES_SIZE) == 0;
		could_be_11 = ((types[next_restype] - types[this_restype])
				 % SCI11_RESMAP_ENTRIES_SIZE) == 0;

		if (could_be_10 && !could_be_11) return SCI_VERSION_1;
		if (could_be_11 && !could_be_10) return SCI_VERSION_1_1;

		this_restype++;
		next_restype++;
	}

	return SCI_VERSION_AUTODETECT;
}

int
sci1_read_resource_map(char *path, resource_t **resource_p, int *resource_nr_p, int *sci_version)
{
	int fsize;
	int fd;
	resource_t *resources;
	int resources_nr;
	int resource_index = 0;
	int max_resfile_nr = 0;
	int ofs, header_size;
	int *types = sci_malloc(sizeof(int) * (sci1_last_resource+1));
	int i;
	byte buf[SCI1_RESMAP_ENTRIES_SIZE];
	int lastrt;
	int entrysize;

	fd = sci_open(RESOURCE_MAP_FILENAME, O_RDONLY | O_BINARY);

	if (!IS_VALID_FD(fd))
		return SCI_ERROR_RESMAP_NOT_FOUND;

	memset(types, 0, sizeof(int) * sci1_last_resource);

	if (!(ofs = header_size = sci1_parse_header(fd, types, &lastrt)))
	{
		close(fd);
		return SCI_ERROR_INVALID_RESMAP_ENTRY;
	}

	if (*sci_version == SCI_VERSION_AUTODETECT)
		*sci_version = sci10_or_11(types);

	if (*sci_version == SCI_VERSION_AUTODETECT) /* That didn't help */
	{
		sciprintf("Unable to detect resource map version\n");
		close(fd);
		return SCI_ERROR_NO_RESOURCE_FILES_FOUND;
	}

	entrysize = *sci_version == SCI_VERSION_1_1 ? SCI11_RESMAP_ENTRIES_SIZE
	                                            : SCI1_RESMAP_ENTRIES_SIZE;
						       
	if ((fsize = sci_fd_size(fd)) < 0) {
		perror("Error occured while trying to get filesize of resource.map");
		close(fd);
		return SCI_ERROR_RESMAP_NOT_FOUND;
	}

	resources_nr = (fsize - header_size) / entrysize;

	resources = sci_calloc(resources_nr, sizeof(resource_t));

	for (i=0; i<resources_nr; i++)
	{	
		int read_ok = read(fd, &buf, entrysize);	
		int j;
		resource_t *res;
		int addto = resource_index;
		int fresh = 1;

		if (read_ok < entrysize)
		{
			perror("While reading from resource.map");
			break;
		}

		res = &(resources[resource_index]);
		res->type = sci1_res_type(ofs, types, lastrt);
		res->number= SCI1_RESFILE_GET_NUMBER(buf);
		res->status = SCI_STATUS_NOMALLOC;

		if (*sci_version < SCI_VERSION_1_1)
		{
		    res->file = SCI1_RESFILE_GET_FILE(buf);
		    res->file_offset = SCI1_RESFILE_GET_OFFSET(buf);
		} else
		{
		    res->file = 0;
		    res->file_offset = SCI11_RESFILE_GET_OFFSET(buf);
		};
		
		res->id = res->number | (res->type << 16);

#if 0
		fprintf(stderr, "Read [%04x] %6d.%s\tresource.%03d, %08x\n",
			res->id, res->number,
			sci_resource_type_suffixes[res->type],
			res->file, res->file_offset);
#endif

		if (resources[resource_index].file > max_resfile_nr)
			max_resfile_nr =
				resources[resource_index].file;
		
		for (j = 0; i < resource_index; i++)
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

		ofs += entrysize;
	}

	close(fd);
	free(types);

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
