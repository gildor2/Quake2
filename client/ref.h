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
#ifndef __REF_INCLUDED__
#define __REF_INCLUDED__


#include "../qcommon/qcommon.h"

#define	MAX_DLIGHTS			32
#define	MAX_ENTITIES		1024
#define	MAX_PARTICLES		4096
#define MAX_PARTICLE_TRACES	256
#define MAX_PARTICLE_BEAMS	256
#define	MAX_LIGHTSTYLES		256


typedef union
{
	byte	c[4];
	int		rgba;
} color_t;


typedef enum
{
	BEAM_RAILBEAM = 256,
	BEAM_RAILSPIRAL,
	BEAM_RAILRINGS
} beamType_t;


typedef struct entity_s
{
	struct model_s *model;	// opaque type outside refresh
	float	angles[3];
	/*------- most recent data --------*/
	float	origin[3];		// also used as RF_BEAM's "from"
	int		frame;			// also used as RF_BEAM's diameter
	/*------- data for lerping --------*/
	float	oldorigin[3]; 	// also used as RF_BEAM's "to"
	int		oldframe;
	float	backlerp;		// 0.0 = current, 1.0 = old
	/*-------------- misc -------------*/
	int		skinnum;		// also used as RF_BEAM's palette index; if >= 256 - extended beam (beamType_t)
	color_t	color;			// for extended beam
	/*----------- color info ----------*/
	int		lightstyle;		// for flashing entities (unused ??)
	float	alpha;			// ignore if RF_TRANSLUCENT isn't set

	struct image_s	*skin;	// NULL for inline skin
	int		flags;
} entity_t;

#define ENTITY_FLAGS  68	// offset to entity_t->flags (used by ref_soft .asm routines only)

typedef struct
{
	vec3_t	origin;
	vec3_t	color;
	float	intensity;
} dlight_t;


typedef enum
{
	PT_DEFAULT,
	PT_SPARKLE
} particleType_t;

typedef struct particle_s
{
	// appearance
	vec3_t	org;
	byte	color;
	qboolean _new;
	float	alpha;
	particleType_t type;
	int		leafNum;				// -1 -- uninitialized
	// some physics
	int		startTime;
	vec3_t	vel;					// org += vel * time
	vec3_t	accel;					// vel += accel * time
	float	alphavel;

	struct particle_s *next;
	struct particle_s *drawNext;
} particle_t;

typedef struct
{
	int		length;
	byte	value;					// 0 .. 255 -> 0.0 .. 2.0
	byte	map[MAX_QPATH];
} lightstyle_t;

typedef struct
{
	int			x, y, width, height;// in virtual screen coordinates
	float		fov_x, fov_y;
	float		vieworg[3];
	float		viewangles[3];
	float		blend[4];			// rgba 0-1 full screen blend
	float		time;				// time is used to auto animate
	int			rdflags;			// RDF_UNDERWATER, etc

	byte		*areabits;			// if not NULL, only areas with set bits will be drawn

	int			num_entities;
	entity_t	*entities;

	int			num_dlights;
	dlight_t	*dlights;

	lightstyle_t *lightstyles;		// [MAX_LIGHTSTYLES]
	particle_t	*particles;
} refdef_t;



#define	API_VERSION			4

// renderer flasg
#define REF_CONSOLE_ONLY	1		// if set -- no graphics output
#define REF_NEW_FX			2		// if set, renderer supports sprite fx

// screenshot flags
#define SHOT_SMALL			1		// stretch screenshot to reduce its dimensions (levelshots, savegames etc.)
#define SHOT_NO_2D			2		// perform screenshot before switching to 2D mode
#define SHOT_WAIT_3D		4		// perform screenshot with first available frame with 3D output
#define SHOT_SILENT			8		// do not write "wrote shotxxx.jpg" to console
#define SHOT_JPEG			0x8000	// can be unsupported


/*-------- These are the functions exported by the refresh module --------*/
typedef struct
{
	// verification field, cb = sizeof(refExport_t)
	int		struc_size;
	// if api_version is different, the dll cannot be used
	int		api_version;
	// if console_only is true, no graphical information will be displayed and no mouse input performed
	int		*flags;

	// called when the library is loaded
	qboolean (*Init) (void);

	// called before the library is unloaded
	void	(*Shutdown) (void);

	// All data that will be used in a level should be
	// registered before rendering any frames to prevent disk hits,
	// but they can still be registered at a later time
	// if necessary.
	//
	// EndRegistration will free any remaining data that wasn't registered.
	// Any model_s or skin_s pointers from before the BeginRegistration
	// are no longer valid after EndRegistration.
	//
	// Skins and images need to be differentiated, because skins
	// are flood filled to eliminate mip map edge errors, and pics have
	// an implicit "pics/" prepended to the name. (a pic name that starts with a
	// slash will not use the "pics/" prefix or the ".pcx" postfix)
	void	(*BeginRegistration) (char *map);
	struct model_s *(*RegisterModel) (char *name);
	struct image_s *(*RegisterSkin) (char *name);
	struct image_s *(*RegisterPic) (char *name);
	void	(*SetSky) (char *name, float rotate, vec3_t axis);
	void	(*EndRegistration) (void);

	void	(*RenderFrame) (refdef_t *fd);
	void	(*Screenshot) (int flags, char *name);

	void	(*ReloadImage) (char *name);
	void	(*DrawGetPicSize) (int *w, int *h, char *name);	// will return (0, 0) if not found
	void	(*DrawPicColor) (int x, int y, char *pic, int color);
	void	(*DrawStretchPic) (int x, int y, int w, int h, char *name);
	void	(*DrawCharColor) (int x, int y, int c, int color);
	void	(*DrawTileClear) (int x, int y, int w, int h, char *name);
	void	(*DrawFill) (int x, int y, int w, int h, int c);
	void	(*DrawFill2) (int x, int y, int w, int h, float r, float g, float b, float a);

	// Draw images for cinematic rendering (which can have a different palette). Note that calls
	void	(*DrawStretchRaw) (int x, int y, int w, int h, int cols, int rows, byte *data);

	/*---- video mode and refresh state management entry points ----*/
	void	(*CinematicSetPalette) (const unsigned char *palette);	// NULL = game palette
	void	(*BeginFrame) (float camera_separation);
	void	(*EndFrame) (void);

	void	(*AppActivate) (qboolean activate);

	/*---- drawing colored text in any screen position ----*/
	void	(*DrawTextPos) (int x, int y, char *text, float r, float g, float b);
	void	(*DrawTextLeft) (char *text, float r, float g, float b);
	void	(*DrawTextRight) (char *text, float r, float g, float b);

	/*---- draw char at (x,y) (char-related coordinates) ----*/
	void	(*DrawConCharColor) (int x, int y, int c, int color);

	/*----------------- lighting -------------------*/
	float	(*GetClientLight) (void);		// used by server to determine client visibility (AI)
} refExport_t;


/*-------- These are the functions imported by the refresh module --------*/
#include "ref_decl.h"


// this is the only function actually exported at the linker level
typedef	refExport_t	(*GetRefAPI_t) (refImport_t);


#endif
