#ifndef __GL_INTERFACE_INCLUDED__
#define __GL_INTERFACE_INCLUDED__


typedef struct
{
	//?? NOTE: when changed, need to syncronize with OLD ref_gl
	char	rendererString[256];
	char	vendorString[256];
	char	versionString[256];
	char	*extensionsString;

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


typedef enum
{
	CULL_FRONT,
	CULL_BACK,
	CULL_NONE
} gl_cullMode_t;	//?? remove this? use GL_ constants?


typedef enum
{
	DEPTH_NEAR,		// Z -> 0          (0, 0)
	DEPTH_HACK,		// Z /= 3          (0, 0.333)
	DEPTH_NORMAL,	// keep original Z (0, 1)
	DEPTH_FAR		// Z -> 1          (1, 1)
} gl_depthMode_t;


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
	gl_cullMode_t currentCullMode;
	gl_depthMode_t currentDepthMode;
	qboolean inverseCull;
	qboolean fogEnabled;

	int		maxUsedShaderIndex;
	qboolean is2dMode;
	qboolean have3d;
} glstate_t;

extern glconfig_t  gl_config;
extern glstate_t   gl_state;


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
void	GL_DepthRange (gl_depthMode_t mode);
void	GL_State (int state);
void	GL_EnableFog (qboolean enable);

void	GL_SetDefaultState (void);
void	GL_Set2DMode (void);
void	GL_Setup (viewPortal_t *port);


#endif
