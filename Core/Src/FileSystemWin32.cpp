#include "Core.h"
#include <io.h>


void osListDirectory (const char *dir, CFileList *List, int flags)
{
	guard(appAddDirFiles);
	_finddata_t found;
	long hFind = _findfirst (dir, &found);
	if (hFind != -1)
	{
		do
		{
			int f = (found.attrib & _A_SUBDIR) ? FS_DIR : FS_FILE;
			if (f & flags)
			{
				CFileItem *item, *place;
				if (!(item = List->Find (found.name, &place)))
				{
					item = new (found.name, List) CFileItem;
					// here: flags=0
					List->InsertAfter (item, place);
				}
				item->flags |= f | FS_OS;
			}
		} while (_findnext (hFind, &found) != -1);
		_findclose (hFind);
	}
	unguard;
}
