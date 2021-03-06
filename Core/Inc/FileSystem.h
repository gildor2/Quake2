/*=============================================================================

	File system

	- Most file operation should be done via CFileSystem object.
	- Access to files restricted by mount points. For example, if CFileSystem
	  have only 1 mount point at "a/b/c", CFileSystem functions will not get
	  access to "a/b/*" or "a/*"
	- Supported Unix-like "hidden" files (name should start with ".")
	- File names are case-insensitive (on Unix systems should be in lower case).

=============================================================================*/


/*-----------------------------------------------------------------------------
	File lists
-----------------------------------------------------------------------------*/

// file container type
#define FS_OS				0x000001
#define FS_PAK				0x000002
#define FS_RES				0x000004
// items to list
#define FS_FILE				0x000100
#define FS_DIR				0x000200
#define FS_MOUNT_POINT		0x000400
// List() format
//??#define FS_PATH_NAMES		0x001000	-- unimplemented -- impossible ?
//??  if implement: make additional field to CFileItem: "char *fullName", and place
//??  string here
#define FS_NOEXT			0x002000
#define FS_LIST_HIDDEN		0x004000
//?? FS_NUMERIC_SORT -- sort List() as numbers (make "file100" > "file11")

#define FS_ALL				(FS_OS|FS_PAK|FS_RES|FS_DIR|FS_FILE)


class CFileItem : public CStringItem
{
public:
	unsigned flags;					// combination of some FS_XXX flags
};


// NOTE: 1st block is CFileList (based on CMemoryChain), all next blocks (if one) -- pure CMemoryChain
class CORE_API CFileList : public CMemoryChain, public TList<CFileItem>
{};

// forwards
class CFileContainer;
class CFileSystem;


/*-----------------------------------------------------------------------------
	Files
-----------------------------------------------------------------------------*/

class CORE_API CFile
{
	//?? can add to CFile/~CFile registering opened files, list with "fsinfo" command
protected:
	CFile()							// no default constructor
	{}
public:
	virtual ~CFile()				// close all opened objects; virtual destructor
	{}
	TString<64>	Name;				// local name inside owner container
	CFileContainer *Owner;
	virtual int Read(void *Buffer, int Size) = 0;
#if LITTLE_ENDIAN
	inline int ByteOrderRead(void *Buffer, int Size)
	{
		return Read(Buffer, Size);
	}
#else
	int ByteOrderRead(void *Buffer, int Size);
#endif
	virtual int GetSize() = 0;
	virtual bool Eof() = 0;
	// read helpers
	byte ReadByte();
	short ReadShort();
	int ReadInt();
	float ReadFloat();
	void ReadString(char *buf, int size);
	template<int N> inline void ReadString(TString<N> &Str)
	{
		ReadString(Str, N);
	}
};


/*-----------------------------------------------------------------------------
	File containers (mounted objects)
-----------------------------------------------------------------------------*/

// all file conainer functions called with normalized filenames (appCopyFilename())
class CFileContainer : public CStringItem
{
	friend class CFileSystem;
	friend void cMount(bool,int,char**);
	friend void cUmount(bool,int,char**);
protected:
	CFileSystem *Owner;
	TString<64> MountPoint;
	unsigned	containFlags;		// FS_OS, FS_PAK ... (used by CFileSystem::List())
	unsigned	numFiles;			// for info
	virtual const char *GetType();
public:
	bool		locked;				// cannot "umount" from console command; but can use FS->Umount()
	virtual ~CFileContainer()		// required for correct destroying
	{}
	virtual bool FileExists(const char *filename) = 0;
	virtual CFile *OpenFile(const char *filename) = 0;
	virtual void List(CFileList &list, const char *mask, unsigned flags = FS_ALL) = 0;
};


