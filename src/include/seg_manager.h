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
#include <sys_strings.h>
#include <vm.h>

#define DEFAULT_SCRIPTS 32
#define DEFAULT_OBJECTS 8	    /* default # of objects per script */
#define DEFAULT_OBJECTS_INCREMENT 4 /* Number of additional objects to
				    ** instantiate if we're running out of them  */

#define SEG_GET_HEAP( s, reg, mem_type ) s->seg_manager.get_heap( &s->seg_manager, reg, mem_type )
#define SEG_GET_HEAP2( s, seg, offset, mem_type ) s->seg_manager.get_heap2( &s->seg_manager, seg, offset, mem_type )
#define SEG_PUT_HEAP( s, reg, v, mem_type ) s->seg_manager.put_heap( &s->seg_manager, reg, v, mem_type )

/* SCRIPT_ID must be 0 */
typedef enum {
	SCRIPT_ID,
	SEG_ID,
} id_flag;

void dbg_print( char* msg, int i );		// for debug only

/* verify the the given condition is true, output the message if condition is false, and exit
** Parameters:
**   cond - condition to be verified
**   msg  - the message to be printed if condition fails
** return:
**   none, terminate the program if fails
*/
#define VERIFY( cond, msg ) if (! ( cond ) ) {\
	sciprintf( "%s, line, %d, %s\n", __FILE__, __LINE__, msg ); \
	BREAKPOINT(); \
	}

#define MEM_OBJ_INVALID 0
#define MEM_OBJ_SCRIPT 1
#define MEM_OBJ_CLONES 2
#define MEM_OBJ_LOCALS 3
#define MEM_OBJ_STACK 4
#define MEM_OBJ_SYS_STRINGS 5
#define MEM_OBJ_LISTS 6
#define MEM_OBJ_NODES 7
#define MEM_OBJ_MAX MEM_OBJ_NODES /* For sanity checking */
typedef int mem_obj_enum;

struct _mem_obj;

#define GET_SEGMENT(mgr, index, rtype) ((index) > 0 && (mgr).heap_size > index)?		\
		(((mgr).heap[index] && (mgr).heap[index]->type == rtype)? (mgr).heap[index]	\
		: NULL) /* Type does not match */						\
	: NULL /* Invalid index */

#define GET_OBJECT_SEGMENT(mgr, index) ((index) > 0 && (mgr).heap_size > index)?		\
		(((mgr).heap[index]								\
                    && ((mgr).heap[index]->type == MEM_OBJ_SCRIPT				\
		        || (mgr).heap[index]->type == MEM_OBJ_CLONES))? (mgr).heap[index]	\
		: NULL) /* Type does not match */						\
	: NULL /* Invalid index */

