#include "gl_local.h"
#include "gl_model.h"
#include "gl_backend.h"
#include "gl_shader.h"


#define map		gl_worldModel

#define MAX_TREE_DEPTH	512


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
//	float	*v;
	cplane_t *fr;
//	vec3_t	box[8];

	if (r_nocull->integer)
		return FRUSTUM_ON;

	frustumMask = e->frustumMask;	//?? get this as OR(all_occupied_leafs->frustumMask) or remove

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
	cplane_t *fr;
	qboolean hasOutside;

	if (r_nocull->integer)
		return FRUSTUM_ON;

	hasOutside = false;
	// perform frustum culling
	for (i = 0, fr = vp.frustum; i < 4; i++, fr++)
	{	// loop by frustum planes
		float	dist;

		dist = DotProduct (origin, fr->normal) - fr->dist;
		if (dist < -radius)
			return FRUSTUM_OUTSIDE;
		if (dist <= radius)
			hasOutside = true;
	}
	return hasOutside ? FRUSTUM_ON : FRUSTUM_INSIDE;
}


// Project localOrigin to a world coordinate system
static void ModelToWorldCoord (vec3_t localOrigin, refEntity_t *e, vec3_t center)
{
	VectorMA (e->origin, localOrigin[0], e->axis[0], center);
	VectorMA (center,	 localOrigin[1], e->axis[1], center);
	VectorMA (center,	 localOrigin[2], e->axis[2], center);
}


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


/*-------------- BSP object insertion ----------------*/

/* NOTE:
  - required for ents with shader->sort > OPAQUE (sort alpha-ents)
  - useful for other entity types when occlusion culling will be imlemented
    (insert ents AFTER world culling)
*/

// Find nearest visible leaf, occupied by entity sphere
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
			if (node->frame == visFrame)
				return node;			// visible => mision complete

			if (!sptr)
				return NULL;	// while tree visited, but not leaf found (may happens when r_nocull & ent outside frustum)

			node = stack[--sptr];
			continue;
		}

		dist = DISTANCE_TO_PLANE(origin, node->plane);
		if (dist > radius)
		{
			// side 1
			node = node->children[0];
		}
		else if (dist < -radius)		// can use mradius = -radius for speedup ??
		{
			// side 2
			node = node->children[1];
		}
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


