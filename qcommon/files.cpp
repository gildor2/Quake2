#include "qcommon.h"

#include "zip.h"


static bool initialized;

/*------------ In memory -----------------*/

class packFile_t : public CStringItem
{
public:
	int		method; 		// 0-store, 8-deflate
	int		pos;			// position of data in .PAK file
	int		cSize;			// size of compressed data
	int		ucSize;			// size of uncompressed data
	int		crc;			// 0 for old (uncompressed) .PAK files
};

class packDir_t : public CStringItem
{
public:
	TList<packDir_t> cDir;	// 1st subdirectory (child subdirectory)
	TList<packFile_t> cFile; // 1st file in this directory (child file)
};

class pack_t
{
public:
	char	*filename;		// .PAK filename (statistics)
	packDir_t *root;		// root directory of .PAK file
	int		numFiles;		// number of files in .PAK (statistics)
	bool	isZip;			// true if PKWare ZIP file (statistics)
	CMemoryChain *chain;	// chain of memory blocks (freed all at once)
};

class resFile_t : public CStringItem
{
public:
	int		pos;
	int		size;
};


/*..............................................................

Internal representation of FILE
for FS_FOpenFile,FS_CloseFile
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
It can be used to point to
 1) "r" - regular files
 2) "i" - id pak files
 3) "z" - zipped pak files
State of this structure:
 1) "a" - allocated (created with FS_OpenFIle())
 2) "o" - opened (changed with a 1st call of FS_ReadFile())
Details (filled fields):
 1) ra: name -> filename
 2) ro: +file
 3) ia: name -> pak name, pfile
 4) io: +file -> pak
 5) za: name -> pak name, pfile, zipped=1
 6) zo: +file -> pak, +zfile -> zip file (?? update this comment for "zipped"->"type")

..............................................................*/

typedef enum
{
	FT_NORMAL,			// regular OS file
	FT_PAK,				// file from non-compressed PAK
	FT_ZPAK,			// file from ZIP
	FT_ZMEM				// inline file
} fileType_t;


class FILE2 : public CStringItem
{
	// used CStringItem::name for info
public:
	fileType_t type;
	FILE	*file;		// handle of file (if non-zipped) or .PAK
	union {
		struct {
			ZFILE	*zFile;		// handle of zipped file (or 0 if non-zipped)
			packFile_t *pFile;	// pointer to a corresponding pak structure
		};
		struct {
			ZBUF	*zBuf;		// handle of in-memory zipped block
			resFile_t *rFile;
		};
	};
};

cvar_t	*fs_gamedirvar;

// debugging
static cvar_t	*fs_debug;
#define DEBUG_LOG(msg)	if (fs_debug->integer) Com_Printf(S_GREEN"%s", msg);


static cvar_t	*fs_basedir;
static cvar_t	*fs_configfile;
#ifdef CD_PATH
static cvar_t	*fs_cddir;
#endif
static char	fs_gamedir[MAX_OSPATH];

typedef struct fileLink_s
{
	struct fileLink_s *next;
	char	*from;
	int		fromlength;
	char	*to;
} fileLink_t;

static fileLink_t *fs_links;

class searchPath_t : public CStringItem
{
public:
	pack_t	*pack;			// only "filename" or "pack" will be used
};

static TList<searchPath_t> fs_searchpaths;
static searchPath_t *fs_base_searchpaths;	// without gamedirs


static const char *is_zip_str[2] = {"", " [zip]"};


/*------------------ Resources ----------------------------*/

#define MAX_RES_FILES	64

extern "C" byte zresource_start[], zresource_end[];
#define OpenZRes() 	Zip_OpenBuf (zresource_start+10, zresource_end-zresource_start-18);	// +10 - skip gzip header, -18 - dhr+crc+size



static CMemoryChain *resChain;
static TList<resFile_t> resFiles;
static int resFileCount;


static void InitResFiles (void)
{
	ZBUF	*z;
	char	c, name[MAX_OSPATH], *s;
	int		fsize, offs;
	resFile_t *f, *last;

	resChain = new CMemoryChain;
	z = OpenZRes();

	last = NULL;
	while (Zip_ReadBuf (z, &fsize, sizeof(int))/* && fsize*/)
	{
		if (!fsize) break;
		s = name;
		// get filename
		do
		{
			Zip_ReadBuf (z, &c, 1);
			*s++ = c;
		} while (c);
		// create file struc
		f = new (name, resChain) resFile_t;
		f->size = fsize;
		// add to chain
		resFiles.InsertAfter (f, last);

		resFileCount++;
		last = f;
	}
	// now zstream points to begin of 1st file data - need to calculate file offsets
	offs = z->readed;
	for (f = resFiles.First(); f; f = resFiles.Next(f))
	{
		f->pos = offs;
		offs += f->size;
//		Com_Printf ("res: %s (%d bytes) at %d\n", f->name, f->size, f->pos);
	}

	Zip_CloseBuf (z);
	Com_Printf ("Added %d inline files\n", resFileCount);
}


/*
=====================================================
Functions to work with in-memory directory/file trees
=====================================================
*/

static void AddDirFilesToList (const char *findname, TList<CStringItem> *List, int flags)
{
	char	pattern[MAX_OSPATH], wildcard[MAX_OSPATH];

	appStrncpyz (pattern, findname, sizeof(pattern));
	char *mask = strrchr (pattern, '/');
	if (mask)
	{
		strcpy (wildcard, mask+1);
		strcpy (mask+1, "*");			// wildcard -> "*" -- list all files
	}
	else
		wildcard[0] = 0;				// no path in filename ? (should not happens)

	for (const char *s = Sys_FindFirst (pattern, flags); s; s = Sys_FindNext ())
	{
		if (!wildcard[0] || s[strlen (s)-1] != '.')	// ignore "." and ".." directory items
		{
			const char *name = strrchr (s, '/');	// should always be not NULL
			if (name && appMatchWildcard (name+1, wildcard, true))
			{
				CStringItem *item, *place;
				if (!(item = List->Find (s, &place)))
				{
					item = new (s) CStringItem;
					appStrncpylwr (item->name, item->name, BIG_NUMBER);		//!! lowercase the name
					List->InsertAfter (item, place);
				}
			}
		}
	}
	Sys_FindClose ();
}


