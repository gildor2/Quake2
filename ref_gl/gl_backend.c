#include "gl_local.h"
#include "gl_shader.h"
#include "gl_backend.h"
#include "gl_model.h"

#define SWAP_ON_BEGIN		// call GLimp_EndFrame() with SwapBuffers() on frame begin (or frame end if not defined)

/* LATER: replace (almost) all ap.time -> shader.time (or global "shaderTime") ??
 * (because time may be take (if engine will be extended) from entity)
 * and: all gl_refdef-> something (flags & RDF_NOWORLDMODEL !!)
 */


typedef union
{
	byte	c[4];
	int		rgba;
} bufColor_t;

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


/*------------- Command buffer -------------*/

byte		backendCommands[MAX_BACKEND_COMMANDS];
int			backendCmdSize;

bufColor_t	currentColor;

shader_t	*currentShader;
refEntity_t *currentEntity;


/*-------------- Vertex arrays ---------------*/


#define MAX_VERTEXES	2048
#define MAX_INDEXES		(MAX_VERTEXES*6)

#define NUM_VERTEX_BUFFERS	64

typedef struct
{
	bufColor_t		color[MAX_VERTEXES];
	bufTexCoord_t	texCoord0[MAX_VERTEXES], texCoord1[MAX_VERTEXES];
	bufVertex_t		verts[MAX_VERTEXES];
} vertexBuffer_t;

static vertexBuffer_t	*vb, *agpBuffer;// pointers to current buffer and to all buffers
static int				currentBuffer;	// index of the current buffer

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
static bufColor_t		srcVertexColor[MAX_VERTEXES];
static bufTexCoordSrc_t	srcTexCoord[MAX_VERTEXES];
static bufVertex_t		normals[MAX_VERTEXES];

static int numVerts, numIndexes;


/*--- Tables for fast wave function computation ---*/

static float sinTable[1024];
static float squareTable[1024];
static float triangleTable[1024];
static float sawtoothTable[1024];
static float inverseSwatoothTable[1024];

static float sqrtTable[256];
static int   noiseTablei[256];
static float noiseTablef[256];


