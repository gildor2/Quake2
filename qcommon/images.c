#include "qcommon.h"
#include "../jpeg/jpeglib.h"


#define MAX_IMG_SIZE	2048

/*-------------------------- PCX loading --------------------------*/


typedef struct
{
	char	manufacturer;
	char	version;
	char	encoding;
	char	bits_per_pixel;
	unsigned short	xmin,ymin,xmax,ymax;
	unsigned short	hres,vres;
	unsigned char	palette[48];
	char	reserved;
	char	color_planes;
	unsigned short	bytes_per_line;
	unsigned short	palette_type;
	char	filler[58];
	unsigned char	data;			// unbounded
} pcx_t;


void LoadPCX (char *filename, byte **pic, byte **palette, int *width, int *height)
{
	byte	*raw;
	pcx_t	*pcx;
	int		x, y;
	int		len;
	int		dataByte, runLength;
	byte	*out, *pix;

	*pic = NULL;
	*palette = NULL;

	// load the file
	len = FS_LoadFile (filename, (void **)&raw);
	if (!raw)
	{
//		Com_DPrintf ("Bad pcx file %s\n", filename);
		return;
	}

	// parse the PCX file
	pcx = (pcx_t *)raw;
	raw = &pcx->data;

	pcx->xmin = LittleShort(pcx->xmin);
	pcx->ymin = LittleShort(pcx->ymin);
	pcx->xmax = LittleShort(pcx->xmax);
	pcx->ymax = LittleShort(pcx->ymax);
	pcx->hres = LittleShort(pcx->hres);
	pcx->vres = LittleShort(pcx->vres);
	pcx->bytes_per_line = LittleShort(pcx->bytes_per_line);
	pcx->palette_type = LittleShort(pcx->palette_type);

	if (pcx->manufacturer != 10 ||
		pcx->version != 5		||
		pcx->encoding != 1		||
		pcx->bits_per_pixel != 8 ||
		pcx->xmax > MAX_IMG_SIZE ||
		pcx->ymax > MAX_IMG_SIZE)
	{
		Com_Printf ("LoadPCX(%s): bad pcx file\n", filename);
		return;
	}

	if (palette)
	{
		*palette = Z_Malloc (768);
		memcpy (*palette, (byte *)pcx + len - 768, 768);
	}

	if (width)
		*width = pcx->xmax + 1;
	if (height)
		*height = pcx->ymax + 1;

	out = Z_Malloc ((pcx->ymax + 1) * (pcx->xmax + 1));

	*pic = out;

	pix = out;

	for (y = 0; y <= pcx->ymax; y++, pix += pcx->xmax + 1)
	{
		for (x = 0; x <= pcx->xmax; )
		{
			dataByte = *raw++;

			if((dataByte & 0xC0) == 0xC0)
			{
				runLength = dataByte & 0x3F;
				dataByte = *raw++;
			}
			else
				runLength = 1;

			// Truncate runLength so we don't overrun the end of the buffer (bad PCX ?)
			if ((y == pcx->ymax) && (x + runLength > pcx->xmax + 1))
				runLength = pcx->xmax - x + 1;

			while (runLength-- > 0)
				pix[x++] = dataByte;
		}
	}

	if ( raw - (byte *)pcx > len)
	{
		Com_DPrintf ("LoadPCX(%s): file was malformed", filename);
		Z_Free (*pic);
		*pic = NULL;
	}

	FS_FreeFile (pcx);
}


/*-------------------------- TGA loading --------------------------*/

#ifdef _WIN32
#pragma pack(push,1)
#endif

typedef struct
{
	unsigned char 	id_length, colormap_type, image_type;
	unsigned short	colormap_index, colormap_length;
	unsigned char	colormap_size;
	unsigned short	x_origin, y_origin, width, height;
	unsigned char	pixel_size, attributes;
} tgaHdr_t;

#ifdef _WIN32
#pragma pack(pop)
#endif


