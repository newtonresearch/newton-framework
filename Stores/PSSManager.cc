/*
	File:		PSSmanager.cc

	Contains:	Newton object store interface.

	Written by:	Newton Research Group.
*/

#include "PSSManager.h"
#include "MemObjManager.h"
#include "MuxStore.h"
#include "FlashStore.h"
#include "Strings.h"


extern size_t		InternalStoreInfo(int inSelector);


/*------------------------------------------------------------------------------
	D a t a
------------------------------------------------------------------------------*/
SCardReinsertionTracker	gCardReinsertionTracker;			// 0C106820

CPSSManager * gPSSManager;
CUPort * gPSSPort;
bool gFormatCardsWhenInserted;

ULong gNumberOfHWSockets = 0;		// 0C100AB4


/*------------------------------------------------------------------------------
	F o r w a r d   D e c l a r a t i o n s
------------------------------------------------------------------------------*/

void		WipeInternalStore(ULong inName, CPersistentDBEntry * inEntry);


/*------------------------------------------------------------------------------
	S e t C a r d R e i n s e r t R e a s o n
------------------------------------------------------------------------------*/

#include "UStringUtils.h"
extern void		ParamString(UniChar * ioStr, size_t inStrLen, const UniChar * inFmt, ...);

UniChar * gCardReinsertReason = NULL;	// 0C100914

void
SetCardReinsertReason(const UniChar * inReason, bool inArg2)
{
	UniChar * reasonStr = NULL;
	if (gCardReinsertReason)
		FreePtr((Ptr)gCardReinsertReason);
	XTRY
	{
		if (inReason)
		{
			if (inArg2)
			{
			// 
				UniChar * alertText = (UniChar *)BinaryData(RA(STRuPackageNeedsCardAlertText));
				size_t reasonStrLen = Ustrlen(alertText) + Ustrlen(inReason);
				XFAIL(reasonStr = (UniChar *)NewPtr((reasonStrLen + 1)*sizeof(UniChar)))
				ParamString(reasonStr, reasonStrLen, alertText, inReason);
			}
			else
			{
				size_t reasonStrLen = Ustrlen(inReason);
				XFAIL(reasonStr = (UniChar *)NewPtr((reasonStrLen + 1)*sizeof(UniChar)))
				Ustrcpy(reasonStr, inReason);
			}
		}
	}
	XENDTRY;
	gCardReinsertReason = reasonStr;
}


/*------------------------------------------------------------------------------
	I n i t i a l i z a t i o n
------------------------------------------------------------------------------*/

NewtonErr
InitPSSManager(ObjectId inEnvId, ObjectId inDomId)
{
	NewtonErr err = noErr;

	// register store protocols
	CMuxStore::classInfo()->registerProtocol();
	CMuxStoreMonitor::classInfo()->registerProtocol();
	CFlashStore::classInfo()->registerProtocol();
	CNewInternalFlash::classInfo()->registerProtocol();

	XTRY
	{
		// create flash store instance and wrap it with mutex access
		CStore *	muxStore, * flashStore;
		XFAILNOT(muxStore = CStore::make("CMuxStore"), err = memFullErr;)
		XFAILNOT(flashStore = CStore::make("CFlashStore"), muxStore->destroy(); err = memFullErr;)
		muxStore->setStore(flashStore, inEnvId);

		// initialize store instance with available memory
		size_t storeSize = InternalStoreInfo(0);
		if (storeSize > 0)
		{
			CNewInternalFlash * newFlash = (CNewInternalFlash *)CFlash::make("CNewInternalFlash");
			newFlash->init(CHeapAllocator::getGlobalAllocator());
			flashStore->init(NULL, storeSize, inEnvId, 0, 0x10, newFlash);
		}
		else
		{
			CPersistentDBEntry	persistentEntry;
#if !defined(forFramework)
			MemObjManager::findEntryByName(kMemObjPersistent, 'rams', &persistentEntry);
			if ((persistentEntry.fFlags & 0x80) != 0)
				WipeInternalStore('rams', &persistentEntry);
			flashStore->init((void *)persistentEntry.fBase, persistentEntry.fSize, inEnvId, 0, 0x08, NULL);
#endif
		}

		// set globals
		gInRAMStore = flashStore;
		gMuxInRAMStore = muxStore;

		// format if needed
		bool doFormat;
		muxStore->needsFormat(&doFormat);
		if (doFormat)
			muxStore->format();

		// preflight -- get the root object into the cache
		PSSId	id;
		size_t size;
		muxStore->getRootId(&id);
		muxStore->getObjectSize(id, &size);

#if !defined(forFramework)
		// register with the NameServer so that Stores.cc : InitExternal() can find it later
		CUNameServer ns;
		ns.registerName("InRAMStore", (char *)kRegisteredStore, (OpaqueRef)muxStore, 0);

		// and finally... init the PSS manager
		CPSSManager * pssManager;
		XFAILNOT(pssManager = new CPSSManager, err =  kOSErrCouldNotCreateObject;)	// inline constructor
		err = pssManager->init('pssm', YES, kSpawnedTaskStackSize);
		gCardReinsertionTracker.f00 = NO;
#endif
	}
	XENDTRY;

	return err;
}


