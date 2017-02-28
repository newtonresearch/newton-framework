/*
	File:		EndpointProto.s

	Contains:	CEndpoint p-class protocol interface.

	Written by:	Newton Research Group, 2016.
*/

#include "../Protocols/ProtoMacro.s"

/* ---------------------------------------------------------------- */

		.globl	__ZN9CEndpoint4makeEPKc
		.globl	__ZN9CEndpoint7destroyEv
		.globl	__ZN9CEndpoint4initEPv
		.globl	__ZN9CEndpoint11handleEventEjP6CEventj
		.globl	__ZN9CEndpoint14handleCompleteEP10CUMsgTokenPjP6CEvent
		.globl	__ZN9CEndpoint13addToAppWorldEv
		.globl	__ZN9CEndpoint18removeFromAppWorldEv
		.globl	__ZN9CEndpoint4openEj
		.globl	__ZN9CEndpoint5closeEv
		.globl	__ZN9CEndpoint5abortEv
//and more
		.globl	__ZN9CEndpoint4nSndEP14CBufferSegmentjjbP12COptionArray
		.globl	__ZN9CEndpoint4nRcvEP14CBufferSegmentjPjjbP12COptionArray
		.globl	__ZN9CEndpoint6nAbortEb
		.globl	__ZN9CEndpoint7timeoutEj
		.globl	__ZN9CEndpoint9isPendingEj

		.text
		.align	2

CEndpoint_name:
		.asciz	"CEndpoint"
		.align	2

__ZN9CEndpoint4makeEPKc:
		New CEndpoint_name
		Dispatch 2
__ZN9CEndpoint7destroyEv:
		Delete 3
__ZN9CEndpoint4initEPv:
		Dispatch 4
__ZN9CEndpoint11handleEventEjP6CEventj:
		Dispatch 5
__ZN9CEndpoint14handleCompleteEP10CUMsgTokenPjP6CEvent:
		Dispatch 6
__ZN9CEndpoint13addToAppWorldEv:
		Dispatch 7
__ZN9CEndpoint18removeFromAppWorldEv:
		Dispatch 8
__ZN9CEndpoint4openEj:
		Dispatch 9
__ZN9CEndpoint5closeEv:
		Dispatch 10
__ZN9CEndpoint5abortEv:
		Dispatch 11
//
//__ZN9CEndpoint setSync(bool sync);
//__ZN9CEndpoint getProtAddr(COptionArray* bndAddr, COptionArray* peerAddr, Timeout timeOut = kNoTimeout);
//__ZN9CEndpoint optMgmt(ULong arrayOpCode, COptionArray* options, Timeout timeOut = kNoTimeout);
//__ZN9CEndpoint bind(COptionArray* addr = NULL, long* qlen = NULL, Timeout timeOut = kNoTimeout);
//__ZN9CEndpoint unbind(Timeout timeOut = kNoTimeout);
//__ZN9CEndpoint listen(COptionArray* addr = NULL, COptionArray* opt = NULL, CBufferSegment* data = NULL, long* seq = NULL, Timeout timeOut = kNoTimeout);
//__ZN9CEndpoint accept(CEndpoint* resfd, COptionArray* addr = NULL, COptionArray* opt = NULL, CBufferSegment* data = NULL, long seq = 0, Timeout timeOut = kNoTimeout);
//__ZN9CEndpoint connect(COptionArray* addr = NULL, COptionArray* opt = NULL, CBufferSegment* data = NULL, long* seq = NULL, Timeout timeOut = kNoTimeout);
//__ZN9CEndpoint disconnect(CBufferSegment* data = NULL, long reason = 0, long seq = 0);
//__ZN9CEndpoint release(Timeout timeOut = kNoTimeout);
//__ZN9CEndpoint snd(UByte* buf, ArrayIndex& nBytes, ULong flags, Timeout timeOut = kNoTimeout);
//__ZN9CEndpoint rcv(UByte* buf, ArrayIndex& nBytes, ArrayIndex thresh, ULong* flags, Timeout timeOut = kNoTimeout);
//__ZN9CEndpoint snd(CBufferSegment* buf, ULong flags, Timeout timeOut = kNoTimeout);
//__ZN9CEndpoint rcv(CBufferSegment* buf, ArrayIndex thresh, ULong* flags, Timeout timeOut = kNoTimeout);
//__ZN9CEndpoint waitForEvent(Timeout timeOut = kNoTimeout);
//__ZN9CEndpoint nBind(COptionArray* opt = NULL, Timeout timeOut = kNoTimeout, bool sync = true);
//__ZN9CEndpoint nListen(COptionArray* opt = NULL, CBufferSegment* data = NULL, long* seq = NULL, Timeout timeOut = kNoTimeout, bool sync = true);
//__ZN9CEndpoint nAccept(CEndpoint* resfd, COptionArray* opt = NULL, CBufferSegment* data = NULL, long seq = 0, Timeout timeOut = kNoTimeout, bool sync = true);
//__ZN9CEndpoint nConnect(COptionArray* opt = NULL, CBufferSegment* data = NULL, long* seq = NULL, Timeout timeOut = kNoTimeout, bool sync = true);
//__ZN9CEndpoint nRelease(Timeout timeOut = kNoTimeout, bool sync = true);
//__ZN9CEndpoint nDisconnect(CBufferSegment* data = NULL, long reason = 0, long seq = 0, Timeout timeOut = kNoTimeout, bool sync = true);
//__ZN9CEndpoint nUnBind(Timeout timeOut = kNoTimeout, bool sync = true);
//__ZN9CEndpoint nOptMgmt(ULong arrayOpCode, COptionArray* options, Timeout timeOut = kNoTimeout, bool sync = true);
//__ZN9CEndpoint nSnd(UByte* buf, ArrayIndex* count, ULong flags, Timeout timeOut = kNoTimeout, bool sync = true, COptionArray* opt = NULL);
//__ZN9CEndpoint nRcv(UByte* buf, ArrayIndex* count, ArrayIndex thresh, ULong* flags, Timeout timeOut = kNoTimeout, bool sync = true, COptionArray* opt = NULL);
//

__ZN9CEndpoint4nSndEP14CBufferSegmentjjbP12COptionArray:
		Dispatch 12
__ZN9CEndpoint4nRcvEP14CBufferSegmentjPjjbP12COptionArray:
		Dispatch 13
__ZN9CEndpoint6nAbortEb:
		Dispatch 14
__ZN9CEndpoint7timeoutEj:
		Dispatch 15
__ZN9CEndpoint9isPendingEj:
		Dispatch 16
