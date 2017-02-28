/*
	File:		PackageEvents.h

	Contains:	Package loading event declarations.

	Written by:	Newton Research Group, 2010.
*/

#if !defined(__PACKAGEEVENTS_H)
#define __PACKAGEEVENTS_H 1

#include "EventHandler.h"
#include "Objects.h"
#include "PackageTypes.h"
#include "PartHandler.h"
#include "ArrayIterator.h"
#include "PartPipe.h"
#include "UserPorts.h"

/* -----------------------------------------------------------------------------
	E v e n t   I d s
----------------------------------------------------------------------------- */

#define kRegisterPartHandlerEventId		'rgtr'
#define kUnregisterPartHandlerEventId	'urgr'
#define kBeginLoadPkgEventId				'pkbl'
#define kBackupPkgEventId					'pkbu'
#define kSafeToDeactivatePkgEventId		'pksc'
#define kRemovePkgEventId					'pkrm'

#define kInstallPartEventId				'prti'
#define kRemovePartEventId					'prtr'


/*------------------------------------------------------------------------------
	C P k B a s e E v e n t
------------------------------------------------------------------------------*/
class CPkBaseEvent : public CEvent
{
public:
			CPkBaseEvent();

	ULong			fEventCode;
	NewtonErr	fEventErr;
// size +10
};


/*------------------------------------------------------------------------------
	C P k R e g i s t e r E v e n t
------------------------------------------------------------------------------*/
class CPkRegisterEvent : public CPkBaseEvent
{
public:
			CPkRegisterEvent(ULong inPartType, ObjectId inPortId);

	ULong		fPartType;
	ObjectId	fPortId;		//+14
};


/*------------------------------------------------------------------------------
	C P k U n r e g i s t e r E v e n t
------------------------------------------------------------------------------*/
class CPkUnregisterEvent : public CPkBaseEvent
{
public:
			CPkUnregisterEvent(ULong inPartType);

	ULong		fPartType;
};


/*------------------------------------------------------------------------------
	C P k B e g i n L o a d E v e n t
------------------------------------------------------------------------------*/
class CPkBeginLoadEvent : public CPkBaseEvent
{
public:
			CPkBeginLoadEvent(SourceType inType, const PartSource & inSource, ObjectId inArg3, ObjectId inArg4, bool inArg5);

	ObjectId		f10;				// they’re port ids
	ObjectId		f14;
	SourceType	fSrcType;		//+18
	PartSource	fSrc;				//+20
	bool			f28;
	ULong			fUniqueId;		//+2C
	size_t		fPkgSize;		//+30
	ArrayIndex	fNumOfParts;	//+34
	ULong			fVersion;		//+38
	size_t		fSizeInMem;		//+3C
	UniChar		fName[32];		//+40
	bool			f80;
	bool			f81;
// size +84
};


/*------------------------------------------------------------------------------
	C P k R e m o v e E v e n t
------------------------------------------------------------------------------*/
class CPkRemoveEvent : public CPkBaseEvent
{
public:
			CPkRemoveEvent(ObjectId inPkgId, ULong inArg2, ULong inArg3);

	ObjectId fPkgId;
	ULong		f14;
	ULong		f18;
};


/*------------------------------------------------------------------------------
	C P k B a c k u p E v e n t
------------------------------------------------------------------------------*/
class CPkBackupEvent : public CPkBaseEvent
{
public:
			CPkBackupEvent(ArrayIndex inArg1, ULong inArg2, bool inArg3, const PartSource & inArg4, ObjectId inArg5, ObjectId inArg6);

	ArrayIndex	fIndex;			//+10
	ULong			f14;
	PartSource	f18;
	ULong			f20;
	ULong			f24;
	size_t		fSize;			//+28
	ULong			fId;				//+2C
	ULong			fVersion;		//+30
	SourceType	fSrcType;		//+34
	ULong			fFlags;			//+3C
	ULong			fDate;			//+40
	UniChar		fName[kMaxPackageNameSize+1];	//+44
	bool			f84;
// size +88
};


/*------------------------------------------------------------------------------
	C P k P a r t I n s t a l l E v e n t
------------------------------------------------------------------------------*/
class CPkPartInstallEvent : public CPkBaseEvent
{
public:
			CPkPartInstallEvent(const PartId & inPartId, const ExtendedPartInfo & info, SourceType inType, const PartSource & inSource);

	PartId			fPartId;			// +10
	ExtendedPartInfo	fPartInfo;	// +18
	SourceType		fType;			// +84
	PartSource		fSource;			// +8C
	char				fInfo[kMaxInfoSize+1];	// +94
	char				fCompressorName[kMaxCompressorNameSize+1];	// +D4
// size +F4
};


/*------------------------------------------------------------------------------
	C P k P a r t I n s t a l l E v e n t R e p l y
------------------------------------------------------------------------------*/
class CPkPartInstallEventReply : public CPkBaseEvent
{
public:
		CPkPartInstallEventReply();

	RemoveObjPtr	f10;		// +10
	bool				f14;		// +14
	
};


/*------------------------------------------------------------------------------
	C P k P a r t R e m o v e E v e n t
------------------------------------------------------------------------------*/
class CPkPartRemoveEvent : public CPkBaseEvent
{
public:
		CPkPartRemoveEvent(PartId inPartId, long inArg2, ULong inArg3, long inArg4);

