#include "gl_local.h"
#include "gl_frontend.h"
#include "gl_backend.h"
#include "gl_buffers.h"
#include "gl_math.h"
#include "protocol.h"		//!! for RF_XXX consts only !


static int visFrame, drawFrame;
static int currentEntity;


viewPortal_t vp;

// entities
refEntity_t gl_entities[MAX_GLENTITIES];
int			gl_numEntities;

// dlights
#define MAX_GLDLIGHTS		(MAX_DLIGHTS * 4)		// reserve more for frames with different dlight sets
refDlight_t	gl_dlights[MAX_GLDLIGHTS];
int			gl_numDlights;


GLenum		gl_fogMode;
float		gl_fogColor[4];
float		gl_fogDensity;
float		gl_fogStart, gl_fogEnd;

#define BACKFACE_EPSILON	1						// Q2: 0.01, Q3: 8


/* Culling on BSP:
 *   1. PVS
 *   2. frustum cull leafs
 *   3. backface/frustum cull leaffaces
 * Culling on inline models:
 *   1. frustum cull (entire model)
 *   2. occlusion cull (entire model)
 *   3. backface/frustum cull model surfaces
 * Culling triangle models:
 *   1. frustum cull (entire model)
 *   2. occlusion cull (entire model)
 */

/*------------------- Frustum culling -------------------*/

#define FRUSTUM_INSIDE			1
#define FRUSTUM_OUTSIDE			2
#define FRUSTUM_CENTER_OUTSIDE	4
#define FRUSTUM_ON				(FRUSTUM_INSIDE|FRUSTUM_OUTSIDE)


static int BoxCull (vec3_t mins, vec3_t maxs, int frustumMask)
{
	int		i, res;
	cplane_t *pl;

	if (!gl_frustumCull->integer)
		return FRUSTUM_ON;

	if (!frustumMask)
		return FRUSTUM_INSIDE;

	res = FRUSTUM_INSIDE;
	for (i = 0, pl = vp.frustum; i < NUM_FRUSTUM_PLANES; i++, pl++)
		if (frustumMask & (1 << i))
		{
			int		tmp;

			tmp = BoxOnPlaneSide (mins, maxs, pl);
			if (tmp == 2) return FRUSTUM_OUTSIDE;
			if (tmp == 3) res = FRUSTUM_ON;
		}
	return res;
}


// Uses entity to determine transformations. We cannot use BoxOnPlaneSide, because
// transformed bounding box is not axial.
static int TransformedBoxCull (vec3_t mins, vec3_t maxs, refEntity_t *e)
{
	int		i, res, frustumMask, mask;
	cplane_t *fr;
#if 0
	float	*v;
	vec3_t	box[8];
#endif

	if (!gl_frustumCull->integer)
		return FRUSTUM_ON;

	frustumMask = e->frustumMask;	//?? get this as OR(all_occupied_leafs->frustumMask) or remove
	if (!frustumMask)
		return FRUSTUM_INSIDE;

#if 1	// this version FASTER
	res = 0;

	for (i = 0, mask = 1, fr = vp.frustum; i < NUM_FRUSTUM_PLANES; i++, mask<<=1, fr++)
	{
		cplane_t pl;

		if (!(mask & frustumMask))
		{
			res |= FRUSTUM_INSIDE;
			continue;
		}

		// transform frustum plane to entity coordinate system
		pl.dist = fr->dist - DotProduct (e->origin, fr->normal);
		pl.normal[0] = DotProduct (fr->normal, e->axis[0]);
		pl.normal[1] = DotProduct (fr->normal, e->axis[1]);
		pl.normal[2] = DotProduct (fr->normal, e->axis[2]);
		SetPlaneSignbits (&pl);

		switch (BoxOnPlaneSide (mins, maxs, &pl))	// do not use BOX_ON_PLANE_SIDE -- useless
		{
		case 1:
			frustumMask &= ~mask;
			res |= FRUSTUM_INSIDE;
			break;
		case 2:
			return FRUSTUM_OUTSIDE;
		default:
			res = FRUSTUM_ON;
		}
	}
#else
	// transform bounding box to a world coordinates
	for (i = 0, v = box[0]; i < 8; i++, v += 3)	// check all 8 verts of bounding box
	{
		float	x, y, z;
		vec3_t	tmp;

		if (i & 1)	x = maxs[0];
		else		x = mins[0];

		if (i & 2)	y = maxs[1];
		else		y = mins[1];

		if (i & 4)	z = maxs[2];
		else		z = mins[2];

		// project point to a world coordinate system (org + x*axis[0] + y*axis[1] + z*axis[2])
		VectorMA (e->origin, x, e->axis[0], tmp);
		VectorMA (tmp,		 y, e->axis[1], tmp);
		VectorMA (tmp,		 z, e->axis[2], v);
	}

	// perform frustum culling
	res = 0;
	for (i = 0, mask = 1, fr = vp.frustum; i < NUM_FRUSTUM_PLANES; i++, mask<<=1, fr++)
	{	// loop by frustum planes
		int		j, side;

		if (!(mask & frustumMask))
		{
			res |= FRUSTUM_INSIDE;
			continue;
		}

		side = 0;
		for (j = 0, v = box[0]; j < 8; j++, v += 3)
		{	// loop by box verts
			if (DotProduct (v, fr->normal) >= fr->dist)
				side |= FRUSTUM_INSIDE;
			else
				side |= FRUSTUM_OUTSIDE;
		}

		if (side == FRUSTUM_OUTSIDE)
			return FRUSTUM_OUTSIDE;	// the box is completely outside this frustum plane
		if (side == FRUSTUM_INSIDE)
			frustumMask &= ~mask;	// the box is completely inside this frustum plane
		res |= side;
	}
#endif

	if (frustumMask & ~MAX_FRUSTUM_MASK)	// update frustum mask for entity
		e->frustumMask = frustumMask & MAX_FRUSTUM_MASK;

	return res;
}


static int SphereCull (vec3_t origin, float radius, byte *frustumMask)
{
	int		i, ret, mask, m;
	cplane_t *pl;

	if (!gl_frustumCull->integer)
	{
		if (frustumMask)
			*frustumMask = MAX_FRUSTUM_MASK;
		return FRUSTUM_ON;
	}

	// perform frustum culling
	ret = 0;
	mask = frustumMask ? *frustumMask : MAX_FRUSTUM_MASK;
	for (i = 0, pl = vp.frustum, m = 1; i < NUM_FRUSTUM_PLANES; i++, pl++, m <<= 1)
	{	// loop by frustum planes
		float	dist;

		if (!(m & mask)) continue;

		dist = DotProduct (origin, pl->normal) - pl->dist;
		if (dist < -radius)
			return FRUSTUM_OUTSIDE;
		if (dist <= radius)
		{
			ret |= FRUSTUM_OUTSIDE;
			if (dist < 0) ret |= FRUSTUM_CENTER_OUTSIDE;
		}
		else
			mask &= ~m;
	}
	if (frustumMask)
		*frustumMask = mask;
	return ret | FRUSTUM_INSIDE;
}


// Reduced SphereCull()
static int PointCull (vec3_t point, int frustumMask)
{
	int		i;
	cplane_t *pl;

	if (!gl_frustumCull->integer)
		return FRUSTUM_INSIDE;

	if (!frustumMask)
		return FRUSTUM_INSIDE;

	for (i = 0, pl = vp.frustum; i < NUM_FRUSTUM_PLANES; i++, pl++)
		if (frustumMask & (1 << i))
		{
			if (DotProduct (point, pl->normal) - pl->dist < 0)
				return FRUSTUM_OUTSIDE;
		}
	return FRUSTUM_INSIDE;
}


#define NUM_TEST_BRUSHES	2 // 32

static bool BoxOccluded (refEntity_t *e, vec3_t size2)
{
	float	mins2[2], maxs2[2];
	vec3_t	v, left, right;
	int		n, brushes[NUM_TEST_BRUSHES];
//	static cvar_t *test;

	if (!GetBoxRect (e, size2, mins2, maxs2, true)) return false;

//	if (!test) test=Cvar_Get("test","32",0);
	// top-left
	VectorMA (e->center, mins2[0], vp.viewaxis[1], left);
	VectorMA (left, mins2[1], vp.viewaxis[2], v);
	n = CM_BrushTrace (vp.vieworg, v, brushes, NUM_TEST_BRUSHES);	// test->integer);
	if (!n) return false;

	// bottom-right (diagonal with 1st point)
	VectorMA (e->center, maxs2[0], vp.viewaxis[1], right);
	VectorMA (right, maxs2[1], vp.viewaxis[2], v);
	n = CM_RefineBrushTrace (vp.vieworg, v, brushes, n);
	if (!n) return false;

	// bottom-left
	VectorMA (left, maxs2[1], vp.viewaxis[2], v);
	n = CM_RefineBrushTrace (vp.vieworg, v, brushes, n);
	if (!n) return false;

	// top-right
	VectorMA (right, mins2[1], vp.viewaxis[2], v);
	n = CM_RefineBrushTrace (vp.vieworg, v, brushes, n);
	if (!n) return false;

	return true;
}


