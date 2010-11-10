#ifndef __GL_INTERFACE_INCLUDED__
#define __GL_INTERFACE_INCLUDED__


// -- namespace declared in "OpenGLDrv.h" -- namespace OpenGLDrv {

// Hardware identitiers for some special cases
// should be a bit mask for easier checking multiple platforms at one step
#define HW_GENERIC		0x00000000		// any `unknown' platform
#define HW_NV			0x00000001		// any NVidia videocard
#define HW_GF2MX		0x00000002		// GeForce2 MX
#define HW_ATI			0x00010000		// any ATi videocard
// NOTE: (HW_GF2MX & HW_NV) == 0


struct glconfig_t
{
	unsigned	platformId;				// HW_xxx ...
	int			maxTextureSize;
	int			maxRectTextureSize;
	unsigned	extensionMask;			// for GL_SUPPORT() macro (used extensions)
	unsigned	disabledExt, ignoredExt; // extensions, disabled by user + covered by a different extension; for gfxinfo()
	unsigned	bugExt;					// buggy extensions, disabled by engine
	// fields for gfxinfo
	const char	*extensions, *extensions2;

	// multitexturing
	int			maxActiveTextures;		// == 1 if no multitexturing

	bool		doubleModulateLM;		// when false, lightmaps lightscaled by 2 (hardware unable to perform src*dst*2 blend)
	bool		multiPassLM;			// when false, upload dynamic lightmaps

	// texture compression formats (0 if unavailable)
	GLenum		formatSolid;			// RGB (no alpha)
	GLenum		formatAlpha;			// RGBA (full alpha range)
	GLenum		formatAlpha1;			// RGB_A1 (1 bit for alpha)

	bool		fullscreen;
	byte		prevMode;				// last valid video mode
	byte		prevBPP;
	bool		prevFS;
	unsigned	width, height;			// viewport dimensions

	// gamma
	bool		deviceSupportsGamma;
	int			overbright;				// 0 - normal, 1 - double gamma
	int			identityLightValue;		// 255/(1<<overbright)
	float		identityLightValue_f;	// 1.0/(1<<overbright)
	bool		vertexLight;

	// tables
	unsigned	tbl_8to32[256];			// palette->RGBA
};

// macro for checking extension support
#define GL_SUPPORT(ext)		(gl_config.extensionMask & (ext))


enum gl_cullMode_t	//?? remove this? use GL_ constants?
{
	CULL_FRONT,
	CULL_BACK,
	CULL_NONE
};


enum gl_depthMode_t
{
	DEPTH_NEAR,		// Z -> 0          (0, 0)
	DEPTH_HACK,		// Z /= 3          (0, 0.333)
	DEPTH_NORMAL,	// keep original Z (0, 1)
	DEPTH_FAR		// Z -> 1          (1, 1)
};


//!! make struc tmuState_t, gl_state_t will have 'tmuState_t _old[32], _new[32]'; GL_SelectTexture() will set
//!! 'tmuState_t *currTmu'; faster copying old->new for GL_Lock, faster GL_Unlock and other funcs ?

struct glstate_t
{
	bool	locked;
	// up to 32 texture units supports by OpenGL 1.3
	byte	currentTmu;
	const image_t *currentBinds[32];
	unsigned currentEnv[32];
	bool	texCoordEnabled[32];
	GLenum	textureTarget[32];					// 0 - disabled, else == currentBinds[]->target
	color_t	texEnvColor[32];
	float	mipBias[32];
	// fields for locked state
	byte	newTmu;
	const image_t *newBinds[32];
	unsigned newEnv[32];
	bool	newTexCoordEnabled[32];
	GLenum	newTextureTarget[32];
	void	*newTCPointer[32];
	color_t	newEnvColor[32];
	float	newMipBias[32];

	unsigned currentState;
	gl_cullMode_t currentCullMode;
	gl_depthMode_t currentDepthMode;
	bool	inverseCull;
	bool	fogEnabled;

	short	maxUsedShaderIndex;
	bool	is2dMode;
	bool	have3d;
	bool	haveFullScreen3d;	// 3D, which fill whole window (incorrect var name? useless?? not works, when scr_viewsize<100)
	bool	useFastSky;
};