// Find directory "name" in "pak". Returns NULL if not found.
static packDir_t *FindPakDirectory (pack_t *pak, const char *name)
{
	packDir_t *dir = pak->root;
	const char *s = name;
	if (!name || !*name || name[0] == '/' && !name[1]) return dir; // empty string or "/" => root directory

	while (true)
	{
		char	*rest, dirname[MAX_OSPATH];
		int		len;
		packDir_t *newdir;

		// get name of the next directory into dirname
		rest = strchr (s, '/');
		if (!rest)	len = strlen (s);
		else		len = rest - s;
		appStrncpyz (dirname, s, len+1);

		if (!dir->cDir.First ())
			return NULL;				// current directory have no childs
		else
		{
			newdir = dir->cDir.Find (dirname);
			if (!newdir) return NULL;	// directory not found
		}
		// rest=NULL or ->"/\0" or ->"/another_text..."
		if (!rest || !rest[1]) return newdir; // found
		s = rest + 1;					// skip '/'
		dir = newdir;
	}
}


// Returns packFile_t structure or NULL
static packFile_t *FindPakFile (pack_t *pak, const char *name)
{
	const char *filename;
	packDir_t *dir;

	filename = strrchr (name, '/');
//	DebugPrintf("Searching file %s (%s) in %s ...\n", filename, name, pak->filename);
	if (!filename)
	{	// root directory
		filename = name;
		dir = pak->root;
	}
	else
	{
		char	dirname[MAX_OSPATH], *dirname2;

		appStrncpyz (dirname, name, filename - name + 1);
		// process "./directory/file" names
		dirname2 = dirname;
		if (dirname[0] == '.' && dirname[1] == '/') dirname2 += 2;
		dir = FindPakDirectory (pak, dirname2);
		if (!dir) return NULL;
		filename++;			// skip '/'
	}
//	DebugPrintf("  ... dir '%s' is found; searching for '%s'\n", dir->name, filename);
	return dir->cFile.Find (filename);
}


/*----------------- Adding stuff ---------------------*/

// Find directory "name" in "pak". If not found - create it.
// Returns found or new directory.
static packDir_t *AddPakDirectory (pack_t *pak, const char *name)
{
	char	dirname[MAX_OSPATH], *rest;
	int		len;

	packDir_t *dir = pak->root;
	const char *s = name;
	if (!name || !*name || name[0] == '/' && !name[1]) return dir; // empty string or "/" => root directory

	while (true)
	{
		// get name of the next directory into dirname
		rest = strchr (s, '/');
		if (!rest) len = strlen (s);
		else len = rest - s;
		Q_strncpylower (dirname, s, len);

		packDir_t *insdir;
		packDir_t *newdir = dir->cDir.Find (dirname, &insdir);
		if (!newdir)
		{
			// not found - create and insert
			newdir = new (dirname, pak->chain) packDir_t;
			dir->cDir.InsertAfter (newdir, insdir);
		}
		// rest=NULL or ->"/\0" or ->"/another_text..."
		if (!rest || !rest[1]) return newdir;
		s = &rest[1];					// skip '/'
		dir = newdir;
	}
}


// Returns UNFILLED packFile_t structure
static packFile_t *AddPakFile (pack_t *pak, char *name)
{
	char	dirname[MAX_OSPATH];
	packDir_t *dir;
	packFile_t *file, *insfile;

	char *filename = strrchr (name, '/');
	if (!filename)
	{	// root directory
		filename = name;
		dir = pak->root;
	}
	else
	{
		Q_strncpylower (dirname, name, filename - name);
		filename++;						// skip '/'
		if (!*filename) return NULL;	// empty name
		dir = AddPakDirectory (pak, dirname);
	}
	file = dir->cFile.Find (filename, &insfile);
	if (!file)
	{
		// normally (if correct .PAK/.ZIP), should newer be found - always create
		file = new (filename, pak->chain) packFile_t;
		appStrncpylwr (file->name, file->name, BIG_NUMBER);		// lowercase the name !!
		dir->cFile.InsertAfter (file, insfile);

		pak->numFiles++;
	}
	return file;
}


// Appends contents of pak directory dir with mask onto a list. Returns start of list.
// list can be NULL (will be created). attr is combination of LISTPAK_FILES and LISTPAK_DIRS
static void ListPakDirectory (pack_t *pak, const char *dir, const char *mask, int flags, TList<CStringItem> *List, const char *prefix)
{
	packDir_t *d, *dirlist;
	packFile_t *filelist;
	char	addbuf[MAX_OSPATH], *addbufptr;

	strcpy (addbuf, prefix);
	addbufptr = strchr (addbuf, 0);		// find end of line
	if (d = FindPakDirectory (pak, dir))
	{
		if (flags & LIST_DIRS)
			for (dirlist = d->cDir.First(); dirlist; dirlist = d->cDir.Next(dirlist))
				if (appMatchWildcard (dirlist->name, mask, true))
				{
					strcpy (addbufptr, dirlist->name);		//?? can use va()
					List->CreateAndInsert (addbuf);
				}
		if (flags & LIST_FILES)
			for (filelist = d->cFile.First(); filelist; filelist = d->cFile.Next(filelist))
				if (appMatchWildcard (filelist->name, mask, true))
				{
					strcpy (addbufptr, filelist->name);
					List->CreateAndInsert (addbuf);
				}
	}
}


/*------------- Debug stuff -------------------*/

static void DumpPakDirectory (packDir_t *dir, FILE *log)
{
	int		i;
	static int dumpPakLevel = 0;

	for (i = 0; i < dumpPakLevel * 2; i++) fputc (' ', log);
	fprintf (log, "%s/\n", dir->name);
	dumpPakLevel++;
	for (packFile_t *f = dir->cFile.First(); f; f = dir->cFile.Next(f))
	{
		for (i = 0; i < dumpPakLevel * 2; i++) fputc (' ', log);
		fprintf (log, "%s\n", f->name);
	}
	for (packDir_t *d = dir->cDir.First(); d; d = dir->cDir.Next(d))
		DumpPakDirectory (d, log);
	dumpPakLevel--;
}

static void DumpPakContents (pack_t *pak, FILE *log)
{
	fprintf (log, "===== Dumping %s%s =====\n", pak->filename, is_zip_str[pak->isZip]);
	DumpPakDirectory (pak->root, log);
	fprintf (log, "***** total %d files\n\n", pak->numFiles);
}


static int FileLength (FILE *f)
{
	unsigned pos = ftell (f);
	fseek (f, 0, SEEK_END);
	unsigned end = ftell (f);
	fseek (f, pos, SEEK_SET);

	return end;
}


/*
============
FS_CreatePath

Creates any directories needed to store the given filename
============
*/
void FS_CreatePath (char *path)
{
	char	*ofs;

	//!! will modify "path"
	DEBUG_LOG(va("path: %s\n", path));
	for (ofs = path + 1; *ofs; ofs++)
	{
		if (*ofs == '/')
		{	// create the directory
			*ofs = 0;
			Sys_Mkdir (path);
			*ofs = '/';
		}
	}
}


