/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

// NOTE: if remove Hunk() functions, can remove Id Software (c) !!

#define WIN32_LEAN_AND_MEAN			// exclude rarely-used services from windown headers
#include <windows.h>
#include <mmsystem.h>

#include <direct.h>
#include <io.h>

#include "../qcommon/qcommon.h"


//===============================================================================


static byte	*membase;
static int	hunkmaxsize;
static int	cursize;

#define	VIRTUAL_ALLOC

void *Hunk_Begin (int maxsize)
{
	// reserve a huge chunk of memory, but don't commit any yet
	cursize = 0;
	hunkmaxsize = maxsize;
#ifdef VIRTUAL_ALLOC
	membase = (byte*)VirtualAlloc (NULL, maxsize, MEM_RESERVE, PAGE_NOACCESS);
#else
	membase = (byte*)malloc (maxsize);
	memset (membase, 0, maxsize);
#endif
	if (!membase)
		Sys_Error ("VirtualAlloc reserve failed");
	return (void *)membase;
}

void *Hunk_Alloc (int size)
{
	void	*buf;

	// round to cacheline
	size = (size+31)&~31;

#ifdef VIRTUAL_ALLOC
	// commit pages as needed
//	buf = VirtualAlloc (membase+cursize, size, MEM_COMMIT, PAGE_READWRITE);
	buf = VirtualAlloc (membase, cursize+size, MEM_COMMIT, PAGE_READWRITE);
	if (!buf)
	{
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &buf, 0, NULL);
		Sys_Error ("VirtualAlloc commit failed.");
	}
#endif
	cursize += size;
	if (cursize > hunkmaxsize)
		Sys_Error ("Hunk_Alloc overflow");

	return (void *)(membase+cursize-size);
}

int Hunk_End (void)
{
	// free the remaining unused virtual memory
#if 0
	void	*buf;

	// write protect it
	buf = VirtualAlloc (membase, cursize, MEM_COMMIT, PAGE_READONLY);
	if (!buf)
		Sys_Error ("VirtualAlloc commit failed");
#endif

	return cursize;
}

void Hunk_Free (void *base)
{
	if (base)
#ifdef VIRTUAL_ALLOC
		VirtualFree (base, 0, MEM_RELEASE);
#else
		free (base);
#endif
}


//===============================================================================


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

char *Sys_FindFirst (char *path, int flags)
{
	struct _finddata_t findinfo;

	COM_FilePath (path, findBase);
	findHandle = _findfirst (path, &findinfo);
	if (findHandle == -1) return NULL;			// not found
	findFlags = flags;

	if (!CheckAttributes (findinfo.attrib))
		return Sys_FindNext ();

	Com_sprintf (ARRAY_ARG(findPath), "%s/%s", findBase, findinfo.name);
	return findPath;
}

char *Sys_FindNext (void)
{
	struct _finddata_t findinfo;

	while (findHandle != -1)
	{
		if (_findnext (findHandle, &findinfo) == -1)
			return NULL;

		if (!CheckAttributes (findinfo.attrib))
			continue;

		Com_sprintf (ARRAY_ARG(findPath), "%s/%s", findBase, findinfo.name);
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
