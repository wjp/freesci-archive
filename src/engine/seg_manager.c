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
			VERIFY ( sm_check (self, id), "invalid seg id" );
#define VERIFY_MEM( mem_ptr, ret ) if (! (mem_ptr) )  {\
					sciprintf( "%s, *d, no enough memory", __FILE__, __LINE__ ); \
					return ret; \
				}
					
#define INVALID_SCRIPT_ID -1
	
void dbg_print( char* msg, int i ) {
	char buf[1000];
	sprintf( buf, "%s = [0x%x], dec:[%d]", msg, i, i);
	perror( buf );
};



void sm_init(seg_manager_t* self) {
	int i;

	self->id_seg_map = new_int_hash_map();
	self->reserved_id = INVALID_SCRIPT_ID;
	int_hash_map_check_value (self->id_seg_map, self->reserved_id, 1, NULL);	// reserver 0 for seg_id
	self->reserved_id--;	// reserved_id runs in the reversed direction to make sure no one will use it.
	
	self->heap_size = DEFAULT_SCRIPTS;
        self->heap = (mem_obj_t**) malloc (self->heap_size * sizeof(mem_obj_t *));

	/*  initialize the heap pointers*/
	for (i = 0; i < self->heap_size; i++) {
		self->heap[i] = NULL;
	}

	// function assignments
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
	self->get_heap2 = sm_get_heap2;
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

	self->set_variables = sm_set_variables;
};

// destroy the object, free the memorys if allocated before
void sm_destroy (seg_manager_t* self) {
	int i;
	/* free memory*/
	for (i = 0; i < self->heap_size; i++) {
		if (self->heap[i]) {
			mem_obj_t* mem_obj = self->heap[i];
			switch (mem_obj->type) {
			case MEM_OBJ_SCRIPT:
				sm_free( mem_obj );
				break;
			case MEM_OBJ_CLONES:
				sciprintf( "destroy for clones haven't been implemented\n" );
				free (mem_obj);
				break;
			default:
				sciprintf( "unknown mem obj type\n" );
				free (mem_obj);
				break;
			}
			self->heap[i] = NULL;
		}
	}
	free (self->heap);
	self->heap = NULL;
};

/* allocate a memory for script from heap
** Parameters: (state_t *) s: The state to operate on
**             (int) script_nr: The script number to load
** Returns   : 0 - allocate failure
**             1 - allocate successfully
**             seg_id - allocated segment id
*/
int sm_allocate (seg_manager_t* self, struct _state *s, int script_nr, int* seg_id) {
	int i;
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
		temp = realloc ((void*)self->heap, self->heap_size * sizeof( mem_obj_t* ) );
		if (!temp) {
			sciprintf("seg_manager.c: Not enough memory space for script size" );
			return 0;
		}
		self->heap = (mem_obj_t**)  temp;
	}

	// allocate the mem_obj_t
	mem = mem_obj_allocate();
	if (!mem) {
		sciprintf("%s, %d, Not enough memory, ", __FILE__, __LINE__ );
		return 0;
	}

	// allocate the script.buf
	script = scir_find_resource(s->resmgr, sci_script, script_nr, 0);
	if (s->version < SCI_VERSION_FTU_NEW_SCRIPT_HEADER) {
		mem->data.script.buf_size = script->size + getUInt16(script->data)*2; // locals_size = getUInt16(script->data)*2;
	}
	else {
		mem->data.script.buf_size = script->size;
	}
	mem->data.script.buf = (char*) malloc (mem->data.script.buf_size);
	dbg_print( "mem->data.script.buf ", mem->data.script.buf );
	if (!mem->data.script.buf) {
		sm_free( mem );
		sciprintf("seg_manager.c: Not enough memory space for script size" );
		mem->data.script.buf_size = 0;
		return 0;
	}
	
	// allocate the objects
	mem->data.script.objects = (object_t*) malloc( sizeof(object_t) * DEFAULT_OBJECTS );
	if( !mem->data.script.objects ) {
		sm_free( mem );
		sciprintf("seg_manager.c: Not enough memory space for script objects" );
		return 0;
	}
	mem->data.script.objects_nr = DEFAULT_OBJECTS;
	for( i = 0; i < mem->data.script.objects_nr; i++ ) {
		sm_object_init( &(mem->data.script.objects[i]) );
	}

	// hook it to the heap
	self->heap[seg] = mem;
	*seg_id = seg;
	return 1;
};

