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
// cmodel.c -- model loading

#include "qcommon.h"

typedef struct
{
	cplane_t	*plane;
	int			children[2];		// negative numbers are leafs
} cnode_t;

typedef struct
{
	cplane_t	*plane;
	csurface_t	*surface;
} cbrushside_t;

typedef struct
{
	int			contents;
	int			numsides;
	int			firstbrushside;
	int			checkcount;		// to avoid repeated testings
} cbrush_t;

typedef struct
{
	int		numareaportals;
	int		firstareaportal;
	int		floodnum;			// if two areas have equal floodnums, they are connected
	int		floodvalid;
} carea_t;

int			checkcount;

char		map_name[MAX_QPATH];

int			numbrushsides;
cbrushside_t map_brushsides[MAX_MAP_BRUSHSIDES];

int			numtexinfo;
csurface_t	map_surfaces[MAX_MAP_TEXINFO];

int			numplanes;
cplane_t	map_planes[MAX_MAP_PLANES+6];		// extra for box hull

int			numnodes;
cnode_t		map_nodes[MAX_MAP_NODES+6];		// extra for box hull

int			numleafs = 1;	// allow leaf funcs to be called without a map
dleaf_t		map_leafs[MAX_MAP_LEAFS];
int			emptyleaf, solidleaf;

int			numleafbrushes;
unsigned short map_leafbrushes[MAX_MAP_LEAFBRUSHES];

int			numcmodels;
cmodel_t	map_cmodels[MAX_MAP_MODELS];

int			numbrushes;
cbrush_t	map_brushes[MAX_MAP_BRUSHES];

int			numvisibility;
byte		*map_visibility;

int			numentitychars;
char		*map_entitystring;

int			numareas = 1;
carea_t		map_areas[MAX_MAP_AREAS];

int			numareaportals;
dareaportal_t *map_areaportals;

int			numclusters = 1;

csurface_t	nullsurface;

int			floodvalid;

static qboolean portalopen[MAX_MAP_AREAPORTALS];


cvar_t		*map_noareas;

void	CM_InitBoxHull (void);
void	FloodAreaConnections (void);


int		c_pointcontents;
int		c_traces, c_brush_traces;


/*
** Support for Half-Life maps
*/

static maptype_t maptype;
static int firstnode; // 0->firstclipnode, this->firstnode


/*
===============================================================================

					MATERIAL-BY-TEXTURE

===============================================================================
*/


typedef struct surfMaterial_s
{
	char	*name;			// full name or wildcard
	struct surfMaterial_s *next;
	material_t material;
} surfMaterial_t;


static void *surfMaterialChain;
static surfMaterial_t *surfMaterialList;


static void CMod_ReadSurfMaterials (char *filename)
{
	char	*file, *in, *s;
	surfMaterial_t *prev;

	if (surfMaterialChain)
	{
		FreeMemoryChain (surfMaterialChain);
		surfMaterialChain = NULL;
		surfMaterialList = NULL;
	}

	FS_LoadFile (filename, &file);
	if (!file)
	{
		Com_DPrintf ("ReadSurfMaterials: %s is not found\n", filename);
		return;
	}

	surfMaterialChain = CreateMemoryChain ();
	prev = NULL;
	in = file;

	while (s = COM_Parse (&in), in)
	{
		char		c;
		material_t	m;
		surfMaterial_t *sm;

		// s points to material name string
		c = s[0];
		if (c >= 'A' && c <= 'Z') c += 32;		// lowercase
		switch (c)
		{
		case 'm':
			m = MATERIAL_METAL;
			break;
		case 't':
			m = MATERIAL_TILE;
			break;
		case 'y':
			m = MATERIAL_GLASS;
			break;
		case 'l':
			m = MATERIAL_GRAVEL;
			break;
		case 'd':
			m = MATERIAL_DIRT;
			break;
		case 'c':
			m = MATERIAL_CONCRETE;
			break;
		case 'w':
			m = MATERIAL_WOOD;
			break;
		case 'n':
			m = MATERIAL_SNOW;
			break;
		case 'z':
			m = MATERIAL_SILENT;
			break;
		default:
			Com_WPrintf ("Unknown material mark \"%s\" in %s\n", s, filename);
			m = MATERIAL_CONCRETE;
		}

		s = COM_Parse (&in);
		if (!in)
		{
			Com_WPrintf ("Unexpected end of file %s\n", filename);
			break;
		}
		// s points to surface name
		sm = (surfMaterial_t*) ChainAllocNamedStruc (sizeof(surfMaterial_t), s, surfMaterialChain);
		sm->material = m;
		// add to list
		if (!prev)
			surfMaterialList = sm;
		else
			prev->next = sm;
		prev = sm;

//		Com_DPrintf ("Added surface material: %i for %s\n", m, s);
	}

	FS_FreeFile (file);
}

// name must be without "textures/" prefix and without extension
material_t CMod_GetSurfMaterial (char *name)
{
	surfMaterial_t *sm;
	char	*checkname;		// points to a name without path

	checkname = strrchr (name, '/');
	if (checkname)
		checkname++;
	else
		checkname = name;

	for (sm = surfMaterialList; sm; sm = sm->next)
	{
		char	*s;
		material_t	m;

		s = sm->name;
		if (strchr (s, '/'))
		{	// mask have a path separator - compare full names
			if (!MatchWildcard (name, sm->name))
				continue;
		}
		else
		{	// compare without path
			if (!MatchWildcard (checkname, sm->name))
				continue;
		}

		m = sm->material;
//		Com_DPrintf ("set material %d for %s\n", m, name);
		return m;
	}
//	Com_DPrintf ("set default material for %s\n", name);
	return MATERIAL_CONCRETE;	// default material
}


/*
===============================================================================

					MAP LOADING

===============================================================================
*/

/*
=================
CMod_LoadSubmodels
=================
*/
static void CMod_LoadSubmodels (dmodel_t *data, int count)
{
	cmodel_t	*out;
	int			i;

	if (count < 1)
		Com_Error (ERR_DROP, "Map with no models");
	if (count > MAX_MAP_MODELS)
		Com_Error (ERR_DROP, "Map has too many models");

	out = map_cmodels;
	numcmodels = count;

	for (i = 0; i < count; i++, data++, out++)
	{
		vec3_t	tmp;

		VectorCopy (data->mins, out->mins);
		VectorCopy (data->maxs, out->maxs);
		VectorSubtract (out->maxs, out->mins, tmp);
		out->radius = VectorLength (tmp) / 2;
//??		VectorCopy (data->origin, out->origin);
		out->headnode = data->headnode;
	}
}


/*
=================
CMod_LoadSurfaces
=================
*/

static void CMod_LoadSurfaces (texinfo_t *data, int size)
{
	texinfo_t	*in;
	csurface_t	*out;
	int			i;

	in = data;

	if (size < 1)
		Com_Error (ERR_DROP, "Map with no surfaces");
	if (size > MAX_MAP_TEXINFO)
		Com_Error (ERR_DROP, "Map has too many surfaces");

	CMod_ReadSurfMaterials ("sound/materials.lst");

	numtexinfo = size;
	out = map_surfaces;

	for (i = 0; i < size; i++, in++, out++)
	{
		int		f, m;

		strncpy (out->name, in->texture, sizeof(out->name)-1);		// texture name limited to 16 chars (compatibility with older mods?)
		strncpy (out->rname, in->texture, sizeof(out->rname)-1);	// name limited to 32 chars (full?)
		f = in->flags;
		out->flags = f & ~SURF_KP_MATERIAL;
		out->value = in->value;
		if (f & SURF_CONCRETE)
			m = MATERIAL_CONCRETE;
		else if (f & SURF_FABRIC)
			m = MATERIAL_FABRIC;
		else if (f & SURF_GRAVEL)
			m = MATERIAL_GRAVEL;
		else if (f & SURF_METAL)
			m = MATERIAL_METAL;
		else if (f & SURF_METAL_L)
			m = MATERIAL_METAL_L;
		else if (f & SURF_SNOW)
			m = MATERIAL_TIN;		//?? MATERIAL_SNOW;
		else if (f & SURF_TILE)
			m = MATERIAL_TILE;
		else if (f & SURF_WOOD)
			m = MATERIAL_WOOD;
		else if (f & SURF_WATER)
			m = MATERIAL_WATER;
		else
			m = CMod_GetSurfMaterial (in->texture);
//			m = MATERIAL_CONCRETE;	// default (Q2)
		out->material = m;
		if (m > 100)
			Com_Printf("Surf %s have material %d\n",in->texture,m);
	}
}


/*
=================
CMod_LoadNodes

=================
*/
static void CMod_LoadNodes (dnode_t *data, int size)
{
	dnode_t *in;
	cnode_t *out;
	int		i;

	in = data;

	if (size < 1)
		Com_Error (ERR_DROP, "Map has no nodes");
	else if (size > MAX_MAP_NODES)
		Com_Error (ERR_DROP, "Map has too many nodes");

	out = map_nodes;

	numnodes = size;

	for (i = 0; i < size; i++, out++, in++)
	{
		out->plane = map_planes + in->planenum;
		out->children[0] = in->children[0];
		out->children[1] = in->children[1];
	}

}

/*
=================
CMod_LoadBrushes

=================
*/
static void CMod_LoadBrushes (dbrush_t *data, int size)
{
	dbrush_t	*in;
	cbrush_t	*out;
	int			i;

	in = data;

	if (size > MAX_MAP_BRUSHES)
		Com_Error (ERR_DROP, "Map has too many brushes");

	out = map_brushes;

	numbrushes = size;

	for (i = 0; i < size; i++, out++, in++)
	{
		out->firstbrushside = in->firstside;
		out->numsides = in->numsides;
		out->contents = in->contents;
	}
}

/*
=================
CMod_LoadLeafs
=================
*/
static void CMod_LoadLeafs (dleaf_t *data, int size)
{
	int			i;
	dleaf_t 	*in;

	if (size < 1)
		Com_Error (ERR_DROP, "Map with no leafs");
	if (size > MAX_MAP_LEAFS)
		Com_Error (ERR_DROP, "Map has too many leafs");

	memcpy (map_leafs, data, sizeof(dleaf_t)*size); // must perform this, because needs 1 more leaf for InitBoxHull()
//--	map_leafs = data;
	numleafs = size;

	in = data;
	numclusters = 0;

	for (i = 0; i < size; i++, in++)
		if (in->cluster >= numclusters)
			numclusters = in->cluster + 1;

	if (map_leafs[0].contents != CONTENTS_SOLID)
		Com_Error (ERR_DROP, "Map leaf 0 is not CONTENTS_SOLID");
	solidleaf = 0;
	emptyleaf = -1;
	for (i = 1; i < numleafs; i++)
	{
		if (!map_leafs[i].contents)
		{
			emptyleaf = i;
			break;
		}
	}
	if (emptyleaf == -1)
		Com_Error (ERR_DROP, "Map does not have an empty leaf");
}

