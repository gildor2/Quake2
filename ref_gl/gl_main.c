#include "gl_local.h"
#include "gl_shader.h"
#include "gl_backend.h"
#include "gl_model.h"

glconfig_t	gl_config;
glstate_t	gl_state;
glrefdef_t	gl_refdef;

viewPortal_t	vp;

drawSpeeds_t gl_speeds;

refImport_t	ri;

//??void GL_Strings_f (void); --> gfxInfo

//------------- Cvars -----------------
cvar_t	*gl_console_only;

// initialization/GLimp
cvar_t	*r_fullscreen;
cvar_t	*vid_ref;
cvar_t	*gl_mode;
cvar_t	*gl_bitdepth;
cvar_t	*gl_driver;
// GLimp
cvar_t	*gl_allow_software;
cvar_t	*gl_drawbuffer;

// image-specific
cvar_t	*gl_nobind;
cvar_t	*r_ignorehwgamma;	// make "gl_" ??
cvar_t	*gl_overBrightBits;
cvar_t	*r_gamma;			// ...
cvar_t	*r_saturation;
cvar_t	*r_intensity;
cvar_t	*gl_texturemode;
cvar_t	*gl_picmip;
cvar_t	*gl_roundImagesDown;
cvar_t	*gl_textureBits;

// rendering speed/quality settings
cvar_t	*gl_fastsky;		// do not draw skybox
cvar_t	*gl_dynamic;		// enable dynamic lightmaps for Q2/HL maps
cvar_t	*gl_vertexLight;	// use glColor() against lightmaps
cvar_t	*gl_ignoreFastPath;	// do not use GenericStageIterator() when possible

// renderer settings
cvar_t	*gl_clear;
cvar_t	*gl_finish;
cvar_t	*gl_znear;
cvar_t	*gl_swapinterval;

// debugging
cvar_t	*r_colorMipLevels;
cvar_t	*gl_showImages;
cvar_t	*gl_logFile;
cvar_t	*r_novis;
cvar_t	*r_nocull;
cvar_t	*gl_facePlaneCull;
cvar_t	*gl_sortAlpha;
cvar_t	*r_speeds;
cvar_t	*gl_lightmap;
cvar_t	*gl_showsky;
cvar_t	*r_drawworld;
cvar_t	*r_drawentities;
cvar_t	*gl_showbboxes;
cvar_t	*gl_showtris;
cvar_t	*gl_singleShader;


static void GL_Register (void)
{
CVAR_BEGIN(vars)
//	CVAR_VAR(r_fullbright, 0, 0),

	CVAR_VAR(gl_texturemode, GL_LINEAR_MIPMAP_NEAREST, CVAR_ARCHIVE),
	CVAR_VAR(gl_picmip, 0, CVAR_ARCHIVE),
	CVAR_VAR(gl_roundImagesDown, 1, 0),

//	CVAR_VAR(gl_shadows, 0, CVAR_ARCHIVE ),
	CVAR_VAR(r_gamma, 1, CVAR_ARCHIVE),
	CVAR_VAR(r_saturation, 1, CVAR_ARCHIVE),
	CVAR_VAR(r_intensity, 1, CVAR_ARCHIVE),
	CVAR_VAR(r_ignorehwgamma, 0, CVAR_ARCHIVE),
	CVAR_VAR(gl_overBrightBits, 1, CVAR_ARCHIVE),
	CVAR_VAR(gl_bitdepth, 0, CVAR_ARCHIVE),
	CVAR_VAR(gl_textureBits, 0, CVAR_ARCHIVE),
	//!! add gl_depthBits

	CVAR_VAR(gl_fastsky, 0, CVAR_ARCHIVE),
	CVAR_VAR(gl_dynamic, 1, 0),
	CVAR_VAR(gl_vertexLight, 0, CVAR_ARCHIVE),
	CVAR_VAR(gl_ignoreFastPath, 1, 0),						//!! use with VertexArrayRange ONLY !!

	CVAR_VAR(gl_finish, 0, CVAR_ARCHIVE),
	CVAR_VAR(gl_clear, 0, 0),
	CVAR_VAR(gl_znear, 4, 0),
	CVAR_VAR(gl_swapinterval, 0, CVAR_ARCHIVE),

	CVAR_VAR(gl_driver, opengl32, CVAR_ARCHIVE),			//?? use different gl_driver default value for Linux
	CVAR_VAR(gl_mode, 3, CVAR_ARCHIVE),

	CVAR_NULL(gl_ext_multitexture, 1, CVAR_ARCHIVE),
	CVAR_NULL(gl_ext_compiled_vertex_array, 1, CVAR_ARCHIVE),
	CVAR_NULL(gl_ext_vertex_array_range, 0, CVAR_ARCHIVE),	//?? do not works now
	CVAR_NULL(gl_ext_compressed_textures, 1, CVAR_ARCHIVE),

	CVAR_VAR(gl_nobind, 0, 0),
	CVAR_VAR(r_colorMipLevels, 0, 0),
	CVAR_VAR(gl_showImages, 0, 0),
	CVAR_VAR(gl_logFile, 0, 0),
	CVAR_VAR(r_novis, 0, 0),
	CVAR_VAR(r_nocull, 0, 0),
	CVAR_VAR(gl_facePlaneCull, 1, 0),
	CVAR_VAR(gl_sortAlpha, 0, 0),
	CVAR_VAR(r_speeds, 0, 0),
//	CVAR_VAR(gl_lockpvs, 0, 0), ????
	CVAR_VAR(gl_lightmap, 0, 0),
	CVAR_VAR(gl_showsky, 0, 0),
	CVAR_VAR(r_drawworld, 1, 0),
	CVAR_VAR(r_drawentities, 1, 0),
	CVAR_VAR(gl_showbboxes, 0, 0),
	CVAR_VAR(gl_showtris, 0, 0),
	CVAR_VAR(gl_singleShader, 0, 0),

	CVAR_VAR(r_fullscreen, 1, CVAR_ARCHIVE),
	CVAR_GET(vid_ref),

	CVAR_VAR(gl_allow_software, 0, 0),
	CVAR_VAR(gl_drawbuffer, GL_BACK, 0)
CVAR_END

	CVAR_GET_VARS(vars);

//	Cmd_AddCommand ("gl_strings", GL_Strings_f);
}

