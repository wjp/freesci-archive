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

#undef DEBUG_SEG_MANAGER /* Define to turn on debugging */
#define GET_SEGID() 	if (flag == SCRIPT_ID) \
				id = sm_seg_get (self, id); \
			VERIFY ( sm_check (self, id), "invalid seg id" );
#define VERIFY_MEM( mem_ptr, ret ) if (! (mem_ptr) )  {\
					sciprintf( "%s, *d, no enough memory", __FILE__, __LINE__ ); \
					return ret; \
				}
					
#define INVALID_SCRIPT_ID -1
	
void dbg_print( char* msg, int i ) {
#ifdef DEBUG_SEG_MANAGER
	char buf[1000];
	sprintf( buf, "%s = [0x%x], dec:[%d]", msg, i, i);
	perror( buf );
#endif
};

/*--------------------------*/
/*-- forward declarations --*/
/*--------------------------*/
static object_t *
sm_script_obj_init(seg_manager_t *self, reg_t obj_pos);

static void
sm_script_relocate(seg_manager_t *self, reg_t block);

static void
sm_script_initialize_locals_zero(seg_manager_t *self, seg_id_t seg, int count);

static void
sm_script_initialize_locals(seg_manager_t *self, reg_t location);

static byte *
sm_get_synonyms(seg_manager_t *seg_manager, int id, int flag);

static void
sm_script_free_unused_objects(seg_manager_t *self, seg_id_t seg);

static dstack_t *
sm_allocate_stack(seg_manager_t *self, int size, seg_id_t *segid);

static sys_strings_t *
sm_allocate_sys_strings(seg_manager_t *self, seg_id_t *segid);

static void
sm_free_script ( mem_obj_t* mem );

static int
_sm_deallocate (seg_manager_t* self, int seg);

static byte*
sm_dereference(seg_manager_t *self, reg_t ref, int *size);

static clone_t *sm_alloc_clone(seg_manager_t *self, reg_t *addr);
static list_t *sm_alloc_list(seg_manager_t *self, reg_t *addr);
static node_t *sm_alloc_node(seg_manager_t *self, reg_t *addr);
static hunk_t *sm_real_alloc_hunk(seg_manager_t *self, char *, int size, reg_t *addr);
static hunk_t *sm_alloc_hunk(seg_manager_t *self, reg_t *addr);

static void sm_free_clone(seg_manager_t *self, reg_t addr);
static void sm_free_list(seg_manager_t *self, reg_t addr);
static void sm_free_node(seg_manager_t *self, reg_t addr);
static void sm_free_hunk(seg_manager_t *self, reg_t addr);


/***--------------------------***/
/** end of forward declarations */
/***--------------------------***/


static inline int
find_free_id(seg_manager_t *self, int *id)
{
	char was_added = 0;
	int retval;

	while (!was_added) {
		retval = int_hash_map_check_value(self->id_seg_map, self->reserved_id,
						  1, &was_added);
		*id = self->reserved_id--;
		if (self->reserved_id < -1000000)
			self->reserved_id = -10;
		/* Make sure we don't underflow */
	}

	return retval;
}

static mem_obj_t *
alloc_nonscript_segment(seg_manager_t *self, mem_obj_enum type, seg_id_t *segid)
{ /* Allocates a non-script segment */
	int id;
	*segid = find_free_id(self, &id);
	return mem_obj_allocate(self, *segid, id, type);
}


