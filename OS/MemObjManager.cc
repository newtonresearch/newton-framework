/*
	File:		MemObjManager.cc

	Contains:	Memory object management.

	Written by:	Newton Research Group.
*/

#include "MemObjManager.h"
#include "MemMgr.h"
#include "UserTasks.h"
#include "VirtualMemory.h"
#include "OSErrors.h"

extern MemInit gDataAreaTable[2];


/*------------------------------------------------------------------------------
	Message (mapped onto task-switched global params) used to talk to the kernel.
------------------------------------------------------------------------------*/

struct MemObjMessage
{
	int	selector;

	union
	{
		struct
		{
			ULong			name;
			ArrayIndex	index;
			bool			isManager;
			bool			isOK;
		} info;

		struct
		{
			MemObjType	type;
			union {
			ArrayIndex	index;
			ULong			name;
			};
			ULong			object;	// actually entire object
		} entry;
	};
};

//	Selector
enum
{
	kGetDomainInfoByIndex,
	kGetDomainInfoByName,
	kGetEnvInfo,
	kGetEnvDomainName,
	kGetEntryByIndex,
	kGetEntryByName,
	kSetEntryByIndex,
	kSetEntryByName
};


/*------------------------------------------------------------------------------
	M e m O b j   D a t a b a s e
------------------------------------------------------------------------------*/

ObjectId				gKernelDomainId;			// 0C101170
void *				gMemObjHeap;				// 0C101174
/* the MemObjHeap is arranged:
	MemObjDBIndex  gMemObjDBIndexTable[kNumOfMemObjTypes];	// 0C108000
	char				gMemObjDB[600];									// 0C108030
*/

const DomainMemInfo *	gDomainTable;		// 0C1011B8 points to…

const DomainMemInfo  g1MegDomainTable[] =	// 0C1011BC domain setups for small (<1MByte) RAM
{	//	tag		address		size			frames		handles		flags
	// kernel variables & heap
	{  'krnl',	0x0C100000,	0x00100000,	0x00000000, 0x00000000,	0x00 },	// 1MB
	// kernel stacks
	{  'kstk',	0x0C200000, 0x00200000,	0x00080000, 0x00000000,	0x04 },	// 2MB
	// user heaps
	{  'user',	0x0C400000, 0x00600000,	0x00080000, 0x00060000,	0x04 },	// 6MB
	// protected packages
	{  'prot',	0x0CA00000, 0x00200000,	0x00080000, 0x00000000,	0x07 },	// 2MB
	// ram store
	{  'rams',	0x0CC00000, 0x00800000,	0x00000000, 0xFFFFFFFF,	0x27 },
	// ROM
	{  'romc',	0x60000000, 0x08000000,	0x00000000, 0x00000000,	0x06 },
	{  'ccl0',	0x70000000, 0x20000000,	0x00000000, 0x00000000,	0x00 },
	{  'csk0',	0x90000000, 0x40000000,	0x00000000, 0x00000000,	0x00 },
	// Licensee Domain
	{  'LicD',	0xE0000000, 0x08000000,	0x00000000, 0x00000000,	0x00 },
	{  0  }
};

const DomainMemInfo  g4MegDomainTable[] =	// 0C1012AC domain setups for 4MB RAM
{	//	tag		address		size			frames		handles		flags
	// kernel variables & heap
	{  'krnl',	0x0C100000,	0x00100000,	0x00000000, 0x00000000,	0x00 },	// 1MB
	// kernel stacks
	{  'kstk',	0x0C200000, 0x00400000,	0x00100000, 0x00000000,	0x04 },	// 4MB
	// user heaps
	{  'user',	0x0C600000, 0x01000000,	0x00380000, 0x00200000,	0x04 },	// 16MB
	// protected packages
	{  'prot',	0x0D600000, 0x00800000,	0x00100000, 0x00000000,	0x07 },	// 8MB
	// ram store
	{  'rams',	0x0DE00000, 0x00800000,	0x00000000, 0xFFFFFFFF,	0x27 },
	// ROM
	{  'romc',	0x60000000, 0x08000000,	0x00000000, 0x00000000,	0x06 },
	{  'ccl0',	0x70000000, 0x20000000,	0x00000000, 0x00000000,	0x00 },
	{  'csk0',	0x90000000, 0x40000000,	0x00000000, 0x00000000,	0x00 },
	// Licensee Domain
	{  'LicD',	0xE0000000, 0x08000000,	0x00000000, 0x00000000,	0x00 },
	{  0  }
};