/*
=================
CMod_LoadPlanes
=================
*/
static void CMod_LoadPlanes (dplane_t *data, int size)
{
	int			i;
	cplane_t	*out;
	dplane_t 	*in;

	in = data;

	if (size < 1)
		Com_Error (ERR_DROP, "Map with no planes");
	if (size > MAX_MAP_PLANES)
		Com_Error (ERR_DROP, "Map has too many planes");

	out = map_planes;
	numplanes = size;

	for (i = 0; i < size; i++, in++, out++)
	{
		VectorCopy (in->normal, out->normal);
		out->dist = in->dist;
		out->type = in->type;
		SetPlaneSignbits (out);
	}
}

/*
=================
CMod_LoadLeafBrushes
=================
*/
static void CMod_LoadLeafBrushes (unsigned short *data, int size)
{
	if (size < 1)
		Com_Error (ERR_DROP, "Map with no leafbrushes");
	if (size > MAX_MAP_LEAFBRUSHES)
		Com_Error (ERR_DROP, "Map has too many leafbrushes");

//	map_leafbrushes = data; -- can't do this, because we need 1 more for InitBoxHull()
	numleafbrushes = size;
	memcpy (map_leafbrushes, data, sizeof(*data)*size);
}

/*
=================
CMod_LoadBrushSides
=================
*/
static void CMod_LoadBrushSides (dbrushside_t *data, int size)
{
	int			i, j;
	cbrushside_t	*out;
	dbrushside_t 	*in;

	in = data;

	if (size > MAX_MAP_BRUSHSIDES)
		Com_Error (ERR_DROP, "Map has too many brushsides");

	out = map_brushsides;
	numbrushsides = size;

	for (i = 0; i < size; i++, in++, out++)
	{
		out->plane = &map_planes[in->planenum];
		j = in->texinfo;
		if (j >= numtexinfo)
			Com_Error (ERR_DROP, "Bad brushside texinfo");
		out->surface = &map_surfaces[j];
	}
}

/*
=================
CMod_LoadAreas
=================
*/
static void CMod_LoadAreas (darea_t *data, int size)
{
	int			i;
	carea_t		*out;
	darea_t 	*in;

	if (size > MAX_MAP_AREAS)
		Com_Error (ERR_DROP, "Map has too many areas");

	in = data;
	out = map_areas;
	numareas = size;

	for (i = 0; i < size; i++, in++, out++)
	{
		out->numareaportals = in->numareaportals;
		out->firstareaportal = in->firstareaportal;
		out->floodvalid = 0;
		out->floodnum = 0;
	}
}

/*
=================
CMod_LoadAreaPortals
=================
*/
static void CMod_LoadAreaPortals (dareaportal_t *data, int size)
{
//	if (count > MAX_MAP_AREAPORTALS)
//		Com_Error (ERR_DROP, "Map has too many areaportals");

	numareaportals = size;
	map_areaportals = data;
}

/*
=================
CMod_LoadVisibility
=================
*/
static void CMod_LoadVisibility (dvis_t *data, int size)
{
	numvisibility = size;
	map_visibility = (byte *) data;
}


/*
=================
CMod_LoadEntityString
=================
*/
static void CMod_LoadEntityString (byte *data, int size)
{
	numentitychars = size;
//	if (l->filelen > MAX_MAP_ENTSTRING)
//		Com_Error (ERR_DROP, "Map has too large entity lump");
	map_entitystring = data;
//?? can't do it now (should copy entstring with ASCIIZ in LoadBspFile()):	map_entitystring[l->filelen] = 0; // ASCIIZ
}



/*
==================

HALF-LIFE map support

==================
*/


void CMod_LoadHLSurfaces (lump_t *l)
{
/*	hl_texinfo_t	*in;
	csurface_t	*out;
	int			i, count;

//--	in = (void *)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Com_Error (ERR_DROP, "MOD_LoadBmodel: funny lump size");
	count = l->filelen / sizeof(*in);
	if (count < 1)
		Com_Error (ERR_DROP, "Map with no surfaces");
	if (count > MAX_MAP_TEXINFO)
		Com_Error (ERR_DROP, "Map has too many surfaces");

	numtexinfo = count;
	out = map_surfaces;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		char buf[32];//!!
		Com_sprintf(buf,sizeof(buf),"surf_%x",i);
		strcpy (out->name, buf); //!!
		strcpy (out->rname, buf);  //!!
//??		strncpy (out->name, in->texture, sizeof(out->name)-1);
//??		strncpy (out->rname, in->texture, sizeof(out->rname)-1);
		out->flags = LittleLong (in->flags);
		out->value = 0; // LittleLong (in->value); - no in HL
	} */
}


#define HL_CONTENTS 15
static int HLContents[HL_CONTENTS] =
{
	0,
	CONTENTS_SOLID,
	CONTENTS_WATER,
	CONTENTS_SLIME,
	CONTENTS_LAVA,
	CONTENTS_MONSTER, // SKY
	CONTENTS_ORIGIN, // removed while bsp
	CONTENTS_SOLID,  // CLIP
	CONTENTS_CURRENT_0 | CONTENTS_WATER,
	CONTENTS_CURRENT_90 | CONTENTS_WATER,
	CONTENTS_CURRENT_180 | CONTENTS_WATER,
	CONTENTS_CURRENT_270 | CONTENTS_WATER,
	CONTENTS_CURRENT_UP | CONTENTS_WATER,
	CONTENTS_CURRENT_DOWN | CONTENTS_WATER,
	CONTENTS_TRANSLUCENT
};

void CMod_LoadHLLeafs (lump_t *l)
{
/*	hl_dleaf_t 	*in;
	dleaf_t 	*out;
	int		i, count, p;

//--	in = (void *)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Com_Error (ERR_DROP, "MOD_LoadBmodel: funny lump size");
	count = l->filelen / sizeof(*in);

	if (count < 1)
		Com_Error (ERR_DROP, "Map with no leafs");
	// need to save space for box planes
	if (count > MAX_MAP_LEAFS)
		Com_Error (ERR_DROP, "Map has too many leafs");

	out = map_leafs;
	numleafs = count;
	numclusters = count;

	for (i = 0; i < count; i++, in++, out++)
	{
		p = LittleLong(in->contents);
		if (p < 0 && p >= -HL_CONTENTS)
			p = HLContents[-p - 1];
		else
		{
			Com_Error (ERR_DROP, "Unknown leaf contents: %d", p);
		}

		out->contents = p;
		out->cluster = LittleLong(in->visofs); // will be used later in Mod_LoadHLVisibility()
		out->area = 0; // no areas in Q1/HL

		out->firstleafbrush = LittleShort(in->firstmarksurface); // temporary store (for LoadHLBrushes())
		out->numleafbrushes = LittleShort(in->nummarksurfaces);  //
	}

	if (map_leafs[0].contents != CONTENTS_SOLID)
		Com_Error (ERR_DROP, "Map leaf 0 is not CONTENTS_SOLID");
	solidleaf = 0;
	emptyleaf = -1;
	for (i=1 ; i<numleafs ; i++)
	{
		if (!map_leafs[i].contents)
		{
			emptyleaf = i;
			break;
		}
	}
	if (emptyleaf == -1)
		Com_Error (ERR_DROP, "Map does not have an empty leaf"); */
}


/*
 * Generate dvis_t structure { numclusters: int; bitofs: array[][2] of int }
 * SHOULD BE CALLED AFTER CMod_LoadHLLeafs() !
 */
void CMod_LoadHLVisibility (lump_t *l)
{
/*	int		i, size, vis;

	numvisibility = l->filelen;
	if (l->filelen > MAX_MAP_VISIBILITY)
		Com_Error (ERR_DROP, "Map has too large visibility lump");

	if (!numvisibility) return;

	size = 4 + 8 * numclusters; // sizeof(dvis_t)
	memcpy ((char*)map_visibility + size, cmod_base + l->fileofs, l->filelen);

	for (i = 0 ; i < numclusters; i++)
	{
		vis = map_leafs[i].cluster; // this field was temporarily stored visofs
		if (vis != -1) vis += size; // -1 will be -1
		map_vis->bitofs[i][DVIS_PVS] = vis;
		map_vis->bitofs[i][DVIS_PHS] = -1; // no visinfo
		map_leafs[i].cluster = i;
	} */
}


void CMod_LoadHLSubmodels (lump_t *l)
{
/*	hl_dmodel_t	*in;
	cmodel_t	*out;
	int		i, j, count;

//--	in = (void *)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Com_Error (ERR_DROP, "MOD_LoadBmodel: funny lump size");
	count = l->filelen / sizeof(*in);

	if (count < 1)
		Com_Error (ERR_DROP, "Map with no models");
	if (count > MAX_MAP_MODELS)
		Com_Error (ERR_DROP, "Map has too many models");

	numcmodels = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		out = &map_cmodels[i];

		for (j=0 ; j<3 ; j++)
		{	// spread the mins / maxs by a pixel
			out->mins[j] = LittleFloat (in->mins[j]) - 1;
			out->maxs[j] = LittleFloat (in->maxs[j]) + 1;
		}
		out->headnode = LittleLong (in->headnode[0]); // use only headnode[0]
	}
	if (map_cmodels[0].headnode)
		Com_Error (ERR_DROP, "HL map has invalid headnode = %i", map_cmodels[0].headnode); */
}


