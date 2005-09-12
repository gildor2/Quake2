#include "qcommon.h"
//?? use "Core.h" instead of "qcommon.h"
#include "zip.h"

//?? implement as classes (1 reason why not: CZipArchive will have only 1 field of FILE* type)

/*
 *  When ZIP_USE_CENTRAL_DIRECTORY is defined, ZipEnum, ZipOpenFile and
 *  ZipExtractFileMem will use central zip directory to get info (faster zip file scan)
 */
#define ZIP_USE_CENTRAL_DIRECTORY

#define READ_BUF_SIZE 65536		// size of buffer for reading compressed contents

typedef unsigned char   uch;	// 1-byte unsigned
typedef unsigned short  ush;	// 2-byte unsigned
typedef unsigned long   ulg;	// 4-byte unsigned


#define CENTRAL_HDR_MAGIC	BYTES4('P','K',1,2)
#define LOCAL_HDR_MAGIC		BYTES4('P','K',3,4)
#define END_HDR_MAGIC		BYTES4('P','K',5,6)


#ifdef _WIN32
#pragma pack(push,1)
#else
#error Adapt for non-Win32 platform!!!
#endif

struct localFileHdr_t
{
	uch		versionNeededToExtract[2];
	ush		generalPurposeBitFlag;
	ush		compressionMethod;
	ulg		lastModDosDatetime;
	ulg		crc32;
	ulg		csize;
	ulg		ucsize;
	ush		filenameLength;
	ush		extraFieldLength;
};

struct cdirFileHdr_t
{
	uch		versionMadeBy[2];
	uch		versionNeededToExtract[2];
	ush		generalPurposeBitFlag;
	ush		compressionMethod;
	ulg		lastModDosDatetime;
	ulg		crc32;
	ulg		csize;
	ulg		ucsize;
	ush		filenameLength;
	ush		extraFieldLength;
	ush		fileCommentLength;
	ush		diskNumberStart;
	ush		internalFileAttributes;
	ulg		externalFileAttributes;
	ulg		relativeOffsetLocalHeader;
};

#ifdef _WIN32
#pragma pack(pop)
#endif


FILE* Zip_OpenArchive (const char *name)
{
	FILE *f;
	if (!(f = fopen (name, "rb"))) return NULL;

	unsigned signature;
	localFileHdr_t hdr;
	if (fread (&signature, 4, 1, f) != 1 || signature != LOCAL_HDR_MAGIC ||		//?? disable this check to allow sfx archives etc
		fread (&hdr, sizeof(hdr), 1, f) != 1 || hdr.compressionMethod >= 9)		//?? don't need this check
	{
		fclose (f);
		return NULL;
	}
	return f;
}


void Zip_CloseArchive (FILE *F)
{
	fclose (F);
}


#ifndef ZIP_USE_CENTRAL_DIRECTORY

// Enumerate zip files
bool Zip_EnumArchive (FILE *f, enumZipFunc_t enum_func)
{
	if (!enum_func) return false;

	fseek(f, 0, SEEK_SET);									// rewind archive
	while (!feof (f))
	{
		unsigned signature;
		if (fread (&signature, 4, 1, f) != 1) return false;	// cannot read signature
		if (signature == CENTRAL_HDR_MAGIC || signature == END_HDR_MAGIC)
			return true;									// central directory structure - not interesting

		if (signature != LOCAL_HDR_MAGIC) return false;		// MUST be local file header

		localFileHdr_t hdr;
		if (fread (&hdr, sizeof(hdr), 1, f) != 1) return false;

		unsigned nameLen = hdr.filenameLength;
		char buf[512];
		if (fread (&buf[0], nameLen, 1, f) != 1) return false;
		buf[nameLen] = 0;

		// fill zipFile_t structure
		zipFile_t file;
		file.name   = buf;
		file.csize  = hdr.csize;
		file.ucsize = hdr.ucsize;
		file.crc32  = hdr.crc32;
		file.method = hdr.compressionMethod;
		file.pos    = ftell (f) + hdr.extraFieldLength;
		// seek to next file
		if (fseek (f, hdr.extraFieldLength + hdr.csize, SEEK_CUR)) return false;
		// if OK - make a callback
		if (!enum_func (&file)) return true;				// break on "false"
		//printf("%10d < %10d  [%1d]  %s\n", hdr.csize, hdr.ucsize, hdr.compressionMethod, buf);
		// dirs have size=0, method=0, last_name_char='/'
	}
	return true;
}


