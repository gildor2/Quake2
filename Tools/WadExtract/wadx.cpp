//?? load whole WAD file for speedup extraction
//?? link with Core/qcommon/lib, store jpg when no alpha-channel used
//?? cmdline option "-norle": do not compress with RLE -- may be better compression with ZIP

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if _MSC_VER
#	define snprintf _snprintf
#endif

//#define DEBUG		1

#if DEBUG
#	define LOG		printf
#else
#	pragma warning(disable : 4002)
#	define LOG()
#endif


#define BYTES4(a,b,c,d)	((a) | ((b)<<8) | ((c)<<16) | ((d)<<24))
typedef unsigned char byte;
template<class T> inline void Exchange(T& A, T& B)
{
	const T tmp = A;
	A = B;
	B = tmp;
}


// WAD/HL texture format

struct WadHdr_t
{
	unsigned signature;
	int		numLumps;
	int		infoOffset;
};


struct TexInfo_t
{
	int		fileOfs, fileSize;
	int		size;
	byte	type;
	bool	compression;		// reserved
	byte	pad[2];
	char	name[16];
};

struct dBsp1Miptex_t
{
	char	name[16];
	unsigned width, height;
	unsigned offsets[4];
};


// TGA file format

#define TGA_ORIGIN_MASK		0x30
#define TGA_BOTLEFT			0x00
#define TGA_BOTRIGHT		0x10					// unused
#define TGA_TOPLEFT			0x20
#define TGA_TOPRIGHT		0x30					// unused

#if _MSC_VER
#pragma pack(push,1)
#endif

struct tgaHdr_t
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


// File reader

FILE *f;

void Read(int count, void *data)
{
	if (fread(data, count, 1, f) != 1)
	{
		printf("Unable to read %d bytes from source file\n", count);
		exit(1);
	}
}


void main(int argc, char **argv)
{
	int i;

	if (argc != 2)
	{
		printf("Usage: wadx filename.wad\n");
		exit(1);
	}
	if (!(f = fopen(argv[1], "rb")))
	{
		printf("Cannot open source file %s\n", argv[1]);
		exit(1);
	}

	WadHdr_t hdr;
	Read(sizeof(hdr), &hdr);
	if (hdr.signature != BYTES4('W','A','D','2') && hdr.signature != BYTES4('W','A','D','3'))
	{
		printf("Invalid WAD file\n");
		exit(1);
	}

	int colormapCompr = 0;
	int rleCompr      = 0;
	int pos = hdr.infoOffset;
	for (int numTex = 0; numTex < hdr.numLumps; numTex++)
	{
		TexInfo_t tex;
		fseek(f, pos, SEEK_SET);
		Read(sizeof(tex), &tex);
		pos += sizeof(tex);

		printf("%d: %s\n", numTex, tex.name);
		char filename[64];
		snprintf(filename, 64, "%s.tga", tex.name);
		strlwr(filename);
		bool alpha = (filename[0] == '{');
		FILE *out = fopen(filename, "wb");
		if (!out)
		{
			printf("Cannot create \"%s\"\n", tex.name);
			continue;
		}

		dBsp1Miptex_t mip;					// texture info
		static byte buf[1024*1024];			// buffer for texture; "static" to avoid stack overflow
		byte pal[256*3];					// palette
		// read texture info
		fseek(f, tex.fileOfs, SEEK_SET);
		LOG("reading mip\n");
		Read(sizeof(dBsp1Miptex_t), &mip);
		bool singleMip = false;
		if (stricmp(mip.name, tex.name))
		{
			LOG("single texture: \"%s\"\n", tex.name);
			fseek(f, tex.fileOfs, SEEK_SET);
			Read(8, &mip.width);
			singleMip = true;
		}
		int width  = mip.width;
		int height = mip.height;
		int size   = width * height;
		// read texture
		LOG("reading tex %d x %d\n", width, height);
		Read(size, buf);
		// read colormap
		LOG("reading pal\n");
		if (!singleMip)
			fseek(f, size * (85-64) / 64 + 2, SEEK_CUR); // skip 3 mipmaps + 2 bytes
		else
			fseek(f, 2, SEEK_CUR);
		Read(256*3, pal);

		// optimize colormap
		bool colorUsed[256];
		// find used colormap indexes
		memset(colorUsed, 0, sizeof(colorUsed));
		for (i = 0; i < size; i++)
			colorUsed[buf[i]] = true;
		// compress colormap
		byte colorRemap[256];
		int numColors = 0;
		for (i = 0; i < 256; i++)
		{
			if (!colorUsed[i])
			{
				colormapCompr += 3;
				continue;
			}
			colorRemap[i] = numColors;
			memcpy(pal + numColors*3, pal + i*3, 3);
			numColors++;
		}
		//?? can sort colorRemap[] by frequency for better compression with
		//?? solid archive format
		// modify texture
		for (i = 0; i < size; i++)
			buf[i] = colorRemap[buf[i]];

		// compress image with RLE
		// taken from main engine, but adapted for paletted textures
		static byte packed[1024*1024];
		byte *threshold = packed + size - 16;			// threshold for "dst"

		byte *src = buf;
		byte *dst = packed;
		int column = 0;
		byte *flag = NULL;
		bool rle = false;

		bool useCompression = true;
		for (i = 0; i < size; i++)
		{
			if (dst >= threshold)								// when compressed is too large, same uncompressed
			{
				useCompression = false;
				break;
			}

			byte b = *src++;

			if (column < width - 1 &&							// not on screen edge; NOTE: when i == size-1, col==width-1
				b == src[0] &&									// next byte will be the same
				!(rle && flag && *flag == 254))					// flag overflow
			{
				if (!rle || !flag)
				{
					// starting new RLE sequence
					flag = dst++;
					*flag = 128 - 1;							// will be incremented below
					*dst++ = b;
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
					*dst++ = b;									// copy texel
					(*flag)++;
					if (*flag == 127) flag = NULL;				// check for overflow
				}
				rle = false;
			}

			if (++column == width) column = 0;
		}

		// create header
		tgaHdr_t outHdr;
		memset(&outHdr, 0, sizeof(tgaHdr_t));
		outHdr.colormap_type   = 1;
		outHdr.image_type      = (useCompression) ? 9 : 1;		// paletted with and without RLE compression
		outHdr.colormap_length = numColors;
		outHdr.colormap_size   = alpha ? 32 : 24;
		outHdr.width           = width;
		outHdr.height          = height;
		outHdr.pixel_size      = 8;
		outHdr.attributes      = TGA_TOPLEFT;
		fwrite(&outHdr, sizeof(tgaHdr_t), 1, out);

		// write colormap
		// swap rgb <-> bgr
		for (i = 0; i < numColors*3; i += 3)
			Exchange(pal[i], pal[i+2]);
		if (!alpha)
			fwrite(pal, numColors*3, 1, out);
		else
		{
			int alphaIdx = colorRemap[255]*3;
			for (i = 0; i < numColors*3; i += 3)
			{
				// write RGB
				fwrite(&pal[i], 3, 1, out);
				// write alpha
				byte alphaChn[] = {255, 0};
				fwrite(&alphaChn[i == alphaIdx], 1, 1, out);	// old color == 255 => transparent
			}
		}

		// write texture
		if (!useCompression)
			fwrite(buf, size, 1, out);
		else
		{
			rleCompr += size - (dst - packed);
			fwrite(packed, dst - packed, 1, out);
		}

		fclose(out);
	}

	fclose(f);
	// display stats
	printf("colormap compression: %d bytes\n"
		   "texture compression:  %d bytes\n",
		   colormapCompr, rleCompr);
}
