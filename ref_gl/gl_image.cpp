//??
#define START_PROFILE(name)	\
	{	\
		static const char _name[] = #name;	\
		int	_time = Sys_Milliseconds();
#define END_PROFILE	\
		_time = Sys_Milliseconds() - _time;	\
		if (Cvar_VariableInt("r_profile2")) Com_Printf(S_MAGENTA"%s: %d ms\n", _name, _time);	\
	}

#define DELAYED_RAW_PAINT


#include "gl_local.h"
#include "gl_image.h"
#include "gl_math.h"

image_t	*gl_defaultImage;
//image_t	*gl_whiteImage;		//?? unneeded: can use "image = NULL" for this (CHECK THIS WITH MTEX!)
image_t	*gl_identityLightImage;
image_t	*gl_dlightImage;
image_t	*gl_particleImage;
//image_t	*gl_fogImage;
image_t	*gl_videoImage;
image_t	*gl_reflImage;
image_t	*gl_reflImage2;

int 	gl_screenshotFlags;
char	*gl_screenshotName;


/*-----------------------------------------------------------------------------
	Local cvars
-----------------------------------------------------------------------------*/

static cvar_t	*gl_showImages;
static cvar_t	*r_colorMipLevels;	//?? can we implement colorized mips in software? (if no, rename to "gl_")
static cvar_t	*gl_picmip, *gl_roundImagesDown;
static cvar_t	*gl_textureBits;

//??
static GLenum	gl_filter_min = GL_LINEAR_MIPMAP_NEAREST;
static GLenum	gl_filter_max = GL_LINEAR;

#define HASH_BITS		8
#define HASH_SIZE		(1 << HASH_BITS)
#define HASH_MASK		(HASH_SIZE - 1)

#define	MAX_TEXTURES	1024
#define MAX_TEX_SIZE	2048				// max_size = min(MAX_TEX_SIZE,gl_config.maxTextureSize)
#define MAX_VIDEO_SIZE	256

static image_t	imagesArray[MAX_TEXTURES];	// no dynamic allocation (array size < 100K - small enough)
static image_t	*hashTable[HASH_SIZE];
static int		imageCount;

/*
 * gammaTable is used for 2 purposes:
 *  1. when gamma is hardware -- for lightscaling screenshots
 *  2. when gamma is software -- for lightscaling uploadable textures
 */
static byte		gammaTable[256];

// name should be in a lower case
static int ComputeHash (char *name)
{
	int		h;
	char	*s, c;

	h = 0;
	s = name;
	while (c = *s++, c && c != '.')		// exclude extension from hash
		h = (h ^ 0x25) + c;				// good values: 07 (best in 10-bit hash), 25, 61, 7B
	return h & HASH_MASK;
}

//-------------- 8-bit palette -----------------------------------------

static void GetPalette (void)
{
	int		width, height;
	byte	*pic, *pal, *p;

	// get the palette
	LoadPCX ("pics/colormap.pcx", &pic, &pal, &width, &height);
	if (!pal)
		Com_FatalError ("Couldn't load pics/colormap.pcx");

	p = pal;
	for (int i = 0; i < 256; i++, p += 3)
	{
		unsigned v = RGB255(p[0], p[1], p[2]);
		gl_config.tbl_8to32[i] = LittleLong(v);
	}
	gl_config.tbl_8to32[255] &= LittleLong(0x00FFFFFF);		// #255 is transparent (alpha = 0)

	// free image
	appFree (pic);
	appFree (pal);
}


static byte *Convert8to32bit (byte *in, int width, int height, unsigned *palette)
{
	byte	*out;
	unsigned *p;

	guard(Convert8to32bit);
	if (!palette)
		palette = gl_config.tbl_8to32;

	int size = width * height;
	out = (byte*)appMalloc (size * 4);

	p = (unsigned*)out;
	for (int i = 0; i < size; i++)
		*p++ = palette[*in++];

	return out;
	unguard;
}


//----------------------------------------------------------------------

void GL_TextureMode (char *name)	//?? change (not strings; use enum {none,bilinear,trilinear,anysotropic})
{
	static struct {
		char	*name;
		GLenum	minimize, maximize;
	} texModes[] = {
		{"GL_NEAREST", GL_NEAREST, GL_NEAREST},				// box filter, no mipmaps
		{"GL_LINEAR", GL_LINEAR, GL_LINEAR},				// linear filter, no mipmaps
		{"GL_NEAREST_MIPMAP_NEAREST", GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST},	// no (box) filter
		{"GL_LINEAR_MIPMAP_NEAREST", GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR},		// bilinear filter
		{"GL_NEAREST_MIPMAP_LINEAR", GL_NEAREST_MIPMAP_LINEAR, GL_NEAREST},
		{"GL_LINEAR_MIPMAP_LINEAR", GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR},		// trilinear filter
		// aliases
		{"none", GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST},
		{"bilinear", GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR},
		{"trilinear", GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR}
	};

	GL_SetMultitexture (1);
	for (int i = 0; i < ARRAY_COUNT(texModes); i++)
	{
		if (!stricmp (texModes[i].name, name))
		{
			image_t *img;

			gl_filter_min = texModes[i].minimize;
			gl_filter_max = texModes[i].maximize;

			int		j;
			// change all the existing mipmap texture objects
			for (j = 0, img = imagesArray; j < MAX_TEXTURES; j++, img++)
			{
				if (!img->name[0]) continue;	// free slot
				if (!(img->flags & IMAGE_MIPMAP)) continue;
				GL_BindForce (img);
				glTexParameteri (img->target, GL_TEXTURE_MIN_FILTER, gl_filter_min);
				glTexParameteri (img->target, GL_TEXTURE_MAG_FILTER, gl_filter_max);
			}
			return;
		}
	}

	Com_WPrintf ("R_TextureMode: bad filter name\n");
}


//----------------------------------------------------------------------

static void ResampleTexture (unsigned *in, int inwidth, int inheight, unsigned *out, int outwidth, int outheight)
{
	int			i, j;
	unsigned	*inrow, *inrow2;
	unsigned	frac, fracstep;
	unsigned	p1[MAX_TEX_SIZE], p2[MAX_TEX_SIZE];
	float		f, f1, f2;

	fracstep = (inwidth << 16) / outwidth;

	frac = fracstep >> 2;
	for (i = 0; i < outwidth; i++)
	{
		p1[i] = 4 * (frac >> 16);
		frac += fracstep;
	}
	frac = 3 * (fracstep >> 2);
	for (i = 0; i < outwidth; i++)
	{
		p2[i] = 4 * (frac >> 16);
		frac += fracstep;
	}

	f = (float)inheight / outheight;
	for (i = 0, f1 = 0.25f * f, f2 = 0.75f * f; i < outheight; i++, out += outwidth, f1 += f, f2 += f)
	{
		inrow = in + inwidth * appFloor (f1);
		inrow2 = in + inwidth * appFloor (f2);
		for (j = 0; j < outwidth; j++)
		{
			int		n, r, g, b, a;
			byte	*pix;

			n = r = g = b = a = 0;
#define PROCESS_PIXEL(row,col)	\
	pix = (byte *)row + col[j];		\
	if (pix[3])	\
	{			\
		n++;	\
		r += *pix++; g += *pix++; b += *pix++; a += *pix;	\
	}
			PROCESS_PIXEL(inrow, p1);
			PROCESS_PIXEL(inrow, p2);
			PROCESS_PIXEL(inrow2, p1);
			PROCESS_PIXEL(inrow2, p2);
#undef PROCESS_PIXEL

			switch (n)		// if use generic version, with "x /= n", will be 50% slower ...
			{
			// case 1 - divide by 1 - do nothing
			case 2:
				r >>= 1; g >>= 1; b >>= 1; a >>= 1;
				break;
			case 3:
				r /= 3; g /= 3; b /= 3; a /= 3;
				break;
			case 4:
				r >>= 2; g >>= 2; b >>= 2; a >>= 2;
				break;
			case 0:
				r = g = b = 0;
				break;
			}

			((byte *)(out+j))[0] = r;
			((byte *)(out+j))[1] = g;
			((byte *)(out+j))[2] = b;
			((byte *)(out+j))[3] = a;
		}
	}
}


