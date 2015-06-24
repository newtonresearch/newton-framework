/*
	File:		RefIO.cc

	Contains:	Frame source and sink protocols.

	Written by:	Newton Research Group, 2007.
*/

#include "Objects.h"
#include "RefIO.h"
#include "StreamObjects.h"

DeclareException(exTranslator, exRootException);


void
InitTranslators(void)
{
#if 0
	PFlattenPtr::classInfo()->registerProtocol();
	PUnflattenPtr::classInfo()->registerProtocol();
	PFlattenRef::classInfo()->registerProtocol();
	PUnflattenRef::classInfo()->registerProtocol();
	PStreamInRef::classInfo()->registerProtocol();
	PStreamOutRef::classInfo()->registerProtocol();
	PScriptDataIn::classInfo()->registerProtocol();
	PScriptDataOut::classInfo()->registerProtocol();
	POptionDataIn::classInfo()->registerProtocol();
	POptionDataOut::classInfo()->registerProtocol();
#endif
}


#pragma mark -
/*------------------------------------------------------------------------------
	S t r e a m i n g
	These are for NCX; they’re not bona fide Newton functions.
------------------------------------------------------------------------------*/

long
FlattenRefSize(RefArg inRef)
{
	CPipe *	nilPipe = NULL;
	CObjectWriter	writer(inRef, *nilPipe, YES);
	writer.setCompressLargeBinaries();
	return writer.size();
}


void
FlattenRef(RefArg inRef, CPipe & inPipe)
{
	CObjectWriter	writer(inRef, inPipe, YES);
	writer.setCompressLargeBinaries();
	writer.write();
}


extern Ref GetStores(void);

Ref
UnflattenRef(CPipe & inPipe)
{
	CObjectReader	reader(inPipe, GetArraySlot(GetStores(), 0));
	return reader.read();
}


Ref
UnflattenRefSize(CPipe & inPipe)
{
	CObjectReader	reader(inPipe);
	return reader.size();
}


Ref
UnflattenFile(const char * inFilename)
{
	RefVar	obj;
	if (inFilename != NULL)
	{
		CStdIOPipe		pipe(inFilename, "r");

		CObjectReader	reader(pipe);
		obj = reader.read();
	}
	return obj;
}


#pragma mark -
/*------------------------------------------------------------------------------
	P F l a t t e n P t r
	Flatten a Ref into a Ptr that we allocate.
------------------------------------------------------------------------------*/

PROTOCOL_IMPL_SOURCE_MACRO(PFlattenPtr)

OpaqueRef
PFlattenPtr::translate(void * inContext, CPipeCallback * ioCallback)
{
	// redacted to remove Handle operations
	Ptr ptr = NULL;
	if (inContext)
	{
		FlattenPtrParms * parms = (FlattenPtrParms *)inContext;
		CPtrPipe pipe;
		CObjectWriter writer(parms->ref, pipe, NO);
		ArrayIndex ptrSize = parms->offset + writer.size();
		ptr = NewPtr(ptrSize);
		if (ptr == NULL)
			ThrowErr(exTranslator, MemError());

		pipe.init(ptr, ptrSize, NO, ioCallback);
		if (parms->offset > 0)
			pipe.writeSeek(parms->offset, -1/*kSeekFromBeginning*/);
		newton_try
		{
			writer.write();
		}
		newton_catch_all
		{
			if (ptr)
				FreePtr(ptr), ptr = NULL;
		}
		end_try;
	}
	return (OpaqueRef)ptr;
}


/*------------------------------------------------------------------------------
	P U n f l a t t e n P t r
------------------------------------------------------------------------------*/

PROTOCOL_IMPL_SOURCE_MACRO(PUnflattenPtr)

Ref
PUnflattenPtr::translate(void * inContext, CPipeCallback * ioCallback)
{
	RefVar obj;
	if (inContext)
	{
		UnflattenPtrParms * parms = (UnflattenPtrParms *)inContext;
		CPtrPipe pipe;
		pipe.init(parms->ptr, parms->ptrSize, NO, ioCallback);

		CObjectReader reader(pipe, parms->options);
		obj = reader.read();
	}
	return obj;
}


#pragma mark -
/*------------------------------------------------------------------------------
	P F l a t t e n R e f
	Flatten a Ref into a binary object.
------------------------------------------------------------------------------*/

