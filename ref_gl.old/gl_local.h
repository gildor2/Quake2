/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include <stdio.h>

#ifdef _WIN32
// need this include, because have wgl and GDI functions here
#	include <windows.h>
#endif

#include <GL/gl.h>
#include "../ref_gl/glext.h"		//?? do not duplicate file

// Obsolete (missing in standard headers), but still supported extensions
/* SGIS_multitexture */
#define GL_SELECTED_TEXTURE_SGIS          0x835C
#define GL_MAX_TEXTURES_SGIS              0x835D
#define GL_TEXTURE0_SGIS                  0x835E
#define GL_TEXTURE1_SGIS                  0x835F
#define GL_TEXTURE2_SGIS                  0x8360
#define GL_TEXTURE3_SGIS                  0x8361


#ifdef __linux__
//#include <GL/fxmesa.h>
#	include <GL/glx.h>
#endif

#include "qgl_decl.h"

#include "../client/ref.h"


qboolean QGL_Init (const char *dllname);
void	QGL_InitExtensions (void);
void	QGL_Shutdown (void);
// logging
void	QGL_EnableLogging (qboolean enable);
void	QGL_LogMessage (const char *text);
#define LOG_STRING(str)		if (gl_logFile->integer) QGL_LogMessage (str);


#define	REF_VERSION	"GL 0.01"

#ifdef REF_HARD_LINKED

#define R_RecursiveWorldNode	GLR_RecursiveWorldNode
#define Draw_FindPic		GLDraw_FindPic
#define Draw_InitLocal		GLDraw_InitLocal
#define Draw_Char			GLDraw_Char
#define Draw_CharColor		GLDraw_CharColor
#define R_DrawTexts			GLR_DrawTexts
#define Draw_GetPicSize		GLDraw_GetPicSize
#define Draw_StretchPic		GLDraw_StretchPic
#define Draw_StretchRaw		GLDraw_StretchRaw
#define Draw_Pic			GLDraw_Pic
#define Draw_PicColor		GLDraw_PicColor
#define Draw_TileClear		GLDraw_TileClear
#define Draw_Fill			GLDraw_Fill
#define Draw_Fill2			GLDraw_Fill2
#define R_RegisterSkin		GLR_RegisterSkin
#define R_MarkLights		GLR_MarkLights
#define R_PushDlights		GLR_PushDlights
#define RecursiveLightPoint	GLRecursiveLightPoint
#define R_LightPoint		GLR_LightPoint
#define R_AddDynamicLights	GLR_AddDynamicLights
#define R_BuildLightMap		GLR_BuildLightMap
#define R_Register			GLR_Register
#define R_Init				GLR_Init
#define R_Shutdown			GLR_Shutdown
#define R_MarkLeaves		GLR_MarkLeaves
#define R_DrawNullModel		GLR_DrawNullModel
#define R_DrawEntitiesOnList	GLR_DrawEntitiesOnList
#define R_SetLightLevel		GLR_SetLightLevel
#define R_RenderFrame		GLR_RenderFrame
#define R_BeginFrame		GLR_BeginFrame
#define R_DrawBeam			GLR_DrawBeam
#define R_SetSky			GLR_SetSky
#define Draw_GetPalette		GLDraw_GetPalette
#define R_SetupFrame		GLR_SetupFrame
#define Mod_Modellist_f		GLMod_Modellist_f
#define Mod_Init			GLMod_Init
#define Mod_ForName			GLMod_ForName
#define Mod_PointInLeaf		GLMod_PointInLeaf
#define Mod_DecompressVis	GLMod_DecompressVis
#define Mod_ClusterPVS		GLMod_ClusterPVS
#define Mod_LoadLighting	GLMod_LoadLighting
#define Mod_LoadVisibility	GLMod_LoadVisibility
#define Mod_LoadVertexes	GLMod_LoadVertexes
#define Mod_LoadSubmodels	GLMod_LoadSubmodels
#define Mod_LoadEdges		GLMod_LoadEdges
#define Mod_LoadTexinfo		GLMod_LoadTexinfo
#define CalcSurfaceExtents	GLCalcSurfaceExtents
#define Mod_LoadFaces		GLMod_LoadFaces
#define Mod_SetParent		GLMod_SetParent
#define Mod_LoadNodes		GLMod_LoadNodes
#define Mod_LoadLeafs		GLMod_LoadLeafs
#define Mod_LoadMarksurfaces	GLMod_LoadMarksurfaces
#define Mod_LoadSurfedges	GLMod_LoadSurfedges
#define Mod_LoadPlanes		GLMod_LoadPlanes
#define Mod_LoadBrushModel	GLMod_LoadBrushModel
#define Mod_LoadAliasModel	GLMod_LoadAliasModel
#define Mod_LoadSpriteModel	GLMod_LoadSpriteModel
#define R_BeginRegistration	GLR_BeginRegistration
#define R_RegisterModel		GLR_RegisterModel
#define R_EndRegistration	GLR_EndRegistration
#define Mod_Free			GLMod_Free
#define Mod_FreeAll			GLMod_FreeAll
#define R_DrawParticles		GLR_DrawParticles
#define R_DrawAlphaSurfaces	GLR_DrawAlphaSurfaces
#define R_TextureAnimation	GLR_TextureAnimation

