/*-----------------------------------------------------------------------------
	id software .pak file (Quake1/2, Half-Life, Kingpin)
-----------------------------------------------------------------------------*/

class CFileContainerPak : public CFileContainerArc
{
protected:
	// file entry
	class CFileInfo : public CStringItem
	{
	public:
		unsigned pos;				// position inside archive
		unsigned size;				// size of file
	};
	// CFile
	class CFilePak : public CFile
	{
	public:
		FILE *file;					// opened file
		const CFileInfo &info;		// pointer to file information
		unsigned readPos;
		CFilePak (const CFileInfo &AInfo, FILE *AFile)
		:	info(AInfo)
		,	file(AFile)
		{}
		~CFilePak ()
		{
			if (file)
				fclose (file);
		}
		int Read (void *Buffer, int Size)
		{
			if (!file)
				return 0;
			int len = info.size - readPos;
			if (len > Size) len = Size;
			int count = fread (Buffer, 1, len, file);
			readPos += count;
			return count;
		}
		int GetSize ()
		{
			return info.size;
		}
		bool Eof ()
		{
			return readPos >= info.size;
		}
	};
	const char *GetType ()
	{
		return "pak";
	}
	// function for file opening
	CFile *OpenLocalFile (const CArcFile &Info)
	{
		const CFileInfo &info = static_cast<const CFileInfo&>(Info);
		FILE *f = fopen (name, "rb");
		if (!f)						// cannot open archive file
			return NULL;
		// rewind to file start
		fseek (f, info.pos, SEEK_SET);
		CFilePak *File = new CFilePak (info, f);
		return File;
	}
public:
	static CFileContainer *Create (const char *filename, FILE *f)
	{
		// file format
#define IDPAKHEADER		BYTES4('P','A','C','K')
		struct DPakHeader
		{
			unsigned ident;			// == IDPAKHEADER
			int		dirofs;
			int		dirlen;
		};

		struct DPakFile
		{
			char	name[56];
			unsigned filepos, filelen;
		};

		// read header
		DPakHeader hdr;
		if (fread (&hdr, sizeof(hdr), 1, f) != 1 ||
			hdr.ident != IDPAKHEADER)
			return NULL;
		// LITTLE_ENDIAN
		if (fseek (f, hdr.dirofs, SEEK_SET))
			return NULL;
		// create CFileContainerPak
		CFileContainerPak *Pak = new (filename) CFileContainerPak;
		Pak->numFiles = hdr.dirlen / sizeof(DPakFile);
		for (int i = 0; i < Pak->numFiles; i++)
		{
			DPakFile file;
			if (fread (&file, sizeof(file), 1, f) != 1)
			{
				delete Pak;
				return NULL;
			}
			TString<64> Filename; Filename.filename (file.name);	// looks cool :)
			// cut filename => path + name
			char *name = Filename.rchr ('/');
			// find/create directory
			CArcDir *Dir;
			if (name)
			{
				*name++ = 0;
				Dir = Pak->FindDir (Filename, true);
			}
			else
			{
				name = Filename;
				Dir = &Pak->Root;
			}
			// add file to dir
			CStringItem *ins;
			CFileInfo *info = static_cast<CFileInfo*>(Dir->Files.Find (name, &ins));
			if (!info)
			{
				// should always pass
				info = new (name, Pak->mem) CFileInfo;
				Dir->Files.InsertAfter (info, ins);
				info->pos  = file.filepos;
				info->size = file.filelen;
			}
		}
		return Pak;
	}
};