int sm_deallocate (seg_manager_t* self, struct _state *s, int script_nr) {
	int seg = sm_seg_get( self, script_nr );
	
	VERIFY ( sm_check (self, seg), "invalid seg id" );

	sm_free ( self->heap[seg] );
	self->heap[seg] = NULL;
	int_hash_map_remove_value( self->id_seg_map, seg );
	return 1;
};

void sm_update (seg_manager_t* self) {
};

mem_obj_t* mem_obj_allocate() {
	mem_obj_t* mem = ( mem_obj_t* ) malloc( sizeof (mem_obj_t) );
	if( !mem ) {
		sciprintf( "seg_manager.c: invalid mem_obj " );
		return 0;
	}
	mem->type = MEM_OBJ_SCRIPT;	// may need to handle non script buffer type at here
	mem->data.script.buf = NULL;
	mem->data.script.buf_size = 0;
	mem->data.script.objects = NULL;
	mem->data.script.objects_nr = 0;
	return mem;
};

void sm_object_init (object_t* object) {
	if( !object )	return;
	object->variables_nr = 0;
	object->variables = NULL;
};

void sm_free( mem_obj_t* mem ) {
	if( !mem ) return;
	if( mem->data.script.buf ) {
		free( mem->data.script.buf );
		mem->data.script.buf = NULL;
		mem->data.script.buf_size = 0;
	}
	if( mem->data.script.objects ) {
		int i;
		for( i = 0; i < mem->data.script.objects_nr; i++ ) {
			object_t* object = &mem->data.script.objects[i];
			if( object->variables ) {
				free( object->variables );
				object->variables = NULL;
				object->variables_nr = 0;
			}
		}
		free( mem->data.script.objects );
		mem->data.script.objects = NULL;
		mem->data.script.objects_nr = 0;
	}
	free( mem );
};

// memory operations
void sm_mset (seg_manager_t* self, int offset, int c, size_t n, int id, int flag) {
	mem_obj_t* mem_obj;
	GET_SEGID();
	mem_obj = self->heap[id];
	switch (mem_obj->type) {
	case MEM_OBJ_SCRIPT:
		if (mem_obj->data.script.buf) {
			memset( mem_obj->data.script.buf + offset, c, n );
		}
		break;
	case MEM_OBJ_CLONES:
		sciprintf( "memset for clones haven't been implemented\n" );
		break;
	default:
		sciprintf( "unknown mem obj type\n" );
		break;
	}
};

void sm_mcpy_in_in (seg_manager_t* self, int dst, const int src, size_t n, int id, int flag) {
	mem_obj_t* mem_obj;
	GET_SEGID();
	mem_obj = self->heap[id];
	switch (mem_obj->type) {
	case MEM_OBJ_SCRIPT:
		if (mem_obj->data.script.buf) {
			memcpy ( mem_obj->data.script.buf + dst, mem_obj->data.script.buf + src, n );
		}
		break;
	case MEM_OBJ_CLONES:
		sciprintf( "memcpy for clones haven't been implemented\n" );
		break;
	default:
		sciprintf( "unknown mem obj type\n" );
		break;
	}
};

void sm_mcpy_in_out (seg_manager_t* self, int dst, const void* src, size_t n, int id, int flag) {
	mem_obj_t* mem_obj;
	GET_SEGID();
	mem_obj = self->heap[id];
	switch (mem_obj->type) {
	case MEM_OBJ_SCRIPT:
		if (mem_obj->data.script.buf) {
			memcpy ( mem_obj->data.script.buf + dst, src, n );
		}
		break;
	case MEM_OBJ_CLONES:
		sciprintf( "memcpy for clones haven't been implemented\n" );
		break;
	default:
		sciprintf( "unknown mem obj type\n" );
		break;
	}
};
void sm_mcpy_out_in (seg_manager_t* self, void* dst, const int src, size_t n, int id, int flag) {
	mem_obj_t* mem_obj;
	GET_SEGID();
	mem_obj = self->heap[id];
	switch (mem_obj->type) {
	case MEM_OBJ_SCRIPT:
		if (mem_obj->data.script.buf) {
			memcpy ( dst, mem_obj->data.script.buf + src, n );
		}
		break;
	case MEM_OBJ_CLONES:
		sciprintf( "memcpy for clones haven't been implemented\n" );
		break;
	default:
		sciprintf( "unknown mem obj type\n" );
		break;
	}
};

