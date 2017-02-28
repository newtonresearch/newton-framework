/*
	File:		UserTasks.h

	Contains:	User level interface to task class.

	Written by:	Newton Research Group.
*/

#if !defined(__USERTASKS_H)
#define __USERTASKS_H 1

#if !defined(__NEWTON_H)
#include "Newton.h"
#endif
#if !defined(__SHAREDTYPES_H)
#include "SharedTypes.h"
#endif
#if !defined(__USERPORTS_H)
#include "UserPorts.h"
#endif
#if !defined(__LONGTIME_H)
#include "LongTime.h"
#endif
#if !defined(__USERGLOBALS_H)
#include "UserGlobals.h"
#endif
#if !defined(__TASKGLOBALS_H)
#include "TaskGlobals.h"
#endif


/*------------------------------------------------------------------------------
	C U T a s k
------------------------------------------------------------------------------*/

class CUTask : public CUObject
{
public:
					CUTask(ObjectId id = 0);
	void			operator=(const CUTask & inCopy);
	NewtonErr	init(TaskProcPtr inTask, size_t inStackSize, size_t inDataSize, void * inData, ULong inPriority, ULong inTaskName, ObjectId inEnvironment);
	NewtonErr	init(TaskProcPtr inTask, size_t inStackSize, size_t inDataSize, void * inData, ULong inPriority = kUserTaskPriority, ULong inTaskName = 'UNAM');

	NewtonErr	start();
	NewtonErr	suspend();
	NewtonErr	setRegister(ULong inReg, ULong inValue);
	NewtonErr	getRegister(ULong inReg, ULong * outValue);
};


enum
{
	kSpawnedTaskStackSize	= 6000,
	kSpawnedTaskAckTimeout	= 15000,
	kSpawnedTaskAckMsgType	= 0x00800000
};

/*------------------------------------------------------------------------------
	C U T a s k   I n l i n e s
------------------------------------------------------------------------------*/

inline			CUTask::CUTask(ObjectId id) : CUObject(id)  { }
inline void		CUTask::operator=(const CUTask & inCopy)  { copyObject(inCopy); }


/*------------------------------------------------------------------------------
	C U T a s k W o r l d

	This is the task spawner class.
	It will spawn a task that represents itself and continue as that task.
------------------------------------------------------------------------------*/

class CUTaskWorld : public SingleObject
{
public:
						CUTaskWorld();
	virtual			~CUTaskWorld();

	NewtonErr		startTask(bool inWantResultFromChild, bool inWantOwnership, Timeout inStartTimeout, size_t inStackSize, ULong inPriority, ULong inTaskName, ObjectId inEnvironment);	// make a new task and object
	NewtonErr		startTask(bool inWantResultFromChild = true, bool inWantOwnership = false, Timeout inStartTimeout = kNoTimeout, size_t inStackSize = kSpawnedTaskStackSize, ULong inPriority = kUserTaskPriority, ULong inTaskName = 'UNAM');	// make a new task and object
	ObjectId			getChildTaskId(void) const;

protected:
	virtual size_t		getSizeOf() const = 0;	// object must return its own size
	virtual NewtonErr taskConstructor();		// spawned task calls here to construct itself
	virtual void		taskDestructor();			// spawned task calls here to destroy itself
	virtual void		taskMain() = 0;			// spawned task calls here to begin running

	void				taskEntry(size_t inDataSize, ObjectId inTaskId);	// low level entry for spawned task (only in base class)

	// these instance vars are used by the task creator (the parent)
	bool			fIsSpawned;					// +04	this indicates if this object is running a new task
	bool			fIsOwnedByParent;			// +05	true if parent owns child, false if child independent
	bool			fWantResult;				// +06	true if result should be passed back from child
	CUPort		fMotherPort;				// +08	port used to talk between objects in pre and post task state
	CUTask		fChildTask;					// +10	created task
};

struct StackLimits	// was ULockStack
{
	VAddr	start;
	VAddr	end;
};

/*------------------------------------------------------------------------------
	C U T a s k W o r l d   I n l i n e s
------------------------------------------------------------------------------*/

inline 	ObjectId	CUTaskWorld::getChildTaskId(void) const
{ return fChildTask; }


/*------------------------------------------------------------------------------
	F u n c t i o n   P r o t o t y p e s
------------------------------------------------------------------------------*/

/* -----------------------------------------------------------------------------
	Reboot -

	This function will reboot the machine in one of two states,
		1) Warmboot: we will try to preserve the persistent areas.
		2) Coldboot: all data will be lost.

	The error parameter is saved so that the main user application,
	upon reboot, can display an appropriate error dialog.

	The safe flag indicates whether we should wait for any protected monitors
	to finish or not.  True means wait.

	Pass kRebootMagicNumber for the rebootType if you want a cold boot,
	zero will do a warm boot.
----------------------------------------------------------------------------- */

NewtonErr	Reboot(NewtonErr inError = noErr, ULong inRebootType = 0, bool inSafe = false);


/* -----------------------------------------------------------------------------
	Environment -
----------------------------------------------------------------------------- */

// Set the current task’s environment and return the previous one.
NewtonErr	SetEnvironment(ObjectId inNewEnvId, ObjectId * outOldEnvId = NULL);

// Return the id of the current task’s environment.
NewtonErr	GetEnvironment(ObjectId * outEnvId);

//	Environment Add/Remove/Test glue (eventually calls GenericSWI)
NewtonErr	EnvironmentHasDomain(ObjectId inEnvId, ObjectId inDomId, bool * outHasDomain, bool * outIsManager);
NewtonErr	AddDomainToEnvironment(ObjectId inEnvId, ObjectId inDomId, ULong inFlags);
NewtonErr	RemoveDomainFromEnvironment(ObjectId inEnvId, ObjectId inDomId);


#endif	/* __USERTASKS_H */
