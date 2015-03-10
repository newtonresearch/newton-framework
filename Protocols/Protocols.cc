/*
	File:		Protocols.cc

	Contains:	Support for protocols;
					monitors, protocol meta-information, protocol registry.

	Written by:	Newton Research Group.

	Each p-class implementation must provide a classInfo struct that is registered with
	the ClassInfoRegistry; assembler-type despatching is performed through the classInfo.

	Is the interface’s static make(name) really needed? It’s only half-heartedly used.
	There seem to be two ways to instantiate a p-class implementation:

	MakeByName
		finds CClassInfo for named implementation
		calls classInfo->make()
			allocates instance
			creates default interface ivars
			calls implementation’s make()

	intf::make
		calls AllocInstanceByName()
			finds CClassInfo for named implementation
			calls classInfo->makeAt()
				creates default interface ivars
		dispatches to implementation’s make()

	So I’m guessing the intf::make style is for 1.0 -- but should we abandon it?
	MakeByName is more powerful since it allows you to specify capabilities rather than
	simply naming an implementation.

	The interface destroy glue
		calls destroy() on the implementation instance
		calls FreeInstance()

	...which means that unless we implement all the dispatch glue,
	we’ll have to call FreeInstance() at the tail of every implementation’s destroy()

#if defined(hasNoProtocols)
	FreeInstance(this);
#endif

*/

#include "Protocols.h"
#include "UserMonitor.h"

size_t			PrivateClassInfoSize(const CClassInfo * inClass);
void				PrivateClassInfoMakeAt(const CClassInfo * inClass, const void * instance);
const char *	PrivateClassInfoInterfaceName(const CClassInfo * inClass);
const char *	PrivateClassInfoImplementationName(const CClassInfo * inClass);
const char *	PrivateClassInfoSignature(const CClassInfo * inClass);


/*------------------------------------------------------------------------------
	C P r o t o c o l
	Base class for protocols and protocol monitors.
------------------------------------------------------------------------------*/

void
CProtocol::become(const CProtocol * instance)
{
	fRealThis = instance;
}


void
CProtocol::become(ObjectId inMonitorId)
{
	fMonitorId = inMonitorId;
}


const CClassInfo *
CProtocol::classInfo(void) const
{
	typedef const CClassInfo * (*ClassInfoProc)(void);
	ClassInfoProc classInfoProc = (ClassInfoProc) ((char *)fRealThis->fBTable + (long)fRealThis->fBTable[1]);
	return classInfoProc();
}


ObjectId
CProtocol::getMonitorId(void) const
{
	return fMonitorId;
}


void
CProtocol::setType(const CClassInfo * info)
{
	fBTable = (int32_t *)((char *)info + info->fBTableDelta);
}


NewtonErr
CProtocol::startMonitor(size_t inStackSize, ObjectId inEnvironmentId, ULong inName, bool inRebootProtected)
{
	NewtonErr err;
#if !defined(forFramework)
	CUMonitor monitor;
	if ((err = monitor.init((MonitorProcPtr)classInfo()->entryProc(), inStackSize, this, inEnvironmentId, NO, inName, inRebootProtected)) == noErr)
	{
		monitor.setDestroyKernelObject(NO);
		fMonitorId = monitor;
	}
#else
	err = -1;
#endif
	return err;
}


NewtonErr
CProtocol::destroyMonitor(void)
{
#if !defined(forFramework)
	CUMonitor * monitor = new CUMonitor(fMonitorId);
	if (monitor == NULL)
		return MemError();
	monitor->setDestroyKernelObject(YES);
	delete monitor;
#endif
	FreePtr((Ptr)this);
	return noErr;
}

#pragma mark -

/*------------------------------------------------------------------------------
	C C l a s s I n f o R e g i s t r y
	P-class interface.
	A registry of protocols.
	This is itself a protocol -- it defines the public interface to
	CClassInfoRegistryImpl.
	The (totally relocatable) code would have been created by ProtocolGen.
------------------------------------------------------------------------------*/

