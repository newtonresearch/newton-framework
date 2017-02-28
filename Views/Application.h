/*
	File:		Application.h

	Contains:	CApplication declarations.
					There is one shared instance of CApplication which handles commands,
					the undo stack and timed actions for all launched applications.

	Written by:	Newton Research Group.
*/

#if !defined(__APPLICATION_H)
#define __APPLICATION_H 1

#include "Responder.h"
#include "NewtonTime.h"
#include "Events.h"

/* -----------------------------------------------------------------------------
	C A l a r m E v e n t
----------------------------------------------------------------------------- */

class CAlarmEvent : public CIdleEvent
{
public:
	CTime			fTriggerTime;
	RefStruct	fCallbackFn;
	RefStruct	fCallbackArgs;
	RefStruct	f18;
};

/* -----------------------------------------------------------------------------
	C A p p l i c a t i o n
----------------------------------------------------------------------------- */
extern "C" Ref FSetSysAlarm(RefArg inRcvr, RefArg inTime, RefArg inCallback, RefArg inCallbackArgs);

class CApplication : public CResponder
{
public:
	VIEW_HEADER_MACRO

							CApplication();

	virtual	bool		doCommand(RefArg inCmd);

	virtual	void		init(void);
	virtual	void		run(void);
	virtual	void		idle(void);
	virtual	void		quit(void);

	virtual	void		initToolbox(void);

	NewtonErr	dispatchCommand(RefArg inCmd);

	void			postUndoCommand(RefArg inCmd);
	void			postUndoCommand(ULong inId, CResponder * inRcvr, long inArg);
	void			undo(void);
	void			clearUndo(void);
	Ref			getUndoStack(int inDepth);
	Ref			getUndoState(void);

	void			updateNextIdleTime(CTime inNextTime);
	CTime			getIdleTime(void) const;

	void			addDelayedAction(RefArg inRcvr, RefArg inFunc, RefArg inArg, RefArg inTime);
	CTime			nextDelayedActionTime(CTime inNextTime);
	bool			runNextDelayedAction(void);

protected:
	CTime			fIdleTime;			// +04
	RefStruct	fUndoStack;			// +0C array
	RefStruct	fRedoStack;			// +10 array
	bool			fNewUndo;			// +14
	bool			fRedo;				// +15
	RefStruct	fDelayedActions;	// +18 array
	ULong			x1C;
	friend Ref FSetSysAlarm(RefArg inRcvr, RefArg inTime, RefArg inCallback, RefArg inCallbackArgs);
	CAlarmEvent	fAlarm;				// +20
};

inline CTime	CApplication::getIdleTime(void) const  { return fIdleTime; }


#endif	/* __APPLICATION_H */
