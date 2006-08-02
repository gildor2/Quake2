#include "CorePrivate.h"
#include <io.h>						// for findfirst() set
#include <direct.h>					// for mkdir()
#include <sys/stat.h>				// for stat()

/*
	We use app wildcard matcher. This allows:
	1. full compatibility of mask names in OS and virtual FS layers
	2. case-insensitive file names under Unix (different cpp file!)
		- but dir names case sensitive ...
		- and OpenFile()/FileExists() will require to use ListDirectoryOS() function
*/

void appListDirectory (const char *mask, CFileList &List, unsigned flags)
{
	guard(appListDirectory);

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


void appMakeDirectory (const char *dirname)
{
	if (!dirname[0]) return;
	// mkdir() and win32 CreateDirectory() does not support "a/b/c" creation,
	// so - we will create "a", then "a/b", then "a/b/c"
	TString<256> Name; Name.filename (dirname);
	for (char *s = *Name; /* empty */ ; s++)
	{
		char c = *s;
		if (c != '/' && c != 0)
			continue;
		*s = 0;						// temporarily cut rest of path
		// here: path delimiter or end of string
		if (Name[0] != '.' || Name[1] != 0)		// do not create "."
			_mkdir (Name);
		if (!c) break;				// end of string
		*s = '/';					// restore string (c == '/')
	}
}


#ifndef S_ISDIR
// no such declarations in windows headers, but exists in mingw32 ...
#define	S_ISDIR(m)	(((m) & S_IFMT) == S_IFDIR)
#define	S_ISREG(m)	(((m) & S_IFMT) == S_IFREG)
#endif

unsigned appFileType (const char *filename)
{
	struct stat buf;
	if (stat (filename, &buf) == -1)
		return 0;					// no such file/dir
	if (S_ISDIR(buf.st_mode))
		return FS_DIR;
	else if (S_ISREG(buf.st_mode))
		return FS_FILE;
	return 0;						// just in case ... (may be, win32 have other file types?)
}
