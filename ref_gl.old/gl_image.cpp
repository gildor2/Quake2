/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "gl_local.h"

image_t		gltextures[MAX_GLTEXTURES];
int			numgltextures;
int			base_textureid;		// gltextures[i] = base_textureid+i

static byte intensitytable[256];
byte	gammatable[256];

cvar_t		*intensity;

unsigned	d_8to24table[256];

bool GL_Upload8 (byte *data, int width, int height, bool mipmap, bool is_sky );
bool GL_Upload32 (unsigned *data, int width, int height, bool mipmap);


int		gl_solid_format = 3;
int		gl_alpha_format = 4;

int		gl_tex_solid_format = 3;
int		gl_tex_alpha_format = 4;

int		gl_filter_min = GL_LINEAR_MIPMAP_NEAREST;
int		gl_filter_max = GL_LINEAR;

void GL_SetTexturePalette( unsigned palette[256] )
{
/*	int i;
	unsigned char temptable[768];

	if ( glColorTableEXT && gl_ext_palettedtexture->integer )
	{
		for ( i = 0; i < 256; i++ )
		{
			temptable[i*3+0] = ( palette[i] >> 0 ) & 0xff;
			temptable[i*3+1] = ( palette[i] >> 8 ) & 0xff;
			temptable[i*3+2] = ( palette[i] >> 16 ) & 0xff;
		}

		glColorTableEXT( GL_SHARED_TEXTURE_PALETTE_EXT,
		   GL_RGB, 256, GL_RGB, GL_UNSIGNED_BYTE, temptable );
	}
*/
}

void GL_EnableMultitexture( bool enable )
{
	if (!GL_SUPPORT(QGL_ARB_MULTITEXTURE|QGL_SGIS_MULTITEXTURE))
		return;

	if (enable)
	{
		GL_SelectTexture (1);
		glEnable (GL_TEXTURE_2D);
		GL_TexEnv (GL_REPLACE);
	}
	else
	{
		GL_SelectTexture (1);
		glDisable (GL_TEXTURE_2D);
		GL_TexEnv (GL_REPLACE);
	}
	GL_SelectTexture (0);
	GL_TexEnv (GL_REPLACE);
}

void GL_SelectTexture (int tmu)
{
	GLenum	tex;

	if (!GL_SUPPORT(QGL_ARB_MULTITEXTURE|QGL_SGIS_MULTITEXTURE))
		return;

	if (tmu == gl_state.currentTmu)
		return;

//	if (tmu > 1) else
//		Com_FatalError ("GL_SelectTexture: unit = %08X", tmu);

	if (GL_SUPPORT(QGL_ARB_MULTITEXTURE))
	{	// ARB_multitexture
		tex = tmu ? GL_TEXTURE1_ARB : GL_TEXTURE0_ARB;
		glActiveTextureARB (tex);
//		glClientActiveTextureARB (tex);
	}
	else
	{
		// SGIS_multitexture
		tex = tmu ? GL_TEXTURE1_SGIS : GL_TEXTURE0_SGIS;
		glSelectTextureSGIS (tex);
	}

	gl_state.currentTmu = tmu;
}

void qglMTexCoord2f (int tmu, float s, float t)
{
	GLenum tex;

	if (!GL_SUPPORT(QGL_ARB_MULTITEXTURE|QGL_SGIS_MULTITEXTURE))
		Com_FatalError ("MTexCoord call without multitexturing");

	if (GL_SUPPORT(QGL_ARB_MULTITEXTURE))
	{	// ARB_multitexture
		tex = tmu ? GL_TEXTURE1_ARB : GL_TEXTURE0_ARB;
		glMultiTexCoord2fARB (tex, s, t);
	}
	else
	{
		// SGIS_multitexture
		tex = tmu ? GL_TEXTURE1_SGIS : GL_TEXTURE0_SGIS;
		glMultiTexCoord2fSGIS (tex, s, t);
	}
}


void GL_TexEnv( GLenum mode )
{
	static int lastmodes[2] = { -1, -1 };

	if ( mode != lastmodes[gl_state.currentTmu] )
	{
		glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, mode );
		lastmodes[gl_state.currentTmu] = mode;
	}
}

void GL_Bind (int texnum)
{
	extern	image_t	*draw_chars;

	if (gl_nobind->integer && draw_chars)		// performance evaluation option
	{
		if (texnum != draw_chars->texnum)
			texnum = r_notexture->texnum;
//		texnum = draw_chars->texnum;
	}
	if (gl_state.currentBinds[gl_state.currentTmu] == texnum)
		return;
	gl_state.currentBinds[gl_state.currentTmu] = texnum;
	gl_speeds.numBinds++;
	glBindTexture (GL_TEXTURE_2D, texnum);
}

void GL_MBind (int tmu, int texnum)
{
	GL_SelectTexture (tmu);
	if (gl_state.currentBinds[tmu] == texnum) return;
	GL_Bind (texnum);
}

typedef struct
{
	char *name;
	int	minimize, maximize;
} glmode_t;

