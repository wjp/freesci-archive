/***************************************************************************
 seg_manager.h Copyright (C) 2000,2001 Christoph Reichenbach


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

enum mem_obj_enum{
	BUFFER,
};

typedef struct _seg_manager_t {
	int_hash_map_t* id_seg_map; // id - script id; seg - index of heap
	mem_obj_t** heap;
        int heap_size;
	void (*init) (struct _seg_manager_t* self);
	int (*instantiate) (struct _seg_manager_t* self, struct _state *s, int script_nr);
	int (*uninstantiate) (struct _seg_manager_t* self, struct _state *s, int script_nr);
	void (*update) (struct _seg_manager_t* self);
	
	int (*isloaded) (struct _seg_manager_t* self, int script_id);
	
	/* script.lockers manipulators */
	void (*increment_lockers) (struct _seg_manager_t* self, int script_id);
	void (*decrement_lockers) (struct _seg_manager_t* self, int script_id);
	int (*get_lockers) (struct _seg_manager_t* self, int script_id);
	void (*set_lockers) (struct _seg_manager_t* self, int script_id, int lockers);
} seg_manager_t;

// implementation of seg_manager method
void sm_init (seg_manager_t* self);
/* initialize the member variables
*/

int sm_instantiate (seg_manager_t* self, struct _state *s, int script_nr);
/* Makes sure that a script and its superclasses get loaded to the heap
** Parameters: (state_t *) s: The state to operate on
**             (int) script_nr: The script number to load
**             (int) recursive: Whether to recurse
** Returns   : (heap_ptr) The address of the script on the heap or 0 if out of heap
** If the script already has been loaded, only the number of lockers is increased.
** All scripts containing superclasses of this script aret loaded recursively as well,
** unless 'recursive' is set to zero.
** The complementary function is "script_uninstantiate()" below.
*/

int sm_uninstantiate (seg_manager_t* self, struct _state *s, int script_nr);
void sm_update (seg_manager_t* self);

int sm_isloaded (seg_manager_t* self, int script_id);

void sm_increment_lockers (seg_manager_t* self, int script_id);
void sm_decrement_lockers (seg_manager_t* self, int script_id);
int sm_get_lockers (seg_manager_t* self, int script_id);
void sm_set_lockers (seg_manager_t* self, int script_id, int lockers);

int sm_check (seg_manager_t* self, int script_id);

#endif