typedef struct _seg_manager_t {
	int_hash_map_t* id_seg_map; // id - script id; seg - index of heap
	struct _mem_obj** heap;
	int heap_size;		// size of the heap
	int reserved_id;

	seg_id_t clones_seg_id; /* ID of the (a) clones segment */
	seg_id_t lists_seg_id; /* ID of the (a) list segment */
	seg_id_t nodes_seg_id; /* ID of the (a) node segment */

	/* member methods */
	void (*init) (struct _seg_manager_t* self);
	void (*destroy) (struct _seg_manager_t* self);

	int (*seg_get) (struct _seg_manager_t* self, int script_nr);

	int (*allocate_script) (struct _seg_manager_t* self, struct _state *s, int script_nr, int* seg_id);
	int (*deallocate_script) (struct _seg_manager_t* self, struct _state *s, int script_nr);
	void (*update) (struct _seg_manager_t* self);

	dstack_t * (*allocate_stack) (struct _seg_manager_t *self, int size, seg_id_t *segid);
	/* Allocates a data stack
	** Parameters: (int) size: Number of stack entries to reserve
	** Returns   : (dstack_t *): The physical stack
	**             (seg_id_t) segid: Segment ID of the stack
	*/

	sys_strings_t * (*allocate_sys_strings) (struct _seg_manager_t *self, seg_id_t *segid);
	/* Allocates a system string table
	** Returns   : (dstack_t *): The physical stack
	**             (seg_id_t) segid: Segment ID of the stack
	** See also sys_string_acquire();
	*/

	// memory operations
	void (*mset) (struct _seg_manager_t* self, int offset, int c, size_t n, int id, int flag);	// memset
	// dst - inside the seg_manager, its an offset, src - the same as dst; memcpy
	void (*mcpy_in_in) (struct _seg_manager_t* self, int dst, const int src, size_t n, int id, int flag);
	// dst - inside the seg_manager, its an offset, src - absolute address, outside seg_manager; memcpy
	void (*mcpy_in_out) (struct _seg_manager_t* self, int dst, const void* src, size_t n, int id, int flag);
	// dst - absolute address, outside seg_manager; src - inside the seg_manager, its an offset, memcpy
	void (*mcpy_out_in) (struct _seg_manager_t* self, void* dst, const int src, size_t n, int id, int flag);

	gint16 (*get_heap) (struct _seg_manager_t* self, reg_t reg, mem_obj_enum mem_type );
	gint16 (*get_heap2) (struct _seg_manager_t* self, seg_id_t seg, int offset, mem_obj_enum mem_type );
	void (*put_heap) (struct _seg_manager_t* self, reg_t reg, gint16 value, mem_obj_enum mem_type );

	int (*isloaded) (struct _seg_manager_t* self, int id, int flag);

	/* script.lockers manipulators */
	void (*increment_lockers) (struct _seg_manager_t* self, int id, int flag);
	void (*decrement_lockers) (struct _seg_manager_t* self, int id, int flag);
	int (*get_lockers) (struct _seg_manager_t* self, int id, int flag);
	void (*set_lockers) (struct _seg_manager_t* self, int lockers, int id, int flag);

	int (*get_heappos) (struct _seg_manager_t* self, int id, int flag);

	void (*set_export_table_offset) (struct _seg_manager_t* self, int offset,
					 int magic_offset, int id, int flag);
	guint16 *(*get_export_table_offset) (struct _seg_manager_t* self, int id, int flag,
					     int *max);

	void (*set_synonyms_offset) (struct _seg_manager_t* self, int offset, int id, int flag);
	byte *(*get_synonyms) (struct _seg_manager_t* self, int id, int flag);

	void (*set_synonyms_nr) (struct _seg_manager_t* self, int nr, int id, int flag);
	int (*get_synonyms_nr) (struct _seg_manager_t* self, int id, int flag);

	void (*set_variables) (struct _seg_manager_t* self, reg_t reg, int obj_index, reg_t variable_reg, int variable_index );
	
	guint16 (*validate_export_func)(struct _seg_manager_t* self, int pubfunct, int seg );

	object_t* (*script_obj_init) (struct _seg_manager_t* self, reg_t obj_pos);
	/* Initializes an object within the segment manager
	** Parameters: (reg_t) obj_pos: Location (segment, offset) of the object
	** Returns   : (object_t *) A newly created object_t describing the object
	** obj_pos must point to the beginning of the script/class block (as opposed
	** to what the VM considers to be the object location)
	** The corresponding object_t is stored within the relevant script.
	*/

	void (*script_relocate) (struct _seg_manager_t* self, reg_t block);
	/* Processes a relocation block witin a script
	** Parameters: (reg_t) obj_pos: Location (segment, offset) of the block
	** Returns   : (object_t *) Location of the relocation block
	** This function is idempotent, but it must only be called after all
	** objects have been instantiated, or a run-time error will occur.
	*/

	void (*script_initialize_locals_zero) (struct _seg_manager_t *self, seg_id_t seg, int nr);
	/* Initializes a script's local variable block
	** Parameters: (seg_id_t) seg: Segment containing the script to initialize
	**             (int) nr: Number of local variables to allocate
	** All variables are initialized to zero.
	*/

	void (*script_initialize_locals) (struct _seg_manager_t *self, reg_t location);
	/* Initializes a script's local variable block according to a prototype
	** Parameters: (reg_t) location: Location to initialize from
	*/

	void (*script_free_unused_objects) (struct _seg_manager_t *self, seg_id_t segid);
	/* Deallocates all unused but allocated entries for objects
	** Parameters: (seg_id_t) segid: segment of the script to prune in this way
	** These entries are created during script instantiation; deallocating them
	** frees up some additional memory.
	*/


	/* Allocation/deallocation primitives for certain objects */
	clone_t* (*alloc_clone)(struct _seg_manager_t *self, reg_t *addr);
	list_t* (*alloc_list)(struct _seg_manager_t *self, reg_t *addr);
	node_t* (*alloc_node)(struct _seg_manager_t *self, reg_t *addr);

	void (*free_clone)(struct _seg_manager_t *self, reg_t addr);
	void (*free_list)(struct _seg_manager_t *self, reg_t addr);
	void (*free_node)(struct _seg_manager_t *self, reg_t addr);


} seg_manager_t;

