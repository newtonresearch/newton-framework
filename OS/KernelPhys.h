/*
	File:		KernelPhys.h

	Contains:	Virtual to physical address mapping.

	Written by:	Newton Research Group.
*/

#if !defined(__KERNELPHYS_H)
#define __KERNELPHYS_H 1

#if !defined(__NEWTON_H)
#include "Newton.h"
#endif

#if !defined(__SHAREDTYPES_H)
#include "SharedTypes.h"
#endif

#if !defined(__KERNELOBJECTS_H)
#include "KernelObjects.h"
#endif


enum PhysicalChange
{
	kMakeRangeInvalid,
	kMakeRangeInaccessible,
	kMakeRangeAccessible
};


/* -----------------------------------------------------------------------------
	C P h y s
----------------------------------------------------------------------------- */

class CPhys : public CObject
{
public:
				CPhys();
				~CPhys();

	NewtonErr	init(PAddr inBase, size_t inSize, bool inReadOnly = NO, bool inCache = YES);
	void			initState(PAddr inBase, size_t inSize, bool inReadOnly = NO, bool inCache = YES);

	// Change any mappings associated with this phys…
	NewtonErr	invalidate();			// to invalid
	NewtonErr	makeInaccessible();	// to inaccessible
	NewtonErr	makeAccessible();		// to accessible

	// Change the mapping associated with the provided virtual address of this phys.
	// This routine is not for the faint of heart.  It cannot cause a page allocation
	// so if the virtual address and range don’t match the pagetable entries exactly it
	// will return an error.  In other words the specified range better match exactly a
	// previously mapped physical range in size and base.
	NewtonErr	changeVirtualMapping(VAddr inVAddr, size_t inVSize, PhysicalChange inAccess);

	PAddr		base(void) const;
	size_t	size(void) const;
	bool		isReadOnly(void) const;
	void		anon0038AB1C(void);

protected:
	friend class CExtPageTracker;

#if defined(correct)
	unsigned int	fPageAddr:20;		// +10
	unsigned int	fIsReadOnly:1;		// 0800
	unsigned int	fIsCached:1;		// 0400
	unsigned int	fIsLittle:1;		// 0200
	unsigned int	fIsWhatever:1;		// 0100
	unsigned int	fNumOfPages:8;		// 00FF
#else
	unsigned long	fPageAddr:52;
	unsigned long	fIsReadOnly:1;
	unsigned long	fIsCached:1;
	unsigned long	fIsLittle:1;
	unsigned long	fIsWhatever:1;
	unsigned long	fNumOfPages:8;
#endif
	size_t			fSize;				// +14
// size +18
};

inline PAddr	CPhys::base(void) const  { return fPageAddr * kPageSize; }
inline size_t	CPhys::size(void) const  { return fIsLittle ? (size_t)fNumOfPages * kPageSize : fSize; }
inline bool		CPhys::isReadOnly(void) const  { return fIsReadOnly; }
inline void		CPhys::anon0038AB1C(void)  { fId = kNoId; }


/*------------------------------------------------------------------------------
	C L i t t l e P h y s
------------------------------------------------------------------------------*/

class CLittlePhys : public CPhys
{
public:
			CLittlePhys();

//protected:
	VAddr		f18;		// +18
// size +1C
};


/*------------------------------------------------------------------------------
	F u n c t i o n   P r o t o t y p e s
------------------------------------------------------------------------------*/

bool		GetPhys(ObjectId inPageId, CPhys * &outPage);
PAddr		PhysBase(ObjectId inId);
size_t	PhysSize(ObjectId inId);
ULong		PhysAlign(ObjectId inId);
bool		PhysReadOnly(ObjectId inId);

void		PhysSubPageCopy(PAddr inFromAddr, PAddr inToAddr);


#endif	/* __KERNELPHYS_H */
