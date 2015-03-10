/*
	File:		InputStreams.mm

	Contains:	Input streams for the NewtonScript compiler.
		 NOTE:	The original uses CR line endings rather than LF.

	Written by:	Newton Research Group.
*/

#include "Objects.h"
#include "InputStreams.h"
#include "Unicode.h"


/*------------------------------------------------------------------------------
	C I n p u t S t r e a m
------------------------------------------------------------------------------*/

CInputStream::CInputStream()
{
	strcpy(fName, "unknown");
}


CInputStream::~CInputStream()
{ }


void
CInputStream::setFileName(const char * inName)
{
	strncpy(fName, inName, sizeof(fName)-1);
}

#pragma mark -

/*------------------------------------------------------------------------------
	C S t r i n g I n p u t S t r e a m
------------------------------------------------------------------------------*/

CStringInputStream::CStringInputStream(RefArg inStr)
{
	fStr = inStr;
	fIndex = 0;
}


UniChar
CStringInputStream::getch(void)
{
	if (end())
		return EOF;

	UniChar *	str = (UniChar *)BinaryData(fStr);
	return str[fIndex++];
}


#if defined(correct)
void
CStringInputStream::ungetch(UniChar inCh)
{
	if (inCh != (UniChar)EOF)
	{
		if (fIndex > 0)
			fIndex--;
	}
}
#endif


bool
CStringInputStream::end(void)
{
	return fIndex >= (Length(fStr) / sizeof(UniChar)) - 1;
}

#pragma mark -

/*------------------------------------------------------------------------------
	C S t d i o I n p u t S t r e a m
------------------------------------------------------------------------------*/

CStdioInputStream::CStdioInputStream(FILE * inFile, const char * inFilename)
{
	fRef = inFile;
	strncpy(fName, inFilename, sizeof(fName)-1);
}


UniChar
CStdioInputStream::getch(void)
{
	char		c = getc(fRef);
//	original handles two-byte character encoding - maybe we should handle UTF-8?
	if (c == EOF)
		return EOF;

	UniChar	ch[1];
	ConvertToUnicode(&c, ch, 1);
	return ch[0];
}


#if defined(correct)
void
CStdioInputStream::ungetch(UniChar inCh)
{
	char		c;
	UniChar	ch[1];
	ch[0] = inCh;
	ConvertFromUnicode(ch, &c, 1);
	ungetc(c, fRef);
}
#endif


bool
CStdioInputStream::end(void)
{
	return feof(fRef);
}

