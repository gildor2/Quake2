#include "gl_local.h"
#include "gl_model.h"
#include "gl_math.h"
#include "gl_poly.h"
#include "gl_lightmap.h"
#include "gl_shader.h"

//#define TEST_LOAD	// will add command "loadmodel <filename>"; may implement this in client (not renderer)

//#define PROFILE_LOADING

#ifdef PROFILE_LOADING
#define START_PROFILE(name)			\
	{								\
		static const char _name[] = #name;\
		const char *_arg = "";		\
		unsigned _time = appCycles();
#define START_PROFILE2(name,arg)	\
	{								\
		static const char _name[] = #name;\
		const char *_arg = arg;		\
		unsigned _time = appCycles();
#define END_PROFILE	\
		if (Cvar_VariableInt("r_profile")) \
			Com_Printf(S_MAGENTA"%s "S_GREEN"%s"S_CYAN": %.2f ms\n", _name, _arg,\
			appDeltaCyclesToMsecf(appCycles() - _time)); \
	}
#else
#define START_PROFILE(name)
#define START_PROFILE2(name,arg)
#define END_PROFILE
#endif


namespace OpenGLDrv {


bspModel_t map;
static bspfile_t *bspfile;

#define MAX_GLMODELS	1024

static model_t *modelsArray[MAX_GLMODELS];
static int	modelCount;


/*-----------------------------------------------------------------------------
	Loading models: common code
-----------------------------------------------------------------------------*/

// forwards
static md3Model_t *LoadMd2 (const char *name, byte *buf, unsigned len);
static md3Model_t *LoadMd3 (const char *name, byte *buf, unsigned len);
static sp2Model_t *LoadSp2 (const char *name, byte *buf, unsigned len);
static void FreeModels ();


model_t	*FindModel (const char *name)
{
	char	name2[MAX_QPATH];
	appCopyFilename (name2, name, sizeof(name2));

#if 0
	// try to load md3 instead of md2
	//!! disable later
	char *ext = strrchr (name2, '.');
	if (ext && !strcmp (ext, ".md2"))
	{
		ext[3] = '3';		// ".md2" -> ".md3"
		if (FS_FileExists (name2))
			return FindModel (name2);
		// md3 model with the same name is not found -- load md2
		ext[3] = '2';		// ".md3" -> ".md2"
	}
#endif

	model_t *m;
	// search already loaded models
	for (int i = 0; i < modelCount; i++)
	{
		m = modelsArray[i];
		if (!m) continue;				//?? should not happens
		if (!strcmp (name2, m->name)) return m;
	}

	if (modelCount == MAX_GLMODELS)
	{
		Com_WPrintf ("R_FindModel: MAX_GLMODELS\n");
		return NULL;
	}

START_PROFILE2(FindModel::Load, name)
	/*----- not found -- load model ------*/
	unsigned len;
	unsigned *file;
	if (!(file = (unsigned*) FS_LoadFile (name2, &len)))
	{
		m = new model_t;
		strcpy (m->name, name2);
		modelsArray[modelCount++] = m;
		Com_DPrintf ("R_FindModel: %s not found\n", name2);
		return NULL;	// file not found
	}
END_PROFILE
START_PROFILE(FindModel::Process)

	switch (LittleLong(*file))
	{
	case MD2_IDENT:
		m = LoadMd2 (name2, (byte*)file, len);
		break;
	case MD3_IDENT:
		m = LoadMd3 (name2, (byte*)file, len);
		break;
	case SP2_IDENT:
		m = LoadSp2 (name2, (byte*)file, len);
		break;
	default:
		// no error here: simply ignore unknown model formats
		Com_WPrintf ("R_FindModel: unknown ID 0x%X in %s", LittleLong (*file), name);
		m = NULL;
	}
	if (m) modelsArray[modelCount++] = m;

END_PROFILE
	FS_FreeFile (file);
	return m;
}


/*-----------------------------------------------------------------------------
	Common BSP loading code
-----------------------------------------------------------------------------*/

// "stride" = sizeof(dplane_t) or sizeof(dplane3_t)
// Both this structures has same fields, except Q3 lost "type"
static void LoadPlanes (dplane_t *data, int count, int stride)
{
	dplane_t *in;
	cplane_t *out;

	map.numPlanes = count;
	map.planes = out = new (map.dataChain) cplane_t [count];
	in = data;

	for (int i = 0; i < count; i++, in = (dplane_t*)((byte*)in + stride), out++)
	{
		VectorCopy (in->normal, out->normal);
		out->dist = in->dist;
		out->type = PlaneTypeForNormal (out->normal);
		SetPlaneSignbits (out);
	}
}


static void LoadFlares (lightFlare_t *data, int count)
{
	gl_flare_t *out;

	// get leafs/owner for already built flares (from SURF_AUTOFLARE)
	for (out = map.flares; out; out = out->next)
	{
		out->leaf = PointInLeaf (out->origin);
		if (out->surf) out->owner = out->surf->owner;
	}

	map.numFlares += count;
	out = new (map.dataChain) gl_flare_t [count];
	for (int i = 0; i < count; i++, data = data->next, out++)
	{
		VectorCopy (data->origin, out->origin);
		out->size = data->size;
		out->radius = data->radius;
		out->color.rgba = *((unsigned*)data->color) | RGBA(0,0,0,1);
		SaturateColor4b (&out->color);
		out->style = data->style;
		out->leaf = PointInLeaf (data->origin);
		if (data->model)
			out->owner = map.models + data->model;

		out->next = map.flares;
		map.flares = out;
	}
}


//??#define MAX_ASPECT	(5.0f / 8)

static void BuildSurfFlare (surfaceBase_t *surf, color_t *color, float intens)
{
	vec3_t	origin, c;

	if (surf->type != SURFACE_PLANAR) return;
	surfacePlanar_t *pl = static_cast<surfacePlanar_t*>(surf);

//	float aspect = (pl->maxs2[0] - pl->mins2[0]) / (pl->maxs2[1] - pl->mins2[1]);
//	if (aspect < MAX_ASPECT || aspect > 1/MAX_ASPECT) return;	// not square

	float x = (pl->maxs2[0] + pl->mins2[0]) / 2;
	float y = (pl->maxs2[1] + pl->mins2[1]) / 2;
	VectorScale (pl->axis[0], x, origin);
	VectorMA (origin, y, pl->axis[1], origin);
	VectorMA (origin, pl->plane.dist + 1, pl->plane.normal, origin);

	gl_flare_t *f = new (map.dataChain) gl_flare_t;

	VectorCopy (origin, f->origin);
	f->surf = surf;
//	f->owner = surf->owner;				// cannot do it: models not yet loaded
	f->leaf = NULL;
	f->size = sqrt (intens) * 3;
	f->radius = 0;
	f->style = 0;

	c[0] = color->c[0];
	c[1] = color->c[1];
	c[2] = color->c[2];
	NormalizeColor255 (c, c);
	f->color.c[0] = appRound (c[0]);
	f->color.c[1] = appRound (c[1]);
	f->color.c[2] = appRound (c[2]);
	f->color.c[3] = 255;				// A
	SaturateColor4b (&f->color);

	f->next = map.flares;
	map.flares = f;
	map.numFlares++;
}


static void LoadSlights (slight_t *data, int count)
{
	gl_slight_t *out;

	map.numSlights = count;
	map.slights = out = new (map.dataChain) gl_slight_t [count];
	// copy slights from map
	for (int i = 0; i < count; i++, data = data->next)
	{
		VectorCopy (data->origin, out->origin);
		VectorCopy (data->color, out->color);
		out->intens = data->intens;
		out->style = data->style;
		out->type = data->type;
		out->spot = data->spot != 0;
		VectorCopy (data->spotDir, out->spotDir);
		out->spotDot = data->spotDot;
		out->focus = data->focus;
		out->fade = data->fade;

		SaturateColor3f (out->color);

		// move away lights from nearby surfaces to avoid precision errors during computation
		for (int j = 0; j < 3; j++)
		{
			static const vec3_t mins = {-0.3, -0.3, -0.3}, maxs = {0.3, 0.3, 0.3};
			trace_t	tr;

			CM_BoxTrace (&tr, out->origin, out->origin, mins, maxs, 0, CONTENTS_SOLID);
			if (tr.allsolid)
				VectorMA (out->origin, 0.5f, tr.plane.normal, out->origin);
			else
				break;
		}

		out++;
	}
}


static void BuildSurfLight (surfacePlanar_t *pl, color_t *color, float area, float intens, bool sky)
{
	vec3_t	c;
	c[0] = color->c[0] / 255.0f;
	c[1] = color->c[1] / 255.0f;
	c[2] = color->c[2] / 255.0f;

	if (bspfile->sunLight && sky)
	{	// have sun -- params may be changed
		float	m;

		m = max(bspfile->sunSurface[0], bspfile->sunSurface[1]);
		m = max(m, bspfile->sunSurface[2]);
		if (m == 0 && !map.haveSunAmbient)
			return;									// no light from sky surfaces

		if (m > 0 && m <= 1)
			VectorCopy (bspfile->sunSurface, c);	// sun_surface specified a color
		else if (m > 1)
		{
			// sun_surface specified an intensity, color -- from surface
			c[0] *= bspfile->sunSurface[0];
			c[1] *= bspfile->sunSurface[1];
			c[2] *= bspfile->sunSurface[2];
			intens = 1;
		}
		else if (m == 0)
			intens = 0;								// this surface will produce ambient light only
	}
	intens *= NormalizeColor (c, c);

	surfLight_t *sl = new (map.dataChain) surfLight_t;
	sl->next = map.surfLights;
	map.surfLights = sl;
	map.numSurfLights++;
	VectorCopy (c, sl->color);
	sl->sky = sky;
	sl->intens = intens * area;
	sl->pl = pl;

	// qrad3 does this ...
	sl->color[0] *= sl->color[0];
	sl->color[1] *= sl->color[1];
	sl->color[2] *= sl->color[2];

	SaturateColor3f (sl->color);

	pl->light = sl;
}


static void SetNodeParent (node_t *node, node_t *parent)
{
	node->parent = parent;

	if (node->isNode)
	{
		SetNodeParent (node->children[0], node);
		SetNodeParent (node->children[1], node);
	}
}


static void SetNodesAlpha (void)
{
	node_t	*node, *p;
	surfaceBase_t **psurf;
	int		i, j;

	// enumerate leafs
	for (node = map.nodes + map.numNodes, i = 0; i < map.numLeafNodes - map.numNodes; node++, i++)
		// check for alpha surfaces
		for (psurf = node->leafFaces, j = 0; j < node->numLeafFaces; psurf++, j++)
			if ((*psurf)->shader->style & (SHADER_ALPHA|SHADER_TRANS33|SHADER_TRANS66))
			{
				// set "haveAlpha" for leaf and all parent nodes
				for (p = node; p; p = p->parent) p->haveAlpha = true;
				break;
			}
}


static void BuildPlanarSurfAxis (surfacePlanar_t *pl)
{
	// generate axis
	if (pl->plane.normal[2] == 1 || pl->plane.normal[2] == -1)
	{
		pl->axis[0][0] = pl->axis[0][2] = 0;
		pl->axis[1][1] = pl->axis[1][2] = 0;
		pl->axis[0][1] = pl->axis[1][0] = 1;
	}
	else
	{
		static const vec3_t up = {0, 0, 1};

		CrossProduct (pl->plane.normal, up, pl->axis[0]);
		VectorNormalize (pl->axis[0]);
		CrossProduct (pl->plane.normal, pl->axis[0], pl->axis[1]);
	}
	// compute 2D bounds
	float min1, max1, min2, max2;
	min1 = min2 = BIG_NUMBER;
	max1 = max2 = -BIG_NUMBER;
	int	i;
	vertex_t *v;
	for (i = 0, v = pl->verts; i < pl->numVerts; i++, v++)
	{
		float	p1, p2;

		p1 = DotProduct (v->xyz, pl->axis[0]);
		p2 = DotProduct (v->xyz, pl->axis[1]);
		v->pos[0] = p1;
		v->pos[1] = p2;
		EXPAND_BOUNDS(p1, min1, max1);
		EXPAND_BOUNDS(p2, min2, max2);
	}
	pl->mins2[0] = min1;
	pl->mins2[1] = min2;
	pl->maxs2[0] = max1;
	pl->maxs2[1] = max2;
}


/*-----------------------------------------------------------------------------
	Loading Quake2 BSP file
-----------------------------------------------------------------------------*/

static void LoadLeafsNodes2 (dnode_t *nodes, int numNodes, dleaf_t *leafs, int numLeafs)
{
	node_t	*out;
	int		i, j;

	map.numNodes = numNodes;
	map.numLeafNodes = numLeafs + numNodes;
	map.nodes = out = new (map.dataChain) node_t [numNodes + numLeafs];

	// Load nodes
	for (i = 0; i < numNodes; i++, nodes++, out++)
	{
		out->isNode = true;
		out->plane = map.planes + nodes->planenum;
		// setup children[]
		for (j = 0; j < 2; j++)
		{
			int p = nodes->children[j];
			if (p >= 0)
				out->children[j] = map.nodes + p;
			else
				out->children[j] = map.nodes + map.numNodes + (-1 - p);
		}
		// copy/convert mins/maxs
		for (j = 0; j < 3; j++)
		{
			out->mins[j] = nodes->mins[j];
			out->maxs[j] = nodes->maxs[j];
		}
	}

	// Load leafs
	for (i = 0; i < numLeafs; i++, leafs++, out++)
	{
		out->isNode = false;

		int p = leafs->cluster;
		out->cluster = p;
		if (p >= map.numClusters)
			map.numClusters = p + 1;
		out->area = leafs->area;
		// copy/convert mins/maxs
		for (j = 0; j < 3; j++)
		{
			out->mins[j] = leafs->mins[j];
			out->maxs[j] = leafs->maxs[j];
		}
		// setup leafFaces
		out->leafFaces = map.leafFaces + leafs->firstleafface;
		out->numLeafFaces = leafs->numleaffaces;
	}

	// Setup node/leaf parents
	SetNodeParent (map.nodes, NULL);
	SetNodesAlpha ();
}


static void LoadInlineModels2 (cmodel_t *data, int count)
{
	inlineModel_t	*out;

	map.models = out = new (map.dataChain) inlineModel_t [count];
	map.numModels = count;

	for (int i = 0; i < count; i++, data++, out++)
	{
		CALL_CONSTRUCTOR(out);
		appSprintf (ARRAY_ARG(out->name), "*%d", i);
		out->size = -1;							// do not delete in FreeModels()
		modelsArray[modelCount++] = out;

		VectorCopy (data->mins, out->mins);
		VectorCopy (data->maxs, out->maxs);
		out->radius = data->radius;
		out->headnode = data->headnode;
		// create surface list
		out->numFaces = data->numfaces;
		out->faces = new (map.dataChain) surfaceBase_t* [out->numFaces];

		int k = data->firstface;
		for (int j = 0; j < out->numFaces; j++, k++)
		{
			surfaceBase_t *s = map.faces[k];
			out->faces[j] = s;
			s->owner = i == 0 ? NULL : out;		// model 0 is world
		}
	}
}


static void LoadSurfaces2 (dface_t *surfs, int numSurfaces, int *surfedges, dedge_t *edges,
	dvertex_t *verts, texinfo_t *tex, cmodel_t *models, int numModels)
{
	int		j;

	map.numFaces = numSurfaces;
	map.faces = new (map.dataChain) surfaceBase_t* [numSurfaces];
	for (int i = 0; i < numSurfaces; i++, surfs++)
	{
		vec3_t	*pverts[MAX_POLYVERTS];

		int numVerts = surfs->numedges;
		/*------- build vertex list -------*/
		int *pedge = surfedges + surfs->firstedge;
		for (j = 0; j < numVerts; j++, pedge++)
		{
			int idx = *pedge;
			if (idx > 0)
				pverts[j] = &(verts + (edges+idx)->v[0])->point;
			else
				pverts[j] = &(verts + (edges-idx)->v[1])->point;
		}

		/*---- Generate shader with name and SURF_XXX flags ----*/
		texinfo_t *stex = tex + surfs->texinfo;
		int sflags = SHADER_WALL;
		if (stex->flags & SURF_ALPHA)	sflags |= SHADER_ALPHA;
		if (stex->flags & SURF_TRANS33)	sflags |= SHADER_TRANS33;
		if (stex->flags & SURF_TRANS66)	sflags |= SHADER_TRANS66;
		if (stex->flags & SURF_SKY && gl_showSky->integer != 2) sflags |= SHADER_SKY;
		if (stex->flags & SURF_FLOWING)	sflags |= SHADER_SCROLL;
		if (stex->flags & SURF_WARP)	sflags |= SHADER_TURB;
		if (stex->flags & SURF_SPECULAR) sflags |= SHADER_ENVMAP;
		if (stex->flags & SURF_DIFFUSE) sflags |= SHADER_ENVMAP2;

		// find owner model
		cmodel_t *owner;
		for (j = 0, owner = models; j < numModels; j++, owner++)
			if (i >= owner->firstface && i < owner->firstface + owner->numfaces)
				break;

		if (owner->flags & CMODEL_ALPHA)
			sflags |= SHADER_ALPHA;

#if 0
		if (Cvar_VariableInt("dlmodels"))	//??
		{
				if (owner->flags & CMODEL_MOVABLE && !(sflags & (SHADER_TRANS33|SHADER_TRANS66|SHADER_ALPHA)))
					sflags = SHADER_SKIN;
		}
#endif

		// check covered contents and update shader flags
		if (stex->flags & (SURF_TRANS33|SURF_TRANS66) && !(stex->flags &
			(SURF_SPECULAR|SURF_DIFFUSE|SURF_WARP|SURF_FLOWING)))		// && gl_autoReflect ??
		{
			vec3_t	mid, p1, p2;
			trace_t	trace;

			int headnode = owner->headnode;
			// find middle point
			VectorClear (mid);
			for (j = 0; j < numVerts; j++)
				VectorAdd (mid, (*pverts[j]), mid);
			float scale = 1.0f / numVerts;
			VectorScale (mid, scale, mid);
			// get trace points
			float *norm = (map.planes + surfs->planenum)->normal;
			// vector with length 1 is not enough for non-axial surfaces
			VectorMA (mid, 2, norm, p1);
			VectorMA (mid, -2, norm, p2);
			// perform trace
			if (!surfs->side)
				CM_BoxTrace (&trace, p1, p2, NULL, NULL, headnode, MASK_SOLID);
			else
				CM_BoxTrace (&trace, p2, p1, NULL, NULL, headnode, MASK_SOLID);
			if (trace.fraction < 1 && !(trace.contents & CONTENTS_MIST))	//?? make MYST to be non-"alpha=f(angle)"-dependent
				sflags |= SHADER_ENVMAP;
		}


		// check for shader lightmap
		bool needLightmap = false;
		if (surfs->lightofs >= 0 && !(sflags & SHADER_SKIN))	//??
		{
			if (sflags & SHADER_SKY)
			{
				//!! change this code (or update comment above)
				if (map.sunColor[0] + map.sunColor[1] + map.sunColor[2] == 0)
				{
					image_t *img = FindImage (va("textures/%s", stex->texture), IMAGE_MIPMAP|IMAGE_PICMIP);	// find sky image only for taking its color
					if (img)
					{
						map.sunColor[0] = img->color.c[0];		// do not require to divide by 255: will be normalized anyway
						map.sunColor[1] = img->color.c[1];
						map.sunColor[2] = img->color.c[2];
						NormalizeColor (map.sunColor, map.sunColor);
					}
					else
						VectorSet (map.sunColor, 1, 1, 1);
				}
			}
			else
			{
				if (sflags & (SHADER_TRANS33|SHADER_TRANS66|SHADER_ALPHA|SHADER_TURB))
					sflags |= SHADER_TRYLIGHTMAP;
				needLightmap = true;
			}
		}

		/*---------- check for texture animation ----------*/
		char	textures[MAX_QPATH * MAX_STAGE_TEXTURES];
		texinfo_t *ptex = stex;
		char *pname = textures;
		int numTextures = 0;
		while (true)
		{
			pname += appSprintf (pname, MAX_QPATH, "textures/%s", ptex->texture) + 1;	// length(format)+1
			numTextures++;

			int n = ptex->nexttexinfo;
			if (n < 0) break;			// no animations
			ptex = tex + n;
			if (ptex == stex) break;	// loop
		}
		*pname = 0;						// make zero (NULL string) after ASCIIZ string

		if (numTextures > MAX_STAGE_TEXTURES)
			Com_WPrintf ("%s: animation chain is too long (%d)\n", stex->texture, numTextures);
		else if (numTextures > 1)
			sflags |= SHADER_ANIM;

		if (needLightmap)
			sflags |= SHADER_LIGHTMAP;

		shader_t *shader = (sflags & SHADER_SKY) ? gl_skyShader : FindShader (textures, sflags);
		//!! update sflags from this (created) shader -- it may be scripted (with different flags)

		if (sflags & (SHADER_TRANS33|SHADER_TRANS66|SHADER_ALPHA|SHADER_TURB|SHADER_SKY))
			numVerts = RemoveCollinearPoints (pverts, numVerts);

		/* numTriangles = numVerts - 2 (3 verts = 1 tri, 4 verts = 2 tri etc.)
		 * numIndexes = numTriangles * 3
		 * Indexes: (0 1 2) (0 2 3) (0 3 4) ... (here: 5 verts, 3 triangles, 9 indexes)
		 */
		int numTris;
		if (shader->tessSize)
		{
			numTris = SubdividePlane (pverts, numVerts, shader->tessSize);
			numVerts = subdivNumVerts;
		}
		else
			numTris = numVerts - 2;

		int numIndexes = numTris * 3;

		/*------- Prepare for vertex generation ----------------*/
		surfacePlanar_t *s = (surfacePlanar_t*) map.dataChain->Alloc (sizeof(surfacePlanar_t) + sizeof(vertex_t)*numVerts + sizeof(int)*numIndexes);
		CALL_CONSTRUCTOR(s);
		s->shader = shader;
		map.faces[i] = s;

		s->plane = map.planes[surfs->planenum];
		if (surfs->side)
		{
			// backface (needed for backface culling)
			VectorNegate (s->plane.normal, s->plane.normal);
			s->plane.dist = -s->plane.dist;
			//?? set signbits
			s->plane.type = PlaneTypeForNormal (s->plane.normal);
		}
		s->numVerts = numVerts;
		s->verts = (vertex_t *) (s+1);	//!!! allocate verts separately (for fast draw - in AGP memory)
		s->numIndexes = numIndexes;
		s->indexes = (int *) (s->verts+numVerts);

		/*-------------- Generate indexes ----------------------*/
		int *pindex = s->indexes;
		if (shader->tessSize)
		{
			GetSubdivideIndexes (pindex);
			FreeSubdividedPlane ();
		}
		else
		{
			for (j = 0; j < numTris; j++)
			{
				*pindex++ = 0;
				*pindex++ = j+1;
				*pindex++ = j+2;
			}
		}

		/*----------- Create surface vertexes ------------------*/
		vertex_t *v = s->verts;
		// ClearBounds2D (mins, maxs)
		float mins[2], maxs[2];				// surface extents
		mins[0] = mins[1] = BIG_NUMBER;
		maxs[0] = maxs[1] = -BIG_NUMBER;
		ClearBounds (s->mins, s->maxs);
		// Enumerate vertexes
		for (j = 0; j < numVerts; j++, pedge++, v++)
		{
			VectorCopy ((*pverts[j]), v->xyz);
			if (!(sflags & SHADER_SKY))
			{
				float v1 = DotProduct (v->xyz, stex->vecs[0]) + stex->vecs[0][3];
				float v2 = DotProduct (v->xyz, stex->vecs[1]) + stex->vecs[1][3];
				/*----------- Update bounds --------------*/
				EXPAND_BOUNDS(v1, mins[0], maxs[0]);
				EXPAND_BOUNDS(v2, mins[1], maxs[1]);
				AddPointToBounds (v->xyz, s->mins, s->maxs);
				/*-------- Texture coordinates -----------*/
				if (!(sflags & SHADER_TURB)) //?? (!shader->tessSize)
				{
					v->st[0] = v1 / shader->width;
					v->st[1] = v2 / shader->height;
				}
				else
				{
					v->st[0] = v1 - stex->vecs[0][3];
					v->st[1] = v2 - stex->vecs[1][3];
				}
				// save intermediate data for lightmap texcoords
				v->lm[0] = v1;
				v->lm[1] = v2;
				/*---------- Vertex color ----------------*/
				v->c.rgba = RGBA(1,1,1,1);
			}
		}

		/*----------------- Lightmap -------------------*/
		if (needLightmap)
		{
			int		size[2];		// lightmap size
			int		imins[2], imaxs[2];		// int mins/maxs, aligned to lightmap grid

			// round mins/maxs to a lightmap cell size
			imins[0] = appFloor(mins[0] / 16.0f) * 16;
			imins[1] = appFloor(mins[1] / 16.0f) * 16;
			imaxs[0] = appCeil(maxs[0] / 16.0f) * 16;
			imaxs[1] = appCeil(maxs[1] / 16.0f) * 16;
			// calculate lightmap size
			size[0] = (imaxs[0] - imins[0]) / 16 + 1;
			size[1] = (imaxs[1] - imins[1]) / 16 + 1;

			v = s->verts;
			for (j = 0; j < numVerts; j++, v++)
			{
				v->lm[0] = (v->lm[0] - imins[0]) / 16 + 0.5;	// divide to lightmap unit size and shift
				v->lm[1] = (v->lm[1] - imins[1]) / 16 + 0.5;
				v->lm2[0] = v->lm[0] - 0.5;
				v->lm2[1] = v->lm[1] - 0.5;
			}
			// create dynamic lightmap
			dynamicLightmap_t *lm = new (map.dataChain) dynamicLightmap_t;
			s->lightmap = lm;
			for (j = 0; j < 4; j++)			// enum styles
			{
				int st = surfs->styles[j];
				if (st == 255) break;
				lm->style[j] = st;
				lm->modulate[j] = (j == 0 ? 128 : 0);			// initial state
				lm->source[j] = (byte*)NULL + surfs->lightofs + j * size[0] * size[1] * 3;	// just offset in map lightData
			}
			lm->numStyles = j;
			lm->w = size[0];
			lm->h = size[1];
		}
		else
			s->lightmap = NULL;

		BuildPlanarSurfAxis (s);
		if (stex->flags & SURF_LIGHT)		//!! + sky when ambient <> 0
		{
			static color_t	defColor = {96, 96, 96, 255};// {255, 255, 255, 255};

			float area = GetPolyArea (pverts, numVerts);
			image_t *img = FindImage (va("textures/%s", stex->texture), IMAGE_MIPMAP);
			BuildSurfLight (s, img ? &img->color : &defColor, area, stex->value, (stex->flags & SURF_SKY) != 0);
			if (stex->flags & SURF_AUTOFLARE && !(stex->flags & SURF_SKY))
				BuildSurfFlare (s, img ? &img->color : &defColor, area);
		}
	}
}


static int LightmapCompare (const void *s1, const void *s2)
{
	surfacePlanar_t *surf1, *surf2;
	byte	*v1, *v2;

	surf1 = *(surfacePlanar_t **)s1;
	surf2 = *(surfacePlanar_t **)s2;

	v1 = (surf1->lightmap) ? surf1->lightmap->source[0] : NULL;
	v2 = (surf2->lightmap) ? surf2->lightmap->source[0] : NULL;

	if (v1 == v2 && v1)
		return (surf1->shader->style & SHADER_TRYLIGHTMAP) ? 1 : -1;	// TRYLIGHTMAP should go after normal lightmaps

	return v1 - v2;
}

static int ShaderCompare (const void *s1, const void *s2)
{
	surfaceBase_t *surf1, *surf2;

	surf1 = *(surfaceBase_t **)s1;
	surf2 = *(surfaceBase_t **)s2;

	return surf1->shader->sortIndex - surf2->shader->sortIndex;
}

static void GenerateLightmaps2 (byte *lightData, int lightDataSize)
{
	int		i, k, optLmTexels;
	byte	*ptr;
	lightmapBlock_t *bl;
	surfacePlanar_t *s;
	dynamicLightmap_t *dl;
	surfacePlanar_t *sortedSurfaces[MAX_MAP_FACES];

	// NOTE: we assume here, that all Q2-bsp map faces are of surfacePlanar_t type

	LM_Init ();		// reset lightmap status (even when vertex lighting used)

	optLmTexels = 0;

	for (i = 0; i < map.numFaces; i++)
	{
		s = static_cast<surfacePlanar_t*>(map.faces[i]);
		sortedSurfaces[i] = s;		// prepare for sorting

		if (dl = s->lightmap)
		{
			color_t avg;

			// convert lightmap offset to pointer
			for (k = 0; k < dl->numStyles; k++)
				dl->source[k] = lightData + (dl->source[k] - (byte*)NULL);

			// optimize lightmaps
			if (LM_IsMonotone (dl, &avg))
			{
				//!! if no mtex (+env_add/env_combine) -- use 1-pixel lm; otherwise -- vertex lm
				//?? can check avg color too: if c[i] < 128 -- can be doubled, so -- vertex light
				if (1)
				{
					vertex_t *v;

					// replace lightmap block with a single texel
					//!! WARNING: this will modify lightmap in bsp_t (will be affect map reloads)
					//?? -OR- use pixel from center instead (modify dl->source[0]) ?
					byte *p = dl->source[0];
					p[0] = avg.c[0];
					p[1] = avg.c[1];
					p[2] = avg.c[2];
					for (k = 0, v = s->verts; k < s->numVerts; k++, v++)
						v->lm[0] = v->lm[1] = 0.5;		// all verts should point to a middle of lightmap texel
					optLmTexels += dl->w * dl->h - 1;
					dl->w = dl->h = 1;
				}
				else
				{
					// convert to a vertex lightmap
					//!! UNFINISHED: need SPECIAL vertex lm, which will be combined with dyn. lm with ENV_ADD
					s->shader = FindShader (s->shader->name, s->shader->style | SHADER_TRYLIGHTMAP);
					optLmTexels += dl->w * dl->h;
				}
			}
		}
	}
	Com_DPrintf ("removed %d lm texels (%.3f blocks)\n", optLmTexels, (float)optLmTexels / (LIGHTMAP_SIZE * LIGHTMAP_SIZE));

	/*------------ find invalid lightmaps -------------*/
	// sort surfaces by lightmap data pointer
	qsort (sortedSurfaces, map.numFaces, sizeof(surfaceBase_t*), LightmapCompare);

	// check for lightmap intersection
	// NOTE: nere we compare lm->source[0], which should always correspond to a lightstyle=0 (global lighting)
	ptr = NULL;
	for (i = 0; i < map.numFaces; i++)
	{
		s = sortedSurfaces[i];
		dl = s->lightmap;
		if (!dl) continue;

		bool bad = false;
		int lmSize = dl->w * dl->h * dl->numStyles * 3;
		if (dl->source[0] + lmSize > lightData + lightDataSize)
			bad = true;		// out of bsp file
		else if (ptr && ptr > dl->source[0] && (s->shader->style & SHADER_TRYLIGHTMAP))
			bad = true;		// erased by previous block

		//?? add check: if current is "try" interlaced with next "not try (normal)" - current is "bad"

		if (bad)
		{
			// remove lightmap info from the shader
			if (s->shader->lightmapNumber != LIGHTMAP_NONE)
			{
				// later (in this function), just don't create vertex colors for this surface
				Com_DPrintf ("Disable lm for %s\n", s->shader->name);
				SetShaderLightmap (s->shader, LIGHTMAP_NONE);
//				Com_Printf ("  diff: %d\n", ptr - lm->source);
//				Com_Printf ("  w: %d  h: %d  st: %d\n", lm->w, lm->h, lm->numStyles);
			}
			continue;
		}
		else
		{	// advance pointer
			ptr = dl->source[0] + lmSize;
		}

		// after this examination. we can resort lightstyles (cannot do this earlier because used source[0])
		LM_SortLightStyles (dl);
		LM_CheckMinlight (dl);
		// set shader lightstyles
		if (s->shader->lightmapNumber >= 0)
		{
			int styles = 0;
			dl->w2 = dl->w;
			dl->numFastStyles = 0;
			for (k = dl->numStyles - 1; k >= 0; k--)
			{
				byte t = dl->style[k];
				if (IS_FAST_STYLE(t))
				{
					styles = (styles << 8) + t;
					dl->w2 += dl->w;
					dl->numFastStyles++;
				}
			}
			if (styles) s->shader = SetShaderLightstyles (s->shader, styles);
		}
	}

	/*-------------- vertex lighting ------------------*/
	// we should genetate lightmaps after validating of ALL lightmaps
	for (i = 0; i < map.numFaces; i++)
	{
		s = static_cast<surfacePlanar_t*>(map.faces[i]);
		if (!s->lightmap) continue;

		if (s->shader->lightmapNumber == LIGHTMAP_NONE)
		{	// lightmap was removed from shader - remove it from surface too
			s->lightmap = NULL;
			continue;
		}

		UpdateDynamicLightmap (s, true, 0);
	}

	/*----------- allocate lightmaps for surfaces -----------*/
	if (!gl_config.vertexLight)
	{
		// resort surfaces by shader
		qsort (sortedSurfaces, map.numFaces, sizeof(surfaceBase_t*), ShaderCompare);

		bl = NULL;
		i = 0;
		while (i < map.numFaces)
		{
			s = sortedSurfaces[i];
			dl = s->lightmap;
			if (!dl || s->shader->lightmapNumber == LIGHTMAP_VERTEX)
			{
				i++;
				continue;
			}

			shader_t *shader = sortedSurfaces[i]->shader;
			// count number of surfaces with the same shader
			int i2;
			for (i2 = i + 1; i2 < map.numFaces; i2++)
				if (sortedSurfaces[i2]->shader != shader) break;

			int numShaderSurfs = i2 - i;
			int nextIndex = i2;

			// try to allocate all surfaces in a single lightmap block
			LM_Rewind ();
			bool fit = false;
			while (!fit)
			{
				bl = LM_NextBlock ();
				LM_Save (bl);					// save layout
				for (i2 = i; i2 < nextIndex; i2++)
				{
					fit = LM_AllocBlock (bl, sortedSurfaces[i2]->lightmap);
					if (!fit)
					{
						if (bl->empty)
						{	// not fit to EMPTY block -- allocate multiple blocks
							LM_Restore (bl);	// restore layout
							LM_Rewind ();
							bl = NULL;			// do not repeat "Restore()"
							fit = true;
							break;
						}
						break;
					}
				}
				if (bl) LM_Restore (bl);		// ...
				else	bl = LM_NextBlock ();	// was not fit to one block - use 1st available block
			}

			// Here: all surfaces fits "bl" block, or allocate multiple blocks started from "bl"
			for (i2 = i; i2 < nextIndex; i2++)
			{
				dl = sortedSurfaces[i2]->lightmap;
				while (!LM_AllocBlock (bl, dl))
				{
					if (bl->empty)
						Com_DropError ("LM_AllocBlock: unable to allocate lightmap %d x %d\n", dl->w, dl->h);
					LM_Flush (bl);				// finilize block
					bl = LM_NextBlock ();
				}
				LM_PutBlock (dl);
			}

			i = nextIndex;
		}
		LM_Done ();

		// update surfaces
		for (i = 0; i < map.numFaces; i++)
		{
			s = sortedSurfaces[i];
			dl = s->lightmap;
			if (!dl || !dl->block) continue;

			// set lightmap for shader
			s->shader = SetShaderLightmap (s->shader, dl->block->index);

			// update vertex lightmap coords
			float s0 = (float)dl->s / LIGHTMAP_SIZE;
			float t0 = (float)dl->t / LIGHTMAP_SIZE;
			int		j;
			vertex_t *v;
			for (j = 0, v = s->verts; j < s->numVerts; j++, v++)
			{
				v->lm[0] = v->lm[0] / LIGHTMAP_SIZE + s0;
				v->lm[1] = v->lm[1] / LIGHTMAP_SIZE + t0;
			}
		}
	}
}


// Q3's leafFaces are int, Q1/2 - short
static void LoadLeafSurfaces2 (unsigned short *data, int count)
{
	surfaceBase_t **out;

	map.numLeafFaces = count;
	map.leafFaces = out = new (map.dataChain) surfaceBase_t* [count];
	for (int i = 0; i < count; i++, data++, out++)
		*out = map.faces[*data];
}


static void LoadVisinfo2 (dvis_t *data, int size)
{
	int		rowSize;

	map.visRowSize = rowSize = (map.numClusters + 7) >> 3;
	if (size)
	{
		byte *dst = map.visInfo = new (map.dataChain) byte [rowSize * map.numClusters];
		for (int i = 0; i < map.numClusters; i++)
		{
			int pos = data->bitofs[i][DVIS_PVS];
			if (pos != -1)
			{
				byte *src = (byte*)data + pos;
				// decompress vis
				int j = rowSize;
				while (j)
				{
					byte	c;

					if (c = *src++)
					{	// non-zero byte
						*dst++ = c;
						j--;
					}
					else
					{	// zero byte -- decompress RLE data (with filler 0)
						c = *src++;				// count
						c = min(c, j);			// should not be, but ...
						j -= c;
						while (c--)
							*dst++ = 0;
					}
				}
			}
			else
			{
				memset (dst, 0xFF, rowSize);	// all visible
				dst += rowSize;
			}
		}
	}
}


static void LoadBsp2 (bspfile_t *bsp)
{
	// Load planes
	LoadPlanes (bsp->planes, bsp->numPlanes, sizeof(dplane_t));
	// Load surfaces
START_PROFILE(LoadSurfaces2)
	LoadSurfaces2 (bsp->faces, bsp->numFaces, bsp->surfedges, bsp->edges, bsp->vertexes, bsp->texinfo, bsp->models, bsp->numModels);
END_PROFILE
	LoadLeafSurfaces2 (bsp->leaffaces, bsp->numLeaffaces);
START_PROFILE(GenerateLightmaps2)
	GenerateLightmaps2 (bsp->lighting, bsp->lightDataSize);
END_PROFILE
	// Load bsp (leafs and nodes)
START_PROFILE(LoadLeafsNodes2)
	LoadLeafsNodes2 (bsp->nodes, bsp->numNodes, bsp->leafs, bsp->numLeafs);
END_PROFILE
START_PROFILE(LoadVisinfo2)
	LoadVisinfo2 (bsp->visibility, bsp->visDataSize);
END_PROFILE
START_PROFILE(LoadInlineModels2)
	LoadInlineModels2 (bsp->models, bsp->numModels);
END_PROFILE

	switch (bsp->fogMode)
	{
	case fog_exp:
		gl_fogMode = GL_EXP;
		gl_fogDensity = bsp->fogDens;
		break;
	case fog_exp2:
		gl_fogMode = GL_EXP2;
		gl_fogDensity = bsp->fogDens;
		break;
	case fog_linear:
		gl_fogMode = GL_LINEAR;
		gl_fogStart = bsp->fogStart;
		gl_fogEnd = bsp->fogEnd;
		break;
	default:
		gl_fogMode = 0;
	}
	if (bsp->fogMode)
		VectorCopy (bsp->fogColor, gl_fogColor);
	else
		VectorClear (gl_fogColor);
	gl_fogColor[3] = 1;
}


/*-----------------------------------------------------------------------------
	LoadWorldMap()
-----------------------------------------------------------------------------*/

void LoadWorldMap (const char *name)
{
	guard(LoadWorldMap);

	char name2[MAX_QPATH];
	appCopyFilename (name2, name, sizeof(name2));

	// map must be reloaded to update shaders (which are restarted every BeginRegistration())
//	if (!strcmp (name2, map.name))
//		return;		// map is not changed

	FreeModels ();
	strcpy (map.name, name2);
	map.dataChain = new CMemoryChain;

	// map should be already loaded by client
	bspfile = LoadBspFile (name2, true, NULL);

	// load sun
	map.sunLight = bspfile->sunLight;
	if (bspfile->sunColor[0] + bspfile->sunColor[1] + bspfile->sunColor[2])		// sun color was overrided
		VectorCopy (bspfile->sunColor, map.sunColor);
	NormalizeColor (map.sunColor, map.sunColor);
	VectorCopy (bspfile->sunVec, map.sunVec);
	VectorCopy (bspfile->sunAmbient, map.sunAmbient);
	if (bspfile->sunAmbient[0] + bspfile->sunAmbient[1] + bspfile->sunAmbient[2] > 0)
		map.haveSunAmbient = true;
	VectorCopy (bspfile->ambientLight, map.ambientLight);

	switch (bspfile->type)
	{
	case map_q2:
	case map_kp:
		LoadBsp2 (bspfile);
		// get ambient light from lightmaps (place to all map types ??)
		if (map.ambientLight[0] + map.ambientLight[1] + map.ambientLight[2] == 0)
		{
			map.ambientLight[0] = lmMinlight.c[0] * 2;
			map.ambientLight[1] = lmMinlight.c[1] * 2;
			map.ambientLight[2] = lmMinlight.c[2] * 2;
			Com_DPrintf ("Used minlight from lightmap {%d, %d, %d}\n", VECTOR_ARG(lmMinlight.c));
		}
		break;
	default:
		Com_DropError ("R_LoadWorldMap: unknown BSP type");
	}
	LoadFlares (bspfile->flares, bspfile->numFlares);
	LoadSlights (bspfile->slights, bspfile->numSlights);
	PostLoadLights ();
	InitLightGrid ();

	unguard;
}


//?? can use cmodel.cpp function
node_t *PointInLeaf (vec3_t p)
{
	if (!map.name[0] || !map.numNodes)
		Com_DropError ("R_PointInLeaf: bad model");

	node_t *node = map.nodes;
	while (true)
	{
		if (!node->isNode)
			return node;

		cplane_t *plane = node->plane;
		if (DotProduct (p, plane->normal) - plane->dist > 0)
			node = node->children[0];
		else
			node = node->children[1];
	}

	return NULL;	// never
}


/*-----------------------------------------------------------------------------
	Loading triangle models
-----------------------------------------------------------------------------*/

// Find xyz_index for each "st" (st have unique xyz, but xyz may have few st on texture seams)
// and fill "st" list.
// This function may be used for Q2 and KP models
static int ParseGlCmds (const char *name, surfaceMd3_t *surf, int *cmds, int *xyzIndexes)
{
	int		vertsIndexes[1024];		// verts per triangle strip/fan

	guard(ParseGlCmds);

	int numTris = 0;
	int allocatedVerts = 0;
	int *idx = surf->indexes;
	while (int count = *cmds++)
	{
		int		i, i1, i2, i3;

		// cmds -> s1, t1, idx1, s2, t2, idx2 ...

		bool strip = count > 0;
		if (!strip)
			count = -count;
		numTris += count - 2;

		if (numTris > surf->numTris)
		{
			Com_WPrintf ("ParseGlCmds(%s): incorrect triangle count\n", name);
			return 0;
		}

		// generate vertexes and texcoords
		for (i = 0; i < count; i++, cmds += 3)
		{
			float s = ((float*)cmds)[0];
			float t = ((float*)cmds)[1];

			// find st in allocated vertexes
			int		index;
			float	*dst;
			for (index = 0, dst = surf->texCoords; index < allocatedVerts; index++, dst += 2)
				if (xyzIndexes[index] == cmds[2] && dst[0] == s && dst[1] == t) break;

			if (index == allocatedVerts)
			{	// vertex not found - allocate it
				if (allocatedVerts == surf->numVerts)
				{
					Com_WPrintf ("ParseGlCmds(%s): too much texcoords\n", name);
					return false;
				}
				dst[0] = s;
				dst[1] = t;
				xyzIndexes[allocatedVerts] = cmds[2];
				allocatedVerts++;
			}

			vertsIndexes[i] = index;
		}

		if (!idx)	// called to calculate numVerts - skip index generation
			continue;

		// generate indexes
		i1 = vertsIndexes[0]; i2 = vertsIndexes[1];
		for (i = 2; i < count; i++)
		{
			i3 = vertsIndexes[i];
			// put indexes
			*idx++ = i1; *idx++ = i2; *idx++ = i3;
			// prepare next step
			if (strip)
			{
				if (!(i & 1))	i1 = i3;
				else			i2 = i3;
			}
			else				i2 = i3;
		}
	}

	surf->numTris = numTris;	// update triangle count
	return allocatedVerts;

	unguard;
}


static float ComputeMd3Radius (vec3_t localOrigin, vertexMd3_t *verts, int numVerts)
{
	guard(ComputeMd3Radius);

	float radius2 = 0;
	for (int i = 0; i < numVerts; i++, verts++)
	{
		vec3_t p;
		p[0] = verts->xyz[0] * MD3_XYZ_SCALE;
		p[1] = verts->xyz[1] * MD3_XYZ_SCALE;
		p[2] = verts->xyz[2] * MD3_XYZ_SCALE;
		VectorSubtract (p, localOrigin, p);
		float tmp = DotProduct (p, p);	// tmp = dist(p, localOrigin) ^2
		radius2 = max(tmp, radius2);
	}
	return sqrt (radius2);

	unguard;
}


static void ProcessMd2Frame (vertexMd3_t *verts, dMd2Frame_t *srcFrame, md3Frame_t *dstFrame, int numVerts, int *xyzIndexes)
{
	guard(ProcessMd2Frame);

	ClearBounds (dstFrame->mins, dstFrame->maxs);
	int i;
	vertexMd3_t *dstVerts;
	for (i = 0, dstVerts = verts; i < numVerts; i++, dstVerts++)
	{
		dMd2Vert_t *srcVert = &srcFrame->verts[xyzIndexes[i]];
		// compute vertex
		vec3_t p;
		p[0] = srcFrame->scale[0] * srcVert->v[0] + srcFrame->translate[0];
		p[1] = srcFrame->scale[1] * srcVert->v[1] + srcFrame->translate[1];
		p[2] = srcFrame->scale[2] * srcVert->v[2] + srcFrame->translate[2];
		// update bounding box
		AddPointToBounds (p, dstFrame->mins, dstFrame->maxs);
		// put vertex in a "short" form
		dstVerts->xyz[0] = appRound (p[0] / MD3_XYZ_SCALE);
		dstVerts->xyz[1] = appRound (p[1] / MD3_XYZ_SCALE);
		dstVerts->xyz[2] = appRound (p[2] / MD3_XYZ_SCALE);
	}
	// compute bounding sphere center
	for (i = 0; i < 3; i++)
		dstFrame->localOrigin[i] = (dstFrame->mins[i] + dstFrame->maxs[i]) / 2;
	// and radius
	dstFrame->radius = ComputeMd3Radius (dstFrame->localOrigin, verts, numVerts);
	unguard;
}


static void BuildMd2Normals (surfaceMd3_t *surf, int *xyzIndexes, int numXyz)
{
	int		i, j, k, *idx;
	vec3_t	normals[MD3_MAX_VERTS];	// normal per xyzIndex
	short	norm_i[MD3_MAX_VERTS];
	vertexMd3_t *verts;
	float	*dst;

	guard(BuildMd2Normals);

	for (i = 0, verts = surf->verts; i < surf->numFrames; i++, verts += surf->numVerts)
	{
		// clear normals array
		memset (normals, 0, sizeof(normals));

		for (j = 0, idx = surf->indexes; j < surf->numTris; j++, idx += 3)
		{
			vec3_t	vecs[3], n;

			// compute triangle normal
			for (k = 0; k < 3; k++)
			{
				VectorSubtract (verts[idx[k]].xyz, verts[idx[k == 2 ? 0 : k + 1]].xyz, vecs[k]);
				VectorNormalizeFast (vecs[k]);
			}
			CrossProduct (vecs[1], vecs[0], n);
			VectorNormalizeFast (n);
			// add normal to verts
			for (k = 0; k < 3; k++)
			{
				float	ang;

#if 1
				ang = -DotProduct (vecs[k], vecs[k == 0 ? 2 : k - 1]);
				ang = ACOS_FUNC(ang);
#else
				ang = acos (-DotProduct (vecs[k], vecs[k == 0 ? 2 : k - 1]));
#endif
				dst = &normals[xyzIndexes[idx[k]]][0];
				VectorMA (dst, ang, n, dst);		// weighted normal: weight ~ angle
			}
		}
		// convert computed xyz normals to compact form
		for (j = 0; j < numXyz; j++)
		{
			byte	a, b;

			dst = &normals[j][0];
#if 1
			VectorNormalizeFast (dst);
			a = appRound (ACOS_FUNC(dst[2]) / (M_PI * 2) * 255);
			if (dst[0])		b = appRound (ATAN2_FUNC (dst[1], dst[0]) / (M_PI * 2) * 255);
#else
			VectorNormalize (dst);
			a = appRound (acos (dst[2]) / (M_PI * 2) * 255);
			if (dst[0])		b = appRound (atan2 (dst[1], dst[0]) / (M_PI * 2) * 255);
#endif
			else			b = dst[1] > 0 ? 127 : -127;
			norm_i[j] = a | (b << 8);
		}
		// copy normals
		for (j = 0; j < surf->numVerts; j++)
			verts[j].normal = norm_i[xyzIndexes[j]];
	}

	unguard;
}


static void SetMd3Skin (const char *name, surfaceMd3_t *surf, int index, const char *skin)
{
	// try specified shader
	shader_t *shader = FindShader (skin, SHADER_CHECK|SHADER_SKIN);
	if (!shader)
	{
		char	mName[MAX_QPATH], sName[MAX_QPATH];	// model/skin
		// try to find skin forcing model directory
		appCopyFilename (mName, name, sizeof(mName));
		char *mPtr = strrchr (mName, '/');
		if (mPtr)	mPtr++;			// skip '/'
		else		mPtr = mName;

		appCopyFilename (sName, skin, sizeof(sName));
		char *sPtr = strrchr (sName, '/');
		if (sPtr)	sPtr++;			// skip '/'
		else		sPtr = sName;

		strcpy (mPtr, sPtr);		// make "modelpath/skinname"
		shader = FindShader (mName, SHADER_CHECK|SHADER_SKIN);
		if (!shader)
		{	// not found
			Com_DPrintf ("SetSkin(%s:%d): %s or %s is not found\n", name, index, skin, mName);
			if (index > 0)
				shader = surf->shaders[0];		// better than default shader
			else
				shader = FindShader ("*identityLight", SHADER_SKIN); // white + diffuse lighting
		}
	}
	// set the shader
	surf->shaders[index] = shader;
}


#define MAX_XYZ_INDEXES		4096	// maximum number of verts in loaded md3 model

#if 0
#define BUILD_SCELETON
//???? rename, may be remove
static void CheckTrisSizes (surfaceMd3_t *surf, dMd2Frame_t *md2Frame = NULL, int md2FrameSize = 0)
{
	int		tri, *inds;
	bool	fixed[MAX_XYZ_INDEXES];

	//!! special cases:
	//!!	1. numFrames == 1 -- static model
	//!!	2. numFixed == numTris: model -> static + info about placement relative to model axis
	//!!		BUT: require to check presence of a SINGLE skeleton (model can have few parts, which are
	//!!		moved one along/around another)
	//!! TODO:
	//!!		- optimization: build list of adjancent tris, list of same verts (doubled on skin seams)
	//!!		- may be, check not tris -- check edges -- for building skeleton

	int numFixed = surf->numTris;
	memset (&fixed, 1, sizeof(fixed));	// fill by "true" ?????

	// compute max md2 frame scale (int * scale = float, so "scale" can be used as possible error)
	vec3_t scale;
	if (md2Frame)
	{
		VectorClear (scale);
		for (int frame = 0; frame < surf->numFrames; frame++, md2Frame = OffsetPointer (md2Frame, md2FrameSize))
		{
			if (md2Frame->scale[0] > scale[0]) scale[0] = md2Frame->scale[0];
			if (md2Frame->scale[1] > scale[1]) scale[1] = md2Frame->scale[1];
			if (md2Frame->scale[2] > scale[2]) scale[2] = md2Frame->scale[2];
		}
	}
	else
	{
		//?? for real md3 model can pass NULL instead of md2Frame and use constant scale[]
		scale[0] = scale[1] = scale[2] = 1.0f / MD3_XYZ_SCALE;
	}

	for (tri = 0, inds = surf->indexes; tri < surf->numTris; tri++, inds += 3)
	{
		int		i, j;

		// integer bounds
		int	deltaMin[3], deltaMax[3];
		// set deltaMin/deltaMax[i] bounds to a maximum diapason
		for (i = 0; i < 3; i++)
		{
			deltaMin[i] = -BIG_NUMBER;
			deltaMax[i] = BIG_NUMBER;
		}
//		float maxError2 = 0;
		float errorConst = DotProduct (scale, scale);

		vertexMd3_t *verts = surf->verts;
		for (int frame = 0; frame < surf->numFrames; frame++, verts += surf->numVerts)
		{
			vec3_t d;

			// compute distance between 2 verts from triangle separately by each coord
			for (i = 0; i < 3; i++)
			{
				int k = (i == 2) ? 0 : i + 1;
				for (j = 0; j < 3; j++)
					d[j] = abs(verts[inds[i]].xyz[j] - verts[inds[k]].xyz[j]) * MD3_XYZ_SCALE;
				// squared distance
				float dist2 = DotProduct(d, d);
				// quant-based error
				float error2 = (2 * DotProduct(d, scale) + errorConst);	//!!!
				// distance-based error
				float dist = SQRTFAST(dist2);
				float err = dist * Cvar_VariableValue("err") / 100;	//!! percent from distance; make const
				// shrink deltaMin[i]/deltaMax[i] bounds to dist2 +/- error2
				float min = dist2 - error2;
				float min2 = (dist - err); min2 *= min2;
				if (min2 < min) min = min2;			// use maximum range: [min..max] or [min2..max2]
				if (deltaMin[i] < min)				// shrink deltaMin
					deltaMin[i] = min;
				float max = dist2 + error2;
				float max2 = (dist + err); max2 *= max2;
				if (max2 > max) max = max2;			// use maximum range
				if (deltaMax[i] > max)				// shrink deltaMax
					deltaMax[i] = max;
				if (deltaMin[i] > deltaMax[i])		// empty range
				{
					fixed[tri] = false;
					numFixed--;
					break;
				}
			}

			if (!fixed[tri]) break;
		}

		//?? can make indexes[tri] = 0 (exclude tri from drawing)
		if (!fixed[tri])
			inds[0] = inds[1] = inds[2] = 0;

		//???
//		Com_Printf ("%s%3d\n", fixed[tri] ? S_GREEN : "", tri);
	}
	Com_Printf (S_CYAN"FIXED: %d / %d\n", numFixed, surf->numTris);	//??
}

#endif


md3Model_t *LoadMd2 (const char *name, byte *buf, unsigned len)
{
	guard(LoadMd2);

	md3Model_t *md3;
	surfaceMd3_t *surf;
	int		i, numVerts, numTris;
	int		xyzIndexes[MAX_XYZ_INDEXES];

	//?? should add ri.LoadMd2 (buf, size) -- sanity check + swap bytes for big-endian machines
	dMd2_t *hdr = (dMd2_t*)buf;
	if (hdr->version != MD2_VERSION)
	{
		Com_WPrintf ("R_LoadMd2: %s has wrong version %d\n", name, hdr->version);
		return NULL;
	}

	if (hdr->numXyz <= 0 || hdr->numTris <= 0 || hdr->numFrames <= 0)
	{
		Com_WPrintf ("R_LoadMd2: %s has incorrect surface params\n", name);
		return NULL;
	}

	/* We should determine number of vertexes for conversion of md2 model into md3 format, because
	 * in md2 one vertex may have few texcoords (for skin seams); ParseGlCmds() will detect seams and
	 * duplicate vertex
	 */
	surf = (surfaceMd3_t*)appMalloc (sizeof(surfaceMd3_t) + MAX_XYZ_INDEXES * 2*sizeof(int));	// alloc space for MAX_XYZ_INDEXES texcoords
	surf->texCoords = (float*)(surf+1);
	surf->numVerts = MAX_XYZ_INDEXES;
	surf->numTris = MAX_XYZ_INDEXES - 2;
	numVerts = ParseGlCmds (name, surf, (int*)(buf + hdr->ofsGlcmds), xyzIndexes);
	numTris = surf->numTris;		// just computed
	if (numTris != hdr->numTris) Com_WPrintf ("LoadMd2(%s): computed numTris %d != %d\n", name, numTris, hdr->numTris);
	appFree (surf);
	Com_DPrintf ("MD2(%s): xyz=%d st=%d verts=%d tris=%d frms=%d\n", name, hdr->numXyz, hdr->numSt, numVerts, numTris, hdr->numFrames);

	/* Allocate memory:
		md3Model_t		[1]
		md3Frame_t		[numFrames]
		surfaceMd3_t	[numSurfaces == 1]
		float			texCoords[2*numVerts]
		int				indexes[3*numTris]
		vertexMd3_t		verts[numVerts][numFrames]
		shader_t*		shaders[numShaders == numSkins]
	 */
	int size = sizeof(md3Model_t) + hdr->numFrames*sizeof(md3Frame_t)
		+ sizeof(surfaceMd3_t)
		+ numVerts*2*sizeof(float) + 3*hdr->numTris*sizeof(int)
		+ numVerts*hdr->numFrames*sizeof(vertexMd3_t) + hdr->numSkins*sizeof(shader_t*);
	md3 = (md3Model_t*) appMalloc (size);
	CALL_CONSTRUCTOR(md3);
	strcpy (md3->name, name);
	md3->size = size;

	/*-------- fill md3 structure --------*/
	md3->numSurfaces = 1;
	md3->numFrames = hdr->numFrames;
	md3->frames = (md3Frame_t*)(md3 + 1);

	md3->surf = surf = (surfaceMd3_t*)(md3->frames + md3->numFrames);
	CALL_CONSTRUCTOR(surf);

	/*-------- fill surf structure -------*/
	surf->shader = gl_defaultShader;		// any
	// counts
	surf->numFrames = hdr->numFrames;
	surf->numVerts = numVerts;
	surf->numTris = numTris;
	surf->numShaders = hdr->numSkins;
	// pointers
	surf->texCoords = (float*)(surf + 1);
	surf->indexes = (int*)(surf->texCoords + 2*surf->numVerts);
	surf->verts = (vertexMd3_t*)(surf->indexes + 3*surf->numTris);
	surf->shaders = (shader_t**)(surf->verts + surf->numVerts*surf->numFrames);

START_PROFILE(..Md2::Parse)
	/*--- build texcoords and indexes ----*/
	if (!ParseGlCmds (name, surf, (int*)(buf + hdr->ofsGlcmds), xyzIndexes))
	{
		appFree (md3);
		return NULL;
	}
END_PROFILE

START_PROFILE(..Md2::Frame)
	/*---- generate vertexes/normals -----*/
	for (i = 0; i < surf->numFrames; i++)
		ProcessMd2Frame (surf->verts + i * numVerts,
				(dMd2Frame_t*)(buf + hdr->ofsFrames + i * hdr->frameSize),
				md3->frames + i, numVerts, xyzIndexes);
END_PROFILE
START_PROFILE(..Md2::Normals)
	BuildMd2Normals (surf, xyzIndexes, hdr->numXyz);
END_PROFILE

#ifdef BUILD_SCELETON	//????
	CheckTrisSizes (surf, (dMd2Frame_t*)(buf + hdr->ofsFrames), hdr->frameSize);
#endif

START_PROFILE(..Md2::Skin)
	/*---------- load skins --------------*/
	surf->numShaders = hdr->numSkins;
	const char *skin;
	for (i = 0, skin = (char*)(buf + hdr->ofsSkins); i < surf->numShaders; i++, skin += MD2_MAX_SKINNAME)
		SetMd3Skin (name, surf, i, skin);
END_PROFILE

	return md3;

	unguardf(("%s", name));
}


md3Model_t *LoadMd3 (const char *name, byte *buf, unsigned len)
{
	guard(LoadMd3);

	int		i;
	dMd3Surface_t *ds;
	surfaceMd3_t *surf;

	//?? load LOD
	//?? should add ri.LoadMd3 (buf, size) -- sanity check + swap bytes for big-endian machines
	dMd3_t *hdr = (dMd3_t*)buf;
	if (hdr->version != MD3_VERSION)
	{
		Com_WPrintf ("R_LoadMd3: %s has wrong version %d\n", name, hdr->version);
		return NULL;
	}

	if (hdr->numSurfaces <= 0 || hdr->numTags < 0 || hdr->numFrames <= 0)
	{
		Com_WPrintf ("R_LoadMd3: %s has incorrect surfaces\n", name);
		return NULL;
	}

	// all-surface counts (total sums)
	int tNumVerts = 0;
	int tNumTris = 0;
	int tNumShaders = 0;
	ds = (dMd3Surface_t*)(buf + hdr->ofsSurfaces);
	for (i = 0; i < hdr->numSurfaces; i++)
	{
		// check: model.numFrames should be == all surfs.numFrames
		if (ds->numFrames != hdr->numFrames)
		{
			Com_WPrintf ("R_LoadMd3: %s have different frame counts in surfaces\n", name);
			return NULL;
		}
		// counts
		tNumVerts += ds->numVerts;
		tNumTris += ds->numTriangles;
		tNumShaders += ds->numShaders;
		// next surface
		ds = OffsetPointer (ds, ds->ofsEnd);
	}
	Com_DPrintf ("MD3(%s): verts=%d tris=%d frms=%d surfs=%d tags=%d\n", name, tNumVerts, tNumTris, hdr->numFrames, hdr->numSurfaces, hdr->numTags);

	/* Allocate memory:
		md3Model_t		[1]
		md3Frame_t		[numFrames]
		md3Tag_t		[numTags][numFrames]
		surfaceMd3_t	[numSurfaces]
		-- surfaces data [numSurfaces] --
		float			texCoords[2*s.numVerts]
		int				indexes[3*s.numTriangles]
		vertexMd3_t		verts[s.numVerts][numFrames]
		shader_t*		shaders[s.numShaders]
	 */
	int size = sizeof(md3Model_t) + hdr->numFrames*sizeof(md3Frame_t) + hdr->numTags*hdr->numFrames*sizeof(md3Tag_t)
		+ hdr->numSurfaces*sizeof(surfaceMd3_t);
	int surfDataOffs = size;			// start of data for surfaces
	size += tNumVerts*2*sizeof(float) + tNumTris*3*sizeof(int) + tNumVerts*hdr->numFrames*sizeof(vertexMd3_t) + tNumShaders*sizeof(shader_t*);

	md3Model_t *md3 = (md3Model_t*) appMalloc (size);
	CALL_CONSTRUCTOR(md3);
	strcpy (md3->name, name);
	md3->size = size;

	/*-------- fill md3 structure --------*/
	// frames
	md3->numFrames = hdr->numFrames;
	md3->frames = (md3Frame_t*)(md3 + 1);
	dMd3Frame_t *fs = (dMd3Frame_t*)(buf + hdr->ofsFrames);
	for (i = 0; i < hdr->numFrames; i++, fs++)
	{
		// NOTE: md3 models, created with id model converter, have frame.localOrigin == (0,0,0)
		// So, we should recompute localOrigin and radius for more effective culling
		md3Frame_t *frm = &md3->frames[i];
		VectorCopy (fs->bounds[0], frm->mins);
		VectorCopy (fs->bounds[1], frm->maxs);
		// compute localOrigin
		for (int j = 0; j < 3; j++)
			frm->localOrigin[j] = (frm->maxs[j] + frm->mins[j]) / 2;
		frm->radius = 0;		// will compute later
	}
	// tags
	//?? can optimize tags by name: between frames tag should not change its name ?!
	md3->numTags = hdr->numTags;
	md3->tags = (md3Tag_t*)(md3->frames + md3->numFrames);
	memcpy (md3->tags, buf + hdr->ofsTags, hdr->numTags*hdr->numFrames*sizeof(dMd3Tag_t));	// same layout on disk and in memory
	// surfaces
	md3->numSurfaces = hdr->numSurfaces;
	md3->surf = (surfaceMd3_t*)(md3->tags + md3->numTags*hdr->numFrames);
	ds = (dMd3Surface_t*)(buf + hdr->ofsSurfaces);
	byte *surfData = (byte*)md3 + surfDataOffs;
	for (i = 0, surf = md3->surf; i < hdr->numSurfaces; i++, surf++)
	{
		CALL_CONSTRUCTOR(surf);
		surf->shader = gl_defaultShader;	// any
		// counts
		surf->numFrames = hdr->numFrames;
		surf->numVerts = ds->numVerts;
		surf->numTris = ds->numTriangles;
		surf->numShaders = ds->numShaders;
		// pointers
		surf->texCoords = (float*)surfData;
		surf->indexes = (int*)(surf->texCoords + 2*surf->numVerts);
		surf->verts = (vertexMd3_t*)(surf->indexes + 3*surf->numTris);
		surf->shaders = (shader_t**)(surf->verts + surf->numVerts*surf->numFrames);
		surfData = (byte*)(surf->shaders + surf->numShaders);
		// triangles: same layout on disk and in memory
		memcpy (surf->indexes, (byte*)ds + ds->ofsTriangles, ds->numTriangles * sizeof(dMd3Triangle_t));
		// texcoords: same layout on disk and in memory
		memcpy (surf->texCoords, (byte*)ds + ds->ofsSt, ds->numVerts * sizeof(dMd3St_t));
		// verts: same layout on disk and in memory
		memcpy (surf->verts, (byte*)ds + ds->ofsXyzNormals, ds->numVerts * ds->numFrames * sizeof(dMd3XyzNormal_t));
		// shaders
		dMd3Shader_t *ss = (dMd3Shader_t*)((byte*)ds + ds->ofsShaders);
		for (int j = 0; j < ds->numShaders; j++, ss++)
			SetMd3Skin (name, surf, j, ss->name);
		// next surface
		ds = OffsetPointer (ds, ds->ofsEnd);
	}
	// compute frame radiuses
	md3Frame_t *frm = md3->frames;
	for (i = 0; i < md3->numFrames; i++, frm++) // iterate frames
	{
		float radius = 0;
		surf = md3->surf;
		for (int j = 0; j < md3->numSurfaces; j++, surf++) // iterate surfaces
		{
			// find surface vertexes for current frame
			vertexMd3_t *verts = surf->verts + surf->numVerts * i;
			float radius2 = ComputeMd3Radius (frm->localOrigin, verts, surf->numVerts);
			if (radius2 > radius) radius = radius2;
		}
		frm->radius = radius;
	}
	return md3;

	unguardf(("%s", name));
}


shader_t *FindSkin (const char *name)
{
	return FindShader (name, SHADER_CHECK|SHADER_SKIN);
}


/*-------------- Sprite models  ----------------*/


static sp2Model_t *LoadSp2 (const char *name, byte *buf, unsigned len)
{
	dSp2_t *hdr = (dSp2_t*)buf;
	if (hdr->version != SP2_VERSION)
	{
		Com_WPrintf ("R_LoadSp2: %s has wrong version %d\n", name, hdr->version);
		return NULL;
	}
	if (hdr->numframes < 0)
	{
		Com_WPrintf ("R_LoadSp2: %s has incorrect frame count %d\n", name, hdr->numframes);
		return NULL;
	}

	int size = sizeof(sp2Model_t) + (hdr->numframes-1) * sizeof(sp2Frame_t);
	sp2Model_t *sp2 = (sp2Model_t*)appMalloc (size);
	CALL_CONSTRUCTOR(sp2);
	strcpy (sp2->name, name);
	sp2->size = size;

	sp2->numFrames = hdr->numframes;
	sp2->radius = 0;
	// parse frames
	dSp2Frame_t *in = hdr->frames;
	sp2Frame_t *out = sp2->frames;
	for (int i = 0; i < hdr->numframes; i++, in++, out++)
	{
		out->width = in->width;
		out->height = in->height;
		out->localOrigin[0] = in->origin_x;
		out->localOrigin[1] = in->origin_y;

		float s = max (out->localOrigin[0], out->width - out->localOrigin[0]);
		float t = max (out->localOrigin[1], out->height - out->localOrigin[1]);
		float radius = sqrt (s * s + t * t);
		sp2->radius = max (sp2->radius, radius);

		out->shader = FindShader (in->name, SHADER_CHECK|SHADER_WALL|SHADER_FORCEALPHA);
		if (!out->shader)
		{
			Com_DPrintf ("R_LoadSp2(%s): %s is not found\n", name, in->name);
			out->shader = gl_defaultShader;
		}
	}

	return sp2;
}


/*----------------------------------------------*/


static void Modellist_f (bool usage, int argc, char **argv)
{
	static const char *types[] = {"unk",	"inl",	"sp2",		"md3"};		// see modelType_t
	static const char *colors[] = {S_RED,	"",		S_MAGENTA, S_GREEN};	// ...

	if (argc > 2 || usage)
	{
		Com_Printf ("Usage: modellist [mask]\n");
		return;
	}
	const char *mask = (argc == 2) ? argv[1] : NULL;

	int totalSize = 0;
	int totalCount = 0;
	Com_Printf ("-----type-size----name---------\n");
	for (int i = 0; i < modelCount; i++)
	{
		model_t *m = modelsArray[i];
		if (mask && !appMatchWildcard (m->name, mask, true)) continue;
		totalCount++;
		Com_Printf ("%-3d  %3s  %-7d %s%s\n", i, types[m->type], m->size, colors[m->type], m->name);
		totalSize += m->size;
	}
	Com_Printf ("Displayed %d/%d models, used %d bytes\n", totalCount, modelCount, totalSize);
}


static void FreeModels ()
{
	// free non-inline models
	for (int i = 0; i < modelCount; i++)
	{
		model_t *m = modelsArray[i];
		if (m && m->size >= 0) delete m;		// when size < 0 -- not allocated directly, used from different place
	}

	memset (modelsArray, 0, sizeof(modelsArray));
	modelCount = 0;

	// free map data
	if (map.dataChain) delete map.dataChain;
	if (map.lightGridChain) delete map.lightGridChain;
	memset (&map, 0, sizeof(map));
}


#ifdef TEST_LOAD
static void LoadModel_f (bool usage, int argc, char **argv)
{
	if (argc != 2 || usage)
	{
		Com_Printf ("Usage: loadmodel <filename>\n");
		return;
	}
	FindModel (argv[1]);
}
#endif


void InitModels ()
{
	memset (&map, 0, sizeof(map));

	RegisterCommand ("modellist", Modellist_f);
#ifdef TEST_LOAD
	RegisterCommand ("loadmodel", LoadModel_f);
#endif
}


void ShutdownModels ()
{
	FreeModels ();

	UnregisterCommand ("modellist");
#ifdef TEST_LOAD
	UnregisterCommand ("loadmodel");
#endif
}


} // namespace
