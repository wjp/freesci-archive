/***************************************************************************
 klists.c Copyright (C) 1999 Christoph Reichenbach


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

    Christoph Reichenbach (CJR) [jameson@linuxgames.com]

***************************************************************************/

#include <engine.h>


#define LOOKUP_NODE(addr) lookup_node(s, (addr), __FILE__, __LINE__)

inline node_t *
lookup_node(state_t *s, reg_t addr, char *file, int line)
{
	mem_obj_t *mobj;
	node_table_t *nt;

	if (!addr.offset && !addr.segment)
		return NULL; /* Non-error null */

	mobj = GET_SEGMENT(s->seg_manager, addr.segment, MEM_OBJ_NODES);
	if (!mobj) {
		sciprintf("%s, L%d: Attempt to use non-node "PREG" as list node\n",
			  __FILE__, __LINE__, PRINT_REG(addr));
		script_debug_flag = script_error_flag = 1;
		return NULL;
	}

	nt = &(mobj->data.nodes);

	if (!ENTRY_IS_VALID(nt, addr.offset)) {
		sciprintf("%s, L%d: Attempt to use non-node "PREG" as list node\n",
			  __FILE__, __LINE__, PRINT_REG(addr));
		script_debug_flag = script_error_flag = 1;
		return NULL;
	}

	return &(nt->table[addr.offset].entry);
}

#define LOOKUP_LIST(addr) _lookup_list(s, addr, __FILE__, __LINE__, 0)
#define LOOKUP_NULL_LIST(addr) _lookup_list(s, addr, __FILE__, __LINE__, 1)


inline list_t *
_lookup_list(state_t *s, reg_t addr, char *file, int line, int may_be_null)
{
	mem_obj_t *mobj;
	list_table_t *lt;

	if (may_be_null && !addr.segment && !addr.offset)
		return NULL;

	mobj = GET_SEGMENT(s->seg_manager, addr.segment, MEM_OBJ_LISTS);

	if (!mobj) {
		sciprintf("%s, L%d: Attempt to use non-list "PREG" as list\n",
			  __FILE__, __LINE__, PRINT_REG(addr));
		script_debug_flag = script_error_flag = 1;
		return NULL;
	}

	lt = &(mobj->data.lists);

	if (!ENTRY_IS_VALID(lt, addr.offset)) {
		sciprintf("%s, L%d: Attempt to use non-list "PREG" as list\n",
			  __FILE__, __LINE__, PRINT_REG(addr));
		script_debug_flag = script_error_flag = 1;
		return NULL;
	}

	return &(lt->table[addr.offset].entry);
}

list_t *
lookup_list(state_t *s, reg_t addr, char *file, int line)
{
	return _lookup_list(s, addr, file, line, 0);
}

int
listp(state_t *s, reg_t addr)
{
	return (s->seg_manager.heap[addr.segment]->type == MEM_OBJ_LISTS
		&& ENTRY_IS_VALID(&(s->seg_manager.heap[addr.segment]->data.lists), addr.offset));
}

#ifdef DISABLE_VALIDATIONS

#define sane_nodep(a, b) 1
#define sane_listp(a, b) 1

#else

static inline int
sane_nodep(state_t *s, reg_t addr)
{
	int have_prev = 0;
	reg_t prev;

	do {
		node_t *node = LOOKUP_NODE(addr);

		if (!node)
			return 0;

		if ((have_prev)
		    && !REG_EQ(node->pred, prev))
			return 0;

		prev = addr;
		addr = node->succ;
		have_prev = 1;

	} while (!IS_NULL_REG(addr));

	return 1;
}


int
sane_listp(state_t *s, reg_t addr)
{
	list_t *l = LOOKUP_LIST(addr);
	int empties = 0;

	if (IS_NULL_REG(l->first))
		++empties;
	if (IS_NULL_REG(l->last))
		++empties;

	/* Either none or both are set */

	if (empties == 1)
		return 1;

	if (!empties) {
		node_t *node_a, *node_z;

		node_a = LOOKUP_NODE(l->first);
		node_z = LOOKUP_NODE(l->last);

		if (!node_a || !node_z)
			return 0;

		if (!IS_NULL_REG(node_a->pred))
			return 0;

		if (!IS_NULL_REG(node_z->succ))
			return 0;

		return sane_nodep(s, l->first);
	}

	return 1; /* Empty list is fine */
}
#endif