PROTOCOL_IMPL_SOURCE_MACRO(PFlattenRef)

OpaqueRef
PFlattenRef::translate(void * inContext, CPipeCallback * ioCallback)
{
	RefVar obj;
	if (inContext)
	{
		FlattenRefParms * parms = (FlattenRefParms *)inContext;
		CRefPipe pipe;
		CObjectWriter writer(parms->ref, pipe, NO);
		pipe.initSink(writer.size(), parms->store, ioCallback);

		writer.write();
		obj = pipe.ref();
	}
	return obj;
}


/*------------------------------------------------------------------------------
	P U n f l a t t e n R e f
------------------------------------------------------------------------------*/

PROTOCOL_IMPL_SOURCE_MACRO(PUnflattenRef)

Ref
PUnflattenRef::translate(void * inContext, CPipeCallback * ioCallback)
{
	RefVar obj;
	if (inContext)
	{
		UnflattenRefParms * parms = (UnflattenRefParms *)inContext;

		CRefPipe pipe;
		pipe.initSource(parms->ref, ioCallback);

		CObjectReader reader(pipe, parms->options);
		if (parms->disallowFunctions)
			reader.setFunctionsAllowed(NO);
		newton_try
		{
			obj = reader.read();
		}
		cleanup
		{
			reader.~CObjectReader();
			pipe.~CRefPipe();
		}
		end_try;
	}
	return obj;
}

#if 0
#pragma mark -
/*------------------------------------------------------------------------------
	P S t r e a m O u t R e f
------------------------------------------------------------------------------*/
#include "EndpointPipe.h"

PROTOCOL_IMPL_SOURCE_MACRO(PStreamOutRef)

OpaqueRef
PStreamOutRef::translate(void * inContext, CPipeCallback * ioCallback)
{
	RefVar obj;
	if (inContext)
	{
		StreamOutParms * parms = (StreamOutParms *)inContext;

		CEndpointPipe pipe;
		pipe.init(parms->ep, 0, 512, parms->timeout, parms->framing, ioCallback);

		CObjectWriter	writer(parms->ref, pipe, NO);
		if (ioCallback != NULL)
			ioCallback->f04 = writer.size();
		writer.write();
		pipe.flushWrite();
	}
	return obj;
}


/*------------------------------------------------------------------------------
	P S t r e a m I n R e f
------------------------------------------------------------------------------*/

PROTOCOL_IMPL_SOURCE_MACRO(PStreamInRef)

Ref
PStreamInRef::translate(void * inContext, CPipeCallback * ioCallback)
{
	RefVar obj;
	if (inContext)
	{
		StreamInParms * parms = (StreamInParms *)inContext;

		CEndpointPipe pipe;
		pipe.init(parms->ep, 512, 0, parms->timeout, parms->framing, ioCallback);

		CObjectReader reader(pipe, parms->options);
		newton_try
		{
			obj = reader.read();
		}
		cleanup
		{
			reader.~CObjectReader();
			pipe.~CEndpointPipe();
		}
		end_try;
	}
	return obj;
}


#pragma mark -
/*------------------------------------------------------------------------------
	P S c r i p t D a t a O u t
------------------------------------------------------------------------------*/

PROTOCOL_IMPL_SOURCE_MACRO(PScriptDataOut)

