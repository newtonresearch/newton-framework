/*
	File:		Arbiter.cc

	Contains:	Recognition controller declarations.

	Written by:	Newton Research Group, 2010.
*/

#include "Arbiter.h"
#include "Controller.h"
#include "ShapeRecognizer.h"

extern bool		ClickInProgress(CRecUnit * inUnit);
extern bool		OnlyStrokeWritten(CStrokeUnit * inStroke);
extern ULong	GetGraphicBiasedScore(CSIUnit * inUnit);


int	gLastType;		// 0C104C64


NewtonErr
InitArbiterState(CArbiter * inArbiter)
{
	inArbiter->f04 = CDArray::make(40, 0);
	inArbiter->f08 = CDArray::make(40, 0);
	inArbiter->f0C = CArray::make(40, 0);
	inArbiter->f10 = CArray::make(40, 0);
	inArbiter->f14 = CDArray::make(4, 0);
	inArbiter->f18 = CDArray::make(4, 0);
	inArbiter->f1C = CDArray::make(4, 0);
	inArbiter->f20 = NO;
	inArbiter->f21 = NO;
	inArbiter->f24 = 0;
	return noErr;
}


bool
IsOfType(RecType inType1, RecType inType2)	// anonymous in the original
{
	return (inType1 == inType2) || CRecDomain::vUnitInClass(inType1, inType2);
}


NewtonErr
ArbiterGetUnitStrokes(CSIUnit * inUnit, CDArray * inArray)
{
	NewtonErr err = noErr;
	XTRY
	{
		XFAILIF(inUnit == NULL || inArray == NULL, err = 1;)

		if (inUnit->getType() == 'STRK')
		{
			ULong minStrok = inUnit->minStroke();
			ULong maxStrok = minStrok + 1;			// ensure maxStrok != minStrok
			ArrayIndex i;
			for (i = inArray->count(); i > 0; i--)
			{
				if ((maxStrok = *(ULong *)inArray->getEntry(i-1)) <= minStrok)
					break;
			}
			if (maxStrok != minStrok)
				XFAILIF(inArray->insertEntry(i, (Ptr)&minStrok) == kIndexNotFound, err = 1;)
		}
		else
		{
			for (ArrayIndex i = 0, count = inUnit->subCount(); i < count; ++i)
			{
				XFAIL(err = ArbiterGetUnitStrokes((CSIUnit *)inUnit->getSub(i), inArray))
			}
		}
	}
	XENDTRY;
	return err;
}


int
GetBestInterpretation(CArray * inArray1, CArray * inArray2)
{
	;
}


int
GetRecognitionCase(CRecArea * inArea)
{
	int theCase = 0;
	bool r7 = NO;
	bool r6 = NO;
	bool r10 = NO;
	bool r9 = NO;

	ArrayIterator iter;
	Assoc assoc, * assocPtr = (Assoc *)inArea->arbitrationTypes()->getIterator(&iter);
	for (ArrayIndex i = 0; i < iter.count(); assocPtr = (Assoc *)iter.getNext(), i++)
	{
		assoc = *assocPtr;
		if (IsOfType(assoc.fType, 'SCRB'))
			theCase++;
		else if (IsOfType(assoc.fType, 'GSHP'))
			r7 = YES;
		else if (IsOfType(assoc.fType, 'WORD'))
			r6 = YES;
		// guessing the original set r10, r9 appropriately -- CALC, CLMN?
	}

	if (r7)
		theCase += 2;
	if (r6)
		theCase += 4;
	if (r10)
		theCase += 8;
	if (r9)
		theCase += 16;

	return theCase;
}


void
SetCaseAndTime(CArray * inArray, ULong inArg2)
{
	CGeneralShapeUnit * unit;
	ArrayIterator iter;
	BestMatch match, * matchPtr = (BestMatch *)inArray->getIterator(&iter);

	match = *matchPtr;
	unit = (CGeneralShapeUnit *)match.x00;
	if (gArbiter->getf24() == 0  &&  IsOfType(unit->getType(), 'WRPL'))
		gArbiter->setf24(GetRecognitionCase(unit->getArea()));

	for (ArrayIndex i = 0; i < iter.count(); matchPtr = (BestMatch *)iter.getNext(), i++)
	{
		match = *matchPtr;
		unit = (CGeneralShapeUnit *)match.x00;
		if (!IsOfType(unit->getType(), 'WRPL'))
			break;
		unit->setf70(gArbiter->getf24());
		unit->setf74((i == 0) ? inArg2 : 0);
	}
}