//!! add top-to-bottom support
void LoadTGA (char *name, byte **pic, int *width, int *height)
{
	int		columns, rows, numPixels;
	byte	*pixbuf;
	int		row, column;
	byte	*buf_p;
	tgaHdr_t header;
	byte	*rgba;
	byte	*buffer;
	int		length;

	*pic = NULL;

	// load the file
	length = FS_LoadFile (name, (void **)&buffer);
	if (!buffer)
	{
//		Com_DPrintf ("Bad tga file %s\n", name);
		return;
	}

	buf_p = buffer;

	header.id_length = *buf_p++;
	header.colormap_type = *buf_p++;
	header.image_type = *buf_p++;

	header.colormap_index = LittleShort (*((short *)buf_p));
	buf_p += 2;
	header.colormap_length = LittleShort (*((short *)buf_p));
	buf_p += 2;
	header.colormap_size = *buf_p++;
	header.x_origin = LittleShort (*((short *)buf_p));
	buf_p += 2;
	header.y_origin = LittleShort (*((short *)buf_p));
	buf_p += 2;
	header.width = LittleShort (*((short *)buf_p));
	buf_p += 2;
	header.height = LittleShort (*((short *)buf_p));
	buf_p += 2;
	header.pixel_size = *buf_p++;
	header.attributes = *buf_p++;

	if (header.image_type != 2	&&
		header.image_type != 10	&&
		header.image_type != 3)
		Com_Error (ERR_DROP, "LoadTGA(%s): Only type 2 (RGB), 3 (grey) and 10 (RGB RLE) targa RGB images supported\n", name);

	if (header.colormap_type != 0)
		Com_Error (ERR_DROP, "LoadTGA(%s): colormaps not supported\n", name);

	if ((header.pixel_size != 32 && header.pixel_size != 24) && header.image_type != 3)
		Com_Error (ERR_DROP, "LoadTGA(%s): Only 32 or 24 bit images supported\n", name);

	columns = header.width;
	rows = header.height;
	numPixels = columns * rows;

	if (width)
		*width = columns;
	if (height)
		*height = rows;

	rgba = Z_Malloc (numPixels * 4);
	*pic = rgba;

	if (header.id_length != 0)
		buf_p += header.id_length;		// skip image comment

	if (header.image_type == 2 || header.image_type == 3)
	{	// Uncompressed RGB or grey scale image
		for (row = rows - 1; row >= 0; row--)
		{
			pixbuf = rgba + row * columns * 4;
			for (column = 0; column < columns; column++)
			{
				unsigned char r, g, b, a;

				switch (header.pixel_size)
				{
				case 8:
					b = g = r = *buf_p++;
					a = 255;
					break;
				case 24:
					b = *buf_p++;
					g = *buf_p++;
					r = *buf_p++;
					a = 255;
					break;
				case 32:
					b = *buf_p++;
					g = *buf_p++;
					r = *buf_p++;
					a = *buf_p++;
					break;
				default:
					Com_Error (ERR_DROP, "LoadTGA(%s): illegal pixel size %d\n", name, header.pixel_size);
				}
				*pixbuf++ = r;
				*pixbuf++ = g;
				*pixbuf++ = b;
				*pixbuf++ = a;
			}
		}
	}
	else if (header.image_type == 10)
	{	// RLE RGB images
		unsigned char r, g, b, a, packetHeader, packetSize, j;

		for (row = rows - 1; row >= 0; row--)
		{
			pixbuf = rgba + row * columns * 4;
			for (column = 0; column < columns; )
			{
				packetHeader = *buf_p++;
				packetSize = 1 + (packetHeader & 0x7F);
				if (packetHeader & 0x80)
				{	// run-length packet
					switch (header.pixel_size)
					{
					case 24:
						b = *buf_p++;
						g = *buf_p++;
						r = *buf_p++;
						a = 255;
						break;
					case 32:
						b = *buf_p++;
						g = *buf_p++;
						r = *buf_p++;
						a = *buf_p++;
						break;
					default:
						Com_Error (ERR_DROP, "LoadTGA(%s): illegal pixel size %d\n", name, header.pixel_size);
					}

					for (j = 0; j < packetSize; j++)
					{
						*pixbuf++ = r;
						*pixbuf++ = g;
						*pixbuf++ = b;
						*pixbuf++ = a;
						column++;
						if (column == columns)
						{	// run spans across rows
							column=0;
							if (row>0)
								row--;
							else
								goto breakOut;
							pixbuf = rgba + row * columns * 4;
						}
					}
				}
				else
				{	// non run-length packet
					for(j = 0; j < packetSize; j++)
					{
						switch (header.pixel_size)
						{
						case 24:
							b = *buf_p++;
							g = *buf_p++;
							r = *buf_p++;
							a = 255;
							break;
						case 32:
							b = *buf_p++;
							g = *buf_p++;
							r = *buf_p++;
							a = *buf_p++;
							break;
						default:
							Com_Error (ERR_DROP, "LoadTGA(%s): illegal pixel size %d in file %s\n", name, header.pixel_size);
						}
						*pixbuf++ = r;
						*pixbuf++ = g;
						*pixbuf++ = b;
						*pixbuf++ = a;
						column++;
						if (column == columns)
						{	// pixel packet run spans across rows
							column = 0;
							if (row > 0)
								row--;
							else
								goto breakOut;
							pixbuf = rgba + row * columns * 4;
						}
					}
				}
			}
breakOut:;
		}
	}

	FS_FreeFile (buffer);
}


