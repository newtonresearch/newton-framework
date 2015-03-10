/*
	File:		PackageManager.cc

	Contains:	Package manager implementation.

	Written by:	Newton Research Group
*/

#include "MemObjManager.h"
#include "AppWorld.h"
#include "NameServer.h"
#include "PackageManager.h"
#include "PackageEvents.h"
#include "Protocols.h"
#include "UStringUtils.h"


/* -----------------------------------------------------------------------------
	T y p e s
----------------------------------------------------------------------------- */

// Packages are registered using the following information:
struct RegistryInfo
{
		RegistryInfo(ULong inArg1, ULong inArg2);

	ULong		f00;
	ULong		f04;
};


// The following information is held about a Package:
struct PackageInfo
{
	ULong			f00;
	UniChar *	f1C;
// size +30
};


/* -----------------------------------------------------------------------------
	D a t a
----------------------------------------------------------------------------- */

CULockingSemaphore *	gPackageSemaphore;


/*------------------------------------------------------------------------------
	P u b l i c   I n t e r f a c e
------------------------------------------------------------------------------*/

NewtonErr
InitializePackageManager(ObjectId inEnvironmentId)
{
	CPackageManager	pm;
	return pm.init('pckm', YES, kSpawnedTaskStackSize, kUserTaskPriority, inEnvironmentId);
}


ObjectId
PackageManagerPortId(void)
{
	CUNameServer ns;
	OpaqueRef thing, spec;
	MAKE_ID_STR('pckm',type);

	if (ns.lookup(type, kUPort, &thing, &spec) != noErr)
		thing = 0;
	return (ObjectId)thing;
}


#pragma mark -
/*------------------------------------------------------------------------------
	C P a c k a g e M a n a g e r
	The Package Manager task responds to events from its port to encapsulate the
	loading/removing of packages.
------------------------------------------------------------------------------*/

size_t
CPackageManager::getSizeOf(void) const
{
	return sizeof(CPackageManager);
}


NewtonErr
CPackageManager::mainConstructor(void)
{
	f74 = NULL;
	f78 = NULL;
	f7C = 0;

	NewtonErr				err;
	CPersistentDBEntry	persistentSpace;
	CHeapDBEntry			kernelSpace;

	CAppWorld::mainConstructor();

	f78 = GetHeap();
	if ((err = MemObjManager::findEntryByName(kMemObjPersistent, 'prot', &persistentSpace)) == noErr)
	{
		persistentSpace.f10 = 0;
		ZapHeap(persistentSpace.fHeap, kZapHeapVerification, YES);
		persistentSpace.fFlags &= ~0x80;
		MemObjManager::registerEntryByName(kMemObjPersistent, 'prot', &persistentSpace);
		DestroyVMHeap(persistentSpace.fHeap);

		XTRY
		{
			XFAIL(err = MemObjManager::findEntryByName(kMemObjHeap, 'kstk', &kernelSpace))
			f74 = kernelSpace.fHeap;
			XFAILNOT(fEventHandler = new CPackageEventHandler, err = MemError();)
			fEventHandler->init(kPackageEventId);
			XFAILNOT(gPackageSemaphore = new CULockingSemaphore, err = MemError();)
		}
		XENDTRY;
	}
	return err;
}


void
CPackageManager::mainDestructor(void)
{
	if (fEventHandler)
		delete fEventHandler;

	if (gPackageSemaphore)
		delete gPackageSemaphore, gPackageSemaphore = NULL;

	CAppWorld::mainDestructor();
}


#pragma mark -
/*------------------------------------------------------------------------------
	R e g i s t r y I n f o
	Information the Package Manager holds about registered packages.
------------------------------------------------------------------------------*/

RegistryInfo::RegistryInfo(ULong inArg1, ULong inArg2)
{
	f00 = inArg1;
	f04 = inArg2;
}


#pragma mark -
/*------------------------------------------------------------------------------
	C P a c k a g e E v e n t H a n d l e r
	The handler for events passed to the Package Manager.
------------------------------------------------------------------------------*/

CPackageEventHandler::CPackageEventHandler()
{
	f1C = NO;
	f28 = 0;
	f3C = 0;
	f2C = 0;
	f40 = 0;
	f64 = NO;
	setPersistentHeap();
	f14 = new CDynamicArray(sizeof(PackageInfo), 6);
	setDefaultHeap();
	f18 = new CDynamicArray(sizeof(RegistryInfo), 8);
	f60 = 0;
	initValidatePackageDriver();
}


CPackageEventHandler::~CPackageEventHandler()
{
	// no delete f14 in original
	if (f18)
		delete f18;
}


void
CPackageEventHandler::eventHandlerProc(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent)
{
	if (*inSize >= sizeof(CPkBaseEvent))
	{
		ULong evtCode = ((CPkBaseEvent *)inEvent)->fEventCode;
		switch (evtCode)
		{
		// package registration
		case 'rgtr':
			registerEvent((CPkRegisterEvent *)inEvent);
			break;
		case 'urgr':
			unregisterEvent((CPkUnregisterEvent *)inEvent);
			break;

		case 'pkbl':
			beginLoadPackage((CPkBeginLoadEvent *)inEvent);
			break;
		case 'pkbu':
			getBackupInfo((CPkBackupEvent *)inEvent);
			break;
		case 'pksc':
			safeToDeactivatePackage((CPkSafeToDeactivateEvent *)inEvent);
			break;
		case 'pkrm':
			removePackage((CPkRemoveEvent *)inEvent, YES, YES);
			break;
		}
	}
}