void FS_CopyFile (char *src, char *dst)
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
	while (1)
	{
		int		len;
		byte	buffer[65536];

		len = fread (buffer, 1, sizeof(buffer), f1);	// here: rec.size=1, num_recs=buf_size
		if (!len) break;
		fwrite (buffer, len, 1, f2);
	}

	fclose (f1);
	fclose (f2);
}


void FS_CopyFiles (char *srcMask, char *dstDir)
{
	char	*found, pattern[MAX_OSPATH];
	int		pos1, pos2;

	DEBUG_LOG(va("CopyFiles(%s->%s)\n", srcMask, dstDir));

	// prepare src string
	found = strrchr (srcMask, '/');
	if (!found)
	{
		Com_WPrintf ("CopyFiles: bad srcMask \"%s\"\n", srcMask);
		return;
	}
	pos1 = found - srcMask + 1;

	// prepare dst string
	appCopyFilename (pattern, dstDir, sizeof(pattern));
	pos2 = strlen (pattern);
	if (!pos2 || pattern[pos2 - 1] != '/')
	{
		pattern[pos2] = '/';
		pos2++;
	}

	// create destination directory
	FS_CreatePath (pattern);
	// copy files
	for (found = Sys_FindFirst (srcMask, LIST_FILES); found; found = Sys_FindNext ())
	{
		strcpy (pattern + pos2, found + pos1);
		FS_CopyFile (found, pattern);
	}
	Sys_FindClose ();
}


void FS_RemoveFiles (char *mask)
{
	char	*found;

	DEBUG_LOG(va("RemoveFiles(%s)\n", mask));
	for (found = Sys_FindFirst (mask, LIST_FILES); found; found = Sys_FindNext ())
	{
		DEBUG_LOG(va("del(%s)\n", found));
		remove (found);
	}
	Sys_FindClose ();
}


/*
==============
FS_FCloseFile
==============
*/

// Stuff for caching open/close/reopen packfiles
static char cached_name[MAX_OSPATH];	// name of last opened file
static FILE *cached_handle;				// handle file cached_name
static FILE *cached_handle2;			// handle, available for opening (same as cached_handle or NULL)

static void ClearFileCache (void)
{
	if (cached_handle2)
		fclose (cached_handle2);		// close file only if it is not in use
	cached_handle = cached_handle2 = NULL; // but always clear cached info
	cached_name[0] = 0;
}


static FILE *FOpenCached (char *name)
{
	if (cached_handle2 && !stricmp(cached_name, name))
	{
		cached_handle2 = NULL; // must be the same as cached_handle
		return cached_handle;
	}
	if (cached_handle2)
		fclose (cached_handle2); // we want to open a different file - close cached
	cached_handle2 = NULL;
	strcpy (cached_name, name);
	cached_handle = fopen (name, "rb");
	return cached_handle;
}


static void FCloseCached (FILE *f)
{
	if (f == cached_handle)
		cached_handle2 = f; // just make handle available, not closing file
	else
		fclose (f); // closing not cached file
}


void FS_FCloseFile (FILE *f)
{
	FILE2*	f2;
	int		rest;

	if (!f) return;
	f2 = (FILE2*)f;

	// simple validation of FILE2 structure
	if ((char *)f2->name - (char *)f2 != sizeof(FILE2))
		Com_FatalError ("FS_FCloseFile: invalid file handle (called by %s)", appSymbolName (GET_RETADDR(f)));

	if (f2->type == FT_NORMAL || f2->type == FT_ZPAK || f2->type == FT_PAK)
	{
		if (f2->zFile)
		{
			rest = f2->zFile->rest_write;
			if (!Zip_CloseFile (f2->zFile) && !rest)
			{	// file readed completely, but bad checksum
				Com_FatalError ("FS_FCloseFile: damaged zip file %s %s", f2->name, f2->pFile->name);
			}
		}
		if (f2->file)
			FCloseCached (f2->file);		// fclose (f2->file) or ZipClose (f2->file);
	}
	else if (f2->type == FT_ZPAK)
		Zip_CloseBuf (f2->zBuf);
//	else Com_FatalError ("FS_FCloseFile: invalid handle type");

	delete f;
}


/*
===========
FS_FOpenFile

Finds the file in the search path.
returns filesize and an open FILE *
Used for streaming data out of either a pak file or
a seperate file.
===========
*/


// Allocates FILE2 structure
static FILE *AllocFileInternal (char *name, FILE *file, fileType_t type)
{
	FILE2* f2;

	f2 = new (name) FILE2;
	f2->file = file;
	f2->type = type;
	return (FILE*) f2;
}


int fileFromPak;		// used for precaching

// WARNING: FS_FOpenFile, FS_Read and FS_FCloseFile works with FILE2* (not with FILE*) structure.

