/* KallistiOS

   fs_iso9660.c
   (c)2000-2001 Dan Potter

   This program is free software; you can redistribute it and/or modify
   it under the terms of the KOS License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   KOS License (README.KOS) for more details.

   You should have received a copy of the KOS License along with this
   program; if not, please visit Cryptic Allusion DCDev at:

     http://dcdev.allusion.net/

*/

/* Modified by Walter van Niftrik <w.f.b.w.v.niftrik@stud.tue.nl> */

/*

This module implements an ISO9660 file system for reading from a CDR or CD
in the DC's GD-Rom drive.

Rock Ridge support has now been implemented, thanks to Andrew Kieschnick
who donated the code. Joliet support is not implemented, but is planned.

This FS is considerably simplified from what you'd find in a bigger kernel
like Linux or BSD, since we have the pleasure of working with only a single
device capable of ISO9660 at once =). So there are a number of things in here
that are global variables that might otherwise not be.

Some thanks are in order here to Marcus Comstedt for providing an ISO9660
implementation that was easy enough to understand without downloading the
full spec =). Thanks also in order to the creators of the BSD and Linux
ISO9660 systems, as these were used as references as well.

*/

#include <dc/fs_iso9660.h>
#include <dc/cdrom.h>

#include <kos/thread.h>
#include <kos/mutex.h>
#include <kos/fs.h>

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <malloc.h>

CVSID("$Id: fs_iso9660.c 1115 2003-02-11 22:57:43Z waltervn $");

static int init_percd();
static int percd_done;

/********************************************************************************/
/* Low-level ISO utils */

/* ISO Directory entry */
typedef struct {
	uint8	length;			/* 711 */
	uint8	ext_attr_length;	/* 711 */
	uint8	extent[8];		/* 733 */
	uint8	size[8];		/* 733 */
	uint8	date[7];		/* 7x711 */
	uint8	flags;
	uint8	file_unit_size;		/* 711 */
	uint8	interleave;		/* 711 */
	uint8	vol_sequence[4];	/* 723 */
	uint8	name_len;		/* 711 */
	char	name[1];
} iso_dirent_t;

/* Util function to reverse the byte order of a uint32 */
/* static uint32 ntohl_32(const void *data) {
	const uint8 *d = (const uint8*)data;
	return (d[0] << 24) | (d[1] << 16) | (d[2] << 8) | (d[3] << 0);
} */

/* This seems kinda silly, but it's important since it allows us
   to do unaligned accesses on a buffer */
static uint32 htohl_32(const void *data) {
	const uint8 *d = (const uint8*)data;
	return (d[0] << 0) | (d[1] << 8) | (d[2] << 16) | (d[3] << 24);
}

/* Read red-book section 7.1.1 number (8 bit) */
/* static uint8 iso_711(const uint8 *from) { return (*from & 0xff); } */

/* Read red-book section 7.3.3 number (32 bit LE / 32 bit BE) */
static uint32 iso_733(const uint8 *from) { return htohl_32(from); }


/********************************************************************************/
/* Low-level block cacheing routines. This implements a simple queue-based 
   LRU/MRU cacheing system. Whenever a block is requested, it will be placed
   on the MRU end of the queue. As more blocks are loaded than can fit in
   the cache, blocks are deleted from the LRU end. */

/* Holds the data for one cache block, and a pointer to the next one.
   As sectors are read from the disc, they are added to the front of
   this cache. As the cache fills up, sectors are removed from the end
   of it. */
typedef struct {
	int32	sector;			/* CD sector */
	uint8	data[2048];		/* Sector data */
} cache_block_t;

/* List of cache blocks (ordered least recently used to most recently) */
#define NUM_CACHE_BLOCKS 16
static cache_block_t *icache[NUM_CACHE_BLOCKS];		/* inode cache */
static cache_block_t *dcache[NUM_CACHE_BLOCKS];		/* data cache */

