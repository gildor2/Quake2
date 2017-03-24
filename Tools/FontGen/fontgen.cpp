#include <stdio.h>
#include <windows.h>


#pragma comment(lib, "gdi32")
#pragma comment(lib, "user32")

#define ARRAY_ARG(array)	array, sizeof(array)/sizeof(array[0])
#define ARRAY_COUNT(array)	(sizeof(array)/sizeof(array[0]))

#define TEX_WIDTH		256
#define TEX_HEIGHT		256

//#define DEBUG_CPP		1
//#define ALLOW_NON_MONOSPACE_FONT 1

void Error(char *fmt, ...)
{
	va_list	argptr;
	va_start(argptr, fmt);
	char buf[4096];
	int len = _vsnprintf(ARRAY_ARG(buf), fmt, argptr);
	va_end(argptr);
	if (len < 0 || len >= sizeof(buf) - 1) exit(1);
	puts(buf);
	exit(1);
}


#define assert(x)	if (!(x)) { Error("Assertsion failed: %s (%s, %d)\n", #x, __FILE__, __LINE__); }


struct letterloc_t
{
	float pos[2];
	float size[2];
};

struct fontDef_t
{
	int			charWidth, charHeight;
	int			indirection[256];
	letterloc_t	locations[256];
	float		height;
	float		aspectRatio;
	char		name[255];
};

class FontGen
{
private:
	int			firstChar, lastChar, fontSize, fontStyle;

	char		fontName[255];
	char		fontWinName[255];

	// font style
	bool		isBold;
	bool		isItalic;
	bool		isUnderline;
	bool		isStrike;
	bool		isAntialiased;

	// output params
	bool		compressTga;
	bool 		monochrome;
	bool		cppFormat;
	bool		packedCpp;

	// vars
	HFONT		hf;
	HDC			dc;
	HBITMAP		dib, oldbitmap;
	unsigned char *bmbits;
	fontDef_t	font_def;
	int			texture_height;

	// Functions
	void PrepareFont();
	void PrepareBitmap();
	void PrepareHeader();
	void DrawFont();
	void SaveTarga();
	void SaveFontdef();
	void SaveCpp();

public:
	FontGen(int argc, char *argv[]);
	virtual ~FontGen();
	static void Usage();
	void Go();
};


static const char *GetArg(int which, int argc, char **argv)
{
	if (which >= argc)
		return "";
	else
		return argv[which];
}

#define ARG(X) GetArg(X, argc, argv)

static void MemSwap(void *a0, void *b0, int count)
{
	byte *a = (byte*)a0;
	byte *b = (byte*)b0;
	for (int i = 0; i < count; i++)
	{
		byte c = *a;
		*a++ = *b;
		*b++ = c;
	}
}


FontGen::FontGen(int argc, char **argv)
:	hf(NULL)
,	dc(NULL)
,	dib(NULL)
,	oldbitmap(NULL)
,	fontStyle(0)
,	fontSize(16)
,	firstChar(' ')
,	lastChar(126)
,	isBold(false)
,	isUnderline(false)
,	isItalic(false)
,	isStrike(false)
,	isAntialiased(false)
,	compressTga(false)
,	monochrome(false)
,	cppFormat(false)
,	packedCpp(false)
{
	fontName[0] = 0;
	fontWinName[0] = 0;

	for (int argIdx = 1; argIdx < argc; argIdx++)
	{
		const char* arg = argv[argIdx];
		if (!stricmp(arg, "-size"))
		{
			fontSize = atoi(ARG(++argIdx));
		}
		else if (!stricmp(arg, "-styles"))
		{
			const char *styles = ARG(++argIdx);

			fontStyle = 0;
			if (strstr(styles, "bold"))
				isBold = true;
			if (strstr(styles, "italic"))
				isItalic = true;
			if (strstr(styles, "underline"))
				isUnderline = true;
			if (strstr(styles, "strikeout"))
				isStrike = true;
			if (strstr(styles, "antialias"))
				isAntialiased = true;
		}
		else if (!stricmp(arg, "-first"))
		{
			firstChar = atoi(ARG(++argIdx));
		}
		else if (!stricmp(arg, "-last"))
		{
			lastChar = atoi(ARG(++argIdx));
		}
		else if (!stricmp(arg, "-winname"))
		{
			strcpy(fontWinName, ARG(++argIdx));
		}
		else if (!stricmp(arg, "-name"))
		{
			strcpy(fontName, ARG(++argIdx));
		}
		else if (!stricmp(arg, "-tga"))
		{
			const char *parms = ARG(++argIdx);
			if (strstr(parms, "packed"))
				compressTga = true;
			if (strstr(parms, "8bit"))
				monochrome = true;
		}
		else if (!stricmp(arg, "-cpp"))
		{
			cppFormat = true;
			const char *parms = ARG(++argIdx);
			if (parms[0] != '-' && parms[0] != 0)
			{
				if (strstr(parms, "packed"))
					packedCpp = true;
			}
		}
	}

	// Check validity...
	if (firstChar >= lastChar)
	{
		Error("ERROR: First char (%d) is >= last char (%d)\n", firstChar, lastChar);
	}
	if (fontSize <= 0)
	{
		Error("ERROR: Font size (%d) is <= 0\n", fontSize);
	}
	if (!fontName[0])
	{
		Error("ERROR: No font name specified (do it with -name)\n");
	}
	if (!fontWinName[0])
	{
		Error("ERROR: No windows font name specified (do it with -winname)\n");
	}
}

