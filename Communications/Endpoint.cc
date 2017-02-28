/*
	File:		Endpoint.cc

	Contains:	Endpoint protocol interface non-virtual methods.

	Written by:	Newton Research Group, 2016.
*/

#include "Endpoint.h"
#include "OSErrors.h"


/*------------------------------------------------------------------------------
	C E n d p o i n t
	NONVIRTUAL protocol methods.
------------------------------------------------------------------------------*/


// delete the endpoint, but leave the tool running
NONVIRTUAL	ObjectId		deleteLeavingTool(void)
{return 0;}

// called as part of CMGetEndpoint
NONVIRTUAL	NewtonErr	initBaseEndpoint(CEndpointEventHandler* handler)
{return 0;}

NONVIRTUAL	void			setClientHandler(ULong clientHandler)
{}


NONVIRTUAL	NewtonErr	getInfo(CCMOTransportInfo * info)
{return 0;}

NONVIRTUAL	void			getAbortParameters(ULong& eventID, ULong& refCon)
{}


NONVIRTUAL	NewtonErr	easyOpen(ULong clientHandler = 0)
{return 0;}
		// calls default Open/Bind/Connect
NONVIRTUAL	NewtonErr	easyConnect(ULong clientHandler = 0, COptionArray* options = NULL, Timeout timeOut = kNoTimeout)
{return 0;}

NONVIRTUAL	NewtonErr	easyClose(void)
{return 0;}
						// calls default Disconnect/UnBind/Close

NONVIRTUAL	void			destroyBaseEndpoint(void)
{}
		// impls must call this in ::destroy()

// So endpoint clients can use forks
NONVIRTUAL	bool			useForks(bool justDoIt)
{return 0;}
		// returns old state