static void InitFuncTables (void)
{
	int		i, n;

	srand (1001);

	for (i = 0; i < 1024; i++)
	{
		if (i < 512)
			squareTable[i] = -1;
		else
			squareTable[i] = 1;

		sinTable[i] = sin (i / 512.0f * M_PI);		// 0 -- 0, 1024 -- 2*pi
		sawtoothTable[i] = i / 1024.0f;
		inverseSwatoothTable[i] = 1 - i / 1024.0f;

		n = (i + 256) & 0x3FF;						// make range -1..1..-1
		if (n >= 512)
			n = 1024 - n;
		triangleTable[i] = (n - 256) / 256.0f;
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


static float *TableForFunc (waveFunc_t func)
{
#if 1
	static float *funcs[5] = {
		sinTable, squareTable, triangleTable, sawtoothTable, inverseSwatoothTable
	};

	return funcs[func];
#else
	switch (func)
	{
	case FUNC_SIN:
		return sinTable;
	case FUNC_SQUARE:
		return squareTable;
	case FUNC_TRIANGLE:
		return triangleTable;
	case FUNC_SAWTOOTH:
		return sawtoothTable;
	case FUNC_INVERSESAWTOOTH:
		return inverseSwatoothTable;
	}
	Com_Error (ERR_FATAL, "R_TableForFunc: invalid function %d", func);
	return NULL;			// never; return something (for compiler)
#endif
}


/*------------------ Scene -----------------*/

// surfaces
#define MAX_SCENE_SURFACES	(64*1024)

static surfaceInfo_t surfaceBuffer[MAX_SCENE_SURFACES], *sortedSurfaces[MAX_SCENE_SURFACES];
static int numSurfacesTotal;


/*--------------- Draw portal --------------*/

static viewPortal_t ap;	// active portal

/*------------------------------------------*/


// forward declaration for skybox
static void AddSkySurfaces (void);


static void SetCurrentShader (shader_t *shader)	//?? + int fogNum
{
	if (numVerts > 0)
	{
		DrawTextLeft ("SetCurrentShader() without flush!",1,0,0);
		Com_WPrintf ("SetCurrentShader(%s) without flush (old: %s, %d verts, %d inds)\n",
			shader->name, currentShader->name, numVerts, numIndexes);
	}
	currentShader = shader;
	numVerts = numIndexes = 0;	// clear buffer
	//?? setup shader time
}


static void GenericStageIterator (void);


static void RB_EndSurface (void)
{
//	DrawTextLeft(va("EndSurface(%s,%d,%d)\n",currentShader->name,numVerts,numIndexes),1,1,1);//!!!
	if (!numIndexes) return;	// buffer is empty

//	if (numIndexes > MAX_INDEXES)
//		Com_Error (ERR_FATAL, "RB_EndSurface: MAX_INDEXES hit");
//	if (numVerts > MAX_VERTEXES)
//		Com_Error (ERR_FATAL, "RB_EndSurface: MAX_VERTEXES hit");

	GenericStageIterator ();
	SwitchVertexArray ();
	WaitVertexArray ();

	numVerts = numIndexes = 0;
}


static void RB_CheckOverflow (int verts, int inds)
{
	if (numIndexes + inds > MAX_INDEXES || numVerts + verts > MAX_VERTEXES)
		RB_EndSurface ();

	if (verts > MAX_VERTEXES)
		Com_Error (ERR_FATAL, "RB_CheckOverflow: verts > MAX_VERTEXES");
	if (inds > MAX_INDEXES)
		Com_Error (ERR_FATAL, "RB_CheckOverflow: inds > MAX_INDEXES");
}


/*----- Process deforms, ...gens etc. ------*/


static void ProcessShaderDeforms (shader_t *sh)
{
	int		i, j;
	deformParms_t *deform;
	float	*table, f, t;
	bufVertex_t *vec, *norm;


	if (!sh->numDeforms)
		return;

	for (i = 0, deform = &sh->deforms[0]; i < sh->numDeforms; i++, deform++)
	{
		switch (deform->type)
		{
		case DEFORM_WAVE:
			table = TableForFunc (deform->wave.type);
			t = deform->wave.freq * ap.time + deform->wave.phase;
			for (j = 0, vec = vb->verts, norm = normals; j < numVerts; j++, vec++, norm++)
			{
				f = table[(int)((t + deform->waveDiv * (vec->xyz[0] + vec->xyz[1] + vec->xyz[2])) * 1024) & 0x3FF]
					* deform->wave.amp + deform->wave.base;
				vec->xyz[0] += f * norm->xyz[0];
				vec->xyz[1] += f * norm->xyz[1];
				vec->xyz[2] += f * norm->xyz[2];
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
		rgba = ((int)(st->rgbGenConst[0]*255) << 0) + ((int)(st->rgbGenConst[1]*255) << 8) +
			   ((int)(st->rgbGenConst[2]*255) << 16) + ((int)(st->alphaGenConst*255) << 24);
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
			vb->color[i].c[0] = srcVertexColor[i].c[0] * gl_config.identityLightValue_f;
			vb->color[i].c[1] = srcVertexColor[i].c[1] * gl_config.identityLightValue_f;
			vb->color[i].c[2] = srcVertexColor[i].c[2] * gl_config.identityLightValue_f;
		}
		break;
	case RGBGEN_ONE_MINUS_VERTEX:
		for (i = 0; i < numVerts; i++)
		{
			vb->color[i].c[0] = 255 - srcVertexColor[i].c[0] * gl_config.identityLightValue_f;
			vb->color[i].c[1] = 255 - srcVertexColor[i].c[1] * gl_config.identityLightValue_f;
			vb->color[i].c[2] = 255 - srcVertexColor[i].c[2] * gl_config.identityLightValue_f;
		}
		break;
	//!! other types
	}

	if ((st->rgbGenType == RGBGEN_CONST && st->alphaGenType == ALPHAGEN_CONST) ||
		(st->rgbGenType == RGBGEN_EXACT_VERTEX && st->alphaGenType == ALPHAGEN_VERTEX))
		return;	// alpha is already set

	/*--------- alphaGen ----------*/
	switch (st->alphaGenType)
	{
	case ALPHAGEN_CONST:
		a = (int) (255 * st->alphaGenConst);
		for (i = 0; i < numVerts; i++)
			vb->color[i].c[3] = a;
		break;
	case ALPHAGEN_ENTITY:
		a = currentEntity->shaderRGBA[3];
		for (i = 0; i < numVerts; i++)
			vb->color[i].c[3] = a;
	case ALPHAGEN_VERTEX:
		break;	//?? now: filled (because rgbGen above is dummy)
	//!! other types
	}
}


static void GenerateTexCoordArray (shaderStage_t *st)
{
	int				i, j;
	stageTMU_t		*tmu;
	tcModParms_t	*tcmod;
	bufTexCoord_t	*dst, *dst2;
	bufTexCoordSrc_t *src;

	for (i = 0, tmu = st->tmu; i < 2, tmu->numAnimTextures > 0; i++, tmu++)
	{
		// choose destination array
		if (i == 0)
			dst = vb->texCoord0;
		else
			dst = vb->texCoord1;

		src = srcTexCoord;
		dst2 = dst;		// save this for tcMod

		// process tcGen
		switch (tmu->tcGenType)
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
		//!! other types
		}

		// process tcMod
		for (j = 0, tcmod = tmu->tcModParms; j < tmu->numTcMods; j++, tcmod++)
		{
			int		k;
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
					float	*vec;

					vec = &vb->verts[k].xyz[0];
					dst->tex[0] += sinTable[(int)(((vec[0] + vec[2]) / 1024.0f + f1) * 1024) & 0x3FF] * f2;	// Q3: vec[0] + vec[2]
					dst->tex[1] += sinTable[(int)(((vec[1] + vec[2]) / 1024.0f + f1) * 1024) & 0x3FF] * f2;	// Q3: vec[1]
				}
				break;
			case TCMOD_WARP:
				for (k = 0; k < numVerts; k++, dst++)
				{
					f1 = dst->tex[0];
					f2 = dst->tex[1];
					dst->tex[0] = f1 / 64.0f + sinTable[(int)((f2 / 16.0f + ap.time) / (2.0f * M_PI) * 1024) & 0x3FF] / 16.0f;
					dst->tex[1] = f2 / 64.0f + sinTable[(int)((f1 / 16.0f + ap.time) / (2.0f * M_PI) * 1024) & 0x3FF] / 16.0f;
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
			//!! other types
			}
		}
	}
}


/*------------ Stage iterators -------------*/


static void BindStageImage (stageTMU_t *tmu)
{
	if (tmu->numAnimTextures == 1)
		GL_Bind (tmu->mapImage[0]);
	else
		GL_Bind (tmu->mapImage[(int)(ap.time * tmu->animMapFreq) % tmu->numAnimTextures]);
}


static void SetupColorArrays (shaderStage_t *stage)
{
	if (stage->rgbGenType != RGBGEN_CONST || stage->alphaGenType != ALPHAGEN_CONST)
	{
		GenerateColorArray (stage);
		qglEnableClientState (GL_COLOR_ARRAY);
		qglColorPointer (4, GL_UNSIGNED_BYTE, 0, vb->color);
	}
	else
	{
		qglDisableClientState (GL_COLOR_ARRAY);
		qglColor4f (stage->rgbGenConst[0], stage->rgbGenConst[1], stage->rgbGenConst[2], stage->alphaGenConst);
	}
}


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
	//?? perform gl_primitives variable
	qglDrawElements (GL_TRIANGLES, numIndexes, GL_UNSIGNED_INT, indexes);
}