/*
==================
R_SetMode
==================
*/
qboolean GL_SetMode (void)
{
	rserr_t err;
	qboolean fullscreen;

	fullscreen = r_fullscreen->integer;

	r_fullscreen->modified = false;
	gl_mode->modified = false;

	if ((err = GLimp_SetMode (&vid.width, &vid.height, gl_mode->integer, fullscreen)) == rserr_ok)
		gl_state.prevMode = gl_mode->integer;
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
			Cvar_SetInteger ("gl_mode", gl_state.prevMode);
			gl_mode->modified = false;
			Com_WPrintf ("R_SetMode() - invalid mode\n");
		}

		// try setting it back to something safe
		if ((err = GLimp_SetMode (&vid.width, &vid.height, gl_state.prevMode, false)) != rserr_ok)
		{
			Com_WPrintf ("R_SetMode() - could not revert to safe mode\n");
			return false;
		}
	}
	return true;
}


void GL_SetDefaultState (void)
{
	qglClearDepth (1.0);
	qglCullFace (GL_FRONT);
	qglColor4f (1, 1, 1, 1);

	if (gl_config.maxActiveTextures > 1)
	{
		GL_SelectTexture (1);
		GL_TexEnv (GL_MODULATE);
		GL_TextureMode (gl_texturemode->string);
		GL_Bind (NULL);
		GL_SelectTexture (0);
	}
	qglEnable (GL_TEXTURE_2D);
	GL_TextureMode (gl_texturemode->string);
	GL_TexEnv (GL_MODULATE);		// 1st TMU always have TexEnv == MODULATE

	// set GL_State
	gl_state.currentState = GLSTATE_DEPTHWRITE|GLSTATE_NODEPTHTEST;
	qglDisable (GL_BLEND);
	qglDisable (GL_ALPHA_TEST);
	qglDepthMask (1);
	qglDisable (GL_DEPTH_TEST);
	qglDepthFunc (GL_LEQUAL);
	qglPolygonMode (GL_FRONT_AND_BACK, GL_FILL);

	GL_CullFace (CULL_NONE);

	qglEnable (GL_SCISSOR_TEST);
	qglShadeModel (GL_SMOOTH);
	qglEnableClientState (GL_VERTEX_ARRAY);
}

