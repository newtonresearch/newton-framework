/*
	File:		PushPopBits.h

	Contains:	Bit twiddling class for LZ compression classes.
	
	Written by:	Newton Research Group, 2006.
					With a little help from Philz, and a disassembly of the DILs.
*/

#if !defined(__PUSHPOPBITS_H)
#define __PUSHPOPBITS_H


class CPushPopper
{
public:
				CPushPopper();
				~CPushPopper();

	void		setReadBuffer(unsigned char * inBuf, size_t inLen);
	void		setWriteBuffer(unsigned char * inBuf, size_t inLen);

	void		restoreBits(int inCount);
	uint32_t	popBits(int inCount);

	void		popString(unsigned char * outBuf, int inCount);
	uint32_t	popFewBits(int inCount);

	void		pushBits(int inCount, uint32_t inValue);
	void		flushBits(void);

	size_t	getByteCount(void)		const;
	size_t	getBitOffset(void)		const;

private:
	size_t				fByteCount;
	size_t				fMaxBytes;
	uint32_t				fSource;
	unsigned char *	fBytePtr;
	int					fBitIndex;
};

inline size_t	CPushPopper::getByteCount(void)		const	{ return fByteCount; }
inline size_t	CPushPopper::getBitOffset(void)		const	{ return fBitIndex; }

#endif	/* __PUSHPOPBITS_H */
