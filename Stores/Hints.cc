/*
	File:		Hints.cc

	Contains:	Text hints, for indexing PSS text.

	Written by:	Newton Research Group, 2007.
*/

#include "Objects.h"
#include "UStringUtils.h"
#include "ROMSymbols.h"
#include "LargeBinaries.h"
#include "Hints.h"


/*------------------------------------------------------------------------------
	H i n t s H a n d l e r s
------------------------------------------------------------------------------*/

ULong		gMaxHintsHandlerId;
ULong		gDefaultHintsHandlerId;

CWordHintsHandler *	gHintsHandlers[kNumOfHintsHandlers];

void
InitHintsHandlers(void)
{
	for (ArrayIndex i = 0; i < kNumOfHintsHandlers; ++i)
		gHintsHandlers[i] = NULL;
	gHintsHandlers[0] = new COldWordHintsHandler;
	gHintsHandlers[1] = new CWordHintsHandler;
	gMaxHintsHandlerId = 1;
	gDefaultHintsHandlerId = 1;
}


/*------------------------------------------------------------------------------
	T e x t H i n t s
------------------------------------------------------------------------------*/

void
ClearHintBits(TextHint * inHint)
{
	inHint->x00[0] =
	inHint->x00[1] = 0;
}


#pragma mark -
/*------------------------------------------------------------------------------
	W o r d H i n t s
------------------------------------------------------------------------------*/

char *
GetWordsHints(RefArg inWords)
{
	//sp-10
	ArrayIndex numOfWords = Length(inWords);	// r8
	size_t hintsSize = numOfWords * sizeof(TextHint);	// sp04
//	sp0C = gMaxHintsHandlerId;
	size_t hintsSizeForAllHandlers = hintsSize * (gMaxHintsHandlerId + 1);	// r4
	char * hints = new char[hintsSizeForAllHandlers];
	if (hints == NULL)
		OutOfMemory();
	memset(hints, 0, hintsSizeForAllHandlers);

	//sp-04
	RefVar theWord;
	bool isHintFound = NO;	// r9
	TextHint * hintPtr = (TextHint *)hints;	// r4
//	sp08 = gHintsHandlers;
	for (ArrayIndex idx = 0; idx < gMaxHintsHandlerId; idx++)	// r7
	{
		CWordHintsHandler * handler = gHintsHandlers[idx];	// r5
		if (handler != NULL)
		{
			for (ArrayIndex i = 0; i < numOfWords; i++, hintPtr++)	// r6
			{
				//sp-0C
				theWord = GetArraySlot(inWords, i);
				UniChar * wordStr = GetUString(theWord);	// can contain more than one word
				ArrayIndex strLen = Ustrlen(wordStr);
				ArrayIndex wordLen;	// sssp00
			//	sssp04 = wordStr;
				for ( ; handler->findHintWord(wordStr, wordLen, strLen); wordStr += wordLen)
				{
					handler->setHints(hintPtr, wordStr, wordLen);
					isHintFound = YES;
				}
			}
		}
		else
			hintPtr += numOfWords;	// point to hints for next handler
	}
	if (!isHintFound)
	{
		delete[] hints;
		hints = NULL;
	}
	return hints;
}


#pragma mark -
/*------------------------------------------------------------------------------
	C W o r d H i n t s H a n d l e r
------------------------------------------------------------------------------*/

UChar
CanonicalCharacter(UniChar inChar)
{
	UChar		str[1];
	UniChar ustr[1];
	ustr[0] = inChar;
	UpperCaseNoDiacriticsText(ustr, 1);
	ConvertFromUnicode(ustr, str, 1);
	return str[0];
}


ULong
HashQuadgram(ULong inQuad, ArrayIndex index)
{
	ArrayIndex shift = index & 0x1F;
	if (shift != 0)
		inQuad >>= shift;
	return inQuad * 0x9E3779B9;
}

