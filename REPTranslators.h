/*
	File:		REPTranslators.h

	Contains:	Read-Evaluate-Print I/O translators.

	Written by:	Newton Research Group.
*/

#if !defined(__REPTRANSLATORS_H)
#define __REPTRANSLATORS_H 1

#include "REP.h"

struct TranslatorInfo
{
	const char *	filename;
	size_t			bufSize;
};

struct StdioTranslatorInfo
{
	FILE *			fref;
	size_t			bufSize;
};


/*------------------------------------------------------------------------------
	P I n T r a n s l a t o r
	P-class interface.
	The protocol upon which all input translators must be based.
------------------------------------------------------------------------------*/

PROTOCOL PInTranslator : public CProtocol
{
public:
	static PInTranslator *	make(const char * inName);
	void			destroy(void);

	NewtonErr	init(void * inContext);
	Timeout		idle(void);
	bool			frameAvailable(void);
	Ref			produceFrame(int inLevel);
};


/*------------------------------------------------------------------------------
	P H a m m e r I n T r a n s l a t o r
	Input from the debugger.
------------------------------------------------------------------------------*/

PROTOCOL PHammerInTranslator : public PInTranslator
	PROTOCOLVERSION(1.0)
{
public:
	PROTOCOL_IMPL_HEADER_MACRO(PHammerInTranslator)

	PHammerInTranslator *	make(void);
	void			destroy(void);

	NewtonErr	init(void * inContext);
	Timeout		idle(void);
	bool			frameAvailable(void);
	Ref			produceFrame(int inLevel);

private:
	FILE *	fileRef;
	char *	fBuf;
	size_t	fBufSize;
	long		f1C;
	long		f20;
};


/*------------------------------------------------------------------------------
	P N u l l I n T r a n s l a t o r
	Input from the bit bucket.
------------------------------------------------------------------------------*/

PROTOCOL PNullInTranslator : public PInTranslator
	PROTOCOLVERSION(1.0)
{
public:
	PROTOCOL_IMPL_HEADER_MACRO(PNullInTranslator)

	PNullInTranslator *	make(void);
	void			destroy(void);

	NewtonErr	init(void * inContext);
	Timeout		idle(void);
	bool			frameAvailable(void);
	Ref			produceFrame(int inLevel);
};


/*------------------------------------------------------------------------------
	P S t d i o I n T r a n s l a t o r
	Input from stdio.
------------------------------------------------------------------------------*/

PROTOCOL PStdioInTranslator : public PInTranslator
	PROTOCOLVERSION(1.0)
{
public:
	PROTOCOL_IMPL_HEADER_MACRO(PStdioInTranslator)

	PStdioInTranslator *	make(void);
	void			destroy(void);

	NewtonErr	init(void * inContext);
	Timeout		idle(void);
	bool			frameAvailable(void);
	Ref			produceFrame(int inLevel);

private:
	FILE *	fileRef;
	char *	fBuf;
	size_t	fBufSize;
};


/*------------------------------------------------------------------------------
	P O u t T r a n s l a t o r
	P-class interface.
	The protocol upon which all output translators must be based.
------------------------------------------------------------------------------*/

PROTOCOL POutTranslator : public CProtocol
{
public:
	static POutTranslator *	make(const char * inName);
	void			destroy(void);

	NewtonErr	init(void * inContext);
	Timeout		idle(void);
	void			consumeFrame(RefArg inObj, int inDepth, int indent);
	void			prompt(int inLevel);
	int			print(const char * inFormat, ...);
	int			vprint(const char * inFormat, va_list args);	// not in original, but required for REPprintf
	int			putc(int inCh);
	void			flush(void);

	void			enterBreakLoop(int);
	void			exitBreakLoop(void);

	void			stackTrace(void * interpreter);
	void			exceptionNotify(Exception * inException);
};


/*------------------------------------------------------------------------------
	P H a m m e r O u t T r a n s l a t o r
	Output to the debugger.
------------------------------------------------------------------------------*/

PROTOCOL PHammerOutTranslator : public POutTranslator
	PROTOCOLVERSION(1.0)
{
public:
	PROTOCOL_IMPL_HEADER_MACRO(PHammerOutTranslator)

	PHammerOutTranslator *	make(void);
	void			destroy(void);

	NewtonErr	init(void * inContext);
	Timeout		idle(void);
	void			consumeFrame(RefArg inObj, int inDepth, int indent);
	void			prompt(int inLevel);
	int			print(const char * inFormat, ...);
	int			vprint(const char * inFormat, va_list args);
	int			putc(int inCh);
	void			flush(void);

	void			enterBreakLoop(int);
	void			exitBreakLoop(void);

	void			stackTrace(void * interpreter);
	void			exceptionNotify(Exception * inException);

private:
	FILE *	fileRef;
	char *	fBuf;
};


/*------------------------------------------------------------------------------
	P N u l l O u t T r a n s l a t o r
	Output to the bit bucket.
------------------------------------------------------------------------------*/

PROTOCOL PNullOutTranslator : public POutTranslator
	PROTOCOLVERSION(1.0)
{
public:
	PROTOCOL_IMPL_HEADER_MACRO(PNullOutTranslator)

	PNullOutTranslator *	make(void);
	void			destroy(void);

	NewtonErr	init(void * inContext);
	Timeout		idle(void);
	void			consumeFrame(RefArg inObj, int inDepth, int indent);
	void			flush(void);
	void			prompt(int inLevel);
	int			print(const char * inFormat, ...);
	int			vprint(const char * inFormat, va_list args);
	int			putc(int inCh);

	void			enterBreakLoop(int);
	void			exitBreakLoop(void);

	void			stackTrace(void * interpreter);
	void			exceptionNotify(Exception * inException);

private:

	bool		fIsInBreakLoop;
};


/*------------------------------------------------------------------------------
	P S t d i o O u t T r a n s l a t o r
	Output to stdio.
------------------------------------------------------------------------------*/

PROTOCOL PStdioOutTranslator : public POutTranslator
	PROTOCOLVERSION(1.0)
{
public:
	PROTOCOL_IMPL_HEADER_MACRO(PStdioOutTranslator)

	PStdioOutTranslator *	make(void);
	void			destroy(void);

	NewtonErr	init(void * inContext);
	Timeout		idle(void);
	void			consumeFrame(RefArg inObj, int inDepth, int indent);
	void			prompt(int inLevel);
	int			print(const char * inFormat, ...);
	int			vprint(const char * inFormat, va_list args);
	int			putc(int inCh);
	void			flush(void);

	void			enterBreakLoop(int);
	void			exitBreakLoop(void);

	void			stackTrace(void * interpreter);
	void			exceptionNotify(Exception * inException);

private:
	FILE *	fileRef;
};


extern PInTranslator *	gREPin;
extern POutTranslator *	gREPout;

#endif	/* __REPTRANSLATORS_H */
