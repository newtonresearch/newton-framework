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

extern void SetCardReinsertReason(const UniChar * inReason, bool inArg2);


/* -----------------------------------------------------------------------------
	T y p e s
----------------------------------------------------------------------------- */
/*------------------------------------------------------------------------------
	C V a l i d a t e P a c k a g e D r i v e r
	P-class interface.
------------------------------------------------------------------------------*/

PROTOCOL CValidatePackageDriver : public CProtocol
{
public:
	static CValidatePackageDriver *	make(const char * inName);
	void			destroy(void);

	void	validateBegin(UniChar * inName, Ptr inAddr, size_t inSize, Ptr);
	void	validateNextBlock(Ptr inAddr, size_t inSize);
	void	validateEnd(NewtonErr inErr);
};


// Packages are registered using the following information:
struct RegistryInfo
{
		RegistryInfo(ULong inType, ULong inArg2);

	ULong		fType;
	ULong		f04;
};


/* -----------------------------------------------------------------------------
	D a t a
----------------------------------------------------------------------------- */

CULockingSemaphore *	gPackageSemaphore;


/*------------------------------------------------------------------------------
	P u b l i c   I n t e r f a c e
------------------------------------------------------------------------------*/
void
MakeFourCharStr(ULong inVal, char * outStr) {
	sprintf(outStr, "%c%c%c%c", inVal>>24, inVal>>16, inVal>>8, inVal);
}


NewtonErr
InitializePackageManager(ObjectId inEnvironmentId)
{
	CPackageManager	pm;
	return pm.init('pckm', true, kSpawnedTaskStackSize, kUserTaskPriority, inEnvironmentId);
}


ObjectId
PackageManagerPortId(void)
{
	CUNameServer ns;
	OpaqueRef thing, spec;
	MAKE_ID_STR('pckm',type);

	if (ns.lookup(type, kUPort, &thing, &spec) != noErr) {
		thing = 0;
	}
	return (ObjectId)thing;
}


void
RegisterPackageWithDebugger(void * inAddr, ULong inPkgId)
{
#if 0
	if (IsSuperMode()) {
		SRegisterPackageWithDebugger(inAddr, inPkgId);
	} else {
		GenericSWI(kRegisterPackageWithDebugger, inAddr, inPkgId);
	}
#endif
}

void
RegisterLoadedCodeWithDebugger(void * inAddr, const char * infoStr, ULong inId)
{
#if 0
	if (IsSuperMode()) {
		SRegisterCodeWithDebugger(inAddr, infoStr, inId);
	} else {
		GenericSWI(kRegisterCodeWithDebugger, inAddr, infoStr, inId);
	}
#endif
}

void
DeregisterLoadedCodeWithDebugger(ULong inId)
{
#if 0
	if (IsSuperMode()) {
		SDeregisterCodeWithDebugger(inId);
	} else {
		GenericSWI(kDeregisterCodeWithDebugger, inId);
	}
#endif
}

void
InformDebuggerMemoryReloaded(void * inAddr, ULong inSize)
{
#if 0
	if (IsSuperMode()) {
		SInformDebuggerMemoryReloaded(inAddr, inSize);
	} else {
		GenericSWI(kInformDebuggerMemoryReloaded, inAddr);
	}
#endif
}


CInstalledPart::CInstalledPart(ULong inType, int inArg2, int inArg3, bool inArg4, bool inArg5, bool inArg6, bool inArg7, VAddr inArg8)
{
	fType = inType;
	f04 = inArg2;
	f08 = inArg3;
	f108 = inArg4;
	f104 = inArg6;
	f102 = inArg7;
	f101 = inArg5;
	f0C = (CClassInfo *)inArg8;
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
	fPersistentHeap = NULL;
	fDefaultHeap = NULL;
	f7C = 0;

	NewtonErr				err;
	CPersistentDBEntry	persistentSpace;
	CHeapDBEntry			kernelSpace;

	CAppWorld::mainConstructor();

	fDefaultHeap = GetHeap();
	if ((err = MemObjManager::findEntryByName(kMemObjPersistent, 'prot', &persistentSpace)) == noErr) {
		persistentSpace.f10 = 0;
		ZapHeap(persistentSpace.fHeap, kZapHeapVerification, true);
		persistentSpace.fFlags &= ~0x80;
		MemObjManager::registerEntryByName(kMemObjPersistent, 'prot', &persistentSpace);
		DestroyVMHeap(persistentSpace.fHeap);

		XTRY
		{
			XFAIL(err = MemObjManager::findEntryByName(kMemObjHeap, 'kstk', &kernelSpace))
			fPersistentHeap = kernelSpace.fHeap;
			fEventHandler = new CPackageEventHandler;
			XFAILIF(fEventHandler == NULL, err = MemError();)
			fEventHandler->init(kPackageEventId);
			gPackageSemaphore = new CULockingSemaphore;
			XFAILIF(gPackageSemaphore == NULL, err = MemError();)
		}
		XENDTRY;
	}
	return err;
}


void
CPackageManager::mainDestructor(void)
{
	if (fEventHandler) {
		delete fEventHandler;
	}
	if (gPackageSemaphore) {
		delete gPackageSemaphore, gPackageSemaphore = NULL;
	}
	CAppWorld::mainDestructor();
}


#pragma mark -
/*------------------------------------------------------------------------------
	R e g i s t r y I n f o
	Information the Package Manager holds about registered packages.
------------------------------------------------------------------------------*/

RegistryInfo::RegistryInfo(ULong inType, ULong inArg2)
{
	fType = inType;
	f04 = inArg2;
}


#pragma mark -
/* -----------------------------------------------------------------------------
	C P a c k a g e B l o c k
----------------------------------------------------------------------------- */

