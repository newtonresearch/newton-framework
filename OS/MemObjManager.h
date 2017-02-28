/*
	File:		MemObjManager.h

	Contains:	Memory object management definitions.

	Written by:	Newton Research Group.
*/

#if !defined(__MEMOBJMANAGER_H)
#define __MEMOBJMANAGER_H 1

#include "Environment.h"
#include "SingleQ.h"


/*------------------------------------------------------------------------------
	M e m I n i t
------------------------------------------------------------------------------*/

struct MemInit
{
	ULong		tag;			// +00
	PAddr		initStart;  // +04
	VAddr		dataStart;  // +08
	VAddr		zeroStart;  // +0C
	size_t	dataSize;	// +10
	size_t	zeroSize;	// +14
};


/*------------------------------------------------------------------------------
	D o m a i n M e m I n f o
------------------------------------------------------------------------------*/

struct DomainMemInfo
{
	ULong		tag;					// +00
	ULong		addr;					// +04
	size_t	size;					// +08
	size_t	framesHeapSize;	// +0C
	size_t	handlesHeapSize;	// +10
	ULong		flags;				// +14
};

extern const DomainMemInfo *	gDomainTable;			// points to one of…
extern const DomainMemInfo		g1MegDomainTable[];
extern const DomainMemInfo		g4MegDomainTable[];

extern ObjectId					gKernelDomainId;


/*------------------------------------------------------------------------------
	M e m O b j

	A memory object can be one of four types as enumerated below.
------------------------------------------------------------------------------*/

enum MemObjType
{
	kMemObjDomain,
	kMemObjEnvironment,
	kMemObjHeap,
	kMemObjPersistent,
	kNumOfMemObjTypes
};


/*------------------------------------------------------------------------------
	D B E n t r y

	Each entry in the memory object database holds its state.
------------------------------------------------------------------------------*/

class CMemDBEntry
{
public:
	ULong			fName;			// +00
};

class CDomainDBEntry : public CMemDBEntry
{
public:
	ObjectId		fId;				// +04
};

class CEnvironmentDBEntry : public CMemDBEntry
{
public:
	ObjectId		fId;				// +04
};

class CHeapDBEntry : public CMemDBEntry
{
public:
	Heap			fHeap;			// +04
};

class CPersistentDBEntry : public CMemDBEntry
{
public:
	void			init(ULong inName, bool inArg2 = true, ArrayIndex index = kIndexNotFound);

	Heap			fHeap;			// +04
	VAddr			fBase;			// +08
	size_t		fSize;			// +0C
	long			f10;				// +10	used by package manager
	CSingleQContainer	fPages;	// +14
	ULong			fDomainName;	// +1C
	ULong			fFlags;			// +20	flags in byte 0
										//			domain index in byte 1
// size +24
};


struct MemObjDBIndex
{
	CMemDBEntry *	entry;		// +00	base address of object array
	ULong				count;		// +04	number of objects in the array
	size_t			size;			// +08	size of each object in bytes
};

extern void	*	gMemObjHeap;


/*------------------------------------------------------------------------------
	M e m O b j M a n a g e r
------------------------------------------------------------------------------*/

class MemObjManager
{
public:
//	Domain management
	static NewtonErr	registerDomainId(ULong inName, ObjectId inId);
	static NewtonErr	findDomainId(ULong inName, ObjectId * outId);

	static bool			getDomainInfo(ULong inName, CDomainInfo * outInfo, NewtonErr * outError);
	static NewtonErr	getDomainInfoByName(ULong inName, CDomainInfo * outInfo);
private:
	static NewtonErr	primGetDomainInfo(ULong inName, CDomainInfo * outInfo);
	static NewtonErr	primGetDomainInfoByName(ULong inName, CDomainInfo * outInfo);

public:
//	Environment management
	static NewtonErr	registerEnvironmentId(ULong inName, ObjectId inId);
	static NewtonErr	findEnvironmentId(ULong inName, ObjectId * outId);

	static bool			getEnvironmentInfo(ULong inName, CEnvironmentInfo * outInfo, NewtonErr * outError);
	static bool			getEnvDomainName(ULong inName, ArrayIndex, ULong *, bool *, NewtonErr * outError);
private:
	static NewtonErr	primGetEnvironmentInfo(ULong inName, CEnvironmentInfo * outInfo);
	static NewtonErr	primGetEnvDomainName(ULong inName, ULong, ULong *, bool *, bool *);

public:
//	Heap management
	static NewtonErr	registerHeapRef(ULong inName, Heap inHeap);
	static NewtonErr	findHeapRef(ULong inName, Heap * outHeap);

//	Persistent object management
	static NewtonErr	registerPersistentNewEntry(ULong inName, CPersistentDBEntry * inEntry);
	static NewtonErr	deregisterPersistentEntry(ULong inName);
	static bool			getPersistentRef(ArrayIndex index, CPersistentDBEntry ** outEntry, NewtonErr * outError);

	static bool			findEntryByIndex(MemObjType inType, ArrayIndex index, void * outObj, NewtonErr * outError);
	static NewtonErr	findEntryByName(MemObjType inType, ULong inName, void * outObj);

//	Table entry lookup
	static NewtonErr	registerEntryByName(MemObjType inType, ULong inName, void * inObj);

private:
	static NewtonErr	primGetEntryByName(MemObjType inType, ULong inName, void * outObj);
	static NewtonErr	primSetEntryByName(MemObjType inType, ULong inName, void * inObj);
	static CMemDBEntry *	entryLocByName(MemObjType inType, ULong inName);

	static NewtonErr	primGetEntryByIndex(MemObjType inType, ArrayIndex index, void * outObj);
	static NewtonErr	primSetEntryByIndex(MemObjType inType, ArrayIndex index, void * inObj);
	static CMemDBEntry *	entryLocByIndex(MemObjType inType, ArrayIndex index);

	static void			copyObject(MemObjType inType, void * dst, void * src);

	friend NewtonErr	PrimGetMemObjInfo(void);
};


/*------------------------------------------------------------------------------
	F u n c t i o n   P r o t o t y p e s
------------------------------------------------------------------------------*/

void		ComputeMemObjDatabaseSize(size_t * outSize);
void		BuildMemObjDatabase(void);
void		InitKernelDomainAndEnvironment(void);


#endif	/* __MEMOBJMANAGER_H */