void
CPackageEventHandler::eventCompletionProc(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent)
{ }


void
CPackageEventHandler::registerEvent(CPkRegisterEvent * inEvent)
{
	ArrayIndex index;
	ULong sp04;
	if (searchRegistry(&index, &sp04, inEvent->f10) == kOSErrPartTypeNotRegistered)
	{
		RegistryInfo info(inEvent->f10, inEvent->f14);
		f18->addElement(&info);
	}
	else
	{
		inEvent->fEventErr = kOSErrPartTypeAlreadyRegistered;
		setReply(sizeof(CPkRegisterEvent), inEvent);
		replyImmed();
	}
}


void
CPackageEventHandler::unregisterEvent(CPkUnregisterEvent * inEvent)
{
	ArrayIndex index;
	ULong sp04;
	if (searchRegistry(&index, &sp04, inEvent->f10) == noErr)
		f18->removeElementsAt(index, 1);
	inEvent->fEventErr = noErr;
	setReply(sizeof(CPkUnregisterEvent), inEvent);
	replyImmed();
}


void
CPackageEventHandler::beginLoadPackage(CPkBeginLoadEvent * inEvent)
{}

void
CPackageEventHandler::getBackupInfo(CPkBackupEvent * inEvent)
{}

void
CPackageEventHandler::safeToDeactivatePackage(CPkSafeToDeactivateEvent * inEvent)
{}

void
CPackageEventHandler::removePackage(CPkRemoveEvent * inEvent, bool, bool)
{}


void
CPackageEventHandler::initValidatePackageDriver(void)
{}

void
CPackageEventHandler::validatePackage(CPkBeginLoadEvent * inEvent, CPackageIterator * inIter)
{}


void
CPackageEventHandler::checkAndInstallPatch(PartInfo&, SourceType, const PartSource&)
{}

void
CPackageEventHandler::loadProtocolCode(void**, PartInfo&, SourceType, const PartSource&)
{}

void
CPackageEventHandler::getUniquePackageId(void)
{}



void
CPackageEventHandler::getPartSize(void)
{}

void
CPackageEventHandler::loadNextPart(long*, unsigned char*, unsigned char*)
{}

void
CPackageEventHandler::installPart(unsigned long*, long*, unsigned char*, const PartId&, ExtendedPartInfo&, SourceType, const PartSource&)
{}

void
CPackageEventHandler::removePart(const PartId&, const CInstalledPart&, unsigned char)
{}


NewtonErr
CPackageEventHandler::searchPackageList(ArrayIndex * outIndex, ULong inArg2)
{
	NewtonErr err = kOSErrNoSuchPackage;
	ArrayIndex index;
	CArrayIterator iter(f14);
	for (index = iter.firstIndex(); iter.more(); index = iter.nextIndex())
	{
		if (((PackageInfo *)f14->elementPtrAt(index))->f00 == inArg2)
		{
			*outIndex = index;
			err = noErr;
			break;
		}
	}
	return err;
}


NewtonErr
CPackageEventHandler::searchPackageList(ArrayIndex * outIndex, UniChar * inName, ULong inArg3)
{
	NewtonErr err = kOSErrNoSuchPackage;
	ArrayIndex index;
	CArrayIterator iter(f14);
	for (index = iter.firstIndex(); iter.more(); index = iter.nextIndex())
	{
		if (Ustrcmp(((PackageInfo *)f14->elementPtrAt(index))->f1C, inName) == 0)
		{
			*outIndex = index;
			err = noErr;
			break;
		}
	}
	return err;
}


NewtonErr
CPackageEventHandler::searchRegistry(ArrayIndex * outIndex, ULong * outArg2, ULong inArg3)
{
	NewtonErr err = kOSErrNoSuchPackage;
	ArrayIndex index;
	CArrayIterator iter(f18);
	for (index = iter.firstIndex(); iter.more(); index = iter.nextIndex())
	{
		RegistryInfo * info = (RegistryInfo *)f18->safeElementPtrAt(index);
		if (info != NULL
		&&  info->f00 == inArg3)
		{
			*outArg2 = info->f04;
			*outIndex = index;
			err = noErr;
			break;
		}
	}
	return err;
}


void
CPackageEventHandler::setDefaultHeap(void)
{
	SetHeap(gPkgWorld->f78);
}

void
CPackageEventHandler::setPersistentHeap(void)
{
	SetHeap(gPkgWorld->f74);
}


#pragma mark -
/* -----------------------------------------------------------------------------
	Get package backup info.
	Useful for returning info to the PM iterator.
----------------------------------------------------------------------------- */

