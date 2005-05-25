#ifndef __REF_INCLUDED__
#define __REF_INCLUDED__


// interfaces to some renderer types
class CRenderModel
{
public:
	char	name[MAX_QPATH];
//	virtual void LerpTag (int frame1, int frame2, CCoords *coord);
};


class CBasicImage
{
public:
	char	name[MAX_QPATH];
	short	width, height;
	virtual void Reload (void) = NULL;
};


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
#undef RGB255
#undef RGBA255

// constant colors
#define RGBA(r,g,b,a)		((int)((r)*255) | ((int)((g)*255)<<8) | ((int)((b)*255)<<16) | ((int)((a)*255)<<24))
#define RGB(r,g,b)			RGBA(r,g,b,1)
#define RGB255(r,g,b)		((r) | ((g)<<8) | ((b)<<16) | (255<<24))
#define RGBA255(r,g,b,a)	((r) | ((g)<<8) | ((b)<<16) | ((a)<<24))

// computed colors
#define RGBAS(r,g,b,a)		(appRound((r)*255) | (appRound((g)*255)<<8) | (appRound((b)*255)<<16) | (appRound((a)*255)<<24))
#define RGBS(r,g,b)			(appRound((r)*255) | (appRound((g)*255)<<8) | (appRound((b)*255)<<16) | (255<<24))


//!!! clean beam comments
struct entity_t
{
	CRenderModel *model;
	CVec3	angles;
	/*------- most recent data --------*/
	CVec3	origin;
	int		frame;
	/*------- data for lerping --------*/
	CVec3	oldorigin;
	int		oldframe;
	float	backlerp;		// 0.0 = current, 1.0 = old
	/*-------------- misc -------------*/
	CVec3	size;
	int		skinnum;
	color_t	color;			// for extended beam
	/*----------- color info ----------*/
	float	alpha;			// ignore if RF_TRANSLUCENT isn't set

	CBasicImage	*skin;		// NULL for inline skin
	int		flags;
};


typedef enum
{
	BEAM_STANDARD,
	BEAM_RAILBEAM,
	BEAM_RAILSPIRAL,
	BEAM_RAILRINGS
} beamType_t;


struct beam_t
{
	CVec3	start;
	CVec3	end;
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
	CVec3	drawStart, drawEnd;

	beam_t *next;
	beam_t *drawNext;
};


typedef struct
{
	CVec3	origin;
	CVec3	color;
	float	intensity;
} dlight_t;


typedef enum
{
	PT_DEFAULT,
	PT_SPARKLE
} particleType_t;

struct particle_t
{
	// appearance
	CVec3	org;
	byte	color;
	bool	_new;
	particleType_t type;
	float	alpha;
	int		leafNum;				// -1 -- uninitialized
	// some physics
	CVec3	vel;					// org += vel * time
	CVec3	accel;					// vel += accel * time
	float	alphavel;

	particle_t *next;
	particle_t *drawNext;
};

typedef struct
{
	byte	length;
	byte	value;					// current lighting; monochrome; 0 .. 255 -> 0.0 .. 2.0
	byte	map[MAX_QPATH];
} lightstyle_t;

typedef struct
{
	int		x, y, width, height;	// in virtual screen coordinates
	float	fov_x, fov_y;
	CVec3	vieworg;
	CVec3	viewangles;
	float	blend[4];				// rgba 0-1 full screen blend
	float	time;					// time is used to auto animate
	unsigned rdflags;				// RDF_UNDERWATER, etc

	byte	*areabits;				// if not NULL, only areas with set bits will be drawn

	int		num_entities;
	entity_t *entities;

	int		num_dlights;
	dlight_t *dlights;

	lightstyle_t *lightstyles;		// [MAX_LIGHTSTYLES]
	particle_t *particles;
	beam_t	 *beams;
} refdef_t;


// renderer flags (capabilities)
#define REF_CONSOLE_ONLY	1		// if set -- no graphics output

// screenshot flags
#define SHOT_SMALL			1		// stretch screenshot to reduce its dimensions (levelshots, savegames etc.)
#define SHOT_NO_2D			2		// perform screenshot before switching to 2D mode
#define SHOT_WAIT_3D		4		// perform screenshot with first available frame with 3D output
#define SHOT_SILENT			8		// do not write "wrote shotxxx.jpg" to console
#define SHOT_NOGAMMA		16		// do not perform gamma correction for image (when hardware gamma available)
#define SHOT_JPEG			0x8000	// can be unsupported

// DrawPic() anchors
#define ANCHOR_TOP_LEFT		0x00
#define ANCHOR_TOP			0x01
#define ANCHOR_TOP_RIGHT	0x02
#define ANCHOR_LEFT			0x10
#define ANCHOR_CENTER		0x11
#define ANCHOR_RIGHT		0x12
#define ANCHOR_BOTTOM_LEFT	0x20
#define ANCHOR_BOTTOM		0x21
#define ANCHOR_BOTTOM_RIGHT	0x22


/*-------------------- Common renderer cvars -----------------------------*/

void InitRendererVars (void);

extern	cvar_t	*r_fullscreen;
extern	cvar_t	*r_gamma, *r_brightness, *r_contrast, *r_saturation;
extern	cvar_t	*r_fullbright, *r_lightmap;


/*------------------- Static/dynamic renderer ----------------------------*/

#include "rexp_intf.h"

//?? rename refImport_t, refExport_t to something another ...

typedef	bool (*CreateDynRenderer_t) (const refImport_t *, refExport_t *);

#ifndef STATIC_BUILD

#define RENDERER_EXPORT		\
	extern "C" DLL_EXPORT bool CreateRenderer(const refImport_t *, refExport_t *) \
	bool CreateRenderer (const refImport_t *rimp, refExport_t *rexp) \
	{						\
		if (rimp.struc_size != sizeof(refImport_t) || \
			rexp->struc_size != sizeof(refExport_t)) \
			return false;	\
		ri = *rimp;			\
		*rexp = re;			\
		InitRendererVars ();\
		return true;		\
	}

#else

#define RENDERER_EXPORT			// empty for STATIC_BUILD

#endif


#endif
