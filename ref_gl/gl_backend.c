#include "gl_local.h"
#include "gl_shader.h"
#include "gl_backend.h"
#include "gl_model.h"

//#define SWAP_ON_BEGIN		// call GLimp_EndFrame() with SwapBuffers() on frame begin (or frame end if not defined)

/* LATER: replace (almost) all ap.time -> shader.time (or global "shaderTime") ??
 * (because time may be take (if engine will be extended) from entity)
 */


typedef struct
{
	float	tex[2];
	float	lm[2];
} bufTexCoordSrc_t;

typedef struct
{
	float	tex[2];
} bufTexCoord_t;

typedef struct
{
	float	xyz[3];
	float	pad;
} bufVertex_t;

// some data may be grouped by few vertexes
typedef struct
{
	int		numVerts;
	int		lmWidth;
	vec3_t	normal;
	surfDlight_t *dlight;
//??	add data for TCGEN_ENVIRONMENT
} bufExtra_t;


/*------------- Command buffer ---------------*/

byte	backendCommands[MAX_BACKEND_COMMANDS];
int		backendCmdSize;
byte	*lastBackendCommand;

static shader_t		*currentShader;
static refEntity_t	*currentEntity;
static int			currentDlightMask;


/*-------------- Vertex arrays ---------------*/


// per-shader limits
#define MAX_VERTEXES	4096		// 1024 in Q3, 2048 in Q2, but have VALID model with more than 2048 verts
#define MAX_INDEXES		(MAX_VERTEXES*6)

#define NUM_VERTEX_BUFFERS	64

typedef struct
{
	bufVertex_t		verts[MAX_VERTEXES];
	color_t			color[MAX_VERTEXES];
	bufTexCoord_t	texCoord[0][MAX_VERTEXES];
} vertexBuffer_t;

static vertexBuffer_t	*vb, *agpBuffer;// pointers to current buffer and to all buffers
static int				currentBuffer;	// index of the current buffer
static int				vbSize;			// size of 1 buffer (depends on multitexturing ability)

static GLuint fences[NUM_VERTEX_BUFFERS];

static void WaitVertexArray (void)
{
	if (qglVertexArrayRangeNV)
	{
		qglFinishFenceNV (fences[currentBuffer]);
	}
}

static void SwitchVertexArray (void)
{
	if (qglVertexArrayRangeNV)
	{
		qglSetFenceNV (fences[currentBuffer], GL_ALL_COMPLETED_NV);
		if (++currentBuffer == NUM_VERTEX_BUFFERS)
			currentBuffer = 0;
		vb = agpBuffer + currentBuffer;
	}
}


static int				indexesArray[MAX_INDEXES];
static color_t			srcVertexColor[MAX_VERTEXES];
static bufTexCoordSrc_t	srcTexCoord[MAX_VERTEXES];
static bufExtra_t		extra[MAX_VERTEXES];

static int numVerts, numIndexes, numExtra;


/*--- Tables for fast periodic function computation ---*/

#define TABLE_SIZE	1024
#define TABLE_MASK	(TABLE_SIZE-1)

static float sinTable[TABLE_SIZE], squareTable[TABLE_SIZE], triangleTable[TABLE_SIZE],
			 sawtoothTable[TABLE_SIZE], inverseSwatoothTable[TABLE_SIZE];

static float *mathFuncs[5] = {
	sinTable, squareTable, triangleTable, sawtoothTable, inverseSwatoothTable
};

#define PERIODIC_FUNC(tbl,val)		tbl[Q_ftol((val)*TABLE_SIZE) & TABLE_MASK]


static float sqrtTable[256];
static int   noiseTablei[256];
static float noiseTablef[256];


static void InitFuncTables (void)
{
	int		i, n;

	for (i = 0; i < TABLE_SIZE; i++)
	{
		float	j;

		j = i;			// just to avoid "(float)i"
		if (i < TABLE_SIZE/2)
			squareTable[i] = -1;
		else
			squareTable[i] = 1;

		sinTable[i] = sin (j / (TABLE_SIZE/2) * M_PI);		// 0 -- 0, last -- 2*pi
		sawtoothTable[i] = j / TABLE_SIZE;
		inverseSwatoothTable[i] = 1 - j / TABLE_SIZE;

		n = (i + TABLE_SIZE/4) & TABLE_MASK;				// make range -1..1..-1
		if (n >= TABLE_SIZE/2)
			n = TABLE_SIZE - n;
		triangleTable[i] = (float)(n - TABLE_SIZE/4) / (TABLE_SIZE/4);
	}

	for (i = 0; i < 256; i++)
	{
		sqrtTable[i] = pow (i / 255.0f, 0.5f);
		n = rand ();
		noiseTablei[i] = (int)(n * 255.0f / RAND_MAX) & 0xFF;
		n = rand ();
		noiseTablef[i] = ((float)n / RAND_MAX) * 2.0f - 1.0f;	// range -1..1
	}
}


/*------------------ Scene -----------------*/

// surfaces
static surfaceInfo_t *sortedSurfaces[MAX_SCENE_SURFACES];


/*--------------- Draw portal --------------*/

