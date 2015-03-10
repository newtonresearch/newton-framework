/*
	File:		Strings.h

	Contains:	Unicode string functions

	Written by:	Newton Research Group
*/

#if !defined(__STRINGS_H)
#define __STRINGS_H 1

extern "C" {
Ref			FFindStringInArray(RefArg inRcvr, RefArg inArray, RefArg inStr);
}

UniChar *	GetUString(RefArg inStr);
bool			MakeStringObject(RefArg obj, UniChar * inStr, ArrayIndex * outLength, ArrayIndex inLength);
NewtonErr	IntegerString(int i, UniChar * ioStr);
NewtonErr	NumberString(double n, UniChar * ioStr, ArrayIndex inLength, const char * inFormat);


#define DEFSTR(name) extern Ref * RS##name;
#define DEFMPSTR(name, str) extern Ref * RS##name;
#include "StringDefs.h"
#undef DEFMPSTR
#undef DEFSTR

#define RSTRLEN(s) (Length(s) / sizeof(UniChar) - 1)

#endif	/* __STRINGS_H */