NewtonErr
cGetPackageBackupInfo(ArrayIndex index, UniChar * outName, size_t * outSize, ULong * outId, ULong * outVersion, SourceType * outSource, ArrayIndex * outIndex, ULong * outFlags, ULong * outTimestamp)
{
//	r5 r4 r2 r3 r10 r9 r6 r8 r7
	NewtonErr err = noErr;
	PartSource source;
	CPkBackupEvent backupEvent(*outIndex, index, YES, source, *gPkgWorld->getMyPort(), *gPkgWorld->getMyPort());
	CUPort pmPort(PackageManagerPortId());
	size_t replySize;
	gPkgWorld->releaseMutex();
	err = pmPort.sendRPC(&replySize, &backupEvent, sizeof(CPkBackupEvent), &backupEvent, sizeof(CPkBackupEvent));
	gPkgWorld->acquireMutex();
/* this is confusing
	*outIndex = sp10;	//backupEvent.f0C
	*outSize = sp0C;	//backupEvent.f10
	*outId = sp2C;	//backupEvent.f10
	*outVersion = sp30;	//backupEvent.f14

	*outSource = backupEvent.f18;
*/	*outFlags = backupEvent.f20;
	*outTimestamp = backupEvent.f24;
	Ustrncpy(outName, backupEvent.f28, kMaxPackageNameSize-1);
	outName[kMaxPackageNameSize-1] = 0;		// original does not use -1 -- buffer overrun, surely?
	return err;
}


/* -----------------------------------------------------------------------------
	Install package.
----------------------------------------------------------------------------- */

NewtonErr
InstallPackage(char * inArg1, SourceType inSource, PSSId * outPkgId, bool * outArg4, bool * outArg5, CStore * inStore, ObjectId inId)
{
#if 0
//r8 r1 r2 r4 r9 r7 r6 r10
	NewtonErr err;
//sp-08
	if ((err = gPkgWorld->fork(NULL)) == noErr)
	{
//sp-98
		PartSource sp98;
		sp98.mem.buffer = (VAddr)inArg1;
		CPkBeginLoadEvent loadEvent(inSource, sp98, *gPkgWorld->getMyPort(), *gPkgWorld->getMyPort(), YES);			//sp10

		CUMonitor	um(GetROMDomainManagerId());	//sp08
		CUPort pmPort(PackageManagerPortId());		//sp00
		size_t replySize;

		gPkgWorld->releaseMutex();
		gPackageSemaphore->acquire(kWaitOnBlock);
		
		err = pmPort.sendRPC(&replySize, &loadEvent, sizeof(CPkBeginLoadEvent), &loadEvent, sizeof(CPkBeginLoadEvent));
		if (err == noErr
		&&  inStore != NULL
		&& !loadEvent.f81)
		{
			RDMParams	parms;
			parms.fStore = inStore;
			parms.fObjId = inId;
			parms.fPkgId = sp10.f2C;
			err = um.invokeRoutine(kRDM_4, &parms);	// set package id
		}

		gPackageSemaphore->release();
		gPkgWorld->acquireMutex();

		*outPkgId = loadEvent.f81 ? 0 : sp10.f2C;
		if (outArg4)
			*outArg4 = loadEvent.f81;
		if (outArg5)
			*outArg5 = loadEvent.f80;
	}
	return err;

#else
	return noErr;
#endif
}

/* no need for this if we use default arguments
NewtonErr
InstallPackage(char*, SourceType, ULong*, bool*, bool*)
{ return noErr; }*/


/* -----------------------------------------------------------------------------
	C P M I t e r a t o r
	Package Manager iterator.
----------------------------------------------------------------------------- */

CPMIterator::CPMIterator()
{
	fIndex = 0;
	f60 = 0;
}

CPMIterator::~CPMIterator()
{ }

NewtonErr
CPMIterator::init(void)
{
	fIndex = 0;
	gPkgWorld->releaseMutex();
	gPackageSemaphore->acquire(kWaitOnBlock);
	gPkgWorld->acquireMutex();
	return cGetPackageBackupInfo(kIndexNotFound, fPkgName, &fPkgSize, &fPkgId, &fPkgVersion, &fSource, &fIndex, &fPkgFlags, &fTimestamp);
}

void
CPMIterator::nextPackage(void)
{
	if (more())
	{
		fIndex++;
		NewtonErr err = cGetPackageBackupInfo(kIndexNotFound, fPkgName, &fPkgSize, &fPkgId, &fPkgVersion, &fSource, &fIndex, &fPkgFlags, &fTimestamp);
		if (err)
			fIndex = kIndexNotFound;
	}
}

bool
CPMIterator::more(void)
{ return fIndex != kIndexNotFound && fPkgSize > 0; }

void
CPMIterator::done(void)
{
	gPackageSemaphore->release();
}


#pragma mark -
/* -----------------------------------------------------------------------------
	C P r i v a t e P a c k a g e I t e r a t o r
----------------------------------------------------------------------------- */

CPrivatePackageIterator::CPrivatePackageIterator()
{
	fMem = NULL;
	fPkgDir = NULL;
	fPkgParts = NULL;
	fPkgData = NULL;
	fTotalInfoSize = 0;
	fReloc = NULL;
	fRelocInfo = NULL;
}


CPrivatePackageIterator::~CPrivatePackageIterator()
{
	disposeDirectory();
}


/* -----------------------------------------------------------------------------
	Initialize.
	Set up pointers to salient areas in the package.
	-- directory -- part entries -- directory data
	-- relocation header -- relocation entries (optional)
	-- part data
	Args:		inPackage		pointer to package in memory
	Return:	error code
----------------------------------------------------------------------------- */

