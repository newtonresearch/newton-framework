/*
	File:		FlashIterator.h

	Contains:	Flash iterator declarations.
	
	Written by:	Newton Research Group, 2010.
*/

#if !defined(__FLASHITERATOR_H)
#define __FLASHITERATOR_H 1

#include "FlashBlock.h"
#include "FlashTracker.h"

enum IterFilterType
{
	kIterFilterType0,
	kIterFilterType1,
	kIterFilterType2,
	kIterFilterType3,
	kIterFilterType4,
	kIterFilterType5
};


/*------------------------------------------------------------------------------
	C F l a s h I t e r a t o r
------------------------------------------------------------------------------*/

class CFlashIterator
{
public:
					CFlashIterator(CFlashStore * inStore, CStoreObjRef * inObj, IterFilterType inFilter);
					CFlashIterator(CFlashStore * inStore, CStoreObjRef * inObj, ZAddr inArg3, IterFilterType inFilter);
					CFlashIterator(CFlashStore * inStore, CStoreObjRef * inObj, CFlashBlock * inBlock, IterFilterType inFilter);
					CFlashIterator(CFlashStore * inStore, SDirEnt * inDirEnt, ZAddr inArg3);

	void			start(IterFilterType inFilter);
	void			start(ZAddr inAddr, IterFilterType inFilter);
	void			start(CFlashTracker * inTracker);

	NewtonErr	lookup(PSSId, int, int*);
	SDirEnt		getDirEnt(ZAddr);
	SDirEnt		readDirBucket(ZAddr);
	ArrayIndex	countUnusedDirEnt(void);

	bool			done(void);
	CStoreObjRef *	next(void);
	void			reset(void);

	ULong			getf28(void) const;

	void			print(void);				// DEBUG

private:
	void			probe(void);

	CStoreObjRef *		fCurrentObj;		// +00
	CFlashStore *		fStore;				// +04
	SDirEnt *			fDirEnt;				// +08
	ArrayIndex			fNumOfFreeDirEnts;	// +0C
	IterFilterType		fFilterType;		//+10
	ZAddr					fRootDirEntAddr;	//+14
	int					fState;				// +18
	long					f1C;
	CFlashTracker *	fTracker;			//+20
	ZAddr					fObjectAddr;		//+24
	ZAddr					f28;
	int					fIndex;				//+2C
	long					f30;
	ZAddr					fDirBucketAddr;	// +34
	SDirEnt				fDirBucket[kBucketSize];	// +38
// size +78
};

inline ULong			CFlashIterator::getf28(void) const { return f28; }


#endif	/* __FLASHITERATOR_H */
