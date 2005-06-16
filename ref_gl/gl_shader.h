#ifndef __GL_SHADER_INCLUDED__
#define __GL_SHADER_INCLUDED__


#include "gl_image.h"


namespace OpenGLDrv {


/*-----------------------------------------------------------------------------
	Shader parameters
-----------------------------------------------------------------------------*/

/* NOTES:
 *	- SKY: move AddSkySurface() call from backend to frontend, and can eliminate this type at all
 *		(currently used for backend to place sky surfaces 1st in sorted array)
 *	- PORTAL - tech-dependent ??
 *	- OPAQUE/SEETHROUGH - useless (opaque - sorted by shader, seethrough - sort bu BSP)
 *	- DECAL, BANNER, UNDERWATER - ??
 *	- SPRITE (ADDITIVE) - no depthwrite, draw it last (flares only?)
 */
typedef enum
{
	SORT_BAD,
	SORT_PORTAL,	// surface, which will be replaced with other scene
	SORT_SKY,
	SORT_OPAQUE,	// opeque walls etc.
	SORT_DECAL,		// marks on the wall
	SORT_SEETHROUGH,// windows etc. (blending + depthWrite)
	SORT_BANNER,	// ?
	SORT_UNK7,		// ?
	SORT_UNDERWATER,// ?
	SORT_SPRITE		// (ADDITIVE ?) have blending, but no depthWrite
	//?? Q3 have: banner=6, underwater=8,additive=10,nearest=16
} sortParam_t;

typedef enum
{
	FUNC_SIN,
	FUNC_SQUARE,
	FUNC_TRIANGLE,
	FUNC_SAWTOOTH,
	FUNC_INVERSESAWTOOTH,
	FUNC_NOISE
} waveFunc_t;

// Function = (func[type](time*freq) + phase) * amp + base
typedef struct
{
	waveFunc_t type;
	float	freq;
	float	phase;
	float	amp;
	float	base;
} waveParams_t;

typedef enum
{
	TCGEN_NONE,
	TCGEN_TEXTURE,
	TCGEN_LIGHTMAP,
	TCGEN_ENVIRONMENT,
	TCGEN_VECTOR,
	// internal-use modes
	TCGEN_LIGHTMAP1, TCGEN_LIGHTMAP2, TCGEN_LIGHTMAP3, TCGEN_LIGHTMAP4,	// for fast lightstyles
	TCGEN_DLIGHT0,				// 32 values, index is for surfDlights[], not for portal.dlights[]
	TCGEN_ZERO = TCGEN_DLIGHT0 + MAX_DLIGHTS,	//?? s = t = 0 (unused/unimplemented)
	TCGEN_FOG									//?? used for fog image (unimplemented)
} tcGenType_t;

typedef enum
{
	TCMOD_TRANSFORM,			// uses transform matrix (2 x 2) + (2 x 1)
	TCMOD_TURB,					// wave(sin), but without base (or: base is vertex coords)
	TCMOD_WARP,					// standard fx for quake "SURF_WARP"
	TCMOD_SCROLL,				// coord[i] += speed[i] * shader.time
	TCMOD_OFFSET,				// coord[i] += offset[i]
//	TCMOD_ENTITYTRANSLATE,		// == SCROLL with speeds, taken from entity; unused in Q3
	TCMOD_SCALE,				// coord[i] *= scales[i]
	TCMOD_STRETCH,				// coord[i] = (coord[i]-0.5) * wave + 0.5  (wave stretch, center-relative)
	TCMOD_ROTATE				// rotate around center with "rotateSpeed" degree per second
} tcModType_t;

typedef enum
{
	RGBGEN_NONE,
	RGBGEN_IDENTITY_LIGHTING,	// => const r=g=b=identityLight
	RGBGEN_IDENTITY,			// => const r=g=b=1
	RGBGEN_CONST,
	RGBGEN_VERTEX,				// vertex lighting (overbrighted)
	RGBGEN_EXACT_VERTEX,		// not affected by overbrighting
	RGBGEN_ONE_MINUS_VERTEX,
	RGBGEN_BOOST_VERTEX,		// vertex color with boosted value to avoid black
	RGBGEN_ENTITY,
	RGBGEN_ONE_MINUS_ENTITY,
	RGBGEN_WAVE,
	RGBGEN_DIFFUSE,				// internally replaced with HALF_DIFFUSE when color*tex*2 available
	RGBGEN_HALF_DIFFUSE,		// more color range (available double brightness)
	RGBGEN_FOG					//??
} rgbGenType_t;

typedef enum
{
	ALPHAGEN_IDENTITY,			// => alphaGen const 1
	ALPHAGEN_CONST,
	ALPHAGEN_ENTITY,
	ALPHAGEN_ONE_MINUS_ENTITY,
	ALPHAGEN_VERTEX,
	ALPHAGEN_ONE_MINUS_VERTEX,
	ALPHAGEN_DOT,
	ALPHAGEN_ONE_MINUS_DOT,
	ALPHAGEN_LIGHTING_SPECULAR,
	ALPHAGEN_WAVE,
	ALPHAGEN_PORTAL,
} alphaGenType_t;

typedef enum
{
	DEFORM_WAVE,
	DEFORM_NORMAL,
	DEFORM_BULGE,
	DEFORM_MOVE,
//	DEFORM_PROJECTIONSHADOW,
	DEFORM_AUTOSPRITE,
	DEFORM_AUTOSPRITE2
} deformType_t;


/*-----------------------------------------------------------------------------
	Shader stage
-----------------------------------------------------------------------------*/

#define MAX_SHADER_DEFORMS	3
#define MAX_STAGE_TCMODS	4
#define MAX_SHADER_STAGES	32
#define MAX_STAGE_TEXTURES	128

struct tcModParms_t
{
	tcModType_t type;
	union {
		struct {	// TCMOD_ROTATE
			float	rotateSpeed;
		};
		struct {	// TCMOD_SCROLL
			float	sSpeed, tSpeed;
		};
		struct {	// TCMOD_OFFSET
			float	sOffset, tOffset;
		};
		struct {	// TCMOD_SCALE
			float	sScale, tScale;
		};
		struct {	// TCMOD_STRETCH, TCMOD_TURB (type=sin)
			waveParams_t wave;
		};
		struct {	// TCMOD_TRANSFORM
			float	m[2][2];
			float	t[2];
		};
	};
};

struct deformParms_t
{
	deformType_t type;
	union {
		struct {	// DEFORM_WAVE
			waveParams_t wave;
			float	waveDiv;			// additional parameter for wave func
		};
		struct {	// DEFORM_BULGE
			float	bulgeWidth, bulgeHeight, bulgeSpeed;
		};
		struct {	// DEFORM_MOVE
			waveParams_t moveWave;
			CVec3	move;
		};
	};
};


struct shaderStage_t
{
	// GL_State ...
	unsigned glState;

