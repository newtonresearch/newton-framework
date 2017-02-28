/*
	File:		RecType.cc

	Contains:	Recognition type list implementation.

	Written by:	Newton Research Group.
*/

#include "RecObject.h"
#include "RecDomain.h"
#include "OSErrors.h"


#pragma mark CTypeList
/*------------------------------------------------------------------------------
	C T y p e L i s t

	A dynamic array of RecTypes.
------------------------------------------------------------------------------*/

CTypeList *
CTypeList::make(void)
{
	CTypeList * list = new CTypeList;
	XTRY
	{
		XFAIL(list == NULL)
		XFAILIF(list->iTypeList() != noErr, list->release(); list = NULL;)
	}
	XENDTRY;
	return list;
}


NewtonErr
CTypeList::iTypeList(void)
{
	NewtonErr err = iDArray(sizeof(RecType), 0);
	compact();
	return err;
}


void
CTypeList::dump(CMsg * outMsg)
{
	outMsg->msgLF();

	for (ArrayIndex i = 0; i < count(); ++i)
	{
		outMsg->msgType(getType(i));
		outMsg->msgChar(' ');
	}
}


NewtonErr
CTypeList::addType(RecType inType)
{
	NewtonErr err = noErr;
	XTRY
	{
		RecType * p;
		XFAILNOT(p = reinterpret_cast<RecType*>(addEntry()), err = kOSErrNoMemory;)
		*p = inType;
	}
	XENDTRY;
	return err;
}


NewtonErr
CTypeList::addUnique(RecType inType)
{
	if (count() > 0)
	{
		RecType *	p = reinterpret_cast<RecType*>(getEntry(0));
		for (ArrayIndex i = 0; i < count(); ++i, ++p)
		{
			if (*p == inType)
				return noErr;
		}
	}
	return addType(inType);
}


ArrayIndex
CTypeList::findType(RecType inType)
{
	for (ArrayIndex i = 0; i < count(); ++i)
	{
		if (getType(i) == inType)
			return i;
	}
	return kIndexNotFound;
}


RecType
CTypeList::getType(ArrayIndex index)
{
	return *reinterpret_cast<RecType*>(getEntry(index));
}

#pragma mark -
#pragma mark CTypeAssoc
/*------------------------------------------------------------------------------
	C T y p e A s s o c
------------------------------------------------------------------------------*/

CTypeAssoc *
CTypeAssoc::make(void)
{
	CTypeAssoc * assoc = new CTypeAssoc;
	XTRY
	{
		XFAIL(assoc == NULL)
		XFAILIF(assoc->iTypeAssoc() != noErr, assoc->release(); assoc = NULL;)
	}
	XENDTRY;
	return assoc;
}


NewtonErr
CTypeAssoc::iTypeAssoc(void)
{
	return iDArray(sizeof(Assoc), 0);
}


void
CTypeAssoc::dump(CMsg * outMsg)
{
	for (ArrayIndex i = 0; i < count(); ++i)
	{
		outMsg->msgChar('(');
		outMsg->msgType(getAssoc(i)->fType);
		outMsg->msgStr(") ");
	}
	outMsg->msgLF();
}


CTypeAssoc *
CTypeAssoc::copy(void)
{
	CTypeAssoc * theCopy = new CTypeAssoc;
	copyInto(theCopy);
	return theCopy;
}


void
CTypeAssoc::dealloc(void)
{
	for (ArrayIndex i = 0; i < count(); ++i)
	{
		Assoc *  assoc = getAssoc(i);
		char *	p;		// was Handle
		if ((p = assoc->fInfo) != NULL && !assoc->f18)
		{
			assoc->fDomain->domainParameter(3, 0, (OpaqueRef)p);
			free(assoc->fInfo);
		}
	}
	// super dealloc
	CArray::dealloc();
}


ArrayIndex
CTypeAssoc::addAssoc(Assoc * inAssoc)
{
	ArrayIndex i;
	for (i = 0; i < count(); ++i)
	{
		Assoc *  assoc = getAssoc(i);
		if (inAssoc->fType < assoc->fType)
			break;
		if (inAssoc->fType == assoc->fType
		&&  inAssoc->fDomain == assoc->fDomain
		&&  inAssoc->fDomainInfo == assoc->fDomainInfo
		&&  inAssoc->fHandler == assoc->fHandler)
			return i;
	}
	i = insert(i);
	if (i != kIndexNotFound)
		*getAssoc(i) = *inAssoc;
	return i;
}


Assoc *
CTypeAssoc::getAssoc(ArrayIndex index)
{
	return reinterpret_cast<Assoc*>(getEntry(index));
}


NewtonErr
CTypeAssoc::mergeAssoc(CTypeAssoc * inAssoc)
{
	Assoc			assoc;
	ArrayIndex  index = 0;
	for (ArrayIndex i = 0; i < count(); ++i)
	{
		assoc = *inAssoc->getAssoc(i);
		index = addAssoc(&assoc);
		if (index == kIndexNotFound)
			break;
	}
	return (index == kIndexNotFound) ? 1 : noErr;
}
