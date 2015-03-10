/*
	File:		KernelObjects.h

	Contains:	Kernel object management definitions.

	Written by:	Newton Research Group.
*/

#if !defined(__KERNELOBJECTS_H)
#define __KERNELOBJECTS_H 1

#include "SharedTypes.h"
#include "KernelTypes.h"

typedef unsigned int KernelObjectState;

/*--------------------------------------------------------------------------------
	O b j e c t M e s s a g e

	The message that gets dispatched to the Object Manager monitor
--------------------------------------------------------------------------------*/
class CUSharedMem;

struct ObjectMessage
{
	size_t		size;				// +00
	int			result;			// +04
	ObjectId		target;			// +08

	union
	{
		struct
		{
			int				f0C;				// +0C
			TaskProcPtr		proc;				// +10
			size_t			stackSize;		// +14
			ObjectId			dataId;			// +18
			ULong				priority;		// +1C
			ULong				name;				// +20
			ObjectId			envId;			// +24
		} task;

		struct
		{
			ULong				number;			// +0C
			ULong				value;			// +10
		} reg;

		struct
		{
			Heap				heap;				// +0C
		} environment;

		struct
		{
			ObjectId			domainId;		// +0C
			bool				isManager;		// +10
			bool				isStack;			// +14
			bool				isHeap;			// +18
		} addDomain;

		struct
		{
			ObjectId			monitorId;		// +0C	fault monitor
			VAddr				rangeStart;		// +10
			size_t			rangeLength;	// +14
		} domain;

		struct
		{
			ObjectId			domainId;		// +0C
			ObjectId			monitorId;		// +10
		} faultMonitor;

		struct
		{
			ULong				opCount;			// +0C
			SemOp				ops[6];			// +10
		} semList;

		struct
		{
			ULong				size;				// +0C
		} semGroup;

		struct
		{
			MonitorProcPtr	proc;					// +0C
			size_t			stackSize;			// +10
			void *			context;				// +14
			ObjectId			envId;				// +18
			bool				isRebootProtected;// +1C
			bool				isFaultMonitor;	// +1D
			ULong				name;					// +20
		} monitor;

		struct
		{
			int				f0C;					// +0C
			PAddr				base;					// +10
			size_t			size;					// +1C
			bool				readOnly;			// +20
			bool				cache;				// +21
		} phys;

		struct
		{
			ULong				huh1;				// +0C
			ULong				huh2;				// +10
		} tracker;
	} request;
//	size 0x28
};
typedef struct ObjectMessage ObjectMessage;

#define MSG_SIZE(_n) ((3 + _n) * sizeof(long))


//	Object Manager monitor dispatch codes
enum
{
	kCreate,
	kDestroy,
	kUnused2,
	kStart,
	kSuspend,
	kSetRegister,
	kGetRegister,
	kAddDomain,
	kGetContent,
	kRemoveDomain,
	kSetDomainFaultMonitor,
	kStartTrackingExtPages,
	kStopTrackingExtPages,
	kKill = 255
};


/*--------------------------------------------------------------------------------
	C O b j e c t
	Everything in the kernel is a CObject identified by its unique id.
	These objects are held in a CObjectTable managed and accessed by the
	CObjectManager which executes as a monitor.
--------------------------------------------------------------------------------*/
class CObjectTable;
class CObjectTableIterator;

class CObject : public SingleObject
{
public:
					CObject(ObjectId id = 0)			{ fId = id; }
					CObject(const CObject & inCopy)	{ fId = inCopy.fId; }

					operator	ObjectId()	const			{ return fId; }

	void			setOwner(ObjectId id)				{ fOwnerId = id; }
	ObjectId		owner(void)				const			{ return fOwnerId; }

	void			assignToTask(ObjectId id)			{ fAssignedOwnerId = id; }
	ObjectId		assignedOwner(void)	const			{ return fAssignedOwnerId; }

protected:
	friend class CObjectTable;
	friend class CObjectTableIterator;

	ObjectId		fId;					// +00
	CObject *	fNext;				// +04
	ObjectId		fOwnerId;			// +08
	ObjectId		fAssignedOwnerId;	// +0C
};


/*--------------------------------------------------------------------------------
	C O b j e c t M a n a g e r
	All access to kernel objects is through the CObjectManmager.
--------------------------------------------------------------------------------*/

class CObjectManager : public SingleObject
{
public:
						CObjectManager();
	NewtonErr		monitorProc(int inSelector, ObjectMessage * inMsg);

private:
	ObjectId			fDeferredId;
};


/*--------------------------------------------------------------------------------
	C O b j e c t T a b l e
--------------------------------------------------------------------------------*/
#define kObjectTableSize	0x80
#define kObjectTableMask	0x7F

typedef void (*ScavengeProcPtr)(CObject *);
typedef ScavengeProcPtr (*GetScavengeProcPtr)(CObject *, ULong);

class CObjectTable
{
public:
	NewtonErr		init(void);

	void				setScavengeProc(GetScavengeProcPtr inScavenger);
	void				scavenge(void);
	void				scavengeAll(void);

	ObjectId			newId(KernelTypes inType);
	ObjectId			add(CObject * inObject, KernelTypes inType, ObjectId inOwnerId);
	bool				exists(ObjectId inId);
	CObject *		get(ObjectId inId);
	NewtonErr		remove(ObjectId inId);

	void				reassignOwnership(ObjectId inFromOwner, ObjectId inToOwner);

private:
	friend class CObjectTableIterator;

	ObjectId			nextGlobalUniqueId(void);
	ArrayIndex		tableIndex(ObjectId inId);

	GetScavengeProcPtr	fScavenge;				// +00
	CObject *		fThisObj;
	CObject *		fPrevObj;
	ArrayIndex		fScavengeIndex;
	CObject *		fObject[kObjectTableSize];	// +10
};


/*--------------------------------------------------------------------------------
	C O b j e c t T a b l e I t e r a t o r
--------------------------------------------------------------------------------*/

class CObjectTableIterator
{
public:
						CObjectTableIterator(CObjectTable * inTable, ObjectId inId);

	bool				setCurrentPosition(ObjectId inId);
	ObjectId			getNextTableId(void);
	ObjectId			getNextTypedId(KernelTypes inType);
	CObject *		getNextTypedObject(KernelTypes inType);

private:
	bool				getThisLineNextEntry(void);

	ArrayIndex		fIndex;
	ArrayIndex		fInitialIndex;
	ObjectId			fId;
	ObjectId			fInitialId;
	bool				fIsDone;
	CObject *		fObject;
	CObjectTable *	fTable;
};


/*--------------------------------------------------------------------------------
	F u n c t i o n   P r o t o t y p e s
--------------------------------------------------------------------------------*/

NewtonErr	RegisterObject(CObject * inObject, KernelTypes inType, ObjectId inOwnerId, ObjectId * outId);
NewtonErr	ConvertIdToObj(KernelTypes inType, ObjectId inId, void * ioObj);


extern CObjectTable *	gObjectTable;
inline CObject *		IdToObj(KernelTypes inType, ObjectId inId)
{ return (ObjectType(inId) == inType) ? gObjectTable->get(inId) : NULL; }


#endif	/* __KERNELOBJECTS_H */