struct EnvironmentMemInfo
{
	ULong		tag;
	ULong		heap;
	ULong		heapDomain;
	ULong		stackDomain;
	const ULong *  clientList;
	const ULong *  managerList;
};

const ULong gKrnlClientList[] =  {	'user', 'krnl', 'kstk', 0 };	// 0C1013B4…
const ULong gUserClientList[] =  {	'user', 'krnl', 'kstk', 'prot', 'rams', 'csk0', 'ccl0', 'romc', 0 };
const ULong gRamsClientList[] =  {	'kstk', 0 };
const ULong gRamsManagerList[] =  {	'rams', 0 };
const ULong gProtClientList[] =  {	'user', 'krnl', 'kstk', 'csk0', 'ccl0', 'romc', 0 };
const ULong gProtManagerList[] =  {	'prot', 0 };
const ULong gROMClientList[] =  {	'user', 'prot', 'rams', 'ccl0', 'kstk', 0 };
const ULong gROMManagerList[] =  {	'romc', 0 };						// …0C101434

const EnvironmentMemInfo  gEnvTable[] =		// 0C10143C environments
{
	{  'krnl',
		'krnl', 'kstk', 'kstk',
		gKrnlClientList, NULL },
	{  'ksrv',
		'kstk', 'kstk', 'kstk',
		gKrnlClientList, NULL },
	{  'cdfm',
		'kstk', 'kstk', 'kstk',
		gKrnlClientList, NULL },
	{  'user',
		'user', 'user', 'user',
		gUserClientList, NULL },
	{  'prot',
		'user', 'user', 'user',
		gProtClientList, gProtManagerList },
	{  'rams',
		'rams', 'kstk', 'kstk',
		gRamsClientList, gRamsManagerList },
	{  'romc',
		'kstk', 'user', 'user',
		gROMClientList, gROMManagerList },
	{  0  }
};


/*------------------------------------------------------------------------------
	Caclculate the size of the MemObj database.
	The MemObj database contains four types of DB entry:
		CDomainDBEntry
		CHeapDBEntry
		CPersistentDBEntry
		CEnvironmentDBEntry
	Args:		outSize
	Return:  --
------------------------------------------------------------------------------*/

void
ComputeMemObjDatabaseSize(size_t * outSize)
{
	size_t	dbSize = 0;

	const DomainMemInfo *	memInfo;
	for (memInfo = gDomainTable; memInfo->tag != 0; memInfo++)
		dbSize += sizeof(CDomainDBEntry);

	for (memInfo = gDomainTable; memInfo->tag != 0; memInfo++)
		if ((memInfo->framesHeapSize != 0 || memInfo->handlesHeapSize != 0)
		 && (memInfo->flags & 0x01) == 0)
			dbSize += sizeof(CHeapDBEntry);
		else
			dbSize += sizeof(CPersistentDBEntry);

	dbSize += 10 * sizeof(CPersistentDBEntry);

	const EnvironmentMemInfo *	envInfo;
	for (envInfo = gEnvTable; envInfo->tag; envInfo++)
		dbSize += sizeof(CEnvironmentDBEntry);

	dbSize += (sizeof(MemObjDBIndex) * kNumOfMemObjTypes);	// index table
	*outSize = dbSize;
}


/*------------------------------------------------------------------------------
	Initialize the MemObj database.
	The gMemObjHeap global pointer points to an array of MemObjDBIndex, one for
	each of the four types of DB entry:
		CDomainDBEntry
		CHeapDBEntry
		CPersistentDBEntry
		CEnvironmentDBEntry
	The MemObjDBIndex points to an array of CMemDBEntry for that DB entry type.
	Args:		--
	Return:  --
------------------------------------------------------------------------------*/

