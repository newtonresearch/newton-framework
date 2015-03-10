/*
	File:		StoreStreamObjects.cc

	Contains:	Newton object store streams interface.

	Written by:	Newton Research Group.
*/

#include "Objects.h"
#include "RefMemory.h"
#include "ROMSymbols.h"
#include "LargeBinaries.h"
#include "StoreStreamObjects.h"
#include "OSErrors.h"

// from StreamObjects.cc
extern bool	gPrecedentsForWritingUsed;
extern bool	gPrecedentsForReadingUsed;

extern bool	PackSmallRect(RefArg inRect, ULong * outPackedRect);
extern Ref	UnpackSmallRect(ULong inPackedRect);


/*------------------------------------------------------------------------------
	S t o r e O b j e c t H e a d e r
	flags:
	1 => has large binary
	4 => large binary is string
------------------------------------------------------------------------------*/

int
StoreObjectHeader::getHintsHandlerId()
{
	return ((flags & 0x02) >> 1) | ((flags & 0x08) >> 2);
}

void
StoreObjectHeader::setHintsHandlerId(int inFlags)
{
	flags |= ((inFlags & 0x01) << 1) | ((inFlags & 0x02) << 2);
}

#pragma mark -

/*------------------------------------------------------------------------------
	C S t o r e R e a d P i p e
	We stream into a buffer which gets refilled from the store when it empties.
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
	Decompression callback.
	Trampoline back to the CStoreReadPipe instance that’s decompressing.
------------------------------------------------------------------------------*/

NewtonErr
DecompCallback(VAddr instance, void * ioBuf, size_t * ioSize, bool * outUnderflow)
{
	return ((CStoreReadPipe*)instance)->decompCallback(ioBuf, ioSize, outUnderflow);
}


/*------------------------------------------------------------------------------
	Construct a CStoreReadPipe that reads from a buffer resident in memory.
------------------------------------------------------------------------------*/

CStoreReadPipe::CStoreReadPipe(char * inBuffer, size_t inSize)
{
	fStoreWrapper = NULL;
	fObjectId = kNoPSSId;
	fBufPtr = inBuffer;
	fDecompressor = NULL;
	fOffset = inSize;
	fSizeRemaining = inSize;
	fBufOffset = 0;
	fBufEnd = inSize;
}


/*------------------------------------------------------------------------------
	Construct a CStoreReadPipe that reads from a store.
------------------------------------------------------------------------------*/

CStoreReadPipe::CStoreReadPipe(CStoreWrapper * inWrapper, CompressionType inCompression)
{
	fStoreWrapper = inWrapper;
	fObjectId = kNoPSSId;
	fDecompressor = (inCompression != 0) ? NewDecompressor(inCompression, DecompCallback, (VAddr)this) : NULL;
	fBufPtr = fBuf;
	fOffset = 0;
	fSizeRemaining = 0;
	fBufOffset = 0;
	fBufEnd = 0;
}


/*------------------------------------------------------------------------------
	Destroy the decompressor if we were using one.
------------------------------------------------------------------------------*/

CStoreReadPipe::~CStoreReadPipe()
{
	if (fDecompressor != NULL)
		fDecompressor->destroy();	// it’s a protocol
}


/*------------------------------------------------------------------------------
	Stream a byte from the store.
------------------------------------------------------------------------------*/

const CStoreReadPipe &
CStoreReadPipe::operator>>(unsigned char & ioValue)
{
	// if we’ve got anything in the buffer, use that
	long bytesRemaining = fBufEnd - fBufOffset;
	if (bytesRemaining >= 1)
	{
		ioValue = *(fBufPtr + fBufOffset);
		fBufOffset++;
	}
	else
		read((char*)&ioValue, 1);
	return *this;
}


/*------------------------------------------------------------------------------
	Stream a character from the store.
------------------------------------------------------------------------------*/

const CStoreReadPipe &
CStoreReadPipe::operator>>(UniChar & ioValue)
{
	unsigned char ch1, ch2;
	*this >> ch1;
	*this >> ch2;
	ioValue = (ch1 << 8) + ch2;
	return *this;
}


/*------------------------------------------------------------------------------
	Stream a long from the store.
	Values < 255 compress to a byte.
------------------------------------------------------------------------------*/

const CStoreReadPipe &
CStoreReadPipe::operator>>(int & ioValue)
{
	unsigned char	byteValue;

	*this >> byteValue;
	if (byteValue == 0xFF)
		read((char*)&ioValue, 4);
	else
		ioValue = byteValue;
	return *this;
}


/*------------------------------------------------------------------------------
	Set the id of the PSS object to read.
------------------------------------------------------------------------------*/

void
CStoreReadPipe::setPSSID(PSSId inObjectId)
{
	fObjectId = inObjectId;
	OSERRIF(fStoreWrapper->store()->getObjectSize(inObjectId, &fSizeRemaining));
}


/*------------------------------------------------------------------------------
	Set the position within the buffer.
------------------------------------------------------------------------------*/

void
CStoreReadPipe::setPosition(fpos_t inPos)
{
	fSizeRemaining += fOffset - inPos;
	fOffset = inPos;
	fBufOffset = 0;
	fBufEnd = 0;
}


