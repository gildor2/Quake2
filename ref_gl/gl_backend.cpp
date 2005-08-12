#include "OpenGLDrv.h"
#include "gl_backend.h"
#include "gl_image.h"
#include "gl_light.h"
#include "gl_lightmap.h"
#include "gl_sky.h"
#include "gl_math.h"
#include "gl_buffers.h"
#include "protocol.h"		//!! for RF_XXX consts only (FULLBRIGHT, BBOX, DEPTHHACK)!

namespace OpenGLDrv {


//#define SPY_SHADER		// comment line to disable gl_spyShader stuff

/*-----------------------------------------------------------------------------
	Local cvars
-----------------------------------------------------------------------------*/

static cvar_t	*gl_clear;
static cvar_t	*gl_finish;			// debug ? (can be a situation, when gl_finish==1 is necessary ? (linux))
static cvar_t	*gl_showbboxes, *gl_showTris, *gl_showNormals;
#ifdef SPY_SHADER
static cvar_t	*gl_spyShader;
#endif


/*-----------------------------------------------------------------------------
	Data
-----------------------------------------------------------------------------*/

struct bufTexCoordSrc_t
{
	float	tex[2];
	float	lm[2];
};


static shader_t		*currentShader;
static refEntity_t	*currentEntity;
static int			currentDlightMask;


// vertex arrays

vertexBuffer_t	*vb;

int				gl_indexesArray[MAX_INDEXES];
bufExtra_t		gl_extra[MAX_VERTEXES];

static color_t			srcVertexColor[MAX_VERTEXES];
static bufTexCoordSrc_t	srcTexCoord[MAX_VERTEXES];

//!! WARNING: do not rename to numVerts etc -- name used in some surfaceBase_t successors
int gl_numVerts, gl_numIndexes, gl_numExtra;


// scene surfaces

static surfaceInfo_t *sortedSurfaces[MAX_SCENE_SURFACES];


/*-----------------------------------------------------------------------------
	Process deforms, ...gens etc.
-----------------------------------------------------------------------------*/

static void ProcessShaderDeforms (shader_t *sh)
{
	int		i, j;
	const deformParms_t *deform;
	bufExtra_t *ex;

	if (!sh->numDeforms)
		return;

	for (i = 0, deform = &sh->deforms[0]; i < sh->numDeforms; i++, deform++)
	{
		bufVertex_t *vec = vb->verts;

		switch (deform->type)
		{
		case DEFORM_WAVE:
			{
#define P deform->wave
				const float *table = mathFuncs[P.type];
				float t = P.freq * vp.time + P.phase;

				for (j = 0, ex = gl_extra; j < gl_numExtra; j++, ex++)
				{
					const CVec3 &norm = ex->normal;
					int k;
					if (P.amp)
					{
						for (k = 0; k < ex->numVerts; k++, vec++)
						{
							float f = PERIODIC_FUNC(table, t + deform->waveDiv * (vec->xyz[0] + vec->xyz[1] + vec->xyz[2])) * P.amp + P.base;
							VectorMA (vec->xyz, f, norm);
						}
					}
					else
					{
						// used for "outline shield" effect (move vertex in direction of normal in constant amount == deform->wave.base)
						CVec3 delta;
						VectorScale (norm, P.base, delta);
						for (k = 0; k < ex->numVerts; k++, vec++)
							VectorAdd (vec->xyz, delta, vec->xyz);
					}
				}
#undef P
			}
			break;
		case DEFORM_MOVE:
			{
#define P deform->moveWave
				float f = PERIODIC_FUNC(mathFuncs[P.type], P.freq * vp.time + P.phase) * P.amp + P.base;
				CVec3 delta;
				VectorScale (deform->move, f, delta);
				for (j = 0; j < gl_numVerts; j++, vec++)
					VectorAdd (vec->xyz, delta, vec->xyz);
#undef P
			}
			break;
		case DEFORM_BULGE:
			{
				float f0 = vp.time * deform->bulgeSpeed;
				bufTexCoordSrc_t *tex = &srcTexCoord[0];
				for (j = 0, ex = gl_extra; j < gl_numExtra; j++, ex++)
				{
					const CVec3 &norm = ex->normal;
					for (int k = 0; k < ex->numVerts; k++, vec++, tex++)
					{
						//?? taken from Q3; change function?
						float f = SIN_FUNC((f0 + tex->tex[0] * deform->bulgeWidth) / (M_PI * 2)) * deform->bulgeHeight;
						VectorMA (vec->xyz, f, norm);
					}
				}
			}
			break;
		case DEFORM_AUTOSPRITE:
			{
				// autoSprite deform require quads
				if (gl_numVerts & 3 || gl_numIndexes != gl_numVerts / 2 * 3)
				{
					DrawTextLeft (va("Incorrect surface for AUTOSPRITE in %s", *currentShader->Name), RGB(1,0,0));	//?? developer
					break;
				}
				// rotate view axis to model coords
				CAxis mViewAxis;
				if (currentEntity->worldMatrix)
					mViewAxis = vp.view.axis;
				else
					currentEntity->coord.axis.TransformAxis (vp.view.axis, mViewAxis);
				// process vertices
				bufTexCoordSrc_t *tex = &srcTexCoord[0];
				int *idx = gl_indexesArray;

				for (j = 0; j < gl_numVerts; j += 4)
				{
					// find middle point
					CVec3 center = vec[0].xyz;
					for (int k = 1; k < 4; k++)
						VectorAdd (center, vec[k].xyz, center);
					center.Scale (1.0f/4);								// average
#if 0
					// compute current quad axis
					CVec3 axis_s, axis_t;
					VectorSubtract (vec[0].xyz, vec[2].xyz, axis_s);
					axis_s.NormalizeFast ();
					VectorSubtract (vec[3].xyz, vec[0].xyz, axis_t);
					VectorMA (axis_t, -dot(axis_t, axis_s), axis_s);	// make axis_t to be orthogonal to axis_s
					axis_t.NormalizeFast ();
					// rotate each vertex
					for (k = 0; k < 4; k++, vec++)
					{
						CVec3 tmp;
						VectorSubtract (vec->xyz, center, tmp);
						float s = dot(tmp, axis_s);
						float t = dot(tmp, axis_t);
						VectorMA (center, s, mViewAxis[1], tmp);
						VectorMA (tmp,    t, mViewAxis[2], vec->xyz);
					}
#else
					// assume source shape is square
					CVec3 tmp;
					VectorSubtract (vec[0].xyz, center, tmp);
					// square side = sqrt(dot(tmp,tmp)) / sqrt(2) = sqrt(dot(tmp,tmp)/2)
					float radius = dot(tmp,tmp) / 2;
					radius = SQRTFAST(radius);
					// compute vertexes
					VectorMA (center,  radius, mViewAxis[1], tmp);		// right
					VectorMA (tmp,  radius, mViewAxis[2], vec->xyz);
					vec++;
					VectorMA (tmp, -radius, mViewAxis[2], vec->xyz);
					vec++;
					VectorMA (center, -radius, mViewAxis[1], tmp);		// left
					VectorMA (tmp, -radius, mViewAxis[2], vec->xyz);
					vec++;
					VectorMA (tmp,  radius, mViewAxis[2], vec->xyz);
					vec++;
					// recompute texcoords
					tex->tex[0] = 0; tex->tex[1] = 0;
					tex++;
					tex->tex[0] = 1; tex->tex[1] = 0;
					tex++;
					tex->tex[0] = 1; tex->tex[1] = 1;
					tex++;
					tex->tex[0] = 0; tex->tex[1] = 1;
					tex++;
					// recompute indexes
					*idx++ = j; *idx++ = j+2; *idx++ = j+1;
					*idx++ = j; *idx++ = j+3; *idx++ = j+2;
#endif
				}
				// store normals
				mViewAxis[0].Negate ();
				for (j = 0, ex = gl_extra; j < gl_numExtra; j++, ex++)
					ex->normal.Zero ();
			}
			break;
		//?? other types: AUTOSPRITE2, NORMAL, PROJECTION_SHADOW (?)
		}
	}
}


static void GenerateColorArray (shaderStage_t *st)
{
	int		i;
	color_t	*src, *dst;

	guard(GenerateColorArray);

	/*---------- rgbGen -----------*/
	src = srcVertexColor;
	dst = vb->color;
	switch (st->rgbGenType)
	{
	case RGBGEN_CONST:
		{
			unsigned rgba = st->rgbaConst.rgba;
			for (i = 0; i < gl_numVerts; i++, dst++)
				dst->rgba = rgba;
		}
		break;
	case RGBGEN_EXACT_VERTEX:
		memcpy (dst, src, gl_numVerts * sizeof(color_t));
		break;
	case RGBGEN_VERTEX:
		for (i = 0; i < gl_numVerts; i++, src++, dst++)
		{
			dst->c[0] = src->c[0] >> gl_config.overbright;
			dst->c[1] = src->c[1] >> gl_config.overbright;
			dst->c[2] = src->c[2] >> gl_config.overbright;
		}
		break;
	case RGBGEN_BOOST_VERTEX:
		{
#define MIN_BOOST_BRIGHT		48
			int ka = (256 - MIN_BOOST_BRIGHT);							// do not lightscale multiplier
			int kb = (MIN_BOOST_BRIGHT * 256) >> gl_config.overbright;	// but lightscale bias
			for (i = 0; i < gl_numVerts; i++, src++, dst++)
			{
				int r = src->c[0];
				int g = src->c[1];
				int b = src->c[2];
				int oldbr = max(r, g);
				oldbr = max(oldbr, b);
				int newbr = oldbr * ka + kb;
				if (oldbr)				// if at least one of r,g,b is non-zero
				{
					int scale = newbr / oldbr;
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
			dst->c[0] = (255 - src->c[0]) >> gl_config.overbright;
			dst->c[1] = (255 - src->c[1]) >> gl_config.overbright;
			dst->c[2] = (255 - src->c[2]) >> gl_config.overbright;
		}
		break;
	case RGBGEN_DIFFUSE:
		DiffuseLight (dst, 1);
		break;
	case RGBGEN_HALF_DIFFUSE:
		DiffuseLight (dst, 0.5f);
		break;
	// other types: FOG
	}

	if (st->rgbGenType == RGBGEN_EXACT_VERTEX && st->alphaGenType == ALPHAGEN_VERTEX)
		return;	// alpha is already set

	/*--------- alphaGen ----------*/
	src = srcVertexColor;
	dst = vb->color;
	switch (st->alphaGenType)
	{
	case ALPHAGEN_CONST:
		{
			byte a = st->rgbaConst.c[3];
			for (i = 0; i < gl_numVerts; i++, dst++)
				dst->c[3] = a;
		}
		break;
	case ALPHAGEN_VERTEX:
		for (i = 0; i < gl_numVerts; i++, src++, dst++)
			dst->c[3] = src->c[3];
		break;
	case ALPHAGEN_ONE_MINUS_VERTEX:
		for (i = 0; i < gl_numVerts; i++, src++, dst++)
			dst->c[3] = 255 - src->c[3];
		break;
	case ALPHAGEN_DOT:
	case ALPHAGEN_ONE_MINUS_DOT:
		{
			bufExtra_t *ex;
			int		j, min;
			float	scale;

			if (st->alphaGenType == ALPHAGEN_DOT)
			{
				min = appRound (st->alphaMin * 255);
				scale = (st->alphaMax - st->alphaMin) * 255;
			}
			else
			{
				min = appRound (st->alphaMax * 255);
				scale = (st->alphaMin - st->alphaMax) * 255;
			}
			bufVertex_t *vec = vb->verts;
			for (j = 0, ex = gl_extra; j < gl_numExtra; j++, ex++)
			{
				CVec3 &norm = ex->normal;
				for (i = 0; i < ex->numVerts; i++, vec++, dst++)
				{
					CVec3	v;
					VectorSubtract (currentEntity->modelvieworg, vec->xyz, v);
					v.NormalizeFast ();
					float d = dot (v, norm);
#if 0
					d = d * d;
					dst->c[3] = appRound (d * scale) + min;
#else
//					if (d < 0)	dst->c[3] = 0;
//					else		dst->c[3] = appRound (d * scale) + min;
					dst->c[3] = bound (appRound (d * scale) + min, 0, 255);
#endif
				}
			}
		}
		break;
	//?? other types: LIGHTING_SPECULAR, PORTAL
	}

	unguard;
}


static void GenerateTexCoordArray (shaderStage_t *st, int tmu, const image_t *tex)
{
	int				j, k;
	tcModParms_t	*tcmod;
	bufTexCoord_t	*dst, *dst2;
	bufTexCoordSrc_t *src;

	dst = vb->texCoord[tmu];

	src = srcTexCoord;
	dst2 = dst;		// save this for tcMod

	guard(GenerateTexCoordArray);
	if (tex && tex->target == GL_TEXTURE_RECTANGLE_NV && st->tcGenType != TCGEN_TEXTURE)
		Com_DropError ("shader %s uses TEXTURE_RECTANGLE with not \"tcGen texture\"", *currentShader->Name);

	// process tcGen
	switch (st->tcGenType)
	{
	case TCGEN_TEXTURE:
		if (!tex || tex->target != GL_TEXTURE_RECTANGLE_NV)
			for (j = 0; j < gl_numVerts; j++, src++, dst++)
			{
				dst->tex[0] = src->tex[0];
				dst->tex[1] = src->tex[1];
			}
		else
		{
			float w = tex->internalWidth;
			float h = tex->internalHeight;
			for (j = 0; j < gl_numVerts; j++, src++, dst++)
			{
				dst->tex[0] = src->tex[0] * w;
				dst->tex[1] = src->tex[1] * h;
			}
		}
		break;
	case TCGEN_LIGHTMAP:
		for (j = 0; j < gl_numVerts; j++, src++, dst++)
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
			float mul = (float)(st->tcGenType - TCGEN_LIGHTMAP1 + 1) / LIGHTMAP_SIZE;
			bufExtra_t *ex;
			for (j = 0, ex = gl_extra; j < gl_numExtra; j++, ex++)
			{
				float shift = ex->lmWidth * mul;
				for (k = 0; k < ex->numVerts; k++, src++, dst++)
				{
					dst->tex[0] = src->lm[0] + shift;
					dst->tex[1] = src->lm[1];
				}
			}
		}
		break;
	case TCGEN_ENVIRONMENT:
		{
			bufExtra_t *ex;
			CVec3	v;
			float	d;

			bufVertex_t *vec = vb->verts;
			for (j = 0, ex = gl_extra; j < gl_numExtra; j++, ex++)
			{
				if (ex->axis)
				{	// compute envmap using surface axis
					CVec3	*axis;	// will point to CVec3[2]

					axis = ex->axis;
					for (k = 0; k < ex->numVerts; k++, vec++, dst++)
					{
						VectorSubtract (currentEntity->modelvieworg, vec->xyz, v);
						v.NormalizeFast ();
						d = dot (v, axis[0]);
						dst->tex[0] = (d - 1) / 2;
						d = dot (v, axis[1]);
						dst->tex[1] = (d - 1) / 2;
					}
				}
				else
				{	// axis is not provided - use normal
					CVec3 &norm = ex->normal;
					for (k = 0; k < ex->numVerts; k++, vec++, dst++)
					{
						VectorSubtract (currentEntity->modelvieworg, vec->xyz, v);
						v.NormalizeFast ();
						d = dot (v, norm) * 2;
						dst->tex[0] = (d * norm[1] - v[1] + 1) / 2;
						dst->tex[1] = (d * norm[2] - v[2] + 1) / 2;
					}
				}
			}
		}
		break;
	case TCGEN_VECTOR:
		{
			bufVertex_t *vec = vb->verts;
			CVec3 v0 = st->tcGenVec[0];		// cache vectors
			CVec3 v1 = st->tcGenVec[1];
			for (j = 0; j < gl_numVerts; j++, vec++, dst++)
			{
				dst->tex[0] = dot (vec->xyz, v0);
				dst->tex[1] = dot (vec->xyz, v1);
			}
		}
		break;
	// other types: ZERO (?), FOG (?)

	default:
		if (st->tcGenType >= TCGEN_DLIGHT0 && st->tcGenType < TCGEN_DLIGHT0 + MAX_DLIGHTS)
		{
			bufExtra_t *ex;

			int num = st->tcGenType - TCGEN_DLIGHT0;
			bufVertex_t *vec = vb->verts;
			for (j = 0, ex = gl_extra; j < gl_numExtra; j++, ex++)
			{
				surfDlight_t *sdl = &ex->dlight[num];
				float invRadius = 0.5f / sdl->radius;
				for (k = 0; k < ex->numVerts; k++, dst++, vec++)
				{
					dst->tex[0] = (dot (vec->xyz, sdl->axis[0]) - sdl->pos[0]) * invRadius + 0.5f;
					dst->tex[1] = (dot (vec->xyz, sdl->axis[1]) - sdl->pos[1]) * invRadius + 0.5f;
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
		case TCMOD_OFFSET:
			if (tcmod->type == TCMOD_SCROLL)
			{
				f1 = tcmod->sSpeed * vp.time;
				f1 = f1 - appFloor (f1);		// frac(f1)
				f2 = tcmod->tSpeed * vp.time;
				f2 = f2 - appFloor (f2);		// frac(f2)
			}
			else
			{
				f1 = tcmod->sOffset;
				f2 = tcmod->tOffset;
			}
			for (k = 0; k < gl_numVerts; k++, dst++)
			{
				dst->tex[0] += f1;
				dst->tex[1] += f2;
			}
			break;
		case TCMOD_TURB:
			{
				bufVertex_t *vec;

				f1 = tcmod->wave.freq * vp.time + tcmod->wave.phase;
				f2 = tcmod->wave.amp;
				for (k = 0, vec = vb->verts; k < gl_numVerts; k++, dst++, vec++)
				{
#if 0
					dst->tex[0] += PERIODIC_FUNC(sinTable, (vec->xyz[0] + vec->xyz[2]) / TABLE_SIZE + f1) * f2;	// Q3: vec[0] + vec[2]
					dst->tex[1] += PERIODIC_FUNC(sinTable, (vec->xyz[1] + vec->xyz[2]) / TABLE_SIZE + f1) * f2;	// Q3: vec[1]
#else
					float f = SIN_FUNC((vec->xyz[0] + vec->xyz[1] + vec->xyz[2]) / TABLE_SIZE + f1) * f2;
					dst->tex[0] += f;
					dst->tex[1] += f;
#endif
				}
			}
			break;
		case TCMOD_WARP:
			for (k = 0; k < gl_numVerts; k++, dst++)
			{
				f1 = dst->tex[0];
				f2 = dst->tex[1];
				dst->tex[0] = f1 / 64.0f + SIN_FUNC((f2 / 16.0f + vp.time) / (2.0f * M_PI)) / 16.0f;
				dst->tex[1] = f2 / 64.0f + SIN_FUNC((f1 / 16.0f + vp.time) / (2.0f * M_PI)) / 16.0f;
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
		case TCMOD_STRETCH:
			{
				// stretch around texture center
#define P tcmod->wave		//?? make as inline function (find all similar places)
				float f1 = PERIODIC_FUNC(mathFuncs[P.type], P.freq * vp.time + P.phase) * P.amp + P.base;
#undef P
				if (f1 < 0.001f) f1 = 0.001f;
				f1 = 1.0f / f1;
				float f2 = (1 - f1) / 2;
				for (k = 0; k < gl_numVerts; k++, dst++)
				{
					dst->tex[0] = dst->tex[0] * f1 + f2;
					dst->tex[1] = dst->tex[1] * f1 + f2;
				}
			}
			break;
		case TCMOD_ROTATE:
			{
				// rotate around texture center
				float f = tcmod->rotateSpeed * vp.time / 360;	// angle
				f1 = SIN_FUNC(f);
				f2 = COS_FUNC(f);
				float c1 = 0.5f * (1 - f1 - f2);
				float c2 = 0.5f * (1 + f1 - f2);
				for (k = 0; k < gl_numVerts; k++, dst++)
				{
					f = dst->tex[0];
					dst->tex[0] = dst->tex[1] * f1 + f * f2 + c1;
					dst->tex[1] = dst->tex[1] * f2 - f * f1 + c2;
				}
			}
			break;
		case TCMOD_TRANSFORM:
			for (k = 0; k < gl_numVerts; k++, dst++)
			{
				f1 = dst->tex[0];
				f2 = dst->tex[1];
				dst->tex[0] = f1 * tcmod->m[0][0] + f2 * tcmod->m[1][0] + tcmod->t[0];
				dst->tex[1] = f1 * tcmod->m[0][1] + f2 * tcmod->m[1][1] + tcmod->t[1];
			}
			break;
		}
	}

	unguard;
}


/*-----------------------------------------------------------------------------
	Shader visualization
-----------------------------------------------------------------------------*/

// should rename "temp", "tmp" ?? (rendererStage_t, combinedStage_t, texUnit_t ?)
//!! WARNING: should not be constructors, because using *tempStage_t=*shaderStage_t
struct tempStage_t : public shaderStage_t
{
	// modified copy of original stage (or auto-generated stage) + additional fields
	unsigned texEnv;		// value for GL_TexEnv()
	bool	isConstRGBA;
	bool	isIdentityRGBA;
	bool	isDoubleRGBA;
	bool	imgNoAlpha;
};


struct renderPass_t
{
	unsigned glState;
	int		numStages;
	tempStage_t *stages;
	shaderStage_t *colorStage;	//?? remove if it is always the same as "stages[0]"
};

#define MAX_RENDER_PASSES	(MAX_SHADER_STAGES+8)

static tempStage_t	tmpSt[MAX_RENDER_PASSES];
static int			numTmpStages;
static renderPass_t renderPasses[MAX_RENDER_PASSES];
static int			numRenderPasses;
//?? move currentShader here


#define BLEND_UNKNOWN			0
#define BLEND_MULTIPLICATIVE	1
#define BLEND_ADDITIVE			2
#define BLEND_ANY				(BLEND_MULTIPLICATIVE|BLEND_ADDITIVE)
#define BLEND_INCOMPATIBLE		0x80000000

#define DEBUG_FULLBRIGHT	1
#define DEBUG_LIGHTMAP		2

//#define LOG_PP(x)	LOG_STRING(x)
//#define LOG_PP(x) DrawTextLeft(x,RGB(0.3,0.6,0.6))

#ifndef LOG_PP
#	ifdef SPY_SHADER
#		define LOG_PP(x)	if (spy) DrawTextLeft(x,RGB(0.3,0.6,0.6));
		// NOTE: when LOG_PP() placed inside "if" operator, we should use {} around LOG_PP() (because LOG_PP have own "if")
#	else
#		define LOG_PP(x)
#		define NO_LOG_PP
#	endif
#endif

//!! separate copy-stages and combine-with-multitexture to different funcs (make few mtex funcs for different extensions ?)
//?? and: rename this func
static void PreprocessShader (shader_t *sh)
{
	int		i;
	shaderStage_t *stage;
	tempStage_t *st;

#ifdef SPY_SHADER
	bool spy = false;
	const char *mask = gl_spyShader->string;
	if ((mask[0] && mask[1]) || mask[0] == '*')		// string >= 2 chars or "*"
		spy = appMatchWildcard (sh->Name, mask, false);
#endif

	int debugMode = 0;
	if (r_fullbright->integer) debugMode |= DEBUG_FULLBRIGHT;
	if (r_lightmap->integer) debugMode |= DEBUG_LIGHTMAP;

	st = tmpSt;
	numTmpStages = 0;
	memset (tmpSt, 0, sizeof(tmpSt));				// initialize all fields with zeros

	// get lightmap stage (should be first ??)
	shaderStage_t *lmStage = NULL;
	if (sh->numStages && sh->stages[0]->isLightmap) lmStage = sh->stages[0];
	int firstStage = 0;

	if (lmStage && gl_dynamic->integer && !r_fullbright->integer)
	{
		if (currentDlightMask)
		{
			unsigned mask;
			int		num;

			for (i = 0, num = 0, mask = currentDlightMask; mask; i++, mask >>= 1)
			{
				if (!(mask & 1)) continue;

				st->mapImage[0] = gl_dlightImage;
				if (!numTmpStages)
					st->glState = lmStage->glState;
				else
					st->glState = BLEND(1,1);		// src + dst
				// set rgba
				st->alphaGenType   = ALPHAGEN_CONST;
				st->rgbGenType     = RGBGEN_CONST;
				st->rgbaConst.rgba = vp.dlights[i].c.rgba;
				// set tcGen
				st->tcGenType = (tcGenType_t)(TCGEN_DLIGHT0 + num);
				st->numTcMods = 0;
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
				byte style = (i < 4 ? currentShader->lightStyles[i] : 0);	// if shader have less than 4 styles, we will get 0 earlier

				// set image
				st->mapImage[0] = lmStage->mapImage[0];
				// set glState
				if (!numTmpStages)
					st->glState = lmStage->glState;
				else
					st->glState = BLEND(1,1);		// src + dst
				// set rgba
				st->alphaGenType = ALPHAGEN_CONST;
				st->rgbGenType   = RGBGEN_CONST;
				if (style)
				{
					byte c = vp.lightStyles[style].value;
					st->rgbaConst.rgba = RGB255(c,c,c);
				}
				else
					st->rgbaConst.rgba = RGBA(1,1,1,1);
				// set tcGen
				st->tcGenType = style ? (tcGenType_t)(TCGEN_LIGHTMAP1 + i) : TCGEN_LIGHTMAP;
				st->numTcMods = 0;
				// finish
				st++;
				numTmpStages++;
				if (!style) break;			// it was main lightmap
			}
			firstStage = 1;			// skip lightmap stage
		}
	}

	/*-------------- copy remainder stages -------------*/

	unsigned glState = 0xFFFFFFFF;
	if (lmStage) glState = lmStage->glState;
	shaderStage_t **pstage;
	for (i = firstStage, pstage = sh->stages + firstStage; i < sh->numStages; i++, pstage++)
	{
		stage = *pstage;

		/*---------- fullbright/lightmap ---------------*/
		if (debugMode == DEBUG_FULLBRIGHT && stage->isLightmap) continue;
		if (debugMode == DEBUG_LIGHTMAP && currentShader->lightmapNumber >= 0 && !stage->isLightmap) break;

		/*--------------- copy stage -------------------*/
		*static_cast<shaderStage_t*>(st) = *stage;
		// select stage image from animation
		if (stage->numAnimTextures > 1)
		{
			int		n;

			if (currentEntity == &gl_entities[ENTITYNUM_WORLD] || !stage->frameFromEntity)
				n = appFloor (vp.time * stage->animMapFreq);
			else
				n = currentEntity->frame;
			const image_t *img = st->mapImage[0] = stage->mapImage[n % stage->numAnimTextures];
			// determine: whether image have alpha-channel
			if (img && img->alphaType)
				st->imgNoAlpha = false;
			else
				st->imgNoAlpha = true;
		}

		/*---------- fullbright/lightmap ---------------*/
		// fix glState if current stage is first after lightmap with r_fullbright!=0
		if (debugMode == DEBUG_FULLBRIGHT && glState != -1)
		{
			st->glState = glState;
			glState = -1;
		}
		if (debugMode == (DEBUG_LIGHTMAP|DEBUG_FULLBRIGHT) && !gl_state.is2dMode)
			st->glState |= GLSTATE_POLYGON_LINE;
		if (!i && (currentShader->lightmapNumber == LIGHTMAP_VERTEX || stage->rgbGenType == RGBGEN_DIFFUSE))
		{
			if (debugMode == DEBUG_LIGHTMAP)
				st->mapImage[0] = NULL;
			else if (debugMode == DEBUG_FULLBRIGHT)
			{
				st->rgbGenType = RGBGEN_CONST;
				st->rgbaConst.rgba |= RGBA(1,1,1,0);
			}
		}

		st++;
		numTmpStages++;
	}

	// override current shader when displaying fillrate
	if (gl_showFillRate->integer && GL_SUPPORT(QGL_EXT_TEXTURE_ENV_COMBINE|QGL_ARB_TEXTURE_ENV_COMBINE|
		QGL_ARB_TEXTURE_ENV_ADD|QGL_NV_TEXTURE_ENV_COMBINE4|QGL_ATI_TEXTURE_ENV_COMBINE3))	// requires env add caps
	{
		numTmpStages = 2;
		//---- 1st stage: color.red == fillrate
		// color
		tmpSt[0].rgbGenType      = RGBGEN_CONST;
		tmpSt[0].alphaGenType    = ALPHAGEN_CONST;
		tmpSt[0].rgbaConst.rgba  = RGB255(sh->numStages * 16, 0, 0);
		// texture
		tmpSt[0].tcGenType       = TCGEN_TEXTURE;	// any
		tmpSt[0].numTcMods       = 0;
		tmpSt[0].numAnimTextures = 1;
		tmpSt[0].mapImage[0]     = NULL;
		// glState
		tmpSt[0].glState         = BLEND(1,1)|GLSTATE_NODEPTHTEST;
		//---- 2nd stage: display wireframe with non-red color
		// color
		tmpSt[1].rgbGenType      = RGBGEN_CONST;
		tmpSt[1].alphaGenType    = ALPHAGEN_CONST;
		tmpSt[1].rgbaConst.rgba  = RGB255(0, 10, 10);
		// texture
		tmpSt[1].tcGenType       = TCGEN_TEXTURE;	// any
		tmpSt[1].numTcMods       = 0;
		tmpSt[1].numAnimTextures = 1;
		tmpSt[1].mapImage[0]     = NULL;
		// glState
		tmpSt[1].glState         = BLEND(1,1)|GLSTATE_NODEPTHTEST|GLSTATE_POLYGON_LINE;
	}

	bool entityLightingDone = false;
	for (i = 0, st = tmpSt; i < numTmpStages; i++, st++)
	{
		/*------- convert some rgbaGen to CONST --------*/
		switch (st->rgbGenType)
		{
		case RGBGEN_NONE:
		case RGBGEN_IDENTITY:
			st->rgbaConst.rgba |= RGBA(1,1,1,0);	// RGB = 255, alpha - unchanged
			st->rgbGenType = RGBGEN_CONST;
			break;
		case RGBGEN_IDENTITY_LIGHTING:
			st->rgbaConst.c[0] = st->rgbaConst.c[1] = st->rgbaConst.c[2] = gl_config.identityLightValue;
			st->rgbGenType = RGBGEN_CONST;
			break;
		case RGBGEN_ENTITY:
			st->rgbaConst.c[0] = currentEntity->shaderColor.c[0] >> gl_config.overbright;
			st->rgbaConst.c[1] = currentEntity->shaderColor.c[1] >> gl_config.overbright;
			st->rgbaConst.c[2] = currentEntity->shaderColor.c[2] >> gl_config.overbright;
			st->rgbGenType = RGBGEN_CONST;
			break;
		case RGBGEN_ONE_MINUS_ENTITY:
			st->rgbaConst.c[0] = (255 - currentEntity->shaderColor.c[0]) >> gl_config.overbright;
			st->rgbaConst.c[1] = (255 - currentEntity->shaderColor.c[1]) >> gl_config.overbright;
			st->rgbaConst.c[2] = (255 - currentEntity->shaderColor.c[2]) >> gl_config.overbright;
			st->rgbGenType = RGBGEN_CONST;
			break;
		case RGBGEN_WAVE:
			{
				//?? function may be NOISE
#define P st->rgbGenWave
				float c1 = PERIODIC_FUNC(mathFuncs[P.type], P.freq * vp.time + P.phase) * P.amp + P.base;
#undef P
				int c2 = appRound (c1 * 255);
				int	c3;
#define STEP(n)							\
	c3 = c2 * st->rgbaConst.c[n] >> 8;	\
	st->rgbaConst.c[n] = bound(c3, 0, 255);
				STEP(0); STEP(1); STEP(2);
				st->rgbGenType = RGBGEN_CONST;
#undef STEP
			}
			break;
		case RGBGEN_DIFFUSE:
			if (!entityLightingDone)
			{
				if (currentEntity->flags & RF_FULLBRIGHT)
				{
					st->rgbaConst.c[0] = st->rgbaConst.c[1] = st->rgbaConst.c[2] = gl_config.identityLightValue;
					st->rgbGenType = RGBGEN_CONST;
				}
				else
				{
					LightForEntity (currentEntity);
					if (GL_SUPPORT(QGL_EXT_TEXTURE_ENV_COMBINE|QGL_ARB_TEXTURE_ENV_COMBINE) &&
						!gl_config.overbright &&		// allows double brightness by itself
						i == 0)			//?? should analyze blend: can be 'src'=='no blend' or 'src*dst'
					{
						st->rgbGenType = RGBGEN_HALF_DIFFUSE;
						st->isDoubleRGBA = true;
					}
				}
				entityLightingDone = true;
			}
			break;
		}

		switch (st->alphaGenType)
		{
		// IDENTITY already converted to CONST
		case ALPHAGEN_ENTITY:
			st->rgbaConst.c[3] = currentEntity->shaderColor.c[3];
			st->alphaGenType = ALPHAGEN_CONST;
			break;
		case ALPHAGEN_ONE_MINUS_ENTITY:
			st->rgbaConst.c[3] = 255 - currentEntity->shaderColor.c[3];
			st->alphaGenType = ALPHAGEN_CONST;
			break;
		case ALPHAGEN_WAVE:
			{
#define P st->alphaGenWave
				float c1 = PERIODIC_FUNC(mathFuncs[P.type], P.freq * vp.time + P.phase) * P.amp + P.base;
#undef P
				int c2 = appRound (c1 * 255);
				st->rgbaConst.c[3] = bound(c2, 0, 255);
				st->alphaGenType = ALPHAGEN_CONST;
			}
			break;
		}

		st->isConstRGBA = (st->rgbGenType == RGBGEN_CONST && st->alphaGenType == ALPHAGEN_CONST);
		st->isIdentityRGBA = (st->isConstRGBA && st->rgbaConst.rgba == RGBA(1,1,1,1));
	}

	if (!numTmpStages)
		DrawTextLeft (va("R_PreprocessShader(%s): 0 stages", *currentShader->Name), RGB(1,0,0));

	if (numTmpStages > MAX_RENDER_PASSES)
		Com_FatalError ("R_PreprocessShader: numStages too large (%d)", numTmpStages);

	renderPass_t *pass = renderPasses;
	st = tmpSt;

	/*--- if no multitexturing or only 1 stage -- nothing to combine -----*/

	if (numTmpStages < 2 || gl_config.maxActiveTextures < 2)
	{
		for (i = 0; i < numTmpStages; i++, st++, pass++)
		{
			pass->glState    = st->glState;
			pass->numStages  = 1;
			pass->stages     = st;
			pass->colorStage = static_cast<shaderStage_t*>(st);
			st->texEnv = st->isDoubleRGBA ?
				TEXENV_C_MODULATE | TEXENV_MUL2 | TEXENV_0PREV_1TEX :
				TEXENV_MODULATE;
		}
		numRenderPasses = numTmpStages;
		return;
	}

	/*-------------------- have multitexturing ---------------------------*/

	numRenderPasses = 0;
	int tmuLeft = 0;
	int tmuUsed = 0;
	unsigned passStyle = BLEND_UNKNOWN;
	LOG_PP(va("--- PreprocessShader(%s) ---\n", *sh->Name));
	for (i = 0; i < numTmpStages; i++, st++)
	{
		if (!tmuLeft)						// all tmu was distributed or just started
		{
			tmuLeft = gl_config.maxActiveTextures;
			if (tmuUsed) pass++;
			tmuUsed = 0;
			// start new pass
			numRenderPasses++;
			pass->glState    = st[0].glState;
			pass->numStages  = 1;
			pass->stages     = st;
			pass->colorStage = static_cast<shaderStage_t*>(st);
			st[0].texEnv = st->isDoubleRGBA ?
				TEXENV_C_MODULATE | TEXENV_MUL2 | TEXENV_0PREV_1TEX :
				TEXENV_MODULATE;			// modulate with rgbaGen

			switch (pass->glState & GLSTATE_BLENDMASK)
			{
			case 0:
				passStyle = BLEND_ANY;
				break;
			case BLEND(1,1):
				passStyle = BLEND_ADDITIVE;
				break;
			case BLEND(D_COLOR,S_COLOR):	// src*dst*2
			case BLEND(D_COLOR,0):			// src*dst
				passStyle = BLEND_MULTIPLICATIVE;
				break;
			default:
				passStyle = BLEND_INCOMPATIBLE;
			}
			LOG_PP("-- next pass\n");
		}
		LOG_PP(va("  tmu[%d:%d] = \"%s\" (rgba: %s %s %8X)\n", i, tmuUsed, *st[0].mapImage[0]->Name,
			st[0].isConstRGBA ? "const" : "var", st[0].isIdentityRGBA ? "ident" : "--", st[0].rgbaConst.rgba));

		tmuUsed++;
		tmuLeft--;
		// if at least 2nd TMU, we may need to update some pass fields
		if (tmuUsed > 1)
			pass->glState |= st[0].glState & GLSTATE_DEPTHWRITE;	// pass.depthwrite = OR(stages.depthwrite)

		if (i == numTmpStages - 1) break;	// no next stage to combine

		/* If can combine: pass->numStages++, set st[1].texEnv and continue
		 * If can't:       set tmuLeft to 0
		 */

		// check for compatibility of current glState with the next glState
		if (tmuLeft < 1 || passStyle == BLEND_INCOMPATIBLE ||
			(st[0].glState ^ st[1].glState) & ~(GLSTATE_BLENDMASK|GLSTATE_DEPTHWRITE))
		{	// incompatible ... next stage will be 1st in the next rendering pass
			tmuLeft = 0;
			continue;
		}

		// now, we can check blendmode and rgbaGen
		unsigned blend2 = st[1].glState & GLSTATE_BLENDMASK;

		// pure multitexture: PREV+T2 or PREV+T2, no multipliers
		// any rgba for current TMU, identity for next
		if (st[1].isIdentityRGBA)
		{
			// pure multitexture can emulate only 2 blendmodes: "src*dst" and "src+dst" (when texenv_add)
			if (blend2 == BLEND(D_COLOR,0) && passStyle & BLEND_MULTIPLICATIVE)
			{
				LOG_PP(va("  MT(MUL): with \"%s\"\n", *st[1].mapImage[0]->Name));
				st[1].texEnv = TEXENV_MODULATE;
				pass->numStages++;
				continue;
			}

			if (GL_SUPPORT(QGL_ARB_TEXTURE_ENV_ADD|QGL_EXT_TEXTURE_ENV_COMBINE|QGL_ARB_TEXTURE_ENV_COMBINE) &&
				blend2 == BLEND(1,1) && passStyle & BLEND_ADDITIVE)
			{
				LOG_PP(va("  MT(ADD): with \"%s\"\n", *st[1].mapImage[0]->Name));
				st[1].texEnv = (GL_SUPPORT(QGL_ARB_TEXTURE_ENV_ADD))
					? TEXENV_ADD : TEXENV_C_ADD | TEXENV_0PREV_1TEX;
				pass->numStages++;
				continue;
			}

			if (GL_SUPPORT(QGL_ARB_TEXTURE_ENV_COMBINE)
				&& blend2 == BLEND(D_COLOR,S_COLOR)		// src*dst + dst*src
				&& passStyle & BLEND_MULTIPLICATIVE)
			{
				LOG_PP(va("  MT(MUL2): with \"%s\"\n", *st[1].mapImage[0]->Name));
				st[1].texEnv = TEXENV_C_MODULATE | TEXENV_MUL2 | TEXENV_0PREV_1TEX;
				pass->numStages++;
				continue;
			}
		}

		if (GL_SUPPORT(QGL_NV_TEXTURE_ENV_COMBINE4|QGL_ATI_TEXTURE_ENV_COMBINE3) && st[1].isConstRGBA && (tmuUsed == 1 || st[0].isConstRGBA) &&
			// NV: This extension can perform A*T1+B*T2, where A/B is 1|0|prev|tex
			//   st[1] is B*T2, if rgba is not 1, rgbGen will eat B and we will not be able to use most of blends ...
			// ATI: can perform A*T1+T2 or T1+A*T2 (similar to NV, but either A or B should be 1)
			(st[1].isIdentityRGBA || (blend2 & GLSTATE_SRCMASK) == SRCBLEND(1)))
		{
			static const unsigned blendToEnv[] = {	// this table is corresponding to GLSTATE_[SRC|DST]_XXX
				0,
				TEXENV_ZERO, TEXENV_ONE,
				TEXENV_TEXTURE, TEXENV_ONE_MINUS_TEXTURE,
				TEXENV_TEXALPHA, TEXENV_ONE_MINUS_TEXALPHA,
				TEXENV_PREVIOUS, TEXENV_ONE_MINUS_PREVIOUS,
				TEXENV_PREVALPHA, TEXENV_ONE_MINUS_PREVALPHA,
				0								// SRC_ALPHASATURATE - cannot be multitextured
			};

			bool combine = true;

			// check for valid blend mode
			unsigned b1 = blend2 & GLSTATE_SRCMASK;
			unsigned b2 = blend2 & GLSTATE_DSTMASK;
			if (passStyle == BLEND_ADDITIVE)
			{
				if (b1 != SRCBLEND(1) && b1 != SRCBLEND(S_ALPHA) && b1 != SRCBLEND(M_S_ALPHA))
					combine = false;
				if (b2 != DSTBLEND(1) && b2 != DSTBLEND(D_ALPHA) && b2 != DSTBLEND(M_D_ALPHA))
					combine = false;
			}
			else if (passStyle == BLEND_MULTIPLICATIVE)
			{
				if (b1 != SRCBLEND(0) && b2 != DSTBLEND(0) && blend2 != BLEND(D_COLOR,S_COLOR))
					combine = false;
			}
			// convert blendmode to TexEnv
			unsigned env1 = blendToEnv[(blend2 & GLSTATE_SRCMASK) >> GLSTATE_SRCSHIFT];
			unsigned env2 = blendToEnv[(blend2 & GLSTATE_DSTMASK) >> GLSTATE_DSTSHIFT];
			if (!(env1 && env2)) combine = false;

			unsigned env;
			if (GL_SUPPORT(QGL_NV_TEXTURE_ENV_COMBINE4))
			{
				// src0*src1+src2*src3
				env = TEXENV_C4_ADD | TEXENV(1,TEXTURE) | TEXENV(3,PREVIOUS);
				if (st[1].isIdentityRGBA || env1 != TEXENV_ONE)
					env |= env1 << TEXENV_SRC0_SHIFT;
				else
					env |= TEXENV(0,CONSTANT) | TEXENV_ENVCOLOR;
				env |= env2 << TEXENV_SRC2_SHIFT;
			}
			else // if (GL_SUPPORT(QGL_ATI_TEXTURE_ENV_COMBINE3))
			{
				// src0*src2+src1
				if (env1 == TEXENV_ONE)
				{	// TEXTURE mul is 1
					if (st[1].isIdentityRGBA)
						env = TEXENV_C3_ADD | TEXENV(1,TEXTURE) | TEXENV(0,PREVIOUS) | (env2<<TEXENV_SRC2_SHIFT); // env2*PREVIOUS+TEXTURE
					else if (env2 == TEXENV_ONE)
						env = TEXENV_C3_ADD | TEXENV(0,TEXTURE) | TEXENV(1,PREVIOUS) |
							TEXENV(2,CONSTANT) | TEXENV_ENVCOLOR;	// envColor*TEXTURE+PREVIOUS
					else // 3 operands not enough
						combine = false;
				}
				else if (env2 == TEXENV_ONE)
				{	// PREVIOUS mul is 1
					env = TEXENV_C3_ADD | TEXENV(0,TEXTURE) | TEXENV(1,PREVIOUS) | (env1<<TEXENV_SRC2_SHIFT);	// env1*TEXTURE+PREVIOUS
				}
				else
					combine = false;
			}

			// combine stages ...
			if (combine)
			{
#ifndef NO_LOG_PP
				//?? move to gl_interface.cpp for logging
				static const char *envNames[] = {
					// corresponds to TEXENV_XXX const (used with TEXENV_SRCx_SHIFT)
					"", "T.c", "(1-T.c)", "T.a", "(1-T.a)", "P.c", "(1-P.c)", "P.a", "(1-P.a)",
					"E.c", "(1-E.c)", "C", "(1-C)", "0", "1"
				};
#define ENV_NAME(x)		envNames[(env >> TEXENV_SRC##x##_SHIFT) & TEXENV_SRC_MASK]

				if (GL_SUPPORT(QGL_NV_TEXTURE_ENV_COMBINE4)) {
					LOG_PP(va("  MT(NV): %X -> %08X == %s x %s + %s x %s\n", blend2, env,
						ENV_NAME(0), ENV_NAME(1), ENV_NAME(2), ENV_NAME(3)));
				} else { // if (GL_SUPPORT(QGL_ATI_TEXTURE_ENV_COMBINE3))
					LOG_PP(va("  MT(ATI): %X -> %08X == %s x %s + %s\n", blend2, env,
						ENV_NAME(0), ENV_NAME(2), ENV_NAME(1)));
				}
#endif // NO_LOG_PP
				st[1].texEnv = env;
				pass->numStages++;
				continue;
			}
		}

		// GL_EXT_TEXTURE_ENV_COMBINE is unsupported here (this extension supports INTERP with SRC_ALPHA only)
		if (GL_SUPPORT(QGL_ARB_TEXTURE_ENV_COMBINE) && blend2 == BLEND(1,1)
			&& passStyle & BLEND_ADDITIVE && st[0].isConstRGBA && st[1].isConstRGBA && tmuUsed == 1)
		{
			LOG_PP(va("  MT(INTERP*2): with \"%s\"\n", *st[1].mapImage[0]->Name));
			st[1].texEnv = TEXENV_C_INTERP | TEXENV_MUL2 | TEXENV_ENVCOLOR | TEXENV_0PREV_1TEX | TEXENV(2,CONSTANT);
			// set RGBA for both stages
			LOG_PP(va("  (old rgba: %X %X", st[0].rgbaConst.rgba, st[1].rgbaConst.rgba));
			for (int k = 0; k < 4; k++)
			{
				int k2 = st[1].rgbaConst.c[k];
				st[0].rgbaConst.c[k] = st[0].rgbaConst.c[k] * 255 / (255*2 - k2);
				st[1].rgbaConst.c[k] = 255 - k2 / 2;
			}
			LOG_PP(va("   new rgba: %X %X)\n",  st[0].rgbaConst.rgba, st[1].rgbaConst.rgba));
			pass->numStages++;
			continue;
		}

		// not combined - begin new pass
		LOG_PP("  not combined\n");
		tmuLeft = 0;
	}
	LOG_PP("-----------------\n");
}
#undef LOG_PP


static void FlushShader ()
{
	guard(FlushShader);

	if (!gl_numIndexes) return;					// buffer is empty
	if (!currentShader->numStages) return;		// wrong shader?

//	DrawTextLeft (va("FlushShader(%s, %d, %d)\n", *currentShader->Name, numVerts, numIndexes));//!!!
	LOG_STRING (va("*** FlushShader(%s, %d, %d) ***\n", *currentShader->Name, gl_numVerts, gl_numIndexes));

	PreprocessShader (currentShader);

	/*------------- prepare renderer --------------*/

	ProcessShaderDeforms (currentShader);
	glVertexPointer (3, GL_FLOAT, sizeof(bufVertex_t), vb->verts);

	GL_CullFace (currentShader->cullMode);
	if (currentShader->usePolygonOffset)
	{
		glEnable (GL_POLYGON_OFFSET_FILL);
		//?? use cvars for units/factor
		glPolygonOffset (-1, -2);
	}

	if (/*numRenderPasses > 1 &&*/ glLockArraysEXT)	//?? GF FX5200 + det.56.72 have geometry bugs when mixing non-CVA/CVA rendering (disappear with gl_showTris=1)
	{
		// do not lock texcoords and colors
		glDisableClientState (GL_COLOR_ARRAY);
		GL_DisableTexCoordArrays ();
		glLockArraysEXT (0, gl_numVerts);
	}

	/*---------------- draw stages ----------------*/
	int		i;
	renderPass_t *pass;
	for (i = 0, pass = renderPasses; i < numRenderPasses; i++, pass++)
	{
		LOG_STRING (va("-- pass %d (mtex %d) --\n", i+1, pass->numStages));
		GL_Lock ();
		GL_SetMultitexture (pass->numStages);

		int		j;
		tempStage_t *st;
		for (j = 0, st = pass->stages; j < pass->numStages; j++, st++)
		{
			GL_SelectTexture (j);
			GL_TexEnv (st->texEnv);
			GL_TexEnvColor (&st->rgbaConst);
			GL_Bind (st->mapImage[0]);
			GenerateTexCoordArray (st, j, st->mapImage[0]);
			GL_TexCoordPointer (vb->texCoord[j]);
		}

		//------ setup color arrays / constant ------
		shaderStage_t *stage = pass->colorStage;
		if (stage->rgbGenType != RGBGEN_CONST || stage->alphaGenType != ALPHAGEN_CONST)
		{
			GenerateColorArray (stage);
			glEnableClientState (GL_COLOR_ARRAY);
			glColorPointer (4, GL_UNSIGNED_BYTE, 0, vb->color);
		}
		else
		{
			glDisableClientState (GL_COLOR_ARRAY);
			glColor4ubv (stage->rgbaConst.c);
		}

		GL_State (pass->glState);
		GL_Unlock ();

		//!! glFog don't works with multi-pass rendering
		if (i == numRenderPasses - 1 && gl_state.haveFullScreen3d && !gl_showFillRate->integer
			&& currentShader->type == SHADERTYPE_NORMAL && !gl_state.is2dMode)
			GL_EnableFog (true);	//!!! else GL_DisableFog()!!!

		glDrawElements (GL_TRIANGLES, gl_numIndexes, GL_UNSIGNED_INT, gl_indexesArray);
	}

	/*----------------- finalize ------------------*/

	if (/*numRenderPasses > 1 &&*/ glUnlockArraysEXT)	//?? ...
		glUnlockArraysEXT ();

	if (currentShader->usePolygonOffset)
		glDisable (GL_POLYGON_OFFSET_FILL);

	// debug
	if (!gl_state.is2dMode)
	{
		DrawTriangles ();
		DrawNormals ();
		gl_speeds.tris += gl_numIndexes * numTmpStages / 3;
		gl_speeds.trisMT += gl_numIndexes * numRenderPasses / 3;
	}
	else
		gl_speeds.tris2D += gl_numIndexes * numTmpStages / 3;

	gl_speeds.numFlushes++;
	gl_numVerts = gl_numIndexes = gl_numExtra = 0;

	unguardf(("%s", *currentShader->Name));
}


static void SetCurrentShader (shader_t *shader)
{
	if (gl_numVerts && gl_numIndexes)
		// we can get situation, when verts==2 and inds==0 due to geometry simplification -- this is OK (??)
		// sample: map "actmet", inside building with rotating glass doors, floor 2: exotic lamp (nested cilinders with alpha)
	{
		DrawTextLeft ("SetCurrentShader() without flush!", RGB(1,0,0));
		Com_WPrintf ("SetCurrentShader(%s) without flush (old: %s, %d verts, %d inds)\n",
			*shader->Name, *currentShader->Name, gl_numVerts, gl_numIndexes);
	}
	currentShader = shader;
	gl_numVerts = gl_numIndexes = gl_numExtra = 0;		// clear buffer
}


static void ReserveVerts (int verts, int inds)
{
	if (gl_numIndexes + inds > MAX_INDEXES || gl_numVerts + verts > MAX_VERTEXES)
		FlushShader ();

	if (verts > MAX_VERTEXES)	Com_DropError ("ReserveVerts: %d > MAX_VERTEXES", verts);
	if (inds > MAX_INDEXES)		Com_DropError ("ReserveVerts: %d > MAX_INDEXES", inds);
}


/*-----------------------------------------------------------------------------
	Scene
-----------------------------------------------------------------------------*/

static void CheckDynamicLightmap (surfacePlanar_t *surf)
{
	if (!gl_dynamic->integer)
		return;

	// check for lightstyle modification (or vertex colors)
	if (!surf->lightmap) return;

	dynamicLightmap_t *dl = surf->lightmap;
	int updateType = 0;
	for (int i = 0; i < dl->numStyles; i++)
	{
		byte style = dl->style[i];
		if (IS_FAST_STYLE(style))
		{
			updateType = 1;		// vertex color
			continue;
		}
		if (dl->modulate[i] != vp.lightStyles[style].value)
		{
			updateType = 2;		// lightmap
			break;
		}
	}

	bool dlightUpdate = (surf->dlightMask && surf->shader->lightmapNumber == LIGHTMAP_VERTEX);
	if (dlightUpdate) updateType |= 1;	// vertex
	if (updateType)
	{
		// require to update lightmap
		for (i = 0; i < dl->numStyles; i++)
			dl->modulate[i] = vp.lightStyles[dl->style[i]].value;
		if (dlightUpdate) dl->modulate[0]--;	// force to update vertex lightmap when dlight disappear

		UpdateDynamicLightmap (surf, updateType == 1, surf->dlightMask);
	}
}


/*-----------------------------------------------------------------------------
	3D tesselators
-----------------------------------------------------------------------------*/

void surfacePlanar_t::Tesselate (refEntity_t &ent)
{
	ReserveVerts (numVerts, numIndexes);

	int firstVert = gl_numVerts;
	gl_numVerts += numVerts;
	int firstIndex = gl_numIndexes;
	gl_numIndexes += numIndexes;

	bufExtra_t *ex = &gl_extra[gl_numExtra++];
	ex->numVerts = numVerts;
	if (lightmap) ex->lmWidth = lightmap->w;
	ex->normal = plane.normal;
	ex->axis = axis;
	ex->dlight = dlights;

	bufVertex_t *v = &vb->verts[firstVert];
	bufTexCoordSrc_t *t = &srcTexCoord[firstVert];
	unsigned *c = &srcVertexColor[firstVert].rgba;

	// copy vertexes
	vertex_t *vs = verts;
	for (int i = 0; i < numVerts; i++, vs++, v++, t++, c++)
	{
		v->xyz = vs->xyz;			// copy vertex
		t->tex[0] = vs->st[0];		// copy texture coords
		t->tex[1] = vs->st[1];
		t->lm[0] = vs->lm[0];		// copy lightmap coords
		t->lm[1] = vs->lm[1];
		*c = vs->c.rgba;			// copy vertex color (sometimes may be ignored??)
	}

	// copy indexes
	int *idx = &gl_indexesArray[firstIndex];
	int *idxSrc = indexes;
	for (i = 0; i < numIndexes; i++)
		*idx++ = *idxSrc++ + firstVert;
}


void surfacePoly_t::Tesselate (refEntity_t &ent)
{
	int		i;

	int numIdx = (numVerts - 2) * 3;
	ReserveVerts (numVerts, numIdx);

	int firstVert = gl_numVerts;
	gl_numVerts += numVerts;
	int firstIndex = gl_numIndexes;
	gl_numIndexes += numIdx;

	bufExtra_t *ex = &gl_extra[gl_numExtra++];
	ex->numVerts = numVerts;
	ex->axis = NULL;
	ex->dlight = NULL;
	ex->normal.Zero ();				// normal = {0,0,0} - compute light for point

	bufVertex_t *v = &vb->verts[firstVert];
	bufTexCoordSrc_t *t = &srcTexCoord[firstVert];
	unsigned *c = &srcVertexColor[firstVert].rgba;

	// copy vertexes
	vertexPoly_t *vs = verts;
	for (i = 0; i < numVerts; i++, vs++, v++, t++, c++)
	{
		v->xyz = vs->xyz;			// copy vertex
		t->tex[0] = vs->st[0];		// copy texture coords
		t->tex[1] = vs->st[1];
//		t->lm[0] = 0;				// lightmap coords are undefined
//		t->lm[1] = 0;
		*c = vs->c.rgba;			// copy vertex color (sometimes may be ignored ?)
	}

	// copy indexes
	int *idx = &gl_indexesArray[firstIndex];
	for (i = 1; i < gl_numVerts - 1; i++)
	{
		*idx++ = firstVert;
		*idx++ = firstVert + i;
		*idx++ = firstVert + i + 1;
	}
}


void surfaceMd3_t::Tesselate (refEntity_t &ent)
{
	int		i;

	int numIdx = numTris * 3;
	ReserveVerts (numVerts, numIdx);

	int firstVert = gl_numVerts;
	gl_numVerts += numVerts;
	int firstIndex = gl_numIndexes;
	gl_numIndexes += numIdx;

	bufExtra_t *ex = &gl_extra[gl_numExtra];
	gl_numExtra += numVerts;

	/*------------- lerp verts ---------------*/
	vertexMd3_t *vs1 = verts + numVerts * currentEntity->frame;
	bufVertex_t *v = &vb->verts[firstVert];
	if (currentEntity->backLerp != 0.0f && currentEntity->frame != currentEntity->oldFrame)
	{
		vertexMd3_t *vs2 = verts + numVerts * currentEntity->oldFrame;
		float backScale = currentEntity->backLerp * MD3_XYZ_SCALE * ent.drawScale;
		float frontLerp = 1.0f - currentEntity->backLerp;
		float frontScale = frontLerp * MD3_XYZ_SCALE * ent.drawScale;
		for (i = 0; i < numVerts; i++, vs1++, vs2++, v++, ex++)
		{
			v->xyz[0] = vs1->xyz[0] * frontScale + vs2->xyz[0] * backScale;
			v->xyz[1] = vs1->xyz[1] * frontScale + vs2->xyz[1] * backScale;
			v->xyz[2] = vs1->xyz[2] * frontScale + vs2->xyz[2] * backScale;
			// lerp normal
			int a1 = vs1->normal & 255;
			int b1 = vs1->normal >> 8;
			float sa1 = SIN_FUNC2(a1,256) * frontLerp;
			int a2 = vs2->normal & 255;
			int b2 = vs2->normal >> 8;
			float sa2 = SIN_FUNC2(a2,256) * currentEntity->backLerp;
			CVec3 &norm = ex->normal;
			norm[0] = sa1 * COS_FUNC2(b1,256) + sa2 * COS_FUNC2(b2,256);
			norm[1] = sa1 * SIN_FUNC2(b1,256) + sa2 * SIN_FUNC2(b2,256);
			norm[2] = COS_FUNC2(a1,256) * frontLerp + COS_FUNC2(a2,256) * currentEntity->backLerp;
			ex->numVerts = 1;
			ex->axis = NULL;
			ex->dlight = NULL;
		}
	}
	else
	{
		float scale = MD3_XYZ_SCALE * ent.drawScale;
		for (i = 0; i < numVerts; i++, vs1++, v++, ex++)	// fast non-lerp case
		{
			v->xyz[0] = vs1->xyz[0] * scale;
			v->xyz[1] = vs1->xyz[1] * scale;
			v->xyz[2] = vs1->xyz[2] * scale;
			int a = vs1->normal & 255;
			int b = vs1->normal >> 8;
			float sa = SIN_FUNC2(a,256);
			CVec3 &norm = ex->normal;
			norm[0] = sa * COS_FUNC2(b,256);	// sin(a)*cos(b)
			norm[1] = sa * SIN_FUNC2(b,256);	// sin(a)*sin(b)
			norm[2] = COS_FUNC2(a,256);			// cos(a)
			ex->numVerts = 1;
			ex->axis = NULL;
			ex->dlight = NULL;
		}
	}

	unsigned *c = &srcVertexColor[firstVert].rgba;
	for (i = 0; i < gl_numVerts; i++)
		*c++ = 0xFFFFFFFF;	//!! vertex color (should not use "RGBGEN_VERTEX", or use another function to generate color; see UberEngine)

	/*----------- copy texcoords -------------*/
	bufTexCoordSrc_t *t = &srcTexCoord[firstVert];
	float *ts = texCoords;
	for (i = 0; i < numVerts; i++, t++)
	{
		t->tex[0] = *ts++;
		t->tex[1] = *ts++;
	}

	/*------------ copy indexes --------------*/
	int *idx = &gl_indexesArray[firstIndex];
	int *idxSrc = indexes;
	for (i = 0; i < numIdx; i++)
		*idx++ = *idxSrc++ + firstVert;
}


/*-----------------------------------------------------------------------------
	Debug output
-----------------------------------------------------------------------------*/


static void FlashColor ()
{
	int i = appRound (vp.time / 3 * TABLE_SIZE);
	glColor3f (sinTable[i & TABLE_MASK] / 2 + 0.5,
			   sinTable[i + 100 & TABLE_MASK] / 2 + 0.5,
			   sinTable[600 - i & TABLE_MASK] / 2 + 0.5);
}


// draw entity bounding boxes
static void DrawBBoxes ()
{
	int		i;
	refEntity_t *ent;

	// common GL setup
	glLoadMatrixf (&vp.modelMatrix[0][0]);		// world matrix
	GL_SetMultitexture (0);		// disable texturing with all tmu's
	glDisableClientState (GL_COLOR_ARRAY);

	if (gl_showbboxes->integer == 2)
		FlashColor ();
	else
		glColor3f (0.6, 0.6, 0.2);
	GL_State (0);

	/*-------- draw bounding boxes --------*/
	for (i = 0, ent = gl_entities + vp.firstEntity; i < vp.numEntities; i++, ent++)
	{
		bufVertex_t v[8];
		static const int inds[24] = {
			0,1, 1,3, 3,2, 2,0,	// rect1
			4,5, 5,7, 7,6, 6,4,	// rect2
			0,4, 1,5, 3,7, 2,6	// connectors
		};

		if (!ent->visible)
			continue;			// entity is culled or from different scene

		model_t	*m;
		if (!(m = ent->model)) continue;	// no bbox info

#if 0
		{
			float	mins2[2], maxs2[2];

			if (GetBoxRect (ent, ent->size2, mins2, maxs2))
			{
				CVec3	h;
				static const int idx2[4] = {0, 2, 3, 1};

				VectorMA (ent->center, mins2[0], vp.view.axis[1], h);
				VectorMA (h, mins2[1], vp.view.axis[2], v[0].xyz);
				VectorMA (h, maxs2[1], vp.view.axis[2], v[1].xyz);
				VectorMA (ent->center, maxs2[0], vp.view.axis[1], h);
				VectorMA (h, mins2[1], vp.view.axis[2], v[2].xyz);
				VectorMA (h, maxs2[1], vp.view.axis[2], v[3].xyz);

				GL_State (GLSTATE_POLYGON_LINE);
				GL_DepthRange (DEPTH_NEAR);
				glVertexPointer (3, GL_FLOAT, sizeof(bufVertex_t), v);
				glColor3f (0.5, 0.1, 0.1);
				glDrawElements (GL_QUADS, 4, GL_UNSIGNED_INT, idx2);

				GL_State (BLEND(S_ALPHA,M_S_ALPHA));
				GL_DepthRange (DEPTH_NORMAL);
				if (!ent->worldMatrix)
					glColor4f (0.1, 0.1, 0.3, 0.4);
				else
					glColor4f (0.5, 0.1, 0.1, 0.4);
				glDrawElements (GL_QUADS, 4, GL_UNSIGNED_INT, idx2);

				glColor3f (0.6, 0.6, 0.2);
			}
		}
#endif
		// generate verts
		for (int j = 0; j < 8; j++)
		{
			CVec3 tmp;
			tmp[0] = (j & 1) ? ent->size2[0] : -ent->size2[0];
			tmp[1] = (j & 2) ? ent->size2[1] : -ent->size2[1];
			tmp[2] = (j & 4) ? ent->size2[2] : -ent->size2[2];

			// project point to a world coordinate system
			if (!ent->worldMatrix)
				UnTransformPoint (ent->center, ent->coord.axis, tmp, v[j].xyz);
			else
				VectorAdd (ent->center, tmp, v[j].xyz);
		}

		// draw it
		if (gl_showbboxes->integer == 3)
		{
			if (!ent->worldMatrix)
				glColor3f (0.6, 0.6, 0.2);
			else
				glColor3f (0.2, 0.8, 0.2);
		}
		glVertexPointer (3, GL_FLOAT, sizeof(bufVertex_t), v);
		glDrawElements (GL_LINES, 24, GL_UNSIGNED_INT, inds);
	}
}


bool DrawTriangles ()
{
	if (!gl_showTris->integer) return false;

	gl_depthMode_t prevDepth = gl_state.currentDepthMode;
	GL_SetMultitexture (0);		// disable texturing
	if (gl_showTris->integer - 1 & 1)
		GL_State (GLSTATE_POLYGON_LINE|GLSTATE_DEPTHWRITE);	// use depth test
	else
	{
		// no depth test
		GL_State (GLSTATE_POLYGON_LINE|GLSTATE_DEPTHWRITE);
		GL_DepthRange (DEPTH_NEAR);
	}
	// setup colors
	if (gl_showTris->integer - 1 & 2)
		FlashColor ();
	else
		glColor3f (0, 0, 0);
	glDisableClientState (GL_COLOR_ARRAY);
	// draw
	glDrawElements (GL_TRIANGLES, gl_numIndexes, GL_UNSIGNED_INT, gl_indexesArray);
	// restore state
	GL_DepthRange (prevDepth);

	return true;
}


bool DrawNormals ()
{
	if (!gl_showNormals->integer) return false;

	gl_depthMode_t prevDepth = gl_state.currentDepthMode;
	GL_SetMultitexture (0);		// disable texturing
	if (gl_showNormals->integer - 1 & 1)
		GL_State (GLSTATE_POLYGON_LINE|GLSTATE_DEPTHWRITE);	// use depth test
	else
	{
		// no depth test
		GL_State (GLSTATE_POLYGON_LINE|GLSTATE_DEPTHWRITE);
		GL_DepthRange (DEPTH_NEAR);
	}
	// setup colors
	if (gl_showNormals->integer - 1 & 2)
		FlashColor ();
	else
		glColor3f (0, 0, 0);
	glDisableClientState (GL_COLOR_ARRAY);
	// draw
	glBegin (GL_LINES);
	bufVertex_t *vec = vb->verts;
	const bufExtra_t *ex = gl_extra;
	for (int i = 0; i < gl_numExtra; i++, ex++)
	{
		for (int j = 0; j < ex->numVerts; j++, vec++)
		{
			glVertex3fv (vec->xyz.v);
			CVec3 vec2;
			VectorAdd (vec->xyz, ex->normal, vec2);
			glVertex3fv (vec2.v);
		}
	}
	glEnd ();
	// restore state
	GL_DepthRange (prevDepth);

	return true;
}



void surfaceEntity_t::Tesselate (refEntity_t &ent)
{
	if (entity->flags & RF_BBOX)
	{
		bufVertex_t v[8];
		static const int inds[24] = {
			0,1, 1,3, 3,2, 2,0,	// rect1
			4,5, 5,7, 7,6, 6,4,	// rect2
			0,4, 1,5, 3,7, 2,6	// connectors
		};

		GL_SetMultitexture (0);		// disable texturing with all tmu's
		glDisableClientState (GL_COLOR_ARRAY);
		glColor4ubv (entity->shaderColor.c);
		GL_State (0);

		// generate verts
		for (int i = 0; i < 8; i++)
		{
			CVec3 tmp;
			tmp[0] = (i & 1) ? entity->size2[0] : -entity->size2[0];
			tmp[1] = (i & 2) ? entity->size2[1] : -entity->size2[1];
			tmp[2] = (i & 4) ? entity->size2[2] : -entity->size2[2];
			// project point to a world coordinate system
			entity->coord.UnTransformPoint (tmp, v[i].xyz);
		}
		// draw it
		glVertexPointer (3, GL_FLOAT, sizeof(bufVertex_t), v);
		glDrawElements (GL_LINES, 24, GL_UNSIGNED_INT, inds);
	}
	else
		DrawTextLeft (va("Unknown ent surf flags: %X", entity->flags), RGB(1,0,0));
}



/*-----------------------------------------------------------------------------
	Drawing the scene
-----------------------------------------------------------------------------*/

void surfaceParticle_t::Tesselate (refEntity_t &ent)
{
	//!! oprimize this (vertex arrays, etc.)

	GL_SetMultitexture (1);
	GL_Bind (gl_particleImage);

	GL_State (BLEND(S_ALPHA,M_S_ALPHA)|/*GLSTATE_DEPTHWRITE|*/GLSTATE_ALPHA_GT0);
	CVec3 up, right;
	VectorScale (vp.view.axis[1], 1.5f, up);
	VectorScale (vp.view.axis[2], 1.5f, right);

	glBegin (GL_TRIANGLES);
	for (const particle_t *p = part; p; p = p->drawNext)
	{
		// get Z-coordinate
		CVec3 tmp;
		VectorSubtract (p->org, vp.view.origin, tmp);
		float scale = dot (tmp, vp.view.axis[0]) * vp.fov_scale;

		if (scale < 10)
			continue;		// too near
		if (scale < 20.0f)
			scale = 1;
		else
			scale = scale / 500.0f + 1.0f;

		int alpha = appRound (p->alpha * 255);
		alpha = bound(alpha, 0, 255);

		byte	c[4];
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
		glColor4ubv (c);

		glTexCoord2f (0.0625f, 0.0625f);
		glVertex3fv (p->org.v);

		glTexCoord2f (1.0625f, 0.0625f);
		glVertex3f (p->org[0] + up[0] * scale, p->org[1] + up[1] * scale, p->org[2] + up[2] * scale);

		glTexCoord2f (0.0625f, 1.0625f);
		glVertex3f (p->org[0] + right[0] * scale, p->org[1] + right[1] * scale, p->org[2] + right[2] * scale);
	}
	glEnd ();
}


void BK_DrawScene ()
{
	guard(DrawScene);

	if (!renderingEnabled) return;

	LOG_STRING (va("******** R_DrawScene: (%d, %d) - (%d, %d) ********\n", vp.x, vp.y, vp.x+vp.w, vp.y+vp.h));

	if (gl_numVerts) FlushShader ();
	GL_Set3DMode (&vp);

	// sort surfaces
	if (gl_finish->integer == 2) glFinish ();
	gl_speeds.beginSort = appCycles ();
	SortSurfaces (&vp, sortedSurfaces);
	gl_speeds.begin3D = appCycles ();
	gl_speeds.surfs += vp.numSurfaces;

	currentDlightMask = 0;
	double worldTime = vp.time;

	/*------------ draw sky --------------*/

	if (!(vp.flags & RDF_NOWORLDMODEL))
	{
		//?? setup currentEntity, currentShader before sky drawing
		DrawSky ();
	}

	/*--- update all dynamic lightmaps ---*/

	int				index;
	surfaceInfo_t	**si;

	for (index = 0, si = sortedSurfaces; index < vp.numSurfaces; index++, si++)
	{
		surfaceBase_t *surf = (*si)->surf;
		if (surf && surf->type == SURFACE_PLANAR)
			CheckDynamicLightmap (static_cast<surfacePlanar_t*>(surf));
	}

	/*-------- draw world/models ---------*/

	int currentShaderNum = -1, currentEntityNum = -1;
	bool currentWorld = false;
	for (index = 0, si = sortedSurfaces; index < vp.numSurfaces; index++, si++)
	{
		surfaceBase_t *surf = (*si)->surf;
		unsigned code   = (*si)->sort;
		unsigned shNum  = (code >> SHADERNUM_SHIFT) & SHADERNUM_MASK;
		unsigned entNum = (code >> ENTITYNUM_SHIFT) & ENTITYNUM_MASK;
		unsigned dlightMask;
		if ((code >> DLIGHTNUM_SHIFT) & DLIGHTNUM_MASK)
			dlightMask = surf->dlightMask;
		else
			dlightMask = 0;

		if (shNum != currentShaderNum || entNum != currentEntityNum || currentDlightMask != dlightMask)
		{
			// flush data for the previous shader
			FlushShader ();

			// change shader
			shader_t *shader = GetShaderByNum (shNum);
			SetCurrentShader (shader);
			currentDlightMask = dlightMask;
			LOG_STRING (va("******** shader = %s ********\n", *shader->Name));
			currentShaderNum = shNum;
		}

		if (entNum != currentEntityNum)
		{
			currentEntity = &gl_entities[entNum];
			currentEntityNum = entNum;

			bool isWorld = (entNum == ENTITYNUM_WORLD) || currentEntity->worldMatrix;

			if (isWorld)
			{
				if (!currentWorld)		// previous entity was not world
				{
					LOG_STRING (va("******** entity = WORLD ********\n"));
					glLoadMatrixf (&vp.modelMatrix[0][0]);
				}
				gl_state.inverseCull = false;
				GL_DepthRange (DEPTH_NORMAL);
			}
			else
			{
				LOG_STRING (va("******** entity = %s ********\n", *currentEntity->model->Name));
				glLoadMatrixf (&currentEntity->modelMatrix[0][0]);
				gl_state.inverseCull = currentEntity->mirror;
				GL_DepthRange (currentEntity->flags & RF_DEPTHHACK ? DEPTH_HACK : DEPTH_NORMAL);
			}
			currentWorld = isWorld;
			vp.time = (entNum == ENTITYNUM_WORLD) ? worldTime : currentEntity->time;
		}

		surf->Tesselate (*currentEntity);
	}

	/*--------- finilize/debug -----------*/
	FlushShader ();
	vp.time = worldTime;				// restore time

	GL_DepthRange (DEPTH_NORMAL);

	if (gl_showbboxes->integer)
		DrawBBoxes ();

	if (gl_showLights->integer)
		ShowLights ();

	if (gl_finish->integer == 2) glFinish ();
	gl_speeds.end3D = appCycles ();

	unguard;
}



/*-----------------------------------------------------------------------------
	2D tesselators
-----------------------------------------------------------------------------*/

void BK_DrawPic (shader_t *shader, int x, int y, int w, int h, float s1, float t1, float s2, float t2, unsigned color, byte flipMode)
{
	if (!renderingEnabled) return;

	if (currentShader != shader)
	{
		FlushShader ();
		SetCurrentShader (shader);
	}

	GL_Set2DMode ();

	ReserveVerts (4, 6);

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

	/*   0 (0,0) -- 1 (1,0)
	 *     |   \____   |
	 *     |        \  |
	 *   3 (0,1) -- 2 (1,1)
	 */
	bufVertex_t *v = &vb->verts[gl_numVerts];
	bufTexCoordSrc_t *t = &srcTexCoord[gl_numVerts];
	unsigned *c = &srcVertexColor[gl_numVerts].rgba;
	int idx0 = gl_numVerts;
	gl_numVerts += 4;

	// set vert.z
	v[0].xyz[2] = v[1].xyz[2] = v[2].xyz[2] = v[3].xyz[2] = 0;
	// set vert.x
	v[0].xyz[0] = v[3].xyz[0] = x;
	v[1].xyz[0] = v[2].xyz[0] = x + w;
	// set vert.y
	v[0].xyz[1] = v[1].xyz[1] = y;
	v[2].xyz[1] = v[3].xyz[1] = y + h;

	//?? make as function, use for sprites too
	//?? make consts for flipMode (1,2,4)
	// swap texture coords
	if (flipMode & 1) Exchange (s1, s2);
	if (flipMode & 2) Exchange (t1, t2);
	// set s
	t[0].tex[0] = t[3].tex[0] = s1;
	t[1].tex[0] = t[2].tex[0] = s2;
	// set t
	t[0].tex[1] = t[1].tex[1] = t1;
	t[2].tex[1] = t[3].tex[1] = t2;
	// flip s and t
	if (flipMode & 4)
	{
		/*
		 *	0 1		==	s1,t1	s1,t2	>>		s1,t1	s2,t1
		 *	3 2		==	s2,t1	s2,t2   >>		s1,t2	s2,t2
		 */
		// swap points 1 and 3
		Exchange (t[1].tex[0], t[3].tex[0]);
		Exchange (t[1].tex[1], t[3].tex[1]);
	}
	// store colors
	c[0] = c[1] = c[2] = c[3] = color;

	int *idx = &gl_indexesArray[gl_numIndexes];
	gl_numIndexes += 6;
	*idx++ = idx0+0; *idx++ = idx0+1; *idx++ = idx0+2;
	*idx++ = idx0+0; *idx++ = idx0+2; *idx++ = idx0+3;
}


// This is slightly optimized version of BK_DrawPic() for drawing texts
void BK_DrawText (const char *text, int len, int x, int y, int w, int h, unsigned color)
{
	if (!len) return;
	if (!renderingEnabled) return;

	if (currentShader != gl_concharsShader)
	{
		FlushShader ();
		SetCurrentShader (gl_concharsShader);
	}

	GL_Set2DMode ();

	ReserveVerts (len * 4, len * 6);

	bufVertex_t *v = &vb->verts[gl_numVerts];
	bufTexCoordSrc_t *t = &srcTexCoord[gl_numVerts];
	unsigned *c = &srcVertexColor[gl_numVerts].rgba;
	int *idx = &gl_indexesArray[gl_numIndexes];
	int idx0 = gl_numVerts;

	float x1 = x;
	float x2 = x + w;

	float size = 1.0f / 16;
	for (int i = len; i > 0 && x1 < vid_width; i--)
	{
		char chr = *text++;
		if (chr != ' ')		// space is hardcoded
		{
			float s1 = (chr & 15) * size;
			float t1 = (chr >> 4 & 15) * size;
			float s2 = s1 + size;
			float t2 = t1 + size;

			// set vert.z
			v[0].xyz[2] = v[1].xyz[2] = v[2].xyz[2] = v[3].xyz[2] = 0;
			// set vert.x
			v[0].xyz[0] = v[3].xyz[0] = x1;
			v[1].xyz[0] = v[2].xyz[0] = x2;
			// set vert.y
			v[0].xyz[1] = v[1].xyz[1] = y;
			v[2].xyz[1] = v[3].xyz[1] = y + h;
			// set s
			t[0].tex[0] = t[3].tex[0] = s1;
			t[1].tex[0] = t[2].tex[0] = s2;
			// set t
			t[0].tex[1] = t[1].tex[1] = t1;
			t[2].tex[1] = t[3].tex[1] = t2;
			// store colors
			c[0] = c[1] = c[2] = c[3] = color;

			*idx++ = idx0+0; *idx++ = idx0+1; *idx++ = idx0+2;
			*idx++ = idx0+0; *idx++ = idx0+2; *idx++ = idx0+3;

			idx0 += 4;
			v += 4;
			t += 4;
			c += 4;
			gl_numVerts += 4;
			gl_numIndexes += 6;
		}
		x1 = x2;
		x2 += w;
	}
}


/*---------------------------------------------------------------------------*/

void BK_BeginFrame ()
{
	if (!renderingEnabled) return;

	if (gl_clear->integer && !gl_state.useFastSky)
	{
		glClearColor (0.1, 0.6, 0.3, 1);
		glClear (GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	}
	if (gl_finish->integer == 2) glFinish ();

//??	GL_ResetState (); -- not helps; see bugs.txt for info
}


void BK_EndFrame ()
{
	if (!renderingEnabled) return;

	FlushShader ();
	if (screenshotName)
		PerformScreenshot ();
	ShowImages ();				// debug
	QGL_SwapBuffers ();
	gl_state.is2dMode = false;	// invalidate 2D mode, because of buffer switching

	if (gl_finish->integer) glFinish ();
}


/*-----------------------------------------------------------------------------
	Init/shutdown
-----------------------------------------------------------------------------*/

void BK_Init ()
{
CVAR_BEGIN(vars)
	CVAR_VAR(gl_clear, 0, 0),
	CVAR_VAR(gl_finish, 0, CVAR_ARCHIVE),
#ifdef SPY_SHADER
	CVAR_VAR(gl_spyShader, 0, 0),
#endif
	CVAR_VAR(gl_showbboxes, 0, CVAR_CHEAT),
	CVAR_VAR(gl_showTris, 0, CVAR_CHEAT),
	CVAR_VAR(gl_showNormals, 0, CVAR_CHEAT)
CVAR_END
	Cvar_GetVars (ARRAY_ARG(vars));
	ClearBuffers ();

	// size of 1 buffer (depends on multitexturing ability)
	int vbSize = vertexBuffer_t::getSize (gl_config.maxActiveTextures);
	// alloc
	vb = (vertexBuffer_t*)appMalloc (vbSize, 16);
}


void BK_Shutdown ()
{
	if (!vb) return;			// not initialized
	appFree (vb);
	vb = NULL;
}


} // namespace