// implementation of seg_manager method
void sm_init (seg_manager_t* self);
/* initialize the member variables
*/

void sm_destroy (seg_manager_t* self);

int sm_seg_get (seg_manager_t* self, int script_nr);

int sm_allocate_script (seg_manager_t* self, struct _state *s, int script_nr, int* seg_id);
/* allocate a memory from heap
** Parameters: (state_t *) s: The state to operate on
**             (int) script_nr: The script number to load
** Returns   : 0 - allocate failure
**             1 - allocate successfully, seg_id contains the allocated seg_id
*/

int sm_deallocate_script (seg_manager_t* self, struct _state *s, int script_nr);
void sm_update (seg_manager_t* self);
// memory operations
void sm_object_init (object_t* object); 
mem_obj_t* mem_obj_allocate();
void sm_free( mem_obj_t* mem );
void sm_mset (seg_manager_t* self, int offset, int c, size_t n, int id, int flag);
void sm_mcpy_in_in (seg_manager_t* self, int dst, const int src, size_t n, int id, int flag);
void sm_mcpy_in_out (seg_manager_t* self, int dst, const void* src, size_t n, int id, int flag);
void sm_mcpy_out_in (seg_manager_t* self, void* dst, const int src, size_t n, int id, int flag);
gint16 sm_get_heap (seg_manager_t* self, reg_t reg, mem_obj_enum mem_type );
gint16 sm_get_heap2 (seg_manager_t* self, seg_id_t seg, int offset, mem_obj_enum mem_type );
void sm_put_heap (seg_manager_t* self, reg_t reg, gint16 value, mem_obj_enum mem_type );

int sm_isloaded (seg_manager_t* self, int id, int flag);

// if flag = 0, id - script id
// otherwise,   id - seg id
void sm_increment_lockers (seg_manager_t* self, int id, int flag);
void sm_decrement_lockers (seg_manager_t* self, int id, int flag);
int sm_get_lockers (seg_manager_t* self, int id, int flag);
void sm_set_lockers (seg_manager_t* self, int lockers, int id, int flag);

int sm_get_heappos (struct _seg_manager_t* self, int id, int flag);	// return 0

void sm_set_export_table_offset (struct _seg_manager_t* self, int offset,
				 int magic_offset, int id, int flag);
guint16 *sm_get_export_table_offset (struct _seg_manager_t* self, int id, int flag, int *max);

void sm_set_synonyms_offset (struct _seg_manager_t* self, int offset, int id, int flag);
	
void sm_set_synonyms_nr (struct _seg_manager_t* self, int nr, int id, int flag);
int sm_get_synonyms_nr (struct _seg_manager_t* self, int id, int flag);

void sm_set_localvar_offset (struct _seg_manager_t* self, int offset, int id, int flag);
int sm_get_localvar_offset (struct _seg_manager_t* self, int id, int flag);

void sm_set_variables (struct _seg_manager_t* self, reg_t reg, int obj_index, reg_t variable_reg, int variable_index );

guint16 sm_validate_export_func(struct _seg_manager_t* self, int pubfunct, int seg );
/* validate the pubfunct is in the export table, also validate the function pointer is not out of the buffer
   return the pubfunction pointer, or 0 if invalid
**/

// validate the seg
// return:
//	0 - invalid seg
//	1 - valid seg
int sm_check (seg_manager_t* self, int seg);

#endif