	PartId			fPartId;			// +10
	long		f18;
	ULong		f1C;
	long		f20;
};


/*------------------------------------------------------------------------------
	C P k S a f e T o D e a c t i v a t e E v e n t
------------------------------------------------------------------------------*/
class CPkSafeToDeactivateEvent : public CPkBaseEvent
{
public:
		CPkSafeToDeactivateEvent(ObjectId inPkgId);

	ObjectId	fPkgId;
	bool		fIsSafeToDeactivate;
};


/*------------------------------------------------------------------------------
	C P a r t E v e n t H a n d l e r
------------------------------------------------------------------------------*/
class CPartEventHandler : public CEventHandler
{
public:
				CPartEventHandler(CPartHandler * inPartHandler);

	virtual	bool	testEvent(CEvent * inEvent);
	virtual	void	eventHandlerProc(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent);
	virtual	void	eventCompletionProc(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent);

private:
	CPartHandler *	fPartHandler;
};


class CPackageLoaderEventHandler : public CEventHandler
{
public:
	virtual	bool	testEvent(CEvent * inEvent);
	virtual	void	eventHandlerProc(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent);
	virtual	void	eventCompletionProc(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent);
};


/* -----------------------------------------------------------------------------
	C P a c k a g e B l o c k
----------------------------------------------------------------------------- */

class CPackageBlock
{
public:
	NewtonErr	init(ULong inId, ULong inVersion, size_t inSize, SourceType inSrcType, ULong inFlags, UniChar * inName, UniChar * inCopyright, ArrayIndex inNumOfParts, ULong inDate);

	ULong			fId;				//+00
	ULong			fVersion;		//+04
	size_t		fSize;			//+08
	SourceType	fSrcType;		//+0C	stream/mem
	ULong			fFlags;			//+14
	ULong			fDate;			//+18
	UniChar *	fName;			//+1C
	UniChar *	fCopyright;		//+20
	CDynamicArray *	fParts;	//+24
	ULong			fSignature;		//+28 'spbl' 'spvd' 'spcm'
	int			f2C;
//size+30
};


/* -----------------------------------------------------------------------------
	C P a c k a g e E v e n t H a n d l e r
----------------------------------------------------------------------------- */
class CClassInfo;

struct CInstalledPart
{
	CInstalledPart(ULong inType, int, int, bool, bool, bool, bool, VAddr);

	ULong		fType;
	int		f04;
	int		f08;
	CClassInfo *	f0C;
	ULong		f108:1;
	ULong		f104:1;
	ULong		f102:1;
	ULong		f101:1;
//size+14
};
class CPackageIterator;
class CValidatePackageDriver;	// actually PROTOCOL

class CPackageEventHandler : public CEventHandler
{
public:
				CPackageEventHandler();
	virtual	~CPackageEventHandler();

	virtual	void	eventHandlerProc(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent);
	virtual	void	eventCompletionProc(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent);

	void		registerEvent(CPkRegisterEvent * inEvent);
	void		unregisterEvent(CPkUnregisterEvent * inEvent);

	void		initValidatePackageDriver(void);

	NewtonErr	checkAndInstallPatch(PartInfo& inPart, SourceType inSrcType, const PartSource& inSource);
	NewtonErr	loadProtocolCode(void**, PartInfo&, SourceType, const PartSource&);
	ULong		getUniquePackageId(void);

	void		getBackupInfo(CPkBackupEvent * inEvent);

	NewtonErr	validatePackage(CPkBeginLoadEvent * inEvent, CPackageIterator * inIter);
	void		beginLoadPackage(CPkBeginLoadEvent * inEvent);
	void		safeToDeactivatePackage(CPkSafeToDeactivateEvent * inEvent);
	void		removePackage(CPkRemoveEvent * inEvent, bool, bool);

	size_t	getPartSize(void);
	bool		loadNextPart(NewtonErr * outErr, bool *, bool *);
	NewtonErr	installPart(VAddr *, int*, bool*, const PartId& inId, ExtendedPartInfo& inPart, SourceType inSrcType, const PartSource& inSource);
	void		removePart(const PartId& inId, const CInstalledPart& inPart, bool);

	NewtonErr	searchPackageList(ArrayIndex * outIndex, ULong inArg2);
	NewtonErr	searchPackageList(ArrayIndex * outIndex, UniChar * inName, ULong inArg3);
	NewtonErr	searchRegistry(ArrayIndex * outIndex, ULong*, ULong);

	void		setDefaultHeap(void);
	void		setPersistentHeap(void);

private:
	ArrayIndex				f10;
	CDynamicArray *		fPackageList;	//+14
	CDynamicArray *		fRegistry;		//+18	registry of user-defined part types
	bool						f1C;
	ObjectId					f20;
	ObjectId					f24;
	CPackageIterator *	fPkg;				//+28
	CPartPipe *				fPipe;			//+2C
	PartSource				fPartSource;	//+30
	ArrayIndex				fPartIndex;		//+38
	CPackageBlock *		fPkgInfo;		//+3C
	CShadowRingBuffer *	fBuffer;			//+40
	CArrayIterator			fBackupIter;	//+44
	int						f60;
	bool						f64;
	CValidatePackageDriver * fValidatePackageDriver;	//+68
// size +6C
};


#endif	/* __PACKAGEEVENTS_H */
