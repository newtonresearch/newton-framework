/*
	File:		CObjectBinaries.h

	Contains:	C object binary declarations.
					CObjectBinaries are a type of LargeBinary object.
					They are C objects (typically a C++ class instance) but
					allocated in the frames heap so they are subject to garbage
					collection.

	Written by:	Newton Research Group, 2007.
*/

#if !defined(__COBJECTBINARIES_H)
#define __COBJECTBINARIES_H 1

#include "LargeBinaries.h"

typedef	void	(*DeleteProc)	(void * inData);
typedef	void	(*MarkProc)		(void * inData);
typedef	void	(*UpdateProc)	(void * inData);


/*------------------------------------------------------------------------------
	C O b j e c t B i n a r y D a t a
------------------------------------------------------------------------------*/

struct CObjectBinaryData
{
	char *		fData;
	DeleteProc	fDeleteProc;
	MarkProc		fMarkProc;
	UpdateProc	fUpdateProc;
};


/*------------------------------------------------------------------------------
	F u n c t i o n   P r o t o t y p e s
------------------------------------------------------------------------------*/

extern	Ref		AllocateCObjectBinary(void * inAddr, DeleteProc inDeleteProc, MarkProc inMarkProc, UpdateProc inUpdateProc);
extern	Ref		AllocateFramesCObject(size_t inSize, DeleteProc inDeleteProc, MarkProc inMarkProc, UpdateProc inUpdateProc);


#endif	/* __COBJECTBINARIES_H */