NewtonErr
InitializeCardStore(PSSStoreInfo * info, bool * outArg)
{
	NewtonErr err = noErr;

	XTRY
	{
#if defined(correct)
		if (info->f39)		// is initialized?
			*outArg = YES;
		else
		{
			info->f40->init(info->f1C, info->f14, info->f3C, info->f18, 1, info);
			info->f39 = YES;
			if (gFormatCardsWhenInserted)
			{
				gFormatCardsWhenInserted = NO;
				err = info->f10->format();
			}
			*outArg = !info->f38;
			info->f38 = NO;
			if (*outArg)
			{
				bool doIt;
				info->f10->needsFormat(&doIt);
				*outArg = !doIt;
				if (*outArg)
					info->f10->newObject(0, NULL);
			}
		}
#endif
	}
	XENDTRY;

	return err;
}


bool
IsValidStore(const CStore * inStore)
{
#if defined(correct)
	return inStore == gMuxInRAMStore
		 || gPSSManager->getStorePSSInfo(inStore, YES) != NULL;
#else
	return YES;
#endif
}


void
GetCardSlotStores(int inSlot, CStore ** outStores)
{
#if defined(correct)
	gPSSManager->getCardSlotStores(inArg1, outStores);
#endif
}


const PSSStoreInfo *
GetStorePSSInfo(const CStore * inStore)
{
#if defined(correct)
	return gPSSManager->getStorePSSInfo(inStore, NO);
#else
	static PSSStoreInfo info;
	info.f18 = 0;
	info.f30 = 'disk';
	return &info;
#endif
}



void
ReinsertCard(int inSlot, const UniChar * inReason, bool inArg3)
{
#if defined(correct)
	gPSSManager->reinsertCard(inArg1, inArg2, inArg3);
#endif
}


void
WipeInternalStore(ULong inName, CPersistentDBEntry * inEntry)
{
	if ((inEntry->fFlags & 0x80) != 0)
	{
		size_t size;
		memset((void *)inEntry->fBase, 0x3F, inEntry->fSize);
		if ((size = InternalStoreInfo(2)) != 0)
			memset((void *)InternalStoreInfo(3), 0x3F, size);
		inEntry->fFlags &= ~0x80;
#if defined(correct)
		MemObjManager::registerEntryByName(kMemObjPersistent, inName, inEntry);
#endif
	}
}


#pragma mark -
#if !defined(forFramework)
/*--------------------------------------------------------------------------------
	C P S S M a n a g e r
--------------------------------------------------------------------------------*/

CPSSManager::CPSSManager()
{ }


NewtonErr
CPSSManager::mainConstructor(void)
{
	NewtonErr err;
	XTRY
	{
		CAppWorld::mainConstructor();

		f70.init(YES);
		f80.clear();
		gPSSPort = f138 = getMyPort();
		f14C.init(kCardServerId);
		f160.init(kCardEventId, 0);
		f180.init(YES);
		f180.setCollectorPort(*gAppWorld->getMyPort());
		f180.setUserRefCon((OpaqueRef)&f14C);
		f304 = gNumberOfHWSockets;
		for (ArrayIndex i = 0; i < f304; ++i)
		{
			f308[i].clear();
			fAF8[i] = 0;
			fB08[i] = 0;
			f244[i].init(YES);
			f244[i].setCollectorPort(0);
			f284[i].fEventClass = kNewtEventClass;
			f284[i].fEventId = kIdleEventId;
			f284[i].f08 = kCardEventId;
			f284[i].f0C = i;
		}
		ObjectId domId;
		XFAIL(err = MemObjManager::findDomainId('ccl0', &domId))
		f178 = domId;
		gPSSManager = this;
	}
	XENDTRY;
	return err;
}