static bool WorldBoxOccluded (vec3_t mins, vec3_t maxs)
{
	int		i;
	vec3_t	v;
	int		n, brushes[NUM_TEST_BRUSHES];
//	static cvar_t *test;

//	if (!test) test=Cvar_Get("test","32",0);
	// optimize !!: 8 -> 4 points (find contour -- fast for axial boxes); change trace order
	// in a case of fast non-occluded test: top-left, bottom-right, other 2 points
	for (i = 0; i < 8; i++)
	{
		v[0] = (i & 1) ? maxs[0] : mins[0];
		v[1] = (i & 2) ? maxs[1] : mins[1];
		v[2] = (i & 4) ? maxs[2] : mins[2];

		if (i == 0)
			n = CM_BrushTrace (vp.vieworg, v, brushes, NUM_TEST_BRUSHES);	// test->integer);
		else
			n = CM_RefineBrushTrace (vp.vieworg, v, brushes, n);
		if (!n) return false;
	}

	return true;
}


static void SetupModelMatrix (refEntity_t *e)
{
	float	matrix[4][4];
	int		i, j, k;

	//?? skip if ent->worldModel == true
	/* Matrix contents:
	 *  a00   a10   a20     x
	 *  a01   a11   a21     y
	 *  a02   a12   a22     z
	 *    0     0     0     1
	 */
	memset (matrix, 0, sizeof(matrix));
	for (i = 0; i < 3; i++)
		for (j = 0; j < 3; j++)
			matrix[i][j] = e->axis[i][j];

	if (e->mirror)
		VectorNegate (matrix[1], matrix[1]);

	matrix[3][0] = e->origin[0];
	matrix[3][1] = e->origin[1];
	matrix[3][2] = e->origin[2];
	matrix[3][3] = 1;
	// e.modelMatrix = world.modelMatrix * matrix
	for (i = 0; i < 4; i++)
		for (j = 0; j < 4; j++)
		{
			float	s;

			s = 0;
			for (k = 0; k < 4; k++)
				s += vp.modelMatrix[k][j] * matrix[i][k];
			e->modelMatrix[i][j] = s;
		}
	// set e.modelvieworg
	// NOTE: in Q3 axis may be non-normalized, so, there result is divided by length(axis[i])
	WorldToModelCoord (vp.vieworg, e, e->modelvieworg);
}


// Point trace to visible INLINE models; function based on CL_ClipMoveToEntities()
// NOTE: can easily extend to any (invisible too) inline models (add flag "visibleOnly")
static void ClipTraceToEntities (trace_t *tr, vec3_t start, vec3_t end, int brushmask)
{
	int		i;
	refEntity_t *e;
	trace_t	trace;
	float	traceLen;
	vec3_t	traceDir;

	VectorSubtract (end, start, traceDir);
	traceLen = VectorNormalizeFast (traceDir);

	for (i = 0, e = gl_entities + vp.firstEntity; i < vp.numEntities; i++, e++)
	{
		inlineModel_t *im;
		float	entPos, dist2;
		vec3_t	tmp, center2;
		static vec3_t zero = {0, 0, 0};

		if (!e->visible || !e->model || e->model->type != MODEL_INLINE)
			continue;

		im = e->model->inlineModel;
		VectorSubtract (e->center, start, center2);

		// collision detection: line vs sphere
		entPos = DotProduct (center2, traceDir);
		if (entPos < -im->radius || entPos > traceLen + im->radius)
			continue;		// too near / too far

		VectorMA (center2, -entPos, traceDir, tmp);
		dist2 = DotProduct (tmp, tmp);
		if (dist2 >= im->radius * im->radius) continue;

		// trace
		if (!e->worldMatrix)
			CM_TransformedBoxTrace2 (&trace, start, end, zero, zero, im->headnode, brushmask, e->origin, e->axis);
		else
			CM_BoxTrace (&trace, start, end, zero, zero, im->headnode, brushmask);

		if (trace.allsolid || trace.startsolid || trace.fraction < tr->fraction)
		{
			trace.ent = (struct edict_s *)e;
		 	if (tr->startsolid)
			{
				*tr = trace;
				tr->startsolid = true;
			}
			else
				*tr = trace;
		}
		else if (trace.startsolid)
			tr->startsolid = true;
		if (tr->allsolid) return;
	}
}


// returns false when cylinder is outside frustum; modifies v1 and v2
static bool CutCylinder (vec3_t v1, vec3_t v2, float radius)
{
	int		i;
	cplane_t *pl;

	for (i = 0, pl = vp.frustum; i < NUM_FRUSTUM_PLANES; i++, pl++)
	{	// loop by frustum planes
		float	dist1, dist2, frac;
		vec3_t	delta;

		dist1 = DotProduct (v1, pl->normal) - pl->dist;
		dist2 = DotProduct (v2, pl->normal) - pl->dist;
		if (dist1 < -radius && dist2 < -radius) return false;		// cylinder is outside the view frustum

		if (dist1 < -radius)
		{	// modify v1 so dist1 == -radius
			VectorSubtract (v1, v2, delta);			// v2 + delta = v1
//			frac = (dist2 - radius) / (dist2 - dist1);	-- test
			frac = (dist2 + radius) / (dist2 - dist1);
			VectorMA (v2, frac, delta, v1);
		}
		else if (dist2 < -radius)
		{	// modify v2 so dist2 == -radius
			VectorSubtract (v2, v1, delta);			// v1 + delta = v2
//			frac = (dist1 - radius) / (dist1 - dist2);	-- test
			frac = (dist1 + radius) / (dist1 - dist2);
			VectorMA (v1, frac, delta, v2);
		}
		// else - completely inside this plane
	}
	return true;
}


/*-------------- BSP object insertion ----------------*/

/* NOTE:
  - required for ents with shader->sort > OPAQUE (sort alpha-ents)
  - useful for other entity types if world occlusion culling will be
    imlemented (insert ents AFTER world culling)
*/

// Find nearest to sphere center visible leaf, occupied by entity bounding sphere
static node_t *SphereLeaf (vec3_t origin, float radius)
{
	int		sptr;
	node_t	*node;
	node_t	*stack[MAX_TREE_DEPTH];

	sptr = 0;
	node = map.nodes;
	while (1)
	{
		float	dist;

		if (!node->isNode)
		{	// leaf found
			if (node->frame == drawFrame)
				return node;			// visible => mision complete

			if (!sptr)
				return NULL;	// whole tree visited, but not leaf found (may happens when !gl_frustumCull & ent outside frustum)

			node = stack[--sptr];
			continue;
		}

		if (node->visFrame != visFrame)
		{
			if (!sptr) return NULL;
			node = stack[--sptr];
			continue;
		}

		dist = DISTANCE_TO_PLANE(origin, node->plane);
		if (dist > radius)
			node = node->children[0];	// side 1
		else if (dist < -radius)		// use mradius = -radius for speedup ??
			node = node->children[1];	// side 2
		else
		{
			// both sides -- go origin's side first
			// (what this will do: if localOrigin point is visible, return PointLeaf(org))
			if (!IsNegative (dist))
			{
				stack[sptr++] = node->children[1];
				node = node->children[0];
			}
			else
			{
				stack[sptr++] = node->children[0];
				node = node->children[1];
			}
		}
	}
}


// Find nearest (by draw order) visible leaf, occupied by entity sphere,
// or nearest to a sphere center leaf with alpha surfaces (if one)
static node_t *AlphaSphereLeaf (vec3_t origin, float radius)
{
	int		sptr, drawOrder;
	node_t	*node, *drawNode;
	node_t	*stack[MAX_TREE_DEPTH];

	sptr = 0;
	node = map.nodes;
	drawOrder = 0;
	drawNode = NULL;
	while (1)
	{
		float	dist;

		if (!node->isNode)
		{	// leaf found
			if (node->frame == drawFrame)
			{
				if (node->haveAlpha)
					return node;		// visible & alpha => mision complete
				else if (node->drawOrder > drawOrder)
				{
					drawOrder = node->drawOrder;
					drawNode = node;
				}
			}

			if (!sptr)
				return drawNode;		// whole tree visited, but no leafs with alpha found

			node = stack[--sptr];
			continue;
		}

		// process node
		if (node->visFrame != visFrame || (!node->haveAlpha && node->drawOrder < drawOrder))
		{
			if (!sptr) return drawNode;
			node = stack[--sptr];
			continue;
		}

		dist = DISTANCE_TO_PLANE(origin, node->plane);
		if (dist > radius)
			node = node->children[0];	// side 1
		else if (dist < -radius)
			node = node->children[1];	// side 2
		else
		{
			// both sides -- go origin's side first
			// (what this will do: if localOrigin point is visible, return PointLeaf(org))
			if (!IsNegative (dist))
			{
				stack[sptr++] = node->children[1];
				node = node->children[0];
			}
			else
			{
				stack[sptr++] = node->children[0];
				node = node->children[1];
			}
		}
	}
}


