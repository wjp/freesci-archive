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


static inline int
listp(state_t *s, heap_ptr address)
{
	int size = UGET_HEAP(address);
	return (size == 6 || size == 8);
}



void
kNewList(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
	heap_ptr listbase = heap_allocate(s->_heap, 4);

	if (!listbase) {
		KERNEL_OOPS("Out of memory while creating a list");
		return;
	}

	listbase += 2; /* Jump over heap header */

	PUT_HEAP(listbase + LIST_FIRST_NODE, 0); /* No first node */
	PUT_HEAP(listbase + LIST_LAST_NODE, 0); /* No last node */

	SCIkdebug(SCIkNODES, "New listbase at %04x\n", listbase);

	s->acc = listbase; /* Return list base address */
}


void
kNewNode(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
	heap_ptr nodebase = heap_allocate(s->_heap, 8);

	if (!nodebase) {
		KERNEL_OOPS("Out of memory while creating a node");
		return;
	}

	nodebase += 2; /* Jump over heap header */

	PUT_HEAP(nodebase + LIST_PREVIOUS_NODE, 0);
	PUT_HEAP(nodebase + LIST_NEXT_NODE, 0);
	PUT_HEAP(nodebase + LIST_NODE_KEY, PARAM(0));
	PUT_HEAP(nodebase + LIST_NODE_VALUE, PARAM(1));

	SCIkdebug(SCIkNODES, "New nodebase at %04x\n", nodebase);

	s->acc = nodebase; /* Return node base address */
}


void
kAddToEnd(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
	heap_ptr listbase = UPARAM(0);
	heap_ptr nodebase = UPARAM(1);
	heap_ptr old_lastnode = GET_HEAP(listbase + LIST_LAST_NODE);
	SCIkdebug(SCIkNODES, "Adding node %04x to end of list %04x\n", nodebase, listbase);

	if (old_lastnode)
		PUT_HEAP(old_lastnode + LIST_NEXT_NODE, nodebase);

	PUT_HEAP(nodebase + LIST_PREVIOUS_NODE, old_lastnode);

	PUT_HEAP(listbase + LIST_LAST_NODE, nodebase);

	if (GET_HEAP(listbase + LIST_FIRST_NODE) == 0)
		PUT_HEAP(listbase + LIST_FIRST_NODE, nodebase);
	/* Set node to be the first and last node if it's the only node of the list */
}


void
kAddToFront(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
	heap_ptr listbase = UPARAM(0);
	heap_ptr nodebase = UPARAM(1);
	heap_ptr old_firstnode = GET_HEAP(listbase + LIST_FIRST_NODE);
	SCIkdebug(SCIkNODES, "Adding node %04x to start of list %04x\n", nodebase, listbase);

	if (old_firstnode)
		PUT_HEAP(old_firstnode + LIST_PREVIOUS_NODE, nodebase);

	PUT_HEAP(nodebase + LIST_NEXT_NODE, old_firstnode);

	PUT_HEAP(listbase + LIST_FIRST_NODE, nodebase);

	if (GET_HEAP(listbase + LIST_LAST_NODE) == 0)
		PUT_HEAP(listbase + LIST_LAST_NODE, nodebase);
	/* Set node to be the first and last node if it's the only node of the list */
}


void
kFindKey(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
	heap_ptr node;
	word key = UPARAM(1);
	SCIkdebug(SCIkNODES, "Looking for key %04x in list %04x\n", key, UPARAM(0));

	node = UGET_HEAP(UPARAM(0) + LIST_FIRST_NODE);

	SCIkdebug(SCIkNODES, "Node at %04x\n", node);

	while (node && (UGET_HEAP(node + LIST_NODE_KEY) != key)) {
		node = UGET_HEAP(node + LIST_NEXT_NODE);
		SCIkdebug(SCIkNODES, "NextNode at %04x\n", node);
	}
	/* Aborts if either the list ends (node == 0) or the key is found */

	SCIkdebug(SCIkNODES, "Looking for key: Result is %04x\n", node);
	s->acc = node;
}


