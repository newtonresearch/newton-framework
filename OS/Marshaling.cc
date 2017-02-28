/*
	File:		Marshaling.cc

	Contains:	Marshal arguments: C++ <-> NewtonScript mapping.

	Written by:	Newton Research Group.
*/

#include "Objects.h"
#include "ROMResources.h"
#include "Unicode.h"
#include "UStringUtils.h"
#include "OSErrors.h"


/* -------------------------------------------------------------------------------
	T y p e s
------------------------------------------------------------------------------- */

enum {
	kMarshalLong = 1,
	kMarshalULong,
	kMarshalShort,
	kMarshalByte,
	kMarshalBoolean,			// 5
	kMarshalHighInt,
	kMarshalHexLong,
	kMarshalSplitLong,
	kMarshalSplitByteLong,
	kMarshalStruct,			// 10
	kMarshalArray,
	kMarshalRef,
	kMarshalChar,
	kMarshalCstring,
	kMarshalUnicodeChar,		// 15
	kMarshalUnicode = 0,
	kMarshalASCIIEncoding = kMarshalULong,
	kMarshalBinary = 17,
	kMarshalReal
};


/* -------------------------------------------------------------------------------
	F u n c t i o n   P r o t o t y p e s
------------------------------------------------------------------------------- */

Ref	UnmarshalStruct(void ** ioParmPtr, RefArg inSpec, NewtonErr * outErr, CharEncoding inEncoding);
Ref	UnmarshalArray(void ** ioParmPtr, RefArg inSpec, NewtonErr * outErr, CharEncoding inEncoding);


/* -------------------------------------------------------------------------------
	Align pointer/size.
	Args:		ioParmPtr
				ioSize
				inAlignment
	Return:	--
------------------------------------------------------------------------------- */

void
AlignBuffer(void ** ioParmPtr, ArrayIndex * ioSize, ArrayIndex inAlignment)
{
	if (ioParmPtr != NULL)
		*ioParmPtr = (void *)ALIGN(*ioParmPtr, inAlignment);
	if (ioSize != NULL)
		*ioSize = ALIGN(*ioSize, inAlignment);
}


/* -------------------------------------------------------------------------------
	Translate marshaling symbol to selector.
	Args:		inSym			might actually be an int
	Return:	marshaling type selector
------------------------------------------------------------------------------- */

int
TranslateTypeMarshalingSymbol(RefArg inSym)
{
	// check it isn’t actually an int anyway
	if (ISINT(inSym))
		return RVALUE(inSym);
	Ref translation = GetFrameSlot(RA(marshalTypes), inSym);
	return ISNIL(translation) ? 0 : RINT(translation);
}


/* -------------------------------------------------------------------------------
	Align parm pointer for marshaling type.
	Args:		ioParmPtr		pointer to be aligned
				inArg2
				inSpec			parm specifier
	Return:	--
------------------------------------------------------------------------------- */

void
AlignForType(void ** ioParmPtr, ULong * inArg2, RefArg inSpec)
{
	if (IsArray(inSpec))
	{
		RefVar typeRef(GetArraySlot(inSpec, 0));
		int type = TranslateTypeMarshalingSymbol(typeRef);
		if (type == kMarshalStruct)
		{
			AlignBuffer(ioParmPtr, inArg2, 4);
		}
		else if (type == kMarshalArray)
		{
			AlignForType(ioParmPtr, inArg2, GetArraySlot(inSpec, 1));
		}
	}
	else
	{
		int type = TranslateTypeMarshalingSymbol(inSpec);
		if (type == kMarshalShort)
		{
			// it gets short-aligned
			AlignBuffer(ioParmPtr, inArg2, 2);
		}
		else if (type == kMarshalByte || type == kMarshalBoolean || type == kMarshalChar)
			;	// no alignment required
		else
		{
			// everything else gets long-aligned
			AlignBuffer(ioParmPtr, inArg2, 4);
		}
	}
}


/* -------------------------------------------------------------------------------
	Unmarshal C++ value -> NewtonScript Ref.
	Args:		ioParmPtr	pointer to C++ data; updated on return
				inSpec		parm specifier
				inAligned	true => treat all data as long-aligned
				outErr
				inEncoding	character encoding (not required for all types)
	Return:	Ref
				outErr		error code
------------------------------------------------------------------------------- */