/*------------------------------------------------------------------------------
	Read a chunk.
------------------------------------------------------------------------------*/

void
CStoreReadPipe::read(char * ioBuf, size_t inSize)
{
	// if we’ve got enough in the buffer, use that
	long bytesRemaining = fBufEnd - fBufOffset;
	if (bytesRemaining >= inSize)
	{
		memmove(ioBuf, fBufPtr + fBufOffset, inSize);
		fBufOffset += inSize;
	}
	else
	{
		if (bytesRemaining > 0)
		{
			memmove(ioBuf, fBufPtr + fBufOffset, bytesRemaining);
			ioBuf += bytesRemaining;
			inSize -= bytesRemaining;
			fBufOffset += bytesRemaining;
		}
		if (inSize <= 256)
		{
			fillBuffer();
			memmove(ioBuf, fBufPtr, inSize);
			fBufOffset += inSize;
		}
		else
			readFromStore(ioBuf, inSize);
	}
}


/*------------------------------------------------------------------------------
	Read a chunk from the store.
------------------------------------------------------------------------------*/

size_t
CStoreReadPipe::readFromStore(char * ioBuf, size_t inSize)
{
	if (fDecompressor != NULL)
	{
		bool	underflow;
		OSERRIF(fDecompressor->readChunk(ioBuf, &inSize, &underflow));
	}

	if (inSize > fSizeRemaining)
		inSize = fSizeRemaining;

	OSERRIF(fStoreWrapper->store()->read(fObjectId, fOffset, ioBuf, inSize));
	fSizeRemaining -= inSize;
	fOffset += inSize;

	return inSize;
}


/*------------------------------------------------------------------------------
	Fill the buffer from the store.
------------------------------------------------------------------------------*/

void
CStoreReadPipe::fillBuffer(void)
{
	long bytesRead = readFromStore(fBufPtr, 256);
	fBufOffset = 0;
	fBufEnd = bytesRead;
}


/*------------------------------------------------------------------------------
	Skip over an item in the stream.
------------------------------------------------------------------------------*/

void
CStoreReadPipe::skip(size_t inSize)
{
	long bytesRemaining = fBufEnd - fBufOffset;
	if (bytesRemaining >= inSize)
		fBufOffset += inSize;
	else
	{
		if (bytesRemaining > 0)
			fBufOffset += inSize - bytesRemaining;
		for ( ; inSize > 256; inSize -= 256)
			fillBuffer();
		fillBuffer();
		fBufOffset += inSize;
	}
}


/*------------------------------------------------------------------------------
	Skip over a byte in the stream.
------------------------------------------------------------------------------*/

void
CStoreReadPipe::skipByte(void)
{
	long bytesRemaining = fBufEnd - fBufOffset;
	if (bytesRemaining >= 1)
		fBufOffset++;
	else
		skip(1);
}


/*------------------------------------------------------------------------------
	Handle a request from the decompressor for more raw (compressed) data.
------------------------------------------------------------------------------*/

NewtonErr
CStoreReadPipe::decompCallback(void * ioBuf, size_t * ioSize, bool * outUnderflow)
{
	if (*ioSize > fSizeRemaining)
	{
		*ioSize = fSizeRemaining;
		*outUnderflow = YES;
	}
	else
		*outUnderflow = NO;
	OSERRIF(fStoreWrapper->store()->read(fObjectId, fOffset, ioBuf, *ioSize));
	fSizeRemaining -= *ioSize;
	fOffset += *ioSize;
	return noErr;
}


StoreRef
ReadReference(CStoreReadPipe & inPipe)
{
	union
	{
		char		buf[4];
		StoreRef	ref;
	} r;
	r.ref = 0;
	inPipe.read(&r.buf[1], 3);
	return CANONICAL_LONG(r.ref);
}

#pragma mark -

/*------------------------------------------------------------------------------
	C S t o r e W r i t e P i p e
	We stream into a buffer which gets flushed to the store when it fills.
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
	Compression callback.
	Springboard back to the CStoreWritePipe instance that’s compressing.
------------------------------------------------------------------------------*/

NewtonErr
CompCallback(VAddr instance, void * inBuf, size_t inSize, bool inArg3)
{
	return ((CStoreWritePipe*)instance)->compCallback(inBuf, inSize, inArg3);
}


/*------------------------------------------------------------------------------
	Construct with no buffer.
------------------------------------------------------------------------------*/

CStoreWritePipe::CStoreWritePipe()
{
	fBufPtr = NULL;
	fCompressor = NULL;
}


/*------------------------------------------------------------------------------
	Destroy buffers if they were created.
------------------------------------------------------------------------------*/

CStoreWritePipe::~CStoreWritePipe()
{
	if (fBufPtr != NULL
	&&  fBufPtr != fBuf)
		FreePtr(fBufPtr);
	if (fCompressor != NULL)
		fCompressor->destroy();	// it’s a protocol
}


/*------------------------------------------------------------------------------
	Initialize for writing to a PSS object.
------------------------------------------------------------------------------*/