static viewPortal_t ap;	// active portal


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
			for (j = 0, ex = extra; j < numExtra; j++, ex++)
			{
				norm = ex->normal;
				for (k = 0; k < ex->numVerts; k++, vec++)
				{
					f = PERIODIC_FUNC(table, t + deform->waveDiv * (vec->xyz[0] + vec->xyz[1] + vec->xyz[2]))
						* deform->wave.amp + deform->wave.base;
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

	/*---------- rgbGen -----------*/
	switch (st->rgbGenType)
	{
	case RGBGEN_CONST:
		rgba = st->rgbaConst.rgba;
		for (i = 0; i < numVerts; i++)
			vb->color[i].rgba = rgba;
		break;
	case RGBGEN_EXACT_VERTEX:
		for (i = 0; i < numVerts; i++)
			vb->color[i].rgba = srcVertexColor[i].rgba;
		break;
	case RGBGEN_VERTEX:
		for (i = 0; i < numVerts; i++)
		{
			vb->color[i].c[0] = srcVertexColor[i].c[0] >> gl_config.overbrightBits;
			vb->color[i].c[1] = srcVertexColor[i].c[1] >> gl_config.overbrightBits;
			vb->color[i].c[2] = srcVertexColor[i].c[2] >> gl_config.overbrightBits;
		}
		break;
	case RGBGEN_ONE_MINUS_VERTEX:
		for (i = 0; i < numVerts; i++)
		{
			vb->color[i].c[0] = (255 - srcVertexColor[i].c[0]) >> gl_config.overbrightBits;
			vb->color[i].c[1] = (255 - srcVertexColor[i].c[1]) >> gl_config.overbrightBits;
			vb->color[i].c[2] = (255 - srcVertexColor[i].c[2]) >> gl_config.overbrightBits;
		}
		break;
	// other types: LIGHTING_DIFFUSE, FOG
	}

	if (st->rgbGenType == RGBGEN_EXACT_VERTEX && st->alphaGenType == ALPHAGEN_VERTEX)
		return;	// alpha is already set

	/*--------- alphaGen ----------*/
	switch (st->alphaGenType)
	{
	case ALPHAGEN_CONST:
		a = st->rgbaConst.c[3];
		for (i = 0; i < numVerts; i++)
			vb->color[i].c[3] = a;
		break;
	case ALPHAGEN_VERTEX:
		for (i = 0; i < numVerts; i++)
			vb->color[i].c[3] = srcVertexColor[i].c[3];
		break;
	case ALPHAGEN_ONE_MINUS_VERTEX:
		for (i = 0; i < numVerts; i++)
			vb->color[i].c[3] = 255 - srcVertexColor[i].c[3];
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
		for (j = 0; j < numVerts; j++, src++, dst++)
		{
			dst->tex[0] = src->tex[0];
			dst->tex[1] = src->tex[1];
		}
		break;
	case TCGEN_LIGHTMAP:
		for (j = 0; j < numVerts; j++, src++, dst++)
		{
			dst->tex[0] = src->lm[0];
			dst->tex[1] = src->lm[1];
		}
		break;
	case TCGEN_LIGHTMAP1:
	case TCGEN_LIGHTMAP2:
	case TCGEN_LIGHTMAP3:
	case TCGEN_LIGHTMAP4:
		{
			float	shift;
			bufExtra_t *ex;

			for (j = 0, ex = extra; j < numExtra; j++, ex++)
			{
				shift = (float)(ex->lmWidth * (st->tcGenType - TCGEN_LIGHTMAP1 + 1)) / LIGHTMAP_SIZE;
				for (k = 0; k < ex->numVerts; k++, src++, dst++)
				{
					dst->tex[0] = src->lm[0] + shift;
					dst->tex[1] = src->lm[1];
				}
			}
		}
		break;
	// other types: ENVIRONMENT, VECTOR, ZERO (?), FOG (?)

	default:
		if (st->tcGenType >= TCGEN_DLIGHT0 && st->tcGenType < TCGEN_DLIGHT0 + MAX_DLIGHTS)
		{
			bufExtra_t *ex;
			bufVertex_t *vec;
			surfDlight_t *sdl;
			int		num;

			num = st->tcGenType - TCGEN_DLIGHT0;
			vec = vb->verts;
			for (j = 0, ex = extra; j < numExtra; j++, ex++)
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
			f1 = f1 - (int)f1;
			f2 = tcmod->tSpeed * ap.time;
			f2 = f2 - (int)f2;
			for (k = 0; k < numVerts; k++, dst++)
			{
				dst->tex[0] += f1;
				dst->tex[1] += f2;
			}
			break;
		case TCMOD_TURB:
			f1 = tcmod->wave.freq * ap.time + tcmod->wave.phase;
			f2 = tcmod->wave.amp;
			for (k = 0; k < numVerts; k++, dst++)
			{
				float	*vec, f;

				vec = &vb->verts[k].xyz[0];
				f = PERIODIC_FUNC(sinTable, (vec[0] + vec[1] + vec[2]) / TABLE_SIZE + f1) * f2;
				dst->tex[0] += f;
				dst->tex[1] += f;
//				dst->tex[0] += PERIODIC_FUNC(sinTable, (vec[0] + vec[2]) / TABLE_SIZE + f1) * f2;	// Q3: vec[0] + vec[2]
//				dst->tex[1] += PERIODIC_FUNC(sinTable, (vec[1] + vec[2]) / TABLE_SIZE + f1) * f2;	// Q3: vec[1]
			}
			break;
		case TCMOD_WARP:
			for (k = 0; k < numVerts; k++, dst++)
			{
				f1 = dst->tex[0];
				f2 = dst->tex[1];
				dst->tex[0] = f1 / 64.0f + PERIODIC_FUNC(sinTable, (f2 / 16.0f + ap.time) / (2.0f * M_PI)) / 16.0f;
				dst->tex[1] = f2 / 64.0f + PERIODIC_FUNC(sinTable, (f1 / 16.0f + ap.time) / (2.0f * M_PI)) / 16.0f;
			}
			break;
		case TCMOD_SCALE:
			f1 = tcmod->sScale;
			f2 = tcmod->tScale;
			for (k = 0; k < numVerts; k++, dst++)
			{
				dst->tex[0] *= f1;
				dst->tex[1] *= f2;
			}
			break;
		// other types: TRANSFORM, ENTITYTRANSLATE, STRETCH, ROTATE
		}
	}
}


/*----------------- Shader visualization -------------------*/


// should rename "temp", "tmp" ??
typedef struct
{
	int		texEnv;			// REPLACE/MODILATE/ADD or COMBINE/COMBINE_NV (uses st.glState.blend ??)
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

static void PreprocessShader (shader_t *sh)
{
	int		firstStage, i, debugMode, glState;
	shaderStage_t *stage, **pstage, *lmStage;
	tempStage_t *st;
	renderPass_t *pass;
	int		tmuUsed, tmuLeft, passStyle;

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
			st->st.mapImage[0] = stage->mapImage[Q_ftol(ap.time * stage->animMapFreq) % stage->numAnimTextures];

		/*---------- fullbright/lightmap ---------------*/
		// fix glState if current stage is first after lightmap with r_fullbright!=0
		if (debugMode == DEBUG_FULLBRIGHT && glState != -1)
		{
			st->st.glState = glState;
			glState = -1;
		}
		if (debugMode == (DEBUG_LIGHTMAP|DEBUG_FULLBRIGHT) && !gl_state.is2dMode)
			st->st.glState |= GLSTATE_POLYGON_LINE;
		if (!i && currentShader->lightmapNumber == LIGHTMAP_VERTEX)
		{
			if (debugMode == DEBUG_LIGHTMAP) st->st.mapImage[0] = NULL;
			if (debugMode == DEBUG_FULLBRIGHT)
			{
				st->st.rgbGenType = RGBGEN_CONST;
				st->st.rgbaConst.rgba |= 0xFFFFFF;
			}
		}

		st++;
		numTmpStages++;
	}

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
				c1 = Q_ftol(PERIODIC_FUNC(mathFuncs[stage->rgbGenWave.type], P.freq * ap.time + P.phase) * P.amp + P.base);
#undef P
				if (c1 <= 0)		c2 = 0;
				else if (c1 >= 1)	c2 = 255;
				else				c2 = Q_ftol(c1 * 255.0f);
				st->st.rgbaConst.c[0] = st->st.rgbaConst.c[1] = st->st.rgbaConst.c[2] = c2;
				st->st.rgbGenType = RGBGEN_CONST;
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

	//!! reorder stages, add fog (?)

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
//	LOG_STRING(va("--- PreprocessShader(%s) ---\n", sh->name));
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
//			LOG_STRING("-- next pass\n");
		}
//		LOG_STRING(va("  tmu[%d:%d] = \"%s\" (rgba: %s %s %8X)\n", i, tmuUsed, st[0].st.mapImage[0]->name,
//			st[0].constRGBA ? "const" : "var", st[0].identityRGBA ? "ident" : "--", st[0].st.rgbaConst.rgba));

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
//				LOG_STRING(va("  MT(MUL): with \"%s\"\n", st[1].st.mapImage[0]->name));
				st[1].texEnv = TEXENV_MODULATE;
				pass->numStages++;
				continue;
			}

			if (GL_SUPPORT(QGL_EXT_TEXTURE_ENV_ADD|QGL_ARB_TEXTURE_ENV_ADD) &&
				blend2 == (GLSTATE_SRC_ONE|GLSTATE_DST_ONE) && passStyle & BLEND_ADDITIVE)
			{
//				LOG_STRING(va("  MT(ADD): with \"%s\"\n", st[1].st.mapImage[0]->name));
				st[1].texEnv = TEXENV_ADD;
				pass->numStages++;
				continue;
			}

			if (GL_SUPPORT(QGL_ARB_TEXTURE_ENV_COMBINE) && blend2 == (GLSTATE_SRC_DSTCOLOR|GLSTATE_DST_SRCCOLOR)
				&& passStyle & BLEND_MULTIPLICATIVE)
			{
//				LOG_STRING(va("  MT(MUL2): with \"%s\"\n", st[1].st.mapImage[0]->name));
				st[1].texEnv = TEXENV_MODULATE|TEXENV_MUL2;
				pass->numStages++;
				continue;
			}
		}
		//!! texenv_combine4 should be here

		if (GL_SUPPORT(QGL_ARB_TEXTURE_ENV_COMBINE) && blend2 == (GLSTATE_SRC_ONE|GLSTATE_DST_ONE)
			&& passStyle & BLEND_ADDITIVE && st[0].constRGBA && st[1].constRGBA && tmuUsed == 1)
		{
			int		k;

//			LOG_STRING(va("  MT(INTERP*2): with \"%s\"\n", st[1].st.mapImage[0]->name));
			st[1].texEnv = TEXENV_INTERP|TEXENV_MUL2|TEXENV_COLOR;
			// set RGBA for both stages
//			LOG_STRING(va("  (old rgba: %X %X", st[0].st.rgbaConst.rgba, st[1].st.rgbaConst.rgba));
			for (k = 0; k < 4; k++)
			{
				int		k2;

				k2 = st[1].st.rgbaConst.c[k];
				st[0].st.rgbaConst.c[k] = st[0].st.rgbaConst.c[k] * 255 / (255*2 - k2);
				st[1].st.rgbaConst.c[k] = 255 - k2 / 2;
			}
//			LOG_STRING(va(" new rgba: %X %X)\n",  st[0].st.rgbaConst.rgba, st[1].st.rgbaConst.rgba));
			pass->numStages++;
			continue;
		}

		// not combined - begin new pass
		tmuLeft = 0;
	}
