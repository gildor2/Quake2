#include "gl_local.h"
#include "gl_model.h"
#include "gl_backend.h"
#include "gl_shader.h"


#define map		gl_worldModel


static int visFrame, drawFrame;
static int currentEntity;


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


#define SATURATE(c,l,v) c = (l+0.5+(c-l)*v); c = bound(c, 0, 255);


/* Culling on BSP:
 *   1. PVS
 *   2. frustum cull leafs
 *   3. backface/frustum cull leaffaces
 * Culling on inline models:
 *   1. frustum cull entire model
 *   2. backface/frustum cull model surfaces
 * Culling triangle models:
 *   1. frustum cull entire model
 *
 * TODO:
 *   - oclusion culling (leafs, models, surfaces)
 *   - trisurfs backface culling
 *   ? weapon model frustum culling (with lower plane)
 */

/*------------------- Frustum culling -------------------*/

#define FRUSTUM_INSIDE	1
#define FRUSTUM_OUTSIDE	2
#define FRUSTUM_ON		(FRUSTUM_INSIDE|FRUSTUM_OUTSIDE)


static int BoxCull (vec3_t mins, vec3_t maxs, int frustumMask)
{
	int		i, res;
	cplane_t *pl;

	if (r_nocull->integer)
		return FRUSTUM_ON;

	if (!frustumMask)
		return FRUSTUM_INSIDE;

	res = FRUSTUM_INSIDE;
	for (i = 0, pl = vp.frustum; i < 4; i++, pl++)
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
//	float	*v;
	cplane_t *fr;
//	vec3_t	box[8];

	if (r_nocull->integer)
		return FRUSTUM_ON;

	frustumMask = e->frustumMask;	//?? get this as OR(all_occupied_leafs->frustumMask) or remove
	if (!frustumMask)
		return FRUSTUM_INSIDE;

#if 1	// this version FASTER
	res = 0;

	for (i = 0, mask = 1, fr = vp.frustum; i < 4; i++, mask<<=1, fr++)
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
//		pl.type = PLANE_NON_AXIAL;
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
	for (i = 0, mask = 1, fr = vp.frustum; i < 4; i++, mask<<=1, fr++)
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

	if (frustumMask & ~15)			// update frustum mask for entity
		e->frustumMask = frustumMask & 15;

	return res;
}


static int SphereCull (vec3_t origin, float radius)
{
	int		i;
	cplane_t *pl;
	qboolean hasOutside;

	if (r_nocull->integer)
		return FRUSTUM_ON;

	hasOutside = false;
	// perform frustum culling
	for (i = 0, pl = vp.frustum; i < 4; i++, pl++)
	{	// loop by frustum planes
		float	dist;

		dist = DotProduct (origin, pl->normal) - pl->dist;
		if (dist < -radius)
			return FRUSTUM_OUTSIDE;
		if (dist <= radius)
			hasOutside = true;
	}
	return hasOutside ? FRUSTUM_ON : FRUSTUM_INSIDE;
}


// Reduced SphereCull()
static int PointCull (vec3_t point, int frustumMask)
{
	int		i;
	cplane_t *pl;

	if (r_nocull->integer)
		return FRUSTUM_INSIDE;

	if (!frustumMask)
		return FRUSTUM_INSIDE;

	for (i = 0, pl = vp.frustum; i < 4; i++, pl++)
		if (frustumMask & (1 << i))
		{
			if (DotProduct (point, pl->normal) - pl->dist < 0)
				return FRUSTUM_OUTSIDE;
		}
	return FRUSTUM_INSIDE;
}


// Project localOrigin to a world coordinate system
static void ModelToWorldCoord (vec3_t localOrigin, refEntity_t *e, vec3_t center)
{
	VectorMA (e->origin, localOrigin[0], e->axis[0], center);
	VectorMA (center,	 localOrigin[1], e->axis[1], center);
	VectorMA (center,	 localOrigin[2], e->axis[2], center);
}