void
CStoreWritePipe::init(CStoreWrapper * inWrapper, PSSId inObjectId, size_t inObjectSize, CompressionType inCompression)
{
	fStoreWrapper = inWrapper;
	fObjectId = inObjectId;
	fCompressor = (inCompression != 0) ? NewCompressor(inCompression, CompCallback, (VAddr)this) : NULL;
	fObjectSize = inObjectSize;
	fOffset = 0;
	fBufIndex = 0;

	size_t objSize = inObjectSize;
	if (fCompressor != NULL)
		objSize += inObjectSize;
	if (objSize > 512
	&&  (fBufPtr = NewPtr(objSize)))
	{
		fBufSize = inObjectSize;
		f22C = YES;
	}
	else
	{
		fBufSize = 512;
		fBufPtr = fBuf;
		f22C = (objSize <= 512);
	}
	if (fCompressor != NULL)
	{
		fSrcStart = fBufPtr + inObjectSize;
		fSrcSize = inObjectSize;
		fSrcOffset = 0;
	}
	if (!f22C
	&&  fObjectId != kNoPSSId)
	{
		OSERRIF(fStoreWrapper->store()->setObjectSize(fObjectId, 0));
		OSERRIF(fStoreWrapper->store()->setObjectSize(fObjectId, fObjectSize));
	}
}


/*------------------------------------------------------------------------------
	Stream out a byte value.
------------------------------------------------------------------------------*/

const CStoreWritePipe &
CStoreWritePipe::operator<<(unsigned char inValue)
{
	if (fBufIndex >= fBufSize)
		flush();
	*(fBufPtr + fBufIndex) = inValue;
	fBufIndex++;
	return *this;
}


/*------------------------------------------------------------------------------
	Stream out a character value.
------------------------------------------------------------------------------*/

const CStoreWritePipe &
CStoreWritePipe::operator<<(UniChar inValue)
{
	*this << (unsigned char) (inValue >> 8);
	*this << (unsigned char) inValue;
	return *this;
}


/*------------------------------------------------------------------------------
	Stream out a long value.
	Values < 255 are compressed to a byte.
------------------------------------------------------------------------------*/

const CStoreWritePipe &
CStoreWritePipe::operator<<(int inValue)
{
	if (inValue >= 0
	&&  inValue < 0xFF)
	{
		*this << (unsigned char) inValue;
	}
	else
	{
		*this << (unsigned char) 0xFF;
		write((char*)&inValue, 4);
	}
	return *this;
}


/*------------------------------------------------------------------------------
	Set the position in the buffer.
------------------------------------------------------------------------------*/

void
CStoreWritePipe::setPosition(fpos_t inPos)
{
	fOffset = inPos;
	if (f22C)
		fBufIndex = inPos;
}


/*------------------------------------------------------------------------------
	Return a pointer into the buffer at a specified offset.
------------------------------------------------------------------------------*/

void *
CStoreWritePipe::getDataPtr(fpos_t inOffset)
{
	return f22C ? fBufPtr + inOffset : NULL;
}


/*------------------------------------------------------------------------------
	Write a chunk of data.
------------------------------------------------------------------------------*/

void
CStoreWritePipe::write(char * ioBuf, size_t inSize)
{
	long bytesRemaining = fBufSize - fBufIndex;
	if (bytesRemaining >= inSize)
	{
		memmove(fBufPtr + fBufIndex, ioBuf, inSize);
		fBufIndex += inSize;
	}
	else
	{
		flush();
		if (fBufSize > inSize)
		{
			memmove(fBufPtr, ioBuf, inSize);
			fBufIndex += inSize;
		}
		else
			writeToStore(ioBuf, inSize);
	}
}


/*------------------------------------------------------------------------------
	Write a chunk of data to the store.
------------------------------------------------------------------------------*/

void
CStoreWritePipe::writeToStore(char * ioBuf, size_t inSize)
{
	if (fCompressor != NULL)
		OSERRIF(fCompressor->writeChunk(ioBuf, inSize));

	if (f22C)
		bufferToObject(ioBuf, inSize);
	else
	{
		if (fObjectId == kNoPSSId)
		{
			if (fObjectSize == kUseObjectSize)
				fObjectSize = fOffset + inSize;
			OSERRIF(fStoreWrapper->store()->newObject(&fObjectId, fObjectSize));
		}
		OSERRIF(fStoreWrapper->store()->write(fObjectId, fOffset, ioBuf, inSize));
	}
	fOffset += inSize;
}


/*------------------------------------------------------------------------------
	Write the buffer to a PSS object.
------------------------------------------------------------------------------*/

void
CStoreWritePipe::bufferToObject(char * ioBuf, size_t inSize)
{
	if (fObjectId == kNoPSSId)
		OSERRIF(fStoreWrapper->store()->newObject(&fObjectId, ioBuf, inSize));
	else
		OSERRIF(fStoreWrapper->store()->replaceObject(fObjectId, ioBuf, inSize));
}


/*------------------------------------------------------------------------------
	Finish up by flushing everything to the store.
------------------------------------------------------------------------------*/