/* Cache modification mutex */
static mutex_t * cache_mutex;

/* Clears all cache blocks */
static void bclear_cache(cache_block_t **cache) {
	int i;
	
	mutex_lock(cache_mutex);
	for (i=0; i<NUM_CACHE_BLOCKS; i++)
		cache[i]->sector = -1;
	mutex_unlock(cache_mutex);
}

/* Graduate a block from its current position to the MRU end of the cache */
static void bgrad_cache(cache_block_t **cache, int block) {
	int		i;
	cache_block_t	*tmp;
	
	/* Don't try it with the end block */
	if (block < 0 || block >= (NUM_CACHE_BLOCKS-1)) return;
	
	/* Make a copy and scoot everything down */
	tmp = cache[block];
	for (i=block; i<(NUM_CACHE_BLOCKS - 1); i++)
		cache[i] = cache[i+1];
	cache[NUM_CACHE_BLOCKS-1] = tmp;
}

/* Pulls the requested sector into a cache block and returns the cache
   block index. Note that the sector in question may already be in the
   cache, in which case it just returns the containing block. */
static void iso_close(file_t fd);
static int bread_cache(cache_block_t **cache, uint32 sector) {
	int i, j, rv;

	rv = -1;	
	mutex_lock(cache_mutex);

	/* Look for a pre-existing cache block */
	for (i=NUM_CACHE_BLOCKS-1; i>=0; i--) {
		if (cache[i]->sector == sector) {
			bgrad_cache(cache, i);
			rv = NUM_CACHE_BLOCKS - 1;
			goto bread_exit;
		}
	}
	
	/* If not, look for an open cache slot; if we find one, use it */
	for (i=0; i<NUM_CACHE_BLOCKS; i++) {
		if (cache[i]->sector == -1) break;
	}
	
	/* If we didn't find one, kick an LRU block out of cache */
	if (i >= NUM_CACHE_BLOCKS) { i = 0; }
	
	/* Load the requested block */
	j = cdrom_read_sectors(cache[i]->data, sector + 150, 1);
	if (j < 0) {
		if (j == ERR_DISC_CHG) {
			init_percd();
			for (i=0; i<MAX_ISO_FILES; i++)
				iso_close(i);
		}
		rv = -1;
		goto bread_exit;
	}
	cache[i]->sector = sector;
	
	/* Move it to the most-recently-used position */
	bgrad_cache(cache, i);
	rv = NUM_CACHE_BLOCKS - 1;

	/* Return the new cache block index */
bread_exit:
	mutex_unlock(cache_mutex);
	return rv;
}

/* read data block */
static int bdread(uint32 sector) {
	return bread_cache(dcache, sector);
}

/* read inode block */
static int biread(uint32 sector) {
	return bread_cache(icache, sector);
}

/* Clear both caches */
static void bclear() {
	bclear_cache(dcache);
	bclear_cache(icache);
}

/********************************************************************************/
/* Higher-level ISO9660 primitives */

/* Root FS session location (in sectors) */
static uint32 session_base = 0;

/* Root directory extent and size in bytes */
static uint32 root_extent = 0, root_size = 0;

/* Root dirent */
static iso_dirent_t root_dirent;


/* Per-disc initialization; this is done every time it's discovered that
   a new CD has been inserted. */
static int init_percd() {
	int		i;
	CDROM_TOC	toc;

	dbglog(DBG_NOTICE, "fs_iso9660: disc change detected\r\n");
	
	/* Start off with no cached blocks */
	bclear();
	
	/* Locate the root session */
	if ((i = cdrom_reinit()) != 0)
		return i;
	if ((i = cdrom_read_toc(&toc, 0)) != 0)
		return i;
	if (!(session_base = cdrom_locate_data_track(&toc)))
		return -1;

	/* Grab and check the volume descriptor */	
	i = biread(session_base + 16 - 150);
	if (i < 0) return i;
	if (memcmp((char*)icache[i]->data, "\01CD001", 6)) {
		dbglog(DBG_ERROR, "fs_iso9660: disc is not iso9660\r\n");
		return -1;
	}

	/* Locate the root directory */
	memcpy(&root_dirent, icache[i]->data+156, sizeof(iso_dirent_t));
	root_extent = iso_733(root_dirent.extent);
	root_size = iso_733(root_dirent.size);
	
	return 0;
}

