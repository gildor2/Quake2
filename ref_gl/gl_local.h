#ifndef __GL_LOCAL_INCLUDED__
#define __GL_LOCAL_INCLUDED__


#include <stdio.h>
#include <math.h>

#ifdef _WIN32
// need this include, because have wgl and GDI functions here
#  include <windows.h>
#endif

#include "gl.h"

#ifdef __linux__
//#include <GL/fxmesa.h>
#include <GL/glx.h>
#endif


#include "../client/ref.h"


qboolean QGL_Init (const char *dllname);
void	QGL_InitExtensions (void);
void	QGL_Shutdown (void);
// logging
void	QGL_EnableLogging (qboolean enable);
void	QGL_LogMessage (char *text);
#define LOG_STRING(str)		if (gl_logFile->integer) QGL_LogMessage (str);

#ifndef APIENTRY
#  define APIENTRY
#endif


#include "qgl_decl.h"


#define	REF_VERSION	"GL 0.99"	//!! will be 1.00 when I'll finish ...

#ifdef REF_HARD_LINKED

#define GetRefAPI			GL_GetRefAPI

#endif


//?? find a way to remove this struc (use vid_width and vid_height or vid_size[2] ??)
typedef struct
{
	unsigned	width, height;		// coordinates from main game
} viddef_t;

extern viddef_t vid;


typedef union
{
	byte	c[4];
	int		rgba;
} color_t;


// forwards
typedef struct viewPortal_s viewPortal_t;


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


extern	image_t		*gl_defaultImage;
//??extern	image_t		*gl_whiteImage;
extern	image_t		*gl_identityLightImage;
extern	image_t		*gl_dlightImage;
extern	image_t		*gl_particleImage;
extern	image_t		*gl_fogImage;
extern	image_t		*gl_reflImage;		// NULL if not found

extern int 	gl_screenshotFlags;
extern char	*gl_screenshotName;

void	GL_TextureMode (char *name);

void	GL_PerformScreenshot (void);

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


/*------------ gl_interface.c ---------------*/


typedef struct
{
	//?? NOTE: when changed, need to syncronize with OLD ref_gl
	char	renderer_string[256];
	char	vendor_string[256];
	char	version_string[256];
	char	extensions_string[8192];	//?? make dynamic

	int		maxTextureSize;
	int		extensionMask;

	// multitexturing
	int		maxActiveTextures;		// == 1 if no multitexturing
	qboolean lightmapOverbright;	// when true, lightmaps lightscaled by 2 (hardware unable to prform src*dst*2 blend)
	qboolean multiPassLM;			// when false, upload dynamic lightmaps

	// texture compression formats (0 if unavailable)
	int		formatSolid;			// RGB (no alpha)
	int		formatAlpha;			// RGBA (full alpha range)
	int		formatAlpha1;			// RGB_A1 (1 bit for alpha)

	int		colorBits;
	byte	fullscreen;
	byte	prevMode;				// last valid video mode
	byte	prevBPP;
	byte	prevFS;

	byte	consoleOnly;			// true if graphics disabled (can use ref_flags & REF_CONSOLE_ONLY !!)

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
	qboolean locked;
	// up to 32 texture units supports by OpenGL 1.3
	int		currentTmu;
	image_t	*currentBinds[32];
	unsigned currentEnv[32];
	byte	texCoordEnabled[32];
	byte	textureEnabled[32];
	color_t	texEnvColor[32];
	// fields for locked state
	int		newTmu;
	image_t	*newBinds[32];
	unsigned newEnv[32];
	byte	newTexCoordEnabled[32];
	byte	newTextureEnabled[32];
	void	*newTCPointer[32];
	color_t	newEnvColor[32];

	int		currentState;
	int		currentCullMode;
	qboolean inverseCull;
	qboolean fogEnabled;

	int		maxUsedShaderIndex;
	qboolean is2dMode;
	qboolean have3d;
} glstate_t;

extern glconfig_t  gl_config;
extern glstate_t   gl_state;


typedef enum
{
	CULL_FRONT,
	CULL_BACK,
	CULL_NONE
} gl_cullMode_t;	//?? remove this? use GL_ constants?


