#include "gl_local.h"


glconfig_t	gl_config;
glstate_t	gl_state;


/*--------------- Lock/unlock -----------------*/


void GL_Lock (void)
{
	if (gl_state.locked) return;

#define COPY(f1, f2)	memcpy (gl_state.f1, gl_state.f2, sizeof(gl_state.f1))
	COPY(newBinds, currentBinds);
	COPY(newEnv, currentEnv);
	COPY(newTexCoordEnabled, texCoordEnabled);
	COPY(newTextureEnabled, textureEnabled);
	COPY(newEnvColor, texEnvColor);
	memset (gl_state.newTCPointer, 0, sizeof(gl_state.newTCPointer));
#undef COPY

	gl_state.locked = true;
}


void GL_Unlock (void)
{
	int		startTmu, tmu, i, n;
	qboolean f;
	color_t	c;

	if (!gl_state.locked) return;
	gl_state.locked = false;

	startTmu = gl_state.currentTmu;
	for (i = -1; i < gl_config.maxActiveTextures; i++)
	{
		if (i == -1)
			tmu = startTmu;		// process current TMU first (iteration "-1")
		else if (i == startTmu)
			continue;			// and skip it later
		else
			tmu = i;

		if ((n = gl_state.newBinds[tmu]) != gl_state.currentBinds[tmu])
		{
			GL_SelectTexture (tmu);
			gl_state.currentBinds[tmu] = n;
			qglBindTexture (GL_TEXTURE_2D, n);
			gl_speeds.numBinds++;
		}
		if ((n = gl_state.newEnv[tmu]) != gl_state.currentEnv[tmu])
		{
			GL_SelectTexture (tmu);
			GL_TexEnv (n);
		}
		if ((f = gl_state.newTexCoordEnabled[tmu]) != gl_state.texCoordEnabled[tmu])
		{
			GL_SelectTexture (tmu);
			gl_state.texCoordEnabled[tmu] = f;
			if (f)	qglEnableClientState (GL_TEXTURE_COORD_ARRAY);
			else	qglDisableClientState (GL_TEXTURE_COORD_ARRAY);
		}
		if ((f = gl_state.newTextureEnabled[tmu]) != gl_state.textureEnabled[tmu])
		{
			GL_SelectTexture (tmu);
			gl_state.textureEnabled[tmu] = f;
			if (f)	qglEnable (GL_TEXTURE_2D);
			else	qglDisable (GL_TEXTURE_2D);
		}
		if (gl_state.newTCPointer[tmu])
		{
			GL_SelectTexture (tmu);
			qglTexCoordPointer (2, GL_FLOAT, 0, gl_state.newTCPointer[tmu]);
		}
		if (gl_state.newEnv[tmu] & TEXENV_COLOR && (c.rgba = gl_state.newEnvColor[tmu].rgba) != gl_state.texEnvColor[tmu].rgba)
		{
			GL_SelectTexture (tmu);
			GL_TexEnvColor (&c);
		}
	}
}


/*------------------ Bind ---------------------*/


void GL_Bind (image_t *tex)
{
	int		tmu;

	if (gl_state.locked)
	{
		tmu = gl_state.newTmu;
		if (tex)
		{
			gl_state.newBinds[tmu] = tex->texnum;
			gl_state.newTextureEnabled[tmu] = true;
		}
		else
			gl_state.newTextureEnabled[tmu] = false;
		return;
	}

	tmu = gl_state.currentTmu;

	if (!tex)
	{
		if (gl_state.textureEnabled[tmu])
		{
			qglDisable (GL_TEXTURE_2D);
			gl_state.textureEnabled[tmu] = false;
		}
	}
	else
	{
		int		h, h1;

		if (!gl_state.textureEnabled[tmu])
		{
			qglEnable (GL_TEXTURE_2D);
			gl_state.textureEnabled[tmu] = true;
		}

		h = tex->texnum;
		if (gl_nobind->integer && (h1 = gl_dlightImage->texnum))
		{
			if (gl_nobind->integer != 2 || strcmp (tex->name, "pics/conchars.pcx"))	//?? need better way to detect force-bind images
				h = h1;
		}

		if (h == gl_state.currentBinds[tmu]) return;

		qglBindTexture (GL_TEXTURE_2D, h);
		gl_speeds.numBinds++;
		gl_state.currentBinds[tmu] = h;
	}
}


