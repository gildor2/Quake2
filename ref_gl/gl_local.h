#ifndef __GL_LOCAL_INCLUDED__
#define __GL_LOCAL_INCLUDED__


#include <stdio.h>
#include <math.h>

#include "../client/ref.h"
#include "qgl.h"

#define	REF_VERSION	"GL 0.99"	//!! will be 1.00 when I'll finish ...

#ifdef REF_HARD_LINKED

#define GetRefAPI			GL_GetRefAPI
#define Vid_CreateWindow	GL_CreateWindow

#endif


typedef struct
{
	unsigned	width, height;		// coordinates from main game
} viddef_t;

extern viddef_t vid;


/*-------------- gl_image.c -----------------*/

// consts for image_t.flags
#define IMAGE_CLAMP			1			// 0 -- repeat
#define	IMAGE_MIPMAP		2
#define IMAGE_PICMIP		4
#define IMAGE_SYSTEM		8			// auto-generated image (default, fog, dlight etc.)
// hints for image creating
#define IMAGE_TRUECOLOR		0x80000000	// hint for GL_CreateImage: force to upload image in 32 bit
#define IMAGE_LIGHTMAP		0x40000000
#define IMAGE_NOCOMPRESS	0x20000000
// mask of flags, stored in image (exclude hints)
#define IMAGE_FLAGMASK		0x0000FFFF

typedef struct image_s
{
	char	name[MAX_QPATH];			// lowercased game-relative image name (system images started with "*")
	int		width, height;				// statistics (?)
	int		alphaType;					// 0 - none, 1 - 1-bit alpha, 2 - 8-bit alpha
	int		internalWidth, internalHeight;	// statistics
	int		internalFormat;				// statistics (for imagelist)
	int		registrationSequence;		// for detecting unused images
	int		texnum;						// gl texture binding
	int		flags;
	struct image_s *hashNext;
} image_t;


typedef enum
{
	CULL_FRONT,
	CULL_BACK,
	CULL_NONE
} gl_cullMode_t;

// GL_State constants
// source blend modes
#define GLSTATE_SRCMASK		0xF
#define GLSTATE_SRCSHIFT	0
#define GLSTATE_SRC_ZERO				1
#define GLSTATE_SRC_ONE					2
#define GLSTATE_SRC_DSTCOLOR			3
#define GLSTATE_SRC_ONEMINUSDSTCOLOR	4
#define GLSTATE_SRC_SRCALPHA			5
#define GLSTATE_SCR_ONEMINUSSRCALPHA	6
#define GLSTATE_SRC_DSTALPHA			7
#define GLSTATE_SRC_ONEMINUSDSTALPHA	8
#define GLSTATE_SRC_SRCALPHASATURATE	9
// destination blend modes
#define GLSTATE_DSTMASK		0xF0
#define GLSTATE_DSTSHIFT	4
#define GLSTATE_DST_ZERO				0x10
#define GLSTATE_DST_ONE					0x20
#define GLSTATE_DST_SRCCOLOR			0x30
#define GLSTATE_DST_ONEMINUSSRCCOLOR	0x40
#define GLSTATE_DST_SRCALPHA			0x50
#define GLSTATE_DST_ONEMINUSSRCALPHA	0x60
#define GLSTATE_DST_DSTALPHA			0x70
#define GLSTATE_DST_ONEMINUSDSTALPHA	0x80
// alpha function (0 - disabled)
#define GLSTATE_ALPHAMASK	0xF00
#define GLSTATE_ALPHA_GT0				0x100
#define GLSTATE_ALPHA_LT05				0x200
#define GLSTATE_ALPHA_GE05				0x300
// depth buffer
#define GLSTATE_DEPTHWRITE				0x1000
#define GLSTATE_NODEPTHTEST				0x2000
#define GLSTATE_DEPTHEQUALFUNC			0x4000	// 0 - GL_LEQUAL, 1 - GL_EQUAL
// polygon mode (fill/line) (needed ??)
#define GLSTATE_POLYGON_LINE			0x8000

extern	image_t		*gl_defaultImage;
extern	image_t		*gl_whiteImage;
extern	image_t		*gl_identityLightImage;
extern	image_t		*gl_dlightImage;
extern	image_t		*gl_particleImage;
extern	image_t		*gl_fogImage;

