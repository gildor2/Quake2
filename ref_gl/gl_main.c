#include "gl_local.h"
#include "gl_frontend.h"
#include "gl_backend.h"
#include "gl_math.h"
#include "gl_image.h"
#include "gl_buffers.h"


glrefdef_t	gl_refdef;

drawSpeeds_t gl_speeds;

static int ref_flags;
refImport_t	ri;

bool gl_renderingEnabled;


//------------- Cvars -----------------

// initialization/GLimp
cvar_t	*vid_ref;
cvar_t	*gl_mode, *gl_bitdepth, *r_fullscreen;
cvar_t	*gl_driver;
// GLimp
cvar_t	*gl_allow_software;	//??
cvar_t	*gl_drawbuffer;

// image-specific
cvar_t	*gl_nobind;

cvar_t	*r_ignorehwgamma;	// rename ??
cvar_t	*gl_overbright;
cvar_t	*r_gamma, *r_brightness, *r_contrast, *r_saturation;

cvar_t	*gl_texMipBias, *gl_skinMipBias;
cvar_t	*gl_texturemode;

// rendering speed/quality settings
cvar_t	*gl_fastsky;		// do not draw skybox
cvar_t	*gl_fog;			//??

cvar_t	*gl_flares;
cvar_t	*gl_dynamic;		// enable dynamic lightmaps for Q2/HL maps
cvar_t	*gl_dlightBacks;	// when disabled, do not draw dlights on backfaces
cvar_t	*gl_vertexLight;	// use glColor() against lightmaps
cvar_t	*gl_nogrid;
cvar_t	*gl_showgrid;

cvar_t	*gl_ignoreFastPath;	// do not use GenericStageIterator() when possible (remove ???)

// game settings
cvar_t	*gl_hand;			// for weapon model

// renderer settings
cvar_t	*gl_znear;
cvar_t	*gl_swapinterval;
cvar_t	*gl_maxTextureUnits;
cvar_t	*gl_finish;			// debug ? (can be a situation, when gl_finish==1 is necessary ? (linux))

// debugging
cvar_t	*gl_logFile;
cvar_t	*r_novis, *r_nocull, *gl_oCull, *gl_facePlaneCull;
cvar_t	*r_speeds;
cvar_t	*r_fullbright, *r_lightmap;
cvar_t	*gl_showsky;
cvar_t	*r_drawworld, *r_drawentities;
cvar_t	*gl_sortAlpha;

cvar_t	*gl_labels;
cvar_t	*gl_lightLines, *gl_showLights;
cvar_t	*gl_singleShader;

static cvar_t	*gl_logTexts;


static void ClearTexts (void);
void DrawTexts (void);


static void Gfxinfo_f (void)
{
	static const char *boolNames[] = {"no", "yes"};
	static const char *overbrNames[2][2] = {"disabled", "forced", "unneeded", "required"};

	Com_Printf ("^1---------- OpenGL info ----------\n");
	Com_Printf ("^1Vendor:^7 %s\n", glGetString (GL_VENDOR));
	Com_Printf ("^1Renderer:^7 %s\n", glGetString (GL_RENDERER));
	Com_Printf ("^1Version:^7 %s\n", glGetString (GL_VERSION));
	//?? colorize used/disabled extensions
	Com_Printf ("^1Base extensions:^7 %s\n", gl_config.extensions);
	if (gl_config.extensions2)
		Com_Printf ("^1Platform extensions:^7 %s\n", gl_config.extensions2);
	Com_Printf ("^1---------------------------------\n");
	Com_Printf ("Multitexturing: ");
	if (GL_SUPPORT(QGL_ARB_MULTITEXTURE|QGL_SGIS_MULTITEXTURE))
	{
		char	name[256];

		strcpy (name, GL_SUPPORT(QGL_ARB_MULTITEXTURE) ? "ARB" : "SGIS");
		if (GL_SUPPORT(QGL_ARB_TEXTURE_ENV_ADD|QGL_EXT_TEXTURE_ENV_ADD))
			strcat (name, " +Add");
		if (GL_SUPPORT(QGL_ARB_TEXTURE_ENV_COMBINE|QGL_EXT_TEXTURE_ENV_COMBINE))
			strcat (name, " +Combine");
		if (GL_SUPPORT(QGL_NV_TEXTURE_ENV_COMBINE4))
			strcat (name, " +NV");

		Com_Printf ("yes, %s, %d texture units\n", name, gl_config.maxActiveTextures);
	}
	else
		Com_Printf ("no\n");
	Com_Printf ("Lighting: %s\n", gl_config.vertexLight ? "vertex" : "lightmap");
	Com_Printf ("Gamma: ");
	if (gl_config.deviceSupportsGamma)
		Com_Printf ("hardware, overbright: %s (%s), lightmap overbright: %s\n",
			//?? NOTE: here used gl_overbright->integer, which can be modified after vid_restart (can be incorrect note)
			boolNames[gl_config.overbright], overbrNames[gl_overbright->integer == 2][gl_config.overbright],
			boolNames[!gl_config.doubleModulateLM]);
	else
		Com_Printf ("software\n");
}


