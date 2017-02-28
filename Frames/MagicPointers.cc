/*
	File:			MagicPointers.c

	Contains:	Newton object system.
					Magic pointer interface.

	Written by:	Newton Research Group.
*/

#include "ImportExportItems.h"
#include "ROMExtension.h"
#include "List.h"
#include "ListIterator.h"
#include "ROMResources.h"
#include "OSErrors.h"


extern CSortedList *			gMPExportList;
extern CSortedList *			gMPImportList;
extern RExPendingImport *	gMPPendingImports;


/*----------------------------------------------------------------------
	F u n c t i o n   P r o t o t y p e s
----------------------------------------------------------------------*/

extern void		InitRExMagicPointerTables(void);
static void		PatchMagicPointerTable(void);

extern Ref		FramePartTopLevelFrame(void * inPart);
extern Ref		GetEntryFromLargeObjectVAddr(VAddr inAddr);
extern void		FlushPackageCache(VAddr inAddr);

void				FulfillPendingImports(MPExportItem * inItem, Ref inArg2);


/*----------------------------------------------------------------------
	Initialize the magic pointer tables.
	Args:		void
	Return:	void
----------------------------------------------------------------------*/

void
InitMagicPointerTables(void)
{
#if !defined(forFramework)
	InitRExMagicPointerTables();
#endif
	PatchMagicPointerTable();
}


void
PatchMagicPointerTable(void)
{ /* this really does nothing */ }


#if !defined(forFramework)

#include "ItemComparer.h"

class CMPExportListNameComparer : public CItemComparer
{
public:
	virtual CompareResult	testItem(const void * inItem) const;
};

CompareResult
CMPExportListNameComparer::testItem(const void * inItem) const
{
	return (CompareResult) symcmp(((MPExportItem *)fItem)->name, ((MPExportItem *)inItem)->name);
}


class CMPImportListSourceComparer : public CItemComparer
{
public:
	virtual CompareResult	testItem(const void * inItem) const;
};

CompareResult
CMPImportListSourceComparer::testItem(const void * inItem) const
{
	if (((MPExportItem *)fItem)->numOfObjects < ((MPExportItem *)inItem)->numOfObjects)
		return kItemLessThanCriteria;
	else if (((MPExportItem *)fItem)->numOfObjects > ((MPExportItem *)inItem)->numOfObjects)
		return kItemGreaterThanCriteria;
	else
		return kItemEqualCriteria;
}


class CMPImportListSourceTester : public CItemComparer
{
public:
	virtual CompareResult	testItem(const void * inItem) const;
};

CompareResult
CMPImportListSourceTester::testItem(const void * inItem) const
{
	if (((MPExportItem *)fItem)->numOfObjects < ((MPExportItem *)inItem)->numOfObjects)
		return kItemLessThanCriteria;
	else if (((MPExportItem *)fItem)->numOfObjects > ((MPExportItem *)inItem)->numOfObjects)
		return kItemGreaterThanCriteria;
	else
		return kItemEqualCriteria;
}


void
InitMPTableRegistry(void)
{
	CMPExportListNameComparer * exportCmp = new CMPExportListNameComparer;
	THROWNOT(exportCmp, exOutOfMemory, kOSErrNoMemory);
	gMPExportList = new CSortedList(exportCmp);
	THROWNOT(gMPExportList, exOutOfMemory, kOSErrNoMemory);

	CMPImportListSourceComparer * importCmp = new CMPImportListSourceComparer;
	THROWNOT(importCmp, exOutOfMemory, kOSErrNoMemory);
	gMPImportList = new CSortedList(importCmp);
	THROWNOT(gMPImportList, exOutOfMemory, kOSErrNoMemory);	
}


void
InstallExportTables(RefArg inTable, void * inSource)
{
	if (gMPExportList == NULL)
		InitMPTableRegistry();

	RefVar item;
	for (ArrayIndex i = 0, count = Length(inTable); i < count; ++i)
	{
		item = GetArraySlot(inTable, i);
		const char * name = SymbolName(GetFrameSlot(item, SYMA(name)));
		MPExportItem * exportItem = (MPExportItem *)NewPtr(sizeof(MPExportItem) + strlen(name) + 1);
		THROWNOT(exportItem, exOutOfMemory, kOSErrNoMemory);	
		newton_try
		{
			strcpy(exportItem->name, name);
			exportItem->f00 = inSource;
			exportItem->majorVersion = RINT(GetFrameSlot(item, SYMA(major)));
			exportItem->minorVersion = RINT(GetFrameSlot(item, SYMA(minor)));
			exportItem->refCount = 0;

			RefVar objects(GetFrameSlot(item, SYMA(objects)));
			exportItem->numOfObjects = Length(objects);
			exportItem->objects = exportItem->f08 = Slots(objects);
			exportItem->f0C = false;

			THROWNOT(gMPExportList->insert(exportItem) == noErr, exOutOfMemory, kOSErrNoMemory);	
			FulfillPendingImports(exportItem, NILREF);
		}
		cleanup
		{
			FreePtr((Ptr)exportItem);
		}
		end_try;
	}
}