void CMod_LoadHLClipnodes (lump_t *l)
{
/*	hl_dclipnode_t	*in;
	cnode_t		*out;
	int		i, j, count, child;
	dleaf_t		*leaf;

//--	in = (void *)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Com_Error (ERR_DROP, "MOD_LoadBmodel: funny lump size");
	count = l->filelen / sizeof(*in);

	if (count < 1)
		Com_Error (ERR_DROP, "Map has no nodes");
	if (count > MAX_MAP_NODES)
		Com_Error (ERR_DROP, "Map has too many nodes");

	out = map_nodes;

	numnodes = count;

	for (i=0 ; i<count ; i++, out++, in++)
	{
		out->plane = map_planes + LittleLong(in->planenum);
		for (j=0 ; j<2 ; j++)
		{
			child = LittleShort(in->children[j]);
			if (child >= 0) // clipnode
				out->children[j] = child;
			else
			{	// child = HL_CONTENTS - create leaf for it ...
				if (numleafs + 1 >= MAX_MAP_LEAFS)
					Com_Error (ERR_DROP, "Mod_LoadHLClipnodes: not enough space to generate leafs");

				out->children[j] = -1 - numleafs; // points to leaf, that will be created below

				leaf = &map_leafs[numleafs++];

				if (child < 0 && child >= -HL_CONTENTS)
					child = HLContents[-child - 1];
				else
					Com_Error (ERR_DROP, "Unknown node contents: %d", child);
				leaf->contents = child;
				leaf->cluster = i;//!! -2; // -1 - no cluster, -2 - calculate with nodes/leafs
				leaf->area = j;//!! 0; // HL map has no areas
				leaf->firstleafbrush = 0;
				leaf->numleafbrushes = 0;
			}
		}
	} */
}


void CMod_LoadHLNodes (lump_t *l)
{
/*	hl_dnode_t	*in;
	cnode_t		*out;
	int		i, j, count, child;

//--	in = (void *)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Com_Error (ERR_DROP, "MOD_LoadBmodel: funny lump size");
	count = l->filelen / sizeof(*in);

	if (count < 1)
		Com_Error (ERR_DROP, "Map has no nodes");
	if (count + numnodes > MAX_MAP_NODES)
		Com_Error (ERR_DROP, "Map has too many nodes");

	firstnode = 0; //?? numnodes; // place nodes after clipnodes
	//?? if all OK, remove "firstnode"
	out = &map_nodes[firstnode];

	numnodes = count;

	for (i=0 ; i<count ; i++, out++, in++)
	{
		out->plane = map_planes + LittleLong(in->planenum);
		for (j=0 ; j<2 ; j++)
		{
			child = LittleShort(in->children[j]);
			if (child < 0)
				out->children[j] = child; // leaf (cluster)
			else
				out->children[j] = child + firstnode; // node (shift to clipnodes count)
		}
	} */
}


void CMod_LoadHLAreas ()
{
/*	numareas = 1;
	map_areas[0].numareaportals = 0;
	map_areas[0].firstareaportal = 0;
	map_areas[0].floodvalid = 0;
	map_areas[0].floodnum = 0; */
}


// Generate brushes
void CMod_LoadHLBrushes ()
{
/*	dleaf_t		*in;
	cbrush_t	*out;
	int			i, count;

	in = map_leafs;
	count = numleafs;
	out = map_brushes;
	numbrushes = count;

	if (count > MAX_MAP_BRUSHES)
		Com_Error (ERR_DROP, "Map has too many brushes");

	for (i = 0; i < count; i++, out++, in++)
	{
		out->firstbrushside = in->firstleafbrush; // hl_dleaf_t->firstmarksurface
		out->numsides = in->numleafbrushes;       // hl_dleaf_t->nummarksurfaces
		Com_Printf(",%d",out->numsides); //!!
		in->firstleafbrush = i; // points to current structure
		in->numleafbrushes = 1;
		out->contents = in->contents;
	} */
}


// Generate brushsides from FACES, MARKSURFACES, VERTEXES, EDGES and SURFEDGES
void CMod_LoadHLBrushSides (lump_t *lf, lump_t *lm, lump_t *lv, lump_t *le, lump_t *lse)
{
/*	int		i;
	cbrush_t	*in;
	cbrushside_t	*out;
	dface_t 	*faces;
	short		*marksurfs;
	dvertex_t	*verts;
	dedge_t		*edges;
	int		*surfedges;
	int		count, numfaces, nummarksurfs, numverts, numedges, numsurfedges;

	// check lumps and set up lump data pointers
	if (lf->filelen % sizeof(*faces) || lm->filelen % sizeof(*marksurfs) ||
	    lv->filelen % sizeof(*verts) || le->filelen % sizeof(*edges) ||
	    lse->filelen % sizeof(*surfedges))
		Com_Error (ERR_DROP, "MOD_LoadBmodel: funny lump size");

//--	faces = (void *)(cmod_base + lf->fileofs);
	numfaces = lf->filelen / sizeof(*faces);

//--	marksurfs = (void *)(cmod_base + lm->fileofs);
	nummarksurfs = lm->filelen / sizeof(*marksurfs);

//--	verts = (void *)(cmod_base + lv->fileofs);
	numverts = lv->filelen / sizeof(*verts);

//--	edges = (void *)(cmod_base + le->fileofs);
	numedges = le->filelen / sizeof(*edges);

//--	surfedges = (void *)(cmod_base + lse->fileofs);
	numsurfedges = lse->filelen / sizeof(*surfedges);

	out = map_brushsides;
	count = 0;
	in = map_brushes;

	for (i = 0 ; i < numbrushes; i++, in++)
	{
		int	k, n, brushside, vert_ct;
		vec3_t	avg;

		n = in->numsides; // nummarksurfaces
		brushside = in->firstbrushside;  // firstbrushside == nummarksurfaces
		in->firstbrushside = count;

		if (brushside + n > nummarksurfs)
			Com_Error (ERR_DROP, "Bad marksurface");

		VectorClear (avg);
		vert_ct = 0;

		in->numsides = 0; //??

		for (k = 0; k < n; k++) // out++, count++
		{
			int	j, num, n1, m;
			dface_t	*face;

			if (count >= MAX_MAP_BRUSHSIDES)
				Com_Error (ERR_DROP, "Map has too many brushsides");

			num = LittleShort (marksurfs[brushside + k]); // number of face
			if (num > numfaces)
				Com_Error (ERR_DROP, "Bad brushside face");
			face = &faces[num];

			//?? skip planebacks
//??			if (LittleShort (face->side)) continue;

			num = LittleShort (face->planenum);
			out->plane = &map_planes[num];
			j = LittleShort (face->texinfo);
			if (j >= numtexinfo)
				Com_Error (ERR_DROP, "Bad brushside texinfo");
			out->surface = &map_surfaces[j];

			out++;
			count++;
			in->numsides++; //??
		}
	}
	numbrushsides = count; */
}


// generate leafbrushes
void CMod_LoadHLLeafBrushes ()
{
/*	int	i, count;

	count = numleafs;

	if (count > MAX_MAP_LEAFBRUSHES)
		Com_Error (ERR_DROP, "Map has too many leafbrushes");

	numleafbrushes = count;

	for (i = 0; i < count; i++)
		map_leafbrushes[i] = i; */
}

/*
void CM_LoadHLMap (char *name, void *buf)
{
	int	i;
	hl_dheader_t	*header;

	header = (hl_dheader_t *)buf;

	maptype = map_hl;

	for (i=0 ; i<sizeof(hl_dheader_t)/4 ; i++)
		((int *)header)[i] = LittleLong ( ((int *)header)[i]);

	CMod_LoadHLSurfaces (&header->lumps[HL_LUMP_TEXINFO]);

	CMod_LoadHLLeafs (&header->lumps[HL_LUMP_LEAFS]);
	CMod_LoadHLVisibility (&header->lumps[HL_LUMP_VISIBILITY]);

	CMod_LoadPlanes (&header->lumps[HL_LUMP_PLANES]);	// same as Q2

	CMod_LoadHLBrushes (); // generate from LEAFS
	CMod_LoadHLBrushSides (&header->lumps[HL_LUMP_FACES],
		&header->lumps[HL_LUMP_MARKSURFACES],
		&header->lumps[HL_LUMP_VERTEXES],
		&header->lumps[HL_LUMP_EDGES],
		&header->lumps[HL_LUMP_SURFEDGES]);
	CMod_LoadHLLeafBrushes ();

	CMod_LoadHLSubmodels (&header->lumps[HL_LUMP_MODELS]);

//??	CMod_LoadHLClipnodes (&header->lumps[HL_LUMP_CLIPNODES]);
	CMod_LoadHLNodes (&header->lumps[HL_LUMP_NODES]);
	CMod_LoadHLAreas (); // simulate loading (create 1 area)
	numareaportals = 0;
	CMod_LoadEntityString (&header->lumps[HL_LUMP_ENTITIES]);

	CM_InitBoxHull ();

	memset (portalopen, 0, sizeof(portalopen));
	FloodAreaConnections ();

	strcpy (map_name, name);
	numtexinfo = 0; // disable texture precaching for half-life maps
}
*/

/*
==================
CM_LoadMap

Loads in the map and all submodels
==================
*/
cmodel_t *CM_LoadMap (char *name, qboolean clientload, unsigned *checksum)
{
	static unsigned	last_checksum;
	bspfile_t	*bsp;

	map_noareas = Cvar_Get ("map_noareas", "0", 0);

	if (map_name[0] && !stricmp (map_name, name) && (clientload || !Cvar_VariableInt ("flushmap")))
	{
		*checksum = last_checksum;
		if (!clientload)
		{
			memset (portalopen, 0, sizeof(portalopen));
			FloodAreaConnections ();
		}
		return &map_cmodels[0];		// still have the right version
	}

	// free old stuff
	numplanes = 0;
	numnodes = 0;
	numleafs = 0;
	numcmodels = 0;
	numvisibility = 0;
	numentitychars = 0;
	map_entitystring = "";
	map_name[0] = 0;

	if (!name || !name[0])
	{
		numleafs = 1;
		numclusters = 1;
		numareas = 1;
		*checksum = 0;
		return &map_cmodels[0];			// cinematic servers won't have anything at all
	}

	bsp = LoadBspFile (name, clientload, &last_checksum);
	*checksum = last_checksum;

	maptype = bsp->type;

	CMod_LoadSurfaces (bsp->texinfo, bsp->numTexinfo);
	CMod_LoadLeafs (bsp->leafs, bsp->numLeafs);
	CMod_LoadLeafBrushes (bsp->leafbrushes, bsp->numLeafbrushes);
	CMod_LoadPlanes (bsp->planes, bsp->numPlanes);
	CMod_LoadBrushes (bsp->brushes, bsp->numBrushes);
	CMod_LoadBrushSides (bsp->brushsides, bsp->numBrushsides);
	CMod_LoadSubmodels (bsp->models, bsp->numModels);
	CMod_LoadNodes (bsp->nodes, bsp->numNodes);
	CMod_LoadAreas (bsp->areas, bsp->numAreas);
	CMod_LoadAreaPortals (bsp->areaportals, bsp->numAreaportals);
	CMod_LoadVisibility (bsp->visibility, bsp->visDataSize);
	CMod_LoadEntityString (bsp->entities, bsp->entDataSize);

	CM_InitBoxHull ();

	memset (portalopen, 0, sizeof(portalopen));
	FloodAreaConnections ();

	strcpy (map_name, name);

	return &map_cmodels[0];
}

