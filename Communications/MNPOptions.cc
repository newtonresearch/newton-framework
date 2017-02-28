/*
	File:		MNPOptions.cc

	Contains:	MNP communications options.

	Written by:	Newton Research Group, 2009.
*/

#include "MNPOptions.h"
#include "CommOptions.h"


/*--------------------------------------------------------------------------------
	CCMOMNPAllocate
--------------------------------------------------------------------------------*/

CCMOMNPAllocate::CCMOMNPAllocate()
	:	COption(kOptionType)
{
	setLabel(kCMOMNPAllocate);
	SETOPTIONLENGTH(CCMOMNPAllocate);

	fMNPAlloc = true;
}


/*--------------------------------------------------------------------------------
	CCMOMNPCompression
--------------------------------------------------------------------------------*/

CCMOMNPCompression::CCMOMNPCompression()
	:	COption(kOptionType)
{
	setLabel(kCMOMNPCompression);
	SETOPTIONLENGTH(CCMOMNPCompression);

	fCompressionType = kMNPCompressionV42bis + kMNPCompressionMNP5 + kMNPCompressionNone;
}


/*--------------------------------------------------------------------------------
	CCMOMNPDataRate
--------------------------------------------------------------------------------*/

CCMOMNPDataRate::CCMOMNPDataRate()
	:	COption(kOptionType)
{
	setLabel(kCMOMNPDataRate);
	SETOPTIONLENGTH(CCMOMNPDataRate);

	fDataRate = 2400;
};


/*--------------------------------------------------------------------------------
	CCMOMNPSpeedNegotiation
--------------------------------------------------------------------------------*/

CCMOMNPSpeedNegotiation::CCMOMNPSpeedNegotiation()
	:	COption(kOptionType)
{
	setLabel(kCMOMNPSpeedNegotiation);
	SETOPTIONLENGTH(CCMOMNPSpeedNegotiation);

	fRequestedSpeed = 57600;
};


/*--------------------------------------------------------------------------------
	CCMOMNPStatistics
--------------------------------------------------------------------------------*/

CCMOMNPStatistics::CCMOMNPStatistics()
	:	COption(kOptionType)
{
	setLabel(kCMOMNPStatistics);
	SETOPTIONLENGTH(CCMOMNPStatistics);

	fAdaptValue = 196;
	fLTRetransCount = 0;
	fLRRetransCount = 0;
	fRetransTotal = 0;
	fRcvBrokenTotal = 0;
	fForceAckTotal = 0;
	fRcvAsyncErrTotal = 0;
	fFramesRcvd = 0;
	fFramesXmited = 0;
	fBytesRcvd = 0;
	fBytesXmited = 0;
	fWriteBytesIn = 0;
	fWriteBytesOut = 0;
	fReadBytesIn = 0;
	fReadBytesOut = 0;
	fWriteFlushCount = 0;
}


/*--------------------------------------------------------------------------------
	CCMOMNPDebugConnect
--------------------------------------------------------------------------------*/

CCMOMNPDebugConnect::CCMOMNPDebugConnect()
	:	COption(kOptionType)
{
	setLabel(kCMOMNPDebugConnect);
	SETOPTIONLENGTH(CCMOMNPDebugConnect);

	fARACompatibleMode = false;
	fClass4 = true;
	fStreamModeMax = true;
	fMaxCredit = 8;
	fMaxDataSize = 64;
}
