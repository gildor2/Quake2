#include "gl_local.h"
#include "gl_shader.h"
#include "gl_backend.h"
#include "gl_lightmap.h"
#include "gl_sky.h"
#include "gl_math.h"

//#define SWAP_ON_BEGIN		// call GLimp_EndFrame() with SwapBuffers() on frame begin (or frame end if not defined)

/* LATER: replace (almost) all ap.time -> shader.time (or global "shaderTime") ??
 * (because time may be take (if engine will be extended) from entity)
 */


typedef struct
{
	float	tex[2];
	float	lm[2];
} bufTexCoordSrc_t;


/*------------- Command buffer ---------------*/

byte	backendCommands[MAX_BACKEND_COMMANDS];
int		backendCmdSize;
byte	*lastBackendCommand;

static shader_t		*currentShader;
static refEntity_t	*currentEntity;
static int			currentDlightMask;


/*-------------- Vertex arrays ---------------*/


#define NUM_VERTEX_BUFFERS	64

typedef struct
{
	bufVertex_t		verts[MAX_VERTEXES];
	color_t			color[MAX_VERTEXES];
	bufTexCoord_t	texCoord[0][MAX_VERTEXES];
} vertexBuffer_t;

static vertexBuffer_t	*vb;			// pointers to current buffer and to all buffers
static int				currentBuffer;	// index of the current buffer
static int				vbSize;			// size of 1 buffer (depends on multitexturing ability)

int				gl_indexesArray[MAX_INDEXES];
bufExtra_t		gl_extra[MAX_VERTEXES];

static color_t			srcVertexColor[MAX_VERTEXES];
static bufTexCoordSrc_t	srcTexCoord[MAX_VERTEXES];

int gl_numVerts, gl_numIndexes, gl_numExtra;


/*------------------ Scene -----------------*/

// surfaces
static surfaceInfo_t *sortedSurfaces[MAX_SCENE_SURFACES];

viewPortal_t ap;		// active portal


/*----- Process deforms, ...gens etc. ------*/


static void ProcessShaderDeforms (shader_t *sh)
{
	int		i, j, k;
	deformParms_t *deform;
	float	*table, f, t, *norm;
	bufVertex_t *vec;
	bufExtra_t	*ex;


	if (!sh->numDeforms)
		return;

	for (i = 0, deform = &sh->deforms[0]; i < sh->numDeforms; i++, deform++)
	{
		switch (deform->type)
		{
		case DEFORM_WAVE:
			table = mathFuncs[deform->wave.type];
			t = deform->wave.freq * ap.time + deform->wave.phase;

			vec = vb->verts;
			for (j = 0, ex = gl_extra; j < gl_numExtra; j++, ex++)
			{
				norm = ex->normal;
				for (k = 0; k < ex->numVerts; k++, vec++)
				{
					if (deform->wave.amp)
						f = PERIODIC_FUNC(table, t + deform->waveDiv * (vec->xyz[0] + vec->xyz[1] + vec->xyz[2]))
							* deform->wave.amp + deform->wave.base;
					else
						f = deform->wave.base;		// used for "outline shield" effect
					VectorMA (vec->xyz, f, norm, vec->xyz);
				}
			}
			break;
		//?? other types
		}
	}
}


static void GenerateColorArray (shaderStage_t *st)
{
	int		i, rgba;
	byte	a;
	color_t	*src, *dst;

	/*---------- rgbGen -----------*/
	src = srcVertexColor;
	dst = vb->color;
	switch (st->rgbGenType)
	{
	case RGBGEN_CONST:
		rgba = st->rgbaConst.rgba;
		for (i = 0; i < gl_numVerts; i++, dst++)
			dst->rgba = rgba;
		break;
	case RGBGEN_EXACT_VERTEX:
		for (i = 0; i < gl_numVerts; i++, src++, dst++)	//?? memcpy()
			dst->rgba = src->rgba;
		break;
	case RGBGEN_VERTEX:
		for (i = 0; i < gl_numVerts; i++, src++, dst++)
		{
			dst->c[0] = src->c[0] >> gl_config.overbrightBits;
			dst->c[1] = src->c[1] >> gl_config.overbrightBits;
			dst->c[2] = src->c[2] >> gl_config.overbrightBits;
		}
		break;
	case RGBGEN_BOOST_VERTEX:
		{
			int		ka, kb;

#define MIN_BOOST_BRIGHT		48
			ka = (256 - MIN_BOOST_BRIGHT) >> gl_config.overbrightBits;
			kb = (MIN_BOOST_BRIGHT * 256) >> gl_config.overbrightBits;
			for (i = 0; i < gl_numVerts; i++, src++, dst++)
			{
				int		r, g, b, oldbr, newbr, scale;

				r = src->c[0];
				g = src->c[1];
				b = src->c[2];
				oldbr = max(r, g);
				oldbr = max(oldbr, b);
				newbr = oldbr * ka + kb;
				if (oldbr)				// if at least one of r,g,b is non-zero
				{
					scale = newbr / oldbr;
					r = (r * scale) >> 8;
					g = (g * scale) >> 8;
					b = (b * scale) >> 8;
					NORMALIZE_COLOR255(r, g, b);
				}
				dst->c[0] = r;
				dst->c[1] = g;
				dst->c[2] = b;
			}
		}
		break;
	case RGBGEN_ONE_MINUS_VERTEX:
		for (i = 0; i < gl_numVerts; i++, src++, dst++)
		{
			dst->c[0] = (255 - src->c[0]) >> gl_config.overbrightBits;
			dst->c[1] = (255 - src->c[1]) >> gl_config.overbrightBits;
			dst->c[2] = (255 - src->c[2]) >> gl_config.overbrightBits;
		}
		break;
	case RGBGEN_LIGHTING_DIFFUSE:
		GL_ApplyEntitySpherelights (dst);
		break;
	// other types: FOG
	}

	if (st->rgbGenType == RGBGEN_EXACT_VERTEX && st->alphaGenType == ALPHAGEN_VERTEX)
		return;	// alpha is already set

	/*--------- alphaGen ----------*/
	dst = vb->color;
	switch (st->alphaGenType)
	{
	case ALPHAGEN_CONST:
		a = st->rgbaConst.c[3];
		for (i = 0; i < gl_numVerts; i++, dst++)
			dst->c[3] = a;
		break;
	case ALPHAGEN_VERTEX:
		for (i = 0; i < gl_numVerts; i++, dst++)
			dst->c[3] = srcVertexColor[i].c[3];
		break;
	case ALPHAGEN_ONE_MINUS_VERTEX:
		for (i = 0; i < gl_numVerts; i++, dst++)
			dst->c[3] = 255 - srcVertexColor[i].c[3];
		break;
	case ALPHAGEN_DOT:
	case ALPHAGEN_ONE_MINUS_DOT:
		{
			bufExtra_t *ex;
			bufVertex_t *vec;
			int		j, min;
			float	scale, *norm;

			if (st->alphaGenType == ALPHAGEN_DOT)
			{
				min = Q_round (st->alphaMin * 255);
				scale = (st->alphaMax - st->alphaMin) * 255;
			}
			else
			{
				min = Q_round (st->alphaMax * 255);
				scale = (st->alphaMin - st->alphaMax) * 255;
			}
			vec = vb->verts;
			for (j = 0, ex = gl_extra; j < gl_numExtra; j++, ex++)
			{
				norm = ex->normal;
				for (i = 0; i < ex->numVerts; i++, vec++, dst++)
				{
					vec3_t	v;
					float	d;

					VectorSubtract (currentEntity->modelvieworg, vec->xyz, v);
					VectorNormalizeFast (v);
					d = DotProduct (v, norm);
#if 0
					d = d * d;
#else
					if (d < 0) d = 0;
#endif
					dst->c[3] = Q_round (d * scale) + min;
				}
			}
		}
		break;
	// other types: LIGHTING_SPECULAR, PORTAL
	}
}