#define r_worldmodel		glr_worldmodel
#define r_avertexnormals	glr_avertexnormals
#define suf	glsuf
#define con_only			glcon_only
#define gammatable			glgammatable

#define Draw_ConCharColor	GLDraw_ConCharColor
#define GetRefAPI			GL_GetRefAPI

#else	// REF_HARD_LINKED

/*extern "C"*/ DLL_EXPORT refExport_t GetRefAPI (refImport_t);

#endif	// REF_HARD_LINKED

// up / down
#define	PITCH	0

// left / right
#define	YAW		1

// fall over
#define	ROLL	2


#ifndef __VIDDEF_T
#define __VIDDEF_T
typedef struct
{
	unsigned		width, height;			// coordinates from main game
} viddef_t;
#endif

extern	viddef_t	vid;

extern byte	gammatable[256];

/*

  skins will be outline flood filled and mip mapped
  pics and sprites with alpha will be outline flood filled
  pic won't be mip mapped

  model skin
  sprite frame
  wall texture
  pic

*/

typedef enum
{
	it_skin,
	it_sprite,
	it_wall,
	it_pic,
	it_sky,
	it_temp
} imagetype_t;

typedef struct image_s
{
	char	name[MAX_QPATH];			// game path, including extension
	imagetype_t	type;
	int		width, height;			// source image
	int		upload_width, upload_height;	// after power of two and picmip
	int		upload_format;			// type of image in video memory
	int		registration_sequence;		// 0 = free
	struct msurface_s	*texturechain;		// for sort-by-texture world drawing
	int		texnum;				// gl texture binding
	float	sl, tl, sh, th;				// 0,0 - 1,1 unless part of the scrap
	qboolean	scrap;
	qboolean	has_alpha;

//	qboolean paletted;
} image_t;

#define	TEXNUM_LIGHTMAPS	1024
#define	TEXNUM_SCRAPS		1152
#define	TEXNUM_IMAGES		1153

#define		MAX_GLTEXTURES	1024

extern int 	gl_screenshotFlags;
extern char	*gl_screenshotName;
void GL_PerformScreenshot (void);

//===================================================================

typedef enum
{
	rserr_ok,

	rserr_invalid_fullscreen,
	rserr_invalid_mode,

	rserr_unknown
} rserr_t;

#include "gl_model.h"

void GL_SetDefaultState( void );
void GL_UpdateSwapInterval( void );

extern	float	gldepthmin, gldepthmax;

typedef struct
{
	float	x, y, z;
	float	s, t;
	float	r, g, b;
} glvert_t;


#define	MAX_LBM_HEIGHT		480

