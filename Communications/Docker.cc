/*
	File:		Docker.cc

	Contains:	Docker implementation.
					The Docker uses the Dock Protocol to communicate with a desktop machine.

	Written by:	Newton Research Group, 2012.
*/

#include "Docker.h"
#include "DockerErrors.h"
#include "NewtonScript.h"
#include "Lookup.h"
#include "Iterators.h"
#include "UStringUtils.h"
#include "StreamObjects.h"
#include "ROMResources.h"
#include "OSErrors.h"

#include "StoreWrapper.h"
#include "Cursors.h"
#include "Soups.h"

#include "AppWorld.h"


extern Ref			GetStores(void);

extern ULong		Ticks(void);
extern Ref			FYieldToFork(RefArg rcvr);

DeclareException(exLongError, exRootException);

#pragma mark -

Ref
FConnBuildStoreFrame(RefArg rcvr, RefArg inStore, RefArg inArg3)
{
	// INCOMPLETE
	return NILREF;
}


#pragma mark CEzPipeProtocol
/* -------------------------------------------------------------------------------
	C E z P i p e P r o t o c o l
	A class to handle reading/writing protocol event headers.
------------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------------
	Initialize the protocol.
	Set up event class and id for event header.
	Args:		inEvtClass		typically 'newt'
				inEvtId			typically 'dock'
	Return:	--
------------------------------------------------------------------------------- */

void
CEzPipeProtocol::protocolInit(EventClass inEvtClass, EventId inEvtId)
{
	fEvtClass = inEvtClass;
	fEvtId = inEvtId;
}


/* -------------------------------------------------------------------------------
	Scan the input stream until an event header is encountered.
	Return the event type and its length.
	Args:		outCommand
				outLength
	Return:	--
------------------------------------------------------------------------------- */

void
CEzPipeProtocol::findDockerHeader(EventType& outCommand, size_t& outLength)
{
	DockEventHeader header;
	bool isEOF;
	size_t chunkLen = 2 * sizeof(ULong);
	fPipe->readChunk(&header.evtClass, chunkLen, isEOF);
	while (header.evtClass != fEvtClass && header.evtId != fEvtId)
	{
		// keep reading bytes
		UChar streamChar;
		*fPipe >> streamChar;
		// shift into the header
		header.evtClass = (header.evtClass << 8) || (header.evtId >> 24);
		header.evtId = (header.evtId << 8) || streamChar;
		// until we match event class and id: 'newt' 'dock' (or whatever)
	}
	chunkLen = 2 * sizeof(ULong);
	fPipe->readChunk(&header.tag, chunkLen, isEOF);
	outCommand = header.tag;
	outLength = header.length;
}


/* -------------------------------------------------------------------------------
	Assume the input stream is aligned on an event header --
	return the event type and its length.
	Args:		outCommand
				outLength
	Return:	--
------------------------------------------------------------------------------- */

void
CEzPipeProtocol::readDockerHeader(EventType& outCommand, size_t& outLength)
{
	DockEventHeader header;
	bool isEOF;
	size_t chunkLen = 2 * sizeof(ULong);
	fPipe->readChunk(&header.evtClass, chunkLen, isEOF);
	if (header.evtClass != fEvtClass || header.evtId != fEvtId)
		ThrowErr(exLongError, kDockErrBadHeader);
	chunkLen = 2 * sizeof(ULong);
	fPipe->readChunk(&header.tag, chunkLen, isEOF);
	outCommand = header.tag;
	outLength = header.length;
}


/* -------------------------------------------------------------------------------
	Write an event header to the output stream.
	Args:		inCommand
				inNoMore		true => no data for this command, it can go now
	Return:	--
------------------------------------------------------------------------------- */

void
CEzPipeProtocol::writeDockerHeader(EventType inCommand, bool inNoMore)
{
	DockEventHeader header;
	header.evtClass = fEvtClass;
	header.evtId = fEvtId;
	header.tag = inCommand;
	header.length = 0;
	if (inNoMore)
	{
		fPipe->writeChunk(&header, sizeof(header), false);
		fPipe->flushWrite();
	}
	else
	{
		// more to come
		// we don’t know the size yet so don’t write that
		fPipe->writeChunk(&header, sizeof(header)-sizeof(header.length), false);
	}
}


#pragma mark -
#pragma mark CDocker
/* -------------------------------------------------------------------------------
	C D o c k e r
------------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------------
	Constructor.
------------------------------------------------------------------------------- */

CDocker::CDocker()
{
	f0C = NILREF;
	fTargetStore = NILREF;
	fTargetSoup = NILREF;
	f18 = NILREF;
	f1C = NILREF;
	fRegdExtensionFns = NILREF;
	fDesktopApps = NILREF;
	fCursors = NULL;
	f40 = NILREF;
	fDesktopKey.hi = 0;
	fDesktopKey.lo = 0;
	fNewtonKey.hi = 0;
	fNewtonKey.lo = 0;
	fAC = false;
	fDesktopType = 2;
	fAF = false;
	fProtocolVersion = kBaseProtocolVersion;
	fEvtLength = 0;
	fError = noErr;
	fPipe = NULL;
	fB1 = false;
	fIsSelectiveSync = false;
	f2C = false;
	f2D = false;
	f2E = 0;
	f28 = NILREF;
	f2F = false;
	f30 = false;
	fLengthRead = 0;
	f31 = false;
	fLengthWritten = 0;
	fIsLocked = false;
	fVBOCompression = kCompressedVBOs;
	fAD = false;
	fAE = false;
	f7C = NULL;
	fRegdExtensionIds = NULL;
	f58 = 0;
	f5C = 0;
	f60 = 2;
	f64 = 2;
	f68 = 0;
	f6C = NILREF;
	f70 = NILREF;
	fB3 = false;
	fB4 = false;
	fSessionType = kSettingUpSession;
}


/* -------------------------------------------------------------------------------
	Destructor.
------------------------------------------------------------------------------- */

CDocker::~CDocker()
{
	tossDataStructures();
}