int
_k_delete_key(state_t *s, heap_ptr list, heap_ptr key)
     /* Removes the specified key from the specified heap list, returns 0 on success, 1 otherwise */
{
	heap_ptr node;

	SCIkdebug(SCIkNODES, "Removing key %04x from list %04x\n", key, list);

	node = UGET_HEAP(list + LIST_FIRST_NODE);

	while (node && ((guint16) UGET_HEAP(node + LIST_NODE_KEY) != key))
		node = GET_HEAP(node + LIST_NEXT_NODE);
	/* Aborts if either the list ends (node == 0) or the key is found */


	if (node) {
		heap_ptr prev_node = UGET_HEAP(node + LIST_PREVIOUS_NODE);
		heap_ptr next_node = UGET_HEAP(node + LIST_NEXT_NODE);

		SCIkdebug(SCIkNODES,"Removing key from list: Succeeded at %04x\n", node);

		if (UGET_HEAP(list + LIST_FIRST_NODE) == node)
			PUT_HEAP(list + LIST_FIRST_NODE, next_node);
		if (UGET_HEAP(list + LIST_LAST_NODE) == node)
			PUT_HEAP(list + LIST_LAST_NODE, prev_node);

		if (next_node)
			PUT_HEAP(next_node + LIST_PREVIOUS_NODE, prev_node);
		if (prev_node)
			PUT_HEAP(prev_node + LIST_NEXT_NODE, next_node);

		heap_free(s->_heap, node - 2);

		return 1;

	} else SCIkdebug(SCIkNODES,"Removing key from list: FAILED\n");

	return 0;
}

void
kDeleteKey(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
	s->acc=_k_delete_key(s, UPARAM(0), UPARAM(1));
}


void
kFirstNode(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
	heap_ptr list = UPARAM(0);

	if (list)
		s->acc = UGET_HEAP(UPARAM(0) + LIST_FIRST_NODE);
	else
		s->acc = 0;
}


void
kEmptyList(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
	heap_ptr list = UPARAM(0);

	CHECK_THIS_KERNEL_FUNCTION;
	SCIkdebug(SCIkWARNING, "Warning: EmptyList() was invoked with %d parameters\n", argc);

	if (list)
		s->acc = !(GET_HEAP(UPARAM(0) + LIST_FIRST_NODE));
}


void
kAddAfter(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
	heap_ptr list = UPARAM(0);
	heap_ptr firstnode = UPARAM(1);
	heap_ptr newnode = UPARAM(2);

	CHECK_THIS_KERNEL_FUNCTION;
	SCIkdebug(SCIkWARNING, "Warning: AddAfter() was invoked with %d parameters\n", argc);

	if (argc != 3) {
		SCIkdebug(SCIkWARNING, "Aborting.\n");
		return;
	}

	if (firstnode) { /* We're really appending after */

		heap_ptr oldnext = GET_HEAP(firstnode + LIST_NEXT_NODE);
		PUT_HEAP(newnode + LIST_PREVIOUS_NODE, firstnode);
		PUT_HEAP(firstnode + LIST_NEXT_NODE, newnode);
		PUT_HEAP(newnode + LIST_NEXT_NODE, oldnext);

		if (!oldnext) /* Appended after last node? */
			PUT_HEAP(list + LIST_LAST_NODE, newnode) /* Set new node as last list node */
		else
			PUT_HEAP(oldnext + LIST_PREVIOUS_NODE, newnode);

	} else { /* Set as initial list node */

		heap_ptr nextnode = GET_HEAP(list + LIST_FIRST_NODE);
		PUT_HEAP(newnode + LIST_NEXT_NODE, nextnode);
		PUT_HEAP(list + LIST_FIRST_NODE, newnode);
		if (!nextnode) /* List was empty? */
			PUT_HEAP(list + LIST_LAST_NODE, newnode)
		else
			PUT_HEAP(nextnode + LIST_PREVIOUS_NODE, newnode);
	}
}


void
kLastNode(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
	heap_ptr list = UPARAM(0);

	if (list)
		s->acc = UGET_HEAP(UPARAM(0) + LIST_LAST_NODE);
	else
		s->acc = 0;
}


void
kPrevNode(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
	s->acc = UGET_HEAP(UPARAM(0) + LIST_PREVIOUS_NODE);
}


void
kNextNode(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
	s->acc = UGET_HEAP(UPARAM(0) + LIST_NEXT_NODE);
}


void
kNodeValue(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
	int a;
  
	a = UPARAM(0) + LIST_NODE_VALUE;

	s->acc=UGET_HEAP(a);

}


void
kDisposeList(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
	heap_ptr address = PARAM(0) - 2; /* -2 to get the heap header */
	heap_ptr node = UGET_HEAP(address + 2 + LIST_FIRST_NODE);

	while (node) { /* Free all nodes */
		heap_ptr node_heapbase = node - 2;

		node = GET_HEAP(node + LIST_NEXT_NODE); /* Next node */
		heap_free(s->_heap, node_heapbase); /* Clear heap space of old node */
	}

	if (!listp(s, address)) {
		SCIkwarn(SCIkERROR, "Attempt to dispose non-list at %04x\n", address);
	} else heap_free(s->_heap, address);
}