int
ArbitrateByRules(CArray * inArray1, CArray * inArray2, ULong inShapeScore, ULong inNumOfShapes, ULong inWordScore, ULong inNumOfWords)
{return -1;}


bool
ArbitrateEarly(BestMatch * inMatch)
{
	CGeneralShapeUnit * unit = (CGeneralShapeUnit *)inMatch->x00;

	if (IsOfType(unit->getType(), 'SCRB')
	&&  OnlyStrokeWritten((CStrokeUnit *)unit->getSub(0)))
		return YES;

	if (unit->getType() == 'GSHP')
	{
		if (unit->getf74() != 1  ||  unit->getf78() <= 327680  ||  !OnlyStrokeWritten((CStrokeUnit *)unit->getSub(0)))
		{
			CDArray * r9 = unit->getGeneralShape();
			int r7 = 0;
			int r6 = 0;
			bool r10 = unit->testFlags(0x00100000);
			bool r8 = YES;
			if (unit->interpretationCount() > 0)
			{
				switch (unit->getLabel(0))
				{
				case 0:
				case 1:
					r7 = 4;
					break;
				case 4:
				case 6:
					r6 = 5;
					break;
				case  9:
				case 10:
				case 11:
				case 12:
					break;
				default:
					r8 = NO;
					break;
				}
				if (r10)
				{
					r6 = 20;
					r8 = YES;
				}
				if (r9 != NULL)
				{
					if (r7 == 0)
						r7 = r9->count();
				}
				if (r8  &&  (r6 == 0 || r7 <= r6))
				{
					FRect box;
					unit->getBBox(&box);
					if (RectangleHeight(&box) >= 19.0  &&  RectangleWidth(&box) >= 19.0)
						return YES;
				}
			}
		}
	}
	return NO;
}


NewtonErr
ArbitrateWithScrubs(CArray * inArray, CArray * outArray)
{
	NewtonErr err = noErr;
	ArrayIterator iter;
	BestMatch * matchPtr = (BestMatch *)inArray->getIterator(&iter);
	for (ArrayIndex i = 0; i < iter.count(); matchPtr = (BestMatch *)iter.getNext(), i++)
	{
		CSIUnit * unit = matchPtr->x00;
		if (!IsOfType(unit->getType(), 'SCRB')
		&&  !IsOfType(unit->getType(), 'WRPL')
		&&  !unit->testFlags(0x00400000))
		{
			BestMatch * newEntry;
			XFAILIF((newEntry = (BestMatch *)outArray->addEntry()) == NULL, err = 1;)
			*newEntry = *matchPtr;
			if (IsOfType(unit->getType(), 'WORD'))
				gLastType = 2;
			else if (IsOfType(unit->getType(), 'GSHP'))
				gLastType = 1;
			else if (IsOfType(unit->getType(), 'CALC')
				  ||  IsOfType(unit->getType(), 'CLMN'))
				gLastType = 3;
		}
	}
	return err;
}


NewtonErr
UnionStrokes(CDArray * inArray1, CDArray * inArray2, ULong inArg3, CDArray * inArray4, ULong * inArg5, ULong * inArg6)
{
	ArrayIterator iter1, iter2;	// sp20, sp00
	inArray1->getIterator(&iter1);
	inArray4->getIterator(&iter2);
//	for (ArrayIndex i = 0; i < iter2.count(); matchPtr = (BestMatch *)iter2.getNext(), i++)
	{
		;
	}
	return noErr;
}


#pragma mark -
/*------------------------------------------------------------------------------
	C A r b i t e r
------------------------------------------------------------------------------*/