/*
===============
R_Init
===============
*/
static int GL_Init (void *hinstance, void *hWnd)
{
	int		err;

	Com_Printf ("------- R_Init -------\n");

	Com_Printf ("ref_gl version: "REF_VERSION"\n");

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

	// initialize OS-specific parts of OpenGL
	if (!GLimp_Init (hinstance, hWnd))
	{
		QGL_Shutdown ();
		return -1;
	}

	// set our "safe" mode
	gl_state.prevMode = 3;

	/*----- create the window and set up the context -----*/
	if (!GL_SetMode ())
	{
		QGL_Shutdown ();
        Com_WPrintf ("ref_gl::R_Init() - could not R_SetMode()\n");
		return -1;
	}

	/*----------- Initialize video config menu -----------*/
	Vid_MenuInit ();
	// reset "modified" field for some vars (for correct menu need-restart detecting)
#define RESET(var) var->modified = false;
	RESET(r_saturation);
	RESET(gl_picmip);
	RESET(gl_vertexLight);
	RESET(gl_bitdepth);
	RESET(gl_textureBits);
#undef RESET

	/*----------------- Get various GL strings ----------------*/
	Q_strncpyz (gl_config.vendor_string, qglGetString (GL_VENDOR), sizeof(gl_config.vendor_string));
	Q_strncpyz (gl_config.renderer_string, qglGetString (GL_RENDERER), sizeof(gl_config.renderer_string));
	Q_strncpyz (gl_config.version_string, qglGetString (GL_VERSION), sizeof(gl_config.version_string));
//??	Q_strncpyz (gl_config.extensions_string, qglGetString (GL_EXTENSIONS), sizeof(gl_config.extensions_string));
	Com_Printf ("GL_VENDOR: %s\n", gl_config.vendor_string);
	Com_Printf ("GL_RENDERER: %s\n", gl_config.renderer_string);
	Com_Printf ("GL_VERSION: %s\n", gl_config.version_string);
//??	Com_Printf ("GL_EXTENSIONS: %s\n", gl_config.extensions_string);

#ifdef __linux__
	Cvar_SetInteger ("gl_finish", 1);
#endif

	/*------------------ Grab extensions ----------------------*/

	QGL_InitExtensions ();

	//?? is maxActiveTextures needed?
	if (GL_SUPPORT(QGL_ARB_MULTITEXTURE))
	{
		qglGetIntegerv (GL_MAX_TEXTURE_UNITS_ARB, &gl_config.maxActiveTextures);
//			Com_Printf ("...using GL_ARB_multitexture with %d texture units\n", gl_config.maxActiveTextures);
	}
	else if (GL_SUPPORT(QGL_SGIS_MULTITEXTURE))
	{
		qglGetIntegerv (GL_MAX_TEXTURES_SGIS, &gl_config.maxActiveTextures);
	}
	else
		gl_config.maxActiveTextures = 1;

	//?? place this decision to Upload() and remove formatSolid from gl_config? (and update oldgl if needed)
	if (GL_SUPPORT(QGL_ARB_TEXTURE_COMPRESSION))
	{
		gl_config.formatSolid = GL_COMPRESSED_RGB_ARB;
		gl_config.formatAlpha = GL_COMPRESSED_RGBA_ARB;
		gl_config.formatAlpha1 = GL_COMPRESSED_RGBA_ARB;
	}
	else if (GL_SUPPORT(QGL_S3_S3TC))
	{
		gl_config.formatSolid = GL_RGB4_S3TC;
		gl_config.formatAlpha = 0;
		gl_config.formatAlpha1 = 0;
	}

	qglGetIntegerv (GL_MAX_TEXTURE_SIZE, &gl_config.maxTextureSize);

	GL_SetDefaultState();

	GL_InitImages ();
	GL_InitShaders ();
//	GL_InitSkins ();
	GL_InitModels ();
	GL_InitBackend ();

	gl_refdef.viewCluster = -2;		// force update visinfo for map:
			// -1 is "no visinfo", >= 0 -- visinfo, so, -2 is unused (not reserved)

#if 0 //!!!
	// from Q3
	GL_BackEnd ();
	GL_InitFlares (); //?
//?	GL_DrawStretchPic (0, 0, 0, 0, 0, 0, 1, 1, NULL);
#endif

	if ((err = qglGetError ()) != GL_NO_ERROR)
		Com_Printf ("^1glGetError() = 0x%8X\n", err);
	else
		Com_Printf ("------- finished R_Init -------\n");

	return 0;	// all OK
}


/*
===============
R_Shutdown
===============
*/
static void GL_Shutdown (void)
{
	GL_ShutdownBackend ();
	GL_ShutdownModels ();
	GL_ShutdownShaders ();
	GL_ShutdownImages ();

	// shut down OS specific OpenGL stuff like contexts, etc.
	GLimp_Shutdown();

	// shutdown our QGL subsystem
	QGL_Shutdown();
}


void GL_Set2DMode (void)
{
	if (gl_state.is2dMode)
		return;

	LOG_STRING ("***** Set2DMode() *****\n");
	qglViewport (0, 0, vid.width, vid.height);
	qglScissor (0, 0, vid.width, vid.height);
	qglMatrixMode (GL_PROJECTION);
	qglLoadIdentity ();
	qglOrtho (0, vid.width, vid.height, 0, 0, 1);
	qglMatrixMode (GL_MODELVIEW);
	qglLoadIdentity ();
//	GL_State (GLSTATE_SRC_SRCALPHA|GLSTATE_DST_ONEMINUSSRCALPHA|GLSTATE_NODEPTHTEST);
	GL_CullFace (CULL_NONE);
	// qglDisable (GL_CLIP_PLANE0);
	gl_state.is2dMode = true;
	//?? remember time?
}


/*---------------- Backend helpers ----------------------*/

void GL_SetColorB (byte color[4])
{
	if (*(int*)color != gl_state.currentColor)
	{
		PUT_BACKEND_COMMAND (bkSetColor_t, c);
		c->type = BACKEND_SET_COLOR;
		gl_state.currentColor = c->rgba = *(int*)color;
	}
}


