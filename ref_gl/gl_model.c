#include "gl_local.h"
#include "gl_model.h"
#include "gl_shader.h"


bspModel_t gl_worldModel;
#define map		gl_worldModel	// short alias

#define MAX_GLMODELS	1024
#define MAX_POLYVERTS	1024

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

	/*----- not found -- load model ------*/
	len = FS_LoadFile (name2, (void**)&file);
	if (!file)
	{
		modelCount--;	// free slot
		Com_DPrintf ("R_FindModel: %s not found\n", name2);
		return NULL;	// file not found
	}

	switch (LittleLong(*file))
	{
	case IDALIASHEADER:
		if (!LoadMd2 (m, (byte*)file, len)) m = NULL;
		break;
	case IDSPRITEHEADER:
		if (!LoadSp2 (m, (byte*)file, len)) m = NULL;
		break;
	default:
		// do not Com_Error() here: simply ignore unknown model formats
		Com_WPrintf ("R_FindModel: unknown ID 0x%X in %s", LittleLong (*file), name);
		m = NULL;
	}
	if (!m)
		modelCount--;	// model was not loaded - free slot back

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
static void CreateSolarColor (float a, float x, float y, float *vec)
{
	float	a0, s, t;
	int		i;

	a0 = a * 5;
	i = (int) a0;
	a0 = a0 - i;	// frac part
	s = (1.0 - x) * y;
	t = (1.0 - a0 * x) * y;
	a0 = y * (1.0 - x * (1.0 - a0));
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

			light = r * 0.33 + g * 0.685 * b * 0.063;
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

#define SATURATE(c,l,v) c = (l+0.5+(c-l)*v); if (c < 0) c = 0; else if (c > 255) c = 255;
				// get color
				r = *src++;  g = *src++;  b = *src++;
				// change saturation
				light = (r + g + b) / 3.0;
				SATURATE(r,light,sat);
				SATURATE(g,light,sat);
				SATURATE(b,light,sat);
				// put color
				*dst++ = r;  *dst++ = g;  *dst++ = b;
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


static void PostprocessSurfaces (void)
{
	surfaceCommon_t *surf;
	int		i;

	for (i = 0, surf = map.faces; i < map.numFaces; i++, surf++)
		switch (surf->type)
		{
		case SURFACE_PLANAR:
			{
				surfacePlanar_t *pl;
				int		k;
				float	min1, max1, min2, max2;
				vertex_t *v;

				// generate axis
				pl = surf->pl;
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
				for (k = 0, v = pl->verts; k < pl->numVerts; k++, v++)
				{
					float	p1, p2;

					p1 = DotProduct (v->xyz, pl->axis[0]);
					p2 = DotProduct (v->xyz, pl->axis[1]);
					v->pos[0] = p1;
					v->pos[1] = p2;
					if (p1 < min1) min1 = p1;
					if (p1 > max1) max1 = p1;
					if (p2 < min2) min2 = p2;
					if (p2 > max2) max2 = p2;
				}
				pl->mins2[0] = min1;
				pl->mins2[1] = min2;
				pl->maxs2[0] = max1;
				pl->maxs2[1] = max2;
/*				if (i < 20) //??? remove
				{
					Com_Printf ("***** surf %d *****\n", i);
					Com_Printf ("norm: %g, %g, %g\n", pl->plane.normal[0], pl->plane.normal[1], pl->plane.normal[2]);
					Com_Printf ("axis[0]: %g, %g, %g\n", pl->axis[0][0], pl->axis[0][1], pl->axis[0][2]);
					Com_Printf ("axis[1]: %g, %g, %g\n", pl->axis[1][0], pl->axis[1][1], pl->axis[1][2]);
					Com_Printf ("bounds: (%g, %g) - (%g, %g)\n", pl->mins2[0], pl->mins2[1], pl->maxs2[0], pl->maxs2[1]);
				} */
			}
			break;
		}
}


/*---------------- Dynamic lightmaps  --------------------*/

static int lightmapsNum, currentLightmapNum;
static lightmapBlock_t lmBlocks[MAX_LIGHTMAPS];

static int lmAllocSaved[LIGHTMAP_SIZE];


static void LM_Init (void)
{
	lightmapsNum = currentLightmapNum = 0;
}


static void LM_Flush (lightmapBlock_t *lm)
{
	if (!lm->empty && !lm->filled)
	{
		// have valid not stored lightmap -- upload it
		lm->image = GL_CreateImage (va("*lightmap%d", lm->index), lm->pic, LIGHTMAP_SIZE, LIGHTMAP_SIZE,
			IMAGE_CLAMP|IMAGE_LIGHTMAP);
		lm->filled = true;
	}
	if (lm->pic)
	{
		// free memory blocks
		Z_Free (lm->pic);
		Z_Free (lm->allocated);
		lm->pic = NULL;
	}
}


static void LM_Done (void)
{
	int		i;

	for (i = 0; i < lightmapsNum; i++)
		LM_Flush (&lmBlocks[i]);
}


static void LM_Save (lightmapBlock_t *lm)
{
	memcpy (lmAllocSaved, lm->allocated, sizeof(lmAllocSaved));
}


static void LM_Restore (lightmapBlock_t *lm)
{
	memcpy (lm->allocated, lmAllocSaved, sizeof(lmAllocSaved));
}


static lightmapBlock_t *LM_NewBlock (void)
{
	lightmapBlock_t *lm;

	// find free slot
	if (++lightmapsNum == MAX_LIGHTMAPS)
		Com_Error (ERR_FATAL, "LM_NewBlock: MAX_LIGHTMAPS hit");
	lm = &lmBlocks[lightmapsNum - 1];
	// init fields
	lm->index = lightmapsNum;
	lm->image = NULL;
	lm->empty = true;
	lm->filled = false;
	// alloc data blocks
	lm->pic = Z_Malloc (LIGHTMAP_SIZE * LIGHTMAP_SIZE * 4);
	lm->allocated = Z_Malloc (LIGHTMAP_SIZE * sizeof(lm->allocated[0]));
	// clear data blocks
	memset (&lm->allocated[0], 0, sizeof(lm->allocated));
	memset (lm->pic, 0, LIGHTMAP_SIZE * LIGHTMAP_SIZE * 4);

	return lm;
}


static void LM_Rewind (void)
{
	currentLightmapNum = 0;
}


static lightmapBlock_t *LM_NextBlock (void)
{
	lightmapBlock_t *lm;

	while (currentLightmapNum < lightmapsNum)
	{
		lm = &lmBlocks[currentLightmapNum++];
		if (!lm->filled)
			return lm;
	}

	return LM_NewBlock ();
}


static qboolean LM_AllocBlock (lightmapBlock_t *lm, dynamicLightmap_t *dl)
{
	int		i, j, w, h;
	int		best, best2;

	w = dl->w2 + 1;					// make a border around lightmap
	h = dl->h + 1;

	best = LIGHTMAP_SIZE;

	for (i = 0; i < LIGHTMAP_SIZE - w; i++)
	{
		best2 = 0;

		for (j = 0; j < w; j++)
		{
			if (lm->allocated[i+j] >= best)
				break;
			if (lm->allocated[i+j] > best2)
				best2 = lm->allocated[i+j];
		}
		if (j == w)
		{	// this is a valid spot
			dl->s = i;
			dl->t = best = best2;
		}
	}

	if (best + h > LIGHTMAP_SIZE)
		return false;

	j = best + h;
	for (i = 0; i < w; i++)
		lm->allocated[dl->s + i] = j;
	dl->block = lm;

	return true;
}


static void LM_PutBlock (dynamicLightmap_t *dl)
{
	byte	*dst, *src;
	int		x, y, stride, i, numFast;

	stride = (LIGHTMAP_SIZE - dl->w) * 4;

	/*------------- put main lightmap -------------*/

	dst = dl->block->pic + (dl->t * LIGHTMAP_SIZE + dl->s) * 4;
	src = dl->source[0];

	for (y = 0; y < dl->h; y++)
	{
		for (x = 0; x < dl->w; x++)
		{
			*dst++ = *src++;
			*dst++ = *src++;
			*dst++ = *src++;
			*dst++ = 0;				// alpha (no additional scale)
		}
		dst += stride;
	}

	/*----------- put dynamic lightmaps -----------*/

	numFast = 0;
	for (i = 0; i < dl->numStyles; i++)
	{
		if (!IS_FAST_STYLE(dl->style[i])) continue;

		numFast++;
		dst = dl->block->pic + (dl->t * LIGHTMAP_SIZE + dl->s + dl->w * numFast) * 4;
		src = dl->source[i];

		for (y = 0; y < dl->h; y++)
		{
			for (x = 0; x < dl->w; x++)
			{
				//!! do not "*2" if have texenv_combine (can modulate at run-time)
				*dst++ = *src++;
				*dst++ = *src++;
				*dst++ = *src++;
				*dst++ = 255;		// alpha (modulate by 2)
			}
			dst += stride;
		}
	}

//	if (dl->w * (numFast + 1) != dl->w2) Com_Printf("^1%d * 1+%d <> %d\n", dl->w, numFast, dl->w2);//!! DEBUG

	// mark lightmap block as used
	dl->block->empty = false;
}


// perform selection sort on lightstyles (mostly not needed, just in case)
static void SortLightStyles (dynamicLightmap_t *dl)
{
	int		i, j;

	if (dl->numStyles < 2) return;	// nothing to sort

	for (i = 0; i < dl->numStyles - 1; i++)	// at iteration "i == dl->numStyles-1" all already will be sorted
	{
		int		min;
		byte	*source, style;

		min = i;
		for (j = i + 1; j < dl->numStyles; j++)
			if (dl->style[j] < dl->style[i]) min = j;
		if (min == i) continue;		// in place
		// exchange styles [i] and [j]
		source = dl->source[i];
		dl->source[i] = dl->source[j];
		dl->source[j] = source;
		style = dl->style[i];
		dl->style[i] = dl->style[j];
		dl->style[j] = style;
	}
}


#define MIN_ALPHA_BRIGHT		48

void GL_UpdateDynamicLightmap (shader_t *shader, surfacePlanar_t *surf, qboolean vertexOnly, unsigned dlightMask)
{
	byte	pic[LIGHTMAP_SIZE * LIGHTMAP_SIZE * 4];
	int		x, z;
	dynamicLightmap_t *dl;

	dl = surf->lightmap;

	if (dl->block && !vertexOnly)
	{
		/*------------- update regular lightmap ---------------*/
		memset (pic, 0, dl->w * dl->h * 4);	// set initial state to zero (add up to 4 lightmaps)
		for (z = 0; z < dl->numStyles; z++)
		{
			int		scale, count;
			byte	*src, *dst;

			if (IS_FAST_STYLE(dl->style[z])) continue;
			src = dl->source[z];
			dst = pic;
			scale = dl->modulate[z] >> gl_config.overbrightBits;
			if (gl_config.lightmapOverbright) scale <<= 1;

			count = dl->w * dl->h;
			for (x = 0; x < count; x++)
			{
				// modulate lightmap: scale==0 -> by 0, scale==128 - by 1; scale==255 - by 2 (or, to exact, 2.0-1/256)
				int		r, g, b;

				r = dst[0] + (scale * *src++ >> 7);
				g = dst[1] + (scale * *src++ >> 7);
				b = dst[2] + (scale * *src++ >> 7);
				NORMALIZE_COLOR255(r, g, b);
				*dst++ = r; *dst++ = g; *dst++ = b;
				dst++;
			}
		}
		GL_Bind (dl->block->image);
		qglTexSubImage2D (GL_TEXTURE_2D, 0, dl->s, dl->t, dl->w, dl->h, GL_RGBA, GL_UNSIGNED_BYTE, pic);
		gl_speeds.numUploads++;
	}
//??	else
	{
		/*-------------- update vertex colors --------------*/
		vertex_t *v;
		int		i;

		for (i = 0, v = surf->verts; i < surf->numVerts; i++, v++)
		{
			unsigned r, g, b;			// 0 -- 0.0f, 256*16384*128 -- 256 (rang=21+8)

			r = g = b = 0;
			for (z = 0; z < dl->numStyles; z++)
			{
				byte	*point;
				unsigned scale;			// 0 -- 0.0f, 128 -- 1.0f, 255 ~ 2.0 (rang=7)
				unsigned frac_s, frac_t;// 0 -- 0.0f, 128 -- 1.0f (rang=7)
				unsigned frac;			// point frac: 0 -- 0.0f, 16384*128 -- 1.0f (rang=14+7=21)

				// calculate vertex color as weighted average of 4 points
				scale = dl->modulate[z];
				point = dl->source[z] + ((int)v->lm2[1] * dl->w + (int)v->lm2[0]) * 3;
				// calculate s/t weights
				frac_s = Q_ftol (v->lm2[0] * 128) & 127;
				frac_t = Q_ftol (v->lm2[1] * 128) & 127;
#define STEP(c,n)	c += point[n] * frac;
				frac = (128 - frac_s) * (128 - frac_t) * scale;
				STEP(r, 0); STEP(g, 1); STEP(b, 2);
				frac = frac_s * (128 - frac_t) * scale;
				STEP(r, 3); STEP(g, 4); STEP(b, 5);
				point += dl->w * 3;		// next line
				frac = (128 - frac_s) * frac_t * scale;
				STEP(r, 0); STEP(g, 1); STEP(b, 2);
				frac = frac_s * frac_t * scale;
				STEP(r, 3); STEP(g, 4); STEP(b, 5);
#undef STEP
			}
			// "float" -> int
			r >>= 21;
			g >>= 21;
			b >>= 21;
			// scale (boost) color for alpha-surfaces
			if (shader->style & (SHADER_TRANS33|SHADER_TRANS66))
			{
				int		oldbr, newbr, scale;

				oldbr = r;
				if (r < g) oldbr = g;
				if (oldbr < b) oldbr = b;
				newbr = oldbr * (256 - MIN_ALPHA_BRIGHT) + MIN_ALPHA_BRIGHT * 256;
				if (oldbr)
				{
					scale = newbr / oldbr;
					r = (r * scale) / 256;
					g = (g * scale) / 256;
					b = (b * scale) / 256;
				}
			}

			/* NOTE: cannot integrate following lines to "scale" above (scale = scale * 2 >> gl_config.overbrightBits)
			 *   because block for SURF_ALPHA (BOOST_COLOR) uses normal (not overbrighten) rgb
			 */
			r = r * 2 >> gl_config.overbrightBits;
			g = g * 2 >> gl_config.overbrightBits;
			b = b * 2 >> gl_config.overbrightBits;

			NORMALIZE_COLOR255(r, g, b);
			v->c.c[0] = r;
			v->c.c[1] = g;
			v->c.c[2] = b;
		}
		// apply vertex dlights
		if (dlightMask)
		{
			surfDlight_t *sdl;
			refDlight_t *dlight;

			sdl = surf->dlights;
			for ( ; dlightMask; dlightMask >>= 1)
			{
				float	intens2;

				if (!(dlightMask & 1)) continue;
				dlight = sdl->dlight;
				intens2 = sdl->radius * sdl->radius;

				for (i = 0, v = surf->verts; i < surf->numVerts; i++, v++)
				{
					float	f1, f2, dist2;
					int		intens, r, g, b;

					f1 = sdl->pos[0] - v->pos[0];
					f2 = sdl->pos[1] - v->pos[1];
					dist2 = f1 * f1 + f2 * f2;
					if (dist2 >= intens2) continue;			// vertex is too far from dlight

					intens = Q_ftol ((1 - dist2 / intens2) * 256);
					r = v->c.c[0] + (dlight->c.c[0] * intens >> 8);
					g = v->c.c[1] + (dlight->c.c[1] * intens >> 8);
					b = v->c.c[2] + (dlight->c.c[2] * intens >> 8);
					NORMALIZE_COLOR255(r, g, b);
					v->c.c[0] = r;
					v->c.c[1] = g;
					v->c.c[2] = b;
				}
				sdl++;
			}
		}
	}
}


/*---------------- Planar surface subdivision ----------------*/


#define POLY_DEBUG
#define SUBDIV_ERROR	0.1		// max deviation from splitting plane

typedef struct poly_s
{
	int		numIndexes;
	int		*indexes;
#ifdef POLY_DEBUG
	int		maxIndexes;
#endif
	struct poly_s *next;
} poly_t;


// verts
static int subdivNumVerts;					// number of vertexes in a new surface
static vec3_t **psubdivVerts;				// pointer to a destination "vector pointer" array
static int subdivNumVerts2;					// number of NEW vertexes
static vec3_t subdivVerts[MAX_POLYVERTS];	// place for a NEW vertexes
// polys
static void *subdivPolyChain;
static poly_t *subdivPolys;


static int Subdivide_NewVert (float x, float y, float z)
{
	int		i;
	vec3_t	*v;

//	DebugPrintf ("  NewVert(%g, %g, %g)", x, y, z);
	for (i = 0; i < subdivNumVerts; i++)
	{
		v = psubdivVerts[i];
		if ((*v)[0] == x && (*v)[1] == y && (*v)[2] == z)
		{
//			DebugPrintf ("  -- already, %d\n", i);
			return i;	// already have this vertex
		}
	}
	if (subdivNumVerts >= MAX_POLYVERTS)
		Com_Error (ERR_DROP, "SubdividePlane: MAX_POLYVERTS hit.\n");

	// alloc vertex
	v = &subdivVerts[subdivNumVerts2++];
	// "i" and "v" points to a place for new vertex
	(*v)[0] = x;
	(*v)[1] = y;
	(*v)[2] = z;
	psubdivVerts[i] = v;
	subdivNumVerts++;	// == i-1
//	DebugPrintf (" -- new, %d\n", i);
	return i;
}


static poly_t *Subdivide_NewPoly (int numVerts)
{
	poly_t	*p;

//	DebugPrintf ("NewPoly(%d)\n", numVerts);
	// alloc poly
	p = AllocChainBlock (subdivPolyChain, sizeof(poly_t) + 4 * numVerts);
	p->indexes = (int*)(p+1);
	p->numIndexes = 0;
#ifdef POLY_DEBUG
	p->maxIndexes = numVerts;
#endif
	p->next = NULL;

	return p;
}


static void Subdivide_AddPointToPoly (poly_t *poly, int index)
{
#ifdef POLY_DEBUG
	if (poly->numIndexes >= poly->maxIndexes)
		Com_Error (ERR_DROP, "Subdivide_AddPointToPoly: index error\n");
#endif
	poly->indexes[poly->numIndexes] = index;
	poly->numIndexes++;
}


static void SubdividePoly (poly_t *poly, poly_t *poly1, poly_t *poly2, int axis, float value, float delta)
{
	int		i, lastIndex, idx1, idx2;
	vec3_t	*v1, *v2;
	float	value1, value2;

//	DebugPrintf ("SubdividePoly: %d inds, axis = %d, value = %g, delta = %g\n", poly->numIndexes, axis, value, delta);
	lastIndex = poly->numIndexes - 1;
	if (lastIndex < 0)
	{
//		DebugPrintf ("...empty!\n");
		return;		// empty poly
	}

	value1 = value - delta;
	value2 = value + delta;

	idx2 = poly->indexes[0];
	v2 = psubdivVerts[idx2];
	for (i = 0; i <= lastIndex; i++)
	{
		int		side1, side2;

		// process next point
		v1 = v2;
		idx1 = idx2;
		if (i == lastIndex)
			idx2 = poly->indexes[0];
		else
			idx2 = poly->indexes[i + 1];
		v2 = psubdivVerts[idx2];

		// check point side
		if ((*v1)[axis] < value1)			side1 = 1;
		else if ((*v1)[axis] > value2)		side1 = 2;
		else								side1 = 3;
		// check next point side
		if ((*v2)[axis] < value1)			side2 = 1;
		else if ((*v2)[axis] > value2)		side2 = 2;
		else								side2 = 3;
		// add point to a corresponding poly
		if (side1 == 3 && side2 == 3)
		{
			// both points are on divider -- add point only to one poly
			if ((*v1)[axis] < (*v2)[axis])
				Subdivide_AddPointToPoly (poly1, idx1);
			else
				Subdivide_AddPointToPoly (poly2, idx1);
		}
		else
		{
			// at least one of points not on divider
			if (side1 & 1)	Subdivide_AddPointToPoly (poly1, idx1);
			if (side1 & 2)	Subdivide_AddPointToPoly (poly2, idx1);
			// if points are on the different sides -- split line
			if (!(side1 & side2))
			{
				float	frac;
				vec3_t	mid;
//				double	frac;//??

				// calculate midpoint
				frac = (value - (*v1)[axis]) / ((*v2)[axis] - (*v1)[axis]);
				VectorSubtract ((*v2), (*v1), mid);
				VectorScale (mid, frac, mid);
				VectorAdd (mid, (*v1), mid);
				idx1 = Subdivide_NewVert (mid[0], mid[1], mid[2]);
				// add it to both polys
				Subdivide_AddPointToPoly (poly1, idx1);
				Subdivide_AddPointToPoly (poly2, idx1);
			}
		}
//		DebugPrintf ("  point #%d: (%g, %g, %g), side: %d\n",
//			poly->indexes[i], (*v1)[0], (*v1)[1], (*v1)[2], side1);
	}
/*	DebugPrintf ("...OK: %d and %d inds (", poly1->numIndexes, poly2->numIndexes);
	for (i = 0; i < poly1->numIndexes; i++)
		DebugPrintf (" %d ", poly1->indexes[i]);
	DebugPrintf (") and (");
	for (i = 0; i < poly2->numIndexes; i++)
		DebugPrintf (" %d ", poly2->indexes[i]);
	DebugPrintf (")\n");
*/
}


// In: *verts[numVerts]
// Out: updated numVerts and filled verts[old_numVerts+1..new_numVerts]
// Returns number of triangles in new surface
static int SubdividePlane (vec3_t **verts, int numVerts, float tessSize)
{
	int		axis, i, numTris;
	poly_t	*poly, *firstPoly, *lastPoly;
	float	tessError;		// deviation from splitting plane

//	DebugPrintf ("SubdividePlane: %d verts, %g tessSize\n", numVerts, tessSize);
	/*------ initialization ---------*/
	psubdivVerts = verts;
	subdivNumVerts = numVerts;
	subdivNumVerts2 = 0;
	subdivPolyChain = CreateMemoryChain ();
	tessError = tessSize * SUBDIV_ERROR;

	/*---- generate initial poly ----*/
	firstPoly = poly = Subdivide_NewPoly (numVerts);
	for (i = 0; i < numVerts; i++)
		poly->indexes[i] = i;
	poly->numIndexes = numVerts;

	/*------- subdivide polys -------*/
	for (axis = 0; axis < 3; axis++)
	{
		poly = firstPoly;
		firstPoly = NULL;
		while (poly)
		{
			vec3_t	mins, maxs;
			int		numIndexes;
			float	value, min, max;
			poly_t	*workPoly, *poly1, *poly2;

//			DebugPrintf ("processing axis %d: %d indexes ...\n", axis, poly->numIndexes);
			numIndexes = poly->numIndexes;
			if (!numIndexes) continue;	// skip empty poly

			// calculate poly bounds
			ClearBounds (mins, maxs);
			for (i = 0; i < numIndexes; i++)
				AddPointToBounds (*psubdivVerts[poly->indexes[i]], mins, maxs);
//			DebugPrintf ("bounds: (%g, %g, %g) - (%g, %g, %g)\n", mins[0], mins[1], mins[2], maxs[0], maxs[1], maxs[2]);

			// mins/maxs, aligned to tessSize grid and shifted to the poly center by tessSize
			min = floor((mins[axis] + tessSize + tessError) / tessSize) * tessSize;
			max = ceil((maxs[axis] - tessSize - tessError) / tessSize) * tessSize;
//			DebugPrintf ("... stepping from %g to %g with step %g\n", min, max, tessSize);

			// shred workPoly
			workPoly = poly;
			for (value = min; value <= max; value += tessSize)
			{
				numIndexes = workPoly->numIndexes;
				// alloc new polys
				poly1 = Subdivide_NewPoly (numIndexes+1);	// subdivision of convex polygon can produce up to numVerts+1 verts
				poly2 = Subdivide_NewPoly (numIndexes+1);
				// add poly1 to chain
				if (!firstPoly)
					firstPoly = poly1;
				else
					lastPoly->next = poly1;
				lastPoly = poly1;
				// subdivide: cut workPoly to poly1 and poly2
				SubdividePoly (workPoly, poly1, poly2, axis, value, tessError);
				// switch to poly2
				workPoly = poly2;
			}
			// add workPoly to chain (rest of the splitting poly)
			if (!firstPoly)
				firstPoly = workPoly;
			else
				lastPoly->next = workPoly;
			lastPoly = workPoly;

			poly = poly->next;
			workPoly->next = NULL;	// in a case when no subdivide loop (with workPoly) performed -- keep original poly
		}
	}
	subdivPolys = firstPoly;
	// calculate number of triangles in a resulting surface
	numTris = 0;
	for (poly = firstPoly; poly; poly = poly->next)
		numTris += poly->numIndexes - 2;	// numTris = numVerts - 2
//	DebugPrintf ("SubdividePlane: OK\n--------------------------\n");

	return numTris;
}


static void FreeSubdividedPlane (void)
{
	if (subdivPolyChain)
	{
		FreeMemoryChain (subdivPolyChain);
		subdivPolyChain = NULL;
	}
}


static void GetSubdivideIndexes (int *pindex)
{
	poly_t	*poly;

	for (poly = subdivPolys; poly; poly = poly->next)
	{
		int		i;

		for (i = 0; i < poly->numIndexes - 2; i++)	// numTris
		{
			*pindex++ = poly->indexes[0];
			*pindex++ = poly->indexes[i+1];
			*pindex++ = poly->indexes[i+2];
		}
	}
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


static void LoadInlineModels2 (dmodel_t *data, int count)
{
	inlineModel_t	*out;
	int		i;

	map.models = out = Hunk_Alloc (count * sizeof(inlineModel_t));
	map.numModels = count;

	for (i = 0; i < count; i++, data++, out++)
	{
		vec3_t	tmp;
		int		j;

		VectorCopy (data->mins, out->mins);
		VectorCopy (data->maxs, out->maxs);
		VectorSubtract (out->maxs, out->mins, tmp);
		out->radius = VectorLength (tmp) / 2;
		// create surface list
		out->numFaces = data->numfaces;
		out->faces = Hunk_Alloc (out->numFaces * sizeof(surfaceCommon_t*));
		for (j = 0; j < out->numFaces; j++)
			out->faces[j] = &map.faces[data->firstface + j];
	}
}


static void LoadSurfaces2 (dface_t *surfs, int numSurfaces, int *surfedges, dedge_t *edges, dvertex_t *verts, texinfo_t *tex)
{
	int		i, j;
	surfaceCommon_t *out;

	map.numFaces = numSurfaces;
	map.faces = out = Hunk_Alloc (numSurfaces * sizeof(*out));
	for (i = 0; i < numSurfaces; i++, surfs++, out++)
	{
		int			numTextures, numVerts, numIndexes, numTris;
		surfacePlanar_t *s;
		vertex_t	*v;
		int			*pedge, *pindex, sflags;
		texinfo_t	*stex, *ptex;
		shader_t	*shader;
		float		mins[2], maxs[2];	// surface extents
		qboolean	needLightmap;
		vec3_t		*pverts[MAX_POLYVERTS];
		int			prevVert, validVerts;
		char		textures[MAX_QPATH * MAX_STAGE_TEXTURES], *pname;

		/*---- Generate shader with name and SURF_XXX flags ----*/
		stex = tex + surfs->texinfo;
		sflags = SHADER_WALL;
		if (stex->flags & SURF_ALPHA)	sflags |= SHADER_ALPHA;
		if (stex->flags & SURF_TRANS33)	sflags |= SHADER_TRANS33;
		if (stex->flags & SURF_TRANS66)	sflags |= SHADER_TRANS66;
		if (stex->flags & SURF_SKY && gl_showsky->integer != 2) sflags |= SHADER_SKY;
		if (stex->flags & SURF_FLOWING)	sflags |= SHADER_SCROLL;
		if (stex->flags & SURF_WARP)	sflags |= SHADER_TURB;

		if (surfs->lightofs >= 0)
		{
			if (sflags & SHADER_SKY)
				needLightmap = false;
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

		if (sflags & SHADER_SKY)
			shader = gl_skyShader;
		else
			shader = GL_FindShader (textures, sflags);
		out->shader = shader;
		//!! update sflags from this (created) shader -- it may be scripted (with different flags)

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

		if (sflags & (SHADER_TRANS33|SHADER_TRANS66|SHADER_ALPHA|SHADER_TURB|SHADER_SKY))
		{
			/*--------- remove collinear points ----------*/
			pverts[j] = pverts[0];			// make a loop
			prevVert = 0;
			for (j = 1; j < numVerts; j++)
			{
				vec3_t	v1, v2;

				VectorSubtract ((*pverts[prevVert]), (*pverts[j]), v1);
				VectorSubtract ((*pverts[j]), (*pverts[j+1]), v2);
				VectorNormalize (v1);
				VectorNormalize (v2);
#define CHECK_AXIS(i)	(fabs(v1[i]-v2[i]) < 0.001f)
				if (CHECK_AXIS(0) && CHECK_AXIS(1) && CHECK_AXIS(2))
				{	// remove point
					pverts[j] = NULL;
				}
				else
				{	// hold point
					prevVert = j;
				}
#undef CHECK_AXIS
			}
			/*----- compress pverts and update numVerts ------*/
			validVerts = 0;
			for (j = 0; j < numVerts; j++)
			{
				if (pverts[j])
					pverts[validVerts++] = pverts[j];
			}
			numVerts = validVerts;
		}

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
		if (!surfs->side)
		{	// backface (needed for backface culling)
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
		mins[0] = mins[1] = 99999;
		maxs[0] = maxs[1] = -99999;
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
				if (v1 < mins[0]) mins[0] = v1;
				if (v2 < mins[1]) mins[1] = v2;
				if (v1 > maxs[0]) maxs[0] = v1;
				if (v2 > maxs[1]) maxs[1] = v2;
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
		}
		else
			s->lightmap = NULL;
	}
	PostprocessSurfaces ();
}


static int LightmapCompare (const void *s1, const void *s2)
{
	surfaceCommon_t *surf1, *surf2;
	byte	*v1, *v2;

	surf1 = *(surfaceCommon_t **)s1;
	surf2 = *(surfaceCommon_t **)s2;

	if (surf1->pl->lightmap)
		v1 = surf1->pl->lightmap->source[0];
	else
		v1 = NULL;

	if (surf2->pl->lightmap)
		v2 = surf2->pl->lightmap->source[0];
	else
		v2 = NULL;

	if (v1 == v2 && v1)
	{
		if (surf1->shader->style & SHADER_TRYLIGHTMAP)
			return 1;		// should go after normal lightmaps
		else
			return -1;
	}

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
		SortLightStyles (dl);
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
						if (c > j) c = j;	// should not be, but ...
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
	LoadLighting2 (bsp->lighting, bsp->lightDataSize);
	// Load planes
	LoadPlanes (bsp->planes, bsp->numPlanes, sizeof(dplane_t));
	// Load surfaces
	LoadSurfaces2 (bsp->faces, bsp->numFaces, bsp->surfedges, bsp->edges, bsp->vertexes, bsp->texinfo);
	LoadLeafSurfaces2 (bsp->leaffaces, bsp->numLeaffaces);
	GenerateLightmaps2 ();
	// Load bsp (leafs and nodes)
	LoadLeafsNodes2 (bsp->nodes, bsp->numNodes, bsp->leafs, bsp->numLeafs);
	LoadVisinfo2 (bsp->visibility, bsp->visDataSize);
	LoadInlineModels2 (bsp->models, bsp->numModels);
}


/*--------------- GL_LoadWorldMap --------------------*/

void GL_LoadWorldMap (char *name)
{
	bspfile_t	*bsp;
	char	name2[MAX_QPATH];

	if (!name[0])
		Com_Error (ERR_FATAL, "R_FindModel: NULL name");

	Q_CopyFilename (name2, name, sizeof(name2)-1);

	// map must be reloaded to update shaders (which are restarted every BeginRegistration())
//	if (!strcmp (name2, map.name))
//		return;		// map is not changed

	if (gl_worldModel.hunk)
		Hunk_Free (map.hunk);

	memset (&map, 0, sizeof(map));
	strcpy (map.name, name2);
	map.hunk = Hunk_Begin (32 * 1024*1024);

	// map should be already loaded by client
	bsp = LoadBspFile (name2, true, NULL);
	switch (bsp->type)
	{
	case map_q2:
		LoadBsp2 (bsp);
		break;
	default:
		Hunk_End ();
		Com_Error (ERR_DROP, "R_FindModel: unknown BSP type");
	}
	Hunk_End ();
}


node_t *GL_PointInLeaf (vec3_t p)	//?? move to gl_world.c
{
	node_t		*node;
	cplane_t	*plane;

	if (!gl_worldModel.name[0] || !gl_worldModel.numNodes)
		Com_Error (ERR_DROP, "R_PointInLeaf: bad model");

	node = gl_worldModel.nodes;
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
				if (dst[0] == s && dst[1] == t && xyzIndexes[index] == cmds[2]) break;

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

		if (!idx)	// called for calculate numVerts - skip index generation
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
				if (!(i & 1))
					i1 = i3;
				else
					i2 = i3;
			}
			else
				i2 = i3;
		}
	}

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
		dstVerts->xyz[0] = (int)(p[0] * 64);	// (1.0f / MD3_XYZ_SCALE)
		dstVerts->xyz[1] = (int)(p[1] * 64);
		dstVerts->xyz[2] = (int)(p[2] * 64);
	}
	// compute bounding sphere
	for (i = 0; i < 3; i++)
		dstFrame->localOrigin[i] = (dstFrame->mins[i] + dstFrame->maxs[i]) / 2;
	radius = 0;
	for (i = 0, dstVerts = verts; i < numVerts; i++, dstVerts++)
	{
		vec3_t	p;
		float	tmp;

		p[0] = dstVerts->xyz[0] / 64.0f;
		p[1] = dstVerts->xyz[1] / 64.0f;
		p[2] = dstVerts->xyz[2] / 64.0f;
		VectorSubtract (p, dstFrame->localOrigin, p);
		tmp = DotProduct (p, p);	// tmp = dist(p, localOrigin) ^2
		if (tmp > radius)
			radius = tmp;
	}
	dstFrame->radius = sqrt (radius);
}