CArbiter *
CArbiter::make(CController * inController)
{
	CArbiter * arbiter;
	XTRY
	{
		XFAIL((arbiter = new CArbiter) == NULL)
		XFAILIF(arbiter->iArbiter(inController) != noErr, delete arbiter; arbiter = NULL;)	// original doesn’t bother
		inController->registerArbiter(arbiter);
	}
	XENDTRY;
	return arbiter;
}


NewtonErr
CArbiter::iArbiter(CController * inController)
{
	fController = inController;
	return InitArbiterState(this);
}


int
CArbiter::waitingForOtherUnits(CRecArea * inArea, BestMatch * inMatch)
{
	XTRY
	{
		int r7 = 1;
		BestMatch * newMatch;
		XFAIL((newMatch = (BestMatch *)f08->addEntry()) == NULL)
		*newMatch = *inMatch;
		long r9 = inArea->getf10();

		CSIUnit * unit;
		ArrayIterator iter;
		BestMatch * matchPtr = (BestMatch *)f08->getIterator(&iter);
		for (ArrayIndex i = 0; i < iter.count(); matchPtr = (BestMatch *)iter.getNext(), i++)
		{
			unit = matchPtr->x00;
			if (!unit->testFlags(0x40000000) && unit->getf24() == r9)
			{
				int r5 = allUnitsPresent(inArea, inMatch);
				XXFAIL(fController->controllerError())
				if (r5 != 0)
					r7 = 0;
				break;
			}
		}
		return r7;
	}
	XENDTRY;
failed:
	fController->signalMemoryError();
	return 0;
}


int
CArbiter::allUnitsPresent(CRecArea * inArea, BestMatch * inMatch)
{
	if (f08->count() <= 1)
		return 0;

	XTRY
	{
		CSIUnit * unit = inMatch->x00;
		unit->setFlags(0x00800000);
		BestMatch * matchPtr;
		XFAIL((matchPtr = (BestMatch *)f0C->addEntry()) == NULL)
		*matchPtr = *inMatch;
		XFAIL(ArbiterGetUnitStrokes(unit, f14))
		int r5 = gatherUnits(inArea->getf14() - 1, YES, f0C);
		XFAIL(fController->controllerError())
		f14->cutToIndex(0);
		unit->unsetFlags(0x00800000);
		return r5;
	}
	XENDTRY;

	fController->signalMemoryError();
	return 1;
}


int
CArbiter::gatherUnits(ULong inArg1, bool inArg2, CArray * inArg3)
{
// r4: r5 sp00 r6
	ULong sp50;	// NOTE! original does this -- could return uninit value
	XTRY
	{
		for (ArrayIndex i = 0; i < f14->count(); ++i)
		{
			ULong * r0;
			XXFAIL((r0 = (ULong *)f18->addEntry()) == NULL)
			*r0 = inArg1;
		}
		ULong r10 = *(ULong *)f14->getEntry(0);
		sp50 = f14->count();
		ULong sp4C = *(ULong *)f14->getEntry(sp50 - 1);
		bool r9, sp00 = 0;

		ArrayIterator iter;	// sp04
		CSIUnit * unit;
		BestMatch match, * matchPtr = (BestMatch *)f08->getIterator(&iter);
		do
		{
			r9 = 0;
			for (ArrayIndex i = 0; i < iter.count(); matchPtr = (BestMatch *)iter.getNext(), ++i)
			{
				match = *matchPtr;
				unit = match.x00;
				if (!unit->testFlags(0x40000000)  &&  r10 <= unit->maxStroke()  &&  sp4C >= unit->minStroke())
				{
					r9 = 1;
					unit->setFlags(0x00800000);
					XXFAIL(ArbiterGetUnitStrokes(unit, f1C))
					XXFAIL(UnionStrokes(f14, f18, inArg1, f1C, &sp50, &sp4C))
					BestMatch * r0;
					XXFAIL((r0 = (BestMatch *)inArg3->addEntry()) == NULL)
					*r0 = match;
					if (sp50 == 0)
						break;
					f1C->cutToIndex(0);
				}
			}
		} while (r9 && sp50 != 0 && sp00 == 0);

		if (inArg2)
		{
			matchPtr = (BestMatch *)f08->getIterator(&iter);
			for (ArrayIndex i = 0; i < iter.count(); matchPtr = (BestMatch *)iter.getNext(), i++)
			{
				unit = matchPtr->x00;
				unit->unsetFlags(0x00800000);
			}
		}
		f1C->cutToIndex(0);
		f18->cutToIndex(0);
		return sp50 == 0;
	}
	XENDTRY;

failed:
	fController->signalMemoryError();
	f1C->cutToIndex(0);
	f18->cutToIndex(0);
	return sp50 == 0;
}


