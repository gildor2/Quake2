//!! this version is slower, than original: seems, AreaEdicts() is useful

/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// world.c -- world query functions

#include "server.h"


typedef struct
{
	qboolean linked;
	edict_t	*owner;
	cmodel_t *model;
	vec3_t	center;
	float	radius;
	vec3_t	axis[3];
	vec3_t	mins, maxs;				// for alias models it is equal to client prediction code bbox
} entityHull_t;

static entityHull_t ents[MAX_EDICTS];


/*
===============================================================================

ENTITY AREA CHECKING

FIXME: this use of "area" is different from the bsp file use
===============================================================================
*/

// (type *)STRUCT_FROM_LINK(link_t *link, type, member)
// ent = STRUCT_FROM_LINK(link,entity_t,order)
// FIXME: remove this mess!
#define	STRUCT_FROM_LINK(l,t,m) ((t *)((byte *)l - (int)&(((t *)0)->m)))

#define	EDICT_FROM_AREA(l) STRUCT_FROM_LINK(l,edict_t,area)

typedef struct areanode_s
{
	int		axis;		// -1 = leaf node
	float	dist;
	struct areanode_s *children[2];
	link_t	trigger_edicts;
	link_t	solid_edicts;
} areanode_t;

#define	AREA_DEPTH	6
#define	AREA_NODES	(1 << (AREA_DEPTH + 1))		// (1<<AREA_DEPTH) for nodes and same count for leafs

static areanode_t sv_areanodes[AREA_NODES];
static int		sv_numareanodes;

static float	*area_mins, *area_maxs;
static edict_t	**area_list;
static int		area_count, area_maxcount;
static int		area_type;

int SV_HullForEntity (edict_t *ent);


// ClearLink is used for new headnodes
static void ClearLink (link_t *l)
{
	l->prev = l->next = l;
}

static void RemoveLink (link_t *l)
{
	l->next->prev = l->prev;
	l->prev->next = l->next;
}

static void InsertLinkBefore (link_t *l, link_t *before)
{
	l->next = before;
	l->prev = before->prev;
	l->prev->next = l;
	l->next->prev = l;
}

/*
===============
SV_CreateAreaNode

Builds a uniformly subdivided tree for the given world size
===============
*/
static areanode_t *SV_CreateAreaNode (int depth, vec3_t mins, vec3_t maxs)
{
	areanode_t	*anode;
	vec3_t		mins1, maxs1, mins2, maxs2;

	anode = &sv_areanodes[sv_numareanodes++];

	ClearLink (&anode->trigger_edicts);
	ClearLink (&anode->solid_edicts);

	if (depth == AREA_DEPTH)
	{
		anode->axis = -1;
		anode->children[0] = anode->children[1] = NULL;
		return anode;
	}

	if (maxs[0] - mins[0] > maxs[1] - mins[1])
		anode->axis = 0;
	else
		anode->axis = 1;

	anode->dist = (maxs[anode->axis] + mins[anode->axis]) / 2.0f;
	VectorCopy (mins, mins1);
	VectorCopy (mins, mins2);
	VectorCopy (maxs, maxs1);
	VectorCopy (maxs, maxs2);

	maxs1[anode->axis] = mins2[anode->axis] = anode->dist;

	anode->children[0] = SV_CreateAreaNode (depth+1, mins2, maxs2);
	anode->children[1] = SV_CreateAreaNode (depth+1, mins1, maxs1);

	return anode;
}

/*
===============
SV_ClearWorld

===============
*/
void SV_ClearWorld (void)
{
	memset (sv_areanodes, 0, sizeof(sv_areanodes));
	memset (ents, 0, sizeof(ents));
	sv_numareanodes = 0;
	if (!sv.models[1])
		return;			// map is not yet loaded (check [1], not [0] ...)
	SV_CreateAreaNode (0, sv.models[1]->mins, sv.models[1]->maxs);
}


/*
===============
SV_UnlinkEdict

===============
*/
void SV_UnlinkEdict (edict_t *ent)
{
	if (!ent->area.prev)
		return;		// not linked in anywhere
	RemoveLink (&ent->area);
	ent->area.prev = ent->area.next = NULL;
	ents[NUM_FOR_EDICT(ent)].linked = false;
}


/*
===============
SV_LinkEdict

===============
*/
#define MAX_TOTAL_ENT_LEAFS		128

