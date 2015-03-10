/*
	File:		MNPService.h

	Contains:	MNP communications tool.

	Written by:	Newton Research Group, 2008.
*/

#if !defined(__MNPSERVICE_H)
#define __MNPSERVICE_H 1

#include "CMService.h"

/*--------------------------------------------------------------------------------
	CMNPService
	Communications Manager service for the MNP Serial tool.
--------------------------------------------------------------------------------*/

PROTOCOL CMNPService : public CCMService
	PROTOCOLVERSION(1.0)
{
public:
	PROTOCOL_IMPL_HEADER_MACRO(CMNPService)

	CMNPService *	make(void);
	void			destroy(void);

	NewtonErr	start(COptionArray * inOptions, ULong inId, CServiceInfo * info);		// start the service
	NewtonErr	doneStarting(CEvent * inEvent, size_t inSize, CServiceInfo * info);	// called back when done starting

private:
// size +10
};


#endif	/* __MNPSERVICE_H */