static void GL_Register (void)
{
/* NOTE: some cvars have "CVAR_NOUPDATE" flag -- for correct video-config-menu work (to detect changes)
 * Every cvar here can have this flag (?) (but only few of them REQUIRE it)
 * If cvar have "CVAR_UPDATE" flag, its value will be re-analyzed (settings, which are (potentially) inactive by defaults)
 */
CVAR_BEGIN(vars)
	CVAR_VAR(gl_texturemode, GL_LINEAR_MIPMAP_NEAREST, CVAR_ARCHIVE),
	CVAR_VAR(gl_texMipBias, -0.5, CVAR_ARCHIVE),
	CVAR_VAR(gl_skinMipBias, -1, CVAR_ARCHIVE),
	CVAR_VAR(r_gamma, 1, CVAR_ARCHIVE),
	CVAR_VAR(r_brightness, 1, CVAR_ARCHIVE),
	CVAR_VAR(r_contrast, 1, CVAR_ARCHIVE),
	CVAR_VAR(r_saturation, 1, CVAR_ARCHIVE|CVAR_NOUPDATE),
	CVAR_VAR(r_ignorehwgamma, 0, CVAR_ARCHIVE),
	CVAR_VAR(gl_overbright, 2, CVAR_ARCHIVE|CVAR_NOUPDATE),
	CVAR_VAR(gl_bitdepth, 0, CVAR_ARCHIVE|CVAR_NOUPDATE),
	//!! add gl_depthBits

	CVAR_VAR(gl_fastsky, 0, CVAR_ARCHIVE),
	CVAR_VAR(gl_fog, 0, 0),	//?? ARCHIVE? CHEAT?

	CVAR_VAR(gl_flares, 1, CVAR_ARCHIVE),
	CVAR_VAR(gl_dynamic, 1, 0),
	CVAR_VAR(gl_dlightBacks, 1, 0),
	CVAR_VAR(gl_vertexLight, 0, CVAR_ARCHIVE|CVAR_NOUPDATE),
	CVAR_VAR(gl_nogrid, 0, 0),
	CVAR_VAR(gl_showgrid, 0, CVAR_CHEAT),

	CVAR_VAR(gl_ignoreFastPath, 1, 0),						//!! use with VertexArrayRange ONLY !!

	{&gl_hand, "hand", "0", CVAR_USERINFO|CVAR_ARCHIVE},

	CVAR_VAR(gl_znear, 4, 0),
	CVAR_VAR(gl_swapinterval, 0, CVAR_ARCHIVE|CVAR_UPDATE),

	CVAR_VAR(gl_driver, opengl32, CVAR_ARCHIVE),			//?? use different gl_driver default value for Linux
	CVAR_VAR(gl_mode, 3, CVAR_ARCHIVE),

	CVAR_NULL(gl_ext_multitexture, 1, CVAR_ARCHIVE),
	CVAR_VAR(gl_maxTextureUnits, 0, CVAR_ARCHIVE),
	CVAR_NULL(gl_ext_texture_env_add, 1, CVAR_ARCHIVE),
	CVAR_NULL(gl_ext_texture_env_combine, 1, CVAR_ARCHIVE),
	CVAR_NULL(gl_ext_texture_env_combine_nv, 1, CVAR_ARCHIVE),
	CVAR_NULL(gl_ext_compiled_vertex_array, 1, CVAR_ARCHIVE),
	CVAR_NULL(gl_ext_fog_distance_nv, 1, CVAR_ARCHIVE),
	CVAR_NULL(gl_ext_vertex_array_range, 0, CVAR_ARCHIVE),	//?? do not works now
	CVAR_NULL(gl_ext_compressed_textures, 1, CVAR_ARCHIVE),

	CVAR_VAR(gl_nobind, 0, 0),
	CVAR_VAR(gl_logFile, 0, 0),
	CVAR_VAR(r_novis, 0, 0),
	CVAR_VAR(r_nocull, 0, 0),
	CVAR_VAR(gl_oCull, 1, 0),
	CVAR_VAR(gl_facePlaneCull, 1, 0),
	CVAR_VAR(r_speeds, 0, 0),
	CVAR_VAR(r_fullbright, 0, CVAR_CHEAT),
	CVAR_VAR(r_lightmap, 0, CVAR_CHEAT),
	CVAR_VAR(gl_showsky, 0, 0),
	CVAR_VAR(r_drawworld, 1, CVAR_CHEAT),
	CVAR_VAR(r_drawentities, 1, 0),
	CVAR_VAR(gl_sortAlpha, 0, 0),
	CVAR_VAR(gl_labels, 0, CVAR_CHEAT),
	CVAR_VAR(gl_lightLines, 0, CVAR_CHEAT),
	CVAR_VAR(gl_showLights, 0, 0),
	CVAR_VAR(gl_singleShader, 0, CVAR_CHEAT),
	CVAR_VAR(gl_logTexts, 0, 0),
	CVAR_VAR(gl_finish, 0, CVAR_ARCHIVE),

	CVAR_VAR(r_fullscreen, 1, CVAR_ARCHIVE),
	CVAR_GET(vid_ref),

	CVAR_VAR(gl_allow_software, 0, 0),
	CVAR_VAR(gl_drawbuffer, GL_BACK, 0)
CVAR_END

	Cvar_GetVars (ARRAY_ARG(vars));

	Cmd_AddCommand ("gfxinfo", Gfxinfo_f);
}


