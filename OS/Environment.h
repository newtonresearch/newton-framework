/*
	File:		Environment.h

	Contains:	Memory architecture - environment declarations.

	Written by:	Newton Research Group.
*/

#if !defined(__ENVIRONMENT_H)
#define __ENVIRONMENT_H 1

#include "KernelObjects.h"
#include "UserObjects.h"


const int kNumOfDomains = 16;
const int kDomainBits = 2;
const int kDomainMask = ((1 << kDomainBits) - 1);

enum DomainType
{
	kFaultDomain,
	kClientDomain,
	kReservedDomain,
	kManagerDomain
};


/*--------------------------------------------------------------------------------
	C D o m a i n I n f o
--------------------------------------------------------------------------------*/

class CDomainInfo
{
public:
	void		initDomainInfo(ULong inName, ULong in04, VAddr inBase, size_t inSize);
	void		initGlobalInfo(VAddr inBase, VAddr inROMBase, size_t inInitSize, size_t inZeroSize);
	void		initHeapInfo(size_t inSize, size_t inHandleSize, ULong inFlags);

	ULong		name(void)					const;
	ULong		base(void)					const;
	size_t	size(void)					const;

	bool		hasGlobals(void)			const;
	VAddr		globalBase(void)			const;
	VAddr		globalROMBase(void)		const;
	size_t	globalSize(void)			const;
	size_t	globalInitSize(void)		const;
	size_t	globalZeroSize(void)		const;

	size_t	heapSize(void)				const;
	size_t	handleHeapSize(void)		const;
	bool		makeHeapDomain(void)		const;
	bool		hasHeap(void)				const;

	bool		isPersistent(void)		const;
	bool		isReadOnly(void)			const;
	bool		isCacheable(void)			const;
	bool		exceptOnNoMem(void)		const;
	bool		isSegregated(void)		const;
	bool		isHunkOMemory(void)		const;

private:
	ULong		fName;				// +00
	ULong		f04;					// +04
	VAddr		fBase;				// +08
	size_t	fSize;				// +0C
	VAddr		fGlobalROMBase;	// +10
	VAddr		fGlobalBase;		// +14
	size_t	fGlobalInitSize;	// +18
	size_t	fGlobalZeroSize;	// +1C
	size_t	fHeapSize;			// +20
	size_t	fHandleHeapSize;	// +24
	ULong		fHeapFlags;			// +28
};

inline ULong	CDomainInfo::name(void) const  { return fName; }
inline ULong	CDomainInfo::base(void) const  { return fBase; }
inline size_t	CDomainInfo::size(void) const  { return fSize; }

inline bool		CDomainInfo::hasGlobals(void) const  { return globalSize() != 0; }
inline VAddr	CDomainInfo::globalBase(void) const  { return fGlobalBase; }
inline VAddr	CDomainInfo::globalROMBase(void) const  { return fGlobalROMBase; }
inline size_t	CDomainInfo::globalSize(void) const  { return fGlobalInitSize + fGlobalZeroSize; }
inline size_t	CDomainInfo::globalInitSize(void) const  { return fGlobalInitSize; }
inline size_t	CDomainInfo::globalZeroSize(void) const  { return fGlobalZeroSize; }

inline size_t	CDomainInfo::heapSize(void) const  { return fHeapSize; }
inline size_t	CDomainInfo::handleHeapSize(void) const  { return fHandleHeapSize; }
inline bool		CDomainInfo::makeHeapDomain(void) const  { return (fHeapFlags & 0x10) != 0; }
inline bool		CDomainInfo::hasHeap(void) const  { return heapSize() != 0 || handleHeapSize() != 0; }

inline bool		CDomainInfo::isPersistent(void) const  { return hasHeap() && (fHeapFlags & 0x01) != 0; }
inline bool		CDomainInfo::isReadOnly(void) const  { return hasHeap() && (fHeapFlags & 0x02) != 0; }
inline bool		CDomainInfo::isCacheable(void) const  { return hasHeap() && (fHeapFlags & 0x04) != 0; }
inline bool		CDomainInfo::exceptOnNoMem(void) const  { return hasHeap() && (fHeapFlags & 0x08) != 0; }
inline bool		CDomainInfo::isSegregated(void) const  { return hasHeap() && (fHandleHeapSize != 0); }
inline bool		CDomainInfo::isHunkOMemory(void) const  { return hasHeap() && (fHeapFlags & 0x20) != 0; }


/*--------------------------------------------------------------------------------
	C D o m a i n
--------------------------------------------------------------------------------*/

class CDomain : public CObject
{
public:
					CDomain();
					~CDomain();