	bool	isLightmap:1;
//	bool	detail:1;				//?? true is stage is detail (unused ??)

	/*---------------- RGBA params ----------------*/
	color_t	rgbaConst;				// if RGBGEN_CONST or ALPHAGEN_CONST
	// rgbGen params
	rgbGenType_t rgbGenType;
	waveParams_t rgbGenWave;		// if RGBGEN_WAVE
	// alphaGen params
	alphaGenType_t alphaGenType;
	waveParams_t alphaGenWave;		// if ALPHAGEN_WAVE
	float	alphaMin, alphaMax;		// if ALPHAGEN_[ONE_MINUS_]DOT
	/*--------------- texture params --------------*/
	// tcGen params
	tcGenType_t tcGenType;
	CVec3	tcGenVec[2];			// for TCGEN_VECTOR
	// tcMod params
	int		numTcMods;
	tcModParms_t *tcModParms;
	// images: variable length
	int		numAnimTextures;		// in range [1..MAX_STAGE_TEXTURES]; FinishShader(): if 0 -- ignore stage (and treat previous as last)
	float	animMapFreq;			// valid only when numAnimTextures > 1 && (frameFromEntity == false || model = WORLD)
	bool	frameFromEntity;
	image_t	*mapImage[1];
};


/*-----------------------------------------------------------------------------
	Shader itself
-----------------------------------------------------------------------------*/

typedef enum	//?? change this
{
	SHADERTYPE_NORMAL,
	SHADERTYPE_SKY,
	SHADERTYPE_FOG,			//?? make this as non-shader?
	SHADERTYPE_PORTAL
} shaderType_t;


class shader_t : public CBasicImage
{
public:
	int		lightmapNumber;
	union {
		byte	lightStyles[4];	// 0 - unused; can be 1..31
		int		lightStyles_i;
	};
	int		sortIndex;
	int		sortParam;			// values from sortParam_t
	int		sortParam2;			// secondary sort values (main image, lightmap num etc.)

	unsigned style;				// SHADER_XXX
	float	tessSize;			// used for warp surface subdivision

	gl_cullMode_t cullMode;

	bool	scripted:1;
	bool	bad:1;				// errors in script or no map image found (for auto-generated shader)
	bool	dependOnEntity:1;	// when false, surface may be mixed with surfaces from different entities
	bool	noDraw:1;			// when true, do not draw this surfaces with this shader

	bool	usePolygonOffset:1;

	byte	numDeforms;
	deformParms_t deforms[MAX_SHADER_DEFORMS];