static void GenerateTexCoordArray (shaderStage_t *st, int tmu)
{
	int				j, k;
	tcModParms_t	*tcmod;
	bufTexCoord_t	*dst, *dst2;
	bufTexCoordSrc_t *src;

	dst = vb->texCoord[tmu];

	src = srcTexCoord;
	dst2 = dst;		// save this for tcMod

	// process tcGen
	switch (st->tcGenType)
	{
	case TCGEN_TEXTURE:
		for (j = 0; j < gl_numVerts; j++, src++, dst++)
		{
			dst->tex[0] = src->tex[0];
			dst->tex[1] = src->tex[1];
		}
		break;
	case TCGEN_LIGHTMAP:
		for (j = 0; j < gl_numVerts; j++, src++, dst++)
		{
			dst->tex[0] = src->lm[0];
			dst->tex[1] = src->lm[1];
		}
		break;
	case TCGEN_ENVIRONMENT:
		{
			bufExtra_t *ex;
			bufVertex_t *vec;
			vec3_t	v;
			float	d;

			vec = vb->verts;
			for (j = 0, ex = gl_extra; j < gl_numExtra; j++, ex++)
			{
				if (ex->axis)
				{	// compute envmap using surface axis
					vec3_t	*axis;

					axis = ex->axis;
					for (k = 0; k < ex->numVerts; k++, vec++, dst++)
					{
						VectorSubtract (currentEntity->modelvieworg, vec->xyz, v);
						VectorNormalizeFast (v);
						d = DotProduct (v, axis[0]);
						dst->tex[0] = (d - 1) / 2;
						d = DotProduct (v, axis[1]);
						dst->tex[1] = (d - 1) / 2;
					}
				}
				else
				{	// axis is not provided - use normal
					float	*norm;

					norm = ex->normal;
					for (k = 0; k < ex->numVerts; k++, vec++, dst++)
					{
						VectorSubtract (currentEntity->modelvieworg, vec->xyz, v);
						VectorNormalizeFast (v);
						d = DotProduct (v, norm);
						dst->tex[0] = (d * norm[1] * 2 - v[1] + 1) / 2.0f;
						dst->tex[1] = (d * norm[2] * 2 - v[2] + 1) / 2.0f;
					}
				}
			}
		}
		break;
	case TCGEN_LIGHTMAP1:
	case TCGEN_LIGHTMAP2:
	case TCGEN_LIGHTMAP3:
	case TCGEN_LIGHTMAP4:
		{
			float	shift, mul;
			bufExtra_t *ex;

			mul = (float)(st->tcGenType - TCGEN_LIGHTMAP1 + 1) / LIGHTMAP_SIZE;
			for (j = 0, ex = gl_extra; j < gl_numExtra; j++, ex++)
			{
				shift = ex->lmWidth * mul;
				for (k = 0; k < ex->numVerts; k++, src++, dst++)
				{
					dst->tex[0] = src->lm[0] + shift;
					dst->tex[1] = src->lm[1];
				}
			}
		}
		break;
	// other types: VECTOR, ZERO (?), FOG (?)

	default:
		if (st->tcGenType >= TCGEN_DLIGHT0 && st->tcGenType < TCGEN_DLIGHT0 + MAX_DLIGHTS)
		{
			bufExtra_t *ex;
			bufVertex_t *vec;
			surfDlight_t *sdl;
			int		num;

			num = st->tcGenType - TCGEN_DLIGHT0;
			vec = vb->verts;
			for (j = 0, ex = gl_extra; j < gl_numExtra; j++, ex++)
			{
				float	invRadius;

				sdl = &ex->dlight[num];
				invRadius = 0.5f / sdl->radius;
				for (k = 0; k < ex->numVerts; k++, dst++, vec++)
				{
					dst->tex[0] = (DotProduct (vec->xyz, sdl->axis[0]) - sdl->pos[0]) * invRadius + 0.5f;
					dst->tex[1] = (DotProduct (vec->xyz, sdl->axis[1]) - sdl->pos[1]) * invRadius + 0.5f;
				}
			}
		}
	}

	// process tcMod
	for (j = 0, tcmod = st->tcModParms; j < st->numTcMods; j++, tcmod++)
	{
		float	f1, f2;

		dst = dst2;
		switch (tcmod->type)
		{
		case TCMOD_SCROLL:
			f1 = tcmod->sSpeed * ap.time;
			f1 = f1 - floor (f1);
			f2 = tcmod->tSpeed * ap.time;
			f2 = f2 - floor (f2);
			for (k = 0; k < gl_numVerts; k++, dst++)
			{
				dst->tex[0] += f1;
				dst->tex[1] += f2;
			}
			break;
		case TCMOD_TURB:
			f1 = tcmod->wave.freq * ap.time + tcmod->wave.phase;
			f2 = tcmod->wave.amp;
			for (k = 0; k < gl_numVerts; k++, dst++)
			{
				float	*vec, f;

				vec = &vb->verts[k].xyz[0];	//?? optimize (vec++, but with different type)
				f = SIN_FUNC((vec[0] + vec[1] + vec[2]) / TABLE_SIZE + f1) * f2;
				dst->tex[0] += f;
				dst->tex[1] += f;
//				dst->tex[0] += PERIODIC_FUNC(sinTable, (vec[0] + vec[2]) / TABLE_SIZE + f1) * f2;	// Q3: vec[0] + vec[2]
//				dst->tex[1] += PERIODIC_FUNC(sinTable, (vec[1] + vec[2]) / TABLE_SIZE + f1) * f2;	// Q3: vec[1]
			}
			break;
		case TCMOD_WARP:
			for (k = 0; k < gl_numVerts; k++, dst++)
			{
				f1 = dst->tex[0];
				f2 = dst->tex[1];
				dst->tex[0] = f1 / 64.0f + SIN_FUNC((f2 / 16.0f + ap.time) / (2.0f * M_PI)) / 16.0f;
				dst->tex[1] = f2 / 64.0f + SIN_FUNC((f1 / 16.0f + ap.time) / (2.0f * M_PI)) / 16.0f;
			}
			break;
		case TCMOD_SCALE:
			f1 = tcmod->sScale;
			f2 = tcmod->tScale;
			for (k = 0; k < gl_numVerts; k++, dst++)
			{
				dst->tex[0] *= f1;
				dst->tex[1] *= f2;
			}
			break;
		case TCMOD_ROTATE:
			{
				float	f, c1, c2;

				f = - tcmod->rotateSpeed * ap.time / 180 * M_PI;	// angle
				f1 = SIN_FUNC(f);
				f2 = COS_FUNC(f);
				c1 = 0.5f * (1 - f1 - f2);
				c2 = 0.5f * (1 + f1 - f2);
				for (k = 0; k < gl_numVerts; k++, dst++)
				{
					f = dst->tex[0];
					dst->tex[0] = dst->tex[1] * f1 + f * f2 + c1;
					dst->tex[1] = dst->tex[1] * f2 - f * f1 + c2;
				}
			}
			break;
		// other types: TRANSFORM, ENTITYTRANSLATE, STRETCH
		}
	}
}


/*----------------- Shader visualization -------------------*/


// should rename "temp", "tmp" ?? (rendererStage_t ?)
typedef struct
{
	unsigned texEnv;		// REPLACE/MODILATE/ADD or COMBINE/COMBINE_NV (uses st.glState.blend ??)
	byte	tmu;
	byte	constRGBA;		// bool
	byte	identityRGBA;	// bool
	shaderStage_t st;		// modified copy of original stage (or auto-generated stage)
} tempStage_t;


typedef struct
{
	int		glState;
	int		numStages;
	tempStage_t *stages;
	shaderStage_t *colorStage;	//?? remove if it is always the same as "stages[0].st"
} renderPass_t;

#define MAX_TMP_STAGES	(MAX_SHADER_STAGES+8)

static tempStage_t	tmpSt[MAX_TMP_STAGES];
static int			numTmpStages;
static renderPass_t renderPasses[MAX_TMP_STAGES];
static int			numRenderPasses;
//?? move "currentShader here


#define BLEND_UNKNOWN			0
#define BLEND_MULTIPLICATIVE	1
#define BLEND_ADDITIVE			2
#define BLEND_ANY				(BLEND_MULTIPLICATIVE|BLEND_ADDITIVE)
#define BLEND_INCOMPATIBLE		0x80000000

#define DEBUG_FULLBRIGHT	1
#define DEBUG_LIGHTMAP		2

//#define LOG_PP(x)	LOG_STRING(x)
#define LOG_PP(x)

