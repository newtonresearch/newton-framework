/*
	File:		RecObject.cc

	Contains:	Recognition object base class implementation.

	Written by:	Newton Research Group.
*/

#include "RecObject.h"

extern void		uitoa(ULong inNum, unsigned char * ioStr);	// Locales.cc


/* -----------------------------------------------------------------------------
	D a t a
------------------------------------------------------------------------------ */

bool gVerbose = YES;		// 0C101160
bool gFileOut = NO;		// 0C104D38	dump-to-file


#pragma mark CMsg
/* -----------------------------------------------------------------------------
	C M s g

	For diagnostic output.
------------------------------------------------------------------------------ */

CMsg::CMsg()
	:	fMem(NULL)
{
	iMsg();
}


CMsg::CMsg(char * inStr)
	:	fMem(NULL)
{
	iMsg();
	msgStr(inStr);
}


CMsg::~CMsg()
{
	if (fMem)
		FreePtr(fMem);
}


CMsg *
CMsg::iMsg(void)
{
	if (fMem == NULL)
		fMem = NewPtr(256);	// was NewHandle
	fLength = 0;
	fCharsInLine = 0;
	fNumOfLines = 0;
	return this;
}


CMsg *
CMsg::msgChar(char inChar)
{
	char	str[2];
	str[0] = inChar;
	str[1] = 0;
	msgStr(str);
	return this;
}


CMsg *
CMsg::msgLF(void)
{
	msgChar('\n');
	return this;
}


CMsg *
CMsg::msgType(RecType inType)
{
	char	str[5];
	*(RecType*)str = CANONICAL_LONG(inType);
	str[4] = 0;
	msgStr(str);
	return this;
}


CMsg *
CMsg::msgNum(ULong inNum, int inWidth)
{
	char	str[32];

	uitoa(inNum, (unsigned char *)str);	// ignores width -- not original, but expedient
	msgStr(str);
	return this;
}


CMsg *
CMsg::msgHex(ULong inHex, int inWidth)
{
	while (inWidth > 8)
		msgChar(' ');

	bool	suppressZeros;
	if (inWidth < 0)
	{
		inWidth = 8;
		suppressZeros = YES;
	}
	else
		suppressZeros = NO;

	for (int shift = inWidth * 4; shift >= 0; shift -=4)
	{
		UChar	nibl = (inHex >> shift) & 0x0F;
		if (!(nibl == 0 && suppressZeros))
		{
			suppressZeros = NO;
			if (nibl > 9)
				nibl += 7;
			msgChar('0' + nibl);
		}
	}
	return this;
}


CMsg *
CMsg::msgStr(const char * inStr)
{
	size_t	strLen = strlen(inStr);
	size_t	newLen = fLength + strLen + 1;
	size_t	prevSize = GetPtrSize(fMem);
	if (newLen > prevSize)
	{
		fMem = ReallocPtr(fMem, ALIGN(newLen, 256));
	}
	while (strLen-- > 0)
	{
		char	ch;
		if ((ch = *inStr++) == '\n')
		{
			fCharsInLine = 0;
			fNumOfLines++;
		}
		fMem[fLength++] = ch;
		fCharsInLine++;
	}
	fMem[fLength] = 0;
	return this;
}


void
CMsg::msgFile(const char * inFilename, ULong inFilebits)
{
	// INCOMPLETE
	// replace LF with CR for mac
	// delete existing file
	// create new file TEXT KAHL and open it
	// lock handle and write file
	// unlock handle and close file
	// restore LF for internal use
}


void
CMsg::msgPrintf(void)
{
	if (gFileOut)
		msgFile("dumper", 0);
	else
	{
		char	buf[256];
		int	length, offset;
		for (length = fLength, offset = 0; length > 0; length -= 255, offset += 255)
		{
			memmove(buf, fMem + offset, 255);
			buf[255] = 0;
			REPprintf("%s",buf);
		}
	}
}

#pragma mark -
#pragma mark CRecObject
/*--------------------------------------------------------------------------------
	C R e c O b j e c t
--------------------------------------------------------------------------------*/

CRecObject::CRecObject()
{ }


CRecObject::~CRecObject()
{ }


void
CRecObject::iRecObject(void)
{
	fFlags = 0;
	fRefCount = 0;
}


void
CRecObject::dealloc(void)
{ }


CRecObject *
CRecObject::retain(void)
{
	fRefCount++;
	return this;
}


void
CRecObject::release(void)
{
	if (fRefCount > 0)
		fRefCount--;
	else
	{
		dealloc();
		delete this;
	}
}


NewtonErr
CRecObject::copyInto(CRecObject * x)
{
	x->fFlags = fFlags;
	x->fRefCount = 0;
	return noErr;
}


size_t
CRecObject::sizeInBytes(void)
{
//	return GetPtrSize((char *)this);	// implies new and delete operate on NewPtr
	return sizeof(this);
}


void
CRecObject::dump(CMsg * outMsg)
{ /* must be overridden */ }


void
CRecObject::dumpObject(char * inStr)
{
	CMsg	amsg(inStr);
	dump(&amsg);
	amsg.msgPrintf();
}


#pragma mark -
#pragma mark ArrayIterator

void
ArrayIterator::removeCurrent(void)
{
	fSize--;
	fIndex--;
	fCurrent -= fElementSize;
}

