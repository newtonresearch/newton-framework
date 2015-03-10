/*
	File:		Relocation.h

	Contains:	Frames relocation declarations.

	Written by:	Newton Research Group, 2010.
*/

#if !defined(__RELOCATION_H)
#define __RELOCATION_H 1

#include "PackageParts.h"
#include "Store.h"


/*------------------------------------------------------------------------------
	C C R e l o c a t i o n G e n e r a t o r
------------------------------------------------------------------------------*/
struct CRelocationHeader
{
	ULong		version;			// +00
	UShort	numOfEntries;	// +04
	ULong		x08;
	ULong		baseAddr;		// +0C
};


class CCRelocationGenerator
{
public:
					CCRelocationGenerator();
					~CCRelocationGenerator();

	NewtonErr	init(RelocationHeader * ioHeader, RelocationEntry * ioEntries);
	void			getRelocDataForBlock(ULong inBlock, char ** outData, long * outSize, char ** outArg4);
	ULong			getRelocDataSizeForBlock(ULong inBlock);

private:
	RelocationHeader *	f00;
	RelocationEntry *		f04;
	long		f08;
	UShort	f0C;
	ULong	f10;
	VAddr	f14;
// size +18
};


/*------------------------------------------------------------------------------
	C C R e l o c a t o r
	Base class.
------------------------------------------------------------------------------*/

class CCRelocator
{
public:
	virtual ULong		getTheNextRelocEntry(void) = 0;
	virtual NewtonErr	relocate(char * inBuf, VAddr inBaseAddr) = 0;
};


/*------------------------------------------------------------------------------
	C S i m p l e C R e l o c a t o r
	CXIPCRelocator has the same virtual methods.
------------------------------------------------------------------------------*/
#define kReloInfoVersionGrokd  0
#define kReloOffsetArraySize 280

class CSimpleCRelocator : public CCRelocator
{
public:
					CSimpleCRelocator();
	virtual		~CSimpleCRelocator();

	NewtonErr			init(CStore * inStore, PSSId inId, size_t * outReloInfoSize);
	virtual ULong		getTheNextRelocEntry(void);
	virtual NewtonErr	relocate(char * inBuf, VAddr inBaseAddr);

	ULong		fSize;					// +04 size of relocation info
	ULong		fIndex;					// +08 index into relocation offsets
	CRelocationHeader fReloInfo;	// +0C relocation header
	UByte		fReloIndex[kReloOffsetArraySize];	// +1C indices of words in page that need relocating
// size +134
};


/*------------------------------------------------------------------------------
	C F r a m e R e l o c a t i o n G e n e r a t o r
------------------------------------------------------------------------------*/

struct FrameRelocationHeader
{
	uint32_t	first10:10;
	uint32_t	 next10:10;
	uint32_t	flags11:1;
	uint32_t	flags8:3;
	uint32_t	flags:8;
};


class CFrameRelocationGenerator
{
public:
		CFrameRelocationGenerator();
		CFrameRelocationGenerator(int inArg1);

	void	getHeader(FrameRelocationHeader * outHeader);
	void	update(long inArg1, char * inArg2, long inArg3, bool inArg4);

private:
	long	f00;
	long	f04;
	long	f08;
	long	f0C;
	long	f10;
	bool	f14;
	bool	f15;
	bool	f16;
	bool	f17;
// size +18
};


/*------------------------------------------------------------------------------
	F u n c t i o n   P r o t o t y p e s
------------------------------------------------------------------------------*/

extern void RelocateFramesInPage(FrameRelocationHeader * inReloc, char * inPage, VAddr inAddr);


#endif	/* __RELOCATION_H */
