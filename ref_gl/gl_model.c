#include "gl_local.h"
#include "gl_model.h"
#include "gl_lightmap.h"
#include "gl_shader.h"
#include "gl_math.h"
#include "gl_poly.h"


#define PROFILE_LOADING

#ifdef PROFILE_LOADING
#define START_PROFILE(name)	\
	{	\
		static char _name[] = #name;	\
		int	_time = Sys_Milliseconds();
#define END_PROFILE	\
		_time = Sys_Milliseconds() - _time;	\
		if (Cvar_VariableInt("r_profile")) Com_Printf("^5%s: %d ms\n", _name, _time);	\
	}
#else
#define START_PROFILE(name)
#define END_PROFILE
#endif

bspModel_t gl_worldModel;
static bspfile_t	*bspfile;

#define MAX_GLMODELS	1024

static model_t modelsArray[MAX_GLMODELS];
static int	modelCount;


/*---------------- Loading models: common ----------------*/

static qboolean LoadMd2 (model_t *m, byte *buf, int len);
static qboolean LoadSp2 (model_t *m, byte *buf, int len);


model_t	*GL_FindModel (char *name)
{
	char	name2[MAX_QPATH];
	int		i, len;
	model_t	*m;
	unsigned *file;

	if (!name[0])
		Com_Error (ERR_FATAL, "R_FindModel: NULL name");

	Q_CopyFilename (name2, name, sizeof(name2)-1);

	// search already loaded models
	for (i = 0, m = modelsArray; i < modelCount; i++, m++)
		if (!strcmp (name2, m->name))
			return m;

	if (i == MAX_GLMODELS)
	{
		Com_WPrintf ("R_FindModel: MAX_GLMODELS\n");
		return NULL;
	}

	// prepare slot
//	m = modelsArray[modelCount]; -- already
	modelCount++;		// reserve slot
	strcpy (m->name, name2);

START_PROFILE(FindModel::Load)
	/*----- not found -- load model ------*/
	len = FS_LoadFile (name2, (void**)&file);
	if (!file)
	{
		modelCount--;	// free slot
		Com_DPrintf ("R_FindModel: %s not found\n", name2);
		return NULL;	// file not found
	}
END_PROFILE
START_PROFILE(FindModel::Process)

	switch (LittleLong(*file))
	{
	case MD2_IDENT:
		if (!LoadMd2 (m, (byte*)file, len)) m = NULL;
		break;
	case SP2_IDENT:
		if (!LoadSp2 (m, (byte*)file, len)) m = NULL;
		break;
	default:
		// do not Com_Error() here: simply ignore unknown model formats
		Com_WPrintf ("R_FindModel: unknown ID 0x%X in %s", LittleLong (*file), name);
		m = NULL;
	}
	if (!m)
		modelCount--;	// model was not loaded - free slot back

END_PROFILE
	FS_FreeFile (file);
	return m;
}


/*------------------ Common BSP loading code --------------------*/

// "stride" = sizeof(dplane_t) or sizeof(dplane3_t)
// Both this structures has same fields, except Q3 lost "type"
static void LoadPlanes (dplane_t *data, int count, int stride)
{
	int		i;
	dplane_t *in;
	cplane_t *out;

	map.numPlanes = count;
	map.planes = out = Hunk_Alloc (count * sizeof(cplane_t));
	in = data;

	for (i = 0; i < count; i++, in = (dplane_t*)((byte*)in + stride), out++)
	{
		VectorCopy (in->normal, out->normal);
		out->dist = in->dist;
		out->type = PlaneTypeForNormal (out->normal);
		SetPlaneSignbits (out);
	}
}


static void LoadFlares (lightFlare_t *data, int count)
{
	int		i;
	gl_flare_t *out;

	// get leafs/owner for already built flares (from SURF_AUTOFLARE)
	for (out = map.flares; out; out = out->next)
	{
		out->leaf = GL_PointInLeaf (out->origin);
		if (out->surf) out->owner = out->surf->owner;
	}

	map.numFlares += count;
	out = Hunk_Alloc (count * sizeof(gl_flare_t));
	for (i = 0; i < count; i++, data = data->next, out++)
	{
		VectorCopy (data->origin, out->origin);
		out->size = data->size;
		out->radius = data->radius;
		out->color.rgba = *((int*)data->color) | 0xFF000000;
		out->style = data->style;
		out->leaf = GL_PointInLeaf (data->origin);
		if (data->model)
			out->owner = map.models + data->model;

		out->next = map.flares;
		map.flares = out;
	}
}


//??#define MAX_ASPECT	(5.0f / 8)

static void BuildSurfFlare (surfaceCommon_t *surf, color_t *color, float intens)
{
	gl_flare_t	*f;
	surfacePlanar_t *pl;
	vec3_t	origin, c;
	float	/* aspect,*/ x, y;

	if (surf->type != SURFACE_PLANAR) return;
	pl = surf->pl;

//	aspect = (pl->maxs2[0] - pl->mins2[0]) / (pl->maxs2[1] - pl->mins2[1]);
//	if (aspect < MAX_ASPECT || aspect > 1/MAX_ASPECT) return;	// not square

	x = (pl->maxs2[0] + pl->mins2[0]) / 2;
	y = (pl->maxs2[1] + pl->mins2[1]) / 2;
	VectorScale (pl->axis[0], x, origin);
	VectorMA (origin, y, pl->axis[1], origin);
	VectorMA (origin, pl->plane.dist + 1, pl->plane.normal, origin);

	f = Hunk_Alloc (sizeof(gl_flare_t));

	VectorCopy (origin, f->origin);
	f->surf = surf;
//	f->owner = surf->owner;				// cannot do it: models not yet loaded
	f->leaf = NULL;
	f->size = sqrt (intens) * 3;
	f->radius = 0;
	f->color.rgba = color->rgba | 0xFF000000;
	f->style = 0;

	c[0] = color->c[0];
	c[1] = color->c[1];
	c[2] = color->c[2];
	NormalizeColor255 (c, c);
	f->color.c[0] = Q_ftol (c[0]);
	f->color.c[1] = Q_ftol (c[1]);
	f->color.c[2] = Q_ftol (c[2]);

	f->next = map.flares;
	map.flares = f;
	map.numFlares++;
}


static void LoadSlights (slight_t *data, int count)
{
	int		i;
	gl_slight_t *out;

	map.numSlights = count;
	map.slights = out = Hunk_Alloc (count * sizeof(gl_slight_t));
	// copy slights from map
	for (i = 0; i < count; i++, data = data->next)
	{
		VectorCopy (data->origin, out->origin);
		VectorCopy (data->color, out->color);
		out->intens = data->intens;
		out->style = data->style;
		out->type = data->type;
		out->spot = data->spot;
		VectorCopy (data->spotDir, out->spotDir);
		out->spotDot = data->spotDot;
		out->focus = data->focus;
		out->fade = data->fade;
		out->cluster = GL_PointInLeaf (data->origin)->cluster;
		if (out->cluster < 0) continue;

		out++;
	}
}


