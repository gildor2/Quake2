#include "gl_local.h"
#include "gl_model.h"			// for accessing to some map info

image_t		*gl_defaultImage;
image_t		*gl_whiteImage;
image_t		*gl_identityLightImage;
image_t		*gl_dlightImage;
image_t		*gl_fogImage;
image_t		*gl_scratchImage;

int		gl_filter_min = GL_LINEAR_MIPMAP_NEAREST;
int		gl_filter_max = GL_LINEAR;

#define HASH_BITS		8
#define HASH_SIZE		(1 << HASH_BITS)
#define HASH_MASK		(HASH_SIZE - 1)

#define	MAX_TEXTURES	1024

static image_t	imagesArray[MAX_TEXTURES];	// no dynamic allocation (array size < 100K - small enough)
static image_t	*hashTable[HASH_SIZE];
static int		imageCount;

/*
 * gammaTable is used for 2 purposes:
 *  1. when gamma is hardware -- for lightscaling screenshots
 *  2. when gamma is software -- for lightscaling uploadable textures
 */
static byte		gammaTable[256], intensGammaTable[256];

//!! Create test function to check hash distrubution
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
	int		i, width, height;
	byte	*pic, *pal, *p;

	// get the palette
	LoadPCX ("pics/colormap.pcx", &pic, &pal, &width, &height);
	if (!pal)
		Com_Error (ERR_FATAL, "Couldn't load pics/colormap.pcx");

	p = pal;
	for (i = 0; i < 256; i++, p += 3)
	{
		unsigned	v;

		v = (p[0]<<0) + (p[1]<<8) + (p[2]<<16) + (255<<24);	// R G B A
		gl_config.tbl_8to32[i] = LittleLong(v);
	}
	gl_config.tbl_8to32[255] &= LittleLong(0x00FFFFFF);	// 255 is transparent (alpha = 0)

	// free image
	Z_Free (pic);
	Z_Free (pal);
}


static byte *Convert8to32bit (byte *in, int width, int height, unsigned *palette)
{
	int		i, size;
	byte	*out;
	unsigned *p;

	if (!palette)
		palette = gl_config.tbl_8to32;

	size = width * height;
	out = Z_Malloc (size * 4);

	p = (unsigned*)out;
	for (i = 0; i < size; i++)
		*p++ = palette[*in++];

	return out;
}


//----------------------------------------------------------------------

void GL_Bind (image_t *tex)
{
	int		tmu, old, h, h1;

	tmu = gl_state.currentTmu;

	if (!tex)
	{
		qglDisable (GL_TEXTURE_2D);
		h = -1;
	}
	else
	{
		old = gl_state.currentBinds[tmu];
		h = tex->texnum;

		if (old == -1)
			qglEnable (GL_TEXTURE_2D);

		if (gl_nobind->integer && (h1 = gl_dlightImage->texnum))
		{
			if (gl_nobind->integer != 2 || strcmp (tex->name, "pics/conchars.pcx"))	//?? need better way to detect force-bind images
				h = h1;
		}

		if (h == old) return;

		qglBindTexture (GL_TEXTURE_2D, h);
		gl_speeds.numBinds++;
	}

	gl_state.currentBinds[tmu] = h;
}


// Bind image even if nobind active (i.e. for uploading image)
static void GL_BindForce (image_t *tex)
{
	int		tmu, old, h;

	tmu = gl_state.currentTmu;
	old = gl_state.currentBinds[tmu];
	h = tex->texnum;
	if (old == h) return;

	if (old == -1)
		qglEnable (GL_TEXTURE_2D);

	qglBindTexture (GL_TEXTURE_2D, h);
	gl_speeds.numBinds++;
	gl_state.currentBinds[tmu] = h;
}


void GL_SelectTexture (int tmu)
{
	int		tex;

	if (tmu == gl_state.currentTmu)
		return;

//	if (tmu > 1) else
//		Com_Error (ERR_FATAL, "GL_SelectTexture: unit = %08X", tmu);

	if (GL_SUPPORT(QGL_ARB_MULTITEXTURE))
	{	// ARB_multitexture
		tex = tmu ? GL_TEXTURE1_ARB : GL_TEXTURE0_ARB;
		qglActiveTextureARB (tex);
		qglClientActiveTextureARB (tex);
	}
	else
	{
		// SGIS_multitexture
		tex = tmu ? GL_TEXTURE1_SGIS : GL_TEXTURE0_SGIS;
		qglSelectTextureSGIS (tex);
	}

	gl_state.currentTmu = tmu;
}


void GL_CullFace (gl_cullMode_t mode)
{
	if (gl_state.currentCullMode == mode)
		return;

	//?? add inverseCullMode for portals (mirrors)
	if (mode == CULL_NONE)
		qglDisable (GL_CULL_FACE);
	else
	{
		qglEnable (GL_CULL_FACE);
		if (mode == CULL_FRONT)
			qglCullFace (GL_FRONT);
		else
			qglCullFace (GL_BACK);
	}

	gl_state.currentCullMode = mode;
}


void GL_TexEnv (int env)
{
	int		tmu;

	tmu = gl_state.currentTmu;
	if (gl_state.currentEnv[tmu] == env)
		return;

	if (env == GL_REPLACE || env == GL_MODULATE || env == GL_ADD || env == GL_DECAL)
		qglTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, env);
	else
		Com_Error (ERR_FATAL, "GL_TexEnv: env = %08X", env);

	gl_state.currentEnv[tmu] = env;
}