void SV_LinkEdict (edict_t *ent)
{
	areanode_t	*node;
	int		leafs[MAX_TOTAL_ENT_LEAFS];
	int		clusters[MAX_TOTAL_ENT_LEAFS];
	int		num_leafs;
	int		i, j, k;
	int		area;
	int		topnode;
	entityHull_t *ex;
	vec3_t	v, tmp;

	if (ent->area.prev)
		SV_UnlinkEdict (ent);	// unlink from old position (i.e. relink edict)

	if (ent == ge->edicts)
		return;					// don't add the world

	if (!ent->inuse)
		return;

	ex = &ents[NUM_FOR_EDICT(ent)];
	ex->owner = ent;
	AnglesToAxis (ent->s.angles, ex->axis);

	// set the size
	VectorSubtract (ent->maxs, ent->mins, ent->size);

	// encode the size into the entity_state for client prediction
	if (ent->solid == SOLID_BBOX && !(ent->svflags & SVF_DEADMONSTER))
	{
		// assume that x/y are equal and symetric
		i = Q_ftol ((ent->maxs[0] + 4) / 8);
		i = bound(i, 1, 31);
		// z is not symetric
		j = Q_ftol ((-ent->mins[2] + 4) / 8);
		j = bound(j, 1, 31);
		// and z maxs can be negative...
		k = Q_ftol ((ent->maxs[2] + 32 + 4) / 8);
		k = bound(k, 1, 63);

		ent->s.solid = (k<<10) | (j<<5) | i;
//		if (ent->s.number >= 2) Com_Printf("^1ADD(%d): %d %d %d (%X)\n", ent->s.number, i, j, k, ent->s.solid);

		i *= 8;
		j *= 8;
		k *= 8;
		VectorSet (ex->mins, -i, -i, -j);
		VectorSet (ex->maxs, i, i, k - 32);
		VectorAdd (ex->maxs, ex->mins, v);
		VectorMA (ent->s.origin, 0.5f, v, ex->center);
		ex->model = NULL;
		ex->radius = VectorDistance (ex->maxs, ex->center);
	}
	else if (ent->solid == SOLID_BSP)
	{
		ex->model = sv.models[ent->s.modelindex];
		if (!ex->model) Com_Error (ERR_FATAL, "MOVETYPE_PUSH with a non bsp model");
		VectorAdd (ex->model->mins, ex->model->maxs, v);
		VectorScale (v, 0.5f, v);
		VectorMA (ent->s.origin, v[0], ex->axis[0], tmp);
		VectorMA (tmp, v[1], ex->axis[1], tmp);
		VectorMA (tmp, v[2], ex->axis[2], ex->center);
		ex->radius = ex->model->radius;

		ent->s.solid = 31;		// a solid_bbox will never create this value
	}
	else
		ent->s.solid = 0;

	ex->linked = true;

	// set the abs box
	if (ent->solid == SOLID_BSP && (ent->s.angles[0] || ent->s.angles[1] || ent->s.angles[2]))
	{
		// expand for rotation
		for (i = 0; i < 3 ; i++)
		{
			ent->absmin[i] = ex->center[i] - ex->radius;
			ent->absmax[i] = ex->center[i] + ex->radius;
		}
	}
	else
	{	// normal
		VectorAdd (ent->s.origin, ent->mins, ent->absmin);
		VectorAdd (ent->s.origin, ent->maxs, ent->absmax);
	}

	// because movement is clipped an epsilon away from an actual edge,
	// we must fully check even when bounding boxes don't quite touch
	ent->absmin[0] -= 1;
	ent->absmin[1] -= 1;
	ent->absmin[2] -= 1;
	ent->absmax[0] += 1;
	ent->absmax[1] += 1;
	ent->absmax[2] += 1;

	// link to PVS leafs
	ent->num_clusters = 0;
	ent->areanum = 0;
	ent->areanum2 = 0;

	// get all leafs, including solids
	num_leafs = CM_BoxLeafnums (ent->absmin, ent->absmax, leafs, MAX_TOTAL_ENT_LEAFS, &topnode);

	// set areas
	for (i = 0; i < num_leafs; i++)
	{
		clusters[i] = CM_LeafCluster (leafs[i]);
		area = CM_LeafArea (leafs[i]);
		if (area)
		{	// doors may legally straggle two areas,
			// but nothing should evern need more than that
			if (ent->areanum && ent->areanum != area)
			{
				if (ent->areanum2 && ent->areanum2 != area && sv.state == ss_loading)
					Com_DPrintf ("Object touching 3 areas at %f %f %f\n",
						ent->absmin[0], ent->absmin[1], ent->absmin[2]);
				ent->areanum2 = area;
			}
			else
				ent->areanum = area;
		}
	}

	if (num_leafs >= MAX_TOTAL_ENT_LEAFS)
	{	// assume we missed some leafs, and mark by headnode
		ent->num_clusters = -1;
		ent->headnode = topnode;
	}
	else
	{
		ent->num_clusters = 0;
		for (i = 0; i < num_leafs; i++)
		{
			if (clusters[i] == -1)
				continue;		// not a visible leaf
			for (j = 0; j < i; j++)
				if (clusters[j] == clusters[i])
					break;
			if (j == i)
			{
				if (ent->num_clusters == MAX_ENT_CLUSTERS)
				{	// assume we missed some leafs, and mark by headnode
					ent->num_clusters = -1;
					ent->headnode = topnode;
					break;
				}

				ent->clusternums[ent->num_clusters++] = clusters[i];
			}
		}
	}

	// if first time, make sure old_origin is valid
	if (!ent->linkcount)
		VectorCopy (ent->s.origin, ent->s.old_origin);
	ent->linkcount++;

	if (ent->solid == SOLID_NOT)
		return;

	// find the first node that the ent's box crosses
	node = sv_areanodes;
	while (1)
	{
		if (node->axis == -1)
			break;		// inside this node
		if (ent->absmin[node->axis] > node->dist)
			node = node->children[0];
		else if (ent->absmax[node->axis] < node->dist)
			node = node->children[1];
		else
			break;		// crosses the node
	}

	// link it in
	if (ent->solid == SOLID_TRIGGER)
		InsertLinkBefore (&ent->area, &node->trigger_edicts);
	else
		InsertLinkBefore (&ent->area, &node->solid_edicts);
}