static bool GL_SetMode (void)
{
	rserr_t err;
	bool	fullscreen;

	fullscreen = r_fullscreen->integer;

	r_fullscreen->modified = false;
	gl_mode->modified = false;

	if ((err = GLimp_SetMode (&vid.width, &vid.height, gl_mode->integer, fullscreen)) == rserr_ok)
	{
		gl_config.prevMode = gl_mode->integer;
		gl_config.prevBPP = gl_bitdepth->integer;
		gl_config.prevFS = gl_config.fullscreen;
	}
	else
	{
		if  (err == rserr_invalid_fullscreen)
		{
			Cvar_SetInteger ("r_fullscreen", 0);
			r_fullscreen->modified = false;
			Com_WPrintf ("R_SetMode() - fullscreen unavailable in this mode\n");
			if ((err = GLimp_SetMode (&vid.width, &vid.height, gl_mode->integer, false)) == rserr_ok)
				return true;
		}
		else if (err == rserr_invalid_mode)
		{
			Cvar_SetInteger ("gl_mode", gl_config.prevMode);
			Cvar_SetInteger ("gl_bitdepth", gl_config.prevBPP);
			gl_mode->modified = false;
			Com_WPrintf ("R_SetMode() - invalid mode\n");
		}

		// try setting it back to something safe
		if ((err = GLimp_SetMode (&vid.width, &vid.height, gl_config.prevMode, gl_config.prevFS)) != rserr_ok)
		{
			Com_WPrintf ("R_SetMode() - could not revert to safe mode\n");	//?? "to previous mode" ? (but, here will be mode "3")
			return false;
		}
	}
	return true;
}


static int GL_Init (void)
{
	Com_Printf ("--- Initializing OpenGL renderer ---\n");

	Com_Printf ("ref_gl version: "REF_VERSION"\n");		//?? remove

	GL_Register ();

	// initialize our QGL dynamic bindings
	if (!QGL_Init (gl_driver->string))
	{
		if (strcmp (gl_driver->string, gl_driver->reset_string))
		{
			Com_WPrintf ("Cannot load %s - trying default driver (%s) ... ", gl_driver->string, gl_driver->reset_string);
			if (QGL_Init (gl_driver->reset_string))
			{
				Com_Printf ("OK\n");
				Cvar_Set ("gl_driver", gl_driver->reset_string);
			}
			else
			{
        		Com_Printf ("^1failed\n");
				QGL_Shutdown ();
				return -1;
			}
		}
		else
		{
			Com_WPrintf ("Cannot load %s\n", gl_driver->string);
			QGL_Shutdown ();
			return -1;
		}
	}

	// set our "safe" mode
	gl_config.prevMode = 3;				//??

	/*----- create the window and set up the context -----*/
	if (!GL_SetMode ())
	{
		QGL_Shutdown ();
        Com_WPrintf ("ref_gl::R_Init() - could not R_SetMode()\n");
		return -1;
	}

	// initialize OS-specific parts of OpenGL
	if (!GLimp_Init ())
	{
		QGL_Shutdown ();
		return -1;
	}

	/*----------- Initialize video config menu -----------*/
//??	Vid_MenuInit ();

#ifdef __linux__
	Cvar_SetInteger ("gl_finish", 1);	//??
#endif

	/*------------------ Grab extensions ----------------------*/
	//?? move this part to gl_interface.c ??
	QGL_InitExtensions ();

	if (GL_SUPPORT(QGL_ARB_MULTITEXTURE))
		glGetIntegerv (GL_MAX_TEXTURE_UNITS_ARB, &gl_config.maxActiveTextures);
	else if (GL_SUPPORT(QGL_SGIS_MULTITEXTURE))
		glGetIntegerv (GL_MAX_TEXTURES_SGIS, &gl_config.maxActiveTextures);
	else
		gl_config.maxActiveTextures = 1;
	if (gl_maxTextureUnits->integer > 0 && gl_config.maxActiveTextures > gl_maxTextureUnits->integer)
	{
		Com_Printf ("^6...multitexture limited by %d units\n", gl_maxTextureUnits->integer);
		gl_config.maxActiveTextures = gl_maxTextureUnits->integer;
	}

	gl_config.doubleModulateLM = true;			// no multitexture or env_combine
	if (GL_SUPPORT(QGL_ARB_MULTITEXTURE|QGL_SGIS_MULTITEXTURE) &&
		!GL_SUPPORT(QGL_EXT_TEXTURE_ENV_COMBINE|QGL_ARB_TEXTURE_ENV_COMBINE|QGL_NV_TEXTURE_ENV_COMBINE4))
		gl_config.doubleModulateLM = false;


	//?? place this decision to Upload() and remove formatSolid from gl_config?
	if (GL_SUPPORT(QGL_ARB_TEXTURE_COMPRESSION))
	{
		gl_config.formatSolid = GL_COMPRESSED_RGB_ARB;
		gl_config.formatAlpha = GL_COMPRESSED_RGBA_ARB;
		gl_config.formatAlpha1 = GL_COMPRESSED_RGBA_ARB;
//		glHint (GL_TEXTURE_COMPRESSION_HINT_ARB, GL_NICEST);
	}
	else if (GL_SUPPORT(QGL_S3_S3TC))
	{
		gl_config.formatSolid = GL_RGB4_S3TC;
		gl_config.formatAlpha = 0;
		gl_config.formatAlpha1 = 0;
	}

	glGetIntegerv (GL_MAX_TEXTURE_SIZE, &gl_config.maxTextureSize);

	if (gl_dynamic->integer != 2)
		gl_config.multiPassLM = true;

	// set screen to particular color while initializing
	glClearColor (0.1, 0, 0, 1);
	glClear (GL_COLOR_BUFFER_BIT);
	GLimp_EndFrame ();

	GL_SetDefaultState ();

	GL_InitFuncTables ();
	GL_InitImages ();
	GL_InitShaders ();
	GL_InitModels ();
	GL_InitBackend ();

	gl_refdef.viewCluster = -2;		// force update visinfo for map:
				// -1 is "no visinfo", >= 0 -- visinfo, so, -2 is unused (not reserved)

	return 0;	// all OK
}


