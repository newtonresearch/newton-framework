/*
	File:		UserTasks.cc

	Contains:	User Task implementation.

	Written by:	Newton Research Group.
*/

#include "NewtonTime.h"
#include "KernelObjects.h"
#include "UserTasks.h"
#include "UserMonitor.h"
#include "OSErrors.h"

/*------------------------------------------------------------------------------
	C U T a s k
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
	Initialize by creating the kernel task we represent in user space.
	Args:		inTask				task function pointer
				inStackSize			taskÕs stack size
				inDataSize			size of global data + context data
				inData				pointer to that data (instance context for inTask)
				inPriority			scheduler priority
				inTaskName			identifier
				inEnvironment		environment; 0 => no restrictions
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
CUTask::init(TaskProcPtr inTask, size_t inStackSize, size_t inDataSize, void * inData, ULong inPriority, ULong inTaskName, ObjectId inEnvironment)
{
	NewtonErr err;

	XTRY
	{
		ObjectMessage	msg;
		CUSharedMem		smem;

		XFAIL(err = smem.init())
		XFAIL(err = smem.setBuffer(inData, inDataSize))
		msg.request.task.proc = inTask;
		msg.request.task.stackSize = inStackSize;
		msg.request.task.dataId = smem;
		msg.request.task.priority = inPriority;
		msg.request.task.name = inTaskName;
		msg.request.task.envId = inEnvironment;
		err = makeObject(kObjectTask, &msg, MSG_SIZE(7));
	}
	XENDTRY;

	return err;
}


/*------------------------------------------------------------------------------
	Initialize the task.
	As above, but with no environment.
------------------------------------------------------------------------------*/

NewtonErr
CUTask::init(TaskProcPtr inTask, size_t inStackSize, size_t inDataSize, void * inData, ULong inPriority, ULong inTaskName)
{
	return init(inTask, inStackSize, inDataSize, inData, inPriority, inTaskName, kNoId);
}


/*------------------------------------------------------------------------------
	Start the task.
	Args:		--
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
CUTask::start()
{
	ObjectMessage	msg;

	msg.target = fId;
	msg.size = MSG_SIZE(0);
	return gUObjectMgrMonitor->invokeRoutine(kStart, &msg);
}


/*--------------------------------------------------------------------------------
	Suspend the task.
	Args:		--
	Return:	error code
--------------------------------------------------------------------------------*/

NewtonErr
CUTask::suspend()
{
	ObjectMessage	msg;

	msg.target = fId;
	msg.size = MSG_SIZE(0);
	return gUObjectMgrMonitor->invokeRoutine(kSuspend, &msg);
}


/*------------------------------------------------------------------------------
	Set a task register.
	Args:		inReg				register index
				inValue			register value
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
CUTask::setRegister(ULong inReg, ULong inValue)
{
	ObjectMessage	msg;

	msg.target = fId;
	msg.size = MSG_SIZE(2);
	msg.request.reg.number = inReg;
	msg.request.reg.value = inValue;
	
	return gUObjectMgrMonitor->invokeRoutine(kSetRegister, &msg);
}


/*------------------------------------------------------------------------------
	Set a task register.
	Args:		inReg				register index
				outValue			register value
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
CUTask::getRegister(ULong inReg, ULong * outValue)
{
	NewtonErr		err;
	ObjectMessage	msg;

	msg.target = fId;
	msg.size = MSG_SIZE(1);
	msg.request.reg.number = inReg;
	
	err = gUObjectMgrMonitor->invokeRoutine(kGetRegister, &msg);
	*outValue = msg.result;
	return err;
}


#pragma mark -
/*------------------------------------------------------------------------------
	C U T a s k W o r l d
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
	Constructor.
------------------------------------------------------------------------------*/

CUTaskWorld::CUTaskWorld()
{
	fIsSpawned = NO;
	fIsOwnedByParent = NO;
	fWantResult = NO;
}


/*------------------------------------------------------------------------------
	Destructor.
------------------------------------------------------------------------------*/

