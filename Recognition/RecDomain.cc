/*
	File:		RecDomain.cc

	Contains:	Recognition domain implementation.

	Written by:	Newton Research Group.
*/

#include "RecDomain.h"
#include "Controller.h"

extern void		UnbufferStroke(CRecStroke * inStroke);


ULong		gRecognitionTimeout;				// 0C101850

LRect		gGSScreenRect;						// 0C104C94
int		gPixScreenRectInset;				// 0C104CA4
int		gPixMaxContextGravity;			// 0C104CA8
int		gPixMaxCollapseSize;				// 0C104CB8
int		gPixMaxSmallDist;					// 0C104CBC
int		gPixMaxClosedDist;				// 0C104CC0
int		gPixMaxConnectDist;				// 0C104CC4
int		gPixMinConnectDist;				// 0C104CC8
int		gPixMinKinkDist;					//	0C104CCC
int		gPixMinRLineOutTolerance;		// 0C104CD0
int		gPixMaxRLineOutTolerance;		// 0C104CD4
int		gPixMaxAvgLenForSmallDists;	// 0C104CD8
int		gPixMinAvgLenForSmallDists;	// 0C104CDC
int		gPixSomeMagicThreshold;			// 0C104CE0
int		gPixLargeInitialValue;			// 0C104CE4
int		gPixLowBlobThreshold;			// 0C104CE8
int		gPixHighBlobThreshold;			// 0C104CEC
int		gPixMaxSizeTrendSlop;			// 0C104CF0
int		gPixPtOnLineSlop;					// 0C104CF4
int		gSmpMinClosedShapePts;			// 0C104CF8
int		gSmpMinSmallDistRun;				// 0C104CFC


LPoint	g0C104D0C;
ULong		g0C104D14;


/*--------------------------------------------------------------------------------
	C R e c D o m a i n

	The CRecDomain class implements a recognizer.
	CRecDomain being the base class implements only default functions; sub-classes
	implement useful functions.

00  TWRecDomain::Dispose()
    TDomain::Dump(TMsg*)
    TDomain::SizeInBytes()
    TRecObject::CopyInto(TRecObject*)
10  TWRecDomain::Classify(TUnit*)
    TWRecDomain::Reclassify(TUnit*)
    TWRecDomain::Group(TUnit*, dInfoRec*)
    TDomain::PreGroup(TUnit*)
20  TDomain::DumpName(TMsg*)
    TDomain::PruneDictionary(TUnit*)
    TDomain::PruneConstraints(TUnit*)
    TWRecDomain::DomainParameter(unsigned long, unsigned long, unsigned long)
30  TWRecDomain::SetParameters(char**)
    TDomain::InvalParameters()
    TDomain::ConfigureSubDomain(TRecArea*)
    TDomain::CompleteUnit()
40  TWRecDomain::ConfigureArea(RefArg, unsigned long)
--------------------------------------------------------------------------------*/

#pragma mark Initialization

CRecDomain::CRecDomain()
{ }

CRecDomain::~CRecDomain()
{ }


/*--------------------------------------------------------------------------------
	The controller calls this method to create the single instance of the domain.

	This is a static method, since there is no instance of the domain to call until one has been made.
	Because it is not a virtual method, domains that are subclassed from CRecDomain are free
	to change the interface to this method.

	A domain's make method should call iRecDomain to perform all initialization.
	The parameters passed to iRecDomain are identical to those passed to make.
	Because it is not a virtual method, subclasses can define their own interfaces to this method.

	Args:		inController		the controller instance so that the new domain can make calls to the controller.
				inUnitType			the type of unit that the domain will recognize.
				inName				the name of the domain
	Return:	a domain instance
--------------------------------------------------------------------------------*/

CRecDomain *
CRecDomain::make(CController * inController, RecType inUnitType, const char * inName)
{
	CRecDomain * domain = new CRecDomain;
	XTRY
	{
		XFAIL(domain == NULL)
		XFAILIF(domain->iRecDomain(inController, inUnitType, inName) != noErr, domain->release(); domain = NULL;)
	}
	XENDTRY;
	return domain;
}


/*--------------------------------------------------------------------------------
	Initialize the domain.
	Args:		inController
				inUnitType
				inName
	Return:	--
--------------------------------------------------------------------------------*/