#else // ZIP_USE_CENTRAL_DIRECTORY


// Enumerate zip central directory (faster)
bool Zip_EnumArchive (FILE *f, enumZipFunc_t enum_func)
{
	if (!enum_func) return false;

	if (fseek (f, 0, SEEK_END)) return false;				// rewind to the archive end
	// Find end of central directory record (by signature)
	unsigned pos;
	char buf[512];
	if ((pos = ftell (f)) < 22) return false;				// file too small to be a zip-archive
	// here: 18 = sizeof(ecdir_rec_ehdr) - sizeof(signature)
	if (pos < sizeof(buf) - 18)
		pos = 0;											// small file
	else
		pos -= sizeof(buf) - 18;

	// try to fill buf 128 times (64K total - max size of central arc comment)
	bool found = false;
	for (int i = 0; i < 128; i++)
	{
		if (fseek (f, pos, SEEK_SET)) return false;
		unsigned rs = fread (buf, 1, sizeof(buf), f);		// rs = read size
		for (char *buf_ptr = buf + rs - 4; buf_ptr >= buf; buf_ptr--)
			if (*(unsigned*)buf_ptr == END_HDR_MAGIC)
			{
				pos += buf_ptr - buf + 16;					// 16 - offset to a central dir ptr in cdir_end struc
				found = true;
				break;
			}
		if (found) break;
		if (!rs || !pos) return false;						// central directory not found
		pos -= sizeof(buf) - 4;								// make a 4 bytes overlap between readings
		if (pos < 0) pos = 0;
	}
	if (!found) return false;

	// get pointer to a central directory
	fseek (f, pos, SEEK_SET);
	if (fread (&pos, 4, 1, f) != 1) return false;
	// seek to a central directory
	if (fseek (f, pos, SEEK_SET)) return false;

	while (!feof(f))
	{
		unsigned signature;
		if (fread (&signature, 4, 1, f) != 1) return false;	// cannot read signature
		if (signature == END_HDR_MAGIC)
			return true;									// end of central directory structure - finish
		if (signature != CENTRAL_HDR_MAGIC) return false;	// MUST be a central directory file header

		cdirFileHdr_t hdr;									// central directory record
		if (fread (&hdr, sizeof(hdr), 1, f) != 1) return false;

		unsigned nameLen = hdr.filenameLength;
		if (fread (&buf[0], nameLen, 1, f) != 1) return false;
		buf[nameLen] = 0;

		// fill zipFile_t structure
		zipFile_t file;
		file.name   = buf;
		file.csize  = hdr.csize;
		file.ucsize = hdr.ucsize;
		file.crc32  = hdr.crc32;
		file.method = hdr.compressionMethod;
		file.pos    = hdr.relativeOffsetLocalHeader;
		// seek to next file
		if (fseek (f, hdr.extraFieldLength + hdr.fileCommentLength, SEEK_CUR)) return false;
		// if OK - make a callback
		if (!enum_func (&file)) return false;				// break on "false"
		//printf("%10d < %10d  [%1d]  %s\n", hdr.csize, hdr.ucsize, hdr.compressionMethod, buf);
		// dirs have size=0, method=0, last_name_char='/'
	}
	return true;
}


#endif // ZIP_USE_CENTRAL_DIRECTORY