void
BuildMemObjDatabase(void)
{
	MemObjDBIndex * memObjDBIndexTable = (MemObjDBIndex *)gMemObjHeap;
	gGlobalsThatLiveAcrossReboot.fMemObjDBIndexTable = memObjDBIndexTable;

	ArrayIndex	entryCount;
	const DomainMemInfo *	memInfo;

// set up domain index entries
	CDomainDBEntry *  domainEntry = (CDomainDBEntry *)(memObjDBIndexTable + kNumOfMemObjTypes);
	memObjDBIndexTable[kMemObjDomain].entry = domainEntry;
	entryCount = 0;
	for (memInfo = gDomainTable; memInfo->tag != 0; memInfo++)
	{
		domainEntry->fName = memInfo->tag;
		domainEntry->fId = kNoId;
		domainEntry++;
		entryCount++;
	}
	memObjDBIndexTable[kMemObjDomain].count = entryCount;
	memObjDBIndexTable[kMemObjDomain].size = sizeof(CDomainDBEntry);

// set up heap index entries
	CHeapDBEntry *  heapEntry = (CHeapDBEntry *)domainEntry;
	memObjDBIndexTable[kMemObjHeap].entry = heapEntry;
	entryCount = 0;
	for (memInfo = gDomainTable; memInfo->tag != 0; memInfo++)
	{
		if ((memInfo->framesHeapSize != 0 || memInfo->handlesHeapSize != 0)
		 && (memInfo->flags & 0x01) == 0)
		{
			heapEntry->fName = memInfo->tag;
			heapEntry->fHeap = NULL;
			heapEntry++;
			entryCount++;
		}
	}
	memObjDBIndexTable[kMemObjHeap].count = entryCount;
	memObjDBIndexTable[kMemObjHeap].size = sizeof(CHeapDBEntry);

// set up persistent index entries
	ArrayIndex index = 0;
	CPersistentDBEntry *  persistentEntry = (CPersistentDBEntry *)heapEntry;
	memObjDBIndexTable[kMemObjPersistent].entry = persistentEntry;
	entryCount = 0;
	for (memInfo = gDomainTable; memInfo->tag; memInfo++)
	{
		if ((memInfo->framesHeapSize != 0 || memInfo->handlesHeapSize != 0)
		 && (memInfo->flags & 0x01) != 0)
		{
			persistentEntry->init(memInfo->tag, true, index++);
			persistentEntry++;
			entryCount++;
		}
	}

// add another ten empty persistent index entries
	for (index = 0; index < 10; index++)
	{
		persistentEntry->init('emty');
		persistentEntry->fFlags &= ~0x40;
		persistentEntry++;
		entryCount++;
	}
	memObjDBIndexTable[kMemObjPersistent].count = entryCount;
	memObjDBIndexTable[kMemObjPersistent].size = sizeof(CPersistentDBEntry);

// set up environment index entries
	const EnvironmentMemInfo * environmentInfo;
	CEnvironmentDBEntry *		environmentEntry = (CEnvironmentDBEntry *)persistentEntry;
	memObjDBIndexTable[kMemObjEnvironment].entry = environmentEntry;
	entryCount = 0;
	for (environmentInfo = gEnvTable; environmentInfo->tag != 0; environmentInfo++)
	{
		environmentEntry->fName = environmentInfo->tag;
		environmentEntry->fId = kNoId;
		environmentEntry++;
		entryCount++;
	}
	memObjDBIndexTable[kMemObjEnvironment].count = entryCount;
	memObjDBIndexTable[kMemObjEnvironment].size = sizeof(CEnvironmentDBEntry);
}


/*------------------------------------------------------------------------------
	Initialize the kernel domain and environment.
	Args:		--
	Return:  --
------------------------------------------------------------------------------*/

void
InitKernelDomainAndEnvironment(void)
{
	ObjectId	environmentId;
	ObjectId	domainId;

	CEnvironment * env = new CEnvironment;
	env->init(gKernelHeap);
	RegisterObject(env, kEnvironmentType, kSystemId, &environmentId);

	CDomain * domain = new CDomain();
	RegisterObject(domain, kDomainType, kSystemId, &domainId);

	CDomainInfo info;
	MemObjManager::getDomainInfoByName('krnl', &info);
	domain->initWithDomainNumber(kNoId, info.base(), info.size(), kKernelDomainHeapDomainNumber);
	env->add(domain, false, false, false);

	MemObjManager::registerEnvironmentId('krnl', environmentId);
	MemObjManager::registerDomainId('krnl', domainId);
	gKernelDomainId = domainId;
}