CClassInfoRegistry *	gProtocolRegistry;


#pragma mark ProtocolEntry

/*------------------------------------------------------------------------------
	P r o t o c o l E n t r y
	The  registry encapsulates an array of ProtocolEntry structs.
	Each of these holds the ClassInfo metadata.
------------------------------------------------------------------------------*/

struct ProtocolEntry
{
	const CClassInfo *	classInfo;		// +00
	ULong			refCon;			// +04	for 1.0? don’t believe this is used any longer
	UShort		refCount;		// +08
	UShort		instanceCount;	// +0A
	UShort		intfHash;		// +0C
	UShort		implHash;		//	+0E
	ULong			version;			// +10

	bool	satisfiesHash(bool inHasInterface, const UShort interfaceHash, bool inHasImpl, const UShort inImplHash, const ULong inVersion);
	bool	satisfiesCapabilities(const char * inKey, const char * inValue);
};

#define kAnyVersion 0xFFFFFFFF


/*------------------------------------------------------------------------------
	Check whether this protocol matches the given interface & implementation hashes.
	Args:		inHasIntf
				intfHash
				inHasImpl
				inImplHash
				inVersion
	Return:	YES => this protocol has the right hashes
------------------------------------------------------------------------------*/

bool
ProtocolEntry::satisfiesHash(bool inHasIntf, const UShort inIntfHash, bool inHasImpl, const UShort inImplHash, const ULong inVersion)
{
	if (!inHasIntf)
		return YES;
	if (intfHash != inIntfHash)
		return NO;
	if (!inHasImpl)
		return YES;
	if (implHash != inImplHash)
		return NO;
	return (version >= inVersion);
}


/*------------------------------------------------------------------------------
	Check whether this protocol matches the given capabilities.
	Args:		inKey
				inKeyValue
	Return:	YES => this protocol has the right capabilities
------------------------------------------------------------------------------*/

bool
ProtocolEntry::satisfiesCapabilities(const char * inKey, const char * inKeyValue)
{
	if (inKey == NULL)
		return YES;
	const char * value = classInfo->getCapability(inKey);
	if (value)
	{
		if (inKeyValue == NULL || strcmp(inKeyValue, value) == 0)
			return YES;
	}
	return NO;
}

#pragma mark -

/*------------------------------------------------------------------------------
	Check whether class info metadata matches the given interface.
	Args:		inClass
				inIntf
				inImpl
				inVersion
	Return:	YES => this metadata has the right interface
------------------------------------------------------------------------------*/

bool
Satisfies(const CClassInfo * inClass, const char * inIntf, const char * inImpl, ULong inVersion)
{
	bool isSatisfied;
	newton_try
	{
		isSatisfied =
			(inIntf == NULL || strcmp(inIntf, inClass->interfaceName()) == 0)
		&& (inImpl == NULL || strcmp(inImpl, inClass->implementationName()) == 0)
		&&  inClass->version() >= inVersion;
	}
	newton_catch(exBusError)
	{
		isSatisfied = NO;
	}
	end_try;
	return isSatisfied;
}


#pragma mark -

/*------------------------------------------------------------------------------
	C C l a s s I n f o C o m p a r a t o r
------------------------------------------------------------------------------*/
#include "NArray.h"

class CClassInfoComparator : public NComparator
{
public:
	int			compareKeys(const void * inKey1, const void * inKey2) const;
};


int
CClassInfoComparator::compareKeys(const void * inKey1, const void * inKey2) const
{
	UShort key1, key2;
	ULong v1, v2;

	key1 = ((ProtocolEntry *)inKey1)->intfHash;
	key2 = ((ProtocolEntry *)inKey2)->intfHash;
	if (key1 < key2)
		return -1;
	if (key1 > key2)
		return 1;

	key1 = ((ProtocolEntry *)inKey1)->implHash;
	key2 = ((ProtocolEntry *)inKey2)->implHash;
	if (key1 < key2)
		return -1;
	if (key1 > key2)
		return 1;

	v1 = ((ProtocolEntry *)inKey1)->version;
	v2 = ((ProtocolEntry *)inKey2)->version;
	if (v1 < v2)
		return -1;
	if (v1 > v2)
		return 1;

	return 0;
}