// Bind image even if nobind active (i.e. for uploading image)
void GL_BindForce (image_t *tex)
{
	int		tmu, h;

	tmu = gl_state.currentTmu;

	if (!gl_state.textureEnabled[tmu])
	{
		qglEnable (GL_TEXTURE_2D);
		gl_state.textureEnabled[tmu] = true;
	}

	h = tex->texnum;
	if (gl_state.currentBinds[tmu] == h) return;

	qglBindTexture (GL_TEXTURE_2D, h);
	gl_speeds.numBinds++;
	gl_state.currentBinds[tmu] = h;
}


/*---------------- Multitexturing -------------*/


void GL_TexEnv (int env)
{
	int		tmu, mode;

	if (gl_state.locked)
	{
		gl_state.newEnv[gl_state.newTmu] = env;
		return;
	}

	tmu = gl_state.currentTmu;
	if (gl_state.currentEnv[tmu] == env)
		return;

	gl_state.currentEnv[tmu] = env;

	switch (env & TEXENV_FUNC_MASK)
	{
	case TEXENV_MODULATE:
		mode = GL_MODULATE;
		break;
	case TEXENV_ADD:
		mode = GL_ADD;
		break;
	case TEXENV_INTERP:
		mode = GL_INTERPOLATE_ARB;
		break;
	//!! other
	default:
		mode = GL_REPLACE;
	}

	if (!(env & ~TEXENV_FUNC_MASK))	//?? need another way to check base multitexturing
	{
		qglTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, mode);
		return;
	}

	//!! add code for combine4_nv

	qglTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);
	// combine mode
	qglTexEnvi (GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, mode);
	qglTexEnvi (GL_TEXTURE_ENV, GL_COMBINE_ALPHA_ARB, mode);
	// source 0
	qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_PREVIOUS_ARB);
	qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
	qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_ARB, GL_PREVIOUS_ARB);
	qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND0_ALPHA_ARB, GL_SRC_ALPHA);
	// source 1
	qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_TEXTURE);
	qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR);
	qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_ARB, GL_TEXTURE);
	qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND1_ALPHA_ARB, GL_SRC_ALPHA);
	// source 2
	if (mode == GL_INTERPOLATE_ARB)
	{
		qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE2_RGB_ARB, GL_CONSTANT_ARB);
		qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND2_RGB_ARB, GL_SRC_COLOR);
		qglTexEnvi (GL_TEXTURE_ENV, GL_SOURCE2_ALPHA_ARB, GL_CONSTANT_ARB);
		qglTexEnvi (GL_TEXTURE_ENV, GL_OPERAND2_ALPHA_ARB, GL_SRC_ALPHA);
	}

	qglTexEnvf (GL_TEXTURE_ENV, GL_RGB_SCALE_ARB, (env & TEXENV_MUL2 ? 2 : 1));
}


void GL_TexEnvColor (color_t *c)
{
	int		tmu;
	float	color[4];

	if (gl_state.locked)
	{
		tmu = gl_state.newTmu;
		if (gl_state.newEnv[tmu] & TEXENV_COLOR)
			gl_state.newEnvColor[gl_state.newTmu].rgba = c->rgba;
		return;
	}

	tmu = gl_state.currentTmu;
	if (!(gl_state.newEnv[tmu] & TEXENV_COLOR) || gl_state.texEnvColor[tmu].rgba == c->rgba)
		return;

	gl_state.texEnvColor[tmu].rgba = c->rgba;
	color[0] = c->c[0] / 255.0f;
	color[1] = c->c[1] / 255.0f;
	color[2] = c->c[2] / 255.0f;
	color[3] = c->c[3] / 255.0f;
	qglTexEnvfv (GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, color);
}


void GL_SelectTexture (int tmu)
{
	int		tex;

	if (gl_state.locked)
	{
		gl_state.newTmu = tmu;
		return;
	}

	if (tmu == gl_state.currentTmu)
		return;

	if (GL_SUPPORT(QGL_ARB_MULTITEXTURE))
	{
		// ARB_multitexture
		tex = GL_TEXTURE0_ARB + tmu;
		qglActiveTextureARB (tex);
		qglClientActiveTextureARB (tex);
	}
	else
	{
		// SGIS_multitexture
		tex = GL_TEXTURE0_SGIS + tmu;
		qglSelectTextureSGIS (tex);
		qglSelectTextureCoordSetSGIS (tex);
	}

	gl_state.currentTmu = tmu;
}