NewtonErr
CPrivatePackageIterator::init(void * inPackage)
{
	NewtonErr err;
	fMem = (Ptr) inPackage;
	fPkgDir = (PackageDirectory *)inPackage;
	XTRY
	{
		XFAIL(err = checkHeader())

		ULong partEntryInfoSize = fPkgDir->numParts * sizeof(PartEntry);
		ULong directoryInfoSize = fPkgDir->directorySize - (sizeof(PackageDirectory) + partEntryInfoSize);
		XFAIL(err = computeSizeOfEntriesAndData(partEntryInfoSize, directoryInfoSize))

		ULong relocationInfoSize;
		XFAIL(err = setupRelocationData(partEntryInfoSize + directoryInfoSize, &relocationInfoSize))

		fTotalInfoSize = sizeof(PackageDirectory) + partEntryInfoSize + directoryInfoSize + relocationInfoSize;

		err = verifyPackage();
	}
	XENDTRY;
	XDOFAIL(err)
	{
		disposeDirectory();
	}
	XENDFAIL;
	return err;
}


/* -----------------------------------------------------------------------------
	Set up pointers to the part entries and text area.
	Args:		inPartEntryInfoSize
				inDirectoryInfoSize
	Return:	error code
----------------------------------------------------------------------------- */

NewtonErr
CPrivatePackageIterator::computeSizeOfEntriesAndData(ULong& inPartEntryInfoSize, ULong& inDirectoryInfoSize)
{
	// part data immediately follows the directory
	fPkgParts = (PartEntry *)(fMem + sizeof(PackageDirectory));
	// text follows the part data
	fPkgData = fMem + sizeof(PackageDirectory) + inPartEntryInfoSize;
	return noErr;
}


/* -----------------------------------------------------------------------------
	Set up pointers to relocation information.
	Args:		inOtherInfoSize
				outRelocationInfoSize
	Return:	error code
----------------------------------------------------------------------------- */

NewtonErr
CPrivatePackageIterator::setupRelocationData(ULong inOtherInfoSize, ULong * outRelocationInfoSize)
{
	NewtonErr err = noErr;
	if ((fPkgDir->flags & kRelocationFlag) != 0)
	{
		fReloc = (RelocationHeader *)(fMem + sizeof(PackageDirectory) + inOtherInfoSize);
		if (fReloc->reserved == 0)
			*outRelocationInfoSize = fReloc->relocationSize;
		else
			err = kOSErrBadPackage;
	}
	else
	{
		// no relocation required
		fReloc = NULL;
		fRelocInfo = NULL;
		*outRelocationInfoSize = 0;
	}
	return err;
}


/* -----------------------------------------------------------------------------
	Set up pointer to relocation chunk information.
	Args:		--
	Return:	error code
----------------------------------------------------------------------------- */

NewtonErr
CPrivatePackageIterator::getRelocationChunkInfo(void)
{
	if (fReloc)
		fRelocInfo = (ULong *)fReloc + 1;
	return noErr;
}


/* -----------------------------------------------------------------------------
	Reset all pointers.
	We don’t own the package data so there’s no actual disposal to be done.
	Args:		--
	Return:	--
----------------------------------------------------------------------------- */

void
CPrivatePackageIterator::disposeDirectory(void)
{
	fMem = NULL;
	fPkgParts = NULL;
	fPkgData = NULL;
	fReloc = NULL;
	fRelocInfo = NULL;
	fTotalInfoSize = 0;
}


/* -----------------------------------------------------------------------------
	Check the package header.
	"package0" => NOS1 package
	"package1" => NOS2 package with relocation information for C functions
	Args:		--
	Return:	error code
----------------------------------------------------------------------------- */
const char * const kPackageMagicNumber = "package01";

NewtonErr
CPrivatePackageIterator::checkHeader(void)
{
	NewtonErr err = noErr;
	ArrayIndex i;
	// first 7 chars must be "package"
	for (i = 0; i < 7 && !err; ++i)
	{
		if (fPkgDir->signature[i] != kPackageMagicNumber[i])
			err = kOSErrBadPackage;
	}
	if (!err)
	{
		// next char must be 0 or 1
		for (err = kOSErrBadPackage; i < 7+2 && err; ++i)
		{
			if (fPkgDir->signature[7] == kPackageMagicNumber[i])
				err = noErr;
		}
	}
	return err;
}


/* -----------------------------------------------------------------------------
	Perform a sanity check on the package data.
	Args:		--
	Return:	error code
----------------------------------------------------------------------------- */

NewtonErr
CPrivatePackageIterator::verifyPackage(void)
{
	NewtonErr err = kOSErrBadPackage;	// assume the worst
//	size_t xx = Ustrlen(packageName());	// sic -- unused!
	XTRY
	{
		// package name must lie within the greater directory
		XFAIL(fPkgDir->name.offset < fPkgDir->directorySize)
		// copyright text (if any) must lie within the greater directory
		XFAIL(fPkgDir->copyright.length > 0 && fPkgDir->copyright.offset < fPkgDir->directorySize)
		XFAIL((fPkgDir->flags & 0xF000) != 0x1000)
		// verify all the part entries
		PartEntry * part = fPkgParts;
		for (ArrayIndex i = 0; i < numberOfParts(); ++i, ++part)
		{
			// if compressed, compressor name must lie within the greater directpry
			XFAIL((part->flags & kCompressedFlag) != 0 && part->compressor.offset < fPkgDir->directorySize)
			// info (if any) must lie within the greater directpry
			XFAIL(part->info.length > 0 && part->info.offset < fPkgDir->directorySize)
			// the part itself must lie within the package but outside the greater directpry
			XFAIL(part->offset > fPkgDir->size - fPkgDir->directorySize)
		}
		err = noErr;	// we passed
	}
	XENDTRY;
	return err;
}


