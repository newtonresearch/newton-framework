/*
	File:		CRC16.h

	Contains:	Class to compute cyclic redundancy check on a block of data.

	Written by:	The Newton Tools group.
*/

#ifndef __CRC16_H
#define __CRC16_H 1

#include <stdlib.h>


/* -----------------------------------------------------------------------------
	C R C 1 6
----------------------------------------------------------------------------- */

struct CRC16
{
	uint16_t	finalCRC;
	uint32_t	workingCRC;

	void		reset(void);
	void		computeCRC(unsigned char * inBlock, size_t inSize);
	uint16_t	get(void);
};


#endif	/* __CRC16_H */
