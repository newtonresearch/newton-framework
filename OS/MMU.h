/*
	File:		MMU.h

	Contains:	Memory Management Unit function declarations.

	Written by:	Newton Research Group.
*/

#if !defined(__MMU_H)
#define __MMU_H 1

#include "SharedTypes.h"

// Level One Descriptor table definition
struct SectionDef
{
	PAddr		start;
	ULong		x04;
	ULong		size;
	ULong		bits;
};

#if defined(__cplusplus)
struct SGlobalsThatLiveAcrossReboot;
extern "C" {
#else
typedef struct SectionDef SectionDef;
typedef struct SGlobalsThatLiveAcrossReboot SGlobalsThatLiveAcrossReboot;
#endif

void			InitPageTable(void);
void			InitPagePermissions(void);

void			InitTheMMUTables(bool inAll, bool inMMUOn, PAddr inPrimaryDef, SGlobalsThatLiveAcrossReboot * info);
PAddr			MakePrimaryMMUTable(PAddr inPrimaryDef, PAddr ioDescriptors, bool inFill, bool inMMUOn, SGlobalsThatLiveAcrossReboot * info);

void			FlushTheMMU(void);

void			PrimSetDomainRangeWithPageTable(PAddr inDescriptors, PAddr inAddr, size_t inSize, ULong inDomain);
NewtonErr	AddPTableWithPageTable(PAddr inDescriptors, VAddr inVAddr, PAddr inPageTable);

NewtonErr	AddSecPerm(VAddr inVAddr, Perm inPerm);
NewtonErr	AddBigPgPerm(VAddr inVAddr, Perm inPerm);
NewtonErr	AddBigSubPgPerm(VAddr inVAddr, Perm inPerm);
NewtonErr	AddPgPerm(VAddr inVAddr, Perm inPerm);
NewtonErr	AddSubPgPerm(VAddr inVAddr, Perm inPerm);

NewtonErr	AddNewSecPNJT(VAddr inVAddr, PAddr inPhysAddr, ULong inDomain, Perm inPerm, bool inCacheable);

NewtonErr	AddSecP(VAddr inVAddr, PAddr inPhysAddr, bool inCacheable);
NewtonErr	AddBigPgP(VAddr inVAddr, PAddr inPhysAddr, bool inCacheable);
NewtonErr	AddPgP(VAddr inVAddr, PAddr inPhysAddr, bool inCacheable);

NewtonErr	AddPgPAndPerm(VAddr inVAddr, ULong inPerm, PAddr inPhysAddr, bool inCacheable);
NewtonErr	AddPgPAndPermWithPageTable(PAddr inDescriptors, VAddr inVAddr, ULong inPerm, PAddr inPhysAddr, bool inCacheable);

NewtonErr	RemoveSecPerm(VAddr inVAddr);
NewtonErr	RemoveBigPgPerm(VAddr inVAddr);
NewtonErr	RemoveBigSubPgPerm(VAddr inVAddr);
NewtonErr	RemovePgPerm(VAddr inVAddr);
NewtonErr	RemoveSubPgPerm(VAddr inVAddr);

NewtonErr	RemoveSecP(VAddr inVAddr);
NewtonErr	RemoveBigPgP(VAddr inVAddr);
NewtonErr	RemovePgP(VAddr inVAddr);

NewtonErr	RemovePgPAndPerm(VAddr inVAddr);

NewtonErr	RememberMappingUsingPAddr(VAddr inVAddr, ULong inPerm, PAddr inPhysAddr, bool inCacheable);

bool			CanUseBigPage(VAddr inVAddr);
NewtonErr	TransformSectionIntoPageTable(VAddr inVAddr);
NewtonErr	TransformBigPageToPages(VAddr inVAddr);

PAddr			PrimVtoP(VAddr inVAddr);
size_t		VtoSizeWithP(VAddr inVAddr, PAddr * outPAddr);
int			VtoUnit(VAddr inVAddr);
int			CheckVAddrRange(VAddr inAddr, size_t inSize);

#if defined(__cplusplus)
}
#endif

#endif	/* __MMU_H */