static void GL_Shutdown (void)
{
	Cmd_RemoveCommand ("gfxinfo");

	GL_ShutdownBackend ();
	GL_ShutdownModels ();
	GL_ShutdownShaders ();
	GL_ShutdownImages ();

	GL_SetMultitexture (0);

	// shut down OS-specific OpenGL stuff like contexts, etc.
	GLimp_Shutdown ();

	// shutdown our QGL subsystem
	QGL_Shutdown ();
}


void GL_EnableRendering (bool enable)
{
	if (gl_renderingEnabled == enable) return;
	gl_renderingEnabled = enable;
	if (enable)
	{
		GL_LoadDelayedImages ();
	}
}


/*---------------- Backend helpers ----------------------*/

static void GL_DrawStretchPic (shader_t *shader, int x, int y, int w, int h, float s1, float t1, float s2, float t2, unsigned color)
{
	PUT_BACKEND_COMMAND (bkDrawPic_t, p);
	p->type = BACKEND_PIC;
	p->shader = shader;
	p->x = x;
	p->y = y;
	p->w = w;
	p->h = h;
	if (w > shader->width * 2)
	{
		s1 += 0.5f / shader->width;
		s2 -= 0.5f / shader->width;
	}
	if (h > shader->height * 2)
	{
		t1 += 0.5f / shader->height;
		t2 -= 0.5f / shader->height;
	}
	p->s1 = s1;
	p->t1 = t1;
	p->s2 = s2;
	p->t2 = t2;
	p->c.rgba = color;
}


/*--------------------- Frame ---------------------------*/


static void GL_BeginFrame (float camera_separation)
{
	if (!gl_renderingEnabled)
	{
		backendCmdSize = 0;				// avoid overflow
		return;
	}

	if (gl_logFile->modified)
	{
		QGL_EnableLogging (gl_logFile->integer);
		gl_logFile->modified = false;
	}
	else if (gl_logFile->integer == 2)
	{
		QGL_EnableLogging (false);
		Cvar_SetInteger ("gl_logFile", 0);
	}

//	DrawTextRight ("---------BeginFrame---------\n", RGB(1,0,0));
	LOG_STRING ("\n---------- Begin Frame ----------\n\n");

	if (gl_finish->integer == 2) glFinish ();
	gl_speeds.beginFrame = Sys_Milliseconds ();
	gl_speeds.numFrames = 0;

	//?? what to do with the camera_separation ?

	gl_state.have3d = false;
	gl_state.maxUsedShaderIndex = -1;

	if (gl_texturemode->modified)
	{
		GL_TextureMode (gl_texturemode->string);
		gl_texturemode->modified = false;
	}

	/*----- update hardware gamma (if present) ------*/
	if ((r_gamma->modified || r_contrast->modified || r_brightness->modified) && gl_config.deviceSupportsGamma)
	{
		GL_SetupGamma ();
		r_gamma->modified = false;
		r_contrast->modified = false;
		r_brightness->modified = false;
	}

	/*------ change modes if necessary ---------*/
	if (gl_mode->modified || r_fullscreen->modified)
		vid_ref->modified = true;

	{
		PUT_BACKEND_COMMAND (int, p);
		*p = BACKEND_BEGIN_FRAME;
	}

//?	GLimp_BeginFrame (camera_separation); -- do (almost in Win32, totally in Linux) nothing
}


static void GL_EndFrame (void)
{
	if (!gl_renderingEnabled) return;

	if (!gl_speeds.numFrames) ClearTexts ();

//	DrawTextRight ("---------EndFrame---------\n", RGB(1,0,0));
	{
		PUT_BACKEND_COMMAND (int, p);
		*p = BACKEND_END_FRAME;
	}

	GL_BackEnd ();
	GL_ClearBuffers ();


	if (gl_finish->integer == 2) glFinish ();
	gl_speeds.endFrame = Sys_Milliseconds ();
	if (r_speeds->integer && gl_speeds.beginFrame <= gl_speeds.begin3D)	// draw only when have 3D on screen
	{
		DrawTextRight (va("fe: %2d bk: %2d (srt: %2d 3d: %2d 2d: %2d)",
			gl_speeds.beginSort - gl_speeds.beginFrame,
			gl_speeds.endFrame - gl_speeds.beginSort,
			gl_speeds.begin3D - gl_speeds.beginSort,
			gl_speeds.begin2D - gl_speeds.begin3D,
			gl_speeds.endFrame - gl_speeds.begin2D
			), RGB(1,0.5,0));
	}
}


/*----------------- Rendering ---------------------*/