// Find nearest (by draw order) visible leaf, occupied by beam
static node_t *BeamLeaf (vec3_t v1, vec3_t v2)
{
	int		sptr, drawOrder;
	node_t	*node, *drawNode;
	vec3_t	v1a, v2a;
	struct {
		node_t	*node;
		vec3_t	v1, v2;
	} stack[MAX_TREE_DEPTH], *st;

	sptr = 0;
	node = map.nodes;
	drawOrder = 0;
	drawNode = NULL;
	VectorCopy (v1, v1a);
	VectorCopy (v2, v2a);

#define PUSH_NODE(n, start, end) \
	st = &stack[sptr++]; \
	st->node = n;		\
	VectorCopy (start, st->v1); \
	VectorCopy (end, st->v2);

#define POP_NODE()		\
	if (!sptr)			\
		node = NULL;	\
	else				\
	{					\
		st = &stack[--sptr]; \
		node = st->node; \
		VectorCopy (st->v1, v1a); \
		VectorCopy (st->v2, v2a); \
	}

	while (node)
	{
		float	t1, t2, frac;
		int		s1, s2, side, i;
		vec3_t	mid;

		if (node->visFrame != visFrame)
		{
			POP_NODE();
			continue;
		}
		if (!node->isNode)
		{
			// leaf found
			if (node->frame != drawFrame)
			{
				POP_NODE();
				continue;
			}

			if (node->drawOrder > drawOrder)
			{
				drawOrder = node->drawOrder;
				drawNode = node;
			}
			POP_NODE();
			continue;
		}

		// node
		t1 = DISTANCE_TO_PLANE(v1a, node->plane);
		t2 = DISTANCE_TO_PLANE(v2a, node->plane);
		s1 = IsNegative (t1); s2 = IsNegative (t2);
		if (!(s1 | s2))
			node = node->children[0];		// side 1
		else if (s1 & s2)
			node = node->children[1];		// side 2
		else
		{
			// both sides
			frac = t1 / (t1 - t2);			// t1 and t2 have different signs, so - |t1-t2| > |t1|, frac in [0..1] range
			side = t1 < t2;					// which side v1 on (child index)
			for (i = 0; i < 3; i++)
				mid[i] = v1a[i] + frac * (v2a[i] - v1a[i]);
			// Recurse(node->children[side^1],mid,v2a)  -- later
			PUSH_NODE(node->children[side^1], mid, v2a);
			// Recurse(node->children[side],v1a,mid)
			VectorCopy (mid, v2a);
			node = node->children[side];
		}
	}
#undef PUSH_NODE
#undef POP_NODE

	return drawNode;
}


/*------------------ Drawing entities --------------------*/


static void AddBspSurfaces (surfaceCommon_t **psurf, int numFaces, int frustumMask, refEntity_t *e)
{
	int		i, numDlights, j, entNum;
	refDlight_t *dl;
	float	*vieworg;

	vieworg = e->modelvieworg;

	for (i = 0; i < numFaces; i++)
	{
		surfaceCommon_t *surf;
		surfacePlanar_t *pl;

		surf = *psurf++;
		if (surf->frame == drawFrame) continue;
		surf->frame = drawFrame;
		if (surf->dlightFrame != drawFrame) surf->dlightMask = 0;

		switch (surf->type)
		{
		case SURFACE_PLANAR:
			pl = surf->pl;

			// backface culling
			if (gl_backfaceCull->integer)
			{
				float	dist;
				gl_cullMode_t cull;

				cull = surf->shader->cullMode;
#define CULL_SURF	\
	{				\
		gl_speeds.cullSurfs++;	\
		continue;	\
	}

				if (cull != CULL_NONE)
				{
					dist = DISTANCE_TO_PLANE(vieworg, &pl->plane);
					if (cull == CULL_FRONT)
					{
						if (dist < -BACKFACE_EPSILON) CULL_SURF;
					}
					else
					{
						if (dist > BACKFACE_EPSILON) CULL_SURF;
					}
				}
			}

			// frustum culling
			if (frustumMask)
			{
				if (e->worldMatrix)
				{
					if (BoxCull (pl->mins, pl->maxs, frustumMask) == FRUSTUM_OUTSIDE) CULL_SURF;
				}
				else
				{
					if (TransformedBoxCull (pl->mins, pl->maxs, e) == FRUSTUM_OUTSIDE) CULL_SURF;
				}
			}
#undef CULL_SURF

			// dlights
			numDlights = 0;
			pl->dlights = NULL;
			if (surf->dlightMask && surf->shader->lightmapNumber != LIGHTMAP_NONE)
			{
				surfDlight_t *sdl;
				unsigned mask;

				sdl = pl->dlights = (surfDlight_t*)GL_AllocDynamicMemory (sizeof(surfDlight_t) * MAX_DLIGHTS);
				if (!sdl) surf->dlightMask = 0;		// easiest way to break the loop below; speed does not matter here
				for (j = 0, mask = 1, dl = vp.dlights; j < vp.numDlights; j++, dl++, mask <<= 1)
					if (surf->dlightMask & mask)
					{
						float	org0, org1, dist, rad;
						float	*dl_org;

#define CULL_DLIGHT	\
	{				\
		surf->dlightMask &= ~mask;	\
		continue;	\
	}
						if (e->worldMatrix)
							dl_org = dl->origin;
						else
							dl_org = dl->modelOrg;
						dist = DISTANCE_TO_PLANE(dl_org, &pl->plane);
						if (!gl_dlightBacks->integer && dist < -8) CULL_DLIGHT;
						if (dist < 0) dist = -dist;

						if (dist >= dl->intensity) CULL_DLIGHT;
						rad = dl->intensity * dl->intensity - dist * dist;
						rad = SQRTFAST(rad);
						org0 = DotProduct (dl_org, pl->axis[0]);
						if (org0 < pl->mins2[0] - rad || org0 > pl->maxs2[0] + rad) CULL_DLIGHT;
						org1 = DotProduct (dl_org, pl->axis[1]);
						if (org1 < pl->mins2[1] - rad || org1 > pl->maxs2[1] + rad) CULL_DLIGHT;
						// save dlight info
						sdl->pos[0] = org0;
						sdl->pos[1] = org1;
						sdl->radius = rad;
						sdl->dlight = dl;
						sdl->axis = pl->axis;
						// next dlight
						numDlights++;
						sdl++;
					}
#undef CULL_DLIGHT
				if (pl->dlights)
					GL_ResizeDynamicMemory (pl->dlights, sizeof(surfDlight_t) * numDlights);
				if (numDlights)
				{
					gl_speeds.dlightSurfs++;
					gl_speeds.dlightVerts += numDlights * pl->numVerts;
				}
			}
			else
				surf->dlightMask = 0;
			break;
		default:
			DrawTextLeft ("unknows surface type", RGB(1, 0, 0));
			continue;
		}
		//!! apply fog
		if (surf->shader->lightmapNumber == LIGHTMAP_VERTEX) numDlights = 0;	// sort all together
		entNum = currentEntity;
		if (entNum != ENTITYNUM_WORLD && e->worldMatrix && !surf->shader->dependOnEntity)
			entNum = ENTITYNUM_WORLD;
		GL_AddSurfaceToPortal (surf, surf->shader, entNum, numDlights);
	}
}

static void AddInlineModelSurfaces (refEntity_t *e)
{
	inlineModel_t	*im;
	surfaceCommon_t	*surf, **psurf;
	int		i;
	refDlight_t *dl;
	unsigned dlightMask, mask;

	im = e->model->inlineModel;
	// check dlights
	dlightMask = 0;
	for (i = 0, dl = vp.dlights, mask = 1; i < vp.numDlights; i++, dl++, mask <<= 1)
	{
		vec3_t	tmp;
		float	dist2, dist2min;

		VectorSubtract (e->center, dl->origin, tmp);
		dist2 = DotProduct (tmp, tmp);
		dist2min = im->radius + dl->intensity;
		dist2min = dist2min * dist2min;
		if (dist2 >= dist2min) continue;	// too far

		if (!e->worldMatrix) WorldToModelCoord (dl->origin, e, dl->modelOrg);
		dlightMask |= mask;
	}
	// mark surfaces
	if (dlightMask)
		for (i = 0, psurf = im->faces; i < im->numFaces; i++, psurf++)
		{
			surf = *psurf;
			surf->dlightFrame = drawFrame;
			surf->dlightMask = dlightMask;
		}
	AddBspSurfaces (im->faces, im->numFaces, e->frustumMask, e);
}


static void AddMd3Surfaces (refEntity_t *e)
{
	md3Model_t	*md3;
	surfaceCommon_t *surf;
	int		i;

	md3 = e->model->md3;

	for (i = 0, surf = md3->surf; i < md3->numSurfaces; i++, surf++)
	{
		shader_t *shader;
		//!! fog
		// choose skin to draw
		if (e->customShader)
			shader = e->customShader;
		else
		{
			if (e->skinNum >= 0 && e->skinNum < surf->md3->numShaders)
				shader = surf->md3->shaders[e->skinNum];
			else
				shader = gl_defaultShader;
		}
		if (e->flags & RF_TRANSLUCENT)
			shader = GL_GetAlphaShader (shader);
		// draw surface
		GL_AddSurfaceToPortal (surf, shader, currentEntity, 0);
	}
}