//	LOG_STRING("-----------------\n");
}


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


static void StageIterator (void)
{
	int		i;
	renderPass_t *pass;

//	DrawTextLeft (va("StageIterator(%s, %d, %d)\n", currentShader->name, numVerts, numIndexes),1,1,1);//!!!
	LOG_STRING (va("*** StageIterator(%s, %d, %d) ***\n", currentShader->name, numVerts, numIndexes));

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
		qglLockArraysEXT (0, numVerts);
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
		DrawArrays (indexesArray, numIndexes);
	}

	/*----------------- finalize ------------------*/

	if (numRenderPasses > 1 && qglUnlockArraysEXT)
		qglUnlockArraysEXT ();

	if (currentShader->usePolygonOffset)
		qglDisable (GL_POLYGON_OFFSET_FILL);

	gl_speeds.tris += numIndexes * numTmpStages / 3;
	gl_speeds.trisMT += numIndexes * numRenderPasses / 3;
	gl_speeds.numIterators++;
}


static void SetCurrentShader (shader_t *shader)
{
	if (numVerts && numIndexes)
		// we can get situation, when verts==2 and inds==0 due to geometry simplification -- this is OK (??)
		// sample: map "actmet", inside building with rotating glass doors, floor 2: exotic lamp (nested cilinders with alpha)
	{
		DrawTextLeft ("SetCurrentShader() without flush!",1,0,0);
		Com_WPrintf ("SetCurrentShader(%s) without flush (old: %s, %d verts, %d inds)\n",
			shader->name, currentShader->name, numVerts, numIndexes);
	}
	currentShader = shader;
	numVerts = numIndexes = numExtra = 0;		// clear buffer
	//?? setup shader time
}