void
CDocker::tossDataStructures(void)
{
	waitForStopToComplete();
	if (fPipe)
	{
		newton_try
		{
			delete fPipe;	//it’s a virtual, anyway
		}
		newton_catch_all
		{ }
		end_try;
		fPipe = NULL;
	}
	if (fCursors)
	{
		newton_try
		{
			delete fCursors;
		}
		newton_catch_all
		{ }
		end_try;
		fCursors = NULL;
	}
	if (f7C)
	{
		newton_try
		{
			delete f7C;
		}
		newton_catch_all
		{ }
		end_try;
		f7C = NULL;
	}
	if (fRegdExtensionIds)
	{
		newton_try
		{
			delete fRegdExtensionIds;
		}
		newton_catch_all
		{ }
		end_try;
		fRegdExtensionIds = NULL;
	}
}


void
CDocker::waitForStopToComplete()
{
	if (fAF)
	{
		ULong now = Ticks();
		for (ULong elapsedTicks = 0; fB0 && elapsedTicks < 180; elapsedTicks = Ticks() - now)
		{
			FYieldToFork(RA(NILREF));
		}
		fAF = false;
		fB0 = false;
	}
}


/* -------------------------------------------------------------------------------
	Lock.
------------------------------------------------------------------------------- */

bool
CDocker::getDockerLock(void)
{
	return fIsLocked;
}


bool
CDocker::waitAndLockDocker(void)
{
	bool isAcquired = false;
	ULong now = Ticks();
	for (ULong elapsedTicks = 0; getDockerLock() && elapsedTicks < 600; elapsedTicks = Ticks() - now)
	{
		FYieldToFork(RA(NILREF));
	}
	if (!getDockerLock())
	{
		fIsLocked = true;
		isAcquired = true;
	}
	return isAcquired;
}


void
CDocker::unlockDocker(void)
{
	fIsLocked = false;
}


void
CDocker::outOfMemory(void)
{
	ThrowErr(exOutOfMemory, kOSErrNoMemory);
}


#pragma mark -

NewtonErr
CDocker::doConnection(RefArg inArg1, RefArg inArg2, RefArg inArg3, bool& outArg4)
{
//	r4: r7 r6 r5 r8
	if (fB3)
		return noErr;

	fAF = false;
	bool sp00 = false;
	outArg4 = true;
	fError = noErr;
	if (fAC == 0)
		fError = kDockErrBadPasswordError;
	else
	{
		waitAndLockDocker();
		f0C = inArg2;
		f1C = inArg3;
		fAD = NOTNIL(inArg1);
		f40 = NILREF;
		freeCurrentStore();
		fTargetSoup = NILREF;
		newton_try
		{
			//r7 = 1;
			fError = gAppWorld->fork(NULL);
			if (fError)
				ThrowErr(exLongError, fError);
			if (fAE)
				compatibilityHacks();
			else
			{
				if (f2F && bytesAvailable(true) == 0)
				{
					if (fSessionType == kSynchronizeSession)
						writeDockerHeader(kDRequestToSync, true);
					else if (fSessionType == kRestoreSession)
						writeDockerHeader(kDRequestToRestore, true);
					else
						writeResult(noErr);
				}
				outArg4 = false;
				while (fError == noErr && !outArg4 && !sp00 && !fAF)
				{
					readDockerHeader(fEvtTag, fEvtLength);
					processCommand(outArg4, sp00);
					if (fSessionType == 9)
					{
						keyboardProcessCommand();
						sp00 = true;
						f2F = true;
					}
				}
			}

		}
		newton_catch_all
		{
			processException(CurrentException());
		}
		end_try;
	}
	if (fAF)
	{
		if (!sp00)
			outArg4 = cleanUpIfStopping(outArg4);
		if (fError == kDockErrReadEntryError)
			fError = noErr;
	}
	if (fError == noErr && fSessionType == kSynchronizeSession && sp00)
	{
		fIsSelectiveSync = true;
	}
	f6C = NILREF;
	f70 = NILREF;
	f60 = 2;
	freeCurrentStore();
	fTargetSoup = NILREF;
	f28 = NILREF;
	cleanUpIfError(outArg4);
	unlockDocker();
	return fError;
}


void
CDocker::connect(RefArg inArg1, RefArg inOptions, RefArg inPassword)
{
//	r4:r6 r9 r5
	fB3 = false;
	fError = noErr;

	waitAndLockDocker();

	newton_try
	{
		f20 = inArg1;
		fPipe = new CEzEndpointPipe;
		if (fPipe == NULL)
			outOfMemory();
		ULong timeout = kDefaultTimeout;
		Ref timeoutOption = GetFrameSlot(inOptions, SYMA(connectTimeout));
		if (NOTNIL(timeoutOption))
			timeout = RINT(timeoutOption);
		fPipe->init(inOptions, timeout*kSeconds);
		fB1 = true;
		if (fAF)
			ThrowErr(exLongError, kDockErrProtocolError);
		timeoutOption = GetFrameSlot(inOptions, SYMA(idleTimeout));
		if (NOTNIL(timeoutOption))
			fPipe->setTimeout(RINT(timeoutOption)*kSeconds);
		protocolInit(kNewtEventClass, kDockEventId);

		writeLong(kDRequestToDock, kBaseProtocolVersion);

		readDockerHeader(fEvtTag, fEvtLength);
		if (fEvtTag == kDLoadPackage)
		{
			fAE = true;
			fAC = true;
			fSessionType = kLoadPackageSession;
		}
		else if (fEvtTag == kDInitiateDocking)
		{
			if (fAF)
				ThrowErr(exLongError, kDockErrProtocolError);
			readInitiateDocking();
			writeNewtonName();
			readDockerHeader(fEvtTag, fEvtLength);
			if (fEvtTag == kDDesktopInfo)
			{
				readDesktopInfo();
				readDockerHeader(fEvtTag, fEvtLength);
			}
			if (fProtocolVersion <= kBaseProtocolVersion)
			{
				if (fAF)
					ThrowErr(exLongError, kDockErrIncompatibleProtocol);
				fAC = true;
			}
			if (fEvtTag == kDWhichIcons)
			{
				setWhichIcons();
				readDockerHeader(fEvtTag, fEvtLength);
			}
			if (fEvtTag == kDSetTimeout)
			{
				*fPipe >> timeout;
				fPipe->setTimeout(timeout*kSeconds);
			}
			else if (fEvtTag == kDResult)
			{
				fError = readResult();
				if (fError != noErr)
					ThrowErr(exLongError, fError);
			}
			else
				ThrowErr(exLongError, kDockErrProtocolError);

			if (fAF)
				ThrowErr(exLongError, kDockErrProtocolError);

			if (fProtocolVersion >= kDanteProtocolVersion)
			{
				writePassword(inPassword);
				readPassword();
			}

			if (fAF)
				ThrowErr(exLongError, kDockErrProtocolError);

			f2F = true;
		}
		else
			ThrowErr(exLongError, fEvtTag == kDRequestToDock ? kDockErrDesktopError : kDockErrBadHeader);
	}
	newton_catch_all
	{
		processException(CurrentException());
	}
	end_try;
	cleanUpIfError(fAF);
	unlockDocker();
}


