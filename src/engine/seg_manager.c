/***************************************************************************
 seg_manager.c Copyright (C) 2000,2001 Christoph Reichenbach


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

#include <seg_manager.h>

void sm_init(seg_manager_t* self) {
	int i;
	
	self->id_seg_map = new_int_hash_map();
	self->heap_size = DEFAULT_SCRIPTS;
        self->heap = (mem_obj_t**) malloc (self->heap_size);

	/*  initialize the heap pointers*/
	for (i = 0; i < self->heap_size; i++) {
		self->heap[i] = NULL;
	}

	self->instantiate = sm_instantiate;
	self->uninstantiate = sm_uninstantiate;
	self->update = sm_update;
	self->isloaded = sm_isloaded;
	
	self->increment_lockers = sm_increment_lockers;
	self->decrement_lockers = sm_decrement_lockers;
	self->get_lockers = sm_get_lockers;
	self->set_lockers = sm_set_lockers;
};

int sm_instantiate (seg_manager_t* self, struct _state *s, int script_nr) {
	int seg;
        char was_added;
	mem_obj_t* mem;
	resource_t *script;
	void* temp;
        
	seg = int_hash_map_check_value (self->id_seg_map, script_nr, 1, &was_added);
	if (!was_added) 
		return;

	if (seg >= self->heap_size) {
		if (seg >= self->heap_size * 2) {
			sciprintf( "seg_manager.c: hash_map error or others??" );
			return 0;
		}
		self->heap_size *= 2;
		temp = realloc ((void*)self->heap, self->heap_size);
		if (!temp) {
			sciprintf("seg_manager.c: Not enough memory space for script size" );
			return 0;
		}
		self->heap = (mem_obj_t**)  temp;
	}
	
	/*script = scir_find_resource(s->resmgr, sci_script, script_nr, 0); //???*/
	mem->size = 100;		// ?????????????????? just for compile script->size;
        mem->data.script.buf = (char*) malloc (mem->size);
	mem->type = BUFFER;
	if (!mem->data.script.buf) {
		sciprintf("seg_manager.c: Not enough memory space for script size" );
		return 0;
	}
	self->heap[seg] = mem;
	return 1;
};

int sm_uninstantiate (seg_manager_t* self, struct _state *s, int script_nr) {
/*
	seg = int_hash_map_check_value (id_seg_map, script_nr, 0, NULL);
	if (seg == -1) 
		return;
	delete heap[seg]
        remove_hash_map_value( ,,,, seg, )
	*/
	return 1;
};

void sm_update (seg_manager_t* self) {
};

int sm_check (seg_manager_t* self, int script_id) {
	int seg = int_hash_map_check_value (self->id_seg_map, script_id, 0, NULL);
	if (seg == -1) 
		return seg;
	if (!self->heap[seg]) {
		sciprintf("seg_manager.c: seg is removed from memory, but not removed from hash_map " );
		return -1;
	}
	return seg;
};

int sm_isloaded (seg_manager_t* self, int script_id) {
	int seg = sm_check (self, script_id);
	return  (seg == -1) ? 0 : 1;
};

void sm_increment_lockers (seg_manager_t* self, int script_id) {
	int seg = sm_check (self, script_id);
	self->heap[seg]->data.script.lockers++;
};

void sm_decrement_lockers (seg_manager_t* self, int script_id) {
	int seg = sm_check (self, script_id);
	self->heap[seg]->data.script.lockers--;
};

int sm_get_lockers (seg_manager_t* self, int script_id) {
	int seg = sm_check (self, script_id);
	return self->heap[seg]->data.script.lockers;
};

void sm_set_lockers (seg_manager_t* self, int script_id, int lockers) {
	int seg = sm_check (self, script_id);
	self->heap[seg]->data.script.lockers = lockers;
};
