#ifndef __REF_INCLUDED__
#define __REF_INCLUDED__


// interfaces to some renderer types
class CRenderModel
{
public:
	TString<64>		Name;
	// return "false" when tag is not found
	virtual bool LerpTag(int frame1, int frame2, float lerp, const char *tagName, CCoords &tag) const = 0;
};


class CBasicImage
{
public:
	TString<64>		Name;
	short			width, height;
	virtual void Reload() = 0;
};


class CBasicFont
{
public:
	TString<64>		Name;
	int				charWidth;
	int				charHeight;
};


class CModelSkin
{
public:
	// declaration of the single surface skin
	struct CSkinnedSurface {
		TString<32> Name;			// size should be enough
		CBasicImage *shader;
	};

	int				numSurfs;
	CSkinnedSurface	surf[32];		// MAX_MD3_SURFACES == 32; should be [numSurfs]

	inline bool IsValid()
	{
		return numSurfs > 0;
	}
};


#define	MAX_DLIGHTS			32
#define	MAX_ENTITIES		1024
#define	MAX_PARTICLES		4096
#define MAX_PARTICLE_TRACES	256
#define MAX_PARTICLE_BEAMS	256
#define	MAX_LIGHTSTYLES		256


struct entity_t
{
	CRenderModel	*model;
	CCoords			pos;
	CVec3			angles;			// not used by renderer
	int				frame;
	int				oldframe;
	float			backlerp;		// 0.0 = current, 1.0 = old; used for frame models only
	float			scale;			// scale model; 0 == 1 == unscaled
			//?? why do we use backlerp instead of simple lerp ?
	// misc
	CVec3			size;			// for RF_BBOX only; size of box
	// color info
	color_t			color;
	float			alpha;			// ignore if RF_TRANSLUCENT isn't set
			//?? use color.c[3]

	// entity shading; priority: 1.customShader 2.skin 3.skinnum
	CBasicImage		*customShader;	// single shader for all entity surfaces
	CModelSkin		*skin;			// NULL for built-in skin
	int				skinnum;		// number of model built-in skin

	unsigned		flags;			// set of RF_XXX flags
	double			time;			// for effects; used as refdef.time-ent.time (starting time of fx)
};


enum beamType_t
{
	BEAM_STANDARD,
	//?? add BEAM_FLAT
	BEAM_STAR,
	BEAM_CYLINDER
};


struct beam_t
{
	CVec3			start;
	CVec3			end;
	float			radius;
	beamType_t		type;
	color_t			color;
	CBasicImage		*shader;
	// internal fields
	float			alpha;			// color.c[3] is byte and have no enough precision
	float			fadeTime;
	float			lifeTime;
	float			dstAlpha;		// alpha, which will be reached at end of life cycle (default==0)
	float			growSpeed;		// radius_delta/sec
	// temporary fields
	CVec3			drawStart, drawEnd;

	beam_t			*next;
	beam_t			*drawNext;
};


struct dlight_t
{
	CVec3			origin;
	CVec3			color;			//?? make color_t
	float			intensity;
};


enum particleType_t
{
	PT_DEFAULT,
	PT_SPARKLE
};

struct particle_t
{
	// appearance
	CVec3			org;
	byte			color;			// paletted
	bool			isNew;
	particleType_t	type;
	float			alpha;
	const CBspLeaf	*leaf;			// NULL -- uninitialized
	// some physics
	CVec3			vel;			// org += vel * time
	CVec3			accel;			// vel += accel * time
	float			alphavel;		// alpha/sec

	particle_t		*next;
	particle_t		*drawNext;
};

struct lightstyle_t
{
	byte			length;
	byte			value;			// current lighting; monochrome; 0 .. 255 -> 0.0 .. 2.0
	byte			map[MAX_QPATH];
};

struct refdef_t
{
	int				x, y, width, height; // using virtual screen coordinates
	float			fov_x, fov_y;
	CVec3			vieworg;
	CVec3			viewangles;
	double			time;			// time for shader effects
	unsigned		rdflags;		// RDF_UNDERWATER, etc

	byte			*zonebits;		// if not NULL, only zones with set bits will be drawn

	int				num_entities;
	entity_t		*entities;

	int				num_dlights;
	dlight_t		*dlights;

	lightstyle_t	*lightstyles;	// [MAX_LIGHTSTYLES]
	particle_t		*particles;
	beam_t			*beams;
};


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

void InitRendererVars();

extern	cvar_t	*r_fullscreen;
extern	cvar_t	*r_gamma, *r_brightness, *r_contrast, *r_saturation;
#if !NO_DEBUG
extern	cvar_t	*r_fullbright, *r_lightmap;
#endif


/*------------------- Static/dynamic renderer ----------------------------*/

#include "rexp_intf.h"

//?? rename refImport_t, refExport_t to something another ...

typedef	bool (*CreateDynRenderer_t) (const refImport_t *, refExport_t *);

#if !STATIC_BUILD

#define RENDERER_EXPORT		\
	extern "C" DLL_EXPORT bool CreateRenderer(const refImport_t *rimp, refExport_t *rexp) \
	{						\
		if (rimp.struc_size != sizeof(refImport_t) || \
			rexp->struc_size != sizeof(refExport_t)) \
			return false;	\
		ri = *rimp;			\
		*rexp = re;			\
		InitRendererVars();\
		return true;		\
	}

#else

#define RENDERER_EXPORT				// empty for STATIC_BUILD

#endif


#endif