void
CPSSManager::mainDestructor(void)
{
	CAppWorld::mainDestructor();
}


void
CPSSManager::theMain(void)
{
	XTRY
	{
		CUNameServer ns;
		OpaqueRef newtPort, cardPort, spec;
		XFAIL(ns.waitForRegister("newt", (char *)kUPort, &newtPort, &spec))
		XFAIL(ns.waitForRegister("cdsv", (char *)kUPort, &cardPort, &spec))
		f13C = (ObjectId)newtPort;
		f144 = (ObjectId)cardPort;

		CAppWorld::theMain();
	}
	XENDTRY;
}


void
CPSSManager::UIEngine(bool inArg)
{
	NewtonErr err;
	XTRY
	{
		err = messageInUse();
		if (inArg)
			err = doReplyTransitions();
		XFAIL(err)
		XFAIL(deregisterStores())
		XFAIL(registerStores())
		gcStores();
	}
	XENDTRY;
}


bool
CPSSManager::messageInUse(void)
{
	for (ArrayIndex slotNo = 0; slotNo < f304; ++slotNo)
	{
		int r3 = f308[slotNo].f00;
		if (r3 == 2 || r3 == 3 || r3 == 6 || r3 == 7)
			return YES;
	}
	return NO;
}


int
CPSSManager::doReplyTransitions(void)
{
	for (ArrayIndex slotNo = 0; slotNo < f304; ++slotNo)
	{
		int r3 = f308[slotNo].f00;
		if (r3 == 2)
			f308[slotNo].f00 = 4;
		else if (r3 == 3)
			f308[slotNo].f00 = 5;
		else if (r3 == 6)
			f308[slotNo].f00 = 8;
		else if (r3 == 7)
			f308[slotNo].f00 = 1;
	}
	return 0;
}


void
CPSSManager::stuffSendAndTransition(int inSlot, int inArg2, int inArg3)
{
	f190.fEventClass = kNewtEventClass;
	f190.fEventId = kIdleEventId;
#if defined(correct)
	f190.f08 = inArg2;

	for (ArrayIndex i = 0; i < 4; ++i)
	{
		f190.f1C[i] = f308[i].f3C;			// it’s actually a little more complicated than this
	}

	f190.f0C = f308[inSlot].fBC.f10;		//f3D4	CStore *
	f190.f10 = &f308[inSlot].fBC.f00;	//f3C4	ObjectId?
#endif
	f308[inSlot].f00 = inArg3;

	f13C.sendRPC(&f180, &f190, 0xB4, &f190, 0xB4);
	// could return the result of that?
}


bool
CPSSManager::registerStores(void)
{
	for (ArrayIndex slotNo = 0; slotNo < f304; ++slotNo)
	{
		int r3 = f308[slotNo].f00;
		if (r3 == 2 || r3 == 3)
			return NO;
	}

	for (ArrayIndex slotNo = 0; slotNo < f304; ++slotNo)
	{
		int r3 = f308[slotNo].f00;
		if (r3 == 1)
		{
			stuffSendAndTransition(slotNo, 'stor', 2);
			return YES;
		}
	}
	return NO;
}


bool
CPSSManager::deregisterStores(void)
{
	for (ArrayIndex slotNo = 0; slotNo < f304; ++slotNo)
	{
		if (f308[slotNo].f00 == 5)
		{
			stuffSendAndTransition(slotNo, 'rstr', 6);
			return YES;
		}
	}
	return NO;
}


void
CPSSManager::gcStores(void)
{
	for (ArrayIndex slotNo = 0; slotNo < f304; ++slotNo)
	{
		if (f308[slotNo].f00 == 8)
		{
			if (fAF8[slotNo] == NULL)
				sendServer(0x34, slotNo, 0, 0, NULL);
			else
			{
//				fB08[slotNo]->f00.messageStuff(0x6F, slotNo, 0);
				fAF8[slotNo]->replyRPC(&fB08[slotNo]->f00, sizeof(CCardMessage), 0);
				delete fAF8[slotNo]; fAF8[slotNo] = NULL;
				delete fB08[slotNo]; fB08[slotNo] = NULL;
				for (ArrayIndex i = 0; i < 4; ++i)
				{
					CStore * store = f308[i].fBC[0].f10;
					if (store)
						store->destroy();
				}
				f308[slotNo].clear();
			}
		}
	}
}