void GL_TexCoordPointer (void *ptr)
{
	if (gl_state.locked)
	{
		gl_state.newTCPointer[gl_state.newTmu] = ptr;
		return;
	}

	qglTexCoordPointer (2, GL_FLOAT, 0, ptr);
}


void GL_SetMultitexture (int level)
{
	int		i;

	if (level > gl_config.maxActiveTextures)
		Com_Error (ERR_FATAL, "R_SetMultitexture(%d > %d)", level, gl_config.maxActiveTextures);

	if (gl_state.locked)
	{
		for (i = 0; i < gl_config.maxActiveTextures; i++)
			if (i < level)
			{
				gl_state.newTexCoordEnabled[i] = true;
			}
			else
			{
				gl_state.newTexCoordEnabled[i] = false;
				gl_state.newTextureEnabled[i] = false;
			}
		return;
	}

	for (i = level; i < gl_config.maxActiveTextures; i++)
		if (gl_state.textureEnabled[i] || gl_state.texCoordEnabled[i])
		{
			GL_SelectTexture (i);
			GL_Bind (NULL);
			if (gl_state.texCoordEnabled[i])
			{
				qglDisableClientState (GL_TEXTURE_COORD_ARRAY);
				gl_state.texCoordEnabled[i] = false;
			}
		}
	for (i = level - 1; i >= 0; i--)
		if (!gl_state.texCoordEnabled[i])
		{
			GL_SelectTexture (i);
			qglEnableClientState (GL_TEXTURE_COORD_ARRAY);
			gl_state.texCoordEnabled[i] = true;
		}
	GL_SelectTexture (0);
}


void GL_DisableTexCoordArrays (void)
{
	int		i;

	if (gl_state.locked)
	{
		memset (gl_state.newTexCoordEnabled, 0, sizeof(gl_state.newTexCoordEnabled));
		return;
	}

	for (i = 0; i < gl_config.maxActiveTextures; i++)
		if (gl_state.texCoordEnabled[i])
		{
			GL_SelectTexture (i);
			qglDisableClientState (GL_TEXTURE_COORD_ARRAY);
			gl_state.texCoordEnabled[i] = false;
		}
}


/*--------------- Miscellaneous ---------------*/


void GL_CullFace (gl_cullMode_t mode)
{
	if (gl_state.currentCullMode == mode)
		return;

	if (mode == CULL_NONE)
		qglDisable (GL_CULL_FACE);
	else
	{
		qglEnable (GL_CULL_FACE);
		qglCullFace (mode == CULL_FRONT ? GL_FRONT : GL_BACK);
	}

	gl_state.currentCullMode = mode;
}