void
CStoreWritePipe::complete(void)
{
	if (fObjectId == kNoPSSId)
		fObjectSize = kUseObjectSize;
	flush();
	if (fCompressor != NULL)
		fCompressor->flush();
	if (!f22C)
	{
		OSERRIF(fStoreWrapper->store()->setObjectSize(fObjectId, fOffset));
	}
}


/*------------------------------------------------------------------------------
	Flush the buffer to the store.
------------------------------------------------------------------------------*/

void
CStoreWritePipe::flush(void)
{
	if (fBufIndex != 0)
	{
		writeToStore(fBufPtr, fBufIndex);
		fBufIndex = 0;
	}
}


/*------------------------------------------------------------------------------
	Handle a request from the compressor for more raw (uncompressed) data.
------------------------------------------------------------------------------*/

NewtonErr
CStoreWritePipe::compCallback(void * ioBuf, size_t inSize, bool inArg3)
{
	if (f22C)
	{
		if (fSrcOffset + inSize > fSrcSize)
		{
			bufferToObject(fSrcStart, fSrcOffset);
			f22C = NO;
			fOffset = fSrcOffset;
			fObjectSize = fSrcOffset;
			compCallback(ioBuf, inSize, inArg3);
		}
		else
		{
			memmove(fSrcStart + fSrcOffset, ioBuf, inSize);
			fSrcOffset += inSize;
			if (inArg3)
				bufferToObject(fSrcStart, fSrcOffset);
		}
	}
	else
	{
		if (fObjectId == kNoPSSId)
		{
			if (fObjectSize == kUseObjectSize)
				fObjectSize = fOffset + inSize;
			OSERRIF(fStoreWrapper->store()->newObject(&fObjectId, fObjectSize));
		}
		if (fOffset + inSize > fObjectSize)
		{
			fObjectSize = inSize + fOffset;
			OSERRIF(fStoreWrapper->store()->setObjectSize(fObjectId, fObjectSize));
		}
		OSERRIF(fStoreWrapper->store()->write(fObjectId, fOffset, ioBuf, inSize));
		fOffset += inSize;
	}

	return noErr;
}


void
WriteReference(CStoreWritePipe & inPipe, StoreRef inRef)
{
	union
	{
		char		buf[4];
		StoreRef	ref;
	} r;
	r.ref = CANONICAL_LONG(inRef);
	inPipe.write(&r.buf[1], 3);
}


void
WriteReference(char * ioBuf, StoreRef inRef)
{
	union
	{
		char		buf[4];
		StoreRef	ref;
	} r;
	r.ref = CANONICAL_LONG(inRef);
	ioBuf[2] = r.buf[3];
	ioBuf[1] = r.buf[2];
	ioBuf[0] = r.buf[1];
}


#pragma mark -

/*------------------------------------------------------------------------------
	C S t o r e O b j e c t R e a d e r
------------------------------------------------------------------------------*/

CStoreObjectReader::CStoreObjectReader(CStoreWrapper * inWrapper, PSSId inObjectId, CDynamicArray ** outLargeBinaries)
	:	fPipe(inWrapper, kCompression), fTextPipe(inWrapper, kUnicodeCompression)
{
	fStoreWrapper = inWrapper;
	fLargeBinaries = outLargeBinaries;
	if (outLargeBinaries != NULL)
		*outLargeBinaries = NULL;
	f0254 = NILREF;

	fPipe.setPSSID(inObjectId);
	StoreObjectHeader root;
	OSERRIF(fStoreWrapper->store()->read(inObjectId, 0, &root, sizeof(root)));
	fUniqueId = root.uniqueId;
	fModTime = root.modTime;
	fPipe.setPosition(offsetof(StoreObjectHeader, textHints) + root.numOfHints * sizeof(TextHint));
	if (root.textBlockId != 0)
		fTextPipe.setPSSID(root.textBlockId);

	if (gPrecedentsForReadingUsed)
	{
		if ((fPrecedents = new CPrecedentsForReading) == NULL)
			OutOfMemory();
	}
	else
	{
		fPrecedents = gPrecedentsForReading;
		gPrecedentsForReadingUsed = YES;
	}
}


CStoreObjectReader::~CStoreObjectReader()
{
	if (fPrecedents == gPrecedentsForReading)
	{
		gPrecedentsForReadingUsed = NO;
		fPrecedents->reset();
	}
	else if (fPrecedents != NULL)
		delete fPrecedents;
}


Ref
CStoreObjectReader::read(void)
{
	RefVar	obj(scan());
	if ((ObjectFlags(obj) & kObjMask) == kFrameObject)
	{
		if (fUniqueId != 0xFFFFFFFF)
			SetFrameSlot(obj, SYMA(_uniqueId), MAKEINT(fUniqueId));
		if (fModTime != 0xFFFFFFFF)
			SetFrameSlot(obj, SYMA(_modTime), MAKEINT(fModTime));
	}
	UndirtyObject(obj);
	return obj;
}


Ref
CStoreObjectReader::scan(void)
{
	return scan1();
}