reg_t
kNewList(state_t *s, int funct_nr, int argc, reg_t *argv)
{
	reg_t listbase;
	list_t *l;
	l = s->seg_manager.alloc_list(&s->seg_manager, &listbase);
	l->first = l->last = NULL_REG;
	SCIkdebug(SCIkNODES, "New listbase at "PREG"\n", PRINT_REG(listbase));

	return listbase; /* Return list base address */
}

reg_t
kDisposeList(state_t *s, int funct_nr, int argc, reg_t *argv)
{
	list_t *l = LOOKUP_LIST(argv[0]);

	if (!l) {
		SCIkwarn(SCIkERROR, "Attempt to dispose non-list at "PREG"!\n",
			 PRINT_REG(argv[0]));
		return NULL_REG;
	}

	if (!sane_listp(s, argv[0]))
		SCIkwarn(SCIkERROR,"List at "PREG" is not sane anymore!\n", PRINT_REG(argv[0]));

	if (!IS_NULL_REG(l->first)) {
		reg_t n_addr = l->first;

		while (!IS_NULL_REG(n_addr)) { /* Free all nodes */
			node_t *n = LOOKUP_NODE(n_addr);
			s->seg_manager.free_node(&s->seg_manager, n_addr);
			n_addr = n->succ;
		} 
	}

	s->seg_manager.free_list(&s->seg_manager, argv[0]);
}


inline reg_t
_k_new_node(state_t *s, reg_t value, reg_t key)
{
	reg_t nodebase;
	node_t *n = s->seg_manager.alloc_node(&s->seg_manager, &nodebase);

	if (!n) {
		KERNEL_OOPS("Out of memory while creating a node");
		return;
	}

	n->pred = n->succ = NULL_REG;
	n->key = key;
	n->value = value;

	return nodebase;
}

reg_t
kNewNode(state_t *s, int funct_nr, int argc, reg_t *argv)
{
	s->r_acc = _k_new_node(s, argv[0], argv[1]);

	SCIkdebug(SCIkNODES, "New nodebase at "PREG"\n", PRINT_REG(s->r_acc));

	return s->r_acc;
}



reg_t
kFirstNode(state_t *s, int funct_nr, int argc, reg_t *argv)
{
	list_t *l = LOOKUP_NULL_LIST(argv[0]);

	if (l && !sane_listp(s, argv[0]))
		SCIkwarn(SCIkERROR,"List at "PREG" is not sane anymore!\n",
			 PRINT_REG(argv[0]));

	if (l)
		return l->first;
	else
		return NULL_REG;
}


reg_t
kLastNode(state_t *s, int funct_nr, int argc, reg_t *argv)
{
	list_t *l = LOOKUP_LIST(argv[0]);

	if (l&&!sane_listp(s, argv[0]))
		SCIkwarn(SCIkERROR,"List at "PREG" is not sane anymore!\n",
			 PRINT_REG(argv[0]));

	if (l)
		return l->last;
	else
		return NULL_REG;
}


reg_t
kEmptyList(state_t *s, int funct_nr, int argc, reg_t *argv)
{
	list_t *l = LOOKUP_LIST(argv[0]);

	if (!l || !sane_listp(s, argv[0]))
		SCIkwarn(SCIkERROR,"List at "PREG" is invalid or not sane anymore!\n",
			 PRINT_REG(argv[0]));

	return make_reg(0, ((l)? IS_NULL_REG(l->first) : 0));
}

inline void
_k_add_to_front(state_t *s, reg_t listbase, reg_t nodebase)
{
	list_t *l = LOOKUP_LIST(listbase);
	node_t *new_n = LOOKUP_NODE(nodebase);

	SCIkdebug(SCIkNODES, "Adding node "PREG" to end of list "PREG"\n",
		  PRINT_REG(nodebase), PRINT_REG(listbase));

	if (!new_n)
		SCIkwarn(SCIkERROR, "Attempt to add non-node ("PREG") to list at "PREG"\n",
			 PRINT_REG(nodebase), PRINT_REG(listbase));
	if (!l || !sane_listp(s, listbase))
		SCIkwarn(SCIkERROR,"List at "PREG" is not sane anymore!\n", PRINT_REG(listbase));

	new_n->succ = l->first;
	new_n->pred = NULL_REG;
	/* Set node to be the first and last node if it's the only node of the list */
	if (IS_NULL_REG(l->first))
		l->last = nodebase;
	else {
		node_t *old_n = LOOKUP_NODE(l->first);
		old_n->pred = nodebase;
	}
	l->first = nodebase;
}