NewtonErr
CDocker::processBuiltinCommand(bool& outDisconnected)
{
	if (fB3)
		return noErr;

	fError = noErr;
	if (fAC == 0)
		return kDockErrBadPasswordError;

	waitAndLockDocker();
	outDisconnected = false;
	bool sp00 = false;
	newton_try
	{
		processCommand(outDisconnected, sp00);
	}
	newton_catch_all
	{
		processException(CurrentException());
	}
	end_try;
	outDisconnected = cleanUpIfStopping(outDisconnected);
	cleanUpIfError(outDisconnected);
	unlockDocker();

	return fError;
}


void
CDocker::stop(void)
{
	if (getDockerLock())
	{
		fAF = true;
		fB0 = false;
		if (fPipe)
			fPipe->abort();
		fB0 = true;
	}
}



#pragma mark Store

Ref
CDocker::makeStoreFrame(RefArg inStore)
{
	return FConnBuildStoreFrame(RA(NILREF), inStore, RA(TRUEREF));
}


void
CDocker::writeStoreNames(void)
{
	RefVar allStores(GetStores());
	RefVar storeFrames(MakeArray(Length(allStores)));
	CObjectIterator iter(allStores);
	for (ArrayIndex i = 0; !iter.done(); iter.next(), i++)
	{
		SetArraySlot(storeFrames, i, makeStoreFrame(iter.value()));
	}
	writeRef(kDStoreNames, storeFrames);
	fPipe->flushWrite();
}


void
CDocker::freeCurrentStore(void)
{
	if (NOTNIL(fTargetStore))
	{
		NSSend(fTargetStore, SYMA(MarkNotBusy), GetFrameSlot(f20, SYMA(appSymbol)));
		fTargetStore = NILREF;
	}
}


void
CDocker::reserveCurrentStore(RefArg inArg)
{
	if (NOTNIL(fTargetStore))
		freeCurrentStore();

	NSSend(fTargetStore, SYMA(MarkBusy), GetFrameSlot(f20, SYMA(appSymbol)), GetFrameSlot(f20, SYMA(appName)));
}



#pragma mark Soup


void
CDocker::writeSoupNames(void)
{
	writeDockerHeader(kDSoupNames, false);
	;
}


void
CDocker::verifySoup(void)
{
	if (fTargetSoup == NULL)
		ThrowErr(exLongError, kDockErrBadCurrentSoup);
}


#pragma mark Cursor

void
CDocker::remoteQuery(void)
{
	XTRY
	{
		RefVar queryRef(readRef(fTargetStore));
		RefVar querySpec(GetFrameSlot(queryRef, SYMA(querySpec)));
		RefVar soupName(GetFrameSlot(queryRef, SYMA(soupName)));
		if (NOTNIL(soupName) && IsString(soupName) && Ustrlen(GetUString(soupName)) > 0)
		{
			fTargetSoup = StoreGetSoup(fTargetStore, soupName);
			XFAILIF(ISNIL(fTargetSoup), writeResult(kDockErrSoupNotFound);)
			setupSoup();
		}
		verifySoup();
		RefVar spm04(SoupQuery(fTargetSoup, querySpec));
		if (fCursors == NULL)
		{
			fCursors = new CCursorArray();
			XFAILIF(fCursors == NULL, outOfMemory();)
		}
		writeLong(kDLongData, fCursors->add(spm04));
	}
	XENDTRY;
}


Ref
CDocker::remoteGetCursor(void)
{}


void
CDocker::remoteCursorGotoKey(void)
{}


void
CDocker::remoteCursorMap(void)
{}


void
CDocker::remoteCursorEntry(void)
{
	RefVar crsr(remoteGetCursor());
	writeRef(kDEntry, CursorEntry(crsr));
}


void
CDocker::remoteCursorMove(void)
{}


void
CDocker::remoteCursorNext(void)
{}


void
CDocker::remoteCursorPrev(void)
{}


void
CDocker::remoteCursorReset(void)
{}


void
CDocker::remoteCursorResetToEnd(void)
{}


void
CDocker::remoteCursorWhichEnd(void)
{}


void
CDocker::remoteCursorCountEntries(void)
{}


void
CDocker::remoteCursorFree(void)
{
	ArrayIndex index;
	*fPipe >> index;
	if (fCursors)
		fCursors->remove(index);
	writeResult(noErr);
}


#pragma mark -

bool
CDocker::cleanUpIfStopping(bool inArg1)
{
	if (fAF && !inArg1)
	{
		waitForStopToComplete();
		NewtonErr r9 = 0xFFFFC17B;
		if (fError == 0 || fError == r9)
		{
			if (fProtocolVersion <= kBaseProtocolVersion)
				return true;
			if (fError != noErr)
				r9 = fError;
			fError = noErr;
			newton_try
			{
				// cancel the operation
				writeDockerHeader(kDOperationCanceled, true);
				bool doSyncStream = true;
				// wait for acknowledge
				while(fError ==noErr && fEvtTag != kDOpCanceledAck && inArg1 == 0)
				{
					if (doSyncStream)
					{
						findDockerHeader(fEvtTag, fEvtLength);
						doSyncStream = false;
					}
					else
						readDockerHeader(fEvtTag, fEvtLength);
					if (fEvtTag == kDDisconnect)
						inArg1 = true;
					flushCommand();
				}
				f2F = true;
			}
			newton_catch_all
			{
				processException(CurrentException());
			}
			end_try;
			if (fError == noErr)
				fError = r9;
		}
	}
	return inArg1;
}


