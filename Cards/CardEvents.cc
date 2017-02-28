/*
	File:		CardEvents.cc

	Contains:	PC card event implementation.

	Written by:	Newton Research Group, 2010.
*/

#include "Objects.h"
#include "CardEvents.h"
#include "Funcs.h"
#include "NewtonScript.h"
#include "NewtGlobals.h"
#include "RSSymbols.h"
#include "REPTranslators.h"

#include "AppWorld.h"


/*------------------------------------------------------------------------------
	D a t a
------------------------------------------------------------------------------*/

CNewtCardEventHandler * gCardEventHandler;


/*------------------------------------------------------------------------------
	I n i t i a l i z a t i o n
------------------------------------------------------------------------------*/

Ref
CallNSCardEventHandler(RefArg inBlock, RefArg inArg2)
{
	RefVar args(MakeArray(1));
	SetArraySlot(args, 0, inArg2);
	return NSCallGlobalFn(SYMA(HandleCardEvent), inBlock, args);
}


Ref
CallNSCardEventHandler(RefArg inBlock, RefArg inArg2, RefArg inArg3)
{
	RefVar args(MakeArray(2));
	SetArraySlot(args, 0, inArg2);
	SetArraySlot(args, 1, inArg3);
	return NSCallGlobalFn(SYMA(HandleCardEvent), inBlock, args);
}


Ref
CardEventPrompt(RefArg inBlock, ULong inArg2)
{
	return CallNSCardEventHandler(inBlock, MAKEINT(inArg2));
}


Ref
CardEventPrompt(RefArg inBlock, ULong inArg2, NewtonErr inErr)
{
	return CallNSCardEventHandler(inBlock, MAKEINT(inArg2), MAKEINT(inErr));
}


Ref
FourCharToSymbol(ULong inFourChar)
{
	MAKE_ID_STR(inFourChar,fourChar);
	return MakeSymbol(fourChar);
}


Ref
HandleNewCard(CNewCardEvent * inEvent)
{
	RefVar sp00(MakeArray(0));
	for (ArrayIndex i = 0; i < 4; ++i)
	{
		ULong sym;
		if ((sym = inEvent->f10[i]) != 0)
			AddArraySlot(sp00, FourCharToSymbol(sym));
	}
	return sp00;
}


void
HandleCardEvents()
{
#if !defined(forFramework)
	CNewtCardEventHandler *	eventHandler = new CNewtCardEventHandler;
	eventHandler->init(kCardServerId);
	gCardEventHandler = eventHandler;
#endif
}


#if !defined(forFramework)
#pragma mark -
/*--------------------------------------------------------------------------------
	C N e w t C a r d E v e n t H a n d l e r
--------------------------------------------------------------------------------*/

NewtonErr
CNewtCardEventHandler::init(EventId inEventId, EventClass inEventClass)
{
	NewtonErr err;
	XTRY
	{
		XFAIL(err = CEventHandler::init(inEventId, inEventClass))
		CUNameServer ns;
		OpaqueRef thing, spec;
		XFAIL(err = ns.lookup("cdsv", kUPort, &thing, &spec))
		f14 = new CUPort((ObjectId)thing);
	}
	XENDTRY;
	return err;
}


void
CNewtCardEventHandler::readyToAcceptCardEvents(void)
{
	CCardMessage msg;
	sendServer(100, 0, *gAppWorld->getMyPort(), &msg);
}


void
CNewtCardEventHandler::eventHandlerProc(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent)
{
	if (handleCardEvent((CCardMessage *)inEvent) == noErr)
		setReply(sizeof(CCardMessage));	// 0xB8
}


void
CNewtCardEventHandler::eventCompletionProc(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent)
{
	handleCardEvent((CCardMessage *)inEvent);
}


NewtonErr
CNewtCardEventHandler::handleCardEvent(CCardMessage * inEvent)
{
	NewtonErr err = noErr;
	newton_try
	{
		switch (inEvent->f08)
		{
		case 3:
			if (inEvent->f0C == -8001)
				CardEventPrompt(SYMA(SRAMCardReplaceBattery), inEvent->f10);
			else
				CardEventPrompt(SYMA(MiscCardError), inEvent->f10, inEvent->f0C);
			break;

		case 51:
			CardEventPrompt(SYMA(CardRemoved), inEvent->f10);
			break;

		case 105:
			if ((inEvent->f0C & 0x01) == 0)
				CardEventPrompt(SYMA(SRAMCardReplaceBattery), inEvent->f10);
			else if ((inEvent->f0C & 0x02) == 0)
				CardEventPrompt(SYMA(SRAMCardLowBattery), inEvent->f10);
			break;

		case 111:
			CCardMessage * msg = inEvent;
			RefVar codeblock = inEvent->f20;
			unwind_protect
			{
				RefArg args(MakeArray(1));
				SetArraySlot(args, 0, MAKEINT(inEvent->f0C));
				DoBlock(codeblock, args);
			}
			on_unwind
			{
//				if (codeblock)
//					delete codeblock;
				if (msg)
					delete msg;
			}
			end_unwind;
			break;
		}
		replyServer(inEvent, 2, inEvent->f10, 0);
	}
	newton_catch_all
	{
		Exception * x = CurrentException();
		ExceptionNotify(x);
		gREPout->exceptionNotify(x);
	}
	end_try;
	return NILREF;		// sic!
}


