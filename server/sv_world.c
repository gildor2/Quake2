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

	if (ent->area.prev)
		SV_UnlinkEdict (ent);	// unlink from old position (i.e. relink edict)

	if (ent == ge->edicts)
		return;					// don't add the world

	if (!ent->inuse)
		return;

	// set the size
	VectorSubtract (ent->maxs, ent->mins, ent->size);

	// encode the size into the entity_state for client prediction
	if (ent->solid == SOLID_BBOX && !(ent->svflags & SVF_DEADMONSTER))
	{	// assume that x/y are equal and symetric
#if 1
		i = Q_ftol ((ent->maxs[0] + 4) / 8);
		// z is not symetric
		j = Q_ftol ((-ent->mins[2] + 4) / 8);
		// and z maxs can be negative...
		k = Q_ftol ((ent->maxs[2] + 32 + 4) / 8);
#else
		i = floor((ent->maxs[0] + 4) / 8);
		// z is not symetric
		j = floor((-ent->mins[2] + 4) / 8);
		// and z maxs can be negative...
		k = floor((ent->maxs[2] + 32 + 4) / 8);
#endif
		i = bound(i, 1, 31);
		j = bound(j, 1, 31);
		k = bound(k, 1, 63);

		ent->s.solid = (k<<10) | (j<<5) | i;
//		if (ent->s.number >= 2) Com_Printf("^1ADD(%d): %d %d %d (%X)\n", ent->s.number, i, j, k, ent->s.solid);
	}
	else if (ent->solid == SOLID_BSP)
		ent->s.solid = 31;		// a solid_bbox will never create this value
	else
		ent->s.solid = 0;

	// set the abs box
	if (ent->solid == SOLID_BSP && (ent->s.angles[0] || ent->s.angles[1] || ent->s.angles[2]))
	{
		// expand for rotation
		float	max, v;
		int		i;

		max = 0;
		for (i = 0; i < 3; i++)
		{
			v = fabs (ent->mins[i]);
			if (v > max) max = v;
			v = fabs (ent->maxs[i]);
			if (v > max) max = v;
		}
		for (i = 0; i < 3 ; i++)
		{
			ent->absmin[i] = ent->s.origin[i] - max;
			ent->absmax[i] = ent->s.origin[i] + max;
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
		if (check->absmin[0] > area_maxs[0] ||
			check->absmin[1] > area_maxs[1] ||
			check->absmin[2] > area_maxs[2] ||
			check->absmax[0] < area_mins[0] ||
			check->absmax[1] < area_mins[1] ||
			check->absmax[2] < area_mins[2])
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
	edict_t		*touch[MAX_EDICTS], *ent;
	int			i, num;
	int			contents, c2;
	int			headnode;
	float		*angles;

	// get base contents from world
	contents = CM_PointContents (p, sv.models[1]->headnode);

	// or in contents from all the other entities
	num = SV_AreaEdicts (p, p, touch, MAX_EDICTS, AREA_SOLID);

	for (i = 0; i < num; i++)
	{
		ent = touch[i];

		if (ent->solid != SOLID_BSP)
		{
			headnode = CM_HeadnodeForBox (ent->mins, ent->maxs);
			angles = vec3_origin;		// boxes don't rotate
		}
		else
		{
			cmodel_t	*model;

			model = sv.models[ent->s.modelindex];
			if (!model) Com_Error (ERR_FATAL, "MOVETYPE_PUSH with a non bsp model");

			headnode = model->headnode;
			angles = ent->s.angles;
		}

		c2 = CM_TransformedPointContents (p, headnode, ent->s.origin, angles);

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
	edict_t	*touchlist[MAX_EDICTS], *ent;
	trace_t	trace;
	vec3_t	boxmins, boxmaxs;		// enclose the test object along entire move

	// create the bounding box of the entire move
	for (i = 0; i < 3; i++)
	{
		if (end[i] > start[i])
		{
			boxmins[i] = start[i] + mins[i] - 1;
			boxmaxs[i] = end[i] + maxs[i] + 1;
		}
		else
		{
			boxmins[i] = end[i] + mins[i] - 1;
			boxmaxs[i] = start[i] + maxs[i] + 1;
		}
	}

	num = SV_AreaEdicts (boxmins, boxmaxs, touchlist, MAX_EDICTS, AREA_SOLID);

	// be careful, it is possible to have an entity in this
	// list removed before we get to it (killtriggered)
	for (i = 0; i < num; i++)
	{
		float	*angles;
		int		headnode;

		ent = touchlist[i];
		if (ent->solid == SOLID_NOT)
			continue;
		if (ent == passedict)
			continue;
		if (tr->allsolid)
			return;

		if (passedict)
		{
		 	if (ent->owner == passedict)
				continue;	// don't clip against own missiles
			if (passedict->owner == ent)
				continue;	// don't clip against owner
		}

		if (!(contentmask & CONTENTS_DEADMONSTER) && (ent->svflags & SVF_DEADMONSTER))
			continue;

		if (ent->solid != SOLID_BSP)
		{
			int		x, zd, zu;
			vec3_t	bmins, bmaxs;

			if (ent->s.solid)
			{
				// make trace to be equal with client movement prediction code -- see "encoded bbox"
				x = 8 * (ent->s.solid & 31);
				zd = 8 * ((ent->s.solid>>5) & 31);
				zu = 8 * ((ent->s.solid>>10) & 63) - 32;
			}
			else
			{
				// ent->s.solid will be 0 when entity is a SVF_DEADMONSTER
				x = ent->maxs[0];
				zd = -ent->mins[2];
				zu = ent->maxs[2];
			}

			bmins[0] = bmins[1] = -x;
			bmaxs[0] = bmaxs[1] = x;
			bmins[2] = -zd;
			bmaxs[2] = zu;

			headnode = CM_HeadnodeForBox (bmins, bmaxs);
			angles = vec3_origin;		// boxes don't rotate
		}
		else
		{
			cmodel_t	*model;

			model = sv.models[ent->s.modelindex];
			if (!model) Com_Error (ERR_FATAL, "MOVETYPE_PUSH with a non bsp model");

			headnode = model->headnode;
			angles = ent->s.angles;
		}

		trace = CM_TransformedBoxTrace (start, end,	mins, maxs, headnode, contentmask, ent->s.origin, angles);

		if (trace.allsolid || trace.startsolid || trace.fraction < tr->fraction)
		{
			trace.ent = ent;
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
