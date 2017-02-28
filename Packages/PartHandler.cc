/*
	File:		PartHandler.cc

	Contains:	Part handler implementation.

	Written by:	Newton Research Group, 2010.
*/

#include "Objects.h"
#include "LargeObjects.h"
#include "StreamObjects.h"
#include "Arrays.h"
#include "ROMResources.h"
#include "Lookup.h"
#include "Funcs.h"
#include "NewtonScript.h"
#include "EventHandler.h"
#include "PartHandler.h"
#include "PackageEvents.h"
#include "MemoryPipe.h"
#include "EndpointPipe.h"
#include "PartPipe.h"
#include "AppWorld.h"
#include "NameServer.h"
#include "Protocols.h"

extern NewtonErr	InstallPart(RefArg inPartType, RefArg inPartFrame, const PartId & inPartId, SourceType inSource, PartInfo * info, RefArg ioContext);
extern NewtonErr	RemovePart(RefArg inPartType, const PartId & inPartId, RefArg ioContext);

extern NewtonErr	FramesException(Exception * inException);

extern void			InstallExportTables(RefArg inTable, void * inSource);
extern Ref			RemoveExportTables(void * inSource);
extern void			InstallImportTable(VAddr inArg1, RefArg inTable, void * inArg3, long inArg4);
extern void			RemoveImportTable(void * inSource);

extern void			VAddrToBase(VAddr * outBase, VAddr inAddr);
extern void			FlushPackageCache(VAddr inAddr);

extern Ref			gFunctionFrame;


/* -----------------------------------------------------------------------------
	C P i p e E v e n t
----------------------------------------------------------------------------- */

class CPipeEvent : public CEvent
{
public:
				CPipeEvent();
				CPipeEvent(ULong, long, long);

private:
	ULong		f08;
	long		f0C;
	long		f10;
// size+14
};


/* -----------------------------------------------------------------------------
	C P i p e E v e n t H a n d l e r
----------------------------------------------------------------------------- */

struct PipeInfo
{
	CEndpointPipe *	x00;
	long		x04;
	long		x08;
	long		x0C;
};


class CPipeEventHandler : public CEventHandler
{
public:
				CPipeEventHandler(PipeInfo*);

	// User supplied test of event - true if event is for this handler
	virtual bool		testEvent(CEvent * inEvent);

	// User supplied handler routines
	virtual void		eventHandlerProc(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent);

private:
	long			f18;
	PipeInfo *	f14;
	bool			f1C;
// size+20
};


/* -----------------------------------------------------------------------------
	C P i p e A p p
----------------------------------------------------------------------------- */

class CPipeApp : public CAppWorld
{
public:
				CPipeApp(const PipeInfo&, bool);

	virtual size_t		getSizeOf(void) const;

	virtual NewtonErr	mainConstructor(void);
	virtual void		mainDestructor(void);

private:
	CPipeEventHandler *	f70;
	PipeInfo		f74;
	bool			f80;		// overlap!!
	bool			f84;
// size+88
};



/*------------------------------------------------------------------------------
	D a t a
------------------------------------------------------------------------------*/


/*------------------------------------------------------------------------------
	U t i l i t i e s
------------------------------------------------------------------------------*/

extern bool	IsInRDMSpace(VAddr inAddr);


Ref
FramePartTopLevelFrame(void * inPart)
{
	if (!IsInRDMSpace((VAddr)inPart) || LargeObjectAddressIsValid((VAddr)inPart))
	{
	//	&& (inPart->x04 & ~1) == 0)
		return GetArraySlotRef(MAKEPTR(inPart), 0);
	}
	return NILREF;
}


#pragma mark -
/*------------------------------------------------------------------------------
	C P a r t P i p e
------------------------------------------------------------------------------*/

CPartPipe::CPartPipe()
	:	f0C(NULL), f04(NULL), f08(false)
{ }


CPartPipe::~CPartPipe()
{
	if (f0C)
		delete f0C;
	if (f08 && f04)
		delete f04;
}


void
CPartPipe::init(ObjectId inPortId, CShadowRingBuffer * inBuffer, bool inOwnBuffer)
{
	f0C = new CUPort(inPortId);
	f04 = inBuffer;
	f08 = inOwnBuffer;
}