NewtonErr
CNewtCardEventHandler::sendAyncServer(CCardAsyncMsg * inMsg, ULong inArg2)
{
	if (inArg2 != 0)
		return inMsg->sendRPC(f14, GetNewtTaskPort(), 0, NULL);
	else
		return inMsg->send(f14, 0, NULL);
}


NewtonErr
CNewtCardEventHandler::sendServer(ULong inArg1, ULong inArg2, ObjectId inPort, CCardMessage * inEvent)
{
	size_t msgSize;
	CCardMessage msg;
	msg.messageStuff(inArg1, inArg2, inPort);
	return f14->sendRPC(&msgSize, &msg, sizeof(CCardMessage),  &msg, sizeof(CCardMessage));
}


void
CNewtCardEventHandler::replyServer(CCardMessage * inEvent, ULong inArg2, ULong inArg3, ObjectId inPort)
{
	inEvent->messageStuff(inArg2, inArg3, inPort);
}


#pragma mark -

/*--------------------------------------------------------------------------------
	C C a r d M e s s a g e
--------------------------------------------------------------------------------*/

CCardMessage::CCardMessage()
	:	CEvent(kCardServerId)
{
	f2E = 0;
	f2F = 0;
	f30 = 0;
	f34 = 0;
	clear();
}


CCardMessage::~CCardMessage()
{ }


void
CCardMessage::clear(void)
{
	f08 = 0;
	f10 = 0;
	f0C = 0;

	f1C = 0;
	f2C = 0;
	f2D = 1;
	f14 = 0;
	f18 = 0;
	f20 = 0;
	f24 = 0;
	f28 = 0;
	memset(&f38, 0, 128);
}


void
CCardMessage::messageStuff(ULong inArg1, ULong inArg2, ObjectId inPort)
{
	clear();

	f08 = inArg1;
	f0C = inArg2;
	f10 = inPort;
}


#pragma mark -

/*--------------------------------------------------------------------------------
	C C a r d A s y n c M e s s a g e
--------------------------------------------------------------------------------*/

CCardAsyncMsg::CCardAsyncMsg()
{
	free();
}


NewtonErr
CCardAsyncMsg::init(void)
{
	fMsg.init(true);
}


void
CCardAsyncMsg::free(void)
{
//	fMsg.fMsg.fId &= ~0x80000000;	// huh?
	clear();
}


NewtonErr
CCardAsyncMsg::sendRPC(CUPort * inPort, CUPort * inReplyToPort, Timeout inTimeout, CTime * inFutureTimeToSend)
{
	NewtonErr err;
	XTRY
	{
		XFAIL(err = fMsg.setCollectorPort(*inReplyToPort))
		err = inPort->sendRPC(&fMsg, this, sizeof(CCardMessage), this, sizeof(CCardMessage), inTimeout, inFutureTimeToSend);
	}
	XENDTRY;
	return err;
}


NewtonErr
CCardAsyncMsg::send(CUPort * inPort, Timeout inTimeout, CTime * inFutureTimeToSend)
{
	NewtonErr err;
	XTRY
	{
		XFAIL(err = fMsg.setCollectorPort(0))
		err = inPort->send(&fMsg, this, sizeof(CCardMessage), inTimeout, inFutureTimeToSend);
	}
	XENDTRY;
	return err;
}


#pragma mark -

/*--------------------------------------------------------------------------------
	C N e w C a r d A s y n c M e s s a g e
--------------------------------------------------------------------------------*/

CNewCardAsyncMsg::CNewCardAsyncMsg()
{ }


NewtonErr
CNewCardAsyncMsg::init(void)
{
	NewtonErr err;
	XTRY
	{
		clear();
		XFAIL(err = fEvent.init())
		XFAIL(err = fMsg.init(true))
		fEvent.setEvent(kSysEvent_NewICCard);
	}
	XENDTRY;
	return err;
}


void
CNewCardAsyncMsg::clear(void)
{
	CCardMessage::clear();	// do we really need to override this?
}


void
CNewCardAsyncMsg::sendSystemEvent(void)
{
	fEvent.sendSystemEvent(&fMsg, this, sizeof(CCardMessage), this, sizeof(CCardMessage));
}

#endif
