#ifndef __ZIP__
#define __ZIP__

#include <stdio.h>
#include "../qcommon/qcommon.h"
#include "zlib.h"


/*
 *  When ZIP_USE_CENTRAL_DIRECTORY undefined, ZipEnum, ZipOpenFile and
 *  ZipExtractFileMem will use central zip directory to get info
 */
#define ZIP_USE_CENTRAL_DIRECTORY

typedef unsigned char   uch;
typedef unsigned short  ush;
typedef unsigned long   ulg;

// Used by unzip functions
typedef struct
{
	ulg		csize;		// compressed size
	ulg		ucsize;		// uncompressed size
	ulg		pos;		// position in ZIP archive
	ulg		crc32;		// file CRC32
	ush		method;		// compression method (0-store, 8-deflate)
	char	*name;		// filename
} zip_file;

// Used by ZipOpenFile, ZipReadFile, ZipCloseFile
typedef struct
{
	char	*buf;		// buffer for reading
	z_stream s;			// zlib stream
	zip_file file;		// other info about a file
	int		zpos;		// current position in ZIP
	int		rest_read;	// size of rest data for reading (compressed)
	int		rest_write;	// size of fest data for writting (uncompressed)
	ulg		crc32;		// current CRC
	FILE	*arc;		// archive handle
} ZFILE;

typedef struct
{
	z_stream s;
	int		readed;
} ZBUF;


typedef int (*enum_zip_func) (zip_file *file);

// Opens archive. Return NULL if file not found or bad archive.
// Remark: we can use simple fopen(name) for this operation, but
// this will skip some structure checks.
FILE* ZipOpen (char *name);
void ZipClose (FILE *F);
int ZipEnum (FILE *F, enum_zip_func enum_func);

// Extract file to memory from archive F
// Fields in zip_file, that must be set: ucsize, csize, method, pos
// Size of buf must be at least file->ucsize
int ZipExtractFileMem (FILE *F, zip_file *file, void *buf);

ZFILE *ZipOpenFile (FILE *F, zip_file *file);
int ZipReadFile (ZFILE *z, void *buf, int size);
int ZipCloseFile (ZFILE *z);

ZBUF *ZipOpenBuf (void *data, int size);
int ZipReadBuf (ZBUF *z, void *buf, int size);
void ZipCloseBuf (ZBUF *z);


#endif // __ZIP__
