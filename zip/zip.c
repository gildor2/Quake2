#include "zip.h"


#define TRUE	1
#define FALSE	0


#define READ_BUF_SIZE 65536


#ifdef _WIN32
#pragma pack(push,1)
#endif

typedef struct local_file_header
{
	uch		version_needed_to_extract[2];
	ush		general_purpose_bit_flag;
	ush		compression_method;
	ulg		last_mod_dos_datetime;
	ulg		crc32;
	ulg		csize;
	ulg		ucsize;
	ush		filename_length;
	ush		extra_field_length;
} local_file_hdr;


typedef struct central_directory_file_header
{
	uch		version_made_by[2];
	uch		version_needed_to_extract[2];
	ush		general_purpose_bit_flag;
	ush		compression_method;
	ulg		last_mod_dos_datetime;
	ulg		crc32;
	ulg		csize;
	ulg		ucsize;
	ush		filename_length;
	ush		extra_field_length;
	ush		file_comment_length;
	ush		disk_number_start;
	ush		internal_file_attributes;
	ulg		external_file_attributes;
	ulg		relative_offset_local_header;
} cdir_file_hdr;


typedef struct end_central_dir_record
{
	ush		number_this_disk;
	ush		num_disk_start_cdir;
	ush		num_entries_centrl_dir_ths_disk;
	ush		total_entries_central_dir;
	ulg		size_central_directory;
	ulg		offset_start_central_directory;
	ush		zipfile_comment_length;
} ecdir_rec;


#ifdef _WIN32
#pragma pack(pop)
#endif


FILE* ZipOpen (char *name)
{
	FILE	*F;
	local_file_hdr hdr;
	int		signature;

	F = fopen (name, "rb");
	if (!F) return NULL; // file not found

	if ((fread (&signature, 4, 1, F) != 1)
		|| (signature != 0x04034B50)
		|| (fread (&hdr, sizeof(hdr), 1, F) != 1)
		|| (hdr.compression_method >= 9))
	{
		fclose (F);
		return NULL;
	}
	return F;
}


void ZipClose (FILE *F)
{
	fclose (F);
}


#ifndef ZIP_USE_CENTRAL_DIRECTORY

// Enumerate zip files
int ZipEnum (FILE *F, enum_zip_func enum_func)
{
	ulg		signature, name_len;
	local_file_hdr hdr;
	char	buf[512];
	zip_file file;

	if (enum_func == NULL) return FALSE;
	fseek(F, 0, SEEK_SET);						// rewind archive
	while (!feof(F))
	{
		if (fread (&signature, 4, 1, F) != 1) return FALSE;	// cannot read signature

		if (signature == 0x02014B50 || signature == 0x06054B50)
			return TRUE; // central directory structure - not interesting

		if (signature != 0x04034B50) return FALSE;	// MUST be local file header
		if (fread (&hdr, sizeof(hdr), 1, F) != 1) return FALSE;
		name_len = hdr.filename_length;
		if (fread (&buf[0], name_len, 1, F) != 1) return FALSE;

		buf[name_len] = 0;
		// fill zip_file structure
		file.name = buf;
		file.csize = hdr.csize;
		file.ucsize = hdr.ucsize;
		file.crc32 = hdr.crc32;
		file.method = hdr.compression_method;
		file.pos = ftell (F) + hdr.extra_field_length;
		// seek to next file
		if (fseek (F, hdr.extra_field_length + hdr.csize, SEEK_CUR)) return FALSE;
		// if OK - make a callback
		if (!enum_func (&file)) return TRUE;	// break on FALSE
		//printf("%10d < %10d  [%1d]  %s\n", hdr.csize, hdr.ucsize, hdr.compression_method, buf);
		// dirs have size=0, method=0, last_name_char='/'
	}
	return TRUE;
}


#else // ZIP_USE_CENTRAL_DIRECTORY