Ref
RemoveExportTables(void * inSource)
{
	RefVar sp00(MakeArray(0));
	CSortedList * sp1C = gMPExportList;
	// INCOMPLETE
	return NILREF;
}


void
InstallImportTable(VAddr inArg1, RefArg inTable, void * inArg3, long inArg4)
{
	if (gMPImportList == NULL)
		InitMPTableRegistry();

	ArrayIndex count = Length(inTable);	// sp04
	MPImportItem * importItem = (MPImportItem *)NewPtr(sizeof(MPImportItem) + count * sizeof(Ref));
	THROWNOT(importItem, exOutOfMemory, kOSErrNoMemory);	
#if defined(correct)
	importItem->f00 = inArg1;
	importItem->f04 = inArg3;
	importItem->f08 = inArg3 + inArg4;
	importItem->f0C = inTable;
	importItem->f10 = count;

	newton_try
	{
		RefVar item;	// r6
		for (ArrayIndex i = 0; i < count; ++i)
		{
			item = GetArraySlot(inTable, i);
			//sp-0C
			char * r10 = SymbolName(GetFrameSlot(item, SYMA(name)));
			int sp08 = RINT(GetFrameSlot(item, SYMA(major)));
			int sp04 = RINT(GetFrameSlot(item, SYMA(minor)));
			r4 = 0;
			r9 = -1;
			// INCOMPLETE
		}
		THROWNOT(sp0C->insertUnique(importItem) == noErr, exOutOfMemory, kOSErrNoMemory);	
	}
	cleanup
	{
		FreePtr((Ptr)importItem);
	}
	end_try;
#endif
}


void
RemoveImportTable(void * inSource)
{
#if defined(correct)
	RemovePendingImports(inSource);
	if (gMPImportList != nl)
	{
		CMPImportListSourceTester cmp(inSource);
		ArrayIndex index = 0;
		MPImportItem * importItem;	// r5
		while ((importItem = (MPImportItem *)gMPImportList->search(&cmp, &index)) != NULL)
		{
			for (ArrayIndex i = 0; i < importItem->f10; ++i)
			{
				if (importItem->f14[i] != 0)
				{
					MPExportItem * exportItem = *(MPExportItem *)(importItem->f14[i] - 1);
					exportItem->refCount--;
					if (exportItem->refCount == 0)
						FreeExportTable(exportItem);
				}
			}
		}
	}
#endif
}


// see also RefMemory.cc, ResolveMagicPtr(Ref r)
#define kMPTableShift	12
#define kMPTableMask		0x3FFFF000
#define kMPIndexMask		0x00000FFF

void
ResolveImportRef(Ref * ioRefPtr, void ** outArg2)
{
//r4,r5
	// we assume the *ioRefPtr IS a magic pointer
	ULong rValue = *ioRefPtr >> kRefTagBits;		// sp00
	ArrayIndex table = rValue >> kMPTableShift;	// r0

	if (table != 0 && table != 1)
	{
#if defined(correct)
		// itÕs neither MPs nor globals -- must be import refs
		int r8 = table-2;
		r6 = *outArg2;
		if (r6 == NULL)
		{
			;
		}

		if (r6 != NULL)
		{
			*outArg2 = r6;
			if (r6->f10 > r8
			&&  xx != 0
			&&  sp00.low12bits < )
				*ioRefPtr |= 0x80000000;
			else
				*ioRefPtr |= 0x80000000;
		}
		*ioRefPtr |= 0x80000000;
#endif
	}
}


void
FulfillPendingImports(MPExportItem * inItem, Ref inArg2)
{
}


extern "C" {
Ref	FCurrentExports(RefArg rcvr);
Ref	FCurrentImports(RefArg rcvr);
Ref	FPendingImports(RefArg rcvr);
Ref	FFlushImports(RefArg rcvr);
Ref	FGetExportTableClients(RefArg rcvr);
Ref	FFulfillImportTable(RefArg rcvr);
};


