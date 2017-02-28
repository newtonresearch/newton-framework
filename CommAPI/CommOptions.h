/*
	File:		CommOptions.h

	Contains:	Options defintions for CommManagerInterface

	Copyright:	� 1992-1994 by Apple Computer, Inc., all rights reserved.
*/

#if !defined(__COMMOPTIONS_H)
#define __COMMOPTIONS_H 1


#ifndef FRAM

#ifndef	__NEWTON_H
#include "Newton.h"
#endif

#ifndef __OPTIONARRAY_H
#include "OptionArray.h"
#endif

#define SETOPTIONLENGTH(oClass)		this->setLength(sizeof(oClass) - sizeof(COption))

#endif // notFRAM


/*--------------------------------------------------------------------------------
	Option definitions and selectors for Newton

	NOTE:  naming conventions

	kCMOxxxx	CommManager option, published in this file
	kCMSxxxx	CommManager service, published in CommServices.h
	kCMAxxxx	CommManager address, published in CommServices.h
	kCMCxxxx	CommManager configuration

	EXAMPLE:

	#define kCMOExampleOption		'exap'

	class CCMOExampleOption : public COption
	{
	public:
					CCMOExampleOption();

		ULong		fSpeed;				// speed of interface
		bool		fFast;				// fast or slow
	};

	CCMOExampleOption::CCMOExampleOption()
		: COption(kOptionType)
	{
		SetLabel(kCMOExampleOption);
		SETOPTIONLENGTH(CCMOExampleOption);
	}
--------------------------------------------------------------------------------*/

#ifndef FRAM

#define kCMOAppleTalkATP		'atp '
#define kCMOAppleTalkNBP		'nbp '
#define kCMOAppleTalkADSP		'adsp'
#define kCMOAppleTalkPAP		'pap '

#define kCMOEndpointName		'endp'
#define kCMOTransportInfo		'tinf'

#define kCMOServiceIdentifier	'sid '

#endif // notFRAM


#ifndef FRAM

/*--------------------------------------------------------------------------------
	CCMOEndpointName
		MakeByName string for the CEndpoint object
--------------------------------------------------------------------------------*/

class CCMOEndpointName : public COption
{
public:
				CCMOEndpointName();

	char		fClassName[64];
};


/*--------------------------------------------------------------------------------
	CCMOTransportInfo
		info used by endpoints and endpoint clients
--------------------------------------------------------------------------------*/

class CCMOTransportInfo : public COption
{
public:
				CCMOTransportInfo();

	ULong		serviceType;	// one of TransportServices (see Transport.h)
	ULong		flags;			// one of TransportFlags (see Transport.h)

	// the following are maximum sizes for various data buffers
	// could be T_INFINITE or T_INVALID
	// see XTI documentation for interpretation of values

	int		tdsu;				// maximum data size transport can send/receive
	int		etdsu;			// maximum expedited data size transport can send/receive
	int		connect;			// max connect data
	int		discon;			// max disconnect data
	int		addr;				// max addr size
	int		opt;				// max option size
};


/*--------------------------------------------------------------------------------
	CCMOServiceIdentifier
		info used by endpoints and endpoint clients
--------------------------------------------------------------------------------*/

class CCMOServiceIdentifier : public COption
{
public:
				CCMOServiceIdentifier();

	ULong		fServiceId;		// service type identifier
	ObjectId	fPortId;			// Id of service port (if non-zero, port is used instead of starting service)
};

#endif // notFRAM

#endif	/* __COMMOPTIONS_H */
