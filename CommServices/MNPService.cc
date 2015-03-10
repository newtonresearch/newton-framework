/*
	File:		MNPService.cc

	Contains:	MNP communications service.

	Written by:	Newton Research Group, 2008.
*/

#include "MNPService.h"
#include "MNPTool.h"
#include "EndpointEvents.h"


/*--------------------------------------------------------------------------------
	CMNPService
	Communications Manager service implementation for the MNP Serial tool.
	TO DO		assembler glue
--------------------------------------------------------------------------------*/

PROTOCOL_IMPL_SOURCE_MACRO(CMNPService)

CMNPService *
CMNPService::make(void)
{ return this; }


void
CMNPService::destroy(void)
{ }


NewtonErr
CMNPService::start(COptionArray * inOptions, ULong inId, CServiceInfo * info)
{
	NewtonErr err;
	XTRY
	{
		CMNPTool	mnpTool(inId);
		XFAIL(err = StartCommTool(&mnpTool, inId, info))
		err = OpenCommTool(info->getPortId(), inOptions, this);
	}
	XENDTRY;
	return err;
}


NewtonErr
CMNPService::doneStarting(CEvent * inEvent, size_t inSize, CServiceInfo * info)
{
	return ((CEndpointEvent *)inEvent)->f08;
}