static void PreprocessShader (shader_t *sh)
{
	int		firstStage, i, debugMode, glState;
	shaderStage_t *stage, **pstage, *lmStage;
	tempStage_t *st;
	renderPass_t *pass;
	int		tmuUsed, tmuLeft, passStyle;
	qboolean entityLightingDone;

	debugMode = 0;
	if (r_fullbright->integer) debugMode |= DEBUG_FULLBRIGHT;
	if (r_lightmap->integer) debugMode |= DEBUG_LIGHTMAP;

	st = tmpSt;
	numTmpStages = 0;

	// get lightmap stage (should be first ??)
	lmStage = NULL;
	if (sh->numStages && sh->stages[0]->isLightmap) lmStage = sh->stages[0];
	firstStage = 0;

	if (lmStage && gl_dynamic->integer && !r_fullbright->integer)
	{
		if (currentDlightMask)
		{
			unsigned mask;
			int		num;

			for (i = 0, num = 0, mask = currentDlightMask; mask; i++, mask >>= 1)
			{
				if (!(mask & 1)) continue;

				st->st.mapImage[0] = gl_dlightImage;
				if (!numTmpStages)
					st->st.glState = lmStage->glState;
				else
					st->st.glState = GLSTATE_SRC_ONE|GLSTATE_DST_ONE;		// src + dst
				// set rgba
				st->st.alphaGenType = ALPHAGEN_CONST;
				st->st.rgbGenType = RGBGEN_CONST;
				st->st.rgbaConst.rgba = ap.dlights[i].c.rgba;
				// set tcGen
				st->st.tcGenType = TCGEN_DLIGHT0 + num;
				st->st.numTcMods = 0;
				// finish
				st++;
				numTmpStages++;
				num++;
			}
		}

		if (sh->lightStyles_i || currentDlightMask)
		{
			/*------------ add dynamic lightmaps ------------*/
			// even if shader have no lightstyles, we will get here to add main lightmap stage
			for (i = 0; i <= 4; i++)		// last iteration reserved for main (slow) lightmap
			{
				byte	style;

				style = (i < 4 ? currentShader->lightStyles[i] : 0);		// if shader have less than 4 styles, we will get 0 earlier

				// set image
				st->st.mapImage[0] = lmStage->mapImage[0];
				// set glState
				if (!numTmpStages)
					st->st.glState = lmStage->glState;
				else
					st->st.glState = GLSTATE_SRC_ONE|GLSTATE_DST_ONE;		// src + dst
				// set rgba
				st->st.alphaGenType = ALPHAGEN_CONST;
				st->st.rgbGenType = RGBGEN_CONST;
				if (style)
				{
					byte	c;

					c = ap.lightStyles[style].value;
					st->st.rgbaConst.rgba = (255 << 24) | (c << 16) | (c << 8) | c;
				}
				else
					st->st.rgbaConst.rgba = 0xFFFFFFFF;
				// set tcGen
				st->st.tcGenType = style ? TCGEN_LIGHTMAP1 + i : TCGEN_LIGHTMAP;
				st->st.numTcMods = 0;
				// finish
				st++;
				numTmpStages++;
				if (!style) break;			// it was main lightmap
			}
			firstStage = 1;			// skip lightmap stage
		}
	}

	/*-------------- copy remainder stages -------------*/

	glState = -1;
	if (lmStage) glState = lmStage->glState;
	for (i = firstStage, pstage = sh->stages + firstStage; i < sh->numStages; i++, pstage++)
	{
		stage = *pstage;

		/*---------- fullbright/lightmap ---------------*/
		if (debugMode == DEBUG_FULLBRIGHT && stage->isLightmap) continue;
		if (debugMode == DEBUG_LIGHTMAP && currentShader->lightmapNumber >= 0 && !stage->isLightmap) break;

		/*--------------- copy stage -------------------*/
		memcpy (&st->st, stage, sizeof(shaderStage_t));
		// select stage image from animation
		if (stage->numAnimTextures > 1)
		{
			int		n;

			if (currentEntity == &gl_entities[ENTITYNUM_WORLD] || !stage->frameFromEntity)
				n = Q_round (ap.time * stage->animMapFreq);
			else
				n = currentEntity->frame;
			st->st.mapImage[0] = stage->mapImage[n % stage->numAnimTextures];
		}

		/*---------- fullbright/lightmap ---------------*/
		// fix glState if current stage is first after lightmap with r_fullbright!=0
		if (debugMode == DEBUG_FULLBRIGHT && glState != -1)
		{
			st->st.glState = glState;
			glState = -1;
		}
		if (debugMode == (DEBUG_LIGHTMAP|DEBUG_FULLBRIGHT) && !gl_state.is2dMode)
			st->st.glState |= GLSTATE_POLYGON_LINE;
		if (!i && (currentShader->lightmapNumber == LIGHTMAP_VERTEX || stage->rgbGenType == RGBGEN_LIGHTING_DIFFUSE))
		{
			if (debugMode == DEBUG_LIGHTMAP)
				st->st.mapImage[0] = NULL;
			else if (debugMode == DEBUG_FULLBRIGHT)
			{
				st->st.rgbGenType = RGBGEN_CONST;
				st->st.rgbaConst.rgba |= 0xFFFFFF;
			}
		}

		st++;
		numTmpStages++;
	}

	entityLightingDone = false;
	for (i = 0, st = tmpSt; i < numTmpStages; i++, st++)
	{
		/*------- convert some rgbaGen to CONST --------*/
		switch (st->st.rgbGenType)
		{
		// NONE, IDENTITY, IDENTITY_LIGHTING: already converted to CONST (on shader loading)
		//?? check later for "entity >> overbrightBits" validness
		case RGBGEN_ENTITY:
			st->st.rgbaConst.c[0] = currentEntity->shaderColor.c[0] >> gl_config.overbrightBits;
			st->st.rgbaConst.c[1] = currentEntity->shaderColor.c[1] >> gl_config.overbrightBits;
			st->st.rgbaConst.c[2] = currentEntity->shaderColor.c[2] >> gl_config.overbrightBits;
			st->st.rgbGenType = RGBGEN_CONST;
			break;
		case RGBGEN_ONE_MINUS_ENTITY:
			st->st.rgbaConst.c[0] = (255 - currentEntity->shaderColor.c[0]) >> gl_config.overbrightBits;
			st->st.rgbaConst.c[1] = (255 - currentEntity->shaderColor.c[1]) >> gl_config.overbrightBits;
			st->st.rgbaConst.c[2] = (255 - currentEntity->shaderColor.c[2]) >> gl_config.overbrightBits;
			st->st.rgbGenType = RGBGEN_CONST;
			break;
		case RGBGEN_WAVE:
			{
				float	c1;
				byte	c2;

				//?? function may be NOISE
#define P stage->rgbGenWave
				c1 = PERIODIC_FUNC(mathFuncs[stage->rgbGenWave.type], P.freq * ap.time + P.phase) * P.amp + P.base;
#undef P
				c2 = Q_round (c1 * 255);
				c2 = bound(c2, 0, 255);
				st->st.rgbaConst.c[0] = st->st.rgbaConst.c[1] = st->st.rgbaConst.c[2] = c2;
				st->st.rgbGenType = RGBGEN_CONST;
			}
			break;
		case RGBGEN_LIGHTING_DIFFUSE:
		/*	if (Cvar_VariableInt("nolight")) // DEBUG
			{
				st->st.rgbaConst.c[0] = st->st.rgbaConst.c[1] = st->st.rgbaConst.c[2] = gl_config.identityLightValue;
				st->st.rgbGenType = RGBGEN_CONST;
				break;
			} */
			if (!entityLightingDone)
			{
				if (currentEntity->flags & RF_FULLBRIGHT)
				{
					st->st.rgbaConst.c[0] = st->st.rgbaConst.c[1] = st->st.rgbaConst.c[2] = gl_config.identityLightValue;
					st->st.rgbGenType = RGBGEN_CONST;
				}
				else
					GL_LightForEntity (currentEntity);
				entityLightingDone = true;
			}
			break;
		}

		switch (st->st.alphaGenType)
		{
		// IDENTITY already converted to CONST
		case ALPHAGEN_ENTITY:
			st->st.rgbaConst.c[3] = currentEntity->shaderColor.c[3];
			st->st.alphaGenType = ALPHAGEN_CONST;
			break;
		case ALPHAGEN_ONE_MINUS_ENTITY:
			st->st.rgbaConst.c[3] = 255 - currentEntity->shaderColor.c[3];
			st->st.alphaGenType = ALPHAGEN_CONST;
			break;
		}

		st->constRGBA = (st->st.rgbGenType == RGBGEN_CONST && st->st.alphaGenType == ALPHAGEN_CONST);
		st->identityRGBA = (st->constRGBA && st->st.rgbaConst.rgba == 0xFFFFFFFF);
	}

	if (!numTmpStages)
		DrawTextLeft (va("R_PreprocessShader(%s): 0 stages", currentShader->name), 1, 0, 0);

	if (numTmpStages > MAX_TMP_STAGES)
		Com_Error (ERR_FATAL, "R_PreprocessShader: numStages is too big (%d)", numTmpStages);

	pass = renderPasses;
	st = tmpSt;

	/*--------- if no multitexturing -- nothing to combine ---------------*/

	if (numTmpStages < 2 || gl_config.maxActiveTextures < 2)
	{
		for (i = 0; i < numTmpStages; i++, st++, pass++)
		{
			pass->glState = st->st.glState;
			pass->numStages = 1;
			pass->stages = st;
			pass->colorStage = &st->st;
			st->tmu = 0;
			st->texEnv = TEXENV_MODULATE;
		}
		numRenderPasses = numTmpStages;
		return;
	}

	/*-------------------- have multitexturing ---------------------------*/

	numRenderPasses = 0;
	tmuLeft = tmuUsed = 0;
	passStyle = BLEND_UNKNOWN;
	LOG_PP(va("--- PreprocessShader(%s) ---\n", sh->name));
	for (i = 0; i < numTmpStages; i++, st++)
	{
		int		blend2;

		if (!tmuLeft)						// all tmu was distributed or just started
		{
			tmuLeft = gl_config.maxActiveTextures;
			if (tmuUsed) pass++;
			tmuUsed = 0;
			// start new pass
			numRenderPasses++;
			pass->glState = st[0].st.glState;
			pass->numStages = 1;
			pass->stages = st;
			pass->colorStage = &st->st;
			st[0].texEnv = TEXENV_MODULATE;		// modulate with rgbaGen

			switch (pass->glState & (GLSTATE_SRCMASK|GLSTATE_DSTMASK))
			{
			case 0:
				passStyle = BLEND_ANY;
				break;
			case GLSTATE_SRC_ONE|GLSTATE_DST_ONE:
				passStyle = BLEND_ADDITIVE;
				break;
			case GLSTATE_SRC_DSTCOLOR|GLSTATE_DST_SRCCOLOR:
			case GLSTATE_SRC_DSTCOLOR|GLSTATE_DST_ZERO:
				passStyle = BLEND_MULTIPLICATIVE;
				break;
			default:
				passStyle = BLEND_INCOMPATIBLE;
			}
			LOG_PP("-- next pass\n");
		}
		LOG_PP(va("  tmu[%d:%d] = \"%s\" (rgba: %s %s %8X)\n", i, tmuUsed, st[0].st.mapImage[0]->name,
			st[0].constRGBA ? "const" : "var", st[0].identityRGBA ? "ident" : "--", st[0].st.rgbaConst.rgba));

		st[0].tmu = tmuUsed++;
		tmuLeft--;
		// if at least 2nd TMU, we may need to update some pass fields
		if (tmuUsed > 1)
			pass->glState |= st[0].st.glState & GLSTATE_DEPTHWRITE;	// pass.depthwrite = OR(stages.depthwrite)

		if (i == numTmpStages - 1) break;	// no next stage to combine

		/* If can combine: pass->numStages++, set st[1].texEnv and continue
		 * If can't:       set tmuLeft to 0
		 */

		// check for compatibility of current glState with the next glState
		if (tmuLeft < 1 || passStyle == BLEND_INCOMPATIBLE ||
			(st[0].st.glState ^ st[1].st.glState) & ~(GLSTATE_SRCMASK|GLSTATE_DSTMASK|GLSTATE_DEPTHWRITE))
		{	// incompatible ... next stage will be 1st in the next rendering pass
			tmuLeft = 0;
			continue;
		}

		// now, we can check blendmode and rgbaGen
		blend2 = st[1].st.glState & (GLSTATE_SRCMASK|GLSTATE_DSTMASK);

		// pure multitexture: any rgba for 1st TMU, identity for others
		if (st[1].identityRGBA && (tmuUsed == 1 || st[0].identityRGBA))
		{
			// pure multitexture can emulate only 2 blendmodes: "src*dst" and "src+dst" (when texenv_add)
			if (blend2 == (GLSTATE_SRC_DSTCOLOR|GLSTATE_DST_ZERO) && passStyle & BLEND_MULTIPLICATIVE)
			{
				LOG_PP(va("  MT(MUL): with \"%s\"\n", st[1].st.mapImage[0]->name));
				st[1].texEnv = TEXENV_MODULATE;
				pass->numStages++;
				continue;
			}

			if (GL_SUPPORT(QGL_EXT_TEXTURE_ENV_ADD|QGL_ARB_TEXTURE_ENV_ADD|QGL_EXT_TEXTURE_ENV_COMBINE|QGL_ARB_TEXTURE_ENV_COMBINE) &&
				blend2 == (GLSTATE_SRC_ONE|GLSTATE_DST_ONE) && passStyle & BLEND_ADDITIVE)
			{
				LOG_PP(va("  MT(ADD): with \"%s\"\n", st[1].st.mapImage[0]->name));
				st[1].texEnv = (GL_SUPPORT(QGL_EXT_TEXTURE_ENV_ADD|QGL_ARB_TEXTURE_ENV_ADD))
					? TEXENV_ADD : TEXENV_C_ADD | TEXENV_0PREV_1TEX;
				pass->numStages++;
				continue;
			}

			if (GL_SUPPORT(QGL_EXT_TEXTURE_ENV_COMBINE|QGL_ARB_TEXTURE_ENV_COMBINE)
				&& blend2 == (GLSTATE_SRC_DSTCOLOR|GLSTATE_DST_SRCCOLOR)
				&& passStyle & BLEND_MULTIPLICATIVE)
			{
				LOG_PP(va("  MT(MUL2): with \"%s\"\n", st[1].st.mapImage[0]->name));
				st[1].texEnv = TEXENV_C_MODULATE | TEXENV_MUL2 | TEXENV_0PREV_1TEX;
				pass->numStages++;
				continue;
			}
		}

		if (GL_SUPPORT(QGL_NV_TEXTURE_ENV_COMBINE4) && st[1].constRGBA && (tmuUsed == 1 || st[0].constRGBA) &&
			// This extension can perform A*T1+B*T2, where A/B is 1|0|prev|tex
			// st[1] is B*T2, if rgba is not 1, rgbGen will eat B and we will not be able to use most of blends ...
			(st[1].identityRGBA || (blend2 & GLSTATE_SRCMASK) == GLSTATE_SRC_ONE))
		{
			unsigned env, env1, env2, b1, b2;
			qboolean combine;
			static unsigned blendToEnv[] = {	// this table is corresponding to GLSTATE_[SRC|DST]_XXX
				0,
				TEXENV_ZERO, TEXENV_ONE,
				TEXENV_TEXTURE, TEXENV_ONE_MINUS_TEXTURE,
				TEXENV_TEXALPHA, TEXENV_ONE_MINUS_TEXALPHA,
				TEXENV_PREVIOUS, TEXENV_ONE_MINUS_PREVIOUS,
				TEXENV_PREVALPHA, TEXENV_ONE_MINUS_PREVALPHA,
				0								// SRC_ALPHASATURATE - cannot be multitextured
			};

			combine = true;

			// check for valid blend mode
			b1 = blend2 & GLSTATE_SRCMASK;
			b2 = blend2 & GLSTATE_DSTMASK;
			if (passStyle == BLEND_ADDITIVE)
			{
				if (b1 != GLSTATE_SRC_ONE && b1 != GLSTATE_SRC_SRCALPHA && b1 != GLSTATE_SRC_ONEMINUSSRCALPHA)
					combine = false;
				if (b2 != GLSTATE_DST_ONE && b2 != GLSTATE_DST_DSTALPHA && b2 != GLSTATE_DST_ONEMINUSDSTALPHA)
					combine = false;
			}
			if (combine && passStyle == BLEND_MULTIPLICATIVE)
			{
				if (b1 != GLSTATE_SRC_ZERO && b2 != GLSTATE_DST_ZERO && blend2 != (GLSTATE_SRC_DSTCOLOR|GLSTATE_DST_SRCCOLOR))
					combine = false;
			}
			// convert blendmode to TexEnv
			env1 = blendToEnv[(blend2 & GLSTATE_SRCMASK) >> GLSTATE_SRCSHIFT];
			env2 = blendToEnv[(blend2 & GLSTATE_DSTMASK) >> GLSTATE_DSTSHIFT];
			if (!(env1 && env2)) combine = false;

			env = TEXENV_C4_ADD | (TEXENV_TEXTURE<<TEXENV_SRC1_SHIFT) | (TEXENV_PREVIOUS<<TEXENV_SRC3_SHIFT);
			if (st[1].identityRGBA || env1 != TEXENV_ONE)
				env |= env1 << TEXENV_SRC0_SHIFT;
			else
				env |= (TEXENV_CONSTANT << TEXENV_SRC0_SHIFT) | TEXENV_ENVCOLOR;
			env |= env2 << TEXENV_SRC2_SHIFT;

			// combine stages ...
			if (combine)
			{
				st[1].texEnv = env;
				LOG_PP(va("  MT(NV): %X -> %08X\n", blend2, env));
				pass->numStages++;
				continue;
			}
		}

		// GL_EXT_TEXTURE_ENV_COMBINE is unsupported here (no INTERP in this extension)
		if (GL_SUPPORT(QGL_ARB_TEXTURE_ENV_COMBINE) && blend2 == (GLSTATE_SRC_ONE|GLSTATE_DST_ONE)
			&& passStyle & BLEND_ADDITIVE && st[0].constRGBA && st[1].constRGBA && tmuUsed == 1)
		{
			int		k;

			LOG_PP(va("  MT(INTERP*2): with \"%s\"\n", st[1].st.mapImage[0]->name));
			st[1].texEnv = TEXENV_C_INTERP | TEXENV_MUL2 | TEXENV_ENVCOLOR | TEXENV_0PREV_1TEX | (TEXENV_CONSTANT<<TEXENV_SRC2_SHIFT);
			// set RGBA for both stages
			LOG_PP(va("  (old rgba: %X %X", st[0].st.rgbaConst.rgba, st[1].st.rgbaConst.rgba));
			for (k = 0; k < 4; k++)
			{
				int		k2;

				k2 = st[1].st.rgbaConst.c[k];
				st[0].st.rgbaConst.c[k] = st[0].st.rgbaConst.c[k] * 255 / (255*2 - k2);
				st[1].st.rgbaConst.c[k] = 255 - k2 / 2;
			}
			LOG_PP(va(" new rgba: %X %X)\n",  st[0].st.rgbaConst.rgba, st[1].st.rgbaConst.rgba));
			pass->numStages++;
			continue;
		}

		// not combined - begin new pass
		tmuLeft = 0;
	}
	LOG_PP("-----------------\n");
}
#undef LOG_PP


