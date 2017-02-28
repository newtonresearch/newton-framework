/*
	File:		NewtWorld.h

	Contains:	Newton world declarations.

	Written by:	Newton Research Group.
*/

#if !defined(__NEWTWORLD_H)
#define __NEWTWORLD_H 1

#include "AppWorld.h"
#include "UserSharedMem.h"
#include "NewtGlobals.h"

/*--------------------------------------------------------------------------------
	C L o a d e r
--------------------------------------------------------------------------------*/

class CLoader : public CAppWorld
{
protected:
	virtual size_t		getSizeOf(void) const;

	virtual NewtonErr	mainConstructor(void);
	virtual void		mainDestructor(void);
	virtual void		theMain(void);
};


/*--------------------------------------------------------------------------------
	C N e w t E v e n t H a n d l e r
--------------------------------------------------------------------------------*/

class CNewtEventHandler : public CEventHandler
{
public:
						CNewtEventHandler();

				void	setWakeupTime(Timeout inDelta);

	virtual	void	eventHandlerProc(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent);
	virtual	void	eventCompletionProc(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent);
	virtual	void	idleProc(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent);
};


/*--------------------------------------------------------------------------------
	C N e w t W o r l d
--------------------------------------------------------------------------------*/

class CNewtWorld : public CAppWorld
{
public:
	void					handleEvents(Timeout inDelta);
	ObjectId				msgId(void) const;
			
protected:
	virtual size_t		getSizeOf(void) const;

	virtual NewtonErr forkInit(CForkWorld * inWorld);
	virtual NewtonErr forkConstructor(CForkWorld * inWorld);
	virtual void		forkDestructor(void);

	virtual NewtonErr	mainConstructor(void);
	virtual NewtonErr	preMain(void);
	virtual void		theMain(void);

	virtual void		forkSwitch(bool inDoFork);
	virtual CForkWorld *	makeFork(void);

	virtual NewtonErr eventDispatch(ULong inArg1, CUMsgToken * inMsg, size_t * inArg3, CEvent * ioEvent);

	CUSharedMemMsg *		f70;
	int32_t					f74;
	CNewtEventHandler *	fEventHandler; // +78
	NewtGlobals				fGlobals;		// +7C
// size +94
};

inline void CNewtWorld::handleEvents(Timeout inDelta)
{ fEventHandler->setWakeupTime(inDelta); }

inline ObjectId CNewtWorld::msgId(void) const
{ return *f70; }


#define gNewtWorld static_cast<CNewtWorld*>(GetGlobals())

#endif	/* __NEWTWORLD_H */
