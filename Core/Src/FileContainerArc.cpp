#include "Core.h"

//!! TODO: cache few last opened archives (make centralized "FILE* OpenArchive(filename)"/
//!!	CloseArchive())

/*-----------------------------------------------------------------------------
	Directory manipulations
-----------------------------------------------------------------------------*/

// "path" should be in a form "dir1/dir2/dir3":
//	- separators are '/'
//	- lowercase names
//	- not finished with '/'
CFileContainerArc::CArcDir *CFileContainerArc::FindDir (const char *path, bool create)
{
	if (!path || !path[0]) return &Root;

	TString<256> Copy; Copy = path;
	CArcDir *Dir = &Root;

	char *rest;
	for (const char *dirName = Copy; dirName; dirName = rest)
	{
		rest = strchr (dirName, '/');
		if (rest)
			*rest++ = 0;					// cut 1 directory layer
		CArcDir *ins;
		CArcDir *SubDir = Dir->Dirs.Find (dirName, &ins);
		if (!SubDir)
		{
			// directory does not exists
			if (!create) return NULL;		// creation is not desired
			// create subdirectory
			SubDir = new (dirName, mem) CArcDir;
			Dir->Dirs.InsertAfter (SubDir, ins);
		}
		Dir = SubDir;
	}

	return Dir;
}


CFileContainerArc::CArcFile *CFileContainerArc::FindFile (const char *filename)
{
	TString<256> Path; Path = filename;
	char *name = Path.rchr ('/');
	CArcDir *Dir;
	if (name)
	{
		*name++ = 0;				// cut filename from path
		Dir = FindDir (Path);
		if (!Dir) return NULL;		// directory not found - file does not exists too
	}
	else
	{
		name = const_cast<char*>(filename);
		Dir = &Root;
	}

	return Dir->Files.Find (name);
}


/*-----------------------------------------------------------------------------
	CFileContainer interface
-----------------------------------------------------------------------------*/

bool CFileContainerArc::FileExists (const char *filename)
{
	return FindFile (filename) != NULL;
}


CFile *CFileContainerArc::OpenFile (const char *filename)
{
	const CArcFile *FileInfo = FindFile (filename);
	if (!FileInfo) return NULL;
	return OpenFile (*FileInfo);
}


void CFileContainerArc::List (CFileList &list, const char *mask, unsigned flags)
{
	TString<256> Path; Path = mask;
	char *name = Path.rchr ('/');
	CArcDir *Dir;
	if (name)
	{
		*name++ = 0;
		Dir = FindDir (Path);
		if (!Dir) return;			// directory not found
	}
	else
	{
		name = Path;
		Dir  = &Root;
	}
	// list directories and files
#if FS_DIR != FS_FILE<<1
#error Check FS_DIR / FS_FILE relationship
#endif
	for (unsigned Flag = FS_FILE; Flag <= FS_DIR; Flag <<= 1)
	{
		if (!(flags & Flag)) continue;

		CListIterator it;
		if (Flag == FS_FILE)		// cannot write this as "it = (Flag == FS_FILE) ? ..."
			it = Dir->Files;
		else
			it = Dir->Dirs;

		for ( ; it; ++it)
		{
			if (!appMatchWildcard (it->name, name)) continue;

			CFileItem *item, *place;
			if (!(item = list.Find (it->name, &place)))
			{
				item = new (it->name, &list) CFileItem;
				// here: item->flags=0
				list.InsertAfter (item, place);
			}
			item->flags |= FS_PAK|Flag;
		}
	}
}