	NewtonErr	init(ObjectId inMonitorId, VAddr inRangeStart, size_t inRangeLength);
	NewtonErr	initWithDomainNumber(ObjectId inMonitorId, VAddr inRangeStart, size_t inRangeLength, ULong inDomNum);
	NewtonErr	setFaultMonitor(ObjectId inMonitorId);
	bool			intersects(VAddr inRangeStart, VAddr inRangeEnd);

	ObjectId		fFaultMonitor;	// +10
	VAddr			fStart;			// +14
	size_t		fLength;			// +18
	long			fNumber;			// +1C
	CDomain *	fNextDomain;	// +20
};


/*--------------------------------------------------------------------------------
	C E n v i r o n m e n t I n f o
--------------------------------------------------------------------------------*/

class CEnvironmentInfo
{
public:
	void			init(ULong inName, ULong inId, ULong inHeap, ULong inHeapDomain, ULong inStackDomain);

	ULong			name(void)						const;
	ULong			defaultHeap(void)				const;
	ULong			defaultHeapDomain(void)		const;
	ULong			defaultStackDomain(void)	const;

	bool			domains(ArrayIndex index, ULong * outName, bool * outIsManager, NewtonErr * outError);

private:
	ULong			fName;						// +00
	ULong			fId;							// +04
	ULong			fDefaultHeap;				// +08
	ULong			fDefaultHeapDomain;		// +0C
	ULong			fDefaultStackDomain;		// +10
};

inline ULong		CEnvironmentInfo::name(void) const  { return fName; }
inline ULong		CEnvironmentInfo::defaultHeap(void) const  { return fDefaultHeap; }
inline ULong		CEnvironmentInfo::defaultHeapDomain(void) const  { return fDefaultHeapDomain; }
inline ULong		CEnvironmentInfo::defaultStackDomain(void) const  { return fDefaultStackDomain; }


/*--------------------------------------------------------------------------------
	C E n v i r o n m e n t
--------------------------------------------------------------------------------*/

class CEnvironment : public CObject
{
public:
					~CEnvironment();

	NewtonErr	init(Heap inHeap);
	NewtonErr	add(CDomain * inDomain, bool inIsManager, bool inHasStack, bool inHasHeap);
	NewtonErr	remove(CDomain * inDomain);
	NewtonErr	hasDomain(CDomain * inDomain, bool * outHasDomain, bool * outIsManager);
	void			incrRefCount(void);
	bool			decrRefCount(void);

	ULong				fDomainAccess;		// +10
	Heap				fHeap;				// +14
	ObjectId			fStackDomainId;	// +18
	ObjectId			fHeapDomainId;		// +1C
	long				fRefCount;			// +20
	bool				f24;
	CEnvironment *	fNextEnvironment;	// +28
};


/*------------------------------------------------------------------------------
	C U E n v i r o n m e n t
------------------------------------------------------------------------------*/

class CUEnvironment : public CUObject
{
public:
					CUEnvironment(ObjectId id = 0);

	NewtonErr	init(Heap inHeap);

	NewtonErr	add(ObjectId inDomId, bool inIsManager, bool inHasStack, bool inHasHeap);
	NewtonErr	remove(ObjectId inDomId);
	NewtonErr	hasDomain(ObjectId inDomId, bool * outHasDomain, bool * outIsManager);
};

inline			CUEnvironment::CUEnvironment(ObjectId id) : CUObject(id)  { }


/*------------------------------------------------------------------------------
	C L i c e n s e e V A d d r e s s

	Maybe not the right place for this...
------------------------------------------------------------------------------*/

class CLicenseeVAddress
{
public:
				CLicenseeVAddress(VAddr);
				CLicenseeVAddress(const CLicenseeVAddress &);
				~CLicenseeVAddress();

//	CLicenseeVAddress&  operator=(const CUPhys &);
    CLicenseeVAddress&  operator=(const CLicenseeVAddress &);

	void		addDomainToEnvironment(void);
	void		setupDomain(void);
	VAddr		getNextVirtualAddress(VAddr);
	void		map(unsigned char, Perm);
	void		unmap(void);
};


/*--------------------------------------------------------------------------------
	F u n c t i o n   P r o t o t y p e s
--------------------------------------------------------------------------------*/

extern void		InitKernelDomainAndEnvironment(void);

extern ULong	DefaultDCR(void);
extern ULong	AddClientToDCR(ULong inBits, int inDomNum);
extern ULong	AddManagerToDCR(ULong inBits, int inDomNum);
extern ULong	RemoveFromDCR(ULong inBits, int inDomNum);
extern int		GetSpecificDomainFromDCR(ULong & ioBits, int inDomNum);
extern int		NextAvailDomainInDCR(ULong & ioBits);


#endif	/* __ENVIRONMENT_H */