void
CDocker::processCommand(bool& outDisconnected, bool& outDone)
{
	outDisconnected = false;
	outDone = false;
	f2F = false;
	if (checkProtocolExtension(fEvtTag, outDone) == false
	&&  checkProtocolPatch(fEvtTag, outDone) == false)
	{
		switch (fEvtTag)
		{
// Protocol Control
		case kDGetPassword:
			doGetPassword();
			break;
		case kDWhichIcons:
			setWhichIcons();
			break;
		case kDSetTimeout:
			setTimeout();
			break;

		case kDHello:
			flushCommand();
			break;
		case kDOperationCanceled:
			outDone = true;
			writeDockerHeader(kDOpCanceledAck, true);
			break;
		case kDOperationDone:
			outDone = true;
			break;
		case kDDisconnect:
			outDisconnected = true;
			break;

		case kDUnknownCommand:
			*fPipe >> fEvtTag;
			ThrowErr(exLongError, kDockErrEntryNotFound);
			break;

// Newton 1 protocol
		case kDGetInheritance:
			writeInheritanceFrame();
			break;
		case kDGetPatches:
			writePatches();
			break;
		case kDRestorePatch:
			doRestorePatch();
			break;

// Protocol Extensions
		case kDRegProtocolExtension:
			readProtocolExtension();
			writeResult(fError);
			break;
		case kDRemoveProtocolExtension:
			readRemoveProtocolExtension();
			break;

// Store
		case kDGetDefaultStore:
			writeDefaultStore();
			break;
		case kDGetStoreNames:
			writeStoreNames();
			break;
		case kDSetStoreName:
			{
				RefVar name(readRef(fTargetStore));
				StoreSetName(fTargetStore, name);
				writeResult(noErr);
			}
			break;
		case kDSetStoreSignature:
			setStoreSignature();
			break;
		case kDSetCurrentStore:
			setCurrentStore(false);
			break;
		case kDSetStoreGetNames:
			setCurrentStore(true);
			break;
		case kDSetStoreToDefault:
			setStoreToDefault();
			writeResult(noErr);
			break;

		case kDSetVBOCompression:
			*fPipe >> fVBOCompression;
			break;

// Soup
		case kDSetSoupGetInfo:
			setCurrentSoup(true);
			break;
		case kDGetSoupNames:
			writeSoupNames();
			break;
		case kDGetSoupIDs:
			writeSoupIDs();
			break;
		case kDGetIndexDescription:
			writeIndexDescription(false);
			break;
		case kDGetChangedIndex:
			writeIndexDescription(true);
			break;
		case kDGetSoupInfo:
			writeSoupInfo(false);
			break;
		case kDSetSoupInfo:
			setSoupInfoFrame();
			writeResult(fError);
			break;
		case kDSetSoupSignature:
			setSoupSignature();
			break;
		case kDSetCurrentSoup:
			setCurrentSoup(false);
			break;
		case kDCreateSoup:
			createSoup();
			break;
		case kDCreateDefaultSoup:
			createSoupFromSoupDef();
			break;
		case kDDeleteSoup:
		case kDEmptySoup:
			emptyOrDelete(fEvtTag);
			break;
		case kDSendSoup:
			sendSoup();
			break;
		case kDBackupSoup:
			backupSoup();
			break;
		case kDGetChangedIDs:
			writeChangedIDs();
			break;

// Soup Entries
		case kDAddEntry:
			addEntry(false);
			break;
		case kDAddEntryWithUniqueID:
			addEntry(true);
			break;
		case kDReturnEntry:
			returnEntry(kDEntry);
			break;
		case kDReturnChangedEntry:
			returnEntry(kDChangedEntry);
			break;
		case kDChangedEntry:
			changeEntry();
			break;
		case kDDeleteEntries:
			deleteEntries();
			break;

// Cursor
		case kDQuery:
			remoteQuery();
			break;
		case kDCursorGotoKey:
			remoteCursorGotoKey();
			break;
		case kDCursorMap:
			remoteCursorMap();
			break;
		case kDCursorEntry:
			remoteCursorEntry();
			break;
		case kDCursorMove:
			remoteCursorMove();
			break;
		case kDCursorNext:
			remoteCursorNext();
			break;
		case kDCursorPrev:
			remoteCursorPrev();
			break;
		case kDCursorReset:
			remoteCursorReset();
			break;
		case kDCursorResetToEnd:
			remoteCursorResetToEnd();
			break;
		case kDCursorWhichEnd:
			remoteCursorWhichEnd();
			break;
		case kDCursorCountEntries:
			remoteCursorCountEntries();
			break;
		case kDCursorFree:
			remoteCursorFree();
			break;

// Packages
		case kDLoadPackage:
			if (fSessionType == kRestoreSession)
			{
				if (NOTNIL(fTargetStore))
					fTargetSoup = StoreGetSoup(fTargetStore, RA(STRextrasSoupName));
			}
			readPackage();
			writeResult(fError);
			if (fSessionType != kRestoreSession && fProtocolVersion == kDanteProtocolVersion)
				outDone = true;
			f2F = true;
			break;
		case kDRemovePackage:
			doRemovePackage();
			break;
		case kDRestorePackage:
			doRestorePackage();
			break;
		case kDGetPackageInfo:
			getPackageInfo();
			break;

// Function/Method Calls
		case kDCallGlobalFunction:
			callFunction(true);
			break;
		case kDCallRootMethod:	// definitely crmf
			callFunction(false);
			break;

		case kDResult:
			fError = readResult();
			break;
		case kDResultString:
			fError = readResultString();
			break;

// Sync
		case kDDesktopInControl:
			fSessionType = kSettingUpSession;
			f2F = false;
			break;
		case kDRequestToSync:
			fSessionType = kSynchronizeSession;
			writeResult(noErr);
			break;
		case kDRequestToRestore:
			fSessionType = kRestoreSession;
			writeResult(noErr);
			break;
		case kDRequestToInstall:
			fSessionType = kLoadPackageSession;
			writeResult(noErr);
			break;

		case kDGetSyncOptions:
			writeSyncOptions();
			break;
		case kDLastSyncTime:
			*fPipe >> f74;
			f74 &= 0x1FFFFFFF;
			f78 = RealClock();
			writeLong(kDCurrentTime, f78);
			break;
		case kDSourceVersion:
			readSourceVersion();
			break;

// Slips
		case 'dslp':	// is this what the spec calls 'stxt'?
			doDisplaySlip();
			break;
		case kDImportParametersSlip:
			doImportParametersSlip();
			break;

// Debug
		case kDTest:
			testMessage();
			break;
		case kDRefTest:
			testRefMessage();
			break;

		}
	}

	if (outDone)
		f2F = true;
}