#pragma mark -

/*------------------------------------------------------------------------------
	C C l a s s I n f o R e g i s t r y I m p l
	The implementation of CClassInfoRegistry.
	(The monitor despatches to a CClassInfoRegistryImpl method.)
------------------------------------------------------------------------------*/

#define kSatisfyCacheSize 4

class CClassInfoRegistryImpl : public CClassInfoRegistry
{
public:
	PROTOCOL_IMPL_HEADER_MACRO(CClassInfoRegistryImpl)

	CClassInfoRegistry * make(void);		// was New()
	void				destroy(void);			// was Delete()

	NewtonErr		registerProtocol(const CClassInfo * inClass, ULong refCon = 0);
	NewtonErr		deregisterProtocol(const CClassInfo * inClass, bool specific = NO);
	bool				isProtocolRegistered(const CClassInfo * inClass, bool specific = NO) const;

	const CClassInfo *	satisfy(const char * intf, const char * impl, ULong version) const;
	const CClassInfo *	satisfy(const char * intf, const char * impl, const char * capability) const;
	const CClassInfo *	satisfy(const char * intf, const char * impl, const char * capability, const char * capabilityValue) const;
	const CClassInfo *	satisfy(const char * intf, const char * impl, const int capability, const int capabilityValue = 0) const;

	void				updateInstanceCount(const CClassInfo * inClass, int inAdjustment);
	ArrayIndex		getInstanceCount(const CClassInfo * inClass);

private:
	ProtocolEntry *	find(const CClassInfo * inClass) const;
	ProtocolEntry *	satisfy(const CClassInfo * inClass) const;
	const UShort		hashString(const char * inStr) const;
	void				invalidateSatisfyCache(void);

	NSortedArray *	fProtocols;		// +10
	int				fSeed;			// +14
	CClassInfoComparator *	fCmp;				// +18
	mutable const CClassInfo *	fCache[kSatisfyCacheSize];		// +1C
};

#pragma mark -

/*------------------------------------------------------------------------------
	Start up the protocol registry.
	Args:		--
	Return:	--
------------------------------------------------------------------------------*/

void
StartupProtocolRegistry(void)
{
	//	Even the protocol registry is a protocol!
	gProtocolRegistry = (CClassInfoRegistry *)CClassInfoRegistryImpl::classInfo()->make();
	gProtocolRegistry->startMonitor(1*KByte);
	CClassInfoRegistryImpl::classInfo()->registerProtocol();
}


/*--------------------------------------------------------------------------------
	Return a pointer to the protocol registry.
	Args:		--
	Return:	the protocol registry
--------------------------------------------------------------------------------*/

CClassInfoRegistry *
GetProtocolRegistry(void)
{
	return gProtocolRegistry;
}


/* ----------------------------------------------------------------
	CClassInfoRegistryImpl implementation class info.
---------------------------------------------------------------- */