NewtonErr
CPackageBlock::init(ULong inId, ULong inVersion, size_t inSize, SourceType inSrcType, ULong inFlags, UniChar * inName, UniChar * inCopyright, ArrayIndex inNumOfParts, ULong inDate)
{
	NewtonErr err = noErr;
	fSignature = 0;
	fId = inId;
	fVersion = inVersion;
	fSize = inSize;
	fSrcType = inSrcType;
	fFlags = inFlags;
	fDate = inDate;
	fName = NULL;
	fCopyright = NULL;
	fParts = NULL;
	
	newton_try
	{
		fName = new UniChar[(Ustrlen(inName) + 1)*sizeof(UniChar)];
		if (fName) {
			Ustrcpy(fName, inName);
		}
		if (inCopyright) {
			fCopyright = new UniChar[(Ustrlen(inName) + 1)*sizeof(UniChar)];
			if (fCopyright)
				Ustrcpy(fCopyright, inName);
		}
	}
	newton_catch_all	// original says newton_catch("")
	{
		err = kOSErrBadPackage;
	}
	end_try;

	fParts = new CDynamicArray(sizeof(CInstalledPart), inNumOfParts);

	if (err != noErr
	||  fParts == NULL
	||  fName == NULL || fName[0] == 0
	|| (fCopyright == NULL && inCopyright != NULL)) {
		if (err == noErr)
			err = kOSErrNoMemory;
		if (fParts)
			delete fParts;
		if (fName)
			delete fName;
		if (fCopyright)
			delete fCopyright;
	}

	return err;
}


#pragma mark -
/*------------------------------------------------------------------------------
	C P a c k a g e E v e n t H a n d l e r
	The handler for events passed to the Package Manager.
------------------------------------------------------------------------------*/

CPackageEventHandler::CPackageEventHandler()
{
	f1C = false;
	fPkg = NULL;
	fPkgInfo = NULL;
	fPipe = NULL;
	fBuffer = NULL;
	f64 = false;

	// the loaded-package list is persistent
	setPersistentHeap();
	fPackageList = new CDynamicArray(sizeof(CPackageBlock), 6);
	setDefaultHeap();

	fRegistry = new CDynamicArray(sizeof(RegistryInfo), 8);
	f60 = 0;
	initValidatePackageDriver();
}


CPackageEventHandler::~CPackageEventHandler()
{
	// no delete fPackageList -- it’s persistent
	if (fRegistry)
		delete fRegistry;
}