void
CDocker::readPackage(void)
{}


void
CDocker::doRemovePackage(void)
{
	RefVar pkgName(readRef(fTargetStore));
	RefVar pkgEntry(NSCallGlobalFn(SYMA(GetPackageEntry), pkgName, fTargetStore));
	if (NOTNIL(pkgEntry))
		NSCallGlobalFn(SYMA(RemovePackage), pkgEntry);
	writeResult(noErr);
	addChangedSoup(SYMA(changed), true);
}


void
CDocker::readCurrentSoup(void)
{}


/* -------------------------------------------------------------------------------
	Read protocol extension from the pipe.
	Args:		--
	Return:	--
	Protocol data:
		EventType	kDRegProtocolExtension	| already read
		Size			length						|
		ULong			command id
		Ref			function
------------------------------------------------------------------------------- */

void
CDocker::readProtocolExtension(void)
{
	// stream in the command id
	ULong extnId;
	*fPipe >> extnId;

	// stream in the protocol extension function for that id
	RefVar fn(readRef(RA(NILREF)));

	// install it
	fError = installProtocolExtension(RA(NILREF), fn, extnId);
}


NewtonErr
CDocker::installProtocolExtension(RefArg inName, RefArg inFn, ULong inId)
{
	if (fB3)
		return noErr;

	fError = noErr;
	newton_try
	{
		if (inId == 0)
		{
			// no id -- try the name
			if (IsString(inName) && Ustrlen(GetUString(inName)) >= 4)
			{
				RefVar fourCharStr(Substring(inName, 0, 4));
				char str[8];	// original says 20
				ConvertFromUnicode(GetUString(fourCharStr), str);
				inId = *(ULong*)str;
#if defined(hasByteSwapping)
				inId = BYTE_SWAP_LONG(inId);
#endif
			}
		}
		XTRY
		{
			XFAILIF(inId == 0, ThrowErr(exLongError, kDockErrProtocolExtAlreadyRegistered);)
			if (fRegdExtensionIds == NULL)
			{
				// create the list of registered extensions
				fRegdExtensionIds = new CDockerDynArray;
				XFAILIF(fRegdExtensionIds == NULL, outOfMemory();)
				fRegdExtensionFns = MakeArray(0);
			}
			ArrayIndex index;
			XFAILIF(fRegdExtensionIds->find(inId) != kIndexNotFound, fError = kDockErrProtocolExtAlreadyRegistered;)
			XFAIL(fError = fRegdExtensionIds->addAndReplaceZero(inId, index))
			if (index < Length(fRegdExtensionFns))
				SetArraySlot(fRegdExtensionFns, index, inFn);
			else
				AddArraySlot(fRegdExtensionFns, inFn);
		}
		XENDTRY;
	}
	newton_catch_all
	{
		processException(CurrentException());
	}
	end_try;

	cleanUpIfStopping(false);
	if (fError != kDockErrProtocolExtAlreadyRegistered)
		cleanUpIfError(false);

	return fError;
}


/* -------------------------------------------------------------------------------
	Read remove protocol extension from the pipe.
	Args:		--
	Return:	--
	Protocol data:
		EventType	kDRemoveProtocolExtension	| already read
		Size			length							|
		ULong			command id
------------------------------------------------------------------------------- */

void
CDocker::readRemoveProtocolExtension(void)
{
	// stream in the command id
	ULong extnId;
	*fPipe >> extnId;

	// remove it
	fError = removeProtocolExtension(RA(NILREF), extnId);
}


NewtonErr
CDocker::removeProtocolExtension(RefArg inName, ULong inId)
{
	if (fB3)
		return noErr;

	fError = noErr;
	newton_try
	{
		if (inId == 0)
		{
			// no id -- try the name
			if (IsString(inName) && Ustrlen(GetUString(inName)) >= 4)
			{
				RefVar fourCharStr(Substring(inName, 0, 4));
				char str[8];	// original says 20
				ConvertFromUnicode(GetUString(fourCharStr), str);
				inId = *(ULong*)str;
#if defined(hasByteSwapping)
				inId = BYTE_SWAP_LONG(inId);
#endif
			}
		}
		ArrayIndex index;
		if (fRegdExtensionIds != NULL
		&&  (index = fRegdExtensionIds->find(inId)) != kIndexNotFound)
		{
			// nil out the id and fn
			fRegdExtensionIds->replace(index, 0);
			SetArraySlot(fRegdExtensionFns, index, RA(NILREF));
		}
	}
	newton_catch_all
	{
		processException(CurrentException());
	}
	end_try;

	cleanUpIfStopping(false);
	cleanUpIfError(false);

	return fError;
}


bool
CDocker::checkProtocolExtension(ULong inId, bool& ioDone)
{
	ArrayIndex index;
	if (fRegdExtensionIds
	&&  (index = fRegdExtensionIds->find(inId)) != kIndexNotFound)
	{
		f30 = true;
		bool wasLocked = getDockerLock();
		unlockDocker();
		ioDone = NOTNIL(NSCall(GetArraySlot(fRegdExtensionFns, index)));
		f30 = false;
		if (wasLocked)
			waitAndLockDocker();
		return true;
	}
	return false;
}