/*------------------------------------------------------------------------------
	Supervisor mode dispatcher to the mem obj manager.
	Callback from SWI.
	Args:		--
	Return:  error code
------------------------------------------------------------------------------*/

NewtonErr
PrimGetMemObjInfo(void)
{
	NewtonErr			err;
	MemObjMessage *	msg = (MemObjMessage *)TaskSwitchedGlobals()->fParams;

	switch (msg->selector)
	{
//	info getters
	case kGetDomainInfoByIndex:
		err = MemObjManager::primGetDomainInfo(msg->info.index, (CDomainInfo *)msg);
		break;
	case kGetDomainInfoByName:
		err = MemObjManager::primGetDomainInfoByName(msg->info.name, (CDomainInfo *)msg);
		break;
	case kGetEnvInfo:
		err = MemObjManager::primGetEnvironmentInfo(msg->info.index, (CEnvironmentInfo *)msg);
		break;
	case kGetEnvDomainName:
		err = MemObjManager::primGetEnvDomainName(msg->info.name, msg->info.index, &msg->info.name, &msg->info.isManager, &msg->info.isOK);
		break;

//	entry getters
	case kGetEntryByIndex:
		err = MemObjManager::primGetEntryByIndex(msg->entry.type, msg->entry.index, &msg->entry.object);
		break;
	case kGetEntryByName:
		err = MemObjManager::primGetEntryByName(msg->entry.type, msg->entry.name, &msg->entry.object);
		break;

//	entry setters
	case kSetEntryByIndex:
		err = MemObjManager::primSetEntryByIndex(msg->entry.type, msg->entry.index, &msg->entry.object);
		break;
	case kSetEntryByName:
		err = MemObjManager::primSetEntryByName(msg->entry.type, msg->entry.name, &msg->entry.object);
		break;

	default:
		err = kOSErrBadParameters;
		break;
	}
	return err;
}

#pragma mark -

/*------------------------------------------------------------------------------
	D o m a i n   m a n a g e m e n t
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
	Register a domain.
	Args:		inName			domain name
				inId				its id
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
MemObjManager::registerDomainId(ULong inName, ObjectId inId)
{
	NewtonErr	err;
	CDomainDBEntry	entry;

	if ((err = findEntryByName(kMemObjDomain, inName, &entry)) == noErr)
	{
		entry.fId = inId;
		err = registerEntryByName(kMemObjDomain, inName, &entry);
	}

	return err;
}


/*------------------------------------------------------------------------------
	Find a previously registered domain.
	Args:		inName			domain name
				outId				its id
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
MemObjManager::findDomainId(ULong inName, ObjectId * outId)
{
	NewtonErr	err;
	CDomainDBEntry	entry;

	if ((err = findEntryByName(kMemObjDomain, inName, &entry)) == noErr)
		*outId = entry.fId;

	return err;
}


/*------------------------------------------------------------------------------
	Get domain info.
	Args:		inName			domain index
				outInfo			pointer to info struct
				outError			result
	Return:	true if successful; if false then outError is set appropriately
------------------------------------------------------------------------------*/

bool
MemObjManager::getDomainInfo(ArrayIndex index, CDomainInfo * outInfo, NewtonErr * outError)
{
	NewtonErr	err;

	if (IsSuperMode())
		err = primGetDomainInfo(index, outInfo);

	else
	{
		MemObjMessage *	msg = (MemObjMessage *)TaskSwitchedGlobals()->fParams;
		msg->selector = kGetDomainInfoByIndex;
		msg->info.index = index;
		if ((err = GenericSWI(kMemObjManagerDispatch)) == noErr)
			*outInfo = *(CDomainInfo *)msg;
	}

	if (err != noErr)
	{
		if (err == kOSErrItemNotFound)
			err = noErr;
		*outError = err;
		return false;
	}

	return true;
}