static glmode_t modes[] = {
	{"GL_NEAREST", GL_NEAREST, GL_NEAREST},
	{"GL_LINEAR", GL_LINEAR, GL_LINEAR},
	{"GL_NEAREST_MIPMAP_NEAREST", GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST},
	{"GL_LINEAR_MIPMAP_NEAREST", GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR},
	{"GL_NEAREST_MIPMAP_LINEAR", GL_NEAREST_MIPMAP_LINEAR, GL_NEAREST},
	{"GL_LINEAR_MIPMAP_LINEAR", GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR}
};

typedef struct
{
	int mode;
	char *name;
} gltmode_t;

#define _STR(s) {s, #s}

static gltmode_t gl_alpha_modes[] = {
	{4, "default"},
	_STR(GL_RGBA),
	_STR(GL_RGBA8),
	_STR(GL_RGB5_A1),
	_STR(GL_RGBA4),
	_STR(GL_RGBA2),
};


gltmode_t gl_solid_modes[] = {
	{3, "default"},
	_STR(GL_RGB),
	_STR(GL_RGB8),
	_STR(GL_RGB5),
	_STR(GL_RGB4),
	_STR(GL_R3_G3_B2),
#ifdef GL_RGB2_EXT
	{GL_RGB2_EXT, "GL_RGB2"},
#endif
};

/*
===============
GL_TextureMode
===============
*/
void GL_TextureMode( char *string )
{
	int		i;
	image_t	*glt;

	for (i=0 ; i< ARRAY_COUNT(modes) ; i++)
	{
		if ( !stricmp( modes[i].name, string ) )
			break;
	}

	if (i == ARRAY_COUNT(modes))
	{
		Com_Printf ("bad filter name\n");
		return;
	}

	gl_filter_min = modes[i].minimize;
	gl_filter_max = modes[i].maximize;

	// change all the existing mipmap texture objects
	for (i=0, glt=gltextures ; i<numgltextures ; i++, glt++)
	{
		if (glt->type != it_pic && glt->type != it_sky && glt->type != it_temp)
		{
			GL_Bind (glt->texnum);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
		}
	}
}

/*
===============
GL_TextureAlphaMode
===============
*/
void GL_TextureAlphaMode( char *string )
{
	int		i;

	for (i=0 ; i< ARRAY_COUNT(gl_alpha_modes) ; i++)
	{
		if ( !stricmp( gl_alpha_modes[i].name, string ) )
			break;
	}

	if (i == ARRAY_COUNT(gl_alpha_modes))
	{
		Com_Printf ("bad alpha texture mode name\n");
		return;
	}

	gl_tex_alpha_format = gl_alpha_modes[i].mode;
}

/*
===============
GL_TextureSolidMode
===============
*/
void GL_TextureSolidMode( char *string )
{
	int		i;

	for (i=0 ; i< ARRAY_COUNT(gl_solid_modes) ; i++)
	{
		if ( !stricmp( gl_solid_modes[i].name, string ) )
			break;
	}

	if (i == ARRAY_COUNT(gl_solid_modes))
	{
		Com_Printf ("bad solid texture mode name\n");
		return;
	}

	gl_tex_solid_format = gl_solid_modes[i].mode;
}

/*
===============
GL_ImageList_f
===============
*/

typedef struct
{
	int	value;
	char	*name;
} fmt_info_t;

const  fmt_info_t fmt_names[] =
{
	// generic modes
	{0,	"????"},
	{1,	"I"},
	{2,	"IA"},
	{3,	"RGB"},
	{4,	"RGBA"},
	// solid modes
	_STR(GL_RGB),
	_STR(GL_RGB4),
	_STR(GL_RGB5),
	_STR(GL_RGB8),
	_STR(GL_R3_G3_B2),
#ifdef GL_RGB2_EXT
	_STR(GL_RGB2_EXT),
#endif
	// alpha modes
	_STR(GL_RGBA),
	_STR(GL_RGBA2),
	_STR(GL_RGBA4),
	_STR(GL_RGBA8),
	_STR(GL_RGB5_A1),
	// compressed modes
//	_STR(GL_RGB_S3TC),
	{GL_RGB4_S3TC,	"S3TC"}, //"RGB4_S3TC"},
//	{GL_RGBA_S3TC,	"RGBA_S3TC"},
//	{GL_RGBA4_S3TC,	"RGBA4_S3TC"},
	{GL_COMPRESSED_RGB_ARB, "RGB_ARB"},
	{GL_COMPRESSED_RGBA_ARB, "RGBA_ARB"},
	// paletted
	{GL_COLOR_INDEX8_EXT, "PAL"},
};

#define NUM_FORMATS (sizeof(fmt_names))