void GL_SetColor (float color[4])
{
	byte c[4];

	c[0] = color[0] * 255;
	c[1] = color[1] * 255;
	c[2] = color[2] * 255;
	c[3] = color[3] * 255;
	GL_SetColorB (c);
}


void GL_DrawStretchPic (shader_t *shader, float x, float y, float w, float h, float s1, float t1, float s2, float t2)
{
	PUT_BACKEND_COMMAND (bkDrawPic_t, p);
	p->type = BACKEND_PIC;
	p->shader = shader;
	p->x = x;
	p->y = y;
	p->w = w;
	p->h = h;
	if (/*s1 == 0 && s2 == 1 &&*/ w > shader->width * 2)
	{
		s1 += 0.5 / shader->width;
		s2 -= 0.5 / shader->width;
	}
	if (/*t1 == 0 && t2 == 1 &&*/ h > shader->height * 2)
	{
		t1 += 0.5 / shader->height;
		t2 -= 0.5 / shader->height;
	}
	p->s1 = s1;
	p->t1 = t1;
	p->s2 = s2;
	p->t2 = t2;
}


/*--------------------- Frame ---------------------------*/


static void GL_BeginFrame (float camera_separation)
{
	static byte	initColor[4] = {255, 255, 255, 255};

	if (gl_logFile->modified)
	{
		GLimp_EnableLogging (gl_logFile->integer);
		gl_logFile->modified = false;
	}
	else if (gl_logFile->integer == 2)
	{
		GLimp_EnableLogging (false);
		Cvar_SetInteger ("gl_logFile", 0);
	}

//	DrawTextRight ("---------BeginFrame---------\n", 1,0,0);//!!!
	LOG_STRING ("\n---------- Begin Frame ----------\n\n");

	if (gl_finish->integer == 2) qglFinish ();
	gl_speeds.beginFrame = Sys_Milliseconds ();

	//?? what to do with the camera_separation ?

	gl_state.finished = false;
	gl_state.maxUsedShaderIndex = -1;
	gl_state.minNewShaderIndex = 65536;	// MAX_SHADERS, to be exact

	if (gl_texturemode->modified)
	{
		GL_TextureMode (gl_texturemode->string);
		gl_texturemode->modified = false;
	}

	/*----- update hardware gamma (if present) ------*/
	if (r_gamma->modified && gl_config.deviceSupportsGamma)
	{
		r_gamma->modified = false;
		GL_SetupGamma ();
	}

	/*------ change modes if necessary ---------*/
	if (gl_mode->modified || r_fullscreen->modified)
		vid_ref->modified = true;

	{
		PUT_BACKEND_COMMAND (int, tmp);
		*tmp = BACKEND_BEGIN_FRAME;
	}

	gl_state.currentColor = 0;	// force to store current color
	GL_SetColorB (initColor);
//?	GLimp_BeginFrame (camera_separation); -- do (almost in Win32, totally in Linux) nothing
}


static void GL_EndFrame (void)
{
//	DrawTextRight ("---------EndFrame---------\n", 1,0,0);//!!!
	{
		PUT_BACKEND_COMMAND (int, tmp);
		*tmp = BACKEND_END_FRAME;
	}

	GL_BackEnd ();
	GL_ClearBuffers ();

	if (gl_finish->integer == 2) qglFinish ();
	gl_speeds.endFrame = Sys_Milliseconds ();
	if (r_speeds->integer)
	{
		DrawTextRight (va("fe: %2d bk: %2d (srt: %2d 3d: %2d 2d: %2d)",
			gl_speeds.beginSort - gl_speeds.beginFrame,
			gl_speeds.endFrame - gl_speeds.beginSort,
			gl_speeds.begin3D - gl_speeds.beginSort,
			gl_speeds.begin2D - gl_speeds.begin3D,
			gl_speeds.endFrame - gl_speeds.begin2D
			), 1, 0.5, 0);
	}
}


/*----------------- Rendering ---------------------*/

static void PrepareWorldModel (void)
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
	VectorCopy (vp.vieworg, vp.modelvieworg);
	vp.modelaxis[0][0] = vp.modelaxis[1][1] = vp.modelaxis[2][2] = 1;	// orthogonal axis
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
#if 0	//!! debug:
#define m vp.vieworg
	DrawTextLeft (va("Org: %9.4g, %9.4g %9.4g", m[0], m[1], m[2]), 0.6, 1, 0.2);
#undef m
#define m vp.viewaxis
	for (i = 0; i < 3; i++)
		DrawTextLeft (va("ax[%d] = {%9.4g, %9.4g %9.4g}", i, m[i][0], m[i][1], m[i][2]), 0.4, 0.8, 1);