NewtonErr
MemObjManager::primGetDomainInfo(ArrayIndex index, CDomainInfo * outInfo)
{
	const DomainMemInfo *	memInfo;
	ArrayIndex			entryIndex = 0;
	for (memInfo = gDomainTable; memInfo->tag != 0; memInfo++, entryIndex++)
	{
		if (index == entryIndex)
		{
			const MemInit *  map;
			for (map = gDataAreaTable; map->tag; map++)
				if (map->tag == memInfo->tag)
					break;

			outInfo->initDomainInfo(memInfo->tag, 0, memInfo->addr, memInfo->size);
			outInfo->initHeapInfo(memInfo->framesHeapSize, memInfo->handlesHeapSize, memInfo->flags);

			if (map->tag == 0)
				outInfo->initGlobalInfo(0, 0, 0, 0);
			else
				outInfo->initGlobalInfo(map->dataStart, map->initStart, map->dataSize, map->zeroSize);

			return noErr;
		}
	}

	return kOSErrItemNotFound;
}


/*------------------------------------------------------------------------------
	Get domain info.
	Args:		inName			domain name
				outInfo			pointer to info struct
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
MemObjManager::getDomainInfoByName(ULong inName, CDomainInfo * outInfo)
{
	NewtonErr	err;

	if (IsSuperMode())
		err = primGetDomainInfoByName(inName, outInfo);

	else
	{
		MemObjMessage *	msg = (MemObjMessage *)TaskSwitchedGlobals()->fParams;
		msg->selector = kGetDomainInfoByName;
		msg->info.name = inName;
		if ((err = GenericSWI(kMemObjManagerDispatch)) == noErr)
			*outInfo = *(CDomainInfo *)msg;
	}

	return err;
}

NewtonErr
MemObjManager::primGetDomainInfoByName(ULong inName, CDomainInfo * outInfo)
{
	CDomainInfo	info;

	for (ArrayIndex i = 0; primGetDomainInfo(i, &info) == noErr; ++i)
	{
		if (inName == info.name())
		{
			*outInfo = info;
			return noErr;
		}
	}

	return kOSErrItemNotFound;
}

#pragma mark -

/*------------------------------------------------------------------------------
	E n v i r o n m e n t   m a n a g e m e n t
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
	Register an environment.
	Args:		inName			environment name
				inId				its id
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
MemObjManager::registerEnvironmentId(ULong inName, ObjectId inId)
{
	NewtonErr	err;
	CEnvironmentDBEntry	entry;

	if ((err = findEntryByName(kMemObjEnvironment, inName, &entry)) == noErr)
	{
		entry.fId = inId;
		err = registerEntryByName(kMemObjEnvironment, inName, &entry);
	}

	return err;
}


/*------------------------------------------------------------------------------
	Find a previously registered environment.
	Args:		inName			environment name
				outId				its id
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
MemObjManager::findEnvironmentId(ULong inName, ObjectId * outId)
{
	NewtonErr	err;
	CEnvironmentDBEntry	entry;

	if ((err = findEntryByName(kMemObjEnvironment, inName, &entry)) == noErr)
		*outId = entry.fId;

	return err;
}


bool
MemObjManager::getEnvironmentInfo(ArrayIndex index, CEnvironmentInfo * outInfo, NewtonErr * outError)
{
	NewtonErr	err;

	if (IsSuperMode())
		err = primGetEnvironmentInfo(index, outInfo);

	else
	{
		MemObjMessage *	msg = (MemObjMessage *)TaskSwitchedGlobals()->fParams;
		msg->selector = kGetEnvInfo;
		msg->info.index = index;
		if ((err = GenericSWI(kMemObjManagerDispatch)) == noErr)
			*outInfo = *(CEnvironmentInfo *)msg;
	}

	if (err != noErr)
	{
		if (err == kOSErrItemNotFound)
			err = noErr;
		*outError = err;
		return false;
	}

	return true;
}


NewtonErr
MemObjManager::primGetEnvironmentInfo(ArrayIndex index, CEnvironmentInfo * outInfo)
{
	const EnvironmentMemInfo *		memInfo;
	ArrayIndex							entryIndex = 0;
	for (memInfo = gEnvTable; memInfo->tag != 0; memInfo++)
	{
		if (index == entryIndex)
		{
			outInfo->init(memInfo->tag, 0, memInfo->heap, memInfo->heapDomain, memInfo->stackDomain);
			return noErr;
		}
		entryIndex++;
	}

	return kOSErrItemNotFound;
}


/*------------------------------------------------------------------------------
	Get the name of a domain in an environment?
	Args:		inName			environment name
				outId				its id
	Return:	error code
------------------------------------------------------------------------------*/