OpaqueRef
PScriptDataOut::translate(void * inContext, CPipeCallback * ioCallback)
{
	RefVar obj;
	if (inContext)
	{
		RefVar sp0C(inContext->x00);
		if (NOTNIL(sp0C))
		{
			NewtonErr	err;	// r6
			long *		r7 = NULL;
			int		i, count;
			r5 = inContext;
			r9 = 0;
			sp08 = NULL;
			sp04 = NULL;
			sp00 = 0;

			unwind_protect
			{
				if (IsArray(sp0C))
				{
					count = Length(sp0C);
					r7 = NewPtr(count*sizeof(long));
					if ((err = MemError()) == noErr)
					{
						*r7 = 0;
						for (i = 0; i < count && err == noErr; sp04 += r7[i], i++)
							err = parseOutputLength(GetArraySlot(sp0C, i), inContext->x04, inContext->x08, &r7[i]);
					}
				}
				else
				{
					err = parseOutputLength(sp0C, inContext->x04, inContext->x08, &sp00));
				}
				if (err)
					ThrowErr(exTranslator, err);

				if (inContext->x0C)
				{
					sp04 = NewHandleClear(sp00 + inContext->x10);
					if (sp04 == NULL)
						ThrowErr(exTranslator, MemError());
					HLock(sp04);
					sp08 = *sp04;
				}
				else
				{
					sp08 = NewPtrClear(sp00 + inContext->x10);
					if (sp08 == NULL)
						ThrowErr(exTranslator, MemError());
				}

				if (r7)
				{
					r10 = inContext->x10;
					
					for (i = 0; i < count && err == noErr; r10 += r7[i], i++)
						err = parseOutput(GetArraySlot(sp0C, i), inContext->x04, inContext->x08, &r7[i], &sp08[i]);
					FreePtr(r7);
				}
				else
				{
					err = parseOutput(sp0C, inContext->x04, inContext->x08, &sp00, &sp08[inContext->x10]);
				}
				if (err)
					ThrowErr(exTranslator, err);

				/*INCOMPLETE*/

			}
			on_unwind
			{
				if (sp04)
					DisposeHandle(sp04);
				else if (sp08)
					free(sp08);
			}
			end_unwind;
			
		}
	}
	return obj;
}

NewtonErr
PScriptDataOut::parseOutputLength(RefArg inArg1, FormType inArg2, long inArg3, long * outLen)
{ return noErr; }

NewtonErr
PScriptDataOut::parseOutput(RefArg inArg1, FormType inArg2, long inArg3, long * outLen, UByte * outArg5)
{ return noErr; }


/*------------------------------------------------------------------------------
	P S c r i p t D a t a I n
------------------------------------------------------------------------------*/

PROTOCOL_IMPL_SOURCE_MACRO(PScriptDataIn)

Ref
PScriptDataIn::translate(void * inContext, CPipeCallback * ioCallback)
{
	RefVar obj;
	if (inContext)
	{
		NewtonErr err = noErr;
		obj = parseInput(inContext->x08, inContext->x0C, inContext->x04, inContext->x00, inContext->x10, &err);
		if (err)
			ThrowErr(exTranslator, err);
	}
	return obj;
}

Ref
PScriptDataIn::parseInput(FormType inType, long, long, bool*, RefArg, NewtonErr * outErr)
{
	
}

#pragma mark -

/*------------------------------------------------------------------------------
	P O p t i o n D a t a O u t
------------------------------------------------------------------------------*/

PROTOCOL_IMPL_SOURCE_MACRO(POptionDataOut)

OpaqueRef
POptionDataOut::translate(void * inContext, CPipeCallback * ioCallback)
{
	RefVar obj;
	if (inContext)
	{
		obj = convertToOptionArray(inContext->x04, inContext->x00, inContext->x08);
		return inContext->x00;
	}
	return noErr;
}

Ref
POptionDataOut::convertToOptionArray(RefArg inArg1, COptionArray * inArg2, PFrameSink * inArg3)
{ return NILREF; }

COption *
POptionDataOut::convertToOption(RefArg inArg1, long & ioArg2, PFrameSink * inArg3)
{ return NULL; }

NewtonErr
POptionDataOut::parseOutput(PFrameSink * inArg1, RefArg inArg2, FormType inArg3, long * ioArg4)
{}


/*------------------------------------------------------------------------------
	P O p t i o n D a t a I n
------------------------------------------------------------------------------*/

PROTOCOL_IMPL_SOURCE_MACRO(POptionDataIn)

Ref
POptionDataIn::translate(void * inContext, CPipeCallback * ioCallback)
{
	if (inContext)
	{
		NewtonErr err = convertFromOptionArray(inContext->x04, inContext->x00, inContext->x08);
		if (err)
			ThrowErr(exTranslator, err);
		return inContext->x04;
	}
	return NILREF;
}

NewtonErr
POptionDataIn::convertFromOptionArray(RefArg, COptionArray*, PFrameSource*)
{}

NewtonErr
POptionDataIn::convertFromOption(RefArg, COption*, PFrameSource*)
{}

NewtonErr
POptionDataIn::parseInput(PFrameSource*, FormType, long, UByte*, RefArg, long*)
{}
#endif