//?? remove function
static void DrawArrays (int *indexes, int numIndexes)
{
/*	if (!strcmp (currentShader->name, "*default"))	//!!
	{
		int *idx, i;
		bufVertex_t *v;
		bufTexCoord_t *t;

		DrawTextLeft (va("--- draw arrays (%d, %d) ---", numVerts, numIndexes), 0,1,1);
		v = &vb->verts[0];
		t = &vb->texCoord0[0];
		for (i = 0; i < numVerts; i++, v++, t++)
		{
			DrawTextLeft(va("%d.  v={%g,%g,%g} st={%g,%g}",i,v->xyz[0],v->xyz[1],v->xyz[2],t->tex[0],t->tex[1]), 0,1,0.5);
		}
		for (i = 0, idx = indexes; i < numIndexes; i += 6, idx += 6)
		{
			DrawTextLeft (va("  i = %d,%d,%d,%d,%d,%d",idx[0],idx[1],idx[2],idx[3],idx[4],idx[5]), 0,1,0.5);
		}
	}
*/
	qglDrawElements (GL_TRIANGLES, numIndexes, GL_UNSIGNED_INT, indexes);
}


// forwards
void DrawTriangles (void);
void DrawNormals (void);


static void StageIterator (void)
{
	int		i;
	renderPass_t *pass;

//	DrawTextLeft (va("StageIterator(%s, %d, %d)\n", currentShader->name, numVerts, numIndexes),1,1,1);//!!!
	LOG_STRING (va("*** StageIterator(%s, %d, %d) ***\n", currentShader->name, gl_numVerts, gl_numIndexes));

	if (!currentShader->numStages)
		return;

	PreprocessShader (currentShader);

	/*------------- prepare renderer --------------*/

	ProcessShaderDeforms (currentShader);
	qglVertexPointer (3, GL_FLOAT, sizeof(bufVertex_t), vb->verts);

	GL_CullFace (currentShader->cullMode);
	if (currentShader->usePolygonOffset)
	{
		qglEnable (GL_POLYGON_OFFSET_FILL);
		//?? use cvars for units/factor
		qglPolygonOffset (-1, -2);
	}

	if (numRenderPasses > 1 && qglLockArraysEXT)
	{
		// do not lock texcoords and colors
		qglDisableClientState (GL_COLOR_ARRAY);
		GL_DisableTexCoordArrays ();
		qglLockArraysEXT (0, gl_numVerts);
	}

	/*---------------- draw stages ----------------*/

	for (i = 0, pass = renderPasses; i < numRenderPasses; i++, pass++)
	{
		shaderStage_t *stage;
		tempStage_t *st;
		int		j;

		LOG_STRING (va("-- pass %d (mtex %d) --\n", i, pass->numStages));
		GL_Lock ();
		GL_SetMultitexture (pass->numStages);

		for (j = 0, st = pass->stages; j < pass->numStages; j++, st++)
		{
			stage = &st->st;
			GL_SelectTexture (st->tmu);
			GL_TexEnv (st->texEnv);
			GL_TexEnvColor (&st->st.rgbaConst);
			GL_Bind (stage->mapImage[0]);
			GenerateTexCoordArray (stage, st->tmu);
			GL_TexCoordPointer (vb->texCoord[st->tmu]);
		}

		//------ setup color arrays / constant ------
		stage = pass->colorStage;
		if (stage->rgbGenType != RGBGEN_CONST || stage->alphaGenType != ALPHAGEN_CONST)
		{
			GenerateColorArray (stage);
			qglEnableClientState (GL_COLOR_ARRAY);
			qglColorPointer (4, GL_UNSIGNED_BYTE, 0, vb->color);
		}
		else
		{
			qglDisableClientState (GL_COLOR_ARRAY);
			qglColor4ubv (stage->rgbaConst.c);
		}

		GL_State (pass->glState);
		GL_Unlock ();

		//!! don't works
		if (i == numRenderPasses - 1 && !(ap.flags & RDF_NOWORLDMODEL)
			&& currentShader->type == SHADERTYPE_NORMAL && !gl_state.is2dMode)
			GL_EnableFog (true);
//		else if (Cvar_VariableInt("test"))//??
//			GL_EnableFog (false);

		DrawArrays (gl_indexesArray, gl_numIndexes);
	}

	/*----------------- finalize ------------------*/

	if (numRenderPasses > 1 && qglUnlockArraysEXT)
		qglUnlockArraysEXT ();

	if (currentShader->usePolygonOffset)
		qglDisable (GL_POLYGON_OFFSET_FILL);

	// debug
	if (!gl_state.is2dMode)
	{
		if (gl_showtris->integer)
			DrawTriangles ();
		if (gl_shownormals->integer)
			DrawNormals ();

		gl_speeds.tris += gl_numIndexes * numTmpStages / 3;
		gl_speeds.trisMT += gl_numIndexes * numRenderPasses / 3;
	}
	else
		gl_speeds.tris2D += gl_numIndexes * numTmpStages / 3;

	gl_speeds.numIterators++;
}