Ref
CStoreObjectReader::scan1(void)
{
	RefVar			obj;
	unsigned char	objType;
	fPipe >> objType;
	switch (objType)
	{
	case kNSImmediate:
		{
			int immed;
			fPipe >> immed;
			obj = (Ref)immed;
		}
		break;

	case kNSCharacter:
		{
			unsigned char value;
			fPipe >> value;
			obj = MAKECHAR(value);
		}
		break;

	case kNSUnicodeCharacter:
		{
			UniChar	ch;
			fPipe >> ch;
			obj = MAKECHAR(ch);
		}
		break;

	case kNSBinaryObject:
	case kNSString:
		{
			int		binSize;
			fPipe >> binSize;

			obj = AllocateBinary(RA(NILREF), binSize);
			fPrecedents->add(obj);

			RefVar	objClass = scan();
			LockRef(obj);
			((BinaryObject *)ObjectPtr(obj))->objClass = objClass;

			if (objType == kNSString)
				fTextPipe.read(BinaryData(obj), binSize);
			else
				fPipe.read(BinaryData(obj), binSize);
			UnlockRef(obj);
		}
		break;

	case kNSArray:
	case kNSPlainArray:
		{
			int	numOfElements;
			fPipe >> numOfElements;

			obj = MakeArray(numOfElements);
			fPrecedents->add(obj);

			if (objType != kNSPlainArray)
				((ArrayObject *)ObjectPtr(obj))->objClass = scan();
			// else it’s 'array by default

			for (ArrayIndex i = 0; i < numOfElements; ++i)
				SetArraySlot(obj, i, scan());
			UndirtyObject(obj);
		}
		break;

	case kNSFrame:
		{
			ArrayIndex index = fPrecedents->add(RA(NILREF));	// placeholder

			RefVar	map(fStoreWrapper->referenceToMap(ReadReference(fPipe)));
			obj = AllocateFrameWithMap(map);
			fPrecedents->replace(index, obj);

			if (ISNIL(f0254))
				f0254 = obj;

			for (ArrayIndex i = 0, numOfSlots = Length(obj); i < numOfSlots; ++i)
				SetArraySlot(obj, i, scan());
			UndirtyObject(obj);
		}
		break;

	case kNSSymbol:
		{
			obj = fStoreWrapper->referenceToSymbol(ReadReference(fPipe));
			fPrecedents->add(obj);
		}
		break;

	case kNSPrecedent:
		{
			int	value;
			fPipe >> value;
			obj = *fPrecedents->get(value);
		}
		break;

	case kNSNIL:
		obj = NILREF;
		break;

	case kNSSmallRect:
		{
			int	value;
			fPipe >> value;
			obj = UnpackSmallRect(value);
			fPrecedents->add(obj);
		}
		break;

	case kNSLargeBinary:
		{
			PSSId		lbId;
			StoreRef	lbClass;
			fPipe.read((char*)&lbId, sizeof(lbId));
			fPipe.read((char*)&lbClass, sizeof(lbClass));
			if (fLargeBinaries != NULL)
			{
				obj = NILREF;
				if (*fLargeBinaries == NULL)
					*fLargeBinaries = new CDynamicArray;
				(*fLargeBinaries)->addElement((void*)(long)lbId);
			}
			else
			{
				obj = LoadLargeBinary(fStoreWrapper, lbId, lbClass);
				LBData *	largeBinary = (LBData *)&((IndirectBinaryObject *)ObjectPtr(obj))->data;
				largeBinary->fEntry = f0254;
			}
			fPrecedents->add(obj);
		}
		break;

	default:
		ThrowErr(exStore, kNSErrObjectCorrupted);
	}
	return obj;
}


int
CStoreObjectReader::eachLargeObjectDo(VBOProcPtr inProc, void * inParms)
{
	unsigned char objType;
	fPipe >> objType;
	switch (objType)
	{
	case kNSImmediate:
		{
			unsigned char value;
			fPipe >> value;
			if (value == 0xFF)
				fPipe.skip(sizeof(long));
		}
		break;

	case kNSCharacter:
		fPipe.skipByte();
		break;

	case kNSUnicodeCharacter:
		fPipe.skip(sizeof(UniChar));
		break;

	case kNSBinaryObject:
	case kNSString:
		{
			int		binSize;
			fPipe >> binSize;
			eachLargeObjectDo(inProc, inParms);	// scan the class

			// kNSString is stored in a different object
			if (objType == kNSBinaryObject)
				fPipe.skip(binSize);
		}
		break;

	case kNSArray:
	case kNSPlainArray:
		{
			int	numOfElements;
			fPipe >> numOfElements;

			if (objType != kNSPlainArray)
				eachLargeObjectDo(inProc, inParms);	// scan the class

			for (ArrayIndex i = 0; i < numOfElements; ++i)
				if (eachLargeObjectDo(inProc, inParms))
					return 1;
		}
		break;

	case kNSFrame:
		{
			for (int i = 0, numOfSlots = Length(fStoreWrapper->referenceToMap(ReadReference(fPipe))) - 1; i < numOfSlots; ++i)
				if (eachLargeObjectDo(inProc, inParms))
					return 1;
		}
		break;

	case kNSSymbol:
		fPipe.skip(3);
		break;

	case kNSPrecedent:
		{
			int	value;
			fPipe >> value;
		}
		break;

	case kNSNIL:
		// nothing more to skip
		break;

	case kNSSmallRect:
		fPipe.skip(sizeof(long));
		break;

	case kNSLargeBinary:
		{
			PSSId		lbId;
			StoreRef	lbClass;
			fPipe.read((char*)&lbId, sizeof(lbId));
			fPipe.read((char*)&lbClass, sizeof(lbClass));
			if (inProc(fStoreWrapper, lbId, lbClass, inParms))
				return 1;
		}
		break;

	default:
		ThrowErr(exStore, kNSErrObjectCorrupted);
	}
	return 0;
}