void
CPartPipe::setStreamSize(size_t inSize)
{
	CPipeEvent event(0, inSize, 0);
	if (f0C)
		f0C->send(&event, sizeof(CPipeEvent));
}


long
CPartPipe::readSeek(long inOffset, int inSelector)
{
	return 0;
}


long
CPartPipe::readPosition(void) const
{
	return 0;
}


long
CPartPipe::writeSeek(long inOffset, int inSelector)
{
	return 0;
}


long
CPartPipe::writePosition(void) const
{
	return 0;
}


void
CPartPipe::readChunk(void * outBuf, size_t & ioSize, bool & outEOF)
{
	if (ioSize > 0)
	{
		size_t remainingSize = ioSize;
		NewtonErr err = f04->copyOut((UChar *)outBuf, remainingSize);
		if (err == -1)		// isEmpty
			err = noErr;	// is not really an error
		if (err)
			ThrowErr(exPipe, noErr);

		while (remainingSize > 0 && err == noErr)
		{
			f10 = remainingSize;
			underflow(remainingSize, outEOF);
			err = f04->copyOut((UChar *)outBuf + ioSize - remainingSize, remainingSize);
			if (err == -1)
				err = noErr;
			if (err)
				ThrowErr(exPipe, noErr);
		}
	}
}


void
CPartPipe::writeChunk(const void * inBuf, size_t inSize, bool inFlush)
{ }


void
CPartPipe::flushRead(void)
{ }


void
CPartPipe::flushWrite(void)
{ }


void
CPartPipe::reset(void)
{
	f04->reset();
}


void
CPartPipe::overflow()
{ }


void
CPartPipe::underflow(long inArg1, bool& inArg2)
{
	size_t replySize;
	CPipeEvent event(1, inArg1, 0);
	NewtonErr err = f0C->sendRPC(&replySize, &event, sizeof(CPipeEvent), &event, sizeof(CPipeEvent));
	if (err)
		ThrowErr(exPipe, err);
/*
	if (err = sp00.f10)		// huh?...
		ThrowErr(exPipe, err);
	if (inArg1 != 0 && sp04.f0C == 0)
		ThrowErr(exPipe, kOSErrSorrySystemError);
	f04->updatePutVector(sp04.f0C);
*/
}


void
CPartPipe::seekEOF(void)
{
	CPipeEvent event(2, 0, 0);
	if (f0C)
		f0C->send(&event, sizeof(CPipeEvent));
}


void
CPartPipe::close(void)
{
	size_t replySize;
	CPipeEvent event(3, 0, 0);
	if (f0C)
		f0C->sendRPC(&replySize, &event, sizeof(CPipeEvent), &event, sizeof(CPipeEvent));
}


#pragma mark -
/*------------------------------------------------------------------------------
	C P i p e A p p
------------------------------------------------------------------------------*/

CPipeApp::CPipeApp(const PipeInfo& info, bool inArg2)
{
	f74 = info;
	f80 = inArg2;
	f84 = inArg2;
}


size_t
CPipeApp::getSizeOf(void) const
{
	return sizeof(CPipeApp);
}


NewtonErr
CPipeApp::mainConstructor(void)
{
	NewtonErr err;
	XTRY
	{
		XFAIL(err = CAppWorld::mainConstructor())
		f70 = new CPipeEventHandler(&f74);
		XFAILIF(f70 == NULL, err = kOSErrNoMemory;)
		f70->init(kPackageEventId);
	}
	XENDTRY;

	if (f84)
	{
		newton_try
		{
//			f74.x00->addToAppWorld();
		}
		newton_catch(exPipe)
		{
			err = (NewtonErr)(long)CurrentException()->data;
		}
		end_try;
	}
	return err;
}


void
CPipeApp::mainDestructor(void)
{
	if (f70)
		delete f70;
	CAppWorld::mainDestructor();
}


#pragma mark -
/*------------------------------------------------------------------------------
	C P i p e E v e n t
------------------------------------------------------------------------------*/

CPipeEvent::CPipeEvent()
	:	CEvent(kPackageEventId)
{
	f08 = 0;
	f0C = 0;
	f10 = 0;
}


