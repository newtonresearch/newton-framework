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
#if 0
/*------------------------------------------------------------------------------
	P F l a t t e n P t r
------------------------------------------------------------------------------*/

PROTOCOL_IMPL_SOURCE_MACRO(PFlattenPtr)

Ptr
PFlattenPtr::translate(void * inContext, CPipeCallback * ioCallback)
{
	Ptr sp44 = NULL;
	Handle sp40 = NULL;
	if (inContext)
	{
		//sp-40
		CPtrPipe pipe;		//sp28
		CObjectWriter writer(inContext->x00, pipe, NO);	//sp00
		ArrayIndex r6 = inContext->offset + writer.size();
		if (inContext->allocHandle)
		{
			sp40 = NewHandle(r6);
			if (sp40 == NULL)
				ThrowErr(exTranslator, MemError());
			HLock(sp40);
			sp44 = *sp40;
		}
		else
		{
			sp44 = NewPtr(r6);
			if (sp44 == NULL)
				ThrowErr(exTranslator, MemError());
		}
		pipe.init(sp44, r6, NO, ioCallback);
		if (inContext->offset > 0)
			pipe.writeSeek(inContext->offset, kSeekFromBeginning);
		newton_try
		{
			writer.write();
		}
		newton_catch_all
		{
			if (sp40)
				FreeHandle(sp40);
			else if (sp44)
				FreePtr(sp44);
		}
		end_try;
		if (sp40)
		{
			HUnlock(sp40);
			return sp40;		// obviously we don’t really want to do this in a non-Handle world
		}
	}
	return sp44;
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
		CPtrPipe pipe;
		pipe.init(inContext->x00, inContext->x04, NO, ioCallback);

		CObjectReader reader(pipe, inContext->x08);
		obj = reader.read();
	}
	return obj;
}

#pragma mark -

/*------------------------------------------------------------------------------
	P F l a t t e n R e f
------------------------------------------------------------------------------*/

PROTOCOL_IMPL_SOURCE_MACRO(PFlattenRef)

Ref
PFlattenRef::translate(void * inContext, CPipeCallback * ioCallback)
{
	RefVar obj;
	if (inContext)
	{
		CRefPipe pipe;
		CObjectWriter writer(inContext->x00, pipe, NO);
		pipe.initSink(writer.size(), inContext->x04, ioCallback);

		writer.write();
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
		CRefPipe pipe;
		pipe.initSource(inContext, ioCallback);

		CObjectReader reader(pipe, inContext->x04);	// Ref options
		if (inContext->x08)
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

#pragma mark -

/*------------------------------------------------------------------------------
	P S t r e a m O u t R e f
------------------------------------------------------------------------------*/

PROTOCOL_IMPL_SOURCE_MACRO(PStreamOutRef)

Ref
PStreamOutRef::translate(void * inContext, CPipeCallback * ioCallback)
{
	RefVar obj;
	if (inContext)
	{
		CEndpointPipe pipe;
		pipe.init(inContext->x04, 0, 512, inContext->x08, inContext->x0C, ioCallback);

		CObjectWriter	writer(inContext->x00, pipe, NO);
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
		CEndpointPipe pipe;
		pipe.init(inContext->x04, 512, 0, inContext->x08, inContext->x0C, ioCallback);

		CObjectReader	reader(pipe, inContext->x00);
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

Ref
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

Ref
POptionDataOut::translate(void * inContext, CPipeCallback * ioCallback)
{
	RefVar obj;
	if (inContext)
	{
		obj = convertToOptionArray(inContext->x04, inContext->x00, inContext->x08);
		return inContext->x04;
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
	P S c r i p t D a t a I n
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