int FS_FOpenFile (const char *filename2, FILE **file)
{
	char			netpath[MAX_OSPATH], *pakname, game[MAX_OSPATH], filename[MAX_OSPATH];
	pack_t			*pak;
	packFile_t		*pfile;
	int				gamelen, gamePos;
	fileLink_t		*link;
	FILE			*f;

	fileFromPak = 0;
	appCopyFilename (filename, filename2, sizeof(filename));

	/*-------------- check for links first ---------------------*/
	for (link = fs_links; link; link = link->next)
	{
		if (!memcmp (filename, link->from, link->fromlength))
		{
			appSprintf (ARRAY_ARG(netpath), "%s%s", link->to, filename + link->fromlength);
			if (f = fopen (netpath, "rb"))
			{
				*file = AllocFileInternal (netpath, f, FT_NORMAL);
				DEBUG_LOG(va("link: %s\n", netpath));
				return FileLength (f);
			}
			*file = NULL;
			return -1;
		}
	}

	/*----- search through the path, one element at a time -----*/
	gamelen = 0;
	gamePos = 0;
	if (filename[1] == ':' && filename[2] == '/')	// CD path?
	{
#ifdef CD_PATH
		gamePos = strlen (fs_cddir->string);
		if (memcmp (filename, fs_cddir->string, gamePos))
		{
			Com_DPrintf ("FS_FOpenFile: bad path %s\n", filename);
			*file = NULL;
			return -1;
		}
		gamePos++;			// skip '/'
#else
		Com_DPrintf ("FS_FOpenFile: bad path %s\n", filename);
		*file = NULL;
		return -1;
#endif
	}
	else if (filename[0] == '.' && filename[1] == '/')
		gamePos = 2;

	if (gamePos)
	{
		pakname = strchr (filename + gamePos, '/');	// find end of [CD +] game directory
		if (pakname)
		{
			pakname++;		// skip '/'
			gamelen = pakname - filename;
			appStrncpyz (game, filename, strrchr (filename, '/') - filename + 2);	// include "/" and space for zero
		}
		else
			pakname = filename;
	}
	else
		pakname = filename;

	// pakname = game-relative path, game = subtracted game path, gamelen = strlen(game).
	// We should refine .pak files with a game path (if specified, gamelen > 0)
	for (searchPath_t *search = fs_searchpaths.First(); search; search = fs_searchpaths.Next(search))
	{
		// is the element a pak file?
		if (pak = search->pack)
		{
			// validate .pak game directory
			if (gamelen && memcmp (pak->filename, game, gamelen))
				continue; // .pak placed in other game directory - skip it
			// look through all the pak file elements
			if (pfile = FindPakFile (pak, pakname)) // found
			{
//				DebugPrintf ("\nAllocPakfile(\"%s\" in \"%s\")%s\n", pakname, pak->filename, is_zip_str[pak->isZip]);
				fileFromPak = 1;
				DEBUG_LOG(va("pfile: %s (%s)\n", pakname, pak->filename));
				// open a new file on the pakfile
				*file = AllocFileInternal (pak->filename, NULL, pak->isZip ? FT_ZPAK : FT_PAK);
				(*(FILE2**)file)->pFile = pfile;
				return pfile->ucSize;
			}
		}
		else
		{
			if (!gamelen)
			{
				// check a file in the directory tree (only for game-relative path)
				appSprintf (ARRAY_ARG(netpath), "%s/%s", search->name, filename);

				if (!(f = fopen (netpath, "rb"))) continue;
				*file = AllocFileInternal (netpath, f, FT_NORMAL);
				DEBUG_LOG(va("file: %s\n", netpath));
				return FileLength (f);
			}
		}
	}

	// check a file in the directory tree for base-relative path
	if (gamelen && (f = fopen (filename, "rb")))
	{
		*file = AllocFileInternal (filename, f, FT_NORMAL);
		DEBUG_LOG(va("file: %s\n", filename));
		return FileLength (f);
	}

	// check for a resource (inline) file
	resFile_t *rf = resFiles.Find (filename);
	if (rf)
	{
		*file = AllocFileInternal (filename, NULL, FT_ZMEM);
		(*(FILE2**)file)->rFile = rf;
		DEBUG_LOG(va("rfile: %s\n", filename));
		return rf->size;
	}

	DEBUG_LOG(va("no file %s\n", filename));

	*file = NULL;
	return -1;
}


/*
=================
FS_FileExists

This is a fast version of FS_FOpenFile()
Will not check inline files (only filesystem or paks)
=================
*/

bool FS_FileExists (char *filename)
{
	char			*pakname, game[MAX_OSPATH], buf[MAX_OSPATH];
	pack_t			*pak;
	packFile_t		*pfile;
	int				gamelen, gamePos;
	fileLink_t		*link;
	FILE			*f;

	fileFromPak = 0;
	appCopyFilename (buf, filename, sizeof(buf));
	filename = buf;
	DEBUG_LOG(va("check: %s\n", filename));

	/*------------- check for links first ----------------------*/
	for (link = fs_links; link; link = link->next)
	{
		if (!memcmp (filename, link->from, link->fromlength))
		{
			if (f = fopen (va("%s%s", link->to, filename + link->fromlength), "rb"))
			{
				fclose (f);
				return true;
			}
			return false;	// have a link, but have no file
		}
	}

	/*----- search through the path, one element at a time -----*/
	gamelen = 0;
	gamePos = 0;
	if (filename[1] == ':' && filename[2] == '/')		// CD path?
	{
#ifdef CD_PATH
		gamePos = strlen (fs_cddir->string);
		if (memcmp (filename, fs_cddir->string, gamePos))
		{
			Com_DPrintf ("FS_FileExists: bad path: %s\n", filename);
			return false;
		}
		gamePos++;			// skip '/'
#else
		Com_DPrintf ("FS_FileExists: bad path: %s\n", filename);
		return false;
#endif
	}
	else if (filename[0] == '.' && filename[1] == '/')
		gamePos = 2;

	if (gamePos)
	{
		pakname = strchr (filename + gamePos, '/');	// find end of [CD +] game directory
		if (pakname)
		{
			pakname++;		// skip '/'
			gamelen = pakname - filename;
			appStrncpyz (game, filename, strrchr (filename, '/') - filename + 2);
		}
		else
			pakname = filename;
	}
	else
		pakname = filename;

//	Com_Printf(S_RED"check %s in %s; l = %d\n", pakname, game, gamelen);
	// pakname = game relative path, game = subtracted game path, gamelen = strlen(game).
	// We should refine .pak files with a game path (if specified, gamelen > 0)
	for (searchPath_t *search = fs_searchpaths.First(); search; search = fs_searchpaths.Next(search))
	{
		// is the element a pak file?
		if (pak = search->pack)
		{
			// validate .pak game directory
			if (gamelen && memcmp (pak->filename, game, gamelen))
				continue; // .pak placed in other game directory - skip it
			// look through all the pak file elements
			if (pfile = FindPakFile (pak, pakname))	// found
			{
				fileFromPak = 1;
				return true;
			}
		}
		else
		{
			if (!gamelen)
			{
				// check a file in the directory tree (only for game-relative path)
				if (!(f = fopen (va("%s/%s",search->name, filename), "rb")))
					continue;

				fclose (f);
				return true;
			}
		}
	}

	// check a file in the directory tree for base-relative path
	if (gamelen && (f = fopen (filename, "rb")))
	{
		fclose (f);
		return true;
	}

	return false;
}


/*
=================
FS_ReadFile

Properly handles partial reads
=================
*/