/* -----------------------------------------------------------------------------
	Return the unique package name.
	Args:		--
	Return:	unicode string
----------------------------------------------------------------------------- */

UniChar *
CPrivatePackageIterator::packageName(void)
{
	InfoRef info = fPkgDir->name;
	return (UniChar *)(fPkgData + info.offset);
}


/* -----------------------------------------------------------------------------
	Return the total size of the package.
	Args:		--
	Return:	size
----------------------------------------------------------------------------- */

size_t
CPrivatePackageIterator::packageSize(void)
{
	return fPkgDir ? fPkgDir->size : 0;
}


/* -----------------------------------------------------------------------------
	Return the number of parts in the package.
	Args:		--
	Return:	number
----------------------------------------------------------------------------- */

ArrayIndex
CPrivatePackageIterator::numberOfParts(void)
{
	return fPkgDir ? fPkgDir->numParts : 0;
}


/* -----------------------------------------------------------------------------
	Fill out a PartInfo record for the specified part.
	Args:		inPartIndex
				outInfo
	Return:	--
----------------------------------------------------------------------------- */

void
CPrivatePackageIterator::getPartInfo(ArrayIndex inPartIndex, PartInfo * const outInfo)
{
	if (inPartIndex < fPkgDir->numParts)
	{
		getPartInfoDesc(inPartIndex, outInfo);
		outInfo->data.address = fMem + getPartDataOffset(inPartIndex);
	}
}


/* -----------------------------------------------------------------------------
	Fill out a PartInfo record for the specified part.
	Args:		inPartIndex
				outInfo
	Return:	--
----------------------------------------------------------------------------- */

void
CPrivatePackageIterator::getPartInfoDesc(ArrayIndex inPartIndex, PartInfo * const outInfo)
{
	PartEntry * part = fPkgParts+inPartIndex;

	outInfo->autoLoad = (part->flags & kAutoLoadFlag) != 0;
	outInfo->autoRemove = (part->flags & kAutoRemovePartFlag) != 0;
	outInfo->compressed = (part->flags & kCompressedFlag) != 0;
	outInfo->notify = (part->flags & kNotifyFlag) != 0;
	outInfo->autoCopy = (part->flags & kAutoCopyFlag) != 0;
	outInfo->kind = (part->flags & 0x0F);
	outInfo->type = part->type;
	outInfo->size = part->size;
	outInfo->sizeInMemory = part->size2;
	outInfo->infoSize = part->info.length;
	outInfo->info = fPkgData + part->info.offset;
	outInfo->compressor = fPkgData + part->compressor.offset;
}


/* -----------------------------------------------------------------------------
	Return the offset to data for the specified part.
	Part data follows ALL header information.
	Args:		inPartIndex
	Return:	offset
----------------------------------------------------------------------------- */

ULong
CPrivatePackageIterator::getPartDataOffset(ArrayIndex inPartIndex)
{
	if (inPartIndex < fPkgDir->numParts
	&&  fPkgParts != NULL)
	{
		PartEntry * part = fPkgParts+inPartIndex;
		return fTotalInfoSize + part->offset;
	}
	return 0;
}


#pragma mark -
/* -----------------------------------------------------------------------------
	C P a c k a g e I t e r a t o r
	It looks to me like the stream source was an afterthought, and instead of
	updating the original iterator, it was made private and wrapped by the
	new iterator. All rather ugly and unnecessary.
----------------------------------------------------------------------------- */

CPackageIterator::CPackageIterator(CPipe * inPipe)
{
	fIsStreamSource = YES;
	fPrivateIter.fMem = NULL;
	fPipe = inPipe;
}


CPackageIterator::CPackageIterator(void * inData)
{
	fIsStreamSource = NO;
	fPrivateIter.fMem = (Ptr)inData;
	fPipe = NULL;
}


CPackageIterator::~CPackageIterator()
{
	disposeDirectory();
}

NewtonErr
CPackageIterator::init(void)
{
	NewtonErr err = noErr;
	newton_try
	{
		if (fIsStreamSource)
		{
			XTRY
			{
				XFAILNOT(fPrivateIter.fPkgDir = new PackageDirectory, err = kOSErrNoMemory;)
				newton_try
				{
					bool isEOF;
					size_t sizeRead = sizeof(PackageDirectory);
					fPipe->readChunk(fPrivateIter.fPkgDir, sizeRead, isEOF);
				}
				newton_catch(exPipe)
				{
					err = (NewtonErr)(unsigned long)CurrentException()->data;
				}
				end_try;
				XFAIL(err)
				XFAIL(err = fPrivateIter.checkHeader())
				ULong partEntryInfoSize = fPrivateIter.fPkgDir->numParts * sizeof(PartEntry);
				ULong directoryInfoSize = fPrivateIter.fPkgDir->directorySize - (sizeof(PackageDirectory) + partEntryInfoSize);
				XFAIL(err = computeSizeOfEntriesAndData(partEntryInfoSize, directoryInfoSize))

				ULong relocationInfoSize;
				XFAIL(err = setupRelocationData(partEntryInfoSize + directoryInfoSize, &relocationInfoSize))

				fPrivateIter.fTotalInfoSize = sizeof(PackageDirectory) + partEntryInfoSize + directoryInfoSize + relocationInfoSize;
				err = verifyPackage();
			}
			XENDTRY;
		}
		else
			err = fPrivateIter.init(fPrivateIter.fMem);
	}
	newton_catch(exRootException)	// would not newton_catch_all be better?
	{
		err = (NewtonErr)(unsigned long)CurrentException()->data;
	}
	end_try;
	if (err)
		disposeDirectory();
	return err;
}