static void AddSp2Surface (refEntity_t *e)
{
	sp2Model_t		*sp2;
	sp2Frame_t		*frame;
	surfacePoly_t	*p;
	surfaceCommon_t *surf;
	vec3_t			down, up2;
	int		color;

	sp2 = e->model->sp2;
	p = (surfacePoly_t*)GL_AllocDynamicMemory (sizeof(surfacePoly_t) + (4-1) * sizeof(vertexPoly_t));
	if (!p)		// out of dynamic memory
		return;

	frame = &sp2->frames[e->frame % sp2->numFrames];
	p->numVerts = 4;

	// setup xyz
	VectorMA (e->origin, -frame->localOrigin[1], vp.viewaxis[2], down);
	VectorMA (down, -frame->localOrigin[0], vp.viewaxis[1], p->verts[0].xyz);	// 0
	VectorMA (down, frame->width - frame->localOrigin[0], vp.viewaxis[1], p->verts[1].xyz);	// 1
	VectorScale (vp.viewaxis[2], frame->height, up2);
	VectorAdd (p->verts[0].xyz, up2, p->verts[3].xyz);	// 3
	VectorAdd (p->verts[1].xyz, up2, p->verts[2].xyz);	// 2

	// setup st
	p->verts[0].st[0] = p->verts[3].st[0] = 1;
	p->verts[1].st[0] = p->verts[2].st[0] = 0;
	p->verts[0].st[1] = p->verts[1].st[1] = 1;
	p->verts[2].st[1] = p->verts[3].st[1] = 0;
	// setup color
	color = e->shaderColor.rgba;
	if (!(e->flags & RF_TRANSLUCENT))
		color |= RGBA(0,0,0,1);						// make it non-transparent
	p->verts[0].c.rgba = p->verts[1].c.rgba = p->verts[2].c.rgba = p->verts[3].c.rgba = color;

	surf = GL_AddDynamicSurface (frame->shader, ENTITYNUM_WORLD);
	surf->poly = p;
	surf->type = SURFACE_POLY;
}


static void AddBeamSurfaces (beam_t *b)
{
	vec3_t	viewDir, tmp;
	vec3_t	axis[3];		// length, width, depth
	vec3_t	dir1, dir2;
	float	z1, z2, size, angle, angleStep;
	int		i, numParts;
	color_t	color;

	// compute level of detail
	VectorSubtract (b->drawStart, vp.vieworg, viewDir);
	z1 = DotProduct (viewDir, vp.viewaxis[0]);		// beamStart.Z
	VectorSubtract (b->drawEnd, vp.vieworg, tmp);
	z2 = DotProduct (tmp, vp.viewaxis[0]);			// beamEnd.Z
	size = min(z1, z2);

	size = b->radius * 200 / (size * vp.fov_scale);
	numParts = appRound(size);
	numParts = bound(numParts, 1, 6);

	// compute beam axis
	VectorSubtract (b->drawEnd, b->drawStart, axis[0]);
	VectorNormalizeFast (axis[0]);
	CrossProduct (axis[0], viewDir, axis[1]);
	VectorNormalizeFast (axis[1]);
	CrossProduct (axis[0], axis[1], axis[2]);		// already normalized

	VectorScale (axis[1], b->radius, dir2);
	angle = 0;
	angleStep = 0.5f / numParts;					// 0.5 -- PI/2
	color.rgba = gl_config.tbl_8to32[b->color.c[0]];
	color.c[3] = b->color.c[3];
	for (i = 0; i < numParts; i++)
	{
		surfacePoly_t *p;
		surfaceCommon_t *surf;
		float	sx, cx;

		//!! use SURF_TRISURF to allocate all parts in one surface with shared verts
		p = (surfacePoly_t*)GL_AllocDynamicMemory (sizeof(surfacePoly_t) + (4-1) * sizeof(vertexPoly_t));
		if (!p)		// out of dynamic memory
			return;
		p->numVerts = 4;

		// rotate dir1 & dir2 vectors
		VectorCopy (dir2, dir1);
		angle += angleStep;
		sx = SIN_FUNC(angle) * b->radius;
		cx = COS_FUNC(angle) * b->radius;
		VectorScale (axis[1], cx, dir2);
		VectorMA (dir2, sx, axis[2], dir2);

		// setup xyz
		VectorAdd (b->drawStart, dir1, p->verts[0].xyz);
		VectorAdd (b->drawEnd, dir1, p->verts[1].xyz);
		VectorAdd (b->drawStart, dir2, p->verts[3].xyz);
		VectorAdd (b->drawEnd, dir2, p->verts[2].xyz);

		// setup st
		p->verts[0].st[0] = p->verts[3].st[0] = 1;
		p->verts[1].st[0] = p->verts[2].st[0] = 0;
		p->verts[0].st[1] = p->verts[1].st[1] = 1;
		p->verts[2].st[1] = p->verts[3].st[1] = 0;

		p->verts[0].c.rgba = p->verts[1].c.rgba = p->verts[2].c.rgba = p->verts[3].c.rgba = color.rgba;

		//!! can be different shader to provide any types of lined particles
		surf = GL_AddDynamicSurface (gl_identityLightShader2, ENTITYNUM_WORLD);
		if (!surf) return;
		surf->poly = p;
		surf->type = SURFACE_POLY;
	}
}


#define CYLINDER_PARTS	16		// should be even number
#define CYLINDER_FIX_ALPHA
#define MIN_FIXED_ALPHA			0.2f

static void AddCylinderSurfaces (beam_t *b, shader_t *shader)
{
	vec3_t	viewDir;
	vec3_t	axis[3];		// length, width, depth
	float	len, st0, angle, anglePrev, angleStep;
	int		i;
#ifdef CYLINDER_FIX_ALPHA
	vec3_t	v;
	float	f, fixAngle;
#endif

	//?? cumpute LOD

	VectorSubtract (b->drawStart, vp.vieworg, viewDir);
	// compute beam axis
	VectorSubtract (b->drawEnd, b->drawStart, axis[0]);
	len = VectorNormalizeFast (axis[0]);
	CrossProduct (axis[0], viewDir, axis[1]);
	VectorNormalizeFast (axis[1]);
	CrossProduct (axis[0], axis[1], axis[2]);		// already normalized

	st0 = VectorDistance (b->drawEnd, b->end);

#ifdef CYLINDER_FIX_ALPHA
	// compute minimal distance to beam
	f = DotProduct (viewDir, axis[0]);
	VectorMA (b->drawStart, -f, axis[0], v);		// v is a nearest point on beam line
	VectorSubtract (v, vp.vieworg, v);
	f = DotProduct (v, v);
	f = SQRTFAST(f);								// distance to line
	if (f <= b->radius)
		fixAngle = -1;
	else
	{
		// here: f > beamRadius (should be > 0)
		f = b->radius / f;
		fixAngle = ASIN_FUNC(f) / (2 * M_PI);
	}

#endif

	for (i = 0; i < CYLINDER_PARTS; i++)
	{
		surfacePoly_t *p;
		surfaceCommon_t *surf;
		float	sx, cx;
		vec3_t	dir1, dir2;

		// cylinder will be drawn with 2 passes, from far to near
		// (for correct image when used shader with depthwrites)
		switch (i)
		{
		case 0:										// starting
			angle = -0.25f;
			angleStep = 1.0f / CYLINDER_PARTS;
			break;
		case CYLINDER_PARTS/2:
			angle = 0.75f;
			angleStep = -1.0f / CYLINDER_PARTS;
			break;
		}

		p = (surfacePoly_t*)GL_AllocDynamicMemory (sizeof(surfacePoly_t) + (4-1) * sizeof(vertexPoly_t));
		if (!p)		// out of dynamic memory
			return;
		p->numVerts = 4;

		// rotate dir1 & dir2 vectors
		anglePrev = angle;
		if (i == CYLINDER_PARTS/2-1 || i == CYLINDER_PARTS-1)
			angle = 0.25f;
		else
			angle += angleStep;

		sx = SIN_FUNC(anglePrev) * b->radius;
		cx = COS_FUNC(anglePrev) * b->radius;
		VectorScale (axis[1], cx, dir1);
		VectorMA (dir1, sx, axis[2], dir1);

		sx = SIN_FUNC(angle) * b->radius;
		cx = COS_FUNC(angle) * b->radius;
		VectorScale (axis[1], cx, dir2);
		VectorMA (dir2, sx, axis[2], dir2);

		// setup xyz
		VectorAdd (b->drawStart, dir1, p->verts[0].xyz);
		VectorAdd (b->drawEnd, dir1, p->verts[1].xyz);
		VectorAdd (b->drawStart, dir2, p->verts[3].xyz);
		VectorAdd (b->drawEnd, dir2, p->verts[2].xyz);

		p->verts[0].st[1] = p->verts[3].st[1] = len + st0;
		p->verts[1].st[1] = p->verts[2].st[1] = st0;
		p->verts[0].st[0] = p->verts[1].st[0] = anglePrev;
		p->verts[2].st[0] = p->verts[3].st[0] = angle;

		p->verts[0].c.rgba = p->verts[1].c.rgba = p->verts[2].c.rgba = p->verts[3].c.rgba = b->color.rgba;
#ifdef CYLINDER_FIX_ALPHA
		// fix alpha: make it depends on angle (better image quality)
		if (fixAngle >= 0)
		{
			float	a1, a2;

			if (i < CYLINDER_PARTS / 2)
			{	// angles in -0.25..0.25 range
				a1 = fabs (anglePrev - fixAngle);
				a2 = fabs (angle - fixAngle);
			}
			else
			{	// angles in 0.25..0.72 range
				a1 = fabs (anglePrev - 0.5f + fixAngle);
				a2 = fabs (angle - 0.5f + fixAngle);
			}
			if (a1 > 0.25f) a1 = 0.25f;
			if (a2 > 0.25f) a2 = 0.25f;
			a1 = a1 * 4 * (1 - MIN_FIXED_ALPHA) + MIN_FIXED_ALPHA;
			a2 = a2 * 4 * (1 - MIN_FIXED_ALPHA) + MIN_FIXED_ALPHA;

			p->verts[0].c.c[3] = p->verts[1].c.c[3] = appRound (b->color.c[3] * a1);
			p->verts[2].c.c[3] = p->verts[3].c.c[3] = appRound (b->color.c[3] * a2);
		}
#endif

		surf = GL_AddDynamicSurface (shader, ENTITYNUM_WORLD);
		if (!surf) return;
		surf->poly = p;
		surf->type = SURFACE_POLY;
	}
}