void	GL_ImageList_f (void)
{
	int	i, j, f;
	image_t	*image;
	int	texels, imgs;
	char	*fmt, *t;

	Com_Printf ("----t---w---h---fmt------name------------\n");
	texels = 0;
	imgs = 0;

	for (i = 0, image=gltextures ; i < numgltextures ; i++, image++)
	{
		if (image->texnum <= 0)
			continue;
		texels += image->upload_width*image->upload_height;
		imgs++;

		switch (image->type)
		{
		case it_skin:
			t = "skn";//'M';
			break;
		case it_sprite:
			t = "spr";//'S';
			break;
		case it_wall:
			t = "wal";//'W';
			break;
		case it_pic:
			t = "pic";//'P';
			break;
		case it_sky:
			t = "sky";
			break;
		case it_temp:
			t = "tmp";
			break;
		default:
			t = " ? ";//' ';
			break;
		}

		f = image->upload_format;
		fmt = NULL;
		for (j = 0; j < NUM_FORMATS; j++)
		{
			if (fmt_names[j].value == f)
			{
				fmt = fmt_names[j].name;
				break;
			}
		}
		if (!fmt)
			fmt = va("0x%4X  ", f);
		if (image->scrap)
			fmt = "(scrap) ";

		Com_Printf ( "%-3i %s %-3i %-3i %-8s %s\n", i,
			t, image->upload_width, image->upload_height, fmt, image->name);
	}
	Com_Printf ("Total %i images with texel count (not counting mipmaps) %i\n", imgs, texels);
}


/*
=============================================================================

  scrap allocation

  Allocate all the little status bar obejcts into a single texture
  to crutch up inefficient hardware / drivers

=============================================================================
*/

#define	MAX_SCRAPS		1
#define	BLOCK_WIDTH		256
#define	BLOCK_HEIGHT	256

int			scrap_allocated[MAX_SCRAPS][BLOCK_WIDTH];
byte		scrap_texels[MAX_SCRAPS][BLOCK_WIDTH*BLOCK_HEIGHT];
bool	scrap_dirty;

// returns a texture number and the position inside it
int Scrap_AllocBlock (int w, int h, int *x, int *y)
{
	int		i, j;
	int		best, best2;
	int		texnum;

	for (texnum=0 ; texnum<MAX_SCRAPS ; texnum++)
	{
		best = BLOCK_HEIGHT;

		for (i=0 ; i<BLOCK_WIDTH-w ; i++)
		{
			best2 = 0;

			for (j=0 ; j<w ; j++)
			{
				if (scrap_allocated[texnum][i+j] >= best)
					break;
				if (scrap_allocated[texnum][i+j] > best2)
					best2 = scrap_allocated[texnum][i+j];
			}
			if (j == w)
			{	// this is a valid spot
				*x = i;
				*y = best = best2;
			}
		}

		if (best + h > BLOCK_HEIGHT)
			continue;

		for (i=0 ; i<w ; i++)
			scrap_allocated[texnum][*x + i] = best + h;

		return texnum;
	}

	return -1;
}

int	scrap_uploads;

void Scrap_Upload (void)
{
	scrap_uploads++;
	GL_Bind(TEXNUM_SCRAPS);
	GL_Upload8 (scrap_texels[0], BLOCK_WIDTH, BLOCK_HEIGHT, false, false );
	scrap_dirty = false;
}


/*
====================================================================

IMAGE FLOOD FILLING

====================================================================
*/


/*
=================
Mod_FloodFillSkin

Fill background pixels so mipmapping doesn't have haloes
=================
*/

typedef struct
{
	short		x, y;
} floodfill_t;

// must be a power of 2 (cyclic queue)
#define FLOODFILL_FIFO_SIZE 0x1000
#define FLOODFILL_FIFO_MASK (FLOODFILL_FIFO_SIZE - 1)

void R_FloodFillSkin (byte *skin, int skinwidth, int skinheight)
{
	byte	oldColor, newColor;
	floodfill_t fifo[FLOODFILL_FIFO_SIZE];
	int		pput, pget, i;

	oldColor = *skin;		// start to fill from this pixel
	newColor = 0;
	// attempt to find opaque black
	for (i = 0; i < 256; ++i)
		if (d_8to24table[i] == (255 << 0) /*+ (255 << 24)*/) // alpha 1.0
		{
			newColor = i;
			break;
		}

	// can't fill to filled color or to transparent color (used as visited marker)
	if ((oldColor == newColor) || (oldColor == 255))
	{
		//printf( "not filling skin from %d to %d\n", fillcolor, filledcolor );
		return;
	}

	fifo[0].x = fifo[0].y = 0;			// start point
	pget = 0;
	pput = 1;

	// visit (in a flood manner) all skin texels with color == oldColor and replace this texel with
	// any adjancent texel color (if its color != oldColor) or with newColor
	while (pput != pget)
	{
		int		x, y;
		byte	*pic, color;

		// get point from queue
		x = fifo[pget].x;
		y = fifo[pget].y;
		pget = (pget + 1) & FLOODFILL_FIFO_MASK;

		pic = &skin[x + skinwidth * y];
		color = newColor;

/* FLOODFILL_STEP(newPoint)
	{
		if (newPoint.color == oldColor) {
			newPoint.Mark();
			newPoint.AddToQueue();
		}
		else if (!newPoint.marked)
			color = newPoint.color;
	}
*/
#define FLOODFILL_STEP(dx, dy)				\
		{									\
			byte *data = pic + dx + dy * skinwidth;	\
			if (*data == oldColor)			\
			{								\
				*data = 255;				\
				fifo[pput].x = x + (dx);	\
				fifo[pput].y = y + (dy);	\
				pput = (pput + 1) & FLOODFILL_FIFO_MASK; \
			}								\
			else if (*data != 255)			\
				color = *data;				\
		}

		if (x > 0)				FLOODFILL_STEP(-1, 0);	// go left
		if (x < skinwidth - 1)	FLOODFILL_STEP( 1, 0);	// go right
		if (y > 0)				FLOODFILL_STEP( 0,-1);	// go down
		if (y < skinheight - 1)	FLOODFILL_STEP( 0, 1);	// go up
		skin[x + skinwidth * y] = color;
	}
}

