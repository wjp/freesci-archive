/***************************************************************************
 sciresource.h Copyright (C) 1999,2000,01 Christoph Reichenbach


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

    Christoph Reichenbach (CR) [creichen@rbg.informatik.tu-darmstadt.de]

 History:

   990327 - created (CR)

***************************************************************************/

#ifndef SCIRESOURCE_H_
#define SCIRESOURCE_H_

/*#define _SCI_RESOURCE_DEBUG */
/*#define _SCI_DECOMPRESS_DEBUG*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <resource.h>

#define SCI_MAX_RESOURCE_SIZE 0x0400000
/* The maximum allowed size for a compressed or decompressed resource */

#ifdef _WIN32
#  define DIR_SEPARATOR_STR "\\"
#  define PATH_SEPARATOR_STR ";"
#else
#  define DIR_SEPARATOR_STR "/"
#  define PATH_SEPARATOR_STR ":"
#endif


/*** RESOURCE STATUS TYPES ***/
#define SCI_STATUS_NOMALLOC 0
#define SCI_STATUS_ALLOCATED 1
#define SCI_STATUS_LOCKED 2 /* Allocated and in use */

#define SCI_RES_FILE_NR_PATCH -1 /* Resource was read from a patch file rather than from a resource */


/*** INITIALIZATION RESULT TYPES ***/
#define SCI_ERROR_IO_ERROR 1
#define SCI_ERROR_EMPTY_OBJECT 2
#define SCI_ERROR_NO_RESOURCE_FILES_FOUND 3
/* neither resource.000 nor resource.001 were found */
#define SCI_ERROR_UNKNOWN_COMPRESSION 4
#define SCI_ERROR_DECOMPRESSION_OVERFLOW 5
/* decompression failed: Buffer overflow (wrong SCI version?)  */
#define SCI_ERROR_DECOMPRESSION_INSANE 6
/* sanity checks failed during decompression */
#define SCI_ERROR_RESOURCE_TOO_BIG 7
/* Resource size exceeds SCI_MAX_RESOURCE_SIZE */
#define SCI_ERROR_UNSUPPORTED_VERSION 8
#define SCI_ERROR_INVALID_SCRIPT_VERSION 9

#define SCI_ERROR_CRITICAL SCI_ERROR_NO_RESOURCE_FILES_FOUND
/* the first critical error number */

/*** SCI VERSION NUMBERS ***/
#define SCI_VERSION_AUTODETECT 0
#define SCI_VERSION_0 1
#define SCI_VERSION_01 2
#define SCI_VERSION_1_EARLY 3
#define SCI_VERSION_1_LATE 4
#define SCI_VERSION_1_1 5
#define SCI_VERSION_32 6
#define SCI_VERSION_LAST SCI_VERSION_1_LATE /* The last supported SCI version */

#define SCI_VERSION_1 SCI_VERSION_1_EARLY

extern DLLEXTERN const char* SCI_Error_Types[];
extern DLLEXTERN const char* SCI_Version_Types[];
extern DLLEXTERN const char* Resource_Types[];
extern DLLEXTERN const char* resource_type_suffixes[]; /* Suffixes for SCI1 patch files */
extern DLLEXTERN const int sci_max_resource_nr[]; /* Highest possible resource numbers */


enum ResourceTypes {
	sci_view=0, sci_pic, sci_script, sci_text,
	sci_sound, sci_memory, sci_vocab, sci_font,
	sci_cursor, sci_patch, sci_bitmap, sci_palette,
	sci_cdaudio, sci_audio, sci_sync, sci_message,
	sci_map, sci_heap, sci_invalid_resource
};

#define sci0_last_resource sci_patch
/* Used for autodetection */


struct resource_index_struct {
	unsigned short resource_id;
	unsigned int resource_location;
}; /* resource type as stored in the resource.map file */

typedef struct resource_index_struct resource_index_t;

typedef struct _resource_struct {
	unsigned char *data;

	unsigned short number;
	unsigned short type;
	guint16 id; /* contains number and type */
	guint16 length;
	unsigned char status;
	unsigned char file; /* Number of the resource file this resource is stored in */

	unsigned short lockers; /* Number of places where this resource was locked */
	unsigned int file_offset; /* Offset in file */
	struct _resource_struct *next; /* Position marker for the LRU queue */
	struct _resource_struct *prev;
} resource_t; /* for storing resources in memory */


typedef struct {
	int max_memory; /* Config option: Maximum total byte number allocated */
	int sci_version; /* SCI resource version to use */

	int resources_nr;
	resource_t *resources;

	int memory_locked; /* Amount of resource bytes in locked memory */ 
	int memory_lru; /* Amount of resource bytes under LRU control */

	char *resource_path; /* Path to the resource and patch files */

	resource_t *lru_first, *lru_last; /* Pointers to the first and last LRU queue entries */
	/* LRU queue: lru_first points to the most recent entry */

	unsigned char allow_patches;
} resource_mgr_t;

extern DLLEXTERN resource_t *resource_map;

/**** FUNCTION DECLARATIONS ****/

/**--- Old resource manager ---**/

int loadResources(int version, int allow_patches);
/* Reads and parses all resource files in the current directory.
** Parameters: version: The SCI version to look for; use SCI_VERSION_AUTODETECT
**                      in the default case.
**             allow_patches: Set to 1 if external patches (those look like
**                            "view.101" or "script.093") should be applied
** Returns   : (int) Always 0.
** TODO: Add some error checking.
*/

