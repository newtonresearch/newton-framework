/*
	File:		RefIO.h

	Contains:	Frame source and sink protocols.

	Written by:	Newton Research Group, 2007.
*/

#if !defined(__REFIO_H)
#define __REFIO_H 1

#include "Protocols.h"
#include "Pipes.h"


/*------------------------------------------------------------------------------
	P F r a m e S o u r c e
	P-class interface.
------------------------------------------------------------------------------*/

PROTOCOL PFrameSource : public CProtocol
{
public:
	static PFrameSource *	make(const char * inName);
	VIRTUAL void		destroy(void) ENDVIRTUAL;

	VIRTUAL Ref			translate(void * inParms, CPipeCallback * inCallback) ENDVIRTUAL;
};


/*------------------------------------------------------------------------------
	P U n f l a t t e n P t r
------------------------------------------------------------------------------*/

PROTOCOL PUnflattenPtr : public PFrameSource
	PROTOCOLVERSION(1.0)
{
public:
	PROTOCOL_IMPL_HEADER_MACRO(PUnflattenPtr)

	PUnflattenPtr *	make(void);
	void			destroy(void);

	Ref			translate(void * inParms, CPipeCallback * inCallback);
};

struct UnflattenPtrParms
{
	UChar *	x00;
	long		x04;
	RefVar	x08;
};


/*------------------------------------------------------------------------------
	P U n f l a t t e n R e f
------------------------------------------------------------------------------*/

PROTOCOL PUnflattenRef : public PFrameSource
	PROTOCOLVERSION(1.0)
{
public:
	PROTOCOL_IMPL_HEADER_MACRO(PUnflattenRef)

	PUnflattenRef *	make(void);
	void			destroy(void);

	Ref			translate(void * inParms, CPipeCallback * inCallback);
};


/*------------------------------------------------------------------------------
	P S t r e a m I n R e f
------------------------------------------------------------------------------*/

PROTOCOL PStreamInRef : public PFrameSource
	PROTOCOLVERSION(1.0)
{
public:
	PROTOCOL_IMPL_HEADER_MACRO(PStreamInRef)

	PStreamInRef *	make(void);
	void			destroy(void);

	Ref			translate(void * inParms, CPipeCallback * inCallback);
};


/*------------------------------------------------------------------------------
	P F r a m e S i n k
	P-class interface.
------------------------------------------------------------------------------*/

PROTOCOL PFrameSink : public CProtocol
{
public:
	static PFrameSink *	make(const char * inName);
	VIRTUAL void		destroy(void) ENDVIRTUAL;

	VIRTUAL Ptr			translate(void * inParms, CPipeCallback * inCallback) ENDVIRTUAL;
};

class COptionArray;
class PScriptDataOut;
struct FrameSinkParms
{
	COptionArray *		x00;
	RefVar				x04;
	PScriptDataOut *	x08;
};


/*------------------------------------------------------------------------------
	P F l a t t e n P t r
------------------------------------------------------------------------------*/

PROTOCOL PFlattenPtr : public PFrameSink
	PROTOCOLVERSION(1.0)
{
public:
	PROTOCOL_IMPL_HEADER_MACRO(PFlattenPtr)

	PFlattenPtr *	make(void);
	void			destroy(void);

	Ptr			translate(void * inParms, CPipeCallback * inCallback);
};

struct FlattenPtrParms
{
	RefVar	x00;
	bool		allocHandle;
	long		offset;
};


/*------------------------------------------------------------------------------
	P F l a t t e n R e f
------------------------------------------------------------------------------*/

PROTOCOL PFlattenRef : public PFrameSink
	PROTOCOLVERSION(1.0)
{
public:
	PROTOCOL_IMPL_HEADER_MACRO(PFlattenRef)

	PFlattenRef *	make(void);
	void			destroy(void);

	Ptr			translate(void * inParms, CPipeCallback * inCallback);
};


/*------------------------------------------------------------------------------
	P S t r e a m O u t R e f
------------------------------------------------------------------------------*/

PROTOCOL PStreamOutRef : public PFrameSink
	PROTOCOLVERSION(1.0)
{
public:
	PROTOCOL_IMPL_HEADER_MACRO(PStreamOutRef)

	PStreamOutRef *	make(void);
	void			destroy(void);

	Ptr			translate(void * inParms, CPipeCallback * inCallback);
};


#endif	/* __REFIO_H */
