#ifndef __ZIP_INCLUDED__
#define __ZIP_INCLUDED__

#include "../lib/zlib/zlib.h"


// Used by unzip functions
typedef struct
{
	unsigned csize;		// compressed size
	unsigned ucsize;	// uncompressed size
	unsigned pos;		// position in ZIP archive
	unsigned crc32;		// file CRC32
	byte	method;		// compression method (0-store, 8-deflate)
	char	*name;		// filename (used in Zip_EnumArchive() only)
} zipFile_t;

// Used by ZipOpenFile, ZipReadFile, ZipCloseFile
typedef struct
{
	char	*buf;		// buffer for reading
	z_stream s;			// zlib stream
	zipFile_t file;		// other info about a file
	unsigned zpos;		// current position in ZIP
	unsigned rest_read;	// size of rest data for reading (compressed)
	unsigned rest_write; // size of fest data for writting (uncompressed)
	unsigned crc32;		// current CRC
	FILE	*arc;		// archive handle
} ZFILE;

typedef struct
{
	z_stream s;
	unsigned readed;
} ZBUF;


typedef bool (*enumZipFunc_t) (zipFile_t *file);

// Opens archive. Return NULL if file not found or bad archive.
// Remark: we can use simple fopen(name) for this operation, but
// this will skip some structure checks.
FILE*	Zip_OpenArchive (char *name);
void	Zip_CloseArchive (FILE *F);
bool	Zip_EnumArchive (FILE *F, enumZipFunc_t enum_func);

// Extract file to memory from archive F
// Fields in zip_file, that must be set: ucsize, csize, method, pos
// Size of buf must be at least file->ucsize
bool	Zip_ExtractFileMem (FILE *F, zipFile_t *file, void *buf);

ZFILE	*Zip_OpenFile (FILE *F, zipFile_t *file);
int		Zip_ReadFile (ZFILE *z, void *buf, int size);
bool	Zip_CloseFile (ZFILE *z);

ZBUF	*Zip_OpenBuf (void *data, int size);
int		Zip_ReadBuf (ZBUF *z, void *buf, int size);
void	Zip_CloseBuf (ZBUF *z);


#endif
