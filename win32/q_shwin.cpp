#include "winquake.h"

#include <direct.h>
#include <io.h>

/*
================
Sys_Milliseconds
================
*/
int	curtime;

int Sys_Milliseconds (void)
{
	static int	base;
	static bool	initialized = false;

	if (!initialized)
	{	// let base retain 16 bits of effectively random data
		base = timeGetTime() & 0xFFFF0000;
		initialized = true;
	}
	curtime = timeGetTime() - base;

	return curtime;
}

void Sys_Mkdir (char *path)
{
	_mkdir (path);
}


//============================================

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
	struct _finddata_t findinfo;

	if (const char *s = strrchr (path, '/'))
		appStrncpyz (findBase, path, s - path + 2);	// +1 for '/' char and +1 for zero
	else
		findBase[0] = 0;
	findHandle = _findfirst (path, &findinfo);
	if (findHandle == -1) return NULL;			// not found
	findFlags = flags;

	if (!CheckAttributes (findinfo.attrib))
		return Sys_FindNext ();

	appSprintf (ARRAY_ARG(findPath), "%s%s", findBase, findinfo.name);
	return findPath;
}

const char *Sys_FindNext (void)
{
	struct _finddata_t findinfo;

	while (findHandle != -1)
	{
		if (_findnext (findHandle, &findinfo) == -1)
			return NULL;

		if (!CheckAttributes (findinfo.attrib))
			continue;

		appSprintf (ARRAY_ARG(findPath), "%s%s", findBase, findinfo.name);
		return findPath;
	}
	return NULL;
}

void Sys_FindClose (void)
{
	if (findHandle != -1)
		_findclose (findHandle);
	findHandle = -1;
}