static void FlushArrays (void)
{
//	DrawTextLeft(va("FlushArrays(%s,%d,%d)\n",currentShader->name,numVerts,numIndexes),1,1,1);
	if (!numIndexes) return;	// buffer is empty

	StageIterator ();
	SwitchVertexArray ();
	WaitVertexArray ();

	numVerts = numIndexes = numExtra = 0;
}


static void CheckOverflow (int verts, int inds)
{
	if (numIndexes + inds > MAX_INDEXES || numVerts + verts > MAX_VERTEXES)
		FlushArrays ();

	if (verts > MAX_VERTEXES)
		Com_Error (ERR_DROP, "R_CheckOverflow: verts > MAX_VERTEXES (%d)", verts);
	if (inds > MAX_INDEXES)
		Com_Error (ERR_DROP, "R_CheckOverflow: inds > MAX_INDEXES (%d)", inds);
}


/*---------------- Skybox ------------------*/


static float skyMins[2][6], skyMaxs[2][6];


#define	ON_EPSILON		0.1			// point on plane side epsilon
#define	MAX_CLIP_VERTS	64

typedef enum {SIDE_FRONT, SIDE_BACK, SIDE_ON};

static void ClipSkyPolygon (int numVerts, vec3_t verts, int stage)
{
	float	*norm;
	float	*vec;
	qboolean	front, back;
	float	dists[MAX_CLIP_VERTS];
	int		sides[MAX_CLIP_VERTS];
	vec3_t	newv[2][MAX_CLIP_VERTS];	// new polys
	int		newc[2];					// number of verts in new polys
	int		i;

	static vec3_t skyClip[6] = {
		{ 1, 1, 0},
		{ 1,-1, 0},
		{ 0,-1, 1},
		{ 0, 1, 1},
		{ 1, 0, 1},
		{-1, 0, 1}
	};

	static int vecToSt[6][3] = {	// s = [0]/[2], t = [1]/[2]
		{-2, 3, 1},
		{ 2, 3,-1},
		{ 1, 3, 2},
		{-1, 3,-2},
		{-2,-1, 3},
		{-2, 1,-3}
	};


	if (!numVerts) return;			// empty polygon

	if (numVerts > MAX_CLIP_VERTS - 2)
		Com_Error (ERR_DROP, "ClipSkyPolygon: MAX_CLIP_VERTS hit");

	if (stage == 6)
	{	// fully clipped -- update skymins/skymaxs
		int		axis;
		vec3_t	av, v;
		float	*vp;

		// decide which face it maps to
		VectorClear (v);
		for (i = 0, vp = verts; i < numVerts; i++, vp += 3)
			VectorAdd (vp, v, v);
		av[0] = fabs(v[0]);
		av[1] = fabs(v[1]);
		av[2] = fabs(v[2]);
		// Here: v = sum vector, av = abs(v)
		if (av[0] > av[1] && av[0] > av[2])
		{
			if (v[0] < 0)	axis = 1;
			else			axis = 0;
		}
		else if (av[1] > av[2] && av[1] > av[0])
		{
			if (v[1] < 0)	axis = 3;
			else			axis = 2;
		}
		else
		{
			if (v[2] < 0)	axis = 5;
			else			axis = 4;
		}

		// project new texture coords
		for (i = 0; i < numVerts; i++, verts += 3)
		{
			int		j;
			float	s, t, dv;

			j = vecToSt[axis][2];
			if (j < 0)	dv = -verts[-j - 1];
			else		dv = verts[j - 1];

			if (dv < 0.001f) continue;	// don't divide by zero

			j = vecToSt[axis][0];
			if (j < 0)	s = -verts[-j - 1] / dv;
			else		s = verts[j - 1] / dv;

			j = vecToSt[axis][1];
			if (j < 0)	t = -verts[-j - 1] / dv;
			else		t = verts[j - 1] / dv;

			if (s < skyMins[0][axis]) skyMins[0][axis] = s;
			if (s > skyMaxs[0][axis]) skyMaxs[0][axis] = s;
			if (t < skyMins[1][axis]) skyMins[1][axis] = t;
			if (t > skyMaxs[1][axis]) skyMaxs[1][axis] = t;
		}
		return;
	}

	front = back = false;
	norm = skyClip[stage];
	for (i = 0, vec = verts; i < numVerts; i++, vec += 3)
	{
		float	d;

		d = DotProduct (vec, norm);
		if (d > ON_EPSILON)
		{
			front = true;
			sides[i] = SIDE_FRONT;
		}
		else if (d < -ON_EPSILON)
		{
			back = true;
			sides[i] = SIDE_BACK;
		}
		else
			sides[i] = SIDE_ON;
		dists[i] = d;
	}

	if (!front || !back)
	{	// not clipped
		ClipSkyPolygon (numVerts, verts, stage + 1);
		return;
	}

	// clip it
	sides[i] = sides[0];
	dists[i] = dists[0];
	VectorCopy (verts, (verts + (i * 3)));
	newc[0] = newc[1] = 0;

	for (i = 0, vec = verts; i < numVerts; i++, vec += 3)
	{
		int		j;
		float	d;

		switch (sides[i])
		{
		case SIDE_FRONT:
			VectorCopy (vec, newv[0][newc[0]]);
			newc[0]++;	// cannot insert this command to VectorCopy(), because it is a macro ...
			break;
		case SIDE_BACK:
			VectorCopy (vec, newv[1][newc[1]]);
			newc[1]++;
			break;
		case SIDE_ON:
			VectorCopy (vec, newv[0][newc[0]]);
			newc[0]++;
			VectorCopy (vec, newv[1][newc[1]]);
			newc[1]++;
			break;
		}

		if (sides[i] == SIDE_ON || sides[i+1] == SIDE_ON || sides[i+1] == sides[i])
			continue;		// line placed on one side of clip plane

		// line intersects clip plane: split line to 2 parts by adding new point
		d = dists[i] / (dists[i] - dists[i+1]);
		for (j = 0; j < 3; j++)
		{
			float	e;

			e = vec[j] + d * (vec[j + 3] - vec[j]);
			newv[0][newc[0]][j] = e;
			newv[1][newc[1]][j] = e;
		}
		newc[0]++;
		newc[1]++;
	}

	// process new polys
	if (newc[0]) ClipSkyPolygon (newc[0], newv[0][0], stage + 1);
	if (newc[1]) ClipSkyPolygon (newc[1], newv[1][0], stage + 1);
}