const CClassInfo *
CClassInfoRegistryImpl::classInfo(void)
{
__asm__ (
CLASSINFO_BEGIN
"		.long		0			\n"
"		.long		1f - .	\n"
"		.long		2f - .	\n"
"		.long		3f - .	\n"
"		.long		4f - .	\n"
"		.long		5f - .	\n"
"		.long		__ZN22CClassInfoRegistryImpl6sizeOfEv - 0b	\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		__ZN22CClassInfoRegistryImpl4makeEv - 0b	\n"
"		.long		__ZN22CClassInfoRegistryImpl7destroyEv - 0b	\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		6f - 0b	\n"
"1:	.asciz	\"CClassInfoRegistryImpl\"	\n"
"2:	.asciz	\"CClassInfoRegistry\"	\n"
"3:	.byte		0			\n"
"		.align	2			\n"
"4:	.long		0			\n"
"		.long		__ZN22CClassInfoRegistryImpl9classInfoEv - 4b	\n"
"		.long		__ZN22CClassInfoRegistryImpl4makeEv - 4b	\n"
"		.long		__ZN22CClassInfoRegistryImpl7destroyEv - 4b	\n"
"		.long		__ZN22CClassInfoRegistryImpl16registerProtocolEPK10CClassInfoj - 4b	\n"
"		.long		__ZN22CClassInfoRegistryImpl18deregisterProtocolEPK10CClassInfob - 4b	\n"
"		.long		__ZNK22CClassInfoRegistryImpl20isProtocolRegisteredEPK10CClassInfob - 4b	\n"
"		.long		__ZNK22CClassInfoRegistryImpl7satisfyEPKcS1_j - 4b	\n"
"		.long		__ZNK22CClassInfoRegistryImpl7satisfyEPKcS1_S1_ - 4b	\n"
"		.long		__ZNK22CClassInfoRegistryImpl7satisfyEPKcS1_S1_S1_ - 4b	\n"
"		.long		__ZNK22CClassInfoRegistryImpl7satisfyEPKcS1_ii - 4b	\n"
"		.long		__ZN22CClassInfoRegistryImpl19updateInstanceCountEPK10CClassInfoi - 4b	\n"
"		.long		__ZN22CClassInfoRegistryImpl16getInstanceCountEPK10CClassInfo - 4b	\n"
CLASSINFO_END
);
}

PROTOCOL_IMPL_SOURCE_MACRO(CClassInfoRegistryImpl)

CClassInfoRegistry *
CClassInfoRegistryImpl::make(void)
{
	fCmp = new CClassInfoComparator;
	fProtocols = new NSortedArray;
	fProtocols->init(fCmp, sizeof(ProtocolEntry)/*element size*/, 8*sizeof(ProtocolEntry)/*chunk size*/, 104/*initial count*/, NO/*don’t shrink*/);
	fSeed = 1;
	invalidateSatisfyCache();
	return this;
}

void
CClassInfoRegistryImpl::destroy(void)
{
	if (fProtocols)
		delete fProtocols;
	if (fCmp)		// not in the original
		delete fCmp;
}


NewtonErr
CClassInfoRegistryImpl::registerProtocol(const CClassInfo * inClass, ULong inRefCon)
{
	NewtonErr err = noErr;
	if (inClass)
	{
		newton_try
		{
//			size_t implLen = strlen(inClass->implementationName());	// in the original, but unused
//			size_t intfLen = strlen(inClass->interfaceName());
//			size_t sigLen = strlen(inClass->signature());
			ProtocolEntry * existingEntry;
			if ((existingEntry = find(inClass)) == NULL)
			{
				invalidateSatisfyCache();
				ProtocolEntry entry;
				entry.classInfo = inClass;
				entry.refCon = inRefCon;
				entry.refCount = 0;
				entry.intfHash = hashString(inClass->interfaceName());
				entry.implHash = hashString(inClass->implementationName());
				entry.version = inClass->version();
				err = fProtocols->insertElements(fProtocols->where(&entry), 1, &entry);
			}
			else
			{
				existingEntry->refCount++;
			}
			fSeed++;
		}
		newton_catch(exAbort)
		{
			err = -1;
		}
		end_try;
	}
	else
		err = -1;
	return err;
}


NewtonErr
CClassInfoRegistryImpl::deregisterProtocol(const CClassInfo * inClass, bool inSpecific)
{
	NewtonErr err = noErr;
	newton_try
	{
		ProtocolEntry * entry = inSpecific ? find(inClass) : satisfy(inClass);
		if (entry)
		{
			if (entry->refCount > 0)
				entry->refCount--;
			if (entry->refCount == 0)
			{
				invalidateSatisfyCache();
				err = fProtocols->removeElements(fProtocols->contains(inClass), 1);
				fSeed++;
			}
		}
		else
			err = -1;
	}
	newton_catch(exAbort)
	{
		err = -1;
	}
	end_try;
	return err;
}