bool
CDocker::checkProtocolPatch(ULong inId, bool& ioDone)
{
	// this really does nothing
	return false;
}


#pragma mark -
#pragma mark Reading

/* -------------------------------------------------------------------------------
	Check how many buffered bytes are available..
	Args:		inArg1			read without locking
	Return:	number of bytes available to read
------------------------------------------------------------------------------- */

size_t
CDocker::bytesAvailable(bool inArg1)
{
	if (fB3)
		return 0;

	size_t count = 0;
	if (inArg1 || waitAndLockDocker())
	{
		newton_try
		{
			count = fPipe->bytesAvailable();
		}
		newton_catch_all
		{ }
		end_try;
	}
	if (!inArg1)
		unlockDocker();
	return count;
}


/* -------------------------------------------------------------------------------
	Read a chunk from the input stream.
	Args:		inBuf			buffer for data
				inLength		size of chunk
				inNoMore		true => no more to come
	Return:	boolean		true => EOF
------------------------------------------------------------------------------- */

bool
CDocker::readChunk(void * inBuf, size_t inLength, bool inNoMore)
{
	bool isEOF;
	fPipe->readChunk(inBuf, inLength, isEOF);
	if (inNoMore)
		flushPadding(inLength);
	return isEOF;
}


// Read bytes into a binary object for FConnReadBytes
NewtonErr
CDocker::readBytes(size_t& ioLength, RefVar inTarget)
{
	if (fB3)
		return noErr;

	fError = noErr;
	waitAndLockDocker();

	newton_try
	{
		XTRY
		{
			XFAILNOT(f30, fError = kDockErrProtocolError;)
			XFAILNOT(IsBinary(inTarget), fError = kNSErrNotABinaryObject;)
			size_t lenRemaining = fEvtLength - fLengthRead;
			if (lenRemaining < ioLength)
				ioLength = lenRemaining;
			XFAILIF(Length(inTarget) < ioLength, fError = kNSErrOutOfBounds;)
			fLengthRead += ioLength;
			CDataPtr data(inTarget);
			readChunk((Ptr)data, ioLength, fLengthRead == fEvtLength);
			if (fLengthRead == fEvtLength)
			{
				fLengthRead = 0;
				f30 = false;
			}
		}
		XENDTRY;
	}
	newton_catch_all
	{
		processException(CurrentException());
	}
	end_try;

	cleanUpIfStopping(false);
	cleanUpIfError(false);
	unlockDocker();
	return fError;
}


// Read command for FConnReadCommand
NewtonErr
CDocker::readCommand(RefVar& outCmd, bool inArg2, bool inArg3)
{
	if (fB3)
		return noErr;

	if (!fAC)
		return -112088;	// sic

	fError = noErr;
	waitAndLockDocker();
	fLengthRead = 0;
	f30 = inArg2;
	outCmd = NILREF;

	newton_try
	{
		outCmd = AllocateFrame();
		do {
			readDockerHeader(fEvtTag, fEvtLength);
		} while (inArg3 && fEvtTag == kDHello);

		UniChar tagStr[4];
		ConvertToUnicode(&fEvtTag, tagStr, 4);	// byte-swapping!
		SetFrameSlot(outCmd, SYMA(command), MakeString(tagStr));
		SetFrameSlot(outCmd, SYMA(length), MAKEINT(fEvtLength));
		RefVar data;
		if (!inArg2)
		{
			f30 = true;
			readData(data);
		}
		SetFrameSlot(outCmd, SYMA(data), data);
		f2F = true;
	}
	newton_catch_all
	{
		processException(CurrentException());
	}
	end_try;

	cleanUpIfStopping(false);
	cleanUpIfError(false);
	unlockDocker();
	return fError;
}


// Read command data for FConnReadCommandData
NewtonErr
CDocker::readCommandData(RefVar& outData)
{
	if (fB3)
		return 0;

	fError = noErr;
	waitAndLockDocker();

	readData(outData);

	cleanUpIfStopping(false);
	cleanUpIfError(false);
	unlockDocker();
	return fError;
}


NewtonErr
CDocker::readResult(void)
{
	int result;
	*fPipe >> result;
	return result;
}


/* -------------------------------------------------------------------------------
	Read a C string from the input stream.
	Args:		length of string
	Return:	pointer to string
				caller must FreePtr this when done
	Protocol data:
		EventType	------				| already read
		Size			length				|
		[possibly other data]
		char[]		the string
------------------------------------------------------------------------------- */

char *
CDocker::readString(size_t inLength)
{
	char * str = NULL;
	XTRY
	{
		str = NewPtr(inLength);
		XFAILIF(str == NULL, outOfMemory();)
		newton_try
		{
			readChunk(str, inLength, true);
		}
		cleanup
		{
			FreePtr(str);
		}
		end_try;
	}
	XENDTRY;
	return str;
}


NewtonErr
CDocker::readResultString(void)
{
	RefVar result(readRef(RA(NILREF)));
	SetFrameSlot(f20, SYMA(desktopResult), result);
	return kDockErrDesktopError;
}


void
CDocker::readData(RefVar& outData)
{
	if (fB3)
		return;

	fError = noErr;
	outData = NILREF;

	XTRY
	{
		XFAILNOT(f30, fError = kDockErrProtocolError;)

		newton_try
		{
			if (fEvtLength > 0)
			{
				if (fEvtLength == 4)
				{
					ULong data;
					*fPipe >> data;
					outData = MAKEINT(data);
				}
				else
				{
					outData = readRef(fTargetStore);
				}
			}
		}
		newton_catch_all
		{
			processException(CurrentException());
		}
		end_try;
	}
	XENDTRY;

	f30 = false;
}


