/*
	File:		PageManager.h

	Contains:	Virtual memory page management declarations.

	Written by:	Newton Research Group.
*/

#if !defined(__PAGEMANAGER_H)
#define __PAGEMANAGER_H 1

#include "KernelPhys.h"
#include "SingleQ.h"
#include "DoubleQ.h"


/*------------------------------------------------------------------------------
	C D y n A r r a y
------------------------------------------------------------------------------*/

class CDynArray
{
public:
				CDynArray(int inUsed, int inSize);

	int		resize(int inDelta);

	int		fAllocSize;		// +00
	int		fUsedSize;		// +04
	void **	fArray;			// +08
};


/*------------------------------------------------------------------------------
	C R i n g
------------------------------------------------------------------------------*/

class CRing
{
public:
				CRing(int inSize)	: fIndex(0), fItems(0, inSize) { }

	int		push(void * inItem);
	void *	pop(void);
	void		rotate(int inDelta);
	void **	operator[](int index);

	int			fIndex;		// +00
	CDynArray	fItems;		// +04
};


/*------------------------------------------------------------------------------
	C P a g e T a b l e M a n a g e r
------------------------------------------------------------------------------*/

// Page Table Manager monitor selectors
enum PTMMonSelector
{
	kAllocatePageTable = 1,
	kReleasePageTable
};



class CPageTableManager : public SingleObject
{
public:
					CPageTableManager();

	NewtonErr	monitorProc(int inSelector, VAddr);
	NewtonErr	allocatePageTable(VAddr, PAddr &);

private:
	NewtonErr	allocatePageTable(void);
	NewtonErr	releasePageTable(VAddr);

	CRing			fRing;	// +00
// size +10
};


/*------------------------------------------------------------------------------
	P M M e s s a g e

	The message sent to the page manager monitor.
	NOTE	This differs from the ObjectMessage format. In particular, we donÕt
			bother with a message size field.
------------------------------------------------------------------------------*/
class CUMonitor;

typedef union
{
	ULong	result;

	struct
	{
		ObjectId		owner;	// +00
		int			index;	// +04
		CUMonitor * monitor; // +08
	} request;
//	size +0C
} PMMessage;


// Page Manager monitor selectors
enum PMMonSelector
{
	kGetPage,
	kGetExtPage,
	kReleasePage,
	kRegister,
	kGetFreePageCount
};


struct PMReleasePagesForFaultHandling
{
	ULong		f00;	// donÕt really know
	ULong		f04;
	ObjectId	f08;
};


/*------------------------------------------------------------------------------
	C P a g e M a n a g e r
------------------------------------------------------------------------------*/

class CPageManager : public SingleObject
{
public:
						CPageManager();

	static NewtonErr	make(ObjectId & outId, ObjectId inOwner, CLittlePhys * inPage);

	NewtonErr		monitorProc(int inSelector, PMMessage * inMsg);

	NewtonErr		get(ObjectId & outId, ObjectId inOwnerId, int index);
	NewtonErr		get(ObjectId & outId, ObjectId inOwnerId, int index, CUMonitor * inMonitor);
	NewtonErr		getExternal(ObjectId & outId, ObjectId inOwnerId, int index);
	NewtonErr		registerClient(ObjectId inClientId);	// was register
	NewtonErr		release(ObjectId inId);
	NewtonErr		releasePagesForFaultHandling(PMReleasePagesForFaultHandling * info);

	CLittlePhys *	queryClients(int inArg1, CUMonitor * inMonitor);
	bool				askOnePageToAClient(int inArg1, ObjectId inClientId);

private:
	bool		f00;			// +00
	CRing		fClients;	// +04
// size +14
};


/*------------------------------------------------------------------------------
	C P a g e T r a c k e r
------------------------------------------------------------------------------*/

class CPageTracker
{
public:
	void					initPages(ULong inOffset);

	void					put(CLittlePhys * inPage);
	CLittlePhys *		take();

	void					initPageCount(void);
	ULong					pageCount(void)	const;

private:
	CSingleQContainer	fPages;			// +00
	ArrayIndex			fPageCount;		// +08
};

inline void		CPageTracker::initPages(ULong inOffset)  { fPages.init(inOffset); }
inline void		CPageTracker::initPageCount(void)  { fPageCount = 0; }
inline ULong	CPageTracker::pageCount(void)	const  { return fPageCount; }

extern CPageTracker *			gPageTracker;


/*------------------------------------------------------------------------------
	C E x t P a g e T r a c k e r
------------------------------------------------------------------------------*/

class CExtPageTracker
{
public:
	static NewtonErr	init(CExtPageTracker ** ioTracker, ObjectId inPhysId, PAddr inBase, size_t inSize);

	bool				put(CLittlePhys * inPhys);
	CLittlePhys *  take(void);
	bool				removeReferences(ObjectId inId, bool * outDone);
	void				doDeferral(void);

private:
	bool					f00;
	bool					f01;
	ObjectId				fPhysId;			// +04
	CDoubleQItem		f08;
	CSingleQContainer	fPages;			// +14
	ArrayIndex			fPageCount;		// +1C
	ArrayIndex			fFreePageCount;// +20
	PAddr					f24;				// base address
	PAddr					f28;				// limit
// size +2C
//	CLittlePhys			f2C[];
};


/*------------------------------------------------------------------------------
	C E x t P a g e T r a c k e r M g r
------------------------------------------------------------------------------*/

class CExtPageTrackerMgr
{
public:
//						CExtPageTrackerMgr();

	NewtonErr		makeNewTracker(ObjectId inId1, PAddr inBase, size_t inSize);
	bool				put(CLittlePhys * inPhys);
	CLittlePhys *  take(void);
	NewtonErr		unhookTracker(ObjectId inId);
	void				doDeferral(void);
	NewtonErr		disposeTracker(ObjectId inId);

private:
	bool					f00;
	CDoubleQContainer	f04;
// size +18
};

extern CExtPageTrackerMgr *	gExtPageTrackerMgr;


/*------------------------------------------------------------------------------
	C U P a g e M a n a g e r
------------------------------------------------------------------------------*/

class CUPageManager
{
public:
	static NewtonErr	get(ObjectId & outId, ObjectId inOwnerId, int index);
	static NewtonErr	getExternal(ObjectId & outId, ObjectId inOwnerId, int index);
	static NewtonErr	registerClient(ObjectId inClientId);
	static NewtonErr	release(ObjectId inId);
	static NewtonErr	freePageCount(ULong * outCount);
};


/*------------------------------------------------------------------------------
	C U P a g e M a n a g e r M o n i t o r
------------------------------------------------------------------------------*/

class CUPageManagerMonitor
{
public:
	static NewtonErr	releaseRequest(ObjectId inMonitorId, int inArg2);
};


/*--------------------------------------------------------------------------------
	F u n c t i o n   P r o t o t y p e s
--------------------------------------------------------------------------------*/

extern CObjectTable *	gTheMemArchObjTbl;

inline CPhys *	PhysIdToObj(KernelTypes inType, ObjectId inId)
{ return (CPhys *)((ObjectType(inId) == inType) ? gTheMemArchObjTbl->get(inId) : NULL); }

NewtonErr	AllocatePageTable(VAddr inVAddr);


#endif	/* __PAGEMANAGER_H */