// mode changing
void	GL_Bind (image_t *tex);
void	GL_SelectTexture (int tmu);
void	GL_CullFace (gl_cullMode_t mode);
void	GL_TexEnv (int env);
void	GL_State (int state);
void	GL_TextureMode (char *name);

// setup
void	GL_SetupGamma (void);
void	GL_InitImages (void);
void	GL_ShutdownImages (void);

// creating images
image_t *GL_CreateImage (char *name, void *pic, int width, int height, int flags);
image_t *GL_FindImage (char *name, int flags);

// video support
void	GL_SetRawPalette (const unsigned char *palette);
void	GL_DrawStretchRaw (int x, int y, int w, int h, int width, int height, byte *pic);

void GL_ShowImages (void);


/*------------ gl_shader.c ------------------*/

// empty declaration
typedef struct shader_s shader_t;

extern	shader_t	*gl_defaultShader;
extern	shader_t	*gl_identityLightShader;
extern	shader_t	*gl_concharsShader;
extern	shader_t	*gl_defaultSkyShader;
extern	shader_t	*gl_skyShader;
extern	shader_t	*gl_alphaShader1, *gl_alphaShader2;

void	GL_InitShaders (void);
void	GL_ShutdownShaders (void);
void	GL_ResetShaders (void);	// should be called every time before loading a new map

// lightmap types (negative numbers -- no lightmap stage, >= 0 -- has lightmap stage)
#define LIGHTMAP_NONE		(-1)
#define LIGHTMAP_VERTEX		(-2)		// no lightmap, but use vertex lighting instead
#define LIGHTMAP_RESERVE	(1024-1)	// lightmap will be set when valid number specified in subsequent FindShader() call

// shader styles for auto-generation (if script is not found)
#define SHADER_SCROLL		1			// SURF_FLOWING (tcMod scroll -1.4 0 ?)
#define SHADER_TURB			2			// SURF_WARP (tcMod turb ...?)
#define SHADER_TRANS33		4			// SURF_TRANS33 (alphaGen const 0.33, blend)
#define SHADER_TRANS66		8			// SURF_TRANS66 (alphaGen const 0.66, blend)
#define SHADER_FORCEALPHA	0x10		// for rgbGen vertex (image itself may be without alpha-channel)
#define SHADER_ALPHA		0x20		// use texture's alpha channel (depends on itage.alphaType: 0->none, 1->alphaTest or blend, 2->blend)
#define SHADER_WALL			0x40		// shader used as a wall texture (not GUI 2D image), also do mipmap
#define SHADER_SKY			0x80		// SURF_SKY (use stage iterator for sky)
#define SHADER_ANIM			0x100		// main stage will contain more than 1 texture (names passed as name1<0>name2<0>...nameN<0><0>)
#define SHADER_LIGHTMAP		0x200		// reserve lightmap stage (need GL_SetShaderLightmap() later)
#define SHADER_TRYLIGHTMAP	0x400		// usualy not containing lightmap, but if present - generate it
// styles (hints) valid for FindShader(), buf not stored in shader_t
#define SHADER_ABSTRACT		0x40000000	// create shader without stages
#define SHADER_CHECK		0x80000000	// if shader doesn't exists, FindShader() will return NULL and do not generate error
// mask of styles, stored to shader (exclude hints)
#define SHADER_STYLEMASK	0x0000FFFF

shader_t *GL_FindShader (char *name, int style);
shader_t *GL_SetShaderLightmap (shader_t *shader, int lightmapNumber);
shader_t *GL_GetAlphaShader (shader_t *shader);
shader_t *GL_GetShaderByNum (int num);


/*------------ gl_model.c -------------------*/

// empty declaration
typedef struct model_s model_t;
typedef struct surfaceCommon_s surfaceCommon_t;
typedef struct surfacePlanar_s surfacePlanar_t;
typedef struct dynamicLightmap_s dynamicLightmap_t;

void	GL_InitModels (void);
void	GL_ShutdownModels (void);
void	GL_ResetModels (void);

model_t	*GL_FindModel (char *name);
shader_t *GL_FindSkin (char *name);
void	GL_LoadWorldMap (char *name);

void	GL_UpdateDynamicLightmap (shader_t *shader, surfacePlanar_t *surf, lightstyle_t *styles);


/*------------ gl_world.c -------------------*/

