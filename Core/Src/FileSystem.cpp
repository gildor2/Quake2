/*!! TODO:
	- implement CFileContainerZip (not included automatically!)
	- implement resource file system:
	  ? may be, change resource format: cut unnecessary gzip headers from data
		stream, add "length" field to start, do not require "end pointer", only
		"start pointer"
*/

#include "Core.h"
#include "FileContainerOS.h"

CFileSystem *GFileSystem;
TString<64> GDefMountPoint;

#define LIST_MOUNT_POINTS						// add mount points to List() output (as directories)


/*-----------------------------------------------------------------------------
	Path service functions
-----------------------------------------------------------------------------*/

static const char *SkipRootDir (const char *filename)
{
	if (filename[0] != '.') return filename;
	if (filename[1] ==  0 ) return filename+1;	// really, ""
	if (filename[1] == '/') return filename+2;	// skip "./"
	return filename;							// possibly, this is "../"
}


// return NULL, when filename not under path
const char *SubtractPath (const char *filename, const char *path)
{
	// skip "./" at filename start and in path
	filename = SkipRootDir (filename);
	const char *point = SkipRootDir (path);
	int len = strlen (point);
	if (!len)									// mounted to root
	{
		if (filename[0] != '.' || filename[1] != '.')
			return filename;
//		appPrintf ("restricted: %s\n", filename);
		return NULL;							// "../" - going higher, than mount point
	}
	if (strlen (filename) < len) return NULL;
	if (filename[len] != '/' && filename[len] != 0)
		return NULL;							// different paths ...
	if (strnicmp (point, filename, len))		// filename not for this container
		return NULL;
	filename += len;
	if (filename[0] == '/') filename++;
	return filename;
}


/*-----------------------------------------------------------------------------
	Simple file reader
-----------------------------------------------------------------------------*/

CFile *appOpenFile (const char *filename)
{
	FILE *f = fopen (filename, "rb");
	if (!f)
		return NULL;
	CFileOS *File = new CFileOS;
	File->file = f;
	File->size = -1;	// uninitialized
	return File;
}


byte CFile::ReadByte ()
{
	byte b = 0;
	Read (&b, 1);
	return b;
}

int CFile::ReadInt ()
{
#ifdef LITTLE_ENDIAN
	int b = 0;
	Read (&b, 4);
	return b;
#else
	byte b[4];
	Read (b, 4);
	return b[0] | (b[1]<<8) | (b[2]<<16) | (b[3]<<24);
#endif
}

void CFile::ReadString (char *buf, int size)
{
	buf[size] = 0;
	char *lim = buf + size - 1;		// do not overwrite last (zero) byte
	char c;
	do
	{
		c = 0;						// automatically track EOF
		Read (&c, 1);
		if (buf < lim)				// when buffer overflowed, read until string end anyway
			*buf++ = c;
	} while (c);
}


/*-----------------------------------------------------------------------------
	CFileContainer
-----------------------------------------------------------------------------*/

const char *CFileContainer::GetType ()
{
	return "unk";
}


/*-----------------------------------------------------------------------------
	CFileSystem
-----------------------------------------------------------------------------*/

void CFileSystem::Mount (CFileContainer &Container, const char *point)
{
	if (Container.Owner)
		appError ("CFileSystem::Mount(): container already mounted");
	if (!point) point = GDefMountPoint;
	mounts.InsertFirst (&Container);
	Container.Owner = this;
	Container.MountPoint.filename (point);
}

void CFileSystem::Umount (CFileContainer &Container)
{
	if (Container.Owner != this)
		appError ("CFileSystem::Umount(): container not owned");
	mounts.Remove (&Container);
	Container.Owner = NULL;
}

CFileContainer *CFileSystem::MountDirectory (const char *path, const char *point)
{
	CFileContainer *Cont = new (path) CFileContainerOS;
	Mount (*Cont, point);
	return Cont;
}


CFileContainer *CFileSystem::MountArchive (const char *filename, const char *point)
{
	guard(CFileSystem::MountArchive);
	FILE *f = fopen (filename, "rb");
	if (!f)
	{
		appWPrintf ("File \"%s\" is not found\n", filename);
		return NULL;
	}
	for (int i = 0; i < ARRAY_COUNT(ArchiveReaders) && ArchiveReaders[i]; i++)
	{
		CFileContainer *Cont;
		guard(CreateArchive);
		Cont = ArchiveReaders[i] (filename, f);
		unguard;
		if (Cont)
		{
			fclose (f);
			Mount (*Cont, point);
			return Cont;
		}
		// not this format
		fseek (f, 0, SEEK_SET);					// rewind archive
	}
	fclose (f);
	appWPrintf ("Unsupported archive format of \"%s\"\n", filename);
	return NULL;
	unguardf(("%s", filename));
}


