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
void	QGL_LogMessage (const char *text);
#define LOG_STRING(str)		if (gl_logFile->integer) QGL_LogMessage (str);

#ifndef APIENTRY
#  define APIENTRY
#endif


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


// forwards
typedef struct viewPortal_s	viewPortal_t;
typedef struct refEntity_s	refEntity_t;
typedef struct image_s		image_t;
typedef struct model_s		model_t;
typedef struct surfaceCommon_s surfaceCommon_t;
typedef struct dynamicLightmap_s dynamicLightmap_t;


#include "qgl_decl.h"
#include "gl_interface.h"


/*------------- gl_light.c ------------------*/


void GL_ShowLights (void);

void GL_LightForEntity (refEntity_t *ent);
void GL_ApplyEntitySpherelights (color_t *dst);


/*------------- gl_main.c -------------------*/


//?? clean this structure: most fields used from viewPortal_t; or -- eliminate at all
/* What we can do: (??)
 * - there is refdef_t from client/ref.h
 * - we are copied most fields to viewPortal_t
 * - but this structure is not for cross-frame work, its contents updated every frame COMPLETELY
 * - so, we need some structure with a static, cross-frame fields (now: glrefdef_t, rudimental structure)
 * 1) make client's refdef static, and add a link from portal to it
 * 2) eliminate refdef fields from portal
 * 3) client should establish mare control under refdef (detect viewcluster changes, areamask changes etc)
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
	qboolean areaMaskChanged;
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


void	DrawTextPos (int x, int y, char *text, float r, float g, float b);
void	DrawTextLeft (char *text, float r, float g, float b);
void	DrawTextRight (char *text, float r, float g, float b);
void	DrawText3D (vec3_t pos, char *text, float r, float g, float b);


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

extern cvar_t	*gl_picmip, *gl_textureBits, *gl_roundImagesDown, *gl_texturemode;

extern cvar_t	*r_gamma, *r_brightness, *r_contrast, *r_saturation;
extern cvar_t	*r_intensity;		//?? remove (or "r_brightness")

extern cvar_t	*gl_overBrightBits;

extern cvar_t	*gl_fastsky;
extern cvar_t	*gl_fog;

extern cvar_t	*gl_flares;
extern cvar_t	*gl_dynamic;
extern cvar_t	*gl_dlightBacks;
extern cvar_t	*gl_vertexLight;
extern cvar_t	*gl_nogrid;
extern cvar_t	*gl_showgrid;

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
extern cvar_t	*gl_oCull;
extern cvar_t	*gl_facePlaneCull;	//?? gl_backfaceCull ?
extern cvar_t	*gl_sortAlpha;
extern cvar_t	*r_speeds;
extern cvar_t	*r_fullbright;
extern cvar_t	*r_lightmap;
extern cvar_t	*gl_showsky;
extern cvar_t	*r_drawworld;
extern cvar_t	*r_drawentities;

extern cvar_t	*gl_showbboxes, *gl_showtris, *gl_shownormals, *gl_labels;
extern cvar_t	*gl_lightLines;
extern cvar_t	*gl_showLights;
extern cvar_t	*gl_singleShader;

extern cvar_t	*gl_clear;
extern cvar_t	*gl_finish;
extern cvar_t	*gl_swapinterval;
extern cvar_t	*gl_drawbuffer;


#endif