void FS_Read (void *buffer, int len, FILE *f)
{
	FILE2	*f2;
	zipFile_t zfs;

	guard(FS_Read);

	if (!f) return;
	f2 = (FILE2*)f;

	// simple validation of FILE2 structure (don't let to pass FILE structure, allocated without FS_FOpenFile())
	if (f2->name - (char *)f2 != sizeof(FILE2))
		Com_FatalError ("FS_Read: invalid file handle (called by %s)", appSymbolName (GET_RETADDR(buffer)));

	if (f2->type == FT_ZMEM)
	{
		if (!f2->zBuf)
		{
			int		rem;

			// open and seek ZBUF
			f2->zBuf = OpenZRes ();
			rem = f2->rFile->pos;
			while (rem)
			{
				char	tmpbuf[4096];	// buffer for seeking

				int tmp = min(rem, sizeof(tmpbuf));
				Zip_ReadBuf (f2->zBuf, tmpbuf, tmp);
				rem -= tmp;
			}
		}
		Zip_ReadBuf (f2->zBuf, buffer, len);
		return;
	}

	if (!f2->file)
	{	// file is only allocated - open it
		if (!f2->pFile)
		{	// regular file
			f2->file = fopen (f2->name, "rb");
			if (!f2->file) Com_FatalError ("cannot open file %s", f2->name);
		}
		else
		{	// pak file
			if (f2->type == FT_PAK)
			{	// id pak file
				f2->file = FOpenCached (f2->name);		// fopen (f2->name, "rb");
				if (!f2->file)
					Com_FatalError ("couldn't reopen %s", f2->name);
				if (fseek (f2->file, f2->pFile->pos, SEEK_SET))
					Com_FatalError ("Cannot seek %s", f2->name);
			}
			else
			{	// zipped pak file
				f2->file = FOpenCached (f2->name);		// ZipOpen (f2->name);
				if (!f2->file)
					Com_FatalError ("couldn't reopen zip %s", f2->name);
				zfs.csize  = f2->pFile->cSize;
				zfs.ucsize = f2->pFile->ucSize;
				zfs.pos    = f2->pFile->pos;
				zfs.method = f2->pFile->method;
				zfs.crc32  = f2->pFile->crc;
				f2->zFile = Zip_OpenFile (f2->file, &zfs);
				if (!f2->zFile)
					Com_FatalError ("cannot open file %s in zip %s", f2->pFile->name, f2->name);
			}
		}
	}

	if (f2->zFile)
	{
		if (Zip_ReadFile (f2->zFile, buffer, len) != len)
			Com_FatalError ("error reading zip file");
	}
	else
	{
		byte	*buf;

		buf = (byte *)buffer;
		int read = fread (buf, len, 1, f2->file);
		if (read != 1)
			Com_FatalError ("cannot read file");
	}

	unguard;
}

/*
============
FS_LoadFile

Filename are reletive to the quake search path
a null buffer will just return the file length without loading
When file is loaded, it contained in buffer with added trailing zero
============
*/
void* FS_LoadFile (const char *path, unsigned *len)
{
	FILE2	*h;

	// look for it in the filesystem or pack files
	unsigned len2 = FS_FOpenFile (path, (FILE **)&h);
	if (!h)
	{
		if (len) *len = 0;
		return NULL;
	}

	if (len) *len = len2;

//	MEM_ALLOCATOR(path);	//!!
	byte *buf = (byte*)appMalloc (len2 + 1); // adds 1 for trailing zero
	FS_Read (buf, len2, (FILE*)h);
	FS_FCloseFile ((FILE*)h);
	// not needed: appMalloc returns zero-filled block:	buffer[len] = 0; // trailing zero

	return buf;
}


/*
=============
FS_FreeFile
=============
*/
void FS_FreeFile (void *buffer)
{
	appFree (buffer);
}


/*
=================
FS_LoadPackFile

Takes an explicit (not game tree related) path to a pak file.

Loads the header and directory, adding the files at the beginning
of the list so they override previous pack files.
=================
*/

static pack_t *createdPak;

static bool EnumZippedPak (zipFile_t *file)
{
	int len = strlen (file->name);
	if (len > 0 && file->name[len - 1] != '/')
	{	// not a directory - add file
		packFile_t *newfile = AddPakFile (createdPak, file->name);
		newfile->method = file->method;
		newfile->pos = file->pos;
		newfile->cSize = file->csize;
		newfile->ucSize = file->ucsize;
		newfile->crc = file->crc32;
	}
	return true;
}


static pack_t *LoadPackFile (char *packfile)
{
	bool	is_zip;

	FILE *packHandle = fopen(packfile, "rb");
	if (!packHandle)
	{
		Com_WPrintf ("Cannot open packfile %s\n", packfile);
		return NULL;
	}

	dPackHeader_t	header;
	if (fread (&header, sizeof(header), 1, packHandle) != 1)
	{
		Com_WPrintf ("Cannot read packfile %s\n", packfile);
		fclose (packHandle);
		return NULL;
	}
	if (LittleLong(header.ident) != IDPAKHEADER)
	{
		fclose (packHandle);
		packHandle = Zip_OpenArchive (packfile);
		if (!packHandle)
		{
			Com_WPrintf ("%s is not a packfile\n", packfile);
			return NULL;
		}
		is_zip = true;
	}
	else
		is_zip = false;

	// create pack_t
	CMemoryChain *chain = new CMemoryChain;
	pack_t *pack = new (chain) pack_t;
	pack->filename = CopyString (packfile, chain);
	pack->root = new ("", chain) packDir_t;
	pack->isZip = is_zip;
	pack->chain = chain;

	if (!is_zip)
	{	// load standard quake pack file
		header.dirofs = LittleLong (header.dirofs);
		header.dirlen = LittleLong (header.dirlen);
		int numpackfiles = header.dirlen / sizeof(dPackFile_t);
		if (fseek (packHandle, header.dirofs, SEEK_SET))
			Com_FatalError ("Cannot seek pakfile %s", packfile);
		// parse the directory
		for (int i = 0; i < numpackfiles; i++)
		{
			dPackFile_t info;
			fread (&info, sizeof(dPackFile_t), 1, packHandle);
			packFile_t *newfile = AddPakFile (pack, info.name);
			newfile->pos = LittleLong(info.filepos);
			newfile->ucSize = LittleLong(info.filelen);
			newfile->cSize = newfile->ucSize; // unpacked file
		}
		fclose (packHandle);
	}
	else
	{	// load zip file
		createdPak = pack;
		Zip_EnumArchive (packHandle, EnumZippedPak);
		Zip_CloseArchive (packHandle);
	}

	Com_DPrintf ("Added packfile%s %s (%d files)\n", is_zip_str[pack->isZip], packfile, pack->numFiles);

	/* debug:
	packhandle = fopen("pakfiles.log", "a+");
	DumpPakContents(pack, packhandle);
	fclose(packhandle);
	*/

	return pack;
}


static void UnloadPackFile (pack_t *pak)
{
	Com_DPrintf ("Unloading pak %s (%d files)\n", pak->filename, pak->numFiles);
	delete pak->chain;
	ClearFileCache(); // we must do it somewhere ... this is a good place.
}


