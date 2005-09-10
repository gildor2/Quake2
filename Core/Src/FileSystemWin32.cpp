#include "Core.h"
#include <io.h>

/*
	We use app wildcard matcher. This will allow:
	1. full compatibility of mask names in OS and virtual FS layers
	2. case-insensitive file names under Unix (different cpp file!)
		- but dir names case sensitive ...
		- and OpenFile()/FileExists() will require to use ListDirectoryOS() function
*/

void appListDirectoryOS (const char *mask, CFileList &List, unsigned flags)
{
	guard(appListDirectoryOS);

	TString<256> Path, Mask;
	Path.filename (mask);
	char *s = Path.rchr ('/');
	if (s)
	{
		*s++ = 0;					// cut filename from path
		Mask = s;
	}
	else
	{
		Mask = Path;
		Path = ".";
	}
	Path += "/*";					// all files

//	appPrintf("mask=[%s] Path=[%s] Mask=[%s]\n", mask, *Path, *Mask);

	_finddata_t found;
	long hFind = _findfirst (Path, &found);
	if (hFind == -1) return;

	do
	{
		if (!(flags & FS_LIST_HIDDEN) && found.name[0] == '.') continue;
		int f = (found.attrib & _A_SUBDIR) ? FS_DIR : FS_FILE;
		if (!(f & flags)) continue;

		// lowercase found.name
		appStrncpylwr (found.name, found.name, sizeof(found.name));
		// refine by wildcard
		if (!appMatchWildcard (found.name, Mask)) continue;
		// process NOEXT
		if (flags & FS_NOEXT)
		{
			char *s = strchr (found.name, '.');
			if (s) *s = 0;
		}
		// add to list
		CFileItem *item, *place;
		if (!(item = List.Find (found.name, &place)))
		{
			item = new (found.name, &List) CFileItem;
			// here: item->flags=0
			List.InsertAfter (item, place);
		}
		item->flags |= f | FS_OS;
	} while (_findnext (hFind, &found) != -1);

	_findclose (hFind);

	unguard;
}