NewtonErr
CPackageIterator::initFields(void)
{ }

NewtonErr
CPackageIterator::computeSizeOfEntriesAndData(ULong& inPartEntryInfoSize, ULong& inDirectoryInfoSize)
{
	NewtonErr err = noErr;
	if (fIsStreamSource)
	{
		XTRY
		{
			XFAILNOT(fPrivateIter.fPkgParts = (PartEntry *)NewPtr(inPartEntryInfoSize), err = kOSErrNoMemory;)
			newton_try
			{
				bool isEOF;
				size_t sizeRead = inPartEntryInfoSize;
				fPipe->readChunk(fPrivateIter.fPkgParts, sizeRead, isEOF);
			}
			newton_catch(exPipe)
			{
				err = (NewtonErr)(unsigned long)CurrentException()->data;
			}
			end_try;
			XFAIL(err)
			XFAILNOT(fPrivateIter.fPkgData = NewPtr(inDirectoryInfoSize), err = kOSErrNoMemory;)
			newton_try
			{
				bool isEOF;
				size_t sizeRead = inDirectoryInfoSize;
				fPipe->readChunk(fPrivateIter.fPkgData, sizeRead, isEOF);
			}
			newton_catch(exPipe)
			{
				err = (NewtonErr)(unsigned long)CurrentException()->data;
			}
			end_try;
		}
		XENDTRY;
	}
	else
		fPrivateIter.computeSizeOfEntriesAndData(inPartEntryInfoSize, inDirectoryInfoSize);
	return err;
}

NewtonErr
CPackageIterator::setupRelocationData(ULong inOtherInfoSize, ULong * outRelocationInfoSize)
{
	NewtonErr err = noErr;
	if ((fPrivateIter.fPkgDir->flags & kRelocationFlag) != 0)
	{
		if (fIsStreamSource)
		{
			XTRY
			{
				XFAILNOT(fPrivateIter.fReloc = (RelocationHeader *)NewPtr(sizeof(RelocationHeader)), err = kOSErrNoMemory;)
				newton_try
				{
					bool isEOF;
					size_t sizeRead = sizeof(RelocationHeader);
					fPipe->readChunk(fPrivateIter.fReloc, sizeRead, isEOF);
				}
				newton_catch(exPipe)
				{
					err = (NewtonErr)(unsigned long)CurrentException()->data;
				}
				end_try;
				XFAILNOT(fPrivateIter.fReloc->reserved == 0, *outRelocationInfoSize = 0; err = kOSErrBadPackage;)
				*outRelocationInfoSize = fPrivateIter.fReloc->relocationSize;
			}
			XENDTRY;
		}
		else
			fPrivateIter.setupRelocationData(inOtherInfoSize, outRelocationInfoSize);
	}
	else
	{
		fPrivateIter.fReloc = NULL;
		fPrivateIter.fRelocInfo = NULL;
		*outRelocationInfoSize = 0;
	}
	return err;
}

NewtonErr
CPackageIterator::getRelocationChunkInfo(void)
{
	NewtonErr err = noErr;
	if (fPrivateIter.fReloc)
	{
		if (fIsStreamSource)
		{
			XTRY
			{
				XFAILNOT(fPrivateIter.fRelocInfo = (ULong *)NewPtr(fPrivateIter.fReloc->relocationSize - sizeof(RelocationHeader)), err = kOSErrNoMemory;)
				newton_try
				{
					bool isEOF;
					size_t sizeRead = fPrivateIter.fReloc->relocationSize - sizeof(RelocationHeader);
					fPipe->readChunk(fPrivateIter.fRelocInfo, sizeRead, isEOF);
				}
				newton_catch(exPipe)
				{
					err = (NewtonErr)(unsigned long)CurrentException()->data;
				}
				end_try;
			}
			XENDTRY;
		}
		else
			fPrivateIter.getRelocationChunkInfo();
	}
	return err;
}

void
CPackageIterator::disposeDirectory(void)
{
	if (fIsStreamSource)
	{
		if (fPrivateIter.fPkgDir)
			delete fPrivateIter.fPkgDir;
		if (fPrivateIter.fPkgParts)
			FreePtr((Ptr)fPrivateIter.fPkgParts);
		if (fPrivateIter.fPkgData)
			FreePtr(fPrivateIter.fPkgData);
		if (fPrivateIter.fReloc)
			FreePtr((Ptr)fPrivateIter.fReloc);
		if (fPrivateIter.fRelocInfo)
			FreePtr((Ptr)fPrivateIter.fRelocInfo);
	}
	fPrivateIter.disposeDirectory();
}