NewtonErr
CRecDomain::iRecDomain(CController * inController, RecType inUnitType, const char * inName)
{
	iRecObject();

	fController = inController;
	fUnitType = inUnitType;
	fDomainName = inName;
	fDelay = 0;
	fPieceTypes = CTypeList::make();
	fParameters = NULL;
//	NamePtr(this, 'CDom');
	return noErr;
}


void
CRecDomain::release(void)
{
	fPieceTypes->release();
	CRecObject::release();
}

#pragma mark Debug

/*--------------------------------------------------------------------------------
	Return the size of this domain instance.
	Args:		--
	Return:	the size in bytes
--------------------------------------------------------------------------------*/

size_t
CRecDomain::sizeInBytes(void)
{
	size_t thisSize = fPieceTypes->sizeInBytes();
	thisSize += (sizeof(CRecDomain) - sizeof(CRecObject));	// original doesn’t need sizeof evaluation b/c it uses dynamic Ptr size
	return CRecObject::sizeInBytes() + thisSize;
}


void
CRecDomain::dump(CMsg * outMsg)
{
	char  str[100];
	dumpName(outMsg);
	if (gVerbose)
	{
		sprintf(str, "flags: %d, ", fFlags);
		outMsg->msgStr(str);
		sprintf(str, "%d PieceTypes", fPieceTypes->count());
		outMsg->msgStr(str);
		for (ArrayIndex i = 0; i < fPieceTypes->count(); ++i)
		{
			outMsg->msgStr("  ");
			outMsg->msgType(fPieceTypes->getType(i));
		}
	}
	outMsg->msgLF();
}


void
CRecDomain::dumpName(CMsg * outMsg)
{
	char  str[100];
	sprintf(str, "DOMAIN: %s ", fDomainName);
	outMsg->msgStr(str);
	outMsg->msgChar('(');
	outMsg->msgType(fUnitType);
	outMsg->msgChar(')');
}

#pragma mark Domain

/*--------------------------------------------------------------------------------
	A domain should call addPieceType to specify a unit type that it accepts as input.
	Args:		inSubType
	Return:	--
--------------------------------------------------------------------------------*/

void
CRecDomain::addPieceType(RecType inSubtype)
{
	fPieceTypes->addUnique(inSubtype);
	fPieceTypes->compact();
}


/*--------------------------------------------------------------------------------
	The controller calls group whenever a unit of a type requested by the domain
	as input is received.
	Args:		inPiece
				info
	Return:	long
--------------------------------------------------------------------------------*/

bool
CRecDomain::group(CRecUnit * inPiece, RecDomainInfo * info)
{
	return false;
}


/*--------------------------------------------------------------------------------
	Args:		inUnit
	Return:	long
--------------------------------------------------------------------------------*/

NewtonErr
CRecDomain::preGroup(CRecUnit * inUnit)
{
	return noErr;
}


/*--------------------------------------------------------------------------------
	The controller calls classify when the delay, if specified, has occurred, or
	after the group method calls CSIUnit::endSub.
	Args:		inUnit
	Return:	--
--------------------------------------------------------------------------------*/

void
CRecDomain::classify(CRecUnit * inUnit)
{ }


void
CRecDomain::reclassify(CRecUnit * inUnit)
{ }


long
CRecDomain::pruneDictionary(CRecUnit * inUnit)
{
	return 0;
}


long
CRecDomain::pruneConstraints(CRecUnit * inUnit)
{
	return 0;
}


bool
CRecDomain::setParameters(Ptr inParms)
{
	if (fParameters != inParms)
	{
		fParameters = inParms;
		return true;
	}
	return false;
}


void
CRecDomain::invalParameters(void)
{
	fParameters = (Ptr) 0xFFFFFFFF;
}


void
CRecDomain::domainParameter(ULong inArg1, OpaqueRef inArg2, OpaqueRef inArg3)
{
	REPprintf("domainParameter(%u, %u, %u)", inArg1, inArg2, inArg3);
	if (inArg1 == 0)
		*(ULong *)inArg2 = 0;
}


void
CRecDomain::configureSubDomain(CRecArea * inArea)
{ }


long
CRecDomain::completeUnit(void)
{
	return 0;
}


bool
CRecDomain::vUnitInClass(RecType inType1, RecType inType2)
{
	return inType2 == 'WORD'
		 && (inType1 == 'XRWR' || inType1 == 'KANJ' || inType1 == 'WREC');
}