/*
================
FS_AddGameDirectory

Sets fs_gamedir, adds the directory to the head of the path,
then loads and adds pak1.pak pak2.pak ...
================
*/
static void AddGameDirectory (char *dir)
{
	strcpy (fs_gamedir, dir);

	/*-------- check for valid game directory -----------------*/
//??	if (Sys_FindFirst (va("%s/%s/*.*", fs_basedir->string, dir), 0, 0))  -- not works with CD
	if (Sys_FindFirst (va("%s/*", dir), LIST_FILES|LIST_DIRS))
		Sys_FindClose ();
	else
	{
		Sys_FindClose ();
		Com_WPrintf ("AddGameDirectory: path \"%s\" is not found\n", dir);
		return;
	}

	/*------- add the directory to the search path ------------*/
	searchPath_t *search = new (dir) searchPath_t;
	fs_searchpaths.InsertAfter (search, NULL);		// first

	TList<CStringItem> PakList;
	AddDirFilesToList (va("%s/*.pak", dir), &PakList, LIST_FILES);
	for (CStringItem *pakName = PakList.First(); pakName; pakName = PakList.Next(pakName))
	{
		pack_t *pak = LoadPackFile (pakName->name);
		if (!pak) continue;

		search = new ("") searchPath_t;
		fs_searchpaths.InsertAfter (search, NULL);	// first
		search->pack = pak;
	}
	PakList.Free();
}

/*
============
FS_Gamedir

Called to find where to write a file (demos, savegames, etc)
============
*/
char *FS_Gamedir (void)
{
	return (fs_gamedir[0]) ? fs_gamedir : BASEDIRNAME;
}

/*
=============
FS_LoadGameConfig
=============
*/
void FS_LoadGameConfig (void)
{
	char	dir[MAX_QPATH];
	FILE	*f;

	char *gdir = Cvar_VariableString ("gamedir");
	// game = "" => gdir = "baseq2"
	appSprintf (ARRAY_ARG(dir), "%s/%s", fs_basedir->string, *gdir ? gdir : BASEDIRNAME);

	if (f = fopen (va("%s/%s", dir, fs_configfile->string), "r"))
	{
		fclose (f);
		Cbuf_AddText (va("unbindall\nreset *\nunalias *\nexec %s\n", fs_configfile->string));
	}
	else
		Cbuf_AddText ("exec default.cfg\n");

	if (f = fopen (va("%s/autoexec.cfg", dir), "r"))
	{
		fclose (f);
		Cbuf_AddText ("exec autoexec.cfg\n");
	}
}


/*
================
FS_SetGamedir

Sets the gamedir and path to a different directory.
================
*/
bool FS_SetGamedir (const char *dir)
{
	char	path[MAX_OSPATH];

	if (strchr (dir, '/') || strchr (dir, '\\') || strchr (dir, ':'))
	{
		Com_Printf ("Gamedir should be a single filename, not a path\n");
		return false;
	}

	if (!dir[0])		// "" -> "baseq2"
		dir = BASEDIRNAME;

	// check for valid game directory
	if (Sys_FindFirst (va("%s/%s/*", fs_basedir->string, dir), LIST_FILES|LIST_DIRS))
		Sys_FindClose ();
	else
	{
		Sys_FindClose ();
		Com_WPrintf ("Invalid game directory: %s\n", dir);
		return false;
	}

	// check for game directory change
	appSprintf (ARRAY_ARG(path), "%s/%s", fs_basedir->string, dir);
	if (!strcmp (path, fs_gamedir))
		return true;		// directory is not changed (should return something another to avoid FS_Restart ??)

	// free up any current game dir info
	searchPath_t *next;
	for (searchPath_t *search = fs_searchpaths.First(); search != fs_base_searchpaths; search = next)
	{
		if (search->pack) UnloadPackFile (search->pack);
		next = fs_searchpaths.Next(search);
		fs_searchpaths.Remove (search);
		delete search;
	}

	// flush all data, so it will be forced to reload
	if (!DEDICATED && initialized)
		Cbuf_AddText ("vid_restart\nsnd_restart\n");

	// setup gamedir vars
	strcpy (fs_gamedir, path);

	Cvar_FullSet ("gamedir", dir, CVAR_SERVERINFO|CVAR_NOSET);
	if (strcmp (dir, BASEDIRNAME))
	{	// add dirs only when current game is not BASEDIRNAME
#ifdef CD_PATH
		if (fs_cddir->string[0])
			AddGameDirectory (va("%s/%s", fs_cddir->string, dir));
#endif
		AddGameDirectory (va("%s/%s", fs_basedir->string, dir));
	}

	return true;
}


static searchPath_t *FindPakStruc (const char *name)
{
	int namelen = strlen (name);
	for (searchPath_t *item = fs_searchpaths.First(); item; item = fs_searchpaths.Next(item))
		if (pack_t *pak = item->pack)
		{
			int len = strlen (pak->filename);
			if (len < namelen) continue;
			// compare last namelen chars if pak filename with a name
			if (!stricmp (name, &pak->filename[len - namelen]))
				return item;
		}
	return NULL;
}


/*
================
FS_LoadPak_f
================
*/

static bool TryLoadPak (char *pakname)
{
	FILE	*f;

	if (FindPakStruc (pakname))
	{
		Com_WPrintf ("Packfile %s is already loaded\n", pakname);
		return true;		// already loaded
	}
	if (f = fopen (pakname, "rb"))
	{
		pack_t	*pak;

		fclose (f);
		if (!(pak = LoadPackFile (pakname))) return 1;	// not loaded, but don't try more
		// include new packfile in search paths
		searchPath_t *search = new ("") searchPath_t;
		fs_searchpaths.InsertAfter (search, NULL);		// first
		search->pack = pak;
		return true;
	}
	return false;
}


static void FS_LoadPak_f (bool usage, int argc, char **argv)
{
	char pakname[MAX_OSPATH];

	if (argc != 2 || usage)
	{
		Com_Printf ("Usage: loadpak <pakname>[.pak]\n"
					"  or   loadpak <wildcard>\n");
		return;
	}
	appSprintf (ARRAY_ARG(pakname), "%s/%s", fs_gamedir, argv[1]);

	if (strchr (pakname, '*'))
	{	// name is a wildcard
		TList<CStringItem> PakList;
		AddDirFilesToList (pakname, &PakList, LIST_FILES);
		if (PakList.First())
		{
			for (CStringItem *item = PakList.First(); item; item = PakList.Next(item))
				TryLoadPak (item->name);
			PakList.Free();
			return;
		}
	}
	else
	{
		if (TryLoadPak (pakname)) return;
		strcat (pakname, ".pak");
		if (TryLoadPak (pakname)) return;
	}

	Com_WPrintf ("Packfile %s is not found\n", argv[1]);
}