#pragma mark -

/*------------------------------------------------------------------------------
	C S t o r e O b j e c t W r i t e r
------------------------------------------------------------------------------*/

CStoreObjectWriter::CStoreObjectWriter(RefArg inObj, CStoreWrapper * inWrapper, PSSId inId)
	:	fObj(inObj)
{
	fObject = inObj;
	fStoreWrapper = inWrapper;
	fStoreObjectSize = 0;
	fStoreTextObjectSize = 0;
	f4A0 = 0;
	fStoreObjectId = inId;
	fStoreObject = NULL;

	if (gPrecedentsForWritingUsed)
	{
		if ((fPrecedents = new CPrecedentsForWriting) == NULL)
			OutOfMemory();
	}
	else
	{
		fPrecedents = gPrecedentsForWriting;
		gPrecedentsForWritingUsed = YES;
	}

	fLargeBinaries = NULL;
	fLargeBinaryIsDuplicate = NO;
	fHasLargeBinary = NO;
	fLargeBinaryIsString = NO;
}


CStoreObjectWriter::~CStoreObjectWriter()
{
	if (fStoreObject != NULL
	&& fPipe.getDataPtr(0) != fStoreObject)
		delete fStoreObject;
	if (fLargeBinaries != NULL)
		delete fLargeBinaries;

	if (fPrecedents == gPrecedentsForWriting)
	{
		gPrecedentsForWritingUsed = NO;
		fPrecedents->reset();
	}
	else if (fPrecedents != NULL)
		delete fPrecedents;
}


PSSId
CStoreObjectWriter::write(void)
{
	bool isObjectOnStore;

	if (fStoreObjectSize == 0)
		prescan();

	fNumOfHints = gHintsHandlers[gDefaultHintsHandlerId]->getNumHintChunks(fStoreTextObjectSize/2, &fHintChunkSize);

	size_t storeObjHeaderSize = sizeof(StoreObjectHeader) + fNumOfHints * sizeof(TextHint);
	fStoreObjectSize += storeObjHeaderSize;

	if (fStoreObjectId != kNoPSSId)
	{
		OSERRIF(fStoreWrapper->store()->read(fStoreObjectId, offsetof(StoreObjectHeader, textBlockId), &fStoreTextObjectId, sizeof(fStoreTextObjectId)));
		if (fStoreTextObjectId != 0)
		{
			if (fStoreTextObjectSize == 0)
				OSERRIF(fStoreWrapper->store()->deleteObject(fStoreTextObjectId));
		}
		else
			fStoreTextObjectId = kNoPSSId;
	}
	else
		fStoreTextObjectId = kNoPSSId;
	fPrecedents->reset();
	fPipe.init(fStoreWrapper, fStoreObjectId, fStoreObjectSize, kCompression);
	if (fStoreTextObjectSize != 0)
		fTextPipe.init(fStoreWrapper, fStoreTextObjectId, fStoreTextObjectSize, kUnicodeCompression);

	fStoreObject = (StoreObjectHeader *)fPipe.getDataPtr(0);
	if (fStoreObject)
		isObjectOnStore = YES;
	else
	{
		fStoreObject = (StoreObjectHeader *)new char[storeObjHeaderSize];
		if (fStoreObject == NULL)
			OutOfMemory();
		isObjectOnStore = NO;
	}
	fCurrHint = fFirstHint = fStoreObject->textHints;
	ClearHintBits(fCurrHint);

	fPipe.setPosition(storeObjHeaderSize);
	scan();

	if (fStoreTextObjectSize != 0)
	{
		fTextPipe.complete();
		fStoreTextObjectId = fTextPipe;
	}
	else
		fStoreTextObjectId = 0;
	while (fFirstHint + fNumOfHints - 1 > fCurrHint)
		nextHintChunk();

	StoreObjectHeader * storeObj = fStoreObject;
	if ((ObjectFlags(fObj) & kObjMask) == kFrameObject)
	{
		Ref r;
		r = GetFrameSlot(fObj, SYMA(_uniqueId));
		storeObj->uniqueId = (NOTNIL(r)) ? RINT(r) : kNoPSSId;
		r = GetFrameSlot(fObj, SYMA(_modTime));
		storeObj->modTime = (NOTNIL(r)) ? RINT(r) & 0x3FFFFFFF : 0xFFFFFFFF;
	}
	storeObj->textBlockId = fStoreTextObjectId;
	storeObj->numOfHints = fNumOfHints;
	storeObj->textBlockSize = fStoreTextObjectSize;
	storeObj->flags = 0;
	storeObj->setHintsHandlerId(gDefaultHintsHandlerId);
	if (fHasLargeBinary)
	{
		storeObj->flags |= 0x01;
		if (fLargeBinaryIsString)
			storeObj->flags |= 0x04;
	}

	fPipe.complete();
	fStoreObjectId = fPipe;
	if (isObjectOnStore)
		OSERRIF(fStoreWrapper->store()->write(fStoreObjectId, 0, storeObj, storeObjHeaderSize));

	return fStoreObjectId;
}


