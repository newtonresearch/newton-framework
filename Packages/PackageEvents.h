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
#include "UserPorts.h"

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
			CPkRegisterEvent(ULong inArg1, ULong inArg2);

	ULong		f10;
	ULong		f14;
};


/*------------------------------------------------------------------------------
	C P k U n r e g i s t e r E v e n t
------------------------------------------------------------------------------*/
class CPkUnregisterEvent : public CPkBaseEvent
{
public:
			CPkUnregisterEvent(ULong inArg1);

	ULong		f10;
};


/*------------------------------------------------------------------------------
	C P k B e g i n L o a d E v e n t
------------------------------------------------------------------------------*/
class CPkBeginLoadEvent : public CPkBaseEvent
{
public:
			CPkBeginLoadEvent(SourceType inType, const PartSource & inSource, ObjectId inArg3, ObjectId inArg4, bool inArg5);

	ULong			f10;
	ULong			f14;
	SourceType	f18;
	PartSource	f20;
	bool			f28;
	long			f2C;
	char			f30[0x54];
// size +84
};


/*------------------------------------------------------------------------------
	C P k R e m o v e E v e n t
------------------------------------------------------------------------------*/
class CPkRemoveEvent : public CPkBaseEvent
{
public:
			CPkRemoveEvent(ULong inArg1, ULong inArg2, ULong inArg3);

	ULong		f10;
	ULong		f14;
	ULong		f18;
};


/*------------------------------------------------------------------------------
	C P k B a c k u p E v e n t
------------------------------------------------------------------------------*/
class CPkBackupEvent : public CPkBaseEvent
{
public:
			CPkBackupEvent(long inArg1, ULong inArg2, bool inArg3, const PartSource & inArg4, ObjectId inArg5, ObjectId inArg6);

	long			f10;
	ULong			f14;
	PartSource	f18;
	ULong			f20;
	ULong			f24;
	UniChar		f28[kMaxPackageNameSize];	// 0x5C?
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
	char				fInfo[kMaxInfoSize];	// +94
	char				fCompressorName[kMaxCompressorNameSize];	// +D4
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
		CPkSafeToDeactivateEvent(ULong inArg1);

		ULong	f10;
		bool	f14;
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


struct CInstalledPart;
class CPackageIterator;

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

	void		checkAndInstallPatch(PartInfo&, SourceType, const PartSource&);
	void		loadProtocolCode(void**, PartInfo&, SourceType, const PartSource&);
	void		getUniquePackageId(void);

	void		getBackupInfo(CPkBackupEvent * inEvent);

	void		validatePackage(CPkBeginLoadEvent * inEvent, CPackageIterator * inIter);
	void		beginLoadPackage(CPkBeginLoadEvent * inEvent);
	void		safeToDeactivatePackage(CPkSafeToDeactivateEvent * inEvent);
	void		removePackage(CPkRemoveEvent * inEvent, bool, bool);

	void		getPartSize(void);
	void		loadNextPart(long*, unsigned char*, unsigned char*);
	void		installPart(unsigned long*, long*, unsigned char*, const PartId&, ExtendedPartInfo&, SourceType, const PartSource&);
	void		removePart(const PartId&, const CInstalledPart&, unsigned char);

	NewtonErr	searchPackageList(ArrayIndex * outIndex, ULong inArg2);
	NewtonErr	searchPackageList(ArrayIndex * outIndex, UniChar * inName, ULong inArg3);
	NewtonErr	searchRegistry(ArrayIndex * outIndex, ULong*, ULong);

	void		setDefaultHeap(void);
	void		setPersistentHeap(void);

private:
	CDynamicArray *	f14;
	CDynamicArray *	f18;
	bool					f1C;
	int					f28;
	int					f2C;
	int					f3C;
	int					f40;
	CArrayIterator		f44;
	int					f60;
	bool					f64;
// size +6C
};


#endif	/* __PACKAGEEVENTS_H */