#define BEAM_PARTS	3		// 1 - flat

static void AddFlatBeam (beam_t *b, shader_t *shader)
{
	vec3_t	viewDir;
	vec3_t	axis[3];		// length, width, depth
	float	len, st0, angle;
	int		i;

	VectorSubtract (b->drawStart, vp.vieworg, viewDir);
	// compute beam axis
	VectorSubtract (b->drawEnd, b->drawStart, axis[0]);
	len = VectorNormalizeFast (axis[0]);
	CrossProduct (axis[0], viewDir, axis[1]);
	VectorNormalizeFast (axis[1]);
	CrossProduct (axis[0], axis[1], axis[2]);		// already normalized

	st0 = VectorDistance (b->drawEnd, b->end);

	angle = 0;
	for (i = 0; i < BEAM_PARTS; i++)
	{
		surfacePoly_t *p;
		surfaceCommon_t *surf;
		float	sx, cx;
		vec3_t	dir1, dir2;

		p = (surfacePoly_t*)GL_AllocDynamicMemory (sizeof(surfacePoly_t) + (4-1) * sizeof(vertexPoly_t));
		if (!p)		// out of dynamic memory
			return;
		p->numVerts = 4;

		sx = SIN_FUNC(angle) * b->radius;
		cx = COS_FUNC(angle) * b->radius;
		VectorScale (axis[1], cx, dir1);
		VectorMA (dir1, sx, axis[2], dir1);
		VectorNegate (dir1, dir2);

		angle += 0.5f / BEAM_PARTS;

		// setup xyz
		VectorAdd (b->drawStart, dir1, p->verts[0].xyz);
		VectorAdd (b->drawEnd, dir1, p->verts[1].xyz);
		VectorAdd (b->drawStart, dir2, p->verts[3].xyz);
		VectorAdd (b->drawEnd, dir2, p->verts[2].xyz);

		p->verts[0].st[1] = p->verts[3].st[1] = len + st0;
		p->verts[1].st[1] = p->verts[2].st[1] = st0;
		p->verts[0].st[0] = p->verts[1].st[0] = 0;
		p->verts[2].st[0] = p->verts[3].st[0] = 1;

		p->verts[0].c.rgba = p->verts[1].c.rgba = p->verts[2].c.rgba = p->verts[3].c.rgba = b->color.rgba;

		surf = GL_AddDynamicSurface (shader, ENTITYNUM_WORLD);
		if (!surf) return;
		surf->poly = p;
		surf->type = SURFACE_POLY;
	}
}


/*------------------ Drawing world -------------------*/


static node_t *WalkBspTree (void)
{
	int		sptr, frustumMask, drawOrder;
	unsigned dlightMask;
	node_t	*node, *firstLeaf, *lastLeaf;

	struct {
		node_t	*node;
		int		frustumMask;
		unsigned dlightMask;
	} stack[MAX_TREE_DEPTH], *st;

	sptr = 0;
	node = map.nodes;
	frustumMask = MAX_FRUSTUM_MASK;		// check all frustum planes
	dlightMask = vp.numDlights >= 32 ? 0xFFFFFFFF : (1 << vp.numDlights) - 1;
	firstLeaf = NULL;
	drawOrder = 0;
	lastLeaf = node;		// just in case

#define PUSH_NODE(n,dl)		\
	st = &stack[sptr++];	\
	st->node = n;			\
	st->frustumMask = frustumMask;	\
	st->dlightMask = dl;

	// if sptr == 0 -- whole tree visited
#define POP_NODE()			\
	if (!sptr)				\
		node = NULL;		\
	else					\
	{						\
		st = &stack[--sptr];			\
		node = st->node;				\
		frustumMask = st->frustumMask;	\
		dlightMask = st->dlightMask;	\
	}

	while (node)			// when whole tree visited - node = NULL
	{
		if (node->visFrame != visFrame)
		{	// discard node/leaf using visinfo
			POP_NODE();
			continue;
		}

		if (gl_frustumCull->integer)
		{
			/*-------- frustum culling ----------*/
#define CULL_NODE(bit)	\
			if (frustumMask & (1<<bit))	\
			{							\
				switch (BoxOnPlaneSide(node->mins, node->maxs, &vp.frustum[bit])) {	\
				case 1:					\
					frustumMask &= ~(1<<bit);	\
					break;				\
				case 2:					\
					POP_NODE();			\
					continue;			\
				}						\
			}

			CULL_NODE(0);
			CULL_NODE(1);
			CULL_NODE(2);
			CULL_NODE(3);
			CULL_NODE(4);
#undef CULL_NODE
		}

#if 0
		if (gl_oCull->integer >= 2 && node->isNode)	// && sptr >= 2 && sptr <= 5)
		{
			static int tests;
			static int frm;
			bool	occl;

			/* NOTES:
			 *   1. 1st version mostly faster (sometimes reverse !)
			 *   2. its faster to cull every node, than cull every leaf
			 */
			if (gl_oCull->integer == 2)
				occl = WorldBoxOccluded (node->mins, node->maxs);
			else
			{	// >= 3
				static refEntity_t ent;		// just zeroed entity
				vec3_t	v, h;

				VectorAdd (node->mins, node->maxs, v);		//?? can pre-compute on map loading
				VectorScale (v, 0.5f, ent.center);
				VectorSubtract (node->maxs, ent.center, ent.size2);
				ent.worldMatrix = true;
				occl = BoxOccluded (&ent, ent.size2);
				if (occl && gl_oCull->integer == 4)
				{
					float	mins2[2], maxs2[2];

					GetBoxRect (&ent, ent.size2, mins2, maxs2, true);
					VectorMA (ent.center, mins2[0], vp.viewaxis[1], h);
					VectorMA (h, mins2[1], vp.viewaxis[2], v);
#if 1
					DrawText3D(v, "*", RGB(0.2,0.6,0.1));
					VectorMA (h, maxs2[1], vp.viewaxis[2], v);
					DrawText3D(v, "*", RGB(0.2,0.6,0.1));
					VectorMA (ent.center, maxs2[0], vp.viewaxis[1], h);
					VectorMA (h, mins2[1], vp.viewaxis[2], v);
					DrawText3D(v, "*", RGB(0.2,0.6,0.1));
					VectorMA (h, maxs2[1], vp.viewaxis[2], v);
					DrawText3D(v, "*", RGB(0.2,0.6,0.1));
#else
					DrawText3D(v, va("(%d: %gx%gx%g)", sptr, ent.maxs[0]*2,ent.maxs[1]*2,ent.maxs[2]*2), RGB(0.3,0.9,0.2));
#endif
				}
			}
			if (occl)
			{
				tests++;
				POP_NODE();
				continue;
			}
			// stats
			if (frm != drawFrame)
			{
				DrawTextLeft (va("occl: %d", tests),RGB(1,1,1));
				frm = drawFrame;
				tests = 0;
			}
		}
#endif

		node->drawOrder = drawOrder++;
		if (node->isNode)
		{
			unsigned dlight0, dlight1;

			// check dlights
			dlight0 = dlight1 = 0;
			if (dlightMask)
			{
				unsigned i, mask;
				refDlight_t *dl;
				float	d;

				for (i = 0, dl = vp.dlights, mask = 1; i < MAX_DLIGHTS; i++, dl++, mask <<= 1)
				{
					if (!(dlightMask & mask)) continue;
					d = DISTANCE_TO_PLANE(dl->origin, node->plane);
					if (d > -dl->intensity) dlight0 |= mask;
					if (d < dl->intensity) dlight1 |= mask;
				}
			}

			if (DISTANCE_TO_PLANE(vp.vieworg, node->plane) < 0)
			{
				// ch[0], then ch[1]
				PUSH_NODE(node->children[1], dlight1);
				node = node->children[0];
				dlightMask = dlight0;
				continue;
			}
			else
			{
				// ch[1], then ch[0]
				PUSH_NODE(node->children[0], dlight0);
				node = node->children[1];
				dlightMask = dlight1;
				continue;
			}
		}


		// node is leaf
		if (!firstLeaf)
			firstLeaf = node;
		else
			lastLeaf->drawNext = node;
		lastLeaf = node;
		gl_speeds.frustLeafs++;

		node->frame = drawFrame;
		node->frustumMask = frustumMask;
		node->drawEntity = NULL;
		node->drawParticle = NULL;
		node->drawBeam = NULL;
		// mark dlights on leaf surfaces
		if (dlightMask)
		{
			int		i;
			surfaceCommon_t **psurf, *surf;

			for (i = 0, psurf = node->leafFaces; i < node->numLeafFaces; i++, psurf++)
			{
				surf = *psurf;
				if (surf->dlightFrame == drawFrame)
					surf->dlightMask |= dlightMask;
				else
				{
					surf->dlightMask = dlightMask;
					surf->dlightFrame = drawFrame;
				}
			}
		}

		POP_NODE();
	}

	// mark end of draw sequence
	lastLeaf->drawNext = NULL;

#undef PUSH_NODE
#undef POP_NODE
	return firstLeaf;
}