void FontGen::Usage()
{
	printf(
		"Usage: fontgen -name <name> -winname \"<name>\" [options]\n"
		"  Required:\n"
		"    -name <name>      - The font name that the game will use\n"
		"    -winname \"<name>\" - The Windows name of the font (in quotes)\n"
		"  Optional:\n"
		"    -size <number>    - The size of the font in pixels (default 16)\n"
		"    -first <number>   - First character number to use. (default 32 (spacebar) )\n"
		"    -last <number>    - Last character number to use (default 126)\n"
		"    -styles <string>  - Styles to apply. <string> is comma separated list of:\n"
		"                        bold, underline, strikeout, italic, ansialias\n"
		"                        Example: For a bold underlined string, use\n"
		"                        \"-styles bold,underline\" (No spaces between commas)\n"
		"    -tga <type>       - TGA output format, comma separated list of:\n"
		"                        8 (8-bit texture), 24, 32, packed\n"
		"    -cpp [type]       - write as C header file instead of binaries\n"
		"                        <type>: \"packed\" = 4bpp, RLE-compressed\n"
	);
}

FontGen::~FontGen()
{
	if (dc)
	{
		SelectObject(dc, GetStockObject(DEFAULT_GUI_FONT));
		if (oldbitmap)
			SelectObject(dc, oldbitmap);
	}

	if (hf)
		DeleteObject(hf);
	if (dib)
		DeleteObject(dib);
	if (dc)
		DeleteDC(dc);
}

void FontGen::PrepareFont()
{
	hf = CreateFont(fontSize, 0, 0, 0,
		isBold      ? FW_BOLD : FW_NORMAL,
		isItalic    ? TRUE    : FALSE,
		isUnderline ? TRUE    : FALSE,
		isStrike    ? TRUE    : FALSE,
		DEFAULT_CHARSET,
		OUT_DEFAULT_PRECIS,//OUT_TT_PRECIS,
		CLIP_DEFAULT_PRECIS,
		isAntialiased ? PROOF_QUALITY : NONANTIALIASED_QUALITY,
		DEFAULT_PITCH | FF_DONTCARE,
		fontWinName
	);

	if (!hf) Error("ERROR: Could not create font object\n");
}

void FontGen::PrepareBitmap()
{
	BITMAPINFO bmi;
	BITMAPINFOHEADER &head = bmi.bmiHeader;

	memset(&bmi, 0, sizeof(bmi));
	head.biSize        = sizeof(BITMAPINFOHEADER);
	head.biWidth       = TEX_WIDTH;
	head.biHeight      = TEX_HEIGHT;
	head.biPlanes      = 1;
	head.biBitCount    = 24;
	head.biCompression = BI_RGB;

	dib = CreateDIBSection(dc, &bmi, DIB_RGB_COLORS, (void **)&bmbits, NULL, 0);
	if (!dib) Error("ERROR: Could not create DIB section.\n");

	dc = CreateCompatibleDC(NULL);
	if (!dc) Error("ERROR: Couldn't create DC\n");
	oldbitmap = (HBITMAP)SelectObject(dc, dib);

	SetTextColor(dc, RGB(0xFF, 0xFF, 0xFF));
	SetBkColor(dc, 0);
	SelectObject(dc, hf);

	SIZE theSize;
	GetTextExtentPoint32( dc, " ", 1, &theSize);
	fontSize = theSize.cy;

	memset(bmbits, 0, TEX_WIDTH * TEX_HEIGHT * 3);
}

