#include "winquake.h"

#include <direct.h>
#include <io.h>

void Sys_Mkdir (const char *path)
{
	_mkdir (path);
}


static char	findBase[MAX_OSPATH];
static char	findPath[MAX_OSPATH];
static int	findHandle, findFlags;

static bool CheckAttributes (unsigned attrs)
{
	if (attrs & _A_SUBDIR)
	{
		if (!(findFlags & LIST_DIRS)) return false;
	}
	else
	{
		if (!(findFlags & LIST_FILES)) return false;
	}

	return true;
}


const char *Sys_FindFirst (const char *path, int flags)
{
	if (const char *s = strrchr (path, '/'))
		appStrncpyz (findBase, path, s - path + 2);	// +1 for '/' char and +1 for zero
	else
		findBase[0] = 0;

	_finddata_t findinfo;
	findHandle = _findfirst (path, &findinfo);
	if (findHandle == -1) return NULL;			// not found
	findFlags = flags;

	if (!CheckAttributes (findinfo.attrib))
		return Sys_FindNext ();

	appSprintf (ARRAY_ARG(findPath), "%s%s", findBase, findinfo.name);
	return findPath;
}

const char *Sys_FindNext ()
{
	while (findHandle != -1)
	{
		_finddata_t findinfo;
		if (_findnext (findHandle, &findinfo) == -1)
			return NULL;

		if (!CheckAttributes (findinfo.attrib))
			continue;

		appSprintf (ARRAY_ARG(findPath), "%s%s", findBase, findinfo.name);
		return findPath;
	}
	return NULL;
}

void Sys_FindClose ()
{
	if (findHandle != -1)
		_findclose (findHandle);
	findHandle = -1;
}

bool Sys_FileExists (const char *path, int flags)
{
	if (!Sys_FindFirst (path, flags)) return false;

	Sys_FindClose ();
	return true;
}