#define BACKFACE_EPSILON	0.01


//====================================================

extern	image_t		gltextures[MAX_GLTEXTURES];
extern	int			numgltextures;


extern	image_t		*r_notexture;
extern	image_t		*r_particletexture;
extern	entity_t	*currententity;
extern	model_t		*currentmodel;
extern	int			r_visframecount;
extern	int			r_framecount;
extern	cplane_t	frustum[4];
extern	int			c_brush_polys, c_alias_polys;


extern	int			gl_filter_min, gl_filter_max;

//
// view origin
//
extern	vec3_t	vup;
extern	vec3_t	vpn;
extern	vec3_t	vright;
extern	vec3_t	r_origin;

//
// screen size info
//
extern	refdef_t	r_newrefdef;
extern	int		r_viewcluster, r_viewcluster2, r_oldviewcluster, r_oldviewcluster2;

extern	cvar_t	*r_norefresh;
extern	cvar_t	*r_lefthand;
extern	cvar_t	*r_drawentities;
extern	cvar_t	*r_drawworld;

extern	cvar_t	*r_speeds;

extern	cvar_t	*r_fullbright;
extern	cvar_t	*r_novis;
extern	cvar_t	*gl_frustumCull;
extern	cvar_t	*r_lerpmodels;

extern	float	gl_lightlevel;

extern	cvar_t	*gl_vertex_arrays;

extern	cvar_t	*gl_particle_min_size;
extern	cvar_t	*gl_particle_max_size;
extern	cvar_t	*gl_particle_size;
extern	cvar_t	*gl_particle_att_a;
extern	cvar_t	*gl_particle_att_b;
extern	cvar_t	*gl_particle_att_c;

extern	cvar_t	*gl_nosubimage;
extern	cvar_t	*gl_bitdepth;
extern	cvar_t	*gl_mode;
extern	cvar_t	*gl_logFile;
extern	cvar_t	*r_lightmap;
extern	cvar_t	*gl_shadows;
extern	cvar_t	*gl_dynamic;
extern	cvar_t	*gl_dlightBacks;
extern  cvar_t  *gl_monolightmap;
extern	cvar_t	*gl_nobind;
extern	cvar_t	*gl_round_down;
extern	cvar_t	*gl_picmip;
extern	cvar_t	*gl_skymip;
extern	cvar_t	*gl_showtris;
extern	cvar_t	*gl_finish;
extern	cvar_t	*gl_ztrick;
extern	cvar_t	*gl_clear;
extern	cvar_t	*gl_cull;
extern	cvar_t	*gl_poly;
extern	cvar_t	*gl_texsort;
extern	cvar_t	*gl_polyblend;
extern	cvar_t	*gl_flashblend;
extern	cvar_t	*gl_modulate;
extern	cvar_t	*gl_playermip;
extern	cvar_t	*gl_drawbuffer;
extern	cvar_t	*gl_3dlabs_broken;
extern  cvar_t  *gl_driver;
extern	cvar_t	*gl_swapinterval;
extern	cvar_t	*gl_texturemode;
extern	cvar_t	*gl_texturealphamode;
extern	cvar_t	*gl_texturesolidmode;
extern  cvar_t  *gl_saturatelighting;
extern  cvar_t  *gl_lockpvs;
extern  cvar_t  *gl_zmax;
extern  cvar_t  *gl_winrefl;

extern	cvar_t	*r_fullscreen;

extern cvar_t	*r_gamma, *r_brightness, *r_contrast, *r_saturation;

extern	int	gl_solid_format;
extern	int	gl_alpha_format;
extern	int	gl_tex_solid_format;
extern	int	gl_tex_alpha_format;

extern	int	c_visible_lightmaps;
extern	int	c_visible_textures;
extern	int	c_visibleleafs;
extern	int	c_leafsinfrustum;

extern	float	r_world_matrix[16];

void R_TranslatePlayerSkin (int playernum);

