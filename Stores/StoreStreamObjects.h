/*
	File:		StoreStreamObjects.h

	Contains:	Newton object store streams interface.

	Written by:	Newton Research Group, 2007.
*/

#if !defined(__STORESTREAMS_H)
#define __STORESTREAMS_H 1

#include "DynamicArray.h"
#include "UnicodeCompression.h"
#include "StreamObjects.h"
#include "Hints.h"


/*------------------------------------------------------------------------------
	Size to specify for an object when you just want to write it all.
------------------------------------------------------------------------------*/

#define kUseObjectSize 0xFFFFFFFF


/*------------------------------------------------------------------------------
	S t o r e O b j e c t H e a d e r
	The root object in the PSS for a soup entry.
------------------------------------------------------------------------------*/

struct StoreObjectHeader
{
	ULong			uniqueId;
	ULong			modTime;
	PSSId			textBlockId;
	uint8_t		numOfHints;
	uint8_t		flags;
	uint16_t		textBlockSize;
	TextHint		textHints[];
//	char			data[];

	int			getHintsHandlerId(void);
	void			setHintsHandlerId(int inFlags);
};


/*------------------------------------------------------------------------------
	C S t o r e R e a d P i p e
------------------------------------------------------------------------------*/

class CStoreReadPipe
{
public:
					CStoreReadPipe(char * inBuffer, size_t inSize);
					CStoreReadPipe(CStoreWrapper * inWrapper, CompressionType inCompression);
					~CStoreReadPipe();

	const CStoreReadPipe &	operator>>(unsigned char&);
	const CStoreReadPipe &	operator>>(UniChar&);
	const CStoreReadPipe &	operator>>(int&);

	void			setPSSID(PSSId inId);
	void			setPosition(fpos_t inPos);
	void			read(char * ioBuf, size_t inSize);
	size_t		readFromStore(char * ioBuf, size_t inSize);
	void			fillBuffer(void);
	void			skip(size_t inSize);
	void			skipByte(void);
	NewtonErr	decompCallback(void * ioBuf, size_t * ioSize, bool * outUnderflow);

private:
	CStoreWrapper *	fStoreWrapper;		// +00
	PSSId					fObjectId;			// +04
	CCallbackDecompressor *	fDecompressor;	// +08
	off_t					fOffset;				// +0C
	size_t				fSizeRemaining;	// +10
	off_t					fBufOffset;			// +14
	off_t					fBufEnd;				// +18
	char					fBuf[256];			// +1C
	char *				fBufPtr;				// +011C
};


/*------------------------------------------------------------------------------
	C S t o r e W r i t e P i p e
------------------------------------------------------------------------------*/

class CStoreWritePipe
{
public:
					CStoreWritePipe();
					~CStoreWritePipe();

	const CStoreWritePipe &	operator<<(unsigned char);
	const CStoreWritePipe &	operator<<(UniChar);
	const CStoreWritePipe &	operator<<(int);

	operator PSSId() const;

	void			init(CStoreWrapper * inWrapper, PSSId inObjectId, size_t inObjectSize, CompressionType inCompression);
	void			setPosition(fpos_t inPos);
	void *		getDataPtr(fpos_t inPos);
	void			write(char * ioBuf, size_t inSize);
	void			writeToStore(char * ioBuf, size_t inSize);
	void			bufferToObject(char * ioBuf, size_t inSize);
	void			complete(void);
	void			flush(void);
	NewtonErr	compCallback(void * ioBuf, size_t inSize, bool inArg3);

private:
	CStoreWrapper *	fStoreWrapper;		// +00
	PSSId					fObjectId;			// +04
	size_t				fObjectSize;		// +08
	CCallbackCompressor *	fCompressor;	// +0C
	off_t					fOffset;				// +10
	off_t					fBufIndex;			// +14
	char					fBuf[512];			// +18
	char *				fBufPtr;				// +0218
	size_t				fBufSize;			// +021C
	char *				fSrcStart;			// +0220	source for compressor
	size_t				fSrcSize;			// +0224
	unsigned int		fSrcOffset;			// +0228
	bool					f22C;					// +022C
};

inline	CStoreWritePipe::operator PSSId() const
{ return fObjectId; }


/*------------------------------------------------------------------------------
	C S t o r e O b j e c t R e a d e r
------------------------------------------------------------------------------*/

typedef int (*VBOProcPtr)(CStoreWrapper*, PSSId, StoreRef, void*);

class CStoreObjectReader
{
public:
			CStoreObjectReader(CStoreWrapper * inStore, PSSId inObjectId, CDynamicArray ** outLargeBinaries);
			~CStoreObjectReader();

	Ref	read(void);
	int	eachLargeObjectDo(VBOProcPtr inFunc, void * inParms);

private:
	Ref	scan(void);
	Ref	scan1(void);

	ULong							fUniqueId;				// +00	unique id
	ULong							fModTime;				// +04	mod time
	CStoreReadPipe				fPipe;					// +08
	CStoreReadPipe				fTextPipe;				// +0128
	CPrecedentsForReading *	fPrecedents;			// +0248
	CStoreWrapper *			fStoreWrapper;			// +024C
	CDynamicArray **			fLargeBinaries;		// +0250	array of PSSIds
	RefStruct					f0254;					// +0254
};


/*------------------------------------------------------------------------------
	C S t o r e O b j e c t W r i t e r
------------------------------------------------------------------------------*/

class CStoreObjectWriter
{
public:
			CStoreObjectWriter(RefArg inObj, CStoreWrapper * inWrapper, PSSId inId);
			~CStoreObjectWriter();

	PSSId	write(void);

	CDynamicArray * largeBinaries(void);
	bool		hasDuplicateLargeBinary(void) const;

private:
	void		prescan(void);
	void		prescan(RefArg inObj);
	void		prescan1(void);
	size_t	longToSize(long inValue) const;
	void		countLargeBinary(void);
	void		scan(void);
	void		scan(RefArg inObj);
	void		scan1(void);
	void		nextHintChunk(void);
	void		writeLargeBinary(void);

	RefVar					fObject;					// +00
	RefStack					fStack;					// +04
	CStoreWrapper *		fStoreWrapper;			// +14
	RefVar					fObj;						// +18
	PSSId						fStoreObjectId;		// +1C
	PSSId						fStoreTextObjectId;	// +20
	CStoreWritePipe		fPipe;					// +24
	CStoreWritePipe		fTextPipe;				// +254
	CPrecedentsForWriting *	fPrecedents;		// +484
	size_t					fStoreObjectSize;		// +488
	StoreObjectHeader *	fStoreObject;			// +48C
	size_t					fStoreTextObjectSize;// +490
	ArrayIndex				fNumOfHints;			// +494	0..255
	TextHint *				fFirstHint;				// +498
	TextHint *				fCurrHint;				// +49C
	long						f4A0;						// +4A0	numOfHintedChars?
	ArrayIndex				fHintChunkSize;		// +4A4
	CDynamicArray *		fLargeBinaries;			// +4A8	array of PSSIds
	bool						fLargeBinaryIsDuplicate;// +4AC
	bool						fHasLargeBinary;			// +4AD
	bool						fLargeBinaryIsString;	// +4AE
};

inline size_t				CStoreObjectWriter::longToSize(long inValue) const
{ return (inValue < 0 || inValue > 254) ? 1 + 4 : 1; }

inline CDynamicArray *	CStoreObjectWriter::largeBinaries(void)
{ return fLargeBinaries; }

inline bool					CStoreObjectWriter::hasDuplicateLargeBinary(void) const
{ return fLargeBinaryIsDuplicate; }


#endif	/* __STORESTREAMS_H */
