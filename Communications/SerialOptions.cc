/*
	File:		SerialOptions.cc

	Contains:	Serial communications options.

	Written by:	Newton Research Group, 2009.
*/

#include "SerialOptions.h"
#include "CommOptions.h"

/*--------------------------------------------------------------------------------
	CCMOFramingParms
--------------------------------------------------------------------------------*/

CCMOFramingParms:: CCMOFramingParms()
{
	setLabel(kCMOFramingParms);
	SETOPTIONLENGTH(CCMOFramingParms);

	fEscapeChar = kDefaultFramingChar;
	fEOMChar = kDefaultEOMChar;
	fDoHeader = true;
	fDoPutFCS = true;
	fDoGetFCS = true;
}

#pragma mark -

/*--------------------------------------------------------------------------------
	CCMOFramedAsyncStats
--------------------------------------------------------------------------------*/

CCMOFramedAsyncStats::CCMOFramedAsyncStats()
{
	setLabel(kCMOFramedAsyncStats);
	SETOPTIONLENGTH(CCMOFramedAsyncStats);

	fPreHeaderByteCount = 0;
};