void FontGen::PrepareHeader()
{
	int i;

	memset(&font_def, 0, sizeof(font_def));
	for (i = 0; i < 256; i++)
		font_def.indirection[i] = -1;
	for (i = firstChar; i <= lastChar; i++)
		font_def.indirection[i] = i-firstChar;

	strcpy(font_def.name, fontName);
	font_def.height = float(fontSize);
}

void FontGen::DrawFont()
{
	int currentLine = 0;
	int currentColumn = 0;
	int maxHeight = 0;
	int numChars = lastChar - firstChar + 1;

	for (int i = 0; i < numChars; i++)
	{
		char s[2];
		SIZE size;
		RECT fmtRect;

		s[0] = i + firstChar;
		s[1] = 0;

		if (!GetTextExtentPoint32(dc, s, 1, &size))
		{
			Error("ERROR: Couldn't draw character #%d (%c) - Bye!\n", (int) s[0], s[0]);
		}

		// These below are padded for bilinear filtering
		size.cx += 1;
		size.cy += 1;

		if (currentColumn + size.cx > TEX_WIDTH)
		{
			currentColumn = 0;
			currentLine += maxHeight;
			maxHeight = 0;
			i--;
			continue;
		}

		if (currentLine + size.cy > TEX_HEIGHT)
		{
			// FIXME allow multiple textures
			Error("ERROR: Font size exceeds %dx%d texture size.\n", TEX_WIDTH, TEX_HEIGHT);
		}

		maxHeight = max(maxHeight, size.cy);

		fmtRect.left   = currentColumn;
		fmtRect.top    = currentLine;
		fmtRect.right  = fmtRect.left + size.cx - 1;
		fmtRect.bottom = fmtRect.top + size.cy - 1;

		if (!DrawText(dc, s, 1, &fmtRect, DT_LEFT | DT_NOPREFIX | DT_TOP))
			Error("ERROR: DrawText failed on char #%d (%c) - Bye!\n", (int) s[0], s[0]);

		font_def.locations[i].pos[0]  = (float) currentColumn;
		font_def.locations[i].pos[1]  = (float) currentLine;
		font_def.locations[i].size[0] = (float) size.cx-1;
		font_def.locations[i].size[1] = (float) size.cy-1;
		if (!i)
		{
			font_def.charWidth  = size.cx;
			font_def.charHeight = size.cy;
		}
		else
		{
			if (size.cx != font_def.charWidth || size.cy != font_def.charHeight)
			{
#if !ALLOW_NON_MONOSPACE_FONT
				Error("Non-monospace font!");
#else
				font_def.charWidth = max(size.cx, font_def.charWidth);
				font_def.charHeight = max(size.cx, font_def.charHeight);
#endif
			}
		}

		currentColumn += size.cx;
	}

	// This sees if it can eliminate things
	texture_height = TEX_HEIGHT;
	currentLine += maxHeight;

	font_def.aspectRatio = 1.f;

	// compute best texture height (will be a power of 2)
	while ((texture_height / 2) > currentLine)
	{
		texture_height /= 2;
		font_def.aspectRatio *= 2.f;
	}

	// I don't know how necessary this is, but it can't hurt.
	GdiFlush();
}


#define TGA_ORIGIN_MASK		0x30
#define TGA_BOTLEFT			0x00
#define TGA_BOTRIGHT		0x10					// unused
#define TGA_TOPLEFT			0x20
#define TGA_TOPRIGHT		0x30					// unused

#if _MSC_VER
#pragma pack(push, 1)
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

