/*----------------------------------------------------------------------
	M e m o r y   s i z e   o p e r a t i o n s
----------------------------------------------------------------------*/

size_t
ComputeRAMLimit(long arg1, long arg2, long arg3)	// 0011E1AC
{
	size_t limit = 0;
	ULong length;
//	Ptr p = ::GetLastRExConfigEntry(kRAMAllocationTag, &length);
//	if (p != nil && p[4] > arg1)
	{
		// INCOMPLETE!
	}
	return limit;
}


long
InternalStoreInfo(long arg1)
{
	long result = 0;

	if (arg1 == 3)
		result = 0x01100000;	// 17M
	else
	{
		 if (arg1 == 0)
			result = gGlobalsThatLiveAcrossReboot.fReserved[14];	// x35C
		else if (gGlobalsThatLiveAcrossReboot.fReserved[14] == 0)
			result = 0x01100000;
		else
		{
			long r0;
			long r6 = 0;
			for (int i = 0; i < kMaximumMemoryPartitians; i++)
			{
				if (gGlobalsThatLiveAcrossReboot.fBank[i].tag != kUndefined
				 && gGlobalsThatLiveAcrossReboot.fBank[i].x10 < 4)
				 	r6 = gGlobalsThatLiveAcrossReboot.fBank[i].size;
			}
			if (r6 != 0 && r6 < 0x00020000)	// 128K
				r0 = 0x00008000;	// 32K
			else
			{
				size_t RAMsize = GetRAMSize();
				r0 = ComputeRAMLimit(0, RAMsize, 0x1000);	// 4K
				if (RAMsize - r0 < 0x00080000)	// 512K
					r0 = RAMsize - 0x00080000;
			}
			if (arg1 == 1)
				result = r0;
			else if (arg1 == 2)
				result = r6;
		}
	}

	return result;
}


size_t
InternalRAMInfo(long arg1, long arg2)
{
	size_t size = GetRAMSize() - InternalStoreInfo(1);
	return (arg1 == -1) ? size : ComputeRAMLimit(arg1, size, arg2);
}


size_t
SystemRAMSize(void)
{
	return InternalRAMInfo(-1, 0);
}

size_t
TotalSystemFree(void)
{
	return SystemFreePageCount() * kPageSize;
}

#pragma mark -

size_t
SBankInfo::NormalRAMSize(void)
{
	return (x10 == 4) ? size : 0;
}


#pragma mark -

typedef enum
{
} EBankDesignation;

class TRAMTable
{
public:
	static void			Init(SBankInfo * bank);
	static NewtonErr	Add(SBankInfo * bank, SBankInfo * info);
	static void			Remove(ULong, ULong, EBankDesignation, ULong);
	static PAddr		GetPPage(ULong tag, SBankInfo * bank);
	static PAddr		GetPPageWithTag(ULong tag);
	static size_t		GetRAMSize(void);
};

void
TRAMTable::Init(SBankInfo bank[])
{
	for (int i = 0; i < kMaximumMemoryPartitians; i++)
	{
		bank[i].addr = nil;
		bank[i].size = 0;
		bank[i].x0C = 0;
		bank[i].tag = kUndefined;
		bank[i].x10 = 0;
	}
}

NewtonErr
TRAMTable::Add(SBankInfo bank[], SBankInfo * info)
{
	for (int i = 0; i < kMaximumMemoryPartitians; i++)
	{
		if (bank[i].tag == kUndefined)
		{
			bank[i] = *info;
			return noErr;
		}
	}
	return kOSErrRAMTableFull;
}

void
TRAMTable::Remove(ULong, ULong, EBankDesignation, ULong)	// 0011E948
{
}

PAddr
TRAMTable::GetPPage(ULong page, SBankInfo * bank)
{
	size_t accumsize_t = 0;

	page *= kPagesize_t;
//	if ((long)tag >= 0)
	{
		for (int i = 0; i < kMaximumMemoryPartitians; i++)
		{
			if (bank[i].tag == kUndefined)
				break;
			if (bank[i].tag == 'krnl')
			{
				accumsize_t += bank[i].NormalRAMSize();
				if (page < accumsize_t)
				{
					return bank[i].addr + page - accumsize_t - bank[i].NormalRAMSize();
				}
			}
		}
	}
	return kIllegalPAddr;
}

PAddr
TRAMTable::GetPPageWithTag(ULong tag)
{
	SBankInfo * bank = gGlobalsThatLiveAcrossReboot.fBank;

	for (int i = 0; i < kMaximumMemoryPartitians; i++)
	{
		if (bank[i].tag == kUndefined)
			break;
		if (bank[i].tag == tag)
			return bank[i].addr;
	}
	return kIllegalPAddr;
}

size_t
TRAMTable::GetRAMSize(void)
{
	size_t RAMSize = 0;
	SBankInfo * bank = gGlobalsThatLiveAcrossReboot.fBank;

	for (int i = 0; i < kMaximumMemoryPartitians; i++)
	{
		if (bank[i].tag == kUndefined)
			break;
		if (bank[i].tag == 'krnl')
			RAMSize += (bank[i].x10 == 4) ? bank[i].size : 0;
	}
	return RAMSize;
}

#pragma mark -

size_t
GetRAMSize(void)
{
	return TRAMTable::GetRAMSize();
}