CPipeEvent::CPipeEvent(ULong inArg1, long inArg2, long inArg3)
	:	CEvent(kPackageEventId)
{
	f08 = inArg1;
	f0C = inArg2;
	f10 = inArg3;
}


#pragma mark -
/*------------------------------------------------------------------------------
	C P i p e E v e n t H a n d l e r
------------------------------------------------------------------------------*/

CPipeEventHandler::CPipeEventHandler(PipeInfo * info)
{
	f18 = 0x7FFFFFFF;
	f14 = info;
	f1C = false;
}


bool
CPipeEventHandler::testEvent(CEvent * inEvent)
{
	return true;
}


void
CPipeEventHandler::eventHandlerProc(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent)
{
	// INCOMPLETE
}


#pragma mark -
/*------------------------------------------------------------------------------
	C P k B a s e E v e n t
------------------------------------------------------------------------------*/

CPkBaseEvent::CPkBaseEvent()
{
	fEventId = kPackageEventId;
}


/*------------------------------------------------------------------------------
	C P k R e g i s t e r E v e n t
------------------------------------------------------------------------------*/

CPkRegisterEvent::CPkRegisterEvent(ULong inPartType, ObjectId inPortId)
{
	fEventCode = kRegisterPartHandlerEventId;
	fEventErr = noErr;
	fPartType = inPartType;
	fPortId = inPortId;
}


/*------------------------------------------------------------------------------
	C P k U n r e g i s t e r E v e n t
------------------------------------------------------------------------------*/

CPkUnregisterEvent::CPkUnregisterEvent(ULong inPartType)
{
	fEventCode = kUnregisterPartHandlerEventId;
	fEventErr = noErr;
	fPartType = inPartType;
}


/*------------------------------------------------------------------------------
	C P k B e g i n L o a d E v e n t
------------------------------------------------------------------------------*/

CPkBeginLoadEvent::CPkBeginLoadEvent(SourceType inType, const PartSource & inSource, ObjectId inArg3, ObjectId inArg4, bool inArg5)
{
	fEventCode = kBeginLoadPkgEventId;
	fEventErr = noErr;
	fUniqueId = 0;
	f14 = inArg4;
	f10 = inArg3;
	fSrcType = inType;
	fSrc = inSource;
	f28 = inArg5;
}


/*------------------------------------------------------------------------------
	C P k R e m o v e E v e n t
------------------------------------------------------------------------------*/

CPkRemoveEvent::CPkRemoveEvent(ObjectId inPkgId, ULong inArg2, ULong inArg3)
{
	fEventCode = kRemovePkgEventId;
	fEventErr = noErr;
	fPkgId = inPkgId;
	f14 = inArg2;
	f18 = inArg3;
}


/*------------------------------------------------------------------------------
	C P k B a c k u p E v e n t
------------------------------------------------------------------------------*/

CPkBackupEvent::CPkBackupEvent(ArrayIndex inSeq, ULong inArg2, bool inArg3, const PartSource & inArg4, ULong inArg5, ULong inArg6)
{
	fEventCode = kBackupPkgEventId;
	fEventErr = noErr;
	fIndex = inSeq;
	f14 = inArg2;
	f18 = inArg4;
	f20 = inArg5;
	f24 = inArg6;
	f84 = inArg3;
}


/*------------------------------------------------------------------------------
	C P k P a r t I n s t a l l E v e n t
------------------------------------------------------------------------------*/

CPkPartInstallEvent::CPkPartInstallEvent(const PartId & inPartId, const ExtendedPartInfo & info, SourceType inType, const PartSource & inSource)
{
	fEventCode = kInstallPartEventId;

	fPartId = inPartId;
	fPartInfo = info;
	fType = inType;
	fSource = inSource;
	memmove(fInfo, info.info, info.infoSize);
	strncpy(fCompressorName, info.compressor, kMaxCompressorNameSize);
}


/*------------------------------------------------------------------------------
	C P k P a r t I n s t a l l E v e n t R e p l y
------------------------------------------------------------------------------*/

CPkPartInstallEventReply::CPkPartInstallEventReply()
{ }


/*------------------------------------------------------------------------------
	C P k P a r t R e m o v e E v e n t
------------------------------------------------------------------------------*/