void
CWordHintsHandler::setHints(TextHint * outHint, UniChar * inStr, ArrayIndex inLen)
{
	ULong hash, bitNo;
	ULong quad = 0x00200000 | (CanonicalCharacter(inStr[0]) << 8) | CanonicalCharacter(inStr[1]);
	for (ArrayIndex i = 2; i < inLen; ++i)
	{
		quad = (quad << 8) | CanonicalCharacter(inStr[i]);
		hash = HashQuadgram(quad, i);
		bitNo = hash >> 26;
		outHint->x00[bitNo / 32] |= (1 << (31 - (bitNo & 31)));
		if (i <= 3)
		{
			hash = HashQuadgram(hash, i);
			bitNo = hash >> 26;
			outHint->x00[bitNo / 32] |= (1 << (31 - (bitNo & 31)));
			hash = HashQuadgram(hash, i);
			bitNo = hash >> 26;
			outHint->x00[bitNo / 32] |= (1 << (31 - (bitNo & 31)));
			if (i == 2)
			{
				hash = HashQuadgram(hash, i);
				bitNo = hash >> 26;
				outHint->x00[bitNo / 32] |= (1 << (31 - (bitNo & 31)));
			}
		}
	}
}

ArrayIndex
CWordHintsHandler::getNumHintChunks(ArrayIndex inNumChars, ArrayIndex * outHintChunkSize)
{
	*outHintChunkSize = 32;
	if (inNumChars >= 8160)
		return 1;
	return inNumChars / 32 + 1;	// 1..255
}

bool
CWordHintsHandler::findHintWord(UniChar *& inStr, ArrayIndex & outWordLen, ArrayIndex & ioStrLen)
{
	// look for start of word (non-delimiter)
	for ( ; ioStrLen > 0 && IsDelimiter(*inStr); inStr++, ioStrLen--)
	{
		if (*inStr == 0)
			return 0;
	}
	if (ioStrLen == 0)
		return NO;

	// look for end of word
	UniChar * s;
	for (s = inStr ; ioStrLen > 0 && !IsDelimiter(*s); s++, ioStrLen--)
		;

	// words shorter than three letters are no good for hints…
	outWordLen = s - inStr;
	if (outWordLen > 2)
		return YES;

	// …so try the next word
	inStr += outWordLen;
	return findHintWord(inStr, outWordLen, ioStrLen);
}


#pragma mark -
/*------------------------------------------------------------------------------
	C O l d W o r d H i n t s H a n d l e r
------------------------------------------------------------------------------*/

void
COldWordHintsHandler::setHints(long * outHint, UniChar * inStr, ArrayIndex inLen)
{
	ULong bitNo;
	ULong quad = 0x00200000 | (CanonicalCharacter(inStr[0]) << 8) | CanonicalCharacter(inStr[1]);
	for (ArrayIndex i = 2; i < inLen; ++i)
	{
		quad = (quad << 8) | CanonicalCharacter(inStr[i]);
		bitNo = HashQuadgram(quad, i) >> 26;		// 0..63
		outHint[bitNo / 32] |= (1 << (31 - (bitNo & 31)));
	}
}


#pragma mark -
/*------------------------------------------------------------------------------
	O b j e c t H i n t s
------------------------------------------------------------------------------*/
#include "StoreWrapper.h"
#include "CachedReadStore.h"
#include "StoreStreamObjects.h"
#include "OSErrors.h"

bool
TestHintBits(TextHint * inBits1, TextHint * inBits2)
{
	return (inBits1->x00[0] & inBits2->x00[0]) == 0
		 && (inBits1->x00[1] & inBits2->x00[1]) == 0;
}


int
TestObjHints(char * inHints, ArrayIndex inNumOfWords, CStoreWrapper * inStoreWrapper, PSSId inId)
{
	if (inNumOfWords == 0)
		return 1;

	//sp-420
	CCachedReadStore theStore(inStoreWrapper->store(), inId, 24);	// original says 96
	StoreObjectHeader * textObj;	// sp00
	OSERRIF(theStore.getDataPtr(0, sizeof(textObj), (void**)&textObj));
	ULong handlerId;
	if ((textObj->flags & 0x04) != 0 || (handlerId = textObj->getHintsHandlerId(), gHintsHandlers[handlerId] == NULL))
		return 1;

	//sp-40
	uint8_t sp[64];	// can’t have more than 64 word hints, then
	for (ArrayIndex i = 0; i < inNumOfWords; ++i)
		sp[i] = 0;

	TextHint * r9 = (TextHint *)inHints + inNumOfWords * handlerId;
	ArrayIndex numOfMatchedWOrds = 0;
	size_t offset = offsetof(StoreObjectHeader, textHints);	// r7
	for (ArrayIndex j = textObj->numOfHints; j > 0; j--, offset += sizeof(TextHint))	// r8
	{
		//sp-04
		TextHint * theHint;
		OSERRIF(theStore.getDataPtr(offset, sizeof(TextHint), (void**)&theHint));
		numOfMatchedWOrds = 0;
		for (ArrayIndex i = 0; i < inNumOfWords; ++i)
		{
			if (sp[i] == 0 && TestHintBits(&r9[i], theHint))
				sp[i] = 1;
			numOfMatchedWOrds += sp[i];
		}
		if (numOfMatchedWOrds == inNumOfWords)
			break;
	}

	return numOfMatchedWOrds == inNumOfWords;
}