static void WorldToModelCoord (vec3_t world, refEntity_t *e, vec3_t local)
{
	vec3_t	v;
	int		i;

	VectorSubtract (world, e->origin, v);
	for (i = 0; i < 3; i++)
		local[i] = DotProduct (v, e->axis[i]);
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
// NOTE: can easily extend to any (invisible too) inline models (add flag "visibleOnly") ( can be useful for lighting ??)
static void ClipTraceToEntities (vec3_t start, vec3_t end, int brushmask, trace_t *tr)
{
	int		i;
	refEntity_t *e;
	trace_t	trace;
	float	traceLen;
	vec3_t	traceDir;

	VectorSubtract (end, start, traceDir);
	traceLen = VectorNormalize (traceDir);	//!! uses sqrt()

	for (i = 0, e = gl_entities + vp.firstEntity; i < vp.numEntities; i++, e++)
	{
		inlineModel_t *im;
		float	entPos, dist2;
		vec3_t	tmp, center2;
		static vec3_t zero = {0, 0, 0};

		if (!e->visible || !e->model || e->model->type != MODEL_INLINE)
			continue;

		im = e->model->inlineModel;
		VectorSubtract (im->center, start, center2);

		// collision detection: line vs sphere
		entPos = DotProduct (center2, traceDir);
		if (entPos < -im->radius || entPos > traceLen + im->radius)
			continue;		// too near / too far

//DrawTextLeft(va("clip: %s (pos: %g)", e->model->name, entPos),0,1,1);//!!
		VectorMA (center2, -entPos, traceDir, tmp);
		dist2 = DotProduct (tmp, tmp);
		if (dist2 >= im->radius * im->radius) continue;

//DrawTextLeft(va("clip: %s (dist2: %g)", e->model->name, dist2),1,1,1);//!!
		// trace
		if (!e->worldMatrix)
		{
			// transform start/end to entity coordinate system
			vec3_t	start2, end2;

			WorldToModelCoord (start, e, start2);
			WorldToModelCoord (end, e, end2);
			trace = CM_BoxTrace (start2, end2, zero, zero, im->headnode, brushmask);
		}
		else
			trace = CM_BoxTrace (start, end, zero, zero, im->headnode, brushmask);

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


/*-------------- BSP object insertion ----------------*/

/* NOTE:
  ?? required for ents with shader->sort > OPAQUE (sort alpha-ents)
  - useful for other entity types when occlusion culling will be imlemented
    (insert ents AFTER world culling)
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
				return NULL;	// whole tree visited, but not leaf found (may happens when r_nocull & ent outside frustum)

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
			if (dist > 0)
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


// Find nearest visible by draw order leaf, occupied by entity sphere,
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
		else if (dist < -radius)		// use mradius = -radius for speedup ??
			node = node->children[1];	// side 2
		else
		{
			// both sides -- go origin's side first
			// (what this will do: if localOrigin point is visible, return PointLeaf(org))
			if (dist > 0)
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


/*------------------ Drawing entities --------------------*/


static void AddBspSurfaces (surfaceCommon_t **psurf, int numFaces, int frustumMask, refEntity_t *e)
{
	int		i, numDlights, j;
	refDlight_t *dl;
	float	*vieworg;

	if (e->worldMatrix)
		currentEntity = ENTITYNUM_WORLD;

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
			if (gl_facePlaneCull->integer)
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
						if (dist > 8) CULL_SURF;
					}
					else
					{
						if (dist < -8) CULL_SURF;
					}
				}
			}

			// frustum culling
			if (frustumMask)
			{
				if (currentEntity == ENTITYNUM_WORLD)
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

				sdl = pl->dlights = GL_AllocDynamicMemory (sizeof(surfDlight_t) * MAX_DLIGHTS);
				if (!sdl) surf->dlightMask = 0;		// easiest way to break the loop below
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
						if (currentEntity == ENTITYNUM_WORLD)
							dl_org = dl->origin;
						else
							dl_org = dl->modelOrg;
						dist = DISTANCE_TO_PLANE(dl_org, &pl->plane);
//if (currentEntity != ENTITYNUM_WORLD)//!!
//	DrawTextLeft(va("dl: dist=%g :: surf=(%g,%g)-(%g,%g)",dist,pl->mins2[0],pl->maxs2[0],pl->mins2[1],pl->maxs2[1]),1,0,1);
						if (dist < 0) dist = -dist;
						else if (!gl_dlightBacks->integer && dist > 8) CULL_DLIGHT;

						if (dist >= dl->intensity) CULL_DLIGHT;
						rad = sqrt (dl->intensity * dl->intensity - dist * dist);	//!! use tables
						org0 = DotProduct (dl_org, pl->axis[0]);
//if (currentEntity != ENTITYNUM_WORLD)//!!
//	DrawTextLeft(va("    rad=%g :: org0=%g",rad,org0),1,0,1);
						if (org0 < pl->mins2[0] - rad || org0 > pl->maxs2[0] + rad) CULL_DLIGHT;
						org1 = DotProduct (dl_org, pl->axis[1]);
//if (currentEntity != ENTITYNUM_WORLD)//!!
//	DrawTextLeft(va("    org1=%g",org1),1,0,1);
						if (org1 < pl->mins2[1] - rad || org1 > pl->maxs2[1] + rad) CULL_DLIGHT;
//if (currentEntity != ENTITYNUM_WORLD)//!!
//	DrawTextLeft(va("    org=(%g,%g)",org0,org1),1,0,1);
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
				GL_ResizeDynamicMemory (pl->dlights, sizeof(surfDlight_t) * numDlights);
			}
			else
				surf->dlightMask = 0;
			break;
		default:
			DrawTextLeft ("unknows surface type", 1, 0, 0);
			continue;
		}
		//!! apply fog
		if (surf->shader->lightmapNumber == LIGHTMAP_VERTEX) numDlights = 0;	// sort all together
		GL_AddSurfaceToPortal (surf, surf->shader, currentEntity, numDlights);
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
//	DrawTextLeft (va("%s {%g,%g,%g}", e->model->name, im->center[0], im->center[1], im->center[2]), 1, 0.2, 0.2);
	// check dlights
	dlightMask = 0;
	for (i = 0, dl = vp.dlights, mask = 1; i < vp.numDlights; i++, dl++, mask <<= 1)
	{
		vec3_t	tmp;
		float	dist2, dist2min;

		VectorSubtract (im->center, dl->origin, tmp);
		dist2 = DotProduct (tmp, tmp);
		dist2min = im->radius + dl->intensity;
		dist2min = dist2min * dist2min;
		if (dist2 >= dist2min) continue;	// too far

		WorldToModelCoord (dl->origin, e, dl->modelOrg);
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
//	if (dlightMask) DrawTextLeft(va("dl_ent %d(%g,%g,%g) %08X", currentEntity,
//		im->center[0],im->center[1],im->center[2],dlightMask),1,1,0);//!!!! REMOVE
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
		//!! dlights, fog
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
	p = GL_AllocDynamicMemory (sizeof(surfacePoly_t) + 4 * sizeof(vertexPoly_t));
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
		color |= 0xFF000000;		// make it non-transparent
	p->verts[0].c.rgba = p->verts[1].c.rgba = p->verts[2].c.rgba = p->verts[3].c.rgba = color;

	surf = GL_AddDynamicSurface (frame->shader, ENTITYNUM_WORLD);
	surf->poly = p;
	surf->type = SURFACE_POLY;
}