bool WriteTGA(const char *name, byte *pic, int width, int height, bool canCompress, bool monochrome)
{
	int		i;

	FILE *f = fopen(name, "wb");
	if (!f)
		Error("WriteTGA(%s): cannot create file\n", name);

	byte *src;
	int size = width * height;

	byte *packed = new byte [width * height * 3];
	byte *threshold = packed + width * height * 3 - 16;		// threshold for "dst"

	src = pic;
	byte *dst = packed;
	int column = 0;
	byte *flag = NULL;
	bool rle = false;

	bool useCompression = true;
	if (!canCompress)
	{
		useCompression = false;
		goto skip_compress;
	}

	for (i = 0; i < size; i++)
	{
		if (dst >= threshold)								// when compressed is too large, same uncompressed
		{
			useCompression = false;
			break;
		}

		byte b = *src++;
		byte g = *src++;
		byte r = *src++;
		if (monochrome) b = (b + g + r) / 3;				// b = B/W color

		bool colorsSame = false;
		if (column < width - 1)
		{
			// not on screen edge; NOTE: when i == size-1, col==width-1
			if (!monochrome)
			{
				colorsSame = b == src[0] && g == src[1] && r == src[2];
			}
			else
			{
				int b2 = (src[0] + src[1] + src[2]) / 3;
				colorsSame = (b == b2);
			}
		}

		if (colorsSame &&									// next byte will be the same
			!(rle && flag && *flag == 254))					// flag overflow
		{
			if (!rle || !flag)
			{
				// starting new RLE sequence
				flag = dst++;
				*flag = 128 - 1;							// will be incremented below
				*dst++ = b;
				if (!monochrome)
				{
					*dst++ = g; *dst++ = r;
				}
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
				// copy texel
				*dst++ = b;
				if (!monochrome)
				{
					*dst++ = g; *dst++ = r;
				}
				(*flag)++;
				if (*flag == 127) flag = NULL;				// check for overflow
			}
			rle = false;
		}

		if (++column == width) column = 0;
	}

skip_compress:
	// write header
	tgaHdr_t header;
	memset(&header, 0, sizeof(header));
	header.width  = width;
	header.height = height;

	if (monochrome)
	{
		// debug: write black/white image
		header.pixel_size = 8;
		header.image_type = useCompression ? 11 : 3;
		fwrite(&header, 1, sizeof(header), f);
		if (useCompression)
		{
			printf("WriteTGA: packed %d -> %d\n", size * 3, dst - packed);
			fwrite(packed, 1, dst - packed, f);
		}
		else
		{
			for (i = 0; i < width * height; i++, pic += 3)
			{
				int c = (pic[0]+pic[1]+pic[2]) / 3;
				fwrite(&c, 1, 1, f);
			}
		}
	}
	else
	{
		header.pixel_size = 24;
		header.image_type = useCompression ? 10 : 2;	// RLE or uncompressed
		fwrite(&header, 1, sizeof(header), f);

		if (useCompression)
		{
			printf("WriteTGA: packed %d -> %d\n", size * 3, dst - packed);
			fwrite(packed, 1, dst - packed, f);
		}
		else
		{
			printf("WriteTGA: save uncompressed, %d bytes\n", size * 3);
			fwrite(pic, 1, size * 3, f);
		}
	}

	fclose(f);
	delete packed;
	return true;
}


void FontGen::SaveTarga()
{
	char fileName[256];
	sprintf(fileName, "%s.tga", font_def.name);

	int start_line = TEX_HEIGHT - texture_height;
	WriteTGA(fileName, bmbits + TEX_WIDTH * 3 * start_line, TEX_WIDTH, texture_height, compressTga, monochrome);
}


void FontGen::SaveFontdef()
{
#if 0

	FILE *f;
	char fileName[255];
	int i;

	sprintf ( fileName, "%s.RitualFont", font_def.name );

	if (!(f = fopen(fileName, "wt")))
		Error("ERROR: Could not write %s\n", fileName);

	fprintf( f, "RitFont\n" );
	fprintf( f, "height %f\n", font_def.height );
	fprintf( f, "aspect %.8f\n", font_def.aspectRatio );
	fprintf( f, "indirections { \n" );
	for ( i=0; i < 256; i++ )
	{
		fprintf ( f, "%d ", font_def.indirection[i] );
		if ( ( i + 1 ) % 10 == 0 )
			fprintf ( f, "\n" );
	}

	fprintf( f, "\n}\n" );
	fprintf( f, "locations {\n" );
	for ( i=0; i < 256; i++ )
	{
		fprintf( f, "{ %g %g %g %g } ", font_def.locations[i].pos[0], font_def.locations[i].pos[1],
			font_def.locations[i].size[0], font_def.locations[i].size[1] );
		if ( ( i + 1 ) % 3 == 0 )
			fprintf( f, "\n" );
	}

	fprintf( f, "\n}\n" );
	fclose(f);

#else

	int i;

	char fileName[255];
	sprintf(fileName, "%s.font", font_def.name);

	FILE *f = fopen(fileName, "wt");
	if (!f) Error("ERROR: Could not write %s\n", fileName);

	fprintf(f,
		"// Generated from %s %d\n\n"
		"bitmap \"%s\"\n"
		"firstChar %d\n"
		"charSize %d %d\n",
		fontWinName, fontSize,
		font_def.name,
		firstChar,
		font_def.charWidth,
		font_def.charHeight
	);

	fclose(f);

#endif
}


