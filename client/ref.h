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
	unsigned rgba;
} color_t;

#undef RGBA
#undef RGB

// constant colors
#define RGBA(r,g,b,a)		((int)((r)*255) | ((int)((g)*255)<<8) | ((int)((b)*255)<<16) | ((int)((a)*255)<<24))
#define RGB(r,g,b)			RGBA(r,g,b,1)
#define RGB255(r,g,b)		((r) | ((g)<<8) | ((b)<<16) | (255<<24))
#define RGBA255(r,g,b,a)	((r) | ((g)<<8) | ((b)<<16) | ((a)<<24))

// computed colors
#define RGBAS(r,g,b,a)		(Q_round((r)*255) | (Q_round((g)*255)<<8) | (Q_round((b)*255)<<16) | (Q_round((a)*255)<<24))
#define RGBS(r,g,b)			(Q_round((r)*255) | (Q_round((g)*255)<<8) | (Q_round((b)*255)<<16) | (255<<24))


//!!! clean beam comments
typedef struct entity_s
{
	struct model_s *model;	// opaque type outside refresh
	float	angles[3];
	/*------- most recent data --------*/
	float	origin[3];
	int		frame;
	/*------- data for lerping --------*/
	float	oldorigin[3];
	int		oldframe;
	float	backlerp;		// 0.0 = current, 1.0 = old
	/*-------------- misc -------------*/
	float	size[3];
	int		skinnum;
	color_t	color;			// for extended beam
	/*----------- color info ----------*/
	float	alpha;			// ignore if RF_TRANSLUCENT isn't set

	struct image_s	*skin;	// NULL for inline skin
	int		flags;
} entity_t;


typedef enum
{
	BEAM_STANDARD,
	BEAM_RAILBEAM,
	BEAM_RAILSPIRAL,
	BEAM_RAILRINGS
} beamType_t;


typedef struct beam_s
{
	vec3_t	start;
	vec3_t	end;
	float	radius;
	beamType_t type;
	color_t	color;
	// internal fields
	float	alpha;					// color.c[3] is byte and have no enough precision
	float	fadeTime;
	float	lifeTime;
	float	dstAlpha;				// alpha, which will be reached at end of life cycle (default==0)
	float	growSpeed;				// radius_delta/sec
	// temporary fields
	vec3_t	drawStart, drawEnd;

	struct beam_s *next;
	struct beam_s *drawNext;
} beam_t;


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
	bool	_new;
	particleType_t type;
	float	alpha;
	int		leafNum;				// -1 -- uninitialized
	// some physics
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
	beam_t		*beams;
} refdef_t;



#define	API_VERSION			4

// renderer flags (capabilities)
#define REF_CONSOLE_ONLY	1		// if set -- no graphics output
#define REF_NEW_FX			2		// if set, renderer supports sprite fx
#define REF_USE_PALETTE		4		// if set, renderer cannot display DrawStretchRaw8 simultaneously with other objects

// screenshot flags
#define SHOT_SMALL			1		// stretch screenshot to reduce its dimensions (levelshots, savegames etc.)
#define SHOT_NO_2D			2		// perform screenshot before switching to 2D mode
#define SHOT_WAIT_3D		4		// perform screenshot with first available frame with 3D output
#define SHOT_SILENT			8		// do not write "wrote shotxxx.jpg" to console
#define SHOT_NOGAMMA		16		// do not perform gamma correction for image (when hardware gamma available)
#define SHOT_JPEG			0x8000	// can be unsupported


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

	// Draw images for cinematic rendering (which can have a different palette)
	void	(*SetRawPalette) (const unsigned char *palette);	// NULL = game palette
	void	(*DrawStretchRaw8) (int x, int y, int w, int h, int cols, int rows, byte *data);

	/*---- video mode and refresh state management entry points ----*/
	void	(*BeginFrame) (float camera_separation);
	void	(*EndFrame) (void);

	void	(*AppActivate) (qboolean activate);

	/*---- drawing colored text in any screen position ----*/
	void	(*DrawTextPos) (int x, int y, char *text, unsigned rgba);
	void	(*DrawTextLeft) (char *text, unsigned rgba);
	void	(*DrawTextRight) (char *text, unsigned rgba);

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