/*
==================
CM_InlineModel
==================
*/
cmodel_t *CM_InlineModel (char *name)
{
	int		num;

	if (!name || name[0] != '*') Com_Error (ERR_DROP, "CM_InlineModel: bad name");

	num = atoi (name+1);
	if (num < 1 || num >= numcmodels) Com_Error (ERR_DROP, "CM_InlineModel: bad number");

	return &map_cmodels[num];
}

int CM_NumClusters (void)
{
	return numclusters;
}

int CM_NumInlineModels (void)
{
	return numcmodels;
}

char *CM_EntityString (void)
{
	return map_entitystring;
}

int CM_LeafContents (int leafnum)
{
	if (leafnum < 0 || leafnum >= numleafs) Com_Error (ERR_DROP, "CM_LeafContents: bad number");
	return map_leafs[leafnum].contents;
}

int CM_LeafCluster (int leafnum)
{
	if (leafnum < 0 || leafnum >= numleafs) Com_Error (ERR_DROP, "CM_LeafCluster: bad number");
	return map_leafs[leafnum].cluster;
}

int CM_LeafArea (int leafnum)
{
	if (leafnum < 0 || leafnum >= numleafs) Com_Error (ERR_DROP, "CM_LeafArea: bad number");
	return map_leafs[leafnum].area;
}

//=======================================================================


cplane_t	*box_planes;
int			box_headnode;
cbrush_t	*box_brush;
dleaf_t		*box_leaf;

/*
===================
CM_InitBoxHull

Set up the planes and nodes so that the six floats of a bounding box
can just be stored out and get a proper clipping hull structure.
===================
*/
void CM_InitBoxHull (void)
{
	int			i;
	int			side;
	cnode_t		*c;
	cplane_t	*p;
	cbrushside_t	*s;

	box_headnode = numnodes;
	box_planes = &map_planes[numplanes];
	if (numnodes+6 > MAX_MAP_NODES
		|| numbrushes+1 > MAX_MAP_BRUSHES
		|| numleafbrushes+1 > MAX_MAP_LEAFBRUSHES
		|| numbrushsides+6 > MAX_MAP_BRUSHSIDES
		|| numplanes+12 > MAX_MAP_PLANES)
		Com_Error (ERR_DROP, "Not enough room for box tree");

	box_brush = &map_brushes[numbrushes];
	box_brush->numsides = 6;
	box_brush->firstbrushside = numbrushsides;
	box_brush->contents = CONTENTS_MONSTER;

	box_leaf = &map_leafs[numleafs];
	box_leaf->contents = CONTENTS_MONSTER;
	box_leaf->firstleafbrush = numleafbrushes;
	box_leaf->numleafbrushes = 1;

	map_leafbrushes[numleafbrushes] = numbrushes;

	for (i=0 ; i<6 ; i++)
	{
		side = i&1;

		// brush sides
		s = &map_brushsides[numbrushsides+i];
		s->plane = map_planes + (numplanes+i*2+side);
		s->surface = &nullsurface;

		// nodes
		c = &map_nodes[box_headnode+i];
		c->plane = map_planes + (numplanes+i*2);
		c->children[side] = -1 - emptyleaf;	// one child -> empty leaf
		if (i != 5)
			c->children[side^1] = box_headnode+i + 1;	// another -> next node
		else
			c->children[side^1] = -1 - numleafs;		// ... or -> box_leaf

		// planes
		p = &box_planes[i*2];
		p->type = i>>1;
		VectorClear (p->normal);
		p->normal[i>>1] = 1;
		SetPlaneSignbits (p);

		p = &box_planes[i*2+1];
		p->type = 3 + (i>>1);
		VectorClear (p->normal);
		p->normal[i>>1] = -1;
		SetPlaneSignbits (p);
	}
}


/*
===================
CM_HeadnodeForBox

To keep everything totally uniform, bounding boxes are turned into small
BSP trees instead of being compared directly.
===================
*/
int	CM_HeadnodeForBox (vec3_t mins, vec3_t maxs)
{
	box_planes[0].dist = maxs[0];
	box_planes[1].dist = -maxs[0];
	box_planes[2].dist = mins[0];
	box_planes[3].dist = -mins[0];
	box_planes[4].dist = maxs[1];
	box_planes[5].dist = -maxs[1];
	box_planes[6].dist = mins[1];
	box_planes[7].dist = -mins[1];
	box_planes[8].dist = maxs[2];
	box_planes[9].dist = -maxs[2];
	box_planes[10].dist = mins[2];
	box_planes[11].dist = -mins[2];

	return box_headnode;
}


/*
==================
CM_PointLeafnum_r

==================
*/
static int CM_PointLeafnum_r (vec3_t p, int num)
{
	float		d;
	cnode_t		*node;
	cplane_t	*plane;

	while (num >= 0)
	{
		node = map_nodes + num;
		plane = node->plane;

		d = DISTANCE_TO_PLANE(p,plane);

		if (d < 0)
			num = node->children[1];
		else
			num = node->children[0];
	}

	c_pointcontents++;		// optimize counter

	return -1 - num;
}

int CM_PointLeafnum (vec3_t p)
{
	if (!numplanes)
		return 0;		// sound may call this without map loaded
	return CM_PointLeafnum_r (p, 0);
}



/*
=============
CM_BoxLeafnums

Fills in a list of all the leafs touched
=============
*/
int		leaf_count, leaf_maxcount;
int		*leaf_list;
float	*leaf_mins, *leaf_maxs;
int		leaf_topnode;

static void CM_BoxLeafnums_r (int nodenum)
{
	cplane_t	*plane;
	cnode_t		*node;
	int		s;

	while (1)
	{
		if (nodenum < 0)
		{
			if (leaf_count >= leaf_maxcount)
			{
//				Com_Printf ("CM_BoxLeafnums_r: overflow\n");
				return;
			}
			leaf_list[leaf_count++] = -1 - nodenum;
			return;
		}

		node = &map_nodes[nodenum];
		plane = node->plane;
		s = BOX_ON_PLANE_SIDE(leaf_mins, leaf_maxs, plane);
		if (s == 1)
			nodenum = node->children[0];
		else if (s == 2)
			nodenum = node->children[1];
		else
		{	// go down both
			if (leaf_topnode == -1)
				leaf_topnode = nodenum;
			CM_BoxLeafnums_r (node->children[0]);
			nodenum = node->children[1];
		}

	}
}

static int	CM_BoxLeafnums_headnode (vec3_t mins, vec3_t maxs, int *list, int listsize, int headnode, int *topnode)
{
	leaf_list = list;
	leaf_count = 0;
	leaf_maxcount = listsize;
	leaf_mins = mins;
	leaf_maxs = maxs;

	leaf_topnode = -1;

	CM_BoxLeafnums_r (headnode);

	if (topnode)
		*topnode = leaf_topnode;

	return leaf_count;
}

int	CM_BoxLeafnums (vec3_t mins, vec3_t maxs, int *list, int listsize, int *topnode)
{
	return CM_BoxLeafnums_headnode (mins, maxs, list,
		listsize, map_cmodels[0].headnode, topnode);
}



/*
==================
CM_PointContents

==================
*/
int CM_PointContents (vec3_t p, int headnode)
{
	int		l;

	if (!numnodes)	// map not loaded
		return 0;

	l = CM_PointLeafnum_r (p, headnode);

	return map_leafs[l].contents;
}

/*
==================
CM_TransformedPointContents

Handles offseting and rotation of the end points for moving and
rotating entities
==================
*/
int	CM_TransformedPointContents (vec3_t p, int headnode, vec3_t origin, vec3_t angles)
{
	vec3_t		p_l;
	vec3_t		temp;
	vec3_t		forward, right, up;
	int			l;

	// subtract origin offset
	VectorSubtract (p, origin, p_l);

	// rotate start and end into the models frame of reference
	if (headnode != box_headnode && (angles[0] || angles[1] || angles[2]))
	{
		AngleVectors (angles, forward, right, up);

		VectorCopy (p_l, temp);
		p_l[0] = DotProduct (temp, forward);
		p_l[1] = -DotProduct (temp, right);
		p_l[2] = DotProduct (temp, up);
	}

	l = CM_PointLeafnum_r (p_l, headnode);

	return map_leafs[l].contents;
}


/*
===============================================================================

BOX TRACING

===============================================================================
*/

#define	DIST_EPSILON	(1.0f/32)

static vec3_t	trace_start, trace_end;
static vec3_t	trace_mins, trace_maxs;
static vec3_t	trace_extents;

static trace_t	trace_trace;
static int		trace_contents;
static qboolean	trace_ispoint;		// optimized case


/*
================
HALF-LIFE specific trace
================
*/

// Maximum number of leafs with different contents, through witch
// we are trace
#define MAX_HL_TRACE 32

typedef struct
{
	int	used;
	int	next;
	float	p1f, p2f, midf; //?? is "midf" needed?
	int	leaf; // leaf for p2f point
} trace_part_t;

static trace_part_t trace_parts[MAX_HL_TRACE];
static float hl_max_frac;
static int trace_parts_start;


//!!! Remove TraceBuf_Dump()::
#include "../client/ref.h"
extern refExport_t re;

static void TraceBuf_Dump (char *name)
{
	char buf[1024];
	trace_part_t *p;
	int i = trace_parts_start;

	strcpy (buf, "TB_Dump:");
	if (name) strcat (buf, name);

	while (i != -1)
	{
		p = &trace_parts[i];
		strcat (buf, va(" {L:%d %g-%g}", p->leaf, p->p1f, p->p2f));
		if (!p->used) strcat (buf, " UN!!");
		i = p->next;
	}
	re.DrawTextRight (buf, 1,1,1);
}
//!!-----------END-------------