void GL_State (int state)
{
	int		dif;

	static int src_blends[] = {
		GL_ZERO, GL_ONE, GL_DST_COLOR,
		GL_ONE_MINUS_DST_COLOR,
		GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
		GL_DST_ALPHA, GL_ONE_MINUS_DST_ALPHA,
		GL_SRC_ALPHA_SATURATE
	};

	static int dst_blends[] = {
		GL_ZERO, GL_ONE,
		GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR,
		GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
		GL_DST_ALPHA, GL_ONE_MINUS_DST_ALPHA
	};

	dif = state ^ gl_state.currentState;
	if (!dif)
		return;

	if (dif & (GLSTATE_SRCMASK|GLSTATE_DSTMASK))
	{
		if (state & (GLSTATE_SRCMASK|GLSTATE_DSTMASK))
		{
			unsigned src, dst;

			src = ((state & GLSTATE_SRCMASK) - GLSTATE_SRC_ZERO) >> GLSTATE_SRCSHIFT;
			if (src >= sizeof(src_blends)/sizeof(int))
				Com_Error (ERR_FATAL, "GL_State: invalid src blend %08X", state);
			src = src_blends[src];

			dst = ((state & GLSTATE_DSTMASK) - GLSTATE_DST_ZERO) >> GLSTATE_DSTSHIFT;
			if (dst >= sizeof(dst_blends)/sizeof(int))
				Com_Error (ERR_FATAL, "GL_State: invalid dst blend %08X", state);
			dst = dst_blends[dst];

			qglEnable (GL_BLEND);
			qglBlendFunc (src, dst);
		}
		else
			qglDisable (GL_BLEND);
	}

	if (dif & GLSTATE_ALPHAMASK)
	{
		int		m;

		m = state & GLSTATE_ALPHAMASK;
		if (!m)
			qglDisable (GL_ALPHA_TEST);
		else
		{
			qglEnable (GL_ALPHA_TEST);
			if (m == GLSTATE_ALPHA_GT0)
				qglAlphaFunc (GL_GREATER, 0.0f);
			else if (m == GLSTATE_ALPHA_LT05)
				qglAlphaFunc (GL_LESS, 0.5f);
			else // if (m == GLSTATE_ALPHA_GE05)
				qglAlphaFunc (GL_GEQUAL, 0.5f);
		}
	}

	if (dif & GLSTATE_DEPTHWRITE)
	{
		GLboolean	m;

		m = state & GLSTATE_DEPTHWRITE ? 1 : 0;
		qglDepthMask (m);
	}

	if (dif & GLSTATE_NODEPTHTEST)
	{
		if (state & GLSTATE_NODEPTHTEST)
			qglDisable (GL_DEPTH_TEST);
		else
			qglEnable (GL_DEPTH_TEST);
	}

	if (dif & GLSTATE_DEPTHEQUALFUNC)
	{
		int		m;

		m = state & GLSTATE_DEPTHEQUALFUNC ? GL_EQUAL : GL_LEQUAL;
		qglDepthFunc (m);
	}

	if (dif & GLSTATE_POLYGON_LINE)
	{
		int		m;

		m = state & GLSTATE_POLYGON_LINE ? GL_LINE : GL_FILL;
		qglPolygonMode (GL_FRONT_AND_BACK, m);
	}

	gl_state.currentState = state;
}


//----------------------------------------------------------------------

typedef struct
{
	char	*name;
	float	minimize, maximize;
} glmode_t;

#define NUM_GL_MODES 6

static glmode_t texModes[] = {
	{"GL_NEAREST", GL_NEAREST, GL_NEAREST},
	{"GL_LINEAR", GL_LINEAR, GL_LINEAR},
	{"GL_NEAREST_MIPMAP_NEAREST", GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST},
	{"GL_LINEAR_MIPMAP_NEAREST", GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR},
	{"GL_NEAREST_MIPMAP_LINEAR", GL_NEAREST_MIPMAP_LINEAR, GL_NEAREST},
	{"GL_LINEAR_MIPMAP_LINEAR", GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR}
};


void GL_TextureMode (char *name)
{
	int		i;
	image_t *img;

	for (i = 0; i < NUM_GL_MODES; i++)
	{
		if (!Q_stricmp (texModes[i].name, name))
			break;
	}

	if (i == NUM_GL_MODES)
	{
		Com_Printf ("bad filter name\n");
		return;
	}

	gl_filter_min = texModes[i].minimize;
	gl_filter_max = texModes[i].maximize;

	// change all the existing mipmap texture objects
	for (i = 0, img = &imagesArray[0]; i < MAX_TEXTURES; i++, img++)
	{
		if (!img->name[0]) continue;	// free slot
		if (!(img->flags & IMAGE_MIPMAP)) continue;
		GL_Bind (img);
		qglTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
		qglTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
	}
}


//----------------------------------------------------------------------