/*-------------------------- JPG loading --------------------------*/


static char *jpegname;
static qboolean jpegerror;

static void JpegError ()
{
	if (jpegerror) return;
	Com_WPrintf ("Damaged JPEG file %s\n", jpegname);
//	Com_Error (ERR_DROP, "Bad JPEG");
	jpegerror = true;
}

METHODDEF(void) J_InitSource (j_decompress_ptr cinfo)
{
}


METHODDEF(boolean) J_FillInputBuffer (j_decompress_ptr cinfo)
{
	JpegError ();
	return true;
}

METHODDEF(void) J_SkipInputData (j_decompress_ptr cinfo, long num_bytes)
{
	if (num_bytes <= 0) return;
	cinfo->src->next_input_byte += (size_t) num_bytes;
	cinfo->src->bytes_in_buffer -= (size_t) num_bytes;
}

METHODDEF(void) J_TermSource (j_decompress_ptr cinfo)
{
}

METHODDEF(void) J_ErrorExit (j_common_ptr cinfo)
{
	Com_Error (ERR_DROP, "JPEG error");
}

METHODDEF(void) J_EmitMessage (j_common_ptr cinfo, int msg_level)
{
	if (msg_level < 0)
	{	// warning
		Com_DPrintf ("JPEG warning %d\n", cinfo->err->msg_code);
	}
	// else -- trace message
}


METHODDEF(void) J_ResetErrorMgr (j_common_ptr cinfo)
{
	cinfo->err->num_warnings = 0;
	cinfo->err->msg_code = 0;
}

// Replacement for jpeg_std_error
static struct jpeg_error_mgr *InitJpegError (struct jpeg_error_mgr *err)
{
	memset (err, 0, sizeof(*err));

	err->error_exit = J_ErrorExit;
	err->emit_message = J_EmitMessage;
	err->output_message = J_ErrorExit;
//	err->format_message = NULL;			// used only from jerror.c
	err->reset_error_mgr = J_ResetErrorMgr;

//	err->trace_level = 0;				// default = no tracing
//	err->num_warnings = 0;				// no warnings emitted yet
//	err->msg_code = 0;					// may be useful as a flag for "no error"

//	err->jpeg_message_table = NULL;		// used only from jerror.c
//	err->last_jpeg_message = 0;			// ...

//	err->addon_message_table = NULL;
//	err->first_addon_message = 0;		//for safety
//	err->last_addon_message = 0;

	return err;
}


void LoadJPG (char *name, byte **pic, int *width, int *height)
{
	int	columns, rows;
	byte	*buffer, *decompr, *scanline, *in, *out;
	int	length, i;
	qboolean error;
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;
	byte	line[MAX_IMG_SIZE*3];

	*pic = NULL;

	error = jpegerror = false;

	// load the file
	length = FS_LoadFile (name, (void **)&buffer);
	if (!buffer)
	{
//		Com_DPrintf ("Bad jpeg file %s\n", name);
		return;
	}

	jpegname = name;

	cinfo.err = InitJpegError(&jerr);
	jpeg_create_decompress(&cinfo);

	// init jpeg source
	cinfo.src = (struct jpeg_source_mgr *) (*cinfo.mem->alloc_small) ((j_common_ptr) &cinfo, JPOOL_PERMANENT,
		sizeof(struct jpeg_source_mgr));
	cinfo.src->init_source = J_InitSource;
	cinfo.src->fill_input_buffer = J_FillInputBuffer;
	cinfo.src->skip_input_data = J_SkipInputData;
	cinfo.src->resync_to_restart = jpeg_resync_to_restart;  // default method
	cinfo.src->term_source = J_TermSource;
	cinfo.src->bytes_in_buffer = length;
	cinfo.src->next_input_byte = buffer;

	jpeg_read_header (&cinfo, true);
	jpeg_start_decompress (&cinfo);

	columns = cinfo.output_width;
	rows = cinfo.output_height;

	if (cinfo.output_components != 3)
	{
		Com_WPrintf ("LoadJPG(%s): color components not equal 3\n", name);
		error = true;
	}
	else if (columns > MAX_IMG_SIZE || rows > MAX_IMG_SIZE)
	{
		Com_WPrintf ("LoadJPG(%s): image is too large\n", name);
		error = true;
	}

	if (error)
	{
		jpeg_destroy_decompress (&cinfo);
		FS_FreeFile (buffer);
		return;
	}

	decompr = Z_Malloc (columns * rows * 4);

	scanline = line;
	out = decompr;

	while (cinfo.output_scanline < rows)
	{
		jpeg_read_scanlines (&cinfo, &scanline, 1);

		in = line;
		for (i = 0; i < columns; i++)
		{
			*out++ = *in++;	// r
			*out++ = *in++;	// g
			*out++ = *in++;	// b
			*out++ = 255;	// a
		}
	}

	jpeg_finish_decompress (&cinfo);
	jpeg_destroy_decompress (&cinfo);

	if (!jpegerror)
	{
		*pic = decompr;
		*width = columns;
		*height = rows;
	}

	FS_FreeFile (buffer);
}