static void ClearSkyBox (void)
{
	int		i;

	// clear bounds for all sky box planes
	for (i = 0; i < 6; i++)
	{
		skyMins[0][i] = skyMins[1][i] = 9999;
		skyMaxs[0][i] = skyMaxs[1][i] = -9999;
	}
}


static void AddSkySurfaces (void)
{
	vec3_t	triangle[4];	// fill 3 points, other (1?) -- filled by ClipSkyPolygon()
	int		index;

	if (gl_fastsky->integer)
		return;

	index = 0;
	while (index < numIndexes)
	{
		int		i;

		// create triangle
		for (i = 0; i < 3; i++)
		{
			VectorSubtract (vb->verts[indexesArray[index]].xyz, ap.vieworg, triangle[i]);
			index++;	// cannot put it to a line above: VectorSubtract() is a macro
		}
		// send triangle to ClipSkyPolygon()
		ClipSkyPolygon (3, triangle[0], 0);
	}
}


// In: s, t in range [-1..1]
static void MakeSkyVec (float s, float t, int axis, float *tex, vec3_t vec)
{
	vec3_t		b;
	int			i;

	static int stToVec[6][3] = {	// 1 = s, 2 = t, 3 = zFar
		{ 3,-1, 2},
		{-3, 1, 2},
		{ 1, 3, 2},
		{-1,-3, 2},
		{-2,-1, 3},		// 0 degrees yaw, look straight up
		{ 2,-1,-3}		// look straight down
	};

	float		scale;

	// sqrt(a^2 + (a^2 + (a/2)^2)) = sqrt(2*a^2 + 1/4*a^2) = a * sqrt(9/4) = a * 1.5
	scale = ap.zFar / 3;	// any non-zero value not works no TNT2 (but works with GeForce2)
	b[0] = s * scale;
	b[1] = t * scale;
	b[2] = scale;

	for (i = 0; i < 3; i++)
	{
		int		tmp;

		tmp = stToVec[axis][i];
		if (tmp < 0)
			vec[i] = -b[-tmp - 1];
		else
			vec[i] = b[tmp - 1];
	}

	if (tex)
	{
		// convert range [-1, 1] to [0, 1]
		s = (s + 1) / 2;
		t = (t + 1) / 2;

		if (s < 0)		s = 0;
		else if (s > 1)	s = 1;
		if (t < 0)		t = 0;
		else if (t > 1)	t = 1;

/*		if (!currentShader->bad && Cvar_VariableInt("skyfix"))
		{
			float	fix, fix2;

			fix = 0.5f / currentShader->width * Cvar_VariableInt("skyfix");
			fix2 = 1 - 2 * fix;
			s = fix + fix2 * s;
			t = fix + fix2 * t;
		} */
		tex[0] = s;
		tex[1] = 1.0 - t;
	}
}


#define SKY_TESS_SIZE	4

#if (SKY_TESS_SIZE*2+1) * (SKY_TESS_SIZE*2+1) > MAX_VERTEXES
#  error Not enough vertex buffer size: cannot tesselate sky surface
#endif