// NOTE: buf size should be at least file->ucsize
bool Zip_ExtractFileMem (FILE *f, zipFile_t *file, void *buf)
{
	if (file->method != 0 && file->method != Z_DEFLATED)
		return false;										// unknown method
	if (fseek (f, file->pos, SEEK_SET))
		return false;

#ifdef ZIP_USE_CENTRAL_DIRECTORY
	unsigned signature;
	// file->pos points to a local file header, we must skip it
	if (fread (&signature, 4, 1, f) != 1) return false;
	if (signature != LOCAL_HDR_MAGIC) return false;

	localFileHdr_t hdr;
	if (fread (&hdr, sizeof(hdr), 1, f) != 1) return false;
	if (fseek (f, hdr.filenameLength + hdr.extraFieldLength, SEEK_CUR)) return false;
#endif

	// init z_stream
	if (file->method == Z_DEFLATED)
	{
		// inflate file
		z_stream s;
		s.zalloc = NULL;
		s.zfree  = NULL;
		s.opaque = NULL;
		if (inflateInit2 (&s, -MAX_WBITS) != Z_OK)
			return false;									// not initialized
		s.avail_in  = 0;
		s.next_out  = (uch*)buf;
		s.avail_out = file->ucsize;
		unsigned rest_read = file->csize;
		while (s.avail_out > 0)
		{
			char read_buf[READ_BUF_SIZE]; // buffer for reading packed contents; any size

			if (s.avail_in == 0 && rest_read)
			{
				// fill read buffer
				unsigned read_size = min(rest_read, sizeof(read_buf));
				if (fread (read_buf, read_size, 1, f) != 1)
				{
					inflateEnd (&s);
					return false;							// cannot read zip
				}
				// update stream
				s.avail_in += read_size;
				s.next_in = (uch*)read_buf;
				rest_read -= read_size;
			}
			// decompress data; exit from inflate() by error or out of stream input buffer
			int ret = inflate (&s, Z_SYNC_FLUSH);
			if ((ret == Z_STREAM_END && (s.avail_in + rest_read)) || // end of stream, but unprocessed data
				(ret != Z_OK && ret != Z_STREAM_END))		// error code
			{
				inflateEnd (&s);
				return false;
			}
		}
		inflateEnd (&s);
		if (rest_read) return false;						// bad compressed size
	}
	else
	{
		// unstore file
		if (fread (buf, file->ucsize, 1, f) != 1)
			return false;
	}
	// check CRC (if provided)
	if (file->crc32 && file->crc32 != crc32 (0, (uch*)buf, file->ucsize))
		return false;										// bad CRC or size

	return true;
}


ZFILE *Zip_OpenFile (FILE *f, zipFile_t *file)
{
	if (file->method != 0 && file->method != Z_DEFLATED)
		return NULL;										// unknown method
	ZFILE *z = new ZFILE;

#ifdef ZIP_USE_CENTRAL_DIRECTORY
	// file->pos points to a local file header, we must skip it
	if (fseek (f, file->pos, SEEK_SET)) return NULL;
	ulg signature;
	if (fread (&signature, 4, 1, f) != 1) return NULL;
	if (signature != LOCAL_HDR_MAGIC) return NULL;
	localFileHdr_t hdr;
	if (fread (&hdr, sizeof(hdr), 1, f) != 1) return NULL;
	z->zpos = file->pos + 4 + sizeof(hdr) + hdr.filenameLength + hdr.extraFieldLength;
#else
	z->zpos = file->pos;
#endif

	if (file->method == Z_DEFLATED)
	{   // inflate file
//		z->s.zalloc = NULL;
//		z->s.zfree  = NULL;
//		z->s.opaque = NULL;
		if (inflateInit2 (&z->s, -MAX_WBITS) != Z_OK)
		{
			delete z;
			return NULL;									// not initialized
		}
//		z->s.avail_in = 0;
		z->buf = new char [READ_BUF_SIZE];
	}
//	z->crc32     = 0;
	z->file      = *file;
	z->restRead  = file->csize;
	z->restWrite = file->ucsize;
	z->arc = f;

	return z;
}


