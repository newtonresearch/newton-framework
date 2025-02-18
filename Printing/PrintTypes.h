/*
	File:		PrintTypes.h

	Copyright:	� 1993-1995 by Apple Computer, Inc., all rights reserved.

	Derived from v9 internal.

*/


#ifndef __PRINTTYPES_H
#define __PRINTTYPES_H


#ifndef __CONFIGPRINTING_H
#include "ConfigPrinting.h"
#endif

#ifndef __NEWTON_H
#include "Newton.h"
#endif

#ifndef __NEWTQD_H
#include "NewtQD.h"
#endif

#ifndef __OBJECTS_H
#include "Objects.h"
#endif

#ifndef __PARTHANDLER_H
#include "PartHandler.h"
#endif

#ifndef __FIXEDMATH_H
#include "FixedMath.h"
#endif

#ifndef __USERABORT_H
#include "UserAbort.h"
#endif


//  the type with which to declare print driver packages

const PartType kPrinterDriverPartType	= 'prnt';


//  Various structures used by print drivers


class TPrinter;


typedef enum {
	kBest = 0,
	kFaster
} PrQuality;


typedef struct	{					// print connection args stored here
	long		fVersion;			// this is version 1
	RefStruct	fConnectInfo;		// the entire connect struct
	RefStruct	fPaperSize;			// paper size symbol, pulled from the connectinfo
	Boolean		fManualFeed;		// always false
	PrQuality	fPrintQuality;		// always kBest;
	Boolean		fPortrait;			// false when landscape printing
} PrintConnect;


const Fixed kFixed72 = Long2Fixed(72);

typedef	struct PrintPort {
	GrafPort	port;
	TPrinter*	prObject;
} PrintPort;

typedef struct	PrPageInfo {
	FPoint		printerDPI;				/* printer's DPI	*/
	Point		printerPageSize;		/* exact number of printable dots	*/
} PrPageInfo;

typedef struct	ScalerInfo {			/* info about generic scaling operation	*/
	Rect			fromRect, toRect;
	FPoint			scaleRatios;
} ScalerInfo;


typedef struct DotPrinterPrefs {
	long		minBand;				/* smallest useable band			*/
	long		optimumBand;			/* a good size to try to default	*/
	Boolean		asyncBanding;			/* true if band data sent async		*/
	Boolean		wantMinBounds;			/* true if minrect is useful		*/
} DotPrinterPrefs;


typedef enum {
	kPrProblemFixed = 0,
	kPrProblemNotFixed
} PrProblemResolution;


#endif