CPkPartRemoveEvent::CPkPartRemoveEvent(PartId inPartId, long inArg2, ULong inArg3, long inArg4)
{
	fEventCode = kRemovePartEventId;
	fPartId = inPartId;
	f18 = inArg2;
	f1C = inArg3;
	f20 = inArg4;
}


/*------------------------------------------------------------------------------
	C P k S a f e T o D e a c t i v a t e E v e n t
------------------------------------------------------------------------------*/

CPkSafeToDeactivateEvent::CPkSafeToDeactivateEvent(ObjectId inArg1)
{
	fEventCode = kSafeToDeactivatePkgEventId;
	fEventErr = noErr;
	fPkgId = inArg1;
	fIsSafeToDeactivate = false;
}


#pragma mark -
/*------------------------------------------------------------------------------
	C P a r t E v e n t H a n d l e r
------------------------------------------------------------------------------*/

CPartEventHandler::CPartEventHandler(CPartHandler * inPartHandler)
{
	fPartHandler = inPartHandler;
}

bool
CPartEventHandler::testEvent(CEvent * inEvent)
{
	ULong evtCode = ((CPkBaseEvent *)inEvent)->fEventCode;

	if (evtCode == kBeginLoadPkgEventId
	||  evtCode == kBackupPkgEventId)
		return false;

	if (evtCode == kInstallPartEventId)
		return ((CPkPartInstallEvent *)inEvent)->fPartInfo.type == fPartHandler->fType;

	if (evtCode == kRemovePartEventId)
		return ((CPkPartRemoveEvent *)inEvent)->f1C == fPartHandler->fType;
}

void
CPartEventHandler::eventHandlerProc(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent)
{
	if (*inSize >= sizeof(CPkBaseEvent)) {
		ULong evtCode = ((CPkBaseEvent *)inEvent)->fEventCode;
		if (evtCode == kInstallPartEventId) {
			CPkPartInstallEvent * evt = (CPkPartInstallEvent *)inEvent;
			evt->fPartInfo.info = evt->fInfo;
			evt->fPartInfo.compressor = evt->fPartInfo.compressed ? evt->fCompressorName : "";
			fPartHandler->install((CPkPartInstallEvent *)inEvent);
		} else if (evtCode == kRemovePartEventId) {
			fPartHandler->remove((CPkPartRemoveEvent *)inEvent);
		}
	}
}

void
CPartEventHandler::eventCompletionProc(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent)
{ }


#pragma mark -
/*------------------------------------------------------------------------------
	C P a r t H a n d l e r
------------------------------------------------------------------------------*/

CPartHandler::CPartHandler()
{
	fEventHandler = NULL;
	fType = 'none';
	fInitFailed = true;
	fAsyncMessage = NULL;
	fRegisterEvent = NULL;
}


CPartHandler::~CPartHandler()
{
	unregisterPart();
	if (fEventHandler)
		delete fEventHandler;
	if (fAsyncMessage)
		delete fAsyncMessage;
}


NewtonErr
CPartHandler::init(ULong inType)
{
	NewtonErr err = noErr;
	fType = inType;

	XTRY
	{
		fEventHandler = new CPartEventHandler(this);
		XFAILIF(fEventHandler == NULL, err = MemError();)
		fAsyncMessage = new CUAsyncMessage;
		XFAILIF(fAsyncMessage == NULL, err = MemError();)
		fRegisterEvent = new CPkRegisterEvent(fType, *gAppWorld->getMyPort());
		XFAILIF(fRegisterEvent == NULL, err = MemError();)
		XFAIL(err = fAsyncMessage->init(true))
		XFAIL(err = fEventHandler->init(kPackageEventId))
		err = registerPart();
	}
	XENDTRY;

	if (err == noErr)
		fInitFailed = false;

	return err;
}


// backup methods
NewtonErr
CPartHandler::getBackupInfo(const PartId& inPartId, PartType inPartType, RemoveObjPtr inRemovePtr, PartInfo * outPartInfo, ULong inLastBackupDate, bool * outNeedsBackup)
{
	*outNeedsBackup = false;
	outPartInfo->sizeInMemory = 0;
	outPartInfo->size = 0;
	outPartInfo->infoSize = 0;
	outPartInfo->compressed = false;
	return noErr;
}


