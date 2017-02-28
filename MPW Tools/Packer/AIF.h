//
//  AIF.h
//  Packer
//
//  Created by Simon Bell on 05/01/2016.
//  Copyright (c) 2016 Newton Research. All rights reserved.
//

#ifndef __AIF_h
#define __AIF_h


/*------------------------------------------------------------------------------
	A I F   S t r u c t u r e s
------------------------------------------------------------------------------*/

struct AIFHeader
{
	uint32_t  decompressCode;
	uint32_t  selfRelocCode;
	uint32_t  zeroInitCode;
	uint32_t  imageEntryCode;
	uint32_t  exitCode;
	uint32_t  roSize;
	uint32_t  rwSize;
	uint32_t  debugSize;
	uint32_t  zeroInitSize;
	uint32_t  debugType;
	uint32_t  imageBase;
	uint32_t  workSpace;
	uint32_t  addressMode;
	uint32_t  dataBase;
	uint32_t  reserved1;
	uint32_t  reserved2;
	uint32_t  zeroInit[16];
};

struct ItemSection
{
	uint16_t		size;
	uint16_t		code;
	uint8_t		language;
	uint8_t		level;
	uint8_t		reserved1;
	uint8_t		version;
	uint32_t		codeStart;
	uint32_t		dataStart;
	uint32_t  	codeSize;
	uint32_t  	dataSize;
	uint32_t		fileInfo;
	uint32_t  	debugSize;
	union
	{
		uint32_t		numOfSyms;
		struct
		{
			uint8_t		nameLength;
			char			name[3]; // actually variable length pascal string
		};
	};
};

struct ItemProcedure
{
	uint16_t		size;
	uint16_t		code;
	uint32_t		type;
	uint32_t		numOfArgs;
	uint32_t		sourcePos;
	uint32_t		startAddr;
	uint32_t		entryAddr;
	uint32_t		endProc;
	uint32_t		fileEntry;
	uint8_t		nameLength;
	char			name[3]; // actually variable length pascal string
};

// code
enum
{
	kItemSection = 1,
	kItemBeginProc,
	kItemEndProc,
	kItemVariable,
	kItemType,
	kItemStruct,
	kItemArray,
	kItemSubrange,
	kItemSet,
	kItemFileInfo,
	kItemContigEnum,
	kItemDiscontigEnum,
	kItemProcDecl,
	kItemBeginScope,
	kItemEndScope,
	kItemBitfield
};

// language
enum
{
	kLanguageNone,
	kLanguageC,
	kLanguagePascal,
	kLanguageFortran,
	kLanguageAssembler
};

// debug level
enum
{
	kDebugLineNumbers,
	kDebugVariableNames
};


struct AIFSymbol
{
	uint32_t  flags  :  8;
	uint32_t  offset : 24;
	int32_t	value;
};


#endif