int
CArbiter::arbitrateUnits(CRecArea * inArea)
{
	ArrayIterator iter;	// sp28
	BestMatch match;
	BestMatch * matchPtr;
	CRecUnit * unit;

	XTRY
	{
		if (gController->getf21())
		{
			matchPtr = (BestMatch *)f0C->getIterator(&iter);
			for (ArrayIndex i = 0; i < iter.count(); matchPtr = (BestMatch *)iter.getNext(), i++)
			{
				match = *matchPtr;
				unit = match.x00;
				if (IsOfType(unit->getType(), 'WRPL'))
				{
					BestMatch * r0;
					XXFAIL((r0 = (BestMatch *)f10->addEntry()) == NULL)
					*r0 = match;
				}
			}
		}

		switch (GetRecognitionCase(inArea))
		{
		case 3:
		case 5:
		case 9:
		case 17:
			XXFAIL(ArbitrateWithScrubs(f0C, f10))
			break;
		case 6:
		case 7:
			arbitrateGraphicsWords(f0C);
			XXFAIL(fController->controllerError())
			break;
		default:	// also 4, 8, 16
			XXFAIL(GetBestInterpretation(f0C, f10))
			break;
		}
		XXFAIL(fController->controllerError())
		if (f21)
		{
			matchPtr = (BestMatch *)f10->getIterator(&iter);
			for (ArrayIndex i = 0; i < iter.count(); matchPtr = (BestMatch *)iter.getNext(), i++)
			{
				match = *matchPtr;
				unit = match.x00;
				unit->setFlags(0x00800000);	// has no effect?!!!
			}
			matchPtr = (BestMatch *)f0C->getIterator(&iter);
			for (ArrayIndex i = 0; i < iter.count(); matchPtr = (BestMatch *)iter.getNext(), i++)
			{
				match = *matchPtr;
				unit = match.x00;
				if (!unit->testFlags(0x00800000)
				&&  !IsOfType(unit->getType(), 'SCRB')
				&&  !IsOfType(unit->getType(), 'WRPL'))
				{
					BestMatch * r0;
					XXFAIL((r0 = (BestMatch *)f10->addEntry()) == NULL)
					*r0 = match;
				}
			}
		}
		if (f10->count() > 0)
			return 1;

		matchPtr = (BestMatch *)f0C->getIterator(&iter);
		for (ArrayIndex i = 0; i < iter.count(); matchPtr = (BestMatch *)iter.getNext(), i++)
		{
			fController->markUnits(matchPtr->x00, 0x40400000);
		}

		return 0;
	}
	XENDTRY;

failed:
	fController->signalMemoryError();
	return 0;
}