static void BuildSurfLight (surfacePlanar_t *pl, color_t *color, float area, float intens, qboolean sky)
{
	surfLight_t *sl;
	vec3_t	c;
	float	m;

	c[0] = color->c[0] / 255.0f;
	c[1] = color->c[1] / 255.0f;
	c[2] = color->c[2] / 255.0f;

	if (bspfile->sunLight && sky)
	{	// have sun -- params may be changed
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

	sl = Hunk_Alloc (sizeof(surfLight_t));
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

	pl->light = sl;
}


static void GetSurfLightCluster (void)
{
	node_t	*n;
	surfLight_t *sl;
	surfaceCommon_t **s;
	surfacePlanar_t *pl;
	int		i, j, cl;

	for (i = 0, sl = map.surfLights; i < map.numSurfLights; i++, sl = sl->next)
		sl->cluster = -2;							// uninitialized

	for (i = 0, n = map.nodes + map.numNodes; i < map.numLeafNodes - map.numNodes; i++, n++)
	{
		cl = n->cluster;
		for (j = 0, s = n->leafFaces; j < n->numLeafFaces; j++, s++)
			if ((*s)->type == SURFACE_PLANAR && !(*s)->owner)		//?? other types
			{
				pl = (*s)->pl;
				if (pl->light)
				{
					if (pl->light->cluster == -2)	// uninitialized
						pl->light->cluster = cl;
					else if (pl->light->cluster != cl)
						pl->light->cluster = -1;	// single surface in few clusters
				}
			}
	}
}


static void InitLightGrid (void)
{
	int		i;

	for (i = 0; i < 3; i++)
	{
		map.gridMins[i] = Q_ftol (floor(map.nodes[0].mins[i] / LIGHTGRID_STEP));
		map.mapGrid[i] = Q_ftol (ceil(map.nodes[0].maxs[i] / LIGHTGRID_STEP)) - map.gridMins[i];
	}
	map.numLightCells = 0;
	map.lightGridChain = CreateMemoryChain ();
	map.lightGrid = AllocChainBlock (map.lightGridChain, sizeof(lightCell_t*) * map.mapGrid[0] * map.mapGrid[1] * map.mapGrid[2]);
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


// Helper function for creating lightmaps with r_lightmap=2 (taken from Q3)
// Converts brightness to color
static void CreateSolarColor (float a, float x, float y, float *vec)
{
	float	a0, s, t;
	int		i;

	a0 = a * 5;
	i = (int) a0;
	a0 = a0 - i;	// frac part
	s = (1.0f - x) * y;
	t = (1.0f - a0 * x) * y;
	a0 = y * (1.0f - x * (1.0f - a0));
	switch (i)
	{
		case 0:
			vec[0] = y; vec[1] = a0; vec[2] = s;
			break;
		case 1:
			vec[0] = t; vec[1] = y; vec[2] = s;
			break;
		case 2:
			vec[0] = s; vec[1] = y; vec[2] = a0;
			break;
		case 3:
			vec[0] = s; vec[1] = t; vec[2] = y;
			break;
		case 4:
			vec[0] = a0; vec[1] = s; vec[2] = y;
			break;
		case 5:
			vec[0] = y; vec[1] = s; vec[2] = t;
			break;
	}
}


// Copy (and light/color scale) lightmaps from src to dst (samples*3 bytes)
static void CopyLightmap (byte *dst, byte *src, int samples)
{
	int		i;

	if (r_lightmap->integer == 2)
	{
		for (i = 0; i < samples; i++)
		{
			float vec[3], r, g, b, light;

			r = *src++; g = *src++; b = *src++;

			light = r * 0.33f + g * 0.685f * b * 0.063f;
			if (light > 255)
				light = 1;
			else
				light = 1.0f / light;

			CreateSolarColor (light, 1, 0.5, vec);
			*dst++ = vec[0] * 255;
			*dst++ = vec[1] * 255;
			*dst++ = vec[2] * 255;
		}
	}
	else
	{
		float	sat;

		sat = r_saturation->value;
		if (r_lightmap->integer == 3)
			sat = -sat;

		if (sat == 1.0f)
			memcpy (dst, src, samples * 3);
		else
		{
			for (i = 0; i < samples; i++)
			{
				float	r, g, b, light;

				// get color
				r = *src++;  g = *src++;  b = *src++;
				// change saturation
				light = (r + g + b) / 3.0;
				SATURATE(r,light,sat);
				SATURATE(g,light,sat);
				SATURATE(b,light,sat);
				// put color
				*dst++ = Q_ftol (r);  *dst++ = Q_ftol (g);  *dst++ = Q_ftol (b);
			}
		}
	}
}


static void SetNodesAlpha (void)
{
	node_t	*node, *p;
	surfaceCommon_t **psurf;
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
	int		i;
	float	min1, max1, min2, max2;
	vertex_t *v;

	// generate axis
	if (pl->plane.normal[2] == 1 || pl->plane.normal[2] == -1)
	{
		pl->axis[0][0] = pl->axis[0][2] = 0;
		pl->axis[1][1] = pl->axis[1][2] = 0;
		pl->axis[0][1] = pl->axis[1][0] = 1;
	}
	else
	{
		static vec3_t up = {0, 0, 1};

		CrossProduct (pl->plane.normal, up, pl->axis[0]);
		VectorNormalize (pl->axis[0]);
		CrossProduct (pl->plane.normal, pl->axis[0], pl->axis[1]);
	}
	// compute 2D bounds
	min1 = min2 = 999999;
	max1 = max2 = -999999;
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


/*------------------ Loading Quake2 BSP file --------------------*/


static byte *lightData;
static int lightDataSize;

static void LoadLighting2 (byte *data, int size)
{
	if (!size)
	{
		lightData = NULL;
		return;
	}

	lightData = Hunk_Alloc (size);
	lightDataSize = size;
	CopyLightmap (lightData, data, size / 3);
}


static void LoadLeafsNodes2 (dnode_t *nodes, int numNodes, dleaf_t *leafs, int numLeafs)
{
	node_t	*out;
	int		i, j, p;

	map.numNodes = numNodes;
	map.numLeafNodes = numLeafs + numNodes;
	map.nodes = out = Hunk_Alloc ((numNodes + numLeafs) * sizeof(node_t));

	// Load nodes
	for (i = 0; i < numNodes; i++, nodes++, out++)
	{
		out->isNode = true;
		out->plane = map.planes + nodes->planenum;
		// setup children[]
		for (j = 0; j < 2; j++)
		{
			p = nodes->children[j];
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

		p = leafs->cluster;
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
	int		i;

	map.models = out = Hunk_Alloc (count * sizeof(inlineModel_t));
	map.numModels = count;

	for (i = 0; i < count; i++, data++, out++)
	{
		int		j;
		surfaceCommon_t *s;

		VectorCopy (data->mins, out->mins);
		VectorCopy (data->maxs, out->maxs);
		out->radius = data->radius;
		out->headnode = data->headnode;
		// create surface list
		out->numFaces = data->numfaces;
		out->faces = Hunk_Alloc (out->numFaces * sizeof(surfaceCommon_t*));
		for (j = 0, s = &map.faces[data->firstface]; j < out->numFaces; j++, s++)
		{
			out->faces[j] = s;
			s->owner = i == 0 ? NULL : out;		// model 0 is world
		}
	}
}


static void LoadSurfaces2 (dface_t *surfs, int numSurfaces, int *surfedges, dedge_t *edges,
	dvertex_t *verts, texinfo_t *tex, cmodel_t *models, int numModels)
{
	int		i, j, optLmTexels;
	surfaceCommon_t *out;

	optLmTexels = 0;
	map.numFaces = numSurfaces;
	map.faces = out = Hunk_Alloc (numSurfaces * sizeof(*out));
	for (i = 0; i < numSurfaces; i++, surfs++, out++)
	{
		int			numTextures, numVerts, numIndexes, numTris;
		surfacePlanar_t *s;
		cmodel_t	*owner;
		vertex_t	*v;
		int			*pedge, *pindex, sflags;
		texinfo_t	*stex, *ptex;
		shader_t	*shader;
		float		mins[2], maxs[2];	// surface extents
		qboolean	needLightmap;
		vec3_t		*pverts[MAX_POLYVERTS];
		char		textures[MAX_QPATH * MAX_STAGE_TEXTURES], *pname;

		numVerts = surfs->numedges;
		/*------- build vertex list -------*/
		pedge = surfedges + surfs->firstedge;
		for (j = 0; j < numVerts; j++, pedge++)
		{
			int		idx;

			idx = *pedge;
			if (idx > 0)
				pverts[j] = &(verts + (edges+idx)->v[0])->point;
			else
				pverts[j] = &(verts + (edges-idx)->v[1])->point;
		}

		/*---- Generate shader with name and SURF_XXX flags ----*/
		stex = tex + surfs->texinfo;
		sflags = SHADER_WALL;
		if (stex->flags & SURF_ALPHA)	sflags |= SHADER_ALPHA;
		if (stex->flags & SURF_TRANS33)	sflags |= SHADER_TRANS33;
		if (stex->flags & SURF_TRANS66)	sflags |= SHADER_TRANS66;
		if (stex->flags & SURF_SKY && gl_showsky->integer != 2) sflags |= SHADER_SKY;
		if (stex->flags & SURF_FLOWING)	sflags |= SHADER_SCROLL;
		if (stex->flags & SURF_WARP)	sflags |= SHADER_TURB;
		if (stex->flags & SURF_SPECULAR) sflags |= SHADER_ENVMAP;
		if (stex->flags & SURF_DIFFUSE) sflags |= SHADER_ENVMAP2;

		// find owner model
		for (j = 0, owner = models; j < numModels; j++, owner++)
			if (i >= owner->firstface && i < owner->firstface + owner->numfaces)
				break;

		if (owner->flags & CMODEL_ALPHA)
			sflags |= SHADER_ALPHA;

		// check covered contents and update shader flags
		if (stex->flags & (SURF_TRANS33|SURF_TRANS66) && !(stex->flags &
			(SURF_SPECULAR|SURF_DIFFUSE|SURF_WARP|SURF_FLOWING)))		// && gl_autoReflect ??
		{
			vec3_t	mid, p1, p2;
			float	scale, *norm;
			trace_t	trace;
			int		headnode;
			static vec3_t v1 = {1, 1, 1}, v2 = {-1, -1, -1};

			headnode = owner->headnode;
			// find middle point
			VectorClear (mid);
			for (j = 0; j < numVerts; j++)
				VectorAdd (mid, (*pverts[j]), mid);
			scale = 1.0f / numVerts;
			VectorScale (mid, scale, mid);
			// get trace points
			norm = (map.planes + surfs->planenum)->normal;
			// vector with length 1 is not enough for non-axial surfaces
			VectorMA (mid, 2, norm, p1);
			VectorMA (mid, -2, norm, p2);
			// perform trace
			if (!surfs->side)
				trace = CM_BoxTrace (p1, p2, v1, v2, headnode, MASK_SOLID);
			else
				trace = CM_BoxTrace (p2, p1, v1, v2, headnode, MASK_SOLID);
			if (trace.fraction < 1 && !(trace.contents & CONTENTS_MIST))
				sflags |= SHADER_ENVMAP;
		}


		// check for shader lightmap
		if (surfs->lightofs >= 0)
		{
			if (sflags & SHADER_SKY)
			{
				image_t	*img;

				//!! change this code (or update comment above)
				needLightmap = false;
				if (map.sunColor[0] + map.sunColor[1] + map.sunColor[2] == 0)
				{
					img = GL_FindImage (va("textures/%s", stex->texture), IMAGE_MIPMAP|IMAGE_PICMIP);	// find sky image only for taking its color
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
		else
			needLightmap = false;

		/*---------- check for texture animation ----------*/
		ptex = stex;
		pname = textures;
		numTextures = 0;
		while (true)
		{
			int		n;

			pname += Com_sprintf (pname, MAX_QPATH, "textures/%s", ptex->texture) + 1;	// length(format)+1
			numTextures++;

			n = ptex->nexttexinfo;
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

		out->shader = shader = (sflags & SHADER_SKY) ? gl_skyShader : GL_FindShader (textures, sflags);
		//!! update sflags from this (created) shader -- it may be scripted (with different flags)

		if (sflags & (SHADER_TRANS33|SHADER_TRANS66|SHADER_ALPHA|SHADER_TURB|SHADER_SKY))
			numVerts = RemoveCollinearPoints (pverts, numVerts);

		/* numTriangles = numVerts - 2 (3 verts = 1 tri, 4 verts = 2 tri etc.)
		 * numIndexes = numTriangles * 3
		 * Indexes: (0 1 2) (0 2 3) (0 3 4) ... (here: 5 verts, 3 triangles, 9 indexes)
		 */
		if (shader->tessSize)
		{
			numTris = SubdividePlane (pverts, numVerts, shader->tessSize);
			numVerts = subdivNumVerts;
		}
		else
			numTris = numVerts - 2;

		numIndexes = numTris * 3;

		/*------- Prepare for vertex generation ----------------*/
		out->type = SURFACE_PLANAR;
		out->pl = s = Hunk_Alloc (sizeof(*s) + sizeof(vertex_t)*numVerts + sizeof(int)*numIndexes);
		memcpy (&s->plane, map.planes + surfs->planenum, sizeof(cplane_t));
		if (surfs->side)
		{
			// backface (needed for backface culling)
			VectorNegate (s->plane.normal, s->plane.normal);
			s->plane.dist = -s->plane.dist;
			//?? set signbits
			s->plane.type = PLANE_NON_AXIAL;	//?? extend plane.type for xyz[i] == -1
		}
		s->numVerts = numVerts;
		s->verts = (vertex_t *) (s+1);	//!!! allocate verts separately (for fast draw - in AGP memory)
		s->numIndexes = numIndexes;
		s->indexes = (int *) (s->verts+numVerts);

		/*-------------- Generate indexes ----------------------*/
		pindex = s->indexes;
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
		v = s->verts;
		// ClearBounds2D (mins, maxs)
		mins[0] = mins[1] = 999999;
		maxs[0] = maxs[1] = -999999;
		ClearBounds (s->mins, s->maxs);
		// Enumerate vertexes
		for (j = 0; j < numVerts; j++, pedge++, v++)
		{
			VectorCopy ((*pverts[j]), v->xyz);
			if (!(sflags & SHADER_SKY))
			{
				float	v1, v2;

				v1 = DotProduct (v->xyz, stex->vecs[0]) + stex->vecs[0][3];
				v2 = DotProduct (v->xyz, stex->vecs[1]) + stex->vecs[1][3];
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
				v->c.rgba = 0xFFFFFFFF;
			}
		}

		/*----------------- Lightmap -------------------*/
		if (needLightmap)
		{
			int		size[2];		// lightmap size
			dynamicLightmap_t *lm;

			// round mins/maxs to a lightmap cell size
			mins[0] = floor(mins[0] / 16) * 16;
			mins[1] = floor(mins[1] / 16) * 16;
			maxs[0] = ceil(maxs[0] / 16) * 16;
			maxs[1] = ceil(maxs[1] / 16) * 16;
			// calculate lightmap size
			size[0] = (maxs[0] - mins[0]) / 16 + 1;
			size[1] = (maxs[1] - mins[1]) / 16 + 1;

			v = s->verts;
			for (j = 0; j < numVerts; j++, v++)
			{
				v->lm[0] = (v->lm[0] - mins[0]) / 16 + 0.5;		// divide to lightmap unit size and shift
				v->lm[1] = (v->lm[1] - mins[1]) / 16 + 0.5;
				v->lm2[0] = v->lm[0] - 0.5;
				v->lm2[1] = v->lm[1] - 0.5;
			}
			// create dynamic lightmap
			s->lightmap = lm = Hunk_Alloc (sizeof (*lm));
			for (j = 0; j < 4; j++)			// enum styles
			{
				int		st;

				st = surfs->styles[j];
				if (st == 255) break;
				lm->style[j] = st;
				lm->modulate[j] = (j == 0 ? 128 : 0);			// initial state
				lm->source[j] = lightData + surfs->lightofs + j * size[0] * size[1] * 3;
			}
			lm->numStyles = j;
			lm->w = size[0];
			lm->h = size[1];

			// check for single-color lightmap block
			if (lm->numStyles == 1)
			{
#define MAX_DEV		4					// maximal deviation of texture color
				byte	*p;
				byte	min[3], max[3];

				p = lm->source[0];
				min[0] = max[0] = *p++;
				min[1] = max[1] = *p++;
				min[2] = max[2] = *p++;
				for (j = 1; j < lm->w * lm->h; j++, p += 3)
				{
					byte	c;
					qboolean m;

					m = false;
#define STEP(n)	\
					c = p[n];		\
					if (c < min[n])	\
					{				\
						min[n] = c;	\
						m = true;	\
					}				\
					if (c > max[n])	\
					{				\
						max[n] = c;	\
						m = true;	\
					}

					STEP(0); STEP(1); STEP(2);
#undef STEP
					if (m && ((max[0] - min[0] > MAX_DEV) || (max[1] - min[1] > MAX_DEV) || (max[2] - min[2] > MAX_DEV)))
					{
						p = NULL;
						break;
					}
				}

				if (p)
				{
					p = lm->source[0];
#if 1
//!! if no mtex+env_add/env_combine -- use 1-pixel lm; otherwise -- vertex lm
					if (1)
					{
						// replace lightmap block with a single texel
						p[0] = (max[0] + min[0]) / 2;		// use average color
						p[1] = (max[1] + min[1]) / 2;
						p[2] = (max[2] + min[2]) / 2;
						for (j = 0, v = s->verts; j < numVerts; j++, v++)
							v->lm[0] = v->lm[1] = 0.5;		// all verts should point to a middle of lightmap texel
						optLmTexels += lm->w * lm->h - 1;
						lm->w = lm->h = 1;
					}
					else
					{
						// convert to a vertex lightmap
						//!! UNFINISHED: need SPECIAL vertex lm, which will be combined with dyn. lm with ENV_ADD
						out->shader = GL_FindShader (textures, sflags | SHADER_TRYLIGHTMAP);
						optLmTexels += lm->w * lm->h;
					}
#else
					// show optimized lightmap zones
					p[0] = 192; p[1] = 32; p[2] = 32; //??
#endif
				}
			}

			//?? test
/*			for (j = 0, v = s->verts; j < numVerts; j++, v++)
				if (v->xyz[0] > -2155 && v->xyz[0] < -2150 &&
					v->xyz[1] > -355 && v->xyz[1] < -350 &&
					v->xyz[2] > 172 && v->xyz[2] < 180 && lm->numStyles == 1)
			{
				int		k, x;
				byte	*p;

				p = lm->source[0];
				Com_Printf ("surf #%d : %d x %d\n", i, lm->w, lm->h);
				for (k = 0, x = 0; k < lm->w * lm->h; k++, p += 3)
				{
					Com_Printf ("^2%d-%d-%d ", p[0], p[1], p[2]);
					if (++x == lm->w)
					{
						Com_Printf ("\n");
						x = 0;
					}
				}
				Com_Printf ("---------\n");
				p = lm->source[0];
			} */

		}
		else
			s->lightmap = NULL;

		BuildPlanarSurfAxis (s);
		if (stex->flags & SURF_LIGHT)		//!! + sky when ambient <> 0
		{
			image_t	*img;
			float	area;
			static color_t	defColor = {96, 96, 96, 255};// {255, 255, 255, 255};

			area = GetPolyArea (pverts, numVerts);
			img = GL_FindImage (va("textures/%s", stex->texture), IMAGE_MIPMAP);
			BuildSurfLight (s, img ? &img->color : &defColor, area, stex->value, stex->flags & SURF_SKY);
			if (stex->flags & SURF_AUTOFLARE && !(stex->flags & SURF_SKY))
				BuildSurfFlare (out, img ? &img->color : &defColor, area);
		}
	}
	Com_DPrintf ("removed %d lm texels (%.3f blocks)\n", optLmTexels, (float)optLmTexels / (LIGHTMAP_SIZE * LIGHTMAP_SIZE));
}


static int LightmapCompare (const void *s1, const void *s2)
{
	surfaceCommon_t *surf1, *surf2;
	byte	*v1, *v2;

	surf1 = *(surfaceCommon_t **)s1;
	surf2 = *(surfaceCommon_t **)s2;

	v1 = (surf1->pl->lightmap) ? surf1->pl->lightmap->source[0] : NULL;
	v2 = (surf2->pl->lightmap) ? surf2->pl->lightmap->source[0] : NULL;

	if (v1 == v2 && v1)
		return (surf1->shader->style & SHADER_TRYLIGHTMAP) ? 1 : -1;	// TRYLIGHTMAP should go after normal lightmaps

	return v1 - v2;
}

static int ShaderCompare (const void *s1, const void *s2)
{
	surfaceCommon_t *surf1, *surf2;

	surf1 = *(surfaceCommon_t **)s1;
	surf2 = *(surfaceCommon_t **)s2;

	return surf1->shader->sortIndex - surf2->shader->sortIndex;
}

static void GenerateLightmaps2 (void)
{
	int		i;
	byte	*ptr;
	lightmapBlock_t *bl;
	surfaceCommon_t *s;
	dynamicLightmap_t *dl;
	surfaceCommon_t *sortedSurfaces[MAX_MAP_FACES];

	/*------------ find invalid lightmaps -------------*/
	// sort surfaces by lightmap data pointer
	for (i = 0; i < map.numFaces; i++)
		sortedSurfaces[i] = &map.faces[i];
	qsort (sortedSurfaces, map.numFaces, sizeof(surfaceCommon_t*), LightmapCompare);

	// check for lightmap intersection
	// NOTE: nere we compare lm->source[0], which should always correspond to a lightstyle=0 (global lighting)
	ptr = NULL;
	for (i = 0; i < map.numFaces; i++)
	{
		qboolean	bad;
		int			lmSize, k;

		s = sortedSurfaces[i];
		dl = s->pl->lightmap;
		if (!dl) continue;

		lmSize = dl->w * dl->h * dl->numStyles * 3;
		if (dl->source[0] + lmSize > lightData + lightDataSize)
			bad = true;		// out of bsp file
		else if (ptr && ptr > dl->source[0] && (s->shader->style & SHADER_TRYLIGHTMAP))
			bad = true;		// erased by previous block
		else
			bad = false;

		//?? add check: if current is "try" interlaced with next "not try (normal)" - current is "bad"

		if (bad)
		{
			// remove lightmap info from the shader
			if (s->shader->lightmapNumber != LIGHTMAP_NONE)
			{
				s->shader->lightmapNumber = LIGHTMAP_NONE;
				// later (in this function), just don't create vertex colors for this surface
				//?? or compute light from scene ?
				Com_DPrintf ("Disable lm for %s\n", s->shader->name);
//				Com_Printf ("  diff: %d\n", ptr - lm->source);
//				Com_Printf ("  w: %d  h: %d  st: %d\n", lm->w, lm->h, lm->numStyles);
			}
		}
		else
		{	// advance pointer
			ptr = dl->source[0] + lmSize;
		}

		// after this examination. we can resort lightstyles (cannot do this earlier because used source[0])
		LM_SortLightStyles (dl);
		// set shader lightstyles
		if (s->shader->lightmapNumber >= 0)
		{
			int		styles;

			styles = 0;
			dl->w2 = dl->w;
			dl->numFastStyles = 0;
			for (k = dl->numStyles - 1; k >= 0; k--)
			{
				byte	t;

				t = dl->style[k];
				if (IS_FAST_STYLE(t))
				{
					styles = (styles << 8) + t;
					dl->w2 += dl->w;
					dl->numFastStyles++;
				}
			}
			if (styles) s->shader = GL_SetShaderLightstyles (s->shader, styles);
		}
	}

	/*-------------- vertex lighting ------------------*/
	// we should genetate lightmaps after validating of ALL lightmaps
	for (i = 0; i < map.numFaces; i++)
	{
		s = &map.faces[i];
		if (!s->pl->lightmap) continue;

		if (s->shader->lightmapNumber == LIGHTMAP_NONE)
		{	// lightmap was removed from shader - remove it from surface too
			s->pl->lightmap = NULL;
			continue;
		}

		GL_UpdateDynamicLightmap (s->shader, s->pl, true, 0);
	}


	/*----------- allocate lightmaps for surfaces -----------*/
	if (!gl_config.vertexLight)
	{
		// resort surfaces by shader
		qsort (sortedSurfaces, map.numFaces, sizeof(surfaceCommon_t*), ShaderCompare);

		LM_Init ();
		bl = NULL;
		i = 0;
		while (i < map.numFaces)
		{
			shader_t *shader;
			int		numShaderSurfs, i2, nextIndex;
			qboolean fit;

			s = sortedSurfaces[i];
			dl = s->pl->lightmap;
			if (!dl || s->shader->lightmapNumber == LIGHTMAP_VERTEX)
			{
				i++;
				continue;
			}

			shader = sortedSurfaces[i]->shader;
			// count number of surfaces with the same shader
			for (i2 = i + 1; i2 < map.numFaces; i2++)
				if (sortedSurfaces[i2]->shader != shader) break;

			numShaderSurfs = i2 - i;
			nextIndex = i2;

			// try to allocate all surfaces in a single lightmap block
			LM_Rewind ();
			fit = false;
			while (!fit)
			{
				bl = LM_NextBlock ();
				LM_Save (bl);					// save layout
				for (i2 = i; i2 < nextIndex; i2++)
				{
					fit = LM_AllocBlock (bl, sortedSurfaces[i2]->pl->lightmap);
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
				dl = sortedSurfaces[i2]->pl->lightmap;
				while (!LM_AllocBlock (bl, dl))
				{
					if (bl->empty)
						Com_Error (ERR_DROP, "LM_AllocBlock: unable to allocate lightmap %d x %d\n", dl->w, dl->h);
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
			int		j;
			vertex_t *v;
			float	s0, t0;
			surfacePlanar_t *pl;

			s = sortedSurfaces[i];
			pl = s->pl;
			dl = pl->lightmap;
			if (!dl || !dl->block) continue;

			// set lightmap for shader
			s->shader = GL_SetShaderLightmap (s->shader, dl->block->index);

			// update vertex lightmap coords
			s0 = (float)dl->s / LIGHTMAP_SIZE;
			t0 = (float)dl->t / LIGHTMAP_SIZE;
			for (j = 0, v = pl->verts; j < pl->numVerts; j++, v++)
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
	int		i;
	surfaceCommon_t **out;

	map.numLeafFaces = count;
	map.leafFaces = out = Hunk_Alloc (count * sizeof(*out));
	for (i = 0; i < count; i++, data++, out++)
		*out = map.faces + *data;
}


static void LoadVisinfo2 (dvis_t *data, int size)
{
	int		rowSize;

	map.visRowSize = rowSize = (map.numClusters + 7) >> 3;
	if (size)
	{
		int		i;
		byte	*dst;

		dst = map.visInfo = Hunk_Alloc (rowSize * map.numClusters);
		for (i = 0; i < map.numClusters; i++)
		{
			int		pos, j;

			pos = data->bitofs[i][DVIS_PVS];
			if (pos != -1)
			{
				byte	*src;

				src = (byte*)data + pos;
				// decompress vis
				j = rowSize;
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
						c = *src++;			// count
						c = min(c, j);		// should not be, but ...
						j -= c;
						while (c--)
							*dst++ = 0;
					}
				}
			}
			else
			{	// all visible
				for (j = 0; j < rowSize; j++)
					*dst++ = 0xFF;
			}
		}
	}
}


static void LoadBsp2 (bspfile_t *bsp)
{
START_PROFILE(LoadLighting2)
	LoadLighting2 (bsp->lighting, bsp->lightDataSize);
END_PROFILE
START_PROFILE(LoadPlanes)
	// Load planes
	LoadPlanes (bsp->planes, bsp->numPlanes, sizeof(dplane_t));
END_PROFILE
	// Load surfaces
START_PROFILE(LoadSurfaces2)
	LoadSurfaces2 (bsp->faces, bsp->numFaces, bsp->surfedges, bsp->edges, bsp->vertexes, bsp->texinfo, bsp->models, bsp->numModels);
END_PROFILE
START_PROFILE(LoadLeafSurfaces2)
	LoadLeafSurfaces2 (bsp->leaffaces, bsp->numLeaffaces);
END_PROFILE
START_PROFILE(GenerateLightmaps2)
	GenerateLightmaps2 ();
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


/*--------------- GL_LoadWorldMap --------------------*/

static void FreeMapData (void)
{
	if (map.hunk) Hunk_Free (map.hunk);
	if (map.lightGridChain) FreeMemoryChain (map.lightGridChain);
}

void GL_LoadWorldMap (char *name)
{
	char	name2[MAX_QPATH];

	if (!name[0])
		Com_Error (ERR_FATAL, "R_LoadWorldMap: NULL name");

	Q_CopyFilename (name2, name, sizeof(name2)-1);

	// map must be reloaded to update shaders (which are restarted every BeginRegistration())
//	if (!strcmp (name2, map.name))
//		return;		// map is not changed

	FreeMapData ();
	memset (&map, 0, sizeof(map));
	strcpy (map.name, name2);
	map.hunk = Hunk_Begin (32 * 1024*1024);

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
			map.ambientLight[0] = lmMinlight.c[0];
			map.ambientLight[1] = lmMinlight.c[1];
			map.ambientLight[2] = lmMinlight.c[2];
			Com_DPrintf ("Used minlight from lightmap {%d, %d, %d}\n", VECTOR_ARGS(lmMinlight.c));
		}
		break;
	default:
		Hunk_End ();
		Com_Error (ERR_DROP, "R_LoadWorldMap: unknown BSP type");
	}
	LoadFlares (bspfile->flares, bspfile->numFlares);
	LoadSlights (bspfile->slights, bspfile->numSlights);
	GetSurfLightCluster ();
	InitLightGrid ();

	Hunk_End ();
}


node_t *GL_PointInLeaf (vec3_t p)
{
	node_t		*node;
	cplane_t	*plane;

	if (!map.name[0] || !map.numNodes)
		Com_Error (ERR_DROP, "R_PointInLeaf: bad model");

	node = map.nodes;
	while (1)
	{
		if (!node->isNode)
			return node;

		plane = node->plane;
		if (DotProduct (p, plane->normal) - plane->dist > 0)
			node = node->children[0];
		else
			node = node->children[1];
	}

	return NULL;	// never
}


/*----------- Loading triangle models ----------*/


// Find xyz_index for each "st" (st have unique xyz, but xyz may have few st on texture seams)
// and fill "st" list.
// This function may be used for Q2 and KP models
static int ParseGlCmds (char *name, surfaceMd3_t *surf, int *cmds, int *xyzIndexes)
{
	int		count, numTris, *idx, allocatedVerts;
	int		vertsIndexes[1024];		// verts per triangle strip/fan

	numTris = 0;
	allocatedVerts = 0;
	idx = surf->indexes;
	while (count = *cmds++)
	{
		qboolean strip;
		int		i, i1, i2, i3;

		// cmds -> s1, t1, idx1, s2, t2, idx2 ...

		strip = count > 0;
		if (!strip)
			count = -count;
		numTris += count - 2;

		if (numTris > surf->numTris)
		{
			Com_WPrintf ("R_LoadMd2: %s has incorrect triangle count\n", name);
			return 0;
		}

		// generate vertexes and texcoords
		for (i = 0; i < count; i++, cmds += 3)
		{
			float	*dst, s, t;
			int		index;

			s = ((float*)cmds)[0];
			t = ((float*)cmds)[1];

			// find st in allocated vertexes
			for (index = 0, dst = surf->texCoords; index < allocatedVerts; index++, dst += 2)
				if (xyzIndexes[index] == cmds[2] && dst[0] == s && dst[1] == t) break;

			if (index == allocatedVerts)
			{	// vertex not found - allocate it
				if (allocatedVerts == surf->numVerts)
				{
					Com_WPrintf ("R_LoadMd2: %s has too much texcoords\n", name);
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
}


static void ProcessMd2Frame (vertexMd3_t *verts, dAliasFrame_t *srcFrame, md3Frame_t *dstFrame, int numVerts, int *xyzIndexes)
{
	int		i;
	vertexMd3_t *dstVerts;
	float	radius;

	ClearBounds (dstFrame->mins, dstFrame->maxs);
	for (i = 0, dstVerts = verts; i < numVerts; i++, dstVerts++)
	{
		vec3_t	p;
		dTriVertx_t *srcVert;

		srcVert = &srcFrame->verts[xyzIndexes[i]];
		// compute vertex
		p[0] = srcFrame->scale[0] * srcVert->v[0] + srcFrame->translate[0];
		p[1] = srcFrame->scale[1] * srcVert->v[1] + srcFrame->translate[1];
		p[2] = srcFrame->scale[2] * srcVert->v[2] + srcFrame->translate[2];
		// update bounding box
		AddPointToBounds (p, dstFrame->mins, dstFrame->maxs);
		// put vertex in a "short" form
		dstVerts->xyz[0] = Q_ftol (p[0] / MD3_XYZ_SCALE);
		dstVerts->xyz[1] = Q_ftol (p[1] / MD3_XYZ_SCALE);
		dstVerts->xyz[2] = Q_ftol (p[2] / MD3_XYZ_SCALE);
	}

	// compute bounding sphere
	for (i = 0; i < 3; i++)
		dstFrame->localOrigin[i] = (dstFrame->mins[i] + dstFrame->maxs[i]) / 2;
	radius = 0;
	for (i = 0, dstVerts = verts; i < numVerts; i++, dstVerts++)
	{
		vec3_t	p;
		float	tmp;

		p[0] = dstVerts->xyz[0] * MD3_XYZ_SCALE;
		p[1] = dstVerts->xyz[1] * MD3_XYZ_SCALE;
		p[2] = dstVerts->xyz[2] * MD3_XYZ_SCALE;
		VectorSubtract (p, dstFrame->localOrigin, p);
		tmp = DotProduct (p, p);	// tmp = dist(p, localOrigin) ^2
		radius = max(tmp, radius);
	}
	dstFrame->radius = sqrt (radius);
}


static void BuildMd2Normals (surfaceMd3_t *surf, int *xyzIndexes, int numXyz)
{
	int		i, j, k, *idx;
	vec3_t	normals[MD3_MAX_VERTS];	// normal per xyzIndex
	short	norm_i[MD3_MAX_VERTS];
	vertexMd3_t *verts;
	float	*dst;

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
			VectorNormalize (dst);
#if 1
			a = Q_ftol (ACOS_FUNC(dst[2]) / (M_PI * 2) * 255);
			if (dst[0])		b = Q_ftol (ATAN2_FUNC (dst[1], dst[0]) / (M_PI * 2) * 255);
#else
			a = Q_ftol (acos (dst[2]) / (M_PI * 2) * 255);
			if (dst[0])		b = Q_ftol (atan2 (dst[1], dst[0]) / (M_PI * 2) * 255);
#endif
			else			b = dst[1] > 0 ? 127 : -127;
			norm_i[j] = a | (b << 8);
		}
		// copy normals
		for (j = 0; j < surf->numVerts; j++)
			verts[j].normal = norm_i[xyzIndexes[j]];
	}
}


static void SetMd3Skin (model_t *m, surfaceMd3_t *surf, int index, char *skin)
{
	char	mName[MAX_QPATH], *mPtr, sName[MAX_QPATH], *sPtr;	// model/skin
	shader_t *shader;

	// try specified shader
	shader = GL_FindShader (skin, SHADER_CHECK|SHADER_SKIN);
	if (!shader)
	{	// try to find skin forcing model directory
		Q_CopyFilename (mName, m->name, sizeof(mName) - 1);
		mPtr = strrchr (mName, '/');
		if (mPtr)	mPtr++;			// skip '/'
		else		mPtr = mName;

		Q_CopyFilename (sName, skin, sizeof(sName) - 1);
		sPtr = strrchr (sName, '/');
		if (sPtr)	sPtr++;			// skip '/'
		else		sPtr = sName;

		strcpy (mPtr, sPtr);		// make "modelpath/skinname"
		shader = GL_FindShader (mName, SHADER_CHECK|SHADER_SKIN);
		if (!shader)
		{	// not found
			Com_DPrintf ("LoadMD2(%s): %s or %s is not found\n", m->name, skin, mName);
			if (index > 0)
				shader = surf->shaders[0];		// better than default shader
			else
				shader = gl_defaultShader;
		}
	}
	// set the shader
	surf->shaders[index] = shader;
}


#define MAX_XYZ_INDEXES		4096

static qboolean LoadMd2 (model_t *m, byte *buf, int len)
{
	dmdl_t	*hdr;
	md3Model_t *md3;
	surfaceCommon_t *cs;
	surfaceMd3_t *surf;
	int		i, numVerts, numTris;
	int		xyzIndexes[MAX_XYZ_INDEXES];
	char	*skin;

	//?? should add ri.LoadMd2 (buf, size) -- sanity check + swap bytes for big-endian machines
	hdr = (dmdl_t*)buf;
	if (hdr->version != MD2_VERSION)
	{
		Com_WPrintf ("R_LoadMd2: %s has wrong version %d\n", m->name, hdr->version);
		return false;
	}

	if (hdr->numXyz <= 0 || hdr->numTris <= 0 || hdr->numFrames <= 0)
	{
		Com_WPrintf ("R_LoadMd2: %s has incorrect triangle count\n", m->name);
		return false;
	}

	/*--- determine number of vertexes ---*/
	surf = Z_Malloc (sizeof(surfaceMd3_t) + MAX_XYZ_INDEXES * sizeof(int));
	surf->texCoords = (float*)(surf+1);
	surf->numVerts = MAX_XYZ_INDEXES;
	surf->numTris = MAX_XYZ_INDEXES - 2;
	numVerts = ParseGlCmds (m->name, surf, (int*)(buf + hdr->ofsGlcmds), xyzIndexes);
	numTris = surf->numTris;		// just computed
	Z_Free (surf);
	Com_DPrintf ("LoadMD2(%s): %d xyz  %d st  %d verts\n", m->name, hdr->numXyz, hdr->numSt, numVerts);

	/* Allocate memory:
		md3Model_t		[1]
		md3Frame_t		[numFrames]
		surfaceCommon_t [numSurfaces == 1]
		surfaceMd3_t	[numSurfaces == 1]
		float			texCoords[2*numVerts]
		int				indexes[3*numTris]
		vertexMd3_t		verts[numVerts*numFrames]
		shader_t*		shaders[numShaders == numSkins]
	 */
	md3 = Z_Malloc (sizeof(md3Model_t) + hdr->numFrames*sizeof(md3Frame_t) +
		sizeof(surfaceCommon_t) + sizeof(surfaceMd3_t) +
		numVerts*2*sizeof(float) + 3*hdr->numTris*sizeof(int)
		+ numVerts*hdr->numFrames*sizeof(vertexMd3_t) + hdr->numSkins*sizeof(shader_t*));

	/*-------- fill md3 structure --------*/
	md3->numSurfaces = 1;
	md3->numFrames = hdr->numFrames;
	md3->frames = (md3Frame_t*)(md3 + 1);

	cs = (surfaceCommon_t*)(md3->frames + md3->numFrames);
	surf = (surfaceMd3_t*)(cs + 1);
	md3->surf = cs;

	/*-------- fill surf structure -------*/
	cs->md3 = surf;
	cs->type = SURFACE_MD3;
	cs->shader = gl_defaultShader;		//?? any?
	surf->numFrames = hdr->numFrames;
	surf->numVerts = numVerts;
	surf->numTris = numTris;
	surf->numShaders = hdr->numSkins;
	surf->texCoords = (float*)(surf + 1);
	surf->indexes = (int*)(surf->texCoords + 2*surf->numVerts);
	surf->verts = (vertexMd3_t*)(surf->indexes + 3*surf->numTris);
	surf->shaders = (shader_t**)(surf->verts + surf->numVerts*surf->numFrames);

START_PROFILE(..Md2::Parse)
	/*--- build texcoords and indexes ----*/
	if (!ParseGlCmds (m->name, surf, (int*)(buf + hdr->ofsGlcmds), xyzIndexes))
	{
		Z_Free (md3);
		return false;
	}
END_PROFILE

START_PROFILE(..Md2::Frame)
	/*---- generate vertexes/normals -----*/
	for (i = 0; i < surf->numFrames; i++)
		ProcessMd2Frame (surf->verts + i * numVerts,
				(dAliasFrame_t*)(buf + hdr->ofsFrames + i * hdr->frameSize),
				md3->frames + i, numVerts, xyzIndexes);
END_PROFILE
START_PROFILE(..Md2::Normals)
	BuildMd2Normals (surf, xyzIndexes, hdr->numXyz);
END_PROFILE

START_PROFILE(..Md2::Skin)
	/*---------- load skins --------------*/
	surf->numShaders = hdr->numSkins;
	for (i = 0, skin = (char*)(buf + hdr->ofsSkins); i < surf->numShaders; i++, skin += MD2_MAX_SKINNAME)
		SetMd3Skin (m, surf, i, skin);
END_PROFILE

	m->type = MODEL_MD3;
	m->md3 = md3;
	return true;
}


shader_t *GL_FindSkin (char *name)
{
	return GL_FindShader (name, SHADER_CHECK|SHADER_SKIN);
	//?? do we need to disable mipmapping for skins ?
}


/*-------------- Sprite models  ----------------*/


static qboolean LoadSp2 (model_t *m, byte *buf, int len)
{
	dsprite_t *hdr;
	dsprframe_t *in;
	sp2Model_t *sp2;
	sp2Frame_t *out;
	int		i;

	hdr = (dsprite_t*)buf;
	if (hdr->version != SP2_VERSION)
	{
		Com_WPrintf ("R_LoadSp2: %s has wrong version %d\n", m->name, hdr->version);
		return false;
	}
	if (hdr->numframes < 0)
	{
		Com_WPrintf ("R_LoadSp2: %s has incorrect frame count %d\n", m->name, hdr->numframes);
		return false;
	}

	sp2 = Z_Malloc (sizeof(sp2Model_t) + hdr->numframes * sizeof(sp2Frame_t));
	sp2->numFrames = hdr->numframes;
	sp2->radius = 0;
	for (i = 0, in = hdr->frames, out = sp2->frames; i < hdr->numframes; i++, in++, out++)
	{
		float	s, t, radius;

		out->width = in->width;
		out->height = in->height;
		out->localOrigin[0] = in->origin_x;
		out->localOrigin[1] = in->origin_y;

		s = max (out->localOrigin[0], out->width - out->localOrigin[0]);
		t = max (out->localOrigin[1], out->height - out->localOrigin[1]);
		radius = sqrt (s * s + t * t);
		sp2->radius = max (sp2->radius, radius);

		out->shader = GL_FindShader (in->name, SHADER_CHECK|SHADER_WALL|SHADER_FORCEALPHA);
		if (!out->shader)
		{
			Com_DPrintf ("R_LoadSp2(%s): %s is not found\n", m->name, in->name);
			out->shader = gl_defaultShader;
		}
	}

	m->type = MODEL_SP2;
	m->sp2 = sp2;
	return true;
}


/*----------------------------------------------*/


static void Modellist_f (void)
{
	model_t	*m;
	int		i;
	static char *modelTypes[] = {"inl", "sp2", "md3"};	// see modelType_t

	Com_Printf ("-----type-name---------\n");
	for (i = 0, m = modelsArray; i < modelCount; i++, m++)
	{
		Com_Printf ("%-3d  %3s  %s\n", i, modelTypes[m->type], m->name);
	}
}


static void FreeModel (model_t *m)
{
	switch (m->type)
	{
	case MODEL_INLINE:
		break;		// nothing to free
	case MODEL_MD3:
		Z_Free (m->md3);
		break;
	case MODEL_SP2:
		Z_Free (m->sp2);
		break;
	default:
		Com_Error (ERR_FATAL, "R_FreeModel: unknown model type %d", m->type);
	}
}


static void FreeModels (void)
{
	int		i;

	// free non-inline models
	for (i = 0; i < modelCount; i++)
		FreeModel (&modelsArray[i]);

	memset (modelsArray, 0, sizeof(modelsArray));
	modelCount = 0;
}


void GL_ResetModels (void)
{
	FreeModels ();

	// create inline models
	if (map.name[0])
	{	// init inline models
		int		i;
		model_t	*m;
		inlineModel_t *im;

		modelCount = map.numModels;
		m = modelsArray;
		im = map.models;
		for (i = 0; i < map.numModels; i++, m++, im++)
		{	// inline model #i -> model #i (same index)
			Com_sprintf (m->name, sizeof(m->name), "*%d", i);
			m->type = MODEL_INLINE;
			m->inlineModel = im;
		}
	}
}


void GL_InitModels (void)
{
	memset (&map, 0, sizeof(map));
	GL_ResetModels ();

	Cmd_AddCommand ("modellist", Modellist_f);
}


void GL_ShutdownModels (void)
{
	FreeModels ();
	FreeMapData ();

	Cmd_RemoveCommand ("modellist");
}