NewtonErr
CPackageIterator::verifyPackage(void)
{
	NewtonErr err = noErr;
	newton_try
	{
		err = fPrivateIter.verifyPackage();
	}
	newton_catch(exAbort)
	{
		err = kOSErrBadPackage;
	}
	end_try;
	return err;
}

ULong
CPackageIterator::getPackageId(void)
{
	return fPrivateIter.fPkgDir ? fPrivateIter.fPkgDir->id : 0;
}

ULong
CPackageIterator::getVersion(void)
{
	return fPrivateIter.fPkgDir ? fPrivateIter.fPkgDir->version : 0;
}

ULong
CPackageIterator::packageFlags(void)
{
	return fPrivateIter.fPkgDir ? fPrivateIter.fPkgDir->flags : 0;
}

UniChar *
CPackageIterator::packageName(void)
{
	return fPrivateIter.packageName();
}

size_t
CPackageIterator::packageSize(void)
{
	size_t pkgSize = 0;
	newton_try
	{
		pkgSize = fPrivateIter.packageSize();
	}
	newton_catch(exAbort)
	{
		pkgSize = kOSErrBadPackage;	// this is babbling, dribbling madness!
	}
	end_try;
	return pkgSize;
}

bool
CPackageIterator::forDispatchOnly(void)
{
	return fPrivateIter.fPkgDir ? (fPrivateIter.fPkgDir->flags & kAutoRemoveFlag) != 0 : NO;
}

bool
CPackageIterator::copyProtected(void)
{
	return fPrivateIter.fPkgDir ? (fPrivateIter.fPkgDir->flags & kCopyProtectFlag) != 0 : NO;
}

UniChar *
CPackageIterator::copyright(void)
{
	InfoRef info = fPrivateIter.fPkgDir->copyright;
	return info.length > 0 ? (UniChar *)(fPrivateIter.fPkgData + info.offset) : NULL;
}

ULong
CPackageIterator::creationDate(void)
{
	return fPrivateIter.fPkgDir ? fPrivateIter.fPkgDir->creationDate : 0;
}

ULong
CPackageIterator::modifyDate(void)
{
	return fPrivateIter.fPkgDir ? fPrivateIter.fPkgDir->modifyDate : 0;
}

size_t
CPackageIterator::directorySize(void)
{
	return fPrivateIter.fPkgDir ? fPrivateIter.fPkgDir->directorySize : 0;
}

ArrayIndex
CPackageIterator::numberOfParts(void)
{
	ArrayIndex count = 0;
	newton_try
	{
		count = fPrivateIter.numberOfParts();
	}
	newton_catch(exAbort)
	{
		count = kOSErrBadPackage;	// this is babbling, dribbling madness!
	}
	end_try;
	return count;
}

void
CPackageIterator::getPartInfo(ArrayIndex inPartIndex, PartInfo * const outInfo)
{
	if (inPartIndex < fPrivateIter.fPkgDir->numParts)
	{
		fPrivateIter.getPartInfoDesc(inPartIndex, outInfo);
		if (fIsStreamSource)
			outInfo->data.streamOffset = getPartDataOffset(inPartIndex);
		else
			outInfo->data.address = fPrivateIter.fMem + getPartDataOffset(inPartIndex);
	}
}

ULong
CPackageIterator::getPartDataOffset(ArrayIndex inPartIndex)
{
	return fPrivateIter.getPartDataOffset(inPartIndex);
}

ULong
CPackageIterator::processorTypeOfPart(ArrayIndex inPartIndex)
{
	return (inPartIndex < fPrivateIter.fPkgDir->numParts) ? fPrivateIter.fPkgParts[inPartIndex].flags & 0xF000 : 0;
}

void
CPackageIterator::store(CStore * inStore, PSSId inId, CCallbackCompressor * inCompressor, CLOCallback * inCallback)
{ /*it’s a biggy*/ }


#pragma mark -
/*------------------------------------------------------------------------------
	P l a i n   C   I n t e r f a c e
------------------------------------------------------------------------------*/
extern "C" {
Ref	FGetPackages(RefArg inRcvr);
}

#if 0
Ref
IteratorToPackageFrame(CPMIterator * inIter)
{
	RefVar pkgFrame(Clone(RScanonicalPMIteratorPackageFrame));

	SetFrameSlot(pkgFrame, SYMA(id), MAKEINT(inIter->packageId()));
	SetFrameSlot(pkgFrame, SYMA(size), MAKEINT(inIter->packageSize()));
	SetFrameSlot(pkgFrame, SYMA(title), MakeString(inIter->packageName()));
	SetFrameSlot(pkgFrame, SYMA(version), MAKEINT(inIter->packageVersion()));
	SetFrameSlot(pkgFrame, SYMA(timestamp), MAKEINT(inIter->timestamp()));
	SetFrameSlot(pkgFrame, SYMA(copyProtection), MAKEBOOLEAN(inIter->isCopyProtected()));

	PSSId storeId;
	CStore * storeObj;
	if (IdToStore(inIter->packageId(), &storeObj, &storeId) == noErr)
	{
		SetFrameSlot(pkgFrame, SYMA(store), ToObject(storeObj));
		SetFrameSlot(pkgFrame, SYMA(pssId), MAKEINT(storeId));
	}

	return pkgFrame;
}


