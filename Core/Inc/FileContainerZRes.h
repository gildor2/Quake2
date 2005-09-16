/*-----------------------------------------------------------------------------
	Embedded to executable compressed files
-----------------------------------------------------------------------------*/

//!! if use few resources, should export CORE_API appMountResource(const void *data) ?
//?? or CFileSystem->MountResource(const void *data)
//!! CHECK: created func as CORE_API, check call from other dynamic module

#include "../../lib/zlib/zlib.h"

class CZResource : public CFileContainerArc
{
protected:
	// container data
	const void *pData;
	unsigned dataSize;
	// helper class
	struct FZlibStream : public z_stream
	{
		FZlibStream (const void *Buf, int Size)
		{
			memset (static_cast<z_stream*>(this), 0, sizeof(z_stream));
			// create inflate object
			inflateInit2 (this, -MAX_WBITS);
			avail_in = Size;
			next_in  = (byte*) Buf;
		}
		~FZlibStream ()
		{
			inflateEnd (this);
		}
		int ReadData (void *Buf, int Size)
		{
			avail_out = Size;
			next_out  = (byte*)Buf;
			// decompress data
			// note: there is no error checking, because all data in executable
			//   and all data already in buffer - so, no loop ...
			int ret = inflate (this, Z_SYNC_FLUSH);
			if ((ret == Z_STREAM_END && avail_in) ||		// end of stream, but unprocessed data
				(ret != Z_OK && ret != Z_STREAM_END))		// error code
				return 0;
			return Size;
		}
	};
	// file entry
	class CFileInfo : public CStringItem
	{
	public:
		unsigned pos;				// position in uncompressed stream
		unsigned size;				// uncompressed size
	};
	// CFile
	class CFileRes : public CFile, public FZlibStream
	{
	public:
		unsigned FileSize;
		unsigned restRead;			// number of bytes until end of file
		CFileRes (const CFileInfo &AInfo, const void *Data, int Size)
		:	restRead (AInfo.size)
		,	FileSize (AInfo.size)
		,	FZlibStream (Data, Size)
		{
			// rewind to info.pos
			int rem = AInfo.pos;
			while (rem > 0)
			{
				byte buf[4096];
				rem -= ReadData (buf, min(rem, sizeof(buf)));
			}
		}
		int Read (void *Buffer, int Size)
		{
			Size = ReadData (Buffer, min(Size, restRead));
			restRead -= Size;
			return Size;
		}
		int GetSize ()
		{
			return FileSize;
		}
		bool Eof ()
		{
			return restRead > 0;
		}
	};

	const char *GetType ()
	{
		return "zres";
	}
	// function for file opening
	CFile *OpenFile (const CArcFile &Info)
	{
		return new CFileRes (static_cast<const CFileInfo&>(Info), pData, dataSize);
	}
public:
	CZResource ()
	{
		containFlags = FS_RES;
	}
	// create resource file container
	CORE_API static CFileContainer *Mount (const char *name, const byte *data, const char *mountPoint = NULL, CFileSystem *FS = GFileSystem)
	{
		// create container
		CZResource *Res = new (name) CZResource;
		Res->dataSize = *((unsigned*)data);
		Res->pData    = data+4;				// skip "length" field
		// read + store dir
		FZlibStream s(Res->pData, Res->dataSize);
		unsigned pos;
		s.ReadData (&pos, sizeof(int));
		unsigned stopPos = pos;
		while (s.total_out < stopPos)
		{
			// get file length
			unsigned fSize;
			s.ReadData (&fSize, sizeof(int));
			TString<256> Filename;
			char *p = *Filename;
			// get filename
			do
			{
				s.ReadData (p, 1);
			} while (*p++);
			Filename.filename (Filename);	// in-place
			// cut filename -> Path + Name
			char *name = Filename.rchr ('/');
			// find/create directory
			CArcDir *Dir;
			if (name)
			{
				*name++ = 0;
				Dir = Res->FindDir (Filename, true);
			}
			else
			{
				name = Filename;
				Dir = &Res->Root;
			}
			// add file to dir
			CStringItem *ins;
			CFileInfo *info = static_cast<CFileInfo*>(Dir->Files.Find (name, &ins));
			if (!info)
			{
				// should always pass
				info = new (name, Res->mem) CFileInfo;
				Dir->Files.InsertAfter (info, ins);
				info->pos  = pos;
				info->size = fSize;
			}
//			appPrintf("    %s -- %s (%X)\n", Dir->name, name, pos);
			// advance "pos" pointer
			pos += fSize;
		}
		// mount
		FS->Mount (*Res, mountPoint);
		return Res;
	}
};
