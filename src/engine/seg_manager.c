/***************************************************************************
 seg_manager.c Copyright (C) 2002 Xiaojun Chen


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
#include <sciresource.h>
#include <versions.h>
#include <engine.h>

#define GET_SEGID() 	if (flag == SCRIPT_ID) \
				id = sm_seg_get (self, id); \
			verify ( sm_check (self, id), __FILE__, __LINE__, "invalid seg id" );


void verify (int cond, const char* file, int line, const char* msg) {
	if (cond)
		return;
	sciprintf( "%s, line, %d, %s", file, line, msg );
	exit ( -1 );
};

void sm_init(seg_manager_t* self) {
	int i;

	self->id_seg_map = new_int_hash_map();
	self->heap_size = DEFAULT_SCRIPTS;
        self->heap = (mem_obj_t**) malloc (self->heap_size * sizeof(mem_obj_t *));

	/*  initialize the heap pointers*/
	for (i = 0; i < self->heap_size; i++) {
		self->heap[i] = NULL;
	}

	self->destroy = sm_destroy;

	self->seg_get = sm_seg_get;

	self->allocate = sm_allocate;
	self->deallocate = sm_deallocate;
	self->update = sm_update;
	self->isloaded = sm_isloaded;
	self->mset = sm_mset;
	self->mcpy_in_in = sm_mcpy_in_in;
	self->mcpy_in_out = sm_mcpy_in_out;
	self->mcpy_out_in = sm_mcpy_out_in;
	self->get_heap = sm_get_heap;
	self->put_heap = sm_put_heap;

	self->increment_lockers = sm_increment_lockers;
	self->decrement_lockers = sm_decrement_lockers;
	self->get_lockers = sm_get_lockers;
	self->set_lockers = sm_set_lockers;
	
	self->get_heappos = sm_get_heappos;

	self->set_export_table_offset = sm_set_export_table_offset;
	self->get_export_table_offset = sm_get_export_table_offset;

	self->set_synonyms_offset = sm_set_synonyms_offset;
	self->get_synonyms_offset = sm_get_synonyms_offset;
	
	self->set_synonyms_nr = sm_set_synonyms_nr;
	self->get_synonyms_nr = sm_get_synonyms_nr;

	self->set_localvar_offset = sm_set_localvar_offset;
	self->get_localvar_offset = sm_get_localvar_offset;

};

void sm_destroy (seg_manager_t* self) {
	int i;
	/* free memory*/
	for (i = 0; i < self->heap_size; i++) {
		if (self->heap[i]) {
			if (self->heap[i]->data.script.buf) {
				free (self->heap[i]->data.script.buf);
				self->heap[i]->data.script.buf = NULL;
			}
			free (self->heap[i]);
			self->heap[i] = NULL;
		}
	}
	free (self->heap);
	self->heap = NULL;
};

/* allocate a memory from heap
** Parameters: (state_t *) s: The state to operate on
**             (int) script_nr: The script number to load
** Returns   : 0 - allocate failure
**             1 - allocate successfully
**             seg_id - allocated segment id
*/
int sm_allocate (seg_manager_t* self, struct _state *s, int script_nr, int* seg_id) {
	int seg;
	char was_added;
	mem_obj_t* mem;
	resource_t *script;
	void* temp;

	seg = int_hash_map_check_value (self->id_seg_map, script_nr, 1, &was_added);
	if (!was_added) {
		*seg_id = seg;
		return 1;
	}

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

	// allocate the mem_obj_t
	mem = (mem_obj_t*) malloc (sizeof (mem_obj_t));
	if (!mem) {
		sciprintf("%s, %d, Not enough memory, ", __FILE__, __LINE__ );
		return 0;
	}

	mem->type = SCRIPT_BUFFER;	// may need to handle non script buffer type at here
	mem->data.script.buf = NULL;
	script = scir_find_resource(s->resmgr, sci_script, script_nr, 0);
	
	if (s->version < SCI_VERSION_FTU_NEW_SCRIPT_HEADER) {
		mem->data.script.buf_size = script->size + getUInt16(script->data)*2; // locals_size = getUInt16(script->data)*2;
	}
	else {
		mem->data.script.buf_size = script->size;
	}
        mem->data.script.buf = (char*) malloc (mem->data.script.buf_size);
	
	if (!mem->data.script.buf) {
		sciprintf("seg_manager.c: Not enough memory space for script size" );
		mem->data.script.buf_size = 0;
		return 0;
	}
	self->heap[seg] = mem;
	*seg_id = seg;
	return 1;
};

