/*
	File:		List.h

	Contains:	Interface to the CList class

	Written by:	Newton Research Group.
*/

#if !defined(__LIST_H)
#define __LIST_H 1

#if !defined(__DYNAMICARRAY_H)
#include "DynamicArray.h"
#endif

class CItemTester;
class CItemComparer;

/*--------------------------------------------------------------------------------
	C L i s t
--------------------------------------------------------------------------------*/

class CList : public CDynamicArray
{
public:

	static CList *	make();
	static CList *	make(ArrayIndex inSize);

					CList();
					~CList();

	// get

	void *		at(ArrayIndex index);
	void *		first(void);
	void *		last(void);

	// insertion

	NewtonErr	insert(void * inItem);
	bool			insertUnique(void * inItem);
	NewtonErr	insertBefore(ArrayIndex index, void * inItem);
	NewtonErr	insertAt(ArrayIndex index, void * inItem);
	NewtonErr	insertFirst(void * inItem);
	NewtonErr	insertLast(void * inItem);

	// removal

	NewtonErr	remove(void * inItem);
	NewtonErr	removeAt(ArrayIndex index);
	NewtonErr	removeFirst(void);
	NewtonErr	removeLast(void);

	// replacement

	NewtonErr	replace(void * inOldItem, void * inNewItem);
	NewtonErr	replaceAt(ArrayIndex index, void * inNewItem);
	NewtonErr	replaceFirst(void * inNewItem);
	NewtonErr	replaceLast(void * inNewItem);

	// indexing

	ArrayIndex	getIdentityIndex(void * inItem);
	ArrayIndex	getEqualityIndex(void * inItem);

	// searching

	void *		search(CItemTester * inTest, ArrayIndex & outIndex);
	bool		 	contains(void * inItem);
/*
	// old names from TList
	long			count()					{ return fSize; };
	bool			empty()					{ return (fSize == 0);};
	bool			addUnique(void* add)	{ return insertUnique(add);}
	ArrayIndex	index(void* item)		{ return getIdentityIndex(item);}
	void *		ith(ArrayIndex index)	{ return at(index);}
*/
};


/*--------------------------------------------------------------------------------
	C L i s t   I n l i n e s
--------------------------------------------------------------------------------*/

inline void * CList::first(void)
{ return at(0); }

inline void * CList::last(void)
{ return at(fSize - 1); }

inline NewtonErr CList::insert(void * inItem)
{ return insertAt(fSize, inItem); }

inline NewtonErr CList::insertBefore(ArrayIndex index, void * inItem)
{ return insertAt(index, inItem); }

inline NewtonErr CList::insertFirst(void * inItem)
{ return insertAt(0, inItem); }

inline NewtonErr CList::insertLast(void * inItem)
{ return insertAt(fSize, inItem); }

inline NewtonErr CList::removeAt(ArrayIndex index)
{ return removeElementsAt(index, 1); }

inline NewtonErr CList::removeFirst(void)
{ return removeElementsAt(0, 1); }

inline NewtonErr CList::removeLast(void)
{ return removeElementsAt(fSize - 1, 1); }

inline NewtonErr CList::replaceFirst(void * inNewItem)
{ return replaceAt(0, inNewItem); }

inline NewtonErr CList::replaceLast(void * inNewItem)
{ return replaceAt(fSize - 1, inNewItem); }

inline ArrayIndex CList::getEqualityIndex(void * inItem)
{ return getIdentityIndex(inItem); }

inline bool		 	CList::contains(void * inItem)
{ return getIdentityIndex(inItem) != kIndexNotFound; }


/*--------------------------------------------------------------------------------
	C S o r t e d L i s t
--------------------------------------------------------------------------------*/

class CSortedList : public CList
{
public:
					CSortedList(CItemComparer * inComparer);
					~CSortedList();

	NewtonErr	insert(void * inItem);
	bool			insertUnique(void * inItem);
	NewtonErr	insertDuplicate(ArrayIndex index, void * inItem, void * inItem2);

	void *		search(CItemTester * inTest, ArrayIndex & outIndex);

private:
	CItemComparer *	fComparer;
};


#endif	/*	__LIST_H	*/