static void ResampleTexture (unsigned *in, int inwidth, int inheight, unsigned *out, int outwidth, int outheight)
{
	int			i, j;
	unsigned	*inrow, *inrow2;
	unsigned	frac, fracstep;
	unsigned	p1[1024], p2[1024];
	byte		*pix1, *pix2, *pix3, *pix4;

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

	for (i = 0; i < outheight; i++, out += outwidth)
	{
		inrow = in + inwidth*(int)((0.25f + i) * inheight / outheight);
		inrow2 = in + inwidth*(int)((0.75f + i) * inheight / outheight);
		frac = fracstep >> 1;
		for (j = 0; j < outwidth; j++)
		{
			int		n, r, g, b, a;

			n = r = g = b = a = 0;
#define PROCESS_PIXEL(arr)	\
	if (arr[3])	\
	{	\
		n++;	\
		r += arr[0]; g += arr[1]; b += arr[2]; a += arr[3];	\
	}
			pix1 = (byte *)inrow + p1[j];
			pix2 = (byte *)inrow + p2[j];
			pix3 = (byte *)inrow2 + p1[j];
			pix4 = (byte *)inrow2 + p2[j];
			PROCESS_PIXEL(pix1);
			PROCESS_PIXEL(pix2);
			PROCESS_PIXEL(pix3);
			PROCESS_PIXEL(pix4);
			if (n)
			{
				r /= n; g /= n; b /= n; a /= 4;
			}
			((byte *)(out+j))[0] = r;
			((byte *)(out+j))[1] = g;
			((byte *)(out+j))[2] = b;
			((byte *)(out+j))[3] = a;
#undef PROCESS_PIXEL
		}
	}
}


static void LightScaleTexture (unsigned *pic, int width, int height, qboolean only_gamma)
{
	float	sat;
	int		i, c;
	byte	*p;

	sat = r_saturation->value;
	c = width * height;

	if (sat != 1.0)
	{
		float	r, g, b;
		float	light;

		p = (byte *)pic;
		for (i = 0; i < c; i++, p+=4)
		{
#define SATURATE(c,l,v) c = (l+0.5+(c-l)*v); if (c < 0) c = 0; else if (c > 255) c = 255;
			// get color
			r = p[0];  g = p[1];  b = p[2];
			// change saturation
			light = (r + g + b) / 3.0;
			SATURATE(r,light,sat);
			SATURATE(g,light,sat);
			SATURATE(b,light,sat);
			// put color
			p[0] = r;  p[1] = g;  p[2] = b;
		}
	}

	if (only_gamma)
	{
		if (gl_config.deviceSupportsGamma)
			return;
		else
		{
			p = (byte *)pic;
			for (i = 0; i < c; i++, p+=4)
			{
				p[0] = gammaTable[p[0]];
				p[1] = gammaTable[p[1]];
				p[2] = gammaTable[p[2]];
			}
		}
	}
	else
	{
		p = (byte *)pic;
		for (i = 0; i < c; i++, p+=4)
		{
			p[0] = intensGammaTable[p[0]];
			p[1] = intensGammaTable[p[1]];
			p[2] = intensGammaTable[p[2]];
		}
	}
}


//!! UNFINISHED (needs multitexturing to finish)
static void LightScaleLightmap (unsigned *pic, int width, int height)
{
	byte	*p;
	int		i, c, shift;

	shift = gl_config.overbrightBits;
	/*!! should lightscale lightmap when:
		1. multitex & !envCombine & !vertexLight -- * (mapOverbright-overbright)
		2. can work without overbright, but overbright forced: / (mapOverbright-overbright)
	 */
	/*------- downscale lightmap -----------*/
	if (!shift)
		return;	// no overbright

	p = (byte *)pic;
	c = width * height;
	for (i = 0; i < c; i++, p += 4)
	{
		p[0] >>= shift;
		p[1] >>= shift;
		p[2] >>= shift;
	}
}


/*
================
GL_MipMap

Operates in place, quartering the size of the texture
================
*/

#ifndef MAX
#define MAX(a,b) ((a)>=(b)?(a):(b))
#endif

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
		if (in[idx+3] > am) am = in[idx+3];	\
	}
			PROCESS_PIXEL(0);
			PROCESS_PIXEL(4);
			PROCESS_PIXEL(width);
			PROCESS_PIXEL(width+4);
#undef PROCESS_PIXEL
			if (n)
			{
				r /= n; g /= n; b /= n; a /= 4;
			}
			else
			{	// don't allow transparent textures be black (in a case of non-transparent its usage)
#define PROCESS_PIXEL(idx)	\
		r += in[idx]; g += in[idx+1]; b += in[idx+2];
				PROCESS_PIXEL(0);
				PROCESS_PIXEL(4);
				PROCESS_PIXEL(width);
				PROCESS_PIXEL(width+4);
#undef PROCESS_PIXEL
				r /= 4; g /= 4; b /= 4;
			}
			out[0] = r; out[1] = g; out[2] = b;
			// generate alpha-channel for mipmaps (don't let it be transparent)
			// dest alpha = (MAX(a[0]..a[3]) + AVG(a[0]..a[3])) / 2
			// if alpha = 255 or 0 (for all 4 points) -- it will holds its value
			out[3] = (am + a) / 2;
		}
	}
}


