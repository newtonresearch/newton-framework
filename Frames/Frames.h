/*
	File:		Frames.h

	Contains:	Frame declarations.

	Written by:	Newton Research Group.
*/

#if !defined(__FRAMES__)
#define __FRAMES__ 1

#include "ObjHeader.h"

//	useful symbol hash values
#define k_protoHash  0x6622439B
#define k_parentHash 0xC5D5F0A1


void			FindOffsetCacheClear(void);
ArrayIndex	FindOffset(Ref frame, Ref tag);

ArrayIndex	FrameSlotPosition(Ref frame, Ref tag);
ArrayIndex	AddSlot(RefArg frame, RefArg tag);

Ref	AllocateFrameWithMap(RefArg theMap);
Ref	AllocateMap(RefArg superMap, ArrayIndex numOfTags);
Ref	AllocateMapWithTags(RefArg superMap, RefArg mapFrame);

Ref	UnsafeGetFrameSlot(Ref frame, Ref tag, bool * exists);

Ref	SharedFrameMap(RefArg inFrame);
ArrayIndex	ComputeMapSize(Ref inMap);
Ref	ExtendSharedMap(RefArg inMap, ArrayIndex byThisMuch);
Ref	ShrinkSharedMap(RefArg inMap, RefArg inImplMap, ArrayIndex index);

struct SortedMapTag
{
	Ref	ref;
	int	index;
};

Ref	GetTag(Ref inFrame, ArrayIndex index, ArrayIndex * mapIndex = NULL);
void	GetFrameMapTags(Ref inFrame, SortedMapTag * outTags, bool inSort);


/*----------------------------------------------------------------------
	O b j e c t C a c h e
----------------------------------------------------------------------*/

struct OffsetCacheItem
{
	Ref			context;
	Ref			slot;
	ArrayIndex	offset;
};


struct ObjectCache
{
	Ref					ref;
	ObjHeader *			ptr;
	Ref					lenRef;
	ArrayIndex			length;
	OffsetCacheItem *	offset;
};

extern ObjectCache gCached;	// 0C10554C

#endif	/* __FRAMES__ */