void
CPackageEventHandler::eventHandlerProc(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent)
{
	if (*inSize >= sizeof(CPkBaseEvent)) {
		ULong evtCode = ((CPkBaseEvent *)inEvent)->fEventCode;
		switch (evtCode) {
		// package registration
		case kRegisterPartHandlerEventId:
			registerEvent((CPkRegisterEvent *)inEvent);
			break;
		case kUnregisterPartHandlerEventId:
			unregisterEvent((CPkUnregisterEvent *)inEvent);
			break;

		case kBeginLoadPkgEventId:
			beginLoadPackage((CPkBeginLoadEvent *)inEvent);
			break;
		case kBackupPkgEventId:
			getBackupInfo((CPkBackupEvent *)inEvent);
			break;
		case kSafeToDeactivatePkgEventId:
			safeToDeactivatePackage((CPkSafeToDeactivateEvent *)inEvent);
			break;
		case kRemovePkgEventId:
			removePackage((CPkRemoveEvent *)inEvent, true, true);
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
	if (searchRegistry(&index, &sp04, inEvent->fPartType) == kOSErrPartTypeNotRegistered) {
char evtType[5];
MakeFourCharStr(inEvent->fPartType, evtType);
printf("  CPackageEventHandler::registerEvent(partType='%s',port=%d)\n", evtType, inEvent->fPortId);
		RegistryInfo info(inEvent->fPartType, inEvent->fPortId);
		fRegistry->addElement(&info);
	} else {
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
	if (searchRegistry(&index, &sp04, inEvent->fPartType) == noErr) {
		fRegistry->removeElementsAt(index, 1);
	}
	inEvent->fEventErr = noErr;
	setReply(sizeof(CPkUnregisterEvent), inEvent);
	replyImmed();
}


void
CPackageEventHandler::beginLoadPackage(CPkBeginLoadEvent * ioEvent)
{
printf("  CPackageEventHandler::beginLoadPackage()\n");
	NewtonErr err;
	ULong quickKey = 0;
	SourceType srcType = ioEvent->fSrcType;
	XTRY
	{
		fPartSource = ioEvent->fSrc;
		fPartIndex = 0;
		f64 = false;
		fPkg = NULL;
		if (IsStream(srcType)) {
			// create a buffer to stream into
			fBuffer = new CShadowRingBuffer;
			fBuffer->init(ioEvent->fSrc.stream.bufferId, 0, 0);
			// pipe data into that buffer
			fPipe = new CPartPipe;
			fPipe->init(ioEvent->fSrc.stream.messagePortId, fBuffer, true);
			// create iterator with streamed pkg
			fPkg = new CPackageIterator(fPipe);
		} else {
			// create iterator with in-memory pkg
			fPkg = new CPackageIterator((void *)ioEvent->fSrc.mem.buffer);
		}
		XFAILIF(fPkg == NULL, err = MemError();)
		XFAIL(err = fPkg->init())	// also validate "package" header signature

		newton_try
		{
			// fill in reply
			ioEvent->fUniqueId = getUniquePackageId();
			ioEvent->fPkgSize = fPkg->packageSize();
			ioEvent->fNumOfParts = fPkg->numberOfParts();
			ioEvent->fVersion = fPkg->getVersion();
			ioEvent->fSizeInMem = 0;
			for (ArrayIndex i = 0, count = fPkg->numberOfParts(); i < count; ++i) {
				PartInfo info;
				fPkg->getPartInfo(i, &info);
				ioEvent->fSizeInMem += info.sizeInMemory;
			}
			Ustrncpy(ioEvent->fName, fPkg->packageName(), kMaxPackageNameSize);
			quickKey = (ioEvent->fName[0] << 16) | (ioEvent->fName[1] << 8) | (ioEvent->fName[2]);

			// validate package
			XTRY
			{
				ArrayIndex index;
				if (searchPackageList(&index, fPkg->packageName(), 0) == noErr) {
					// we already have a package with this name
					CPackageBlock * existingPkg = (CPackageBlock *)fPackageList->elementPtrAt(index);
					if (existingPkg != NULL) {
						// return id of existing duplicate package
						ioEvent->fUniqueId = existingPkg->fId;
						if (existingPkg->fVersion != 0 && fPkg->getVersion() != 0) {
							// compare versions
							XFAILIF(fPkg->getVersion() > existingPkg->fVersion, err = kOSErrOlderPackageAlreadyExists;)
							XFAILIF(fPkg->getVersion() < existingPkg->fVersion, err = kOSErrNewerPackageAlreadyExists;)
						}
					}
					XFAIL(err = kOSErrPackageAlreadyExists)
				}
				if (fValidatePackageDriver != NULL) {
					XFAIL(err = validatePackage(ioEvent, fPkg))
				}
				// package is valid -- add package info to our persistent list
char pkgName[32+1], *p = pkgName;
UniChar * s = fPkg->packageName();
while (*s) {
	*p++ = *s++;
}
*p = 0;
printf("  adding %s to package list\n", pkgName);
				setPersistentHeap();
				XTRY
				{
					CPackageBlock block;
					XFAIL(err = block.init(ioEvent->fUniqueId, fPkg->getVersion(), fPkg->packageSize(), srcType, fPkg->packageFlags(), fPkg->packageName(), fPkg->copyright(), fPkg->numberOfParts(), fPkg->modifyDate()))
					XFAIL(err = fPackageList->addElement(&block))
					fPkgInfo = (CPackageBlock *)fPackageList->safeElementPtrAt(fPackageList->count()-1);
					fPkgInfo->fSignature = 'spbl';
					// let Hammer know about the package
					if (IsMemory(srcType))
						RegisterPackageWithDebugger((void *)ioEvent->fSrc.mem.buffer, fPkgInfo->fId);
				}
				XENDTRY;
				setDefaultHeap();

				f1C = true;
				f20 = ioEvent->f10;
				f24 = ioEvent->f14;
			}
			XENDTRY;
		}
		newton_catch_all	// original says newton_catch("")
		{
			setDefaultHeap();
			err = kOSErrBadPackage;
		}
		end_try;
	}
	XENDTRY;
	XDOFAIL(err)
	{
		if (fPkg != NULL) {
			delete fPkg, fPkg = NULL;
		}
	}
	XENDFAIL;

	if (err == kOSErrPackageAlreadyExists) {
		err = noErr;	// not original
	}

	if (err == noErr) {
		while (loadNextPart(&err, &ioEvent->f81, &ioEvent->f80)) {
			;	/* just keep loading */
		}
	}
	if (err == noErr && fValidatePackageDriver == NULL && srcType.format == kFixedMemory && quickKey == 'VPD') {	// VPD = ValidatePackageDriver ?
		initValidatePackageDriver();
	}
	ioEvent->fEventErr = err;
	setReply(sizeof(CPkBeginLoadEvent), ioEvent);
	replyImmed();
}


void
CPackageEventHandler::getBackupInfo(CPkBackupEvent * ioEvent)
{
	NewtonErr err = noErr;	//r10
	f1C = true;
	f20 = ioEvent->f20;
	f24 = ioEvent->f24;
	CArrayIterator * iter = &fBackupIter;
	if (ioEvent->fIndex == 0) {
		iter->init(fPackageList, 0, fPackageList->count() - 1, true);
	} else {
		iter->nextIndex();
	}
	ArrayIndex pkgIndex = iter->more() ? iter->currentIndex() : kIndexNotFound;
	for ( ; iter->more(); pkgIndex = iter->nextIndex()) {
		CPackageBlock * pkgInfo = (CPackageBlock *)fPackageList->elementPtrAt(pkgIndex);
		if (!FLAGTEST(pkgInfo->fFlags, kInvisibleFlag) && ioEvent->f14 == -1) {
			ioEvent->fSize = pkgInfo->fSize;
			ioEvent->fId = pkgInfo->fId;
			ioEvent->fVersion = pkgInfo->fVersion;
			ioEvent->fSrcType = pkgInfo->fSrcType;
			ioEvent->fFlags = pkgInfo->fFlags;
			ioEvent->fDate = pkgInfo->fDate;
			Ustrncpy(ioEvent->fName, pkgInfo->fName, kMaxPackageNameSize);
			break;
		}
	}
	if (pkgIndex == kIndexNotFound) {
		ioEvent->fSize = 0;
		ioEvent->fId = 0;
		ioEvent->fVersion = 0;
		ioEvent->fFlags = 0;
	}
	f1C = false;
	ioEvent->fEventErr = err;
	setReply(sizeof(CPkBackupEvent), ioEvent);
}


void
CPackageEventHandler::safeToDeactivatePackage(CPkSafeToDeactivateEvent * ioEvent)
{
	ioEvent->fIsSafeToDeactivate = true;
	ArrayIndex pkgIndex;
	NewtonErr err = searchPackageList(&pkgIndex, ioEvent->fPkgId);
	if (err == noErr) {
		CPackageBlock * pkgInfo = (CPackageBlock *)fPackageList->elementPtrAt(pkgIndex);
		CArrayIterator iter(pkgInfo->fParts, 0, pkgInfo->fParts->count() - 1, false);
		for (ArrayIndex i = iter.firstIndex(); iter.more(); i = iter.nextIndex()) {
			CInstalledPart * part = (CInstalledPart *)pkgInfo->fParts->elementPtrAt(i);
			if (part->f04 == 0) {
				ioEvent->fIsSafeToDeactivate = GetProtocolRegistry()->getInstanceCount(part->f0C) == 0;
				if (!ioEvent->fIsSafeToDeactivate)
					break;
			}
		}
	}
	ioEvent->fEventErr = err;
	setReply(sizeof(CPkSafeToDeactivateEvent), ioEvent);
}


void
CPackageEventHandler::removePackage(CPkRemoveEvent * ioEvent, bool inArg2, bool inArg3)
{
	ArrayIndex pkgIndex;
	ObjectId pkgId = ioEvent->fPkgId;
	f1C = true;
	f20 = ioEvent->f14;
	f24 = ioEvent->f18;
	NewtonErr err = searchPackageList(&pkgIndex, pkgId);
	if (err == noErr) {
		CPackageBlock * pkgInfo = (CPackageBlock *)fPackageList->elementPtrAt(pkgIndex);
		int r8 = 0;
		if (pkgInfo->fSignature == 'spcm')
			r8 = pkgInfo->f2C;

		SetCardReinsertReason(pkgInfo->fName, true);
		PartId partId;
		partId.packageId = pkgId;
		pkgInfo->fSignature = 'spcm';
		CArrayIterator iter(pkgInfo->fParts, r8, pkgInfo->fParts->count() - 1, false);
		for (ArrayIndex i = iter.firstIndex(); iter.more(); i = iter.nextIndex()) {
			CInstalledPart * part = (CInstalledPart *)pkgInfo->fParts->elementPtrAt(i);
			partId.partIndex = i;
			pkgInfo->f2C = i+1;
			removePart(partId, *part, inArg3);
		}
		SetCardReinsertReason(NULL, false);

		setPersistentHeap();
		delete pkgInfo->fParts;
		delete[] pkgInfo->fName;
		delete[] pkgInfo->fCopyright;
		fPackageList->removeElementsAt(pkgIndex, 1);
		setDefaultHeap();
	}
	f1C = false;
	DeregisterLoadedCodeWithDebugger(pkgId);
	ioEvent->fEventErr = err;
	if (inArg2)
		setReply(sizeof(CPkRemoveEvent), ioEvent);
}


void
CPackageEventHandler::initValidatePackageDriver(void)
{
	fValidatePackageDriver = (CValidatePackageDriver *)MakeByName("CValidatePackageDriver", "CValidatePackage");
}


NewtonErr
CPackageEventHandler::validatePackage(CPkBeginLoadEvent * ioEvent, CPackageIterator * inIter)
{
	NewtonErr err = noErr;
	if (ioEvent->fSrcType.format != kFixedMemory) {
		Ptr pkgBase = NULL;
		Ptr lastPart = NULL;
		if (ioEvent->fSrcType.format == kRemovableMemory) {
			ArrayIndex numOfParts;
			pkgBase = (Ptr)ioEvent->fSrc.mem.buffer;
			if (FLAGTEST(inIter->packageFlags(), 0x01000000)
			&&  (numOfParts = inIter->numberOfParts()) > 1) {
				lastPart = pkgBase + inIter->getPartDataOffset(numOfParts - 1);
			}
		}
		size_t pkgSize = inIter->packageSize();
#if 0
		err = fValidatePackageDriver->validateBegin(ioEvent->fName, pkgBase, pkgSize, lastPart);
		if (err != noErr && err != kOSErrPackageAlreadyExists && pkgBase != NULL) {
			CValidateBackupPipe pipe;
			pipe.init(fValidatePackageDriver);
			RDMParams parms;
			GetLargeObjectInfo(&parms, ioEvent->fSrc.stream.bufferId);
			err = BackupPackage(&pipe, parms.fStore, parms.fObjId);
			err = fValidatePackageDriver->validateEnd(err);
		}
#endif
	}
	return err;
}


NewtonErr
CPackageEventHandler::checkAndInstallPatch(PartInfo&, SourceType, const PartSource&)
{return noErr;}


NewtonErr
CPackageEventHandler::loadProtocolCode(void ** outCode, PartInfo& info, SourceType inSrcType, const PartSource& inSource)
{
	NewtonErr err = noErr;
	XTRY
	{
		if (IsStream(inSrcType)) {
			// create a buffer to stream into
			CShadowRingBuffer buf;
			buf.init(inSource.stream.bufferId, 0, 0);
			// pipe data into that buffer
			CPartPipe pipe;
			pipe.init(inSource.stream.messagePortId, &buf, true);
			// malloc persistent mem for code
			setPersistentHeap();
			*outCode = malloc(info.size);
			setDefaultHeap();
			XFAILIF(*outCode == NULL, err = MemError();)
			newton_try
			{
				bool isEOF = false;
				size_t size = info.size;
				pipe.readChunk(*outCode, size, isEOF);
			}
			newton_catch(exPipe)
			{
				err = kOSErrBadPackage;
			}
			end_try;
		} else {
			// IsMemory
			if (info.autoCopy) {
				setPersistentHeap();
				*outCode = malloc(info.size);
				setDefaultHeap();
				XFAILIF(*outCode == NULL, err = MemError();)
				memmove(*outCode, inSource.mem.buffer, info.size);
			} else {
				*outCode = inSource.mem.buffer;
			}
		}
	}
	XENDTRY;
	return err;
}


ULong
CPackageEventHandler::getUniquePackageId(void)
{
	ULong uid;
	ArrayIndex index;
	do {
		uid = rand() & 0x00FFFFFF;
		if (uid == 0)
			uid = 1;
	} while (searchPackageList(&index, uid) == noErr);
	return uid;
}



size_t
CPackageEventHandler::getPartSize(void)
{
	if (fPkg == NULL)
		return 0;
	size_t partEndOffset = fPartIndex < (fPkg->numberOfParts() - 1) ? fPkg->getPartDataOffset(fPartIndex + 1) : fPkg->packageSize();
	return partEndOffset - fPkg->getPartDataOffset(f10);
}


bool
CPackageEventHandler::loadNextPart(NewtonErr * outErr, bool * outArg2, bool * outArg3)
{
//r4: r7 r6 r5
//sp-74
	PartId thePart;	//sp6C
	ExtendedPartInfo partInfo;	//sp74
	NewtonErr err = noErr;	// r8
	bool isMore = true;	//r10
	if (fPartIndex < fPkg->numberOfParts()) {
printf("CPackageEventHandler::loadNextPart() index=%d\n", fPartIndex);
		newton_try
		{
			thePart.packageId = fPkgInfo->fId;
			thePart.partIndex = fPartIndex;
			//sp-0C
			fPkg->getPartInfo(fPartIndex, &partInfo);
			Ustrncpy(partInfo.packageName, fPkg->packageName(), kMaxPackageNameSize);
			partInfo.packageName[kMaxPackageNameSize] = 0;
			if (fPipe != NULL) {
				fPipe->setStreamSize(getPartSize());
			} else {
				fPartSource.mem.buffer = partInfo.data.address;
			}
			if (partInfo.kind == kFrames && partInfo.infoSize != 0 && !partInfo.compressed && IsMemory(fPkgInfo->fSrcType)) {
				char infoStr[kMaxInfoSize+1];
				memmove(infoStr, partInfo.info, partInfo.infoSize);
				infoStr[partInfo.infoSize] = 0;
				RegisterLoadedCodeWithDebugger(partInfo.data.address, infoStr, thePart.packageId);
			}
			VAddr loadedAddr;
			bool sp04;
			int sp08;
			err = installPart(&loadedAddr, &sp08, &sp04, thePart, partInfo, fPkgInfo->fSrcType, fPartSource);
			if (fPipe != NULL) {
				fPipe->seekEOF();
			}
			if (err == noErr) {
				if (partInfo.kind == kProtocol && !partInfo.compressed) {
					CClassInfo * driver = (CClassInfo *)loadedAddr;
					if (driver == NULL && IsMemory(fPkgInfo->fSrcType)) {
						driver = (CClassInfo *)partInfo.data.address;
					}
					if (driver != NULL) {
						char infoStr[64];
						strcpy(infoStr, driver->implementationName());
						RegisterLoadedCodeWithDebugger((void *)loadedAddr, infoStr, thePart.packageId);
					}
				}
				setPersistentHeap();
				CInstalledPart part(partInfo.type, partInfo.kind, sp08, partInfo.autoLoad, partInfo.autoCopy? true : (partInfo.autoLoad && IsStream(fPkgInfo->fSrcType)), partInfo.notify, sp04, loadedAddr);
				err = fPkgInfo->fParts->addElement(&part);
				setDefaultHeap();
				if (err == noErr) {	// do we really want to keep retrying to load this part?
					++fPartIndex;
				}
			}
		}
		newton_catch_all	// original says newton_catch("")
		{
			err = kOSErrBadPackage;
		}
		end_try;
	}

	if (err != noErr || fPartIndex == fPkg->numberOfParts())
	{
		if (fPipe != NULL) {
			fPipe->close();
			delete fPipe, fPipe = NULL;
			fBuffer = NULL;
		}
		newton_try
		{
			if (fPkg != NULL && fPkgInfo != NULL) {
				if (fPartIndex >= fPkg->numberOfParts()) {
					fPkgInfo->fSignature = 'spvd';
				} else {
					CPkRemoveEvent evt(fPkgInfo->fId, f20, f24);
					removePackage(&evt, false, true);
				}
			}
		}
		newton_catch_all	// original says newton_catch("")
		{ }
		end_try;
		*outArg2 = false;
		*outArg3 = false;

		if (fPkg != NULL) {
			newton_try
			{
				if (fPkg->forDispatchOnly()) {
					*outArg2 = true;
					CPkRemoveEvent evt(fPkgInfo->fId, f20, f24);
					removePackage(&evt, false, false);
				}
				*outArg3 = f64;
				f64 = false;
			}
			newton_catch_all	// original says newton_catch("")
			{ }
			end_try;
			delete fPkg, fPkg = NULL;
		}
		fPkgInfo = NULL;
		f1C = false;
		f60 = 0;
		isMore = false;
	}
	*outErr = err;
	return isMore;
}


NewtonErr
CPackageEventHandler::installPart(VAddr * outCode, int * outArg2, bool * outArg3, const PartId& inId, ExtendedPartInfo& inPart, SourceType inSrcType, const PartSource& inSource)
{
//r4: r5 r7 r6 r8 xx xx r10
//sp-0C
printf("CPackageEventHandler::installPart()\n");
	NewtonErr err = noErr;
	*outArg2 = 0;
	*outCode = NULL;
	ULong processorType = fPkg->processorTypeOfPart(fPartIndex);
	if (processorType == 0 || processorType == 0x1000) {
		if (inPart.autoLoad) {
			if (inPart.kind == kRaw) {
				if (inPart.type == kPatchPartType) {
					err = checkAndInstallPatch(inPart, inSrcType, inSource);
				}
			} else if (inPart.kind == kProtocol) {
				CClassInfo * protocolImpl;
				if (loadProtocolCode((void **)&protocolImpl, inPart, inSrcType, inSource) == noErr) {
					err = protocolImpl->registerProtocol();
					if (err == noErr) {
						*outCode = (VAddr)protocolImpl;
					} else if (protocolImpl != NULL) {
						if (IsStream(inSrcType) || (IsMemory(inSrcType) && inPart.autoCopy)) {
							free(protocolImpl);	// yes, really: protocol code was malloc’d
						}
					}
				}
			}
		}
		if (err == noErr && inPart.notify && inPart.type != kPatchPartType) {
			PartId existingPart;
			if ((err = searchRegistry(&existingPart.partIndex, &existingPart.packageId, inPart.type)) == noErr) {	// will be kOSErrNoSuchPackage when loading drivers
				if (f1C && f20 == existingPart.packageId) {
					existingPart.packageId = f24;
				}
				CUPort appPort(existingPart.packageId);
				if (*outCode) {
					inPart.data.address = (Ptr)*outCode;
				}
				CPkPartInstallEvent installEvent(inId, inPart, inSrcType, inSource);
				CPkPartInstallEventReply reply;
				size_t replySize;
printf("sending CPkPartInstallEvent to port %d\n", existingPart.packageId);
				err = appPort.sendRPC(&replySize, &installEvent, sizeof(CPkPartInstallEvent), &reply, sizeof(CPkPartInstallEventReply));
				if (err == noErr) {
					err = installEvent.fEventErr;
				}
				*outArg2 = reply.f10;
				*outArg3 = reply.f14;
			}
		}
		CClassInfo * protocolImpl = (CClassInfo *)*outCode;
		if (protocolImpl != NULL && err != noErr) {
			protocolImpl->deregisterProtocol();
			if (IsStream(inSrcType) || inPart.autoCopy) {
				free(protocolImpl);	// yes, really: protocol code was malloc’d
			}
			*outCode = NULL;
		}
	} else {
		*outArg3 = false;
	}
	return err;
}


void
CPackageEventHandler::removePart(const PartId& inId, const CInstalledPart& inPart, bool inArg3)
{
	if (inArg3 && inPart.f104) {
		PartId existingPart;
		if (searchRegistry(&existingPart.partIndex, &existingPart.packageId, inPart.fType) == noErr) {
			if (f1C && f20 == existingPart.packageId) {
				existingPart.packageId = f24;
			}
			CPkPartRemoveEvent removeEvent(inId, inPart.f04, inPart.fType, inPart.f08);
			CUPort appPort(existingPart.packageId);
			size_t replySize;
			appPort.sendRPC(&replySize, &removeEvent, sizeof(CPkPartRemoveEvent), NULL, 0, 2*kSeconds);
		}
	}
	CClassInfo * protocolImpl;
	if (inPart.f108 && (protocolImpl = inPart.f0C) != NULL && inPart.f04 == 0) {
		protocolImpl->deregisterProtocol();
		if (inPart.f101) {
			free(protocolImpl);	// yes, really: protocol code was malloc’d
		}
	}
}


NewtonErr
CPackageEventHandler::searchPackageList(ArrayIndex * outIndex, ULong inId)
{
	NewtonErr err = kOSErrNoSuchPackage;
	ArrayIndex index;
	CArrayIterator iter(fPackageList);
	for (index = iter.firstIndex(); iter.more(); index = iter.nextIndex()) {
		if (((CPackageBlock *)fPackageList->elementPtrAt(index))->fId == inId) {
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
	CArrayIterator iter(fPackageList);
	for (index = iter.firstIndex(); iter.more(); index = iter.nextIndex()) {
		if (Ustrcmp(((CPackageBlock *)fPackageList->elementPtrAt(index))->fName, inName) == 0) {
			*outIndex = index;
			err = noErr;
			break;
		}
	}
	return err;
}


NewtonErr
CPackageEventHandler::searchRegistry(ArrayIndex * outIndex, ObjectId * outPkgId, ULong inPartType)
{
	NewtonErr err = kOSErrPartTypeNotRegistered;
	ArrayIndex index;
	CArrayIterator iter(fRegistry);
	for (index = iter.firstIndex(); iter.more(); index = iter.nextIndex()) {
		RegistryInfo * info = (RegistryInfo *)fRegistry->safeElementPtrAt(index);
		if (info != NULL && info->fType == inPartType) {
			*outPkgId = info->f04;
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
	SetHeap(gPkgWorld->fDefaultHeap);
}

void
CPackageEventHandler::setPersistentHeap(void)
{
	SetHeap(gPkgWorld->fPersistentHeap);
}


#pragma mark -
/* -----------------------------------------------------------------------------
	Get package backup info.
	Useful for returning info to the PM iterator.
----------------------------------------------------------------------------- */

NewtonErr
cGetPackageBackupInfo(ArrayIndex index, UniChar * outName, size_t * outSize, ULong * outId, ULong * outVersion, SourceType * outSource, ArrayIndex * outIndex, ULong * outFlags, ULong * outTimestamp)
{
	NewtonErr err = noErr;
	PartSource source;
	CPkBackupEvent backupEvent(*outIndex, index, true, source, *gPkgWorld->getMyPort(), *gPkgWorld->getMyPort());
	CUPort pkgMgr(PackageManagerPortId());
	size_t replySize;
	gPkgWorld->releaseMutex();
	err = pkgMgr.sendRPC(&replySize, &backupEvent, sizeof(CPkBackupEvent), &backupEvent, sizeof(CPkBackupEvent));
	gPkgWorld->acquireMutex();
	*outIndex = backupEvent.fIndex;
	*outSize = backupEvent.fSize;
	*outId = backupEvent.fId;
	*outVersion = backupEvent.fVersion;
	*outSource = backupEvent.fSrcType;
	*outFlags = backupEvent.fFlags;
	*outTimestamp = backupEvent.fDate;
	Ustrncpy(outName, backupEvent.fName, kMaxPackageNameSize);
//	outName[kMaxPackageNameSize-1] = 0;		// original does not use -1 -- buffer overrun, surely? but redundant anyway b/c Ustrncpy adds the nul terminator
	return err;
}


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
	if (more()) {
		fIndex++;
		NewtonErr err = cGetPackageBackupInfo(kIndexNotFound, fPkgName, &fPkgSize, &fPkgId, &fPkgVersion, &fSource, &fIndex, &fPkgFlags, &fTimestamp);
		if (err) {
			fIndex = kIndexNotFound;
		}
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
	fPackage = NULL;
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
		directory
			package header
			parts directory
			directory data
		relocation header
			relocation entries (optional)
		part data
	Args:		inPackage		pointer to package in memory
	Return:	error code
----------------------------------------------------------------------------- */

NewtonErr
CPrivatePackageIterator::init(void * inPackage)
{
	NewtonErr err;
#if 1
	fPackage = new NewtonPackage(inPackage);
	fPkgDir = fPackage->directory();
	fMem = (Ptr)fPkgDir;		// watch this
#else
	fMem = (Ptr)inPackage;
	fPkgDir = (PackageDirectory *)inPackage;
#endif

	XTRY
	{
		XFAILIF(fMem == NULL, err = kOSErrBadPackage;)
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
	if (FLAGTEST(fPkgDir->flags, kRelocationFlag)) {
		fReloc = (RelocationHeader *)(fMem + sizeof(PackageDirectory) + inOtherInfoSize);
		if (fReloc->reserved == 0)
			*outRelocationInfoSize = fReloc->relocationSize;
		else
			err = kOSErrBadPackage;
	} else {
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
	if (fReloc) {
		fRelocInfo = (ULong *)fReloc + 1;
	}
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
	delete fPackage, fPackage = NULL;
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

NewtonErr
CPrivatePackageIterator::checkHeader(void)
{
	NewtonErr err = noErr;
	ArrayIndex i;
	// first 7 chars must be "package"
	for (i = 0; i < 7 && !err; ++i) {
		if (fPkgDir->signature[i] != kPackageMagicNumber[i]) {
			err = kOSErrBadPackage;
		}
	}
	if (err == noErr) {
		// next char must be 0 or 1
		err = kOSErrBadPackage;
		for ( ; i < 7+2 && err; ++i) {
			if (fPkgDir->signature[7] == kPackageMagicNumber[i]) {
				err = noErr;
			}
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
		XFAIL(fPkgDir->name.offset > fPkgDir->directorySize)
		// copyright text (if any) must lie within the greater directory
		XFAIL(fPkgDir->copyright.length > 0 && fPkgDir->copyright.offset > fPkgDir->directorySize)
//		XFAIL((fPkgDir->flags & 0xF000) != 0x1000)	// huh? cf. processorTypeOfPart()
		// verify all the part entries
		PartEntry * part = fPkgParts;
		for (ArrayIndex i = 0; i < numberOfParts(); ++i, ++part) {
			// if compressed, compressor name must lie within the greater directpry
			XFAIL(FLAGTEST(part->flags, kCompressedFlag) && part->compressor.offset > fPkgDir->directorySize)
			// info (if any) must lie within the greater directpry
			XFAIL(part->info.length > 0 && part->info.offset > fPkgDir->directorySize)
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
	if (inPartIndex < fPkgDir->numParts) {
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

	outInfo->autoLoad = FLAGTEST(part->flags, kAutoLoadPartFlag);
	outInfo->autoRemove = FLAGTEST(part->flags, kAutoRemovePartFlag);
	outInfo->compressed = FLAGTEST(part->flags, kCompressedFlag);
	outInfo->notify = FLAGTEST(part->flags, kNotifyFlag);
	outInfo->autoCopy = FLAGTEST(part->flags, kAutoCopyFlag);
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
	&&  fPkgParts != NULL) {
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
	fIsStreamSource = true;
	fPrivateIter.fMem = NULL;
	fPipe = inPipe;
}


CPackageIterator::CPackageIterator(void * inData)
{
	fIsStreamSource = false;
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
		if (fIsStreamSource) {
			XTRY
			{
				fPrivateIter.fPkgDir = new PackageDirectory;
				XFAILIF(fPrivateIter.fPkgDir == NULL, err = kOSErrNoMemory;)
				newton_try
				{
					bool isEOF;
					size_t sizeRead = sizeof(PackageDirectory);
					fPipe->readChunk(fPrivateIter.fPkgDir, sizeRead, isEOF);
				}
				newton_catch(exPipe)
				{
					err = (NewtonErr)(long)CurrentException()->data;
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
		err = (NewtonErr)(long)CurrentException()->data;
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
	if (fIsStreamSource) {
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
				err = (NewtonErr)(long)CurrentException()->data;
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
				err = (NewtonErr)(long)CurrentException()->data;
			}
			end_try;
		}
		XENDTRY;
	} else {
		fPrivateIter.computeSizeOfEntriesAndData(inPartEntryInfoSize, inDirectoryInfoSize);
	}
	return err;
}

NewtonErr
CPackageIterator::setupRelocationData(ULong inOtherInfoSize, ULong * outRelocationInfoSize)
{
	NewtonErr err = noErr;
	if ((fPrivateIter.fPkgDir->flags & kRelocationFlag) != 0) {
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
					err = (NewtonErr)(long)CurrentException()->data;
				}
				end_try;
				XFAILNOT(fPrivateIter.fReloc->reserved == 0, *outRelocationInfoSize = 0; err = kOSErrBadPackage;)
				*outRelocationInfoSize = fPrivateIter.fReloc->relocationSize;
			}
			XENDTRY;
		} else {
			fPrivateIter.setupRelocationData(inOtherInfoSize, outRelocationInfoSize);
		}
	} else {
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
	if (fPrivateIter.fReloc) {
		if (fIsStreamSource) {
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
					err = (NewtonErr)(long)CurrentException()->data;
				}
				end_try;
			}
			XENDTRY;
		} else {
			fPrivateIter.getRelocationChunkInfo();
		}
	}
	return err;
}

void
CPackageIterator::disposeDirectory(void)
{
	if (fIsStreamSource) {
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
	return fPrivateIter.fPkgDir ? (fPrivateIter.fPkgDir->flags & kAutoRemoveFlag) != 0 : false;
}

bool
CPackageIterator::copyProtected(void)
{
	return fPrivateIter.fPkgDir ? (fPrivateIter.fPkgDir->flags & kCopyProtectFlag) != 0 : false;
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
		if (fIsStreamSource) {
			outInfo->data.streamOffset = getPartDataOffset(inPartIndex);
		} else {
			outInfo->data.address = fPrivateIter.fMem + getPartDataOffset(inPartIndex);
		}
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
#include "ROMResources.h"
#include "LargeObjects.h"
#include "EndpointPipe.h"

extern Ref	ToObject(CStore * inStore);

extern "C" {
Ref	FGetPackages(RefArg inRcvr);
}


Ref
IteratorToPackageFrame(CPMIterator * inIter)
{
	RefVar pkgFrame(Clone(RA(canonicalPMIteratorPackageFrame)));

	SetFrameSlot(pkgFrame, SYMA(id), MAKEINT(inIter->packageId()));
	SetFrameSlot(pkgFrame, SYMA(size), MAKEINT(inIter->packageSize()));
	SetFrameSlot(pkgFrame, SYMA(title), MakeString(inIter->packageName()));
	SetFrameSlot(pkgFrame, SYMA(version), MAKEINT(inIter->packageVersion()));
	SetFrameSlot(pkgFrame, SYMA(timestamp), MAKEINT(inIter->timestamp()));
	SetFrameSlot(pkgFrame, SYMA(copyProtection), MAKEBOOLEAN(inIter->isCopyProtected()));

	PSSId storeId;
	CStore * storeObj;
	if (IdToStore(inIter->packageId(), &storeObj, &storeId) == noErr) {
		SetFrameSlot(pkgFrame, SYMA(store), ToObject(storeObj));
		SetFrameSlot(pkgFrame, SYMA(pssid), MAKEINT(storeId));
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
	for (iter.init(); iter.more(); iter.nextPackage()) {
		AddArraySlot(pkgs, IteratorToPackageFrame(&iter));
	}
	iter.done();
	return pkgs;
}


/* -----------------------------------------------------------------------------
	C P a c k a g e L o a d e r
----------------------------------------------------------------------------- */

class CPackageLoader
{
public:
					CPackageLoader(CEndpointPipe * inSource, SourceType inType);
					CPackageLoader(CPipe * inSource, SourceType inType);
					CPackageLoader(Ptr inSource, SourceType inType);
					~CPackageLoader();

	void			reset(void);
	NewtonErr	load(void);
	void			done(bool * outArg1, bool * outArg2);

private:
	Ptr			fMem;			//+00
	CPipe *		fPipe;		//+04
	SourceType	fSrcType;	//+08
	bool			fIsEndpoint;	//+10
	CRingBuffer *	fBuffer;	//+14
	ULong			fPkgId;		//+18
	CPackageLoaderEventHandler *	fEvtHandler;	//+1C
	bool			f20;
	bool			f21;
//size+24
};


// construct to load package over the wire
CPackageLoader::CPackageLoader(CEndpointPipe * inSource, SourceType inType)
{
	fSrcType = inType;
	fPipe = (CPipe *)inSource;
	fIsEndpoint = true;
	fEvtHandler = NULL;
	fBuffer = NULL;
}

// construct to load package from stream
CPackageLoader::CPackageLoader(CPipe * inSource, SourceType inType)
{
	fSrcType = inType;
	fPipe = inSource;
	fIsEndpoint = false;
	fEvtHandler = NULL;
	fBuffer = NULL;
}

// construct to load package from memory
CPackageLoader::CPackageLoader(Ptr inSource, SourceType inType)
{
	fSrcType = inType;
	fMem = inSource;
	fIsEndpoint = false;
	fEvtHandler = NULL;
	fBuffer = NULL;
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
	f20 = false;
	f21 = false;
	XTRY
	{
		fEvtHandler = new CPackageLoaderEventHandler;
		XFAIL(err = fEvtHandler->init(kPackageEventId))
		if (IsStream(fSrcType)) {
			fBuffer = new CRingBuffer;
			XFAILIF(fBuffer == NULL, err = kOSErrNoMemory;)
			XFAIL(err = fBuffer->init(256))
			fBuffer->makeShared(0);
#if 0
			PipeInfo pipe;	// sp00 size+10
			pipe.f00 = fPipe;
			pipe.f08 = fBuffer;
			if (fIsEndpoint) {
				newton_try
				{
					((CEndpointPipe *)fPipe)->removeFromAppWorld();
				}
				newton_catch(exPipe)
				{
					err = (NewtonErr)(long)CurrentException()->data;
				}
				end_try;
				XFAIL(err)
			}
			//sp-88
			CPipeApp loader(pipe, fIsEndpoint);	//sp00
			XFAIL(err = loader.init('pipe', true, kSpawnedTaskStackSize))
			//sp-18
			OpaqueRef thing, spec;
			CUNameServer ns;
			XFAIL(err = ns.lookup("pipe", kUPort, &thing, &spec))
//			sp10 = fBuffer->fSharedMem;	// don’t believe these are used
//			sp14 = thing;
#endif
		}
		PartSource source;
		source.mem.buffer = fMem;
		CPkBeginLoadEvent loadEvent(fSrcType, source, *gPkgWorld->getMyPort(), *gPkgWorld->getMyPort(), true);
		CUPort pkgMgr(PackageManagerPortId());
		size_t replySize;
		gPkgWorld->releaseMutex();
		err = pkgMgr.sendRPC(&replySize, &loadEvent, sizeof(CPkBeginLoadEvent), &loadEvent, sizeof(CPkBeginLoadEvent));
		gPkgWorld->acquireMutex();
		if (err == noErr) {
			err = loadEvent.fEventErr;
		}
		fPkgId = loadEvent.fUniqueId;
		XFAIL(err)
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
	if (IsStream(fSrcType) && fIsEndpoint) {
		NewtonErr err = noErr;
		newton_try
		{
			((CEndpointPipe *)fPipe)->addToAppWorld();
		}
		newton_catch(exPipe)
		{
			err = (NewtonErr)(long)CurrentException()->data;
		}
		end_try;
		if (fEvtHandler)
			delete fEvtHandler, fEvtHandler = NULL;
		if (fBuffer)
			delete fBuffer, fBuffer = NULL;
		gPackageSemaphore->release();
	}
}


#pragma mark -
/* -----------------------------------------------------------------------------
	C P a c k a g e L o a d e r E v e n t H a n d l e r
	The handler for events passed to the Package Loader.
----------------------------------------------------------------------------- */
#include "AppWorld.h"

bool
CPackageLoaderEventHandler::testEvent(CEvent * inEvent)
{
	CPkBaseEvent * evt = (CPkBaseEvent *)inEvent;
	return evt->fEventCode == kBeginLoadPkgEventId || evt->fEventCode == kRemovePkgEventId || evt->fEventCode == kBackupPkgEventId;
}


void
CPackageLoaderEventHandler::eventHandlerProc(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent)
{
	gAppWorld->eventTerminateLoop();
}


void
CPackageLoaderEventHandler::eventCompletionProc(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent)
{
	gAppWorld->eventTerminateLoop();
}