void GL_State (int state)
{
	int		dif;

	static int blends[] = {
		GL_ZERO, GL_ONE,
		GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR,
		GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
		GL_DST_COLOR, GL_ONE_MINUS_DST_COLOR,
		GL_DST_ALPHA, GL_ONE_MINUS_DST_ALPHA,
		GL_SRC_ALPHA_SATURATE
	};

	dif = state ^ gl_state.currentState;
	if (!dif)
		return;

	if (dif & (GLSTATE_SRCMASK|GLSTATE_DSTMASK))
	{
		if (state & (GLSTATE_SRCMASK|GLSTATE_DSTMASK))
		{
			unsigned src, dst;

			src = ((state & GLSTATE_SRCMASK) - GLSTATE_SRC_ZERO) >> GLSTATE_SRCSHIFT;
			src = blends[src];

			dst = ((state & GLSTATE_DSTMASK) - GLSTATE_DST_ZERO) >> GLSTATE_DSTSHIFT;
			dst = blends[dst];

			qglEnable (GL_BLEND);
			qglBlendFunc (src, dst);
		}
		else
			qglDisable (GL_BLEND);
	}

	if (dif & GLSTATE_ALPHAMASK)
	{
		int		m;

		m = state & GLSTATE_ALPHAMASK;
		if (!m)
			qglDisable (GL_ALPHA_TEST);
		else
		{
			qglEnable (GL_ALPHA_TEST);
			if (m == GLSTATE_ALPHA_GT0)
				qglAlphaFunc (GL_GREATER, 0.05f);	//?? 0.0f
			else if (m == GLSTATE_ALPHA_LT05)
				qglAlphaFunc (GL_LESS, 0.5f);
			else // if (m == GLSTATE_ALPHA_GE05)
				qglAlphaFunc (GL_GEQUAL, 0.5f);
		}
	}

	if (dif & GLSTATE_DEPTHWRITE)
		qglDepthMask ((GLboolean)(state & GLSTATE_DEPTHWRITE ? GL_TRUE : GL_FALSE));

	if (dif & GLSTATE_NODEPTHTEST)
	{
		if (state & GLSTATE_NODEPTHTEST)
			qglDisable (GL_DEPTH_TEST);
		else
			qglEnable (GL_DEPTH_TEST);
	}

	if (dif & GLSTATE_DEPTHEQUALFUNC)
		qglDepthFunc (state & GLSTATE_DEPTHEQUALFUNC ? GL_EQUAL : GL_LEQUAL);

	if (dif & GLSTATE_POLYGON_LINE)
		qglPolygonMode (GL_FRONT_AND_BACK, state & GLSTATE_POLYGON_LINE ? GL_LINE : GL_FILL);

	gl_state.currentState = state;
}


void GL_SetDefaultState (void)
{
	int		i;

	qglDisable (GL_CULL_FACE);
	qglColor4f (1, 1, 1, 1);

	// setup texturing
	for (i = 1; i < gl_config.maxActiveTextures; i++)
	{
		GL_SelectTexture (i);
		qglDisable (GL_TEXTURE_2D);
		gl_state.currentBinds[i] = -1;
		GL_TexEnv (TEXENV_REPLACE);
	}
	if (gl_config.maxActiveTextures > 1) GL_SelectTexture (0);
	qglDisable (GL_TEXTURE_2D);
	gl_state.currentBinds[0] = -1;
	GL_TexEnv (TEXENV_MODULATE);

	GL_TextureMode (gl_texturemode->string);

	// set GL_State to a corresponding zero values
	qglDisable (GL_BLEND);
	qglDisable (GL_ALPHA_TEST);
	qglDepthMask (GL_TRUE);
	qglEnable (GL_DEPTH_TEST);
	qglDepthFunc (GL_LEQUAL);
	qglPolygonMode (GL_FRONT_AND_BACK, GL_FILL);

	qglEnable (GL_SCISSOR_TEST);
	qglShadeModel (GL_SMOOTH);
	qglEnableClientState (GL_VERTEX_ARRAY);
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
	GL_State (GLSTATE_SRC_SRCALPHA|GLSTATE_DST_ONEMINUSSRCALPHA|GLSTATE_NODEPTHTEST);
	GL_CullFace (CULL_NONE);
	gl_state.is2dMode = true;
}


// Setup OpenGL as specified in active portal
void GL_Setup (viewPortal_t *port)
{
	int		bits;

	LOG_STRING ("*** GL_Setup() ***\n");
	gl_state.is2dMode = false;
	qglMatrixMode (GL_PROJECTION);
	qglLoadMatrixf (&port->projectionMatrix[0][0]);
	qglMatrixMode (GL_MODELVIEW);
	qglLoadMatrixf (&port->modelMatrix[0][0]);		// required for sky drawing (remove this comment ??)

	qglViewport (port->x, port->y, port->w, port->h);
	qglScissor (port->x, port->y, port->w, port->h);
	GL_State (GLSTATE_DEPTHWRITE);

	bits = GL_DEPTH_BUFFER_BIT;
	if (gl_fastsky->integer && !(port->flags & RDF_NOWORLDMODEL))
	{
		bits |= GL_COLOR_BUFFER_BIT;
		qglClearColor (0, 0, 0, 1);
	}
	else if (port->flags & RDF_NOWORLDMODEL)
	{
		bits |= GL_COLOR_BUFFER_BIT;
		qglClearColor (0, 0.03, 0.03, 1);
	}
	qglClear (bits);
}
