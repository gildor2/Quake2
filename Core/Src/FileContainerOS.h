/*-----------------------------------------------------------------------------
	Operating system directory as CFileContainer
-----------------------------------------------------------------------------*/

// file type
class CFileOS : public CFile
{
public:
	int		size;		// used as "-1" when uninitialized
	FILE	*file;
	unsigned readPos;
	~CFileOS ()
	{
		if (file)
			fclose (file);
	}
	int Read (void *Buffer, int Size)
	{
		if (!file)
			return 0;
		int count = fread (Buffer, 1, Size, file);
		readPos += count;
		return count;
	}
	int GetSize ()
	{
		if (size != -1)
			return size;
		int pos = ftell (file); fseek (file, 0, SEEK_END);
		size = ftell (file);  fseek (file, pos, SEEK_SET);
		return size;
	}
	bool Eof ()
	{
		if (!file)
			return true;
		return feof (file) != 0;
	}
};


class CFileContainerOS : public CFileContainer
{
protected:
	const char *GetType ()
	{
		return "dir";
	}
public:
	CFileContainerOS ()
	{
		containFlags = FS_OS;
	}
	bool FileExists (const char *filename)
	{
		return appFileType (va("%s/%s", name, filename)) == FS_FILE;
	}
	CFile *OpenFile (const char *filename)
	{
#if 1
		return appOpenFile (va("%s/%s", name, filename));
#else
		FILE *f = fopen (va("%s/%s", name, filename), "rb");
		if (!f)
			return NULL;
		CFileOS *File = new CFileOS;
		File->file = f;
		File->size = -1;	// uninitialized
		return File;
#endif
	}
	void List (CFileList &list, const char *mask, unsigned flags)
	{
		appListDirectory (va("%s/%s", name, mask), list, flags);
	}
};