void GL_Bind (int texnum);
void GL_MBind (int tmu, int texnum);
void GL_TexEnv( GLenum value );
void GL_EnableMultitexture( qboolean enable );
void GL_SelectTexture (int tmu);
void qglMTexCoord2f (int tmu, float s, float t);	// will choose ARB or SGIS function

void R_LightPoint (vec3_t p, vec3_t color);
void R_PushDlights (void);

//====================================================================

extern	model_t	*r_worldmodel;

extern	unsigned	d_8to24table[256];

extern	int		registration_sequence;


void V_AddBlend (float r, float g, float b, float a, float *v_blend);

int 	R_Init( void );
void	R_Shutdown( void );

void R_RenderView (refdef_t *fd);
void R_DrawAliasModel (entity_t *e);
void R_DrawBrushModel (entity_t *e);
void R_DrawSpriteModel (entity_t *e);
void R_DrawBeam( beam_t *b );
void R_DrawWorld (void);
void R_RenderDlights (void);
void R_DrawAlphaSurfaces (void);
void R_RenderBrushPoly (msurface_t *fa);
void R_InitParticleTexture (void);
void Draw_InitLocal (void);
void GL_SubdivideSurface (msurface_t *fa);
qboolean R_CullBox (vec3_t mins, vec3_t maxs);
void R_RotateForEntity (entity_t *e);
void R_MarkLeaves (void);

glpoly_t *WaterWarpPolyVerts (glpoly_t *p);
void EmitWaterPolys (msurface_t *fa);
void R_AddSkySurface (msurface_t *fa);
void R_ClearSkyBox (void);
void R_DrawSkyBox (void);
void R_MarkLights (dlight_t *light, int bit, mnode_t *node);

#if 0
short LittleShort (short l);
short BigShort (short l);
int	LittleLong (int l);
float LittleFloat (float f);

char	*va(char *format, ...);
// does a varargs printf into a temp buffer
#endif

void COM_StripExtension (char *in, char *out);

void	Draw_GetPicSize (int *w, int *h, char *name);
void	Draw_Pic (int x, int y, char *name);
void	Draw_PicColor (int x, int y, char *pic, int color);
void	Draw_StretchPic (int x, int y, int w, int h, char *name);
void	Draw_Char (int x, int y, int c);
void	Draw_CharColor (int x, int y, int num, int color);
void	Draw_TileClear (int x, int y, int w, int h, char *name);
void	Draw_Fill (int x, int y, int w, int h, int c);
void	Draw_Fill2 (int x, int y, int w, int h, float r, float g, float b, float a);
void	Draw_StretchRaw (int x, int y, int w, int h, int cols, int rows, byte *data);

void	R_BeginFrame( float camera_separation );
void	R_SwapBuffers( int );
void	R_SetPalette ( const unsigned char *palette);

int		Draw_GetPalette (void);

void GL_BufCorrectGamma (byte *pic, int size);
void GL_ResampleTexture (unsigned *in, int inwidth, int inheight, unsigned *out,  int outwidth, int outheight);

struct image_s *R_RegisterSkin (char *name);

//void LoadPCX (char *filename, byte **pic, byte **palette, int *width, int *height);
image_t *GL_LoadPic (char *name, byte *pic, int width, int height, imagetype_t type, int bits);
image_t	*GL_FindImage (char *name, imagetype_t type);
void	GL_TextureMode( char *string );
void	GL_ImageList_f (void);

void	GL_SetTexturePalette( unsigned palette[256] );

void	GL_InitImages (void);
void	GL_ShutdownImages (void);

void	GL_FreeUnusedImages (void);

void GL_TextureAlphaMode( char *string );
void GL_TextureSolidMode( char *string );

/*
** GL extension emulation functions
*/
void GL_DrawParticles (particle_t *particles, const unsigned colortable[768]);