class CORE_API CFileContainerArc : public CFileContainer
{
protected:
	// file entry
	// FFileInfo will be defined in derived classes of CFileContainerArc
	// and it should be derived from CStringItem
	typedef CStringItem FFileInfo;
	// directory entry
	class FDirInfo : public CStringItem
	{
	public:
		TList<FDirInfo>	 Dirs;
		TList<FFileInfo> Files;
	};

	CMemoryChain *mem;				// will hold whole directory data
	FDirInfo Root;					// root archive directory
	// directory manipulations
	FDirInfo  *FindDir(const char *path, bool create = false);
	FFileInfo *FindFile(const char *filename);
	// Opening file by info from internal directory structure.
	virtual CFile *OpenLocalFile(const FFileInfo &Info) = 0;
	// Derived classes should:
	//	1. define own CFile
	//	2. define own FFileInfo
	//	3. overload OpenLocalFile()
	//	4. declare "static Create(const char *filename, FILE *f)" function
public:
	inline CFileContainerArc()
	{
		containFlags = FS_PAK;
		mem = new CMemoryChain;
	}
	~CFileContainerArc()
	{
		delete mem;
	}
	bool FileExists(const char *filename);
	CFile *OpenFile(const char *filename);
	void List(CFileList &list, const char *mask, unsigned flags);
};


/*-----------------------------------------------------------------------------
	File system
-----------------------------------------------------------------------------*/

#define MAX_ARCHIVE_FORMATS		16
typedef CFileContainer* (*CreateArchive_t) (const char *name, FILE *f);


//?? derive from CFileContainer; this will allow to mount one fs to another
class CORE_API CFileSystem
{
	friend void cMount(bool,int,char**);
	friend void cUmount(bool,int,char**);
private:
	TList<CFileContainer> mounts;
	static CreateArchive_t ArchiveReaders[MAX_ARCHIVE_FORMATS];
public:
	virtual ~CFileSystem()			// shut up gcc4 warning; virtual destructor
	{}
	unsigned modifyCount;			// incremented every time something mounted/umounted; rename??
	// registering archive format
	static void RegisterFormat(CreateArchive_t reader);
	// file operations
	virtual bool FileExists(const char *filename, unsigned flags = FS_ALL);
	virtual CFile *OpenFile(const char *filename, unsigned flags = FS_ALL);
	virtual CFileList *List(const char *mask, unsigned flags = FS_ALL, CFileList *list = NULL);
	// mount/unmount containers
	// if point == NULL, will mount to GDefMountPoint
	void Mount(CFileContainer &Container, const char *point = NULL);
	void Umount(CFileContainer &Container);
	CFileContainer *MountDirectory(const char *path, const char *point = NULL);
	CFileContainer *MountArchive(const char *filename, const char *point = NULL);
	void Mount(const char *mask, const char *point = NULL);
	void Umount(const char *mask);
	bool IsFileMounted(const char *filename);
	// file loading; can be free'ed with "delete" or "appFree()"
	void *LoadFile(const char *filename, unsigned *size = NULL);
};


CORE_API extern CFileSystem *GFileSystem;
CORE_API extern TString<64> GDefMountPoint;

CORE_API void appInitFileSystem(CFileSystem &FS);


//?? separate: FileSystem -- object-based; these functions works independently from CFileSystem;
//?? should separate from cpp too
// simple file reader using OS file system
CORE_API CFile *appOpenFile(const char *filename);
// This function can be used without CFileSystem objects at all; access to files is
// not restricted by file system mounts (function works without it)
CORE_API void appListDirectory(const char *dir, CFileList &List, unsigned flags);
// Create directory; supports multiple nested dirs creation
CORE_API void appMakeDirectory(const char *dirname);
// Verify file type: return FS_FILE|FS_DIR; if file does not exists (or special Unix file) - return 0
CORE_API unsigned appFileType(const char *filename);

// Create directory, which can hold filename
//?? may be, integrate this function into fopen(name,"w") analog (appOpenFileWrite() etc)
CORE_API void appMakeDirectoryForFile(const char *filename);