#pragma mark -
/* -----------------------------------------------------------------------------
	P e r m O b j e c t T e x t
----------------------------------------------------------------------------- */

/* -----------------------------------------------------------------------------
	C O b j T e x t D e c o m p r e s s o r
----------------------------------------------------------------------------- */
#define kTextCacheSize 1000

class CObjTextDecompressor
{
public:
					CObjTextDecompressor();
					~CObjTextDecompressor();

	UniChar *	decompress(CStoreWrapper * inStoreWrapper, PSSId inObjId, size_t * ioSize);
	UniChar *	slowDecompress(CStoreWrapper * inStoreWrapper, PSSId inObjId, size_t * ioSize);
	NewtonErr	textDecompCallback(void * outBuffer, size_t * ioSize, bool * outEOF);

	const UniChar *	text(void) const;

private:
	unsigned char	fCompressedText[kTextCacheSize];
	UniChar			fUnicodeText[kTextCacheSize];
	CCallbackDecompressor * fDecompressor;
	UniChar *		fTextBuffer;
	size_t			fCompressedSize;
	size_t			fCompressedTextOffset;
};

inline const UniChar *	CObjTextDecompressor::text(void) const { return fTextBuffer; }


NewtonErr
TextDecompCallback(VAddr instance, void * outBuffer, size_t * ioSize, bool * outEOF)
{
	return ((CObjTextDecompressor *)instance)->textDecompCallback(outBuffer, ioSize, outEOF);
}


/* -----------------------------------------------------------------------------
	Constructor.
----------------------------------------------------------------------------- */

CObjTextDecompressor::CObjTextDecompressor()
{
	fDecompressor = NewDecompressor(kUnicodeCompression, TextDecompCallback, (VAddr)this);
	fTextBuffer = fUnicodeText;
}


/* -----------------------------------------------------------------------------
	Destructor.
----------------------------------------------------------------------------- */

CObjTextDecompressor::~CObjTextDecompressor()
{
	if (fDecompressor)
		fDecompressor->destroy();
}


/* -----------------------------------------------------------------------------
	Decompress a PSS object into a Unicode text buffer.
	Args:		inStoreWrapper
				inObjId			id of text object
				ioSize			size required
	Return:	pointer to text buffer
----------------------------------------------------------------------------- */

UniChar *
CObjTextDecompressor::decompress(CStoreWrapper * inStoreWrapper, PSSId inObjId, size_t * ioSize)
{
	NewtonErr err;
	XTRY
	{
		XFAIL(err = inStoreWrapper->store()->getObjectSize(inObjId, &fCompressedSize))
		if (*ioSize > kTextCacheSize*sizeof(UniChar) || fCompressedSize > kTextCacheSize*sizeof(unsigned char))
			return slowDecompress(inStoreWrapper, inObjId, ioSize);

		XFAIL(err = inStoreWrapper->store()->read(inObjId, 0, fCompressedText, fCompressedSize))

		bool underflow;
		fCompressedTextOffset = 0;
		fDecompressor->reset();
		err = fDecompressor->readChunk(fTextBuffer, ioSize, &underflow);
	}
	XENDTRY;

	XDOFAIL(err)
		ThrowOSErr(err);
	XENDFAIL;

	return fTextBuffer;
}


/* -----------------------------------------------------------------------------
	Decompress a PSS object into a Unicode text buffer.
	If the text won’t fit in our buffers, we need to allocate extra.
	Args:		inStoreWrapper
				inObjId			id of text object
				ioSize			size required
	Return:	pointer to text buffer
				It is the caller’s responsibility to FreePtr() this --
				see WithPermObjectTextDo().
----------------------------------------------------------------------------- */