void
CArbiter::arbitrateGraphicsWords(CArray * inArray)
{
	XTRY
	{
		ULong numOfWords = 0, numOfShapes = 0;
		ULong wordScore = 0,  shapeScore = 0;
		bool sp04 = YES,      sp08 = YES;
		bool sp0C;

		ArrayIterator iter;	// sp10
		CSIUnit * unit;
		BestMatch * matchPtr = (BestMatch *)inArray->getIterator(&iter);
		for (ArrayIndex i = 0; i < iter.count(); matchPtr = (BestMatch *)iter.getNext(), i++)
		{
			unit = matchPtr->x00;
			sp0C = unit->testFlags(0x00400000);
			if (IsOfType(unit->getType(), 'WORD'))
			{
				if (!sp0C)
				{
					UnitInterpretation * interp = unit->getInterpretation(0);
					wordScore += interp->score;
					sp08 = NO;
				}
				else
				{
					wordScore += 10000;
				}
				numOfWords++;
			}
			else if (IsOfType(unit->getType(), 'GSHP'))
			{
				if (!sp0C)
				{
					shapeScore += GetGraphicBiasedScore(unit);
					sp04 = NO;
				}
				else
				{
					shapeScore += 10000;
				}
				numOfShapes++;
			}
		}
		if (numOfWords > 0)
			wordScore /= numOfWords;
		if (numOfShapes > 0)
			shapeScore /= numOfShapes;

		if (!sp08 || !sp04)
		{
			int r0 = ArbitrateByRules(inArray, f10, shapeScore, numOfShapes, wordScore, numOfWords);
			XXFAIL(r0 < 0)
			if (r0 == 0)
			{
				BestMatch * entry;
				matchPtr = (BestMatch *)inArray->getIterator(&iter);
				if (wordScore < shapeScore)
				{
					for (ArrayIndex i = 0; i < iter.count(); matchPtr = (BestMatch *)iter.getNext(), i++)
					{
						unit = matchPtr->x00;
						if (IsOfType(unit->getType(), 'WORD'))
						{
							XXFAIL((entry = (BestMatch *)f10->addEntry()) == NULL)
							*entry = *matchPtr;
							gLastType = 2;
						}
					}
				}
				else
				{
					for (ArrayIndex i = 0; i < iter.count(); matchPtr = (BestMatch *)iter.getNext(), i++)
					{
						unit = matchPtr->x00;
						if (IsOfType(unit->getType(), 'GSHP'))
						{
							XXFAIL((entry = (BestMatch *)f10->addEntry()) == NULL)
							*entry = *matchPtr;
							gLastType = 1;
						}
					}
				}
			}
		}
		return;
	}
	XENDTRY;

failed:
	fController->signalMemoryError();
}


void
CArbiter::doArbitration(void)
{
// sp-94
	XTRY
	{
		bool r6 = NO;
		if (f04->count() > 0)
		{
			ArrayIterator iter;
			CRecUnit * unit;
			BestMatch match, * matchPtr = (BestMatch *)f04->getIterator(&iter);
			for (ArrayIndex i = 0; i < iter.count(); matchPtr = (BestMatch *)iter.getNext(), i++)
			{
//sp90 = gController
				match = *matchPtr;
				unit = match.x00;
				if (!unit->testFlags(0x40000000))
				{
					CRecArea * r8;
					XXFAIL((r8 = unit->getArea()) == NULL)
					int r7 = r8->getf14();
					if (match.x08 == 0  ||  r7 == 1)
					{
						r6 = YES;
						if (!unit->testFlags(0x00400000))
						{
							BestMatch * entry;
							XXFAIL((entry = (BestMatch *)f10->addEntry()) == NULL)
							*entry = match;
						}
						else
							fController->markUnits(unit, 0x40400000);
					}
					else
					{
						if (!unit->testFlags(0x00400000)  &&  ArbitrateEarly(&match))
						{
							BestMatch * entry;
							XXFAIL((entry = (BestMatch *)f10->addEntry()) == NULL)
							*entry = match;
							fController->setFlags(0x40000000);
//							r6 != sp84(f10);
							f10->cutToIndex(0);
							fController->unsetFlags(0x40000000);
						}
						if (unit->testFlags(0x40000000))	// not really...
							r6 = YES;
						bool r10 = waitingForOtherUnits(r8, &match);
						XXFAIL(fController->controllerError())
						if (r10)
							r8 = 0;
						else
						{
//							r8 = arbitrateUnits(r8);
							XXFAIL(fController->controllerError())
							if (r8 == 0)
								r6 = YES;
						}
						if (r8)
						{
							r6 = YES;
							BestMatch sp40;
							memcpy(&sp40, f10->getEntry(0), sizeof(BestMatch));
							fController->setFlags(0x40000000);
							if (gController->getf21())
								SetCaseAndTime(f10, 1);
//							sp5C(f10);
							fController->unsetFlags(0x40000000);

							ArrayIterator iter2;
							BestMatch * matchPtr2 = (BestMatch *)f10->getIterator(&iter2);
							for (ArrayIndex j = 0; j < iter2.count(); matchPtr2 = (BestMatch *)iter2.getNext(), j++)
							{
								match = *matchPtr2;
								unit = match.x00;
								if (match.x08 == 1
								&&  (IsOfType(unit->getType(), 'SCRB')  ||  r7 == 1)
								&&  !unit->testFlags(0x40000000))
									fController->markUnits(unit, 0x40400000);
							}
							if (r7 == 0)
							{
								memcpy(&match, f10->getEntry(0), sizeof(BestMatch));
								unit = match.x00;
								if (!unit->testFlags(0x40000000)  &&  IsOfType(unit->getType(), 'CLIK'))
									fController->markUnits(unit, 0x40400000);
							}
							
							matchPtr2 = (BestMatch *)f0C->getIterator(&iter2);
							for (ArrayIndex j = 0; j < iter2.count(); matchPtr2 = (BestMatch *)iter2.getNext(), j++)
							{
								match = *matchPtr;
							//	unit = match.x00;
								if (!match.x00->testFlags(0x40000000))
									fController->markUnits(unit, 0x40400000);	// sic -- using prev unit
							}
						}
					}
					f0C->cutToIndex(0);
					f10->cutToIndex(0);
				}
			}
			if (r6)
				fController->setf2C(GetTicks());
			f0C->cutToIndex(0);
			f10->cutToIndex(0);
			f04->cutToIndex(0);
		}
		if (f21 && r6)
			cleanUp();
		return;
	}
	XENDTRY;

failed:
	fController->signalMemoryError();
}