NewtonErr
CPartHandler::backup(const PartId& inPartId, RemoveObjPtr inRemovePtr, CPipe * inPipe)
{
	return noErr;
}


NewtonErr
CPartHandler::expand(void * outData, CPipe * inPipe, PartInfo * info)
{
	NewtonErr err;
	newton_try
	{
		bool isEOF = false;
		size_t size = info->size;
		inPipe->readChunk(outData, size, isEOF);
	}
	newton_catch(exPipe)
	{
		err = (NewtonErr)(long)CurrentException()->data;
	}
	end_try;
	return err;
}


// return information functions
void
CPartHandler::setRemoveObjPtr(RemoveObjPtr inObj)
{
	fRemoveObjPtr = inObj;
}


void
CPartHandler::rejectPart(void)
{
	fAccept = false;
}


void
CPartHandler::replyImmed(NewtonErr inErr)
{
	CPkPartInstallEventReply reply;
	fReplied = true;
	reply.fEventErr = inErr;
	reply.f14 = fAccept;
	reply.f10 = fRemoveObjPtr;
	fEventHandler->setReply(sizeof(CPkPartInstallEventReply), &reply);
	fEventHandler->replyImmed();
}


// query functions
Ptr
CPartHandler::getSourcePtr(void)
{
	return IsMemory(fSourceType) ? fPartInfo->data.address : NULL;
}


// load part functions
NewtonErr
CPartHandler::copy(void * outData)
{
	NewtonErr err = noErr;

	if (IsStream(fSourceType))
	{
		CShadowRingBuffer * buf = new CShadowRingBuffer;
		CPartPipe * pipe = new CPartPipe;
		buf->init(fSource.stream.bufferId, 0, 0);
		pipe->init(fSource.stream.messagePortId, buf, false);
		err = expand(outData, pipe, fPartInfo);
		delete pipe;
		delete buf;
	}

	else
	{
		newton_try
		{
			memmove(outData, (const void *)fSource.mem.buffer, fPartInfo->size);
		}
		newton_catch_all	// original says newton_catch(0)
		{
			err = kOSErrUnexpectedEndOfPkgPart;
		}
		end_try;
	}
	
	return err;
}


void
CPartHandler::install(CPkPartInstallEvent * installEvent)
{
	NewtonErr err;

	fAccept = true;
	fSourceType = installEvent->fType;
	fSource = installEvent->fSource;
//	fSource.mem.buffer = (VAddr) &installEvent->fPartInfo;	huh?
	fReplied = false;
	newton_try
	{
		err = install(installEvent->fPartId, fSourceType, &installEvent->fPartInfo);
	}
	newton_catch_all	// original says newton_catch(0)
	{
		err = (NewtonErr)(long)CurrentException()->data;
	}
	end_try;
	if (!fReplied)
		replyImmed(err);
}


void
CPartHandler::remove(CPkPartRemoveEvent * inRemoveEvent)
{
	remove(inRemoveEvent->fPartId, inRemoveEvent->f1C, inRemoveEvent->f20);
}


NewtonErr
CPartHandler::registerPart(void)
{
	NewtonErr err;
	CUPort pkgMgr(PackageManagerPortId());
	err = pkgMgr.sendRPC(fAsyncMessage, fRegisterEvent, sizeof(CPkRegisterEvent), fRegisterEvent, sizeof(CPkRegisterEvent));
	Sleep(10*kMilliseconds);
	return err;
}


NewtonErr
CPartHandler::unregisterPart(void)
{
	NewtonErr err;
	CPkUnregisterEvent evt(fType);
	CUPort pkgMgr(PackageManagerPortId());
	size_t replySize;

	XTRY
	{
		XFAILNOT(fAsyncMessage, err = kOSErrNoMemory;)
		XFAIL(err = fAsyncMessage->blockTillDone(NULL, NULL, NULL, NULL))
		XFAIL(err = fRegisterEvent->fEventErr)
		err = pkgMgr.sendRPC(&replySize, &evt, sizeof(CPkUnregisterEvent), &evt, sizeof(CPkUnregisterEvent));
		if (err == noErr)
			err = evt.fEventErr;
	}
	XENDTRY;

	return err;
}