CFileList *CFileSystem::List (const char *mask, unsigned flags)
{
	TString<256> Mask; Mask.filename (mask);
	// make separate path
	TString<256> Path; Path = Mask;
	char *s = Path.rchr ('/');
	if (s) *s = 0;
	else Path[0] = 0;							// List(root)
	// create list
	CFileList *list = new CFileList;
	// iterate containers
	for (TListIterator<CFileContainer> it = mounts; it; ++it)
	{
		if (!(it->containFlags & flags)) continue;
		// process mount point
		const char *localFilename = SubtractPath (Mask, it->MountPoint);
		if (localFilename)						// inside this container
			it->List (*list, localFilename, flags);
#ifdef LIST_MOUNT_POINTS
		// may be, add mount point to list
		if (flags & FS_DIR)
		{
			// add mount point to list as directory
			localFilename = SubtractPath (it->MountPoint, Path);
			if (localFilename && localFilename[0] != 0 &&				// not empty
				(localFilename[0] != '.' || (flags & FS_LIST_HIDDEN)))	// not hidden
			{
				TString<64> Dir; Dir = localFilename;
				s = Dir.chr ('/'); if (s) *s = 0; // cut 1st path part
				// add to list
				CFileItem *item, *place;
				if (!(item = list->Find (Dir, &place)))
				{
					item = new (Dir, list) CFileItem;
					list->InsertAfter (item, place);
				}
				item->flags |= FS_DIR|FS_MOUNT_POINT;
			}
		}
#endif
	}
	return list;
}


// static data
CreateArchive_t CFileSystem::ArchiveReaders[MAX_ARCHIVE_FORMATS];

// static function
void CFileSystem::RegisterFormat (CreateArchive_t reader)
{
	for (int i = 0; i < ARRAY_COUNT(ArchiveReaders); i++)
	{
		if (ArchiveReaders[i] == reader) return;	// already registered
		if (!ArchiveReaders[i])
		{
			// register reader
			ArchiveReaders[i] = reader;
			return;
		}
	}
	appWPrintf ("Cannot register archive format: \"%s\"\n", appSymbolName ((address_t)reader));
}


bool CFileSystem::FileExists (const char *filename)
{
	TString<256> Name;
	Name.filename (filename);
	// iterate containers
	for (TListIterator<CFileContainer> it = mounts; it; ++it)
	{
		const char *localFilename = SubtractPath (Name, it->MountPoint);
		if (!localFilename) continue;			// not in this container
		if (it->FileExists (localFilename)) return true;
	}
	return false;
}

CFile *CFileSystem::OpenFile (const char *filename)
{
	TString<256> Name;
	Name.filename (filename);
	// iterate containers
	for (TListIterator<CFileContainer> it = mounts; it; ++it)
	{
		const char *localFilename = SubtractPath (Name, it->MountPoint);
		if (!localFilename) continue;			// not in this container
		CFile *File = it->OpenFile (localFilename);
		if (!File) continue;					// search in
		// copy filename (as local file system name)
		File->Owner = *it;
		File->Name  = localFilename;
		return File;
	}
	return NULL;
}

void *CFileSystem::LoadFile (const char *filename, int *size)
{
	guard(CFileSystem::LoadFile);
	CFile *f = OpenFile (filename);
	if (!f)
	{
		// file is not found
		if (size) *size = 0;
		return NULL;
	}
	int len = f->GetSize ();
	char *buf = new char[len + 1];				// 1 byte for null-terminated text files
	if (f->Read (buf, len) != len)
		appError ("Cannot read file %s:%s", f->Owner->name, *f->Name);
	delete f;

	return buf;
	unguard;
}


/*-----------------------------------------------------------------------------
	Service console functions
-----------------------------------------------------------------------------*/

