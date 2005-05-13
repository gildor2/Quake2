#ifndef __GL_LOCAL_INCLUDED__
#define __GL_LOCAL_INCLUDED__


#include <stdio.h>

#ifdef _WIN32
#if 0
	// include "windows.h" headers
#	define WIN32_LEAN_AND_MEAN		// exclude rarely-used services from windown headers
#	include <windows.h>				// need this include, because have wgl and GDI functions in gl.h
#else // 0
	// try to preform some windows defines to avoid "windows.h" including
#	ifndef APIENTRY
#		define APIENTRY __stdcall
#	endif
#	ifndef WINGDIAPI
#		define WINGDIAPI
		typedef unsigned		HDC;
		typedef unsigned		HGLRC;
		typedef const char *	LPCSTR;
		typedef int				BOOL;
		typedef unsigned char	BYTE;
		typedef unsigned short	WORD;
		typedef unsigned int	UINT;
		typedef int (APIENTRY *PROC)();
		typedef void PIXELFORMATDESCRIPTOR;		// structure
		typedef PIXELFORMATDESCRIPTOR * LPPIXELFORMATDESCRIPTOR;
#	endif // WINGDIAPI
#	ifndef CONST
#		define CONST const
#	endif
#endif // 0
#endif

#include <GL/gl.h>
#ifndef GL_GLEXT_VERSION
#	include "glext.h"				// include glext.h only when its contents not in gl.h (warning: GL_GLEXT_VERSION may be too low)
#endif

// Obsolete (missing in standard headers), but still supported extensions
/* SGIS_multitexture */
#define GL_SELECTED_TEXTURE_SGIS	0x835C
#define GL_MAX_TEXTURES_SGIS		0x835D
#define GL_TEXTURE0_SGIS			0x835E
#define GL_TEXTURE1_SGIS			0x835F
#define GL_TEXTURE2_SGIS			0x8360
#define GL_TEXTURE3_SGIS			0x8361


#include "qcommon.h"
#include "../client/ref.h"


namespace OpenGLDrv {

#include "qgl_decl.h"

bool	QGL_Init (const char *libName);
void	QGL_Shutdown (void);

void	QGL_InitExtensions (void);
void	QGL_PrintExtensionsString (const char *label, const char *str, const char *mask = NULL);

void	QGL_SwapBuffers (void);

void	QGL_EnableLogging (bool enable);
void	QGL_LogMessage (const char *text);
#define LOG_STRING(str)		if (gl_logFile->integer) QGL_LogMessage (str);


//??
extern unsigned vid_width, vid_height;

// forwards
class image_t;
class shader_t;
class model_t;
class surfaceCommon_t;
class surfacePlanar_t;
struct dynamicLightmap_t;
struct viewPortal_t;
struct refEntity_t;


#include "gl_interface.h"


/*----------------- gl_main.cpp -------------------*/

extern bool renderingEnabled;
void GL_EnableRendering (bool enable);

extern const unsigned colorTable[];		// C_XXX->rgba; used for DrawChar() and DrawPic() only


//?? clean this structure: most fields used from viewPortal_t; or -- eliminate at all
/* What we can do: (??)
 * - there is refdef_t from client/ref.h
 * - we are copied most fields to viewPortal_t
 * - but this structure is not for cross-frame work, its contents updated every frame COMPLETELY
 * - so, we need some structure (set of vars) with a static, cross-frame fields
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


//!! need to perform clearing this struc with "memset()" (clear all fields at once)
typedef struct
{
	// geometry complexity
	int		leafs, visLeafs, frustLeafs;	//?? frustLeafs -> ~cullLeafs
	int		surfs, cullSurfs;
	int		tris, trisMT;		// number of tris, which will be drawn without mtex and with mtex (same if no multitexture)
	int		tris2D;				// number of triangles for HUD/console
	int		ents, cullEnts, ocullEnts;
	int		parts, cullParts;	// particles
	int		dlightSurfs, dlightVerts;
	int		flares, testFlares, cullFlares;
	// OpenGL statistics
	int		numBinds, numUploads, numIterators;
	// pefromance measuring (appCycles())
	unsigned beginFrame;		// front-end
	unsigned beginSort;			// sorting
	unsigned begin3D;			// back-end (3D)
	unsigned begin2D;			// back-end (2D)
	unsigned endFrame;
} drawSpeeds_t;


extern drawSpeeds_t gl_speeds;


/*----------- gl_text.cpp -------------------*/

void	InitTexts ();
void	ClearTexts ();
void	FlushTexts ();

void	DrawChar (int x, int y, int c, int color);
void	DrawConChar (int x, int y, int c, int color);

void	DrawTextLeft (const char *text, unsigned rgba);
void	DrawTextRight (const char *text, unsigned rgba);
void	DrawText3D (vec3_t pos, const char *text, unsigned rgba);


/*------ Platform-specific functions --------*/

bool	GLimp_SetMode (unsigned *pwidth, unsigned *pheight, int mode, bool fullscreen);
void	GLimp_Shutdown (bool complete);
void	AppActivate (bool active);

bool	GLimp_HasGamma (void);
void	GLimp_SetGamma (float gamma);


/*----------------- Cvars -------------------*/

extern cvar_t	*gl_texMipBias, *gl_skinMipBias;
extern cvar_t	*gl_texturemode;

extern cvar_t	*gl_overbright;

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
extern cvar_t	*gl_showSky;
extern cvar_t	*r_drawworld, *r_drawentities;
extern cvar_t	*gl_sortAlpha;
extern cvar_t	*gl_finish;

extern cvar_t	*gl_labels;
extern cvar_t	*gl_lightLines, *gl_showLights;
extern cvar_t	*gl_singleShader;
extern cvar_t	*gl_showFillRate;

extern cvar_t	*gl_znear;
extern cvar_t	*gl_swapinterval;


} // namespace

#endif