gint16 sm_get_heap (seg_manager_t* self, reg_t reg, mem_obj_enum mem_type ) {
	mem_obj_t* mem_obj;
	VERIFY( sm_check (self, reg.segment), "Invalid seg id" );
	mem_obj = self->heap[reg.segment];
	switch( mem_type ) {
	case MEM_OBJ_SCRIPT:
		VERIFY( reg.offset + 1 < mem_obj->data.script.buf_size, "invalid offset" );
		return (unsigned char)mem_obj->data.script.buf[reg.offset] +
		     ( ((unsigned char)mem_obj->data.script.buf[reg.offset+1]) << 8 );
	case MEM_OBJ_CLONES:
		sciprintf( "memcpy for clones haven't been implemented\n" );
		break;
	default:
		sciprintf( "unknown mem obj type\n" );
		break;
	}
	return 0;	// never get here
}

gint16 sm_get_heap2 (seg_manager_t* self, seg_id_t seg, int offset, mem_obj_enum mem_type ) {
  reg_t reg;
  reg.segment = seg;
  reg.offset = offset;
  return sm_get_heap( self, reg, mem_type );
};

void sm_put_heap (seg_manager_t* self, reg_t reg, gint16 value, mem_obj_enum mem_type ) {
	mem_obj_t* mem_obj;
	VERIFY( sm_check (self, reg.segment), "Invalid seg id" );
	mem_obj = self->heap[reg.segment];
	switch( mem_type ) {
	case MEM_OBJ_SCRIPT:
		VERIFY( reg.offset + 1 < mem_obj->data.script.buf_size, "invalid offset" );
		mem_obj->data.script.buf[reg.offset] = value & 0xff;
		mem_obj->data.script.buf[reg.offset + 1] = value >> 8;
		break;
	case MEM_OBJ_CLONES:
		sciprintf( "memcpy for clones haven't been implemented\n" );
		break;
	default:
		sciprintf( "unknown mem obj type\n" );
		break;
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
	VERIFY ( sm_check (self, id), "invalid seg id" );
	self->heap[id]->data.script.lockers++;
};

void sm_decrement_lockers (seg_manager_t* self, int id, int flag) {
	if (flag == SCRIPT_ID)
		id = sm_seg_get (self, id);
	VERIFY ( sm_check (self, id), "invalid seg id" );
	self->heap[id]->data.script.lockers--;
};

int sm_get_lockers (seg_manager_t* self, int id, int flag) {
	if (flag == SCRIPT_ID)
		id = sm_seg_get (self, id);
	VERIFY ( sm_check (self, id), "invalid seg id" );
	return self->heap[id]->data.script.lockers;
};

void sm_set_lockers (seg_manager_t* self, int lockers, int id, int flag) {
	if (flag == SCRIPT_ID)
		id = sm_seg_get (self, id);
	VERIFY ( sm_check (self, id), "invalid seg id" );
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

void sm_set_variables (struct _seg_manager_t* self, reg_t reg, int obj_index, reg_t variable_reg, int variable_index ) {
	script_t* script;
	VERIFY ( sm_check (self, reg.segment), "invalid seg id" );
	VERIFY ( self->heap[reg.segment], "invalid mem" );
	
	script = &(self->heap[reg.segment]->data.script);
	if( obj_index >= script->objects_nr ) {
		// reallocate the memory for objects
		void* temp = realloc( script->objects, (script->objects_nr * 2) * sizeof(object_t) );
		VERIFY_MEM( temp, );
		script->objects = temp;
		script->objects_nr *= 2;
	}
	VERIFY( obj_index < script->objects_nr, "Invalid obj_index" );
	
	if( script->objects[obj_index].variables_nr == 0 ) {
		// allocate the new memory
		int size = DEFAULT_VARIABLES;	// = maximum ( variable_index + 1, DEFAULT_VARIABLES );
		size = variable_index + 1 <= size ? size : variable_index + 1;
		script->objects[obj_index].variables = (reg_t*) malloc( sizeof(reg_t) * size ); // allocate only one 
		VERIFY_MEM( script->objects[obj_index].variables, );
		script->objects[obj_index].variables_nr = size;
	} else if( script->objects[obj_index].variables_nr <= variable_index ) {
		// reallocate the memory
		int size = script->objects[obj_index].variables_nr;
		reg_t* temp;
		while( size <= variable_index ) size *= 2;
		temp = (reg_t*) realloc( script->objects[obj_index].variables, sizeof(reg_t) * size ); // allocate only one 
		VERIFY_MEM( temp, );
		script->objects[obj_index].variables_nr = size;
		script->objects[obj_index].variables = temp;
	}

	script->objects[obj_index].variables[variable_index] = variable_reg;
};