bool
CClassInfoRegistryImpl::isProtocolRegistered(const CClassInfo * inClass, bool inSpecific) const
{
	ProtocolEntry * entry = inSpecific ? find(inClass) : satisfy(inClass);
	return entry != NULL;
}


ProtocolEntry *
CClassInfoRegistryImpl::find(const CClassInfo * inClass) const
{
	ProtocolEntry * entry = (ProtocolEntry *)fProtocols->at(0);
	for (ArrayIndex i = 0, count = fProtocols->count(); i < count; ++i, ++entry)
	{
		if (entry->classInfo == inClass)
			return entry;
	}
	return NULL;
}


ProtocolEntry *
CClassInfoRegistryImpl::satisfy(const CClassInfo * inClass) const
{
	ProtocolEntry * satisfiedEntry = NULL;
	newton_try
	{
		const char * interfaceName = inClass->interfaceName();
		const char * implementationName = inClass->implementationName();
		ULong version = inClass->version();

		ProtocolEntry * entry;
		ProtocolEntry reqdProtocol;
		reqdProtocol.intfHash = hashString(interfaceName);
		reqdProtocol.implHash = hashString(implementationName);
		reqdProtocol.version = kAnyVersion;

		for (ArrayIndex count = fProtocols->where(&reqdProtocol), i = count - 1; ((entry = (ProtocolEntry *)fProtocols->at(i)) != NULL) && entry->satisfiesHash(YES, reqdProtocol.intfHash, YES, reqdProtocol.implHash, version); --i)
		{
			if (Satisfies(entry->classInfo, interfaceName, implementationName, version))
			{
				satisfiedEntry = entry;
				break;
			}
		}
	}
	newton_catch(exBusError)
	{ /* just catch it */ }
	end_try;
	return satisfiedEntry;
}


const CClassInfo *
CClassInfoRegistryImpl::satisfy(const char * inInterfaceName, const char * inImplementationName, ULong inVersion) const
{
	const CClassInfo * info = NULL;
	if (inInterfaceName != NULL && inImplementationName != NULL && inVersion == 0)
	{
		// try the cache
		newton_try
		{
			const CClassInfo * cacheEntry;
			for (ArrayIndex i = 0; i < kSatisfyCacheSize; ++i)
			{
				if ((cacheEntry = fCache[i]) != NULL
				&&  strcmp(cacheEntry->interfaceName(), inInterfaceName) == 0
				&&  strcmp(cacheEntry->implementationName(), inImplementationName) == 0)
				{
					info = cacheEntry;
					break;	// original doesn’t break
				}
			}
		}
		newton_catch(exBusError)
		{ /* just catch it */ }
		end_try;
		if (info)
			return info;
	}

	ProtocolEntry * entry;
	ProtocolEntry reqdProtocol;
	reqdProtocol.intfHash = hashString(inInterfaceName);
	reqdProtocol.implHash = hashString(inImplementationName);
	reqdProtocol.version = kAnyVersion;
	bool hasInterface = (inInterfaceName != NULL);
	bool hasImplementation = (inImplementationName != NULL);
	for (ArrayIndex count = fProtocols->where(&reqdProtocol), i = count - 1; ((entry = (ProtocolEntry *)fProtocols->at(i)) != NULL) && entry->satisfiesHash(hasInterface, reqdProtocol.intfHash, hasImplementation, reqdProtocol.implHash, inVersion); --i)
	{
		if (Satisfies(entry->classInfo, inInterfaceName, inImplementationName, inVersion))
		{
			info = entry->classInfo;
			if (inInterfaceName != NULL && inImplementationName != NULL && inVersion == 0)
			{
				// enter it into the cache
				for (ArrayIndex j = kSatisfyCacheSize-1; j > 0; --j)
					fCache[j] = fCache[j-1];
				fCache[0] = info;
			}
			break;
		}
	}
	return info;
}