// GL_State constants

// source blend modes
#define GLSTATE_SRCMASK		0xF
#define GLSTATE_SRCSHIFT	0
#define GLSTATE_SRC_ZERO				1
#define GLSTATE_SRC_ONE					2
#define GLSTATE_SRC_SRCCOLOR			3		// inaccessible in OpenGL 1.1
#define GLSTATE_SRC_ONEMINUSSRCCOLOR	4		// ---
#define GLSTATE_SRC_SRCALPHA			5
#define GLSTATE_SRC_ONEMINUSSRCALPHA	6
#define GLSTATE_SRC_DSTCOLOR			7
#define GLSTATE_SRC_ONEMINUSDSTCOLOR	8
#define GLSTATE_SRC_DSTALPHA			9
#define GLSTATE_SRC_ONEMINUSDSTALPHA	0xA
#define GLSTATE_SRC_SRCALPHASATURATE	0xB
// destination blend modes (same set as source blend, const_src = const_dst>>4)
#define GLSTATE_DSTMASK		0xF0
#define GLSTATE_DSTSHIFT	4
#define GLSTATE_DST_ZERO				0x10
#define GLSTATE_DST_ONE					0x20
#define GLSTATE_DST_SRCCOLOR			0x30
#define GLSTATE_DST_ONEMINUSSRCCOLOR	0x40
#define GLSTATE_DST_SRCALPHA			0x50
#define GLSTATE_DST_ONEMINUSSRCALPHA	0x60
#define GLSTATE_DST_DSTCOLOR			0x70	// ---
#define GLSTATE_DST_ONEMINUSDSTCOLOR	0x80	// ---
#define GLSTATE_DST_DSTALPHA			0x90
#define GLSTATE_DST_ONEMINUSDSTALPHA	0xA0
// alpha function (0 - disabled)
#define GLSTATE_ALPHAMASK	0xF00
#define GLSTATE_ALPHA_GT0				0x100
#define GLSTATE_ALPHA_LT05				0x200
#define GLSTATE_ALPHA_GE05				0x300
// depth buffer
#define GLSTATE_DEPTHWRITE				0x1000
#define GLSTATE_NODEPTHTEST				0x2000
#define GLSTATE_DEPTHEQUALFUNC			0x4000	// 0 - GL_LEQUAL, 1 - GL_EQUAL
// polygon mode (fill/line)
#define GLSTATE_POLYGON_LINE			0x8000


// GL_TexEnv constants

// standard modes (0 - undefined)
#define TEXENV_REPLACE				1
#define TEXENV_MODULATE				2
#define TEXENV_ADD					3
// combine_arb modes
#define TEXENV_C_MODULATE			4
#define TEXENV_C_ADD				5
#define TEXENV_C_INTERP				6
// combine4_nv mode(s)
#define TEXENV_C4_ADD				7

#define TEXENV_FUNC_MASK			15		// 4 bits

// some flags
#define TEXENV_MUL2					0x10
#define TEXENV_ENVCOLOR				0x20

// source selection (0 - undefined)
#define TEXENV_TEXTURE				1
#define TEXENV_ONE_MINUS_TEXTURE	2
#define TEXENV_TEXALPHA				3
#define TEXENV_ONE_MINUS_TEXALPHA	4
#define TEXENV_PREVIOUS				5
#define TEXENV_ONE_MINUS_PREVIOUS	6
#define TEXENV_PREVALPHA			7
#define TEXENV_ONE_MINUS_PREVALPHA	8
#define TEXENV_CONSTANT				9		// environment color
#define TEXENV_ONE_MINUS_CONSTANT	10
#define TEXENV_COLOR2				11		// primary color     !!! rename to _COLOR (temporary name to remove old name usage)
#define TEXENV_ONE_MINUS_COLOR		12
// combine4_nv:
#define TEXENV_ZERO					13
#define TEXENV_ONE					14


#define TEXENV_SRC_BITS				4
#define TEXENV_SRC_MASK				((1<<TEXENV_SRC_BITS) - 1)