#undef m
#define m vp.modelMatrix
	DrawTextLeft (va("----- modelview matrix -----"), 1, 0.2, 0.2);
	for (i = 0; i < 4; i++)
		DrawTextLeft (va("{%9.4g, %9.4g, %9.4g, %9.4g}", m[0][i], m[1][i], m[2][i], m[3][i]), 0.8, 0.3, 0.3);
#undef m
#endif
}


static void SetFrustum (void)
{
	float	sx, sy, cx, cy;
	int		i;

#define SCALE	(M_PI/360.0f)
//#define SCALE	(M_PI/360.0 * 0.7)	//!! debug: smaller fov
	sx = sin (vp.fov_x * SCALE);	// fov/2 * pi/180
	sy = sin (vp.fov_y * SCALE);
	cx = cos (vp.fov_x * SCALE);
	cy = cos (vp.fov_y * SCALE);
#undef SCALE
	// calculate normals
	for (i = 0; i < 3; i++)
	{
		vp.frustum[0].normal[i] = vp.viewaxis[0][i] * sx + vp.viewaxis[1][i] * cx;
		vp.frustum[1].normal[i] = vp.viewaxis[0][i] * sx - vp.viewaxis[1][i] * cx;
		vp.frustum[2].normal[i] = vp.viewaxis[0][i] * sy + vp.viewaxis[2][i] * cy;
		vp.frustum[3].normal[i] = vp.viewaxis[0][i] * sy - vp.viewaxis[2][i] * cy;
	}
	// complete planes
	for (i = 0; i < 4; i++)
	{
		vp.frustum[i].type = PLANE_NON_AXIAL;
		vp.frustum[i].dist = DotProduct (vp.vieworg, vp.frustum[i].normal);
		SetPlaneSignbits (&vp.frustum[i]);
	}
}


// Create projection matrix as glFrustum() does
static void SetPerspective (void)
{
#define m	vp.projectionMatrix
	float	xmax, xmin, ymax, ymin, zmin, zmax, tfx, tfy;

	if (gl_refdef.flags & RDF_NOWORLDMODEL)
		vp.zFar = 2048;
	else
	{	// calculate zFar depends on visible bounding box size
		int		i;
		float	d;

		d = 0;
		for (i = 0; i < 8; i++)	// check all 8 verts of bounding box
		{
			vec3_t	v;
			float	d1;

			if (i & 1)	v[0] = vp.maxs[0];
			else		v[0] = vp.mins[0];

			if (i & 2)	v[1] = vp.maxs[1];
			else		v[1] = vp.mins[1];

			if (i & 4)	v[2] = vp.maxs[2];
			else		v[2] = vp.mins[2];

			VectorSubtract (v, vp.vieworg, v);
			d1 = DotProduct (v, v);	// square of vector length
			if (d1 > d) d = d1;
		}
		vp.zFar = sqrt (d);
	}
	//-------------
	tfx = tan (vp.fov_x * M_PI / 360.0f);
	tfy = tan (vp.fov_y * M_PI / 360.0f);
	zmin = gl_znear->value;
	zmax = vp.zFar;
	xmin = -zmin * tfx;
	xmax = zmin * tfx;
	ymin = -zmin * tfy;
	ymax = zmin * tfy;
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
#if 0	//!! debug
#define dr DrawTextRight
	dr (va("zFar: %g;  frustum: x[%g, %g] y[%g, %g]", vp.zFar, xmin, xmax, ymin, ymax), 1, 0.1, 0.2);
#define m vp.projectionMatrix
	dr (va("----- projection matrix -----"), 1, 0.2, 0.2);
	dr (va("{%9.4g, %9.4g, %9.4g, %9.4g}", m[0][0], m[1][0], m[2][0], m[3][0]), 0.8, 0.3, 0.3);
	dr (va("{%9.4g, %9.4g, %9.4g, %9.4g}", m[0][1], m[1][1], m[2][1], m[3][1]), 0.8, 0.3, 0.3);
	dr (va("{%9.4g, %9.4g, %9.4g, %9.4g}", m[0][2], m[1][2], m[2][2], m[3][2]), 0.8, 0.3, 0.3);
	dr (va("{%9.4g, %9.4g, %9.4g, %9.4g}", m[0][3], m[1][3], m[2][3], m[3][3]), 0.8, 0.3, 0.3);
#undef m
#undef dr
#endif
}


void DrawTexts (void);