Ref
UnmarshalValue(void ** ioParmPtr, RefArg inSpec, bool inAligned, NewtonErr * outErr, CharEncoding inEncoding)
{
	RefVar value;
	*outErr = noErr;

	if (IsArray(inSpec))
	{
		// marshal vector value
		RefVar typeRef(GetArraySlot(inSpec, 0));
		int type = TranslateTypeMarshalingSymbol(typeRef);
		switch (type)
		{
		case kMarshalStruct:
			*ioParmPtr = (void *)LONGALIGN(*ioParmPtr);
			value = UnmarshalStruct(ioParmPtr, inSpec, outErr, inEncoding);
			break;

		case kMarshalArray:
			{
				typeRef = GetArraySlot(inSpec, 1);
				type = TranslateTypeMarshalingSymbol(typeRef);
				// can only have array of certain types
				switch (type)
				{
				case kMarshalChar:
					{
						char * s = (char *)*ioParmPtr;
						Ref strLenRef = GetArraySlot(inSpec, 2);
						ArrayIndex strLen = RINT(strLenRef);
						if (strLen == 0)
						{
							strLen = strlen(s);
							*ioParmPtr = s + LONGALIGN(strLen);
						}
						else
							*ioParmPtr = s + strLen;
						value = AllocateBinary(SYMA(string), (strLen+1)*sizeof(UniChar));
						ConvertToUnicode((const void *)s, (UniChar *)BinaryData(value), strLen, inEncoding);
					}
					break;

				case kMarshalUnicodeChar:
					{
						UniChar * s = (UniChar *)*ioParmPtr;
						Ref strLenRef = GetArraySlot(inSpec, 2);
						ArrayIndex strLen = RINT(strLenRef);
						if (strLen == 0)
							strLen = ALIGN(Ustrlen(s), sizeof(UniChar));
						*ioParmPtr = s + strLen;
						value = MakeString(s);
					}
					break;

				default:
					{
						AlignForType(ioParmPtr, NULL, inSpec);
						value = UnmarshalArray(ioParmPtr, inSpec, outErr, inEncoding);
					}
					break;
				}
			}
			break;

		default:
			*outErr = -70001;
			break;
		}
		
	}
	else
	{
		// marshal scalar value
		int type = TranslateTypeMarshalingSymbol(inSpec);
		switch (type)
		{
		case kMarshalLong:
		case kMarshalULong:
//		case kMarshalASCIIEncoding:
			{
				int32_t * p = (int32_t *)LONGALIGN(*ioParmPtr);
				value = MAKEINT(*p);
				*ioParmPtr = p+1;
			}
			break;

		case kMarshalShort:
			if (inAligned)
			{
				int32_t * p = (int32_t *)(*ioParmPtr);
				value = MAKEINT(*p & 0xFFFF);
				*ioParmPtr = p+1;
			}
			else
			{
				int16_t * p = (int16_t *)*ioParmPtr;
				value = MAKEINT(*p);
				*ioParmPtr = p+1;
			}
			break;

		case kMarshalByte:
			if (inAligned)
			{
				int32_t * p = (int32_t *)*ioParmPtr;
				value = MAKEINT(*p & 0xFF);
				*ioParmPtr = p+1;
			}
			else
			{
				int8_t * p = (int8_t *)*ioParmPtr;
				value = MAKEINT(*p);
				*ioParmPtr = p+1;
			}
			break;

		case kMarshalBoolean:
			if (inAligned)
			{
				int32_t * p = (int32_t *)*ioParmPtr;
				value = MAKEBOOLEAN(*p != 0);
				*ioParmPtr = p+1;
			}
			else
			{
				int8_t * p = (int8_t *)*ioParmPtr;
				value = MAKEBOOLEAN(*p != 0);
				*ioParmPtr = p+1;
			}
			break;

		case kMarshalHighInt:
			{
				*ioParmPtr = (void *)LONGALIGN(*ioParmPtr);
				int32_t * p = (int32_t *)*ioParmPtr;
				value = MAKEINT(*p >> 2);
				*ioParmPtr = p+1;
			}
			break;

		case kMarshalHexLong:
			{
				*ioParmPtr = (void *)LONGALIGN(*ioParmPtr);
				uint32_t * p = (uint32_t *)*ioParmPtr;
				*ioParmPtr = p+1;

				uint32_t v = *p;
				char hex[8+1];
				for (int i = 7; i >= 0; --i, v >>= 4)
				{
					int x = v & 0x0F;
					if (x < 10)
						x += '0';
					else
						x += 'A'-10;
					hex[i] = x;
				}
				hex[8] = 0;
				value = MakeStringFromCString(hex);
			}
			break;

		case kMarshalSplitLong:
			value = MakeArray(2);
			{
				*ioParmPtr = (void *)LONGALIGN(*ioParmPtr);
				uint32_t * p = (uint32_t *)*ioParmPtr;
				*ioParmPtr = p+1;
				SetArraySlot(value, 0, MAKEINT((*p >> 16) & 0xFFFF));
				SetArraySlot(value, 1, MAKEINT(*p & 0xFFFF));
			}
			break;

		case kMarshalSplitByteLong:
			value = MakeArray(4);
			{
				*ioParmPtr = (void *)LONGALIGN(*ioParmPtr);
				uint32_t * p = (uint32_t *)*ioParmPtr;
				*ioParmPtr = p+1;
				SetArraySlot(value, 0, MAKEINT((*p >> 24) & 0xFF));
				SetArraySlot(value, 1, MAKEINT((*p >> 16) & 0xFF));
				SetArraySlot(value, 2, MAKEINT((*p >> 8) & 0xFF));
				SetArraySlot(value, 3, MAKEINT(*p & 0xFF));
			}
			break;

//		case kMarshalRef:
//			break;

		case kMarshalChar:
			{
				char c;
				UniChar uc;
				if (inAligned)
				{
					int32_t * p = (int32_t *)*ioParmPtr;
					c = (*p & 0xFF);
					*ioParmPtr = p+1;
				}
				else
				{
					int8_t * p = (int8_t *)*ioParmPtr;
					c = *p;
					*ioParmPtr = p+1;
				}
				ConvertToUnicode((const void *)&c, &uc, 1, inEncoding);
				value = MAKECHAR(uc);
			}
			break;

		case kMarshalCstring:
			{
				*ioParmPtr = (void *)LONGALIGN(*ioParmPtr);
				const char ** p = (const char **)ioParmPtr;
				value = MakeStringFromCString(*p);
				*ioParmPtr = p+1;
			}
			break;

		case kMarshalUnicodeChar:
			if (inAligned)
			{
				int32_t * p = (int32_t *)*ioParmPtr;
				value = MAKECHAR(*p & 0xFFFF);
				*ioParmPtr = p+1;
			}
			else
			{
				UniChar * p = (UniChar *)*ioParmPtr;
				value = MAKECHAR(*p);
				*ioParmPtr = p+1;
			}
			break;

		case kMarshalUnicode:
			{
				*ioParmPtr = (void *)LONGALIGN(*ioParmPtr);
				const UniChar ** p = (const UniChar **)ioParmPtr;
				value = MakeString(*p);
				*ioParmPtr = p+1;
			}
			break;

//		case kMarshalBinary:
//			break;

		case kMarshalReal:
			{
				*ioParmPtr = (void *)LONGALIGN(*ioParmPtr);
				double * p = (double *)*ioParmPtr;
				value = MakeReal(*p);
				*ioParmPtr = p+1;
			}
			break;
		}
		
	}
	return value;
}


