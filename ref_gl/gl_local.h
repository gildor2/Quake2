#ifndef __GL_LOCAL_INCLUDED__
#define __GL_LOCAL_INCLUDED__


#include <stdio.h>

#ifdef _WIN32
// need this include, because have wgl and GDI functions here
#	include <windows.h>
#endif

#include <GL/gl.h>
#include "glext.h"

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


bool	QGL_Init (const char *libName);
void	QGL_InitExtensions (void);
void	QGL_PrintExtensionsString (const char *label, const char *str);
void	QGL_Shutdown (void);
// logging
void	QGL_EnableLogging (qboolean enable);
void	QGL_LogMessage (const char *text);
#define LOG_STRING(str)		if (gl_logFile->integer) QGL_LogMessage (str);


#ifdef REF_HARD_LINKED
#	define GetRefAPI			GL_GetRefAPI
#else
//?? make common declaration
/*extern "C"*/ DLL_EXPORT refExport_t GetRefAPI (const refImport_t *);
#endif


//?? find a way to remove this struc (use vid_width and vid_height or vid_size[2] ??)
typedef struct
{
	unsigned	width, height;		// coordinates from main game
} viddef_t;

extern viddef_t vid;


// forwards
typedef struct viewPortal_s	viewPortal_t;
typedef struct refEntity_s	refEntity_t;
typedef struct image_s		image_t;
typedef struct model_s		model_t;
typedef struct surfaceCommon_s surfaceCommon_t;
typedef struct surfacePlanar_s surfacePlanar_t;
typedef struct dynamicLightmap_s dynamicLightmap_t;


#include "gl_interface.h"


/*------------- gl_main.c -------------------*/

bool gl_renderingEnabled;
void GL_EnableRendering (bool enable);


//?? clean this structure: most fields used from viewPortal_t; or -- eliminate at all
/* What we can do: (??)
 * - there is refdef_t from client/ref.h
 * - we are copied most fields to viewPortal_t
 * - but this structure is not for cross-frame work, its contents updated every frame COMPLETELY
 * - so, we need some structure with a static, cross-frame fields (now: glrefdef_t, rudimental structure)
 * 1) make client's refdef static, and add a link from portal to it
 * 2) eliminate refdef fields from portal
 * 3) client should establish more control under refdef (detect viewcluster changes, areamask changes etc)
 * 4) portals/mirrors: we needs a way to keep few visibility marks for leafs ? (useless -- can't find a point
 *    of view source in common case (all reflections), when it will come from "inside wall" ?)
 *    -- use occlusion culling for mirrors ? (OR in most cases of Q3 portals (not mirrors), can use PVS - portals
 *    visible only within a near distance from surface)
 * 5) get visinfo from client ? (remove visinfo from renderer; call CM_ClusterPVS() when needed ?)
 * 6) PVS: can perform non-alpha-water-vis-bug avoiding in client (send combined vis)
 */
typedef struct
{
	int		viewCluster;		//?? place to portal
	// map areas
	byte	areaMask[MAX_MAP_AREAS/8];
	bool	areaMaskChanged;
} glrefdef_t;


//!! need to perform clearing this struc with "memset()" (clear all fields in one place)
typedef struct
{
	int		numFrames;
	// geometry complexity
	int		leafs, visLeafs, frustLeafs;	//?? frustLeafs -> ~cullLeafs
	int		surfs, cullSurfs;
	int		tris, trisMT;		// number of tris, which will be drawn without mtex and with mtex (same if no multitexture)
	int		tris2D;				// number of triangles for HUD/console
	int		ents, cullEnts, cullEntsBox, cullEnts2, ocullEnts;
	int		parts, cullParts;	// particles
	int		dlightSurfs, dlightVerts;
	int		flares, testFlares, cullFlares;
	// OpenGL statistics
	int		numBinds, numUploads, numIterators;
	// pefromance measuring
	int		beginFrame;			// front-end
	int		beginSort;			// sorting
	int		begin3D;			// back-end (3D)
	int		begin2D;			// back-end (2D)
	int		endFrame;
} drawSpeeds_t;


extern glrefdef_t  gl_refdef;	//!! used from gl_frontend.c :: MarkLeaves() only (filled from gl_main.c)
extern drawSpeeds_t gl_speeds;


void	DrawTextPos (int x, int y, char *text, unsigned rgba);
void	DrawTextLeft (char *text, unsigned rgba);
void	DrawTextRight (char *text, unsigned rgba);
void	DrawText3D (vec3_t pos, char *text, unsigned rgba);


/*----------- Imported functions ------------*/

#ifdef DYNAMIC_REF
extern	refImport_t	ri;
#endif


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
int     GLimp_SetMode (int *pwidth, int *pheight, int mode, bool fullscreen);
void	GLimp_AppActivate (qboolean active);

bool	GLimp_HasGamma (void);
void	GLimp_SetGamma (float gamma);


/*----------------- Cvars -------------------*/

extern cvar_t	*gl_texMipBias, *gl_skinMipBias;
extern cvar_t	*gl_texturemode;

extern cvar_t	*r_gamma, *r_brightness, *r_contrast, *r_saturation;

extern cvar_t	*gl_overbright;

extern cvar_t	*gl_fastSky;
extern cvar_t	*gl_fog;

extern cvar_t	*gl_flares;
extern cvar_t	*gl_dynamic;
extern cvar_t	*gl_dlightBacks;
extern cvar_t	*gl_vertexLight;
extern cvar_t	*gl_noGrid;
extern cvar_t	*gl_showGrid;

extern cvar_t	*gl_driver;
extern cvar_t	*gl_bitdepth;

extern cvar_t	*gl_hand;

extern cvar_t	*gl_nobind;
extern cvar_t	*gl_logFile;
extern cvar_t	*r_novis;			//?? ~gl_pvsCull ?
extern cvar_t	*gl_frustumCull;
extern cvar_t	*gl_oCull;
extern cvar_t	*gl_backfaceCull;
extern cvar_t	*r_speeds;
extern cvar_t	*r_fullbright, *r_lightmap;
extern cvar_t	*gl_showSky;
extern cvar_t	*r_drawworld, *r_drawentities;
extern cvar_t	*gl_sortAlpha;
extern cvar_t	*gl_finish;

extern cvar_t	*gl_labels;
extern cvar_t	*gl_lightLines, *gl_showLights;
extern cvar_t	*gl_singleShader;

extern cvar_t	*gl_znear;
extern cvar_t	*gl_swapinterval;
extern cvar_t	*gl_drawbuffer;


#endif