static void output(FILE *f, byte c)
{
	static int out_len = 10000;
	if (out_len >= 17)
	{
		out_len = 0;
		fprintf(f, "\n\t");
	}
	fprintf(f, "0x%02X,", c);
	out_len++;
}


void FontGen::SaveCpp()
{
	char fileName[255];
	sprintf(fileName, "Font%s.h", font_def.name);

	FILE *f = fopen(fileName, "wt");
	if (!f) Error("ERROR: Could not write %s\n", fileName);

	fprintf(f,
		"// Generated from %s %d\n\n"
		"#define CHAR_WIDTH        %d\n"
		"#define CHAR_HEIGHT       %d\n"
		"#define FONT_FIRST_CHAR   %d\n"
		"#define TEX_WIDTH         %d\n"
		"#define TEX_HEIGHT        %d\n"
		"\n"
		"byte TEX_DATA[] = {",
		fontWinName, fontSize,
		font_def.charWidth,
		font_def.charHeight,
		firstChar,
		TEX_WIDTH,
		texture_height
	);

	int start_line = TEX_HEIGHT - texture_height;
	byte *src  = bmbits + TEX_WIDTH * 3 * start_line;
	int width  = TEX_WIDTH;
	int height = texture_height;

	// flip texture vertically
	int y1 = 0;
	int y2 = height - 1;
	while (y1 < y2)
	{
		MemSwap(src + y1 * width * 3, src + y2 * width * 3, width * 3);
		y1++;
		y2--;
	}

	int size = width * height;
	int i;

	// convert to monochrome
	byte *mono = new byte[size];
	for (i = 0; i < size; i++)
	{
		byte b = *src++;
		byte g = *src++;
		byte r = *src++;
		mono[i] = (b + g + r) / 3;				// B/W color
	}

	if (packedCpp)
	{
		// make font 4 bits per pixel
		src = mono;
		byte *dst = src;
		for (i = 0; i < size/2; i++)
		{
			byte c1 = *src++;
			byte c2 = *src++;
			// 0->0, 255->15, 255/15 = 17
			*dst++ = (c1 / 17 << 4) | (c2 / 17);
		}
		size /= 2;
	}

	// dump
	src = mono;
	byte *end = src + size;
	while (src < end)
	{
		byte c = *src++;

		if (!packedCpp)
		{
			output(f, c);
			continue;
		}

		// try RLE for zero bytes
		if (c == 0 && src + 1 < end && *src == 0)
		{
			src++;
			int count = 0;	// means 1 zero byte
			while ((src < end) && (count < 127) && (*src == 0))
			{
				src++;
				count++;
			}
			assert(count < 128);
#if DEBUG_CPP
			fprintf(f, "/*null %d*/", count);
#endif
			output(f, count);
			continue;
		}

		// copy non-zero bytes
		byte *src2 = src;
		int count = 0;
		while ((src2 < end) && (count < 127))
		{
			if (src2 + 1 < end && src2[0] == 0 && src2[1] == 0)
				break;		// break on 2 zero bytes only
			src2++;
			count++;
		}
		assert(count < 128);
		output(f, 0x80 | count);
		src--;		// should put previous byte too
#if DEBUG_CPP
		fprintf(f, "/*copy %d*/", count);
#endif
		while (src < src2)
			output(f, *src++);
#if DEBUG_CPP
		fprintf(f, "/*end*/");
#endif
	}

	fprintf(f, "\n};\n\n");

	delete[] mono;

	fclose(f);
}


void FontGen::Go()
{
	// Starts genereting the font stuff
	PrepareFont();
	PrepareBitmap();
	PrepareHeader();
	DrawFont();
	if (!cppFormat)
	{
		SaveTarga();
		SaveFontdef();
	}
	else
	{
		SaveCpp();
	}
}

void main(int argc, char **argv)
{
	if (argc == 1)
	{
		FontGen::Usage();
		return;
	}

	FontGen font(argc, argv);
	font.Go();
	printf("Success!\n");
}
