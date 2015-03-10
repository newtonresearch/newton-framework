/*
	File:		TraceEvents.cc

	Contains:	Event history collector implementation.

	Written by:	Newton Research Group, 2010.
*/

#include "TraceEvents.h"


/*--------------------------------------------------------------------------------
	C H i s t o r y C o l l e c t o r
--------------------------------------------------------------------------------*/

class CHistoryCollector : public CEventCollector
	PROTOCOLVERSION(1.0)
{
public:
	PROTOCOL_IMPL_HEADER_MACRO(CHistoryCollector)

	CHistoryCollector *	make(void);
	void			destroy(void);

	NewtonErr	init(unsigned int, char*, char*, int, int);

	void			addAddressReset(void);
	void			addAddress(void);
	void			addDescriptions(EventTraceCauseDesc*, int);
	void			addReset(const void*);
	void			addReset(unsigned char);
	void			addReset(unsigned long);
	void			add(const void*);
	void			add(unsigned char);
	void			add(unsigned long);
	void			collectionControl(int);
};


void
InitEvents(void)
{
//	CHistoryCollector::classInfo()->registerProtocol();
}

