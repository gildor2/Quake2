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


// Uses entity to determine transformations. We cannot use BoxOnPlaneSide, because
// transformed bounding box is not axial.
static int TransformedBoxCull (vec3_t mins, vec3_t maxs, refEntity_t *e)
{
	int		i, res, frustumMask, mask;
	vec3_t	box[8];
	float	*v;
	cplane_t *fr;

	if (r_nocull->integer)
		return FRUSTUM_ON;

	frustumMask = e->frustumMask;

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

	if (frustumMask & ~15)			// update frustum mask for entity
		e->frustumMask = frustumMask & 15;

	return res;
}


static int TransformedSphereCull (vec3_t localOrigin, float radius, refEntity_t *e)
{
	vec3_t	center;
	int		i;
	cplane_t *fr;
	qboolean hasOutside;

	if (r_nocull->integer)
		return FRUSTUM_ON;

	// project localOrigin to a world coordinate system
	VectorMA (e->origin, localOrigin[0], e->axis[0], center);
	VectorMA (center,	 localOrigin[1], e->axis[1], center);
	VectorMA (center,	 localOrigin[2], e->axis[2], center);

	hasOutside = false;
	// perform frustum culling
	for (i = 0, fr = vp.frustum; i < 4; i++, fr++)
	{	// loop by frustum planes
		float	dist;

		dist = DotProduct (center, fr->normal) - fr->dist;
		if (dist < -radius)
			return FRUSTUM_OUTSIDE;
		if (dist <= radius)
			hasOutside = true;
	}
	return hasOutside ? FRUSTUM_ON : FRUSTUM_INSIDE;
}


/*------------------ Drawing world -------------------*/

static void RecursiveWorldNode (node_t *node, int frustumMask)
{
	if (node->frame != visFrame)
		return;		// discard node using visinfo

	if (!r_nocull->integer)
	{
		/*-------- frustum culling ----------*/
#define CULL_NODE(bit)	\
		if (frustumMask & (1<<bit))	\
		{							\
			switch (BOX_ON_PLANE_SIDE(node->mins, node->maxs, &vp.frustum[bit])) {	\
			case 1:					\
				frustumMask &= ~(1<<bit);	\
				break;				\
			case 2:					\
				return;				\
			}						\
		}

		CULL_NODE(0);
		CULL_NODE(1);
		CULL_NODE(2);
		CULL_NODE(3);
#undef CULL_NODE
	}

	//!! check dlights

	if (node->isNode)
	{	// Q3: child0, than child1; Q1/2: child depends on clipPlane (standard BSP usage)
		if (DISTANCE_TO_PLANE(vp.vieworg, node->plane) < 0)
		{
			RecursiveWorldNode (node->children[0], frustumMask);
			RecursiveWorldNode (node->children[1], frustumMask);
		}
		else
		{
			RecursiveWorldNode (node->children[1], frustumMask);
			RecursiveWorldNode (node->children[0], frustumMask);
		}
	}
	else
	{
		int		i;
		surfaceCommon_t **psurf;

		gl_speeds.frustLeafs++;

		/*------- update world bounding box -------*/
		if (node->mins[0] < vp.mins[0])
			vp.mins[0] = node->mins[0];
		if (node->mins[1] < vp.mins[1])
			vp.mins[1] = node->mins[1];
		if (node->mins[2] < vp.mins[2])
			vp.mins[2] = node->mins[2];
		if (node->maxs[0] > vp.maxs[0])
			vp.maxs[0] = node->maxs[0];
		if (node->maxs[1] > vp.maxs[1])
			vp.maxs[1] = node->maxs[1];
		if (node->maxs[2] > vp.maxs[2])
			vp.maxs[2] = node->maxs[2];

		/*------ add leafFaces to draw list -------*/
		for (i = 0, psurf = node->leafFaces; i < node->numLeafFaces; i++, psurf++)
		{
			surfaceCommon_t *surf;
			surfacePlanar_t *pl;

			surf = *psurf;
			if (surf->frame == drawFrame) continue;
			surf->frame = drawFrame;

			//!! frustum culling non-planar surfaces
			switch (surf->type)
			{
			case SURFACE_PLANAR:
				pl = surf->pl;
				// backface culling
				if (gl_facePlaneCull->integer)
				{
					cplane_t *plane;
					float	dist;
					gl_cullMode_t cull;

					cull = surf->shader->cullMode;
					if (cull != CULL_NONE)
					{
						plane = &pl->plane;
						dist = DotProduct (plane->normal, vp.vieworg);
						if (cull == CULL_FRONT)
						{
							if (plane->dist + 8 < dist)
							{
								gl_speeds.cullSurfs++;
								continue;
							}
						}
						else
						{
							if (plane->dist - 8 > dist)
							{
								gl_speeds.cullSurfs++;
								continue;
							}
						}
					}
				}
				// frustum culling
				if (!r_nocull->integer && frustumMask)
				{
#define CULL_PLANE(bit)	\
					if (frustumMask & (1<<bit) &&	\
						BOX_ON_PLANE_SIDE(pl->mins, pl->maxs, &vp.frustum[bit]) == 2)	\
						{							\
							gl_speeds.cullSurfs++;	\
							continue;				\
						}
					CULL_PLANE(0);
					CULL_PLANE(1);
					CULL_PLANE(2);
					CULL_PLANE(3);
#undef CULL_PLANE
				}
				break;
			}
			//!! apply dlights, fog
			GL_AddSurfaceToPortal (surf, surf->shader, ENTITYNUM_WORLD);
		}
	}
}


