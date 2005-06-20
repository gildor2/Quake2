//#define PROFILE_LOADING

#ifdef PROFILE_LOADING
#define START_PROFILE(name)			\
	{								\
		static const char _name[] = #name;\
		const char *_arg = "";		\
		unsigned _time = appCycles();
#define START_PROFILE2(name,arg)	\
	{								\
		static const char _name[] = #name;\
		const char *_arg = arg;		\
		unsigned _time = appCycles();
#define END_PROFILE	\
		if (Cvar_VariableInt("r_profile2")) \
			Com_Printf(S_MAGENTA"%s "S_GREEN"%s"S_CYAN": %.2f ms\n", _name, _arg, \
			appDeltaCyclesToMsecf(appCycles() - _time)); \
	}
#else
#define START_PROFILE(name)
#define START_PROFILE2(name,arg)
#define END_PROFILE
#endif

#include "OpenGLDrv.h"
#include "gl_image.h"
#include "gl_backend.h"			// for BK_DrawPic() only

namespace OpenGLDrv {


image_t	*gl_defaultImage;
//image_t	*gl_whiteImage;		//?? unneeded: can use "image = NULL" for this (CHECK THIS WITH MTEX!)
			//?? may be, use dummy image (static) and handle this pointer specially (when GL_Bind() - use glDisable() etc ...)
			//?? This may be required, when using $white image, and system will try to get image params
			//?? May be, instead of special processing, setup img->target == 0 (not TEXTURE2D or TEXTURE_RECTANGLE ...)
image_t	*gl_identityLightImage;
image_t	*gl_dlightImage;
image_t	*gl_particleImage;
//image_t	*gl_fogImage;
image_t	*gl_videoImage;
image_t	*gl_reflImage;
image_t	*gl_reflImage2;

int 	screenshotFlags;
char	*screenshotName;


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

static image_t	imagesArray[MAX_TEXTURES];	// no dynamic allocation (array size < 100K - small enough)
static image_t	*hashTable[HASH_SIZE];
static int		imageCount;

/*
 * gammaTable is used for 2 purposes:
 *  1. when gamma is hardware -- for lightscaling screenshots
 *  2. when gamma is software -- for lightscaling uploadable textures
 */
//?? sw gamma: should lightscale lightmaps or textures (not both) ?
static byte		gammaTable[256];

// name should be in a lower case
static int ComputeHash (const char *name)
{
	int h = 0;
	char c;
	while (c = *name++, c && c != '.')	// exclude extension from hash
		h = (h ^ 0x25) + c;				// good values: 07 (best in 10-bit hash), 25, 61, 7B
	return h & HASH_MASK;
}

//-------------- 8-bit palette -----------------------------------------

static void GetPalette ()
{
	int		width, height;
	byte	*pic, *pal;

	// get the palette
	LoadPCX ("pics/colormap.pcx", pic, pal, width, height);
	if (!pal)
		Com_FatalError ("Can not load pics/colormap.pcx");

	const byte *p = pal;
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


static byte *Convert8to32bit (byte *in, int width, int height, const unsigned *palette)
{
	guard(Convert8to32bit);
	if (!palette)
		palette = gl_config.tbl_8to32;

	int size = width * height;
	byte *out = new byte [size * 4];

	unsigned *p = (unsigned*) out;
	for (int i = 0; i < size; i++)
		*p++ = palette[*in++];

	return out;
	unguard;
}


//----------------------------------------------------------------------

void GL_TextureMode (const char *name)	//?? change (not strings; use enum {none,bilinear,trilinear,anysotropic})
{
	static const struct {
		const char *name;
		GLenum	minimize, maximize;
	} texModes[] = {
		//?? remove this later:
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
		if (stricmp (texModes[i].name, name)) continue;

		gl_filter_min = texModes[i].minimize;
		gl_filter_max = texModes[i].maximize;

		int j;
		const image_t *img;
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


#define TBL_SATURATE		// should be enabled

static void LightScaleTexture (unsigned *pic, int width, int height)
{
	int		i;
	byte	*p;
#ifdef TBL_SATURATE
	static float lastSat = 1.0f;		// when sat. == 1 -- pic will not be changed
	static int sat1[256 * 3];
	static int sat2[256];

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
		float tmp = (1.0f - lastSat) * 2 / 3;				// modulated by 2
		for (i = 0; i < ARRAY_COUNT(sat1); i++)
			sat1[i] = appRound (i * tmp);
		for (i = 0; i < ARRAY_COUNT(sat2); i++)
			sat2[i] = appRound (i * lastSat * 2);	// modulated by 2
	}
#endif

	float sat = r_saturation->value;
	int c = width * height;

	if (sat != 1.0)
	{
		p = (byte *)pic;
		for (i = 0; i < c; i++, p+=4)
		{
#ifdef TBL_SATURATE
			int		r, g, b;

			r = p[0]; g = p[1]; b = p[2];
			int s1 = sat1[r + g + b];
			r = (s1 + sat2[r]) >> 1;
			p[0] = bound(r,0,255);
			g = (s1 + sat2[g]) >> 1;
			p[1] = bound(g,0,255);
			b = (s1 + sat2[b]) >> 1;
			p[2] = bound(b,0,255);
#else
			float	r, g, b;

			// get color
			r = p[0];  g = p[1];  b = p[2];
			// change saturation
			float light = (r + g + b) / 3.0f;
			SATURATE(r,light,sat);
			SATURATE(g,light,sat);
			SATURATE(b,light,sat);
			// put color
			p[0] = appRound (r);  p[1] = appRound (g);  p[2] = appRound (b);
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
	int shift = gl_config.overbright;
	if (!gl_config.doubleModulateLM)
		shift--;

	byte *p = (byte *)pic;
	int c = width * height;
	for (int i = 0; i < c; i++, p += 4)
	{
		int sh = shift + (signed char)p[3];		// usa alpha as additional shift info
		if (sh > 0)
		{
			p[0] >>= sh;
			p[1] >>= sh;
			p[2] >>= sh;
		}
		else
		{
			sh = -sh;

			int r = p[0] << sh;
			int g = p[1] << sh;
			int b = p[2] << sh;
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
	width *= 4;		// sizeof(rgba)
	height >>= 1;
	byte *out = in;
	for (int i = 0; i < height; i++, in += width)
	{
		for (int j = 0; j < width; j += 8, out += 4, in += 8)
		{
			int		r, g, b, a, am, n;

			r = g = b = a = am = n = 0;
//!! should perform removing of alpha-channel when IMAGE_NOALPHA specified
//!! should perform removing (making black) color channel when alpha==0 NOT ALWAYS
//!!  - should analyze shader, and it will not use blending with alpha (or no blending at all)
//!!    then remove alpha channel (require to process shader's *map commands after all other commands, this
//!!    can be done with delaying [map|animmap|clampmap|animclampmap] lines and executing after all)
#if 0
#define PROCESS_PIXEL(idx)	\
	if (in[idx+3])	\
	{	\
		n++;	\
		r += in[idx]; g += in[idx+1]; b += in[idx+2]; a += in[idx+3];	\
		am = max(am, in[idx+3]);	\
	}
#else
#define PROCESS_PIXEL(idx)	\
	{	\
		n++;	\
		r += in[idx]; g += in[idx+1]; b += in[idx+2]; a += in[idx+3];	\
		am = max(am, in[idx+3]);	\
	}
#endif
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
	for (int sw = 1; sw < width; sw <<= 1) ;
	for (int sh = 1; sh < height; sh <<= 1) ;

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

	int maxSize = min(gl_config.maxTextureSize, MAX_TEX_SIZE);
	while (sw > maxSize) sw >>= 1;
	while (sh > maxSize) sh >>= 1;

	if (sw < 1) sw = 1;
	if (sh < 1) sh = 1;

	*scaledWidth = sw;
	*scaledHeight = sh;
}


static void ComputeImageColor (void *pic, int width, int height, color_t *color)
{
	int		c[4], i;
	byte	*p;

	int count = width * height;
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
static void Upload (void *pic, unsigned flags, image_t *image)
{
	int		scaledWidth, scaledHeight;
	int		format, size;

	LOG_STRING(va("// Upload(%s)\n", image->name));

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

	/*---------------- Resample/lightscale texture ------------------*/
	size = scaledWidth * scaledHeight * 4;
	unsigned *scaledPic = (unsigned*)appMalloc (size);
START_PROFILE(..up::scale)
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
	//?? add GL_LUMINANCE and GL_ALPHA formats ?
	//?? LUMINAMCE = (L,L,L,1); INTENSITY = (I,I,I,I); ALPHA = (0,0,0,A); RGB = (R,G,B,1); RGBA = (R,G,B,A)
	//?? see OpenGL spec: "Rasterization/Texturing/Texture Environments ..."

	if (flags & IMAGE_LIGHTMAP)
		format = 3;
	else if (flags & IMAGE_TRUECOLOR)
		format = GL_RGBA8;
	else
	{
		int alpha = image->alphaType;

		format = 0;
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
		// setup min/max filter
		glTexParameteri (image->target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri (image->target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}
	else
	{
START_PROFILE(..up::mip)
		int miplevel = 0;
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
				// here miplevel >= 1
				color_t	c;
				c.rgba = 0;
				c.c[(miplevel - 1) % 3] = 255;
				byte *p = (byte *) scaledPic;
				for (int i = 0; i < size; i++, p += 4)
				{
					p[0] = (c.c[0] + p[0]) / 4;
					p[1] = (c.c[1] + p[1]) / 4;
					p[2] = (c.c[2] + p[2]) / 4;
				}
			}
			glTexImage2D (image->target, miplevel, format, scaledWidth, scaledHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaledPic);
		}
		// setup min/max filter
		glTexParameteri (image->target, GL_TEXTURE_MIN_FILTER, gl_filter_min);
		glTexParameteri (image->target, GL_TEXTURE_MAG_FILTER, gl_filter_max);
END_PROFILE
	}

	// setup wrap flags
    GLenum repMode = flags & IMAGE_CLAMP ? GL_CLAMP : GL_REPEAT;
	glTexParameteri (image->target, GL_TEXTURE_WRAP_S, repMode);
	glTexParameteri (image->target, GL_TEXTURE_WRAP_T, repMode);

	appFree (scaledPic);
}


image_t *CreateImage (const char *name, void *pic, int width, int height, unsigned flags)
{
	char	name2[MAX_QPATH];
	int		texnum;
	image_t	*image;

	guard(CreateImage);
	if (!name[0]) Com_FatalError ("R_CreateImage: null name");

//	Com_Printf ("CreateImage(%s)\n", name);//!!!!

//	if (strlen (name) >= MAX_QPATH)
//		Com_FatalError ("R_CreateImage: name \"%s\" is too long", name);
	appCopyFilename (name2, name, sizeof(name2));

	// find image with the same name
	bool reuse = false;
	image_t *freeSlot = NULL;
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

	int size = width * height;
	int alpha = 0;

	// eliminate the alpha channel when needed
	if (flags & IMAGE_NOALPHA)
	{
		bool alphaRemoved = false;
		byte *p = (byte*)pic + 3;
		for (int i = 0; i < size; i++, p += 4)
		{
			if (*p != 255)
			{
				alphaRemoved = true;
				*p = 255;
			}
		}
		if (!alphaRemoved)
			flags &= ~IMAGE_NOALPHA;		// original image have no alpha-channel -- reset NOALPHA flag
	}
	else
	{
		// check for alpha channel in image
		int numAlpha0 = 0;
		byte *p = (byte*)pic + 3;
		for (int i = 0; i < size; i++, p += 4)
		{
			byte a = *p;
			if (a == 255) continue; // opaque
			if (a == 0)
			{
				alpha = 1;			// have alpha == 0 (1 bit alpha)
				numAlpha0++;
				continue;
			}
			alpha = 2;				// have alpha == [1..254] (8 bit alpha)
			break;
		}

		if (alpha == 1 && numAlpha0 > (size / 100) && (flags & IMAGE_MIPMAP))
			alpha = 2;				// mipmaps will create middle alpha's
	}
	image->alphaType = alpha;

	// setup image_t fields
	strcpy (image->name, name2);
	image->width = width;
	image->height = height;
	image->flags = flags & IMAGE_FLAGMASK;
	image->target = GL_TEXTURE_2D;

	if (width & (width-1) | height & (height-1))
	{
		// non power-of-two sized texture
		// choose TEXTURE_RECTANGLE target when possible
		if ((flags & (IMAGE_CLAMP|IMAGE_MIPMAP|IMAGE_PICMIP)) == IMAGE_CLAMP &&	// !wrap, !mipmap, !picmip
			width  <= gl_config.maxRectTextureSize	&&		// suitable size (when extensions is disabled, maxRectTextureSize==0)
			height <= gl_config.maxRectTextureSize)
			image->target = GL_TEXTURE_RECTANGLE_NV;
		else if (alpha)
			image->alphaType = 2;	// image will be resampled, and it has alpha channel, so cannot use 1-bit alpha channel
	}

	ComputeImageColor (pic, width, height, &image->color);

	if (renderingEnabled)
	{
		// upload image
		GL_SetMultitexture (1);
		GL_BindForce (image);
		Upload (pic, flags, image);
	}
	else
	{
		// save image for later upload
		if (image->pic)
			delete image->pic;
		size *= 4;
		image->pic = new byte [size];
		memcpy (image->pic, pic, size);
		image->flags = flags;			//?? without IMAGE_FLAGMASK
		image->internalFormat = 0;		//??
	}

	if (!reuse)
	{
		// insert image into a hash table
		int hash = ComputeHash (name2);
		image->hashNext = hashTable[hash];
		hashTable[hash] = image;
	}

	return image;

	unguardf(("%s", name));
}


void LoadDelayedImages ()
{
	int		i;
	image_t	*img;

	unsigned time = appCycles();
	int num = 0;
	for (i = 0, img = imagesArray; i < MAX_TEXTURES; i++, img++)
	{
		if (!img->name[0]) continue;	// free slot
		if (!(img->pic)) continue;

//		Com_Printf ("up: %s\n", img->name);
		// upload image
		GL_SetMultitexture (1);
		GL_BindForce (img);
		Upload (img->pic, img->flags, img);

		delete img->pic;
		img->pic = NULL;
		img->flags &= IMAGE_FLAGMASK;

		num++;
	}
	Com_DPrintf ("%d images uploaded in %g sec\n", num, appDeltaCyclesToMsecf(appCycles() - time) / 1000.0f);
}


/*-------------------- Video support ----------------------*/


void DrawStretchRaw8 (int x, int y, int w, int h, int width, int height, byte *pic, unsigned *palette)
{
	image_t	*image;
	byte	*pic32;

	guard(DrawStretchRaw8);

	// prepare video image
	image = gl_videoImage;		//?? to allow multiple videos in a screen, should use gl_videoImage[some_number]
	GL_SetMultitexture (1);
	GL_BindForce (image);
	image->internalFormat = 3;

	if (GL_SUPPORT(QGL_NV_TEXTURE_RECTANGLE))	//?? check for max [rect] texture size
	{
		// convert 8->32 bit
		pic32 = Convert8to32bit (pic, width, height, palette);
#if 0
		if (image->target != GL_TEXTURE_RECTANGLE_NV)
			Com_DropError ("bad target");
#else
		glDeleteTextures (1, &image->texnum);
		image->internalWidth = width + 1;		// something different
		image->target = GL_TEXTURE_RECTANGLE_NV;
#endif
	}
	else
	{
		int		scaledWidth, scaledHeight;

		// we can perform uploading pic without scaling using TexSubImage onto a large texture and using
		// (0..width/large_width) - (0..height/large_height) coords when drawing ?? (but needs palette conversion anyway)
		GetImageDimensions (width, height, &scaledWidth, &scaledHeight, false);
		pic32 = new byte [scaledWidth * scaledHeight * 4];

		// fast resampling with conversion 8->32 bit
		float hScale = (float) height / scaledHeight;
		for (int i = 0; i < scaledHeight; i++)
		{
			int row = appFloor (hScale * i);
			if (row > height) break;

			byte *src = &pic[width * row];
			unsigned *dst = (unsigned*)pic32 + scaledWidth * i;
			int fracStep = (width << 16) / scaledWidth;
			int frac = fracStep >> 1;
			for (int j = 0; j < scaledWidth; j++)
			{
				*dst++ = palette[src[frac >> 16]];
				frac += fracStep;
			}
		}

		// set texture parameters
		glTexParameteri (image->target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri (image->target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		width = scaledWidth;
		height = scaledHeight;
	}

	// upload
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
	gl_speeds.numUploads++;
	// free converted pic
	delete pic32;

	// draw
	gl_videoShader->width = width;
	gl_videoShader->height = height;
	BK_DrawPic (gl_videoShader, x, y, w, h, 0, 0, 1, 1, RGB(1,1,1));
	if (gl_detailShader && width*3/2 < w)		// detail is not available or not needed
	{
#define VIDEO_NOISE_FPS		20
//		if (!Cvar_VariableInt("detail")) return;
		static int lastNoiseTime;
		static float dx, dy;
		static int flipMode;

		int time = appMilliseconds ();
		if (time - lastNoiseTime >= 1000/VIDEO_NOISE_FPS)
		{
			lastNoiseTime = time;
			dx = frand() / 2;
			dy = frand() / 2;
			flipMode = rand() & 3;
		}
		BK_DrawPic (gl_detailShader, x, y, w, h, dx, dy,
			w / gl_detailShader->width + dx, h / gl_detailShader->height + dy, RGB(1,1,1), flipMode);
	}

	unguard;
}


static void FreeImage (image_t *image)	//?? unused function
{
	if (!image->name[0])
		return;		// already ...
	if (image->flags & IMAGE_SYSTEM)
		return;		// system image - don't free it

	// delete gl texture
	glDeleteTextures (1, &image->texnum);
	if (image->pic)
	{
		delete image->pic;
		image->pic = NULL;
	}

	// remove from hash chain
	int hash = ComputeHash (image->name);
	image_t *img = hashTable[hash];
	if (img == image)	// first in chain
	{
		hashTable[hash] = image->hashNext;
		img = NULL;
	}
	while (img)
	{
		image_t	*next = img->hashNext;
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


void SetupGamma ()
{
	gl_config.vertexLight = gl_vertexLight->integer != 0;
	gl_config.deviceSupportsGamma = GLimp_HasGamma ();

	unsigned overbright = gl_overbright->integer;
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

	float invGamma = 1.0 / r_gamma->Clamp (0.5, 3.0);

	if (gl_config.deviceSupportsGamma)
		GLimp_SetGamma (r_gamma->value);

	for (int i = 0; i < 256; i++)
	{
		int		v;

		if (invGamma == 1)
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

	static const struct {
		GLenum	fmt;
		const char *name;
	} fmtInfo[] = {
#define _STR(s) {s, #s}
		{3,	"RGB"},
		{4,	"RGBA"},
		_STR(GL_RGB5),
		_STR(GL_RGB8),
		_STR(GL_RGBA4),
		_STR(GL_RGBA8),
		_STR(GL_RGB5_A1),
		{GL_RGB4_S3TC,	"S3TC"}, // RGB4_S3TC
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
	static const char alphaTypes[3] = {' ', '1', '8'};
	static const char boolTypes[2] = {' ', '+'};
	int		idx, texels, n;

	if (argc > 2 || usage)
	{
		Com_Printf ("Usage: imagelist [<mask>]\n");
		return;
	}

	const char *mask = (argc == 2) ? argv[1] : NULL;

	Com_Printf ("----w----h----a-wr-m-fmt------name----------\n");
	idx = texels = n = 0;

	for (int i = 0; i < MAX_TEXTURES; i++)
	{
		image_t *img = &imagesArray[i];
		if (!img->name[0]) continue;	// free slot

		idx++;
		if (mask && !appMatchWildcard (img->name, mask, true)) continue;
		n++;

		texels += img->internalWidth * img->internalHeight;

		GLenum f = img->internalFormat;
		const char *fmt = S_RED"???"S_WHITE;
		for (int fi = 0; fi < ARRAY_COUNT(fmtInfo); fi++)
			if (fmtInfo[fi].fmt == f)
			{
				fmt = fmtInfo[fi].name;
				break;
			}
		const char *color = "";
		if (img->flags & IMAGE_WORLD)
			color = S_GREEN;
		else if (img->flags & IMAGE_SKIN)
			color = S_CYAN;

		Com_Printf ("%-3d %-4d %-4d %c %c  %c %-8s %s%s\n", idx, img->internalWidth, img->internalHeight,
			img->flags & IMAGE_NOALPHA ? '-' : alphaTypes[img->alphaType],
			boolTypes[(img->flags & IMAGE_CLAMP) == 0], boolTypes[(img->flags & IMAGE_MIPMAP) != 0],
			fmt, color, img->name);
	}

	Com_Printf ("Displayed %d/%d images; texel count (no mipmaps) %d\n", n, idx, texels);

	if (mask && mask[0] == '*' && mask[1] == 0)	// mask = "*"
	{
		int		distr[HASH_SIZE];

		int max = 0;
		memset (distr, 0, sizeof(distr));

		for (i = 0; i < HASH_SIZE; i++)
		{
			int num = 0;
			for (image_t *img = hashTable[i]; img; img = img->hashNext) num++;
			if (num > max) max = num;
			distr[num]++;
		}
		Com_Printf ("--------------\nHash distribution:\n");
		for (i = 1; i <= max; i++)
			Com_Printf ("Chain %dx : %d\n", i, distr[i]);
	}
#endif
}


static void ImageReload_f (bool usage, int argc, char **argv)
{
	int		i;
	image_t	*img;

	if (argc != 2 || usage)
	{
		Com_Printf ("Usage: img_reload <mask>\n");
		return;
	}

	unsigned time = appCycles();
	const char *mask = argv[1];
	int num = 0;
	for (i = 0, img = imagesArray; i < MAX_TEXTURES; i++, img++)
	{
		if (!img->name[0]) continue;	// free slot
		if (!appMatchWildcard (img->name, mask, true)) continue;

		if (img->name[0] == '*' || img->flags & IMAGE_SYSTEM) continue;
		if (FindImage (img->name, img->flags | IMAGE_RELOAD))
		{
//			Com_DPrintf ("%s reloaded\n", img->name);
			num++;
		}
	}
	Com_DPrintf ("%d images reloaded in %g sec\n", num, appDeltaCyclesToMsecf(appCycles() - time) / 1000.0f);
}


/*-------------------- Screenshots ---------------------*/


#define LEVELSHOT_W		256
#define LEVELSHOT_H		256


void PerformScreenshot ()
{
	byte	*src, *dst;
	char	name[MAX_OSPATH];
	int		i, width, height;
	FILE	*f;

	if (!screenshotName ||
		((screenshotFlags & SHOT_WAIT_3D) && !gl_state.have3d))
		return;		// already performed in current frame or wait another frame

	glFinish ();

	const char *ext = (screenshotFlags & SHOT_JPEG ? ".jpg" : ".tga");
	if (screenshotName[0])
	{
		strcpy (name, screenshotName);
		strcat (name, ext);
	}
	else
	{
		// autogenerate name
		//?? should ListFiles(..."shot*") in current mod (OS FS) and find unused name in this list
		for (i = 0; i < 10000; i++)
		{	// check for a free filename
			appSprintf (ARRAY_ARG(name), "%s/screenshots/shot%04d%s", FS_Gamedir (), i, ext);
			if (!(f = fopen (name, "rb")))
				break;	// file doesn't exist
			fclose (f);
		}

	}
	screenshotName = NULL;

	// create the screenshots directory if it doesn't exist
	FS_CreatePath (name);

	// allocate buffer for 4 color components (required for ResampleTexture()
	byte *buffer = new byte [vid_width * vid_height * 4];
	// read frame buffer data
	glReadPixels (0, 0, vid_width, vid_height, GL_RGBA, GL_UNSIGNED_BYTE, buffer);

/*
	if (!buffer[2000] && !buffer[2001] && !buffer[2002] && !buffer[2004] && !buffer[2005] && !buffer[2006]) Com_WPrintf("BLACK!\n");
	for (i = 2000; i < 2240; i++) Com_Printf("%02X  ", buffer[i]);
	Com_Printf ("\n");
*/

	if (screenshotFlags & SHOT_SMALL)
	{
		byte *buffer2 = new byte [LEVELSHOT_W * LEVELSHOT_H * 4];
		ResampleTexture ((unsigned *)buffer, vid_width, vid_height, (unsigned *)buffer2, LEVELSHOT_W, LEVELSHOT_H);
		// replace "buffer" pointer with a resampled image
		delete buffer;
		buffer = buffer2;
		width = LEVELSHOT_W;
		height = LEVELSHOT_H;
	}
	else
	{
		width = vid_width;
		height = vid_height;
	}
	int size = width * height;

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
			if (screenshotFlags & SHOT_NOGAMMA)
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

	bool result;
	if (screenshotFlags & SHOT_JPEG)
		result = WriteJPG (name, buffer, width, height, (screenshotFlags & SHOT_SMALL));
	else
		result = WriteTGA (name, buffer, width, height);

	delete buffer;

	if (result && !(screenshotFlags & SHOT_SILENT))
		Com_Printf ("Wrote %s\n", strrchr (name, '/') + 1);
}


/*------------------ Init/shutdown --------------------*/


#define DEF_IMG_SIZE	16
#define DLIGHT_SIZE		16

void InitImages ()
{
CVAR_BEGIN(vars)
	CVAR_VAR(gl_showImages, 0, 0),
	CVAR_VAR(r_colorMipLevels, 0, 0),
	CVAR_VAR(gl_picmip, 0, CVAR_ARCHIVE|CVAR_NOUPDATE),
	CVAR_VAR(gl_roundImagesDown, 0, CVAR_ARCHIVE),
	CVAR_VAR(gl_textureBits, 0, CVAR_ARCHIVE|CVAR_NOUPDATE)
CVAR_END
	byte	tex[256*128*4], *p;
	int		texnum, x, y;
	image_t	*image;

	guard(InitImages);

	Cvar_GetVars (ARRAY_ARG(vars));
	imageCount = 0;
	memset (hashTable, 0, sizeof(hashTable));
	memset (imagesArray, 0, sizeof(imagesArray));
	for (texnum = 0, image = &imagesArray[0]; texnum < MAX_TEXTURES; texnum++, image++)
		image->texnum = texnum;

	SetupGamma ();		// before texture creation
	GetPalette ();		// read palette for 8-bit WAL textures

	RegisterCommand ("imagelist", Imagelist_f);
	RegisterCommand ("img_reload", ImageReload_f);

	/*--------- create default texture -----------*/
	memset (tex, 0, DEF_IMG_SIZE*DEF_IMG_SIZE*4);
	for (y = 0, p = &tex[0]; y < DEF_IMG_SIZE; y++)
		for (x = 0; x < DEF_IMG_SIZE; x++, p += 4)
			if (x < 2 || x >= DEF_IMG_SIZE-2 || y < 2 || y >= DEF_IMG_SIZE-2)
				p[0] = p[1] = p[2] = p[3] = 255;
	gl_defaultImage = CreateImage ("*default", tex, DEF_IMG_SIZE, DEF_IMG_SIZE, IMAGE_MIPMAP);
	gl_defaultImage->flags |= IMAGE_SYSTEM;

	/*---------- create video image ------------*/
	memset (tex, 255, 16*16*4);
	if (!GL_SUPPORT(QGL_NV_TEXTURE_RECTANGLE))
		gl_videoImage = CreateImage ("*video", tex, 16, 16, IMAGE_CLAMP);
	else	// without this, uploading video as rect texture will damage other texture:
		gl_videoImage = CreateImage ("*video", tex, 10, 5, IMAGE_CLAMP);
	gl_videoImage->flags |= IMAGE_SYSTEM;

	/*------ create identity light image ---------*/
#if 0
	y = gl_config.identityLightValue;
	for (x = 0, p = &tex[0]; x < 8*8; x++, p += 4)
	{
		p[0] = p[1] = p[2] = y;
		p[3] = 255;
	}
	gl_identityLightImage = CreateImage ("*identityLight", tex, 8, 8, IMAGE_MIPMAP);
#else
	tex[0] = tex[1] = tex[2] = gl_config.identityLightValue;
	tex[3] = 255;
	gl_identityLightImage = CreateImage ("*identityLight", tex, 1, 1, IMAGE_MIPMAP);
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
			v = appRound (255 * (1 - (sqrt (xv + yv) + 1) / (DLIGHT_SIZE/2)));		// linear
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
	gl_dlightImage = CreateImage ("*dlight", tex, 16, 16, IMAGE_CLAMP|IMAGE_TRUECOLOR);
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
	gl_particleImage = CreateImage ("*particle", tex, 8, 8, IMAGE_CLAMP|IMAGE_MIPMAP);
	gl_particleImage->flags |= IMAGE_SYSTEM;

	/*--------- create detail texture ------------*/
	// compute float intensity with avg=1
#define DETAIL_SIDE		128
#define DETAIL_DIFFUSE	0.1		// Cvar_VariableValue("det")
	float ftex[DETAIL_SIDE*DETAIL_SIDE];
	float *fp = ftex;
	float fsum = 0;
	for (x = 0; x < ARRAY_COUNT(ftex); x++)
	{
		float rnd = 1 + crand() * DETAIL_DIFFUSE;
		*fp++ = rnd;
		fsum += rnd;
	}
	// compute multiplier for adjusting average color to 1 + for converting to integer
	fsum = 1.0f / (fsum / (DETAIL_SIDE*DETAIL_SIDE)) * 128;
	fp = ftex;
	p = tex;
	for (x = 0; x < ARRAY_COUNT(ftex); x++)
	{
		byte b = appRound (*fp++ * fsum);
		*p++ = b; *p++ = b; *p++ = b;		// rgb (use luminance or alpha only ?)
		*p++ = 255;							// alpha
	}
	// fill tex
	CreateImage ("*detail", tex, DETAIL_SIDE, DETAIL_SIDE, IMAGE_MIPMAP);

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
	gl_fogImage = CreateImage ("*fog", tex, 256, 32, IMAGE_CLAMP|IMAGE_TRUECOLOR);	//?? mipmap
	gl_fogImage->flags |= IMAGE_SYSTEM; */

	gl_reflImage = FindImage ("fx/specular", IMAGE_MIPMAP);	//!! move reflection images to a different place
	gl_reflImage2 = FindImage ("fx/diffuse", IMAGE_MIPMAP);

	unguard;
}


void ShutdownImages ()
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
			delete img->pic;
			img->pic = NULL;
		}
	}
	imageCount = 0;

	// don't clear other fields: after GL_ShutdownImages should not be called nothing
	// but GL_InitImages, which will perform this work
}


void ShowImages ()
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
		if (appMatchWildcard (name, mask, true)) numImg++;
	}
	if (!numImg) return;		// no matched images

	// setup OpenGL
	GL_Set2DMode ();
	GL_SetMultitexture (1);
	GL_State (BLEND(S_ALPHA,M_S_ALPHA)|GLSTATE_NODEPTHTEST);
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
	dx = vid_width / nx;
	dy = vid_height / ny;
	num = numImg;
	img = &imagesArray[0];
	for (y = 0; y < ny && num; y++)
	{
		float	y0, x0, s, t;

		y0 = y * dy;
		for (x = 0; x < nx; x++)
		{
			if (!num) break;

			while (true)
			{
				name = img->name;
				if (name[0] && appMatchWildcard (name, mask, true)) break;
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

// FindImage -- main image creating/loading function
image_t *FindImage (const char *name, unsigned flags)
{
	char	name2[MAX_QPATH], *s;
	int		width, height, len;
	image_t	*img;
	byte	*pic;

	if (!name[0])
		return gl_defaultImage;		// NULL name -- NULL image

	appCopyFilename (name2, name, sizeof(name2));

	/*------- find image using hash table --------*/
	int hash = ComputeHash (name2);
//	Com_Printf ("FindImage(%s): hash = %X\n", name2, hash);//!!

	// find extension
	s = strrchr (name2, '.');
	if (s)
	{
		// found "." - check for supported image extensions
		if (strcmp (s, ".pcx") && strcmp (s, ".wal") &&
			strcmp (s, ".jpg") && strcmp (s, ".tga"))
			s = NULL;				// this is not an image extension - try to add extension
	}
	if (!s)
		s = strchr (name2, 0);		// == &name2[strlen(name2)]; points to a place, where extension will be added
	len = s - name2;				// length of name without extension

	unsigned flags2 = flags & (IMAGE_MIPMAP|IMAGE_CLAMP);
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
	int prefFmt = 0;
	if (!strcmp (s, ".tga"))
		prefFmt = IMAGE_TGA;
	else if (!strcmp (s, ".jpg"))
		prefFmt = IMAGE_JPG;
//	else if (!strcmp (s, ".pcx"))	//-- commented - never prefer 8-bit textures
//		prefFmt = IMAGE_PCX;
//	else if (!strcmp (s, ".wal"))
//		prefFmt = IMAGE_WAL;

	*s = 0; // cut extension
	int fmt = ImageExists (name2, IMAGE_32BIT);
	if (fmt & prefFmt)
		fmt = prefFmt;				// restrict loading to this image type

	// load image within a preferred (or prioritized) format
	if (fmt & IMAGE_TGA)
	{
		strcpy (s, ".tga");
START_PROFILE2(img::tga, name)
		LoadTGA (name2, pic, width, height);
END_PROFILE
	}
	else if (fmt & IMAGE_JPG)
	{
		strcpy (s, ".jpg");
START_PROFILE2(img::jpg, name)
		LoadJPG (name2, pic, width, height);
END_PROFILE
	}
	else if (fmt & IMAGE_PCX)
	{
		byte	*pic8, *palette;

		strcpy (s, ".pcx");
START_PROFILE2(img::pcx, name)
		LoadPCX (name2, pic8, palette, width, height);
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
START_PROFILE2(img::wal, name)
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
		img = CreateImage (name2, pic, width, height, flags);
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


} // namespace