/* Compare an ISO9660 filename against a normal filename. This takes into
   account the version code on the end and is not case sensitive. Also
   takes into account the trailing period that some CD burning software
   adds. */
static int fncompare(const char *isofn, int isosize, const char *normalfn) {
	int i;

	/* Compare ISO name */
	for (i=0; i<isosize; i++) {
		/* Weed out version codes */
		if (isofn[i] == ';') break;

		/* Deal with crap '.' at end of filenames */
		if (isofn[i] == '.' &&
				(i == (isosize-1) || isofn[i+1] == ';'))
			break;

		/* Otherwise, compare the chars normally */
		if (tolower(isofn[i]) != tolower(normalfn[i]))
			return -1;
	}

	/* Catch ISO name shorter than normal name */
	if (normalfn[i] != '/' && normalfn[i] != '\0')
		return -1;
	else
		return 0;
}

/* Locate an ISO9660 object in the given directory; this can be a directory or
   a file, it works fine for either one. Pass in:
   
   fn:		object filename (relative to the passed directory)
   dir:		0 if looking for a file, 1 if looking for a dir
   dir_extent:	directory extent to start with
   dir_size:	directory size (in bytes)
   
   It will return a pointer to a transient dirent buffer (i.e., don't
   expect this buffer to stay around much longer than the call itself).
 */
static iso_dirent_t *find_object(const char *fn, int dir,
		uint32 dir_extent, uint32 dir_size) {
	int		i, c;
	iso_dirent_t	*de;

	int		len;
	uint8		*pnt;
	char		rrname[MAX_FN_LEN];
	int		rrnamelen;
	
	while (dir_size > 0) {
		c = biread(dir_extent);
		if (c < 0) return NULL;
		
		for (i=0; i<2048 && i<dir_size; ) {
			/* Assume no Rock Ridge name */
			rrnamelen = 0;
		
			/* Locate the current dirent */
			de = (iso_dirent_t *)(icache[c]->data + i);
			if (!de->length) break;
			
			/* Check for Rock Ridge NM extension */
			len = de->length - sizeof(iso_dirent_t)
				+ sizeof(de->name) - de->name_len;
			pnt = (uint8*)de + sizeof(iso_dirent_t)
				- sizeof(de->name) + de->name_len;
			if ((de->name_len & 1) == 0) {
				pnt++; len--;
			}
			while ((len >= 4) && ((pnt[3] == 1) || (pnt[3] == 2))) {
				if (strncmp(pnt, "NM", 2) == 0) {
					rrnamelen = pnt[2] - 5;
					strncpy(rrname, pnt+5, rrnamelen);
					rrname[rrnamelen] = 0;
				}
				len -= pnt[2];
				pnt += pnt[2];
			}
			
			
			/* Check the filename against the requested one */
			if (rrnamelen > 0) {
				char *p = strchr(fn, '/');
				int fnlen;
				
				if (p)
					fnlen = p - fn;
				else
					fnlen = strlen(fn);
				
				if (!strnicmp(rrname, fn, fnlen)) {
					if (!((dir << 1) ^ de->flags))
						return de;
				}
			} else {
				if (!fncompare(de->name, de->name_len, fn)) {
					if (!((dir << 1) ^ de->flags))
						return de;
				}
			}
			
			i += de->length;
		}
		
		dir_extent++;
		dir_size -= 2048;
	}
	
	return NULL;
}