#define RELAX_COLOR_ARRAYS

static void GenericStageIterator (void)
{
	int		numStages, i;
	shaderStage_t *stage;
	qboolean multiPass;

//	DrawTextLeft (va("StageIterator(%s, %d, %d)\n", currentShader->name, numVerts, numIndexes),1,1,1);//!!!
	LOG_STRING (va("*** StageIterator(%s, %d, %d) ***\n", currentShader->name, numVerts, numIndexes));

	if (!currentShader->numStages)
		return;

	ProcessShaderDeforms (currentShader);

	GL_CullFace (currentShader->cullMode);
	if (currentShader->usePolygonOffset)
	{
		qglEnable (GL_POLYGON_OFFSET_FILL);
		//?? use cvars r_offsetunits = -2; t_offsetfactor = -1;
		qglPolygonOffset (-1, -2);	// offsetFactor, offsetUnits
	}

	numStages = currentShader->numStages;
	stage = currentShader->stages;

	/*------------- prepare renderer --------------*/
	if (numStages > 1 || stage->texEnvMode)
	{
		multiPass = true;
		qglDisableClientState (GL_COLOR_ARRAY);	//?? is it needed?
		qglDisableClientState (GL_TEXTURE_COORD_ARRAY); // ...
	}
	else
	{
		// single-pass shader: only one set of colors and textures
		multiPass = false;
#ifdef RELAX_COLOR_ARRAYS
		SetupColorArrays (stage);
#else
		GenerateColorArray (stage);
		qglEnableClientState (GL_COLOR_ARRAY);
		qglColorPointer (4, GL_UNSIGNED_BYTE, 0, vb->color);
#endif
		GenerateTexCoordArray (stage);
		qglEnableClientState (GL_TEXTURE_COORD_ARRAY);
		qglTexCoordPointer (2, GL_FLOAT, 0, vb->texCoord0);
	}

	qglVertexPointer (3, GL_FLOAT, sizeof(bufVertex_t), vb->verts);
	if (qglLockArraysEXT)
		qglLockArraysEXT (0, numVerts);

	if (multiPass)
	{
#ifndef RELAX_COLOR_ARRAYS
		qglEnableClientState (GL_COLOR_ARRAY);	// keep it always on?
#endif
		qglEnableClientState (GL_TEXTURE_COORD_ARRAY);
	}

	/*---------------- draw stages ----------------*/
	for (i = 0; i < numStages; i++, stage++)
	{
		if (multiPass)
		{
#ifdef RELAX_COLOR_ARRAYS
			SetupColorArrays (stage);
#else
			GenerateColorArray (stage);
			qglColorPointer (4, GL_UNSIGNED_BYTE, 0, vb->color);
#endif
			GenerateTexCoordArray (stage);
//			qglEnableClientState (GL_COLOR_ARRAY);
		}
		if (!stage->tmu[1].numAnimTextures)
		{
			/*----- no multitexturing ------*/
			if (multiPass)
				qglTexCoordPointer (2, GL_FLOAT, 0, vb->texCoord0);
//			if (vertexLighting && gl_lightmap) -- unimplemented
//				GL_Bind(gl_whiteImage) or GL_Bind(NULL) (disable texturing)
//			else
				BindStageImage (&stage->tmu[0]);
			GL_State (stage->glState);
			DrawArrays (indexesArray, numIndexes);
		}
		else
		{
			/*----- use multitexturing -----*/
			GL_State (stage->glState);
			//?? process portals: qglPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
			// setup TMU 0
			GL_SelectTexture (0);
			qglTexCoordPointer (2, GL_FLOAT, 0, vb->texCoord0);
			BindStageImage (&stage->tmu[0]);

			// setup TMU 1
			GL_SelectTexture (1);
			qglEnable (GL_TEXTURE_2D);		// enable multitexturing
			qglEnableClientState (GL_TEXTURE_COORD_ARRAY);
			GL_TexEnv (stage->texEnvMode);	// for gl_lightmap force GL_REPLACE
			qglTexCoordPointer (2, GL_FLOAT, 0, vb->texCoord1);
			BindStageImage (&stage->tmu[1]);

			DrawArrays (indexesArray, numIndexes);

			qglDisable (GL_TEXTURE_2D);		// disable multitexturing
			GL_SelectTexture (0);
		}
	}

	/*----------------- finalize ------------------*/
	if (qglUnlockArraysEXT)
		qglUnlockArraysEXT ();

	if (currentShader->usePolygonOffset)
		qglDisable (GL_POLYGON_OFFSET_FILL);

	gl_speeds.numIterators++;
}


