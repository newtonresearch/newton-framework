/*
	File:		Allocation.h

	Contains:	Functions to allocate memory.

	Written by:	The Newton Tools group.
*/

#ifndef __ALLOCATION_H
#define __ALLOCATION_H

void *	calloc_or_exit(unsigned int inNumItems, size_t inSize, const char * inMessage);
void *	malloc_or_exit(size_t inSize, const char * inMessage);

#endif /* __ALLOCATION_H */
