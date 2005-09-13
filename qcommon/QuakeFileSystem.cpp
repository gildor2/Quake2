//#include "Core.h"
#include "qcommon.h"		//?? should be removed later

#include "FileContainerPak.h"
#include "FileContainerZip.h"

// debugging
static cvar_t	*fs_debug;
#define DEBUG_LOG(msg)	if (fs_debug->integer) appPrintf(S_GREEN"%s", msg);

static cvar_t	*fs_gamedirvar;
static cvar_t	*fs_cddir;
static cvar_t	*fs_configfile;


//?? interface -> header
class CQuakeFileSystem : public CFileSystem
{
private:
	TString<256> Name1, Name2;
	void CheckFilename (const char *name)
	{
		Name2[0] = 0;
		TString<256> Name; Name.filename (name);
		if (Name[0] == '.' && Name[1] == '/')
		{
			// absolute path - only 1 name
			Name1 = Name;
			return;
		}
		Name1.sprintf ("./%s/%s", *GameDir, *Name);
		if (!stricmp (GameDir, BASEDIRNAME)) return;	// mod == BASEDIRNAME
		Name2.sprintf ("./" BASEDIRNAME "/%s", *Name);
	}
public:
	// overloaded virtual functions
	// will check GameDir, then BASEDIRNAME (if GameDir!=BASEDIRNAME)
	bool FileExists (const char *filename)
	{
		DEBUG_LOG(va("chk: %s\n", filename));
		CheckFilename (filename);
		if (CFileSystem::FileExists (Name1))
			return true;
		if (Name2[0])
			return (CFileSystem::FileExists (Name2));
		return false;
	}
	CFile *OpenFile (const char *filename)
	{
		DEBUG_LOG(va("open: %s\n", filename));
		CheckFilename (filename);
		CFile *file = CFileSystem::OpenFile (Name1);
		if (file)
			return file;
		if (Name2[0])
			return CFileSystem::OpenFile (Name2);
		return NULL;
	}
	CFileList *List (const char *mask, unsigned flags, CFileList *list)
	{
		DEBUG_LOG(va("list: %s\n", mask));
		CheckFilename (mask);
		if (!list) list = new CFileList;
		CFileSystem::List (Name1, flags, list);
		if (Name2[0])
			CFileSystem::List (Name2, flags, list);
		return list;
	}

	TString<64> GameDir;			// not empty

	void MountGame (const char *name)
	{
		if (fs_cddir->string[0])
		{
			Mount (va("%s/%s", fs_cddir->string, *GDefMountPoint));
			Mount (va("%s/%s/*.pak", fs_cddir->string, *GDefMountPoint));
		}
		Mount (va("./%s", *GDefMountPoint));
		Mount (va("./%s/*.pak", *GDefMountPoint));
	}

	bool SetGameDir (const char *name)
	{
		guard(CQuakeFileSystem::SetGameDir);

		if (!name[0]) name = BASEDIRNAME;
		if (!stricmp (GameDir, name))
			return true;			// not changed
		// check directory
		CFileList *list = new CFileList;
		appListDirectoryOS (va("./%s/*", name), *list, FS_DIR|FS_FILE);
		bool empty = !(*list);
		delete list;
		if (empty)
		{
			appWPrintf ("Invalid game directory: %s\n", name);
			return false;
		}
		if (stricmp (GameDir, BASEDIRNAME)) // base dir not umounted
		{
			Umount (va("./%s/*.pak", *GameDir));
			Umount (va("./%s", *GameDir));
			if (fs_cddir->string[0])
			{
				Umount (va("%s/%s/*.pak", fs_cddir->string, *GameDir));
				Umount (va("%s/%s", fs_cddir->string, *GameDir));
			}
		}
		GameDir = name;
		GDefMountPoint = GameDir;
		if (stricmp (GameDir, BASEDIRNAME))	// do not remount base dir
			MountGame (name);
		Cvar_FullSet ("gamedir", name, CVAR_SERVERINFO|CVAR_NOSET);

		return true;

		unguard;
	}
};



static void cDir (bool usage, int argc, char **argv)
{
	//!!
}


static CQuakeFileSystem FS;

/*??---------------------------------------------------------------------------
	Temporary wrappers
-----------------------------------------------------------------------------*/

// Called to find where to write a file (demos, savegames, etc)
const char *FS_Gamedir ()
{
	return FS.GameDir;
}

bool FS_SetGamedir (const char *dir)
{
	return FS.SetGameDir (dir);
}

//?? move from here
void FS_LoadGameConfig ()
{
	//!! CHANGE: use FileExists("game/config", FS_OS|FS_FILE)
	FILE *f;
	if (f = fopen (va("./%s/%s", *FS.GameDir, fs_configfile->string), "r"))
	{
		fclose (f);
		Cbuf_AddText (va("unbindall\nreset *\nunalias *\nexec %s\n", fs_configfile->string));
	}
	else
		Cbuf_AddText ("exec default.cfg\n");

	if (f = fopen (va("./%s/autoexec.cfg", *FS.GameDir), "r"))
	{
		fclose (f);
		Cbuf_AddText ("exec autoexec.cfg\n");
	}
}