void GL_DrawWorld (void)
{
	int		cluster, i;
	node_t	*n;

	if (!r_drawworld->integer) return;
	if (gl_refdef.flags & RDF_NOWORLDMODEL) return;

	currentEntity = ENTITYNUM_WORLD;	//??
	drawFrame++;

	gl_speeds.leafs = map.numLeafNodes - map.numNodes;

	// determine the vieworg cluster
	cluster = GL_PointInLeaf (vp.modelvieworg)->cluster;
	// if cluster or areamask changed -- re-mark visible leaves
	if (gl_refdef.viewCluster != cluster || gl_refdef.areaMaskChanged)
	{
		gl_refdef.viewCluster = cluster;
		visFrame++;
		if (r_novis->integer || cluster == -1 || map.numClusters <= 1)
		{	// mark ALL nodes
			for (i = 0, n = map.nodes; i < map.numLeafNodes; i++, n++)
				n->frame = visFrame;
			gl_speeds.visLeafs = gl_speeds.leafs;
		}
		else
		{	// use visinfo to mark nodes
			byte	*row;

			gl_speeds.visLeafs = 0;

			if (cluster < 0 || cluster >= map.numClusters)
				row = map.noVis;	// mark ALL nodes, but using area mask
			else
				row = map.visInfo + cluster * map.visRowSize;

			for (i = map.numNodes, n = map.nodes + map.numNodes; i < map.numLeafNodes; i++, n++)
			{
				node_t	*p;
				int		cl, ar;

				cl = n->cluster;
				ar = n->area;
				if (row[cl>>3] & (1<<(cl&7)) && gl_refdef.areaMask[ar>>3] & (1<<(ar&7)))	// NOTE: Q3 has inverted areaBits (areaMask)
				{
					for (p = n; p && p->frame != visFrame; p = p->parent)
						p->frame = visFrame;
					gl_speeds.visLeafs++;
				}
			}
		}
	}

	gl_speeds.frustLeafs = 0;

	ClearBounds (vp.mins, vp.maxs);
	RecursiveWorldNode(map.nodes, 0xF);
}


/*------------------ Drawing entities --------------------*/


static void SetupModelMatrix (refEntity_t *e)
{
	float	matrix[4][4];
	int		i, j, k;
	vec3_t	v;

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
	VectorSubtract (vp.vieworg, e->origin, v);
	for (i = 0; i < 3; i++)
		e->modelvieworg[i] = DotProduct (v, e->axis[i]);
}


static void AddInlineModelSurfaces (refEntity_t *e)
{
	inlineModel_t	*im;
	surfaceCommon_t	*surf;
	surfacePlanar_t	*pl;
	int		i, cull;

	im = e->model->inlineModel;

	e->frustumMask = -1;		// for updating
	// frustum culling (entire model)
	if ((cull = TransformedBoxCull (im->mins, im->maxs, e)) == FRUSTUM_OUTSIDE)
	{
		gl_speeds.cullEnts++;
		return;
	}

	for (i = 0, surf = im->faces; i < im->numFaces; i++, surf++)
	{
		// This loop similar to RecursiveWorldNode()
		switch (surf->type)
		{
		case SURFACE_PLANAR:
			pl = surf->pl;
			// backface culling
			if (gl_facePlaneCull->integer)
			{
				cplane_t *plane;
				float	dist;
				gl_cullMode_t cull;

				cull = surf->shader->cullMode;
				if (cull != CULL_NONE)
				{
					plane = &pl->plane;
					dist = DotProduct (plane->normal, e->modelvieworg);
					if (cull == CULL_FRONT)
					{
						if (plane->dist + 8 < dist)
						{
							gl_speeds.cullSurfs++;
							continue;
						}
					}
					else
					{
						if (plane->dist - 8 > dist)
						{
							gl_speeds.cullSurfs++;
							continue;
						}
					}
				}
			}
			// frustum culling
			if (!r_nocull->integer && cull == FRUSTUM_ON &&
				TransformedBoxCull (pl->mins, pl->maxs, e) == FRUSTUM_OUTSIDE)
			{
				gl_speeds.cullSurfs++;
				continue;
			}
			break;
		}
		//!! apply dlights, fog
		GL_AddSurfaceToPortal (surf, surf->shader, currentEntity);
	}
}