static void TraceBuf_Init ()
{
	int i;
	{
		char buf[256];
                Com_sprintf(buf,sizeof(buf),"TB_Init: {%g,%g,%g}-{%g,%g,%g}",trace_start[0],trace_start[1],trace_start[2],
                	trace_end[0],trace_end[1],trace_end[2]);
		re.DrawTextRight (buf,1,1,1); //!!
	}
	hl_max_frac = 1.0;
	trace_parts_start = -1;
	for (i = 0; i < MAX_HL_TRACE; i++)
		trace_parts[i].used = 0;
}


static void TraceBuf_LimitFrac (float max_frac)
{
	int i, found;
	trace_part_t *p;

	if (max_frac >= hl_max_frac) return;

	hl_max_frac = max_frac;
	found = 0;
        i = trace_parts_start;

//	TraceBuf_Dump("before limit "); //!!

	while (i != -1)
	{
		p = &trace_parts[i];
		i = p->next;

		if (found)
			p->used = 0; // just free slot
		else
		{
			if (p->p2f >= max_frac)
			{
				found = 1;
				p->next = -1; // break chain and free all following slots
                        }
		}
	}
//	TraceBuf_Dump("after limit "); //!!
}


static void TraceBuf_AddLeaf (int leafnum, float p1f, float p2f)
{
	int i, prev, next;
	trace_part_t *p, *pn;
	float midf;

	i = trace_parts_start;
	prev = -1;
	next = i;

	{//!!!
		char buf[256];
		Com_sprintf (buf, sizeof(buf),"  TB_Add {L: %d, %g-%g}", leafnum, p1f, p2f);//!!
		re.DrawTextRight (buf, 1,1,0);
	}//===*/

//	midf = (p1f + p2f) / 2.0; //??
	midf = p1f + (p2f - p1f) / 50.0;

	while (i != -1)
	{
		p = &trace_parts[i];

		if (p2f <= p->p1f) // we must insert before this part
		{
			if (p2f == p->p1f) // merge to p as start
			{
				p->p1f = p1f;
				return;
			}
			break;
		}

		if (p1f == p->p2f) // merge to p as end
		{
			p->p2f = p2f;
			p->midf = midf;
			p->leaf = leafnum;
			// check to merge with next leaf too
			i = p->next;
			if (i == -1) return; // last part
			pn = &trace_parts[i];
			if (pn->p1f == p2f)
			{
				// do merge
				p->p2f = pn->p2f;
				p->midf = pn->midf;
				p->leaf = pn->leaf;
				p->next = pn->next;
				// free merged slot
				pn->used = 0;
			}
			return;
		}

		prev = i;
		i = p->next;
		next = i;
	}
	// find free slot -> i
	for (i = 0; i < MAX_HL_TRACE; i++)
		if (!trace_parts[i].used) break;

	if (i == MAX_HL_TRACE)
	{
		Com_Printf ("HL_Trace: buffer overflow!\n");
		return;
	}

	// here: i = array index, prev = prev index (-1 - make it first), next = next index
	if (prev == -1)
		trace_parts_start = i;
	else
		trace_parts[prev].next = i;
	p = &trace_parts[i];
	p->used = 1;
	p->next = next;
	p->p1f = p1f;
	p->p2f = p2f;
	p->midf = midf;
	p->leaf = leafnum;
}


static trace_part_t *TraceBuf_FindPart (float frac)
{
	int i;
	trace_part_t *p;

	i = trace_parts_start;
	while (i != -1)
	{
		p = &trace_parts[i];
		if (p->p1f <= frac && p->p2f >= frac)
			return p;
		i = p->next;
	}
	return NULL;
}


// Finds trace->plane & trace->surface
static void CM_ClipBoxToHLLeaf (vec3_t mins, vec3_t maxs, vec3_t p1, vec3_t p2, int leafnum)
{
	int			i, j;
	cplane_t	*plane, *clipplane;
	float		dist, frac, offset;
	vec3_t		ofs;
	float		d1, d2;
	float		f;
	cbrushside_t	*side, *leadside;
	cbrush_t	*brush;

	brush = &map_brushes[map_leafbrushes[map_leafs[leafnum].firstleafbrush]]; // every leaf in loaded HL map have exactly one brush

	frac = -1;
	clipplane = NULL;

	if (!brush->numsides)
		return;

	leadside = NULL;

/*	{ //!!
		char buf[256];
		Com_sprintf (buf, sizeof(buf), "Checking leaf %d - %d sides...", leafnum, brush->numsides);
		re.DrawTextRight (buf, 0,0.5,0.3);
	} //====*/

	for (i=0 ; i<brush->numsides ; i++)
	{
		side = &map_brushsides[brush->firstbrushside+i];
		plane = side->plane;
		// FIXME: special case for axial

/*		if (!trace_ispoint)
		{	// general box case

			// push the plane out apropriately for mins/maxs

			// FIXME: use signbits into 8 way lookup for each mins/maxs
			for (j=0 ; j<3 ; j++)
			{
				if (plane->normal[j] < 0)
					ofs[j] = maxs[j];
				else
					ofs[j] = mins[j];
			}
			dist = plane->dist - DotProduct (ofs, plane->normal);
		}
		else
		{	// special point case
			dist = plane->dist;
		}

		d1 = DotProduct (p1, plane->normal) - dist;
		d2 = DotProduct (p2, plane->normal) - dist;
*/
		dist = plane->dist;
		d1 = DotProduct (p1, plane->normal) - dist;
		d2 = DotProduct (p2, plane->normal) - dist;

		//!!!! IF d1 < 0, we must get inverted plane!!!
		if (d1 < 0)
		{
			plane = &map_planes[(plane - map_planes) ^ 1];
			d1 = -d1;
			d2 = -d2;
		}

		if (!trace_ispoint)
		{
			for (j = 0; j < 3; j++)
			{
				if (plane->normal[j] < 0)
					ofs[j] = maxs[j];
				else
					ofs[j] = mins[j];
			}
			offset = DotProduct (ofs, plane->normal);
			d1 += offset;
			d2 += offset;
		}

//		//!!!! IF d1 < 0, we must get inverted plane!!!
//		if (d1 < 0) plane = &map_planes[(plane - map_planes) ^ 1];

/*		{//!!!!
			char buf[256];
			Com_sprintf(buf,sizeof(buf),"-- d1=%g  d2=%g  plane:(d=%g  %g,%g,%g) dist=%g (surf=%s) ofs={%g,%g,%g} fr=%g",d1,d2,
				plane->dist,plane->normal[0],plane->normal[1],plane->normal[2],
				dist,side->surface->rname, ofs[0],ofs[1],ofs[2],
				d1 / (d1 - d2));
			re.DrawTextRight(buf,0,1,0);
			Com_sprintf(buf,sizeof(buf),"** p1=(%g,%g,%g)  p2=(%g,%g,%g)  d1=%g  d2=%g  dist=%g",
				p1[0],p1[1],p1[2],
				p2[0],p2[1],p2[2],
				DotProduct(p1, plane->normal),
				DotProduct(p2, plane->normal),
				plane->dist);
			re.DrawTextRight(buf,0,0.8,0.2);
		}//==*/

/*		// if completely in front of face, no intersection
		if (d1 > 0 && d2 >= d1)
		{
			// should not happen!
			Com_Printf ("Error #1 in CL_ClipToHLBrush (l=%d,i=%d)\n", leaf-map_leafs,i); //!!
			return;
		}

		if (d1 <= 0 && d2 <= 0) // completely in back of this face
			continue; */

		//??
		if (d1 < 0 && d2 < 0 || d1 > 0 && d2 > 0) continue;

/*		// crosses face
		if (d1 > d2)
			// enter
			f = (d1-DIST_EPSILON) / (d1-d2);
		else
			// leave
			f = (d1+DIST_EPSILON) / (d1-d2);
*/
		f = (d1 - DIST_EPSILON) / (d1-d2);
/*		{ //!!!
			char buf[256];
			Com_sprintf(buf,sizeof(buf),"Cross %s with frac=%g, d1=%g, norm=(%g,%g,%g)",side->surface->rname, f, d1,
				side->plane->normal[0], side->plane->normal[1], side->plane->normal[2]);
			re.DrawTextRight(buf,0,1,1);
		} //===*/
		if (f > frac)
		{
			frac = f;
			clipplane = plane;
			leadside = side;
		}
	}

	if (frac >= 0.0)
	{
		trace_trace.fraction = frac; // enterfrac;
		if (clipplane)
		{
			trace_trace.plane = *clipplane;
			trace_trace.surface = leadside->surface;
		}
		else
		{
			i = CM_PointLeafnum (p1);
			{ //!!!
				char buf[256];
				Com_sprintf (buf,sizeof(buf),"Leaf %d [%d={%g,%g,%g}] -- %d br: no plane", leafnum,i,p1[0],p1[1],p1[2],brush->numsides);//!!
				re.DrawTextRight (buf, 0.5,1,0.2);
			}//====
		}
	}
	else
	{
		// should not happen!
		char buf[256];
		Com_sprintf (buf,sizeof(buf),"CL_ClipToHLBrush: not intersected with leaf %d", leafnum);
		re.DrawTextRight (buf,0.2,1,0.2); //!!

		trace_trace.fraction = 1;
		trace_trace.startsolid = false;
		trace_trace.allsolid = false;
		trace_trace.contents = 0;
	}
}


static void CM_TraceToHLLeaf (int leafnum, float p1f, float p2f)
{
	dleaf_t		*leaf;

	leaf = &map_leafs[leafnum];
	if (!(leaf->contents & trace_contents))
	{
		TraceBuf_AddLeaf (leafnum, p1f, p2f);
		return;
	}

	{//!!!!!
		char buf[256];
		Com_sprintf (buf,sizeof(buf),"  ** TB_Limit (L: %d, f1: %g, f2: %g, cont: %d) **", leafnum, p1f, p2f, leaf->contents);
		re.DrawTextRight (buf,0,1,1);
	}//=====*/
	TraceBuf_LimitFrac (p1f);
	trace_trace.fraction = p1f;
	trace_trace.contents = leaf->contents;
	if (p1f == 0.0)
	{
		trace_trace.startsolid = true;
		if (p2f == 1.0) trace_trace.allsolid = true;
	}
}


