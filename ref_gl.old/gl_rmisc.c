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
// r_misc.c

#include "gl_local.h"

int 	gl_screenshotFlags;
char	*gl_screenshotName;

/*
==================
R_InitParticleTexture
==================
*/
byte	dottexture[8][8] =
{
	{0,0,0,0,0,0,0,0},
	{0,0,1,1,0,0,0,0},
	{0,1,1,1,1,0,0,0},
	{0,1,1,1,1,0,0,0},
	{0,0,1,1,0,0,0,0},
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0},
};

void R_InitParticleTexture (void)
{
	int		x,y;
	byte	data[8][8][4];

	//
	// particle texture
	//
	for (x=0 ; x<8 ; x++)
	{
		for (y=0 ; y<8 ; y++)
		{
			data[y][x][0] = 255;
			data[y][x][1] = 255;
			data[y][x][2] = 255;
			data[y][x][3] = dottexture[x][y]*255;
		}
	}
	r_particletexture = GL_LoadPic ("***particle***", (byte *)data, 8, 8, it_sprite, 32);

	//
	// also use this for bad textures, but without alpha
	//
	for (x=0 ; x<8 ; x++)
	{
		for (y=0 ; y<8 ; y++)
		{
			data[y][x][0] = dottexture[x&3][y&3]*255;
			data[y][x][1] = 0; // dottexture[x&3][y&3]*255;
			data[y][x][2] = 0; //dottexture[x&3][y&3]*255;
			data[y][x][3] = 255;
		}
	}
	r_notexture = GL_LoadPic ("***r_notexture***", (byte *)data, 8, 8, it_wall, 32);
}


/*
==============================================================================

						SCREEN SHOTS

==============================================================================
*/

typedef struct _TargaHeader {
	unsigned char 	id_length, colormap_type, image_type;
	unsigned short	colormap_index, colormap_length;
	unsigned char	colormap_size;
	unsigned short	x_origin, y_origin, width, height;
	unsigned char	pixel_size, attributes;
} TargaHeader;


/*-------------------- Screenshots ---------------------*/


#define		LEVELSHOT_W		256
#define		LEVELSHOT_H		256


void GL_PerformScreenshot (void)
{
	byte	*buffer, *src, *dst;
	char	name[MAX_OSPATH], *ext;
	int		i, width, height, size;
	qboolean result;
	FILE	*f;

	if (!gl_screenshotName) return;		// already performed in current frame

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
		for (i = 0; i < 10000; i++)
		{	// check for a free filename
			Com_sprintf (name, sizeof(name), "%s/screenshots/quake%04d%s", FS_Gamedir (), i, ext);
			if (!(f = fopen (name, "rb")))
				break;	// file doesn't exist
			fclose (f);
		}

	}
	gl_screenshotName = NULL;

	// create the screenshots directory if it doesn't exist
	FS_CreatePath (name);

	// allocate buffer for 4 color components (required for ResampleTexture()
	buffer = Z_Malloc (vid.width * vid.height * 4);
	// read frame buffer data
	glReadPixels (0, 0, vid.width, vid.height, GL_RGBA, GL_UNSIGNED_BYTE, buffer);

	if (gl_screenshotFlags & SHOT_SMALL)
	{
		byte *buffer2;

		//?? check: is there map rendered?
		buffer2 = Z_Malloc (LEVELSHOT_W * LEVELSHOT_H * 4);
		GL_ResampleTexture ((unsigned *)buffer, vid.width, vid.height, (unsigned *)buffer2, LEVELSHOT_W, LEVELSHOT_H);
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

	// remove 4th color component and correct gamma
	src = dst = buffer;
	for (i = 0; i < size; i++)
	{
		byte	r, g, b;

		r = *src++; g = *src++; b = *src++;
		src++;	// skip alpha
		// correct gamma (disabled: too lazy to implement !!)
		if (GLimp_HasGamma () && !(gl_screenshotFlags & SHOT_NOGAMMA))
		{
			r = gammatable[r];
			g = gammatable[g];
			b = gammatable[b];
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

	Z_Free (buffer);

	if (result && !(gl_screenshotFlags & SHOT_SILENT))
		Com_Printf ("Wrote %s\n", strrchr (name, '/') + 1);
}


/*
** GL_Strings_f (gfxinfo)
*/
void GL_Strings_f( void )
{
	Com_Printf ("^1GL_VENDOR:^7 %s\n", gl_config.vendorString);
	Com_Printf ("^1GL_RENDERER:^7 %s\n", gl_config.rendererString);
	Com_Printf ("^1GL_VERSION:^7 %s\n", gl_config.versionString);
//	Com_Printf ("GL_EXTENSIONS: %s\n", gl_config.extensionsString );
	QGL_PrintExtensionsString ("GL", gl_config.extensions);
#ifdef _WIN32
	QGL_PrintExtensionsString ("WGL", gl_config.extensions2);
#endif
}

/*
** GL_SetDefaultState
*/
void GL_SetDefaultState( void )
{
	glClearColor (1,0, 0.5 , 0.5);
	glCullFace(GL_FRONT);
	glEnable(GL_TEXTURE_2D);

	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.666);

	glDisable (GL_DEPTH_TEST);
	glDisable (GL_CULL_FACE);
	glDisable (GL_BLEND);

	glColor4f (1,1,1,1);

	glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
	glShadeModel (GL_SMOOTH);
//	glShadeModel (GL_FLAT);	--&&

	GL_TextureMode( gl_texturemode->string );
	GL_TextureAlphaMode( gl_texturealphamode->string );
	GL_TextureSolidMode( gl_texturesolidmode->string );

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	GL_TexEnv( GL_REPLACE );

/*	if ( glPointParameterfEXT )
	{
		float attenuations[3];

		attenuations[0] = gl_particle_att_a->value;
		attenuations[1] = gl_particle_att_b->value;
		attenuations[2] = gl_particle_att_c->value;

		glEnable( GL_POINT_SMOOTH );
		glPointParameterfEXT( GL_POINT_SIZE_MIN_EXT, gl_particle_min_size->value );
		glPointParameterfEXT( GL_POINT_SIZE_MAX_EXT, gl_particle_max_size->value );
		glPointParameterfvEXT( GL_DISTANCE_ATTENUATION_EXT, attenuations );
	}
*/
/*	if ( glColorTableEXT && gl_ext_palettedtexture->integer )
	{
		glEnable( GL_SHARED_TEXTURE_PALETTE_EXT );

		GL_SetTexturePalette( d_8to24table );
	}
*/
	GL_UpdateSwapInterval();
}

void GL_UpdateSwapInterval( void )
{
	if ( gl_swapinterval->modified )
	{
		gl_swapinterval->modified = false;

		if ( !gl_state.stereo_enabled )
		{
#ifdef _WIN32
			if (GL_SUPPORT(QWGL_EXT_SWAP_CONTROL))
				wglSwapIntervalEXT( gl_swapinterval->value );
#endif
		}
	}
}