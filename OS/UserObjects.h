/*
	File:		UserObjects.h

	Contains:	User Object definition.

	Written by:	Newton Research Group.
*/

#if !defined(__USEROBJECTS_H)
#define __USEROBJECTS_H 1

#if !defined(__NEWTON_H)
#include "Newton.h"
#endif

#if !defined(__KERNELTYPES_H)
#include "KernelTypes.h"
#endif

#if !defined(__SHAREDTYPES_H)
#include "SharedTypes.h"
#endif


// We have to explicitly declare this here before we can tell CUObject that it's a friend
extern "C" void MonitorExitAction(ObjectId inMonitorId, ULong inAction);


/*------------------------------------------------------------------------------
	C U O b j e c t

	This is the interface to the object manager.  User objects are derived from
	this class.

	All objects consist fundamentally of an object Id.  This Id is given
	to the kernel on all calls related to that Id.  If you want to pass
	a given object around, the correct thing to do is to ask for the Id
	by assigning this object to something of the type ObjectId.
------------------------------------------------------------------------------*/

class CUObject : public SingleObject
{
public:
				CUObject(ObjectId id = 0);
				CUObject(CUObject & inCopy);
				~CUObject();

				operator	ObjectId()	const;
	void		operator=(ObjectId id);
	void		operator=(const CUObject & inCopy);

	NewtonErr	makeObject(ObjectTypes inObjectType, struct ObjectMessage * inMsg, size_t inMsgSize);
	void		destroyObject(void);
	void		denyOwnership(void);
	bool		isExtPage(void);

	ObjectId	fId;

protected:
	friend	void MonitorExitAction(ObjectId inMonitorId, ULong inAction);
	friend	class CUMonitor;

	void		copyObject(const ObjectId inId);
	void		copyObject(const CUObject & inCopy);

	bool		fObjectCreatedByUs;
};

/*------------------------------------------------------------------------------
	C U O b j e c t   I n l i n e s
------------------------------------------------------------------------------*/

inline			CUObject::CUObject(ObjectId id)  { fObjectCreatedByUs = NO; fId = id; }
inline			CUObject::CUObject(CUObject & inCopy)	{ fObjectCreatedByUs = NO; fId = inCopy.fId; }
inline			CUObject::operator ObjectId()	const  { return fId; }
inline void		CUObject::operator=(ObjectId id)  { copyObject(id); }
inline void		CUObject::operator=(const CUObject & inCopy)  { copyObject(inCopy); }
inline void		CUObject::copyObject(const CUObject & inCopy)  { copyObject(inCopy.fId); }
inline void		CUObject::denyOwnership(void)  { fObjectCreatedByUs = NO; }
inline bool		CUObject::isExtPage(void)  { return ObjectType(fId) == kExtPhysType; }


/*------------------------------------------------------------------------------
	P u b l i c   I n t e r f a c e
------------------------------------------------------------------------------*/

extern "C" NewtonErr	TaskGiveObject(ObjectId inId, ObjectId inAssignToTaskId);
extern "C" NewtonErr	TaskAcceptObject(ObjectId inId);


#if !defined(__USERSHAREDMEM_H)
#include "UserSharedMem.h"
#endif

#endif	/* __USEROBJECTS_H */