void
CPSSManager::cardAvailable(CCardMessage * inMessage)
{}

void
CPSSManager::cardGone(CCardMessage * inMessage)
{}

void
CPSSManager::cardIsSame(CCardMessage * inMessage)
{}


void
CPSSManager::reinsertCard(int inSlot, const UniChar * inReason, bool inArg3)
{
	if (inSlot >= 0 && inSlot < f304)
	{
		char sp00;
		SetCardReinsertReason(inReason, inArg3);
		memmove(&sp00, f308[inSlot].fBC[0].f1C, 1);
		SetCardReinsertReason(NULL, NO);
	}
}


size_t
CPSSManager::getCardSlotStores(int inSlot, CStore ** outStores) const
{
	size_t numOfStores = 0;
	if (inSlot >= 0 && inSlot < f304)
	{
		const PSSStoreInfo * info = f308[inSlot].fBC;
		for (ArrayIndex i = 0; i < 4; ++i, ++info)
		{
			if (info->f10)
				outStores[numOfStores++] = info->f10;
		}
	}
	return numOfStores;
}


const PSSStoreInfo *
CPSSManager::getStorePSSInfo(const CStore * inStore, bool inArg2) const
{
	for (ArrayIndex slotNo = 0; slotNo < f304; ++slotNo)
	{
		int r3 = f308[slotNo].f00;
		if (inArg2 || r3 == 4 || r3 == 7 || r3 == 2)
		{
			for (ArrayIndex i = 0; i < 4; ++i)
			{
				const PSSStoreInfo * info = &f308[i].fBC[0];
				if (info->f10 == inStore)
					return info;
			}
		}
	}
	return NULL;
}


void
CPSSManager::sendServer(ULong inArg1, ULong inArg2, ULong inArg3, Timeout inTimeout, CTime * inTimeToSend)
{
	f80.messageStuff(inArg1, inArg2, inArg3);
	f144.send(&f70, &f80, sizeof(f80), inTimeout, inTimeToSend);
}


void
CPSSManager::replyServer(CCardMessage * inMessage, ULong inArg2, ULong inArg3, ULong inArg4)
{
	inMessage->messageStuff(inArg2, inArg3, inArg4);
}


void
CPSSManager::doCommand(CUMsgToken * inToken, size_t * inSize, CCardMessage * inMessage, bool)
{}


void
CPSSManager::doReply(CUMsgToken * inToken, size_t * inSize, CCardMessage * inMessage, bool)
{
	if (inMessage->f08 == 2)
		return;

	if (inMessage->f08 == 'rstr'		// storage card removed
	||  inMessage->f08 == 'stor')		// storage card inserted
		UIEngine(YES);
}

#pragma mark -

/*--------------------------------------------------------------------------------
	P S S S t o r e I n f o
--------------------------------------------------------------------------------*/

void
PSSStoreInfo::clear(void)
{
	memset(this, 0, sizeof(PSSStoreInfo));
}

#pragma mark -

/*--------------------------------------------------------------------------------
	P S S S l o t I n f o
--------------------------------------------------------------------------------*/

void
PSSSlotInfo::clear(void)
{
	f00 = 0;
	memset(&f04, 0, sizeof(CCardMessage));
	for (ArrayIndex i = 0; i < 4; ++i)
		fBC[i].clear();
}

#pragma mark -

/*--------------------------------------------------------------------------------
	C P S S E v e n t H a n d l e r
--------------------------------------------------------------------------------*/

void
CPSSEventHandler::eventHandlerProc(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent)
{
	gPSSWorld->doCommand(inToken, inSize, (CCardMessage *)inEvent, NO);
	gPSSWorld->eventSetReply(0xB8);	// coincidentally sizeof(CCardMessage)?
}

void
CPSSEventHandler::eventCompletionProc(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent)
{
	gPSSWorld->doReply(inToken, inSize, (CCardMessage *)inEvent, YES);
}

#pragma mark -

/*--------------------------------------------------------------------------------
	C P S S S y s t e m E v e n t H a n d l e r
--------------------------------------------------------------------------------*/

void
CPSSSystemEventHandler::newCard(CEvent * inEvent)
{
	gPSSWorld->cardAvailable((CCardMessage *)inEvent);
}

#endif