Ref
UnmarshalStruct(void ** ioParmPtr, RefArg inSpec, NewtonErr * outErr, CharEncoding inEncoding)
{
	ArrayIndex specLen = Length(inSpec);
	RefVar value(MakeArray(specLen-1));
	for (ArrayIndex i = 1; i < specLen; ++i)
	{
		RefVar specItem(GetArraySlot(inSpec, i));
		SetArraySlot(value, i-1, UnmarshalValue(ioParmPtr, specItem, false, outErr, inEncoding));
		if (*outErr != noErr)
			break;
	}
	return value;
}


Ref
UnmarshalArray(void ** ioParmPtr, RefArg inSpec, NewtonErr * outErr, CharEncoding inEncoding)
{
}


/* -------------------------------------------------------------------------------
	Marshal NewtonScript Ref -> C++ struct.
------------------------------------------------------------------------------- */

NewtonErr
DoMarshal(RefArg inArray1, RefArg inArray2, void ** outArg3, void ** outArg4, void ** outArg5, ULong * outArg6, ULong * outArg7, ULong * outArg8, int inArg9, int inArg10, int inArg11, int inArg12, int inArg13)	// 13 args -- you havin’ a laff?
{ return noErr; }


NewtonErr
MarshalArgumentSize(RefArg inArray1, RefArg inArray2, size_t * outArg3, int inArg4)
{
	NewtonErr err;
	ULong sp00 = 0;
	ULong sp04 = 0;
	err = DoMarshal(inArray1, inArray2, NULL, NULL, NULL, &sp04, &sp00, NULL, 0, 0, 65536, 1, inArg4);
	*outArg3 = sp00 + sp04;
	return err;
}


NewtonErr
MarshalArguments(RefArg inArray1, RefArg inArray2, void * inParmBlock, size_t inParmSize, int inArg5)
{
	NewtonErr err;
	ULong sp00 = 0;
	ULong sp04 = 0;
	err = DoMarshal(inArray1, inArray2, NULL, NULL, NULL, &sp04, &sp00, NULL, 0, 0, 65536, 1, inArg5);
	if (err == noErr)
	{
		void * spm00 = (char *)inParmBlock + sp04;
		void * spm04 = inParmBlock;
		err = DoMarshal(inArray1, inArray2, &spm04, &spm00, NULL, NULL, NULL, NULL, 0, 0, 65536, 1, inArg5);
	}
	return err;
}


NewtonErr
MarshalArguments(RefArg inArray1, RefArg inArray2, void ** ioParmPtr, int inArg4)
{
	NewtonErr err = noErr;
	void * parmBlock = NULL;
	XTRY
	{
		size_t parmSize = 0;
		*ioParmPtr = NULL;
		XFAIL(err = MarshalArgumentSize(inArray1, inArray2, &parmSize, inArg4))
		parmBlock = malloc(parmSize);
		XFAILIF(parmBlock == NULL, err = MemError();)
		XFAIL(err = MarshalArguments(inArray1, inArray2, parmBlock, parmSize, inArg4))
		*ioParmPtr = parmBlock;
	}
	XENDTRY;
	XDOFAIL(err)
	{
		if (parmBlock)
			free(parmBlock);
	}
	XENDFAIL;
	return err;
}


Ref
ConstructReturnValue(void * inParmBlock, RefArg inSpec, NewtonErr * outErr, CharEncoding inEncoding)
{
	return UnmarshalValue(&inParmBlock, inSpec, true, outErr, inEncoding);
}