static void AddInlineModelSurfaces (refEntity_t *e)
{
	inlineModel_t	*im;
	surfaceCommon_t	*surf;
	surfacePlanar_t	*pl;
	int		i;

	im = e->model->inlineModel;

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
				gl_cullMode_t glcull;

				glcull = surf->shader->cullMode;
				if (glcull != CULL_NONE)
				{
					plane = &pl->plane;
					dist = DotProduct (plane->normal, e->modelvieworg);
					if (glcull == CULL_FRONT)
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
			if (!r_nocull->integer && e->frustumMask &&
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
		GL_AddSurfaceToPortal (surf, shader, currentEntity);
	}
}


/*------------------ Drawing world -------------------*/


static node_t *WalkBspTree (void)
{
	int		sptr, frustumMask;
	node_t	*node, *firstLeaf, *lastLeaf;

	struct {
		node_t	*node;
		int		frustumMask;
	} stack[MAX_TREE_DEPTH], *st;

	sptr = 0;
	node = map.nodes;
	frustumMask = 0xF;		// check all frustum planes
	firstLeaf = NULL;
	lastLeaf = node;		// just in case

#define PUSH_NODE(n)		\
	st = &stack[sptr++];	\
	st->node = n;			\
	st->frustumMask = frustumMask;

	// if sptr == 0 -- whole tree visited
#define POP_NODE()			\
	if (!sptr)				\
		node = NULL;		\
	else					\
	{						\
		st = &stack[--sptr];			\
		node = st->node;				\
		frustumMask = st->frustumMask;	\
	}

	while (node)			// when whole tree visited - node = NULL
	{
		if (node->frame != visFrame)
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

		//!! check dlights (may require addons to "stack")

		if (node->isNode)
		{	// Q3: child0, than child1; Q1/2: child depends on clipPlane (standard BSP usage)
			if (DISTANCE_TO_PLANE(vp.vieworg, node->plane) < 0)
			{
				// ch[0] than ch[1]
				PUSH_NODE(node->children[1]);
				node = node->children[0];
				continue;
			}
			else
			{
				// ch[1] than ch[0]
				PUSH_NODE(node->children[0]);
				node = node->children[1];
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

		node->frustumMask = frustumMask;
		node->drawEntity = NULL;
		node->drawParticle = NULL;
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

	if (!numEntities || !r_drawentities->integer) return;

	for (i = 0, e = gl_entities + firstEntity; i < numEntities; i++, e++)
	{
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
					if (TransformedBoxCull (im->mins, im->maxs, e) == FRUSTUM_OUTSIDE)
					{
						gl_speeds.cullEnts++;
						continue;
					}

					leaf = SphereLeaf (im->center, im->radius);
				}
				break;
			case MODEL_MD3:
				{
					md3Model_t	*md3;
					md3Frame_t	*frame1, *frame2;
					int		cull1, cull2;
					vec3_t	center1, center2;

					md3 = e->model->md3;

					// frustum culling
					frame1 = md3->frames + e->frame;
					frame2 = md3->frames + e->oldFrame;
					ModelToWorldCoord (frame1->localOrigin, e, center1);
					cull1 = SphereCull (center1, frame1->radius);
					if (frame1 == frame2 && cull1 == FRUSTUM_OUTSIDE)
					{
						gl_speeds.cullEnts++;
						continue;
					}
					else
					{
						ModelToWorldCoord (frame2->localOrigin, e, center2);
						cull2 = SphereCull (center2, frame2->radius);
						if (cull1 == cull2 && cull1 == FRUSTUM_OUTSIDE)
						{	// culled
							gl_speeds.cullEnts++;
							continue;
						}
					}
					leaf = SphereLeaf (center1, frame1->radius);
				}
				break;
			default:
				DrawTextLeft ("Bad model type", 1, 0, 0);
				leaf = NULL;
			}

			if (!leaf)
			{	// entity do not occupy any visible leafs
				gl_speeds.cullEnts2++;
				continue;
			}

			// add entity to leaf's entity list
			e->drawNext = leaf->drawEntity;
			leaf->drawEntity = e;

			SetupModelMatrix (e);
		}
		else
		{
			//?? draw null model or special entities (rail beam etc)
			DrawTextLeft (va("NULL entity %d", i), 1, 0, 0);
		}
	}
}


static void DrawParticles (particle_t *p)
{
	for ( ; p; p = p->next)
	{
		node_t	*leaf;

		leaf = &map.nodes[p->leafNum + map.numNodes];
		if (leaf->frame == visFrame)
		{
			p->drawNext = leaf->drawParticle;
			leaf->drawParticle = p;
		}
		else
			gl_speeds.cullParts++;

		gl_speeds.parts++;
	}
}


static void DrawBspSequence (node_t *leaf)
{
	int		i;
	surfaceCommon_t **psurf;
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

		for (i = 0, psurf = leaf->leafFaces; i < leaf->numLeafFaces; i++, psurf++)
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
				if (!r_nocull->integer && leaf->frustumMask)
				{
#define CULL_PLANE(bit)	\
					if (leaf->frustumMask & (1<<bit) &&	\
						BoxOnPlaneSide(pl->mins, pl->maxs, &vp.frustum[bit]) == 2)	\
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
	}
}


static void GL_MarkLeaves (void)
{
	int		cluster, i;
	node_t	*n;

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

	out->shaderRGBA[3] = (int)(ent->alpha * 255);	//!! should fill RGB too

	// model-specific code and calculate distance to object (to "v")
	if (ent->model)
	{
		switch (ent->model->type)
		{
		case MODEL_INLINE:
			{
				inlineModel_t *im;

				im = ent->model->inlineModel;
				VectorAdd (im->mins, im->maxs, v);
				VectorMA (ent->origin, 0.5, v, im->center);		// middle point in world coordinates
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

	gl_speeds.ents++;
}


/*--------------------------------------------------------------*/


void GL_DrawPortal (void)
{
	node_t	*firstLeaf;
	int		i;
	refEntity_t *e;

	if (r_drawworld->integer && !(gl_refdef.flags & RDF_NOWORLDMODEL))
	{
		drawFrame++;
		gl_speeds.frustLeafs = 0;

		GL_MarkLeaves ();
		ClearBounds (vp.mins, vp.maxs);
		firstLeaf = WalkBspTree ();
		DrawEntities (vp.firstEntity, vp.numEntities);
		DrawParticles (vp.particles);

		DrawBspSequence (firstLeaf);
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
				}
		}
}