static void AddMd3Surfaces (refEntity_t *e)
{
	md3Model_t	*md3;
	surfaceCommon_t *surf;
	md3Frame_t	*frame1, *frame2;
	int		i, cull1, cull2;

	md3 = e->model->md3;

	// frustum culling
	frame1 = md3->frames + e->frame;
	frame2 = md3->frames + e->oldFrame;
	cull1 = TransformedSphereCull (frame1->localOrigin, frame1->radius, e);
	if (frame1 == frame2 && cull1 == FRUSTUM_OUTSIDE)
	{
		gl_speeds.cullEnts++;	//?? gl_speeds.cullMd3Surfs ?
		return;
	}
	else
	{
		cull2 = TransformedSphereCull (frame2->localOrigin, frame2->radius, e);
		if (cull1 == cull2 && cull1 == FRUSTUM_OUTSIDE)
		{	// culled
			gl_speeds.cullEnts++;
			return;
		}
	}
	//?? may perform BoxCulling too (more conservative)

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
		GL_AddSurfaceToPortal (surf, shader, currentEntity);
	}
}


static int EntCompare (const void *e1, const void *e2)
{
	float	d;

	d = ((refEntity_t *)e1)->dist2 - ((refEntity_t *)e2)->dist2;
	if (d < 0)	// draw from front to back
		return 1;
	else if (d > 0)
		return -1;
	return 0;
}


void GL_DrawEntities (int firstEntity, int numEntities)
{
	int		i;
	refEntity_t	*e;

	if (!numEntities || !r_drawentities->integer) return;

	// depth-sort entity for correct drawing of translucent objects (for example, inline-model windows)
	qsort (&gl_entities[firstEntity], numEntities, sizeof(refEntity_t), EntCompare);

	for (i = firstEntity, e = gl_entities + firstEntity; i < firstEntity + numEntities; i++, e++)
	{
		currentEntity = i;

		if (e->model)
		{
			SetupModelMatrix (e);
			switch (e->model->type)
			{
			case MODEL_INLINE:
				AddInlineModelSurfaces (e);
				break;
			case MODEL_MD3:
				AddMd3Surfaces (e);
				break;
			//?? other types
			}
		}
		else
		{
			//?? draw null model or special entities (rail beam etc)
			DrawTextLeft (va("NULL entity %d", i), 1, 0, 0);
		}
	}
}


void GL_AddEntity (entity_t *ent)
{
	refEntity_t	*out;
	vec3_t	v;

	if (gl_numEntities >= MAX_GLENTITIES)
	{
		Com_WPrintf ("R_AddEntity: MAX_GLENTITIES hit\n");
		return;
	}
	out = &gl_entities[gl_numEntities++];

	out->model = ent->model;
	out->customShader = (shader_t*) ent->skin;	//!! should use customSkin
	out->skinNum = ent->skinnum;	//?? check skinnum in [0..model.numSkins]

	VectorCopy (ent->origin, out->origin);
	AnglesToAxis (ent->angles, out->axis);
	out->flags = ent->flags;

	out->frame = ent->frame;
	out->oldFrame = ent->oldframe;
	out->backLerp = ent->backlerp;

	out->shaderRGBA[3] = (int)(ent->alpha * 255);	//!! should fill whole array

	// model-specific code and calculate distance to object (to "v")
	if (ent->model)
	{
		switch (ent->model->type)
		{
		case MODEL_INLINE:
			VectorAdd (ent->model->inlineModel->mins, ent->model->inlineModel->maxs, v);
			VectorMA (ent->origin, 0.5, v, v);	// middle point in world coordinates
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
				// copy origin to v
				VectorCopy (ent->origin, v);
			}
			break;
		default:
			//?? other types (is it needed? bboxes may be world-aligned for inline models only)
			VectorCopy (ent->origin, v);
		}
	}
	else
		VectorCopy (ent->origin, v);
	VectorSubtract (v, vp.vieworg, v);
	out->dist2 = DotProduct (v, v);

	gl_speeds.ents++;
}