static void GetImageDimensions (int width, int height, int *scaledWidth, int *scaledHeight, qboolean picmip)
{
	int		sw, sh;

	for (sw = 1; sw < width; sw <<= 1) ;
	for (sh = 1; sh < height; sh <<= 1) ;

	if (gl_roundImagesDown->integer)
	{	// scale down only if image dimension is larger than 64 and
		// larger than 4/3 of original image dimension
		if (sw > 64 && sw > (width * 4 / 3)) sw >>= 1;
		if (sh > 64 && sh > (height * 4 / 3)) sh >>= 1;
	}
	if (picmip)
	{	// perform picmip only for large images
		if (sw > 64) sw >>= gl_picmip->integer;
		if (sh > 64) sh >>= gl_picmip->integer;
	}

	while (sw > gl_config.maxTextureSize) sw >>= 1;
	while (sh > gl_config.maxTextureSize) sh >>= 1;

	if (sw < 1) sw = 1;
	if (sh < 1) sh = 1;

	*scaledWidth = sw;
	*scaledHeight = sh;
}


// We need to pass "flags" because image->flags is masked with IMAGE_FLAGMASK
static void Upload (void *pic, int flags, image_t *image)
{
	int		scaledWidth, scaledHeight;
	int		format, size;
	unsigned *scaledPic;

	/*----- Calculate internal dimensions of the new texture --------*/
	GetImageDimensions (image->width, image->height, &scaledWidth, &scaledHeight, image->flags & IMAGE_PICMIP);

	image->internalWidth = scaledWidth;
	image->internalHeight = scaledHeight;

	/*---------------- Resample/lightscale texture ------------------*/
	size = scaledWidth * scaledHeight * 4;
	scaledPic = Z_Malloc (size);
	if (image->width != scaledWidth || image->height != scaledHeight)
		ResampleTexture (pic, image->width, image->height, scaledPic, scaledWidth, scaledHeight);
	else
		memcpy (scaledPic, pic, size);

	if (!(flags & IMAGE_LIGHTMAP))
		// scale gamma only for non-wall textures
		LightScaleTexture (scaledPic, scaledWidth, scaledHeight, !(flags & IMAGE_MIPMAP));
	else
		LightScaleLightmap (scaledPic, scaledWidth, scaledHeight);			// lightmap overbright

	/*------------- Determine texture format to upload --------------*/
	if (flags & IMAGE_LIGHTMAP)
		format = 3;
	else if (flags & IMAGE_TRUECOLOR)
		format = GL_RGBA8;
	else
	{
		int		alpha, i, n;
		byte	*scan, a;
		// Check for alpha channel in image
		alpha = 0;
		n = scaledWidth * scaledHeight;
		scan = (byte *)scaledPic + 3;
		for (i = 0; i < n; i++, scan += 4)
		{
			a = *scan;
			if (a == 255) continue; // opaque
			if (a == 0)
			{
				alpha = 1;	// have alpha == 0 (1 bit alpha)
				continue;
			}
			alpha = 2;		// have alpha == [1..254] (8 bit alpha)
			break;
		}

		// select texture format
		format = 0;
		if (alpha && (flags & IMAGE_MIPMAP))
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
		if (!format || flags & IMAGE_NOCOMPRESS)
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
				if (alpha)
					format = GL_RGBA8;
				else
					format = GL_RGB8;
				break;
			default:	// 0
				if (alpha)
					format = 4;
				else
					format = 3;
			}
		}
	}

	image->internalFormat = format;

	/*------------------ Upload the image ---------------------------*/
	qglTexImage2D (GL_TEXTURE_2D, 0, format, scaledWidth, scaledHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaledPic);
	if (!(flags & IMAGE_MIPMAP))
	{
		qglTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		qglTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}
	else
	{
		int		miplevel;

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
				int		i, r, g, b;
				byte	*p;

				// here miplevel >= 1
				r = g = b = 0;
				switch (miplevel % 3)
				{
				case 1:
					r = 255;
					break;
				case 2:
					g = 255;
					break;
				default:	// case 2
					b = 255;
				}
				p = (byte *) scaledPic;
				for (i = 0; i < size; i++, p += 4)
				{
					p[0] = (r + p[0]) / 4;
					p[1] = (g + p[1]) / 4;
					p[2] = (b + p[2]) / 4;
				}
			}
			qglTexImage2D (GL_TEXTURE_2D, miplevel, format, scaledWidth, scaledHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaledPic);
		}
		qglTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
		qglTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
	}

	Z_Free (scaledPic);
}


image_t *GL_CreateImage (char *name, void *pic, int width, int height, int flags)
{
	char	name2[MAX_QPATH];
	int		texnum, hash;
	image_t	*image;

	if (!name[0])
		Com_Error (ERR_FATAL, "R_CreateImage: null name");

	if (imageCount == MAX_TEXTURES)
	{
		Com_WPrintf ("R_CreateImage(%s): MAX_TEXTURES hit\n", name);
		return gl_defaultImage;
	}

//	Com_Printf ("CreateImage(%s)\n", name);//!!!!

//	if (strlen (name) >= MAX_QPATH)
//		Com_Error (ERR_FATAL, "R_CreateImage: name \"%s\" is too long", name);
	Q_CopyFilename (name2, name, sizeof(name2)-1);

	// allocate free image slot
	for (texnum = 0, image = &imagesArray[0]; texnum < MAX_TEXTURES; texnum++, image++)
		if (!image->name[0]) break;		// found unused slot
	imageCount++;

	// setup image_t fields
	image->texnum = texnum;
	strcpy (image->name, name2);
	image->width = width;
	image->height = height;
	image->flags = flags & IMAGE_FLAGMASK;

	// upload image
	GL_BindForce (image);
	Upload (pic, flags, image);
	if (flags & IMAGE_CLAMP)
	{
		qglTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		qglTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	}
	else
	{
		qglTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		qglTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}

	// insert image into a hash table
	hash = ComputeHash (name2);
	image->hashNext = hashTable[hash];
	hashTable[hash] = image;

	return image;
}