int Zip_ReadFile (ZFILE *z, void *buf, int size)
{
	if (fseek (z->arc, z->zpos, SEEK_SET)) return 0;		// cannot seek to data

	if (size > z->restWrite) size = z->restWrite;
	int writeSize;
	if (z->file.method == Z_DEFLATED)
	{
		z->s.avail_out = size;
		z->s.next_out  = (uch*)buf;
		while (z->s.avail_out > 0)
		{
			if (z->s.avail_in == 0 && z->restRead > 0)
			{
				// fill read buffer
				int readSize = min(z->restRead, READ_BUF_SIZE);
				if (fread (z->buf, readSize, 1, z->arc) != 1) return 0; // cannot read zip
				z->zpos       += readSize;
				// update stream
				z->s.avail_in += readSize;
				z->restRead   -= readSize;
				z->s.next_in = (uch*)z->buf;
			}
			// decompress data; exit from inflate() by error or
			// out of stream input buffer
			int ret = inflate (&z->s, Z_SYNC_FLUSH);
			if ((ret == Z_STREAM_END && (z->s.avail_in + z->restRead)) // end of stream, but unprocessed data
				|| (ret != Z_OK && ret != Z_STREAM_END))	// error code
			return 0;
		}
		writeSize = size;
	}
	else
	{
		// stored
		writeSize = min(z->restRead, size);
		if (fread (buf, writeSize, 1, z->arc) != 1) return 0; // cannot read
		z->restRead -= writeSize;
		z->zpos     += writeSize;
	}
	z->restWrite -= writeSize;
	// update CRC32 (if needed)
	if (z->file.crc32) z->crc32 = crc32 (z->crc32, (uch*)buf, writeSize);

	return writeSize;
}


// Check size & CRC32 and close file
bool Zip_CloseFile (ZFILE *z)
{
	if (z->file.method == Z_DEFLATED)
	{
		inflateEnd (&z->s);
		delete z->buf;
	}
	bool res = (!z->restRead && !z->restWrite && z->crc32 == z->file.crc32);
	delete z;

	return res;
}


ZBUF *Zip_OpenBuf (void *data, int size)
{
	ZBUF *z = new ZBUF;
//	z->s.zalloc = NULL;
//	z->s.zfree  = NULL;
//	z->s.opaque = NULL;
	if (inflateInit2 (&z->s, -MAX_WBITS) != Z_OK)
	{
		delete z;
		return NULL;										// not initialized
	}
	z->s.avail_in = size;
	z->s.next_in  = (uch*)data;
//	z->readed = 0;

	return z;
}


int Zip_ReadBuf (ZBUF *z, void *buf, int size)
{
	z->s.avail_out = size;
	z->s.next_out  = (uch*)buf;
	while (z->s.avail_out > 0)
	{
		// decompress data; exit from inflate() by error or
		// out of stream input buffer
		int ret = inflate (&z->s, Z_SYNC_FLUSH);
		if ((ret == Z_STREAM_END && z->s.avail_in) ||		// end of stream, but unprocessed data
			(ret != Z_OK && ret != Z_STREAM_END))			// error code
			return 0;
		//?? detect unexpected EOF ("readed" is less than "size")
		z->readed += size;
	}

	return size;
}


void Zip_CloseBuf (ZBUF *z)
{
	inflateEnd (&z->s);
	delete z;
}


/*-----------------------------------------------------------------------------
	Replacement of zutil.c functions
-----------------------------------------------------------------------------*/

extern "C" {
	void *zcalloc (int opaque, int items, int size);
	void zcfree (int opaque, void *ptr);
}

void *zcalloc (int opaque, int items, int size)
{
	return appMalloc (items * size);
}

void zcfree (int opaque, void *ptr)
{
	appFree (ptr);
}