static void SetCurrentShader (shader_t *shader)
{
	if (gl_numVerts && gl_numIndexes)
		// we can get situation, when verts==2 and inds==0 due to geometry simplification -- this is OK (??)
		// sample: map "actmet", inside building with rotating glass doors, floor 2: exotic lamp (nested cilinders with alpha)
	{
		DrawTextLeft ("SetCurrentShader() without flush!",1,0,0);
		Com_WPrintf ("SetCurrentShader(%s) without flush (old: %s, %d verts, %d inds)\n",
			shader->name, currentShader->name, gl_numVerts, gl_numIndexes);
	}
	currentShader = shader;
	gl_numVerts = gl_numIndexes = gl_numExtra = 0;		// clear buffer
	//?? setup shader time
}


static void FlushArrays (void)
{
//	DrawTextLeft(va("FlushArrays(%s,%d,%d)\n",currentShader->name,numVerts,numIndexes),1,1,1);
	if (!gl_numIndexes) return;	// buffer is empty

	StageIterator ();
	gl_numVerts = gl_numIndexes = gl_numExtra = 0;
}


static void ReserveVerts (int verts, int inds)
{
	if (gl_numIndexes + inds > MAX_INDEXES || gl_numVerts + verts > MAX_VERTEXES)
		FlushArrays ();

	if (verts > MAX_VERTEXES)	Com_Error (ERR_DROP, "ReserveVerts: %d > MAX_VERTEXES", verts);
	if (inds > MAX_INDEXES)		Com_Error (ERR_DROP, "ReserveVerts: %d > MAX_INDEXES", inds);
}


/*---------------- Skybox ------------------*/


static void DrawSkyBox (void)
{
	int		side;

	LOG_STRING ("*** DrawSkyBox() ***\n");
	if (gl_fastsky->integer) return;

	if (currentShader->skyRotate) GL_ShowWholeSky ();

	GL_DepthRange (gl_showsky->integer ? DEPTH_NEAR : DEPTH_FAR);

	qglDisableClientState (GL_COLOR_ARRAY);
	if (currentShader != gl_defaultSkyShader)
		qglColor3f (gl_config.identityLightValue_f,
					gl_config.identityLightValue_f,
					gl_config.identityLightValue_f); // avoid overbright
	else
//		qglColor3f (0, 0, 0);	// bad sky -- make it black (almost as gl_fastsky)
		qglColor3fv (gl_fogColor);

	qglPushMatrix ();
	// if we will add "NODEPTHTEST" if gl_showsky mode -- DEPTHWITE will no effect
	GL_State (gl_showsky->integer ? GLSTATE_DEPTHWRITE : GLSTATE_NODEPTHTEST);
	GL_SetMultitexture (1);
	// change modelview matrix
	qglTranslatef (VECTOR_ARGS(ap.vieworg));
	if (currentShader->skyRotate)
	{
		qglRotatef (ap.time * currentShader->skyRotate,
			currentShader->skyAxis[0],
			currentShader->skyAxis[1],
			currentShader->skyAxis[2]);
	}

	GL_TexEnv (TEXENV_MODULATE);
	qglTexCoordPointer (2, GL_FLOAT, 0, vb->texCoord[0]);
	qglVertexPointer (3, GL_FLOAT, sizeof(bufVertex_t), vb->verts);

	for (side = 0; side < 6; side++)
	{
		gl_numIndexes = GL_TesselateSkySide (side, vb->verts, vb->texCoord[0], ap.zFar);
		if (!gl_numIndexes) continue;	// no surfaces on this side

		if (currentShader != gl_defaultSkyShader)
			GL_Bind (currentShader->skyFarBox[side]);
		else
			GL_Bind (NULL);			// disable texturing

		DrawArrays (gl_indexesArray, gl_numIndexes);

		//?? some debug stuff from StageIterator()
		if (gl_showtris->integer)
		{
			DrawTriangles ();
			// need to perform some state restoration (do it with another way ??)
			GL_State (gl_showsky->integer ? GLSTATE_DEPTHWRITE : GLSTATE_NODEPTHTEST);
			if (currentShader != gl_defaultSkyShader)
				qglColor3f (gl_config.identityLightValue_f,
							gl_config.identityLightValue_f,
							gl_config.identityLightValue_f); // avoid overbright
			else
//				qglColor3f (0, 0, 0);
				qglColor3fv (gl_fogColor);
			GL_SetMultitexture (1);
		}
		gl_speeds.tris += gl_numIndexes * numTmpStages / 3;
		gl_speeds.trisMT += gl_numIndexes * numRenderPasses / 3;
	}
	qglPopMatrix ();

	gl_numVerts = gl_numIndexes = gl_numExtra = 0;

	GL_DepthRange (DEPTH_NORMAL);
}


/*----------------- Scene ------------------*/

static void CheckDynamicLightmap (surfaceCommon_t *s)
{
	int		i, updateType;
	surfacePlanar_t *surf;
	dynamicLightmap_t *dl;
	qboolean dlightUpdate;

	if (!gl_dynamic->integer)
		return;

	// check for lightstyle modification (or vertex colors)
	surf = s->pl;
	if (!surf->lightmap) return;

	dl = surf->lightmap;
	updateType = 0;
	for (i = 0; i < dl->numStyles; i++)
	{
		byte	style;

		style = dl->style[i];
		if (IS_FAST_STYLE(style))
		{
			updateType = 1;		// vertex color
			continue;
		}
		if (dl->modulate[i] != ap.lightStyles[style].value)
		{
			updateType = 2;		// lightmap
			break;
		}
	}

	dlightUpdate = (s->dlightMask && s->shader->lightmapNumber == LIGHTMAP_VERTEX);
	if (dlightUpdate) updateType |= 1;	// vertex
	if (updateType)
	{
		// require to update lightmap
		for (i = 0; i < dl->numStyles; i++)
			dl->modulate[i] = ap.lightStyles[dl->style[i]].value;
		if (dlightUpdate) dl->modulate[0]--;	// force to update vertex lightmap when dlight disappear

		GL_UpdateDynamicLightmap (s->shader, surf, updateType == 1, s->dlightMask);
	}
}


static void TesselatePlanarSurf (surfacePlanar_t *surf)
{
	bufVertex_t	*v;
	bufExtra_t	*ex;
	bufTexCoordSrc_t *t;
	int			*idx, *idxSrc, i, firstVert, firstIndex;
	int			*c;
	vertex_t	*vs;

	ReserveVerts (surf->numVerts, surf->numIndexes);

	firstVert = gl_numVerts;
	gl_numVerts += surf->numVerts;
	firstIndex = gl_numIndexes;
	gl_numIndexes += surf->numIndexes;

	ex = &gl_extra[gl_numExtra++];
	ex->numVerts = surf->numVerts;
	if (surf->lightmap) ex->lmWidth = surf->lightmap->w;
	VectorCopy (surf->plane.normal, ex->normal);
	ex->axis = surf->axis;
	ex->dlight = surf->dlights;

	v = &vb->verts[firstVert];
	t = &srcTexCoord[firstVert];
	c = &srcVertexColor[firstVert].rgba;

	// copy vertexes
	vs = surf->verts;
	for (i = 0; i < surf->numVerts; i++, vs++, v++, t++, c++)
	{
		VectorCopy (vs->xyz, v->xyz);		// copy vertex
		t->tex[0] = vs->st[0];		// copy texture coords
		t->tex[1] = vs->st[1];
		t->lm[0] = vs->lm[0];		// copy lightmap coords
		t->lm[1] = vs->lm[1];
		*c = vs->c.rgba;			// copy vertex color (sometimes may be ignored??)
	}

	// copy indexes
	idx = &gl_indexesArray[firstIndex];
	idxSrc = surf->indexes;
	for (i = 0; i < surf->numIndexes; i++)
		*idx++ = *idxSrc++ + firstVert;
}


static void TesselatePolySurf (surfacePoly_t *surf)
{
	bufVertex_t	*v;
	bufExtra_t	*ex;
	bufTexCoordSrc_t *t;
	int			*idx, i, firstVert, firstIndex;
	int			*c;
	int			numIdx;
	vertexPoly_t *vs;

	numIdx = (surf->numVerts - 2) * 3;
	ReserveVerts (surf->numVerts, numIdx);

	firstVert = gl_numVerts;
	gl_numVerts += surf->numVerts;
	firstIndex = gl_numIndexes;
	gl_numIndexes += numIdx;

	ex = &gl_extra[gl_numExtra++];
	ex->numVerts = surf->numVerts;
	// setup normal ??
	ex->axis = NULL;
	ex->dlight = NULL;

	v = &vb->verts[firstVert];
	t = &srcTexCoord[firstVert];
	c = &srcVertexColor[firstVert].rgba;

	// copy vertexes
	vs = surf->verts;
	for (i = 0; i < surf->numVerts; i++, vs++, v++, t++, c++)
	{
		VectorCopy (vs->xyz, v->xyz);		// copy vertex
		t->tex[0] = vs->st[0];		// copy texture coords
		t->tex[1] = vs->st[1];
//	??	t->lm[0] = 0;				// copy lightmap coords
//		t->lm[1] = 0;
		*c = vs->c.rgba;			// copy vertex color (sometimes may be ignored??)
	}

	// copy indexes
	idx = &gl_indexesArray[firstIndex];

	for (i = 1; i < gl_numVerts - 1; i++)
	{
		*idx++ = firstVert;
		*idx++ = firstVert + i;
		*idx++ = firstVert + i + 1;
	}
}


