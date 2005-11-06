#include "CorePrivate.h"

#if DBG_SYMBOLS			// do not compile unit when debug symbols are disabled, even when it was specified in a makefile

#define DBG_SYMBOLS_FILE	"symbols.dbg"


class CSymbol : public CStringItem
{
public:
	unsigned address;	// offset from package start
};


class CPackageSymbols : public CMemoryChain, public TList<CSymbol>
{
public:
	TString<MAX_PKG_NAME> Name;
};


static CPackageSymbols* pkgDebugInfo[MAX_PACKAGES];
static int dbgPackages;


void appLoadDebugSymbols ()
{
	guard(appLoadDebugSymbols);

	//?? can GZip debug file
	CFile *f = appOpenFile (DBG_SYMBOLS_FILE);
	if (!f)
	{
		appPrintf (DBG_SYMBOLS_FILE " is not found\n");		//?? DPrintf
		return;
	}

	while (true)
	{
		if (f->Eof ())
		{
			appWPrintf (DBG_SYMBOLS_FILE ": unexpected EOF\n");
			break;
		}
		// read package name
		TString<256> PkgName;
		f->ReadString (PkgName);
		if (!PkgName[0]) break;		// eof mark

		if (dbgPackages >= MAX_PACKAGES)
		{
			appWPrintf (DBG_SYMBOLS_FILE": too much packages\n");
			break;
		}

		CPackageSymbols *pkg = pkgDebugInfo[dbgPackages++] = new CPackageSymbols;
		pkg->Name = PkgName;

		// read symbols
		CSymbol *lastSym = NULL;
		while (true)
		{
			// read symbol address
			unsigned addr = f->ReadInt ();		//?? can use ReadIndex() for more compact file
			if (!addr) break;		// end of package mark
			// read symbol name
			TString<256> SymName;
			f->ReadString (SymName);

			CSymbol *sym = new (SymName, pkg) CSymbol;
			sym->address = addr;
			// insert into list
			pkg->InsertAfter (sym, lastSym);	// == InsertLast()
			lastSym = sym;
		}
	}

	delete f;

#if 0
#include "OutputDeviceFile.h"
	COutputDeviceFile F("./symbols");
	for (int i = 0; i < dbgPackages; i++)
	{
		CPackageSymbols *pkg = pkgDebugInfo[i];

		F.Printf ("\n--------------\nPkg: %s\n", *pkg->Name);
		for (CSymbol *s = pkg->First(); s; s = pkg->Next(s))
			F.Printf ("  %08X: %s\n", s->address, s->name);
	}
#endif

	unguard;
}


bool appSymbolName (address_t addr, char *buffer, int size)
{
	char	package[256];
	int		offset;
	if (!osGetAddressPackage (addr, ARRAY_ARG(package), offset))
		return false;

	// find info for our packages
	CSymbol *sym = NULL;
	int i;
	for (i = 0; i < dbgPackages; i++)
	{
		CPackageSymbols *pkg = pkgDebugInfo[i];
		if (!appStricmp (pkg->Name, package))
		{
			for (TListIterator<CSymbol> s = *pkg; s; ++s)
			{
				if (s->address > offset) break;
				sym = *s;
			}
			break;
		}
	}

	const char *name;
	if (i == dbgPackages)
	{
		// symbol belongs to a system library
		// if function success, package[] will be modified
		osGetAddressSymbol (addr, ARRAY_ARG(package), offset);
	}

	if (sym)
	{
		// func + offset
		name = sym->name;
		offset -= sym->address;
	}
	else
	{
		// package + offset
		name = package;
	}
	if (offset)
		appSprintf (buffer, size, "%s+%X", name, offset);
	else
		appStrncpyz (buffer, name, size);

	return true;
}


const char *appSymbolName (address_t addr)
{
	static char	buf[256];

	if (!appSymbolName (addr, ARRAY_ARG(buf)))
		appSprintf (ARRAY_ARG(buf), "%08X", addr);
	return buf;
}

#endif					// DBG_SYMBOLS