// Enumerate zip central directory (faster)
int ZipEnum (FILE *F, enum_zip_func enum_func)
{
	ulg		signature, name_len, pos, rs;
	cdir_file_hdr hdr;							// central directory record
	char	buf[512], *buf_ptr;
	zip_file file;
	int		i, found;

	if (enum_func == NULL) return FALSE;
	// Find end of central directory record (by signature)
	if (fseek (F, 0, SEEK_END)) return FALSE;	// rewind to the archive end
	if ((pos = ftell (F)) < 22) return FALSE;	// file too small to be a zip-archive
	// here: 18 = sizeof(ecdir_rec_ehdr) - sizeof(signature)
	if (pos < sizeof(buf) - 18)
		pos = 0;								// small file
	else
		pos -= sizeof(buf) - 18;
	// try to fill buf 128 times (64K total - max size of central arc comment)
	found = 0;
	for (i = 0; i < 128; i++)
	{
		if (fseek (F, pos, SEEK_SET)) return FALSE;
		rs = fread (buf, 1, sizeof(buf), F);	// rs = read size
		for (buf_ptr = &buf[rs - 4]; buf_ptr >= buf; buf_ptr--)
			if (*(ulg*)buf_ptr == 0x06054B50)
			{
				pos += buf_ptr - buf + 16;		// 16 - offset to a central dir ptr in cdir_end struc
				found = 1;
				break;
			}
		if (found) break;
		if (!rs || !pos) return FALSE;			// central directory not found
		pos -= sizeof(buf) - 4;					// make a 4 bytes overlap between readings
		if (pos < 0) pos = 0;
	}
	if (!found) return FALSE;

	// get pointer to a central directory
	fseek(F, pos, SEEK_SET);
	if (fread (&pos, 4, 1, F) != 1) return FALSE;
	// seek to a central directory
	if (fseek (F, pos, SEEK_SET)) return FALSE;

	while (!feof(F))
	{
		if (fread(&signature, 4, 1, F) != 1) return FALSE;	// cannot read signature
		if (signature == 0x06054B50)
			return TRUE;						// end of central directory structure - finish
		if (signature != 0x02014B50) return FALSE;			// MUST be a central directory file header
		if (fread (&hdr, sizeof(hdr), 1, F) != 1) return FALSE;
		name_len = hdr.filename_length;
		if (fread (&buf[0], name_len, 1, F) != 1) return FALSE;
		buf[name_len] = 0;
		// fille zip_file structure
		file.name = buf;
		file.csize = hdr.csize;
		file.ucsize = hdr.ucsize;
		file.crc32 = hdr.crc32;
		file.method = hdr.compression_method;
		file.pos = hdr.relative_offset_local_header;
		// seek to next file
		if (fseek (F, hdr.extra_field_length + hdr.file_comment_length, SEEK_CUR)) return FALSE;
		// if OK - make a callback
		if (!enum_func (&file)) return TRUE;	// break on FALSE
		//printf("%10d < %10d  [%1d]  %s\n", hdr.csize, hdr.ucsize, hdr.compression_method, buf);
		// dirs have size=0, method=0, last_name_char='/'
	}
	return TRUE;
}


#endif // ZIP_USE_CENTRAL_DIRECTORY


int ZipExtractFileMem (FILE *F, zip_file *file, void *buf)
{
	z_stream s;
	char	read_buf[READ_BUF_SIZE];
	int		read_size, rest_read, ret;
#ifdef ZIP_USE_CENTRAL_DIRECTORY
	ulg		signature;
	local_file_hdr hdr;
#endif

	if (file->method != 0 && file->method != Z_DEFLATED) return FALSE;	// unknown method
	if (fseek (F, file->pos, SEEK_SET)) return FALSE;	// cannot seek to data

#ifdef ZIP_USE_CENTRAL_DIRECTORY
	// file->pos points to a local file header, we must skip it
	if (fread (&signature, 4, 1, F) != 1) return FALSE;
	if (signature != 0x04034B50) return FALSE;
	if (fread (&hdr, sizeof(hdr), 1, F) != 1) return FALSE;
	if (fseek (F, hdr.filename_length + hdr.extra_field_length, SEEK_CUR)) return FALSE;
#endif

	// init z_stream
	if (file->method == Z_DEFLATED)
	{
		// inflate file
		s.zalloc = NULL;
		s.zfree = NULL;
		s.opaque = NULL;
		if (inflateInit2 (&s, -MAX_WBITS) != Z_OK) return FALSE;	// not initialized
		s.avail_in = 0;
		s.next_out = buf;
		s.avail_out = file->ucsize;
		rest_read = file->csize;
		while (s.avail_out > 0)
		{
			if (s.avail_in == 0 && rest_read > 0)
			{
				// fill read buffer
				read_size = READ_BUF_SIZE;
				if (read_size > rest_read) read_size = rest_read;
				if (fread (read_buf, read_size, 1, F) != 1)
				{
					inflateEnd (&s);
					return FALSE;				// cannot read zip
				}
				// update stream
				s.avail_in += read_size;
				s.next_in = read_buf;
				rest_read -= read_size;
			}
			// decompress data; exit from inflate() by error or
			// out of stream input buffer
			ret = inflate (&s, Z_SYNC_FLUSH);
			if ((ret == Z_STREAM_END && (s.avail_in + rest_read))	// end of stream, but unprocessed data
				|| (ret != Z_OK && ret != Z_STREAM_END))			// error code
			{
				inflateEnd (&s);
				return FALSE;
			}
		}
		inflateEnd (&s);
		if (rest_read) return FALSE;			// bad compressed size
	}
	else
	{
		// unstore file
		if (fread (buf, file->ucsize, 1, F) != 1) return FALSE;
	}
	// check CRC (if provided)
	if (file->crc32 && file->crc32 != crc32 (0, buf, file->ucsize)) return FALSE;	// bad CRC or size

	return TRUE;
}


