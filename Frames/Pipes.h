/*
	File:		Pipes.h

	Contains:	Pipe classes.
					A note on types: we use (long) to represent an offset, since
					this is what stdio functions use.

	Written by:	Newton Research Group.
*/

#if !defined(__PIPES_H)
#define __PIPES_H 1

#include "Objects.h"

DeclareException(exPipe, exRootException);


/*------------------------------------------------------------------------------
	C P i p e C a l l b a c k
------------------------------------------------------------------------------*/

class CPipeCallback
{
public:
				CPipeCallback();
	virtual	~CPipeCallback();

	virtual bool	status(size_t inNumOfBytesGot, size_t inNumOfBytesPut) = 0;

	size_t		f04;		// max values for progress indication
	size_t		f08;
};


/*------------------------------------------------------------------------------
	C P i p e
------------------------------------------------------------------------------*/

class CPipe
{
public:
				CPipe();
	virtual	~CPipe();

	virtual	long		readSeek(long inOffset, int inSelector) = 0;
	virtual	long		readPosition(void) const = 0;
	virtual	long		writeSeek(long inOffset, int inSelector) = 0;
	virtual	long		writePosition(void) const = 0;
	virtual	void		readChunk(void * outBuf, size_t & ioSize, bool & outEOF) = 0;
	virtual	void		writeChunk(const void * inBuf, size_t inSize, bool inFlush) = 0;
	virtual	void		flushRead(void) = 0;
	virtual	void		flushWrite(void) = 0;
	virtual	void		reset(void) = 0;
	virtual	void		resetRead();
	virtual	void		resetWrite();
	virtual	void		overflow() = 0;
	virtual	void		underflow(long, bool&) = 0;

	const CPipe &	operator>>(char&);
	const CPipe &	operator>>(unsigned char&);
	const CPipe &	operator>>(short&);
	const CPipe &	operator>>(unsigned short&);
	const CPipe &	operator>>(int&);
	const CPipe &	operator>>(unsigned int&);
	const CPipe &	operator>>(size_t&);		// NRG

	const CPipe &	operator<<(char);
	const CPipe &	operator<<(unsigned char);
	const CPipe &	operator<<(short);
	const CPipe &	operator<<(unsigned short);
	const CPipe &	operator<<(int);
	const CPipe &	operator<<(unsigned int);
	const CPipe &	operator<<(size_t&);		// NRG
};


/*------------------------------------------------------------------------------
	C P t r P i p e
------------------------------------------------------------------------------*/

class CPtrPipe : public CPipe
{
public:
				CPtrPipe();
				~CPtrPipe();

	void		init(size_t inSize, CPipeCallback * inCallback);
	void		init(void * inPtr, size_t inSize, bool inAssumePtrOwnership, CPipeCallback * inCallback);

	long		readSeek(long inOffset, int inSelector);
	long		readPosition(void) const;
	long		writeSeek(long inOffset, int inSelector);
	long		writePosition(void) const;
	void		readChunk(void * outBuf, size_t & ioSize, bool & outEOF);
	void		writeChunk(const void * inBuf, size_t inSize, bool inFlush);
	void		flushRead(void);
	void		flushWrite(void);
	void		reset(void);
	void		overflow();
	void		underflow(long, bool&);

private:
	long		seek(long inOffset, int inSelector);

	Ptr					fPtr;
	long					fOffset;
	long					fEnd;
	CPipeCallback *	fCallback;
	bool					fPtrIsOurs;
};


/*------------------------------------------------------------------------------
	C R e f P i p e
------------------------------------------------------------------------------*/

class CRefPipe : public CPtrPipe
{
public:
				CRefPipe();
				~CRefPipe();

	void		initSink(size_t inSize, RefArg inRef, CPipeCallback * inCallback);
	void		initSource(RefArg inRef, CPipeCallback * inCallback);

	Ref		ref(void) const;

private:
//	int32_t		f14;
	RefStruct	fTheRef;
};
inline Ref	CRefPipe::ref(void) const { return fTheRef; }


/*------------------------------------------------------------------------------
	C S t d I O P i p e
------------------------------------------------------------------------------*/

class CStdIOPipe : public CPipe
{
public:
				CStdIOPipe(const char * inFilename, const char * inMode);
				~CStdIOPipe();

	long		readSeek(long inOffset, int inSelector);
	long		readPosition(void) const;
	long		writeSeek(long inOffset, int inSelector);
	long		writePosition(void) const;
	void		readChunk(void * outBuf, size_t & ioSize, bool & outEOF);
	void		writeChunk(const void * inBuf, size_t inSize, bool inFlush);
	void		flushRead(void);
	void		flushWrite(void);
	void		reset(void);
	void		overflow();
	void		underflow(long, bool&);

private:
	long		seek(long inOffset, int inSelector);
	void		flush(void);

	FILE *	fFile;
	int		fState;
};


#endif	/* __PIPES_H */
