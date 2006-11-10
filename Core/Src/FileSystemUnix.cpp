#include "CorePrivate.h"
#include <dirent.h>
#include <sys/stat.h>


void appListDirectory(const char *mask, CFileList &List, unsigned flags)
{
	guard(appListDirectory);

	TString<256> Path, Mask;
	Path.filename(mask);
	char *s = Path.rchr('/');
	if (s)
	{
		*s++ = 0;									// cut filename from path
		Mask = s;
	}
	else
	{
		Mask = Path;
		Path = ".";
	}

//	appPrintf("mask=[%s] Path=[%s] Mask=[%s]\n", mask, *Path, *Mask);	//!!

	DIR *dir = opendir(Path);
	if (!dir) return;

	while (struct dirent *ent = readdir(dir))
	{
		// process hidden files
		if (!(flags & FS_LIST_HIDDEN) && ent->d_name[0] == '.') continue;

		// get/check entry attributes
		struct stat buf;
		TString<256> Full; Full.sprintf("%s/%s", *Path, ent->d_name);
		if (stat(Full, &buf) < 0) continue;			// or break?
		int f = 0;
		if (S_ISDIR(buf.st_mode))
			f = FS_DIR;
		else if (S_ISREG(buf.st_mode))				// regular file
			f = FS_FILE;
		if (!(f & flags)) continue;

		// lowercase found.name
		TString<256> Lower; Lower.toLower(ent->d_name);
		// refine by wildcard
		if (!appMatchWildcard(Lower, Mask)) continue;
		// process NOEXT
		if (flags & FS_NOEXT)
		{
			char *s = Lower.chr('.');
			if (s) *s = 0;
		}
		// add to list
		CFileItem *item, *place;
		if (!(item = List.Find(Lower, &place)))
		{
			item = new (Lower, &List) CFileItem;
			// here: item->flags=0
			List.InsertAfter(item, place);
		}
		item->flags |= f | FS_OS;
	}

	closedir(dir);

	unguard;
}


void appMakeDirectory(const char *dirname)
{
	if (!dirname[0]) return;
	// mkdir() does not support "a/b/c" creation,
	// so - we will create "a", then "a/b", then "a/b/c"
	TString<256> Name; Name.filename(dirname);
	for (char *s = *Name; /* empty */ ; s++)
	{
		char c = *s;
		if (c != '/' && c != 0)
			continue;
		*s = 0;						// temporarily cut rest of path
		// here: path delimiter or end of string
		if (Name[0] != '.' || Name[1] != 0)		// do not create "."
			mkdir(Name, S_IRWXU);
		if (!c) break;				// end of string
		*s = '/';					// restore string (c == '/')
	}
}


unsigned appFileType(const char *filename)
{
	struct stat buf;
	if (stat(filename, &buf) < 0)
		return 0;					// no such file/dir
	if (S_ISDIR(buf.st_mode))
		return FS_DIR;
	else if (S_ISREG(buf.st_mode))
		return FS_FILE;
	return 0;						// unsupported file type
}