CUTaskWorld::~CUTaskWorld()
{ }


/*------------------------------------------------------------------------------
	Start a child task.
	Args:		inWantResultFromChild	YES => wait for task init ack before starting task
				inWantOwnerShip			YES => we own the child task; NO => itÕs independent
				inStartTimeout				delay after which we give up if no start ack
				inStackSize					stack size
				inPriority					priority
				inTaskName					name
				inEnvironment				environment id
	Return	error code
------------------------------------------------------------------------------*/

NewtonErr
CUTaskWorld::startTask(bool inWantResultFromChild, bool inWantOwnerShip, Timeout inStartTimeout, size_t inStackSize, ULong inPriority, ULong inTaskName, ObjectId inEnvironment)
{
	NewtonErr	err;

	XTRY
	{
		fWantResult = inWantResultFromChild;
		fIsOwnedByParent = inWantOwnerShip;

		if (fWantResult)
			XFAIL(err = fMotherPort.init())

		XFAIL(err = fChildTask.init(MemberFunctionCast(TaskProcPtr, this, &CUTaskWorld::taskEntry), inStackSize, getSizeOf(), this, inPriority, inTaskName, inEnvironment))
		XFAIL(err = fChildTask.start())
		if (fIsOwnedByParent)
		{
			ObjectId		childId = fChildTask;
			fChildTask = kNoId;	// sic
			fChildTask = childId;
			XFAIL(err = TaskAcceptObject(fChildTask))
		}

		if (fWantResult)
		//	send empty message saying we spawned successfully
			err = fMotherPort.sendRPC(NULL, NULL, 0, NULL, 0, inStartTimeout, kSpawnedTaskAckMsgType);
	}
	XENDTRY;

	return err;
}


/*------------------------------------------------------------------------------
	Start a child task in the global environment.
	Args:		inWantResultFromChild	YES => wait for task init ack before starting task
				inWantOwnerShip			YES => we own the child task; NO => itÕs independent
				inStartTimeout				delay after which we give up if no start ack
				inStackSize					stack size
				inPriority					priority
				inTaskName					name
	Return	error code
------------------------------------------------------------------------------*/

NewtonErr
CUTaskWorld::startTask(bool inWantResultFromChild, bool inWantOwnerShip, Timeout inStartTimeout, size_t inStackSize, ULong inPriority, ULong inTaskName)
{
	return startTask(inWantResultFromChild, inWantOwnerShip, inStartTimeout, inStackSize, inPriority, inTaskName, 0);
}


/*------------------------------------------------------------------------------
	Child task constructor.
	To be overridden.
	Args:		--
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
CUTaskWorld::taskConstructor()
{ return noErr; }


/*------------------------------------------------------------------------------
	Child task destructor.
	To be overridden.
	Args:		--
	Return:	--
------------------------------------------------------------------------------*/

void
CUTaskWorld::taskDestructor()
{ }


/*------------------------------------------------------------------------------
	Child task entry.
	Construct the child task; call its main function; if it should ever return,
	destroy it.
	Args:		inDataSize			size of child task data - unused
				inTaskId				id of child task to enter
	Return:	error code
------------------------------------------------------------------------------*/

void
CUTaskWorld::taskEntry(size_t inDataSize, ObjectId inTaskId)
{
	NewtonErr err;

	XTRY
	{
		CUMsgToken token;
		fChildTask = inTaskId;
		fIsSpawned = YES;
		if (fWantResult)
		//  wait for notification that task spawned successfully
			XFAIL(err = fMotherPort.receive(NULL, NULL, 0, &token, NULL, kNoTimeout, kSpawnedTaskAckMsgType, NO, NO))

		err = taskConstructor();

		if (fWantResult)
			//  send notification of child task construction to parent
			XFAIL(token.replyRPC(NULL, 0, err))

		XFAIL(err)

		taskMain();
	}
	XENDTRY;

	taskDestructor();
	this->~CUTaskWorld();
}