//#define INT_SATURATE		// should be disabled
#define TBL_SATURATE		// should be enabled

static void LightScaleTexture (unsigned *pic, int width, int height)
{
	float	sat;
	int		i, c;
	byte	*p;
#ifdef INT_SATURATE
	int		isat = appRound (r_saturation->value * 255);
#endif
#ifdef TBL_SATURATE
	static float lastSat = 1.0f;		// when sat. == 1 -- pic will not be changed
	static int sat1[256 * 3];
	static int sat2[256];
	float	tmp;

	if (r_saturation->value != lastSat)
	{
		/*
			l = (r+g+b)/3
			c' = l + (c-l)*s = l*(1-s) + c*s = sat1(r,g,b) + sat2(c)
			sat1(r,g,b) = (r+g+b)/3 * (1-s)
			sat2(c) = c*s
		*/
		lastSat = r_saturation->value;
		Com_DPrintf ("rebuilding saturation table for %g\n", lastSat);
		tmp = (1.0f - lastSat) * 2 / 3;				// modulated by 2
		for (i = 0; i < ARRAY_COUNT(sat1); i++)
			sat1[i] = appRound (i * tmp);
		for (i = 0; i < ARRAY_COUNT(sat2); i++)
			sat2[i] = appRound (i * lastSat * 2);	// modulated by 2
	}
#endif

	sat = r_saturation->value;
	c = width * height;

	if (sat != 1.0)
	{
		p = (byte *)pic;
		for (i = 0; i < c; i++, p+=4)
		{
#ifdef TBL_SATURATE
			int		r, g, b;
			int		s1;

			r = p[0]; g = p[1]; b = p[2];
			s1 = sat1[r + g + b];
			r = (s1 + sat2[r]) >> 1;
			p[0] = bound(r,0,255);
			g = (s1 + sat2[g]) >> 1;
			p[1] = bound(g,0,255);
			b = (s1 + sat2[b]) >> 1;
			p[2] = bound(b,0,255);
#else
#	ifndef INT_SATURATE
			float	r, g, b, light;

			// get color
			r = p[0];  g = p[1];  b = p[2];
			// change saturation
			light = (r + g + b) / 3.0f;
			SATURATE(r,light,sat);
			SATURATE(g,light,sat);
			SATURATE(b,light,sat);
			// put color
			p[0] = appRound (r);  p[1] = appRound (g);  p[2] = appRound (b);
#	else
			int		r, g, b, light;

			r = p[0]; g = p[1]; b = p[2];
			light = (r + g + b) / 3;
			r = light + (((r - light) * isat) >> 8);
			g = light + (((g - light) * isat) >> 8);
			b = light + (((b - light) * isat) >> 8);
			p[0] = bound(r,0,255);
			p[1] = bound(g,0,255);
			p[2] = bound(b,0,255);
#	endif
#endif
		}
	}

	if (gl_config.deviceSupportsGamma)
		return;

	p = (byte *)pic;
	for (i = 0; i < c; i++, p+=4)
	{
		p[0] = gammaTable[p[0]];
		p[1] = gammaTable[p[1]];
		p[2] = gammaTable[p[2]];
	}
}


// Scale lightmap; additional scaling data encoded in alpha-channel (0 - don't scale; 1 - /=2; -1==255 - *=2)
static void LightScaleLightmap (unsigned *pic, int width, int height)
{
	byte	*p;
	int		i, c, shift;

	shift = gl_config.overbright;
	if (!gl_config.doubleModulateLM)
		shift--;

	p = (byte *)pic;
	c = width * height;
	for (i = 0; i < c; i++, p += 4)
	{
		int		r, g, b, sh;

		sh = shift + (signed char)p[3];		// usa alpha as additional shift info
		if (sh > 0)
		{
			p[0] >>= sh;
			p[1] >>= sh;
			p[2] >>= sh;
		}
		else
		{
			sh = -sh;

			r = p[0] << sh;
			g = p[1] << sh;
			b = p[2] << sh;
			NORMALIZE_COLOR255(r, g, b);
			p[0] = r;
			p[1] = g;
			p[2] = b;
		}
		p[3] = 255;		// alpha
	}
}


static void MipMap (byte *in, int width, int height)
{
	int		i, j;
	byte	*out;

	width <<=2;
	height >>= 1;
	out = in;
	for (i = 0; i < height; i++, in += width)
	{
		for (j = 0; j < width; j += 8, out += 4, in += 8)
		{
			int		r, g, b, a, am, n;

			r = g = b = a = am = n = 0;
#define PROCESS_PIXEL(idx)	\
	if (in[idx+3])	\
	{	\
		n++;	\
		r += in[idx]; g += in[idx+1]; b += in[idx+2]; a += in[idx+3];	\
		am = max(am, in[idx+3]);	\
	}
			PROCESS_PIXEL(0);
			PROCESS_PIXEL(4);
			PROCESS_PIXEL(width);
			PROCESS_PIXEL(width+4);
#undef PROCESS_PIXEL
			switch (n)
			{
			// case 1 - divide by 1 - do nothing
			case 2:
				r >>= 1; g >>= 1; b >>= 1; a >>= 1;
				break;
			case 3:
				r /= 3; g /= 3; b /= 3; a /= 3;
				break;
			case 4:
				r >>= 2; g >>= 2; b >>= 2; a >>= 2;
				break;
			case 0:
				r = g = b = 0;
				break;
			}
			out[0] = r; out[1] = g; out[2] = b;
			// generate alpha-channel for mipmaps (don't let it be transparent)
			// dest alpha = (MAX(a[0]..a[3]) + AVG(a[0]..a[3])) / 2
			// if alpha = 255 or 0 (for all 4 points) -- it will holds its value
			out[3] = (am + a) / 2;
		}
	}
}


static void GetImageDimensions (int width, int height, int *scaledWidth, int *scaledHeight, bool picmip)
{
	int		sw, sh, maxSize;

	for (sw = 1; sw < width; sw <<= 1) ;
	for (sh = 1; sh < height; sh <<= 1) ;

	if (gl_roundImagesDown->integer)
	{	// scale down only when new image dimension is larger than 64 and
		// larger than 4/3 of original image dimension
		if (sw > 64 && sw > (width * 4 / 3)) sw >>= 1;
		if (sh > 64 && sh > (height * 4 / 3)) sh >>= 1;
	}
	if (picmip)
	{
		sw >>= gl_picmip->integer;
		sh >>= gl_picmip->integer;
	}

	maxSize = min(gl_config.maxTextureSize, MAX_TEX_SIZE);
	while (sw > maxSize) sw >>= 1;
	while (sh > maxSize) sh >>= 1;

	if (sw < 1) sw = 1;
	if (sh < 1) sh = 1;

	*scaledWidth = sw;
	*scaledHeight = sh;
}