/*
================
FS_UnloadPak_f
================
*/
static void FS_UnloadPak_f (bool usage, int argc, char **argv)
{
	char	pakname[MAX_OSPATH];

	if (argc != 2 || usage)
	{
		Com_Printf ("Usage: unloadpak <pakname>[.pak]\n"
					"  or   unloadpak <wildcard>\n");
		return;
	}
	if (strchr (argv[1], '*'))
	{
		// name is a wildcard
		bool found = false;
		appSprintf (ARRAY_ARG(pakname), "%s/%s", fs_gamedir, argv[1]);

		searchPath_t *next;
		for (searchPath_t *item = fs_searchpaths.First(); item; item = next)
		{
			next = fs_searchpaths.Next(item);
			pack_t *pak = item->pack;
			if (pak && appMatchWildcard (pak->filename, pakname, true))
			{
				fs_searchpaths.Remove (item);
				if (fs_base_searchpaths == item)
					fs_base_searchpaths = next;

				UnloadPackFile (pak);
				delete item;

				found = true;
			}
		}
		if (!found)
			Com_WPrintf ("Packfile %s is not loaded\n", pakname);
		return;
	}

	appSprintf (ARRAY_ARG(pakname), "/%s", argv[1]);

	searchPath_t *search;
	if (!(search = FindPakStruc (pakname)))
	{
		strcat (pakname, ".pak");
		if (!(search = FindPakStruc (pakname)))
		{
			Com_WPrintf ("Packfile %s is not loaded\n", pakname);
			return;
		}
	}
	for (searchPath_t *item = fs_searchpaths.First(); item; item = fs_searchpaths.Next(item))
		if (item == search)
		{
			fs_searchpaths.Remove (item);
			if (fs_base_searchpaths == item)
				fs_base_searchpaths = fs_searchpaths.Next(item);;
			UnloadPackFile (item->pack);
			delete item;
			return;
		}
}


/*
================
FS_Link_f
================
*/
static void FS_Link_f (bool usage, int argc, char **argv)
{
	fileLink_t	*l, **prev;

	if (argc != 3 || usage)
	{
		Com_Printf ("Usage: link <from> <to>\n");
		return;
	}

	// see if the link already exists
	prev = &fs_links;
	for (l = fs_links; l; l = l->next)
	{
		if (!strcmp (l->from, argv[1]))
		{
			appFree (l->to);
			if (!argv[2][0])	// <to> is ""
			{	// delete it
				*prev = l->next;
				appFree (l->from);
				appFree (l);
				return;
			}
			l->to = CopyString (argv[2]);
			return;
		}
		prev = &l->next;
	}

	// create a new link
	l = (fileLink_t*)appMalloc(sizeof(*l));
	l->next = fs_links;
	fs_links = l;
	l->from = CopyString (argv[1]);
	l->fromlength = strlen (l->from);
	l->to = CopyString (argv[2]);
}


/*
================
FS_ListFiles
================
*/

TList<CStringItem> FS_ListFiles (char *name, int *numfiles, int flags)
{
	char	buf[MAX_OSPATH], game[MAX_OSPATH], *pakname, *mask, path[MAX_OSPATH];
	int		gamelen;
	int		gamePos;

	appCopyFilename (buf, name, sizeof(buf));
	DEBUG_LOG(va("list: %s\n", name));
	name = buf;

	/*	Make gamePos = index of first char in game directory part of source filename (after '/').
	 *  After all calculations, gamelen = position of 1st char of game-relative path in the source filename.
	 */

	// initialize file list
	TList<CStringItem> List;

	gamelen = 0;
	gamePos = 0;
	game[0] = 0;		// "game" will be used aniway as "prefix" for ListPakDirectory()
	if (name[1] == ':' && name[2] == '/')	// allow only CD path with X:/path
	{
#ifdef CD_PATH
		gamePos = strlen (fs_cddir->string);
		if (memcmp (name, fs_cddir->string, gamePos))
		{
			Com_DPrintf ("FS_ListFiles: bad path %s\n", name);
			return List;	// empty
		}
		gamePos++;		// skip '/'
#else
		Com_DPrintf ("FS_ListFiles: bad path %s\n", name);
		return List;		// empty
#endif
	}
	else if (name[0] == '.' && name[1] == '/')
		gamePos = 2;

	if (gamePos)
	{
		pakname = strchr (name + gamePos, '/');	// find end of [CD +] game directory
		if (pakname)
		{
			pakname++;
			gamelen = pakname - name;
			appStrncpyz (game, name, strrchr (name, '/') - name + 2);
		}
		else
			pakname = name;		// shouldn't happens
	}
	else
		pakname = name;

//	Com_Printf (S_RED"list: %s in %s (l = %d); fullname = \"%s\"\n", pakname, game, gamelen, name);

	// extract wildcard
	mask = strrchr (pakname, '/');
	if (!mask)
	{
		mask = pakname;
		path[0] = 0;
	}
	else
	{	// extract path
		appStrncpyz (path, pakname, mask - pakname + 1);
		mask++; // skip '/'
	}

	/*------------- check pak files --------------------*/
	for (searchPath_t *search = fs_searchpaths.First(); search; search = fs_searchpaths.Next(search))
	{
		if (pack_t *pak = search->pack)
		{
			// validate .pak game directory
			if (gamelen && memcmp(pak->filename, game, gamelen))
			{
//				Com_Printf (S_RED"  ignore pak %s\n", pak->filename);
				continue;		// .pak placed in other game directory - skip it
			}
//			Com_Printf (S_RED"  use pak %s\n", pak->filename);
			ListPakDirectory (pak, path, mask, flags, &List, game);
		}
	}

	/*------------ check directory tree ----------------*/
	if (name[0] == '.' && name[1] == '/')		// root-relative listing
		AddDirFilesToList (name, &List, flags);
	else if (name[0] == '.' && name[1] == '.' && name[2] == '/')	// "../dir" (game-relative) -> "./dir" (root-relative) path
		AddDirFilesToList (name+1, &List, flags);
	else
	{	// game is not specified - list all searchpaths
		const char *path2 = NULL;
		while (path2 = FS_NextPath (path2))
			AddDirFilesToList (va("%s/%s", path2, name), &List, flags);
	}

	/*----------- count number of files ----------------*/
	if (numfiles)
	{
		*numfiles = 0;
		for (CStringItem *item = List.First(); item; item = List.Next(item))
			(*numfiles)++;
	}

	return List;
}