// Called from GL_RenderFrame() only
static void RenderView (void)
{
	// setup viewPortal structure
	memset (&vp, 0, sizeof(vp));
	vp.x = gl_refdef.x;
	vp.y = vid.height - (gl_refdef.y + gl_refdef.height);
	vp.w = gl_refdef.width;
	vp.h = gl_refdef.height;
	vp.fov_x = gl_refdef.fov_x;
	vp.fov_y = gl_refdef.fov_y;

	vp.lightStyles = gl_refdef.lightStyles;
	vp.time = gl_refdef.time;

	VectorCopy (gl_refdef.vieworg, vp.vieworg);
	vp.vieworg[0] = gl_refdef.vieworg[0];
	vp.vieworg[1] = gl_refdef.vieworg[1];
	vp.vieworg[2] = gl_refdef.vieworg[2];
	AxisCopy (gl_refdef.viewaxis, vp.viewaxis);
	GL_ClearPortal ();
	PrepareWorldModel ();
	SetFrustum ();
	GL_DrawWorld ();
	SetPerspective ();
	//?? GL_AddGameSurfacesToBuffer ();
	GL_DrawEntities (gl_refdef.firstEntity, gl_refdef.numEntities);
	//?? draw portals here (if buffer has shader.sort == SORT_PORTAL)
	GL_FinishPortal ();
	DrawTexts ();
}


// Can be called few RenderFrame() between BeginFrame() and EndFrame()
static void GL_RenderFrame (refdef_t *fd)
{
	int		i;
	entity_t	*ent;

	if (!(fd->rdflags & RDF_NOWORLDMODEL) && !gl_worldModel.name)
		Com_Error (ERR_FATAL, "R_RenderFrame: NULL worldModel");

	gl_refdef.lightStyles = fd->lightstyles;

//	DrawTextRight ("---------RenderFrame---------\n", 1,0,0);//!!!

	/*----------- prepare data ------------*/

	gl_refdef.flags = fd->rdflags;
	// copy screen params
	gl_refdef.x = fd->x;
	gl_refdef.y = fd->y;
	gl_refdef.width = fd->width;
	gl_refdef.height = fd->height;
	gl_refdef.fov_x = fd->fov_x;
	gl_refdef.fov_y = fd->fov_y;
	// setup vieworg
	VectorCopy (fd->vieworg, gl_refdef.vieworg);
	AnglesToAxis (fd->viewangles, gl_refdef.viewaxis);
	// setup time
	gl_refdef.time = fd->time;

	if (!(gl_refdef.flags & RDF_NOWORLDMODEL))
	{
		int		areaSum;
		byte	*areas, floodArea[MAX_MAP_AREAS/8];

		// check for areabits changes
		areaSum = 0;
		areas = fd->areabits;
		if (!areas)
		{
			memset (floodArea, 0xFF, sizeof(floodArea));
			areas = &floodArea[0];
		}
		for (i = 0; i < MAX_MAP_AREAS/8; i++)
		{
			byte	c;

			c = *areas++;
			areaSum |= c ^ gl_refdef.areaMask[i];
			gl_refdef.areaMask[i] = c;
		}
		gl_refdef.areaMaskChanged = areaSum;
	}

	/*----------- add entities -----------*/

	gl_speeds.ents = gl_speeds.cullEnts = 0;
	gl_refdef.firstEntity = gl_numEntities;
	gl_refdef.numEntities = fd->num_entities;
	for (i = 0, ent = fd->entities; i < fd->num_entities; i++, ent++)
		GL_AddEntity (ent);

	/*------------ rendering -------------*/

	RenderView ();

	if (r_speeds->integer)
	{
		DrawTextRight (va("visLeafs: %d frLeafs: %d total: %d",
			gl_speeds.visLeafs, gl_speeds.frustLeafs, gl_speeds.leafs), 1, 0.5, 0);
		DrawTextRight (va("surfs: %d culled: %d",
			gl_speeds.surfs, gl_speeds.cullSurfs), 1, 0.5, 0);
		DrawTextRight (va("ents: %d culled: %d",
			gl_speeds.ents, gl_speeds.cullEnts), 1, 0.5, 0);
		DrawTextRight (va("binds: %d uploads: %2d draws: %d",
			gl_speeds.numBinds, gl_speeds.numUploads, gl_speeds.numIterators), 1, 0.5, 0);

		/* perform clearing after displaying, so, we will see speeds of previous frame
		 * (some data will be set up in EndFrame()->BackEnd())
		 */
		gl_speeds.surfs = 0;
		gl_speeds.cullSurfs = 0;
		gl_speeds.numBinds = 0;
		gl_speeds.numUploads = 0;
		gl_speeds.numIterators = 0;
	}
}


/*-------- Dummy functions for console-only mode ---------*/

static void	D_RenderFrame (refdef_t *fd) {}
static void	D_BeginRegistration (char *map) {}
static struct model_s *D_RegisterModel (char *name) { return NULL; }
static struct image_s *D_RegisterSkin (char *name) { return NULL; }
static struct image_s *D_FindPic (char *name) { return NULL; }
static void D_SetSky (char *name, float rotate, vec3_t axis) {}
static void	D_EndRegistration (void) {}

static void	D_Draw_GetPicSize (int *w, int *h, char *pic)
{
	if (w) *w = 0;
	if (h) *h = 0;
}

