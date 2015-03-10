/*
	File:		StoreDriver.cc

	Contains:	SRAM store driver implementation.
					The original Newton memory hardware added an extra chip giving
					non-contiguous access to bytes within long words.
					The store driver stitches these memory blocks together.
	
	Written by:	Newton Research Group, 2010.
*/

#include "StoreDriver.h"


/*------------------------------------------------------------------------------
	C S t o r e D r i v e r
------------------------------------------------------------------------------*/

void
CStoreDriver::init(char * inStoreBase, size_t inStoreSize, char * inMemBase, ULong inArg4)
{
	fStoreBase = inStoreBase;
	fStoreSize = inStoreSize;
	fMemBase = inMemBase;
	f0C = inArg4;
	f10 = inStoreSize + inArg4;
}


char *
CStoreDriver::addressOf(ULong inAddr, bool * outIsContiguous)
{
	char * p;
	if (inAddr < f0C)
	{
		p = (fMemBase + (inAddr & 0x0001FFFF) * sizeof(long)) + ((inAddr >> 17) ^ 3);
		if (outIsContiguous)
			*outIsContiguous = NO;
	}
	else
	{
		p = fStoreBase + (inAddr - f0C);
		if (outIsContiguous)
			*outIsContiguous = YES;
	}
	return p;
}


char *
CStoreDriver::addressOfAligned(ULong inAddr, bool * outIsContiguous)
{
	char * p;
	if (inAddr < f0C)
	{
		p = (fMemBase + (inAddr & 0x0001FFFF) * sizeof(long));
		if (outIsContiguous)
			*outIsContiguous = NO;
	}
	else
	{
		p = fStoreBase + (inAddr - f0C);
		if (outIsContiguous)
			*outIsContiguous = YES;
	}
	return p;
}


void
CStoreDriver::copy(ULong inFromAddr, ULong inToAddr, size_t inLen)
{
	bool isContiguousFrom;
	char * pFrom = addressOf(inFromAddr, &isContiguousFrom);

	bool isContiguousTo;
	char * pTo = addressOf(inToAddr, &isContiguousTo);

	ULong delta = inFromAddr > inToAddr ? inFromAddr - inToAddr : inToAddr - inFromAddr;
	if (delta < inLen)
	{
		if (inFromAddr < inToAddr)
		{
			if (isContiguousFrom)
			{
				pFrom += (inLen - 1);
				pTo += (inLen - 1);
				for (ArrayIndex i = 0; i < inLen; ++i)
					*pTo-- = *pFrom--;
			}
			else
			{
				pFrom += (inLen - 4);
				pTo += (inLen - 4);
				for (ArrayIndex i = 0; i < inLen; ++i, pFrom -= 4, pTo -= 4)
					*pTo = *pFrom;
			}
		}
		else
		{
			if (isContiguousFrom)
				for (ArrayIndex i = 0; i < inLen; ++i)
					*pTo++ = *pFrom++;
			else
				for (ArrayIndex i = 0; i < inLen; ++i, pFrom += 4, pTo += 4)
					*pTo = *pFrom;
		}
	}

	else if (isContiguousFrom == isContiguousTo)
	{
		if (isContiguousFrom)
			memmove(pTo, pFrom, inLen);
		else
			for (ArrayIndex i = 0; i < inLen; ++i, pFrom += 4, pTo += 4)
				*pTo = *pFrom;
	}

	else
	{
		if (isContiguousFrom)
			for (ArrayIndex i = 0; i < inLen; ++i, pTo += 4)
				*pTo = *pFrom++;
		else
			for (ArrayIndex i = 0; i < inLen; ++i, pFrom += 4)
				*pTo++ = *pFrom;
	}
}


void
CStoreDriver::persistentCopy(ULong, ULong, ULong)
{}


void
CStoreDriver::continuePersistentCopy(void)
{
	if (f24)
		doPersistentCopy(f14, f18);
}


void
CStoreDriver::doPersistentCopy(ULong, ULong)
{}


void
CStoreDriver::set(VAddr inAddr, size_t inLen, ULong inPattern)
{
	bool isContiguous;
	char * p = addressOf(inAddr, &isContiguous);
	if (isContiguous)
		memset(p, inLen, inPattern);
	else
		for (ArrayIndex i = 0; i < inLen; ++i, p += 4)
			*p = inPattern;
}


void
CStoreDriver::read(char * outBuf, VAddr inAddr, size_t inLen)
{
	bool isContiguous;
	char * p = addressOf(inAddr, &isContiguous);
	if (isContiguous)
		memmove(outBuf, p, inLen);
	else
		for (ArrayIndex i = 0; i < inLen; ++i, p += 4)
			*outBuf++ = *p;
}


void
CStoreDriver::write(char * inBuf, VAddr inAddr, size_t inLen)
{
	bool isContiguous;
	char * p = addressOf(inAddr, &isContiguous);
	if (isContiguous)
		memmove(p, inBuf, inLen);
	else
		for (ArrayIndex i = 0; i < inLen; ++i, p+= 4)
			*p = *inBuf++;
}