static void DrawSkyBox (void)
{
	int		i, j;

	LOG_STRING ("*** DrawSkyBox() ***\n");
	if (gl_fastsky->integer) return;

	if (!currentShader->skyRotate)
	{
		for (i = 0; i < 6; i++)
		{
			for (j = 0; j < 2; j++)
			{
				float	v;

				v = floor (skyMins[j][i] * SKY_TESS_SIZE) / SKY_TESS_SIZE;
				if (v < -1) v = -1;
				skyMins[j][i] = v;

				v = ceil (skyMaxs[j][i] * SKY_TESS_SIZE) / SKY_TESS_SIZE;
				if (v > 1) v = 1;
				skyMaxs[j][i] = v;
			}
		}
	}
	else
	{
		for (i = 0; i < 6; i++)
		{
			skyMins[0][i] = skyMins[1][i] = -1;
			skyMaxs[0][i] = skyMaxs[1][i] = 1;
		}
	}

	if (gl_showsky->integer)
		qglDepthRange (0, 0);	// hack to see sky (for debugging)
	else
		qglDepthRange (1, 1);	// put sky to the back of scene (i.e. ignore Z-axis)

	qglDisableClientState (GL_COLOR_ARRAY);
	if (currentShader != gl_defaultSkyShader)
		qglColor3f (gl_config.identityLightValue_f,
					gl_config.identityLightValue_f,
					gl_config.identityLightValue_f); // avoid overbright
	else
		qglColor3f (0, 0, 0);	// bad sky -- make it black (almost as gl_fastsky)

	qglPushMatrix ();
	if (gl_showsky->integer)
		GL_State (GLSTATE_DEPTHWRITE|GLSTATE_NODEPTHTEST);
	else
		GL_State (GLSTATE_NODEPTHTEST);
	// change modelview matrix
	qglTranslatef (ap.vieworg[0], ap.vieworg[1], ap.vieworg[2]);
	if (currentShader->skyRotate)
	{
		qglRotatef (ap.time * currentShader->skyRotate,
			currentShader->skyAxis[0],
			currentShader->skyAxis[1],
			currentShader->skyAxis[2]);
	}

	GL_SetMultitexture (1);
	qglTexCoordPointer (2, GL_FLOAT, 0, vb->texCoord[0]);
	qglVertexPointer (3, GL_FLOAT, sizeof(bufVertex_t), vb->verts);

	for (i = 0; i < 6; i++)
	{
		float	s, t;
		int		ds, dt, si, ti;
		bufVertex_t		*vec;
		bufTexCoord_t	*tex;
		int				*idx, i1, i2;

		if (skyMins[0][i] > skyMaxs[0][i] || skyMins[1][i] > skyMaxs[1][i])
			continue;				// no visible sky surfaces on this plane

		// tesselate sky surface (up to 9 points per diminsion)
		vec = &vb->verts[0];
		tex = vb->texCoord[0];
		numVerts = 0;
		for (t = skyMins[1][i]; t <= skyMaxs[1][i]; t += 1.0f/SKY_TESS_SIZE)
			for (s = skyMins[0][i]; s <= skyMaxs[0][i]; s += 1.0f/SKY_TESS_SIZE)
			{
				MakeSkyVec (s, t, i, tex->tex, vec->xyz);
				tex++;
				vec++;
				numVerts++;
			}
		ds = (skyMaxs[0][i] - skyMins[0][i]) * SKY_TESS_SIZE + 1;
		dt = (skyMaxs[1][i] - skyMins[1][i]) * SKY_TESS_SIZE + 1;
		// generate indexes
		idx = indexesArray;
		i1 = 0;
		i2 = ds;
		numIndexes = 0;
		for (ti = 0; ti < dt - 1; ti++)
		{
			for (si = 0; si < ds - 1; si++)
			{
				// triangle 1 (1-3-2)
				*idx++ = i1++;		// i1(1) ----- i1+1(2)
				*idx++ = i2;		//  |           |
				*idx++ = i1;		//  |           |
									// i2(3) ----- i2+1(4)
				// triangle 2 (3-4-2)
				*idx++ = i2++;
				*idx++ = i2;
				*idx++ = i1;
				numIndexes += 6;
			}
			i1++;
			i2++;
		}

		if (currentShader != gl_defaultSkyShader)
			GL_Bind (currentShader->skyFarBox[i]);
		else
			GL_Bind (NULL);		// disable texturing

		DrawArrays (indexesArray, numIndexes);
	}
	qglPopMatrix ();

	numVerts = numIndexes = numExtra = 0;

	qglDepthRange (0, 1);		// restore depth range (to default values)
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

	CheckOverflow (surf->numVerts, surf->numIndexes);

	firstVert = numVerts;
	numVerts += surf->numVerts;
	firstIndex = numIndexes;
	numIndexes += surf->numIndexes;

	ex = &extra[numExtra++];
	ex->numVerts = surf->numVerts;
	if (surf->lightmap) ex->lmWidth = surf->lightmap->w;
	// copy normal
	VectorCopy (surf->plane.normal, ex->normal);
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
	idx = &indexesArray[firstIndex];
	idxSrc = surf->indexes;
	for (i = 0; i < surf->numIndexes; i++)
		*idx++ = *idxSrc++ + firstVert;
}


static void TesselatePolySurf (surfacePoly_t *surf)
{
	bufVertex_t	*v;
	bufTexCoordSrc_t *t;
	int			*idx, i, firstVert, firstIndex;
	int			*c;
	int			numIdx;
	vertexPoly_t *vs;

	numIdx = (surf->numVerts - 2) * 3;
	CheckOverflow (surf->numVerts, numIdx);

	firstVert = numVerts;
	numVerts += surf->numVerts;
	firstIndex = numIndexes;
	numIndexes += numIdx;

	// setup normal ??

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
	idx = &indexesArray[firstIndex];

	for (i = 1; i < numVerts-1; i++)
	{
		*idx++ = firstVert;
		*idx++ = firstVert + i;
		*idx++ = firstVert + i + 1;
	}
}


static void TesselateMd3Surf (surfaceMd3_t *surf)
{
	bufVertex_t	*v;
	bufTexCoordSrc_t *t;
	int			*idx, *idxSrc, i, firstVert, firstIndex;
	int			*c;
	vertexMd3_t	*vs1, *vs2;
	float		*ts;
	int		numIdx;

	numIdx = surf->numTris * 3;
	CheckOverflow (surf->numVerts, numIdx);

	firstVert = numVerts;
	numVerts += surf->numVerts;
	firstIndex = numIndexes;
	numIndexes += numIdx;

	/*------------- lerp verts ---------------*/
	vs1 = surf->verts + surf->numVerts * currentEntity->frame;
	v = &vb->verts[firstVert];
	if (currentEntity->backLerp != 0.0f && currentEntity->frame != currentEntity->oldFrame)
	{
		float	frontScale, backScale;

		vs2 = surf->verts + surf->numVerts * currentEntity->oldFrame;
		backScale = currentEntity->backLerp / 64.0f;				// MD3_XYZ_SCALE
		frontScale = (1.0f - currentEntity->backLerp) / 64.0f;		// MD3_XYZ_SCALE
		for (i = 0; i < surf->numVerts; i++, vs1++, vs2++, v++)
		{
			v->xyz[0] = vs1->xyz[0] * frontScale + vs2->xyz[0] * backScale;
			v->xyz[1] = vs1->xyz[1] * frontScale + vs2->xyz[1] * backScale;
			v->xyz[2] = vs1->xyz[2] * frontScale + vs2->xyz[2] * backScale;
		}
	}
	else
		for (i = 0; i < surf->numVerts; i++, vs1++, v++)
		{
			v->xyz[0] = vs1->xyz[0] / 64.0f;						// MD3_XYZ_SCALE
			v->xyz[1] = vs1->xyz[1] / 64.0f;
			v->xyz[2] = vs1->xyz[2] / 64.0f;
		}

	c = &srcVertexColor[firstVert].rgba;
	for (i = 0; i < numVerts; i++, c++)
		*c = 0xFFFFFFFF;	//!! color

	/*----------- copy texcoords -------------*/
	t = &srcTexCoord[firstVert];
	ts = surf->texCoords;
	for (i = 0; i < surf->numVerts; i++, t++)
	{
		t->tex[0] = *ts++;
		t->tex[1] = *ts++;
	}

	/*------------ copy indexes --------------*/
	idx = &indexesArray[firstIndex];
	idxSrc = surf->indexes;
	for (i = 0; i < numIdx; i++)
		*idx++ = *idxSrc++ + firstVert;
}


