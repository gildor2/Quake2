#include "qcommon.h"

// allow jpeglib to work from C++ source (no "#ifdef __cplusplus ..." in jpeglib.h)
extern "C" {
#include "../lib/jpeglib/jpeglib.h"
}


#define MAX_IMG_SIZE	2048


/*-----------------------------------------------------------------------------
	PCX loading
-----------------------------------------------------------------------------*/

// NOTE: this structure have good 4-byte alignment
struct pcxHdr_t
{
	char	manufacturer;
	char	version;
	char	encoding;
	char	bits_per_pixel;
	unsigned short xmin,ymin,xmax,ymax;
	unsigned short hres,vres;
	unsigned char palette[48];
	char	reserved;
	char	color_planes;
	unsigned short bytes_per_line;
	unsigned short palette_type;
	char	filler[58];
};


byte* LoadPCX (const char *name, int &width, int &height, byte *&palette)
{
	palette = NULL;

	byte *file;
	unsigned filelen;
	if (!(file = (byte*) GFileSystem->LoadFile (name, &filelen))) return NULL;

	pcxHdr_t *hdr = (pcxHdr_t*)file;

	int w = LittleShort(hdr->xmax) + 1;
	int h = LittleShort(hdr->ymax) + 1;

	const char *errMsg = NULL;
	if (filelen < sizeof(pcxHdr_t) + 768 || hdr->manufacturer != 10 || hdr->version != 5 || hdr->encoding != 1 || hdr->bits_per_pixel != 8)
		errMsg = "bad pcx file";
	else if (w > MAX_IMG_SIZE || h > MAX_IMG_SIZE)
		errMsg = "image is too large";

	if (errMsg)
	{
		delete file;
		Com_DropError ("LoadPCX(%s): %s", name, errMsg);
	}

	palette = new byte [768];
	memcpy (palette, (byte *)hdr + filelen - 768, 768);

	width  = w;
	height = h;

	byte *src = (byte*)(hdr + 1);
	byte *dst = (byte*)appMalloc (w * h);
	byte *ret = dst;

	for (int y = 0; y < h; y++)
	{
		for (int x = 0; x < w; )
		{
			byte c = *src++;
			if (c >= 0xC0)
			{
				int len = c & 0x3F;
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

	delete hdr;
	return ret;
}


/*-----------------------------------------------------------------------------
	TGA loading
-----------------------------------------------------------------------------*/

#define TGA_ORIGIN_MASK		0x30
#define TGA_BOTLEFT			0x00
#define TGA_BOTRIGHT		0x10					// unused
#define TGA_TOPLEFT			0x20
#define TGA_TOPRIGHT		0x30					// unused

#if _MSC_VER
#pragma pack(push,1)
#endif

struct GCC_PACK tgaHdr_t
{
	byte 	id_length, colormap_type, image_type;
	unsigned short colormap_index, colormap_length;
	byte	colormap_size;
	unsigned short x_origin, y_origin;				// unused
	unsigned short width, height;
	byte	pixel_size, attributes;
};

#if _MSC_VER
#pragma pack(pop)
#endif


byte* LoadTGA (const char *name, int &width, int &height)
{
	byte	*file;
	if (!(file = (byte*) GFileSystem->LoadFile (name))) return NULL;

	tgaHdr_t *hdr = (tgaHdr_t*)file;

	const char *errMsg = NULL;

	// swap header
	LTL(hdr->width);
	LTL(hdr->height);
	LTL(hdr->colormap_index);
	LTL(hdr->colormap_length);

	width  = LittleShort(hdr->width);
	height = LittleShort(hdr->height);

	bool isPaletted = (hdr->image_type == 1 || hdr->image_type == 9);
	bool isRGB      = (hdr->image_type == 2 || hdr->image_type == 10);
	bool isBW       = (hdr->image_type == 3 || hdr->image_type == 11);
	bool isRle      = (hdr->image_type == 9 || hdr->image_type == 10 || hdr->image_type == 11);
	byte colorBits  = (isPaletted) ? hdr->colormap_size : hdr->pixel_size;
	byte colorBytes = colorBits >> 3;

	if (!(isPaletted || isRGB || isBW || isRle))
		errMsg = va("unsupported type %d", hdr->image_type);
	else if (colorBits != 24 && colorBits != 32)
		errMsg = va("unsupported color depth %d", colorBits);
	else if ((isBW || isPaletted) && hdr->pixel_size != 8)
		errMsg = va("unsupported color depth %d for format %d", hdr->pixel_size, hdr->image_type);
	else if (width > MAX_IMG_SIZE || height > MAX_IMG_SIZE)
		errMsg = "image is too large";
	else if (isPaletted)
	{
		if (hdr->colormap_type != 1)
			errMsg = va("unsupported colormap type %d", hdr->colormap_type);
		else if (hdr->colormap_index != 0 || hdr->colormap_length != 256)
			errMsg = "partial colormaps unsupported";
	}

	if (errMsg)
	{
		delete file;
		Com_DropError ("LoadTGA(%s): %s", name, errMsg);
	}

	int numPixels = width * height;

	byte *src = (byte*)(hdr + 1);
	src += hdr->id_length;							// skip image comment
	byte *pal = NULL;
	if (isPaletted)
	{
		pal = src;
		src += hdr->colormap_length * colorBytes;	// skip palette
	}

	byte *dst = new byte [numPixels * 4];
	byte *ret = dst;

	int stride;
	if ((hdr->attributes & TGA_ORIGIN_MASK) == TGA_TOPLEFT)
	{
		stride = 0;
	}
	else
	{
		stride = -width * 4 * 2;
		dst = dst + (height - 1) * width * 4;
	}

	int column = 0, num = 0;
	int copy = 0;
	while (num < numPixels)
	{
		byte	r, g, b, a;
		int		ct;

		if (copy) copy--;
		if (isRle && !copy)
		{
			byte f = *src++;
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

		b = *src++;
		if (isRGB)
		{
			g = *src++;
			r = *src++;
			if (colorBytes == 4) a = *src++; else a = 255;
		}
		else if (isPaletted)
		{
			byte *s = pal + b * colorBytes;
			b = *s++;
			g = *s++;
			r = *s++;
			if (colorBytes == 4) a = *s++; else a = 255;
		}
		else // if (isBW)  (monochrome)
		{
			r = g = b;
			a = 255;
		}

		for (int i = 0; i < ct; i++)
		{
			*dst++ = r; *dst++ = g; *dst++ = b; *dst++ = a;
			if (++column == width)
			{
				column = 0;
				dst += stride;
			}
		}
		num += ct;
	}

	delete file;
	return ret;
}


/*-----------------------------------------------------------------------------
	JPG loading
-----------------------------------------------------------------------------*/

static const char *jpegname;
static bool jpegerror;


METHODDEF(void) J_InitSource (j_decompress_ptr cinfo)
{}


METHODDEF(boolean) J_FillInputBuffer (j_decompress_ptr cinfo)
{
	if (!jpegerror)
	{
		appWPrintf ("LoadJPG(%s): damaged file\n", jpegname);
//		Com_DropError ("Bad JPEG");
		jpegerror = true;
	}
	return true;
}

METHODDEF(void) J_SkipInputData (j_decompress_ptr cinfo, long num_bytes)
{
	if (num_bytes <= 0) return;
	cinfo->src->next_input_byte += (size_t) num_bytes;
	cinfo->src->bytes_in_buffer -= (size_t) num_bytes;
}

METHODDEF(void) J_TermSource (j_decompress_ptr cinfo)
{}

METHODDEF(void) J_ErrorExit (j_common_ptr cinfo)
{
	Com_DropError ("JPEG error %d", cinfo->err->msg_code);
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
	cinfo->err->msg_code     = 0;
}

// Replacement for jpeg_std_error
static struct jpeg_error_mgr *InitJpegError (struct jpeg_error_mgr *err)
{
	memset (err, 0, sizeof(*err));

	err->error_exit      = J_ErrorExit;
	err->emit_message    = J_EmitMessage;
	err->output_message  = J_ErrorExit;
//	err->format_message  = NULL;		// used only from jerror.c
	err->reset_error_mgr = J_ResetErrorMgr;

//	err->trace_level  = 0;				// default = no tracing
//	err->num_warnings = 0;				// no warnings emitted yet
//	err->msg_code     = 0;				// may be useful as a flag for "no error"

//	err->jpeg_message_table = NULL;		// used only from jerror.c
//	err->last_jpeg_message  = 0;		// ...

//	err->addon_message_table = NULL;
//	err->first_addon_message = 0;		//for safety
//	err->last_addon_message  = 0;

	return err;
}


byte* LoadJPG (const char *name, int &width, int &height)
{
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;
	byte	line[MAX_IMG_SIZE*3];

	unsigned length;
	byte *buffer;
	if (!(buffer = (byte*) GFileSystem->LoadFile (name, &length))) return NULL;

	jpegerror = false;
	jpegname  = name;

	cinfo.err = InitJpegError (&jerr);
	jpeg_create_decompress (&cinfo);

	// init jpeg source
	cinfo.src = (struct jpeg_source_mgr *) (*cinfo.mem->alloc_small) ((j_common_ptr) &cinfo, JPOOL_PERMANENT,
		sizeof(struct jpeg_source_mgr));
	cinfo.src->init_source       = J_InitSource;
	cinfo.src->fill_input_buffer = J_FillInputBuffer;
	cinfo.src->skip_input_data   = J_SkipInputData;
	cinfo.src->resync_to_restart = jpeg_resync_to_restart;  // default method
	cinfo.src->term_source       = J_TermSource;
	cinfo.src->bytes_in_buffer   = length;
	cinfo.src->next_input_byte   = buffer;

	jpeg_read_header (&cinfo, true);
	jpeg_start_decompress (&cinfo);

	int columns = cinfo.output_width;
	int rows    = cinfo.output_height;

	const char *errMsg = NULL;
	if (cinfo.output_components != 3)
		errMsg = "color components not equal 3";
	else if (columns > MAX_IMG_SIZE || rows > MAX_IMG_SIZE)
		errMsg = "image is too large";

	if (errMsg)
	{
		jpeg_destroy_decompress (&cinfo);
		delete buffer;
		Com_DropError ("LoadJPG(%s): %s", name, errMsg);
	}

	byte *decompr = (byte*)appMalloc (columns * rows * 4);

	byte *scanline = line;
	byte *out      = decompr;

	while (cinfo.output_scanline < rows)
	{
		jpeg_read_scanlines (&cinfo, &scanline, 1);

		byte *in = line;
		for (int i = 0; i < columns; i++)
		{
			*out++ = *in++;	// r
			*out++ = *in++;	// g
			*out++ = *in++;	// b
			*out++ = 255;	// a
		}
	}

	jpeg_finish_decompress (&cinfo);
	jpeg_destroy_decompress (&cinfo);

	delete buffer;
	if (jpegerror)
	{
		delete decompr;
		return NULL;
	}

	width  = columns;
	height = rows;
	return decompr;
}


// Finds name.* images within all game dirs and paks; returns 0 if not found or OR'ed
// set of IMAGE_XXX flags. Name should be without extension and in a lowercase.
int ImageExists (const char *name, int stop_mask)
{
	int out = 0;

	const char *path = NULL;
	while (path = FS_NextPath (path))
	{
		bool rootBased = (name[0] == '.' && name[1] == '/');
		CFileList *list = GFileSystem->List (rootBased ?
			va("%s.*", name) :				// root-based
			va("%s/%s.*", path, name),		// game-based
			FS_FILE);
		for (TListIterator<CFileItem> item = *list; item; ++item)
		{
			const char *ext = strrchr (item->name, '.');	// find last dot in a filename
			if (!ext) continue;

			if		(!strcmp (ext, ".pcx")) out |= IMAGE_PCX;
			else if (!strcmp (ext, ".wal")) out |= IMAGE_WAL;
			else if (!strcmp (ext, ".tga")) out |= IMAGE_TGA;
			else if (!strcmp (ext, ".jpg")) out |= IMAGE_JPG;
		}
		delete list;
		if (rootBased) break;
		if (out & stop_mask) break;
		if ((out & IMAGE_8BIT) && (out & IMAGE_32BIT))
			break; // found both 8 and 32-bit images - stop search
	}

	return out;
}


/*-----------------------------------------------------------------------------
	Image writting
-----------------------------------------------------------------------------*/
// All functions receive data in RGB format; data may be modified while writting.

bool WriteTGA (const char *name, byte *pic, int width, int height)
{
	int		i;

	appMakeDirectoryForFile (name);
	FILE *f;
	if (!(f = fopen (name, "wb")))
	{
		appWPrintf ("WriteTGA(%s): cannot create file\n", name);
		return false;
	}

	byte *src;
	int size = width * height;
	// convert RGB to BGR (inplace!)
	for (i = 0, src = pic; i < size; i++, src += 3)
		Exchange (src[0], src[2]);

	byte *packed = (byte*)appMalloc (width * height * 3);
	byte *threshold = packed + width * height * 3 - 16;		// threshold for "dst"

	src = pic;
	byte *dst = packed;
	int column = 0;
	byte *flag = NULL;
	bool rle = false;

	bool done = true;
	for (i = 0; i < size; i++)
	{
		if (dst >= threshold)								// when compressed is too large, same uncompressed
		{
			done = false;
			break;
		}

		byte b = *src++;
		byte g = *src++;
		byte r = *src++;

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
	tgaHdr_t header;
	memset (&header, 0, sizeof(header));
	header.width      = LittleShort (width);
	header.height     = LittleShort (height);
#if 0
	// write black/white image
	header.pixel_size = 8;
	header.image_type = 3;
	fwrite (&header, 1, sizeof(header), f);
	for (i = 0; i < width * height; i++, pic += 3)
	{
		int c = (pic[0]+pic[1]+pic[2]) / 3;
		fwrite (&c, 1, 1, f);
	}
#else
	header.pixel_size = 24;
	if (done)
	{
		Com_DPrintf ("WriteTGA: packed %d -> %d\n", size * 3, dst - packed);
		header.image_type = 10;		// RLE
		// write data
		fwrite (&header, 1, sizeof(header), f);
		fwrite (packed, 1, dst - packed, f);
	}
	else
	{
		Com_DPrintf ("WriteTGA: revert to uncompressed\n");
		header.image_type = 2;		// uncompressed
		// write data
		fwrite (&header, 1, sizeof(header), f);
		fwrite (pic, 1, size * 3, f);
	}
#endif

	fclose (f);
	appFree (packed);
	return true;
}


bool WriteJPG (const char *name, byte *pic, int width, int height, bool highQuality)
{
	appMakeDirectoryForFile (name);
	FILE	*f;
	if (!(f = fopen (name, "wb")))
	{
		appWPrintf ("WriteJPG(%s): cannot create file\n", name);
		return false;
	}

	// standard way to encode JPEG file (see libjpeg.doc for details)
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;
	cinfo.err = InitJpegError (&jerr);
	jpeg_create_compress (&cinfo);
	jpeg_stdio_dest (&cinfo, f);

	cinfo.image_width      = width;
	cinfo.image_height     = height;
	cinfo.input_components = 3;
	cinfo.in_color_space   = JCS_RGB;
	jpeg_set_defaults (&cinfo);
	if (highQuality)
		jpeg_set_quality (&cinfo, 100, TRUE);

	jpeg_start_compress (&cinfo, TRUE);
	int stride = 3 * width;
	while (cinfo.next_scanline < height)
	{
		JSAMPROW row[1];
		row[0] = pic + stride * (height - 1 - cinfo.next_scanline);	// top-to-bottom?
		jpeg_write_scanlines (&cinfo, row, 1);
	}
	jpeg_finish_compress (&cinfo);
	jpeg_destroy_compress (&cinfo);

	fclose (f);
	return true;
}