static void	D_Draw_PicColor (int x, int y, char *name, int color) {}
static void	D_Draw_StretchPic (int x, int y, int w, int h, char *pic) {}
static void	D_Draw_CharColor (int x, int y, int c, int color) {}
static void	D_Draw_TileClear (int x, int y, int w, int h, char *name) {}
static void	D_Draw_Fill (int x, int y, int w, int h, int c) {}
static void	D_Draw_FadeScreen (void) {}
static void	D_DrawTextPos (int x, int y, char *text, float r, float g, float b) {}
static void	D_DrawTextSide (char *text, float r, float g, float b) {}
static void	D_Draw_StretchRaw (int x, int y, int w, int h, int cols, int rows, byte *data) {}
static void	D_SetPalette (const unsigned char *palette) {}


/*--------------------- Text output ---------------------*/

#define I 255
#define o 51
static byte colorTable[8][4] = {
	{0, 0, 0, 255},
	{I, o, o, 255},
	{o, I, o, 255},
	{I, I, o, 255},
	{o, o, I, 255},
	{I, o, I, 255},
	{o, I, I, 255},
	{I, I, I, 255}
};
#undef I
#undef o

static void DrawCharColor (int x, int y, int c, int color)
{
	float	s1, t1, size;

	if (c == ' ') return;
	size = 1.0f / 16.0f;

	s1 = size * (c & 15);
	t1 = size * (c >> 4 & 15);
	if (color < 0 || color > 7) color = 7;
	GL_SetColorB (colorTable[color]);
	GL_DrawStretchPic (gl_concharsShader, x, y, 8, 8, s1, t1, s1+size, t1+size);
}


static void	DrawConCharColor (int x, int y, int c, int color)
{
	if (c == ' ') return;
	DrawCharColor (x*8, y*8, c, color);
}


/*--------------------- 2D picture output ---------------------*/

shader_t *FindPic (char *name, qboolean force)
{
	char	fullname[MAX_QPATH], *s;
	int		flags;

	if (name[0] != '/' && name[0] != '\\')
	{
		Com_sprintf (fullname, sizeof(fullname), "pics/%s.pcx", name);
		s = fullname;
	}
	else
		s = name+1;

	flags = SHADER_ALPHA;
	if (!force)
		flags = SHADER_CHECK;
	return GL_FindShader (s, flags);
}


static void DrawFill (int x, int y, int w, int h, int c)
{
	GL_SetColorB ((byte *)&gl_config.tbl_8to32[c]);
	GL_DrawStretchPic (gl_identityLightShader, x, y, w, h, 0, 0, 0, 0);
}


static void FadeScreen (void)
{
	static byte fadeColor[4] = {0, 0, 0, 224};

	GL_SetColorB (fadeColor);
	GL_DrawStretchPic (gl_identityLightShader, 0, 0, vid.width, vid.height, 0, 0, 0, 0);
}


static void DrawTileClear (int x, int y, int w, int h, char *name)
{
	GL_SetColorB (colorTable[7]);	// white
	GL_DrawStretchPic (FindPic (name, true), x, y, w, h, x/64.0, y/64.0, (x+w)/64.0, (y+h)/64.0);
}


static void DrawStretchPic (int x, int y, int w, int h, char *name)
{
	GL_SetColorB (colorTable[7]);	// white
	GL_DrawStretchPic (FindPic (name, true), x, y, w, h, 0, 0, 1, 1);
}


static void DrawPicColor (int x, int y, char *name, int color)
{
	shader_t *sh;

	sh = FindPic (name, true);
	if (color < 0 || color > 7) color = 7;
	GL_SetColorB (colorTable[color]);
	GL_DrawStretchPic (sh, x, y, sh->width, sh->height, 0, 0, 1, 1);
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
	union {
		byte	c[4];
		int		ci;
	};
	char	*text;
	struct textRec_s *next;
} textRec_t;


static char	textbuf[TEXTBUF_SIZE];
static int	textbufPos;			// position for next record
static int	textbufCount;		// count of records in buffer (0 or greater)
static textRec_t *lastTextRec;

static int nextLeft_y = TOP_TEXT_POS;
static int nextRight_y = TOP_TEXT_POS;


static void DrawColoredText (int x, int y, char *text, byte *color)
{
	char	c;
	float	size;

	if (!text || !*text) return; // nothing to out

	size = 1.0f / 16.0f;

	while ((c = *text++) && x < vid.width)
	{
		if (c != ' ')
		{
			float	s1, t1;

			s1 = size * (c & 15);
			t1 = size * (c >> 4 & 15);

			GL_SetColorB (color);
			GL_DrawStretchPic (gl_concharsShader, x, y, CHARSIZE_X, CHARSIZE_Y, s1, t1, s1+size, t1+size);
		}
		x += CHARSIZE_X;
	}
}


static void DrawTexts (void)
{
	textRec_t *rec;

	nextLeft_y = nextRight_y = TOP_TEXT_POS;
	if (!textbufCount) return;

	rec = (textRec_t*)textbuf;
	while (rec)
	{
		DrawColoredText (rec->x, rec->y, rec->text, rec->c);
		rec = rec->next;
	}

	textbufCount = 0;
}


