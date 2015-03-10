/*
	File:		PSSManager.h

	Contains:	Newton Persistent Store System (PSS) interface.
					The PSSManager handles insertion and ejection of storage cards.

	Written by:	Newton Research Group.
*/

#if !defined(__PSSMANAGER_H)
#define __PSSMANAGER_H 1

#include "AppWorld.h"
#include "Arrays.h"
#include "Globals.h"
#include "LargeBinaries.h"
#include "CardEvents.h"


/*--------------------------------------------------------------------------------
	C P S S E v e n t
--------------------------------------------------------------------------------*/

class CPSSEvent : public CEvent
{
public:
	ULong		f08;		// 'card'
	ULong		f0C;		// index
// size+20?
};


/*--------------------------------------------------------------------------------
	C P S S E v e n t H a n d l e r
--------------------------------------------------------------------------------*/

class CPSSEventHandler : public CEventHandler
{
public:
	virtual	void	eventHandlerProc(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent);
	virtual	void	eventCompletionProc(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent);
};


/*--------------------------------------------------------------------------------
	C P S S S y s t e m E v e n t H a n d l e r
--------------------------------------------------------------------------------*/

class CPSSSystemEventHandler : public CSystemEventHandler
{
public:
	void	newCard(CEvent * inEvent);
};


/*--------------------------------------------------------------------------------
	P S S S t o r e I n f o
	P S S S l o t I n f o
--------------------------------------------------------------------------------*/

struct PSSStoreInfo
{
	void		clear(void);

#if defined(correct)
	CUAsyncMessage	f00;
#endif
	CStore *	f10;
	ULong		f14;
	int		f18;
	void *	f1C;
	ULong		f30;
	bool		f38;
	bool		f39;
	ULong		f3C;
	CStore *	f40;
// size +50
};


struct PSSSlotInfo
{
	void		clear(void);

	int				f00;
	CCardMessage	f04;
	PSSStoreInfo	fBC[4];
// size +1FC
};


/*--------------------------------------------------------------------------------
	C P S S M a n a g e r
	Handles insertion and ejection of storage cards.
--------------------------------------------------------------------------------*/

class CPSSManager : public CAppWorld
{
public:
					CPSSManager();

	void			UIEngine(bool);
	bool			messageInUse(void);

	int			doReplyTransitions(void);
	void			stuffSendAndTransition(int, int, int);

	bool			registerStores(void);
	bool			deregisterStores(void);
	void			gcStores(void);

	void			cardAvailable(CCardMessage * inMessage);
	void			cardGone(CCardMessage * inMessage);
	void			cardIsSame(CCardMessage * inMessage);
	size_t		getCardSlotStores(int inSlot, CStore ** outStores) const;
	const PSSStoreInfo *	getStorePSSInfo(const CStore * inStore, bool) const;

	void			reinsertCard(int inSlot, const UniChar * inReason, bool);

	void			sendServer(ULong, ULong, ULong, Timeout, CTime*);
	void			replyServer(CCardMessage * inMessage, ULong, ULong, ULong);

	void			doCommand(CUMsgToken * inToken, size_t * inSize, CCardMessage * inMessage, bool);
	void			doReply(CUMsgToken * inToken, size_t * inSize, CCardMessage * inMessage, bool);

protected:
	virtual NewtonErr	mainConstructor(void);
	virtual void		mainDestructor(void);
	virtual void		theMain(void);

private:
// start +70
	CUAsyncMessage		f70;
	CCardMessage		f80;
	CUPort *				f138;
	CUPort				f13C;		// newtPort
	CUPort				f144;		// cardPort
	CPSSEventHandler	f14C;
	CPSSSystemEventHandler	f160;
	CUObject				f178;
	CUAsyncMessage		f180;
	CEvent				f190;		// size +B4
	CUAsyncMessage		f244[4];
	CPSSEvent			f284[4];
	ArrayIndex			f304;		// numOfHWSockets
	PSSSlotInfo			f308[4];
	CUMsgToken *		fAF8[4];
	PSSSlotInfo *		fB08[4];
// size +B18
};


#define gPSSWorld static_cast<CPSSManager*>(GetGlobals())


extern bool							IsValidStore(const CStore * inStore);
extern const PSSStoreInfo *	GetStorePSSInfo(const CStore * inStore);


struct SCardReinsertionTracker
{
	bool			f00;
	CStore *		f04;
	PSSId			f08;
	ULong			f0C;
};


/*------------------------------------------------------------------------------
	D a t a
------------------------------------------------------------------------------*/

extern CStore *	gInRAMStore;		// 0C1016C4
extern CStore *	gMuxInRAMStore;	// 0C1016C8


#endif	/* __PSSMANAGER_H */