void
CClassInfoRegistryImpl::invalidateSatisfyCache(void)
{
	for (ArrayIndex i = 0; i < kSatisfyCacheSize; ++i)
		fCache[i] = NULL;
}


//	2.0 calls
const CClassInfo *
CClassInfoRegistryImpl::satisfy(const char * inInterfaceName, const char * inImplementationName, const char * inCapability) const
{
	return satisfy(inInterfaceName, inImplementationName, inCapability, NULL);
}

const CClassInfo *
CClassInfoRegistryImpl::satisfy(const char * inInterfaceName, const char * inImplementationName, const char * inCapabilityKey, const char * inCapabilityValue) const
{
	ProtocolEntry * entry;
	ProtocolEntry reqdProtocol;
	reqdProtocol.intfHash = hashString(inInterfaceName);
	reqdProtocol.implHash = hashString(inImplementationName);
	reqdProtocol.version = kAnyVersion;
	bool hasInterface = (inInterfaceName != NULL);
	bool hasImplementation = (inImplementationName != NULL);
	for (ArrayIndex count = fProtocols->where(&reqdProtocol), i = count - 1; ((entry = (ProtocolEntry *)fProtocols->at(i)) != NULL) && entry->satisfiesHash(hasInterface, reqdProtocol.intfHash, hasImplementation, reqdProtocol.implHash, 0); --i)
	{
		if (Satisfies(entry->classInfo, inInterfaceName, inImplementationName, 0)
		&&  (inCapabilityKey == NULL
		  || entry->satisfiesCapabilities(inCapabilityKey, inCapabilityValue)))
			return entry->classInfo;
	}
	return NULL;
}

const CClassInfo *
CClassInfoRegistryImpl::satisfy(const char * inInterfaceName, const char * inImplementationName, const int inCapabilityKey, const int inCapabilityValue) const
{
	if (inCapabilityKey == 0)
		return satisfy(inInterfaceName, inImplementationName, (const char *)NULL, (const char *)NULL);

	char key[5];
	char value[5];
	memmove(key, &inCapabilityKey, 4);
	key[4] = 0;
	memmove(value, &inCapabilityValue, 4);
	value[4] = 0;
	return satisfy(inInterfaceName, inImplementationName, key, inCapabilityValue == 0 ? NULL : value);
}


const UShort
CClassInfoRegistryImpl::hashString(const char * inStr) const
{
	if (inStr == NULL)
		return 0xFFFF;

	unsigned int ch, hash = 0;
	while ((ch = *inStr++))
	{
		hash = (hash >> 7) | (hash << 25);	// rotate right 7 bits
		hash ^= ch;
	}
	return hash * 0x9E3779B9;
}


void
CClassInfoRegistryImpl::updateInstanceCount(const CClassInfo * inClassInfo, int inAdjustment)
{
	ProtocolEntry * entry = find(inClassInfo);
	if (entry)
		entry->instanceCount += inAdjustment;
}


ArrayIndex
CClassInfoRegistryImpl::getInstanceCount(const CClassInfo * inClassInfo)
{
	ProtocolEntry * entry = find(inClassInfo);
	if (entry)
		return entry->instanceCount;
	return 0;
}


#pragma mark -
/*------------------------------------------------------------------------------
	C C l a s s I n f o
	Meta-information for protocols.
	In other words, every protocol implementation has a CClassInfo object
	describing it.
------------------------------------------------------------------------------*/

size_t
CClassInfo::size(void) const
{
	return PrivateClassInfoSize(this);
}

AllocProcPtr
CClassInfo::allocProc(void) const
{
	return fAllocBranch != 0 ? (AllocProcPtr)((char *)this + fAllocBranch) : NULL;
}

FreeProcPtr
CClassInfo::freeProc(void) const
{
	return fFreeBranch != 0 ? (FreeProcPtr)((char *)this + fFreeBranch) : NULL;
}

EntryProcPtr
CClassInfo::entryProc(void) const
{
	return (EntryProcPtr)((char *)this + fEntryProcDelta);
}