/* Locate an ISO9660 object anywhere on the disc, starting at the root,
   and expecting a fully qualified path name. This is analogous to find_object
   but it searches with the path in mind.

   fn:		object filename (relative to the passed directory)
   dir:		0 if looking for a file, 1 if looking for a dir
   dir_extent:	directory extent to start with
   dir_size:	directory size (in bytes)
   
   It will return a pointer to a transient dirent buffer (i.e., don't
   expect this buffer to stay around much longer than the call itself).
 */
static iso_dirent_t *find_object_path(const char *fn, int dir, iso_dirent_t *start) {
	char		*cur;

	/* If the object is in a sub-tree, traverse the trees looking
	   for the right directory */
	while ((cur = strchr(fn, '/'))) {
		if (cur != fn) {
			/* Note: trailing path parts don't matter since find_object
			   only compares based on the FN length on the disc. */
			start = find_object(fn, 1, iso_733(start->extent), iso_733(start->size));
			if (start == NULL) return NULL;
		}
		fn = cur + 1;
	}

	/* Locate the file in the resulting directory */
	if (*fn) {
		start = find_object(fn, dir, iso_733(start->extent), iso_733(start->size));
		return start;
	}
	else {
		if (!dir)
			return NULL;
		else
			return start;
	}
}

/********************************************************************************/
/* File primitives */

/* File handles.. I could probably do this with a linked list, but I'm just
   too lazy right now. =) */
static struct {
	uint32		first_extent;	/* First sector */
	int		dir;		/* >0 if a directory */
	uint32		ptr;		/* Current read position in bytes */
	uint32		size;		/* Length of file in bytes */
	dirent_t	dirent;		/* A static dirent to pass back to clients */
} fh[MAX_ISO_FILES];

/* Mutex for file handles */
static mutex_t * fh_mutex;

/* Open a file or directory */
static file_t iso_open(vfs_handler_t * vfs, const char *fn, int mode) {
	file_t		fd;
	iso_dirent_t	*de;

	/* Make sure they don't want to open things as writeable */
	if ((mode & O_MODE_MASK) != O_RDONLY)
		return 0;
	
	/* Do this only when we need to (this is still imperfect) */
	if (!percd_done && init_percd() < 0)
		return 0;
	percd_done = 1;
	
	/* Find the file we want */
	de = find_object_path(fn, (mode & O_DIR)?1:0, &root_dirent);
	if (!de) return 0;
	
	/* Find a free file handle */
	mutex_lock(fh_mutex);
	for (fd=0; fd<MAX_ISO_FILES; fd++)
		if (fh[fd].first_extent == 0) {
			fh[fd].first_extent = -1;
			break;
		}
	mutex_unlock(fh_mutex);
	if (fd >= MAX_ISO_FILES)
		return 0;

	/* Fill in the file handle and return the fd */
	fh[fd].first_extent = iso_733(de->extent);
	fh[fd].dir = (mode & O_DIR)?1:0;
	fh[fd].ptr = 0;
	fh[fd].size = iso_733(de->size);
	
	return fd;
}

/* Close a file or directory */
static void iso_close(file_t fd) {
	/* Check that the fd is valid */
	if (fd < MAX_ISO_FILES) {
		/* No need to lock the mutex: this is an atomic op */
		fh[fd].first_extent = 0;
	}
}