static void TesselateMd3Surf (surfaceMd3_t *surf)
{
	bufVertex_t	*v;
	bufExtra_t	*ex;
	bufTexCoordSrc_t *t;
	int			*idx, *idxSrc, i, firstVert, firstIndex;
	int			*c;
	vertexMd3_t	*vs1, *vs2;
	float		*ts;
	int		numIdx;

	numIdx = surf->numTris * 3;
	ReserveVerts (surf->numVerts, numIdx);

	firstVert = gl_numVerts;
	gl_numVerts += surf->numVerts;
	firstIndex = gl_numIndexes;
	gl_numIndexes += numIdx;

	ex = &gl_extra[gl_numExtra];
	gl_numExtra += surf->numVerts;

	/*------------- lerp verts ---------------*/
	vs1 = surf->verts + surf->numVerts * currentEntity->frame;
	v = &vb->verts[firstVert];
	if (currentEntity->backLerp != 0.0f && currentEntity->frame != currentEntity->oldFrame)
	{
		float	frontScale, backScale, frontLerp;
		float	a1, sa1, b1, a2, sa2, b2, *norm;

		vs2 = surf->verts + surf->numVerts * currentEntity->oldFrame;
		backScale = currentEntity->backLerp * MD3_XYZ_SCALE;
		frontLerp = 1.0f - currentEntity->backLerp;
		frontScale = frontLerp * MD3_XYZ_SCALE;
		for (i = 0; i < surf->numVerts; i++, vs1++, vs2++, v++, ex++)
		{
			v->xyz[0] = vs1->xyz[0] * frontScale + vs2->xyz[0] * backScale;
			v->xyz[1] = vs1->xyz[1] * frontScale + vs2->xyz[1] * backScale;
			v->xyz[2] = vs1->xyz[2] * frontScale + vs2->xyz[2] * backScale;
			norm = ex->normal;
			// lerp normal
			a1 = (vs1->normal & 255) / 255.0f;
			b1 = ((vs1->normal >> 8) & 255) / 255.0f;
			sa1 = SIN_FUNC(a1) * frontLerp;
			a2 = (vs2->normal & 255) / 255.0f;
			b2 = ((vs2->normal >> 8) & 255) / 255.0f;
			sa2 = SIN_FUNC(a2) * currentEntity->backLerp;
			norm[0] = sa1 * COS_FUNC(b1) + sa2 * COS_FUNC(b2);
			norm[1] = sa1 * SIN_FUNC(b1) + sa2 * SIN_FUNC(b2);
			norm[2] = COS_FUNC(a1) * frontLerp + COS_FUNC(a2) * currentEntity->backLerp;
			ex->numVerts = 1;
			ex->axis = NULL;
			ex->dlight = NULL;
		}
	}
	else
		for (i = 0; i < surf->numVerts; i++, vs1++, v++, ex++)
		{
			float	a, sa, b, *norm;

			v->xyz[0] = vs1->xyz[0] * MD3_XYZ_SCALE;
			v->xyz[1] = vs1->xyz[1] * MD3_XYZ_SCALE;
			v->xyz[2] = vs1->xyz[2] * MD3_XYZ_SCALE;
			norm = ex->normal;
			a = (vs1->normal & 255) / 255.0f;
			b = ((vs1->normal >> 8) & 255) / 255.0f;
			sa = SIN_FUNC(a);
			norm[0] = sa * COS_FUNC(b);			// sin(a)*cos(b)
			norm[1] = sa * SIN_FUNC(b);			// sin(a)*sin(b)
			norm[2] = COS_FUNC(a);				// cos(a)
			ex->numVerts = 1;
			ex->axis = NULL;
			ex->dlight = NULL;
		}

	c = &srcVertexColor[firstVert].rgba;
	for (i = 0; i < gl_numVerts; i++, c++)
		*c = 0xFFFFFFFF;	//!! vertex color (should not use "RGBGEN_VERTEX", or use another function to generate color; see UberEngine)

	/*----------- copy texcoords -------------*/
	t = &srcTexCoord[firstVert];
	ts = surf->texCoords;
	for (i = 0; i < surf->numVerts; i++, t++)
	{
		t->tex[0] = *ts++;
		t->tex[1] = *ts++;
	}

	/*------------ copy indexes --------------*/
	idx = &gl_indexesArray[firstIndex];
	idxSrc = surf->indexes;
	for (i = 0; i < numIdx; i++)
		*idx++ = *idxSrc++ + firstVert;
}


/*-------------- Debug output ----------------*/


static void FlashColor (void)
{
	int		i;

	i = Q_round (ap.time / 3 * TABLE_SIZE);
	qglColor3f (sinTable[i & TABLE_MASK] / 2 + 0.5,
				sinTable[i + 100 & TABLE_MASK] / 2 + 0.5,
				sinTable[600 - i & TABLE_MASK] / 2 + 0.5);
}


#define BBOX_WORLD	1	// debug?
// draw entity bounding boxes
static void DrawBBoxes (void)
{
	int		i;
	refEntity_t *ent;

	// common GL setup
#ifdef BBOX_WORLD
	qglLoadMatrixf (&ap.modelMatrix[0][0]);		// world matrix
#endif
	GL_SetMultitexture (0);		// disable texturing with all tmu's
	qglDisableClientState (GL_COLOR_ARRAY);

	if (gl_showbboxes->integer == 2)
		FlashColor ();
	else
		qglColor3f (0.6, 0.6, 0.2);
	GL_State (0);

	/*-------- draw bounding boxes --------*/
	for (i = 0, ent = gl_entities + ap.firstEntity; i < ap.numEntities; i++, ent++)
	{
		model_t		*m;
		float		*mins, *maxs;
		int			j;
		bufVertex_t v[8];
		static int inds[24] = {
			0,1, 1,3, 3,2, 2,0,	// rect1
			4,5, 5,7, 7,6, 6,4,	// rect2
			0,4, 1,5, 3,7, 2,6	// connectors
		};

		if (!ent->visible)
			continue;			// entity is culled or from different scene

		if (!(m = ent->model)) continue;	// no bbox info

		// get bbox data
		switch (m->type)
		{
		case MODEL_INLINE:
			mins = m->inlineModel->mins;
			maxs = m->inlineModel->maxs;
			break;
		case MODEL_MD3:
			mins = m->md3->frames[ent->frame].mins;
			maxs = m->md3->frames[ent->frame].maxs;
			break;
		default:
			continue;	// do not draw
		}

		// generate verts
		for (j = 0; j < 8; j++)
		{
			float	x, y, z;
			vec3_t	tmp;

			x = (j & 1) ? maxs[0] : mins[0];
			y = (j & 2) ? maxs[1] : mins[1];
			z = (j & 4) ? maxs[2] : mins[2];

#ifdef BBOX_WORLD
			// project point to a world coordinate system (org + x*axis[0] + y*axis[1] + z*axis[2])
			if (!ent->worldMatrix)
			{
				VectorMA (ent->origin, x, ent->axis[0], tmp);
				VectorMA (tmp,		   y, ent->axis[1], tmp);
				VectorMA (tmp,		   z, ent->axis[2], v[j].xyz);
			}
			else
			{
				v[j].xyz[0] = x; v[j].xyz[1] = y; v[j].xyz[2] = z;
			}
#else
			v[j].xyz[0] = x; v[j].xyz[1] = y; v[j].xyz[2] = z;
#endif
		}

		// draw it
#ifndef BBOX_WORLD
		qglLoadMatrixf (&ent->modelMatrix[0][0]);
#endif
		if (gl_showbboxes->integer == 3)
		{
			if (!ent->worldMatrix)
				qglColor3f (0.6, 0.6, 0.2);
			else
				qglColor3f (0.2, 0.8, 0.2);
		}
		qglVertexPointer (3, GL_FLOAT, sizeof(bufVertex_t), v);
		qglDrawElements (GL_LINES, 24, GL_UNSIGNED_INT, inds);
	}
}


static void DrawTriangles (void)
{
	gl_depthMode_t prevDepth;

	prevDepth = gl_state.currentDepthMode;
	GL_SetMultitexture (0);		// disable texturing
	if (gl_showtris->integer - 1 & 1)
		GL_State (GLSTATE_POLYGON_LINE|GLSTATE_DEPTHWRITE);	// use depth test
	else	// no depth test
	{
		GL_State (GLSTATE_POLYGON_LINE|GLSTATE_DEPTHWRITE);
		GL_DepthRange (DEPTH_NEAR);
	}
	// setup colors
	if (gl_showtris->integer - 1 & 2)
		FlashColor ();
	else
		qglColor3f (0, 0, 0);
	qglDisableClientState (GL_COLOR_ARRAY);
	// draw
	DrawArrays (gl_indexesArray, gl_numIndexes);
	// restore state
	GL_DepthRange (prevDepth);
}


static void DrawNormals (void)
{
	bufExtra_t *ex;
	bufVertex_t *vec;
	float	*norm;
	int		i, j;
	gl_depthMode_t prevDepth;

	prevDepth = gl_state.currentDepthMode;
	GL_SetMultitexture (0);		// disable texturing
	if (gl_shownormals->integer - 1 & 1)
		GL_State (GLSTATE_POLYGON_LINE|GLSTATE_DEPTHWRITE);	// use depth test
	else	// no depth test
	{
		GL_State (GLSTATE_POLYGON_LINE|GLSTATE_DEPTHWRITE);
		GL_DepthRange (DEPTH_NEAR);
	}
	// setup colors
	if (gl_shownormals->integer - 1 & 2)
		FlashColor ();
	else
		qglColor3f (0, 0, 0);
	qglDisableClientState (GL_COLOR_ARRAY);
	// draw
	qglBegin (GL_LINES);
	vec = vb->verts;
	for (i = 0, ex = gl_extra; i < gl_numExtra; i++, ex++)
	{
		norm = ex->normal;
		for (j = 0; j < ex->numVerts; j++, vec++)
		{
			vec3_t	vec2;

			qglVertex3fv (vec->xyz);
			VectorAdd (vec->xyz, ex->normal, vec2);
			qglVertex3fv (vec2);
		}
	}
	qglEnd ();
	// restore state
	GL_DepthRange (prevDepth);
}