void
CArbiter::cleanUp(void)
{
	ArrayIterator iter;
	CRecUnit * unit;
	BestMatch * matchPtr;

	matchPtr = (BestMatch *)f08->getIterator(&iter);
	for (ArrayIndex i = 0; i < iter.count(); matchPtr = (BestMatch *)iter.getNext(), i++)
	{
		unit = matchPtr->x00;
		if (unit->testFlags(0x40000000))
		{
			f08->deleteEntry(i);
			iter.removeCurrent();
			i--;
		}
	}

	matchPtr = (BestMatch *)f0C->getIterator(&iter);
	for (ArrayIndex i = 0; i < iter.count(); matchPtr = (BestMatch *)iter.getNext(), i++)
	{
		unit = matchPtr->x00;
		if (unit->testFlags(0x40000000))
		{
			if (!unit->testFlags(0x08000000))
				fController->regroupUnclaimedSubs(unit);
		}
	}

	matchPtr = (BestMatch *)f08->getIterator(&iter);
	for (ArrayIndex i = 0; i < iter.count(); matchPtr = (BestMatch *)iter.getNext(), i++)
	{
		unit = matchPtr->x00;
		if (unit->testFlags(0x40000000))
		{
			if (!ClickInProgress(unit)  &&  !unit->testFlags(0x08000000))
				fController->regroupUnclaimedSubs(unit);
		}
	}

	matchPtr = (BestMatch *)f0C->getIterator(&iter);
	for (ArrayIndex i = 0; i < iter.count(); matchPtr = (BestMatch *)iter.getNext(), i++)
	{
		unit = matchPtr->x00;
		if (unit->testFlags(0x40000000))
		{
			fController->cleanGroupQ(unit);
			fController->deleteUnit(i);
			iter.removeCurrent();
			i--;
		}
	}

	matchPtr = (BestMatch *)f08->getIterator(&iter);
	for (ArrayIndex i = 0; i < iter.count(); matchPtr = (BestMatch *)iter.getNext(), i++)
	{
		unit = matchPtr->x00;
		if (unit->testFlags(0x40000000))
		{
			if (!ClickInProgress(unit))
			{
				fController->cleanGroupQ(unit);
				if (unit->getType() == 'STRK'  &&  unit->testFlags(0x00400000))
					fController->expireStroke(unit);
				fController->deletePiece(i);
				iter.removeCurrent();
				i--;
			}
		}
	}

	f04->compact();
	f08->compact();
	f0C->compact();
	f10->compact();
	f14->compact();
	f18->compact();
	f1C->compact();
}
