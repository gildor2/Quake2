#include "qcommon.h"

// hack to allow jpeglib to work from C++ source (no "#ifdef __cplusplus ..." in jpeglib.h)
extern "C" {
#include "../lib/jpeglib/jpeglib.h"
}


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
} pcx_t;


void LoadPCX (const char *name, byte **pic, byte **palette, int *width, int *height)
{
	byte	*src, *dst;
	pcx_t	*hdr;
	unsigned filelen;
	int		x, y, w, h;
	char	*errMsg;

	*pic = NULL;
	*palette = NULL;

	if (!(hdr = (pcx_t*) FS_LoadFile (name, &filelen))) return;

	src = (byte *)(hdr + 1);

//	hdr->xmin = LittleShort(hdr->xmin);
//	hdr->ymin = LittleShort(hdr->ymin);
	w = LittleShort(hdr->xmax) + 1;
	h = LittleShort(hdr->ymax) + 1;
//	hdr->hres = LittleShort(hdr->hres);
//	hdr->vres = LittleShort(hdr->vres);
//	hdr->bytes_per_line = LittleShort(hdr->bytes_per_line);
//	hdr->palette_type = LittleShort(hdr->palette_type);

	errMsg = NULL;
	if (filelen < sizeof(pcx_t) + 768 || hdr->manufacturer != 10 || hdr->version != 5 || hdr->encoding != 1 || hdr->bits_per_pixel != 8)
		errMsg = "LoadPCX(%s): bad pcx file\n";
	else if (w > MAX_IMG_SIZE || h > MAX_IMG_SIZE)
		errMsg = "LoadPCX(%s): image is too large\n";

	if (errMsg)
	{
		FS_FreeFile (hdr);
		Com_DropError (errMsg, name);
	}

	if (palette)
	{
		*palette = (byte*)appMalloc (768);
		memcpy (*palette, (byte *)hdr + filelen - 768, 768);
	}

	if (width)	*width = w;
	if (height)	*height = h;

	dst = (byte*)appMalloc (w * h);
	*pic = dst;

	for (y = 0; y < h; y++)
	{
		for (x = 0; x < w; )
		{
			int		len;
			byte	c;

			c = *src++;
			if (c >= 0xC0)
			{
				len = c & 0x3F;
				c = *src++;
				while (len-- > 0)
				{
					*dst++ = c;
					if (++x == w) break;
				}
			}
			else
			{
				*dst++ = c;
				x++;
			}
		}
	}

	FS_FreeFile (hdr);
}


/*-------------------------- TGA loading --------------------------*/

#define TGA_ORIGIN_MASK		0x30
#define TGA_BOTLEFT			0x00
#define TGA_BOTRIGHT		0x10		// unused
#define TGA_TOPLEFT			0x20
#define TGA_TOPRIGHT		0x30		// unused

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
void LoadTGA (const char *name, byte **pic, int *width, int *height)
{
	int		numColumns, numRows, bpp, numPixels;
	int		type, flags;
	int		num, column, stride, copy;
	byte	*file, *src, *dst;
	tgaHdr_t header;		//??
	char	*errMsg;

	*pic = NULL;

	if (!(file = (byte*) FS_LoadFile (name))) return;

	src = file;

#define GET_BYTE(x)		x = *src++
#define GET_SHORT(x)	x = LittleShort(*(short*)src); src += 2;

	GET_BYTE(header.id_length);
	GET_BYTE(header.colormap_type);
	GET_BYTE(type);

	GET_SHORT(header.colormap_index);
	GET_SHORT(header.colormap_length);
	GET_BYTE(header.colormap_size);
	GET_SHORT(header.x_origin);
	GET_SHORT(header.y_origin);
	GET_SHORT(numColumns);
	GET_SHORT(numRows);
	GET_BYTE(bpp);
	GET_BYTE(flags);

	errMsg = NULL;

	if (type != 2 && type != 10 && type != 3)
		errMsg = "LoadTGA(%s): Only type 2 (RGB), 3 (grey) and 10 (RGB RLE) targa RGB images supported\n";
	else if (header.colormap_type != 0)
		errMsg = "LoadTGA(%s): colormaps not supported\n";
	else if (bpp != 32 && bpp != 24 && !(type == 3 && bpp == 8))
	{
		FS_FreeFile (file);
		Com_DropError ("LoadTGA(%s): invalid color depth %d for format %d\n", name, bpp, type);
	}
	else if (numColumns > MAX_IMG_SIZE || numRows > MAX_IMG_SIZE)
		errMsg = "LoadTGA(%s): image is too large\n";

	if (errMsg)
	{
		FS_FreeFile (file);
		Com_DropError (errMsg, name);
	}

	numPixels = numColumns * numRows;

	if (width)	*width = numColumns;
	if (height)	*height = numRows;

	dst = (byte*)appMalloc (numPixels * 4);
	*pic = dst;

	if (header.id_length != 0)
		src += header.id_length;		// skip image comment

	if ((flags & TGA_ORIGIN_MASK) == TGA_TOPLEFT)
	{
		stride = 0;
	}
	else
	{
		stride = -numColumns * 4 * 2;
		dst = dst + (numRows - 1) * numColumns * 4;
	}

	column = num = 0;
	copy = 0;
	while (num < numPixels)
	{
		byte	r, g, b, a;
		int		ct, i;

		if (copy) copy--;
		if (type == 10 && !copy)
		{
			byte	f;

			GET_BYTE(f);
			if (f & 0x80)
				ct = f + 1 - 0x80;	// packed
			else
			{
				copy = f + 1;		// not packed
				ct = 1;
			}
		}
		else
			ct = 1;

		GET_BYTE(b);
		if (bpp > 8)
		{
			GET_BYTE(g);
			GET_BYTE(r);
			if (bpp == 32) GET_BYTE(a); else a = 255;
		}
		else
		{
			r = g = b;
			a = 255;
		}

		for (i = 0; i < ct; i++)
		{
			*dst++ = r; *dst++ = g; *dst++ = b; *dst++ = a;
			if (++column == numColumns)
			{
				column = 0;
				dst += stride;
			}
		}
		num += ct;
	}
#undef GET_SHORT
#undef GET_BYTE

	FS_FreeFile (file);
}


