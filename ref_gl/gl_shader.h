#ifndef __GL_SHADER_INCLUDED__
#define __GL_SHADER_INCLUDED__


/*-------------- Shader parameters -------------*/

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
	TCGEN_FOG									//?? used for fog image
} tcGenType_t;

typedef enum
{
	TCMOD_TRANSFORM,			// uses transform matrix (2 x 2) + (2 x 1)
	TCMOD_TURB,					// wave(sin), but without base (or: base is vertex coords)
	TCMOD_WARP,					// standard fx for quake "SURF_WARP"
	TCMOD_SCROLL,				// coord[i] += speed[i] * shader.time
	TCMOD_ENTITYTRANSLATE,		// == SCROLL with speeds, taken from entity
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
	RGBGEN_BOOST_VERTEX,		// vertex color with boosted
	RGBGEN_ENTITY,
	RGBGEN_ONE_MINUS_ENTITY,
	RGBGEN_WAVE,
	RGBGEN_LIGHTING_DIFFUSE,
	RGBGEN_FOG					//??
} rgbGenType_t;

typedef enum
{
	ALPHAGEN_IDENTITY,			// => alphaGen const 1
	ALPHAGEN_CONST,
//	ALPHAGEN_NONE,				//?? use const (is it needed?)
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
	DEFORM_PROJECTIONSHADOW,
	DEFORM_AUTOSPRITE,
	DEFORM_AUTOSPRITE2
} deformType_t;


/*---------------- Shader stage ----------------*/

#define MAX_SHADER_DEFORMS	3
#define MAX_STAGE_TCMODS	4
#define MAX_SHADER_STAGES	32
#define MAX_STAGE_TEXTURES	128

typedef struct
{
	tcModType_t type;
	union {
		struct {	// TCMOD_ROTATE
			float	rotateSpeed;
		};
		struct {	// TCMOD_SCROLL
			float	sSpeed, tSpeed;
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
} tcModParms_t;

typedef struct
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
			float	move_x, move_y, move_z;
		};
	};
} deformParms_t;


typedef struct
{
	// GL_State ...
	int		glState;

	byte	isLightmap;
	byte	detail;					//?? true is stage is detail (unused ??)

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
	vec3_t	tcGenVec[2];			// for TCGEN_VECTOR
	// tcMod params
	int		numTcMods;
	tcModParms_t *tcModParms;
	// images: variable length
	int		numAnimTextures;		// in range [1..MAX_STAGE_TEXTURES]; if 0 -- ignore stage (and treat previous as last)
	float	animMapFreq;			// valid only when numAnimTextures > 1 && (frameFromEntity == false || model = WORLD)
	byte	frameFromEntity;		// bool
	image_t	*mapImage[1];
} shaderStage_t;


/*--------------- Shader itself ----------------*/

typedef enum
{
	SHADERTYPE_NORMAL,
	SHADERTYPE_SKY,
	SHADERTYPE_FOG,			//?? make this as non-shader?
	SHADERTYPE_PORTAL
} shaderType_t;


typedef struct shader_s
{
	char	name[MAX_QPATH];

	int		lightmapNumber;
	union {
		byte	lightStyles[4];	// 0 - unused; can be 1..31
		int		lightStyles_i;
	};
	int		sortIndex;
	float	sortParam;		// values from sortParam_t, but in float representation
	int		sortParam2;		// secondary sort values (main image, lightmap num etc.)
	int		width, height;	// required due to Q2 architecture; if contains default image, both are zero

	int		style;			// SHADER_XXX
//	int		surfaceFlags;	// SURF_XXX			??
//	int		contentsFlags;	// CONTENTS_XXX		??
	float	tessSize;		// used for warp surface subdivision

	gl_cullMode_t cullMode;

	//!! make bitfield (bool)
	byte	scripted;
	byte	bad;			// errors in script or no map image found (for auto-generated shader)
	byte	fast;			// have no deforms, tcGen/tcMod, rgb/alpha-gen (remove ??)
	byte	dependOnEntity;	// when false, surface may be mixed with surfaces from different entities

	byte	usePolygonOffset;

	int		numDeforms;
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
			vec3_t	skyAxis;
		};
		// portal params (SHADERTYPE_PORTAL)
/*		struct {
			float	portalRange;
		};
	}; */

	// remap shader
	struct shader_s *alphaShader;		// for skins: same shader as current, but translucent

	struct shader_s *hashNext;

	// stages: variable length
	int		numStages;
	shaderStage_t *stages[1];
} shader_t;


#endif
