/*
	File:		PowerManager.h

	Contains:	Power management task world.

	Written by:	Newton Research Group.
*/

#if !defined(__POWERMANAGER_H)
#define __POWERMANAGER_H 1

#include "AppWorld.h"

class CPowerManagerEvent;

class CPowerManager : public CAppWorld
{
public:
				CPowerManager();
	virtual	~CPowerManager();

protected:
	virtual size_t		getSizeOf(void) const;

	virtual NewtonErr	mainConstructor(void);
	virtual void		mainDestructor(void);

	void		doCommand(CUMsgToken * inToken, size_t * inSize, CPowerManagerEvent * inEvent);
	void		doReply(CUMsgToken * inToken, size_t * inSize, CPowerManagerEvent * inEvent);

	void		backlightMessage(void);
	void		powerOffMessage(void);
	void		powerOffTimeout(void);

	CUAsyncMessage	x90;
// size +E4
};

extern void		InitPowerManager(void);


#endif	/* __POWERMANAGER_H */