/*-------------------------- JPG loading --------------------------*/


static const char *jpegname;
static bool jpegerror;

static void JpegError ()
{
	if (jpegerror) return;
	Com_WPrintf ("LoadJPG(%s): damaged file\n", jpegname);
//	Com_DropError ("Bad JPEG");
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
	Com_DropError ("JPEG error");
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


void LoadJPG (const char *name, byte **pic, int *width, int *height)
{
	int		i, columns, rows;
	unsigned length;
	byte	*buffer, *decompr, *scanline, *in, *out;
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;
	byte	line[MAX_IMG_SIZE*3];
	char	*errMsg;

	*pic = NULL;

	jpegerror = false;

	if (!(buffer = (byte*) FS_LoadFile (name, &length))) return;

	jpegname = name;
	errMsg = NULL;

	cinfo.err = InitJpegError (&jerr);
	jpeg_create_decompress (&cinfo);

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
		errMsg = "LoadJPG(%s): color components not equal 3\n";
	else if (columns > MAX_IMG_SIZE || rows > MAX_IMG_SIZE)
		errMsg = "LoadJPG(%s): image is too large\n";

	if (errMsg)
	{
		jpeg_destroy_decompress (&cinfo);
		FS_FreeFile (buffer);
		Com_DropError (errMsg, name);
	}

	decompr = (byte*)appMalloc (columns * rows * 4);

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

int ImageExists (const char *name, int stop_mask)
{
	basenamed_t *list, *item;
	char	*ext;

	char *path = NULL;
	int out = 0;

	while (path = FS_NextPath (path))
	{
		if (list = FS_ListFiles (va("%s/%s.*", path, name), NULL, LIST_FILES))
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

bool WriteTGA (const char *name, byte *pic, int width, int height)
{
	FILE	*f;
	tgaHdr_t header;
	int		size, i, column;
	byte	*src, *dst, *packed, *flag, *threshold;
	bool	rle, done;

	if (!(f = fopen (name, "wb")))
	{
		Com_WPrintf ("WriteTGA(%s): cannot create file\n", name);
		return false;
	}

	size = width * height;
	// convert RGB to BGR (inplace conversion !)
	for (i = 0, src = pic; i < size; i++, src += 3)
	{
		byte	tmp;

		tmp = src[2];		// B
		src[2] = src[0];	// R
		src[0] = tmp;
	}

	packed = (byte*)appMalloc (width * height * 3);
	threshold = packed + width * height * 3 - 16;			// threshold for "dst"

	src = pic;
	dst = packed;
	i = column = 0;
	flag = NULL;
	rle = false;

	done = true;
	for (i = 0; i < size; i++)
	{
		byte	r, g, b;

		if (dst >= threshold)								// when compressed is too large, same uncompressed
		{
			done = false;
			break;
		}

		b = *src++;
		g = *src++;
		r = *src++;

		if (column < width - 1 &&							// not on screen edge; NOTE: when i == size-1, col==width-1
			b == src[0] && g == src[1] && r == src[2] &&	// next byte will be the same
			!(rle && flag && *flag == 254))					// flag overflow
		{
			if (!rle || !flag)
			{
				// starting new RLE sequence
				flag = dst++;
				*flag = 128 - 1;							// will be incremented below
				*dst++ = b; *dst++ = g; *dst++ = r;
			}
			(*flag)++;										// enqueue one more texel
			rle = true;
		}
		else
		{
			if (rle)
			{
				// previous block was RLE, and next (now: current) byte was
				// the same - enqueue it to previous block and close block
				(*flag)++;
				flag = NULL;
			}
			else
			{
				if (!flag)
				{
					// start new copy sequence
					flag = dst++;
					*flag = 0 - 1;							// 255, to be exact
				}
				*dst++ = b; *dst++ = g; *dst++ = r;			// copy texel
				(*flag)++;
				if (*flag == 127) flag = NULL;				// check for overflow
			}
			rle = false;
		}

		if (++column == width) column = 0;
	}

	// write header
	memset (&header, 0, sizeof(header));
	header.width = LittleShort (width);
	header.height = LittleShort (height);
	header.pixel_size = 24;
	if (done)
	{
		Com_DPrintf ("WriteTGA(): packed %d -> %d\n", size * 3, dst - packed);
		header.image_type = 10;		// RLE
		// write data
		fwrite (&header, 1, sizeof(header), f);
		fwrite (packed, 1, dst - packed, f);
	}
	else
	{
		Com_DPrintf ("WriteTGA(): revert to uncompressed\n");
		header.image_type = 2;		// uncompressed
		// write data
		fwrite (&header, 1, sizeof(header), f);
		fwrite (pic, 1, size * 3, f);
	}

	fclose (f);
	appFree (packed);
	return true;
}


bool WriteJPG (const char *name, byte *pic, int width, int height, bool highQuality)
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
		row[0] = pic + stride * (height - 1 - cinfo.next_scanline);	// top-to-bottom?
		jpeg_write_scanlines (&cinfo, row, 1);
	}
	jpeg_finish_compress (&cinfo);
	jpeg_destroy_compress (&cinfo);

	fclose (f);
	return true;
}
