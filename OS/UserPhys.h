/*
	File:		UserPhys.h

	Contains:	Virtual to physical address mapping.

	Written by:	Newton Research Group.
*/

#if !defined(__USERPHYS_H)
#define __USERPHYS_H 1

#if !defined(__NEWTON_H)
#include "Newton.h"
#endif

#if !defined(__SHAREDTYPES_H)
#include "SharedTypes.h"
#endif

#if !defined(__USEROBJECTS_H)
#include "UserObjects.h"
#endif

#if !defined(__USERPORTS_H)
#include "UserPorts.h"
#endif

#if !defined(__KERNELPHYS_H)
#include "KernelPhys.h"
#endif


/*--------------------------------------------------------------------------------
	C U P h y s
--------------------------------------------------------------------------------*/

class CUPhys : public CUObject
{
public:
					CUPhys(ObjectId id = 0);
	void			operator=(const CUPhys & inCopy);
	NewtonErr	init(PAddr inBase, size_t inSize, bool inReadOnly = NO, bool inCache = YES);

	// change any mappings associated with this phys
	NewtonErr	invalidate();			// to invalid
	NewtonErr	makeInaccessible();	// to inaccessible
	NewtonErr	makeAccessible();		// to accessible

	// change the mapping associated with the provided virtual address of this phys.
	// this routine is not for the faint of heart.  It cannot cause a page allocation
	// so if the virtual address and range don't match the pagetable entries exactly it
	// will return an error.  In other words the specified range better match exactly a
	// previously mapped physical range in size and base.
	NewtonErr	changeVirtualMapping(VAddr inVirtAddr, size_t inVSize, PhysicalChange inAccess);

	// mode info
	NewtonErr	readOnly(bool & ioRO);

	NewtonErr	size(size_t & ioSize);			// the size of this phys
	NewtonErr	align(ULong & ioAlignment);	// the alignment of this phys
	NewtonErr	base(PAddr & ioPhysAddr);		// the physical address of the base of this phys
};

/*------------------------------------------------------------------------------
	C U P h y s   I n l i n e s
------------------------------------------------------------------------------*/

inline	CUPhys::CUPhys(ObjectId id) : CUObject(id)  { }
inline	CUPhys::operator=(const CUPhys & inCopy)  { copyObject(inCopy); }


#endif __USERPHYS_H