#define TEXENV_SRC0_SHIFT			(32-TEXENV_SRC_BITS)
#define TEXENV_SRC1_SHIFT			(32-TEXENV_SRC_BITS*2)
#define TEXENV_SRC2_SHIFT			(32-TEXENV_SRC_BITS*3)
#define TEXENV_SRC3_SHIFT			(32-TEXENV_SRC_BITS*4)

#define TEXENV_SRC0_MASK			(TEXENV_SRC_MASK<<TEXENV_SRC0_SHIFT)
#define TEXENV_SRC1_MASK			(TEXENV_SRC_MASK<<TEXENV_SRC1_SHIFT)
#define TEXENV_SRC2_MASK			(TEXENV_SRC_MASK<<TEXENV_SRC2_SHIFT)
#define TEXENV_SRC3_MASK			(TEXENV_SRC_MASK<<TEXENV_SRC3_SHIFT)

// some common defines
#define TEXENV_0PREV_1TEX			((TEXENV_PREVIOUS<<TEXENV_SRC0_SHIFT)|(TEXENV_TEXTURE<<TEXENV_SRC1_SHIFT))


void	GL_Lock (void);
void	GL_Unlock (void);

void	GL_Bind (image_t *tex);
void	GL_BindForce (image_t *tex);

void	GL_SelectTexture (int tmu);
void	GL_TexCoordPointer (void *ptr);
void	GL_TexEnv (unsigned env);
void	GL_TexEnvColor (color_t *c);
void	GL_SetMultitexture (int level);
void	GL_DisableTexCoordArrays (void);

void	GL_CullFace (gl_cullMode_t mode);
void	GL_State (int state);
void	GL_EnableFog (qboolean enable);

void	GL_SetDefaultState (void);
void	GL_Set2DMode (void);
void	GL_Setup (viewPortal_t *port);


/*------------ gl_shader.c ------------------*/

// empty declaration
typedef struct shader_s shader_t;

extern	shader_t	*gl_defaultShader;
extern	shader_t	*gl_identityLightShader;	// no depth test/write
extern	shader_t	*gl_identityLightShader2;	// with depth test/write
extern	shader_t	*gl_concharsShader;
extern	shader_t	*gl_defaultSkyShader;
extern	shader_t	*gl_particleShader;
extern	shader_t	*gl_flareShader;			// NULL if not found
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
#define SHADER_FORCEALPHA	0x10		// for alphaGen vertex (image itself may be without alpha-channel)
#define SHADER_ALPHA		0x20		// use texture's alpha channel (depends on itage.alphaType: 0->none, 1->alphaTest or blend, 2->blend)
#define SHADER_WALL			0x40		// shader used as a wall texture (not GUI 2D image), also do mipmap
#define SHADER_SKY			0x80		// SURF_SKY (use stage iterator for sky)
#define SHADER_ANIM			0x100		// main stage will contain more than 1 texture (names passed as name1<0>name2<0>...nameN<0><0>)
#define SHADER_LIGHTMAP		0x200		// reserve lightmap stage (need GL_SetShaderLightmap() later)
#define SHADER_TRYLIGHTMAP	0x400		// usualy not containing lightmap, but if present - generate it
#define SHADER_ENVMAP		0x800		// make additional rendering pass with default environment map
// styles (hints) valid for FindShader(), buf not stored in shader_t
#define SHADER_ABSTRACT		0x20000000	// create shader without stages
#define SHADER_CHECK		0x40000000	// if shader doesn't exists, FindShader() will return NULL and do not generate error
#define SHADER_CHECKLOADED	0x80000000	// if shader loaded, return it, else - NULL
// mask of styles, stored to shader (exclude hints)
#define SHADER_STYLEMASK	0x0000FFFF

shader_t *GL_FindShader (char *name, int style);
shader_t *GL_SetShaderLightmap (shader_t *shader, int lightmapNumber);
shader_t *GL_SetShaderLightstyles (shader_t *shader, int styles);
shader_t *GL_GetAlphaShader (shader_t *shader);
shader_t *GL_GetShaderByNum (int num);