static void CM_TestInHLLeaf (int leafnum)
{
	dleaf_t		*leaf;

	leaf = &map_leafs[leafnum];
	if ( !(leaf->contents & trace_contents))
		return;

	// no brushes - trace complete
	trace_trace.fraction = 0;
	trace_trace.startsolid = true;
	trace_trace.allsolid = true;
	trace_trace.contents = leaf->contents;
	return;
}


static void CM_FinishHLTrace (void)
{
	trace_part_t	*p;
//	vec3_t	mid;//, end;
//	int	i;
//	float	frac;

	p = TraceBuf_FindPart (trace_trace.fraction);
	if (!p)
	{
	//!!!!!
		char buf[256];
		Com_sprintf(buf,sizeof(buf),"TB_Finish: no part (frac=%g)",trace_trace.fraction);
		re.DrawTextRight (buf,0,1,0); //!!
		TraceBuf_Dump ("on finish ");
	//=====*/
		return; // startsolid ?
	}

/*	frac = p->midf; // start from leaf middle point of the last leaf
	for (i=0 ; i<3 ; i++)
		mid[i] = trace_start[i] + frac*(trace_end[i] - trace_start[i]);

	{//!!!!!!
		char buf[256];
		Com_sprintf(buf,sizeof(buf),"try to clip L: %d on frac=%g",p->leaf,frac);
		re.DrawTextRight(buf,1,1,1);
	}//====*/
//	VectorSubtract (trace_end, trace_start, end);
//	frac = VectorLength (end);
//	VectorScale (end, (frac + trace_extents[2]) / frac, end);
//	VectorAdd (trace_start, end, end);
	CM_ClipBoxToHLLeaf (trace_mins, trace_maxs, trace_start, trace_end, p->leaf);
/*	{//!!!!!!
		char buf[256];
		Com_sprintf(buf,sizeof(buf),"TB_Finish: frac=%g, cont=%X", trace_trace.fraction, trace_trace.contents);
		TraceBuf_Dump ("on finish ");
		re.DrawTextRight (buf, 1,1,1);
	}//====*/
}


static void CM_RecursiveHLHullCheck (int num, float p1f, float p2f, vec3_t p1, vec3_t p2)
{
	cnode_t		*node;
	cplane_t	*plane;
	float		t1, t2, offset;
	float		frac, frac2;
	float		idist;
	int			i;
	vec3_t		mid;
	int			side;
	float		midf;

	if (trace_trace.fraction <= p1f)
		return;		// already hit something nearer

	// if < 0, we are in a leaf node
	if (num < 0)
	{
		CM_TraceToHLLeaf (-1-num, p1f, p2f);
		return;
	}

	//
	// find the point distances to the separating plane
	// and the offset for the size of the box
	//
	node = map_nodes + num;
	plane = node->plane;

	if (plane->type < 3)
	{
		t1 = p1[plane->type] - plane->dist;
		t2 = p2[plane->type] - plane->dist;
		offset = trace_extents[plane->type];
	}
	else
	{
		t1 = DotProduct (plane->normal, p1) - plane->dist;
		t2 = DotProduct (plane->normal, p2) - plane->dist;
		if (trace_ispoint)
			offset = 0;
		else
			offset = fabs(trace_extents[0]*plane->normal[0]) +
				fabs(trace_extents[1]*plane->normal[1]) +
				fabs(trace_extents[2]*plane->normal[2]);
	}

	// see which sides we need to consider
	if (t1 >= offset && t2 >= offset)
	{
		CM_RecursiveHLHullCheck (node->children[0], p1f, p2f, p1, p2);
		return;
	}
	if (t1 < -offset && t2 < -offset)
	{
		CM_RecursiveHLHullCheck (node->children[1], p1f, p2f, p1, p2);
		return;
	}

	// put the crosspoint DIST_EPSILON pixels on the near side
	if (t1 < t2)
	{
		idist = 1.0/(t1-t2);
		side = 1;
		frac2 = (t1 + offset + DIST_EPSILON)*idist;
		frac = (t1 - offset + DIST_EPSILON)*idist;
	}
	else if (t1 > t2)
	{
		idist = 1.0/(t1-t2);
		side = 0;
		frac2 = (t1 - offset - DIST_EPSILON)*idist;
		frac = (t1 + offset + DIST_EPSILON)*idist;
	}
	else
	{
		side = 0;
		frac = 1;
		frac2 = 0;
	}

	if (frac < 0)
		frac = 0;
	if (frac > 1)
		frac = 1;

	if (frac2 < 0)
		frac2 = 0;
	if (frac2 > 1)
		frac2 = 1;

/*	{//!!!!
		char buf[256];
		Com_sprintf(buf,sizeof(buf),"RHC: divide (%d): t1=%g t2=%g offs=%g {N:%d %g-%g} {N:%d %g-%g} plane(d:%g %g,%g,%g)",num,t1,t2,offset,
			node->children[side], p1f, p1f + (p2f - p1f)*frac,
                        node->children[side^1], midf = p1f + (p2f - p1f)*frac2, p2f,
                        plane->dist,plane->normal[0],plane->normal[1],plane->normal[2]);
		re.DrawTextRight(buf,1,0.3,0.3);
	}//===*/

	// move up to the node
	midf = p1f + (p2f - p1f)*frac;
	for (i=0 ; i<3 ; i++)
		mid[i] = p1[i] + frac*(p2[i] - p1[i]);

	CM_RecursiveHLHullCheck (node->children[side], p1f, midf, p1, mid);


	// go past the node
	midf = p1f + (p2f - p1f)*frac2;
	for (i = 0; i < 3; i++)
		mid[i] = p1[i] + frac2*(p2[i] - p1[i]);

	CM_RecursiveHLHullCheck (node->children[side^1], midf, p2f, mid, p2);
}


/*
================
CM_ClipBoxToBrush
================
*/
static void CM_ClipBoxToBrush (vec3_t mins, vec3_t maxs, vec3_t p1, vec3_t p2, trace_t *trace, cbrush_t *brush)
{
	int			i;
	cplane_t	*clipplane;
	float		enterfrac, leavefrac;
	qboolean	getout, startout;
	cbrushside_t *leadside;

	enterfrac = -1;
	leavefrac = 1;
	clipplane = NULL;

	if (!brush->numsides)
		return;

	c_brush_traces++;

	getout = false;
	startout = false;
	leadside = NULL;

	for (i = 0; i < brush->numsides; i++)
	{
		cbrushside_t *side;
		cplane_t *plane;
		int		j;
		float	d1, d2, dist, f;
		vec3_t	ofs;

		side = &map_brushsides[brush->firstbrushside+i];

//		if (Cvar_VariableInt ("test") && //!!
//			trace_ispoint && side->surface && side->surface->flags & SURF_ALPHA)		//?? shooting through SURF_ALPHA
//			continue;

		plane = side->plane;
		// FIXME: special case for axial

		if (!trace_ispoint)
		{	// general box case

			// push the plane out apropriately for mins/maxs

			// FIXME: use signbits into 8 way lookup for each mins/maxs
			for (j = 0; j < 3; j++)
			{
				if (plane->normal[j] < 0)
					ofs[j] = maxs[j];
				else
					ofs[j] = mins[j];
			}
			dist = plane->dist - DotProduct (ofs, plane->normal);
		}
		else
		{	// special point case
			dist = plane->dist;
		}

		d1 = DotProduct (p1, plane->normal) - dist;
		d2 = DotProduct (p2, plane->normal) - dist;

		if (d2 > 0)
			getout = true;	// endpoint is not in solid
		if (d1 > 0)
			startout = true;

		// if completely in front of face, no intersection
		if (d1 > 0 && d2 >= d1)
			return;

		if (d1 <= 0 && d2 <= 0) // completely in back of this face
			continue;

		// crosses face
		if (d1 > d2)
		{	// enter
			f = (d1-DIST_EPSILON) / (d1-d2);
			if (f > enterfrac)
			{
				enterfrac = f;
				clipplane = plane;
				leadside = side;
			}
		}
		else
		{	// leave
			f = (d1+DIST_EPSILON) / (d1-d2);
			if (f < leavefrac)
				leavefrac = f;
		}
/*		{//!!!!
			char buf[256];
			Com_sprintf(buf,sizeof(buf),"-- d1=%g  d2=%g  plane:(d=%g  %g,%g,%g) dist=%g (surf=%s) ofs={%g,%g,%g} fr=%g",d1,d2,
				plane->dist,plane->normal[0],plane->normal[1],plane->normal[2],
				dist,side->surface->rname, ofs[0],ofs[1],ofs[2],
				d1 / (d1 - d2));
			re.DrawTextRight(buf,0,1,0);
			Com_sprintf(buf,sizeof(buf),"** p1=(%g,%g,%g)  p2=(%g,%g,%g)  d1=%g  d2=%g  dist=%g",
				p1[0],p1[1],p1[2],
				p2[0],p2[1],p2[2],
				DotProduct(p1, plane->normal),
				DotProduct(p2, plane->normal),
				plane->dist);
			re.DrawTextRight(buf,0,0.8,0.2);
		}//==*/

	}

	if (!startout)
	{	// original point was inside brush
		trace->startsolid = true;
		if (!getout)
			trace->allsolid = true;
		return;
	}
	if (enterfrac < leavefrac)
	{
		if (enterfrac > -1 && enterfrac < trace->fraction)
		{
			if (enterfrac < 0) // when startsolid && allsolid
				enterfrac = 0;
			trace->fraction = enterfrac;
			trace->plane = *clipplane;
			trace->surface = leadside->surface;
			trace->contents = brush->contents;
		}
	}
}

/*
================
CM_TestBoxInBrush

if box inside this brush or intersects its brushsides:
 1) trace->fraction=0
 2) trace->startsolid = true
 3) trace->allsolid = true
else trace is unchanged
================
*/
static void CM_TestBoxInBrush (vec3_t mins, vec3_t maxs, vec3_t p1, trace_t *trace, cbrush_t *brush)
{
	int			i, j;
	cplane_t	*plane;
	float		dist;
	vec3_t		ofs;
	float		d1;
	cbrushside_t	*side;

	if (!brush->numsides)
		return;

	for (i = 0; i < brush->numsides; i++)
	{
		side = &map_brushsides[brush->firstbrushside+i];
		plane = side->plane;

		// FIXME: special case for axial

		// general box case

		// push the plane out apropriately for mins/maxs

		// FIXME: use signbits into 8 way lookup for each mins/maxs
		for (j = 0; j < 3; j++)
		{
			if (plane->normal[j] < 0)
				ofs[j] = maxs[j];
			else
				ofs[j] = mins[j];
		}
		dist = plane->dist - DotProduct (ofs, plane->normal);

		d1 = DotProduct (p1, plane->normal) - dist;

		// if completely in front of face, no intersection
		if (d1 > 0)
			return;

	}

	// inside this brush
	trace->startsolid = trace->allsolid = true;
	trace->fraction = 0;
	trace->contents = brush->contents;
}


