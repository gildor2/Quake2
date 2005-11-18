#include "CorePrivate.h"

#if __CYGWIN__
#define NO_DLADDR	1
#endif

/* NOTES:
	- Cygwin and Unix have have alternative API for resolving symbol names: BFD
	- dladdr() family allows us to retreive info about current process only
	- using "/proc/(num)/maps" works for any process
*/

#if NO_DLADDR

bool osGetAddressPackage (address_t address, char *pkgName, int bufSize, int &offset)
{
	FILE *f = fopen ("/proc/self/maps", "r");
	if (!f) return false;	// should not happens ...

	char line[1024];
	while (fgets (ARRAY_ARG(line), f))
	{
		char mapFile[256];
		unsigned mapStart, mapEnd, mapOffset;
		if (sscanf (line, "%x-%x %*15s %x %*x:%*x %*u %255s", &mapStart, &mapEnd, &mapOffset, mapFile) == 4)
		{
			if (address < mapStart || address >= mapEnd) continue;	// not this file
			char *s = strrchr (mapFile, '/');
			if (s) s++;
			else s = mapFile;
#if __CYGWIN__
			// under normal Unix system, "offset" is offset of mapped part from file start (single
			// file can be mapped by parts with a different access rights);
			// but, under cygwin, this field contains exe/dll entry point ...
			mapOffset = 0;
#endif
			// cut extension + version ("lib.so.version" => strchr() instead of strrchr())
			char *ext = strchr (s, '.');
			if (ext) *ext = 0;
			appStrncpyz (pkgName, s, bufSize);
			offset = address - mapStart + mapOffset;

			fclose (f);
			return true;
		}
	}
	// not found
	fclose (f);
	return false;
}

bool osGetAddressSymbol (address_t address, char *exportName, int bufSize, int &offset)
{
	return false;
}

#else // NO_DLADDR; here: supports dladdr()

static char module[256];

bool osGetAddressPackage (address_t address, char *pkgName, int bufSize, int &offset)
{
	Dl_info info;
	if (!dladdr ((void*) address, &info)) return false;
	appStrncpyz (module, info.dli_fname, bufSize);

	char *s;
	if (s = strrchr (module, '/'))
		strcpy (module, s+1);		// remove "path/" part
	if (s = strchr (module, '.'))
		*s = 0;						// cut extension + version

	offset = address - (address_t)info.dli_fbase;
	appStrncpyz (pkgName, module, bufSize);

	return true;
}

bool osGetAddressSymbol (address_t address, char *exportName, int bufSize, int &offset)
{
	Dl_info info;
	if (!dladdr ((void*) address, &info)) return false;
	// check: address is inside module, but no symbol reference
	if (!info.dli_sname) return false;
	appSprintf (exportName, bufSize, "%s!%s", module, info.dli_sname);
	offset = address - (address_t)info.dli_saddr;
	return true;
}

#endif // NO_DLADDR
