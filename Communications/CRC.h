/*
	File:		CRC.h

	Contains:	CRC16 declarations (used by framed async serial communications tools).

	Written by:	Newton Research Group, 2009.
*/

#if !defined(__CRC16_H)
#define __CRC16_H 1


/*--------------------------------------------------------------------------------
	CRC16
--------------------------------------------------------------------------------*/

struct CRC16
{
				CRC16();

	void		reset(void);
	void		computeCRC(UByte inChar);
	void		computeCRC(CBufferList& inData);
	void		computeCRC(UByte * inData, size_t inSize);
	void		get(void);

	UByte		crc16[2];
	ULong		workingCRC;
};

inline CRC16::CRC16() { reset(); }

#endif	/* __CRC16_H */