/*
================
CM_TraceToLeaf
================
*/
static void CM_TraceToLeaf (int leafnum)
{
	int			k;
	int			brushnum;
	dleaf_t		*leaf;
	cbrush_t	*b;

	leaf = &map_leafs[leafnum];
	if ( !(leaf->contents & trace_contents))
		return;
	// trace line against all brushes in the leaf
	for (k = 0; k < leaf->numleafbrushes; k++)
	{
		trace_t save_trace;

		brushnum = map_leafbrushes[leaf->firstleafbrush+k];
		b = &map_brushes[brushnum];
		if (b->checkcount == checkcount)
			continue;	// already checked this brush in another leaf
		b->checkcount = checkcount;
		if (!(b->contents & trace_contents))
			continue;

		save_trace = trace_trace;
		CM_ClipBoxToBrush (trace_mins, trace_maxs, trace_start, trace_end, &trace_trace, b);
		if (trace_ispoint && trace_trace.surface && trace_trace.surface->flags & SURF_ALPHA)	//?? shooting through SURF_ALPHA
		{
			trace_trace = save_trace;
			continue;
		}

		if (!trace_trace.fraction) // when startsolid && allsolid
			return;
	}
}


/*
================
CM_TestInLeaf

Call CM_TestBoxInBrush for each leafbrush.
When first intersection found (trace.fraction == 0) - return
================
*/
static void CM_TestInLeaf (int leafnum)
{
	int			k;
	int			brushnum;
	dleaf_t		*leaf;
	cbrush_t	*b;

	leaf = &map_leafs[leafnum];
	if ( !(leaf->contents & trace_contents))
		return;

	// trace line against all brushes in the leaf
	for (k = 0; k < leaf->numleafbrushes; k++)
	{
		brushnum = map_leafbrushes[leaf->firstleafbrush+k];
		b = &map_brushes[brushnum];
		if (b->checkcount == checkcount)
			continue;	// already checked this brush in another leaf
		b->checkcount = checkcount;

		if (!(b->contents & trace_contents))
			continue;
		CM_TestBoxInBrush (trace_mins, trace_maxs, trace_start, &trace_trace, b);
		if (!trace_trace.fraction)
			return;
	}
}


/*
==================
CM_RecursiveHullCheck

==================
*/
static void CM_RecursiveHullCheck (int num, float p1f, float p2f, vec3_t p1, vec3_t p2)
{
	cnode_t		*node;
	cplane_t	*plane;
	float		t1, t2, offset;
	float		frac, frac2;
	float		idist;
	int			i;
	vec3_t		mid;
	int			side;
	float		midf;

	if (trace_trace.fraction <= p1f)
		return;		// already hit something nearer

	// if < 0, we are in a leaf node
	if (num < 0)
	{
		CM_TraceToLeaf (-1-num);
		return;
	}

	// find the point distances to the separating plane
	// and the offset for the size of the box
	node = map_nodes + num;
	plane = node->plane;

	if (plane->type < 3)
	{
		t1 = p1[plane->type] - plane->dist;
		t2 = p2[plane->type] - plane->dist;
		offset = trace_extents[plane->type];
	}
	else
	{
		t1 = DotProduct (plane->normal, p1) - plane->dist;
		t2 = DotProduct (plane->normal, p2) - plane->dist;
		if (trace_ispoint)
			offset = 0;
		else
			offset = fabs (trace_extents[0]*plane->normal[0]) +
					 fabs (trace_extents[1]*plane->normal[1]) +
					 fabs (trace_extents[2]*plane->normal[2]);
	}

	// see which sides we need to consider
	if (t1 >= offset && t2 >= offset)
	{
		CM_RecursiveHullCheck (node->children[0], p1f, p2f, p1, p2);
		return;
	}
	if (t1 < -offset && t2 < -offset)
	{
		CM_RecursiveHullCheck (node->children[1], p1f, p2f, p1, p2);
		return;
	}

	// put the crosspoint DIST_EPSILON pixels on the near side
	if (t1 < t2)
	{
		idist = 1.0f / (t1 - t2);
		side = 1;
		frac2 = (t1 + offset + DIST_EPSILON) * idist;
		frac = (t1 - offset + DIST_EPSILON) * idist;
	}
	else if (t1 > t2)
	{
		idist = 1.0f / (t1 - t2);
		side = 0;
		frac2 = (t1 - offset - DIST_EPSILON) * idist;
		frac = (t1 + offset + DIST_EPSILON) * idist;
	}
	else
	{
		side = 0;
		frac = 1;
		frac2 = 0;
	}

	if (frac < 0)	frac = 0;
	if (frac > 1)	frac = 1;

	if (frac2 < 0)	frac2 = 0;
	if (frac2 > 1)	frac2 = 1;

/*	{//!!!!
		char buf[256];
		Com_sprintf(buf,sizeof(buf),"RHC: divide (%d): t1=%g t2=%g offs=%g {N:%d %g-%g} {N:%d %g-%g} plane(d:%g %g,%g,%g)",num,t1,t2,offset,
			node->children[side], p1f, p1f + (p2f - p1f)*frac,
                        node->children[side^1], midf = p1f + (p2f - p1f)*frac2, p2f,
                        plane->dist,plane->normal[0],plane->normal[1],plane->normal[2]);
		re.DrawTextRight(buf,1,0.3,0.3);
	}//===*/

	// move up to the node
	midf = p1f + frac * (p2f - p1f);
	for (i = 0; i < 3; i++)
		mid[i] = p1[i] + frac * (p2[i] - p1[i]);

	CM_RecursiveHullCheck (node->children[side], p1f, midf, p1, mid);


	// go past the node
	midf = p1f + frac2 * (p2f - p1f);
	for (i = 0; i < 3; i++)
		mid[i] = p1[i] + frac2 * (p2[i] - p1[i]);

	CM_RecursiveHullCheck (node->children[side^1], midf, p2f, mid, p2);
}



//======================================================================

/*
==================
CM_BoxTrace

When start==end:
  1) trace.contents is set (only this field contains information!)
  2) trace.fraction = 0
  3) trace.endpos = start
  4) trace.allsolid = trace.startsolid = true
  5) trace.surface = nullsurface
  6) trace.ent & trace.plane are unchanged
==================
*/
trace_t		CM_BoxTrace (vec3_t start, vec3_t end, vec3_t mins, vec3_t maxs, int headnode, int brushmask)
{
	int		i;

	checkcount++;		// for multi-check avoidance
	c_traces++;			// for statistics, may be zeroed

	// fill in a default trace
	memset (&trace_trace, 0, sizeof(trace_trace));
	trace_trace.fraction = 1;
	trace_trace.surface = &nullsurface;

	if (!numnodes)	// map not loaded
		return trace_trace;

/*	if (maptype == map_hl)
	//!!!!
	{
		char buf[256];
//	Com_Printf(">");
		for (i = 0; i < numcmodels; i++)
		{
			if (headnode == map_cmodels[i].headnode)
			{
				Com_sprintf (buf,sizeof(buf),"CM_BoxTrace (model %d, mask=%X)", i, brushmask);
				re.DrawTextRight (buf, 1,0,1);
				if (i) return trace_trace; //!!
				break;
			}
		}
		if (i == numcmodels)
		{
			if (headnode == box_headnode)
				re.DrawTextRight ("CM_BoxTrace (box_headnode)", 1,0,1);
			else
			{
				Com_sprintf (buf,sizeof(buf),"CM_BoxTrace (unk %d)", headnode);
				re.DrawTextRight (buf, 1,0.2,0.2);
			}
			return trace_trace; //!!
		}
//		Com_sprintf(buf,sizeof(buf),"start=(%g,%g,%g) end=(%g,%g,%g)",start[0],start[1],start[2],end[0],end[1],end[2]);
//		re.DrawTextRight(buf,0.6,1,1);
//		Com_sprintf(buf,sizeof(buf),"mins=(%g,%g,%g) maxs=(%g,%g,%g)",mins[0],mins[1],mins[2],maxs[0],maxs[1],maxs[2]);
//		re.DrawTextRight(buf,0.6,1,1);
//	Com_Printf("<");
	}
	//====*/

	trace_contents = brushmask;
	VectorCopy (start, trace_start);
	VectorCopy (end, trace_end);
	VectorCopy (mins, trace_mins);
	VectorCopy (maxs, trace_maxs);

	// check for "position test" special case
	if (start[0] == end[0] && start[1] == end[1] && start[2] == end[2])
	{
		int		leafs[1024];
		int		i, numleafs;
		vec3_t	c1, c2;
		int		topnode;

/*		if (maptype == map_hl) //!!!
			re.DrawTextRight ("CM_TestInLeaf()",0.5,0.5,1);
		//==*/

		for (i = 0; i < 3; i++)
		{
			c1[i] = start[i] + mins[i] - 1;
			c2[i] = start[i] + maxs[i] + 1;
		}

		numleafs = CM_BoxLeafnums_headnode (c1, c2, leafs, 1024, headnode, &topnode);
		for (i=0 ; i<numleafs ; i++)
		{
/*			if (maptype == map_hl)
				CM_TestInHLLeaf (leafs[i]);
			else */
				CM_TestInLeaf (leafs[i]);
			if (trace_trace.allsolid) // always set when 1st intersection bt CM_TestBoxInBrush()
				break;
		}
		VectorCopy (start, trace_trace.endpos);

/*		if (maptype == map_hl)
		{//!!!!
			char buf[256];
			Com_sprintf (buf,sizeof(buf),"  *** CM_BoxTrace_End: frac=%g  cont=%X",
				trace_trace.fraction,trace_trace.contents);
			re.DrawTextRight (buf,1,1,1);
			re.DrawTextRight ("====================", 0.5,0,0.5);
		}//===*/
		return trace_trace;
	}

	// our "clip to all" special case (mins > maxs) -- point trace, but not go through SURF_ALPHA
	if (mins[0] > maxs[0] && mins[1] > maxs[1] && mins[2] > maxs[2])
	{
		static vec3_t zero_vec = {0, 0, 0};

		trace_ispoint = false;	//??
		mins = maxs = zero_vec;
		VectorClear (trace_extents);
	}
	// check for "point" special case
	else if (mins[0] == 0 && mins[1] == 0 && mins[2] == 0
		&& maxs[0] == 0 && maxs[1] == 0 && maxs[2] == 0)
	{
		trace_ispoint = true;
		VectorClear (trace_extents);
	}
	else
	{
		trace_ispoint = false;
		trace_extents[0] = -mins[0] > maxs[0] ? -mins[0] : maxs[0];
		trace_extents[1] = -mins[1] > maxs[1] ? -mins[1] : maxs[1];
		trace_extents[2] = -mins[2] > maxs[2] ? -mins[2] : maxs[2];
	}

	// general sweeping through world
/*	if (maptype == map_hl)
	{
		TraceBuf_Init ();
		CM_RecursiveHLHullCheck (headnode, 0, 1, start, end);
		CM_FinishHLTrace ();
	}
	else */
		CM_RecursiveHullCheck (headnode, 0, 1, start, end);

	if (trace_trace.fraction == 1)
	{
		VectorCopy (end, trace_trace.endpos);
	}
	else
	{
		for (i=0 ; i<3 ; i++)
			trace_trace.endpos[i] = start[i] + trace_trace.fraction * (end[i] - start[i]);
	}

/*	if (maptype == map_hl)
	{//!!
		char buf[256];
		Com_sprintf (buf,sizeof(buf),"  *** CM_BoxTrace_End: startsolid=%d  allsolid=%d  frac=%g  cont=%X  surf=%s",
			trace_trace.startsolid,trace_trace.allsolid,trace_trace.fraction,trace_trace.contents,trace_trace.surface->name);
		re.DrawTextRight (buf,1,1,1);
		re.DrawTextRight ("====================", 0.5,0,0.5);
	}//===*/

/*	//!!
	if (trace_trace.fraction < 1 && trace_trace.surface && trace_trace.surface->flags & SURF_ALPHA)
	{
		Com_Printf ("%s  min={%g %g %g} max={%g %g %g}\n", trace_trace.surface->name,
			mins[0], mins[1], mins[2], maxs[0], maxs[1], maxs[2]);
	}
*/
	return trace_trace;
}