Ref
FGetPackages(RefArg inRcvr)
{
	if (gPkgWorld->fork(NULL) != noErr)
		ThrowMsg("couldn’t fork it over");

	RefVar pkgs(MakeArray(0));
	CPMIterator iter;
	for (iter.init(); iter.more(); iter.nextPackage())
		AddArraySlot(pkgs, IteratorToPackageFrame(&iter));
	iter.done();
	return pkgs;
}


#pragma mark -
/* -----------------------------------------------------------------------------
	C P a c k a g e B l o c k
----------------------------------------------------------------------------- */

class CPackageBlock
{
public:

	NewtonErr	init(ULong, ULong, ULong, SourceType, ULong, UShort*, UShort*, ULong, ULong);
};


/* -----------------------------------------------------------------------------
	C P a c k a g e L o a d e r
----------------------------------------------------------------------------- */

class CPackageLoader
{
public:
					CPackageLoader(CEndpointPipe * inSource, SourceType inType);
					CPackageLoader(CPipe * inSource, SourceType inType);
					CPackageLoader(char * inSource, SourceType inType);
					~CPackageLoader();

	void			reset(void);
	NewtonErr	load(void);
	void			done(bool * outArg1, bool * outArg2);

private:
	Ptr			f00;
	CPipe *		f04;
	SourceType	f08;
	bool			f10;
	CRingBuffer *	f14;
	int			f18;
	CPackageLoaderEventHandler *	f1C;
	bool			f20;
	bool			f21;
//size+24
};


// construct to load package over the wire
CPackageLoader::CPackageLoader(CEndpointPipe * inSource, SourceType inType)
{
	f08 = inType;
	f04 = inSource;
	f10 = YES;
	f1C = NULL;
	f14 = NULL;
}

// construct to load package from stream
CPackageLoader::CPackageLoader(CPipe * inSource, SourceType inType)
{
	f08 = inType;
	f04 = inSource;
	f10 = NO;
	f1C = NULL;
	f14 = NULL;
}

// construct to load package from memory
CPackageLoader::CPackageLoader(char * inSource, SourceType inType)
{
	f08 = inType;
	f00 = inSource;
	f10 = NO;
	f1C = NULL;
	f14 = NULL;
}

CPackageLoader::~CPackageLoader()
{ /* this really does nothing */ }

void
CPackageLoader::reset(void)
{ /* this really does nothing */ }

NewtonErr
CPackageLoader::load(void)
{
	NewtonErr err = noErr;
	//sp-18
	gPkgWorld->releaseMutex();
	gPackageSemaphore->acquire(kWaitOnBlock);
	gPkgWorld->acquireMutex();
	f20 = NO;
	f21 = NO;
	XTRY
	{
		f1C = new CPackageLoaderEventHandler;
		XFAIL(err = f1C->init(kPackageEventId))
		if (IsStream(f08))
		{
			XFAILNOT(f14 = new CRingBuffer, err = kOSErrNoMemory;)
			XFAIL(err = f14->init(256))
			f14->makeShared(0);
			CRingBuffer * sp08 = f14;
			CPipe * sp00 = f04;
			if (f10)
			{
				newton_try
				{
					((CEndpointPipe *)f04)->removeFromAppWorld();
				}
				newton_catch(exPipe)
				{
					err = (NewtonErr)(unsigned long)CurrentException()data;
				}
				end_try;
				XFAIL(err)
			}
			//sp-88
			CPipeApp sp00(sp88, f10);
			XFAIL(err = sp00.init('pipe', YES, kSpawnedTaskStackSize))
			ULong thing, spec;
			CUNameServer ns;
			XFAIL(err = ns.lookup("pipe", kUPort, &thing, &spec))
			spB0 = f14->fSharedMem;	//ObjectId
			spB4 = thing;
		}
		PartSource sp84 = fPipe;	// huh?
		CPkBeginLoadEvent loadEvent(f08, sp84, gPkgWorld->getMyPort(), gPkgWorld->getMyPort(), YES);
		CUPort pmPort(PackageManagerPortId());
		size_t replySize;
		gPkgWorld->releaseMutex();
		err = pmPort.sendRPC(&replySize, &loadEvent, sizeof(CPkBeginLoadEvent), &loadEvent, sizeof(CPkBeginLoadEvent));
		gPkgWorld->acquireMutex();
		XFAIL(err)
		XFAIL(err = loadEvent.fEventErr)
		f18 = sp2C;
		f20 = loadEvent.f81;
		f21 = loadEvent.f80;
	}
	XENDTRY;
	return err;
}

void
CPackageLoader::done(bool * outArg1, bool * outArg2)
{
	if (outArg1)
		*outArg1 = f20;
	if (outArg2)
		*outArg2 = f21;
	if (IsStream(f08) && f10)
	{
		NewtonErr err = noErr;
		newton_try
		{
			((CEndpointPipe *)f04)->addToAppWorld();
		}
		newton_catch(exPipe)
		{
			err = (NewtonErr)(unsigned long)CurrentException()data;
		}
		end_try;
		if (f1C)
			delete f1C, f1C = NULL;
		if (f14)
			delete f14, f14 = NULL;
		gPackageSemaphore->release();
	}
}

#endif