//=======================================================


/*
================
GL_ResampleTexture
================
*/
void GL_ResampleTexture (unsigned *in, int inwidth, int inheight, unsigned *out,  int outwidth, int outheight)
{
	int		i, j;
	unsigned	*inrow, *inrow2;
	unsigned	frac, fracstep;
	unsigned	p1[1024], p2[1024];
	byte		*pix1, *pix2, *pix3, *pix4;

	fracstep = inwidth * 0x10000 / outwidth;

	frac = fracstep>>2;
	for (i = 0; i < outwidth; i++)
	{
		p1[i] = 4 * (frac>>16);
		frac += fracstep;
	}
	frac = 3*(fracstep>>2);
	for (i=0 ; i<outwidth ; i++)
	{
		p2[i] = 4*(frac>>16);
		frac += fracstep;
	}

#if 1		// old code
	for (i=0 ; i<outheight ; i++, out += outwidth)
	{
		inrow = in + inwidth*(int)((i+0.25)*inheight/outheight);
		inrow2 = in + inwidth*(int)((i+0.75)*inheight/outheight);
		frac = fracstep >> 1;
		for (j=0 ; j<outwidth ; j++)
		{
			pix1 = (byte *)inrow + p1[j];
			pix2 = (byte *)inrow + p2[j];
			pix3 = (byte *)inrow2 + p1[j];
			pix4 = (byte *)inrow2 + p2[j];
			((byte *)(out+j))[0] = (pix1[0] + pix2[0] + pix3[0] + pix4[0])>>2;
			((byte *)(out+j))[1] = (pix1[1] + pix2[1] + pix3[1] + pix4[1])>>2;
			((byte *)(out+j))[2] = (pix1[2] + pix2[2] + pix3[2] + pix4[2])>>2;
			((byte *)(out+j))[3] = (pix1[3] + pix2[3] + pix3[3] + pix4[3])>>2;
		}
	}
#else		// new code
	for (i = 0; i < outheight; i++, out += outwidth)
	{
		inrow = in + inwidth*(int)((i+0.25)*inheight/outheight);
		inrow2 = in + inwidth*(int)((i+0.75)*inheight/outheight);
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
#endif
}


/*
================
GL_BufCorrectGamma

Corrects gamma for buffer when hardware gamma supported
================
*/


void GL_BufCorrectGamma (byte *pic, int size)
{
	int i;

	if (!GLimp_HasGamma ())
		return;		// no hardware gamma - nothing to correct

	for (i = 0; i < size; i++)
		pic[i] = gammatable[intensitytable[pic[i]]];
}


/*
================
GL_LightScaleTexture

Scale up the pixel values in a texture to increase the
lighting range
================
*/

void GL_LightScaleTexture (unsigned *in, int inwidth, int inheight, bool only_gamma)
{
	float sat;
	int i, c;
	byte *p;

	sat = r_saturation->value;
	c = inwidth * inheight;

	if (sat != 1.0)
	{
		float r, g, b;
		float light;

		p = (byte *)in;
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

	if (GLimp_HasGamma()) return; // hardware gamma control

	if (only_gamma)
	{
		p = (byte *)in;
		for (i = 0; i < c; i++, p+=4)
		{
			p[0] = gammatable[p[0]];
			p[1] = gammatable[p[1]];
			p[2] = gammatable[p[2]];
		}
	}
	else
	{
		p = (byte *)in;
		for (i = 0; i < c; i++, p+=4)
		{
			p[0] = gammatable[intensitytable[p[0]]];
			p[1] = gammatable[intensitytable[p[1]]];
			p[2] = gammatable[intensitytable[p[2]]];
		}
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

void GL_MipMap (byte *in, int width, int height)
{
	int		i, j;
	byte	*out;

	width <<=2;
	height >>= 1;
	out = in;
	for (i=0 ; i<height ; i++, in+=width)
	{
		for (j=0 ; j<width ; j+=8, out+=4, in+=8)
		{
			int a, i;

			out[0] = (in[0] + in[4] + in[width+0] + in[width+4])>>2;
			out[1] = (in[1] + in[5] + in[width+1] + in[width+5])>>2;
			out[2] = (in[2] + in[6] + in[width+2] + in[width+6])>>2;
//			out[3] = (in[3] + in[7] + in[width+3] + in[width+7])>>2;
			// generate alpha-channel for mipmaps (don't let it be transparent)
			i = (in[3] + in[7] + in[width+3] + in[width+7])>>2;
			a = MAX(in[3],in[7]);
			a = MAX(a,in[width+3]);
			out[3] = (MAX(a,in[width+7]) + i) >> 1;
		}
	}
}

/*
===============
GL_Upload32

Returns has_alpha
===============
*/
void GL_BuildPalettedTexture( unsigned char *paletted_texture, unsigned char *scaled, int scaled_width, int scaled_height )
{
	int i;

	for ( i = 0; i < scaled_width * scaled_height; i++ )
	{
		unsigned int r, g, b, c;

		r = ( scaled[0] >> 3 ) & 31;
		g = ( scaled[1] >> 2 ) & 63;
		b = ( scaled[2] >> 3 ) & 31;

		c = r | ( g << 5 ) | ( b << 11 );

		paletted_texture[i] = gl_state.d_16to8table[c];

		scaled += 4;
	}
}

int	upload_width, upload_height, upload_format;

bool GL_Upload32 (unsigned *data, int width, int height, bool mipmap)
{
	int			samples;
	unsigned	scaled[256*256];
//	unsigned char paletted_texture[256*256];
	int			scaled_width, scaled_height;
	int			i, c;
	byte		*scan;
	int comp;
	bool	a1;

	upload_format = 0;

	for (scaled_width = 1 ; scaled_width < width ; scaled_width<<=1)
		;
	if (gl_round_down->integer && scaled_width > width && mipmap)
		scaled_width >>= 1;
	for (scaled_height = 1 ; scaled_height < height ; scaled_height<<=1)
		;
	if (gl_round_down->integer && scaled_height > height && mipmap)
		scaled_height >>= 1;

	// let people sample down the world textures for speed
	if (mipmap)
	{
		scaled_width >>= gl_picmip->integer;
		scaled_height >>= gl_picmip->integer;
	}

	// don't ever bother with >256 textures
	if (scaled_width > 256)
		scaled_width = 256;
	if (scaled_height > 256)
		scaled_height = 256;

	if (scaled_width < 1)
		scaled_width = 1;
	if (scaled_height < 1)
		scaled_height = 1;

	upload_width = scaled_width;
	upload_height = scaled_height;

	if (scaled_width * scaled_height > sizeof(scaled)/4)
		Com_DropError ("GL_Upload32: too big");

	// scan the texture for any non-255 alpha
	c = width*height;
	scan = ((byte *)data) + 3;
	samples = gl_solid_format;
	a1 = true;
	for (i=0 ; i<c ; i++, scan += 4)
	{
		if (*scan == 255) continue;	// no alpha
		if (*scan == 0)			// max alpha
		{
			samples = gl_alpha_format;
			continue;
		}
		samples = gl_alpha_format;
		a1 = false;			// mid alpha
		break;
	}

	// stuff for texture compression
	if (samples == gl_solid_format)
	{
		if (i = gl_state.texture_format_solid)
			comp = i;
		else
			comp = 3; //gl_tex_solid_format;
	}
	else if (samples == gl_alpha_format)
	{
		if (a1)
		{
			if (i = gl_state.texture_format_alpha1)
				comp = i;
			else
				comp = gl_tex_alpha_format;//?? GL_RGB5_A1;
		}
		else
		{
			if (i = gl_state.texture_format_alpha)
				comp = i;
			else
				comp = gl_tex_alpha_format;//?? 4
		}
	}
	else
	{
	    Com_Printf ("Unknown number of texture components %i\n", samples);
	    comp = samples;
	}

	upload_format = comp;

#if 0
	if (mipmap)
		gluBuild2DMipmaps (GL_TEXTURE_2D, samples, width, height, GL_RGBA, GL_UNSIGNED_BYTE, trans);
	else if (scaled_width == width && scaled_height == height)
		glTexImage2D (GL_TEXTURE_2D, 0, comp, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, trans);
	else
	{
		gluScaleImage (GL_RGBA, width, height, GL_UNSIGNED_BYTE, trans,
			scaled_width, scaled_height, GL_UNSIGNED_BYTE, scaled);
		glTexImage2D (GL_TEXTURE_2D, 0, comp, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaled);
	}
#else

	GL_LightScaleTexture (data, width, height, !mipmap);

	if (scaled_width == width && scaled_height == height)
	{
		if (!mipmap)
		{
/*			if ( glColorTableEXT && gl_ext_palettedtexture->integer && samples == gl_solid_format )
			{
				GL_BuildPalettedTexture( paletted_texture, ( unsigned char * ) data, scaled_width, scaled_height );
				glTexImage2D( GL_TEXTURE_2D,
							  0,
							  GL_COLOR_INDEX8_EXT,
							  scaled_width,
							  scaled_height,
							  0,
							  GL_COLOR_INDEX,
							  GL_UNSIGNED_BYTE,
							  paletted_texture );
				upload_format = GL_COLOR_INDEX8_EXT;
			}
			else
*/			{
				glTexImage2D (GL_TEXTURE_2D, 0, comp, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
			}
			goto done;
		}
		memcpy (scaled, data, width*height*4);
	}
	else
		GL_ResampleTexture (data, width, height, scaled, scaled_width, scaled_height);

//	GL_LightScaleTexture (scaled, scaled_width, scaled_height, !mipmap);

/*	if ( glColorTableEXT && gl_ext_palettedtexture->integer && ( samples == gl_solid_format ) )
	{
		GL_BuildPalettedTexture( paletted_texture, ( unsigned char * ) scaled, scaled_width, scaled_height );
		glTexImage2D( GL_TEXTURE_2D,
					  0,
					  GL_COLOR_INDEX8_EXT,
					  scaled_width,
					  scaled_height,
					  0,
					  GL_COLOR_INDEX,
					  GL_UNSIGNED_BYTE,
					  paletted_texture );
		upload_format = GL_COLOR_INDEX8_EXT;
	}
	else
*/	{
		glTexImage2D( GL_TEXTURE_2D, 0, comp, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaled );
	}

	if (mipmap)
	{
		int		miplevel;

		miplevel = 0;
		while (scaled_width > 1 || scaled_height > 1)
		{
			GL_MipMap ((byte *)scaled, scaled_width, scaled_height);
			scaled_width >>= 1;
			scaled_height >>= 1;
			if (scaled_width < 1)
				scaled_width = 1;
			if (scaled_height < 1)
				scaled_height = 1;
			miplevel++;
/*			if ( glColorTableEXT && gl_ext_palettedtexture->integer && samples == gl_solid_format )
			{
				GL_BuildPalettedTexture( paletted_texture, ( unsigned char * ) scaled, scaled_width, scaled_height );
				glTexImage2D( GL_TEXTURE_2D,
							  miplevel,
							  GL_COLOR_INDEX8_EXT,
							  scaled_width,
							  scaled_height,
							  0,
							  GL_COLOR_INDEX,
							  GL_UNSIGNED_BYTE,
							  paletted_texture );
				upload_format = GL_COLOR_INDEX8_EXT;
			}
			else
*/			{
				glTexImage2D (GL_TEXTURE_2D, miplevel, comp, scaled_width, scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, scaled);
			}
		}
	}
done: ;
#endif


	if (mipmap)
	{
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
	}
	else
	{
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_max);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
	}

//??	if (err = glGetError ()) Com_Printf ("GLError: %d\n", err);

	return (samples != gl_solid_format);
}

/*
===============
GL_Upload8

Returns has_alpha
===============
*/
/*
static bool IsPowerOf2( int value )
{
	int i = 1;


	while ( 1 )
	{
		if ( value == i )
			return true;
		if ( i > value )
			return false;
		i <<= 1;
	}
}
*/

bool GL_Upload8 (byte *data, int width, int height, bool mipmap, bool is_sky )
{
	unsigned	trans[512*256];
	int			i, s;
	int			p;

	s = width*height;

	if (s > sizeof(trans)/4)
		Com_DropError ("GL_Upload8: too large");

/*	if ( glColorTableEXT &&
		 gl_ext_palettedtexture->integer &&
		 is_sky )
	{
		glTexImage2D( GL_TEXTURE_2D,
					  0,
					  GL_COLOR_INDEX8_EXT,
					  width,
					  height,
					  0,
					  GL_COLOR_INDEX,
					  GL_UNSIGNED_BYTE,
					  data );
		upload_format = GL_COLOR_INDEX8_EXT;

		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_max);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
	}
	else
*/	{
		for (i=0 ; i<s ; i++)
		{
			p = data[i];
			trans[i] = d_8to24table[p];

			if (p == 255)
			{	// transparent, so scan around for another color
				// to avoid alpha fringes
				// FIXME: do a full flood fill so mips work...
				if (i > width && data[i-width] != 255)
					p = data[i-width];
				else if (i < s-width && data[i+width] != 255)
					p = data[i+width];
				else if (i > 0 && data[i-1] != 255)
					p = data[i-1];
				else if (i < s-1 && data[i+1] != 255)
					p = data[i+1];
				else
					p = 0;
				// copy rgb components
				((byte *)&trans[i])[0] = ((byte *)&d_8to24table[p])[0];
				((byte *)&trans[i])[1] = ((byte *)&d_8to24table[p])[1];
				((byte *)&trans[i])[2] = ((byte *)&d_8to24table[p])[2];
			}
		}

		return GL_Upload32 (trans, width, height, mipmap);
	}
}


/*
================
GL_LoadPic

This is also used as an entry point for the generated r_notexture
================
*/
image_t *GL_LoadPic (char *name, byte *pic, int width, int height, imagetype_t type, int bits)
{
	image_t		*image;
	int			i;

	// find a free image_t
	for (i=0, image=gltextures ; i<numgltextures ; i++,image++)
	{
		if (!image->texnum)
			break;
	}
	if (i == numgltextures)
	{
		if (numgltextures == MAX_GLTEXTURES)
			Com_DropError ("MAX_GLTEXTURES");
		numgltextures++;
	}
	image = &gltextures[i];

	if (strlen(name) >= sizeof(image->name))
		Com_DropError ("Draw_LoadPic: \"%s\" is too long", name);
	strcpy (image->name, name);
	image->registration_sequence = registration_sequence;

	image->width = width;
	image->height = height;
	image->type = type;

	if (type == it_skin && bits == 8)
		R_FloodFillSkin(pic, width, height);

	// load little pics into the scrap
	if ((image->type == it_pic || image->type == it_temp) && bits == 8
		&& image->width <= 64 && image->height <= 64)
	{
		int		x, y;
		int		i, j, k;
		int		texnum;

		texnum = Scrap_AllocBlock (image->width, image->height, &x, &y);
		if (texnum == -1)
			goto nonscrap;
		scrap_dirty = true;

		// copy the texels into the scrap block
		k = 0;
		for (i=0 ; i<image->height ; i++)
			for (j=0 ; j<image->width ; j++, k++)
				scrap_texels[texnum][(y+i)*BLOCK_WIDTH + x + j] = pic[k];
		image->texnum = TEXNUM_SCRAPS + texnum;
		image->scrap = true;
		image->has_alpha = true;
		image->sl = (x+0.01)/(float)BLOCK_WIDTH;
		image->sh = (x+image->width-0.01)/(float)BLOCK_WIDTH;
		image->tl = (y+0.01)/(float)BLOCK_WIDTH;
		image->th = (y+image->height-0.01)/(float)BLOCK_WIDTH;
	}
	else
	{
nonscrap:
		image->scrap = false;
		image->texnum = TEXNUM_IMAGES + (image - gltextures);
		GL_Bind(image->texnum);
		if (bits == 8)
			image->has_alpha = GL_Upload8 (pic, width, height,
				(image->type != it_pic && image->type != it_sky && image->type != it_temp), image->type == it_sky);
		else
			image->has_alpha = GL_Upload32 ((unsigned *)pic, width, height,
				(image->type != it_pic && image->type != it_sky && image->type != it_temp));
		image->upload_width = upload_width;		// after power of 2 and scales
		image->upload_height = upload_height;
		image->upload_format = upload_format;
		image->sl = 0;
		image->sh = 1;
		image->tl = 0;
		image->th = 1;
	}

	//Con_DPrintf ("Loaded texture %s #%d\n", name, image->texnum);
	return image;
}


/*
================
GL_LoadWal
================
*/
image_t *GL_LoadWal (char *name)
{
	miptex_t	*mt;
	int			width, height, ofs;
	image_t		*image;

	FS_LoadFile (name, (void **)&mt);
	if (!mt)
	{
		Com_Printf ("GL_FindImage: can't load %s\n", name);
		return r_notexture;
	}

	width = LittleLong (mt->width);
	height = LittleLong (mt->height);
	ofs = LittleLong (mt->offsets[0]);

	image = GL_LoadPic (name, (byte *)mt + ofs, width, height, it_wall, 8);

	FS_FreeFile ((void *)mt);

	return image;
}

/*
===============
GL_FindImage

Finds or loads the given image
===============
*/
image_t	*GL_FindImage (char *name, imagetype_t type)
{
	image_t	*image;
//	image_t *findimage;
	int	i, len, check_other;
	byte	*pic, *palette;
	int	width, height, bits, fmt;
	char	c, *s, *d, buf[256];

	if (!name)
		return NULL;	//	Com_DropError ("GL_FindImage: NULL name");

	// make a local copy of image name with a lower case conversion
	s = name;
	d = buf;
	while (c = *s++)
	{
		if (c >= 'A' && c <= 'Z') c += 32;
		*d++ = c;
	}
	*d = 0; // ASCIIZ
	name = buf;

	len = strlen(name);
	if (len<5)
		return NULL;	//	Com_DropError ("GL_FindImage: bad name: %s", name);

	len -= 4;	// &name[len] -> ".ext"
	s = &name[len];		// points to extension

	// Check if texture already loaded

	// if extension is .??? (originally, .PCX or .WAL), check TGA and JPG first, then PCX/WAL
	check_other = !strcmp (s, ".???") || !strcmp (s, ".pcx") || !strcmp (s, ".wal");
	// look for it
//	findimage = NULL;
	for (i = 0, image = gltextures; i < numgltextures; i++,image++)
	{
		char *s1;

		if (memcmp (name, image->name, len)) continue; // different name

		s1 = &image->name[len];
		if (check_other)
		{
			if (!strcmp (s1, ".tga") || !strcmp (s1, ".jpg")
			 || !strcmp (s1, ".wal") || !strcmp (s1, ".pcx"))
			{
				image->registration_sequence = registration_sequence;
				return image;
			}
		}
		else
		{	// full image name specified - check rest of filename
			if (!strcmp (s1, s))
			{
				image->registration_sequence = registration_sequence;
				return image;
			}
		}
	}
//	if (findimage) return findimage; // TGA or JPG not found, but PCX or WAL found

	if (type == it_pic && memcmp (name, "pics/", 4))	// mark pics from other directories as temporary
		type = it_temp;

	//
	// load the pic from disk
	//
	pic = NULL;
	palette = NULL;
	image = NULL;
	bits = 0;
	if (check_other)
	{
		*s = 0; // cut extension
		fmt = ImageExists (name, IMAGE_32BIT);
		if (fmt & IMAGE_TGA)
			strcpy (s, ".tga");
		else if (fmt & IMAGE_JPG)
			strcpy (s, ".jpg");
		else if (fmt & IMAGE_PCX)
			strcpy (s, ".pcx");
		else if (fmt & IMAGE_WAL)
			strcpy (s, ".wal");
		else
		{
			Com_DPrintf ("Cannot find image %s.*\n", name);
			return NULL;
		}
	}
	else
	{
		if (!strcmp (s, ".tga"))
			fmt = IMAGE_TGA;
		else if (!strcmp (s, ".jpg"))
			fmt = IMAGE_JPG;
		else if (!strcmp (s, ".pcx"))
			fmt = IMAGE_PCX;
		else if (!strcmp (s, ".wal"))
			fmt = IMAGE_WAL;
		else
		{
			Com_WPrintf ("Unrecognized image extension: %s\n", name);
			return NULL;
		}
	}

	if (fmt & IMAGE_TGA)
	{
		LoadTGA (name, &pic, &width, &height);
		bits = 32;
	}
	else if (fmt & IMAGE_JPG)
	{
		LoadJPG (name, &pic, &width, &height);
		bits = 32;
	}
	else if (fmt & IMAGE_PCX)
	{
		LoadPCX (name, &pic, &palette, &width, &height);
		bits = 8;
	}
	else if (fmt & IMAGE_WAL)
		image = GL_LoadWal (name);

	if (pic)
		image = GL_LoadPic (name, pic, width, height, type, bits);

	if (!image)
		Com_WPrintf ("Cannot load texture %s\n", name);

	if (pic)
		Z_Free (pic);
	if (palette)
		Z_Free (palette);

	return image;
}



/*
===============
R_RegisterSkin
===============
*/
struct image_s *R_RegisterSkin (char *name)
{
	return GL_FindImage (name, it_skin);
}


/*
================
GL_FreeUnusedImages

Any image that was not touched on this registration sequence
will be freed.
================
*/
void GL_FreeUnusedImages (void)
{
	int		i;
	image_t	*image;

	// never free r_notexture or particle texture
	r_notexture->registration_sequence = registration_sequence;
	r_particletexture->registration_sequence = registration_sequence;

	for (i=0, image=gltextures ; i<numgltextures ; i++, image++)
	{
		if (image->registration_sequence == registration_sequence)
			continue;		// used this sequence
		if (!image->registration_sequence)
			continue;		// free image_t slot
		if (image->type == it_pic)
			continue;		// don't free pics
		if (image->scrap)
			continue;
		// free it
		//Com_DPrintf ("Freeing unused texture #%d %s from slot #%d%s\n",
		//	image->texnum, image->name, i, (image->scrap?" (scrap)":"")); //!!
		glDeleteTextures (1, &image->texnum);
		memset (image, 0, sizeof(*image));
	}
}


/*
===============
Draw_GetPalette
===============
*/
int Draw_GetPalette (void)
{
	int		i;
	int		r, g, b;
	unsigned	v;
	byte	*pic, *pal;
	int		width, height;

	// get the palette

	LoadPCX ("pics/colormap.pcx", &pic, &pal, &width, &height);
	if (!pal)
		Com_FatalError ("Couldn't load pics/colormap.pcx");

	for (i=0 ; i<256 ; i++)
	{
		r = pal[i*3+0];
		g = pal[i*3+1];
		b = pal[i*3+2];

		v = (255<<24) + (r<<0) + (g<<8) + (b<<16);
		d_8to24table[i] = LittleLong(v);
	}

	d_8to24table[255] &= LittleLong(0xffffff);	// 255 is transparent

	Z_Free (pic);
	Z_Free (pal);

	return 0;
}


/*
===============
GL_InitImages
===============
*/
void GL_InitImages (void)
{
	int		i, j;
	float	g = 1.0f / r_gamma->value;

	registration_sequence = 1;

	// init intensity conversions
	intensity = Cvar_Get ("intensity", "1", CVAR_ARCHIVE); // old default: 2

//	if (saturation->value < 0) saturation->value = 0;
//	else if (saturation->value > 2) saturation->value = 2;

//	if ( intensity->value <= 1 )
//		Cvar_Set( "intensity", "1" );

	gl_state.inverse_intensity = 1 / intensity->value;

	Draw_GetPalette ();

/*	if ( glColorTableEXT )
	{
		FS_LoadFile( "pics/16to8.dat", &gl_state.d_16to8table );
		if ( !gl_state.d_16to8table )
			Com_FatalError ("Couldn't load pics/16to8.dat");
	}

	if ( gl_config.renderer & ( GL_RENDERER_VOODOO | GL_RENDERER_VOODOO2 ) )
	{
		g = 1.0F;
	}
*/
	for ( i = 0; i < 256; i++ )
	{
		if ( g == 1 )
		{
			gammatable[i] = i;
		}
		else
		{
			float inf;

			inf = 255 * pow ( (i+0.5)/255.5 , g ) + 0.5;
			if (inf < 0)
				inf = 0;
			if (inf > 255)
				inf = 255;
			gammatable[i] = inf;
		}
	}

	for (i=0 ; i<256 ; i++)
	{
		j = i*intensity->value;
		if (j > 255)
			j = 255;
		intensitytable[i] = j;
	}

	GLimp_SetGamma (r_gamma->value, intensity->value);
}

/*
===============
GL_ShutdownImages
===============
*/
void	GL_ShutdownImages (void)
{
	int		i;
	image_t	*image;

	for (i = 0, image = gltextures; i < numgltextures; i++, image++)
	{
		if (!image->registration_sequence)
			continue;		// free image_t slot
		// free it
		glDeleteTextures (1, &image->texnum);
		memset (image, 0, sizeof(*image));
	}
}
