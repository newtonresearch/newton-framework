/*
	File:		StoreCompanderWrapper.cc

	Contains:	Store compression protocol.

	Written by:	Newton Research Group.
*/

#include "StoreCompander.h"
#include "OSErrors.h"

extern NewtonErr	LODefaultDoTransaction(CStore * inStore, PSSId, PSSId, int, bool);


/*------------------------------------------------------------------------------
	C S t o r e C o m p a n d e r W r a p p e r
------------------------------------------------------------------------------*/

/* ----------------------------------------------------------------
	CStoreCompanderWrapper implementation class info.
---------------------------------------------------------------- */

const CClassInfo *
CStoreCompanderWrapper::classInfo(void)
{
__asm__ (
CLASSINFO_BEGIN
"		.long		0			\n"
"		.long		1f - .	\n"
"		.long		2f - .	\n"
"		.long		3f - .	\n"
"		.long		4f - .	\n"
"		.long		5f - .	\n"
"		.long		__ZN22CStoreCompanderWrapper6sizeOfEv - 0b	\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		__ZN22CStoreCompanderWrapper4makeEv - 0b	\n"
"		.long		__ZN22CStoreCompanderWrapper7destroyEv - 0b	\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		6f - 0b	\n"
"1:	.asciz	\"CStoreCompanderWrapper\"	\n"
"2:	.asciz	\"CStoreCompander\"	\n"
"3:	.byte		0			\n"
"		.align	2			\n"
"4:	.long		0			\n"
"		.long		__ZN22CStoreCompanderWrapper9classInfoEv - 4b	\n"
"		.long		__ZN22CStoreCompanderWrapper4makeEv - 4b	\n"
"		.long		__ZN22CStoreCompanderWrapper7destroyEv - 4b	\n"
"		.long		__ZN22CStoreCompanderWrapper4initEP6CStorejjbb - 4b	\n"
"		.long		__ZN22CStoreCompanderWrapper9blockSizeEv - 4b	\n"
"		.long		__ZN22CStoreCompanderWrapper4readEmPcmm - 4b	\n"
"		.long		__ZN22CStoreCompanderWrapper5writeEmPcmm - 4b	\n"
"		.long		__ZN22CStoreCompanderWrapper20doTransactionAgainstEij - 4b	\n"
"		.long		__ZN22CStoreCompanderWrapper10isReadOnlyEv - 4b	\n"
CLASSINFO_END
);
}

PROTOCOL_IMPL_SOURCE_MACRO(CStoreCompanderWrapper)

CStoreCompanderWrapper *
CStoreCompanderWrapper::make(void)
{
	fStore = NULL;
	fDecompressor = NULL;
	return this;
}


void
CStoreCompanderWrapper::destroy(void)
{
	char	buf[128];

	if (fDecompressor)
	{
#if 0
// we don’t implement xxxCleanup companders anywhere, and this is a cause of crashes in NCX
		sprintf(buf, "%sCleanup", fCompanderName);

		const CClassInfo * info;
		if ((info = ClassInfoByName("CStoreDecompressor", buf)) != NULL)
			fDecompressor->setType(info);
#endif
		fDecompressor->destroy();
	}
}


NewtonErr
CStoreCompanderWrapper::init(CStore * inStore, PSSId inRootId, PSSId inParmsId, bool inShared, bool inArg5)
{
	return kOSErrCallNotImplemented;
}


NewtonErr
CStoreCompanderWrapper::init(CStore * inStore, char * inCompanderName, PSSId inRootId, PSSId inParmsId, char * inLZWBuffer)
{
	NewtonErr	err;

	XTRY
	{
		fStore = inStore;
		fRootId = inRootId;
		fCompanderName = inCompanderName;
		fDecompressor = (CStoreDecompressor *)MakeByName("CStoreDecompressor", inCompanderName);
		XFAILIF(fDecompressor == NULL, err = kOSErrBadParameters;)
		XFAIL(err = fDecompressor->init(inStore, inParmsId, inLZWBuffer))

		PackageRoot	root;
		err = inStore->read(fRootId, 0, &root, sizeof(root));
		fChunksId = root.fDataId;
	}
	XENDTRY;

	return err;
}


size_t
CStoreCompanderWrapper::blockSize(void)
{
	return kSubPageSize;
}


NewtonErr
CStoreCompanderWrapper::read(size_t inOffset, char * outBuf, size_t inBufLen, VAddr inBaseAddr)
{
	NewtonErr	err;
	PSSId			objId;

	XTRY
	{
		// read id of chunk
		XFAIL(err = fStore->read(fChunksId, (inOffset/kSubPageSize)*sizeof(PSSId), &objId, sizeof(PSSId)))		// get chunk size
		err = fDecompressor->read(objId, outBuf, inBufLen, inBaseAddr);
	}
	XENDTRY;

	return err;
}


NewtonErr
CStoreCompanderWrapper::write(size_t inOffset, char * inBuf, size_t inBufLen, VAddr inBaseAddr)
{
	return kOSErrCallNotImplemented;
}


NewtonErr
CStoreCompanderWrapper::doTransactionAgainst(int inArg1, ULong inArg2)
{
	return LODefaultDoTransaction(fStore, fRootId, fChunksId, inArg1, false);
}


bool
CStoreCompanderWrapper::isReadOnly(void)
{
	return true;
}

