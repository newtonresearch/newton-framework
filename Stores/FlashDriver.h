/*
	File:		FlashDriver.h

	Contains:	Newton flash memory interface.

	Written by:	Newton Research Group, 2010.
*/

#if !defined(__FLASHDRIVER_H)
#define __FLASHDRIVER_H 1

#include "FlashRange.h"

/*------------------------------------------------------------------------------
	C F l a s h D r i v e r
	It’s actually a protocol; but life’s too short.
------------------------------------------------------------------------------*/

class CFlashDriver
{
public:
	virtual NewtonErr	init(CMemoryAllocator&);
	void			initializeDriverData(CFlashRange&, CMemoryAllocator&);

	virtual bool		identify(VAddr, ULong, SFlashChipInformation&);
	void			cleanUp(CMemoryAllocator&);
	void			cleanUpDriverData(CFlashRange&, CMemoryAllocator&);

	void			startReadingArray(CFlashRange&);
	void			doneReadingArray(CFlashRange&);

	void			resetBlockStatus(CFlashRange&, ULong);
	virtual NewtonErr	startErase(CFlashRange&, PAddr);
	bool			isEraseComplete(CFlashRange&, ULong, long&);

	void			lockBlock(CFlashRange&, ULong);

	void			beginWrite(CFlashRange&, ULong, ULong);
	virtual void		write(ULong, ULong, PAddr, CFlashRange&);
	NewtonErr	reportWriteResult(CFlashRange&, ULong);
};


class CPseudoFlashDriver : public CFlashDriver
{
public:
	virtual NewtonErr	init(CMemoryAllocator&);
	virtual bool		identify(VAddr, ULong, SFlashChipInformation&);
	virtual NewtonErr	startErase(CFlashRange&, VAddr);
	virtual void		write(ULong, ULong, VAddr, CFlashRange&);
private:
	int			nothing;
};


#endif	/* __FLASHDRIVER_H */