void sm_init(seg_manager_t* self) {
	int i;

	self->id_seg_map = new_int_hash_map();
	self->reserved_id = INVALID_SCRIPT_ID;
	int_hash_map_check_value (self->id_seg_map, self->reserved_id, 1, NULL);	/* reserve 0 for seg_id */
	self->reserved_id--;	/* reserved_id runs in the reversed direction to make sure no one will use it. */
	
	self->heap_size = DEFAULT_SCRIPTS;
        self->heap = (mem_obj_t**) sci_calloc (self->heap_size, sizeof(mem_obj_t *));

	self->clones_seg_id = 0;
	self->lists_seg_id = 0;
	self->nodes_seg_id = 0;
	self->hunks_seg_id = 0;

	/*  initialize the heap pointers*/
	for (i = 0; i < self->heap_size; i++) {
		self->heap[i] = NULL;
	}

	/* function assignments */
	self->destroy = sm_destroy;

	self->seg_get = sm_seg_get;

	self->allocate_script = sm_allocate_script;
	self->deallocate_script = sm_deallocate_script;
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

	self->allocate_stack = sm_allocate_stack;
	self->allocate_sys_strings = sm_allocate_sys_strings;

	self->get_heappos = sm_get_heappos;

	self->set_export_table_offset = sm_set_export_table_offset;
	self->get_export_table_offset = sm_get_export_table_offset;

	self->set_synonyms_offset = sm_set_synonyms_offset;
	self->get_synonyms = sm_get_synonyms;

	self->set_synonyms_nr = sm_set_synonyms_nr;
	self->get_synonyms_nr = sm_get_synonyms_nr;

	self->validate_export_func = sm_validate_export_func;

	self->set_variables = sm_set_variables;
	self->script_obj_init = sm_script_obj_init;
	self->script_relocate = sm_script_relocate;
	self->script_free_unused_objects = sm_script_free_unused_objects;

	self->script_initialize_locals_zero = sm_script_initialize_locals_zero;
	self->script_initialize_locals = sm_script_initialize_locals;

	self->dereference = sm_dereference;

	self->alloc_clone = sm_alloc_clone;
	self->alloc_list = sm_alloc_list;
	self->alloc_node = sm_alloc_node;
	self->alloc_hunk = sm_real_alloc_hunk;

	self->free_clone = sm_free_clone;
	self->free_list = sm_free_list;
	self->free_node = sm_free_node;
	self->free_hunk = sm_free_hunk;
};

/* destroy the object, free the memorys if allocated before */
void sm_destroy (seg_manager_t* self) {
	int i;
	/* free memory*/
	for (i = 0; i < self->heap_size; i++) {
		if (self->heap[i])
			_sm_deallocate(self, i);
	}
	sci_free (self->heap);
	self->heap = NULL;
};

/* allocate a memory for script from heap
** Parameters: (state_t *) s: The state to operate on
**             (int) script_nr: The script number to load
** Returns   : 0 - allocate failure
**             1 - allocate successfully
**             seg_id - allocated segment id
*/
int sm_allocate_script (seg_manager_t* self, struct _state *s, int script_nr, int* seg_id) {
	int i;
	int seg;
	char was_added;
	mem_obj_t* mem;
	resource_t *script;
	void* temp;
	script_t *scr;

	seg = int_hash_map_check_value (self->id_seg_map, script_nr, 1, &was_added);
	if (!was_added) {
		*seg_id = seg;
		return 1;
	}

	/* allocate the mem_obj_t */
	mem = mem_obj_allocate(self, seg, script_nr, MEM_OBJ_SCRIPT);
	if (!mem) {
		sciprintf("%s, %d, Not enough memory, ", __FILE__, __LINE__ );
		return 0;
	}

	/* allocate the script.buf */
	script = scir_find_resource(s->resmgr, sci_script, script_nr, 0);
	if (s->version < SCI_VERSION_FTU_NEW_SCRIPT_HEADER) {
		mem->data.script.buf_size = script->size + getUInt16(script->data)*2; 
		/* locals_size = getUInt16(script->data)*2; */
	}
	else {
		mem->data.script.buf_size = script->size;
	}
	mem->data.script.buf = (char*) sci_malloc (mem->data.script.buf_size);
	dbg_print( "mem->data.script.buf ", (int) mem->data.script.buf );
	if (!mem->data.script.buf) {
		sm_free_script ( mem );
		sciprintf("seg_manager.c: Not enough memory space for script size" );
		mem->data.script.buf_size = 0;
		return 0;
	}
	
	/* Initialize objects */
	scr = &(mem->data.script);
	scr->objects = NULL;
	scr->objects_allocated = 0;
	scr->objects_nr = 0; /* No objects recorded yet */

	scr->locals_offset = 0;
	scr->locals_block = NULL;

	scr->nr = script_nr;

	*seg_id = seg;
	return 1;
};