/*-------------- Debug output ----------------*/


static void FlashColor (void)
{
	int		i;

	i = (int)(ap.time / 3 * TABLE_SIZE);
	qglColor3f (sinTable[i & TABLE_MASK] / 2 + 0.5,
				sinTable[i + 100 & TABLE_MASK] / 2 + 0.5,
				sinTable[600 - i & TABLE_MASK] / 2 + 0.5);
}


#define BBOX_WORLD	1	// debug?
// draw entity bounding boxes
static void DrawBBoxes (void)
{
	int		i;
	byte	entsVisible[MAX_GLENTITIES];
	refEntity_t *ent;
	surfaceInfo_t **si;

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

	/*----- check entity visibility -------*/
	memset (entsVisible, 0, gl_numEntities);
	for (i = 0, si = sortedSurfaces; i < ap.numSurfaces; i++, si++)
	{
		int		entNum;

		entNum = ((*si)->sort >> ENTITYNUM_SHIFT) & ENTITYNUM_MASK;
		if (entNum < gl_numEntities)
			entsVisible[entNum] = true;
	}

	/*-------- draw bounding boxes --------*/
	for (i = 0, ent = gl_entities; i < gl_numEntities; i++, ent++)
	{
		model_t		*m;
		float		*mins, *maxs;
		int		j;
		bufVertex_t v[8];
		static int inds[24] = {
			0,1, 1,3, 3,2, 2,0,	// rect1
			4,5, 5,7, 7,6, 6,4,	// rect2
			0,4, 1,5, 3,7, 2,6	// connectors
		};

		if (!entsVisible[i])
			continue;	// entity is culled or from different scene

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

			if (j & 1)	x = maxs[0];
			else		x = mins[0];

			if (j & 2)	y = maxs[1];
			else		y = mins[1];

			if (j & 4)	z = maxs[2];
			else		z = mins[2];

#ifdef BBOX_WORLD
			// project point to a world coordinate system (org + x*axis[0] + y*axis[1] + z*axis[2])
			VectorMA (ent->origin, x, ent->axis[0], tmp);
			VectorMA (tmp,		   y, ent->axis[1], tmp);
			VectorMA (tmp,		   z, ent->axis[2], v[j].xyz);
#else
			v[j].xyz[0] = x; v[j].xyz[1] = y; v[j].xyz[2] = z;
#endif
		}

		// draw it
#ifndef BBOX_WORLD
		qglLoadMatrixf (&ent->modelMatrix[0][0]);
#endif
		qglVertexPointer (3, GL_FLOAT, sizeof(bufVertex_t), v);
		qglDrawElements (GL_LINES, 24, GL_UNSIGNED_INT, inds);
	}
}