/*-------------------- Video support ----------------------*/


static unsigned rawPalette[256];

void GL_SetRawPalette (const unsigned char *palette)
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
//	else	//?? try to remove
//		memcpy (rawPalette, gl_config.tbl_8to32, sizeof(rawPalette));
}


void GL_DrawStretchRaw (int x, int y, int w, int h, int width, int height, byte *pic)
{
#if 0
	byte	*pic32;

	GL_BindForce (gl_scratchImage);
	// convert 8 bit -> 32 bit
	pic32 = Convert8to32bit (pic, width, height, rawPalette);
	// upload
	Upload (pic32, width, height, false, false, false, gl_scratchImage);
	Z_Free (pic32);
#else
	int		scaledWidth, scaledHeight, i;
	unsigned scaledPic[256*256];
	image_t	*image;
	float	hScale;

	/*?? we can do uploading pic without scaling using TexSubImage onto a large texture and using
	 * (0..width/large_width) - (0..height/large_height) coords when drawing
	 */
	GetImageDimensions (width, height, &scaledWidth, &scaledHeight, false);
	if (scaledWidth > 256) scaledWidth = 256;
	if (scaledHeight > 256) scaledHeight = 256;

	/*--- fast resampling with conversion 8->32 bit ---*/
	hScale = (float) height / scaledHeight;
	for (i = 0; i < scaledHeight; i++)
	{
		int		frac, fracStep, row, j;
		byte	*src;
		unsigned *dst;

		row = (int)(hScale * i);
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
	image = gl_scratchImage;
	GL_BindForce (image);

	image->internalFormat = 3;
	if (image->internalWidth == scaledWidth && image->internalHeight == scaledHeight)
	{	// TexSubImage()
		qglTexSubImage2D (GL_TEXTURE_2D, 0, 0, 0, scaledWidth, scaledHeight, GL_RGBA, GL_UNSIGNED_BYTE, scaledPic);
	}
	else
	{	// TexImage()
		image->internalWidth = scaledWidth;
		image->internalHeight = scaledHeight;
		qglTexImage2D (GL_TEXTURE_2D, 0, image->internalFormat, scaledWidth, scaledHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaledPic);
	}
	// set texture parameters
	qglTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	qglTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	qglTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	qglTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	gl_speeds.numUploads++;

	width = scaledWidth;	//!!!
	height = scaledHeight;
#endif
	/*--------------------- draw ----------------------*/
	GL_Set2DMode ();
	qglColor3f (gl_config.identityLightValue_f, gl_config.identityLightValue_f, gl_config.identityLightValue_f);
	qglBegin (GL_QUADS);
	qglTexCoord2f (0.5f / width, 0.5f / height);
	qglVertex2f (x, y);
	qglTexCoord2f (1.0f - 0.5f / width, 0.5f / height);
	qglVertex2f (x + w, y);
	qglTexCoord2f (1.0f - 0.5f / width, 1.0f - 0.5f / height);
	qglVertex2f (x + w, y + h);
	qglTexCoord2f (0.5f / width, 1.0f - 0.5f / height);
	qglVertex2f (x, y + h);
	qglEnd ();
}


static void GL_FreeImage (image_t *image)
{
	int		hash;
	image_t	*img;

	if (!image->name)
		return;		// already ...
	if (image->flags & IMAGE_SYSTEM)
		return;		// system image - don't free it

	// delete gl texture
	qglDeleteTextures (1, &image->texnum);

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
	image->hashNext = 0;
	imageCount--;
}


//-------------------------------------------------------------------


void GL_SetupGamma (void)
{
	unsigned	overbright;
	int			i;
	float		invGamma, intens;

	gl_config.vertexLight = gl_vertexLight->integer;
	gl_config.deviceSupportsGamma = GLimp_HasGamma ();

	overbright = gl_overBrightBits->integer;
	if (!gl_state.fullscreen || !gl_config.deviceSupportsGamma) overbright = 0;

	if (gl_state.colorBits <= 16)
	{
		if (overbright > 1)
			overbright = 1;
	}
	else
		if (overbright > 2)
			overbright = 2;

	gl_config.overbrightBits = overbright;
	gl_config.identityLightValue = 255 / (1 << overbright);
	gl_config.identityLightValue_f = 1.0 / (float)(1 << overbright);

	invGamma = 1.0 / Cvar_Clamp (r_gamma, 0.5, 3.0);
	intens = Cvar_Clamp (r_intensity, 0, 1);

	if (gl_config.deviceSupportsGamma)
		GLimp_SetGamma (r_gamma->value, 1);

	for (i = 0; i < 256; i++)
	{
		int		v;

		if (invGamma == 1.0)
			v = i;
		else
			v = (int) (pow (i / 255.0f, invGamma) * 255.0f + 0.5f);

		v <<= overbright;
		if (v < 0) v = 0;
		else if (v > 255) v = 255;
		gammaTable[i] = v;
	}

	for (i = 0; i < 256; i++)
	{
		int		v;

		v = (int) (intens * (float)i + 0.5f);

		if (v < 0) v = 0;
		else if (v > 255) v = 255;

		if (!gl_config.deviceSupportsGamma)
			v = gammaTable[v];

		intensGammaTable[i] = v;
	}
}


/*----------------- Imagelist -----------------*/


typedef struct
{
	int		fmt;
	char	*name;
} fmtInfo_t;

#define _STR(s) {s, #s}

static const fmtInfo_t fmtInfo[] =
{
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
	{GL_COMPRESSED_RGB_ARB, "RGB_ARB"},
	{GL_COMPRESSED_RGBA_ARB, "RGBA_ARB"}
};

static void Imagelist_f (void)
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
	static char alphaTypes[3] = {' ', '1', '8'};
	static char boolTypes[2] = {' ', '+'};
	char	*mask;

	if (Cmd_Argc () > 2)
	{
		Com_Printf ("Usaeg: imagelist [<mask>]\n");
		return;
	}

	if (Cmd_Argc () == 2)
		mask = Cmd_Argv (1);
	else
		mask = NULL;

	Com_Printf ("----w---h---a-wr-m-fmt------name----------\n");
	idx = texels = n = 0;

	for (i = 0; i < MAX_TEXTURES; i++)
	{
		char	*fmt;
		image_t	*img;
		int		f, fi;

		img = &imagesArray[i];
		if (!img->name[0]) continue;	// free slot

		idx++;
		if (mask && !MatchWildcard (img->name, mask)) continue;
		n++;

		texels += img->internalWidth * img->internalHeight;

		f = img->internalFormat;
		fmt = "???";
		for (fi = 0; fi < sizeof(fmtInfo)/sizeof(fmtInfo_t); fi++)
			if (fmtInfo[fi].fmt == f)
			{
				fmt = fmtInfo[fi].name;
				break;
			}

		Com_Printf ("%-3d %-3d %-3d %c %c  %c %-8s %s\n", idx, img->internalWidth, img->internalHeight,
			alphaTypes[img->alphaType], boolTypes[(img->flags & IMAGE_CLAMP) == 0], boolTypes[(img->flags & IMAGE_MIPMAP) != 0],
			fmt, img->name);
	}

	Com_Printf ("Total %d images with texel count (not counting mipmaps) %d\n", n, texels);

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


/*-------------------- Screenshots ---------------------*/


#define		LEVELSHOT_W		256
#define		LEVELSHOT_H		256


static void PerformScreenshot (qboolean jpeg)
{
	byte	*buffer, *src, *dst;
	char	name[MAX_OSPATH];
	char	*dir, *ext;
	int		i, width, height, size;
	qboolean levelshot, result;
	FILE	*f;

	levelshot = !stricmp (Cmd_Argv(1), "levelshot");
	if (levelshot)
		dir = "levelshots";
	else
		dir = "screenshots";

	if (jpeg)
		ext = "jpg";
	else
		ext = "tga";

	// create the screenshots directory if it doesn't exist
	Com_sprintf (name, sizeof(name), "%s/%s", FS_Gamedir(), dir);
	Sys_Mkdir (name);

	// find a file name to save it to
	if (levelshot)
	{
		char	mapname[32], *tmp;

		// cur path
		tmp = strrchr (gl_worldModel.name, '/');
		if (!tmp) tmp = gl_worldModel.name;
		strcpy (mapname, tmp);
		// cut extension
		tmp = strrchr (mapname, '.');
		if (tmp)
			*tmp = 0;
		else	// no ".bsp"
		{
			Com_WPrintf ("PerformScreenshot: cannot get mapname\n");
			return;
		}
		// create name
		Com_sprintf (name, sizeof(name), "%s/%s/%s.%s", FS_Gamedir(), dir, mapname, ext);
	}
	else
		for (i = 0; i < 10000; i++)
		{	// check for a free filename
			Com_sprintf (name, sizeof(name), "%s/%s/shot%04d.%s", FS_Gamedir(), dir, i, ext);
			if (!(f = fopen (name, "rb")))
				break;	// file doesn't exist
			fclose (f);
		}

	// allocate buffer for 4 color components (required for ResampleTexture()
	buffer = Z_Malloc (vid.width * vid.height * 4);
	// read frame buffer data
	qglReadPixels (0, 0, vid.width, vid.height, GL_RGBA, GL_UNSIGNED_BYTE, buffer);

	if (levelshot)
	{
		byte *buffer2;

		//?? check: is there map rendered?
		buffer2 = Z_Malloc (LEVELSHOT_W * LEVELSHOT_H * 4);
		ResampleTexture ((unsigned *)buffer, vid.width, vid.height, (unsigned *)buffer2, LEVELSHOT_W, LEVELSHOT_H);
		Z_Free (buffer);
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

	// remove 4th coor component and correct gamma
	src = dst = buffer;
	for (i = 0; i < size; i++)
	{
		byte	r, g, b;

		r = *src++; g = *src++; b = *src++;
		src++;	// skip alpha
		// correct gamma
		if (gl_config.deviceSupportsGamma)
		{
			r = gammaTable[r];
			g = gammaTable[g];
			b = gammaTable[b];
		}
		// put new values back
		*dst++ = r;
		*dst++ = g;
		*dst++ = b;
	}

	if (jpeg)
		result = WriteJPG (name, buffer, width, height, levelshot);
	else
		result = WriteTGA (name, buffer, width, height);

	Z_Free (buffer);

	if (result && strcmp (Cmd_Argv(1), "silent"))
		Com_Printf ("Wrote %s\n", strrchr (name, '/') + 1);
}


static Screenshot_f (void)
{
	PerformScreenshot (false);
}


static ScreenshotJPEG_f (void)
{
	PerformScreenshot (true);
}


/*------------------ Init/shutdown --------------------*/


void GL_InitImages (void)
{
	byte	tex[256*32*4], *p;
	int		x, y;

	imageCount = 0;
	memset (hashTable, 0, sizeof(hashTable));
	memset (imagesArray, 0, sizeof(imagesArray));

	GL_SetupGamma ();	// before texture creation
	GetPalette ();		// read palette for 8-bit WAL textures

	Cmd_AddCommand ("imagelist", Imagelist_f);
	Cmd_AddCommand ("screenshot", Screenshot_f);
	Cmd_AddCommand ("screenshotJPEG", ScreenshotJPEG_f);

	/*--------- create default texture -----------*/
	memset (tex, 0, 16*16*4);
	for (y = 0, p = &tex[0]; y < 16; y++)
		for (x = 0; x < 16; x++, p += 4)
			if (x < 2 || x >= 14 || y < 2 || y >= 14)
				p[0] = p[1] = p[2] = p[3] = 255;
	gl_defaultImage = GL_CreateImage ("*default", tex, 16, 16, IMAGE_MIPMAP);
	gl_defaultImage->flags |= IMAGE_SYSTEM;
	/*----------- create white image -------------*/
	memset (tex, 255, 8*8*4);
	gl_whiteImage = GL_CreateImage ("*white", tex, 8, 8, 0);
	gl_whiteImage->flags |= IMAGE_SYSTEM;
	/*---------- create scratch image ------------*/
	memset (tex, 255, 16*16*4);
	gl_scratchImage = GL_CreateImage ("*scratch", tex, 16, 16, 0);
	gl_scratchImage->flags |= IMAGE_SYSTEM;
	/*------ create identity light image ---------*/
	y = gl_config.identityLightValue;
	for (x = 0, p = &tex[0]; x < 8*8; x++, p += 4)
	{
		p[0] = p[1] = p[2] = y;
		p[3] = 255;
	}
	gl_identityLightImage = GL_CreateImage ("*identityLight", tex, 8, 8, 0);
	gl_identityLightImage->flags |= IMAGE_SYSTEM;
	/*----------- create dlight image ------------*/
	p = &tex[0];
	for (y = 0; y < 16; y++)
	{
		float	yv;

		yv = (y - 7.5); yv *= yv;
		for (x = 0; x < 16; x++)
		{
			float	xv;
			int		v;

			xv = (x - 7.5); xv *= xv;
			v = (int) (4000.0f / (xv + yv));
			if (v < 75) v = 0;
			if (v > 255) v = 255;
			p[0] = p[1] = p[2] = v;
			p[3] = 255;
			p += 4;
		}
	}
	gl_dlightImage = GL_CreateImage ("*dlight", tex, 16, 16, IMAGE_CLAMP|IMAGE_TRUECOLOR);
	gl_dlightImage->flags |= IMAGE_SYSTEM;
	/*------------ create fog image --------------*/
	p = &tex[0];
	for (y = 0; y < 32; y++)
	{
		float	yf;

		yf = ((float)y + 0.5) / 32;
		for (x = 0; x < 256; x++)
		{
			float	xf, v;

			xf = ((float)x + 0.5) / 256.0 - 1.0/512.0;
			if (xf < 0 || yf < 1.0/32.0)
				v = 0;
			else
			{
				if (yf < 31.0/32.0)
					v = (yf - 1.0/32.0) * xf * 16/15;
				else
					v = xf;

				v *= 8;
				if (v > 1) v = 1;
				v = sqrt (v);
			}

			p[0] = p[1] = p[2] = 255;
			p[3] = (int)(v * 255);
			p += 4;
		}
	}
	gl_fogImage = GL_CreateImage ("*fog", tex, 256, 32, IMAGE_CLAMP|IMAGE_TRUECOLOR);
	gl_fogImage->flags |= IMAGE_SYSTEM;
}


void GL_ShutdownImages (void)
{
	int		i;
	image_t	*img;

	Cmd_RemoveCommand ("imagelist");
	Cmd_RemoveCommand ("screenshot");
	Cmd_RemoveCommand ("screenshotJPEG");

	if (!imageCount) return;

	for (i = 0, img = &imagesArray[0]; i < MAX_TEXTURES; i++, img++)
	{
		if (!img->name[0]) continue;	// free slot
		qglDeleteTextures (1, &img->texnum);
	}
	imageCount = 0;

	// clear binds
	if (gl_config.maxActiveTextures > 1)
	{
		GL_SelectTexture (1);
		qglBindTexture (GL_TEXTURE_2D, 0);
		GL_SelectTexture (0);
	}
	qglBindTexture (GL_TEXTURE_2D, 0);
	// don't clear other fields: after GL_ShutdownImages should not be called nothing
	// but GL_InitImages, which will perform this work
}


void GL_ShowImages (void)
{
	int		i, nx, ny, x, y, num;
	image_t	*img;
	float	x0, y0, dx, dy;

	GL_Set2DMode ();
	GL_State (GLSTATE_SRC_SRCALPHA|GLSTATE_DST_ONEMINUSSRCALPHA|GLSTATE_NODEPTHTEST);
	qglClear (GL_COLOR_BUFFER_BIT);
	qglColor3f (1, 1, 1);
//	qglFinish ();

	for (i = 1; i < 10; i++)
	{
		nx = i * 4;
		ny = i * 3;
		if (imageCount <= nx*ny) break;
	}

	dx = vid.width / nx;
	dy = vid.height / ny;
	num = imageCount;
	img = &imagesArray[0];
	for (y = 0; y < ny; y++)
	{
		if (!num) break;

		y0 = y * dy;
		for (x = 0; x < nx; x++)
		{
			if (!num) break;

			while (!img->name[0]) img++;

			x0 = x * dx;

			GL_BindForce (img);
			qglBegin (GL_QUADS);
			qglTexCoord2f (0, 0);
			qglVertex2f (x0, y0);
			qglTexCoord2f (1, 0);
			qglVertex2f (x0 + dx, y0);
			qglTexCoord2f (1, 1);
			qglVertex2f (x0 + dx, y0 + dy);
			qglTexCoord2f (0, 1);
			qglVertex2f (x0, y0 + dy);
			qglEnd ();

			num--;
			img++;
		}
	}
}

//------------- Loading images --------------------------

// GL_FindImage -- main image creating/loading function
image_t *GL_FindImage (char *name,int flags)
{
	char	name2[MAX_QPATH], *s;
	int		hash, flags2, fmt, prefFmt, width, height, len;
	image_t	*img;
	byte	*pic;

	if (!name[0])
		return gl_defaultImage;		// NULL name -- NULL image

	Q_CopyFilename (name2, name, sizeof(name2)-1);

	/*------- find image using hash table --------*/
	hash = ComputeHash (name2);
//	Com_Printf ("FindImage(%s): hash = %X\n", name2, hash);//!!

	// find extension
	s = strrchr (name2, '.');
	if (!s)
		s = strchr (name2, 0);		// == &name2[strlen(name2)]; points to a place, where extension will be added
	len = s - name2;				// length of name without extension

	flags2 = flags & (IMAGE_MIPMAP|IMAGE_CLAMP);
	for (img = hashTable[hash]; img; img = img->hashNext)
		if (!Q_strncasecmp (img->name, name2, len) &&		// len chars matched
			(!img->name[len] || img->name[len] == '.'))		// rest of img->name if null or extension
		{
			// found a name ...
			// compare image flags
			if ((img->flags & (IMAGE_MIPMAP|IMAGE_CLAMP)) == flags2)
				return img;
//			Com_Printf ("hash: %s  (fl: %X)  --> %s  (fl: %X)", name, flags, img->name, img->flags);//!!!!!
			// flags are different ...
			if (name[0] == '*')
			{
				Com_WPrintf ("R_FindImage(%s): Using different flags (%X != %X) for system image\n", name2, flags, img->flags);
				return img;
			}
			hash = -1;	// mark: image used with a different flags
			// continue search ...
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

	// load image within a preferred (or priorized) format
	if (fmt & IMAGE_TGA)
	{
		strcpy (s, ".tga");
		LoadTGA (name2, &pic, &width, &height);
	}
	else if (fmt & IMAGE_JPG)
	{
		strcpy (s, ".jpg");
		LoadJPG (name2, &pic, &width, &height);
	}
	else if (fmt & IMAGE_PCX)
	{
		byte	*pic8, *palette;

		strcpy (s, ".pcx");
		LoadPCX (name2, &pic8, &palette, &width, &height);
		if (pic8)
		{
			pic = Convert8to32bit (pic8, width, height, NULL);
			Z_Free (pic8);
			Z_Free (palette);
		}
		else
			pic = NULL;
	}
	else if (fmt & IMAGE_WAL)
	{
		miptex_t	*mt;

		strcpy (s, ".wal");
		FS_LoadFile (name2, (void **)&mt);
		if (mt)
		{
			width = LittleLong(mt->width);
			height = LittleLong(mt->height);
			pic = Convert8to32bit ((byte*)mt+LittleLong(mt->offsets[0]), width, height, NULL);
			FS_FreeFile ((void*)mt);
		}
		else
			pic = NULL;
	}
	else
	{
		Com_DPrintf ("Cannot find image %s.*\n", name2);
		return NULL;
	}

	// upload image
	if (pic)
	{
		img = GL_CreateImage (name2, pic, width, height, flags);
		Z_Free (pic);
		return img;
	}
	else
	{
		Com_WPrintf ("R_FindImage(%s): cannot load texture\n", name2);
		return NULL;
	}
}