/* Read from a file */
static ssize_t iso_read(file_t fd, void *buf, size_t bytes) {
	int rv, toread, thissect, c;
	uint8 * outbuf;

	/* Check that the fd is valid */
	if (fd >= MAX_ISO_FILES || fh[fd].first_extent == 0)
		return -1;
	
	rv = 0;
	outbuf = (uint8 *)buf;

	/* Read zero or more sectors into the buffer from the current pos */
	while (bytes > 0) {
		/* Figure out how much we still need to read */
		toread = (bytes > (fh[fd].size - fh[fd].ptr)) ?
			fh[fd].size - fh[fd].ptr : bytes;
		if (toread == 0) break;

		/* How much more can we read in the current sector? */
		thissect = 2048 - (fh[fd].ptr % 2048);

		/* If we're on a sector boundary and we have more than one
		   full sector to read, then short-circuit the cache here
		   and use the multi-sector reads from the CD unit (this
		   should theoretically be a lot faster) */
		/* XXX This code isn't actually faster, but I'm leaving it
		   here commented out in case we could find a better use
		   for it later than speed (i.e., preventing thread context
		   switches). */
		/* if (thissect == 2048 && toread >= 2048) {
			// Round it off to an even sector count
			thissect = toread / 2048;
			toread = thissect * 2048;

			printf("cdrom: short-circuit read for %d sectors\n",
				thissect);

			// Do the read
			if (cdrom_read_sectors(outbuf,
				fh[fd].first_extent + fh[fd].ptr/2048 + 150,
				thissect) < 0)
			{
				// Something went wrong...
				return -1;
			}
		} else { */
			toread = (toread > thissect) ? thissect : toread;
		
			/* Do the read */
			c = bdread(fh[fd].first_extent + fh[fd].ptr/2048);
			if (c < 0) return -1;
			memcpy(outbuf, dcache[c]->data + (fh[fd].ptr%2048), toread);
		/* } */
		
		/* Adjust pointers */
		outbuf += toread;
		fh[fd].ptr += toread;
		bytes -= toread;
		rv += toread;
	}
	
	return rv;
}

/* Seek elsewhere in a file */
static off_t iso_seek(file_t fd, off_t offset, int whence) {
	/* Check that the fd is valid */
	if (fd>=MAX_ISO_FILES || fh[fd].first_extent==0)
		return -1;

	/* Update current position according to arguments */
	switch (whence) {
		case SEEK_SET:
			fh[fd].ptr = offset;
			break;
		case SEEK_CUR:
			fh[fd].ptr += offset;
			break;
		case SEEK_END:
			fh[fd].ptr = fh[fd].size + offset;
			break;
		default:
			return -1;
	}
	
	/* Check bounds */
	if (fh[fd].ptr < 0) fh[fd].ptr = 0;
	if (fh[fd].ptr > fh[fd].size) fh[fd].ptr = fh[fd].size;
	
	return fh[fd].ptr;
}

/* Tell where in the file we are */
static off_t iso_tell(file_t fd) {
	if (fd>=MAX_ISO_FILES || fh[fd].first_extent==0)
		return -1;

	return fh[fd].ptr;
}

/* Tell how big the file is */
static size_t iso_total(file_t fd) {
	if (fd>=MAX_ISO_FILES || fh[fd].first_extent==0)
		return -1;

	return fh[fd].size;
}

/* Helper function for readdir: post-processes an ISO filename to make
   it a bit prettier. */
static void fn_postprocess(char *fn) {
	while (*fn && *fn != ';' && !(*fn == '.' && (!*(fn+1) || *(fn+1) == ';'))) {
		*fn = tolower(*fn);
		fn++;
	}
	*fn = 0;
}

