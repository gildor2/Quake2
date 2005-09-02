#include "Core.h"
//!! SHOULD BE INCLUDED FROM "Core.h"
#include "FileSystem.h"

CFileContainer *appFileExists (const char *name, int flags, int tags)
{
	//!!
	return NULL;
}


CFileList::CFileList (const char *find, int flags, unsigned tags)
{
	//!! enum all file systems and list all of them, which tag is in "tags"
	osListDirectory (find, *this, flags);
}