Ref
CDocker::readRef(RefArg inStore)
{
	RefVar theRef;
	CObjectReader reader(*fPipe, inStore);
	newton_try
	{
		theRef = reader.read();
	}
	cleanup
	{
		reader.~CObjectReader();
	}
	end_try;
	if (fEvtLength != kIndeterminateLength)
		flushPadding(fEvtLength);
	return theRef;
}


/* -------------------------------------------------------------------------------
	Flush the command in progress. Read any remaining data.
	Args:		--
	Return:	--
	Protocol data:
		EventType	------				| already read
		Size			length				|
		char			(optional) data to be flushed
------------------------------------------------------------------------------- */

void
CDocker::flushCommand(void)
{
	XTRY
	{
		XFAIL(fEvtLength == 0)	// no need to flush anything

		char * buf = NewPtr(fEvtLength);
		XFAILIF(buf == NULL, outOfMemory();)
		unwind_protect
		{
			readChunk(buf, fEvtLength, true);
		}
		on_unwind
		{
			FreePtr(buf);
		}
		end_unwind;
	}
	XENDTRY;
}


NewtonErr
CDocker::flushCommandData(void)
{
	XTRY
	{
		XFAILNOT(f30, fError = kDockErrProtocolError;)

		waitAndLockDocker();

		newton_try
		{
			flushCommand();
		}
		newton_catch_all
		{
			processException(CurrentException());
		}
		end_try;
	}
	XENDTRY;

	cleanUpIfStopping(false);
	cleanUpIfError(false);
	unlockDocker();
	return fError;
}


/* -------------------------------------------------------------------------------
	Reading: flush pad bytes to align the stream.
	Args:		inLength		actual total length of streamed object
	Return:	--
------------------------------------------------------------------------------- */

void
CDocker::flushPadding(size_t inLength)
{
	size_t delta = TRUNC(inLength, kStreamAlignment);
	if (delta > 0)
	{
		ULong padding;
		size_t len = kStreamAlignment - delta;
		bool isEOF;
		fPipe->readChunk(&padding, len, isEOF);
	}
}


#pragma mark Protocol Exchange

/* -------------------------------------------------------------------------------
	Read the initiate docking command.
	Args:		--
	Return:	--
	Protocol data:
		EventType	------				| already read
		Size			length				|
		ULong			session type
------------------------------------------------------------------------------- */

void
CDocker::readInitiateDocking(void)
{
	if (fEvtLength != 4)
		ThrowErr(exLongError, kDockErrBadCommandLength);

	*fPipe >> fSessionType;
}


/* -------------------------------------------------------------------------------
	Read the desktop info command.
	Args:		--
	Return:	--
	Protocol data:
		EventType	------				| already read
		Size			length				|
		ULong			protocol version
		ULong			desktopType				// 0 = Mac, 1 = Windows
		ULong			encrypted key			// 2 longs
		ULong			session type
		ULong			allowSelectiveSync	// 0 = no, 1 = yes
		ULong			desktopApps				// ref
------------------------------------------------------------------------------- */

void
CDocker::readDesktopInfo(void)
{
	ULong parm;

	// read info about about desktop
	*fPipe >> fProtocolVersion;
	if (fProtocolVersion < kDanteProtocolVersion)
		ThrowErr(exLongError, kDockErrIncompatibleProtocol);

	*fPipe >> fDesktopType;		// Mac or Windows

	*fPipe >> fDesktopKey.hi;	// for handshake challenge
	*fPipe >> fDesktopKey.lo;

	*fPipe >> fSessionType;		// session type (again?)

	*fPipe >> parm;
	fIsSelectiveSync = (parm != 0);

	size_t lenRemaining = fEvtLength - 6*sizeof(ULong);
	if (lenRemaining >= 4)
		fDesktopApps = readRef(RA(NILREF));
	else
		fDesktopApps = NILREF;

	// respond with info about us -- actual version, and our key
	writeDockerHeader(kDNewtonInfo, false);
	*fPipe << (ULong)(3*sizeof(ULong));	// evt length

	ULong ourProtocolVersion = RINT(GetProtoVariable(f20, SYMA(protocolVersion)));
	if (fProtocolVersion > ourProtocolVersion)
		fProtocolVersion = ourProtocolVersion;
	*fPipe << fProtocolVersion;
	SetFrameSlot(f20, SYMA(protocolVersion), MAKEINT(fProtocolVersion));

	// randomise keys
/*	we don’t have the XxxRandSeed() functions yet
	ULong savedSeed = GetRandSeed();
	SetRandSeed(RealClock());
	fNewtonKey.hi = Random() + (Random() << 8);
	fNewtonKey.lo = Random() + (Random() << 8);
	SetRandSeed(savedSeed);
*/
	// and send them
	*fPipe << fNewtonKey.hi;
	*fPipe << fNewtonKey.lo;

	fPipe->flushWrite();
}

// then kDWhichIcons
// then kDSetTimeout

void
CDocker::readPassword(void)
{
	readDockerHeader(fEvtTag, fEvtLength);
	if (fEvtTag == kDPassword)
		verifyPassword();
	else if (fEvtTag == kDPWWrong)
		fError = kDockErrRetryPW;
	else if (fEvtTag == kDResult)
	{
		fError = readResult();
		if (fError)
			ThrowErr(exLongError, fError);
	}
	else
		ThrowErr(exLongError, kDockErrProtocolError);
}

/* -------------------------------------------------------------------------------
	Read the source version.
	Args:		--
	Return:	--
	Protocol data:
		EventType	------				| already read
		Size			length				|
		char			(optional) data to be flushed
------------------------------------------------------------------------------- */

void
CDocker::readSourceVersion(void)
{
	*fPipe >> f60;
	if (fEvtLength > 4)
	{
		*fPipe >> f64;
		*fPipe >> f68;
	}
	else
	{
		f64 = 0;
		f68 = 0;
	}
	writeResult(noErr);
}


#pragma mark -
#pragma mark Writing

/* -------------------------------------------------------------------------------
	For FConnWriteBytes:
	Write bytes from binary object to the stream.
	Args:		inTarget		binary data
	Return:	--
------------------------------------------------------------------------------- */

