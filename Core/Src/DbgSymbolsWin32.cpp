#define WIN32_LEAN_AND_MEAN			// exclude rarely-used services from windown headers
#include <windows.h>
#include "CorePrivate.h"

static char		module[256];
static HMODULE	hModule;

bool osGetAddressPackage (address_t address, char *pkgName, int bufSize, int &offset)
{
	hModule = NULL;

	MEMORY_BASIC_INFORMATION mbi;
	if (!VirtualQuery ((void*) address, &mbi, sizeof(mbi)))
		return false;
	if (!(hModule = (HMODULE)mbi.AllocationBase))
		return false;
	if (!GetModuleFileName (hModule, ARRAY_ARG(module)))
		return false;

	char *s;
	if (s = strrchr (module, '.'))	// cut extension
		*s = 0;
	if (s = strrchr (module, '\\'))
		strcpy (module, s+1);		// remove "path\" part

	offset = address - (unsigned)hModule;
	appStrncpyz (pkgName, module, bufSize);

	return true;
}


bool osGetAddressSymbol (address_t address, char *exportName, int bufSize, int &offset)
{
	if (!hModule) return false;		// there was no osGetAddressPackage() call before this

	TString<256> Func;
	TRY {							// should use IsBadReadPtr() instead of SEH for correct work under mingw32
		// we want to walk system memory -- not very safe action
		if (*(WORD*)hModule != 'M'+('Z'<<8)) return false;		// bad MZ header
		const WORD* tmp = (WORD*) (*(DWORD*) OffsetPointer (hModule, 0x3C) + (char*)hModule);
		if (*tmp != 'P'+('E'<<8)) return false;					// non-PE executable
		IMAGE_OPTIONAL_HEADER *hdr = (IMAGE_OPTIONAL_HEADER*) OffsetPointer (tmp, 4 + sizeof(IMAGE_FILE_HEADER));
		// check export directory
		if (hdr->DataDirectory[0].VirtualAddress == 0 || hdr->DataDirectory[0].Size == 0)
			return false;
		IMAGE_EXPORT_DIRECTORY* exp = (IMAGE_EXPORT_DIRECTORY*) OffsetPointer (hModule, hdr->DataDirectory[0].VirtualAddress);

		const DWORD* addrTbl = (DWORD*) OffsetPointer (hModule, exp->AddressOfFunctions);
		const DWORD* nameTbl = (DWORD*) OffsetPointer (hModule, exp->AddressOfNames);
		unsigned bestRVA = 0;
		int bestIndex    = -1;
		unsigned RVA = address - (unsigned)hModule;
		for (int i = 0; i < exp->NumberOfFunctions; i++)
		{
			if (addrTbl[i] <= RVA && addrTbl[i] > bestRVA)
			{
				bestRVA   = addrTbl[i];
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
			offset = RVA - bestRVA;
		}
		else
			return false;			// when 0 < RVA < firstFuncRVA (should not happens)
	} CATCH {
		return false;
	} END_CATCH

	appSprintf (exportName, bufSize, "%s!%s", module, *Func);

	hModule = NULL;					// disallow second subsequentional call
	return true;
}