static void cMount (bool usage, int argc, char **argv)
{
	if (usage || argc > 3)
	{
		appPrintf ("Usage: mount\n"
				   "  or   mount <path or archive> [mount point]\n");
		return;
	}
	if (argc <= 1)
	{
		// list mounts
		int n = 0;
		appPrintf ("----L-type-nfiles-name----------------------------mount point----\n");
		for (TListIterator<CFileContainer> it = GFileSystem->mounts; it; ++it)
		{
			//?? may be, skip hidden mounts (name[0] == '.')
			n++;
			appPrintf ("%-3d %c %-4s %-6d %-30s  %s\n", n,
				it->locked ? '+' : ' ', it->GetType(), it->numFiles, it->name, *it->MountPoint);
		}
		return;
	}
	// add mount
	//?? TODO:
	//?? - check "already mounted"
	//?? - allow q2's "mount pak0[.pak]" instead of "mount baseq2/pak0.pak"
	TString<256> Path; Path.filename (argv[1]);
	const char *point = argc > 2 ? argv[2] : NULL;
	// sanity check
	if (Path[0] == 0)							// used <mount ""> -- argv[1] == ""
	{
		appWPrintf ("mount: no empty names\n");
		return;
	}
	// if Path is not wildcard - mount it implicitly
	if (!appIsWildcard (Path))
	{
		// check for file
		FILE *f = fopen (Path, "rb");
		if (f)
		{
			GFileSystem->MountArchive (Path, point);
			fclose (f);
			return;
		}
		// check for dir: sometimes impossible, so - mount directory, even if it is not exists
		GFileSystem->MountDirectory (Path, point);
		return;
	}

	CFileList *list = new CFileList;
	appListDirectoryOS (Path, *list, FS_FILE|FS_DIR|FS_PATH_NAMES);
	if (!list->First())
	{
		appWPrintf ("mount: nothing in \"%s\"\n", *Path);
		delete list;
		return;
	}
	// prepare stats
	int numFiles = 0, numMounts = 0;
	// cut path from path/maskOrFilename
	char *s = Path.rchr ('/');
	if (s) *s = 0;
	else Path = ".";
	// process list
	for (TListIterator<CFileItem> it = *list; it; ++it)
	{
		numMounts++;
		if (it->flags & FS_FILE)
		{
			CFileContainer *Cont = GFileSystem->MountArchive (va("%s/%s", *Path, it->name), point);
			if (Cont) numFiles += Cont->numFiles;
			continue;
		}
		if (it->flags & FS_DIR)
		{
			// NOTE: cannot mount drive root (e.g. "C:" - there is no dir, when C: in placed ...)
			GFileSystem->MountDirectory (va("%s/%s", *Path, it->name), point);
			continue;
		}
		// else -- should not happen
	}
	// free file list
	delete list;
	// show stats
	appPrintf ("Mounted %d containers (%d archived files)\n", numMounts, numFiles);
}

static void cUmount (bool usage, int argc, char **argv)
{
	if (usage || argc != 2)
	{
		appPrintf ("Usage: umount <mask>\n");
		return;
	}
	// unmount file container
	const char *mask = SkipRootDir (argv[1]);	// ignore "./" at start
	int n = 0;
	for (TListIterator<CFileContainer> it = GFileSystem->mounts; it; ++it)
		if (appMatchWildcard (SkipRootDir (it->name), mask) && !it->locked)
		{
			GFileSystem->Umount (**it);
			delete *it;
			n++;
		}
	if (!n)
		appPrintf ("No mount points match \"%s\"\n", argv[1]);
	else
		appPrintf ("Unmounted %d containers\n", n);

	//!! should (may) accept following forms:
	//!! ? umount pak0.pak -- name w/o path --> umount defMountPoint/pak0.pak
	//!! * umount path/pak0.pak -- exact item
	//!! * umount path/mask -- all matched items (CHECK this form !)
}

static void cCat (bool usage, int argc, char **argv)
{
	if (usage || argc != 2)
	{
		appPrintf ("Usage: cat <filename>\n");
		return;
	}
	CFile *f = GFileSystem->OpenFile (argv[1]);
	if (!f)
	{
		appPrintf ("File \"%s\" is not found\n", argv[1]);
		return;
	}
	appPrintf ("-------------------------------------------\n");
	while (true)
	{
		char buf[256];
		int get = f->Read (ARRAY_ARG(buf));
		if (!get) break;

		char *p, *lim = buf + get;
		for (p = buf; *p && p < lim; p++)
			if (*p != '\r') appPrintf ("%c", *p);

		if (p < lim)
		{
			appPrintf ("\nbinary file ... stopped.\n");
			break;
		}
	}
	delete f;
	appPrintf ("\n");
	appPrintf ("-------------------------------------------\n");
}

static void cLs (bool usage, int argc, char **argv)
{
	if (usage || argc > 2)
	{
		appPrintf ("Usage: ls [path/][mask]\n");
		return;
	}
	//!! reformat, colorize
	TString<256> Mask;
	if (argc > 1)
	{
		Mask = argv[1];
		if (!Mask.chr ('*')) Mask += "/*";
	}
	else
		Mask = "*";
	CFileList *list = GFileSystem->List (Mask);
	for (TListIterator<CFileItem> item = *list; item; ++item)
		appPrintf ("[%c%c] %s\n", item->flags & FS_FILE ? 'f' : '-', item->flags & FS_DIR ? 'd' : '-', item->name);
	delete list;
}

static void cType (bool usage, int argc, char **argv)
{
	if (usage || argc != 2)
	{
		appPrintf ("Usage: type <filename>\n");
		return;
	}
	CFile *f = GFileSystem->OpenFile (argv[1]);
	if (!f)
	{
		appPrintf ("File \"%s\" is not found\n", argv[1]);
		return;
	}
	// display info about file
	appPrintf ("File: %s\nFound in %s\nlength: %d bytes\n", *f->Name, f->Owner->name, f->GetSize ());
	delete f;
}


void appInitFileSystem (CFileSystem &FS)
{
	GFileSystem = &FS;
	GDefMountPoint = ".";
	// console commands
	RegisterCommand ("mount", cMount);
	RegisterCommand ("umount", cUmount);
	RegisterCommand ("cat", cCat);
	RegisterCommand ("ls", cLs);
	RegisterCommand ("type", cType);
}