Ref
FCurrentExports(RefArg rcvr)
{
	ArrayIndex count = gMPExportList ? gMPExportList->count() : 0;
	RefVar exports(MakeArray(count));
	RefVar item;
	for (ArrayIndex i = 0; i < count; ++i)
	{
		MPExportItem * src = (MPExportItem *)gMPExportList->at(i);
		item = Clone(RA(canonicalCurrentExport));
		SetFrameSlot(item, SYMA(name), MakeSymbol(src->name));
		SetFrameSlot(item, SYMA(major), MAKEINT(src->majorVersion));
		SetFrameSlot(item, SYMA(minor), MAKEINT(src->minorVersion));
		SetFrameSlot(item, SYMA(refCount), MAKEINT(src->refCount));
//		SetFrameSlot(item, SYMA(exportTable), REF(src->objects - SIZEOF_ARRAYOBJECT(0)));
		SetArraySlot(exports, i, item);
	}
	return exports;
}


Ref
FCurrentImports(RefArg rcvr)
{
	ArrayIndex count = gMPImportList ? gMPImportList->count() : 0;
	RefVar imports(MakeArray(count));
	RefVar item;
	for (ArrayIndex i = 0; i < count; ++i)
	{
		MPImportItem * src = (MPImportItem *)gMPImportList->at(i);
		item = Clone(RA(canonicalCurrentImport));
		SetFrameSlot(item, SYMA(ImportTable), src->f0C);
		SetFrameSlot(item, SYMA(client), GetEntryFromLargeObjectVAddr((VAddr)src->f00));
		SetArraySlot(imports, i, item);
	}
	return imports;
}


Ref
FPendingImports(RefArg rcvr)
{
	RefVar imports(MakeArray(0));
	RefVar item;
	MPPendingImportItem * src;	// canÕt be right?
	for (src = (MPPendingImportItem *)gMPPendingImports; src != NULL; src = (MPPendingImportItem *)src->next)
	{
		item = Clone(RA(canonicalPendingImport));
		SetFrameSlot(item, SYMA(name), MakeString(src->name));
		SetFrameSlot(item, SYMA(major), MAKEINT(src->majorVersion));
		SetFrameSlot(item, SYMA(minor), MAKEINT(src->minorVersion));
		if (src->f08 == 0)
			SetFrameSlot(item, SYMA(client), GetEntryFromLargeObjectVAddr((VAddr)src->f18));
		AddArraySlot(imports, item);
	}
	return imports;
}


Ref
FFlushImports(RefArg rcvr)
{
	for (ArrayIndex i = 0, count = gMPImportList ? gMPImportList->count() : 0; i < count; ++i)
	{
		MPImportItem * src = (MPImportItem *)gMPImportList->at(i);
		FlushPackageCache(src->f00);
	}
	return NILREF;
}


Ref
FGetExportTableClients(RefArg rcvr, RefArg inArg2)
{
	RefVar clients(MakeArray(0));
	RefVar item, clientItem, aClient;
	Ref * sp2C = Slots(inArg2);
	CListIterator iter(gMPExportList);
	MPExportItem * exportItem;
	for (exportItem = (MPExportItem *)iter.firstItem(); iter.more(); exportItem = (MPExportItem *)iter.nextItem())
	{
		if (exportItem->objects == sp2C)
		{
			int r10 = NULL;//&exportItem->f04;
			for (ArrayIndex i = 0, count = gMPImportList->count(); i < count; ++i)
			{
				MPImportItem * importItem = (MPImportItem *)gMPImportList->at(i);
				for (ArrayIndex j = 0; j < importItem->f10; ++j)
				{
					if (importItem->f14[j] == r10)
					{
						aClient = FramePartTopLevelFrame(importItem->f04);
						if (NOTNIL(aClient))
						{
							item = GetArraySlot(importItem->f0C, j);
							clientItem = Clone(RA(canonicalExportTableClient));
							SetFrameSlot(clientItem, SYMA(name), GetFrameSlot(item, SYMA(name)));
							SetFrameSlot(clientItem, SYMA(major), GetFrameSlot(item, SYMA(major)));
							SetFrameSlot(clientItem, SYMA(minor), GetFrameSlot(item, SYMA(minor)));
							SetFrameSlot(clientItem, SYMA(client), aClient);
							AddArraySlot(clients, clientItem);
						}
					}
				}
			}
		}
	}
	return clients;
}


Ref
FFulfillImportTable(RefArg rcvr)
{
	if (gMPExportList != NULL)
	{
		CListIterator iter(gMPExportList);
		MPExportItem * item;
		for (item = (MPExportItem *)iter.firstItem(); iter.more(); item = (MPExportItem *)iter.nextItem())
		{
			FulfillPendingImports(item, rcvr);
		}
		return TRUEREF;
	}
	return NILREF;
}


#endif
