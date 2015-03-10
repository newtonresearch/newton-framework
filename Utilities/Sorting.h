/*
	File:		Sorting.h

	Contains:	Unicode sorting functions.

	Written by:	Newton Research Group.
*/

#if !defined(__SORTING_H)
#define __SORTING_H 1

#include "Unicode.h"

/*------------------------------------------------------------------------------
	C S o r t i n g T a b l e
------------------------------------------------------------------------------*/

typedef struct
{
	UniChar  f00;
	UniChar  f02;
}
ProjectionEntry;


typedef struct
{
	UniChar  f00;  // don’t really know
	UniChar  f02;
	UniChar  f04;
}
LigatureEntry;


class CSortingTable
{
public:
	ArrayIndex			calcSize() const;
	void					convertTextToLowestSort(UniChar *, ArrayIndex) const;
	ProjectionEntry *	getProjectionEntry(UniChar) const;
	LigatureEntry *	getLigatureEntry(UniChar) const;

	short fId;  // +00
	short f06;
	short f08;
	short f20;
	short f24;
	short f26;
	short f2A;
	int  f44;
};


/*------------------------------------------------------------------------------
	C S o r t T a b l e s
------------------------------------------------------------------------------*/

class CSortTables
{
	struct TableEntry
	{
		const CSortingTable *	fSortingTable;		// +00
		bool							fOwnedByUs;			// +04
		int							fRefCount;			// +08
	};

public:
	bool							setDefaultTableId(int inTableId);
	bool							addSortTable(const CSortingTable * inTable, bool);
	const CSortingTable *	getSortTable(int inTableId, size_t * outSize) const;
	TableEntry *				getTableEntry(int inTableId) const;
	void							subscribe(int inTableId);
	void							unsubscribe(int inTableId);

//private:
	TableEntry					fEntry[kNumOfEncodings];	// +00
	int							fDefaultTableId;  // +3C
	const CSortingTable *	fDefaultTable;		// +40
};


#endif	/* __SORTING_H */