/*
** GL config stuff
*/
#define GL_RENDERER_VOODOO		0x00000001
#define GL_RENDERER_VOODOO2   	0x00000002
#define GL_RENDERER_VOODOO_RUSH	0x00000004
#define GL_RENDERER_BANSHEE		0x00000008
#define	GL_RENDERER_3DFX		0x0000000F

#define GL_RENDERER_PCX1		0x00000010
#define GL_RENDERER_PCX2		0x00000020
#define GL_RENDERER_PMX			0x00000040
#define	GL_RENDERER_POWERVR		0x00000070

#define GL_RENDERER_PERMEDIA2	0x00000100
#define GL_RENDERER_GLINT_MX	0x00000200
#define GL_RENDERER_GLINT_TX	0x00000400
#define GL_RENDERER_3DLABS_MISC	0x00000800
#define	GL_RENDERER_3DLABS	0x00000F00

#define GL_RENDERER_REALIZM		0x00001000
#define GL_RENDERER_REALIZM2	0x00002000
#define	GL_RENDERER_INTERGRAPH	0x00003000

#define GL_RENDERER_3DPRO		0x00004000
#define GL_RENDERER_REAL3D		0x00008000
#define GL_RENDERER_RIVA128		0x00010000
#define GL_RENDERER_DYPIC		0x00020000

#define GL_RENDERER_V1000		0x00040000
#define GL_RENDERER_V2100		0x00080000
#define GL_RENDERER_V2200		0x00100000
#define	GL_RENDERER_RENDITION	0x001C0000

#define GL_RENDERER_O2          0x00100000
#define GL_RENDERER_IMPACT      0x00200000
#define GL_RENDERER_RE			0x00400000
#define GL_RENDERER_IR			0x00800000
#define	GL_RENDERER_SGI			0x00F00000

#define GL_RENDERER_MCD			0x01000000
#define GL_RENDERER_OTHER		0x80000000


typedef struct
{
	// (required for GLimp_... functions)
	char	rendererString[256];
	char	vendorString[256];
	char	versionString[256];
	char	*extensionsString;

	int		maxTextureSize;
	int		extensionMask;

	// multitexturing
//	int		maxActiveTextures;		// == 1 if no multitexturing

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

	// tables
	unsigned tbl_8to32[256];		// palette->RGBA
	//--------------------------
	int         renderer;
	qboolean	allow_cds;
} glconfig_t;

// macro for checking extension support
#define GL_SUPPORT(ext)		(gl_config.extensionMask & (ext))


typedef struct
{
	int		currentBinds[2];
	int		currentTmu;

	float inverse_intensity;

	unsigned char *d_16to8table;

	int lightmap_textures;

	float camera_separation;
	qboolean stereo_enabled;

	int texture_format_solid;
	int texture_format_alpha;
	int texture_format_alpha1;
} glstate_t;

typedef struct
{
	// statistics
	int		numBinds, numUploads;
	// timings
	int		beginFrame;
	int		beginWorld, beginBk, endBk;
	int		endFrame;
} gl_speeds_t;

extern glconfig_t  gl_config;
extern glstate_t   gl_state;
extern gl_speeds_t gl_speeds;

/*
====================================================================

IMPORTED FUNCTIONS

====================================================================
*/

extern	refImport_t	ri;


/*
====================================================================

IMPLEMENTATION SPECIFIC FUNCTIONS

====================================================================
*/

void		GLimp_BeginFrame( float camera_separation );
void		GLimp_EndFrame( void );
int 		GLimp_Init( void );
void		GLimp_Shutdown(void);
int     	GLimp_SetMode (int *pwidth, int *pheight, int mode, bool fullscreen);
void		GLimp_AppActivate (qboolean active);

bool		GLimp_HasGamma (void);
void		GLimp_SetGamma (float gamma, float intens);


void	R_DrawTexts (void);
void	DrawTextPos (int x, int y, char *text, unsigned rgba);
void	DrawTextLeft (char *text, unsigned rgba);
void	DrawTextRight (char *text, unsigned rgba);

extern qboolean con_only;