static void TesselateEntitySurf (refEntity_t *e)
{
	if (e->flags & RF_BBOX)
	{
		int		i;
		bufVertex_t v[8];
		static int inds[24] = {
			0,1, 1,3, 3,2, 2,0,	// rect1
			4,5, 5,7, 7,6, 6,4,	// rect2
			0,4, 1,5, 3,7, 2,6	// connectors
		};

		GL_SetMultitexture (0);		// disable texturing with all tmu's
		qglDisableClientState (GL_COLOR_ARRAY);
		qglColor4ubv (e->shaderColor.c);
		GL_State (0);

		// generate verts
		for (i = 0; i < 8; i++)
		{
			float	x, y, z;
			vec3_t	tmp;

			x = (i & 1) ? e->boxSize[0] : -e->boxSize[0];
			y = (i & 2) ? e->boxSize[1] : -e->boxSize[1];
			z = (i & 4) ? e->boxSize[2] : -e->boxSize[2];

			// project point to a world coordinate system (org + x*axis[0] + y*axis[1] + z*axis[2])
			VectorMA (e->center, x, e->boxAxis[0], tmp);
			VectorMA (tmp,		 y, e->boxAxis[1], tmp);
			VectorMA (tmp,		 z, e->boxAxis[2], v[i].xyz);
		}
		// draw it
		qglVertexPointer (3, GL_FLOAT, sizeof(bufVertex_t), v);
		qglDrawElements (GL_LINES, 24, GL_UNSIGNED_INT, inds);
	}
	else
		DrawTextLeft (va("Unknown ent surf flags: %X", e->flags), 1, 0, 0);
}


/*---------- Fast drawing (without filling arrays) ----------*/
//?? fast drawing may use global vertex/color/texcoord array (filled when map loaded) and just operate with indexes
//!! In this case, indexes in surfaces must be global

//?? REMOVE THIS ??

static void DrawFastSurfaces (surfaceInfo_t **surfs, int numSurfs)
{
	int		i;
	shaderStage_t *stage, **pstage;

	LOG_STRING (va("*** FastIterator(%s, %d) ***\n", currentShader->name, numSurfs));
	for (i = 0, pstage = currentShader->stages; i < currentShader->numStages; i++, pstage++)
	{
		surfaceInfo_t **psurf;
		int		j;

		stage = *pstage;
		GL_State (stage->glState);
		GL_CullFace (currentShader->cullMode);
		// setup RGBA (if const)
		if (stage->rgbGenType == RGBGEN_CONST)
		{	// here: alphaGenFunc == ALPHAGEN_CONST (condition for shader.fast flag)
			qglDisableClientState (GL_COLOR_ARRAY);
			qglColor4ubv (stage->rgbaConst.c);
		}
		else
			qglEnableClientState (GL_COLOR_ARRAY);
		//---- BindStageImage (stage); ----
		if (stage->numAnimTextures == 1)
			GL_Bind (stage->mapImage[0]);
		else
			GL_Bind (stage->mapImage[Q_round(ap.time * stage->animMapFreq) % stage->numAnimTextures]);

		for (j = 0, psurf = surfs; j < numSurfs; j++, psurf++)
		{
			surfaceCommon_t *surf;
			surfacePlanar_t *pl;

			surf = (*psurf)->surf;
			switch (surf->type)
			{
			case SURFACE_PLANAR:
				pl = surf->pl;

				qglVertexPointer (3, GL_FLOAT, sizeof(vertex_t), pl->verts);
				if (stage->tcGenType == TCGEN_TEXTURE)
					qglTexCoordPointer (2, GL_FLOAT, sizeof(vertex_t), pl->verts[0].st);
				else
					qglTexCoordPointer (2, GL_FLOAT, sizeof(vertex_t), pl->verts[0].lm);
//				if (stage->rgbGenType == RGBGEN_EXACT_VERTEX) -- always for fast shaders
					qglColorPointer (4, GL_UNSIGNED_BYTE, sizeof(vertex_t), pl->verts[0].c.c);
				qglDrawElements (GL_TRIANGLES, pl->numIndexes, GL_UNSIGNED_INT, pl->indexes);
				break;
			}
		}
	}

	if (gl_finish->integer == 2)
		qglFinish ();
	gl_speeds.numIterators++;
}


/*------------------- Drawing the scene ---------------------*/

static void DrawParticles (particle_t *p)
{
	vec3_t	up, right;
	byte	c[4];

	//!! oprimize this (vertex arrays, etc.)

	GL_SetMultitexture (1);
	GL_Bind (gl_particleImage);

	GL_State (GLSTATE_SRC_SRCALPHA|GLSTATE_DST_ONEMINUSSRCALPHA|/*GLSTATE_DEPTHWRITE|*/GLSTATE_ALPHA_GT0);
	VectorScale (ap.viewaxis[1], 1.5, up);
	VectorScale (ap.viewaxis[2], 1.5, right);

	qglBegin (GL_TRIANGLES);
	for ( ; p; p = p->drawNext)
	{
		float	scale;
		int		alpha;

		scale = (p->org[0] - ap.vieworg[0]) * ap.viewaxis[0][0] +
				(p->org[1] - ap.vieworg[1]) * ap.viewaxis[0][1] +
				(p->org[2] - ap.vieworg[2]) * ap.viewaxis[0][2];		// get Z-coordinate
		scale *= ap.fov_scale;
		if (scale < 10)
			continue;		// too near
		if (scale < 20.0f)
			scale = 1;
		else
			scale = scale / 500.0f + 1.0f;

		alpha = p->alpha * 255;
		alpha = bound(alpha, 0, 255);

		switch (p->type)
		{
		case PT_SPARKLE:
			if (alpha < 64)
			{
				c[0] = 255;
				c[1] = 135 + alpha * 120 / 64;
				c[2] = 144;
			}
			else
			{
				c[0] = c[1] = 255;
				c[2] = (alpha - 64) * 4 / 3 * 111 / 256 + 144;
			}
			break;
		default:
			*(int*)c = gl_config.tbl_8to32[p->color];
		}
		c[3] = alpha;
		qglColor4ubv (c);

		qglTexCoord2f (0.0625, 0.0625);
		qglVertex3fv (p->org);

		qglTexCoord2f (1.0625, 0.0625);
		qglVertex3f (p->org[0] + up[0] * scale, p->org[1] + up[1] * scale, p->org[2] + up[2] * scale);

		qglTexCoord2f (0.0625, 1.0625);
		qglVertex3f (p->org[0] + right[0] * scale, p->org[1] + right[1] * scale, p->org[2] + right[2] * scale);
	}
	qglEnd ();
}


// Macro for determining fast-draw ability
#define IS_FAST(shader,surf)	(shader->fast && surf->type == SURFACE_PLANAR)

#define FLUSH()				\
	if (numFastSurfs)		\
	{						\
		DrawFastSurfaces (fastSurf, numFastSurfs);	\
		numFastSurfs = 0;	\
	}						\
	else					\
		FlushArrays ();

// Draw scene, specified in "ap"
static void DrawScene (void)
{
	int				index, index2, numSkySurfs, numFastSurfs;
	surfaceInfo_t	**si, **si2, **fastSurf;
	shader_t		*shader;
	surfaceCommon_t	*surf;
	// current state
	int		currentShaderNum, currentEntityNum;
	qboolean currentWorld, isWorld;

	LOG_STRING (va("******** R_DrawScene: (%g, %g) - (%g, %g) ********\n", ap.x, ap.y, ap.x+ap.w, ap.y+ap.h));

	if (gl_numVerts) FlushArrays ();
	GL_Setup (&ap);

	// sort surfaces
	if (gl_finish->integer == 2) qglFinish ();
	gl_speeds.beginSort = Sys_Milliseconds ();
	GL_SortSurfaces (&ap, sortedSurfaces);
	gl_speeds.begin3D = Sys_Milliseconds ();

	currentDlightMask = 0;

	/*------------ draw sky --------------*/

	GL_ClearSkyBox ();
	numSkySurfs = 0;

	si = sortedSurfaces;
	for (index = 0; index < ap.numSurfaces; index++, si++)
	{
		surf = (*si)->surf;
		shader = GL_GetShaderByNum (((*si)->sort >> SHADERNUM_SHIFT) & SHADERNUM_MASK);
		if (shader->type != SHADERTYPE_SKY) break;

		if (!index) SetCurrentShader (shader);

		if (!shader->skyRotate)
			GL_AddSkySurface (surf->pl, ap.vieworg);	//?? may be another types
		numSkySurfs++;
	}
	//?? may be, require to set dlightMask, currentShader, currentEntity etc.
	if (numSkySurfs) DrawSkyBox ();

	// update r_speeds values
	gl_speeds.surfs += ap.numSurfaces - index;	// numSurfaces - numSkySurfaces

	/*--- update all dynamic lightmaps ---*/

	for (index2 = index, si2 = si; index2 < ap.numSurfaces; index2++, si2++)
	{
		surf = (*si2)->surf;
		if (surf && surf->type == SURFACE_PLANAR)
			CheckDynamicLightmap (surf);
	}

	/*-------- draw world/models ---------*/

//??	if (!(ap.flags & RDF_NOWORLDMODEL))	GL_EnableFog (true);

	numFastSurfs = 0;
	currentShaderNum = currentEntityNum = -1;
	currentWorld = false;
	for (/* continue */; index < ap.numSurfaces; index++, si++)
	{
		int		shNum, entNum;
		int		code;
		unsigned dlightMask;

		surf = (*si)->surf;
		code = (*si)->sort;
		shNum = (code >> SHADERNUM_SHIFT) & SHADERNUM_MASK;
		entNum = (code >> ENTITYNUM_SHIFT) & ENTITYNUM_MASK;
		if ((code >> DLIGHTNUM_SHIFT) & DLIGHTNUM_MASK)
			dlightMask = surf->dlightMask;
		else
			dlightMask = 0;

		if (shNum != currentShaderNum || entNum != currentEntityNum || currentDlightMask != dlightMask)
		{
			// flush data for the previous shader
			FLUSH();

			// change shader
			shader = GL_GetShaderByNum (shNum);
			SetCurrentShader (shader);
			currentDlightMask = dlightMask;
			LOG_STRING (va("******** Change shader to %s ********\n", shader->name));
			currentShaderNum = shNum;

			if (IS_FAST(shader, surf))
			{
				numFastSurfs = 0;
				fastSurf = si;
			}
		}

		if (entNum != currentEntityNum)
		{
			currentEntity = &gl_entities[entNum];
			currentEntityNum = entNum;

			isWorld = (entNum == ENTITYNUM_WORLD) || currentEntity->worldMatrix;

			if (isWorld)
			{
				if (!currentWorld)		// previous entity was not world
				{
					//?? set shader.time to ap.time
					LOG_STRING (va("******** Change entity to WORLD ********\n"));
					qglLoadMatrixf (&ap.modelMatrix[0][0]);
				}
				gl_state.inverseCull = false;
				GL_DepthRange (DEPTH_NORMAL);
			}
			else
			{
				//?? set shader.time to ap.time - entity.time
				LOG_STRING (va("******** Change entity to %s ********\n", currentEntity->model->name));
				qglLoadMatrixf (&currentEntity->modelMatrix[0][0]);
				gl_state.inverseCull = currentEntity->mirror;
				GL_DepthRange (currentEntity->flags & RF_DEPTHHACK ? DEPTH_HACK : DEPTH_NORMAL);
			}
			currentWorld = isWorld;
		}

		if (IS_FAST(shader, surf))
			numFastSurfs++;
		else
		{
			switch (surf->type)
			{
			case SURFACE_PLANAR:
				TesselatePlanarSurf (surf->pl);
				break;
			case SURFACE_MD3:
				TesselateMd3Surf (surf->md3);
				break;
			case SURFACE_PARTICLE:
				DrawParticles (surf->part);
				break;
			case SURFACE_POLY:
				TesselatePolySurf (surf->poly);
				break;
			case SURFACE_ENTITY:
				TesselateEntitySurf (surf->ent);
				break;
			//!! other types
			}
		}
	}

	/*--------- finilize/debug -----------*/
	FLUSH();

	GL_EnableFog (false);	//??
	GL_DepthRange (DEPTH_NORMAL);

	if (gl_showbboxes->integer)
		DrawBBoxes ();

	if (gl_showLights->integer)
		GL_ShowLights ();

	if (gl_finish->integer == 2) qglFinish ();
	gl_speeds.begin2D = Sys_Milliseconds ();
}


