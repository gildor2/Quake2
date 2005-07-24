#include "Core.h"

#if DBG_SYMBOLS			// do not compile unit when debug symbols are disabled, even when it was specified in a makefile

//?? should (try to) get package name from GPackage, not from memory_info -> dll_name

class CSymbol : public CStringItem
{
public:
	unsigned address;	// offset from package start
};


class CPackageSymbols : public CMemoryChain, public TList<CSymbol>
{
public:
	char	name[MAX_PKG_NAME];
};


static CPackageSymbols* pkgDebugInfo[MAX_PACKAGES];
static int dbgPackages;


void appLoadDebugSymbols ()
{
	guard(appLoadDebugSymbols);

	//?? should use engine file system (can GZip debug file)
	FILE *f = fopen (DBG_SYMBOLS_FILE, "rb");
	if (!f)
	{
		appPrintf (DBG_SYMBOLS_FILE " is not found\n");		//?? DPrintf
		return;
	}

	while (true)
	{
		char	c, pkgName[MAX_PKG_NAME];
		CSymbol *lastSym;

		// read package name
		int i = 0;

		do
		{
			if (fread (&c, sizeof(char), 1, f) != 1) appError ("unexpected EOF");
			if (i >= sizeof(pkgName)) appError ("package name too long");
			pkgName[i++] = c;
		} while (c);

		if (!pkgName[0]) break;		// eof mark

		if (dbgPackages >= MAX_PACKAGES)
			appError ("too much packages");

		CPackageSymbols *pkg = pkgDebugInfo[dbgPackages++] = new CPackageSymbols;
		lastSym = NULL;

		appStrcpy (pkg->name, pkgName);
		// read symbols
		while (true)
		{
			char	c, symName[256];
			unsigned addr;

			// read symbol address
			if (fread (&addr, sizeof(unsigned), 1, f) != 1) appError ("unexpected EOF");
			if (!addr) break;		// end of package mark
			// read symbol name
			i = 0;
			do
			{
				if (fread (&c, sizeof(char), 1, f) != 1) appError ("unexpected EOF");
				if (i >= sizeof(symName)) appError ("symbol name too long");
				symName[i++] = c;
			} while (c);

			CSymbol *sym = new (symName, pkg) CSymbol;
			sym->address = addr;
			// insert into list
			pkg->InsertAfter (sym, lastSym);
			lastSym = sym;
		}
	}

	fclose (f);

#if 0
	for (int i = 0; i < dbgPackages; i++)
	{
		CPackageSymbols *pkg = pkgDebugInfo[i];

		appPrintf ("pkg: %s\n", pkg->name);
		for (CSymbol *s = pkg->First(); s; s = pkg->Next(s))
			appPrintf ("  %08X: %s\n", s->address, s->name);
	}
#endif

	unguard;
}


bool appSymbolName (address_t addr, char *buffer, int size)
{
	char	package[256];
	int		offset;

	if (!osAddressInfo (addr, ARRAY_ARG(package), &offset))
		return false;

	// find info for our packages
	CSymbol *sym = NULL;
	for (int i = 0; i < dbgPackages; i++)
	{
		CPackageSymbols *pkg = pkgDebugInfo[i];
		if (!appStricmp (pkg->name, package))
		{
			for (CSymbol *s = pkg->First(); s; s = pkg->Next(s))
			{
				if (s->address > offset) break;
				sym = s;
			}
			break;
		}
	}

	const char *name;
	if (i == dbgPackages)
	{
		// symbol belongs to a system library
		if (osModuleInfo (addr, ARRAY_ARG(package), &offset))
			name = package;
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

	if (appSymbolName (addr, ARRAY_ARG(buf)))
		return buf;
	else
		return va("%08X", addr);
}

#endif					// DBG_SYMBOLS
