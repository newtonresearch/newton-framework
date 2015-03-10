/*
	File:		PrecedentsForIO.h

	Contains:	Precedents classesfor reading and writing.
	
	Written by:	Newton Research Group.
*/

#if !defined(__PRECEDENTSFORIO_H)
#define __PRECEDENTSFORIO_H 1

/*----------------------------------------------------------------------
	C B u c k e t A r r a y
----------------------------------------------------------------------*/

#define kBucketSize	64

class CBucketArray
{
public:
					CBucketArray(ArrayIndex inElementSize);
					~CBucketArray();

	void *		elementAt(ArrayIndex index);
	ArrayIndex	count(void) const;				// was getArraySize()
	void			setArraySize(ArrayIndex inSize);

private:
	ArrayIndex	fElementSize;		// +00
	ArrayIndex	fNumOfElements;	// +04
	ArrayIndex	fNumOfBuckets;		// +08
	void **		fBucketBlock;		// +0C
};

inline	ArrayIndex		CBucketArray::count(void) const
{ return fNumOfElements; }


/*----------------------------------------------------------------------
	C P r e c e d e n t s F o r R e a d i n g
----------------------------------------------------------------------*/

class CPrecedentsForReading : public CBucketArray
{
public:
					CPrecedentsForReading();
					~CPrecedentsForReading();

	ArrayIndex	add(RefArg inObj);
	Ref *			get(ArrayIndex index);
	void			replace(ArrayIndex index, RefArg inObj);
	void			reset(void);

private:
	void			markAllRefs(void);
	void			updateAllRefs(void);

	static void		GCMark(void * inContext);
	static void		GCUpdate(void * inContext);
};

inline	Ref *		CPrecedentsForReading::get(ArrayIndex index)
{ return (Ref *)elementAt(index); }


/*----------------------------------------------------------------------
	C P r e c e d e n t s F o r W r i t i n g
----------------------------------------------------------------------*/

struct WrPrec
{
	Ref		ref;
	unsigned	f04;					// offset to lower
	unsigned	highestBit :  8;	// +08
	unsigned	f09 : 24;			// offset to higher
};

class CPrecedentsForWriting : public CBucketArray
{
public:
					CPrecedentsForWriting();
					~CPrecedentsForWriting();

	ArrayIndex	add(RefArg inObj);
	WrPrec *		get(ArrayIndex index);
	void			reset(void);

	ArrayIndex	search(RefArg inObj);
	ArrayIndex	find(RefArg inObj);
	void			generateLinks(ArrayIndex index);
	void			rebuildTable(void);

private:
	void			markAllRefs(void);
	void			updateAllRefs(void);

	static void		GCOccurred(void * inContext);
	static void		GCMark(void * inContext);
	static void		GCUpdate(void * inContext);
};

inline	WrPrec *	CPrecedentsForWriting::get(ArrayIndex index)
{ return (WrPrec *)elementAt(index); }


extern CPrecedentsForWriting *	gPrecedentsForWriting;
extern CPrecedentsForReading *	gPrecedentsForReading;


#endif	/* __PRECEDENTSFORIO_H */