ZFILE *ZipOpenFile (FILE *F, zip_file *file)
{
	ZFILE	*z;
#ifdef ZIP_USE_CENTRAL_DIRECTORY
	ulg		signature;
	local_file_hdr hdr;
#endif

	if (file->method != 0 && file->method != Z_DEFLATED) return NULL;	// unknown method
	z = malloc (sizeof(ZFILE));
	if (!z) return NULL;						// out of memory

#ifdef ZIP_USE_CENTRAL_DIRECTORY
	// file->pos points to a local file header, we must skip it
	if (fseek (F, file->pos, SEEK_SET)) return NULL;
	if (fread (&signature, 4, 1, F) != 1) return NULL;
	if (signature != 0x04034B50) return NULL;
	if (fread (&hdr, sizeof(hdr), 1, F) != 1) return NULL;
	z->zpos = file->pos + 4 + sizeof(hdr) + hdr.filename_length + hdr.extra_field_length;
#else
	z->zpos = file->pos;
#endif

	if (file->method == Z_DEFLATED)
	{   // inflate file
		z->s.zalloc = NULL;
		z->s.zfree = NULL;
		z->s.opaque = NULL;
		if (inflateInit2 (&z->s, -MAX_WBITS) != Z_OK)
		{
			free (z);
			return NULL;						// not initialized
		}
		z->s.avail_in = 0;
		z->buf = malloc (READ_BUF_SIZE);
		if (!z->buf)
		{
			free (z);
			return NULL;						// out of memory
		}
	}
	z->crc32 = 0;
	memcpy (&z->file, file, sizeof(zip_file));
	z->rest_read = file->csize;
	z->rest_write = file->ucsize;
	z->arc = F;

	return z;
}


int ZipReadFile (ZFILE *z, void *buf, int size)
{
	int		read_size, write_size, ret;

	if (fseek (z->arc, z->zpos, SEEK_SET)) return 0;	// cannot seek to data

	if (size > z->rest_write) size = z->rest_write;
	if (z->file.method == Z_DEFLATED)
	{
		z->s.avail_out = size;
		z->s.next_out = buf;
		while (z->s.avail_out > 0)
		{
			if (z->s.avail_in == 0 && z->rest_read > 0)
			{
				// fill read buffer
				read_size = READ_BUF_SIZE;
				if (read_size > z->rest_read) read_size = z->rest_read;
				if (fread (z->buf, read_size, 1, z->arc) != 1) return 0;	// cannot read zip
				z->zpos += read_size;
				// update stream
				z->s.avail_in += read_size;
				z->s.next_in = z->buf;
				z->rest_read -= read_size;
			}
			// decompress data; exit from inflate() by error or
			// out of stream input buffer
			ret = inflate (&z->s, Z_SYNC_FLUSH);
			if ((ret == Z_STREAM_END && (z->s.avail_in + z->rest_read))		// end of stream, but unprocessed data
				|| (ret != Z_OK && ret != Z_STREAM_END))					// error code
			return 0;
		}
		write_size = size;
	}
	else
	{
		// stored
		write_size = z->rest_read;
		if (write_size > size) write_size = size;
		if (fread(buf, write_size, 1, z->arc) != 1) return 0;				// cannot read
		z->rest_read -= write_size;
		z->zpos += write_size;
	}
	z->rest_write -= write_size;
	// update CRC32 (if needed)
	if (z->file.crc32) z->crc32 = crc32 (z->crc32, buf, write_size);

	return write_size;
}


// Check size & CRC32 and close file
int ZipCloseFile (ZFILE *z)
{
	int res;

	res = TRUE;
	if (z->file.method == Z_DEFLATED)
	{
		inflateEnd (&z->s);
		free (z->buf);
	}
	if (z->rest_read || z->rest_write) res = FALSE;		// bad sizes
	if (z->crc32 != z->file.crc32) res = FALSE;			// bad CRC
	free (z);

	return res;
}


ZBUF *ZipOpenBuf (void *data, int size)
{
	ZBUF	*z;

	z = malloc (sizeof(ZBUF));
	if (!z) return NULL;						// out of memory
	z->s.zalloc = NULL;
	z->s.zfree = NULL;
	z->s.opaque = NULL;
	if (inflateInit2 (&z->s, -MAX_WBITS) != Z_OK)
	{
		free (z);
		return NULL;							// not initialized
	}
	z->s.avail_in = size;
	z->s.next_in = data;
	z->readed = 0;

	return z;
}


int ZipReadBuf (ZBUF *z, void *buf, int size)
{
	int		ret;

	z->s.avail_out = size;
	z->s.next_out = buf;
	while (z->s.avail_out > 0)
	{
		// decompress data; exit from inflate() by error or
		// out of stream input buffer
		ret = inflate (&z->s, Z_SYNC_FLUSH);
		if ((ret == Z_STREAM_END && z->s.avail_in)		// end of stream, but unprocessed data
			|| (ret != Z_OK && ret != Z_STREAM_END))	// error code
		return 0;
		//?? detect unexpected EOF ("readed" is less than "size")
		z->readed += size;
	}

	return size;
}


void ZipCloseBuf (ZBUF *z)
{
	inflateEnd (&z->s);
	free (z);
}