/*------------ gl_model.c -------------------*/

// empty declarations
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

void	GL_UpdateDynamicLightmap (shader_t *shader, surfacePlanar_t *surf, qboolean vertexOnly, unsigned dlightMask);


/*------------ gl_world.c -------------------*/


//?? move to gl_state/gl_config ??
GLenum		gl_fogMode;
float		gl_fogColor[4];
float		gl_fogDensity;
float		gl_fogStart, gl_fogEnd;


//?? rename (gl_entity_t ? entity_t) and separate beam
typedef struct refEntity_s
{
	int		flags;
	model_t	*model;
	float	dist2;						// Z-coordinate of model
	qboolean visible;					// valid for current frame

	union {
		struct {
		/*-------- entity with model --------*/
			// position info
			vec3_t	origin;
			vec3_t	axis[3];
			byte	worldMatrix;		// bool
			byte	mirror;				// bool
			byte	frustumMask;		//?? remove
			vec3_t	modelvieworg;		// vieworg in model coordinate system
			float	modelMatrix[4][4];	// modelview matrix
			// info for frame lerping
			int		frame, oldFrame;
			float	backLerp;
			// shading
			shader_t *customShader;		// one shader for all surfaces
			//?? skin_t *customSkin;	// multiple shaders (1 per surface)
			int		skinNum;			// number of default (inline) skin
		};
		struct {
		/*------------- beam ----------------*/
			vec3_t	beamStart, beamEnd;
			float	beamRadius;
		};
	};

	color_t		shaderColor;			// for "rgbGen/alphaGen entity"
	// draw sequence
	struct refEntity_s *drawNext;
} refEntity_t;

extern refEntity_t	gl_entities[];
extern int			gl_numEntities;


typedef struct
{
	vec3_t	origin;
	float	intensity;
	vec3_t	modelOrg;					// temporaty: origin in model coordinates
	color_t	c;
} refDlight_t;

extern refDlight_t	gl_dlights[];
extern int			gl_numDlights;


void	GL_DrawPortal (void);
void	GL_AddEntity (entity_t *ent);
void	GL_AddDlight (dlight_t *dl);


/*------------ gl_buffers.c -----------------*/


typedef struct surfaceInfo_s
{
	int		sort;
	surfaceCommon_t			*surf;
	struct surfaceInfo_s	*sortNext;
} surfaceInfo_t;


typedef struct viewPortal_s
{
	int		flags;
	float	time;				// time in seconds (for shader effects etc)
	lightstyle_t *lightStyles;	// light styles for Q2/HL dynamic lightmaps
	refDlight_t *dlights;
	int		numDlights;
	// view params
	vec3_t	vieworg;
	vec3_t	viewaxis[3];
	// modelview params (unused now, required for portals (??))
/* following fields may be useful when Q3 portals will be implemented (or useless ??)
	vec3_t	modelorg;			// {0 0 0} for world model (for non-portal view)
	vec3_t	modelaxis[3];		// {1 0 0}, {0 1 0}, {0 0 1} for world model
	vec3_t	modelvieworg;		// coords of vieworg in modelaxis coord system (same as vieworg for world model) */
	float	modelMatrix[4][4];
	// projection params
	float	x, y, w, h;			// viewport
	float	fov_x, fov_y, fov_scale;
	cplane_t frustum[4];		// used for frustum culling
	float	projectionMatrix[4][4];
	vec3_t	mins, maxs;			// bounding box of all visible leafs
	float	zFar;				// maximim distance from vieworg to mins/maxs vertexes
	// surfaces
	surfaceInfo_t *surfaces;
	int		numSurfaces;
	// entities
	int		firstEntity, numEntities;
	// particles
	particle_t *particles;
} viewPortal_t;


void	*GL_AllocDynamicMemory (int size);
void	GL_ResizeDynamicMemory (void *ptr, int newSize);
void	GL_ClearBuffers (void);
void	GL_ClearPortal (void);
void	GL_AddSurfaceToPortal (surfaceCommon_t *surf, shader_t *shader, int entityNum, int numDlights);
surfaceCommon_t *GL_AddDynamicSurface (shader_t *shader, int entityNum);
void	GL_InsertShaderIndex (int index);
void	GL_FinishPortal (void);
void	GL_SortSurfaces (viewPortal_t *port, surfaceInfo_t **destination);