static void SetWorldModelview (void)
{
	float	matrix[4][4];
	int		i, j, k;
	static float baseMatrix[4][4] = {	// axias {0 0 -1} {-1 0 0} {0 1 0}
		{  0,  0, -1,  0},
		{ -1,  0,  0,  0},
		{  0,  1,  0,  0},
		{  0,  0,  0,  1}
	};

	// all fields should be zeroed before call this function !
//	VectorCopy (vp.vieworg, vp.modelvieworg); // portals ??
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
			matrix[i][j] = vp.viewaxis[j][i];
	matrix[3][0] = -DotProduct (vp.vieworg, vp.viewaxis[0]);
	matrix[3][1] = -DotProduct (vp.vieworg, vp.viewaxis[1]);
	matrix[3][2] = -DotProduct (vp.vieworg, vp.viewaxis[2]);
	matrix[3][3] = 1;
	// vp.modelMatrix = baseMatrix * matrix
	for (i = 0; i < 4; i++)
		for (j = 0; j < 4; j++)
		{
			float	s;

			s = 0;
			for (k = 0; k < 4; k++)
				s += baseMatrix[k][j] * matrix[i][k];
			vp.modelMatrix[i][j] = s;
		}
#if 0
#define m vp.vieworg
	DrawTextLeft (va("Org: %9.4g, %9.4g %9.4g", VECTOR_ARG(m)), RGB(0.6,1,0.2));
#undef m
#define m vp.viewaxis
	for (i = 0; i < 3; i++)
		DrawTextLeft (va("ax[%d] = {%9.4g, %9.4g %9.4g}", i, VECTOR_ARG(m[i])), RGB(0.4,0.8,1));
#undef m
#define m vp.modelMatrix
	DrawTextLeft (va("----- modelview matrix -----"), RGB(1,0.2,0.2));
	for (i = 0; i < 4; i++)
		DrawTextLeft (va("{%9.4g, %9.4g, %9.4g, %9.4g}", m[0][i], m[1][i], m[2][i], m[3][i]), RGB(0.8,0.3,0.3));
#undef m
#endif
}


static void SetFrustum (void)
{
	float	sx, sy, cx, cy;
	int		i;

	// setup plane [0]: view direction + znear
	VectorCopy (vp.viewaxis[0], vp.frustum[0].normal);

#define SCALE	(0.5/360.0f)
	sx = SIN_FUNC(vp.fov_x * SCALE);	// fov/2 * pi/180
	sy = SIN_FUNC(vp.fov_y * SCALE);
	cx = COS_FUNC(vp.fov_x * SCALE);
	cy = COS_FUNC(vp.fov_y * SCALE);
#undef SCALE
	// calculate normals
	for (i = 0; i < 3; i++)
	{
		vp.frustum[1].normal[i] = vp.viewaxis[0][i] * sx + vp.viewaxis[1][i] * cx;
		vp.frustum[2].normal[i] = vp.viewaxis[0][i] * sx - vp.viewaxis[1][i] * cx;
		vp.frustum[3].normal[i] = vp.viewaxis[0][i] * sy + vp.viewaxis[2][i] * cy;
		vp.frustum[4].normal[i] = vp.viewaxis[0][i] * sy - vp.viewaxis[2][i] * cy;
	}
	// complete planes
	for (i = 0; i < NUM_FRUSTUM_PLANES; i++)
	{
		vp.frustum[i].type = PLANE_NON_AXIAL;
		vp.frustum[i].dist = DotProduct (vp.vieworg, vp.frustum[i].normal);
		SetPlaneSignbits (&vp.frustum[i]);
	}
	vp.frustum[0].dist += gl_znear->value;
}


