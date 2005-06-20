#ifndef __REF_INCLUDED__
#define __REF_INCLUDED__


// interfaces to some renderer types
class CRenderModel
{
public:
	char	name[MAX_QPATH];
	// return "false" when tag not found
	virtual bool LerpTag (int frame1, int frame2, float lerp, const char *tagName, CCoords &tag) const = NULL;
};


class CBasicImage
{
public:
	char	name[MAX_QPATH];
	short	width, height;
	virtual void Reload (void) = NULL;
};


class CModelSkin
{
public:
	int		numSurfs;
	struct {
		char	surfName[32];		// should be enough
		CBasicImage *shader;
	} surf[32];						// MAX_MD3_SURFACES == 32
	inline bool IsValid ()
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
	CRenderModel *model;
	CCoords	pos;
	CVec3	angles;					// not used by renderer
	int		frame;
	int		oldframe;
	float	backlerp;				// 0.0 = current, 1.0 = old; used for frame models only
	float	scale;					// scale model; 0 == 1 == unscaled
		//?? why we use backlerp instead of simple lerp ?
	// misc
	CVec3	size;					// for RF_BBOX only; size of box
	// color info
	color_t	color;
	float	alpha;					// ignore if RF_TRANSLUCENT isn't set
			//?? use color.c[3]

	// entity shading; priority: 1.skin 2.skinnum 3.customShader
	CBasicImage *customShader;		// single shader for all entity surfaces
	CModelSkin *skin;				// NULL for built-in skin
	int		skinnum;				// number of model built-in skin

	unsigned flags;					// set of RF_XXX flags
	float	time;					// for effects; used as refdef.time-ent.time (starting time of fx)
};


typedef enum
{
	BEAM_STANDARD,
	//?? add BEAM_FLAT
	BEAM_STAR,
	BEAM_CYLINDER
} beamType_t;


struct beam_t
{
	CVec3	start;
	CVec3	end;
	float	radius;
	beamType_t type;
	color_t	color;
	CBasicImage *shader;
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


struct dlight_t
{
	CVec3	origin;
	CVec3	color;					//?? make color_t
	float	intensity;
};


typedef enum
{
	PT_DEFAULT,
	PT_SPARKLE
} particleType_t;

struct particle_t
{
	// appearance
	CVec3	org;
	byte	color;					// paletted
	bool	isNew;
	particleType_t type;
	float	alpha;
	int		leafNum;				// -1 -- uninitialized
	// some physics
	CVec3	vel;					// org += vel * time
	CVec3	accel;					// vel += accel * time
	float	alphavel;				// alpha/sec

	particle_t *next;
	particle_t *drawNext;
};

struct lightstyle_t
{
	byte	length;
	byte	value;					// current lighting; monochrome; 0 .. 255 -> 0.0 .. 2.0
	byte	map[MAX_QPATH];
};

struct refdef_t
{
	int		x, y, width, height;	// in virtual screen coordinates
	float	fov_x, fov_y;
	CVec3	vieworg;
	CVec3	viewangles;
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
};


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

#define RENDERER_EXPORT				// empty for STATIC_BUILD

#endif


#endif