static int
_sm_deallocate (seg_manager_t* self, int seg)
{
	mem_obj_t *mobj;
	VERIFY ( sm_check (self, seg), "invalid seg id" );

	mobj = self->heap[seg];
	int_hash_map_remove_value( self->id_seg_map, mobj->segmgr_id );

	switch (mobj->type) {

	case MEM_OBJ_SCRIPT:
		sm_free_script ( mobj );

		mobj->data.script.buf = NULL;
		if (mobj->data.script.locals_segment)
			_sm_deallocate(self, mobj->data.script.locals_segment);
		break;

	case MEM_OBJ_LOCALS:
		sci_free(mobj->data.locals.locals);
		mobj->data.locals.locals = NULL;
		break;

	default:
		fprintf(stderr, "Deallocating segment type %d not supported!\n",
			mobj->type);
		BREAKPOINT();
	}

	free(mobj);
	self->heap[seg] = NULL;
}

int sm_deallocate_script (seg_manager_t* self, struct _state *s, int script_nr) {
	int seg = sm_seg_get( self, script_nr );

	_sm_deallocate(self, seg);
	return 1;
};

void sm_update (seg_manager_t* self) {
};

mem_obj_t*
mem_obj_allocate(seg_manager_t *self, seg_id_t segid, int hash_id, mem_obj_enum type)
{
	mem_obj_t* mem = ( mem_obj_t* ) sci_calloc( sizeof (mem_obj_t), 1 );
	if( !mem ) {
		sciprintf( "seg_manager.c: invalid mem_obj " );
		return NULL;
	}

	if (segid >= self->heap_size) {
		void *temp;
		int i, oldhs = self->heap_size;

		if (segid >= self->heap_size * 2) {
			sciprintf( "seg_manager.c: hash_map error or others??" );
			return NULL;
		}
		self->heap_size *= 2;
		temp = sci_realloc ((void*)self->heap, self->heap_size * sizeof( mem_obj_t* ) );
		if (!temp) {
			sciprintf("seg_manager.c: Not enough memory space for script size" );
			return NULL;
		}
		self->heap = (mem_obj_t**)  temp;

		/* Clear pointers */
		memset(self->heap + oldhs, 0, sizeof(mem_obj_t *) * (self->heap_size - oldhs));
	}

	mem->segmgr_id = hash_id;
	mem->type = type;

	/* hook it to the heap */
	self->heap[segid] = mem;
	return mem;
};

void sm_object_init (object_t* object) {
	if( !object )	return;
	object->variables_nr = 0;
	object->variables = NULL;
};

static void sm_free_script ( mem_obj_t* mem ) {
	if( !mem ) return;
	if( mem->data.script.buf ) {
		sci_free( mem->data.script.buf );
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
};

/* memory operations */
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
		VERIFY( reg.offset + 1 < mem_obj->data.script.buf_size, "invalid offset\n" );
		return (unsigned char)mem_obj->data.script.buf[reg.offset] |
		     ( ((unsigned char)mem_obj->data.script.buf[reg.offset+1]) << 8 );
	case MEM_OBJ_CLONES:
		sciprintf( "memcpy for clones haven't been implemented\n" );
		break;
	default:
		sciprintf( "unknown mem obj type\n" );
		break;
	}
	return 0;	/* never get here */
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

/* return the seg if script_id is valid and in the map, else -1 */
int sm_seg_get (seg_manager_t* self, int script_id) {
	return int_hash_map_check_value (self->id_seg_map, script_id, 0, NULL);
};