/*
====================
SV_AreaEdicts

====================
*/
static void SV_AreaEdicts_r (areanode_t *node)
{
	link_t		*l, *next, *start;

	// touch linked edicts
	if (area_type == AREA_SOLID)
		start = &node->solid_edicts;
	else
		start = &node->trigger_edicts;

	for (l = start->next; l != start; l = next)
	{
		edict_t		*check;

		next = l->next;
		check = EDICT_FROM_AREA(l);

		if (check->solid == SOLID_NOT)
			continue;		// deactivated
		if (check->absmin[2] > area_maxs[2] || check->absmax[2] < area_mins[2] ||
			check->absmin[0] > area_maxs[0] || check->absmax[0] < area_mins[0] ||
			check->absmin[1] > area_maxs[1] || check->absmax[1] < area_mins[1])
			continue;		// not touching

		if (area_count == area_maxcount)
		{
			Com_WPrintf ("SV_AreaEdicts: MAXCOUNT\n");
			return;
		}

		area_list[area_count] = check;
		area_count++;
	}

	if (node->axis == -1)
		return;		// terminal node

	// recurse down both sides
	if (area_maxs[node->axis] > node->dist)
		SV_AreaEdicts_r (node->children[0]);
	if (area_mins[node->axis] < node->dist)
		SV_AreaEdicts_r (node->children[1]);
}


int SV_AreaEdicts (vec3_t mins, vec3_t maxs, edict_t **list, int maxcount, int areatype)
{
	area_mins = mins;
	area_maxs = maxs;
	area_list = list;
	area_count = 0;
	area_maxcount = maxcount;
	area_type = areatype;

	SV_AreaEdicts_r (sv_areanodes);

	return area_count;
}


//===========================================================================

/*
=============
SV_PointContents
=============
*/
int SV_PointContents (vec3_t p)
{
	edict_t	*list[MAX_EDICTS], *edict;
	int		i, num, contents, c2;
	entityHull_t *ent;
//	vec3_t	delta;
//	float	dist2;

	// get base contents from world
	contents = CM_PointContents (p, sv.models[1]->headnode);

	num = SV_AreaEdicts (p, p, list, MAX_EDICTS, AREA_SOLID);

	for (i = 0; i < num; i++)
	{
//		float	*mins, *maxs;

		edict = list[i];
		ent = &ents[NUM_FOR_EDICT(edict)];

/*		if (!ent->linked) continue;
		if (ent->owner->solid == SOLID_NOT) continue;		// deactivated blocker

		mins = ent->owner->absmin;
		maxs = ent->owner->absmax;
		// check box
		if (maxs[2] < p[2] || mins[2] > p[2] ||
			maxs[0] < p[0] || mins[0] > p[0] ||
			maxs[1] < p[1] || mins[1] > p[1])
		continue; */

		// check bounding sphere (not needed, because bbox checked before ?)
//		VectorSubtract (ent->center, p, delta);
//		dist2 = DotProduct (delta, delta);
//		if (dist2 > ent->radius * ent->radius) continue;	// too far

		if (ent->model)
			c2 = CM_TransformedPointContents2 (p, ent->model->headnode, edict->s.origin, ent->axis);
		else
			c2 = CM_TransformedPointContents (p, CM_HeadnodeForBox (ent->mins, ent->maxs), edict->s.origin, vec3_origin);
		contents |= c2;
	}

	return contents;
}