void
CStoreObjectWriter::nextHintChunk(void)
{
	if (fFirstHint + fNumOfHints - 1 > fCurrHint)
	{
		fCurrHint++;
		ClearHintBits(fCurrHint);
	}
	else
	{
		fCurrHint->x00[1] = 0xFFFFFFFF;
		fCurrHint->x00[0] = 0xFFFFFFFF;
	}
	f4A0 = 0;
}


void
CStoreObjectWriter::scan(void)
{
	scan1();
}


void
CStoreObjectWriter::scan(RefArg inObject)
{
	*(fStack.top)++ = fObj;
	fObj = inObject;
	scan1();
	fObj = *(--fStack.top);
}


void
CStoreObjectWriter::scan1(void)
{
	if (ISPTR(fObj))
	{
		ArrayIndex index = fPrecedents->find(fObj);
		if (index != kIndexNotFound)
		{
			fPipe << (unsigned char) kNSPrecedent;
			fPipe << (int)index;
		}
		else
		{
			UndirtyObject(fObj);
			fPrecedents->add(fObj);

			ULong flags = ObjectFlags(fObj);
			if ((flags & kObjSlotted) != 0)
			{
				if ((flags & kObjFrame) != 0)
				{
					ULong	packedRect;
					if (PackSmallRect(fObj, &packedRect))	// small rect
					{
						fPipe << (unsigned char) kNSSmallRect;
						fPipe << (int) packedRect;
					}
					else						// frame
					{
						fPipe << (unsigned char) kNSFrame;

						ArrayIndex * slotIndices, * slotIndexPtr;
						ArrayIndex numOfSlots;
						WriteReference(fPipe, fStoreWrapper->frameToMapReference(fObj, fStack.top == fStack.base, &numOfSlots, &slotIndices));	// only top-level frame is actually a soup entry

						slotIndexPtr = slotIndices;
						for (ArrayIndex i = 0; i < numOfSlots; ++i)
							scan(GetArraySlot(fObj, *slotIndexPtr++));

						delete slotIndices;
					}
				}
				else							// array
				{
					Ref	arrayClass = ((ArrayObject *)ObjectPtr(fObj))->objClass;
					unsigned char	objType = EQRef(arrayClass, RSYMarray) ? kNSPlainArray : kNSArray;
					ArrayIndex numOfSlots = Length(fObj);

					fPipe << objType;
					fPipe << (int)numOfSlots;

					if (objType == kNSArray)
						scan(arrayClass);

					for (ArrayIndex i = 0; i < numOfSlots; ++i)
						scan(GetArraySlot(fObj, i));
				}
			}
			else if ((flags & kObjMask) == kIndirectBinaryObject)
			{
				writeLargeBinary();
			}
			else if (IsSymbol(fObj))	// symbol
			{
				fPipe << (unsigned char) kNSSymbol;
				WriteReference(fPipe, fStoreWrapper->symbolToReference(fObj));
			}
			else								// binary
			{
				unsigned char	objType = IsInstance(fObj, RSYMstring) ? kNSString : kNSBinaryObject;
				size_t	objSize = Length(fObj);

				fPipe << (unsigned char) objType;
				fPipe << (int) objSize;

				scan(((BinaryObject *)ObjectPtr(fObj))->objClass);

				if (objType == kNSBinaryObject)
					fPipe.write(BinaryData(fObj), objSize);
				else
				{
					fTextPipe.write(BinaryData(fObj), objSize);
					if (!EQRef(ClassOf(fObj), RSYMstring_2Enohint))
					{
						UniChar * text = (UniChar *)BinaryData(fObj);	// r5
						UniChar * word = text;	// sp08
						ArrayIndex textLen = objSize / sizeof(UniChar) - 1;	// sp00
						ArrayIndex wordLen;				// sp04
						ArrayIndex hintLen;				// r6
						while (gHintsHandlers[gDefaultHintsHandlerId]->findHintWord(word, wordLen, textLen))
						{
							gHintsHandlers[gDefaultHintsHandlerId]->setHints(fCurrHint, word, wordLen);
							word += wordLen;
							hintLen = (word - text) / sizeof(UniChar);
							f4A0 += hintLen;
							// textLen -= hintLen;	// not done in the original but looks like it’s needed
							text = word;
							for ( ; hintLen > fHintChunkSize; hintLen -= fHintChunkSize)
								nextHintChunk();
						}
					}
				}
			}
		}
	}

	else if (ISNIL(fObj))		// NIL
		fPipe << (unsigned char) kNSNIL;

	else if (ISCHAR(fObj))		// character
	{
		UniChar	ch = (UniChar) RIMMEDVALUE(fObj);
		if (ch < 256)
		{
			fPipe << (unsigned char) kNSCharacter;
			fPipe << (unsigned char) ch;
		}
		else
		{
			fPipe << (unsigned char) kNSUnicodeCharacter;
			fPipe << ch;
		}
	}

	else								// immediate
	{
		int immed = (Ref) fObj;
		fPipe << (unsigned char) kNSImmediate;
		fPipe << immed;
	}
}


