//?? load whole WAD file for speedup extraction
//?? store compact palette (when not all indices used)
//?? connect to Core/qcommon, store jpg when no alpha-channel used
//?? store alpha-channel when present

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define BYTES4(a,b,c,d)	((a) | ((b)<<8) | ((c)<<16) | ((d)<<24))
typedef unsigned char byte;
template<class T> inline void Exchange (T& A, T& B)
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


// TGA format

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

void Read (int count, void *data)
{
	if (fread (data, count, 1, f) != 1)
	{
		printf ("Unable to read source file\n");
		exit (1);
	}
}


void main (int argc, char **argv)
{
	if (argc != 2)
	{
		printf ("Usage: wadx filename.wad\n");
		exit (1);
	}
	if (!(f = fopen (argv[1], "rb")))
	{
		printf ("Cannot open source file %s\n", argv[1]);
		exit (1);
	}

	WadHdr_t hdr;
	Read (sizeof(hdr), &hdr);
	if (hdr.signature != BYTES4('W','A','D','2') && hdr.signature != BYTES4('W','A','D','3'))
	{
		printf ("Invalid WAD file\n");
		exit (1);
	}

	fseek (f, hdr.infoOffset, SEEK_SET);

	for (int numTex = 0; numTex < hdr.numLumps; numTex++)
	{
		TexInfo_t tex;
		Read (sizeof(tex), &tex);
		int pos = ftell (f);

		printf ("%d: %s\n", numTex, tex.name);
		char filename[64];
		_snprintf (filename, 64, "%s.tga", tex.name);
		FILE *out = fopen (filename, "wb");
		if (!out)
		{
			printf ("Cannot create \"%s\"\n", tex.name);
			continue;
		}

		fseek (f, tex.fileOfs, SEEK_SET);
		dBsp1Miptex_t mip;
		Read (sizeof(dBsp1Miptex_t), &mip);

		// create header
		tgaHdr_t outHdr;
		memset (&outHdr, 0, sizeof(tgaHdr_t));
		outHdr.colormap_type   = 1;
		outHdr.image_type      = 1;			// paletted, uncompressed
		outHdr.colormap_length = 256;
		outHdr.colormap_size   = 24;		//?? or 32
		outHdr.width           = mip.width;
		outHdr.height          = mip.height;
		outHdr.pixel_size      = 8;
		outHdr.attributes      = TGA_TOPLEFT;
#if 1
		fwrite (&outHdr, sizeof(tgaHdr_t), 1, out);
		// write colormap
		fseek (f, mip.width * mip.height * 85 / 64 + 2, SEEK_CUR);
		byte pal[256*3];
		Read (256*3, pal);
		// swap rgb <-> bgr
		for (int i = 0; i < 256*3; i += 3)
			Exchange (pal[i], pal[i+2]);

		fwrite (&pal, 256*3, 1, out);		//?? *4
		// seek back to texture
		fseek (f, tex.fileOfs+sizeof(dBsp1Miptex_t), SEEK_SET);
#else
		// export as black/white image (testing)
		outHdr.colormap_type   = 0;
		outHdr.image_type      = 3;
		outHdr.colormap_length = 0;
		outHdr.colormap_size   = 0;
		fwrite (&outHdr, sizeof(tgaHdr_t), 1, out);
#endif

		byte buf[512*512];					//?? greater size?
		Read (mip.width * mip.height, buf);
		fwrite (buf, mip.width * mip.height, 1, out);

		// write texture

		fclose (out);
		fseek (f, pos, SEEK_SET);
	}

	fclose (f);
}