/*------------- gl_main.c -------------------*/


//?? clean this structure: most fields used from viewPortal_t; or -- eliminate at all
typedef struct
{
	int		viewCluster;		//?? place to portal
	// map areas
	byte	areaMask[MAX_MAP_AREAS/8];
	qboolean areaMaskChanged;
} glrefdef_t;


typedef struct
{
	int		numFrames;
	// geometry complexity
	int		leafs, visLeafs, frustLeafs;	//?? frustLeafs -> ~cullLeafs
	int		surfs, cullSurfs;
	int		tris, trisMT;		// number of tris, which will be drawn without mtex and with mtex (same if no multitexture)
	int		ents, cullEnts, cullEnts2;
	int		parts, cullParts;	// particles
	int		flares, cullFlares;
	// OpenGL statistics
	int		numBinds, numUploads, numIterators;
	// pefromance measuring
	int		beginFrame;			// front-end
	int		beginSort;			// sorting
	int		begin3D;			// back-end (3D)
	int		begin2D;			// back-end (2D)
	int		endFrame;
} drawSpeeds_t;


extern glrefdef_t  gl_refdef;
extern viewPortal_t	vp;
extern drawSpeeds_t gl_speeds;


void	DrawTextPos (int x, int y, char *text, float r, float g, float b);
void	DrawTextLeft (char *text, float r, float g, float b);
void	DrawTextRight (char *text, float r, float g, float b);


/*----------- gl_backend.c ------------------*/


void	GL_BackEnd (void);

void	GL_InitBackend (void);
void	GL_ShutdownBackend (void);


/*----------- Imported functions ------------*/

extern	refImport_t	ri;


/*---- Implementation-specific functions ----*/

typedef enum
{	//?? remove
	rserr_ok,
	rserr_invalid_fullscreen,
	rserr_invalid_mode,
	rserr_unknown
} rserr_t;

void	GLimp_BeginFrame (float camera_separation);
void	GLimp_EndFrame (void);
int 	GLimp_Init (void);
void	GLimp_Shutdown (void);
int     GLimp_SetMode (int *pwidth, int *pheight, int mode, qboolean fullscreen);
void	GLimp_AppActivate (qboolean active);

qboolean GLimp_HasGamma (void);
void	GLimp_SetGamma (float gamma, float intens);


/*----------------- Cvars -------------------*/

extern cvar_t	*gl_picmip;
extern cvar_t	*gl_textureBits;
extern cvar_t	*gl_roundImagesDown;
extern cvar_t	*r_gamma;
extern cvar_t	*r_saturation;
extern cvar_t	*r_intensity;
extern cvar_t	*gl_texturemode;
extern cvar_t	*gl_overBrightBits;

extern cvar_t	*gl_fastsky;
extern cvar_t	*gl_fog;
extern cvar_t	*gl_flares;
extern cvar_t	*gl_dynamic;
extern cvar_t	*gl_dlightBacks;
extern cvar_t	*gl_vertexLight;
extern cvar_t	*gl_ignoreFastPath;

extern cvar_t	*gl_driver;
extern cvar_t	*gl_bitdepth;

extern cvar_t	*gl_hand;

extern cvar_t	*gl_nobind;
extern cvar_t	*r_colorMipLevels;	//?? can we implement colorized mips in software? (if no, rename to "gl_")
extern cvar_t	*gl_logFile;
extern cvar_t	*gl_showImages;
extern cvar_t	*r_novis;			//?? ~gl_pvsCull ?
extern cvar_t	*r_nocull;			//?? ~gl_frustumCull ?
extern cvar_t	*gl_facePlaneCull;	//?? gl_backfaceCull ?
extern cvar_t	*gl_sortAlpha;
extern cvar_t	*r_speeds;
extern cvar_t	*r_fullbright;
extern cvar_t	*r_lightmap;
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