typedef struct
{
	int		flags;
	// position info
	vec3_t	origin;
	vec3_t	axis[3];
	int		frustumMask;
	vec3_t	modelvieworg;		// vieworg in model coordinate system
	float	modelMatrix[4][4];	// modelview matrix
	int		dist2;				// used for depth sorting entities
	// info for frame lerping
	int		frame, oldFrame;
	float	backLerp;
	// model info
	model_t	*model;
	// shading
	shader_t *customShader;		// one shader for all surfaces
	//?? skin_t *customSkin;	// multiple shaders (1 per surface)
	int		skinNum;			// number of default (inline) skin
	byte	shaderRGBA[4];		// for "rgbGen/alphaGen entity"
} refEntity_t;

extern refEntity_t	gl_entities[];
extern int			gl_numEntities;


void	GL_DrawWorld (void);
void	GL_DrawEntities (int firstEntity, int numEntities);
void	GL_AddEntity (entity_t *ent);


/*------------- gl_main.c -------------------*/


void	GL_Set2DMode (void);

void	DrawTextPos (int x, int y, char *text, float r, float g, float b);
void	DrawTextLeft (char *text, float r, float g, float b);
void	DrawTextRight (char *text, float r, float g, float b);


/*----------- gl_backend.c ------------------*/


void	GL_ClearBuffers (void);

// works with "vp" (current view portal)
void	GL_ClearPortal (void);
void	GL_AddSurfaceToPortal (surfaceCommon_t *surf, shader_t *shader, int entityNum);
void	GL_InsertShaderIndex (int index);
void	GL_FinishPortal (void);

void	GL_BackEnd (void);

void	GL_InitBackend (void);
void	GL_ShutdownBackend (void);


/*-------------------------------------------*/


typedef struct
{
	char	renderer_string[256];
	char	vendor_string[256];
	char	version_string[256];
	char	extensions_string[8192];

	int		maxTextureSize;
	int		extensionMask;

	// multitexturing
	int		maxActiveTextures;		// == 1 if no multitexturing

	// texture compression formats (0 if unavailable)
	int		formatSolid;			// RGB (no alpha)
	int		formatAlpha;			// RGBA (full alpha range)
	int		formatAlpha1;			// RGB_A1 (1 bit for alpha)

	int		colorBits, depthBits, stencilBits;

	qboolean consoleOnly;			// true if graphics disabled

	// gamma
	qboolean deviceSupportsGamma;
	int		overbrightBits;			// gl_overBrightBits->integer
	int		identityLightValue;		// 255/(1<<overbrightBits)
	float	identityLightValue_f;	// 1.0/(1<<overbrightBits)
	qboolean vertexLight;

	// tables
	unsigned tbl_8to32[256];		// palette->RGBA
} glconfig_t;

// macro for checking extension support
#define GL_SUPPORT(ext)		(gl_config.extensionMask & (ext))

typedef struct
{
	//?? NOTE: when changed, need to syncronize with OLD ref_gl
	int		currentBinds[2];
	int		currentEnv[2];
	int		currentTmu;
	int		currentState;
	int		currentCullMode;
	int		currentColor;

	int		maxUsedShaderIndex;
	qboolean finished;	//?? remove
	qboolean is2dMode;

	int		prevMode;				// last valid video mode
	qboolean fullscreen;
	int		colorBits;				//?? 0 == 16, 32
} glstate_t;

//!! clean this structure: most fields used from viewPortal_t; or -- eliminate at all
typedef struct
{
	int		flags;
	lightstyle_t *lightStyles;	// light styles for Q2/HL dynamic lightmaps
	// screen params
	int		x, y, width, height;
	float	fov_x, fov_y;
	// vieworg orientation
	vec3_t	vieworg;
	vec3_t	viewaxis[3];
	int		viewCluster;
	// time in seconds for shader effects etc
	float	time;
	// map areas
	byte	areaMask[MAX_MAP_AREAS/8];
	qboolean areaMaskChanged;
	// entities
	int		firstEntity, numEntities;
} glrefdef_t;

typedef struct surfaceInfo_s
{
	int		sort;
	surfaceCommon_t			*surf;
	struct surfaceInfo_s	*sortNext;
} surfaceInfo_t;