//!! change this mechanism
const char *FS_NextPath (const char *prevpath)
{
	static TString<64> Hold;
	if (!prevpath)
	{
		// prev==NULL => return GameDir
		Hold.sprintf ("./%s", *FS.GameDir);
		return Hold;							// return GameDir; may be == BASEDIRNAME
	}
	// here: prev==BASEDIRNAME or GameDir
	if (stricmp (prevpath+2, BASEDIRNAME))		// prev!=BASEDIRNAME => return BASEDIRNAME
		return "./" BASEDIRNAME;
	return NULL;
}

// Creates any directories needed to store the given filename
void FS_CreatePath (const char *path)
{
	// need a copy of path, because will modify (and restore) path string
	TString<MAX_OSPATH>	Copy;
	Copy.filename (path);
	DEBUG_LOG(va("path: %s\n", *Copy));
	for (char *ofs = Copy + 1; *ofs; ofs++)
	{
		if (*ofs == '/')
		{
			// create the directory
			*ofs = 0;			// cut rest of path
			Sys_Mkdir (Copy);	//!! change
			*ofs = '/';			// restore path
		}
	}
}

void FS_CopyFile (const char *src, const char *dst)
{
	FILE	*f1, *f2;

	DEBUG_LOG(va("copy(%s -> %s)", src, dst));

	if (!(f1 = fopen (src, "rb")))
	{
		DEBUG_LOG(": no src\n");
		return;
	}
	if (!(f2 = fopen (dst, "wb")))
	{
		DEBUG_LOG(": can't dst\n");
		fclose (f1);
		return;
	}

	DEBUG_LOG("\n");
	while (true)
	{
		byte	buffer[65536];

		int len = fread (buffer, 1, sizeof(buffer), f1);	// here: rec.size=1, num_recs=buf_size
		if (!len) break;
		fwrite (buffer, len, 1, f2);
	}

	fclose (f1);
	fclose (f2);
}

void FS_CopyFiles (const char *srcMask, const char *dstDir)
{
	DEBUG_LOG(va("CopyFiles(%s->%s)\n", srcMask, dstDir));

	// prepare src string
	const char *found = strrchr (srcMask, '/');
	if (!found)
	{
		appWPrintf ("CopyFiles: bad srcMask \"%s\"\n", srcMask);
		return;
	}
	int pos1 = found - srcMask + 1;

	// prepare dst string
	TString<MAX_OSPATH> Pattern;
	Pattern.filename (dstDir);
	int pos2 = Pattern.len ();
	if (!pos2 || Pattern[pos2 - 1] != '/')
	{
		Pattern[pos2] = '/';
		pos2++;
	}

	// create destination directory
	FS_CreatePath (Pattern);
#if 0
	// copy files
	// HERE: Sys_FindFirst() uses fill path names
	for (found = Sys_FindFirst (srcMask, LIST_FILES); found; found = Sys_FindNext ())
	{
		strcpy (Pattern + pos2, found + pos1);
		FS_CopyFile (found, Pattern);
	}
	Sys_FindClose ();
#else
	appWPrintf ("COPY FILES: %s -> %s\n", srcMask, dstDir);	//!!!!!
#endif
}

void FS_RemoveFiles (const char *mask)
{
#if 0
	DEBUG_LOG(va("RemoveFiles(%s)\n", mask));
	// HERE: Sys_FindFirst() uses full path names
	for (const char *found = Sys_FindFirst (mask, LIST_FILES); found; found = Sys_FindNext ())
	{
		DEBUG_LOG(va("del(%s)\n", found));
		remove (found);
	}
	Sys_FindClose ();
#else
	appWPrintf ("REMOVE FILES: %s\n", mask);	//!!!!!
#endif
}


/*-----------------------------------------------------------------------------
	Initialization
-----------------------------------------------------------------------------*/

void InitFileSystem ()
{
	guard(InitFileSystem);

	appInitFileSystem (FS);
	CFileSystem::RegisterFormat (CFileContainerPak::Create);
	CFileSystem::RegisterFormat (CFileContainerZip::Create);
	CFileContainer *Cont = FS.MountDirectory (".");	// standard mount
	Cont->locked = true;
	//!! mount+lock resources

	// initialize interface
CVAR_BEGIN(vars)
	CVAR_FULL(&fs_configfile, "cfgfile", CONFIGNAME, CVAR_NOSET),	//?? config system will be changed later
	// cddir <path>	-- allow game to be partially installed - read missing data from CD
	CVAR_FULL(&fs_cddir, "cddir", "", CVAR_NOSET),
	CVAR_FULL(&fs_gamedirvar, "game", "", CVAR_SERVERINFO|CVAR_LATCH),
	CVAR_VAR(fs_debug, 0, 0)		//?? unused now?
CVAR_END
	Cvar_GetVars (ARRAY_ARG(vars));
	RegisterCommand ("dir", cDir);

	if (fs_cddir->string[0])
		appPrintf ("FS: using \"%s\" as CDROM image\n", fs_cddir->string);
	GDefMountPoint = BASEDIRNAME;
	FS.MountGame (BASEDIRNAME);
	FS.GameDir = BASEDIRNAME;
	FS.SetGameDir (fs_gamedirvar->string);

	unguard;
}
