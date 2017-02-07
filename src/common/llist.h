
#ifndef _LLIST_H
#define _LLIST_H

// this file provides macros which implement common operations on linked lists.
// this saves a little bit of thinking and helps prevent bugs caused by
// forgetting one of the steps.

// call them giving them the node to work on and the names of the fields
// for next, prev, first and last nodes.

// add a node to the end of a linked list
#define LL_ADD_END(O, PREV, NEXT, FIRST, LAST)	\
{	\
	if (LAST)	\
		LAST->NEXT = O;	\
	else	\
		FIRST = O;	\
	\
	O->PREV = LAST;	\
	O->NEXT = NULL; \
	LAST = O;	\
}

// add a node at the start of a linked list
#define LL_ADD_BEGIN(O, PREV, NEXT, FIRST, LAST)	\
{	\
	O->NEXT = FIRST;	\
	O->PREV = NULL;	\
	\
	if (FIRST)	\
		FIRST->PREV = O;	\
	else	\
		FIRST = LAST = O;	\
}

// insert a node just before another node
#define LL_INSERT_BEFORE(O, BEHIND, PREV, NEXT, FIRST, LAST)	\
{	\
	if (BEHIND == FIRST)	\
		FIRST = O;	\
	else	\
		BEHIND->PREV->NEXT = O;	\
	\
	O->NEXT = BEHIND;	\
	O->PREV = BEHIND->PREV;	\
	BEHIND->PREV = O;	\
}

// insert a node just after another node
#define LL_INSERT_AFTER(O, AFTER, PREV, NEXT, FIRST, LAST)	\
{	\
	if (AFTER == LAST)	\
		LAST = O;	\
	else	\
		AFTER->NEXT->PREV = O;	\
	\
	O->NEXT = AFTER->NEXT;	\
	O->PREV = AFTER;	\
	AFTER->NEXT = O;	\
}

// remove a node from a linked list
#define LL_REMOVE(O, PREV, NEXT, FIRST, LAST)	\
{	\
	if (O == FIRST)	\
		FIRST = FIRST->NEXT;	\
	else if (O->PREV)	\
		O->PREV->NEXT = O->NEXT;	\
	\
	if (O == LAST)	\
		LAST = LAST->PREV;	\
	else if (O->NEXT)	\
		O->NEXT->PREV = O->PREV;	\
}

// debug function
#define LL_DUMP_LIST(START, PREV, NEXT, NODETYPE)	\
{	\
	stat("LL_DUMP_LIST from %s using %s", #START, #NEXT);	\
	\
	NODETYPE *n = START;	\
	int iter = 0;	\
	while(n)	\
	{	\
		stat("%d: %08x   P:%08x  N:%08x", iter++, n, n->PREV, n->NEXT);	\
		n = n->NEXT;	\
	}	\
}


#endif