static void TesselateStretchPic (bkDrawPic_t *p)
{
	bufVertex_t	*v;
	bufTexCoordSrc_t *t;
	int			*idx, idx0;
	int			*c;

	if (currentShader != p->shader)
	{
		FlushArrays ();
		SetCurrentShader (p->shader);
	}

	GL_Set2DMode ();

	ReserveVerts (4, 6);

	/*   0 (0,0) -- 1 (1,0)
	 *     |   \____   |
	 *     |        \  |
	 *   3 (0,1) -- 2 (1,1)
	 */
	v = &vb->verts[gl_numVerts];
	t = &srcTexCoord[gl_numVerts];
	c = &srcVertexColor[gl_numVerts].rgba;
	idx0 = gl_numVerts;
	gl_numVerts += 4;

	// set vert.z
	v[0].xyz[2] = v[1].xyz[2] = v[2].xyz[2] = v[3].xyz[2] = 0;
	// set vert.x
	v[0].xyz[0] = v[3].xyz[0] = p->x;
	v[1].xyz[0] = v[2].xyz[0] = p->x + p->w;
	// set vert.y
	v[0].xyz[1] = v[1].xyz[1] = p->y;
	v[2].xyz[1] = v[3].xyz[1] = p->y + p->h;
	// set s
	t[0].tex[0] = t[3].tex[0] = p->s1;
	t[1].tex[0] = t[2].tex[0] = p->s2;
	// set t
	t[0].tex[1] = t[1].tex[1] = p->t1;
	t[2].tex[1] = t[3].tex[1] = p->t2;
	// store colors
	c[0] = c[1] = c[2] = c[3] = p->c.rgba;

	idx = &gl_indexesArray[gl_numIndexes];
	gl_numIndexes += 6;
	*idx++ = idx0+0; *idx++ = idx0+1; *idx++ = idx0+2;
	*idx++ = idx0+0; *idx++ = idx0+2; *idx++ = idx0+3;
}


// This is slightly optimized version of TesselateStretchPic() for drawing texts
void TesselateText (bkDrawText_t *p)
{
	bufVertex_t	*v;
	bufTexCoordSrc_t *t;
	int		*c;
	int		i, *idx, idx0;
	float	x, xp, size;
	char	*s, chr;

	if (currentShader != gl_concharsShader)
	{
		FlushArrays ();
		SetCurrentShader (gl_concharsShader);
	}

	GL_Set2DMode ();

	ReserveVerts (p->len * 4, p->len * 6);

	v = &vb->verts[gl_numVerts];
	t = &srcTexCoord[gl_numVerts];
	c = &srcVertexColor[gl_numVerts].rgba;
	idx = &gl_indexesArray[gl_numIndexes];
	idx0 = gl_numVerts;

	s = p->text;
	xp = p->x;
	x = p->x + p->w;

	size = 1.0f / 16;
	for (i = p->len; i > 0 && xp < vid.width; i--)
	{
		chr = *s++;
		if (chr != ' ')		// space is hardcoded
		{
			float	s1, t1, s2, t2;

			s1 = (chr & 15) * size;
			t1 = (chr >> 4 & 15) * size;
			s2 = s1 + size;
			t2 = t1 + size;

			// set vert.z
			v[0].xyz[2] = v[1].xyz[2] = v[2].xyz[2] = v[3].xyz[2] = 0;
			// set vert.x
			v[0].xyz[0] = v[3].xyz[0] = xp;
			v[1].xyz[0] = v[2].xyz[0] = x;
			// set vert.y
			v[0].xyz[1] = v[1].xyz[1] = p->y;
			v[2].xyz[1] = v[3].xyz[1] = p->y + p->h;
			// set s
			t[0].tex[0] = t[3].tex[0] = s1;
			t[1].tex[0] = t[2].tex[0] = s2;
			// set t
			t[0].tex[1] = t[1].tex[1] = t1;
			t[2].tex[1] = t[3].tex[1] = t2;
			// store colors
			c[0] = c[1] = c[2] = c[3] = p->c.rgba;

			*idx++ = idx0+0; *idx++ = idx0+1; *idx++ = idx0+2;
			*idx++ = idx0+0; *idx++ = idx0+2; *idx++ = idx0+3;

			idx0 += 4;
			v += 4;
			t += 4;
			c += 4;
			gl_numVerts += 4;
			gl_numIndexes += 6;
		}
		xp = x;
		x += p->w;
	}
}


void GL_BackEnd (void)
{
	int		*p;
#if 0
	static int max = 0;

	// measure maximal backend command buffer size
	if (backendCmdSize > max) max = backendCmdSize;
	Cvar_SetInteger ("backend", max);
#endif
	* ((int*) &backendCommands[backendCmdSize]) = BACKEND_STOP;
	backendCmdSize = 0;
	lastBackendCommand = NULL;

	p = (int*) &backendCommands[0];
	while (*p != BACKEND_STOP)
	{
		switch (*p)
		{
		case BACKEND_PIC:
			{
				bkDrawPic_t *p1;

				p1 = (bkDrawPic_t *) p;
				TesselateStretchPic (p1);
				p = (int *) ++p1;
			}
			break;

		case BACKEND_TEXT:
			{
				bkDrawText_t *p1;

				p1 = (bkDrawText_t *) p;
				TesselateText (p1);
				p = (int *) ((byte *)p1 + sizeof(bkDrawText_t) + p1->len);
			}
			break;

		case BACKEND_BEGIN_FRAME:
#ifdef SWAP_ON_BEGIN
			GLimp_EndFrame ();
			if (gl_finish->integer) qglFinish ();
#endif

			qglDrawBuffer (stricmp (gl_drawbuffer->string, "GL_FRONT") ? GL_BACK : GL_FRONT);
			if (gl_config.consoleOnly)
			{
				qglClearColor (0, 0.2, 0, 1);
				qglClear (GL_COLOR_BUFFER_BIT);
			}
			else if (gl_clear->integer)
			{
				qglClearColor (0.1, 0.6, 0.3, 1);
				qglClear (GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
			}
			p++;
			break;

		case BACKEND_END_FRAME:
			FlushArrays ();
			if (gl_screenshotName)
				GL_PerformScreenshot ();
			if (strcmp (gl_showImages->string, "0"))
				GL_ShowImages ();

#ifndef SWAP_ON_BEGIN
			if (gl_finish->integer) qglFinish ();
			GLimp_EndFrame ();
#endif
			gl_state.is2dMode = false;	// invalidate 2D mode, because of buffer switching
			p++;
			break;

		case BACKEND_DRAW_FRAME:
			{
				bkDrawFrame_t *p1;

				p1 = (bkDrawFrame_t *) p;
				memcpy (&ap, &p1->portal, sizeof(viewPortal_t));
				DrawScene ();
				p = (int *) ++p1;
			}
			break;
		}
	}
}


/*--------------- Init/shutdown --------------*/

void GL_InitBackend (void)
{
	GL_ClearBuffers ();

	vbSize = sizeof(vertexBuffer_t) + gl_config.maxActiveTextures * sizeof(bufTexCoord_t) * MAX_VERTEXES;
	vb = Z_Malloc (vbSize);
	Com_Printf("^1 **** buf: %08X (%d bytes) ****\n", vb, vbSize);//!!
}


void GL_ShutdownBackend (void)
{
	Z_Free (vb);
	vb = NULL;
}
