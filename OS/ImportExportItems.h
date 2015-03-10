/*
	File:		ImportExportItems.h

	Contains:	ROM extension and Magic Pointer import/export item delarations.

	Written by:	Newton Research Group, 2010.
*/

#include "Objects.h"


/* -----------------------------------------------------------------------------
	M a g i c   P o i n t e r   I t e m s
----------------------------------------------------------------------------- */

struct MPExportItem
{
	void *		f00;
	ULong			numOfObjects;		// +04
	Ref *			f08;					// +08
	bool			f0C;
	Ref *			objects;				// +10
	int			majorVersion;		// +14
	int			minorVersion;		// +18
	ULong			refCount;			// +1C
	char			name[];				// +20
};


struct MPImportItem
{
	VAddr			f00;
	void *		f04;
	void *		f08;
	Ref			f0C;
	ULong			f10;
	Ref			f14[];
// size+14
};


struct MPPendingImportItem
{
	MPPendingImportItem * next;	// +04
	int			f08;
	UniChar *	name;					// +0C
	int			majorVersion;		// +10
	int			minorVersion;		// +14
	VAddr			f18;
};



/* -----------------------------------------------------------------------------
	R O M   E x t e n s i o n   I t e m s
----------------------------------------------------------------------------- */

struct RExImport
{
	int			majorVersion;		// +00
	int			minorVersion;		// +04
	int			f08;
	ULong			count;				// +0C
	char			name[];				// +10
};


class RExPendingImport
{
public:
						RExPendingImport()	{ }

	virtual void	fulfill(MPExportItem * inItem);
	virtual int		match(void * inItem);

//private:
	RExPendingImport *	f04;
	int			f08;
	char *		f0C;
	int			f10;
	int			f14;
	RExImport *	f18;
	Ref *			f1C;
};