static void DrawEntities (int firstEntity, int numEntities)
{
	int		i, ret;
	refEntity_t	*e;
	node_t	*leaf;
	vec3_t	delta;
	float	dist2;

	for (i = 0, e = gl_entities + firstEntity; i < numEntities; i++, e++)
	{
		e->visible = false;
#define CULL_ENT	\
	{				\
		gl_speeds.cullEnts++;	\
		continue;	\
	}
#define CULL_ENT2	\
	{				\
		gl_speeds.cullEntsBox++;	\
		continue;	\
	}
		if (e->model)
		{
			if (!r_drawentities->integer) continue;		// do not draw entities with model in this mode

			switch (e->model->type)
			{
			case MODEL_UNKNOWN:
//??				if (developer->integer) -- cvar is not in renderer
					DrawText3D (e->origin, va("no model: %s", e->model->name), RGB(1,0,0));
				break;
			case MODEL_INLINE:
				{
					inlineModel_t *im;

					im = e->model->inlineModel;
					e->frustumMask = 0xFF;		// for updating
					ret = SphereCull (e->center, e->radius, &e->frustumMask);
					// try to cull bounding sphere (faster than box cull)
					if (ret == FRUSTUM_OUTSIDE) CULL_ENT;
					// frustum culling (entire model)
					if (ret & FRUSTUM_CENTER_OUTSIDE)
						if (TransformedBoxCull (im->mins, im->maxs, e) == FRUSTUM_OUTSIDE) CULL_ENT2;

					leaf = SphereLeaf (e->center, e->radius);
				}
				break;
			case MODEL_MD3:
				// frustum culling
				if (SphereCull (e->center, e->radius, NULL) == FRUSTUM_OUTSIDE) CULL_ENT;
				leaf = SphereLeaf (e->center, e->radius);
				break;
			case MODEL_SP2:
				leaf = AlphaSphereLeaf (e->origin, e->radius);
				break;
			default:
				DrawTextLeft (va("DrawEntities: bad model type %d", e->model->type), RGB(1, 0, 0));
				continue;
			}
		}
		else if (e->flags & RF_BBOX)
		{
			surfaceCommon_t *surf;

			// just add surface and continue
			surf = GL_AddDynamicSurface (gl_entityShader, ENTITYNUM_WORLD);
			if (surf)
			{
				surf->type = SURFACE_ENTITY;
				surf->ent = e;
			}
			continue;
		}
		else
		{
			DrawText3D (e->origin, va("* bad ent %d: f=%X", i, e->flags), RGB(1,0,0));
			continue;
		}

		if (!leaf) CULL_ENT;			// entity do not occupy any visible leafs

		// occlusion culling
		if (e->model && gl_oCull->integer && !(e->flags & RF_DEPTHHACK) &&
			(e->model->type == MODEL_INLINE || e->model->type == MODEL_MD3))
			//?? sprites too (depend on size/dist; use 4 its points to cull)
		{
			if (BoxOccluded (e, e->size2))
			{
				if (gl_labels->integer == 2)
					DrawText3D (e->center, va("occluded\n%s", e->model->name), RGB(0.1,0.2,0.4));
				gl_speeds.ocullEnts++;
				continue;
			}
		}

		if (e->model && gl_labels->integer)
		{
			switch (e->model->type)
			{
			case MODEL_INLINE:
				DrawText3D (e->center, va("origin: %g %g %g\nbmodel: %s\nflags: $%X",
					VECTOR_ARG(e->origin), e->model->name, e->flags), RGB(0.1,0.4,0.2));
				break;
			case MODEL_MD3:
				DrawText3D (e->center, va("origin: %g %g %g\nmd3: %s\nskin: %s\nflags: $%X",
					VECTOR_ARG(e->origin), e->model->name, e->customShader ? e->customShader->name : "(default)", e->flags), RGB(0.1,0.4,0.2));
				break;
//			default:
//				DrawText3D (e->center, va("origin: %g %g %g\nmodel: %s",
//					VECTOR_ARG(e->origin), e->model->name), RGB(0.4,0.1,0.2));
			}
		}

		e->visible = true;
		// calc model distance
		VectorSubtract (e->center, vp.vieworg, delta);
		dist2 = e->dist2 = DotProduct (delta, vp.viewaxis[0]);
		// add entity to leaf's entity list
		if (leaf->drawEntity)
		{
			refEntity_t *prev, *e1;

			for (e1 = leaf->drawEntity, prev = NULL; e1; prev = e1, e1 = e1->drawNext)
				if (dist2 > e1->dist2)
				{
					e->drawNext = e1;
					if (prev)	prev->drawNext = e;
					else     	leaf->drawEntity = e;	// insert first
					break;
				}
			if (!e1)	// insert last
			{
				prev->drawNext = e;
				e->drawNext = NULL;
			}
		}
		else
		{	// first entity for this leaf
			leaf->drawEntity = e;
			e->drawNext = NULL;
		}

		if (e->model) SetupModelMatrix (e);
	}
#undef CULL_ENT
#undef CULL_ENT2
}


static void DrawParticles (void)
{
	particle_t *p;
	beam_t	*b;
	node_t	*leaf;

	for (p = vp.particles; p; p = p->next)
	{
		leaf = &map.nodes[p->leafNum + map.numNodes];
		if (leaf->frame == drawFrame)
		{
			p->drawNext = leaf->drawParticle;
			leaf->drawParticle = p;
		}
		else
			gl_speeds.cullParts++;

		gl_speeds.parts++;
	}

	for (b = vp.beams; b; b = b->next)
	{
		VectorCopy (b->start, b->drawStart);
		VectorCopy (b->end, b->drawEnd);
		if (!CutCylinder (b->drawStart, b->drawEnd, b->radius))
		{
			gl_speeds.cullParts++;
			continue;
		}
		else
		{
			vec3_t	center;

			VectorAdd (b->drawStart, b->drawEnd, center);
			VectorScale (center, 0.5f, center);
			leaf = BeamLeaf (b->drawStart, b->drawEnd);
		}

		if (leaf)
		{
			b->drawNext = leaf->drawBeam;
			leaf->drawBeam = b;
		}
		else
			gl_speeds.cullParts++;
		gl_speeds.parts++;
	}
}


#define FLARE_DIST0		40
#define FLARE_DIST1		256
#define FLARE_DIST2		768		// Cvar_VariableValue("flare")
#define FLARE_FADE		0.2		// Cvar_VariableValue("flare")
#define SUN_DRAWDIST	32

