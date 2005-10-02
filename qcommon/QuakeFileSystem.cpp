//#include "Core.h"
#include "qcommon.h"		//?? should be removed later -- used for cvar and BASEDIRNAME only

// supported archive formats
#include "FileContainerPak.h"
#include "FileContainerZip.h"
#include "FileContainerZRes.h"

// debugging
#include "OutputDeviceFile.h"

#define FS_LOG	"fs.log"
static cvar_t	*fs_logFile;
static COutputDevice *FSLog;


static cvar_t	*fs_game;
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
	bool FileExists (const char *filename, unsigned flags)
	{
		CheckFilename (filename);
		// check
		bool chk = CFileSystem::FileExists (Name1, flags);
		if (!chk && Name2[0])
			chk = CFileSystem::FileExists (Name2, flags);
		// log
		static const char boolStr[] = {'-', '+'};
		FSLog->Printf ("chk: %s %c\n", filename, boolStr[chk]);
		return chk;
	}
	CFile *OpenFile (const char *filename, unsigned flags)
	{
		FSLog->Printf ("open: %s ", filename);
		CheckFilename (filename);
		// open
		CFile *file = CFileSystem::OpenFile (Name1, flags);
		if (!file && Name2[0])
			file = CFileSystem::OpenFile (Name2, flags);
		// log
		if (file)
			FSLog->Printf ("(%s)\n", file->Owner->name);
		else
			FSLog->Printf ("(failed)\n");
		return file;
	}
	CFileList *List (const char *mask, unsigned flags, CFileList *list)
	{
		FSLog->Printf ("list: %s\n", mask);
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
			Mount (va("%s/%s/*.pak", fs_cddir->string, *GDefMountPoint));
		Mount (va("./%s/*.pak", *GDefMountPoint));
	}

	// return "true" when game was changed
	bool SetGameDir (const char *name)
	{
		guard(CQuakeFileSystem::SetGameDir);

		FSLog->Printf ("Set gamedir: %s\n", name);
		if (!name[0]) name = BASEDIRNAME;
		if (!stricmp (GameDir, name))
			return false;				// not changed
		// check directory
		CFileList *list = List (va("./%s/*", name), FS_DIR|FS_FILE|FS_OS, NULL);
		bool empty = !(*list);
		delete list;
		if (empty)
		{
			appWPrintf ("Invalid game directory: %s\n", name);
			return false;
		}
		if (GameDir != BASEDIRNAME)		// base dir not umounted
		{
			Umount (va("./%s/*.pak", *GameDir));
			if (fs_cddir->string[0])
				Umount (va("%s/%s/*.pak", fs_cddir->string, *GameDir));
		}
		GameDir.filename (name);
		GDefMountPoint = GameDir;
		if (GameDir != BASEDIRNAME)		// do not remount base dir
			MountGame (name);

		return true;

		unguard;
	}
};


static CQuakeFileSystem FS;

/*??---------------------------------------------------------------------------
	Temporary wrappers
-----------------------------------------------------------------------------*/

// Called to find where to write a file (demos, savegames, etc)
const char *FS_Gamedir ()
{
	return FS.GameDir;
}

static void LoadGameConfig ()
{
	if (GFileSystem->FileExists (va("./%s/%s", *FS.GameDir, fs_configfile->string), FS_OS))
		Cbuf_AddText (va("unbindall\nreset *\nunalias *\nexec %s\n", fs_configfile->string));
	else
		Cbuf_AddText ("exec default.cfg\n");

	if (GFileSystem->FileExists (va("./%s/autoexec.cfg", *FS.GameDir), FS_OS))
		Cbuf_AddText ("exec autoexec.cfg\n");
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
	// here: prev==BASEDIRNAME
	return NULL;
}


void FS_CopyFile (const char *src, const char *dst)
{
	FILE	*f1, *f2;

	FSLog->Printf ("copy(%s -> %s)", src, dst);

	if (!(f1 = fopen (src, "rb")))
	{
		FSLog->Printf (": no src\n");
		return;
	}
	if (!(f2 = fopen (dst, "wb")))
	{
		FSLog->Printf (": can't dst\n");
		fclose (f1);
		return;
	}

	FSLog->Printf ("\n");
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
	FSLog->Printf ("CopyFiles(%s->%s)\n", srcMask, dstDir);

	TString<256> Base; Base.filename (srcMask);
	char *s = Base.rchr ('/');
	if (!s) s = Base;
	else s++;								// will point to mask start

	// prepare dst string: should ends with '/'
	TString<256> Pattern; Pattern.filename (dstDir);
	int pos = Pattern.len ();
	if (!pos || Pattern[pos - 1] != '/')
		Pattern[pos++] = '/';

	appMakeDirectoryForFile (Pattern);
#if 1
	CFileList *list = new CFileList;
	appListDirectoryOS (srcMask, *list, FS_FILE);
#else
	CFileList *list = GFileSystem->List (srcMask, FS_OS);
#endif
	for (TListIterator<CFileItem> it = *list; it; ++it)
	{
		strcpy (s, it->name);				// create source filename
		strcpy (Pattern + pos, it->name);	// create destination filename
		FS_CopyFile (Base, Pattern);
	}
	delete list;
}