/*
===============
FS_Dir_f
===============
*/
static void FS_Dir_f (bool usage, int argc, char **argv)
{
	char	*path = NULL;
	char	findname[1024];
	char	wildcard[256];
	int		len, maxlen, col, colwidth, colcount;
	char	*name;

	if (argc > 2 || usage)
	{
		Com_Printf ("Usage: dir [<mask>]\n");
		return;
	}

	if (argc == 1)
		strcpy (wildcard, "*.*");
	else
	{
		strcpy (wildcard, argv[1]);
		if (wildcard[strlen (wildcard) - 1] == '/')
			strcat (wildcard, "*.*");
		else if (!strchr (wildcard, '*'))	// not a file mask
			strcat (wildcard, "/*.*");
	}

	while (path = FS_NextPath (path))
	{
		appSprintf (ARRAY_ARG(findname), "%s/%s", path, wildcard);
		appCopyFilename (findname, findname, sizeof(findname));	// in-place compact filename
		Com_Printf (S_GREEN"Directory of %s\n-------------------\n", findname);

		TList<CStringItem> dirnames = FS_ListFiles (findname, NULL, LIST_FILES|LIST_DIRS);
		if (dirnames.First())
		{
			maxlen = 0;
			for (CStringItem *item = dirnames.First(); item; item = dirnames.Next(item))
			{
				if (name = strrchr (item->name, '/'))
					item->name = name + 1; // cut directory prefix
				if ((len = strlen (item->name)) > maxlen) maxlen = len;
			}

#define SPACING 6
			colcount = (linewidth - 2 + SPACING) / (maxlen + SPACING);
			if (!colcount) colcount = 1;

			colwidth = maxlen + SPACING;
			for (col = 0, item = dirnames.First(); item; item = dirnames.Next(item))
			{
				Com_Printf ("%s", item->name);
				if (++col >= colcount)
				{
					col = 0;
					Com_Printf ("\n");
					continue;
				}
				len = strlen (item->name);
				for (int i = 0; i < colwidth - len; i++) Com_Printf (" ");
			}
			if (col) Com_Printf ("\n");
			dirnames.Free();
		}
		Com_Printf ("\n");
		// if wildcard starts from "../" - dir is root-based, ignore other paths
		if (wildcard[0] == '.' && wildcard[1] == '.' && wildcard[2] == '/') break;
	}
}


/*
============
FS_Path_f
============
*/
static void FS_Path_f (void)
{
	fileLink_t		*l;

	Com_Printf (S_GREEN"Current search path:\n");
	Com_Printf ("----zip-files-name------------\n");
	int n = 0;
	for (searchPath_t *s = fs_searchpaths.First(); s; s = fs_searchpaths.Next(s))
	{
		n++;
		if (s == fs_base_searchpaths)
			Com_Printf (S_GREEN"------------------------\n");
		if (s->pack)
			Com_Printf ("%-3d  %c  %-5d %s\n", n, s->pack->isZip ? '+' : ' ', s->pack->numFiles, s->pack->filename);
		else
			Com_Printf ("%-3d     --    %s\n", n, s->name);
	}

	if (fs_links)
	{
		Com_Printf (S_GREEN"\nLinks:\n");
		for (l = fs_links; l; l = l->next)
			Com_Printf ("%s : %s\n", l->from, l->to);
	}
}


static void FS_Cat_f (bool usage, int argc, char **argv)
{
	FILE	*f;
	int		len;
	char	buf[64];

	if (argc != 2 || usage)
	{
		Com_Printf ("Usage: cat <filename>\n");
		return;
	}
	len = FS_FOpenFile (argv[1], &f);
	if (!f)
	{
		Com_Printf ("File %s is not found\n", argv[1]);
		return;
	}
	Com_Printf ("\n--------\n");
	while (len)
	{
		int		i;
		char	*p;

		int get = min(len, sizeof(buf));
		FS_Read (buf, get, f);

		for (i = 0, p = buf; i < get && *p; i++, p++)
			if (*p != '\r') Com_Printf ("%c", *p);

		if (i < get)
		{
			Com_Printf ("\nbinary file ... stopped.\n");
			break;
		}
		len -= get;
	}
	FS_FCloseFile (f);
	Com_Printf ("\n--------\n");
}


static void FS_Type_f (bool usage, int argc, char **argv)
{
	FILE	*f;
	int		len, oldDev;
	char	*name;

	if (argc != 2 || usage)
	{
		Com_Printf ("Usage: type <filename>\n");
		return;
	}
	name = argv[1];
	oldDev = fs_debug->integer;
	Cvar_SetInteger ("fs_debug", 1);	//?? another way
	len = FS_FOpenFile (name, &f);
	Cvar_SetInteger ("fs_debug", oldDev);
	if (!f)
	{
		Com_Printf ("File \"%s\" is not found\n", name);
		return;
	}
	// display info about file
	Com_Printf ("length: %d bytes\n", len);
	FS_FCloseFile (f);
}


/*
================
FS_NextPath

Allows enumerating all of the directories in the search path
================
*/
char *FS_NextPath (const char *prevpath)
{
	const char *prev;

	prev = NULL;
	for (searchPath_t *s = fs_searchpaths.First(); s; s = fs_searchpaths.Next(s))
	{
		if (s->pack) continue;
		if (prevpath == prev)
			return s->name;
		prev = s->name;
	}
	return NULL;
}


/*
================
FS_InitFilesystem
================
*/
void FS_InitFilesystem (void)
{
CVAR_BEGIN(vars)
	// basedir <path>  -- allows the game to run from outside the data tree
	CVAR_FULL(&fs_basedir, "basedir", ".", CVAR_NOSET),
	CVAR_FULL(&fs_configfile, "cfgfile", CONFIGNAME, CVAR_NOSET),
	// cddir <path>	   -- logically concatenates the cddir after the basedir for
	// allows the game to run from outside the data tree
#ifdef CD_PATH
	CVAR_FULL(&fs_cddir, "cddir", "", CVAR_NOSET),
#endif
	CVAR_FULL(&fs_gamedirvar, "game", "", CVAR_LATCH|CVAR_SERVERINFO),
	CVAR_VAR(fs_debug, 0, 0)
CVAR_END

	Cvar_GetVars (ARRAY_ARG(vars));

	RegisterCommand ("path", FS_Path_f);
	RegisterCommand ("link", FS_Link_f);
	RegisterCommand ("dir", FS_Dir_f);
	RegisterCommand ("loadpak", FS_LoadPak_f);
	RegisterCommand ("unloadpak", FS_UnloadPak_f);
	RegisterCommand ("cat", FS_Cat_f);
	RegisterCommand ("type", FS_Type_f);

	InitResFiles ();

#ifdef CD_PATH
	if (fs_cddir->string[0])
		AddGameDirectory (va("%s/"BASEDIRNAME, fs_cddir->string));
#endif

	// start up with baseq2 by default
	AddGameDirectory (va("%s/"BASEDIRNAME, fs_basedir->string));

	// any set gamedirs will be freed up to here
	fs_base_searchpaths = fs_searchpaths.First();

	// check for game override
	if (fs_gamedirvar->string[0])
		FS_SetGamedir (fs_gamedirvar->string);
	initialized = true;
}