static void DrawTriangles (void)
{
	int		i, index, lastEntity;
	surfaceInfo_t	**si;
	surfaceCommon_t *surf;
	int		inds[512];

	GL_SetMultitexture (0);		// disable texturing
	if (gl_showtris->integer - 1 & 1)
	{	// use depth test
		GL_State (0);
//		qglEnable (GL_POLYGON_OFFSET_FILL);
//		qglPolygonOffset (-3, -2);
	}
	else	// no depth test
		GL_State (GLSTATE_NODEPTHTEST);

	if (gl_showtris->integer - 1 & 4)
		FlashColor ();
	else
		qglColor3f (0, 0, 0);
	qglDisableClientState (GL_COLOR_ARRAY);

	for (i = 0; i < 512; i++)
		inds[i] = i;

	lastEntity = -1;
	for (i = 0, si = sortedSurfaces; i < ap.numSurfaces; i++, si++)
	{
		surfacePlanar_t *pl;
		int		entityNum;

		entityNum = ((*si)->sort >> ENTITYNUM_SHIFT) & ENTITYNUM_MASK;
		if (entityNum != lastEntity)
		{
			if (entityNum == ENTITYNUM_WORLD)
				qglLoadMatrixf (&ap.modelMatrix[0][0]);
			else
				qglLoadMatrixf (&gl_entities[entityNum].modelMatrix[0][0]);
			lastEntity = entityNum;
		}

		surf = (*si)->surf;
		if (!surf) continue;	// dummy surface ??

		switch (surf->type)
		{
		case SURFACE_PLANAR:
			pl = surf->pl;
			qglVertexPointer (3, GL_FLOAT, sizeof(vertex_t), pl->verts);

			if (!(gl_showtris->integer - 1 & 2))
			{
				if (qglLockArraysEXT) qglLockArraysEXT (0, pl->numVerts);

				// show triangles
				for (index = 0; index < pl->numIndexes; index += 3)
					qglDrawElements (GL_LINE_LOOP, 3, GL_UNSIGNED_INT, &pl->indexes[index]);

				if (qglUnlockArraysEXT) qglUnlockArraysEXT ();
			}
			else	// show edges
				qglDrawElements (GL_LINE_LOOP, pl->numVerts, GL_UNSIGNED_INT, inds);

			break;
		}
	}

//	if (gl_showtris->integer - 1 & 1)
//		qglDisable (GL_POLYGON_OFFSET_FILL);
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
			GL_Bind (stage->mapImage[Q_ftol(ap.time * stage->animMapFreq) % stage->numAnimTextures]);

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
		if (scale < 20.0f)
			scale = 1;
		else
			scale = scale / 500.0f + 1.0f;

		alpha = p->alpha * 255;
		if (alpha < 0)			alpha = 0;
		else if (alpha > 255)	alpha = 255;

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
	qboolean currentDepthHack, depthHack;

	LOG_STRING (va("******** R_DrawScene: (%g, %g) - (%g, %g) ********\n", ap.x, ap.y, ap.x+ap.w, ap.y+ap.h));

	if (numVerts) FlushArrays ();
	GL_Setup (&ap);

	// sort surfaces
	if (gl_finish->integer == 2) qglFinish ();
	gl_speeds.beginSort = Sys_Milliseconds ();
	GL_SortSurfaces (&ap, sortedSurfaces);
	gl_speeds.begin3D = Sys_Milliseconds ();

	currentDlightMask = 0;

	/*------------ draw sky --------------*/

	ClearSkyBox ();
	numSkySurfs = 0;

	si = sortedSurfaces;
	for (index = 0; index < ap.numSurfaces; index++, si++)
	{
		surf = (*si)->surf;
		shader = GL_GetShaderByNum (((*si)->sort >> SHADERNUM_SHIFT) & SHADERNUM_MASK);
		if (shader->type != SHADERTYPE_SKY) break;

		if (!index) SetCurrentShader (shader);

		TesselatePlanarSurf (surf->pl);	//!! may be other types
		AddSkySurfaces ();
		numSkySurfs++;
		numVerts = numIndexes = 0;
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

	numFastSurfs = 0;
	currentShaderNum = currentEntityNum = -1;
	currentDepthHack = false;
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
			currentEntityNum = entNum;
			if (entNum == ENTITYNUM_WORLD)
			{
				//?? set shader.time to ap.time
				LOG_STRING (va("******** Change entity to WORLD ********\n"));
				qglLoadMatrixf (&ap.modelMatrix[0][0]);
				depthHack = false;
				currentEntity = NULL;		//?? gl_worldEntity ?
			}
			else
			{
				currentEntity = &gl_entities[entNum];
				//?? set shader.time to ap.time - entity.time
				LOG_STRING (va("******** Change entity to %s ********\n", currentEntity->model->name));
				qglLoadMatrixf (&currentEntity->modelMatrix[0][0]);
				depthHack = ((currentEntity->flags & RF_DEPTHHACK) != 0);
			}

			if (depthHack != currentDepthHack)
			{
				qglDepthRange (0, depthHack ? 0.3333 : 1);
				currentDepthHack = depthHack;
			}
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
			//!! other types
			}
		}
	}

	/*--------- finilize/debug -----------*/
	FLUSH();

	if (currentDepthHack)
		qglDepthRange (0, 1);

	if (gl_showtris->integer)
		DrawTriangles ();

	if (gl_showbboxes->integer)
		DrawBBoxes ();

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

	CheckOverflow (4, 6);

	/*   0 (0,0) -- 1 (1,0)
	 *     |   \____   |
	 *     |        \  |
	 *   3 (0,1) -- 2 (1,1)
	 */
	v = &vb->verts[numVerts];
	t = &srcTexCoord[numVerts];
	c = &srcVertexColor[numVerts].rgba;
	idx0 = numVerts;
	numVerts += 4;

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

	idx = &indexesArray[numIndexes];
	numIndexes += 6;
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

	CheckOverflow (p->len * 4, p->len * 6);

	v = &vb->verts[numVerts];
	t = &srcTexCoord[numVerts];
	c = &srcVertexColor[numVerts].rgba;
	idx = &indexesArray[numIndexes];
	idx0 = numVerts;

	s = p->text;
	xp = p->x;
	x = p->x + p->w;

	size = 1.0f / 16.0f;
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
			numVerts += 4;
			numIndexes += 6;
		}
		xp = x;
		x += p->w;
	}
}


void GL_BackEnd (void)
{
	int		*p;
//	static int max = 0;

//	if (backendCmdSize > max) max = backendCmdSize;
//	Cvar_SetInteger("backend", max);
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

			if (stricmp (gl_drawbuffer->string, "GL_FRONT"))
				qglDrawBuffer (GL_BACK);
			else
				qglDrawBuffer (GL_FRONT);

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
	if (qglVertexArrayRangeNV)
	{
		agpBuffer = qglAllocateMemoryNV (NUM_VERTEX_BUFFERS * vbSize, 0, 0, 0.5);
		if (!agpBuffer)
			agpBuffer = qglAllocateMemoryNV (NUM_VERTEX_BUFFERS * vbSize, 0, 0, 1);

		if (!agpBuffer)
		{
			Com_Printf ("^1Cannot allocate video memory for vertex buffers. Disabling GL_NV_vertex_array_range.\n");
			qglVertexArrayRangeNV = NULL;
		}
		else
		{
			qglGenFencesNV (NUM_VERTEX_BUFFERS, fences);
			qglVertexArrayRangeNV (NUM_VERTEX_BUFFERS * vbSize, agpBuffer);
			qglEnableClientState (GL_VERTEX_ARRAY_RANGE_NV);
			vb = agpBuffer;
			currentBuffer = 0;
		}
	}

	if (!agpBuffer)
		vb = Z_Malloc (vbSize);
	Com_Printf("^1 **** buf: %08X (%d bytes) ****\n", vb, vbSize);//!!

	InitFuncTables ();
}


void GL_ShutdownBackend (void)
{
	if (agpBuffer)
	{
		qglDeleteFencesNV (NUM_VERTEX_BUFFERS, fences);
		qglDisableClientState (GL_VERTEX_ARRAY_RANGE_NV);
		qglFreeMemoryNV (agpBuffer);
		agpBuffer = NULL;
	}
	else if (vb)
	{
		Z_Free (vb);
		vb = NULL;
	}
}