static void ComputeImageColor (void *pic, int width, int height, color_t *color)
{
	int		c[4], i, count;
	byte	*p;

	count = width * height;
	c[0] = c[1] = c[2] = c[3] = 0;
	for (i = 0, p = (byte*)pic; i < count; i++, p += 4)
	{
		c[0] += p[0];
		c[1] += p[1];
		c[2] += p[2];
		c[3] += p[3];
	}
	color->c[0] = c[0] / count;
	color->c[1] = c[1] / count;
	color->c[2] = c[2] / count;
	color->c[3] = c[3] / count;
}


// We need to pass "flags" because image->flags is masked with IMAGE_FLAGMASK
static void Upload (void *pic, int flags, image_t *image)
{
	int		scaledWidth, scaledHeight;
	int		format, size;
	unsigned *scaledPic;

	LOG_STRING(va("// Upload(%s)\n", image->name));

	/*------------ Eliminate alpha-channel when needed---------------*/
	if (flags & IMAGE_NOALPHA)
	{
		int		i;
		byte	*p;
		bool	alphaRemoved;

		alphaRemoved = false;
		size = image->width * image->height;
		p = (byte*)pic + 3;
		for (i = 0; i < size; i++, p += 4)
		{
			if (*p != 255)
			{
				alphaRemoved = true;
				*p = 255;
			}
		}
		if (!alphaRemoved)
			image->flags &= ~IMAGE_NOALPHA;		// original image have no alpha-channel -- reset NOALPHA flag
	}

	/*----- Calculate internal dimensions of the new texture --------*/
	if (image->target != GL_TEXTURE_RECTANGLE_NV)
		GetImageDimensions (image->width, image->height, &scaledWidth, &scaledHeight, (image->flags & IMAGE_PICMIP) != 0);
	else
	{
		scaledWidth = image->width;
		scaledHeight = image->height;
	}

	image->internalWidth = scaledWidth;
	image->internalHeight = scaledHeight;

START_PROFILE(..up::scale)
	/*---------------- Resample/lightscale texture ------------------*/
	size = scaledWidth * scaledHeight * 4;
	scaledPic = (unsigned*)appMalloc (size);
	if (image->width != scaledWidth || image->height != scaledHeight)
		ResampleTexture ((unsigned*)pic, image->width, image->height, scaledPic, scaledWidth, scaledHeight);
	else
		memcpy (scaledPic, pic, size);
END_PROFILE

START_PROFILE(..up::lscale)
	if (!(flags & IMAGE_LIGHTMAP))
		LightScaleTexture (scaledPic, scaledWidth, scaledHeight);
	else
		LightScaleLightmap (scaledPic, scaledWidth, scaledHeight);			// lightmap overbright
END_PROFILE

	/*------------- Determine texture format to upload --------------*/
	if (flags & IMAGE_LIGHTMAP)
		format = 3;
	else if (flags & IMAGE_TRUECOLOR)
		format = GL_RGBA8;
	else
	{
		int		alpha, numAlpha0, i, n;
		byte	*scan, a;

		// Check for alpha channel in image
		alpha = numAlpha0 = 0;
		n = scaledWidth * scaledHeight;
		scan = (byte *)scaledPic + 3;
		for (i = 0; i < n; i++, scan += 4)
		{
			a = *scan;
			if (a == 255) continue; // opaque
			if (a == 0)
			{
				alpha = 1;	// have alpha == 0 (1 bit alpha)
				numAlpha0++;
				continue;
			}
			alpha = 2;		// have alpha == [1..254] (8 bit alpha)
			break;
		}

		// select texture format
		format = 0;
		if (alpha == 1 && numAlpha0 > (n / 100) && (flags & IMAGE_MIPMAP))
			alpha = 2;		// mipmaps will create middle alpha's
		image->alphaType = alpha;

		// select compressed format
		if (alpha)
		{
			if (alpha == 1)
				format = gl_config.formatAlpha1;
			if (!format)
				format = gl_config.formatAlpha;
		}
		else
			format = gl_config.formatSolid;

		// no suitable compressed format
		if (!format || flags & IMAGE_NOCOMPRESS || image->target == GL_TEXTURE_RECTANGLE_NV)	// ATI will crash when uploading rect as compressed
		{
			switch (gl_textureBits->integer)
			{
			case 16:
				if (alpha == 1)
					format = GL_RGB5_A1;
				else if (alpha == 2)
					format = GL_RGBA4;
				else
					format = GL_RGB5;
				break;
			case 32:
				format = (alpha ? GL_RGBA8 : GL_RGB8);
				break;
			default:	// 0
				format = (alpha ? 4 : 3);
			}
		}
	}

	image->internalFormat = format;

	/*------------------ Upload the image ---------------------------*/
START_PROFILE(..up::gl)
	glTexImage2D (image->target, 0, format, scaledWidth, scaledHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaledPic);
END_PROFILE
	if (!(flags & IMAGE_MIPMAP))
	{
		glTexParameteri (image->target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri (image->target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}
	else
	{
		int		miplevel;

START_PROFILE(..up::mip)
		miplevel = 0;
		while (scaledWidth > 1 || scaledHeight > 1)
		{
			MipMap ((byte *) scaledPic, scaledWidth, scaledHeight);
			miplevel++;
			scaledWidth >>= 1;
			scaledHeight >>= 1;
			if (scaledWidth < 1) scaledWidth = 1;
			if (scaledHeight < 1) scaledHeight = 1;

			// show mipmap levels as colors
			size = scaledWidth * scaledHeight;
			if (r_colorMipLevels->integer)
			{
				int		i;
				byte	*p;
				color_t	c;

				// here miplevel >= 1
				c.rgba = 0;
				c.c[(miplevel - 1) % 3] = 255;
				p = (byte *) scaledPic;
				for (i = 0; i < size; i++, p += 4)
				{
					p[0] = (c.c[0] + p[0]) / 4;
					p[1] = (c.c[1] + p[1]) / 4;
					p[2] = (c.c[2] + p[2]) / 4;
				}
			}
			glTexImage2D (image->target, miplevel, format, scaledWidth, scaledHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaledPic);
		}
		glTexParameteri (image->target, GL_TEXTURE_MIN_FILTER, gl_filter_min);
		glTexParameteri (image->target, GL_TEXTURE_MAG_FILTER, gl_filter_max);
END_PROFILE
	}

	appFree (scaledPic);
}


image_t *GL_CreateImage (const char *name, void *pic, int width, int height, int flags)
{
	char	name2[MAX_QPATH];
	int		texnum;
	image_t	*image, *freeSlot;
	bool	reuse;

	guard(GL_CreateImage);
	if (!name[0]) Com_FatalError ("R_CreateImage: null name");

//	Com_Printf ("CreateImage(%s)\n", name);//!!!!

//	if (strlen (name) >= MAX_QPATH)
//		Com_FatalError ("R_CreateImage: name \"%s\" is too long", name);
	Q_CopyFilename (name2, name, sizeof(name2));

	// find image with the same name
	reuse = false;
	freeSlot = NULL;
	for (texnum = 0, image = &imagesArray[0]; texnum < MAX_TEXTURES; texnum++, image++)
	{
		if (!image->name[0])
		{
			if (!freeSlot) freeSlot = image;
			continue;
		}
		if (!strcmp (name2, image->name) && image->flags == (flags & IMAGE_FLAGMASK))
		{
			reuse = true;
			break;
		}
	}

	if (!reuse)
	{
		if (!freeSlot)
		{
			Com_WPrintf ("R_CreateImage(%s): MAX_TEXTURES hit\n", name);
			return gl_defaultImage;
		}
		// not found - allocate free image slot
		image = freeSlot;
		imageCount++;
	}

	// setup image_t fields
	strcpy (image->name, name2);
	image->width = width;
	image->height = height;
	image->flags = flags & IMAGE_FLAGMASK;
	image->target = GL_TEXTURE_2D;

	// choose TEXTURE_RECTANGLE target when possible
	if ((flags & (IMAGE_CLAMP|IMAGE_MIPMAP|IMAGE_PICMIP)) == IMAGE_CLAMP &&	// !wrap, !mipmap, !picmip
		width  <= gl_config.maxRectTextureSize	&&				// suitable size (when extensions is disabled, maxRectTextureSize==0)
		height <= gl_config.maxRectTextureSize	&&
		(width & (width-1) | height & (height-1)))				// non power-of-2 dimensions
		image->target = GL_TEXTURE_RECTANGLE_NV;

	ComputeImageColor (pic, width, height, &image->color);

	if (gl_renderingEnabled)
	{
		GLenum	repMode;

		// upload image
		GL_SetMultitexture (1);
		GL_BindForce (image);
		Upload (pic, flags, image);

		repMode = flags & IMAGE_CLAMP ? GL_CLAMP : GL_REPEAT;
		glTexParameteri (image->target, GL_TEXTURE_WRAP_S, repMode);
		glTexParameteri (image->target, GL_TEXTURE_WRAP_T, repMode);
	}
	else
	{
		int		size;

		// save image for later upload
		if (image->pic)
			appFree (image->pic);
		size = width * height * 4;
		image->pic = (byte*)appMalloc (size);
		image->internalFormat = 0;
		memcpy (image->pic, pic, size);
	}

	if (!reuse)
	{
		int		hash;

		// insert image into a hash table
		hash = ComputeHash (name2);
		image->hashNext = hashTable[hash];
		hashTable[hash] = image;
	}

	return image;

	unguardf(("%s", name));
}


/*-------------------- Video support ----------------------*/


static unsigned rawPalette[256];

void GL_SetRawPalette (const byte *palette)
{
	if (palette)
	{
		int		i;
		byte	*src, *dst;

		src = (byte*) palette;
		dst = (byte*) rawPalette;
		for (i = 0; i < 256; i++)
		{
			*dst++ = *src++;
			*dst++ = *src++;
			*dst++ = *src++;
			*dst++ = 255;
		}
	}
}


void GL_DrawStretchRaw8 (int x, int y, int w, int h, int width, int height, byte *pic)
{
	int		scaledWidth, scaledHeight, i;
	unsigned scaledPic[MAX_VIDEO_SIZE*MAX_VIDEO_SIZE];
	image_t	*image;
	float	hScale;

	guard(GL_DrawStretchRaw8);

	if (GL_SUPPORT(QGL_NV_TEXTURE_RECTANGLE))	//?? check for max [rect] texture size
	{
		// convert 8->32 bit
		byte *pic32 = Convert8to32bit (pic, width, height, rawPalette);
		// upload texture
		image = gl_videoImage;		//?? to allow multiple videos in a screen, should use gl_videoImage[some_number]
		if (image->target != GL_TEXTURE_RECTANGLE_NV)
			Com_DropError ("bad target");
		GL_SetMultitexture (1);
		GL_BindForce (image);

		image->internalFormat = 3;
		if (image->internalWidth == width && image->internalHeight == height)
		{	// TexSubImage()
			glTexSubImage2D (image->target, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pic32);
		}
		else
		{	// TexImage()
			image->internalWidth = width;
			image->internalHeight = height;
			glTexImage2D (image->target, 0, image->internalFormat, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pic32);
		}
		// draw
#ifndef DELAYED_RAW_PAINT
//		GL_BackEnd ();		// need to perform begin_frame in a case of gl_clear!=0; or - enqueue draw command into backend command list
		GL_Set2DMode ();
		glColor3f (gl_config.identityLightValue_f, gl_config.identityLightValue_f, gl_config.identityLightValue_f);
		glBegin (GL_QUADS);
		glTexCoord2f (0.5f, 0.5f);
		glVertex2f (x, y);
		glTexCoord2f (width - 0.5f, 0.5f);
		glVertex2f (x + w, y);
		glTexCoord2f (width - 0.5f, height - 0.5f);
		glVertex2f (x + w, y + h);
		glTexCoord2f (0.5f, height - 0.5f);
		glVertex2f (x, y + h);
		glEnd ();
#else
		gl_videoShader->width = width;
		gl_videoShader->height = height;
		GL_DrawStretchPic (gl_videoShader, x, y, w, h, 0, 0, 1, 1, RGB(1,1,1));
#endif
		// free converted pic
		appFree (pic32);
		return;
	}

	/* we can do uploading pic without scaling using TexSubImage onto a large texture and using
	 * (0..width/large_width) - (0..height/large_height) coords when drawing ?? (but needs palette conversion anyway)
	 */
	GetImageDimensions (width, height, &scaledWidth, &scaledHeight, false);
	if (scaledWidth > MAX_VIDEO_SIZE) scaledWidth = MAX_VIDEO_SIZE;
	if (scaledHeight > MAX_VIDEO_SIZE) scaledHeight = MAX_VIDEO_SIZE;

	/*--- fast resampling with conversion 8->32 bit ---*/
	hScale = (float) height / scaledHeight;
	for (i = 0; i < scaledHeight; i++)
	{
		int		frac, fracStep, row, j;
		byte	*src;
		unsigned *dst;

		row = appFloor (hScale * i);
		if (row > height) break;

		src = &pic[width * row];
		dst = &scaledPic[scaledWidth * i];
		fracStep = (width << 16) / scaledWidth;
		frac = fracStep >> 1;
		for (j = 0; j < scaledWidth; j++)
		{
			*dst++ = rawPalette[src[frac >> 16]];
			frac += fracStep;
		}
	}

	/*-------------------- upload ---------------------*/
	image = gl_videoImage;		//?? to allow multiple videos in a screen, should use gl_videoImage[some_number]
	GL_SetMultitexture (1);
	GL_BindForce (image);

	image->internalFormat = 3;
	if (image->internalWidth == scaledWidth && image->internalHeight == scaledHeight)
	{	// TexSubImage()
		glTexSubImage2D (image->target, 0, 0, 0, scaledWidth, scaledHeight, GL_RGBA, GL_UNSIGNED_BYTE, scaledPic);
	}
	else
	{	// TexImage()
		image->internalWidth = scaledWidth;
		image->internalHeight = scaledHeight;
		glTexImage2D (image->target, 0, image->internalFormat, scaledWidth, scaledHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaledPic);
	}
	// set texture parameters
	glTexParameteri (image->target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri (image->target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	gl_speeds.numUploads++;

	width = scaledWidth;	//!!!
	height = scaledHeight;

	/*--------------------- draw ----------------------*/
#ifndef DELAYED_RAW_PAINT
//	GL_BackEnd ();		// need to perform begin_frame in a case of gl_clear!=0; or - enqueue draw command into backend command list
	GL_Set2DMode ();
	glColor3f (gl_config.identityLightValue_f, gl_config.identityLightValue_f, gl_config.identityLightValue_f);
	glBegin (GL_QUADS);
	glTexCoord2f (0.5f / width, 0.5f / height);
	glVertex2f (x, y);
	glTexCoord2f (1.0f - 0.5f / width, 0.5f / height);
	glVertex2f (x + w, y);
	glTexCoord2f (1.0f - 0.5f / width, 1.0f - 0.5f / height);
	glVertex2f (x + w, y + h);
	glTexCoord2f (0.5f / width, 1.0f - 0.5f / height);
	glVertex2f (x, y + h);
	glEnd ();
#else
	gl_videoShader->width = width;
	gl_videoShader->height = height;
	GL_DrawStretchPic (gl_videoShader, x, y, w, h, 0, 0, 1, 1, RGB(1,1,1));
#endif

	unguard;
}


static void GL_FreeImage (image_t *image)	//?? unused function
{
	int		hash;
	image_t	*img;

	if (!image->name[0])
		return;		// already ...
	if (image->flags & IMAGE_SYSTEM)
		return;		// system image - don't free it

	// delete gl texture
	glDeleteTextures (1, &image->texnum);
	if (image->pic)
	{
		appFree (image->pic);
		image->pic = NULL;
	}

	// remove from hash chain
	hash = ComputeHash (image->name);
	img = hashTable[hash];
	if (img == image)	// first in chain
	{
		hashTable[hash] = image->hashNext;
		img = NULL;
	}
	while (img)
	{
		image_t	*next;

		next = img->hashNext;
		if (next == image)
		{
			img->hashNext = image->hashNext;
			break;
		}
		img = next;
	}

	// mark slot as free
	image->name[0] = 0;
	image->hashNext = NULL;
	imageCount--;
}


//-------------------------------------------------------------------


void GL_SetupGamma (void)
{
	unsigned	overbright;
	int			i;
	float		invGamma;

	gl_config.vertexLight = gl_vertexLight->integer != 0;
	gl_config.deviceSupportsGamma = GLimp_HasGamma ();

	overbright = gl_overbright->integer;
	if (!gl_config.fullscreen || !gl_config.deviceSupportsGamma) overbright = 0;

	if (overbright == 2 && gl_config.doubleModulateLM && !gl_config.vertexLight)	// auto
		overbright = 0;

	if (overbright)
	{
		gl_config.doubleModulateLM = false;		// when gamma overbrighted, use 'src*dst' instead of 'src*dst*2 with lm /= 2'
		overbright = 1;							// 0 or 1 only!
	}

	gl_config.overbright = overbright;
	gl_config.identityLightValue = 255 / (1 << overbright);
	gl_config.identityLightValue_f = 1.0f / (float)(1 << overbright);

	invGamma = 1.0 / Cvar_Clamp (r_gamma, 0.5, 3.0);

	if (gl_config.deviceSupportsGamma)
		GLimp_SetGamma (r_gamma->value);

	for (i = 0; i < 256; i++)
	{
		int		v;

		if (invGamma == 1.0)
			v = i;
		else
			v = appRound (pow (i / 255.0f, invGamma) * 255.0f);

		v <<= overbright;
		gammaTable[i] = bound(v, 0, 255);
	}
}


/*----------------- Imagelist -----------------*/


static void Imagelist_f (bool usage, int argc, char **argv)
{
#if 0
	// testing hash function
	int		hash, i, j, distr[HASH_SIZE], distr2[HASH_SIZE], max;
	static int last = 0;

	hash = last;
	last = (last + 16) & 0x1FF;

	for (j = 0; j < 16; j++)
	{
		Com_Printf ("---- Hash distribution (0x%4X) ----\n", hash);
		memset (distr, 0, sizeof(distr));
		memset (distr2, 0, sizeof(distr2));
		max = 0;

		for (i = 0; i < MAX_TEXTURES; i++)
		{
			image_t *img;
			int		h;
			char	*s, c;

			img = &imagesArray[i];
			if (!img->name[0]) continue;

			h = 0;
			s = img->name;
			while (c = *s++) h = (h ^ hash) + c;
			h &= HASH_MASK;

			distr[h]++;
			if (distr[h] > max) max = distr[h];
		}

		for (i = 0; i < HASH_SIZE; i++)
			distr2[distr[i]]++;

		for (i = 0; i <= max; i++)
			Com_Printf ("Chain %dx : %d\n", i, distr2[i]);
		hash++;
	}

#else

	int		texels, i, idx, n;
	char	*mask;

	static struct {
		int		fmt;
		char	*name;
	} fmtInfo[] = {
#define _STR(s) {s, #s}
		{3,	"RGB"},
		{4,	"RGBA"},
		_STR(GL_RGB5),
		_STR(GL_RGB8),
		_STR(GL_RGBA4),
		_STR(GL_RGBA8),
		_STR(GL_RGB5_A1),
		{GL_RGB4_S3TC,	"S3TC"}, //"RGB4_S3TC"},
		{GL_RGBA_S3TC,	"RGBA_S3TC"},
		{GL_RGBA4_S3TC,	"RGBA4_S3TC"},
		{GL_COMPRESSED_RGB_S3TC_DXT1_EXT, "DXT1"},
		{GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, "DXT1A"},
		{GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, "DXT3"},
		{GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, "DXT5"},
		{GL_COMPRESSED_RGB_ARB, "RGB_ARB"},
		{GL_COMPRESSED_RGBA_ARB, "RGBA_ARB"}
#undef _STR
	};
	static char alphaTypes[3] = {' ', '1', '8'};
	static char boolTypes[2] = {' ', '+'};

	if (argc > 2 || usage)
	{
		Com_Printf ("Usage: imagelist [<mask>]\n");
		return;
	}

	if (argc == 2)
		mask = argv[1];
	else
		mask = NULL;

	Com_Printf ("----w----h----a-wr-m-fmt------name----------\n");
	idx = texels = n = 0;

	for (i = 0; i < MAX_TEXTURES; i++)
	{
		char	*fmt, *color;
		image_t	*img;
		int		f, fi;

		img = &imagesArray[i];
		if (!img->name[0]) continue;	// free slot

		idx++;
		if (mask && !MatchWildcard (img->name, mask, true)) continue;
		n++;

		texels += img->internalWidth * img->internalHeight;

		f = img->internalFormat;
		fmt = S_RED"???"S_WHITE;
		for (fi = 0; fi < ARRAY_COUNT(fmtInfo); fi++)
			if (fmtInfo[fi].fmt == f)
			{
				fmt = fmtInfo[fi].name;
				break;
			}
		if (img->flags & IMAGE_WORLD)
			color = S_GREEN;
		else if (img->flags & IMAGE_SKIN)
			color = S_CYAN;
		else
			color = "";

		Com_Printf ("%-3d %-4d %-4d %c %c  %c %-8s %s%s\n", idx, img->internalWidth, img->internalHeight,
			img->flags & IMAGE_NOALPHA ? '-' : alphaTypes[img->alphaType],
			boolTypes[(img->flags & IMAGE_CLAMP) == 0], boolTypes[(img->flags & IMAGE_MIPMAP) != 0],
			fmt, color, img->name);
	}

	Com_Printf ("Displayed %d/%d images; texel count (no mipmaps) %d\n", n, idx, texels);

	if (mask && mask[0] == '*' && mask[1] == 0)	// mask = "*"
	{
		int		distr[HASH_SIZE], max;

		Com_Printf ("--------------\nHash distribution:\n");

		max = 0;
		memset (distr, 0, sizeof(distr));

		for (i = 0; i < HASH_SIZE; i++)
		{
			int		num;
			image_t	*img;

			num = 0;
			for (img = hashTable[i]; img; img = img->hashNext) num++;
			if (num > max) max = num;
			distr[num]++;
		}
		for (i = 0; i <= max; i++)
			Com_Printf ("Chain %dx : %d\n", i, distr[i]);
	}
#endif
}


static void ImageReload_f (bool usage, int argc, char **argv)
{
	char	*mask;
	int		i, num, time;
	image_t	*img;

	if (argc != 2 || usage)
	{
		Com_Printf ("Usage: img_reload <mask>\n");
		return;
	}

	time = Sys_Milliseconds ();
	mask = argv[1];
	num = 0;
	for (i = 0, img = imagesArray; i < MAX_TEXTURES; i++, img++)
	{
		if (!img->name[0]) continue;	// free slot
		if (!MatchWildcard (img->name, mask, true)) continue;

		if (img->name[0] == '*' || img->flags & IMAGE_SYSTEM) continue;
		if (GL_FindImage (img->name, img->flags | IMAGE_RELOAD))
		{
//			Com_DPrintf ("%s reloaded\n", img->name);
			num++;
		}
	}
	Com_DPrintf ("%d images reloaded in %g sec\n", num, (Sys_Milliseconds () - time) / 1000.0f);
}


/*-------------------- Screenshots ---------------------*/


#define LEVELSHOT_W		256
#define LEVELSHOT_H		256


void GL_PerformScreenshot (void)
{
	byte	*buffer, *src, *dst;
	char	name[MAX_OSPATH], *ext;
	int		i, width, height, size;
	bool	result;
	FILE	*f;

	if (!gl_screenshotName ||
		((gl_screenshotFlags & SHOT_WAIT_3D) && !gl_state.have3d))
		return;		// already performed in current frame or wait another frame

	glFinish ();

	ext = (gl_screenshotFlags & SHOT_JPEG ? ".jpg" : ".tga");
	if (gl_screenshotName[0])
	{
		strcpy (name, gl_screenshotName);
		strcat (name, ext);
	}
	else
	{
		// autogenerate name
		//?? should ListFiles(..."shot*") in current mod (OS FS) and find unused name in this list
		for (i = 0; i < 10000; i++)
		{	// check for a free filename
			Com_sprintf (ARRAY_ARG(name), "%s/screenshots/shot%04d%s", FS_Gamedir (), i, ext);
			if (!(f = fopen (name, "rb")))
				break;	// file doesn't exist
			fclose (f);
		}

	}
	gl_screenshotName = NULL;

	// create the screenshots directory if it doesn't exist
	FS_CreatePath (name);

	// allocate buffer for 4 color components (required for ResampleTexture()
	buffer = (byte*)appMalloc (vid.width * vid.height * 4);
	// read frame buffer data
	glReadPixels (0, 0, vid.width, vid.height, GL_RGBA, GL_UNSIGNED_BYTE, buffer);

/*
	if (!buffer[2000] && !buffer[2001] && !buffer[2002] && !buffer[2004] && !buffer[2005] && !buffer[2006]) Com_WPrintf("BLACK!\n");
	for (i = 2000; i < 2240; i++) Com_Printf("%02X  ", buffer[i]);
	Com_Printf ("\n");
*/

	if (gl_screenshotFlags & SHOT_SMALL)
	{
		byte *buffer2;

		buffer2 = (byte*)appMalloc (LEVELSHOT_W * LEVELSHOT_H * 4);
		ResampleTexture ((unsigned *)buffer, vid.width, vid.height, (unsigned *)buffer2, LEVELSHOT_W, LEVELSHOT_H);
		appFree (buffer);
		buffer = buffer2;
		width = LEVELSHOT_W;
		height = LEVELSHOT_H;
	}
	else
	{
		width = vid.width;
		height = vid.height;
	}
	size = width * height;

	// remove 4th color component and correct gamma
	src = dst = buffer;
	for (i = 0; i < size; i++)
	{
		unsigned	r, g, b;

		r = *src++; g = *src++; b = *src++;
		src++;	// skip alpha
		// correct gamma
		if (gl_config.deviceSupportsGamma)
		{
			if (gl_screenshotFlags & SHOT_NOGAMMA)
			{
				// we need to correct overbright
				r <<= gl_config.overbright; r = bound(r, 0, 255);
				g <<= gl_config.overbright; g = bound(g, 0, 255);
				b <<= gl_config.overbright; b = bound(b, 0, 255);
			}
			else
			{
				r = gammaTable[r];
				g = gammaTable[g];
				b = gammaTable[b];
			}
		}
		// put new values back
		*dst++ = r;
		*dst++ = g;
		*dst++ = b;
	}

	if (gl_screenshotFlags & SHOT_JPEG)
		result = WriteJPG (name, buffer, width, height, (gl_screenshotFlags & SHOT_SMALL));
	else
		result = WriteTGA (name, buffer, width, height);

	appFree (buffer);

	if (result && !(gl_screenshotFlags & SHOT_SILENT))
		Com_Printf ("Wrote %s\n", strrchr (name, '/') + 1);
}


/*------------------ Init/shutdown --------------------*/


#define DEF_IMG_SIZE	16
#define DLIGHT_SIZE		16

void GL_InitImages (void)
{
CVAR_BEGIN(vars)
	CVAR_VAR(gl_showImages, 0, 0),
	CVAR_VAR(r_colorMipLevels, 0, 0),
	CVAR_VAR(gl_picmip, 0, CVAR_ARCHIVE|CVAR_NOUPDATE),
	CVAR_VAR(gl_roundImagesDown, 0, CVAR_ARCHIVE),
	CVAR_VAR(gl_textureBits, 0, CVAR_ARCHIVE|CVAR_NOUPDATE)
CVAR_END
	byte	tex[256*32*4], *p;
	int		texnum, x, y;
	image_t	*image;

	guard(GL_InitImages);

	Cvar_GetVars (ARRAY_ARG(vars));
	imageCount = 0;
	memset (hashTable, 0, sizeof(hashTable));
	memset (imagesArray, 0, sizeof(imagesArray));
	for (texnum = 0, image = &imagesArray[0]; texnum < MAX_TEXTURES; texnum++, image++)
		image->texnum = texnum;

	GL_SetupGamma ();	// before texture creation
	GetPalette ();		// read palette for 8-bit WAL textures

	RegisterCommand ("imagelist", Imagelist_f);
	RegisterCommand ("img_reload", ImageReload_f);

	/*--------- create default texture -----------*/
	memset (tex, 0, DEF_IMG_SIZE*DEF_IMG_SIZE*4);
	for (y = 0, p = &tex[0]; y < DEF_IMG_SIZE; y++)
		for (x = 0; x < DEF_IMG_SIZE; x++, p += 4)
			if (x < 2 || x >= DEF_IMG_SIZE-2 || y < 2 || y >= DEF_IMG_SIZE-2)
				p[0] = p[1] = p[2] = p[3] = 255;
	gl_defaultImage = GL_CreateImage ("*default", tex, DEF_IMG_SIZE, DEF_IMG_SIZE, IMAGE_MIPMAP);
	gl_defaultImage->flags |= IMAGE_SYSTEM;

	/*---------- create video image ------------*/
	memset (tex, 255, 16*16*4);
	if (!GL_SUPPORT(QGL_NV_TEXTURE_RECTANGLE))
		gl_videoImage = GL_CreateImage ("*video", tex, 16, 16, IMAGE_CLAMP);
	else	// without this, uploading video as rect texture will damage other texture:
		gl_videoImage = GL_CreateImage ("*video", tex, 10, 5, IMAGE_CLAMP);
	gl_videoImage->flags |= IMAGE_SYSTEM;

	/*------ create identity light image ---------*/
#if 0
	y = gl_config.identityLightValue;
	for (x = 0, p = &tex[0]; x < 8*8; x++, p += 4)
	{
		p[0] = p[1] = p[2] = y;
		p[3] = 255;
	}
	gl_identityLightImage = GL_CreateImage ("*identityLight", tex, 8, 8, IMAGE_MIPMAP);
#else
	tex[0] = tex[1] = tex[2] = gl_config.identityLightValue;
	tex[3] = 255;
	gl_identityLightImage = GL_CreateImage ("*identityLight", tex, 1, 1, IMAGE_MIPMAP);
#endif
	gl_identityLightImage->flags |= IMAGE_SYSTEM;

	/*----------- create dlight image ------------*/
	p = tex;
	for (y = 0; y < DLIGHT_SIZE; y++)
	{
		float	yv;

		yv = (y - DLIGHT_SIZE/2 + 0.5f); yv *= yv;
		for (x = 0; x < DLIGHT_SIZE; x++)
		{
			float	xv;
			int		v;

			xv = (x - DLIGHT_SIZE/2 + 0.5f); xv *= xv;
#if 1
			v = 255 * (1 - (sqrt (xv + yv) + 1) / (DLIGHT_SIZE/2));		// linear
//			xv = (1 - (sqrt (xv + yv) + 1) / (DLIGHT_SIZE/2)); xv = bound(xv, 0, 1); v = xv * xv * 255;	// square
			v = bound(v, 0, 255);
#else
			v = appCeil (4000.0f / (xv + yv));
			if (v < 75) v = 0;
			if (v > 255) v = 255;
#endif
			p[0] = p[1] = p[2] = v;
			p[3] = 255;
			p += 4;
		}
	}
	// NOTE: be sure image border is black and do not create mipmaps (this will change border)
	gl_dlightImage = GL_CreateImage ("*dlight", tex, 16, 16, IMAGE_CLAMP|IMAGE_TRUECOLOR);
	gl_dlightImage->flags |= IMAGE_SYSTEM;

	/*----------- create particle image ----------*/
	p = tex;
	for (y = 0; y < 8; y++)
		for (x = 0; x < 8; x++)
		{
			if (y >= 1 && y <= 4 && x >= 1 && x <= 4)
			{
				p[0] = p[1] = p[2] = 255;

				if (y == 1 || y == 4 || x == 1 || x == 4)
					p[3] = 128;

				if ((y == 1 || y == 4) && (x == 1 || x == 4))
					p[3] = 64;
				else
					p[3] = 255;
			}
			else
				p[0] = p[1] = p[2] = p[3] = 0;
			p += 4;
		}
	gl_particleImage = GL_CreateImage ("*particle", tex, 8, 8, IMAGE_CLAMP|IMAGE_MIPMAP);
	gl_particleImage->flags |= IMAGE_SYSTEM;

	/*------------ create fog image --------------*/
/*	p = tex;
	for (y = 0; y < 32; y++)
	{
		float	yf;

		yf = (y + 0.5f) / 32;
		for (x = 0; x < 256; x++)
		{
			float	xf, v;

			xf = (x + 0.5f) / 256;		//?? - 1.0f/512;
			if (xf < 0 || yf < 1.0f/32)
				v = 0;
			else
			{
				if (yf < 31.0f/32)
					v = (yf - 1.0f/32) * xf * 16/15;
				else
					v = xf;

				v *= 8;
				if (v > 1) v = 1;
				v = sqrt (v);
			}

			p[0] = p[1] = p[2] = 255;
			p[3] = appRound (v * 255);
			p += 4;
		}
	}
	gl_fogImage = GL_CreateImage ("*fog", tex, 256, 32, IMAGE_CLAMP|IMAGE_TRUECOLOR);	//?? mipmap
	gl_fogImage->flags |= IMAGE_SYSTEM; */

	gl_reflImage = GL_FindImage ("fx/specular", IMAGE_MIPMAP|IMAGE_TRUECOLOR);	//!! move reflection images to a different place
	gl_reflImage2 = GL_FindImage ("fx/diffuse", IMAGE_MIPMAP|IMAGE_TRUECOLOR);

	unguard;
}


void GL_ShutdownImages (void)
{
	int		i;
	image_t	*img;

	UnregisterCommand ("imagelist");
	UnregisterCommand ("img_reload");

	if (!imageCount) return;

	// clear binds
	GL_SetMultitexture (0);

	// delete textures
	for (i = 0, img = imagesArray; i < MAX_TEXTURES; i++, img++)
	{
		if (!img->name[0]) continue;	// free slot
		glDeleteTextures (1, &img->texnum);
		img->name[0] = 0;				// required for statically linked renderer
		if (img->pic)
		{
			appFree (img->pic);
			img->pic = NULL;
		}
	}
	imageCount = 0;

	// don't clear other fields: after GL_ShutdownImages should not be called nothing
	// but GL_InitImages, which will perform this work
}


void GL_LoadDelayedImages (void)
{
	int		i, num, time;
	image_t	*img;

	time = Sys_Milliseconds ();
	num = 0;
	for (i = 0, img = imagesArray; i < MAX_TEXTURES; i++, img++)
	{
		if (!img->name[0]) continue;	// free slot
		if (!(img->pic)) continue;

//		Com_Printf ("up: %s\n", img->name);
		GL_CreateImage (img->name, img->pic, img->width, img->height, img->flags);
		appFree (img->pic);
		img->pic = NULL;
		num++;
	}
	Com_DPrintf ("%d images uploaded in %g sec\n", num, (Sys_Milliseconds () - time) / 1000.0f);
}


void GL_ShowImages (void)
{
	int		i, nx, ny, x, y, num, numImg;
	image_t	*img;
	float	dx, dy;
	char	*name, *mask;

	mask = gl_showImages->string;
	if (!mask[0] || (!mask[1] && mask[0] == '0')) return;	// "" or "0"
	if (mask[0] == '1' && mask[1] == 0) mask = "*";			// "1" -> "*"

	// count matches
	numImg = 0;
	for (i = 0; i < MAX_TEXTURES; i++)
	{
		name = imagesArray[i].name;
		if (!name[0]) continue;
		if (MatchWildcard (name, mask, true)) numImg++;
	}
	if (!numImg) return;		// no matched images

	// setup OpenGL
	GL_Set2DMode ();
	GL_SetMultitexture (1);
	GL_State (GLSTATE_SRC_SRCALPHA|GLSTATE_DST_ONEMINUSSRCALPHA|GLSTATE_NODEPTHTEST);
	glClearColor (0.1, 0.02, 0.02, 1);
	glClear (GL_COLOR_BUFFER_BIT);
	glColor3f (1, 1, 1);

	// compute best grid size
	for (i = 1; i < 10; i++)
	{
		nx = i * 4;
		ny = i * 3;
		if (numImg <= nx*ny) break;
	}

	// display images
	dx = vid.width / nx;
	dy = vid.height / ny;
	num = numImg;
	img = &imagesArray[0];
	for (y = 0; y < ny && num; y++)
	{
		float	y0, x0, s, t;

		y0 = y * dy;
		for (x = 0; x < nx; x++)
		{
			if (!num) break;

			while (1)
			{
				name = img->name;
				if (name[0] && MatchWildcard (name, mask, true)) break;
				img++;
			}

			x0 = x * dx;

			GL_BindForce (img);
			glBegin (GL_QUADS);
			if (img->target != GL_TEXTURE_RECTANGLE_NV)
			{
				s = 1;
				t = 1;
			}
			else
			{
				s = img->internalWidth;
				t = img->internalHeight;
			}
			glTexCoord2f (0, 0);
			glVertex2f (x0, y0);
			glTexCoord2f (s, 0);
			glVertex2f (x0 + dx, y0);
			glTexCoord2f (s, t);
			glVertex2f (x0 + dx, y0 + dy);
			glTexCoord2f (0, t);
			glVertex2f (x0, y0 + dy);
			glEnd ();

			num--;
			img++;
		}
	}
}

//------------- Loading images --------------------------

// GL_FindImage -- main image creating/loading function
image_t *GL_FindImage (const char *name, int flags)
{
	char	name2[MAX_QPATH], *s;
	int		hash, flags2, fmt, prefFmt, width, height, len;
	image_t	*img;
	byte	*pic;

	if (!name[0])
		return gl_defaultImage;		// NULL name -- NULL image

	Q_CopyFilename (name2, name, sizeof(name2));

	/*------- find image using hash table --------*/
	hash = ComputeHash (name2);
//	Com_Printf ("FindImage(%s): hash = %X\n", name2, hash);//!!

	// find extension
	s = strrchr (name2, '.');
	if (!s)
		s = strchr (name2, 0);		// == &name2[strlen(name2)]; points to a place, where extension will be added
	len = s - name2;				// length of name without extension

	flags2 = flags & (IMAGE_MIPMAP|IMAGE_CLAMP);
	if (!(flags & IMAGE_RELOAD))
	{
		for (img = hashTable[hash]; img; img = img->hashNext)
			if (!strnicmp (img->name, name2, len) &&			// len chars matched
				(!img->name[len] || img->name[len] == '.'))		// rest of img->name if null or extension
			{
				// found a name ...
				// compare image flags
				if ((img->flags & (IMAGE_MIPMAP|IMAGE_CLAMP)) == flags2)
				{
					if (img->flags & IMAGE_SYSTEM) return img;

					// check required alpha channel
					if (flags & IMAGE_NOALPHA)
					{
						if (img->alphaType == 0) return img;
					}
					else
					{
						if (!(img->flags & IMAGE_NOALPHA)) return img;
					}
				}
				else
				{
					// flags are different ...
					if (name[0] == '*' || img->flags & IMAGE_SYSTEM)
					{
						Com_WPrintf ("R_FindImage(%s): using different flags (%X != %X) for system image\n", name2, flags, img->flags);
						return img;
					}
					hash = -1;	// mark: image used with a different flags (for displaying a warning message)
				}
				// continue search ...
			}
	}
	if (hash == -1)
		Com_WPrintf ("R_FindImage(%s): Reusing image with a different flags %X\n", name2, flags);

	/*---------- image not found -----------*/
//	Com_Printf("FindImage: disk name = %s\n",name2);//!!
	// select preferred format
	if (!strcmp (s, ".tga"))
		prefFmt = IMAGE_TGA;
	else if (!strcmp (s, ".jpg"))
		prefFmt = IMAGE_JPG;
//	else if (!strcmp (s, ".pcx"))	//-- commented - never prefer 8-bit textures
//		prefFmt = IMAGE_PCX;
//	else if (!strcmp (s, ".wal"))
//		prefFmt = IMAGE_WAL;
	else
		prefFmt = 0;

	*s = 0; // cut extension
	fmt = ImageExists (name2, IMAGE_32BIT);
	if (fmt & prefFmt)
		fmt = prefFmt;				// restrict loading to this image type

	// load image within a preferred (or prioritized) format
	if (fmt & IMAGE_TGA)
	{
		strcpy (s, ".tga");
START_PROFILE(..img::tga)
		LoadTGA (name2, &pic, &width, &height);
END_PROFILE
	}
	else if (fmt & IMAGE_JPG)
	{
		strcpy (s, ".jpg");
START_PROFILE(..img::jpg)
		LoadJPG (name2, &pic, &width, &height);
END_PROFILE
	}
	else if (fmt & IMAGE_PCX)
	{
		byte	*pic8, *palette;

		strcpy (s, ".pcx");
START_PROFILE(..img::pcx)
		LoadPCX (name2, &pic8, &palette, &width, &height);
		if (pic8)
		{
#if 0
			unsigned pal[256];
			byte	*p;
			int		i;

			p = palette;
			for (i = 0; i < 256; i++, p += 3)
			{
				unsigned	v;

				v = RGB255(p[0], p[1], p[2]);
				pal[i] = LittleLong(v);
			}
			pal[255] &= LittleLong(0x00FFFFFF);						// #255 is transparent (alpha = 0)
			pic = Convert8to32bit (pic8, width, height, pal);
#else
			pic = Convert8to32bit (pic8, width, height, NULL);
#endif
			appFree (pic8);
			appFree (palette);
		}
		else
			pic = NULL;
END_PROFILE
	}
	else if (fmt & IMAGE_WAL)
	{
		miptex_t	*mt;

		strcpy (s, ".wal");
START_PROFILE(..img::wal)
		if (mt = (miptex_t*) FS_LoadFile (name2))
		{
			width = LittleLong(mt->width);
			height = LittleLong(mt->height);
			pic = Convert8to32bit ((byte*)mt + LittleLong(mt->offsets[0]), width, height, NULL);
			FS_FreeFile (mt);
		}
		else
			pic = NULL;
END_PROFILE
	}
	else
	{
		Com_DPrintf ("Cannot find image %s.*\n", name2);
		return NULL;
	}

	// upload image
	if (pic)
	{
START_PROFILE(..img::up)
		img = GL_CreateImage (name2, pic, width, height, flags);
		appFree (pic);
END_PROFILE
		return img;
	}
	else
	{
		Com_WPrintf ("R_FindImage(%s): cannot load texture\n", name2);
		return NULL;
	}
}