/*---------------- Skybox ------------------*/


float	skyMins[2][6], skyMaxs[2][6];
float	sky_min, sky_max;


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

			if (dv < 0.001) continue;	// don't divide by zero

			j = vecToSt[axis][0];
			if (j < 0)	s = -verts[-j - 1] / dv;
			else		s = verts[j - 1] / dv;

			j = vecToSt[axis][1];
			if (j < 0)	t = -verts[-j - 1] / dv;
			else		t = verts[j - 1] / dv;

			if (s < skyMins[0][axis]) skyMins[0][axis] = s;
			if (t < skyMins[1][axis]) skyMins[1][axis] = t;
			if (s > skyMaxs[0][axis]) skyMaxs[0][axis] = s;
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
	if (newc[0])
		ClipSkyPolygon (newc[0], newv[0][0], stage + 1);
	if (newc[1])
		ClipSkyPolygon (newc[1], newv[1][0], stage + 1);
}


static void ClearSkyBox (void)
{
	int		i;

	// clear bounds for all sky box planes
	for (i = 0; i < 6; i++)
	{
		skyMins[0][i] = 9999;
		skyMins[1][i] = 9999;
		skyMaxs[0][i] = -9999;
		skyMaxs[1][i] = -9999;
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

	scale = vp.zFar * 4/7;			//?? 4/7 is almost 1/sqrt(3)
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

		// clamp new coords to [sky_min, sky_max]
		if (s < sky_min)
			s = sky_min;
		else if (s > sky_max)
			s = sky_max;
		if (t < sky_min)
			t = sky_min;
		else if (t > sky_max)
			t = sky_max;

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

	sky_min = 0;
	sky_max = 1;

	qglDisableClientState (GL_COLOR_ARRAY);
	if (currentShader != gl_defaultSkyShader)
		qglColor3f (gl_config.identityLightValue_f,
					gl_config.identityLightValue_f,
					gl_config.identityLightValue_f); // avoid overbright
	else
		qglColor3f (0, 0, 0);	// bad sky -- make it black (almost as gl_fastsky)

	qglPushMatrix ();
	if (gl_showsky->integer)
		GL_State (GLSTATE_DEPTHWRITE/*|GLSTATE_NODEPTHTEST*/);
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

	for (i = 0; i < 6; i++)
	{
		float	s, t;
		int		ds, dt, si, ti;
		bufVertex_t		*vec;
		bufTexCoord_t	*tex;
		int				*idx, i1, i2;

		if (skyMins[0][i] > skyMaxs[0][i] ||
			skyMins[1][i] > skyMaxs[1][i])
			continue;		// no sky surfaces on this plane

		// tesselate sky surface (up to 9 points per diminsion)
		vec = &vb->verts[0];
		tex = &vb->texCoord0[0];
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

		qglTexCoordPointer (2, GL_FLOAT, 0, vb->texCoord0);
		qglVertexPointer (3, GL_FLOAT, sizeof(bufVertex_t), vb->verts);

		if (qglLockArraysEXT)
			qglLockArraysEXT (0, numVerts);

		DrawArrays (indexesArray, numIndexes);

		if (qglUnlockArraysEXT)
			qglUnlockArraysEXT ();
	}
	qglPopMatrix ();

	numVerts = numIndexes = 0;

	qglDepthRange (0, 1);		// restore depth range (to default values)
}


/*----------------- Scene ------------------*/


static void CheckDynamicLightmap (surfaceCommon_t *s)
{
	int		i;
	surfacePlanar_t *surf;
	dynamicLightmap_t *dl;
	lightstyle_t *ls;

	// update dynamic lightmap (or vertex colors)
	surf = s->pl;
	if (!surf->lightmap) return;

	dl = surf->lightmap;
	for (i = 0; i < dl->numStyles; i++)
	{
		ls = &ap.lightStyles[dl->style[i]];
#define CHECK(n)	(dl->modulate[i][n] != ls->rgb[n])
		if (CHECK(0) || CHECK(1) || CHECK(2))	//?? make it faster (cache results in bool[256] ?)
		{
//			Com_Printf("dl%d: %g, %g, %g\n",dl->style[i], ls->rgb[0], ls->rgb[1], ls->rgb[2]);
			i = -1;
			break;
		}
#undef CHECK
	}

	if (i < 0)
	{	// require to update lightmap
		for (i = 0; i < dl->numStyles; i++)
		{
			ls = &ap.lightStyles[dl->style[i]];
			dl->modulate[i][0] = ls->rgb[0];
			dl->modulate[i][1] = ls->rgb[1];
			dl->modulate[i][2] = ls->rgb[2];
		}
		GL_UpdateDynamicLightmap (s->shader, surf, ap.lightStyles);
	}
}


static void TesselatePlanarSurf (surfacePlanar_t *surf)
{
	bufVertex_t	*v, *n;
	bufTexCoordSrc_t *t;
	int			*idx, *idxSrc, i, firstVert, firstIndex;
	int			*c;
	vertex_t	*vs;

	RB_CheckOverflow (surf->numVerts, surf->numIndexes);

	firstVert = numVerts;
	numVerts += surf->numVerts;
	firstIndex = numIndexes;
	numIndexes += surf->numIndexes;

	// copy normal
	n = &normals[firstVert];
	for (i = 0; i < surf->numVerts; i++, n++)	// sometimes may be ignored??
		VectorCopy (surf->plane.normal, n->xyz);

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
		*c = vs->rgba;				// copy vertex color (sometimes may be ignored??)
	}

	// copy indexes
	idx = &indexesArray[firstIndex];
	idxSrc = surf->indexes;
	for (i = 0; i < surf->numIndexes; i++)
		*idx++ = *idxSrc++ + firstVert;
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
	RB_CheckOverflow (surf->numVerts, numIdx);

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


// Setup OpenGL as specified in active portal
static void SetupGL (void)
{
	int		bits;

	if (numVerts)
		RB_EndSurface ();

	LOG_STRING ("*** SetupGL() ***\n");
	gl_state.is2dMode = false;
	qglMatrixMode (GL_PROJECTION);
	qglLoadMatrixf (&ap.projectionMatrix[0][0]);
	qglMatrixMode (GL_MODELVIEW);
	qglLoadMatrixf (&ap.modelMatrix[0][0]);	// required for sky drawing

	qglViewport (ap.x, ap.y, ap.w, ap.h);
	qglScissor (ap.x, ap.y, ap.w, ap.h);
	GL_State (GLSTATE_DEPTHWRITE);

	//?? perform setup for stencil shadows

	bits = GL_DEPTH_BUFFER_BIT;
	if (gl_fastsky->integer && !(gl_refdef.flags & RDF_NOWORLDMODEL))
	{
		bits |= GL_COLOR_BUFFER_BIT;
		qglClearColor (0, 0, 0, 1);
	}
	else if (gl_refdef.flags & RDF_NOWORLDMODEL)
	{
		bits |= GL_COLOR_BUFFER_BIT;
		qglClearColor (0, 0.03, 0.03, 1);
    }
	qglClear (bits);

	//?? can process other RDF_... flags

	//?? can set qglClipPlane() for portals
}


/*-------- radix sort surfaces ----------*/

#define SORT_BITS	8	// 11
#define SORT_SIZE	(1<<SORT_BITS)
#define SORT_MASK	((1<<SORT_BITS)-1)

static void SortSurfaces (void)
{
	int		i, k;
	surfaceInfo_t *s, **src1, **dst1;
	surfaceInfo_t **src, **dst;
	int		alpha1, alpha2;		// indexes of "*alpha1" and "*alpha2" shaders

	surfaceInfo_t	*sortFirst[SORT_SIZE], *sortFirst2[SORT_SIZE],
					// we will use sortFirst and sortFirst2 by turns
					*sortLast[SORT_SIZE];
					// save pointer to a last chain element (fast insertion to the end)

	if (!gl_sortAlpha->integer)
	{
		alpha1 = gl_alphaShader1->sortIndex << SHADERNUM_SHIFT;
		alpha2 = gl_alphaShader2->sortIndex << SHADERNUM_SHIFT;
	}
	else
	{	// debug mode
		alpha1 = 1 << 14;
		alpha2 = 0;
	}

#define GET_SORT(value,result)	\
	{							\
		int		tmp;			\
		tmp = value & (SHADERNUM_MASK << SHADERNUM_SHIFT);	\
		if (tmp > alpha1 && tmp < alpha2)	\
			result = value & ~(SHADERNUM_MASK << SHADERNUM_SHIFT) | alpha2;	\
		else					\
			result = tmp;		\
	}

	/* Sort by least significant SORT_BITS. Get source directly from
	 * surfaces array and put it to sortFirst
	 */
	memset (sortFirst, 0, sizeof(sortFirst));
	for (i = 0, s = ap.surfaces; i < ap.numSurfaces; i++, s++)
	{
		int		b;

		GET_SORT(s->sort,b);
		b &= SORT_MASK;
		if (!sortFirst[b])
			sortFirst[b] = s;			// insert it first
		else
			sortLast[b]->sortNext = s;	// insert to the chain end
		s->sortNext = NULL;
		sortLast[b] = s;
	}

	src1 = &sortFirst[0];
	dst1 = &sortFirst2[0];
	/* Sort other bits. Use sortFirst, sortFirst2, sortFirst ... as source.
	 * This is a modified variant of the previous loop.
	 */
	for (k = SORT_BITS; k < 32; k += SORT_BITS)	// shift index
	{
		src = src1;
		dst = dst1;
		src1 = dst1; dst1 = src;			// swap src1 and dst1 (for next loop)

		memset (dst, 0, sizeof(sortFirst));	// clear dst heads
		for (i = 0; i < SORT_SIZE; i++)
		{
			s = *src++;		// next chain
			while (s)
			{
				int		b;
				surfaceInfo_t *next;

				next = s->sortNext;				// s->sortNext will be set to NULL below
				GET_SORT(s->sort,b);
				b = (b >> k) & SORT_MASK;
				if (!dst[b])
					dst[b] = s;					// 1st element in chain
				else
					sortLast[b]->sortNext = s;	// add to the chain end
				s->sortNext = NULL;				// no next element
				sortLast[b] = s;

				s = next;
			}
		}
	}
	/*------ fill sortedSurfaces array --------*/
	src = src1;
	dst = &sortedSurfaces[0];
	for (i = 0; i < SORT_SIZE; i++)
	{
		for (s = *src1++; s; s = s->sortNext)
			*dst++ = s;
	}
#undef GET_SORT
}


/*-------------- Debug output ----------------*/


static void FlashColor (void)
{
	int		i;

	i = (int)(ap.time / 3 * 1024);
	qglColor3f (sinTable[i & 1023] / 2 + 0.5, sinTable[i + 100 & 1023] / 2 + 0.5, sinTable[600 - i & 1023] / 2 + 0.5);
}


#define BBOX_WORLD	1	// debug?
// draw entity bounding boxes
static void DrawBBoxes (void)
{
	int		i;
	byte	entsVisible[MAX_GLENTITIES];
	surfaceInfo_t **si;

	// common GL setup
#ifdef BBOX_WORLD
	qglLoadMatrixf (&ap.modelMatrix[0][0]);
#endif
	GL_Bind (NULL);		// disable texturing
	qglDisableClientState (GL_COLOR_ARRAY);
	qglDisableClientState (GL_TEXTURE_COORD_ARRAY);
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
	for (i = 0; i < gl_numEntities; i++)
	{
		refEntity_t *ent;
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

		ent = &gl_entities[i];
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
			DrawTextLeft (va("Unknown modeltype for entity %d", i), 1, 0, 0);
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

	GL_Bind (NULL);
	if (gl_showtris->integer - 1 & 1)
	{	// use depth test
		GL_State (0);
		qglEnable (GL_POLYGON_OFFSET_FILL);
		qglPolygonOffset (-3, -2);
	}
	else	// no depth test
		GL_State (GLSTATE_NODEPTHTEST);

	if (gl_showtris->integer - 1 & 4)
		FlashColor ();
	else
		qglColor3f (0, 0, 0);
	qglDisableClientState (GL_COLOR_ARRAY);
	qglDisableClientState (GL_TEXTURE_COORD_ARRAY);

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
		switch (surf->type)
		{
		case SURFACE_PLANAR:
			pl = surf->pl;
			qglVertexPointer (3, GL_FLOAT, sizeof(vertex_t), pl->verts);
			if (qglLockArraysEXT)
				qglLockArraysEXT (0, pl->numVerts);

			if (!(gl_showtris->integer - 1 & 2))
			{
				// show triangles
				for (index = 0; index < pl->numIndexes; index += 3)
					qglDrawElements (GL_LINE_LOOP, 3, GL_UNSIGNED_INT, &pl->indexes[index]);
			}
			else	// show edges
				qglDrawElements (GL_LINE_LOOP, pl->numVerts, GL_UNSIGNED_INT, inds);

			if (qglUnlockArraysEXT)
				qglUnlockArraysEXT ();
			break;
		}
	}

	if (gl_showtris->integer - 1 & 1)
		qglDisable (GL_POLYGON_OFFSET_FILL);
}


/*---------- Fast drawing (without filling arrays) ----------*/
//?? fast drawing may use global vertex/color/texcoord array (filled when map loaded) and just operate with indexes
//!! In this case, indexes in surfaces must be global

static void DrawFastSurfaces (surfaceInfo_t **surfs, int numSurfs)
{
	int		i;
	shaderStage_t *stage;
//	static int inds[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19};

/* Feauture version (??):
	// set global vertex array:
	VertexPointer(globalVerts.xyz);
	LockArrays();
	loop(stage) {
		TexCoordPointer(globalVerts.st) or (.lm)  (depends on)
		if (rgba.type == CONST)
		{
			DisableClientState(COLOR_ARRAYS);
			Color(const);
		}
		else
		{
			EnableClientState(COLOR_ARRAYS);
			ColorPointer()
		}
		GL_State(stage.state);
		loop(surf) {
			DrawArrays(surf->indexes, surf->numIndexes);
		}
	}
	UnlockArrays();

 * + multitexture version!
 */

	LOG_STRING (va("*** FastIterator(%s, %d) ***\n", currentShader->name, numSurfs));
	for (i = 0, stage = currentShader->stages; i < currentShader->numStages; i++, stage++)
	{
		surfaceInfo_t **psurf;
		int		j;

		GL_State (stage->glState);
		GL_CullFace (currentShader->cullMode);
		// setup RGBA (if const)
		if (stage->rgbGenType == RGBGEN_CONST)
		{	// here: alphaGenFunc == ALPHAGEN_CONST (condition for shader.fast flag)
			qglDisableClientState (GL_COLOR_ARRAY);
			qglColor4f (stage->rgbGenConst[0], stage->rgbGenConst[1], stage->rgbGenConst[2], stage->alphaGenConst);
		}
		else
			qglEnableClientState (GL_COLOR_ARRAY);
		// bind texture(s)
		BindStageImage (&stage->tmu[0]);	//!! add multitetxuring

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
				if (stage->tmu[0].tcGenType == TCGEN_TEXTURE)	//!! add multitexturing
					qglTexCoordPointer (2, GL_FLOAT, sizeof(vertex_t), pl->verts[0].st);
				else
					qglTexCoordPointer (2, GL_FLOAT, sizeof(vertex_t), pl->verts[0].lm);
				if (stage->rgbGenType == RGBGEN_EXACT_VERTEX)
					qglColorPointer (4, GL_UNSIGNED_BYTE, sizeof(vertex_t), pl->verts[0].c);
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

// Macro for determining fast-draw ability
#define IS_FAST(shader,surf)	(shader->fast && surf->type == SURFACE_PLANAR)


// Draw scene, specified in "ap"
static void RB_DrawScene (void)
{
	int				index, index2, numSkySurfs, numFastSurfs;
	surfaceInfo_t	**si, **si2, **fastSurf;
	shader_t		*shader;
	surfaceCommon_t	*surf;
	// current state
	int		currentShaderNum, currentEntityNum;
	qboolean currentDepthHack, depthHack;

	LOG_STRING (va("******** DrawScene: (%g, %g) - (%g, %g) ********\n", ap.x, ap.y, ap.x+ap.w, ap.y+ap.h));

	SetupGL ();

	// sort surfaces
	if (gl_finish->integer == 2) qglFinish ();
	gl_speeds.beginSort = Sys_Milliseconds ();
	SortSurfaces ();
	gl_speeds.begin3D = Sys_Milliseconds ();

	/*------------ draw sky --------------*/
	ClearSkyBox ();
	numSkySurfs = 0;

	si = sortedSurfaces;
	SetCurrentShader ((*si)->surf->shader);
	for (index = 0; index < ap.numSurfaces; index++, si++)
	{
		surf = (*si)->surf;
		shader = surf->shader;
		if (shader->type != SHADERTYPE_SKY)
			break;

		TesselatePlanarSurf (surf->pl);	//!! may be other types
		AddSkySurfaces ();
		numSkySurfs++;
		numVerts = numIndexes = 0;
	}
	if (numSkySurfs)
		DrawSkyBox ();

	// update r_speeds values
	gl_speeds.surfs += ap.numSurfaces - index;	// numSurfaces - numSkySurfaces

	/*--- update all dynamic lightmaps ---*/
	for (index2 = index, si2 = si; index2 < ap.numSurfaces; index2++, si2++)
	{
		surf = (*si2)->surf;
		if (surf->type == SURFACE_PLANAR)
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

		surf = (*si)->surf;
		code = (*si)->sort;
		shNum = (code >> SHADERNUM_SHIFT) & SHADERNUM_MASK;
		entNum = (code >> ENTITYNUM_SHIFT) & ENTITYNUM_MASK;

		if (shNum != currentShaderNum || entNum != currentEntityNum)
		{
			// flush data for the previous shader
			if (numFastSurfs)
			{
				DrawFastSurfaces (fastSurf, numFastSurfs);
				numFastSurfs = 0;
			}
			else
				RB_EndSurface ();

			// change shader
			shader = GL_GetShaderByNum (shNum);
			SetCurrentShader (shader);
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

		if (IS_FAST(currentShader, surf))
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
			//!! other types
			}
		}
	}
	/*--------- finilize/debug -----------*/
	if (numFastSurfs)
		DrawFastSurfaces (fastSurf, numFastSurfs);
	else
		RB_EndSurface ();

	if (currentDepthHack)
		qglDepthRange (0, 1);

	if (gl_showtris->integer)
		DrawTriangles ();

	if (gl_showbboxes->integer)
		DrawBBoxes ();

	if (gl_finish->integer == 2) qglFinish ();
	gl_speeds.begin2D = Sys_Milliseconds ();
}


void GL_ClearBuffers (void)
{
	numSurfacesTotal = 0;
	gl_numEntities = 0;
	//?? clear dlights etc.
}


void GL_ClearPortal (void)
{
	vp.surfaces = &surfaceBuffer[numSurfacesTotal];
	vp.numSurfaces = 0;
}


// Add surface to a current scene (to a "vp" structure)
void GL_AddSurfaceToPortal (surfaceCommon_t *surf, shader_t *shader, int entityNum)
{
	surfaceInfo_t *si;

	if (numSurfacesTotal >= MAX_SCENE_SURFACES - 1) return;	// buffer is full
	si = vp.surfaces + vp.numSurfaces++;
	si->sort = (shader->sortIndex << SHADERNUM_SHIFT) | (entityNum << ENTITYNUM_SHIFT);
	si->surf = surf;
	numSurfacesTotal++;
	// update maxUsedShaderIndex
	if (shader->sortIndex > gl_state.maxUsedShaderIndex)
		gl_state.maxUsedShaderIndex = shader->sortIndex;
}


void GL_FinishPortal (void)
{
	if (!vp.numSurfaces) return;
	{
		PUT_BACKEND_COMMAND (bkDrawFrame_t, c);
		c->type = BACKEND_DRAW_FRAME;
		memcpy (&c->portal, &vp, sizeof(vp));
	}
}


static void TesselateStretchPic (bkDrawPic_t *p)
{
	bufVertex_t	*v;
	bufTexCoordSrc_t *t;
	int			*idx, i;
	int			*c;

	if (currentShader != p->shader)
	{
		RB_EndSurface ();
		SetCurrentShader (p->shader);
	}

	GL_Set2DMode ();

	RB_CheckOverflow (4, 6);

	/*   0 (0,0) -- 1 (1,0)
	 *     |   \____   |
	 *     |        \  |
	 *   3 (0,1) -- 2 (1,1)
	 */
	v = &vb->verts[numVerts];
	t = &srcTexCoord[numVerts];
	c = &srcVertexColor[numVerts].rgba;
	i = numVerts;
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
	c[0] = c[1] = c[2] = c[3] = currentColor.rgba;

	idx = &indexesArray[numIndexes];
	numIndexes += 6;
	*idx++ = i+0; *idx++ = i+1; *idx++ = i+2;
	*idx++ = i+0; *idx++ = i+2; *idx++ = i+3;
}


void GL_BackEnd (void)
{
	int		*p;

	* ((int*) &backendCommands[backendCmdSize]) = BACKEND_STOP;
	backendCmdSize = 0;

	if (gl_state.maxUsedShaderIndex >= gl_state.minNewShaderIndex)
	{	// scene uses shader, which index was changed with ResortShader() -- drop frame
		Com_DPrintf ("GL_BackEnd: drop frame (%d >= %d)\n", gl_state.maxUsedShaderIndex, gl_state.minNewShaderIndex);
		return;
	}

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

		case BACKEND_SET_COLOR:
			{
				bkSetColor_t *p1;

				p1 = (bkSetColor_t *) p;
				currentColor.rgba = p1->rgba;
				p = (int *) ++p1;
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
			RB_EndSurface ();
			if (gl_showImages->integer)
				GL_ShowImages ();

#ifndef SWAP_ON_BEGIN
			if (!gl_state.finished && gl_finish->integer)	//?? if ".finished" needed?
			{
				qglFinish ();
				gl_state.finished = true;
			}
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
				RB_DrawScene ();
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

	if (qglVertexArrayRangeNV)
	{
		agpBuffer = qglAllocateMemoryNV (NUM_VERTEX_BUFFERS * sizeof(vertexBuffer_t), 0, 0, 0.5);
		if (!agpBuffer)
			agpBuffer = qglAllocateMemoryNV (NUM_VERTEX_BUFFERS * sizeof(vertexBuffer_t), 0, 0, 1);

		if (!agpBuffer)
		{
			Com_Printf ("^1Cannot allocate video memory for vertex buffers. Disabling GL_NV_vertex_array_range.\n");
			qglVertexArrayRangeNV = NULL;
		}
		else
		{
			qglGenFencesNV (NUM_VERTEX_BUFFERS, fences);
			qglVertexArrayRangeNV (NUM_VERTEX_BUFFERS * sizeof(vertexBuffer_t), agpBuffer);
			qglEnableClientState (GL_VERTEX_ARRAY_RANGE_NV);
			vb = agpBuffer;
			currentBuffer = 0;
		}
	}

	if (!agpBuffer)
		vb = Z_Malloc (sizeof (vertexBuffer_t));

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
	else
		if (vb)
		{
			Z_Free (vb);
			vb = NULL;
		}
}
