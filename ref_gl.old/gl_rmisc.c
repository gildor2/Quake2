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


/*
==================
GL_ScreenShot_f
==================
*/

#define		LEVELSHOT_W		256
#define		LEVELSHOT_H		256

void GL_Levelshot ()
{
	byte		*buffer, *buffer2, *in, *out;
	char		filename[MAX_OSPATH], scratch[MAX_OSPATH], *mapname, *ext;
	int			i;
	FILE		*f;

	// create the levelshots directory if it doesn't exist
	Com_sprintf (filename, sizeof(filename), "%s/levelshots", FS_Gamedir());
	Sys_Mkdir (filename);

	mapname = strrchr (r_worldmodel->name, '/');
	if (!mapname)
		mapname = r_worldmodel->name;	// normally, maps must be stored in "maps/*", but ...
	else
		mapname++;
	Com_sprintf (scratch, sizeof(scratch), "/%s", mapname);
	ext = strrchr (scratch, '.');
	if (!ext)
		ext = strrchr (scratch, 0);		// normally, ext must points to ".bsp"
	strcpy (ext, ".tga");
	strcat (filename, scratch);

	buffer = Z_Malloc (LEVELSHOT_W*LEVELSHOT_H*4 + 18 + 4);

	buffer2 = Z_Malloc (vid.width*vid.height*4);
	qglReadPixels (0, 0, vid.width, vid.height, GL_RGBA, GL_UNSIGNED_BYTE, buffer2);
	//?? may be, GL_ResampleTexture crushes when vid.height > 1024
	GL_ResampleTexture ((unsigned*)buffer2, vid.width, vid.height, (unsigned*)(buffer + 18 + 4), LEVELSHOT_W, LEVELSHOT_H);
	Z_Free (buffer2);

	memset (buffer, 0, 18);
	buffer[2] = 2;		// uncompressed type
	buffer[12] = LEVELSHOT_W&255;
	buffer[13] = LEVELSHOT_W>>8;
	buffer[14] = LEVELSHOT_H&255;
	buffer[15] = LEVELSHOT_H>>8;
	buffer[16] = 24;	// pixel size

	// convert RGBA to BGR
	in = buffer + 18 + 4;
	out = buffer + 18;
	for (i = 0; i < LEVELSHOT_W*LEVELSHOT_H; i++)
	{
		*out++ = in[2];	// B
		*out++ = in[1];	// G
		*out++ = in[0];	// R
		in += 4;
	}

	GL_BufCorrectGamma (&buffer[18], LEVELSHOT_W*LEVELSHOT_H*3);

	f = fopen (filename, "wb");
	if (!f)
	{
		Com_Printf ("Cannot write file %s\n", filename);
		return;
	}
	fwrite (buffer, 1, LEVELSHOT_W*LEVELSHOT_H*3 + 18, f);
	fclose (f);

	Z_Free (buffer);
}


void GL_ScreenShot_f (void)
{
	byte		*buffer;
	char		picname[80];
	char		checkname[MAX_OSPATH];
	int			i, c;
	FILE		*f;

	if (!strcmp (Cmd_Argv(1), "levelshot"))
	{
		GL_Levelshot ();
		return;
	}

	// create the screenshots directory if it doesn't exist
	Com_sprintf (checkname, sizeof(checkname), "%s/screenshots", FS_Gamedir());
	Sys_Mkdir (checkname);

	//
	// find a file name to save it to
	//
	strcpy(picname,"quake00.tga");

	for (i = 0; i < 100; i++)
	{
		picname[5] = i/10 + '0';
		picname[6] = i%10 + '0';
		Com_sprintf (checkname, sizeof(checkname), "%s/screenshots/%s", FS_Gamedir(), picname);
		f = fopen (checkname, "rb");
		if (!f)
			break;	// file doesn't exist
		fclose (f);
	}
	if (i == 100)
	{
		Com_Printf ("SCR_ScreenShot_f: Couldn't create a file\n");
		return;
 	}


	buffer = Z_Malloc (vid.width*vid.height*3 + 18);
	memset (buffer, 0, 18);
	buffer[2] = 2;		// uncompressed type
	buffer[12] = vid.width&255;
	buffer[13] = vid.width>>8;
	buffer[14] = vid.height&255;
	buffer[15] = vid.height>>8;
	buffer[16] = 24;	// pixel size

	qglReadPixels (0, 0, vid.width, vid.height, GL_RGB, GL_UNSIGNED_BYTE, buffer+18 );

	// swap rgb to bgr
	c = 18+vid.width*vid.height*3;
	for (i = 18; i < c; i+=3)
	{
		int temp;

		temp = buffer[i];
		buffer[i] = buffer[i+2];
		buffer[i+2] = temp;
	}

	GL_BufCorrectGamma (&buffer[18], c - 18);

	f = fopen (checkname, "wb");
	fwrite (buffer, 1, c, f);
	fclose (f);

	Z_Free (buffer);
	if (strcmp (Cmd_Argv(1), "silent"))
		Com_Printf ("Wrote %s\n", picname);
}

/*
** GL_Strings_f
*/
void GL_Strings_f( void )
{
	Com_Printf ("GL_VENDOR: %s\n", gl_config.vendor_string );
	Com_Printf ("GL_RENDERER: %s\n", gl_config.renderer_string );
	Com_Printf ("GL_VERSION: %s\n", gl_config.version_string );
	Com_Printf ("GL_EXTENSIONS: %s\n", gl_config.extensions_string );
}

/*
** GL_SetDefaultState
*/
void GL_SetDefaultState( void )
{
	qglClearColor (1,0, 0.5 , 0.5);
	qglCullFace(GL_FRONT);
	qglEnable(GL_TEXTURE_2D);

	qglEnable(GL_ALPHA_TEST);
	qglAlphaFunc(GL_GREATER, 0.666);

	qglDisable (GL_DEPTH_TEST);
	qglDisable (GL_CULL_FACE);
	qglDisable (GL_BLEND);

	qglColor4f (1,1,1,1);

	qglPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
	qglShadeModel (GL_SMOOTH);
//	qglShadeModel (GL_FLAT);	--&&

	GL_TextureMode( gl_texturemode->string );
	GL_TextureAlphaMode( gl_texturealphamode->string );
	GL_TextureSolidMode( gl_texturesolidmode->string );

	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);

	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	qglBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	GL_TexEnv( GL_REPLACE );

/*	if ( qglPointParameterfEXT )
	{
		float attenuations[3];

		attenuations[0] = gl_particle_att_a->value;
		attenuations[1] = gl_particle_att_b->value;
		attenuations[2] = gl_particle_att_c->value;

		qglEnable( GL_POINT_SMOOTH );
		qglPointParameterfEXT( GL_POINT_SIZE_MIN_EXT, gl_particle_min_size->value );
		qglPointParameterfEXT( GL_POINT_SIZE_MAX_EXT, gl_particle_max_size->value );
		qglPointParameterfvEXT( GL_DISTANCE_ATTENUATION_EXT, attenuations );
	}
*/
/*	if ( qglColorTableEXT && gl_ext_palettedtexture->integer )
	{
		qglEnable( GL_SHARED_TEXTURE_PALETTE_EXT );

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
			if ( qwglSwapIntervalEXT )
				qwglSwapIntervalEXT( gl_swapinterval->value );
#endif
		}
	}
}