void freeResources();
/* Frees all allocated resource memory.
** Parameters: none
** Returns   : (void)
*/

resource_t * findResource(unsigned short type,
                          unsigned short number);
/* Searches for a resource with the specified resource type and number.
** Parameters: type: The resource type (one of ResourceTypes)
**             number: The resource number (the "042" in "view.042")
** Returns   : (resource_t *) pointing to the resource, or NULL
**             if it could not be found.
*/

/**--- New Resource manager ---**/

resource_mgr_t *
scir_new_resource_manager(char *dir, int version, char allow_patches, int max_memory);
/* Creates a new FreeSCI resource manager
** Parameters: (char *) dir: Path to the resource and patch files (not modified or freed
**                           by the resource manager)
**             (int) version: The SCI version to look for; use SCI_VERSION_AUTODETECT
**                            in the default case.
**             (char ) allow_patches: Set to 1 if external patches (those look like
**                                   "view.101" or "script.093") should be applied
**             (int) max_memory: Maximum number of bytes to allow allocated for resources
** Returns   : (resource_mgr_t *) A newly allocated resource manager
** max_memory will not be interpreted as a hard limit, only as a restriction for resources
** which are not explicitly locked. However, a warning will be issued whenever this limit
** is exceeded.
*/

resource_t *
scir_find_resource(resource_mgr_t *mgr, int type, int number, int lock);
/* Looks up a resource's data
** Parameters: (resource_mgr_t *) mgr: The resource manager to look up in
**             (int) type: The resource type to look for
**             (int) number: The resource number to search
**             (int) lock: non-zero iff the resource should be locked
** Returns   : (resource_t *): The resource, or NULL if it doesn't exist
** Locked resources are guaranteed not to have their contents freed until
** they are unlocked explicitly (by scir_unlock_resource).
*/

void
scir_unlock_resource(resource_mgr_t *mgr, resource_t *res);
/* Unlocks a previously locked resource
** Parameters: (resource_mgr_t *) mgr: The manager the resource should be freed from
**             (resource_t *) res: The resource to free
** Returns   : (void)
*/

void
scir_free_resource_manager(resource_mgr_t *mgr);
/* Frees a resource manager and all memory handled by it
** Parameters: (resource_mgr_t *) mgr: The Manager to free
** Returns   : (void)
*/

/**--- Decompression functions ---**/

int decompress0(resource_t *result, int resh);
/* Decrypts resource data and stores the result for SCI0-style compression.
** Parameters : result: The resource_t the decompressed data is stored in.
**              resh  : File handle of the resource file
** Returns    : (int) 0 on success, one of SCI_ERROR_* if a problem was
**               encountered.
*/

int decompress01(resource_t *result, int resh);
/* Decrypts resource data and stores the result for SCI01-style compression.
** Parameters : result: The resource_t the decompressed data is stored in.
**              resh  : File handle of the resource file
** Returns    : (int) 0 on success, one of SCI_ERROR_* if a problem was
**               encountered.
*/

int decompress1e(resource_t *result, int resh);
/* Decrypts resource data and stores the result for early SCI1-style compression.
** Parameters : result: The resource_t the decompressed data is stored in.
**              resh  : File handle of the resource file
** Returns    : (int) 0 on success, one of SCI_ERROR_* if a problem was
**               encountered.
*/

int decompress1l(resource_t *result, int resh);
/* Decrypts resource data and stores the result for late SCI1-style compression.
** Parameters : result: The resource_t the decompressed data is stored in.
**              resh  : File handle of the resource file
** Returns    : (int) 0 on success, one of SCI_ERROR_* if a problem was
**               encountered.
*/

int decompress11(resource_t *result, int resh);
/* Decrypts resource data and stores the result for SCI1.1-style compression.
** Parameters : result: The resource_t the decompressed data is stored in.
**              resh  : File handle of the resource file
** Returns    : (int) 0 on success, one of SCI_ERROR_* if a problem was
**               encountered.
*/


int decrypt2(guint8* dest, guint8* src, int length, int complength);
/* Huffman token decryptor - defined in decompress0.c and used in decompress01.c
*/

int decrypt4(guint8* dest, guint8* src, int length, int complength);
/* DCL inflate- implemented in decompress1.c
*/

/**** Internal #defines ****/

/* Resource type encoding */
#define SCI0_B1_RESTYPE_MASK  0xf8
#define SCI0_B1_RESTYPE_SHIFT 3
#define SCI0_B3_RESFILE_MASK  0xfd
#define SCI0_B3_RESFILE_SHIFT 2

#define SCI0_RESID_GET_TYPE(bytes) \
    (((bytes)[1] & SCI0_B1_RESTYPE_MASK) >> SCI0_B1_RESTYPE_SHIFT)
#define SCI0_RESID_GET_NUMBER(bytes) \
    ((((bytes)[1] & ~SCI0_B1_RESTYPE_MASK) << 8) | ((bytes)[0]))
#define SCI0_RESFILE_GET_FILE(bytes) \
    (((bytes)[3] & SCI0_B3_RESFILE_MASK) >> SCI0_B3_RESFILE_SHIFT)
#define SCI0_RESFILE_GET_OFFSET(bytes) \
    ((((bytes)[3] & ~SCI0_B3_RESFILE_MASK) << 24) \
      | (((bytes)[2]) << 16) \
      | (((bytes)[1]) << 8) \
      | (((bytes)[0]) << 0))

#endif