/*
 * Finds name.* images within all game dirs and paks; returns 0 if not found or OR'ed
 * set of IMAGE_XXX flags. Name should not have dots and MUST be in a lowercase
 */

int ImageExists (char *name, int stop_mask)
{
	basenamed_t *list, *item;
	char	*ext;

	char *path = NULL;
	int out = 0;

	while (path = FS_NextPath (path))
	{
		if (list = FS_ListFiles (va("%s/%s.*", path, name), NULL, 0 , SFF_SUBDIR))
		{
			for (item = list; item; item = item->next)
			{
				ext = strrchr (item->name, '/'); // find filename
				if (!ext) continue;
				ext = strchr (ext, '.'); // find FIRST dot in a filename
				if (!ext) continue;

				if (!strcmp (ext, ".pcx")) out |= IMAGE_PCX;
				else if (!strcmp (ext, ".wal")) out |= IMAGE_WAL;
				else if (!strcmp (ext, ".tga")) out |= IMAGE_TGA;
				else if (!strcmp (ext, ".jpg")) out |= IMAGE_JPG;
			}
			FreeNamedList (list);
		}
		if (out & stop_mask) break;
		if ((out & IMAGE_8BIT) && (out & IMAGE_32BIT))
			break; // found both 8 and 32-bit images - stop search
	}

	return out;
}


/*------------------------- Image writting ------------------------*/

// All functions receive data in RGB format; data may be damaged while writting.

qboolean WriteTGA (char *name, byte *pic, int width, int height)
{
	FILE	*f;
	tgaHdr_t header;
	int		size, i;
	byte	*pic2;

	if (!(f = fopen (name, "wb")))
	{
		Com_WPrintf ("WriteTGA(%s): cannot create file\n", name);
		return false;
	}

	size = width * height;
	// convert RGB to BGR (inplace conversion !)
	for (i = 0, pic2 = pic; i < size; i++, pic2 += 3)
	{
		byte	tmp;

		tmp = pic2[2];		// B
		pic2[2] = pic2[0];	// R
		pic2[0] = tmp;
	}

	// write header
	memset (&header, 0, sizeof(header));
	header.image_type = 2;		// uncompressed
	header.width = width;
	header.height = height;
	header.pixel_size = 24;
	// write data
	fwrite (&header, 1, sizeof(header), f);
	fwrite (pic, 1, size * 3, f);

	fclose (f);
	return true;
}


qboolean WriteJPG (char *name, byte *pic, int width, int height, qboolean highQuality)
{
	FILE	*f;
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;
	JSAMPROW row[1];
	int		stride;


	if (!(f = fopen (name, "wb")))
	{
		Com_WPrintf ("WriteJPG(%s): cannot create file\n", name);
		return false;
	}

	// standard way to encode JPEG file (see libjpeg.doc for details)
	cinfo.err = InitJpegError (&jerr);
	jpeg_create_compress (&cinfo);
	jpeg_stdio_dest (&cinfo, f);

	cinfo.image_width = width;
	cinfo.image_height = height;
	cinfo.input_components = 3;
	cinfo.in_color_space = JCS_RGB;
	jpeg_set_defaults (&cinfo);
	if (highQuality)
		jpeg_set_quality (&cinfo, 100, TRUE);

	jpeg_start_compress (&cinfo, TRUE);
	stride = 3 * width;
	while (cinfo.next_scanline < height)
	{
		row[0] = pic + stride * (height  - 1 - cinfo.next_scanline);	// top-to-bottom?
		jpeg_write_scanlines (&cinfo, row, 1);
	}
	jpeg_finish_compress (&cinfo);
	jpeg_destroy_compress (&cinfo);

	fclose (f);
	return true;
}