inline void
_k_add_to_end(state_t *s, reg_t listbase, reg_t nodebase)
{
	list_t *l = LOOKUP_LIST(listbase);
	node_t *new_n = LOOKUP_NODE(nodebase);

	SCIkdebug(SCIkNODES, "Adding node "PREG" to end of list "PREG"\n",
		  PRINT_REG(nodebase), PRINT_REG(listbase));

	if (!new_n)
		SCIkwarn(SCIkERROR, "Attempt to add non-node ("PREG") to list at "PREG"\n",
			 PRINT_REG(nodebase), PRINT_REG(listbase));
	if (!l || !sane_listp(s, listbase))
		SCIkwarn(SCIkERROR,"List at "PREG" is not sane anymore!\n", PRINT_REG(listbase));

	new_n->succ = NULL_REG;
	new_n->pred = l->last;
	/* Set node to be the first and last node if it's the only node of the list */
	if (IS_NULL_REG(l->last))
		l->first = nodebase;
	else {
		node_t *old_n = LOOKUP_NODE(l->last);
		old_n->succ = nodebase;
	}
	l->last = nodebase;
}


reg_t
kNextNode(state_t *s, int funct_nr, int argc, reg_t *argv)
{
	node_t *n = LOOKUP_NODE(argv[0]);
	if (!sane_nodep(s, argv[0]))
		SCIkwarn(SCIkERROR,"List node at "PREG" is not sane anymore!\n",
			 PRINT_REG(argv[0]));

	return n->succ;
}

reg_t
kPrevNode(state_t *s, int funct_nr, int argc, reg_t *argv)
{
	node_t *n = LOOKUP_NODE(argv[0]);
	if (!sane_nodep(s, argv[0]))
		SCIkwarn(SCIkERROR,"List node at "PREG" is not sane anymore!\n",
			 PRINT_REG(argv[0]));

	return n->pred;
}


reg_t
kNodeValue(state_t *s, int funct_nr, int argc, reg_t *argv)
{
	node_t *n = LOOKUP_NODE(argv[0]);
	if (!sane_nodep(s, argv[0]))
		SCIkwarn(SCIkERROR,"List node at "PREG" is not sane!\n",
			 PRINT_REG(argv[0]));

	return n->value;
}


reg_t
kAddToFront(state_t *s, int funct_nr, int argc, reg_t *argv)
{
	_k_add_to_front(s, argv[0], argv[1]);
	return s->r_acc;
}

reg_t
kAddAfter(state_t *s, int funct_nr, int argc, reg_t *argv)
{
	list_t *l = LOOKUP_LIST(argv[0]);
	node_t *firstnode = IS_NULL_REG(argv[1])? NULL : LOOKUP_NODE(argv[1]);
	node_t *newnode = LOOKUP_NODE(argv[2]);

	if (!l || !sane_listp(s, argv[0]))
		SCIkwarn(SCIkERROR,"List at "PREG" is not sane anymore!\n",
			 PRINT_REG(argv[0]));

	if (!newnode) {
		SCIkwarn(SCIkERROR,"New 'node' "PREG" is not a node!\n",
			 argv[1], argv[2]);
		return NULL_REG;
	}

	if (argc != 3) {
		SCIkdebug(SCIkWARNING, "Aborting.\n");
		return NULL_REG;
	}

	if (firstnode) { /* We're really appending after */
		reg_t oldnext = firstnode->succ;

		newnode->pred = argv[1];
		firstnode->succ = argv[2];
		newnode->succ = oldnext;

		if (IS_NULL_REG(oldnext))  /* Appended after last node? */
			/* Set new node as last list node */
			l->last = argv[2];
		else
			LOOKUP_NODE(oldnext)->pred = argv[2];

		return s->r_acc;

	} else { /* !firstnode */
		/* Prepare call to AddToFront... */
		argv[1] = argv[0];
		return kAddToFront(s, funct_nr, 2, argv + 1);/* Set as initial list node */
	}
}


