/*
	File:		StoreObjRef.h

	Contains:	Store object reference declarations.
	
	Written by:	Newton Research Group, 2010.
*/

#if !defined(__STOREOBJREF_H)
#define __STOREOBJREF_H 1

#include "PSSTypes.h"

class CFlashStore;

#define MAKE_ID(n) (n >> 2)
#define MAKE_OFFSET(n) (n >> 2)
#define MAKE_ADDR(n) (n << 2)

/*------------------------------------------------------------------------------
	Erased bits in flash memory are always set.
------------------------------------------------------------------------------*/

#define kVirginBits 0xFFFFFFFF

#define ISDIRTYBIT(_bit) (_bit == (fStore->fDirtyBits & 1))
#define ISVIRGINBIT(_bit) (_bit == (fStore->fVirginBits & 1))

#define ISVIRGINOBJECT(_obj) (*(ULong *)_obj == _obj->fStore->fVirginBits)
#define ISVIRGINDIRENT(_dire) (*(ULong *)_dire == fStore->fVirginBits)


/*------------------------------------------------------------------------------
	S D i r E n t
------------------------------------------------------------------------------*/

// number of SDirEnt items in a bucket
#define kBucketSize	16


struct SDirEnt
{
	bool		isValid(CFlashStore * inStore);

	static bool	isValidMigratedObjectInfo(int, int);
	void		setMigratedObjectInfo(int, int);
	void		getMigratedObjectInfo(int*, int*) const;
#if forDebug
	void		print(void);
#endif

	unsigned int		zaddr:24;
//or migrated object info?
//	unsigned int		x31:10;
//	unsigned int		x21:14;	// object number?

	unsigned int		x7:1;		// LSL 24
	unsigned int		x6:1;		// LSL 25
	unsigned int		x5:1;		// LSL 26
	unsigned int		x4:1;		// LSL 27
	unsigned int		x3:1;		// LSL 28	isInUse?
	unsigned int		x2:1;		// LSL 29
};


/*------------------------------------------------------------------------------
	S t o r e O b j H e a d e r
	Was SObject.
	Every object in the store is preceded by this header.
------------------------------------------------------------------------------*/

struct StoreObjHeader
{
	bool		isValid(CFlashStore * inStore);

	unsigned int x63:1;			//+00
	unsigned int zapped:1;		//x62
	unsigned int x61:1;
	unsigned int x60:1;
	unsigned int id:28;

	unsigned int size:16;		//+32
	unsigned int transBits:8;	//+48
	unsigned int x7:1;
	unsigned int x6:4;
	unsigned int x2:2;
	unsigned int validity:1;
};


/*------------------------------------------------------------------------------
	C S t o r e O b j R e f
	Was TObjRef.
	Gathers everything you need to know about an object on store:
		its header
		its address
		its directory entry address
------------------------------------------------------------------------------*/
extern UByte gObjectStateToTransBits[15];
extern UByte gObjectTransBitsToState[128];

class CStoreObjRef
{
public:
					CStoreObjRef(ULong inVirginBits, CFlashStore * inStore);

	CStoreObjRef &	operator=(const CStoreObjRef& inObj);
	NewtonErr	clone(int, CStoreObjRef& inObj, bool);
	NewtonErr	copyTo(CStoreObjRef & inObj, size_t inOffset, size_t inLen);
	NewtonErr	set(ZAddr inObjectAddr, ZAddr inDirEntryAddr);

	ULong			findSuperceeded(CStoreObjRef& inObj);
	ULong			findSuperceeder(CStoreObjRef& inObj);

	NewtonErr	setSeparateTransaction(void);
	NewtonErr	clearSeparateTransaction(void);

	NewtonErr	rewriteObjHeader(void);
	ZAddr			getDirEntryOffset(void);

	NewtonErr	read(void * outBuf, size_t inOffset, size_t inLen);
	NewtonErr	write(void * inBuf, size_t inOffset, size_t inLen);
	NewtonErr	dlete(void);

	NewtonErr	setState(int inState);
	NewtonErr	setCommittedState(void);

	unsigned int	transBits(void) const;
	unsigned int	state(void) const;

private:
	friend class CFlashStore;
	friend class CFlashBlock;
	friend class CFlashIterator;
	friend class CFlashStoreLookupCache;

	NewtonErr	cloneEmpty(int, size_t, CStoreObjRef&, bool);
	NewtonErr	cloneEmpty(int, CStoreObjRef&, bool);

	StoreObjHeader		fObj;					// +00
	ZAddr					fObjectAddr;		//	+08
	ZAddr					fDirEntryAddr;		// +0C
	CFlashStore *		fStore;				// +10
	CStoreObjRef *		fPrev;				// +14
	CStoreObjRef *		fNext;				// +18
};


inline CStoreObjRef::CStoreObjRef(ULong inVirginBits, CFlashStore * inStore)
{
	fObj.id = inVirginBits;
	fStore = inStore;
}


#endif	/* __STOREOBJREF_H */