//===========================================================================

/*
====================
SV_ClipMoveToEntities

====================
*/
void SV_ClipMoveToEntities (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, edict_t *passedict, int contentmask, trace_t *tr)
{
	int		i, num;
	edict_t	*list[MAX_EDICTS], *edict;
	entityHull_t *ent;
	trace_t	trace;
	float	t, traceLen, traceWidth, b1, b2;
	vec3_t	amins, amaxs, traceDir;
	qboolean test = Cvar_VariableInt("test");//!!

	for (i = 0; i < 3; i++)
	{
		if (start[i] < end[i])
		{
			amins[i] = start[i] + mins[i];
			amaxs[i] = end[i] + maxs[i];
		}
		else
		{
			amins[i] = end[i] + mins[i];
			amaxs[i] = start[i] + maxs[i];
		}
	}
	num = SV_AreaEdicts (amins, amaxs, list, MAX_EDICTS, AREA_SOLID);
	if (!num) return;

if (test) {//!!
	b1 = DotProduct (mins, mins);
	b2 = DotProduct (maxs, maxs);
	t = b1 > b2 ? b1 : b2;
	traceWidth = SQRTFAST(t);
	VectorSubtract (end, start, traceDir);
	traceLen = VectorNormalize (traceDir) + traceWidth;
}//!!

	for (i = 0; i < num; i++)
	{
		vec3_t	center, tmp;
		float	entPos, dist2, dist0;

		edict = list[i];
		ent = &ents[NUM_FOR_EDICT(edict)];
//		if (!ent->linked) continue;

		if (edict->solid == SOLID_NOT || edict == passedict) continue;
		if (passedict)
		{
		 	if (edict->owner == passedict)
				continue;	// don't clip against own missiles
			if (passedict->owner == edict)
				continue;	// don't clip against owner
		}
		if (!(contentmask & CONTENTS_DEADMONSTER) && (edict->svflags & SVF_DEADMONSTER))
			continue;

if (test) { //!!
		VectorSubtract (ent->center, start, center);
		// check position of point projection on line
		entPos = DotProduct (center, traceDir);
		if (entPos < -traceWidth - ent->radius || entPos > traceLen + ent->radius)
			continue;		// too near / too far

		// check distance between point and line
		VectorMA (center, -entPos, traceDir, tmp);
		dist2 = DotProduct (tmp, tmp);
		dist0 = ent->radius + traceWidth;
		if (dist2 >= dist0 * dist0) continue;
}//!!

		if (ent->model)
			trace = CM_TransformedBoxTrace2 (start, end, mins, maxs, ent->model->headnode, contentmask, edict->s.origin, ent->axis);
		else
			trace = CM_TransformedBoxTrace (start, end, mins, maxs,
				CM_HeadnodeForBox (ent->mins, ent->maxs), contentmask, edict->s.origin, vec3_origin);

		if (trace.allsolid || trace.startsolid || trace.fraction < tr->fraction)
		{
			trace.ent = edict;
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


/*
==================
SV_Trace

Moves the given mins/maxs volume through the world from start to end.

Passedict and edicts owned by passedict are explicitly not checked.

==================
*/
trace_t SV_Trace (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, edict_t *passedict, int contentmask)
{
	trace_t trace;

	if (!mins)	mins = vec3_origin;
	if (!maxs)	maxs = vec3_origin;

	// clip to world
	trace = CM_BoxTrace (start, end, mins, maxs, 0, contentmask);

	trace.ent = ge->edicts;
	if (!trace.fraction) return trace;		// blocked by the world

	// clip to other solid entities
	SV_ClipMoveToEntities (start, mins, maxs, end, passedict, contentmask, &trace);

	return trace;
}