UniChar *
CObjTextDecompressor::slowDecompress(CStoreWrapper * inStoreWrapper, PSSId inObjId, size_t * ioSize)
{
	CStoreReadPipe pipe(inStoreWrapper, kUnicodeCompression);
	pipe.setPSSID(inObjId);
	UniChar * buf = (UniChar *)NewPtr(*ioSize);	// original calls new
	if (buf == NULL) OutOfMemory();
	*ioSize = pipe.readFromStore((char *)buf, *ioSize);
	return buf;
}


/* -----------------------------------------------------------------------------
	Unicode text decompressor calls back here to get more compressed text to
	work on.
	Args:		outBuffer
				ioSize			size required
				outEOF			YES => there is no more
	Return:	error code
----------------------------------------------------------------------------- */

NewtonErr
CObjTextDecompressor::textDecompCallback(void * outBuffer, size_t * ioSize, bool * outEOF)
{
	if (fCompressedSize > *ioSize)
		*outEOF = NO;
	else
	{
		*ioSize = fCompressedSize;
		*outEOF = YES;
	}
	memcpy(outBuffer, fCompressedText + fCompressedTextOffset, *ioSize);
	fCompressedSize -= *ioSize;
	fCompressedTextOffset += *ioSize;
	return noErr;
}


#pragma mark -
/* -----------------------------------------------------------------------------
	WithPermObjectTextDo
----------------------------------------------------------------------------- */
struct TextProcParms
{
	TextProcPtr textProc;
	void * data;
};


int
CallLargeObjectTextProc(CStoreWrapper * inStoreWrapper, PSSId inId, StoreRef inClassRef, void * inParms)
{
	if (!IsSubclass(inStoreWrapper->referenceToSymbol(inClassRef), RSYMstring))
		return 0;

	RefVar obj(LoadLargeBinary(inStoreWrapper, inId, inClassRef));
	CDataPtr ptr(obj);
	return ((TextProcParms *)inParms)->textProc((UniChar *)(char *)ptr, Length(obj) / sizeof(UniChar), ((TextProcParms *)inParms)->data);
}


/* -----------------------------------------------------------------------------
	Perform an operation on a (compressed) stored text object.
	Args:		inStoreWrapper
				inObjId			id of text object
				inTextProc
				inTextProcData
				ioRefCon
	Return:	YES => success,	eg string object matches supplied text
----------------------------------------------------------------------------- */

bool
WithPermObjectTextDo(CStoreWrapper * inStoreWrapper, PSSId inObjId, TextProcPtr inTextProc, void * inTextProcData, void ** ioRefCon)
{
	NewtonErr err;
	bool result = NO;
	StoreObjectHeader textRoot;

	if ((err = inStoreWrapper->store()->read(inObjId, 0, &textRoot, sizeof(textRoot))))
		ThrowOSErr(err);

	CObjTextDecompressor * decompressor = NULL;
	if (textRoot.textBlockId != 0)
	{
		// we are using unicode text compression -- ensure we have a decompressor; use it as the refcon
		decompressor = (CObjTextDecompressor *)*ioRefCon;
		if (decompressor == NULL)
		{
			decompressor = new CObjTextDecompressor;
			if (decompressor == NULL) OutOfMemory();
			*ioRefCon = decompressor;
		}

		ArrayIndex txLen = textRoot.textBlockSize / sizeof(UniChar);
		size_t txSize = textRoot.textBlockSize;
		UniChar * txBuf = decompressor->decompress(inStoreWrapper, textRoot.textBlockId, &txSize);
		bool isBufAllocd = (txBuf != decompressor->text());
		result = inTextProc(txBuf, txLen, inTextProcData);
		if (isBufAllocd)
			FreePtr((Ptr)txBuf);	// original calls delete
	}

	if (decompressor == NULL && (textRoot.flags & 0x04) != 0)
	{
		// we are using VBO text storage
		CStoreObjectReader reader(inStoreWrapper, inObjId, NULL);
		newton_try
		{
			TextProcParms parms;
			parms.textProc = inTextProc;
			parms.data = inTextProcData;
			result = reader.eachLargeObjectDo(CallLargeObjectTextProc, &parms);
		}
		cleanup
		{
			reader.~CStoreObjectReader();
		}
		end_try;
	}

	return result;
}


void
ReleasePermObjectTextCache(void * instance)
{
	if (instance)
		delete (CObjTextDecompressor *)instance;
}