int sm_deallocate (seg_manager_t* self, struct _state *s, int script_nr) {
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

// memory operations
void sm_mset (seg_manager_t* self, int offset, int c, size_t n, int id, int flag) {
	GET_SEGID();
	memset( self->heap[id] + offset, c, n );
};

void sm_mcpy_in_in (seg_manager_t* self, int dst, const int src, size_t n, int id, int flag) {
	GET_SEGID();
	memcpy ( self->heap[id] + dst, self->heap[id] + src, n );
};

void sm_mcpy_in_out (seg_manager_t* self, int dst, const void* src, size_t n, int id, int flag) {
	GET_SEGID();
	memcpy ( self->heap[id] + dst, src, n );
};
void sm_mcpy_out_in (seg_manager_t* self, void* dst, const int src, size_t n, int id, int flag) {
	GET_SEGID();
	memcpy ( dst, self->heap[id] + src, n );
};


gint16 sm_get_heap (seg_manager_t* self, reg_t reg, mem_obj_enum mem_type ) {
	switch( mem_type ) {
	case SCRIPT_BUFFER:
		verify ( sm_check (self, reg.segment) && reg.offset + 1 < self->heap[reg.segment]->size , __FILE__, __LINE__, "invalid seg id, or offset" );
		return ( self->heap[reg.segment]->data.script.buf[reg.offset] +
		  self->heap[reg.segment]->data.script.buf[reg.offset + 1] << 8 );
	default:
		sciprintf( "haven't implemented yet" );
		exit( -1 );
	}
	return 0;	// never get here
}

void sm_put_heap (seg_manager_t* self, reg_t reg, gint16 value, mem_obj_enum mem_type ) {
	switch( mem_type ) {
	case SCRIPT_BUFFER:
		verify ( sm_check (self, reg.segment) && reg.offset + 1 < self->heap[reg.segment]->size , __FILE__, __LINE__, "invalid seg id, or offset" );
		self->heap[reg.segment]->data.script.buf[reg.offset] = value & 0xff;
		self->heap[reg.segment]->data.script.buf[reg.offset + 1] = value >> 8;
		break;
	default:
		sciprintf( "haven't implemented yet" );
		exit( -1 );
	}
};

// return the seg if script_id is valid and in the map, else -1
int sm_seg_get (seg_manager_t* self, int script_id) {
	return int_hash_map_check_value (self->id_seg_map, script_id, 0, NULL);
};

// validate the seg
// return:
//	0 - invalid seg
//	1 - valid seg
int sm_check (seg_manager_t* self, int seg) {
	if ( seg < 0 || seg >= self->heap_size ) {
		return 0;
	}
	if (!self->heap[seg]) {
		sciprintf("seg_manager.c: seg is removed from memory, but not removed from hash_map " );
		return 0;
	}
	return 1;
};

int sm_isloaded (seg_manager_t* self, int id, int flag) {
	if (flag == SCRIPT_ID)
		id = sm_seg_get (self, id);
	return sm_check (self, id);
};

void sm_increment_lockers (seg_manager_t* self, int id, int flag) {
	if (flag == SCRIPT_ID)
		id = sm_seg_get (self, id);
	verify ( sm_check (self, id), __FILE__, __LINE__, "invalid seg id" );
	self->heap[id]->data.script.lockers++;
};

void sm_decrement_lockers (seg_manager_t* self, int id, int flag) {
	if (flag == SCRIPT_ID)
		id = sm_seg_get (self, id);
	verify ( sm_check (self, id), __FILE__, __LINE__, "invalid seg id" );
	self->heap[id]->data.script.lockers--;
};

int sm_get_lockers (seg_manager_t* self, int id, int flag) {
	if (flag == SCRIPT_ID)
		id = sm_seg_get (self, id);
	verify ( sm_check (self, id), __FILE__, __LINE__, "invalid seg id" );
	return self->heap[id]->data.script.lockers;
};

void sm_set_lockers (seg_manager_t* self, int lockers, int id, int flag) {
	if (flag == SCRIPT_ID)
		id = sm_seg_get (self, id);
	verify ( sm_check (self, id), __FILE__, __LINE__, "invalid seg id" );
	self->heap[id]->data.script.lockers = lockers;
};

void sm_set_export_table_offset (struct _seg_manager_t* self, int offset, int id, int flag) {
	GET_SEGID();
	self->heap[id]->data.script.export_table_offset = offset;
};

int sm_get_export_table_offset (struct _seg_manager_t* self, int id, int flag) {
	GET_SEGID();
	return self->heap[id]->data.script.export_table_offset;
};

void sm_set_synonyms_offset (struct _seg_manager_t* self, int offset, int id, int flag) {
	GET_SEGID();
	self->heap[id]->data.script.synonyms_offset = offset;
};
int sm_get_synonyms_offset (struct _seg_manager_t* self, int id, int flag) {
	GET_SEGID();
	return self->heap[id]->data.script.synonyms_offset;
};	
	
void sm_set_synonyms_nr (struct _seg_manager_t* self, int nr, int id, int flag) {
	GET_SEGID();
	self->heap[id]->data.script.synonyms_nr = nr;
};

int sm_get_synonyms_nr (struct _seg_manager_t* self, int id, int flag) {
	GET_SEGID();
	return self->heap[id]->data.script.synonyms_nr;
};

void sm_set_localvar_offset (struct _seg_manager_t* self, int offset, int id, int flag) {
	GET_SEGID();
	// self->heap[id]->data.script.localvar_offset = offset; ????
};

int sm_get_localvar_offset (struct _seg_manager_t* self, int id, int flag) {
	GET_SEGID();
	return 1; //self->heap[id]->data.script.localvar_offset; // ???? , we removed it!!!
};

int sm_get_heappos (struct _seg_manager_t* self, int id, int flag) {
	GET_SEGID();
	return 0;
}