/*
==================
CM_TransformedBoxTrace

Handles offseting and rotation of the end points for moving and
rotating entities
==================
*/


trace_t CM_TransformedBoxTrace (vec3_t start, vec3_t end,
				vec3_t mins, vec3_t maxs, int headnode, int brushmask,
				vec3_t origin, vec3_t angles)
{
	trace_t		trace;
	vec3_t		start_l, end_l;
	vec3_t		a;
	vec3_t		forward, right, up;
	vec3_t		temp;
	qboolean	rotated;

	// subtract origin offset
	VectorSubtract (start, origin, start_l);
	VectorSubtract (end, origin, end_l);

	// rotate start and end into the models frame of reference
	if (headnode != box_headnode && (angles[0] || angles[1] || angles[2]))
		rotated = true;
	else
		rotated = false;

	if (rotated)
	{
		AngleVectors (angles, forward, right, up);

		VectorCopy (start_l, temp);
		start_l[0] = DotProduct (temp, forward);
		start_l[1] = -DotProduct (temp, right);
		start_l[2] = DotProduct (temp, up);

		VectorCopy (end_l, temp);
		end_l[0] = DotProduct (temp, forward);
		end_l[1] = -DotProduct (temp, right);
		end_l[2] = DotProduct (temp, up);
	}

	// sweep the box through the model
	trace = CM_BoxTrace (start_l, end_l, mins, maxs, headnode, brushmask);

	if (rotated && trace.fraction != 1.0)
	{
		// FIXME: figure out how to do this with existing angles
		VectorNegate (angles, a);
		AngleVectors (a, forward, right, up);

		VectorCopy (trace.plane.normal, temp);
		trace.plane.normal[0] = DotProduct (temp, forward);
		trace.plane.normal[1] = -DotProduct (temp, right);
		trace.plane.normal[2] = DotProduct (temp, up);
	}

	trace.endpos[0] = start[0] + trace.fraction * (end[0] - start[0]);
	trace.endpos[1] = start[1] + trace.fraction * (end[1] - start[1]);
	trace.endpos[2] = start[2] + trace.fraction * (end[2] - start[2]);

	return trace;
}



/*
===============================================================================

PVS / PHS

===============================================================================
*/

/*
===================
CM_DecompressVis
===================
*/
void CM_DecompressVis (byte *in, byte *out)
{
	int		c;
	byte	*out_p;
	int		row;

	row = (numclusters + 7) >> 3;
	out_p = out;

	if (!in || !numvisibility)
	{	// no vis info, so make all visible
		while (row)
		{
			*out_p++ = 0xff;
			row--;
		}
		return;
	}

	do
	{
		if (*in)
		{
			*out_p++ = *in++;
			continue;
		}

		c = in[1];
		in += 2;
		if ((out_p - out) + c > row)
		{
			c = row - (out_p - out);
			Com_DPrintf ("WARNING: Vis decompression overrun\n");
		}
		while (c)
		{
			*out_p++ = 0;
			c--;
		}
	} while (out_p - out < row);
}

static byte	pvsrow[MAX_MAP_LEAFS/8];
static byte	phsrow[MAX_MAP_LEAFS/8];

byte *CM_ClusterPVS (int cluster)
{
	int	i;

	if (cluster <= -1)
		memset (pvsrow, 0, (numclusters + 7) >> 3);
	else
	{
		i = ((dvis_t *)map_visibility)->bitofs[cluster][DVIS_PVS];
		if (i != -1)
			CM_DecompressVis (map_visibility + i, pvsrow);
		else
			CM_DecompressVis (NULL, pvsrow);
	}
	return pvsrow;
}

byte *CM_ClusterPHS (int cluster)
{
	int	i;

	if (cluster <= -1)
		memset (phsrow, 0, (numclusters + 7) >> 3);
	else
	{
		i = ((dvis_t *)map_visibility)->bitofs[cluster][DVIS_PHS];
		if (i != -1)
			CM_DecompressVis (map_visibility + i, phsrow);
		else
			CM_DecompressVis (NULL, phsrow);
	}
	return phsrow;
}


/*
===============================================================================

AREAPORTALS

===============================================================================
*/

static void FloodArea_r (carea_t *area, int floodnum)
{
	int		i;
	dareaportal_t	*p;

	if (area->floodvalid == floodvalid)
	{
		if (area->floodnum == floodnum)
			return;
		Com_Error (ERR_DROP, "FloodArea_r: reflooded");
	}

	area->floodnum = floodnum;
	area->floodvalid = floodvalid;
	p = &map_areaportals[area->firstareaportal];
	for (i = 0; i < area->numareaportals; i++, p++)
	{
		if (portalopen[p->portalnum])
			FloodArea_r (&map_areas[p->otherarea], floodnum);
	}
}

/*
====================
FloodAreaConnections


====================
*/
void FloodAreaConnections (void)
{
	int		i;
	carea_t	*area;
	int		floodnum;

	// all current floods are now invalid
	floodvalid++;
	floodnum = 0;

	// area 0 is not used
	for (i=1 ; i<numareas ; i++)
	{
		area = &map_areas[i];
		if (area->floodvalid == floodvalid)
			continue;		// already flooded into
		floodnum++;
		FloodArea_r (area, floodnum);
	}

}

void CM_SetAreaPortalState (int portalnum, qboolean open)
{
	if (portalnum > numareaportals)
		Com_Error (ERR_DROP, "areaportal > numareaportals");

	portalopen[portalnum] = open;
	FloodAreaConnections ();
}

qboolean CM_AreasConnected (int area1, int area2)
{
	if (map_noareas->integer)
		return true;

	if (area1 > numareas || area2 > numareas)
		Com_Error (ERR_DROP, "area > numareas");

	if (map_areas[area1].floodnum == map_areas[area2].floodnum)
		return true;
	return false;
}


/*
=================
CM_WriteAreaBits

Writes a length byte followed by a bit vector of all the areas
that area in the same flood as the area parameter

This is used by the client refreshes to cull visibility
=================
*/
int CM_WriteAreaBits (byte *buffer, int area)
{
	int		i, floodnum, bytes;

	bytes = (numareas + 7) >> 3;

	if (map_noareas->integer)
	{	// for debugging, send everything
		memset (buffer, 255, bytes);
	}
	else
	{
		memset (buffer, 0, bytes);

		floodnum = map_areas[area].floodnum;
		for (i = 0; i < numareas; i++)
		{
			if (map_areas[i].floodnum == floodnum || !area)
				buffer[i >> 3] |= 1 << (i & 7);
		}
	}

	return bytes;
}


/*
===================
CM_WritePortalState

Writes the portal state to a savegame file
===================
*/
void CM_WritePortalState (FILE *f)
{
	fwrite (portalopen, sizeof(portalopen), 1, f);
}

/*
===================
CM_ReadPortalState

Reads the portal state from a savegame file
and recalculates the area connections
===================
*/
void CM_ReadPortalState (FILE *f)
{
	fread (portalopen, sizeof(portalopen), 1, f);
	FloodAreaConnections ();
}

/*
=============
CM_HeadnodeVisible

Returns true if any leaf under headnode has a cluster that
is potentially visible
=============
*/
// This function used only from sv_ents.c :: SV_BuildClientFrame()
qboolean CM_HeadnodeVisible (int nodenum, byte *visbits)
{
	int		leafnum;
	int		cluster;
	cnode_t	*node;

	if (nodenum < 0)
	{
		leafnum = -1-nodenum;
		cluster = map_leafs[leafnum].cluster;
		if (cluster == -1)
			return false;
		if (visbits[cluster>>3] & (1<<(cluster&7)))
			return true;
		return false;
	}

	node = &map_nodes[nodenum];
	if (CM_HeadnodeVisible(node->children[0], visbits))
		return true;
	return CM_HeadnodeVisible(node->children[1], visbits);
}
