/*
	File:		CommOptions.cc

	Contains:	Options defintions for CommManagerInterface

	Copyright:	Newton Resrearch Group, 2015.
*/

#include "CommOptions.h"
#include "Transport.h"

/* -------------------------------------------------------------------------------
	CCMOEndpointName
		MakeByName string for the CEndpoint object
------------------------------------------------------------------------------- */

CCMOEndpointName::CCMOEndpointName()
	:	COption(kOptionType)
{
	fLabel = kCMOEndpointName;
	fLength = sizeof(fClassName);
	// set default endpoint
	memcpy(fClassName, "CSerialEndpoint", 16);
}


/* -------------------------------------------------------------------------------
	CCMOTransportInfo
		info used by endpoints and endpoint clients
------------------------------------------------------------------------------- */

CCMOTransportInfo::CCMOTransportInfo()
	:	COption(kOptionType)
{
	fLabel = kCMOTransportInfo;
	fLength = 32;

	serviceType = 0;
	flags = 0;

	tdsu = T_INVALID;
	etdsu = T_INVALID;
	connect = T_INVALID;
	discon = T_INVALID;
	addr = T_INVALID;
	opt = T_INVALID;
}


/* -------------------------------------------------------------------------------
	CCMOServiceIdentifier
		info used by endpoints and endpoint clients
------------------------------------------------------------------------------- */

CCMOServiceIdentifier::CCMOServiceIdentifier()
	:	COption(kOptionType)
{
	fLabel = kCMOServiceIdentifier;
	fLength = 8;

	fServiceId = 0;
	fPortId = kNoId;

	setAsService();
}