#pragma mark -
/*------------------------------------------------------------------------------
	C F r a m e P a r t H a n d l e r
------------------------------------------------------------------------------*/

NewtonErr
CFramePartHandler::install(const PartId & inPartId, SourceType inSource, PartInfo * info)
{
//	r4: r7 r6|r5 r10
	NewtonErr err = noErr;
	RefVar theFrame;
	Ptr src = NULL;

	XTRY
	{
		if (IsStream(inSource))
		{
			Ref data;
			err = copy(&data);
			theFrame = data;
		}
		else
		{
			src = getSourcePtr();
			if (info->compressed && strcmp(info->compressor, "streamed") == 0)
			{
				CBufferSegment buf;
				buf.init(src, info->size);

				CMemoryPipe pipe;
				pipe.init(&buf, NULL, false);

				Ref data;
				err = expand(&data, &pipe, info);
				theFrame = data;
			}
			else
			{
				theFrame = FramePartTopLevelFrame(src);
				XFAILIF(ISNIL(theFrame), err = kOSErrBadPackage;)
				RefVar table(GetFrameSlot(theFrame, SYMA(_exportTable)));
				if (NOTNIL(table))
					InstallExportTables(table, src);
				if (src >= (Ptr)0x03800000)
				{
					table = GetFrameSlot(theFrame, SYMA(_importTable));
					if (NOTNIL(table))
					{
						VAddr addr;
						if (IsInRDMSpace((VAddr)src))
							VAddrToBase(&addr, (VAddr)src);
						else
							addr = 0;
						InstallImportTable(addr, table, src, info->sizeInMemory);
						FlushPackageCache(addr);
					}
				}
			}
		}
		XFAILIF(ISNIL(theFrame) || !IsFrame(theFrame), err = kOSErrBadPackage;)

		fContext = new RemoveObj;
		XFAILIF(fContext == NULL, err = MemError();)
		fContext->src = src;
		XFAIL(err = installFrame(theFrame, inPartId, inSource, info))
		setRemoveObjPtr((RemoveObjPtr)fContext);
	}
	XENDTRY;

	return err;
}


NewtonErr
CFramePartHandler::remove(const PartId & inPartId, PartType inType, RemoveObjPtr inContext)
{
	NewtonErr err;
	RefVar context;
	RefVar removedExports;
	RemoveObj * contextPtr = (RemoveObj *)inContext;
	if (contextPtr->src)
	{
		removedExports = RemoveExportTables(contextPtr->src);
		RemoveImportTable(contextPtr->src);
	}
	context = contextPtr->obj;
	delete contextPtr;
	err = removeFrame(context, inPartId, inType);

	GC();
	ICacheClear();

	if (Length(removedExports) > 0)
	{
		newton_try
		{
			if (FrameHasSlot(gFunctionFrame, SYMA(ReportDeadUnitImports)))
				NSCallGlobalFn(SYMA(ReportDeadUnitImports), removedExports);
		}
		newton_catch_all
		{ }
		end_try;
	}

	return err;
}


NewtonErr
CFramePartHandler::expand(void * outData, CPipe * inPipe, PartInfo * info)
{
	NewtonErr err = noErr;
	CObjectReader reader(*inPipe);
	newton_try
	{
		*(Ref *)outData = reader.read();
	}
	newton_catch(exPipe)
	{
		err = (NewtonErr)(long)CurrentException()->data;
	}
	end_try;
	return err;
}


NewtonErr
CFramePartHandler::setFrameRemoveObject(RefArg inObject)
{
	fContext->obj = inObject;
	return noErr;
}


#pragma mark -
/*------------------------------------------------------------------------------
	C F o r m P a r t H a n d l e r
------------------------------------------------------------------------------*/

NewtonErr
CFormPartHandler::installFrame(RefArg inPartFrame, const PartId & inPartId, SourceType inSource, PartInfo * info)
{
	RefArg context(Clone(RA(canonicalFramePartSavedObject)));
	setFrameRemoveObject(context);
	return InstallPart(SYMA(form), inPartFrame, inPartId, inSource, info, context);
}


NewtonErr
CFormPartHandler::removeFrame(RefArg inContext, const PartId & inPartId, PartType inSource)
{
	return RemovePart(SYMA(form), inPartId, inContext);
}