/* Read a directory entry */
static dirent_t *iso_readdir(file_t fd) {
	int		c;
	iso_dirent_t	*de;
	
	int		len;
	uint8		*pnt;

	if (fd>=MAX_ISO_FILES || fh[fd].first_extent==0 || !fh[fd].dir)
		return NULL;

	/* Scan forwards until we find the next valid entry, an
	   end-of-entry mark, or run out of dir size. */
	c = -1; de = NULL;
	while(fh[fd].ptr < fh[fd].size) {
		/* Get the current dirent block */
		c = biread(fh[fd].first_extent + fh[fd].ptr/2048);
		if (c < 0) return NULL;
	
		de = (iso_dirent_t *)(icache[c]->data + (fh[fd].ptr%2048));
		if (de->length) break;

		/* Skip to the next sector */
		fh[fd].ptr += 2048 - (fh[fd].ptr%2048);
	}
	if (fh[fd].ptr >= fh[fd].size) return NULL;
	
	/* If we're at the first, skip the two blank entries */
	if (!de->name[0]) {
		fh[fd].ptr += de->length;
		de = (iso_dirent_t *)(icache[c]->data + (fh[fd].ptr%2048));
		fh[fd].ptr += de->length;
		de = (iso_dirent_t *)(icache[c]->data + (fh[fd].ptr%2048));
		if (!de->length) return NULL;
	}

	
	/* Fill out the VFS dirent */
	strncpy(fh[fd].dirent.name, de->name, de->name_len);
	fh[fd].dirent.name[de->name_len] = 0;
	fn_postprocess(fh[fd].dirent.name);

	/* Check for Rock Ridge NM extension */
	len = de->length - sizeof(iso_dirent_t) + sizeof(de->name) - de->name_len;
	pnt = (uint8*)de + sizeof(iso_dirent_t) - sizeof(de->name) + de->name_len;
	if ((de->name_len & 1) == 0) {
		pnt++; len--;
	}
	while ((len >= 4) && ((pnt[3] == 1) || (pnt[3] == 2))) {
		if (strncmp(pnt, "NM", 2) == 0) {
			strncpy(fh[fd].dirent.name, pnt+5, pnt[2] - 5);
			fh[fd].dirent.name[pnt[2] - 5] = 0;
		}
		len -= pnt[2];
		pnt += pnt[2];
	}

	if (de->flags & 2)
		fh[fd].dirent.size = -1;
	else
		fh[fd].dirent.size = iso_733(de->size);
	
	fh[fd].ptr += de->length;
	
	return &fh[fd].dirent;
}

/* There's only one ioctl at the moment (re-initialize caches) but you should
   always clear data and size. */
int iso_ioctl(file_t hnd, void *data, size_t size) {
	int i;
	
	for (i=1; i<MAX_ISO_FILES; i++)
		iso_close(i);
	percd_done = 0;

	return 0;
}

/* Put everything together */
static vfs_handler_t vh = {
	{ "/cd" },	/* path prefix */
	0, 0,		/* In-kernel, no cacheing */
	NULL,		/* privdata */
	VFS_LIST_INIT,	/* linked list pointer */
	
	iso_open,
	iso_close,
	iso_read,
	NULL,
	iso_seek,
	iso_tell,
	iso_total,
	iso_readdir,
	iso_ioctl,
	NULL,
	NULL,
	NULL
};

/* Initialize the file system */
int fs_iso9660_init() {
	int i;

	/* Reset fd's */
	memset(fh, 0, sizeof(fh));
	
	/* Mark the first as active so we can have an error FD of zero */
	fh[0].first_extent = -1;
	
	/* Init thread mutexes */
	cache_mutex = mutex_create();
	fh_mutex = mutex_create();

	/* Allocate cache block space */
	for (i=0; i<NUM_CACHE_BLOCKS; i++) {
		icache[i] = malloc(sizeof(cache_block_t));
		icache[i]->sector = -1;
		dcache[i] = malloc(sizeof(cache_block_t));
		dcache[i]->sector = -1;
	}

	percd_done = 0;

	/* Register with VFS */
	return fs_handler_add("/cd", &vh);
}

/* De-init the file system */
int fs_iso9660_shutdown() {
	int i;
	
	/* Dealloc cache block space */
	for (i=0; i<NUM_CACHE_BLOCKS; i++) {
		free(icache[i]);
		free(dcache[i]);
	}

	/* Free muteces */
	if (cache_mutex != NULL)
		mutex_destroy(cache_mutex);
	if (fh_mutex != NULL)
		mutex_destroy(fh_mutex);
	cache_mutex = fh_mutex = NULL;
	
	return fs_handler_remove(&vh);
}