	shaderType_t type;
/*	union
	{
		// fog params (SHADERTYPE_FOG)
		struct {
			float	fogDist;
			float	fogColor[3];
		}; */
		// sky params SHADERTYPE_SKY)
		struct {
//			float	skyCloudHeight;
			image_t	*skyFarBox[6];
//			image_t	*skyNearBox[6];
			float	skyRotate;
			CVec3	skyAxis;
		};
		// portal params (SHADERTYPE_PORTAL)
/*		struct {
			float	portalRange;
		};
	}; */

	// remap shader
	shader_t *alphaShader;		// for skins: same shader as current, but translucent

	shader_t *hashNext;

	// stages: variable length
	int		numStages;
	shaderStage_t *stages[1];	// WARNING: should be last

	// functions
	virtual void Reload ();
};


/*-----------------------------------------------------------------------------
	System shaders
-----------------------------------------------------------------------------*/

extern	shader_t	*gl_defaultShader;
extern	shader_t	*gl_identityLightShader;	// no depth test/write
extern	shader_t	*gl_identityLightShader2;	// with depth test/write
extern	shader_t	*gl_concharsShader;
extern	shader_t	*gl_videoShader;
extern	shader_t	*gl_defaultSkyShader;
extern	shader_t	*gl_particleShader;
extern	shader_t	*gl_entityShader;
extern	shader_t	*gl_flareShader;			// NULL if not found
extern	shader_t	*gl_detailShader;			// NULL if not found
extern	shader_t	*gl_skyShader;
extern	shader_t	*gl_alphaShader1, *gl_alphaShader2;


/*-----------------------------------------------------------------------------
	Functions
-----------------------------------------------------------------------------*/

void	InitShaders (void);
void	ShutdownShaders (void);
void	ResetShaders (void);			// should be called every time before loading a new map

// lightmap types (negative numbers -- no lightmap stage, >= 0 -- has lightmap stage)
#define LIGHTMAP_NONE		(-1)
#define LIGHTMAP_VERTEX		(-2)		// no lightmap, but use vertex lighting instead
#define LIGHTMAP_RESERVE	(1024-1)	// lightmap will be set when valid number specified in subsequent SetShaderLightmap() call

// shader styles for auto-generation (if script is not found)
#define SHADER_SCROLL		0x0001		// SURF_FLOWING (tcMod scroll -1.4 0)
#define SHADER_TURB			0x0002		// SURF_WARP (tcMod warp, tessSize 64)
#define SHADER_TRANS33		0x0004		// SURF_TRANS33 (alphaGen const 0.33, blend)
#define SHADER_TRANS66		0x0008		// SURF_TRANS66 (alphaGen const 0.66, blend)
#define SHADER_FORCEALPHA	0x0010		// for alphaGen vertex (image itself may be without alpha-channel)
#define SHADER_ALPHA		0x0020		// use texture's alpha channel (depends on itage.alphaType: 0->none, 1->alphaTest or blend, 2->blend)
#define SHADER_WALL			0x0040		// shader used as a wall texture (not GUI 2D image), also do mipmap
#define SHADER_SKIN			0x0080		// shader used as skin for frame models
#define SHADER_SKY			0x0100		// SURF_SKY (use surface for sky drawing)
#define SHADER_ANIM			0x0200		// main stage will contain more than 1 texture (names passed as name1<0>name2<0>...nameN<0><0>)
#define SHADER_LIGHTMAP		0x0400		// reserve lightmap stage (need GL_SetShaderLightmap() later)
#define SHADER_TRYLIGHTMAP	0x0800		// usualy not containing lightmap, but if present - generate it
#define SHADER_ENVMAP		0x1000		// make additional rendering pass with specular environment map
#define SHADER_ENVMAP2		0x2000		// add diffuse environment map
#define SHADER_CLAMP		0x4000		// load image with IMAGE_CLAMP flag

// mask of styles, stored to shader (exclude hints)
#define SHADER_STYLEMASK	0x0000FFFF

// styles (hints) valid for FindShader(), buf not stored in shader_t
#define SHADER_ABSTRACT		0x20000000	// create shader without stages
#define SHADER_CHECK		0x40000000	// if shader doesn't exists, FindShader() will return NULL and do not generate error
#define SHADER_CHECKLOADED	0x80000000	// if shader loaded, return it, else - NULL

shader_t *FindShader (const char *name, unsigned style = 0);
shader_t *SetShaderLightmap (shader_t *shader, int lightmapNumber);
shader_t *SetShaderLightstyles (shader_t *shader, unsigned styles);
shader_t *GetAlphaShader (shader_t *shader);
shader_t *GetShaderByNum (int num);		//?? for backend: sort index -> shader


} // namespace

#endif
