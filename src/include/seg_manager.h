/***************************************************************************
 seg_manager.h Copyright (C) 2002 Xiaojun Chen


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


#ifndef _SCI_SEG_MANAGER_H
#define _SCI_SEG_MANAGER_H

#include <int_hashmap.h>
#include <vm.h>

#define DEFAULT_SCRIPTS 32

#define GET_HEAP( s, reg, mem_type ) s->seg_manager.get_heap( &s->seg_manager, reg, mem_type )
#define PUT_HEAP( s, reg, v, mem_type ) s->seg_manager.put_heap( &s->seg_manager, reg, v, mem_type )

/* SCRIPT_ID must be 0 */
typedef enum {
	SCRIPT_ID,
	SEG_ID,
} id_flag;

typedef enum {
	SCRIPT_BUFFER,
} mem_obj_enum;


/* verify the the given condition is true, output the message if condition is false, and exit
** Parameters:
**   cond - condition to be verified
**   msg  - the message to be printed if condition fails
** return:
**   none, terminate the program if fails
*/
void verify (int cond, const char* file, int line, const char* msg);

#define MEM_OBJ_SCRIPT (1 << 0)
#define MEM_OBJ_CLONES (1 << 1)


struct _mem_obj;

#define GET_SEGMENT(mgr, index, rtype) ((index) >= 0 && (mgr).heap_size > index)?	\
		(((mgr).heap[index]->type & rtype)? (mgr).heap[index]			\
		: NULL) /* Type does not match */					\
	: NULL /* Invalid index */

typedef struct _seg_manager_t {
	int_hash_map_t* id_seg_map; // id - script id; seg - index of heap
	struct _mem_obj** heap;
	int heap_size;		// size of the heap

	/* member methods */
	void (*init) (struct _seg_manager_t* self);
	void (*destroy) (struct _seg_manager_t* self);
	
	int (*seg_get) (struct _seg_manager_t* self, int script_nr);

	int (*allocate) (struct _seg_manager_t* self, struct _state *s, int script_nr, int* seg_id);
	int (*deallocate) (struct _seg_manager_t* self, struct _state *s, int script_nr);
	void (*update) (struct _seg_manager_t* self);
	
	// memory operations
	void (*mset) (struct _seg_manager_t* self, int offset, int c, size_t n, int id, int flag);	// memset
	// dst - inside the seg_manager, its an offset, src - the same as dst; memcpy
	void (*mcpy_in_in) (struct _seg_manager_t* self, int dst, const int src, size_t n, int id, int flag);
	// dst - inside the seg_manager, its an offset, src - absolute address, outside seg_manager; memcpy
	void (*mcpy_in_out) (struct _seg_manager_t* self, int dst, const void* src, size_t n, int id, int flag);
	// dst - absolute address, outside seg_manager; src - inside the seg_manager, its an offset, memcpy
	void (*mcpy_out_in) (struct _seg_manager_t* self, void* dst, const int src, size_t n, int id, int flag);
	
	gint16 (*get_heap) (struct _seg_manager_t* self, reg_t reg, mem_obj_enum mem_type );
	void (*put_heap) (struct _seg_manager_t* self, reg_t reg, gint16 value, mem_obj_enum mem_type );

	int (*isloaded) (struct _seg_manager_t* self, int id, int flag);

	/* script.lockers manipulators */
	void (*increment_lockers) (struct _seg_manager_t* self, int id, int flag);
	void (*decrement_lockers) (struct _seg_manager_t* self, int id, int flag);
	int (*get_lockers) (struct _seg_manager_t* self, int id, int flag);
	void (*set_lockers) (struct _seg_manager_t* self, int lockers, int id, int flag);
	
	int (*get_heappos) (struct _seg_manager_t* self, int id, int flag);

	void (*set_export_table_offset) (struct _seg_manager_t* self, int offset, int id, int flag);
	int (*get_export_table_offset) (struct _seg_manager_t* self, int id, int flag);

	void (*set_synonyms_offset) (struct _seg_manager_t* self, int offset, int id, int flag);
	int (*get_synonyms_offset) (struct _seg_manager_t* self, int id, int flag);	
	
	void (*set_synonyms_nr) (struct _seg_manager_t* self, int nr, int id, int flag);
	int (*get_synonyms_nr) (struct _seg_manager_t* self, int id, int flag);

	void (*set_localvar_offset) (struct _seg_manager_t* self, int offset, int id, int flag);
	int (*get_localvar_offset) (struct _seg_manager_t* self, int id, int flag);
	
} seg_manager_t;

// implementation of seg_manager method
void sm_init (seg_manager_t* self);
/* initialize the member variables
*/

void sm_destroy (seg_manager_t* self);

int sm_seg_get (seg_manager_t* self, int script_nr);

int sm_allocate (seg_manager_t* self, struct _state *s, int script_nr, int* seg_id);
/* allocate a memory from heap
** Parameters: (state_t *) s: The state to operate on
**             (int) script_nr: The script number to load
** Returns   : 0 - allocate failure
**             1 - allocate successfully, seg_id contains the allocated seg_id
*/

int sm_deallocate (seg_manager_t* self, struct _state *s, int script_nr);
void sm_update (seg_manager_t* self);
// memory operations
void sm_mset (seg_manager_t* self, int offset, int c, size_t n, int id, int flag);
void sm_mcpy_in_in (seg_manager_t* self, int dst, const int src, size_t n, int id, int flag);
void sm_mcpy_in_out (seg_manager_t* self, int dst, const void* src, size_t n, int id, int flag);
void sm_mcpy_out_in (seg_manager_t* self, void* dst, const int src, size_t n, int id, int flag);
gint16 sm_get_heap (seg_manager_t* self, reg_t reg, mem_obj_enum mem_type );
void sm_put_heap (seg_manager_t* self, reg_t reg, gint16 value, mem_obj_enum mem_type );

int sm_isloaded (seg_manager_t* self, int id, int flag);

// if flag = 0, id - script id
// otherwise,   id - seg id
void sm_increment_lockers (seg_manager_t* self, int id, int flag);
void sm_decrement_lockers (seg_manager_t* self, int id, int flag);
int sm_get_lockers (seg_manager_t* self, int id, int flag);
void sm_set_lockers (seg_manager_t* self, int lockers, int id, int flag);

int sm_get_heappos (struct _seg_manager_t* self, int id, int flag);	// return 0

void sm_set_export_table_offset (struct _seg_manager_t* self, int offset, int id, int flag);
int sm_get_export_table_offset (struct _seg_manager_t* self, int id, int flag);

void sm_set_synonyms_offset (struct _seg_manager_t* self, int offset, int id, int flag);
int sm_get_synonyms_offset (struct _seg_manager_t* self, int id, int flag);	
	
void sm_set_synonyms_nr (struct _seg_manager_t* self, int nr, int id, int flag);
int sm_get_synonyms_nr (struct _seg_manager_t* self, int id, int flag);

void sm_set_localvar_offset (struct _seg_manager_t* self, int offset, int id, int flag);
int sm_get_localvar_offset (struct _seg_manager_t* self, int id, int flag);

// validate the seg
// return:
//	0 - invalid seg
//	1 - valid seg
int sm_check (seg_manager_t* self, int seg);

#endif