bool
MemObjManager::getEnvDomainName(ULong inName, ArrayIndex index, ULong * outName, bool * outIsManager, NewtonErr * outError)
{
	NewtonErr	err;

	if (IsSuperMode())
	{
		bool	isOK;
		err = primGetEnvDomainName(inName, index, outName, outIsManager, &isOK);
		if (!isOK)
			*outError = err;
		return isOK;
	}

	else
	{
		MemObjMessage *	msg = (MemObjMessage *)TaskSwitchedGlobals()->fParams;
		msg->selector = kGetEnvDomainName;
		msg->info.name = inName;
		msg->info.index = index;
		if ((err = GenericSWI(kMemObjManagerDispatch)) == noErr)
		{
			if (msg->info.isOK)
			{
				*outName = msg->info.name;
				*outIsManager = msg->info.isManager;
			}
		}
		if (!msg->info.isOK)
			*outError = err;
		return msg->info.isOK;
	}
}


NewtonErr
MemObjManager::primGetEnvDomainName(ULong inEnvName, ArrayIndex index, ULong * outName, bool * outIsManager, bool * outOK)
{
	const EnvironmentMemInfo * memInfo;
	for (memInfo = gEnvTable; memInfo->tag != 0; memInfo++)
	{
		if (memInfo->tag == inEnvName)
		{
		// found the environment name
			ULong				tagIndex = 0;
			const ULong *  tagPtr;

			if ((tagPtr = memInfo->clientList) != NULL)
			{
				for ( ; *tagPtr != 0; tagPtr++, tagIndex++)
				{
					if (tagIndex == index)
					{
						*outName = *tagPtr;
						*outIsManager = false;
						*outOK = true;
						return noErr;
					}
				}
			}

			if ((tagPtr = memInfo->managerList) != NULL)
			{
				for ( ; *tagPtr != 0; tagPtr++, tagIndex++)
				{
					if (tagIndex == index)
					{
						*outName = *tagPtr;
						*outIsManager = true;
						*outOK = true;
						return noErr;
					}
				}
			}

			*outOK = false;
			return noErr;
		}
	}
	*outOK = false;
	return kOSErrItemNotFound;
}

#pragma mark -

/*------------------------------------------------------------------------------
	H e a p   M a n a g e m e n t
------------------------------------------------------------------------------*/

NewtonErr
MemObjManager::registerHeapRef(ULong inName, Heap inHeap)
{
	NewtonErr	err;
	CHeapDBEntry	entry;

	if ((err = findEntryByName(kMemObjHeap, inName, &entry)) == noErr)
	{
		entry.fHeap = inHeap;
		err = registerEntryByName(kMemObjHeap, inName, &entry);
	}

	return err;
}

NewtonErr
MemObjManager::findHeapRef(ULong inName, Heap * outHeap)
{
	NewtonErr	err;
	CHeapDBEntry	entry;

	if ((err = findEntryByName(kMemObjHeap, inName, &entry)) == noErr)
		*outHeap = entry.fHeap;
	else
	{
		CPersistentDBEntry	persistentEntry;
		if ((err = findEntryByName(kMemObjPersistent, inName, &persistentEntry)) == noErr)
			*outHeap = persistentEntry.fHeap;
	}

	return err;
}


#pragma mark -

/*------------------------------------------------------------------------------
	P e r s i s t e n t   o b j e c t   m a n a g e m e n t
------------------------------------------------------------------------------*/

NewtonErr
MemObjManager::registerPersistentNewEntry(ULong inName, CPersistentDBEntry * inEntry)
{
	CPersistentDBEntry	entry, * newEntry;

	if (findEntryByName(kMemObjPersistent, inName, &entry) == noErr)
		return kOSErrDuplicateObject;

	if ((newEntry = (CPersistentDBEntry *)entryLocByName(kMemObjPersistent, 'emty')) == NULL)
		return kOSErrCouldNotCreateObject;

	copyObject(kMemObjPersistent, newEntry, inEntry);
	return noErr;
}