NewtonErr
CFormPartHandler::getBackupInfo(const PartId & inPartId, ULong, long, PartInfo *, ULong, bool *)
{
	return noErr;
}


NewtonErr
CFormPartHandler::backup(const PartId & inPartId, RemoveObjPtr inRemovePtr, CPipe * inPipe)
{
	return noErr;
}


#pragma mark -
/*------------------------------------------------------------------------------
	C B o o k P a r t H a n d l e r
	Implementation probably belongs with the Librarian.
------------------------------------------------------------------------------*/

NewtonErr
CBookPartHandler::install(const PartId & inPartId, SourceType inSource, PartInfo * info)
{ return noErr; }


NewtonErr
CBookPartHandler::remove(const PartId & inPartId, PartType inSource, RemoveObjPtr inContext)
{ return noErr; }


NewtonErr
CBookPartHandler::expand(void * outData, CPipe * inPipe, PartInfo * info)
{ return noErr; }


#pragma mark -
/*------------------------------------------------------------------------------
	C D i c t P a r t H a n d l e r
------------------------------------------------------------------------------*/

NewtonErr
CDictPartHandler::install(const PartId & inPartId, SourceType inSource, PartInfo * info)
{ return noErr; }


NewtonErr
CDictPartHandler::remove(const PartId & inPartId, PartType inSource, RemoveObjPtr inContext)
{ return noErr; }


NewtonErr
CDictPartHandler::expand(void * outData, CPipe * inPipe, PartInfo * info)
{ return noErr; }


NewtonErr
CDictPartHandler::addDictionaries(RefArg, RefArg)
{ return noErr; }


#pragma mark -
/*------------------------------------------------------------------------------
	C A u t o S c r i p t P a r t H a n d l e r
------------------------------------------------------------------------------*/

NewtonErr
CAutoScriptPartHandler::installFrame(RefArg inPartFrame, const PartId & inPartId, SourceType inSource, PartInfo * info)
{
	RefArg context(Clone(RA(canonicalFramePartSavedObject)));
	setFrameRemoveObject(context);
	return InstallPart(SYMA(auto), inPartFrame, inPartId, inSource, info, context);
}


NewtonErr
CAutoScriptPartHandler::removeFrame(RefArg inContext, const PartId & inPartId, PartType inSource)
{
	return RemovePart(SYMA(auto), inPartId, inContext);
}


#pragma mark -
/*------------------------------------------------------------------------------
	C C o m m P a r t H a n d l e r
------------------------------------------------------------------------------*/

NewtonErr
CCommPartHandler::installFrame(RefArg inPartFrame, const PartId & inPartId, SourceType inSource, PartInfo * info)
{
	NewtonErr err = noErr;
	RefVar commConfigs(GetFrameSlot(inPartFrame, SYMA(configurations)));
	if (NOTNIL(commConfigs))
	{
		RefVar args(MakeArray(1));
		SetArraySlot(args, 0, commConfigs);
		newton_try
		{
			DoBlock(GetFrameSlot(gFunctionFrame, SYMA(RegCommConfigArray)), args);
		}
		newton_catch(exFrames)
		{
			err = FramesException(CurrentException());
		}
		end_try;
	}
	CAutoScriptPartHandler::installFrame(inPartFrame, inPartId, inSource, info);
	return err;
}


NewtonErr
CCommPartHandler::removeFrame(RefArg inContext, const PartId & inPartId, PartType inSource)
{
	NewtonErr err = noErr;
	RefVar commConfigs(GetFrameSlot(inContext, SYMA(configurations)));
	if (NOTNIL(commConfigs))
	{
		RefVar args(MakeArray(1));
		SetArraySlot(args, 0, commConfigs);
		newton_try
		{
			DoBlock(GetFrameSlot(gFunctionFrame, SYMA(UnRegCommConfigArray)), args);
		}
		newton_catch(exFrames)
		{
			err = FramesException(CurrentException());
		}
		end_try;
	}
	CAutoScriptPartHandler::removeFrame(inContext, inPartId, inSource);
	return err;
}


#pragma mark -
/*------------------------------------------------------------------------------
	C F o n t P a r t H a n d l e r
	Originally called just CFontPart.
------------------------------------------------------------------------------*/