void
CStoreObjectWriter::prescan(void)
{
	prescan1();
}


void
CStoreObjectWriter::prescan(RefArg inObject)
{
	*(fStack.top)++ = fObj;
	fObj = inObject;
	prescan1();
	fObj = *(--fStack.top);
}


void
CStoreObjectWriter::prescan1(void)
{
	if (ISPTR(fObj))
	{
		ArrayIndex	index;
		if ((index = fPrecedents->find(fObj)) != kIndexNotFound)		// precedent
			fStoreObjectSize += (1 + longToSize(index));

		else
		{
			fPrecedents->add(fObj);
			ULong flags = ObjectFlags(fObj);
			if ((flags & kObjSlotted) != 0)
			{
				if ((flags & kObjFrame) != 0)
				{
					ULong	packedRect;
					if (PackSmallRect(fObj, &packedRect))	// small rect
						fStoreObjectSize += (1 + 4);
					else						// frame
					{
						ArrayIndex	protoIndex, uniqueIdIndex, modTimeIndex;
						ArrayIndex numOfSlots = Length(fObj);

						RefVar	map(((FrameObject*)ObjectPtr(fObj))->map);
						protoIndex = FindOffset(map, RSYM_proto);
						if (fStack.top == fStack.base)
						{
							uniqueIdIndex = FindOffset(map, RSYM_uniqueId);
							modTimeIndex = FindOffset(map, RSYM_modTime);
						}
						else
						{
							uniqueIdIndex = kIndexNotFound;
							modTimeIndex = kIndexNotFound;
						}

						fStoreObjectSize += (1 + longToSize(numOfSlots));
						for (ArrayIndex i = 0; i < numOfSlots; ++i)
						{
							if (i != protoIndex
							&&  i != uniqueIdIndex
							&&  i != modTimeIndex)
								prescan(GetArraySlot(fObj, i));
						}
					}
				}
				else							// array
				{
					Ref	arrayClass = ((ArrayObject *)ObjectPtr(fObj))->objClass;
					ArrayIndex numOfSlots = Length(fObj);
					if (!EQRef(arrayClass, RSYMarray))
						prescan(arrayClass);

					fStoreObjectSize += (1 + longToSize(numOfSlots));
					for (ArrayIndex i = 0; i < numOfSlots; ++i)
						prescan(GetArraySlot(fObj, i));
				}
			}
			else if ((flags & kObjMask) == kIndirectBinaryObject)
			{
				fStoreObjectSize += (1 + sizeof(PSSId) + sizeof(StoreRef));
			}
			else if (IsSymbol(fObj))	// symbol
			{
				fStoreObjectSize += (1 + sizeof(StoreRef));
			}
			else								// binary
			{
				ArrayIndex	objSize = Length(fObj);
				fStoreObjectSize += (1 + longToSize(objSize));

				Ref	binClass = ((BinaryObject *)ObjectPtr(fObj))->objClass;
				prescan(binClass);

				if (IsInstance(fObj, RSYMstring))
					fStoreTextObjectSize += objSize;
				else
					fStoreObjectSize += objSize;
			}
		}
	}

	else if (ISNIL(fObj))	// NIL
		fStoreObjectSize++;

	else if (ISCHAR(fObj))	// character
		fStoreObjectSize += (1 + ((RIMMEDVALUE(fObj) < 256) ? sizeof(char) : sizeof(UniChar)));

	else							// immediate
		fStoreObjectSize += (1 + longToSize(fObj));
}


void
CStoreObjectWriter::writeLargeBinary(void)
{
	RefVar	obj(fObj);
	fHasLargeBinary = YES;
	fLargeBinaryIsString = IsInstance(obj, RSYMstring);
	fPipe << (unsigned char)kNSLargeBinary;

	LBData *	largeBinary = (LBData *)&((IndirectBinaryObject *)ObjectPtr(obj))->data;
	if (largeBinary->getStore() != fStoreWrapper
	||  (NOTNIL(largeBinary->fEntry) && fStoreObjectId == kNoPSSId)
	||  !largeBinary->isSameEntry(fObject))
	{
		obj = DuplicateLargeBinary(obj, fStoreWrapper);
		largeBinary = (LBData *)&((IndirectBinaryObject *)ObjectPtr(obj))->data;
		fLargeBinaryIsDuplicate = YES;
	}

	if (fLargeBinaries == NULL)
		fLargeBinaries = new CDynamicArray;
	fLargeBinaries->addElement((void*)(long)(PSSId)*largeBinary);

	largeBinary->fEntry = fObject;
	fPipe.write((char*)&largeBinary->fId, sizeof(PSSId));
	fPipe.write((char*)&largeBinary->fClass, sizeof(StoreRef));
	CommitLargeBinary(obj);
}