NewtonErr
MemObjManager::deregisterPersistentEntry(ULong inName)
{
	CPersistentDBEntry	emptyEntry, * entry;

	emptyEntry.init('emty');
	emptyEntry.fFlags &= ~0x40;

	if ((entry = (CPersistentDBEntry *)entryLocByName(kMemObjPersistent, inName)) == NULL
	 || MASKTEST(entry->fFlags, 0x40))
		return kOSErrItemNotFound;

	copyObject(kMemObjPersistent, entry, &emptyEntry);
	return noErr;
}


bool
MemObjManager::getPersistentRef(ArrayIndex index, CPersistentDBEntry ** outEntry, NewtonErr * outError)
{
	CPersistentDBEntry *	entry = (CPersistentDBEntry *)entryLocByIndex(kMemObjPersistent, index);
	if (entry == NULL)
	{
		*outError = noErr;
		return false;
	}

	*outEntry = entry;
	return true;
}


#pragma mark -

/*------------------------------------------------------------------------------
	O b j e c t   D a t a b a s e   M a n a g e m e n t
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
	Register a named object.
	Args:		inType			the type of object
				inName			its name
				ioEntry			the object
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
MemObjManager::registerEntryByName(MemObjType inType, ULong inName, void * inEntry)
{
	NewtonErr	err;

	if (IsSuperMode())
		err = primSetEntryByName(inType, inName, inEntry);

	else
	{
		MemObjMessage *	msg = (MemObjMessage *)TaskSwitchedGlobals()->fParams;
		msg->selector = kSetEntryByName;
		msg->entry.type = inType;
		msg->entry.name = inName;
		copyObject(inType, &msg->entry.object, inEntry);
		err = GenericSWI(kMemObjManagerDispatch);
	}

	return err;
}


/*------------------------------------------------------------------------------
	Find an object entry by name.
	Args:		inType			the type of object
				inName			its name
				outObj			where to copy the object
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
MemObjManager::findEntryByName(MemObjType inType, ULong inName, void * outObj)
{
	NewtonErr	err;

	if (IsSuperMode())
		err = primGetEntryByName(inType, inName, outObj);

	else
	{
		MemObjMessage *	msg = (MemObjMessage *)TaskSwitchedGlobals()->fParams;
		msg->selector = kGetEntryByName;
		msg->entry.type = inType;
		msg->entry.name = inName;
		if ((err = GenericSWI(kMemObjManagerDispatch)) == noErr)
			copyObject(inType, outObj, &msg->entry.object);
	}

	return err;
}


NewtonErr
MemObjManager::primGetEntryByName(MemObjType inType, ULong inName, void * outObj)
{
	CMemDBEntry *	entry;

	if ((entry = entryLocByName(inType, inName)) == NULL)
		return kOSErrItemNotFound;

	copyObject(inType, outObj, entry);
	return noErr;
}


NewtonErr
MemObjManager::primSetEntryByName(MemObjType inType, ULong inName, void * inObj)
{
	CMemDBEntry *	entry;

	if ((entry = entryLocByName(inType, inName)) == NULL)
		return kOSErrItemNotFound;

	copyObject(inType, entry, inObj);
	return noErr;
}


/*------------------------------------------------------------------------------
	Return a pointer to an object given its name.
	Args:		inType			the object type
				inName			its name
	Return:	pointer to object
------------------------------------------------------------------------------*/

CMemDBEntry *
MemObjManager::entryLocByName(MemObjType inType, ULong inName)
{
	if (inType < kNumOfMemObjTypes)
	{
		MemObjDBIndex *	objInfo = gGlobalsThatLiveAcrossReboot.fMemObjDBIndexTable + inType;
		CMemDBEntry *		p = objInfo->entry;
		for (ArrayIndex i = 0; i < objInfo->count; i++, p = (CMemDBEntry *)((Ptr)p + objInfo->size))
			if (p->fName == inName)
				// we found it
				return p;
	}

	// if we get here we didn’t find it
	return NULL;
}


