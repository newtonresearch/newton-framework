/*
	File:		TaskGlobals.h

	Contains:	User level interface to task class.

	Written by:	Newton Research Group.
*/

#if !defined(__TASKGLOBALS_H)
#define __TASKGLOBALS_H 1


// Get a pointer to the object you passed to the task
// which constitutes the "task global space".

extern "C" void *	GetGlobals(void);
extern void *		gCurrentGlobals;			// pointer to per task globals


/*------------------------------------------------------------------------------
	T a s k G l o b a l s

	The OS “switches” these writable globals.
------------------------------------------------------------------------------*/

typedef ULong			KernelParams[12];		// arguments and return values

struct TaskGlobals	// STaskSwitchedGlobals
{
	KernelParams		fParams;					// parameters for kernel calls
	int					fErrNo;					// slot for errors from stdio
	ObjectId				fDefaultHeapDomainId;
	VAddr					fStackTop;				// byte beyond top of stack we are running on
	VAddr					fStackBottom;			// last usable byte in this stack
	ObjectId				fTaskId;					// current task id
	Heap					fCurrentHeap;			// -> current heap
	NewtonErr			fMemErr;					// current task's MemError() status
	ULong					fTaskName;				// task name, from Init call
	ExceptionGlobals	fExceptionGlobals;	// globals for try/catch/Throw
// Exceptions currently assume that fExceptionGlobals appears LAST
//	size +54
};


// TaskGlobals immediately precede the task global space

inline TaskGlobals * TaskSwitchedGlobals()
{ return ((TaskGlobals *)gCurrentGlobals) - 1; }


extern NewtonErr  ResetAccountTime(void);
extern NewtonErr  GetNextTaskId(ObjectId inLastId, ObjectId * outId);
extern NewtonErr  Yield(ObjectId inTaskId);
extern void			TaskKillSelf(void);

// Arrange for current task's objs to go to 'inTo' when the task exits

extern NewtonErr	SetBequeathId(ObjectId inTo);


#endif	/* __TASKGLOBALS_H */
