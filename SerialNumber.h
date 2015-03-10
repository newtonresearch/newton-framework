/*
	File:		SerialNumber.h

	Contains:	ROM serial number emulation.
					We use the Mac serial number; which may actually include alpha characters.
					See https://developer.apple.com/library/mac/technotes/tn1103/_index.html

	Written by:	Newton Research Group, 2014.
*/

#if !defined(__SERIALNUMBER_H)
#define __SERIALNUMBER_H

#include "Newton.h"

#define kSerialNumberBufferLength 64


struct SerialNumber
{
	ULong word[2];
};


class CSerialNumberROM
{
public:
	void			init(void);
	NewtonErr	getSystemSerialNumber(SerialNumber * outSerialNumber);

private:
	int			nextChar(char * inStr);
	int			nextXDigit(char * inStr);
	ArrayIndex	fIndex;
};

extern CSerialNumberROM * GetSerialNumberROMObject(void);

#endif /* __SERIALNUMBER_H */