NewtonErr
CDocker::writeBytes(RefArg inTarget)
{
	if (fB3)
		return noErr;

	fError = noErr;
	waitAndLockDocker();

	newton_try
	{
		XTRY
		{
			XFAILNOT(f31, fError = kDockErrProtocolError;)
			XFAILNOT(IsBinary(inTarget), fError = kNSErrNotABinaryObject;)	// not in the original, but makes sense?
			size_t dataLength = Length(inTarget);
			size_t lenRemaining = fEvtLength - fLengthWritten;
			if (lenRemaining < dataLength)
				dataLength = lenRemaining;
			fLengthWritten += dataLength;
			CDataPtr data(inTarget);
			fPipe->writeChunk((Ptr)data, dataLength, false);
			if (fLengthWritten == fEvtLength)
			{
				f31 = false;
				fPipe->flushWrite();
			}
		}
		XENDTRY;
	}
	newton_catch_all
	{
		processException(CurrentException());
	}
	end_try;

	cleanUpIfStopping(false);
	cleanUpIfError(false);
	unlockDocker();
	return fError;
}


/* -------------------------------------------------------------------------------
	Pad output to stream alignment.
	Args:		inLength		actual total length of streamed object
	Return:	--
------------------------------------------------------------------------------- */

void
CDocker::pad(size_t inLength)
{
	size_t delta = TRUNC(inLength, kStreamAlignment);
	if (delta > 0)
	{
		ULong padding = 0;
		fPipe->writeChunk(&padding, kStreamAlignment-delta, false);
	}
}


void
CDocker::writeLong(EventType inTag, ULong inValue)
{
	writeDockerHeader(inTag, false);
	*fPipe << (ULong)sizeof(inValue);
	*fPipe << inValue;
	fPipe->flushWrite();
}


void
CDocker::writeResult(NewtonErr inResult)
{
	writeLong(kDResult, inResult);
}


void
CDocker::writeString(const UniChar * inString)
{
	size_t dataLen = (Ustrlen(inString) + 1) * sizeof(UniChar);
	*fPipe << dataLen;
	fPipe->writeChunk(inString, dataLen, false);
	pad(dataLen);
	fPipe->flushWrite();
}


void
CDocker::writeRef(EventType inTag, RefArg inValue)
{
	if (inTag == 0)
	{
		CObjectWriter writer(inValue, *fPipe, false);
		newton_try
		{
			writer.write();
		}
		cleanup
		{
			writer.~CObjectWriter();
		}
		end_try;
	}
	else
	{
		size_t nsofLength = 0;
		writeDockerHeader(inTag, false);
		CObjectWriter writer(inValue, *fPipe, false);
		newton_try
		{
			if (fVBOCompression == 2
			|| (f2E != 0 && f2E == 1))	// huh?
				writer.setCompressLargeBinaries();
			nsofLength = writer.size();
			*fPipe << nsofLength;
			writer.write();
		}
		cleanup
		{
			writer.~CObjectWriter();
		}
		end_try;
		pad(nsofLength);
	}
}


// For FConnWriteCommand
void
CDocker::writeCommand(RefArg, RefArg, long, bool, ULong&)
{}

#pragma mark Protocol Exchange

void
CDocker::writeNewtonName(void)
{}

void
CDocker::writePassword(RefArg)
{}

void
CDocker::retryPassword(RefArg)
{}


/* -------------------------------------------------------------------------------
	Stream out the inheritance frame.
	Args:		--
	Return:	--
	Protocol data:
		EventType	kDInheritance
		Size			length
		Size			number of slots in the inheritance frame
		C string		tag		| per slot
		C string		value		|
------------------------------------------------------------------------------- */
extern Ref	gInheritanceFrame;

void
CDocker::writeInheritanceFrame(void)
{
	ArrayIndex numOfSlots = 0;
	size_t streamLen = 0;
	CObjectIterator iter(gInheritanceFrame);
	for ( ; !iter.done(); iter.next())
	{
		numOfSlots++;
		streamLen += (strlen(SymbolName(iter.tag()))+1 + strlen(SymbolName(iter.value()))+1);
	}
	streamLen += sizeof(numOfSlots);

	writeDockerHeader(kDInheritance, false);
	*fPipe << streamLen;

	*fPipe << numOfSlots;
	CObjectIterator iter2(gInheritanceFrame);
	for ( ; !iter2.done(); iter2.next())
	{
		char * symName = SymbolName(iter2.tag());
		fPipe->writeChunk(symName, strlen(symName)+1, false);
		symName = SymbolName(iter2.value());
		fPipe->writeChunk(symName, strlen(symName)+1, false);
	}

	pad(streamLen);
	fPipe->flushWrite();
}

#pragma mark -

/* -------------------------------------------------------------------------------
	Respond to test message. Echo it.
	Args:		--
	Return:	--
	Protocol data:
		EventType	kDTest				| already read
		Size			length				|
		char			(optional) data to be echoed
------------------------------------------------------------------------------- */

void
CDocker::testMessage(void)
{
	if (fEvtLength == 0)
		writeDockerHeader(kDTest, true);

	else
	{
		XTRY
		{
			char * buf = NewPtr(fEvtLength);
			XFAILIF(buf == NULL, outOfMemory();)
			readChunk(buf, fEvtLength, true);

			writeDockerHeader(kDTest, false);
			*fPipe << fEvtLength;
			fPipe->writeChunk(buf, fEvtLength, false);
			pad(fEvtLength);
			fPipe->flushWrite();
			FreePtr(buf);
		}
		XENDTRY;
	}
}


/* -------------------------------------------------------------------------------
	Respond to test ref message. Echo it.
	Args:		--
	Return:	--
	Protocol data:
		EventType	kDRefTest			| already read
		Size			length				|
		Ref			(optional) value to be echoed
------------------------------------------------------------------------------- */

void
CDocker::testRefMessage(void)
{
	if (fEvtLength == 0)
		writeDockerHeader(kDRefTest, true);

	else
	{
		RefVar value(readRef(RA(NILREF)));
		writeDockerHeader(kDRefTest, false);
		writeRef(kDRefTest, value);
	}
}