static void SetMd3Skin (model_t *m, surfaceMd3_t *surf, int index, char *skin)
{
	char	mName[MAX_QPATH], *mPtr, sName[MAX_QPATH], *sPtr;	// model/skin
	shader_t *shader;

	// try specified shader
	shader = GL_FindShader (skin, SHADER_CHECK|SHADER_WALL);	//?? another flags (no WALL)
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
		shader = GL_FindShader (mName, SHADER_CHECK|SHADER_WALL);	//?? other flags
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
	int		i, numVerts;
	int		xyzIndexes[MAX_XYZ_INDEXES];
	char	*skin;

	//?? should add ri.LoadMd2 (buf, size) -- sanity check + swap bytes for big-endian machines
	hdr = (dmdl_t*)buf;
	if (hdr->version != ALIAS_VERSION)
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
	surf->numTris = hdr->numTris;
	surf->numShaders = hdr->numSkins;
	surf->texCoords = (float*)(surf + 1);
	surf->indexes = (int*)(surf->texCoords + 2*surf->numVerts);
	surf->verts = (vertexMd3_t*)(surf->indexes + 3*surf->numTris);
	surf->shaders = (shader_t**)(surf->verts + surf->numVerts*surf->numFrames);

	/*--- build texcoords and indexes ----*/
	if (!ParseGlCmds (m->name, surf, (int*)(buf + hdr->ofsGlcmds), xyzIndexes))
	{
		Z_Free (md3);
		return false;
	}

	/*-------- generate vertexes ---------*/
	for (i = 0; i < surf->numFrames; i++)
		ProcessMd2Frame (surf->verts + i * numVerts,
				(dAliasFrame_t*)(buf + hdr->ofsFrames + i * hdr->frameSize),
				md3->frames + i, numVerts, xyzIndexes);

	/*---------- load skins --------------*/
	surf->numShaders = hdr->numSkins;
	for (i = 0, skin = (char*)(buf + hdr->ofsSkins); i < surf->numShaders; i++, skin += MAX_SKINNAME)
		SetMd3Skin (m, surf, i, skin);

	//!! generate light normals

	m->type = MODEL_MD3;
	m->md3 = md3;
	return true;
}


shader_t *GL_FindSkin (char *name)
{
	return GL_FindShader (name, SHADER_CHECK|SHADER_WALL);	//!! should use better flags and: better function (almost empty now)
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
	if (hdr->version != SPRITE_VERSION)
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

		s = out->width - out->localOrigin[0];
		if (s < out->localOrigin[0])
			s = out->localOrigin[0];
		t = out->height - out->localOrigin[1];
		if (t < out->localOrigin[1])
			t = out->localOrigin[1];
		radius = sqrt (s * s + t * t);
		if (radius > sp2->radius)
			sp2->radius = radius;

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


void GL_ResetModels (void)
{
	int		i;

	// free non-inline models
	for (i = 0; i < modelCount; i++)
		FreeModel (&modelsArray[i]);

	memset (modelsArray, 0, sizeof(modelsArray));
	modelCount = 0;

	// create inline models
	if (gl_worldModel.name[0])
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
	memset (&gl_worldModel, 0, sizeof(gl_worldModel));
	GL_ResetModels ();

	Cmd_AddCommand ("modellist", Modellist_f);
}


void GL_ShutdownModels (void)
{
	//?? free models and world model

	if (map.hunk) Hunk_Free (map.hunk);
	Cmd_RemoveCommand ("modellist");
}