NewtonErr
CClassInfo::registerProtocol(void) const
{
	return gProtocolRegistry->registerProtocol(this);
}

NewtonErr
CClassInfo::deregisterProtocol(void) const
{
	return gProtocolRegistry->deregisterProtocol(this, YES);
}

//typedef void (CProtocol::*NewProcPtr)(void);
typedef void (*NewProcPtr)(CProtocol*);

CProtocol *
CClassInfo::make(void) const
{
	CProtocol * instance;
	AllocProcPtr allocFn;
	if ((allocFn = allocProc()) != NULL)
		instance = (CProtocol *)(*allocFn)();
	else
		instance = (CProtocol *)NewPtr(size());
	if (instance != NULL)
	{
		makeAt(instance);
		NewProcPtr maker = (NewProcPtr)((char *)this + fDefaultNewBranch);
//		(instance->*maker)();
		(*maker)(instance);
		if (gProtocolRegistry)
			gProtocolRegistry->updateInstanceCount(this, 1);
	}
	return instance;
}

void
CClassInfo::makeAt(const void * instance) const
{
	PrivateClassInfoMakeAt(this, instance);
}

void
CClassInfo::destroy(CProtocol * instance) const
{
	// doesn’t call destroy on the instance? *fDefaultDeleteBranch();
	FreeProcPtr freeFn;
	if ((freeFn = freeProc()) != NULL)
		(*freeFn)(instance);
	else
		FreePtr((Ptr)instance);
	if (gProtocolRegistry)
		gProtocolRegistry->updateInstanceCount(this, -1);
}

CodeProcPtr
CClassInfo::selector(void) const
{}

const char *
CClassInfo::getCapability(const char * inKey) const
{
	const char * s;
	for (s = signature(); s != NULL && *s != 0; s = s + strlen(s) + 1)
	{
		if (inKey == NULL || strcmp(inKey, s) == 0)
			return s + strlen(s) + 1;
		s = s + strlen(s) + 1;		
	}
	return NULL;
}

const char *
CClassInfo::getCapability(ULong inKey) const
{
// MAKE_ID_STR(inKey,key)
	char key[5];
#if defined(hasByteSwapping)
	inKey = BYTE_SWAP_LONG(inKey);
#endif
	*(ULong *)key = inKey;
	key[4] = 0;

	for (const char * s = signature(); s != NULL && *s != 0; s = s + strlen(s) + 1)
	{
		if (strcmp(key, s) == 0)
			return s + strlen(s) + 1;
		s = s + strlen(s) + 1;		
	}
	return NULL;
}

bool
CClassInfo::hasInstances(ArrayIndex * outCount) const
{
	*outCount = 0;
	if (gProtocolRegistry)
	{
		*outCount = gProtocolRegistry->getInstanceCount(this);
		if (*outCount != 0)
			return YES;
	}
	return NO;
}

const char *
CClassInfo::implementationName(void) const
{
	return PrivateClassInfoImplementationName(this);
}

const char *
CClassInfo::interfaceName(void) const
{
	return PrivateClassInfoInterfaceName(this);
}

// signature is key-value pairs of null-terminated capability strings
const char *
CClassInfo::signature(void) const
{
	return PrivateClassInfoSignature(this);
}


#pragma mark -

/*--------------------------------------------------------------------------------
	P u b l i c   C   I n t e r f a c e
	Described in the DDK Introduction
--------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------
	Attempt to find the p-class implementation with the named interface and implementation.
	The value NULL is a wild card. (Passing two nils has unpredictable results).
	An instance of the implementation with the highest version number is returned.
	The instance’s make(void) method is called before the instance is returned.
	If there is no matching implementation, NULL is returned.
	Args:		inInterface			string naming a p-class interface
				inImplementation	string naming a p-class implementation
	Return:	a protocol instance
--------------------------------------------------------------------------------*/

CProtocol *
MakeByName(const char * inInterface, const char * inImplementation)
{
	const CClassInfo * classInfo;
	if ((classInfo = gProtocolRegistry->satisfy(inInterface, inImplementation, (ULong)0)) != NULL)
		return classInfo->make();
	return NULL;
}


