/*
	File:		Stores.h

	Contains:	Store declarations.
	
	Written by:	Newton Research Group.
*/

#if !defined(__STORES_H)
#define __STORES_H 1

#include "StoreWrapper.h"

class CSortingTable;

struct StoreRootData
{
	ULong		signature;
	ULong		version;
	PSSId		mapTableId;
	PSSId		symbolTableId;
	PSSId		permId;
	PSSId		ephemeralsId;
};


Ref	MakeStoreObject(CStore * inStore);
void	KillStoreObject(RefArg inStoreObj);

void	CheckWriteProtect(CStore * inStore);

Ref	FGetCardSlotStores(void);

Ref	StoreGetPasswordKey(CStore * inStore);
bool	CheckStorePassword(CStore * inStore, RefArg inPassword);

Ref	StoreConvertSoupSortTables(void);
const CSortingTable *	StoreGetDirSortTable(RefArg);
bool	StoreHasSortTables(RefArg);
void	StoreSaveSortTable(RefArg, long);
void	StoreRemoveSortTable(RefArg, long);

NewtonErr	GetStoreVersion(CStore * inStore, ULong * outVersion);
NewtonErr	SetStoreVersion(RefArg ioStoreObject, ULong inVersion);


#endif	/* __STORES_H */