void FS_RemoveFiles (const char *mask)
{
	FSLog->Printf (va("RemoveFiles(%s)\n", mask));

	TString<256> Base; Base.filename (mask);
	char *s = Base.rchr ('/');
	if (!s) s = Base;
	else s++;								// will point to mask start

#if 1
	CFileList *list = new CFileList;
	appListDirectoryOS (mask, *list, FS_FILE);
#else
	CFileList *list = GFileSystem->List (mask, FS_OS);
#endif
	for (TListIterator<CFileItem> it = *list; it; ++it)
	{
		strcpy (s, it->name);				// create full filename
		remove (Base);
	}
	delete list;
}


/*-----------------------------------------------------------------------------
	Initialization, ticking
-----------------------------------------------------------------------------*/

extern "C" byte zresource[];


void InitFileSystem ()
{
	guard(InitFileSystem);

	// initialize interface
CVAR_BEGIN(vars)
	CVAR_FULL(&fs_configfile, "cfgfile", CONFIGNAME, CVAR_NOSET),	//?? config system will be changed later
	// cddir <path>	-- allow game to be partially installed - read missing data from CD
	CVAR_FULL(&fs_cddir, "cddir", "", CVAR_NOSET),
	CVAR_FULL(&fs_game, "game", "", CVAR_SERVERINFO|CVAR_LATCH),
	CVAR_VAR(fs_logFile, 0, 0)
CVAR_END
	Cvar_GetVars (ARRAY_ARG(vars));

	CFileContainer *Mnt;

	FSLog = GNull;
	appInitFileSystem (FS);
	CFileSystem::RegisterFormat (CFileContainerPak::Create);
	CFileSystem::RegisterFormat (CFileContainerZip::Create);
	// mount resources
	Mnt = CZResource::Mount ("resources", zresource, ".");
	Mnt->locked = true;
	// mount CDROM
	if (fs_cddir->string[0])
	{
		Mnt = FS.MountDirectory (fs_cddir->string);
		Mnt->locked = true;
		appPrintf ("FS: using \"%s\" as CDROM image\n", fs_cddir->string);
	}
	// mount root file container
	Mnt = FS.MountDirectory (".");
	Mnt->locked = true;
	// mount basedir
	GDefMountPoint = BASEDIRNAME;
	FS.GameDir = BASEDIRNAME;
	FS.MountGame (BASEDIRNAME);
	// change game, if needed
	FS.SetGameDir (fs_game->string);
	// load configuration
	LoadGameConfig ();

	unguard;
}


//?? make as CQuakeFileSystem::Tick()
void CL_WriteConfiguration (const char *filename);

void FS_Tick ()
{
	// process "fs_logFile" changes
	if (fs_logFile->modified)
	{
		fs_logFile->modified = false;

		if (FSLog != GNull && FSLog != GLog)
			delete FSLog;

		switch (fs_logFile->integer)
		{
		case 0:
			FSLog = GNull;
			break;
		case 1:
			FSLog = GLog;		//?? colorized log
			break;
		default:				// case 2
			FSLog = new COutputDeviceFile (FS_LOG);
			FSLog->Printf ("\n*** File system activity, %s ***\n\n", appTimestamp ());
			break;
		}
	}

	// process "game" changes
	if (fs_game->modified)
	{
		if (!stricmp (fs_game->string, BASEDIRNAME))
			Cvar_ForceSet ("game", "");	// BASEDIRNAME->""
		CL_WriteConfiguration (fs_configfile->string);
		if (FS.SetGameDir (fs_game->string))
		{
			LoadGameConfig ();
			if (!DEDICATED)
				Cbuf_AddText ("vid_restart\nsnd_restart\n");
		}
		Cvar_ForceSet ("game", (FS.GameDir == BASEDIRNAME) ? "" : FS.GameDir);
		fs_game->modified = false;
	}
}