extern glconfig_t  gl_config;
extern glstate_t   gl_state;


/*-----------------------------------------------------------------------------
	GL_State constants
-----------------------------------------------------------------------------*/

// blend modes
enum
{
	// names: BLEND_[M_][S|D]_[COLOR|ALPHA] == GL_[ONE_MINUS_]_[SRC|DST]_[COLOR|ALPHA]
	BLEND_0 = 1,
	BLEND_1,
	BLEND_S_COLOR,				// for dst only *
	BLEND_M_S_COLOR,			// for dst only *
	BLEND_S_ALPHA,
	BLEND_M_S_ALPHA,
	BLEND_D_COLOR,				// for src only *
	BLEND_M_D_COLOR,			// for src only *
	BLEND_D_ALPHA,
	BLEND_M_D_ALPHA,
	BLEND_S_ALPHASATURATE		// for src only
};
// NOTE: marked with '*' inaccessible in OpenGL 1.1; appears in OpenGL 1.4 or with GL_NV_blend_square extension
#define GLSTATE_SRCMASK		0xF
#define GLSTATE_SRCSHIFT	0
#define GLSTATE_DSTMASK		0xF0
#define GLSTATE_DSTSHIFT	4
#define GLSTATE_BLENDMASK	(GLSTATE_SRCMASK|GLSTATE_DSTMASK)

#define SRCBLEND(n)				(BLEND_##n << GLSTATE_SRCSHIFT)
#define DSTBLEND(n)				(BLEND_##n << GLSTATE_DSTSHIFT)
#define BLEND(src,dst)			(SRCBLEND(src)|DSTBLEND(dst))

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


/*-----------------------------------------------------------------------------
	GL_TexEnv constants
-----------------------------------------------------------------------------*/

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
// combine3_ati mode(s)
#define TEXENV_C3_ADD				8

#define TEXENV_FUNC_MASK			15		// 4 bits

// some flags
#define TEXENV_MUL2					0x10
#define TEXENV_ENVCOLOR				0x20

// source selection (0 - undefined)
//?? rename like BLEND_XXX, make enum
//??   TEXENV_[M_][P_COLOR|T_COLOR|P_ALPHA|T_ALPHA|CONST|COLOR] | 0|1
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
#define TEXENV_COLOR				11		// primary color
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
#define TEXENV(n,type)				(TEXENV_##type << TEXENV_SRC##n##_SHIFT)
#define TEXENV_0PREV_1TEX			(TEXENV(0,PREVIOUS) | TEXENV(1,TEXTURE))


/*-----------------------------------------------------------------------------
	Functions
-----------------------------------------------------------------------------*/

void	GL_Lock();
void	GL_Unlock();

void	GL_Bind(const image_t *tex);
void	GL_BindForce(const image_t *tex);

void	GL_SelectTexture(int tmu);
void	GL_TexCoordPointer(void *ptr);
void	GL_TexEnv(unsigned env);
void	GL_TexMipBias(float f);
void	GL_TexEnvColor(const color_t *c);
void	GL_SetMultitexture(int level);
void	GL_DisableTexCoordArrays();

void	GL_CullFace(gl_cullMode_t mode);
void	GL_DepthRange(gl_depthMode_t mode);
void	GL_State(unsigned state);
void	GL_EnableFog(bool enable);

void	GL_SetDefaultState();
void	GL_Set2DMode();
void	GL_Set3DMode(viewPortal_t *port);
void	GL_ResetState();

// Function with provided minimal and maximal indices (start and end);
// note: real vertex count is end-start+1, e.g. all indices restricted
// to [start,end] but not to [start,end-1]
inline void GL_DrawRangeElements(GLenum mode, GLuint start, GLuint end,
	GLsizei count, GLenum type, const GLvoid *indices)
{
	if (GL_SUPPORT(QGL_EXT_DRAW_RANGE_ELEMENTS))
		glDrawRangeElementsEXT(mode, start, end, count, type, indices);
	else
		glDrawElements(mode, count, type, indices);
}

//} // namespace

#endif