/*------------------------------------------------------------------------------
	Find an object entry by its index into the database.
	Args:		inType			the type of object
				index				its index
				outObj			where to copy the object
				outError			error code
	Return:	true if found
------------------------------------------------------------------------------*/

bool
MemObjManager::findEntryByIndex(MemObjType inType, ArrayIndex index, void * outObj, NewtonErr * outError)
{
	NewtonErr	err;

	if (IsSuperMode())
	{
		if ((err = primGetEntryByIndex(inType, index, outObj)) != noErr)
		{
			*outError = err;
			return false;
		}
	}

	else
	{
		MemObjMessage *	msg = (MemObjMessage *)TaskSwitchedGlobals()->fParams;
		msg->selector = kGetEntryByIndex;
		msg->entry.type = inType;
		msg->entry.index = index;
		if ((err = GenericSWI(kMemObjManagerDispatch)) != noErr)
		{
			if (err == kOSErrItemNotFound)
				err = noErr;
			*outError = err;
			return false;
		}
		copyObject(inType, outObj, &msg->entry.object);
	}

	return true;
}


NewtonErr
MemObjManager::primGetEntryByIndex(MemObjType inType, ArrayIndex index, void * outObj)
{
	void *	entry;

	if ((entry = entryLocByIndex(inType, index)) == NULL)
		return kOSErrItemNotFound;

	copyObject(inType, outObj, entry);
	return noErr;
}


NewtonErr
MemObjManager::primSetEntryByIndex(MemObjType inType, ArrayIndex index, void * inObj)
{
	void *	entry;

	if ((entry = entryLocByIndex(inType, index)) == NULL)
		return kOSErrItemNotFound;

	copyObject(inType, entry, inObj);
	return noErr;
}


/*------------------------------------------------------------------------------
	Return a pointer to an object given its index into the database.
	Args:		inType			the object type
				index				its index
	Return:	pointer to object
------------------------------------------------------------------------------*/

CMemDBEntry *
MemObjManager::entryLocByIndex(MemObjType inType, ArrayIndex index)
{
	if (inType < kNumOfMemObjTypes)
	{
		MemObjDBIndex *	objInfo = gGlobalsThatLiveAcrossReboot.fMemObjDBIndexTable + inType;
		if (index < objInfo->count)
			return (CMemDBEntry *)((Ptr)objInfo->entry + (index * objInfo->size));
	}

	// if we get here we didn’t find it
	return NULL;
}


/*------------------------------------------------------------------------------
	Copy an object.
	Args:		inType			the object type - we need to know its size
				outObj			where to copy it to
				inObj				the object to be copied
	Return:	--
------------------------------------------------------------------------------*/

void
MemObjManager::copyObject(MemObjType inType, void * outObj, void * inObj)
{
	switch (inType)
	{
	case kMemObjDomain:
		memmove(outObj, inObj, sizeof(CDomainDBEntry));
		break;
	case kMemObjEnvironment:
		memmove(outObj, inObj, sizeof(CEnvironmentDBEntry));
		break;
	case kMemObjHeap:
		memmove(outObj, inObj, sizeof(CHeapDBEntry));
		break;
	case kMemObjPersistent:
		memmove(outObj, inObj, sizeof(CPersistentDBEntry));
		break;
	}
}

#pragma mark -

/*------------------------------------------------------------------------------
	C P e r s i s t e n t D B E n t r y
------------------------------------------------------------------------------*/

void
CPersistentDBEntry::init(ULong inName, bool inArg2, ArrayIndex inDomainIndex)
{
	if (inArg2)
	{
		NewtonErr		err;
		CDomainDBEntry	domain;

		fHeap = NULL;
		fName = inName;
		fBase = 0;
		fSize	= 0;
	//	f10 = 0;
		fPages.init(20);	// 20 returned from a function - getSizeOf()? offsetof()?

		fFlags = MASKCLEAR(fFlags, 0xFF) | 0x80 | ((inDomainIndex & 0xFF) << 8) | 0x40;
		bool	isFound = MemObjManager::findEntryByIndex(kMemObjDomain, inDomainIndex, &domain, &err);
		fDomainName = isFound ? domain.fName : kUndefined;
	}
	fFlags &= 0xFFFF;
}

