#define WIN32_LEAN_AND_MEAN			// exclude rarely-used services from windown headers
#include <windows.h>
#include "CorePrivate.h"

#if __MINGW32__
//?? incompatibility in headers
#define IMAGE_OPTIONAL_HEADER32		IMAGE_OPTIONAL_HEADER
#endif

static char		module[256];
static HMODULE	hModule;

bool osAddressInfo (address_t address, char *pkgName, int bufSize, int *offset)
{
	hModule = NULL;

	MEMORY_BASIC_INFORMATION mbi;
	if (!VirtualQuery ((void*) address, &mbi, sizeof(mbi)))
		return false;
	if (!(hModule = (HMODULE)mbi.AllocationBase))
		return false;
	if (!GetModuleFileName (hModule, ARRAY_ARG(module)))
		return false;

	char	*s;
	if (s = strrchr (module, '.')) *s = 0;
	if (!(s = strrchr (module, '\\')))
		s = module;
	else
		appStrcpy (module, s+1);	// remove "path\" part

	*offset = address - (unsigned)hModule;
	appStrncpyz (pkgName, module, bufSize);

	return true;
}


bool osModuleInfo (address_t address, char *exportName, int bufSize, int *offset)
{
	if (!hModule) return false;		// there was no osAddressInfo() call before this

	TString<256> Func;
	__try
	{
		// we want to walk system memory -- not very safe action
		if (*(WORD*)hModule != 'M'+('Z'<<8)) return false;		// bad MZ header
		const WORD* tmp = (WORD*) (*(DWORD*) OffsetPointer (hModule, 0x3C) + (char*)hModule);
		if (*tmp != 'P'+('E'<<8)) return false;					// non-PE executable
		IMAGE_OPTIONAL_HEADER32 *hdr = (IMAGE_OPTIONAL_HEADER32*) OffsetPointer (tmp, 4 + sizeof(IMAGE_FILE_HEADER));
		// check export directory
		if (hdr->DataDirectory[0].VirtualAddress == 0 || hdr->DataDirectory[0].Size == 0)
			return false;
		IMAGE_EXPORT_DIRECTORY* exp = (IMAGE_EXPORT_DIRECTORY*) OffsetPointer (hModule, hdr->DataDirectory[0].VirtualAddress);

		const DWORD* addrTbl = (DWORD*) OffsetPointer (hModule, exp->AddressOfFunctions);
		const DWORD* nameTbl = (DWORD*) OffsetPointer (hModule, exp->AddressOfNames);
		unsigned bestRVA = 0;
		int bestIndex = -1;
		unsigned RVA = address - (unsigned)hModule;
		for (int i = 0; i < exp->NumberOfFunctions; i++)
		{
			if (addrTbl[i] <= RVA && addrTbl[i] > bestRVA)
			{
				bestRVA = addrTbl[i];
				bestIndex = i;
			}
		}
		if (bestIndex >= 0)
		{
			if (nameTbl[bestIndex])
				Func = (char*) OffsetPointer (hModule, nameTbl[bestIndex]);
			else
				Func.sprintf ("#%d", exp->Base +		// ordinal base
					((WORD*) OffsetPointer (hModule, exp->AddressOfNameOrdinals))[bestIndex]);
			*offset = RVA - bestRVA;
		}
		else
			return false;			// when 0 < RVA < firstFuncRVA (should not happens)
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return false;
	}

	appSprintf (exportName, bufSize, "%s!%s", module, *Func);

	hModule = NULL;					// disallow second subsequentional call
	return true;
}
