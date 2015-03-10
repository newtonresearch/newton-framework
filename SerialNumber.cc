/*
	File:		SerialNumber.cc

	Contains:	ROM serial number emulation.
					We use the Mac serial number; which may actually include alpha characters.

	Written by:	Newton Research Group, 2014.
*/

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>

#include "SerialNumber.h"
#include "OSErrors.h"


/* -----------------------------------------------------------------------------
	From https://developer.apple.com/library/mac/technotes/tn1103/_index.html
----------------------------------------------------------------------------- */
// Returns the serial number as a CFString.
// It is the caller's responsibility to release the returned CFString when done with it.

void
CopySerialNumber(CFStringRef *serialNumber)
{
	if (serialNumber != NULL)
	{
		*serialNumber = NULL;

		io_service_t platformExpert = IOServiceGetMatchingService(kIOMasterPortDefault,
																					 IOServiceMatching("IOPlatformExpertDevice"));

		if (platformExpert)
		{
			CFTypeRef serialNumberAsCFString = IORegistryEntryCreateCFProperty(platformExpert,
																									CFSTR(kIOPlatformSerialNumberKey),
																									kCFAllocatorDefault, 0);
			if (serialNumberAsCFString)
				*serialNumber = (CFStringRef)serialNumberAsCFString;

			IOObjectRelease(platformExpert);
		}
	}
}


/* -----------------------------------------------------------------------------
	P u b l i c   I n t e r f a c e
----------------------------------------------------------------------------- */

CSerialNumberROM gSerialNumberROM;

CSerialNumberROM *
GetSerialNumberROMObject(void)
{ return &gSerialNumberROM; }


/* -----------------------------------------------------------------------------
	C S e r i a l N u m b e r R O M
----------------------------------------------------------------------------- */

void
CSerialNumberROM::init(void)
{ /* not required */ }


int
CSerialNumberROM::nextChar(char * inStr)
{
	int ch = inStr[fIndex++];
	if (fIndex >= kSerialNumberBufferLength)
		fIndex = 0;
	return ch;
}


int
CSerialNumberROM::nextXDigit(char * inStr)
{
	int ch = nextChar(inStr);
	return ch & 0x0F;
}


NewtonErr
CSerialNumberROM::getSystemSerialNumber(SerialNumber * outSerialNumber)
{
	CFStringRef serNo;
	CopySerialNumber(&serNo);	// 12 alphanumeric chars

	char str[kSerialNumberBufferLength];
	if (CFStringGetCString(serNo, str, kSerialNumberBufferLength, kCFStringEncodingASCII))
	{
		fIndex = 0;
		for (ArrayIndex j = 0; j < 2; j++)	// 2 ULongs => 64-bit serial number
		{
			ULong word = 0;
			for (ArrayIndex i = 0; i < 8; ++i)	// 8 nybls / ULong
			{
				word = (word << 4) | nextXDigit(str);
			}
			outSerialNumber->word[j] = word;
		}
		return noErr;
	}
	return kOSErrCouldNotCreateObject;
}


/* -----------------------------------------------------------------------------
	P l a i n   C   I n t e r f a c e
----------------------------------------------------------------------------- */
#include "Objects.h"
#include "ROMSymbols.h"
extern "C" Ref		FGetSerialNumber(RefArg inRcvr);

Ref
FGetSerialNumber(RefArg inRcvr)
{
//	RefVar huh(inRcvr);	// sic
	RefVar serno(AllocateBinary(SYMA(serialNumber), sizeof(SerialNumber)));
	
	NewtonErr err = GetSerialNumberROMObject()->getSystemSerialNumber((SerialNumber *)BinaryData(serno));
	if (err == noErr)
		return serno;
	return NILREF;
}