static void DrawFlares (void)
{
	gl_flare_t	*f;

	for (f = map.flares; f ; f = f->next)
	{
		bool	cull;
		surfaceCommon_t *surf;
		surfacePoly_t *p;
		float	scale;
		vec3_t	tmp, flarePos;
		color_t	color;
		int		style;

		cull = false;

		if (f->radius >= 0)
		{
			inlineModel_t *im;

			if (f->surf && f->surf->frame != drawFrame) cull = true;
			VectorCopy (f->origin, flarePos);
			if (im = f->owner)
			{	// flare linked to entity - shift it with entity origin
				vec3_t	tmp;
				refEntity_t *e;
				int		i;

				// check entity visibility (can add entity origin only when it is valid for current frame)
				if (!f->surf || cull)					// culled/fading flare
				{
					surfaceCommon_t **s;

					for (i = 0, s = im->faces; i < im->numFaces; i++, s++)
						if ((*s)->frame == drawFrame) break;
					if (i == im->numFaces) continue;	// no visible faces - skip flare
				}
				// find entity
				for (i = 0, e = gl_entities + vp.firstEntity; i < vp.numEntities; i++, e++)
					if (e->model && e->model->type == MODEL_INLINE && e->model->inlineModel == im)
					{
						i = -1;
						break;
					}
				if (i > 0) continue;					// should not happens ...
				// compute flare origin
				VectorAdd (im->mins, im->maxs, tmp);
				VectorMA (e->center, -0.5f, tmp, tmp);
				VectorAdd (flarePos, tmp, flarePos);
//				DrawTextLeft (va("flare shift: %g %g %g -> flarePos: %g %g %g", VECTOR_ARG(tmp), VECTOR_ARG(flarePos)),RGB(1,1,1));
			}
			// perform PVS cull for flares with radius 0 (if flare have radius > 0
			// it (mostly) will be placed inside invisible (solid) leaf)
			if (f->radius == 0 && !im && f->leaf->frame != drawFrame) cull = true;
		}
		else
		{
			// sun flare
			if (gl_state.useFastSky || gl_skyShader == gl_defaultSkyShader)
				continue;	// no flare with this sky
			VectorMA (vp.vieworg, SUN_DRAWDIST, f->origin, flarePos);
			scale = f->size * SUN_DRAWDIST / (2 * FLARE_DIST1);
		}

		// should perform frustum culling even if flare not in PVS:
		// can be occluded / outside frustum -- fade / hide
		if (PointCull (flarePos, (f->owner || cull || f->radius < 0) ? MAX_FRUSTUM_MASK : f->leaf->frustumMask) == FRUSTUM_OUTSIDE)
		{
			gl_speeds.cullFlares++;		// outside frustum - do not fade
			continue;
		}
		if (f->radius > 0)
		{
			if (!SphereLeaf (flarePos, f->radius)) cull = true;
		}

		color.rgba = f->color.rgba;
		style = vp.lightStyles[f->style].value;

		// get viewsize
		if (f->radius >= 0)
		{
			float	dist;

			scale = f->size / 2;
			dist  = (flarePos[0] - vp.vieworg[0]) * vp.viewaxis[0][0] +
					(flarePos[1] - vp.vieworg[1]) * vp.viewaxis[0][1] +
					(flarePos[2] - vp.vieworg[2]) * vp.viewaxis[0][2];		// get Z-coordinate

			if (dist < FLARE_DIST0)			// too near - do not fade
			{
				gl_speeds.cullFlares++;
				continue;
			}
			if (dist < FLARE_DIST1)
				style = appRound (style * (dist - FLARE_DIST0) / (FLARE_DIST1 - FLARE_DIST0));
//				scale = scale * (dist - FLARE_DIST0) / (FLARE_DIST1 - FLARE_DIST0);
			else if (dist > FLARE_DIST2)
				scale = scale * dist / FLARE_DIST2;
		}

		if (!cull)
		{
			trace_t	trace;
			static vec3_t zero = {0, 0, 0};

			gl_speeds.testFlares++;
			// check visibility with trace
			if (f->radius >= 0)
			{
				CM_BoxTrace (&trace, vp.vieworg, flarePos, zero, zero, 0, CONTENTS_SOLID);
				ClipTraceToEntities (&trace, vp.vieworg, flarePos, CONTENTS_SOLID);
				if (trace.fraction < 1 && (f->radius <= 0 || (VectorDistance (trace.endpos, flarePos) > f->radius)))
					cull = true;
			}
			else
			{	// sun flare
				vec3_t	tracePos;

				VectorMA (vp.vieworg, BIG_NUMBER, f->origin, tracePos);
				CM_BoxTrace (&trace, vp.vieworg, tracePos, zero, zero, 0, CONTENTS_SOLID);
				ClipTraceToEntities (&trace, vp.vieworg, tracePos, CONTENTS_SOLID);
				if (!(trace.fraction < 1 && trace.surface->flags & SURF_SKY))
					cull = true;
			}

			if (!cull)
				f->lastTime = vp.time;
		}

		if (cull)
		{
			float	timeDelta;

			// fade flare
			timeDelta = (vp.time - f->lastTime) / FLARE_FADE;
			if (timeDelta >= 1)
			{
				gl_speeds.cullFlares++;
				continue;
			}
			style = appRound((1 - timeDelta) * style);
		}

		// alloc surface
		p = (surfacePoly_t*)GL_AllocDynamicMemory (sizeof(surfacePoly_t) + (4-1) * sizeof(vertexPoly_t));
		if (!p) return;
		p->numVerts = 4;

		// setup xyz
		VectorMA (flarePos, -scale, vp.viewaxis[2], tmp);			// down
		VectorMA (tmp, -scale, vp.viewaxis[1], p->verts[0].xyz);	// 0
		VectorMA (tmp, scale, vp.viewaxis[1], p->verts[1].xyz);		// 1
		VectorScale (vp.viewaxis[2], scale * 2, tmp);				// up-down
		VectorAdd (p->verts[0].xyz, tmp, p->verts[3].xyz);			// 3
		VectorAdd (p->verts[1].xyz, tmp, p->verts[2].xyz);			// 2

		// setup st
		p->verts[0].st[0] = p->verts[3].st[0] = 1;
		p->verts[1].st[0] = p->verts[2].st[0] = 0;
		p->verts[0].st[1] = p->verts[1].st[1] = 1;
		p->verts[2].st[1] = p->verts[3].st[1] = 0;

		// setup color
		if (style != 128)
		{
			int		r, g, b;
			r = color.c[0] * style >> 7;
			g = color.c[1] * style >> 7;
			b = color.c[2] * style >> 7;
			if (style > 128)
				NORMALIZE_COLOR255(r, g, b);
			color.c[0] = r;
			color.c[1] = g;
			color.c[2] = b;
		}
		p->verts[0].c.rgba = p->verts[1].c.rgba = p->verts[2].c.rgba = p->verts[3].c.rgba = color.rgba;

		surf = GL_AddDynamicSurface (gl_flareShader, ENTITYNUM_WORLD);
		if (!surf) return;
		surf->poly = p;
		surf->type = SURFACE_POLY;
	}

	gl_speeds.flares = map.numFlares;
}


static void DrawBspSequence (node_t *leaf)
{
	refEntity_t *e;
	beam_t	*b;

	for ( ; leaf; leaf = leaf->drawNext)
	{
		/*------- update world bounding box -------*/

		if (leaf->mins[0] < vp.mins[0])
			vp.mins[0] = leaf->mins[0];
		if (leaf->mins[1] < vp.mins[1])
			vp.mins[1] = leaf->mins[1];
		if (leaf->mins[2] < vp.mins[2])
			vp.mins[2] = leaf->mins[2];
		if (leaf->maxs[0] > vp.maxs[0])
			vp.maxs[0] = leaf->maxs[0];
		if (leaf->maxs[1] > vp.maxs[1])
			vp.maxs[1] = leaf->maxs[1];
		if (leaf->maxs[2] > vp.maxs[2])
			vp.maxs[2] = leaf->maxs[2];

		/*------ add leafFaces to draw list -------*/

		currentEntity = ENTITYNUM_WORLD;
		AddBspSurfaces (leaf->leafFaces, leaf->numLeafFaces, leaf->frustumMask, &gl_entities[ENTITYNUM_WORLD]);

		/*---------- draw leaf entities -----------*/

		for (e = leaf->drawEntity; e; e = e->drawNext)
		{
			currentEntity = e - gl_entities;
			if (e->model)
				switch (e->model->type)
				{
				case MODEL_INLINE:
					AddInlineModelSurfaces (e);
					break;
				case MODEL_MD3:
					AddMd3Surfaces (e);
					break;
				case MODEL_SP2:
					AddSp2Surface (e);
					break;
				}
		}

		/*------------ draw particles -------------*/

		if (leaf->drawParticle)
		{
			surfaceCommon_t *surf;

			surf = GL_AddDynamicSurface (gl_particleShader, ENTITYNUM_WORLD);
			if (surf)
			{
				surf->type = SURFACE_PARTICLE;
				surf->part = leaf->drawParticle;
			}
		}

		for (b = leaf->drawBeam; b; b = b->drawNext)
		{
			switch (b->type)
			{
			case BEAM_STANDARD:
				AddBeamSurfaces (b);
				break;
			case BEAM_RAILBEAM:
				AddFlatBeam (b, gl_railBeamShader);
				break;
			case BEAM_RAILSPIRAL:
				AddCylinderSurfaces (b, gl_railSpiralShader);
				break;
			case BEAM_RAILRINGS:
				AddCylinderSurfaces (b, gl_railRingsShader);
				break;
			}
		}
	}
}


static void MarkLeaves (void)
{
	int		cluster, i;
	node_t	*n;

	gl_speeds.leafs = map.numLeafNodes - map.numNodes;

	// determine the vieworg cluster
	cluster = GL_PointInLeaf (vp.vieworg)->cluster;
	// if cluster or areamask changed -- re-mark visible leaves
	if (gl_refdef.viewCluster != cluster || gl_refdef.areaMaskChanged)
	{
		gl_refdef.viewCluster = cluster;
		visFrame++;
		if (r_novis->integer || cluster < 0 || cluster >= map.numClusters || map.numClusters <= 1)
		{	// mark ALL nodes
			for (i = 0, n = map.nodes; i < map.numLeafNodes; i++, n++)
				n->visFrame = visFrame;
			gl_speeds.visLeafs = gl_speeds.leafs;
		}
		else
		{	// use visinfo to mark nodes
			byte	*row;

			gl_speeds.visLeafs = 0;

			row = map.visInfo + cluster * map.visRowSize;

			for (i = map.numNodes, n = map.nodes + map.numNodes; i < map.numLeafNodes; i++, n++)
			{
				node_t	*p;
				int		cl, ar;

				cl = n->cluster;
				ar = n->area;
				if (row[cl>>3] & (1<<(cl&7)) && gl_refdef.areaMask[ar>>3] & (1<<(ar&7)))
				{
					for (p = n; p && p->visFrame != visFrame; p = p->parent)
						p->visFrame = visFrame;
					gl_speeds.visLeafs++;
				}
			}
		}
	}
}


