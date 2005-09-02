
/*-----------------------------------------------------------------------------
	Files
-----------------------------------------------------------------------------*/

//??

class CFile
{
public:
	virtual unsigned Read (unsigned size, void *buffer) = 0;
};


/*-----------------------------------------------------------------------------
	File containers
-----------------------------------------------------------------------------*/

//!! either put this to a core-local h/cpp (BETTER), or make it CORE_API (worse)
/* !! variants:
	1) export base container + base file, hide pak implementation (best ?)
	2) export all
	3) hide all
*/

class CFileList;	//?? may be, remove this forward

class CFileContainer : public CStringItem
{
public:
	unsigned	tags;		// bitfield; for en masse operations
//	virtual bool *FileExists (const char *name) = 0;
//	virtual CFile *CreateReader (const char *name)
//	virtual void ListFiles (const char *find, CFileList *List, int flags = 0) = 0;
	virtual ~CFileContainer () = 0;
};


class COsFileContainer : public CFileContainer
{
};


//?? will contain fields for both compressed and uncompressed files
class CArchiveItem : public CStringItem
{
};


//?? common for ZIP and ID-pak archives
//!! INLINE FILE SYSTEM (arc+GZip)
class CArchiveFileContainer : public CMemoryChain, public CFileContainer
{
	//!! CMemoryChain and CFileContainer->CStringItem both have "operator new" - should override it
	//??   (make special template for simplifying ??)
	/*?? operator new(size) {
		CMemoryChain::operator new(size);	// alloc self
		name = Alloc(strlen(str)+1);		// alloc string in self
		strcpy (name, str);
	}
		BUT: this will break CStringItem compatibility (different layout) !!
	 */
};


/*-----------------------------------------------------------------------------
	File lists
-----------------------------------------------------------------------------*/

// file container
#define FS_OS				0x0001
#define FS_PAK				0x0002
// items to list
#define FS_FILE				0x0004
#define FS_DIR				0x0008
// output format
#define FS_PATH_NAMES		0x0010		//!! implement
#define FS_NOEXT			0x0020		//!!

//?? all tags -> FS_BASE/!FS_BASE - use base directory only (baseq2 etc)

class CFileItem : public CStringItem
{
public:
	byte	flags;		// combination of some FS_XXX flags
};


// NOTE: 1st block is CFileList, all next blocks (if one) -- CMemoryChain
class CORE_API CFileList : public CMemoryChain, public TList<CFileItem>
{
public:
	CFileList (const char *find, int flags = FS_OS|FS_PAK|FS_FILE, unsigned tags = 0xFFFFFFFF);
};


/*-----------------------------------------------------------------------------
	Platform-specific code
-----------------------------------------------------------------------------*/

//?? move to local headers (CoreLocal.h) ?
CORE_API void osListDirectory (const char *dir, CFileList &List, int flags);