void DrawTextPos (int x, int y, char *text, float r, float g, float b)
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
	rec->c[0] = r * 255;
	rec->c[1] = g * 255;
	rec->c[2] = b * 255;
	rec->c[3] = 255;
	rec->next = NULL;

	if (lastTextRec) lastTextRec->next = rec;
	lastTextRec = rec;
	textbufPos += size;
	textbufCount++;
}


void DrawTextLeft (char *text, float r, float g, float b)
{
	if (nextLeft_y >= vid.height) return;	// out of screen
	DrawTextPos (0, nextLeft_y, text, r, g, b);
	nextLeft_y += CHARSIZE_Y;
}


void DrawTextRight (char *text, float r, float g, float b)
{
	if (nextRight_y >= vid.height) return;	// out of screen
	DrawTextPos (vid.width - strlen(text) * CHARSIZE_X, nextRight_y, text, r, g, b);
	nextRight_y += CHARSIZE_Y;
}


/*------------------- Model registration ----------------------*/

static void BeginRegistration (char *mapname)
{
	char name2[MAX_QPATH];

	GL_ResetShaders ();				// delete all shaders and re-create auto-shaders
	Com_sprintf (name2, sizeof(name2), "maps/%s.bsp", mapname);
	GL_LoadWorldMap (name2);
	GL_ResetModels();				// delete all models and create inline models
	gl_refdef.viewCluster = -2;
}


static shader_t *RegisterPic (char *name)
{
	return FindPic (name, false);
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
	for (i = 0, surf = gl_worldModel.faces; i < gl_worldModel.numFaces; i++, surf++)
		if (surf->shader == old) surf->shader = shader;
}


/*--------------------- GetRefAPI ---------------------*/

refExport_t GetRefAPI (refImport_t rimp)
{
	refExport_t	re;

	ri = rimp;

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

	gl_console_only = Cvar_Get ("gl_console_only", "0", 0);
	gl_config.consoleOnly = gl_console_only->integer;

	re.struc_size = sizeof(re);
	re.api_version = API_VERSION;
	re.console_only = gl_config.consoleOnly;

	re.Init =			GL_Init;
	re.Shutdown =		GL_Shutdown;
	re.BeginFrame =		GL_BeginFrame;
	re.EndFrame =		GL_EndFrame;
	re.AppActivate =	GLimp_AppActivate;
	re.DrawConCharColor = DrawConCharColor;

	if (!gl_config.consoleOnly)
	{
		re.RenderFrame =	GL_RenderFrame;
		re.BeginRegistration = BeginRegistration;
		re.RegisterModel =	GL_FindModel;
		re.RegisterSkin =	(image_t* (*)(char *)) GL_FindSkin;
		re.RegisterPic =	(image_t* (*)(char *)) RegisterPic;
		re.SetSky =			SetSky;
		re.EndRegistration = D_EndRegistration;	//GL_EndRegistration;

		re.DrawGetPicSize =	GetPicSize;
		re.DrawPicColor =	DrawPicColor;
		re.DrawStretchPic =	DrawStretchPic;
		re.DrawCharColor =	DrawCharColor;
		re.DrawTileClear =	DrawTileClear;
		re.DrawFill =		DrawFill;
		re.DrawFadeScreen =	FadeScreen;

		re.DrawStretchRaw =	GL_DrawStretchRaw;
		re.CinematicSetPalette = GL_SetRawPalette;

		re.DrawTextPos =	DrawTextPos;
		re.DrawTextLeft =	DrawTextLeft;
		re.DrawTextRight =	DrawTextRight;
	}
	else
	{
		re.RenderFrame =	D_RenderFrame;
		re.BeginRegistration = D_BeginRegistration;
		re.RegisterModel =	D_RegisterModel;
		re.RegisterSkin =	D_RegisterSkin;
		re.RegisterPic =	D_FindPic;
		re.SetSky =			D_SetSky;
		re.EndRegistration = D_EndRegistration;

		re.DrawGetPicSize =	D_Draw_GetPicSize;
		re.DrawPicColor =	D_Draw_PicColor;
		re.DrawStretchPic =	D_Draw_StretchPic;
		re.DrawCharColor =	D_Draw_CharColor;
		re.DrawTileClear =	D_Draw_TileClear;
		re.DrawFill =		D_Draw_Fill;
		re.DrawFadeScreen =	D_Draw_FadeScreen;

		re.DrawStretchRaw =	D_Draw_StretchRaw;
		re.CinematicSetPalette = D_SetPalette;

		re.DrawTextPos =	D_DrawTextPos;
		re.DrawTextLeft =	D_DrawTextSide;
		re.DrawTextRight =	D_DrawTextSide;
	}

	Swap_Init ();

	return re;
}