/*--------------------------------------------------------------------------------
	The same as the above MakeByName, except that inVersion specifies an exact
	p-class version to instantiate.
	Args:		inInterface			string naming a p-class interface
				inImplementation	string naming a p-class implementation
				inVersion			version number
	Return:	a protocol instance
--------------------------------------------------------------------------------*/

CProtocol *
MakeByName(const char * inInterface, const char * inImplementation, ULong inVersion)
{
	const CClassInfo * classInfo;
	if ((classInfo = gProtocolRegistry->satisfy(inInterface, inImplementation, inVersion)) != NULL)
		return classInfo->make();
	return NULL;
}


/*--------------------------------------------------------------------------------
	Similar to the first MakeByName, except that the p-class must have the specified
	capability. The capability’s value doesn’t matter. The instance created is
	that of the implementation with the specified capability with the highest
	version number.
	Args:		inInterface			string naming a p-class interface
				inImplementation	string naming a p-class implementation
				inCapability		string naming a p-class capability
	Return:	a protocol instance
--------------------------------------------------------------------------------*/

CProtocol *
MakeByName(const char * inInterface, const char * inImplementation, const char * inCapability)
{
	const CClassInfo * classInfo;
	if ((classInfo = gProtocolRegistry->satisfy(inInterface, inImplementation, inCapability)) != NULL)
		return classInfo->make();
	return NULL;
}


/*--------------------------------------------------------------------------------
	Similar to the first MakeByName, except that the instance’s make(void) method
	is not called.
	Args:		inInterface			string naming a p-class interface
				inImplementation	string naming a p-class implementation
	Return:	a protocol instance
--------------------------------------------------------------------------------*/

CProtocol *
AllocInstanceByName(const char * inInterface, const char * inImplementation)
{
	CProtocol * instance = NULL;
	const CClassInfo * classInfo;
	if (gProtocolRegistry != NULL
	&&  (classInfo = gProtocolRegistry->satisfy(inInterface, inImplementation, (ULong)0)) != NULL)
	{
		AllocProcPtr allocFn;
		if ((allocFn = classInfo->allocProc()) != NULL)
			instance = (CProtocol *)(*allocFn)();
		else
		{
			size_t instanceSize = classInfo->size();
			instance = (CProtocol *)NewPtr(instanceSize);
		}
		if (instance != NULL)
		{
			classInfo->makeAt(instance);
			gProtocolRegistry->updateInstanceCount(classInfo, 1);
		}
	}
	return instance;
}


/*--------------------------------------------------------------------------------
	Free an instance of a p-class. The instance’s destroy(void) method is not called.
	This is a fairly low-level operation.
	You shouldn’t have to use this method.
	Use destroy() methods instead.
	Args:		instance		a protocol instance
	Return:	--
--------------------------------------------------------------------------------*/

void
FreeInstance(CProtocol * instance)
{
	const CClassInfo * classInfo;
	if ((classInfo = instance->classInfo()) != NULL)
	{
		FreeProcPtr freeFn;
		if ((freeFn = classInfo->freeProc()) != NULL)
			(*freeFn)(instance);
		else
			FreePtr((Ptr)instance);
		if (gProtocolRegistry != NULL)
			gProtocolRegistry->updateInstanceCount(classInfo, -1);
	}
}


/*--------------------------------------------------------------------------------
	Return the class information for an implementation (or NULL, if no match is found).
	If inVersion is nonzero, only the implementation with that exact version number
	is returned. If inVersion is zero, the implementation with the highest version
	number is returned.
	Args:		inInterface			string naming a p-class interface
				inImplementation	string naming a p-class implementation
				inVersion			version number
	Return:	class information
--------------------------------------------------------------------------------*/

const CClassInfo *
ClassInfoByName(const char * inInterface, const char * inImplementation, ULong inVersion)
{
	return gProtocolRegistry->satisfy(inInterface, inImplementation, inVersion);
}