typedef struct
{
	// view params
	vec3_t	vieworg;
	vec3_t	viewaxis[3];
	lightstyle_t *lightStyles;
	float	time;
	// modelview params (unused now, required for portals (??))
	vec3_t	modelorg;		// {0 0 0} for world model (for non-portal view)
	vec3_t	modelaxis[3];	// {1 0 0}, {0 1 0}, {0 0 1} for world model
	vec3_t	modelvieworg;	// coords of vieworg in modelaxis coord system (same as vieworg for world model)
	float	modelMatrix[4][4];
	// projection params
	float	x, y, w, h;		// viewport
	float	fov_x, fov_y;
	cplane_t frustum[4];	// used for frustum culling
	float	projectionMatrix[4][4];
	vec3_t	mins, maxs;		// bounding box of all visible leafs
	float	zFar;			// maximim distance from vieworg to mins/maxs vertexes
	// surfaces
	surfaceInfo_t *surfaces;
	int		numSurfaces;
	// particles
	particle_t *particles;
	int		numParticles;
} viewPortal_t;

typedef struct
{
	// geometry complexity
	int		leafs, visLeafs, frustLeafs;	//?? frustLeafs -> ~cullLeafs
	int		surfs, cullSurfs;
	int		ents, cullEnts;
	// OpenGL statistics
	int		numBinds, numUploads, numIterators;
	// pefromance measuring
	int		beginFrame;		// front-end
	int		beginSort;		// sorting
	int		begin3D;		// back-end (3D)
	int		begin2D;		// back-end (2D)
	int		endFrame;
} drawSpeeds_t;


extern glconfig_t  gl_config;
extern glstate_t   gl_state;
extern glrefdef_t  gl_refdef;

extern viewPortal_t	vp;

extern drawSpeeds_t gl_speeds;


/*----------- Imported functions ------------*/

extern	refImport_t	ri;


/*---- Implementation-specific functions ----*/

typedef enum
{
	rserr_ok,
	rserr_invalid_fullscreen,
	rserr_invalid_mode,
	rserr_unknown
} rserr_t;

void	GLimp_BeginFrame (float camera_separation);
void	GLimp_EndFrame (void);
int 	GLimp_Init (void *hinstance, void *hWnd);
void	GLimp_Shutdown (void);
int     GLimp_SetMode (int *pwidth, int *pheight, int mode, qboolean fullscreen);
void	GLimp_AppActivate (qboolean active);

// logging
void	GLimp_LogMessage (char *text);
#define LOG_STRING(str)		if (gl_logFile->integer) GLimp_LogMessage (str);

qboolean GLimp_HasGamma (void);
void	GLimp_SetGamma (float gamma, float intens);


/*----------------- Cvars -------------------*/

extern cvar_t	*gl_picmip;
extern cvar_t	*gl_textureBits;
extern cvar_t	*gl_roundImagesDown;
extern cvar_t	*r_gamma;
extern cvar_t	*r_saturation;
extern cvar_t	*r_intensity;
extern cvar_t	*gl_overBrightBits;

extern cvar_t	*gl_fastsky;
extern cvar_t	*gl_dynamic;
extern cvar_t	*gl_vertexLight;
extern cvar_t	*gl_ignoreFastPath;

extern cvar_t	*gl_driver;
extern cvar_t	*gl_bitdepth;

extern cvar_t	*gl_nobind;
extern cvar_t	*r_colorMipLevels;	//?? can we implement colorized mips in software? (if no, rename to "gl_")
extern cvar_t	*gl_logFile;
extern cvar_t	*gl_showImages;
extern cvar_t	*r_novis;			//?? ~gl_pvsCull ?
extern cvar_t	*r_nocull;			//?? ~gl_frustumCull ?
extern cvar_t	*gl_facePlaneCull;	//?? gl_backfaceCull ?
extern cvar_t	*gl_sortAlpha;
extern cvar_t	*r_speeds;
extern cvar_t	*gl_lightmap;
extern cvar_t	*gl_showsky;
extern cvar_t	*r_drawworld;
extern cvar_t	*r_drawentities;
extern cvar_t	*gl_showbboxes;
extern cvar_t	*gl_showtris;
extern cvar_t	*gl_singleShader;

extern cvar_t	*gl_clear;
extern cvar_t	*gl_finish;
extern cvar_t	*gl_swapinterval;
extern cvar_t	*gl_drawbuffer;


#endif