/* validate the seg
** return:
**	0 - invalid seg
**	1 - valid seg
*/
int sm_check (seg_manager_t* self, int seg) {
	if ( seg < 0 || seg >= self->heap_size ) {
		return 0;
	}
	if (!self->heap[seg]) {
		sciprintf("seg_manager.c: seg %x is removed from memory, but not removed from hash_map\n", seg );
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

void sm_set_export_table_offset (struct _seg_manager_t* self, int offset, int magic_offset,
				 int id, int flag)
{
	script_t *scr = &(self->heap[id]->data.script);
	int i;

	GET_SEGID();
	if (offset) {
		scr->export_table = (guint16 *)(scr->buf + offset + 2);
		scr->exports_nr = getUInt16((byte *)(scr->export_table - 1));
		if (magic_offset) {
			fprintf(stderr, "Adjusting for magic offset!!\n");
			for (i = 0; i < scr->exports_nr; i++) {
				int val = getUInt16((byte *) (scr->export_table + i));
				val += magic_offset;
				putInt16((byte *) (scr->export_table + i), val);
			}
		}
	} else {
		scr->export_table = NULL;
		scr->exports_nr = 0;
	}
};

guint16 *sm_get_export_table_offset (struct _seg_manager_t* self, int id, int flag, int *max)
{
	GET_SEGID();
	if (max)
		*max = self->heap[id]->data.script.exports_nr;
	return self->heap[id]->data.script.export_table;
};

void sm_set_synonyms_offset (struct _seg_manager_t* self, int offset, int id, int flag) {
	GET_SEGID();
	self->heap[id]->data.script.synonyms = 
		self->heap[id]->data.script.buf + offset;
};

static byte *
sm_get_synonyms(seg_manager_t *self, int id, int flag)
{
	GET_SEGID();
	return self->heap[id]->data.script.synonyms;
};

void sm_set_synonyms_nr (struct _seg_manager_t* self, int nr, int id, int flag) {
	GET_SEGID();
	self->heap[id]->data.script.synonyms_nr = nr;
};

int sm_get_synonyms_nr (struct _seg_manager_t* self, int id, int flag) {
	GET_SEGID();
	return self->heap[id]->data.script.synonyms_nr;
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

	VERIFY( obj_index < script->objects_nr, "Invalid obj_index" );

	VERIFY( variable_index >= 0
		&& variable_index < script->objects[obj_index].variables_nr,
		"Attempt to write to invalid variable number" );

	script->objects[obj_index].variables[variable_index] = variable_reg;
};


static inline int
_relocate_block(reg_t *block, int block_location, int block_items, seg_id_t segment, int location)
{
	int rel = location - block_location;
	int index;

	if (rel < 0)
		return 0;

	index = rel >> 1;

	if (index >= block_items)
		return 0;

	if (rel & 1) {
		sciprintf("Error: Attempt to relocate odd variable #%d.5e (relative to %04x)\n",
			  index, block_location);
		return 0;
	}
	block[index].segment = segment; /* Perform relocation */

	return 1;
}

static inline int
_relocate_local(script_t *scr, seg_id_t segment, int location)
{
	if (scr->locals_block)
		return _relocate_block(scr->locals_block->locals, scr->locals_offset,
				       scr->locals_block->nr,
				       segment, location);
	else
		return 0; /* No hands, no cookies */
}

static inline int
_relocate_object(object_t *obj, seg_id_t segment, int location)
{
	return _relocate_block(obj->variables, obj->pos.offset, obj->variables_nr,
			       segment, location);
}

static void
sm_script_relocate(seg_manager_t *self, reg_t block)
{
	mem_obj_t *mobj = self->heap[block.segment];
	script_t *scr;
	int count;
	int i;

	VERIFY( !(block.segment >= self->heap_size || mobj->type != MEM_OBJ_SCRIPT),
		"Attempt relocate non-script\n" );

	scr = &(mobj->data.script);

	VERIFY( block.offset < scr->buf_size
		&& getUInt16(scr->buf + block.offset)*2 + block.offset < scr->buf_size,
		"Relocation block outside of script\n" );

	count = getUInt16(scr->buf + block.offset);

	for (i = 0; i < count; i++) {
		int pos = getUInt16(scr->buf + block.offset + 2 + (i*2));

		if (!_relocate_local(scr, block.segment, pos)) {
			int k, done = 0;

			for (k = 0; !done && k < scr->objects_nr; k++) {
				if (_relocate_object(scr->objects + k, block.segment, pos))
					done = 1;
			}

			if (!done) {
				sciprintf("While processing relocation block "PREG":\n",
					  PRINT_REG(block));
				sciprintf("Relocation failed for index %04x\n", pos);
				if (scr->locals_block)
					sciprintf("- locals: %d at %04x\n",
						  scr->locals_block->nr,
						  scr->locals_offset);
				else
					sciprintf("- No locals\n");
				for (k = 0; k < scr->objects_nr; k++)
					sciprintf("- obj#%d at %04x w/ %d vars\n",
						  k,
						  scr->objects[k].pos.offset,
						  scr->objects[k].variables_nr);
				sciprintf("Triggering breakpoint...\n");
				BREAKPOINT();
			}
		}
	}
}

static object_t *
sm_script_obj_init(seg_manager_t *self, reg_t obj_pos)
{
	mem_obj_t *mobj = self->heap[obj_pos.segment];
	script_t *scr;
	object_t *obj;
	int id;
	int base = obj_pos.offset - SCRIPT_OBJECT_MAGIC_OFFSET;

	VERIFY( !(obj_pos.segment >= self->heap_size || mobj->type != MEM_OBJ_SCRIPT),
		"Attempt to initialize object in non-script\n" );

	scr = &(mobj->data.script);

	VERIFY( base < scr->buf_size,
		"Attempt to initialize object beyond end of script\n" );

	if (!scr->objects) {
		scr->objects_allocated = DEFAULT_OBJECTS;
		scr->objects = sci_malloc(sizeof(object_t) * scr->objects_allocated);
	}
	if (scr->objects_nr == scr->objects_allocated) {
		scr->objects_allocated += DEFAULT_OBJECTS_INCREMENT;
		scr->objects = sci_realloc(scr->objects,
					   sizeof(object_t)
					   * scr->objects_allocated);
								
	}
	obj = scr->objects + (id = scr->objects_nr++);

	VERIFY( base + SCRIPT_FUNCTAREAPTR_OFFSET  < scr->buf_size,
		"Function area pointer stored beyond end of script\n" );
		
	{
		byte *data = scr->buf + base;
		int funct_area = getUInt16( data + SCRIPT_FUNCTAREAPTR_OFFSET );
		int variables_nr;
		int functions_nr;
		int is_class;
		int i;

		obj->pos = make_reg(obj_pos.segment, base);

		VERIFY ( base + funct_area < scr->buf_size,
			 "Function area pointer references beyond end of script" );

		variables_nr = getUInt16( data + SCRIPT_SELECTORCTR_OFFSET );
		functions_nr = getUInt16( data + funct_area - 2 );
		is_class = getUInt16( data + SCRIPT_INFO_OFFSET ) & SCRIPT_INFO_CLASS;

		/* Store object ID within script */
		data[SCRIPT_LOCALVARPTR_OFFSET] = id & 0xff;
		data[SCRIPT_LOCALVARPTR_OFFSET + 1] = (id >> 8) & 0xff;

		VERIFY ( base + funct_area + functions_nr * 2
			 /* add again for classes, since those also store selectors */
			 + (is_class? functions_nr * 2 : 0) < scr->buf_size,
			 "Function area extends beyond end of script" );

		obj->variables_nr = variables_nr;
		obj->variables = sci_malloc(sizeof(reg_t) * variables_nr);

		obj->methods_nr = functions_nr;
		obj->base = scr->buf;
		obj->base_obj = data;
		obj->base_method = (guint16 *) (data + funct_area);

		for (i = 0; i < variables_nr; i++)
			obj->variables[i] = make_reg(0, getUInt16(data + (i*2)));
	}

	return obj;
}

static local_variables_t *
_sm_alloc_locals_segment(seg_manager_t *self, script_t *scr, int count)
{
	if (!count) { /* No locals */
		scr->locals_segment = 0;
		scr->locals_block = NULL;
		return NULL;
	} else {
		mem_obj_t *mobj = alloc_nonscript_segment(self, MEM_OBJ_LOCALS,
							  &scr->locals_segment);
		local_variables_t *locals = scr->locals_block = &(mobj->data.locals);

		locals->script_id = scr->nr;
		locals->locals = sci_calloc(sizeof(reg_t), count);
		locals->nr = count;

		return locals;
	}
}

static void
sm_script_initialize_locals_zero(seg_manager_t *self, seg_id_t seg, int count)
{
	mem_obj_t *mobj = self->heap[seg];
	script_t *scr;

	VERIFY( !(seg >= self->heap_size || mobj->type != MEM_OBJ_SCRIPT),
		"Attempt to initialize locals in non-script\n" );

	scr = &(mobj->data.script);

	scr->locals_offset = -count * 2; /* Make sure it's invalid */

	_sm_alloc_locals_segment(self, scr, count);
}

static void
sm_script_initialize_locals(seg_manager_t *self, reg_t location)
{
	mem_obj_t *mobj = self->heap[location.segment];
	int count;
	script_t *scr;
	local_variables_t *locals;

	VERIFY( !(location.segment >= self->heap_size || mobj->type != MEM_OBJ_SCRIPT),
		"Attempt to initialize locals in non-script\n" );

	scr = &(mobj->data.script);

	VERIFY( location.offset + 1 < scr->buf_size,
		"Locals beyond end of script\n" );

	count = getUInt16(scr->buf + location.offset - 2) >> 1;
	/* half block size */

	scr->locals_offset = location.offset;

	VERIFY( location.offset + count * 2 + 1 < scr->buf_size,
		"Locals extend beyond end of script\n" );

	locals = _sm_alloc_locals_segment(self, scr, count);
	if (locals) {
		int i;
		byte *base = scr->buf + location.offset;

		for (i = 0; i < count; i++)
			locals->locals[i].offset = getUInt16(base + i*2);
	}
}

static void
sm_script_free_unused_objects(seg_manager_t *self, seg_id_t seg)
{
	mem_obj_t *mobj = self->heap[seg];
	script_t *scr;

	VERIFY( !(seg >= self->heap_size || mobj->type != MEM_OBJ_SCRIPT),
		"Attempt to free unused objects in non-script\n" );


	scr = &(mobj->data.script);
	if (scr->objects_allocated > scr->objects_nr) {
		if (scr->objects_nr)
			scr->objects = sci_realloc(scr->objects, sizeof(object_t)
						   * scr->objects_nr);
		else {
			if (scr->objects_allocated)
				sci_free(scr->objects);
			scr->objects = NULL;
		}
		scr->objects_allocated = scr->objects_nr;
	}
}

static inline char *dynprintf(char *msg, ...)
{
	va_list argp;
	char *buf = malloc(strlen(msg) + 100);

	va_start(argp, msg);
	vsprintf(buf, msg, argp);
	va_end(argp);

	return buf;
}


static dstack_t *
sm_allocate_stack(seg_manager_t *self, int size, seg_id_t *segid)
{
	mem_obj_t *memobj = alloc_nonscript_segment(self, MEM_OBJ_STACK, segid);
	dstack_t *retval = &(memobj->data.stack);

	retval->entries = sci_calloc(size, sizeof(reg_t));
	retval->nr = size;

	return retval;
}

static sys_strings_t *
sm_allocate_sys_strings(seg_manager_t *self, seg_id_t *segid)
{
	mem_obj_t *memobj = alloc_nonscript_segment(self, MEM_OBJ_SYS_STRINGS, segid);
	sys_strings_t *retval = &(memobj->data.sys_strings);
	int i;

	for (i = 0; i < SYS_STRINGS_MAX; i++)
		retval->strings[i].name = NULL; /* Not reserved */

	return retval;
}

guint16 sm_validate_export_func(struct _seg_manager_t* self, int pubfunct, int seg ) {
	script_t* script; 
	guint16 offset;
	VERIFY ( sm_check (self, seg), "invalid seg id" );
	script = &self->heap[seg]->data.script;
	if( script->exports_nr <= pubfunct ) {
		sciprintf( "pubfunct is invalid" );
		return 0;
	}
	offset = getUInt16( (byte*)(script->export_table + pubfunct) ); 
	VERIFY ( offset < script->buf_size, "invalid export function pointer" );

	return offset;
};

static hunk_t *
sm_real_alloc_hunk(seg_manager_t *self, char *hunk_type, int size, reg_t *reg)
{
	hunk_t *h = sm_alloc_hunk(self, reg);

	if (!h)
		return NULL;

	h->mem = sci_malloc(size);
	h->size = size;
	h->type = hunk_type;

	return h;
}

static void
_clone_cleanup(clone_t *clone)
{
	if (clone->variables)
		sci_free(clone->variables); /* Free the dynamically allocated memory part */
}

static void
_hunk_cleanup(hunk_t *hunk)
{
	if (hunk->mem)
		free (hunk->mem);
}

DEFINE_HEAPENTRY(list, 8, 4);
DEFINE_HEAPENTRY(node, 32, 16);
DEFINE_HEAPENTRY_WITH_CLEANUP(clone, 16, 4, _clone_cleanup);
DEFINE_HEAPENTRY_WITH_CLEANUP(hunk, 4, 4, _hunk_cleanup);

#define DEFINE_ALLOC_DEALLOC(TYPE, SEGTYPE) \
static TYPE##_t *										  \
sm_alloc_##TYPE(seg_manager_t *self, reg_t *addr)						  \
{												  \
	mem_obj_t *mobj;									  \
	TYPE##_table_t *table;									  \
	int offset;										  \
												  \
	if (!self->TYPE##s_seg_id) {								  \
		mobj = alloc_nonscript_segment(self, SEGTYPE, &(self->TYPE##s_seg_id));		  \
		init_##TYPE##_table(&(mobj->data.##TYPE##s));					  \
	} else											  \
		mobj = self->heap[self->TYPE##s_seg_id];					  \
												  \
	table = &(mobj->data.##TYPE##s);							  \
	offset = alloc_##TYPE##_entry(table);							  \
												  \
	*addr = make_reg(self->TYPE##s_seg_id, offset);						  \
	return &(mobj->data.##TYPE##s.table[offset].entry);					  \
}												  \
												  \
static void											  \
sm_free_##TYPE(seg_manager_t *self, reg_t addr)							  \
{												  \
	mem_obj_t *mobj = GET_SEGMENT(*self, addr.segment, SEGTYPE);				  \
												  \
	if (!mobj) {										  \
		sciprintf("Attempt to free " #TYPE " from address "PREG				  \
			  ": Invalid segment type\n",						  \
			  PRINT_REG(addr));							  \
		return;										  \
	}											  \
												  \
	free_##TYPE##_entry(&(mobj->data.##TYPE##s), addr.offset);				  \
}

DEFINE_ALLOC_DEALLOC(clone, MEM_OBJ_CLONES);
DEFINE_ALLOC_DEALLOC(list, MEM_OBJ_LISTS);
DEFINE_ALLOC_DEALLOC(node, MEM_OBJ_NODES);
DEFINE_ALLOC_DEALLOC(hunk, MEM_OBJ_HUNK);



static byte *
sm_dereference(seg_manager_t *self, reg_t reg, int *size)
{
	mem_obj_t *mobj = self->heap[reg.segment];
	VERIFY( sm_check (self, reg.segment), "Invalid seg id" );

	switch (mobj->type) {

	case MEM_OBJ_SCRIPT:
		if (size)
			*size = mobj->data.script.buf_size - reg.offset;
		return mobj->data.script.buf + reg.offset;
		break;

	case MEM_OBJ_LOCALS: /* Emulate for strings */
		if (size)
			*size = (mobj->data.locals.nr - reg.offset) * 2;
		return (byte *)(mobj->data.locals.locals + reg.offset);
		break;

	case MEM_OBJ_STACK: /* Emulate for strings */
		if (size)
			*size = (mobj->data.stack.nr - reg.offset) * 2;
		return (byte *)(mobj->data.stack.entries + reg.offset);
		break;

	case MEM_OBJ_SYS_STRINGS:
		if (size)
			*size = mobj->data.sys_strings.strings[reg.offset].max_size;
		return mobj->data.sys_strings.strings[reg.offset].value;
		break;

	default:
		if (size)
			*size = 0;
		return NULL;
	}
}