reg_t
kAddToEnd(state_t *s, int funct_nr, int argc, reg_t *argv)
{
	_k_add_to_end(s, argv[0], argv[1]);
	return s->r_acc;
}


reg_t
kFindKey(state_t *s, int funct_nr, int argc, reg_t *argv)
{
	reg_t node_pos;
	reg_t key = argv[1];
	reg_t list_pos = argv[0];

	SCIkdebug(SCIkNODES, "Looking for key "PREG" in list "PREG"\n",
		  PRINT_REG(key), PRINT_REG(list_pos));

	if (!sane_listp(s, list_pos))
		SCIkwarn(SCIkERROR,"List at "PREG" is not sane anymore!\n",
			 PRINT_REG(list_pos));

	node_pos = LOOKUP_LIST(list_pos)->first;

	SCIkdebug(SCIkNODES, "First node at "PREG"\n", PRINT_REG(node_pos));

	while (!IS_NULL_REG(node_pos)) {
		node_t *n = LOOKUP_NODE(node_pos);
		if (REG_EQ(n->key, key)) {
			SCIkdebug(SCIkNODES, " Found key at "PREG"\n", PRINT_REG(node_pos));
			return node_pos;
		}

		node_pos = n->succ;
		SCIkdebug(SCIkNODES, "NextNode at "PREG"\n", PRINT_REG(node_pos));
	}

	SCIkdebug(SCIkNODES, "Looking for key without success\n");
	return NULL_REG;
}


reg_t
kDeleteKey(state_t *s, int funct_nr, int argc, reg_t *argv)
{
	reg_t node_pos = kFindKey(s, funct_nr, 2, argv);
	node_t *n;
	list_t *l = LOOKUP_LIST(argv[0]);

	if (IS_NULL_REG(node_pos))
		return NULL_REG; /* Signal falure */

	n = LOOKUP_NODE(node_pos);
	if (REG_EQ(l->first, node_pos))
		l->first = n->succ;
	if (REG_EQ(l->last, node_pos))
		l->last = n->pred;

	if (!IS_NULL_REG(n->pred))
		LOOKUP_NODE(n->pred)->succ = n->succ;
	if (!IS_NULL_REG(n->succ))
		LOOKUP_NODE(n->succ)->pred = n->pred;

	s->seg_manager.free_node(&s->seg_manager, node_pos);

	return make_reg(0, 1); /* Signal success */
}




typedef struct
{
	int key, value;
	int order;
} sort_temp_t;

void
kSort(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
#ifdef __GNUC__
#warning "Fix kSort()"
#endif
#if 0
	heap_ptr source = UPARAM(0);
	heap_ptr dest = UPARAM(1);
	heap_ptr order_func = UPARAM(2);

	int input_size = UGET_SELECTOR(source, size);
	heap_ptr input_data = UGET_SELECTOR(source, elements);
	heap_ptr output_data = UGET_SELECTOR(dest, elements);

	heap_ptr node;
	int i = 0, j, ordval;
	sort_temp_t *temparray = (sort_temp_t *) 
		malloc(sizeof(sort_temp_t)*input_size);

	if (input_size)
		return;

	PUT_SELECTOR(dest, size, input_size);

	node = UGET_HEAP(UPARAM(0) + LIST_FIRST_NODE);

	while (node)
	{
		ordval=invoke_selector(INV_SEL(order_func, doit, 0), 1, UGET_HEAP(node + LIST_NODE_VALUE));

		j=i-1;
		while (j>0)
		{
			if (ordval>temparray[j].order)
				break;
			j--;
		}
		temparray[j+1].key=UGET_HEAP(node + LIST_NODE_KEY);
		temparray[j+1].value=UGET_HEAP(node + LIST_NODE_VALUE);
		temparray[j+1].order=ordval;
		i++;
		node=UGET_HEAP(node + LIST_NEXT_NODE);
	}

	for (i=0;i<input_size;i++)
	{
		node = _k_new_node(s, temparray[i].key,
				   temparray[i].value);
		_k_add_to_end(s, output_data, node);
	}
#endif
}