// Create projection matrix as glFrustum() does
static void SetPerspective (void)
{
#define m	vp.projectionMatrix
	float	xmax, xmin, ymax, ymin, zmin, zmax;

	if (vp.flags & RDF_NOWORLDMODEL)
		vp.zFar = 1024;
	else
	{	// calculate zFar depends on visible bounding box size
		int		i;
		float	d;

		d = 0;
		for (i = 0; i < 8; i++)	// check all 8 verts of bounding box
		{
			vec3_t	v;
			float	d1;

			v[0] = (i & 1) ? vp.maxs[0] : vp.mins[0];
			v[1] = (i & 2) ? vp.maxs[1] : vp.mins[1];
			v[2] = (i & 4) ? vp.maxs[2] : vp.mins[2];

			VectorSubtract (v, vp.vieworg, v);
			d1 = DotProduct (v, v);		// square of vector length
			if (d1 > d) d = d1;
		}
		vp.zFar = SQRTFAST(d) + 100;	// avoid precision errors
	}
	//-------------
	zmin = gl_znear->value;
	zmax = vp.zFar;
	xmin = -zmin * vp.t_fov_x;
	xmax = zmin * vp.t_fov_x;
	ymin = -zmin * vp.t_fov_y;
	ymax = zmin * vp.t_fov_y;
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
	m[2][0] = (xmax + xmin) / (xmax - xmin);		// C
	m[2][1] = (ymax + ymin) / (ymax - ymin);		// D
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
//?? so: rename - RenderPortal ?
static void GL_RenderFrame (refdef_t *fd)
{
	entity_t *ent;
	dlight_t *dl;
	int		i;

	if (!gl_renderingEnabled) return;

	if (!(fd->rdflags & RDF_NOWORLDMODEL) && !map.name)
		Com_Error (ERR_FATAL, "R_RenderFrame: NULL worldModel");

//	DrawTextRight ("---------RenderFrame---------\n", RGB(1,0,0));

	/*----------- prepare data ------------*/

	gl_state.have3d = true;
	if (!(fd->rdflags & RDF_NOWORLDMODEL))
	{
		byte	*areas, floodArea[MAX_MAP_AREAS/8];

		// check for areabits changes
		areas = fd->areabits;
		if (!areas)
		{
			memset (floodArea, 0xFF, sizeof(floodArea));
			areas = &floodArea[0];
		}
		gl_refdef.areaMaskChanged = memcmp (gl_refdef.areaMask, fd->areabits, sizeof(gl_refdef.areaMask));
		memcpy (gl_refdef.areaMask, fd->areabits, sizeof(floodArea));
	}

	/*------------ rendering -------------*/

	// setup viewPortal structure
	memset (&vp, 0, sizeof(vp));
	vp.flags = fd->rdflags;
	vp.x = fd->x;
	vp.y = vid.height - (fd->y + fd->height);
	vp.w = fd->width;
	vp.h = fd->height;
	vp.fov_x = fd->fov_x;
	vp.fov_y = fd->fov_y;
	vp.t_fov_x = tan (vp.fov_x * M_PI / 360.0f);
	vp.t_fov_y = tan (vp.fov_y * M_PI / 360.0f);
	vp.fov_scale = tan (fd->fov_x / 2.0f / 180.0f * M_PI);
	if (vp.fov_scale < 0.01) vp.fov_scale = 0.01;

	// set vieworg/viewaxis before all to allow 3D text output
	VectorCopy (fd->vieworg, vp.vieworg);
	AnglesToAxis (fd->viewangles, vp.viewaxis);

	vp.lightStyles = fd->lightstyles;
	vp.time = fd->time;

//DrawTextLeft(va("begin scene: %d ents (%d+) %d dlights (%d+)",fd->num_entities,gl_numEntities,fd->num_dlights,gl_numDlights), RGB(1,0,0));
	// add entities
	gl_speeds.ents = gl_speeds.cullEnts = gl_speeds.cullEntsBox = gl_speeds.cullEnts2 = gl_speeds.ocullEnts = 0;
	vp.firstEntity = gl_numEntities;
	for (i = 0, ent = fd->entities; i < fd->num_entities; i++, ent++)
		GL_AddEntity (ent);

	// add dlights
	vp.dlights = &gl_dlights[gl_numDlights];
	vp.numDlights = fd->num_dlights;
	for (i = 0, dl = fd->dlights; i < fd->num_dlights; i++, dl++)
		GL_AddDlight (dl);

	gl_speeds.dlightSurfs = gl_speeds.dlightVerts = 0;
	gl_speeds.flares = gl_speeds.testFlares = gl_speeds.cullFlares = 0;
	gl_speeds.parts = gl_speeds.cullParts = 0;
	vp.particles = fd->particles;
	vp.beams = fd->beams;

	GL_ClearPortal ();
	SetWorldModelview ();
	SetFrustum ();

	GL_DrawPortal ();

	SetPerspective ();

	GL_FinishPortal ();

	if (fd->blend[3])
	{
		color_t		c;

//		DrawTextLeft(va("blend: %f %f %f %f",VECTOR_ARG(fd->blend),fd->blend[3]), RGB(1,1,1));
		// standard Quake2 blending
		c.c[0] = Q_round (fd->blend[0] * 255);
		c.c[1] = Q_round (fd->blend[1] * 255);
		c.c[2] = Q_round (fd->blend[2] * 255);
		c.c[3] = Q_round (fd->blend[3] * 255);
		GL_DrawStretchPic (gl_identityLightShader, 0, 0, vid.width, vid.height, 0, 0, 0, 0, c.rgba);
	}

	/*------------ debug info ------------*/

	gl_speeds.numFrames++;
	if (r_speeds->integer)
	{
		DrawTextRight (va("visLeafs: %d frLeafs: %d total: %d",
			gl_speeds.visLeafs, gl_speeds.frustLeafs, gl_speeds.leafs), RGB(1,0.5,0));
		DrawTextRight (va("surfs: %d culled: %d",
			gl_speeds.surfs, gl_speeds.cullSurfs), RGB(1,0.5,0));
		DrawTextRight (va("tris: %d (+%d) mtex: %1.2f",
			gl_speeds.trisMT, gl_speeds.tris2D, gl_speeds.trisMT ? (float)gl_speeds.tris / gl_speeds.trisMT : 0), RGB(1,0.5,0));
		DrawTextRight (va("ents: %d fcull: %d+%d cull: %d ocull: %d",
			gl_speeds.ents, gl_speeds.cullEnts, gl_speeds.cullEntsBox, gl_speeds.cullEnts2, gl_speeds.ocullEnts), RGB(1,0.5,0));
		DrawTextRight (va("particles: %d cull: %d",
			gl_speeds.parts, gl_speeds.cullParts), RGB(1,0.5,0));
		DrawTextRight (va("dlights: %d surfs: %d verts: %d",
			gl_numDlights, gl_speeds.dlightSurfs, gl_speeds.dlightVerts), RGB(1,0.5,0));
		DrawTextRight (va("flares: %d test: %d cull: %d lgrid: %d/%d",
			gl_speeds.flares, gl_speeds.testFlares, gl_speeds.cullFlares,
			map.numLightCells, map.mapGrid[0]*map.mapGrid[1]*map.mapGrid[2]), RGB(1,0.5,0));
		DrawTextRight (va("binds: %d uploads: %2d draws: %d",
			gl_speeds.numBinds, gl_speeds.numUploads, gl_speeds.numIterators), RGB(1,0.5,0));

		/* perform clearing after displaying, so, we will see speeds of previous frame
		 * (some data will be set up in EndFrame()->BackEnd())
		 */
		gl_speeds.surfs = gl_speeds.cullSurfs = 0;
		gl_speeds.tris = gl_speeds.trisMT = gl_speeds.tris2D = 0;
		gl_speeds.numBinds = gl_speeds.numUploads = 0;
		gl_speeds.numIterators = 0;
	}

	DrawTexts ();
}


/*--------------------- Text output ---------------------*/

#define I 255
#define o 51
static unsigned colorTable[8] = {
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

static void DrawCharColor (int x, int y, int c, int color)
{
	bkDrawText_t *p;
	unsigned col;

	if (c == ' ') return;

	col = colorTable[color];

	p = (bkDrawText_t*)lastBackendCommand;
	if (p && p->type == BACKEND_TEXT &&
		p->c.rgba == col &&
		p->w == 8 && p->h == 8 &&
		p->y == y && (p->x + p->len * 8) == x)
	{
		if (GET_BACKEND_SPACE(1))
		{
			p->text[p->len++] = c;
			return;
		}
	}

	{
		PUT_BACKEND_COMMAND2(bkDrawText_t, p1, 1);
		p1->type = BACKEND_TEXT;
		p1->len = 1;
		p1->x = x;
		p1->y = y;
		p1->c.rgba = col;
		p1->w = p1->h = 8;
		p1->text[0] = c;
	}
}


static void	DrawConCharColor (int x, int y, int c, int color)
{
	if (c == ' ') return;
	DrawCharColor (x*8, y*8, c, color);
}


/*--------------------- 2D picture output ---------------------*/

static shader_t *FindPic (char *name, bool force)
{
	char	*s;
	int		flags;

	if (name[0] != '/' && name[0] != '\\')
		s = va("pics/%s.pcx", name);
	else
		s = name+1;

	flags = SHADER_ALPHA;
	if (!force) flags |= SHADER_CHECK;
	return GL_FindShader (s, flags);
}


static void DrawFill (int x, int y, int w, int h, int c)
{
	GL_DrawStretchPic (gl_identityLightShader, x, y, w, h, 0, 0, 0, 0, gl_config.tbl_8to32[c]);
}

static void DrawFill2 (int x, int y, int w, int h, float r, float g, float b, float a)
{
	color_t	c;

	c.c[0] = Q_round (r * 255);
	c.c[1] = Q_round (g * 255);
	c.c[2] = Q_round (b * 255);
	c.c[3] = Q_round (a * 255);
	GL_DrawStretchPic (gl_identityLightShader, x, y, w, h, 0, 0, 0, 0, c.rgba);
}

static void DrawTileClear (int x, int y, int w, int h, char *name)
{
	GL_DrawStretchPic (FindPic (name, true), x, y, w, h, x/64.0f, y/64.0f, (x+w)/64.0f, (y+h)/64.0f, colorTable[7]);
}


static void DrawStretchPic (int x, int y, int w, int h, char *name)
{
	GL_DrawStretchPic (FindPic (name, true), x, y, w, h, 0, 0, 1, 1, colorTable[7]);
}


static void DrawPicColor (int x, int y, char *name, int color)
{
	shader_t *sh;

	sh = FindPic (name, true);
	if (color < 0 || color > 7) color = 7;
	GL_DrawStretchPic (sh, x, y, sh->width, sh->height, 0, 0, 1, 1, colorTable[color]);
}


static void GetPicSize (int *w, int *h, char *name)
{
	shader_t *sh;

	sh = FindPic (name, false);
	if (sh)
	{
		if (w) *w = sh->width;
		if (h) *h = sh->height;
	}
	else
	{
		if (w) *w = 0;
		if (h) *h = 0;
	}
}


/*------------------- Static text output ----------------------*/

#define TEXTBUF_SIZE 65536

#define TOP_TEXT_POS 64
#define CHARSIZE_X 6
#define CHARSIZE_Y 8


typedef struct textRec_s
{
	int		x, y;
	color_t	c;
	char	*text;
	struct textRec_s *next;
} textRec_t;


static char	textbuf[TEXTBUF_SIZE];
static int	textbufPos;			// position for next record
static int	textbufCount;		// count of records in buffer (0 or greater)
static textRec_t *lastTextRec;

static int nextLeft_y = TOP_TEXT_POS;
static int nextRight_y = TOP_TEXT_POS;


static void ClearTexts (void)
{
	nextLeft_y = nextRight_y = TOP_TEXT_POS;
	textbufCount = 0;
}


static void DrawTexts (void)
{
	textRec_t *rec;

	nextLeft_y = nextRight_y = TOP_TEXT_POS;
	if (!textbufCount) return;

	for (rec = (textRec_t*)textbuf; rec; rec = rec->next)
	{
		int		len;

		len = strlen (rec->text);
		if (!len) continue;

		if (gl_logTexts->integer)
			Com_Printf ("^5%s\n", rec->text);

		{
			PUT_BACKEND_COMMAND2(bkDrawText_t, p, len);
			p->type = BACKEND_TEXT;
			p->len = len;
			p->x = rec->x;
			p->y = rec->y;
			p->c.rgba = rec->c.rgba;
			p->w = CHARSIZE_X;
			p->h = CHARSIZE_Y;
			memcpy (p->text, rec->text, len);	// not ASCIIZ
		}
	}
	if (gl_logTexts->integer == 2)				// special value: log only 1 frame
		Cvar_SetInteger ("gl_logTexts", 0);

	textbufCount = 0;
}


static void DrawTextPos (int x, int y, char *text, unsigned rgba)
{
	int size;
	char *textCopy;
	textRec_t *rec;

	if (!text || !*text) return;	// empty text
	if (!textbufCount)
	{	// 1st record - perform initialization
		lastTextRec = NULL;
		textbufPos = 0;
		rec = (textRec_t*) textbuf;
	}
	else
		rec = (textRec_t*) &textbuf[textbufPos];

	size = sizeof(textRec_t) + strlen (text) + 1;
	if (size + textbufPos > TEXTBUF_SIZE) return;	// out of buffer space

	textCopy = (char*)rec + sizeof(textRec_t);
	strcpy (textCopy, text);
	rec->x = x;
	rec->y = y;
	rec->text = textCopy;
	rec->c.rgba = rgba;
	rec->next = NULL;

	if (lastTextRec) lastTextRec->next = rec;
	lastTextRec = rec;
	textbufPos += size;
	textbufCount++;
}


void DrawTextLeft (char *text, unsigned rgba)
{
	if (nextLeft_y >= vid.height) return;	// out of screen
	DrawTextPos (0, nextLeft_y, text, rgba);
	nextLeft_y += CHARSIZE_Y;
}


void DrawTextRight (char *text, unsigned rgba)
{
	if (nextRight_y >= vid.height) return;	// out of screen
	DrawTextPos (vid.width - strlen(text) * CHARSIZE_X, nextRight_y, text, rgba);
	nextRight_y += CHARSIZE_Y;
}


void DrawText3D (vec3_t pos, char *text, unsigned rgba)
{
	int		coords[2];

	if (!ProjectToScreen (pos, coords)) return;

	do
	{
		char	buf[256], *next;
		int		len;

		next = strchr (text, '\n');
		if (next)
		{
			len = next - text + 1;
			if (len > sizeof(buf)) len = sizeof(buf);
			Q_strncpyz (buf, text, len);
			text = buf;
			next++;		// skip '\n'
		}
		DrawTextPos (coords[0], coords[1], text, rgba);
		text = next;
		coords[1] += CHARSIZE_Y;
	} while (text);
}


/*------------------- Model registration ----------------------*/

static void BeginRegistration (char *mapname)
{
	GL_ResetShaders ();				// delete all shaders and re-create auto-shaders
	GL_LoadWorldMap (va("maps/%s.bsp", mapname));
	GL_ResetModels();				// delete all models and create inline models
	gl_refdef.viewCluster = -2;
}


static void EndRegistration (void)
{
	//?? empty
}


static shader_t *RegisterPic (char *name)
{
	return FindPic (name, false);
}

// Reload shader image
static void ReloadImage (char *name)
{
	shader_t *sh;

	sh = FindPic (name, false);
	// NOTE: this function will not work with shaders, whose names are differs from image name
	if (sh) GL_FindImage (sh->name, IMAGE_RELOAD);
}


static void SetSky (char *name, float rotate, vec3_t axis)
{
	int		i;
	shader_t *shader, *old;
	surfaceCommon_t *surf;
	char	name2[MAX_OSPATH] = "env/";

	old = gl_skyShader;
	// find sky shader
	strcpy (&name2[4], name);
	shader = GL_FindShader (name2, SHADER_SKY);
	if (shader->type != SHADERTYPE_SKY)
	{
		Com_WPrintf ("%s is not a sky shader\n", name2);
		shader = gl_skyShader;
	}

	if (shader != gl_defaultSkyShader)
	{
		shader->skyRotate = rotate;
		VectorCopy (axis, shader->skyAxis);
	}

	if (shader == old)
		return;		// sky is not changed

	gl_skyShader = shader;
	// change all sky surfaces
	for (i = 0, surf = map.faces; i < map.numFaces; i++, surf++)
		if (surf->shader == old) surf->shader = shader;
}

static void Screenshot (int flags, char *name)
{
	static char shotName[MAX_QPATH];

	strcpy (shotName, name);
	gl_screenshotFlags = flags;
	gl_screenshotName = shotName;
}


static float GetClientLight (void)
{
	return 150;		//!! need to implement
}


/*--------------------- GetRefAPI ---------------------*/

refExport_t GetRefAPI (refImport_t rimp)
{
	refExport_t	re;

	ri = rimp;
	gl_renderingEnabled = true;

#ifndef REF_HARD_LINKED
	if (ri.struc_size != sizeof(refImport_t) || ri.api_version != API_VERSION)
	{
		re.struc_size = 0;
		re.api_version = 0;
		return re;
	}
#endif

	memset (&gl_config, 0, sizeof(gl_config));
	memset (&gl_state, 0, sizeof(gl_state));

	gl_config.consoleOnly = Cvar_Get ("gl_console_only", "0", 0)->integer;

	re.struc_size = sizeof(re);
	re.api_version = API_VERSION;
	re.flags = &ref_flags;
	ref_flags = REF_NEW_FX;
	if (gl_config.consoleOnly)
		ref_flags |= REF_CONSOLE_ONLY;

	re.Init =			GL_Init;
	re.Shutdown =		GL_Shutdown;
	re.BeginFrame =		GL_BeginFrame;
	re.EndFrame =		GL_EndFrame;
	re.AppActivate =	GLimp_AppActivate;
	re.DrawConCharColor = DrawConCharColor;
	re.Screenshot =		Screenshot;

	re.RenderFrame =	GL_RenderFrame;
	re.BeginRegistration = BeginRegistration;
	re.RegisterModel =	GL_FindModel;
	re.RegisterSkin =	(image_t* (*)(char *)) GL_FindSkin;
	re.RegisterPic =	(image_t* (*)(char *)) RegisterPic;
	re.SetSky =			SetSky;
	re.EndRegistration = EndRegistration;

	re.ReloadImage =    ReloadImage;
	re.DrawGetPicSize =	GetPicSize;
	re.DrawPicColor =	DrawPicColor;
	re.DrawStretchPic =	DrawStretchPic;
	re.DrawCharColor =	DrawCharColor;
	re.DrawTileClear =	DrawTileClear;
	re.DrawFill =		DrawFill;
	re.DrawFill2 =		DrawFill2;

	re.DrawStretchRaw =	GL_DrawStretchRaw;
	re.SetRawPalette = GL_SetRawPalette;

	re.DrawTextPos =	DrawTextPos;
	re.DrawTextLeft =	DrawTextLeft;
	re.DrawTextRight =	DrawTextRight;

	re.GetClientLight = GetClientLight;

	Swap_Init ();

	return re;
}