static void AddBeamSurfaces (refEntity_t *e)
{
	vec3_t	viewDir, viewDir2;
	vec3_t	axis[3];	// length, width, depth
	vec3_t	dir1, dir2;
	float	z1, z2, size, angle, angleStep;
	int		i, numParts;

	// compute detail level
	VectorSubtract (e->beamStart, vp.vieworg, viewDir);
	z1 = DotProduct (viewDir, vp.viewaxis[0]);		// beamStart.Z
	VectorSubtract (e->beamEnd, vp.vieworg, viewDir2);
	z2 = DotProduct (viewDir2, vp.viewaxis[0]);		// beamEnd.Z
	if ((z1 < z2 && z1 > 0) || z2 <= 0)
		size = z1;
	else
		size = z2;
	if (size < 0)
	{	// both Z-coords < 0
		gl_speeds.cullEnts2++;
		return;
	}
	size = e->beamRadius * 200 / (size * vp.fov_scale);
	numParts = Q_ftol(size);
	numParts = bound(numParts, 1, 6);

	// compute beam axis
	VectorSubtract (e->beamEnd, e->beamStart, axis[0]);
	VectorNormalizeFast (axis[0]);
	CrossProduct (axis[0], viewDir, axis[1]);
	VectorNormalizeFast (axis[1]);
	CrossProduct (axis[0], axis[1], axis[2]);		// already normalized

	VectorScale (axis[1], e->beamRadius, dir2);
	angle = 0;
	angleStep = M_PI / numParts;
	for (i = 0; i < numParts; i++)
	{
		surfacePoly_t *p;
		surfaceCommon_t *surf;
		float	sx, cx;

		p = GL_AllocDynamicMemory (sizeof(surfacePoly_t) + 4 * sizeof(vertexPoly_t));
		if (!p)		// out of dynamic memory
			return;
		p->numVerts = 4;

		// rotate dir1 & dir2 vectors
		VectorCopy (dir2, dir1);
		angle += angleStep;
		sx = sin (angle) * e->beamRadius;		//?? do it with tables
		cx = cos (angle) * e->beamRadius;
		VectorScale (axis[1], cx, dir2);
		VectorMA (dir2, sx, axis[2], dir2);

		// setup xyz
		VectorAdd (e->beamStart, dir1, p->verts[0].xyz);
		VectorAdd (e->beamEnd, dir1, p->verts[1].xyz);
		VectorAdd (e->beamStart, dir2, p->verts[3].xyz);
		VectorAdd (e->beamEnd, dir2, p->verts[2].xyz);

		// setup st
		p->verts[0].st[0] = p->verts[3].st[0] = 1;
		p->verts[1].st[0] = p->verts[2].st[0] = 0;
		p->verts[0].st[1] = p->verts[1].st[1] = 1;
		p->verts[2].st[1] = p->verts[3].st[1] = 0;

		p->verts[0].c.rgba = p->verts[1].c.rgba = p->verts[2].c.rgba = p->verts[3].c.rgba = e->shaderColor.rgba;

		surf = GL_AddDynamicSurface (gl_identityLightShader2, ENTITYNUM_WORLD);
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
	frustumMask = 0xF;		// check all frustum planes
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

		if (!r_nocull->integer)
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
#undef CULL_NODE
		}

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
	int		i;
	refEntity_t	*e;
	node_t	*leaf;
	vec3_t	delta, center;
	float	dist2;

	if (!numEntities || !r_drawentities->integer) return;

	for (i = 0, e = gl_entities + firstEntity; i < numEntities; i++, e++)
	{
		e->visible = false;
#define CULL_ENT	\
	{				\
		gl_speeds.cullEnts++;	\
		continue;	\
	}
		if (e->model)
		{
			switch (e->model->type)
			{
			case MODEL_INLINE:
				{
					inlineModel_t *im;

					im = e->model->inlineModel;
					e->frustumMask = -1;		// for updating
					// frustum culling (entire model)
					if (TransformedBoxCull (im->mins, im->maxs, e) == FRUSTUM_OUTSIDE) CULL_ENT;

					leaf = SphereLeaf (im->center, im->radius);
					VectorCopy (im->center, center);
				}
				break;
			case MODEL_MD3:
				{
					md3Model_t	*md3;
					md3Frame_t	*frame1, *frame2;
					int		cull1, cull2;
					vec3_t	center2;

					md3 = e->model->md3;

					// frustum culling
					frame1 = md3->frames + e->frame;
					frame2 = md3->frames + e->oldFrame;
					ModelToWorldCoord (frame1->localOrigin, e, center);
					cull1 = SphereCull (center, frame1->radius);
					if (frame1 == frame2 && cull1 == FRUSTUM_OUTSIDE)
						CULL_ENT
					else
					{
						ModelToWorldCoord (frame2->localOrigin, e, center2);
						cull2 = SphereCull (center2, frame2->radius);
						if (cull1 == cull2 && cull1 == FRUSTUM_OUTSIDE) CULL_ENT;
					}
					leaf = SphereLeaf (center, frame1->radius);
				}
				break;
			case MODEL_SP2:
				leaf = AlphaSphereLeaf (e->origin, e->model->sp2->radius);
				VectorCopy (e->origin, center);
				break;
			default:
				DrawTextLeft ("Bad model type", 1, 0, 0);
				continue;
			}
		}
		else if (e->flags & RF_BEAM)
		{
			float	radius;

			VectorAdd (e->beamStart, e->beamEnd, center);
			VectorScale (center, 0.5f, center);
			radius = VectorDistance (e->beamStart, center);
			leaf = AlphaSphereLeaf (center, radius);
		}
		else
		{
			DrawTextLeft (va("NULL entity %d", i), 1, 0, 0);
			continue;
		}

		if (!leaf) CULL_ENT;			// entity do not occupy any visible leafs

		e->visible = true;
		// calc model distance
		VectorSubtract (center, vp.vieworg, delta);
		dist2 = e->dist2 = DotProduct (delta, vp.viewaxis[0]);
		// add entity to leaf's entity list
		if (leaf->drawEntity)
		{
			refEntity_t *prev, *e1;

			for (e1 = leaf->drawEntity, prev = NULL; e1; prev = e1, e1 = e1->drawNext)
				if (dist2 > e1->dist2)
				{
					e->drawNext = e1;
					if (prev)
						prev->drawNext = e;
					else
						leaf->drawEntity = e;	// insert first
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
}


static void DrawParticles (particle_t *p)
{
	for ( ; p; p = p->next)
	{
		node_t	*leaf;

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
}


#define FLARE_DIST0		40
#define FLARE_DIST1		256
#define FLARE_DIST2		512		// Cvar_VariableValue("flare")
#define FLARE_FADE		0.2		// Cvar_VariableValue("flare")

static void DrawFlares (void)
{
	int		i;
	gl_flare_t	*f;

	for (i = 0, f = map.flares; i < map.numFlares; i++, f++)
	{
		qboolean cull;
		surfaceCommon_t *surf;
		surfacePoly_t *p;
		float	dist, scale;
		vec3_t	tmp, flarePos;
		color_t	color;
		int		style;

		cull = false;

		if (f->radius >= 0)
		{
			VectorCopy (f->origin, flarePos);
			// perform PVS cull for flares with radius 0 (if flare have radius > 0
			// it (mostly) will be placed inside invisible (solid) leaf)
			if (f->radius == 0)
			{
				if (f->leaf->frame != drawFrame)
					cull = true;
			}
			else
			{
				if (!SphereLeaf (f->origin, f->radius))
					cull = true;
			}
		}
		else
			VectorMA (vp.vieworg, FLARE_DIST1, f->origin, flarePos);

		// should perform frustum culling even if flare not in PVS:
		// can be occluded / outside frustum -- fade / hide
		if (PointCull (flarePos, cull || f->radius < 0 ? 15 : f->leaf->frustumMask) == FRUSTUM_OUTSIDE)
		{
			gl_speeds.cullFlares++;		// outside frustum - do not fade
			continue;
		}

		// get viewsize
		scale = f->size / 2;
		dist  = (flarePos[0] - vp.vieworg[0]) * vp.viewaxis[0][0] +
				(flarePos[1] - vp.vieworg[1]) * vp.viewaxis[0][1] +
				(flarePos[2] - vp.vieworg[2]) * vp.viewaxis[0][2];		// get Z-coordinate

		color.rgba = f->color.rgba;
		style = vp.lightStyles[f->style].value;

		if (dist < FLARE_DIST0)			// too near - do not fade
		{
			gl_speeds.cullFlares++;
			continue;
		}
		if (dist < FLARE_DIST1)
			style = Q_ftol (style * (dist - FLARE_DIST0) / (FLARE_DIST1 - FLARE_DIST0));
//			scale = scale * (dist - FLARE_DIST0) / (FLARE_DIST1 - FLARE_DIST0);
		else if (dist > FLARE_DIST2)
			scale = scale * dist / FLARE_DIST2;

		if (!cull)
		{
			trace_t	trace;
			static vec3_t zero = {0, 0, 0};

			// check visibility with trace
			if (f->radius >= 0)
			{
				trace = CM_BoxTrace (vp.vieworg, flarePos, zero, zero, 0, CONTENTS_SOLID);
				ClipTraceToEntities (vp.vieworg, flarePos, CONTENTS_SOLID, &trace);
				if (trace.fraction < 1 && (f->radius <= 0 || (VectorDistance (trace.endpos, flarePos) > f->radius)))
					cull = true;
			}
			else
			{	// sun flare
				vec3_t	tracePos;

				VectorMA (vp.vieworg, 99999, f->origin, tracePos);
				trace = CM_BoxTrace (vp.vieworg, tracePos, zero, zero, 0, CONTENTS_SOLID);
				ClipTraceToEntities (vp.vieworg, tracePos, CONTENTS_SOLID, &trace);
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
			style = Q_ftol((1 - timeDelta) * style);
		}

		// alloc surface
		p = GL_AllocDynamicMemory (sizeof(surfacePoly_t) + 4 * sizeof(vertexPoly_t));
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
			else if (e->flags & RF_BEAM)
				AddBeamSurfaces (e);
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
	qboolean mirror;

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
		out->shaderColor.rgba = 0xFFFFFF;			//?? white
		out->shaderColor.c[3] = (int)(ent->alpha * 255);

		// model-specific code and calculate model center
		switch (ent->model->type)
		{
		case MODEL_INLINE:
			{
				inlineModel_t *im;

				im = ent->model->inlineModel;
				VectorAdd (im->mins, im->maxs, v);
				VectorMA (ent->origin, 0.5f, v, im->center);	// middle point in world coordinates
			}
			break;
		case MODEL_MD3:
			{
				md3Model_t	*md3;

				md3 = ent->model->md3;
				// sanity check
				if (out->frame >= md3->numFrames || out->frame < 0)
				{
					Com_WPrintf ("R_AddEntity: no frame %d in %s\n", out->frame, out->model->name);
					out->frame = out->oldFrame = 0;
				}
				if (out->oldFrame >= md3->numFrames || out->oldFrame < 0)
				{
					Com_WPrintf ("R_AddEntity: no frame %d in %s\n", out->oldFrame, out->model->name);
					out->frame = out->oldFrame = 0;
				}
				// lerp origin
				VectorSubtract (ent->oldorigin, ent->origin, v);
				VectorMA (ent->origin, out->backLerp, v, out->origin);
			}
			break;
		}
	}
	else if (ent->flags & RF_BEAM)
	{
		VectorCopy (ent->origin, out->beamStart);
		VectorCopy (ent->oldorigin, out->beamEnd);
		out->beamRadius = ent->frame / 2.0f;
		out->shaderColor.rgba = gl_config.tbl_8to32[ent->skinnum];
		out->shaderColor.c[3] = (int)(ent->alpha * 255);
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
	r1 = Q_ftol(r);
	g1 = Q_ftol(g);
	b1 = Q_ftol(b);
	if (gl_config.lightmapOverbright)
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
		VectorCopy (vp.vieworg, gl_entities[ENTITYNUM_WORLD].modelvieworg);
		gl_entities[ENTITYNUM_WORLD].worldMatrix = true;

		MarkLeaves ();
		ClearBounds (vp.mins, vp.maxs);
		firstLeaf = WalkBspTree ();
		DrawEntities (vp.firstEntity, vp.numEntities);
		DrawParticles (vp.particles);

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
