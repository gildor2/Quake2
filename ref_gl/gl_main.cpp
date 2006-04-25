#include "OpenGLDrv.h"
#include "gl_frontend.h"
#include "gl_backend.h"
#include "gl_math.h"
#include "gl_image.h"
#include "gl_buffers.h"

/*!! TODO:
 *  - UT BeginFrame / EndFrame (SwapBuffers) -- Lock / Unlock
 */


namespace OpenGLDrv {

#if STATS
FDrawStats	gl_stats;
FLoadStats	gl_ldStats;
cvar_t		*r_stats;
#endif

#if !STATIC_BUILD
refImport_t	ri;
#endif

bool renderingEnabled;

unsigned vid_width, vid_height;	//?? remove; move to gl_config

#define I 255
#define o 51
const unsigned colorTable[8] = {
	RGB255(0, 0, 0),
	RGB255(I, o, o),
	RGB255(o, I, o),
	RGB255(I, I, o),
	RGB255(o, o, I),
	RGB255(I, o, I),
	RGB255(o, I, I),
	RGB255(I, I, I)
};
#undef I
#undef o

//------------- Cvars -----------------

// initialization/GLimp
cvar_t	*gl_mode, *gl_bitdepth;
cvar_t	*gl_driver;

// image-specific
cvar_t	*gl_nobind;

cvar_t	*r_ignorehwgamma;	// rename ??
cvar_t	*gl_overbright;

cvar_t	*gl_texMipBias, *gl_skinMipBias;
cvar_t	*gl_texturemode;

// rendering speed/quality settings
static cvar_t *gl_fastSky;	// do not draw sky
cvar_t	*gl_fog;			//??

cvar_t	*gl_flares;
cvar_t	*gl_dynamic;		// enable dynamic lightmaps for Q2/HL maps + dlights
cvar_t	*gl_dlightBacks;	// when disabled, do not draw dlights on backfaces
cvar_t	*gl_vertexLight;	// use glColor() against lightmaps
cvar_t	*gl_noGrid;
cvar_t	*gl_showGrid;

// renderer settings
cvar_t	*gl_znear;
cvar_t	*gl_swapinterval;
cvar_t	*gl_maxTextureUnits;

// debugging
#if !NO_GL_LOG
cvar_t	*gl_logFile;
#endif
cvar_t	*r_novis, *gl_frustumCull, *gl_oCull, *gl_backfaceCull;
cvar_t	*gl_showSky;
cvar_t	*r_drawworld, *r_drawentities;
cvar_t	*gl_sortAlpha;

cvar_t	*gl_labels;
cvar_t	*gl_lightLines, *gl_showLights;		//?? LIGHT_DEBUG
cvar_t	*gl_singleShader;
cvar_t	*gl_showFillRate;


//?? move to gl_interface.cpp ??
static void cGfxinfo (bool usage, int argc, char **argv)
{
	static const char *boolNames[] = {"no", "yes"};
	static const char *overbrNames[2][2] = {"disabled", "forced", "unneeded", "required"};

	if (argc > 2 || usage)
	{
		appPrintf ("Usage: gfxinfo [extension_mask]");
		return;
	}
	const char *mask = (argc == 2) ? argv[1] : NULL;

	appPrintf (S_RED"---------- OpenGL info ----------\n");
	appPrintf (S_RED"Vendor:  "S_WHITE" %s\n", glGetString (GL_VENDOR));
	appPrintf (S_RED"Renderer:"S_WHITE" %s\n", glGetString (GL_RENDERER));
	appPrintf (S_RED"Version: "S_WHITE" %s\n", glGetString (GL_VERSION));
	QGL_PrintExtensionsString ("Base", gl_config.extensions, mask);
	if (gl_config.extensions2)
		QGL_PrintExtensionsString ("Platform", gl_config.extensions2, mask);
	appPrintf (S_RED"---------------------------------\n");
	// multitexturing
	appPrintf ("Multitexturing: ");
	if (GL_SUPPORT(QGL_ARB_MULTITEXTURE|QGL_SGIS_MULTITEXTURE))
	{
		TString<256> Name;
		Name = GL_SUPPORT(QGL_ARB_MULTITEXTURE) ? "ARB" : "SGIS";
		if (GL_SUPPORT(QGL_ARB_TEXTURE_ENV_ADD))
			Name += " +Add";
		if (GL_SUPPORT(QGL_ARB_TEXTURE_ENV_COMBINE|QGL_EXT_TEXTURE_ENV_COMBINE))
			Name += " +Combine";
		if (GL_SUPPORT(QGL_NV_TEXTURE_ENV_COMBINE4))
			Name += " +NV";
		if (GL_SUPPORT(QGL_ATI_TEXTURE_ENV_COMBINE3))
			Name += " +ATI";

		appPrintf ("yes, %s, %d texture units\n", *Name, gl_config.maxActiveTextures);
	}
	else
		appPrintf ("no\n");
	// texturing
	appPrintf ("Max texture size: %d\n", gl_config.maxTextureSize);
	appPrintf ("Texture rectangle: ");
	if (GL_SUPPORT(QGL_NV_TEXTURE_RECTANGLE))
		appPrintf ("max size is %d\n", gl_config.maxRectTextureSize);
	else
		appPrintf ("unsupported\n");
	// gamma info
	appPrintf ("Lighting: %s\n", gl_config.vertexLight ? "vertex" : "lightmap");
	appPrintf ("Gamma: ");
	if (gl_config.deviceSupportsGamma)
		appPrintf ("hardware, overbright: %s (%s), lightmap overbright: %s\n",
			//?? NOTE: here used gl_overbright->integer, which can be modified after vid_restart
			boolNames[gl_config.overbright], overbrNames[gl_overbright->integer == 2][gl_config.overbright],
			boolNames[!gl_config.doubleModulateLM]);
	else
		appPrintf ("software\n");
}


static void RegisterCvars ()
{
/* NOTE: some cvars have "CVAR_NOUPDATE" flag -- for correct video-config-menu work (to detect changes)
 * Every cvar here can have this flag (?) (but only few of them REQUIRE it)
 * If cvar have "CVAR_UPDATE" flag, its value will be re-analyzed (settings, which are (potentially) inactive by defaults)
 */
CVAR_BEGIN(vars)
	CVAR_VAR(gl_texturemode, bilinear, CVAR_ARCHIVE),
	CVAR_VAR(gl_texMipBias, -0.5, CVAR_ARCHIVE),
	CVAR_VAR(gl_skinMipBias, -1, CVAR_ARCHIVE),
	CVAR_VAR(r_ignorehwgamma, 0, CVAR_ARCHIVE),
	CVAR_VAR(gl_overbright, 2, CVAR_ARCHIVE|CVAR_NOUPDATE),
	CVAR_VAR(gl_bitdepth, 0, CVAR_ARCHIVE|CVAR_NOUPDATE),			//?? gl_colorBits
	//!! add gl_depthBits

	CVAR_VAR(gl_fastSky, 0, CVAR_ARCHIVE),
	CVAR_VAR(gl_fog, 0, 0),	//?? ARCHIVE? CHEAT?

	CVAR_VAR(gl_flares, 1, CVAR_ARCHIVE),
	CVAR_VAR(gl_dynamic, 1, 0),
	CVAR_VAR(gl_dlightBacks, 1, 0),
	CVAR_VAR(gl_vertexLight, 0, CVAR_ARCHIVE|CVAR_NOUPDATE),
	CVAR_VAR(gl_noGrid, 0, 0),
	CVAR_VAR(gl_showGrid, 0, CVAR_CHEAT),

	CVAR_VAR(gl_znear, 4, 0),
	CVAR_VAR(gl_swapinterval, 0, CVAR_ARCHIVE|CVAR_UPDATE),

#if _WIN32
	CVAR_VAR(gl_driver, opengl32, CVAR_ARCHIVE),
#else
	CVAR_VAR(gl_driver, libGL.so.1, CVAR_ARCHIVE),
#endif
	CVAR_VAR(gl_mode, 3, CVAR_ARCHIVE),

	CVAR_VAR(gl_maxTextureUnits, 0, CVAR_ARCHIVE),

	CVAR_VAR(gl_nobind, 0, 0),
#if !NO_GL_LOG
	CVAR_VAR(gl_logFile, 0, 0),
#endif
	CVAR_VAR(r_novis, 0, 0),
	CVAR_VAR(gl_frustumCull, 1, 0),
	CVAR_VAR(gl_oCull, 1, 0),
	CVAR_VAR(gl_backfaceCull, 1, 0),
#if STATS
	CVAR_VAR(r_stats, 0, 0),
#endif
	CVAR_VAR(gl_showSky, 0, 0),
	CVAR_VAR(r_drawworld, 1, CVAR_CHEAT),
	CVAR_VAR(r_drawentities, 1, 0),
	CVAR_VAR(gl_sortAlpha, 0, 0),
	CVAR_VAR(gl_labels, 0, CVAR_CHEAT),
	CVAR_VAR(gl_lightLines, 0, CVAR_CHEAT),
	CVAR_VAR(gl_showLights, 0, 0),
	CVAR_VAR(gl_singleShader, 0, CVAR_CHEAT),
	CVAR_VAR(gl_showFillRate, 0, CVAR_CHEAT)
CVAR_END

	Cvar_GetVars (ARRAY_ARG(vars));

	RegisterCommand ("gfxinfo", cGfxinfo);
}


static bool SetMode ()
{
	r_fullscreen->modified = false;
	gl_mode->modified = false;

	if (GLimp_SetMode (&vid_width, &vid_height, gl_mode->integer, r_fullscreen->integer != 0))
	{
		gl_config.prevMode = gl_mode->integer;
		gl_config.prevBPP  = gl_bitdepth->integer;
		gl_config.prevFS   = gl_config.fullscreen;
		return true;
	}

	appWPrintf ("SetMode() - invalid mode\n");

	Cvar_SetInteger ("gl_mode", gl_config.prevMode);
	gl_mode->modified = false;
	Cvar_SetInteger ("gl_bitdepth", gl_config.prevBPP);

	// try setting it back to something safe
	if (!GLimp_SetMode (&vid_width, &vid_height, gl_config.prevMode, gl_config.prevFS))
	{
		appWPrintf ("SetMode() - could not revert to safe mode\n");	//?? "to previous mode" ? (but, here will be mode "3")
		return false;
	}
	// initialize OS-specific parts of OpenGL
	return true;
}


bool Init ()
{
	guard(OpenGLDrv::Init);

	appPrintf ("\n--- Initializing OpenGL renderer ---\n");

	renderingEnabled = true;

	memset (&gl_config, 0, sizeof(gl_config));

	RegisterCvars ();

	// initialize our QGL dynamic bindings
	if (!QGL_Init (gl_driver->string))
	{
		if (!gl_driver->IsDefault())
		{
			appWPrintf ("Cannot load %s - trying default driver (%s) ... ", gl_driver->string, gl_driver->resetString);
			if (QGL_Init (gl_driver->resetString))
			{
				appPrintf ("OK\n");
				gl_driver->Reset();
			}
			else
			{
				appPrintf (S_RED"failed\n");
				QGL_Shutdown ();
				return false;
			}
		}
		else
		{
			appWPrintf ("Cannot load %s\n", gl_driver->string);
			QGL_Shutdown ();
			return false;
		}
	}

	// set our "safe" mode
	gl_config.prevMode = 3;				//??

	/*----- create the window and set up the context -----*/
	if (!SetMode ())
	{
		GLimp_Shutdown (true);
		QGL_Shutdown ();
		return false;
	}

	/*------------------- Check platform -----------------------*/
	//?? move to gl_interface.cpp ??
	//!! add cvar to disable videocard detection (for bugtest)
	const char *vendor = (const char*)glGetString (GL_VENDOR);
	const char *render = (const char*)glGetString (GL_RENDERER);
	if (!strnicmp (vendor, "NVIDIA ", 7))
	{
		Com_DPrintf ("NV");
		gl_config.platformId = HW_NV;
		if (!strnicmp (render, "GeForce2 MX", 11))
		{
			Com_DPrintf ("/GF2MX");
			gl_config.platformId |= HW_GF2MX;
		}
	}
	else if (!strnicmp (vendor, "ATI ", 4))
	{
		Com_DPrintf ("ATI");
		gl_config.platformId = HW_ATI;
	}
	else
		Com_DPrintf ("GENERIC");
	Com_DPrintf (" video detected\n");

	/*------------------ Check extensions ----------------------*/
	//?? move this part to gl_interface.cpp ??
	QGL_InitExtensions ();

	if (GL_SUPPORT(QGL_ARB_MULTITEXTURE))
		glGetIntegerv (GL_MAX_TEXTURE_UNITS_ARB, &gl_config.maxActiveTextures);
	else if (GL_SUPPORT(QGL_SGIS_MULTITEXTURE))
		glGetIntegerv (GL_MAX_TEXTURES_SGIS, &gl_config.maxActiveTextures);
	else
		gl_config.maxActiveTextures = 1;
	if (!gl_config.maxActiveTextures)
	{
		appWPrintf ("Error in multitexture support\n");
		gl_config.maxActiveTextures = 1;
	}
	if (gl_maxTextureUnits->integer > 0 && gl_config.maxActiveTextures > gl_maxTextureUnits->integer)
	{
		appPrintf (S_CYAN"...multitexture limited by %d units\n", gl_maxTextureUnits->integer);
		gl_config.maxActiveTextures = gl_maxTextureUnits->integer;
	}

	gl_config.doubleModulateLM = true;			// no multitexture or env_combine
	if (!GL_SUPPORT(QGL_EXT_TEXTURE_ENV_COMBINE|QGL_ARB_TEXTURE_ENV_COMBINE|QGL_NV_TEXTURE_ENV_COMBINE4))
		gl_config.doubleModulateLM = false;
	if (!GL_SUPPORT(QGL_SGIS_MULTITEXTURE|QGL_ARB_MULTITEXTURE))	// no multitexturing - can do normal blending
		gl_config.doubleModulateLM = false;

	if (GL_SUPPORT(QGL_ARB_TEXTURE_COMPRESSION))
	{
		gl_config.formatSolid  = GL_COMPRESSED_RGB_ARB;
		gl_config.formatAlpha  = GL_COMPRESSED_RGBA_ARB;
		gl_config.formatAlpha1 = GL_COMPRESSED_RGBA_ARB;
//		glHint (GL_TEXTURE_COMPRESSION_HINT_ARB, GL_NICEST);
	}
	else if (GL_SUPPORT(QGL_S3_S3TC))
	{
		gl_config.formatSolid  = GL_RGB4_S3TC;
		gl_config.formatAlpha  = GL_RGBA4_S3TC;
		gl_config.formatAlpha1 = GL_RGBA_S3TC;
	}
	else if (GL_SUPPORT(QGL_EXT_TEXTURE_COMPRESSION_S3TC))
	{
		gl_config.formatSolid  = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
		gl_config.formatAlpha  = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;	// DXT5 - compressed alpha; DXT3 - uncompressed alpha
		gl_config.formatAlpha1 = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;	// RGBA DXT1 - 1bit alpha (bugs with NV), DXT5 - 8bit alpha
	}

	if (GL_SUPPORT(QGL_NV_TEXTURE_RECTANGLE))
		glGetIntegerv (GL_MAX_RECTANGLE_TEXTURE_SIZE_NV, &gl_config.maxRectTextureSize);

	glGetIntegerv (GL_MAX_TEXTURE_SIZE, &gl_config.maxTextureSize);

	if (gl_dynamic->integer != 2)
		gl_config.multiPassLM = true;

	// set screen to particular color while initializing
	glClearColor (0.1, 0, 0, 1);
	glClear (GL_COLOR_BUFFER_BIT);
	QGL_SwapBuffers ();

	GL_SetDefaultState ();

	TRY {							// needs GUARD() in a case of error in some of following functions
		InitFuncTables ();
		InitTexts ();
		InitImages ();
		InitShaders ();
		InitModels ();
		BK_Init ();
		CreateBuffers ();
	} CATCH {
		GLimp_Shutdown (true);
		QGL_Shutdown ();
		THROW_AGAIN;	//?? or return "false" ?
	} END_CATCH

	forceVisMap = true;				// force update visinfo for map

	return true;

	unguard;
}


void Shutdown (bool complete)
{
	guard(OpenGLDrv::Shutdown);

	UnregisterCommand ("gfxinfo");

	FreeBuffers ();
	BK_Shutdown ();
	ShutdownModels ();
	ShutdownShaders ();
	ShutdownImages ();

	GL_SetMultitexture (0);

	// shut down OS-specific OpenGL stuff like contexts, etc.
	GLimp_Shutdown (complete);

	// shutdown our QGL subsystem
	QGL_Shutdown ();

	unguard;
}


void GL_EnableRendering (bool enable)
{
	if (renderingEnabled == enable) return;
	renderingEnabled = enable;
	if (enable)
	{
		LoadDelayedImages ();
	}
}


/*--------------------- Frame ---------------------------*/

//?? move to frontend?

static double defaultTime;

void BeginFrame (double time)
{
	defaultTime = vp.time = time;

#if !NO_GL_LOG
	if (gl_logFile->modified)
	{
		QGL_EnableLogging (gl_logFile->integer != 0);
		gl_logFile->modified = false;
	}
	else if (gl_logFile->integer == 2)
	{
		QGL_EnableLogging (false);
		Cvar_SetInteger ("gl_logFile", 0);
	}
#endif

	LOG_STRING ("\n---------- Begin Frame ----------\n\n");

	gl_state.have3d = gl_state.haveFullScreen3d = false;
	gl_state.maxUsedShaderIndex = -1;
	//?? useFastSky: when gl_fastSky!=0 - should clear screen only when at least one of sky surfaces visible
	//?? (perform glClear() in DrawSky() ?)
	gl_state.useFastSky = gl_fastSky->integer || (r_fullbright->integer && r_lightmap->integer) || gl_showFillRate->integer;

	if (gl_texturemode->modified)
	{
		GL_TextureMode (gl_texturemode->string);
		gl_texturemode->modified = false;
	}

	/*----- update hardware gamma (if present) ------*/
	if ((r_gamma->modified || r_contrast->modified || r_brightness->modified) && gl_config.deviceSupportsGamma)
	{
		SetupGamma ();
		r_gamma->modified      = false;
		r_contrast->modified   = false;
		r_brightness->modified = false;
	}

	/*------ change modes if necessary ---------*/
	if (gl_mode->modified || r_fullscreen->modified)
		Vid_Restart ();

	BK_BeginFrame ();
}


void EndFrame ()
{
	if (!gl_state.have3d) ClearTexts ();
	FlushTexts ();

	BK_EndFrame ();
}


/*----------------- Rendering ---------------------*/

static void SetWorldModelview ()
{
	float	matrix[4][4];
	int		i, j, k;
	static const float baseMatrix[4][4] = {	// axias {0 0 -1} {-1 0 0} {0 1 0}
		{  0,  0, -1,  0},
		{ -1,  0,  0,  0},
		{  0,  1,  0,  0},
		{  0,  0,  0,  1}
	};

	// all fields should be zeroed before call this function !
//	vp.modelvieworg = vp.view.origin;			// portals ??
//	vp.modelaxis[0][0] = vp.modelaxis[1][1] = vp.modelaxis[2][2] = 1;	// orthogonal axis ( portals ??)
	/* Matrix contents:
	 *  a00   a01   a02    -x
	 *  a10   a11   a12    -y
	 *  a20   a21   a22    -z
	 *    0     0     0     1
	 * where: x = dot(a0,org); y = dot(a1,org); z = dot(a2,org)
	 */
	memset (matrix, 0, sizeof(matrix));
	for (i = 0; i < 3; i++)
		for (j = 0; j < 3; j++)
			matrix[i][j] = vp.view.axis[j][i];
	matrix[3][0] = - dot (vp.view.origin, vp.view.axis[0]);
	matrix[3][1] = - dot (vp.view.origin, vp.view.axis[1]);
	matrix[3][2] = - dot (vp.view.origin, vp.view.axis[2]);
	matrix[3][3] = 1;
	// vp.modelMatrix = baseMatrix * matrix
	for (i = 0; i < 4; i++)
		for (j = 0; j < 4; j++)
		{
			float s = 0;
			for (k = 0; k < 4; k++)
				s += baseMatrix[k][j] * matrix[i][k];
			vp.modelMatrix[i][j] = s;
		}
#if 0
	DrawTextLeft (va("Org: %9.4g, %9.4g %9.4g", VECTOR_ARG(vp.view.origin)), RGB(0.6,1,0.2));
	for (i = 0; i < 3; i++)
		DrawTextLeft (va("ax[%d] = {%9.4g, %9.4g %9.4g}", i, VECTOR_ARG(vp.view.axis[i])), RGB(0.4,0.8,1));
#define m vp.modelMatrix
	DrawTextLeft (va("----- modelview matrix -----"), RGB(1,0.2,0.2));
	for (i = 0; i < 4; i++)
		DrawTextLeft (va("{%9.4g, %9.4g, %9.4g, %9.4g}", m[0][i], m[1][i], m[2][i], m[3][i]), RGB(0.8,0.3,0.3));
#undef m
#endif
}


static void SetFrustum ()
{
	int		i;

	// setup plane [0]: view direction + znear
	vp.frustum[0].normal = vp.view.axis[0];

#define SCALE	(0.5/360.0f)
	float sx = SIN_FUNC(vp.fov_x * SCALE);	// fov/2 * pi/180
	float sy = SIN_FUNC(vp.fov_y * SCALE);
	float cx = COS_FUNC(vp.fov_x * SCALE);
	float cy = COS_FUNC(vp.fov_y * SCALE);
#undef SCALE
	// calculate normals
	for (i = 0; i < 3; i++)
	{
		vp.frustum[1].normal[i] = vp.view.axis[0][i] * sx + vp.view.axis[1][i] * cx;
		vp.frustum[2].normal[i] = vp.view.axis[0][i] * sx - vp.view.axis[1][i] * cx;
		vp.frustum[3].normal[i] = vp.view.axis[0][i] * sy + vp.view.axis[2][i] * cy;
		vp.frustum[4].normal[i] = vp.view.axis[0][i] * sy - vp.view.axis[2][i] * cy;
	}
	// complete planes
	for (i = 0; i < NUM_FRUSTUM_PLANES; i++)
	{
		vp.frustum[i].type = PLANE_NON_AXIAL;
		vp.frustum[i].dist = dot (vp.view.origin, vp.frustum[i].normal);
		vp.frustum[i].SetSignbits ();
	}
	vp.frustum[0].dist += gl_znear->value;
}


// Create projection matrix as glFrustum() does
static void SetPerspective ()
{
#define m	vp.projectionMatrix
	if (vp.flags & RDF_NOWORLDMODEL)
		vp.zFar = 1024;
	else
	{
		// calculate zFar depends on visible bounding box size
		float d = 0;
		for (int i = 0; i < 8; i++)			// enumarate all 8 verts of bounding box
		{
			CVec3	v;
			CBox &b = vp.bounds;
			v[0] = (i & 1) ? b.maxs[0] : b.mins[0];
			v[1] = (i & 2) ? b.maxs[1] : b.mins[1];
			v[2] = (i & 4) ? b.maxs[2] : b.mins[2];
			// get Z-coordinate
			VectorSubtract (v, vp.view.origin, v);
			float d1 = dot (v, vp.view.axis[0]);
			if (d1 > d) d = d1;
		}
		vp.zFar = d + 100;					// avoid float precision bugs
	}
	//-------------
	float zmin = gl_znear->value;
	float zmax = vp.zFar;
	float xmin = -zmin * vp.t_fov_x;
	float xmax =  zmin * vp.t_fov_x;
	float ymin = -zmin * vp.t_fov_y;
	float ymax =  zmin * vp.t_fov_y;
	/* Matrix contents:
	 *  |   0    1    2    3
	 * -+-------------------
	 * 0|   A    0    C    0
	 * 1|   0    B    D    0
	 * 2|   0    0    E    F
	 * 3|   0    0   -1    0
	 */
	memset (m, 0, sizeof(m));
	m[0][0] = zmin * 2 / (xmax - xmin);				// A
	m[1][1] = zmin * 2 / (ymax - ymin);				// B
	m[2][0] =  (xmax + xmin) / (xmax - xmin);		// C
	m[2][1] =  (ymax + ymin) / (ymax - ymin);		// D
	m[2][2] = -(zmax + zmin) / (zmax - zmin);		// E
	m[2][3] = -1;
	m[3][2] = -2.0f * zmin * zmax / (zmax - zmin);	// F
#undef m
#if 0
#define dr DrawTextRight
	dr (va("zFar: %g;  frustum: x[%g, %g] y[%g, %g]", vp.zFar, xmin, xmax, ymin, ymax), RGB(1,0.1,0.2));
#define m vp.projectionMatrix
	dr (va("----- projection matrix -----"), RGB(1,0.2,0.2));
	dr (va("{%9.4g, %9.4g, %9.4g, %9.4g}", m[0][0], m[1][0], m[2][0], m[3][0]), RGB(0.8,0.3,0.3));
	dr (va("{%9.4g, %9.4g, %9.4g, %9.4g}", m[0][1], m[1][1], m[2][1], m[3][1]), RGB(0.8,0.3,0.3));
	dr (va("{%9.4g, %9.4g, %9.4g, %9.4g}", m[0][2], m[1][2], m[2][2], m[3][2]), RGB(0.8,0.3,0.3));
	dr (va("{%9.4g, %9.4g, %9.4g, %9.4g}", m[0][3], m[1][3], m[2][3], m[3][3]), RGB(0.8,0.3,0.3));
#undef m
#undef dr
#endif
}


// Can be called few RenderFrame() between BeginFrame() and EndFrame()
void RenderFrame (refdef_t *fd)
{
	if (!renderingEnabled) return;

	STAT(gl_stats.Zero ());
	STAT(clock(gl_stats.frontend));

	/*----------- prepare data ------------*/

	gl_state.have3d = true;

	if (!(fd->rdflags & RDF_NOWORLDMODEL))
	{
		byte	floodArea[MAX_MAP_AREAS/8];

		// check for areabits changes
		byte *areas = fd->areabits;
		if (!areas)
		{
			memset (floodArea, 0xFF, sizeof(floodArea));
			areas = &floodArea[0];
		}
		if (memcmp (areaMask, areas, sizeof(areaMask)) != 0)
		{
			memcpy (areaMask, areas, sizeof(areaMask));
			forceVisMap = true;
		}
	}

	gl_state.haveFullScreen3d = (vp.x == 0) && (vp.y == 0) && (vp.w == vid_width) && (vp.h == vid_height);

	// setup viewPortal structure
	memset (&vp, 0, sizeof(vp));
	vp.flags        = fd->rdflags;
	vp.x            = fd->x;
	vp.y            = vid_height - (fd->y + fd->height);
	vp.w            = fd->width;
	vp.h            = fd->height;
	vp.fov_x        = fd->fov_x;
	vp.fov_y        = fd->fov_y;
	vp.t_fov_x      = tan (vp.fov_x * M_PI / 360.0f);
	vp.t_fov_y      = tan (vp.fov_y * M_PI / 360.0f);
	vp.fov_scale    = tan (fd->fov_x / 2.0f / 180.0f * M_PI);
	if (vp.fov_scale < 0.01f) vp.fov_scale = 0.01f;

	// set vp.view before all to allow 3D text output
	vp.view.origin  = fd->vieworg;
	vp.view.axis.FromAngles (fd->viewangles);

	vp.lightStyles  = fd->lightstyles;

	vp.time         = fd->time;				// set scene time

	// add entities
	vp.firstEntity  = gl_numEntities;		//!! gl_numEntities is always 0 here (vp.firstEntity!=0 only when scene contains portals)
	int		i;
	entity_t *ent;
	for (i = 0, ent = fd->entities; i < fd->num_entities; i++, ent++)
		AddEntity (ent);
	// add all CMODEL_SHOW inline models
	inlineModel_t *model = map.models;
	entity_t tempEnt;
	memset (&tempEnt, 0, sizeof(tempEnt));
	tempEnt.pos.axis.FromAngles (tempEnt.angles);
	for (i = 0; i < map.numModels; i++, model++)
	{
		if (!(model->flags & CMODEL_SHOW)) continue;
		// add model via AddEntity()
		tempEnt.model = model;
		AddEntity (&tempEnt);
	}

	// add dlights
	vp.dlights      = &gl_dlights[gl_numDlights]; //!! gl_numDlights is always 0 here
	vp.numDlights   = fd->num_dlights;
	dlight_t *dl;
	for (i = 0, dl = fd->dlights; i < fd->num_dlights; i++, dl++)
		AddDlight (dl);
	// add particle effects
	vp.particles    = fd->particles;
	vp.beams        = fd->beams;

	/*------------ rendering -------------*/

	ClearPortal ();
	SetWorldModelview ();					// prepare modelview matrix
	SetFrustum ();							// setup frustum planes
	DrawPortal ();							// collect data for rasterization

	// NOTE: vp.numSurfaces may be 0, when only sky is visible
	SetPerspective ();						// prepare projection matrix using some info from DrawPortal()

	STAT(unclock(gl_stats.frontend));

	STAT(clock(gl_stats.backend));
	BK_DrawScene ();						// rasterization
	STAT(unclock(gl_stats.backend));

	/*------------ debug info ------------*/

#if STATS
	if (r_stats->integer)
	{
		//?? implement as loop, with table {OFS2FIELD(), name}
#define S gl_stats
#define T(name)		appDeltaCyclesToMsecf (gl_stats.name)
		DrawTextRight (va(
						"--- frontend %5.2f ---\n"
						" dlight surf   %.3f\n"
						" occl test     %.3f\n"
						" flare trace   %.3f\n"
						"--- backend  %5.2f ---\n"
						" sort          %.3f\n"
						" entity light  %.3f\n"
						" mesh tess     %.3f\n"
						" mesh light    %.3f\n"
						" comp dyn lm   %.3f",
						T(frontend), T(dlightSurf), T(occlTest), T(flareTrace),
						T(backend), T(sort), T(entLight), T(meshTess), T(meshLight), T(dynLightmap)
			), RGB(0.1,0.6,0.1));
		int lgridSize = map.mapGrid[0]*map.mapGrid[1]*map.mapGrid[2];
		if (!lgridSize) lgridSize = 1;	// to avoid zero divide
		//?? can be processed per-frame (not per-scene): tris2D
		//?? split structure: gl_stats -> frameStats + sceneStats
		DrawTextRight (va(
			"------ common counts ------\n"
			" surfs: %d culled: %d\n"
			" tris: %d (+%d 2D) mtex: %1.2f\n"
			" ents: %d culled: %2d occl:  %d\n"
			" bind: %d upload: %2d flush: %d",
			S.surfs, S.cullSurfs,
			S.trisMT, S.tris2D, S.trisMT ? (float)S.tris / S.trisMT : 0,
			S.ents, S.cullEnts, S.ocullEnts,
			S.numBinds, S.numUploads, S.numFlushes
			), RGB(0.1,0.6,0.1));

		if (!(fd->rdflags & RDF_NOWORLDMODEL))
		{
		DrawTextRight (va(
			"--------- world ----------\n"
			"leafs: vis: %-3d fr: %-3d total: %d\n"
			"particles: %d cull: %d\n"
			"dlights: %d surfs: %d verts: %d\n"
			"flares: %d test: %d cull: %d\n"
			"lightgrid: %d/%d (%.1f%%)\n",		//?? add memory size too
			S.visLeafs, S.frustLeafs, map.numLeafNodes - map.numNodes,
			S.parts, S.cullParts,
			gl_numDlights, S.dlightSurfs, S.dlightVerts,
			S.flares, S.testFlares, S.cullFlares,
			map.numLightCells, lgridSize, (float)map.numLightCells / lgridSize * 100
			), RGB(0.1,0.6,0.1));
		}
#undef S
#undef T
	}
#endif // STATS

	ClearBuffers ();						// cleanup scene info (after stats display)
	vp.time = defaultTime;					// restore time for 2D
}


/*--------------------- 2D picture output ---------------------*/

void Fill8 (int x, int y, int w, int h, int c)
{
	BK_DrawPic (gl_identityLightShader, x, y, w, h, 0, 0, 0, 0, gl_config.tbl_8to32[c]);
}

void Fill (int x, int y, int w, int h, unsigned rgba)
{
	BK_DrawPic (gl_identityLightShader, x, y, w, h, 0, 0, 0, 0, rgba);
}

void TileClear (int x, int y, int w, int h, const char *name)
{
	shader_t *sh = FindShader (name, SHADER_CHECK);
	if (sh)
		BK_DrawPic (sh, x, y, w, h, (float)x / sh->width, (float)y / sh->height,
			(float)(x + w) / sh->width, (float)(y + h) / sh->height);
	else
		Fill (x, y, w, h, RGB(0,0,0));
}

inline shader_t *FindPic (const char *name, bool force)	//?? rename
{
	int flags = SHADER_ALPHA|SHADER_CLAMP;
	if (!force) flags |= SHADER_CHECK;
	return FindShader (name, flags);
}

void DrawStretchPic (int x, int y, int w, int h, const char *name)
{
	BK_DrawPic (FindPic (name, true), x, y, w, h);
}


void DrawDetailedPic (int x, int y, int w, int h, const char *name)
{
	shader_t *sh = FindPic (name, true);
	BK_DrawPic (sh, x, y, w, h);
	if (!gl_detailShader || sh->width * 2 > w) return;		// detail is not available or not needed
//	if (!Cvar_VariableInt("detail")) return;
	BK_DrawPic (gl_detailShader, x, y, w, h, 0, 0, w / gl_detailShader->width, h / gl_detailShader->height);
}


void DrawPic (int x, int y, const char *name, int anchor, int color)
{
	shader_t *sh = FindPic (name, true);
	// invalid color -> white
	if (color < 0 || color > 7) color = C_WHITE;
	// apply anchor displacement
	x -= (anchor & 0xF) * sh->width / 2;			// 0x00 -- keep X; 0x01 -- x-=w/2; 0x02 -- x-=w
	y -= (anchor & 0xF0) * sh->height / (2 * 16);	// 0x00 -- keep Y; 0x10 -- y-=h/2; 0x20 -- y-=h
	// draw
	BK_DrawPic (sh, x, y, sh->width, sh->height, 0, 0, 1, 1, colorTable[color]);
}


/*------------------- Model registration ----------------------*/

void LoadNewWorld (const char *mapname)
{
	STAT(gl_ldStats.Zero ());		// clear loading stats
	ResetShaders ();				// delete all shaders and re-create auto-shaders
	LoadWorldMap (va("maps/%s.bsp", mapname));
	forceVisMap = true;
}


#if STATS
void DumpLoadStats ()
{
	if (!r_stats->integer) return;

	// draw loading statistics
	typedef struct {
		int64 *data;
		const char *name;
	} statInfo_t;
	static const statInfo_t info[] = {
#define F(field,name)	&gl_ldStats.field, name
		F(imgResample,		"tex resample"),
		F(imgMipmap,		"tex mipmap"),
		F(imgLightscale,	"tex lightscale"),
		F(imgUpload,		"tex upload"),
		F(md2normals,		"md2 normals"),
		F(bspLoad,			"bsp load")
#undef F
	};
	int i;
	int maxLen = 0;
	for (i = 0; i < ARRAY_COUNT(info); i++)
	{
		int len = strlen (info[i].name);
		if (len > maxLen) maxLen = len;
	}
	TString<64> Fmt;
	// build "%-NNs  %7.3f ms"
	Fmt.sprintf (S_GREEN"%%-%ds  %%7.3f ms\n", maxLen);
	for (i = 0; i < ARRAY_COUNT(info); i++)
	{
		if (!*info[i].data) continue;	// skip this line
		appPrintf (Fmt, info[i].name, appDeltaCyclesToMsecf (*info[i].data));
	}
}
#endif

void FinishLoadingWorld ()
{
	STAT(DumpLoadStats ());
}


CBasicImage *RegisterPic (const char *name)
{
	return FindPic (name, false);
}

CBasicImage *RegisterSkin (const char *name, bool force)
{
	unsigned flags = SHADER_SKIN;
	if (!force) flags |= SHADER_CHECK;
	return FindShader (name, flags);
}

CRenderModel *RegisterModel (const char *name)
{
	return FindModel (name);
}


void SetSky (const char *name, float rotate, const CVec3 &axis)
{
	shader_t *old = gl_skyShader;
	// find sky shader
	TString<64> Name2;
	Name2.sprintf ("env/%s", name);
	shader_t *shader = FindShader (Name2, SHADER_SKY);
	if (shader->type != SHADERTYPE_SKY)
	{
		appWPrintf ("%s is not a sky shader\n", *Name2);
		shader = gl_skyShader;
	}

	if (shader != gl_defaultSkyShader)
	{
		shader->skyRotate = rotate;
		shader->skyAxis = axis;
	}

	if (shader == old)
		return;		// sky is not changed

	gl_skyShader = shader;
	// change all sky surfaces
	//?? is it really needed ? whole map uses at most 1 sky shader, which processed specially
	for (int i = 0; i < map.numFaces; i++)
	{
		surfaceBase_t *surf = map.faces[i];
		if (surf->shader == old) surf->shader = shader;
	}
}

void Screenshot (int flags, const char *name)
{
	static TString<64> ShotName;
	ShotName        = name;
	screenshotFlags = flags;
	screenshotName  = ShotName;
}


float GetClientLight ()
{
	return 150;		//!! need to implement (outside renderer !!)
}


/*-----------------------------------------------------------------------------
	Renderer interface
-----------------------------------------------------------------------------*/

#if !SINGLE_RENDERER
#	include "../client/rexp_exp.h"
RENDERER_EXPORT
#endif


} // namespace
