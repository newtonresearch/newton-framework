/*
	File:		InputStreams.h

	Contains:	Input streams for the NewtonScript compiler.

	Written by:	Newton Research Group.
*/

#if !defined(__INPUTSTREAMS_H)
#define __INPUTSTREAMS_H 1

/*------------------------------------------------------------------------------
	C I n p u t S t r e a m
------------------------------------------------------------------------------*/

class CInputStream
{
public:
							CInputStream();
	virtual				~CInputStream();

	virtual UniChar	getch(void) = 0;
#if defined(correct)
	virtual void		ungetch(UniChar inCh) = 0;		// no longer necessary
#endif

	void					setFileName(const char * inName);
	const char *		fileName(void) const;

protected:
	char		fName[256];
};

inline const char *	CInputStream::fileName(void) const
{ return fName; }


/*------------------------------------------------------------------------------
	C S t r i n g I n p u t S t r e a m
------------------------------------------------------------------------------*/

class CStringInputStream : public CInputStream
{
public:
							CStringInputStream(RefArg inStr);

	virtual UniChar	getch(void);
#if defined(correct)
	virtual void		ungetch(UniChar inCh);
#endif

protected:
	bool					end(void);

	RefVar	fStr;
	ULong		fIndex;
};


/*------------------------------------------------------------------------------
	C S t d i o I n p u t S t r e a m
------------------------------------------------------------------------------*/

class CStdioInputStream : public CInputStream
{
public:
							CStdioInputStream(FILE * inFile, const char * inFilename);

	virtual UniChar	getch(void);
#if defined(correct)
	virtual void		ungetch(UniChar inCh);
#endif

protected:
	bool					end(void);

	FILE *	fRef;
};

#endif	/* __INPUTSTREAMS_H */