void GL_AddEntity (entity_t *ent)
{
	refEntity_t	*out;
	vec3_t	v;
	bool	mirror;

	if (gl_numEntities >= MAX_GLENTITIES)
	{
		Com_WPrintf ("R_AddEntity: MAX_GLENTITIES hit\n");
		return;
	}

	mirror = false;
	if (ent->flags & RF_WEAPONMODEL)
	{
		if (gl_hand->integer == 1)
			mirror = true;
		else if (gl_hand->integer == 2)		// do not draw weapon when "hand" == 2
			return;
	}

	out = &gl_entities[gl_numEntities++];
	vp.numEntities++;

	// common fields
	out->flags = ent->flags;
	out->model = ent->model;

	if (ent->model)
	{
		VectorCopy (ent->origin, out->origin);
		if (ent->model->type == MODEL_MD3)
		{	// need to negate angles[2]
			ent->angles[2] = -ent->angles[2];
			AnglesToAxis (ent->angles, out->axis);
			ent->angles[2] = -ent->angles[2];
		}
		else
			AnglesToAxis (ent->angles, out->axis);

		if (!ent->origin[0] && !ent->origin[1] && !ent->origin[2] &&
			!ent->angles[0] && !ent->angles[1] && !ent->angles[2])
			out->worldMatrix = true;
		else
			out->worldMatrix = false;
		out->mirror = mirror;
		out->frame = ent->frame;
		out->oldFrame = ent->oldframe;
		out->backLerp = ent->backlerp;

		out->customShader = (shader_t*) ent->skin;	//!! should use customSkin
		out->skinNum = ent->skinnum;				//?? check skinnum in [0..model.numSkins]
		out->shaderColor.rgba = RGB(1,1,1);
		out->shaderColor.c[3] = appRound (ent->alpha * 255);

		// model-specific code and calculate model center
		switch (ent->model->type)
		{
		case MODEL_UNKNOWN:
			VectorCopy (ent->origin, out->origin);
			break;
		case MODEL_INLINE:
			{
				inlineModel_t *im;

				im = ent->model->inlineModel;
				VectorAdd (im->mins, im->maxs, v);
				VectorScale (v, 0.5f, v);
				VectorSubtract (im->maxs, v, out->size2);
				ModelToWorldCoord (v, out, out->center);
				out->radius = im->radius;
			}
			break;
		case MODEL_MD3:
			{
				md3Model_t	*md3;
				md3Frame_t	*frame1, *frame2;
				vec3_t	center1, center2;

				md3 = ent->model->md3;
				// sanity check
				if (out->frame >= md3->numFrames || out->frame < 0)
				{
					DrawTextLeft (va("R_AddEntity: no frame %d in %s\n", out->frame, out->model->name), RGB(1,0,0));
					out->frame = out->oldFrame = 0;
				}
				if (out->oldFrame >= md3->numFrames || out->oldFrame < 0)
				{
					DrawTextLeft (va("R_AddEntity: no frame %d in %s\n", out->oldFrame, out->model->name), RGB(1,0,0));
					out->frame = out->oldFrame = 0;
				}
				frame1 = md3->frames + out->frame;
				frame2 = md3->frames + out->oldFrame;
				// lerp origin
				VectorSubtract (ent->oldorigin, ent->origin, v);
				VectorMA (ent->origin, out->backLerp, v, out->origin);
				// lerp center
				ModelToWorldCoord (frame1->localOrigin, out, center1);
				ModelToWorldCoord (frame2->localOrigin, out, center2);
				VectorSubtract (center2, center1, center2);				// delta
				VectorMA (center1, out->backLerp, center2, out->center);
				// lerp radius
				out->radius = (frame2->radius - frame1->radius) * out->backLerp + frame1->radius;
				// compute mins/maxs (lerp ??)
				VectorSubtract (frame1->maxs, frame1->localOrigin, out->size2);
				// check for COLOR_SHELL
				if (ent->flags & (RF_SHELL_RED|RF_SHELL_GREEN|RF_SHELL_BLUE|RF_SHELL_HALF_DAM|RF_SHELL_DOUBLE))
				{
					out->customShader = gl_colorShellShader;
					out->shaderColor.rgba = 0x202020;			// required for RED/GREEN/BLUE
					if (ent->flags & RF_SHELL_HALF_DAM)
						out->shaderColor.rgba = RGB(0.56, 0.59, 0.45);
					if (ent->flags & RF_SHELL_DOUBLE)
						out->shaderColor.rgba = RGB(0.9, 0.7 ,0);
					if (ent->flags & RF_SHELL_RED)
						out->shaderColor.c[0] = 255;
					if (ent->flags & RF_SHELL_GREEN)
						out->shaderColor.c[1] = 255;
					if (ent->flags & RF_SHELL_BLUE)
						out->shaderColor.c[2] = 255;
					out->shaderColor.c[3] = 255;
				}
			}
			break;
		case MODEL_SP2:
			VectorCopy (out->origin, out->center);
			out->radius = out->model->sp2->radius;
			break;
		}
	}
	else if (ent->flags & RF_BBOX)
	{
		VectorSubtract (ent->oldorigin, ent->origin, v);
		VectorMA (ent->origin, ent->backlerp, v, out->center);
		VectorCopy (ent->size, out->boxSize);
		AnglesToAxis (ent->angles, out->boxAxis);
		out->shaderColor.rgba = ent->color.rgba;
	}
	else
	{
		// unknown entity type: copy origin for displaying warning in 3D
		VectorCopy (ent->origin, out->origin);
	}

	gl_speeds.ents++;
}


void GL_AddDlight (dlight_t *dl)
{
	refDlight_t	*out;
	float	r, g, b, l, sat;
	int		r1, g1, b1;

	if (gl_numDlights >= MAX_GLDLIGHTS)
	{
		Com_WPrintf ("R_AddDlight: MAX_GLDLIGHTS hit\n");
		return;
	}
	out = &gl_dlights[gl_numDlights++];
	VectorCopy (dl->origin, out->origin);
	out->intensity = dl->intensity;
	r = dl->color[0] * gl_config.identityLightValue;
	g = dl->color[1] * gl_config.identityLightValue;
	b = dl->color[2] * gl_config.identityLightValue;
	sat = r_saturation->value;
	if (sat != 1.0f)
	{
		l = (r + g + b) / 3.0f;
		SATURATE(r,l,sat);
		SATURATE(g,l,sat);
		SATURATE(b,l,sat);
	}
	r1 = appRound(r); g1 = appRound(g); b1 = appRound(b);
	if (!gl_config.doubleModulateLM)
	{
		r1 <<= 1;
		g1 <<= 1;
		b1 <<= 1;
		NORMALIZE_COLOR255(r1, g1, b1);
	}
	out->c.c[0] = r1;
	out->c.c[1] = g1;
	out->c.c[2] = b1;
	out->c.c[3] = 255;		// alpha
}



/*--------------------------------------------------------------*/


void GL_DrawPortal (void)
{
	node_t	*firstLeaf;
	int		i;
	refEntity_t *e;

	if (r_drawworld->integer && !(vp.flags & RDF_NOWORLDMODEL))
	{
		drawFrame++;
		gl_speeds.frustLeafs = 0;

		// setup world entity
		memset (&gl_entities[ENTITYNUM_WORLD], 0, sizeof(refEntity_t));
		VectorCopy (vp.vieworg, gl_entities[ENTITYNUM_WORLD].modelvieworg);
		gl_entities[ENTITYNUM_WORLD].worldMatrix = true;
//		gl_entities[ENTITYNUM_WORLD].axis[0][0] = 1;
//		gl_entities[ENTITYNUM_WORLD].axis[1][1] = 1;
//		gl_entities[ENTITYNUM_WORLD].axis[2][2] = 1;

		MarkLeaves ();
		ClearBounds (vp.mins, vp.maxs);
		firstLeaf = WalkBspTree ();
		DrawEntities (vp.firstEntity, vp.numEntities);
		DrawParticles ();

		DrawBspSequence (firstLeaf);
		if (gl_flares->integer && gl_flareShader)
			DrawFlares ();
	}
	else
		for (i = 0, e = gl_entities + vp.firstEntity; i < vp.numEntities; i++, e++)
		{
			if (!e->model)
				continue;

			SetupModelMatrix (e);

			currentEntity = e - gl_entities;
			if (e->model)
			{
				e->visible = true;
				switch (e->model->type)
				{
				case MODEL_INLINE:
					AddInlineModelSurfaces (e);
					break;
				case MODEL_MD3:
					AddMd3Surfaces (e);
					break;
				case MODEL_SP2:
					AddSp2Surface (e);
					break;
				}
			}
		}
}
