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
	cplane_t *plane;
	int		children[2];		// negative numbers are leafs
} cnode_t;

typedef struct
{
	cplane_t *plane;
	csurface_t *surface;
} cbrushside_t;

typedef struct
{
	int		contents;
	int		numsides;
	int		firstbrushside;
	int		traceFrame;			// to avoid repeated testings
} cbrush_t;

typedef struct
{
	int		numareaportals;
	int		firstareaportal;
	int		floodnum;			// if two areas have equal floodnums, they are connected
	int		floodvalid;
} carea_t;

char		map_name[MAX_QPATH];

static int			numbrushsides;
static cbrushside_t map_brushsides[MAX_MAP_BRUSHSIDES+6];	// extra for box hull

int			numtexinfo;								// global (for precache)
csurface_t	map_surfaces[MAX_MAP_TEXINFO];			// global (for precache)

static int			numplanes;
static cplane_t	map_planes[MAX_MAP_PLANES+12];		// extra for box hull

static int			numnodes;
static cnode_t		map_nodes[MAX_MAP_NODES+6];		// extra for box hull

static int			numleafs = 1;	// allow leaf funcs to be called without a map
static dleaf_t		map_leafs[MAX_MAP_LEAFS];

static int			emptyleaf, solidleaf;

static int			numleafbrushes;
static unsigned short map_leafbrushes[MAX_MAP_LEAFBRUSHES+1];	// extra for box hull

static int			numcmodels;
static cmodel_t	*map_cmodels;

static int			numbrushes;
static cbrush_t	map_brushes[MAX_MAP_BRUSHES+1];		// extra for box hull

static int			numvisibility;
static byte		*map_visibility;

static int			numentitychars;
static char		*map_entitystring;

static int			numareas = 1;
static carea_t		map_areas[MAX_MAP_AREAS];

static int			numareaportals;
static dareaportal_t *map_areaportals;

static int			numclusters = 1;

static csurface_t	nullsurface;

static int			floodvalid;

static bool portalopen[MAX_MAP_AREAPORTALS];


static cvar_t		*map_noareas;

void	CM_InitBoxHull (void);
void	FloodAreaConnections (void);


int		c_traces, c_pointcontents;

bspfile_t *map_bspfile;

bool	map_clientLoaded;

/* Support for Half-Life maps ?? */

static mapType_t maptype;
static int firstnode; // 0->firstclipnode, this->firstnode ??


/*
===============================================================================

					MATERIAL-BY-TEXTURE

===============================================================================
*/


class surfMaterial_t : public CStringItem
{
public:
	material_t material;
};


static CMemoryChain *surfMaterialChain;
static TList<surfMaterial_t> surfMaterialList;


static void CMod_ReadSurfMaterials (char *filename)
{
	if (surfMaterialChain)
	{
		delete surfMaterialChain;
		surfMaterialChain = NULL;
		surfMaterialList.Reset();
	}

	char *file;
	if (!(file = (char*) FS_LoadFile (filename)))
	{
		Com_DPrintf ("ReadSurfMaterials: %s is not found\n", filename);
		return;
	}

	surfMaterialChain = new CMemoryChain;
	surfMaterial_t *prev = NULL;
	char *in = file;

	const char *s;
	while (s = COM_Parse (in), in)
	{
		material_t	m;
		surfMaterial_t *sm;

		// s points to material name string
		char c = s[0];
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

		s = COM_Parse (in);
		if (!in)
		{
			Com_WPrintf ("Unexpected end of file %s\n", filename);
			break;
		}
		// s points to surface name
		sm = new (s) surfMaterial_t;
		sm->material = m;
		// add to list
		surfMaterialList.InsertAfter (sm, prev);

		prev = sm;
//		Com_DPrintf ("Added surface material: %d for %s\n", m, s);
	}

	FS_FreeFile (file);
}

// name must be without "textures/" prefix and without extension
material_t CMod_GetSurfMaterial (char *name)
{
	char	*checkname;		// points to a name without path

	checkname = strrchr (name, '/');
	if (checkname)
		checkname++;
	else
		checkname = name;

	for (surfMaterial_t *sm = surfMaterialList.First(); sm; sm = surfMaterialList.Next(sm))
	{
		const char *s = sm->name;
		if (strchr (s, '/'))
		{	// mask have a path separator - compare full names
			if (!appMatchWildcard (name, sm->name, true))
				continue;
		}
		else
		{	// compare without path
			if (!appMatchWildcard (checkname, sm->name, true))
				continue;
		}

		material_t m = sm->material;
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
static void CMod_LoadSubmodels (cmodel_t *data, int count)
{
	map_cmodels = data;
	numcmodels = count;
	if (map_cmodels[0].headnode)
		Com_DropError ("map has invalid headnode = %d", map_cmodels[0].headnode);
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
		Com_DropError ("Map with no surfaces");
	if (size > MAX_MAP_TEXINFO)
		Com_DropError ("Map has too many surfaces");

	CMod_ReadSurfMaterials ("sound/materials.lst");

	numtexinfo = size;
	out = map_surfaces;

	for (i = 0; i < size; i++, in++, out++)
	{
		int		f, m;

		appStrncpyz (out->name, in->texture, sizeof(out->name));		// texture name limited to 16 chars (compatibility with older mods?)
		appStrncpyz (out->rname, in->texture, sizeof(out->rname));	// name limited to 32 chars (full?)
		f = in->flags;
		out->flags = f & ~SURF_KP_MATERIAL;
		out->value = in->value;

		if (f & SURF_CONCRETE)		m = MATERIAL_CONCRETE;
		else if (f & SURF_FABRIC)	m = MATERIAL_FABRIC;
		else if (f & SURF_GRAVEL)	m = MATERIAL_GRAVEL;
		else if (f & SURF_METAL) 	m = MATERIAL_METAL;
		else if (f & SURF_METAL_L)	m = MATERIAL_METAL_L;
		else if (f & SURF_SNOW)		m = MATERIAL_TIN;		//?? MATERIAL_SNOW;
		else if (f & SURF_TILE)		m = MATERIAL_TILE;
		else if (f & SURF_WOOD)		m = MATERIAL_WOOD;
		else if (f & SURF_WATER)	m = MATERIAL_WATER;
		else						m = CMod_GetSurfMaterial (in->texture);
		out->material = m;
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
		Com_DropError ("Map has no nodes");
	else if (size > MAX_MAP_NODES)
		Com_DropError ("Map has too many nodes");

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
		Com_DropError ("Map has too many brushes");

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
		Com_DropError ("Map with no leafs");
	if (size > MAX_MAP_LEAFS)
		Com_DropError ("Map has too many leafs");

	memcpy (map_leafs, data, sizeof(dleaf_t)*size); // must perform this, because needs 1 more leaf for InitBoxHull()
//--	map_leafs = data;
	numleafs = size;

	in = data;
	numclusters = 0;

	for (i = 0; i < size; i++, in++)
		if (in->cluster >= numclusters)
			numclusters = in->cluster + 1;

	if (map_leafs[0].contents != CONTENTS_SOLID)
		Com_DropError ("Map leaf 0 is not CONTENTS_SOLID");
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
		Com_DropError ("Map does not have an empty leaf");
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
		Com_DropError ("Map with no planes");
	if (size > MAX_MAP_PLANES)
		Com_DropError ("Map has too many planes");

	out = map_planes;
	numplanes = size;

	for (i = 0; i < size; i++, in++, out++)
	{
		VectorCopy (in->normal, out->normal);
		out->dist = in->dist;
		out->type = PlaneTypeForNormal (out->normal);	// in->type is incorrect for negative normals!
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
		Com_DropError ("Map with no leafbrushes");
	if (size > MAX_MAP_LEAFBRUSHES)
		Com_DropError ("Map has too many leafbrushes");

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
		Com_DropError ("Map has too many brushsides");

	out = map_brushsides;
	numbrushsides = size;

	for (i = 0; i < size; i++, in++, out++)
	{
		out->plane = &map_planes[in->planenum];
		j = in->texinfo;
		if (j >= numtexinfo)
			Com_DropError ("Bad brushside texinfo");
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
		Com_DropError ("Map has too many areas");

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
static void CMod_LoadEntityString (char *data, int size)
{
	numentitychars = size;
	map_entitystring = data;
}



/*
==================

HALF-LIFE map support

==================
*/

#if 0

void CMod_LoadHLSurfaces (lump_t *l)
{
	hl_texinfo_t	*in;
	csurface_t	*out;
	int			i, count;

//--	in = (void *)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Com_DropError ("MOD_LoadBmodel: funny lump size");
	count = l->filelen / sizeof(*in);
	if (count < 1)
		Com_DropError ("Map with no surfaces");
	if (count > MAX_MAP_TEXINFO)
		Com_DropError ("Map has too many surfaces");

	numtexinfo = count;
	out = map_surfaces;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		char buf[32];//!!
		appSprintf(ARRAY_ARG(buf),"surf_%x",i);
		strcpy (out->name, buf); //!!
		strcpy (out->rname, buf);  //!!
//??		strncpy (out->name, in->texture, sizeof(out->name)-1);
//??		strncpy (out->rname, in->texture, sizeof(out->rname)-1);
		out->flags = LittleLong (in->flags);
		out->value = 0; // LittleLong (in->value); - no in HL
	}
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
	hl_dleaf_t 	*in;
	dleaf_t 	*out;
	int		i, count, p;

//--	in = (void *)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Com_DropError ("MOD_LoadBmodel: funny lump size");
	count = l->filelen / sizeof(*in);

	if (count < 1)
		Com_DropError ("Map with no leafs");
	// need to save space for box planes
	if (count > MAX_MAP_LEAFS)
		Com_DropError ("Map has too many leafs");

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
			Com_DropError ("Unknown leaf contents: %d", p);
		}

		out->contents = p;
		out->cluster = LittleLong(in->visofs); // will be used later in Mod_LoadHLVisibility()
		out->area = 0; // no areas in Q1/HL

		out->firstleafbrush = LittleShort(in->firstmarksurface); // temporary store (for LoadHLBrushes())
		out->numleafbrushes = LittleShort(in->nummarksurfaces);  //
	}

	if (map_leafs[0].contents != CONTENTS_SOLID)
		Com_DropError ("Map leaf 0 is not CONTENTS_SOLID");
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
		Com_DropError ("Map does not have an empty leaf");
}


/*
 * Generate dvis_t structure { numclusters: int; bitofs: array[][2] of int }
 * SHOULD BE CALLED AFTER CMod_LoadHLLeafs() !
 */
void CMod_LoadHLVisibility (lump_t *l)
{
	int		i, size, vis;

	numvisibility = l->filelen;
	if (l->filelen > MAX_MAP_VISIBILITY)
		Com_DropError ("Map has too large visibility lump");

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
	}
}


void CMod_LoadHLSubmodels (lump_t *l)
{
	hl_dmodel_t	*in;
	cmodel_t	*out;
	int		i, j, count;

//--	in = (void *)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Com_DropError ("MOD_LoadBmodel: funny lump size");
	count = l->filelen / sizeof(*in);

	if (count < 1)
		Com_DropError ("Map with no models");
	if (count > MAX_MAP_MODELS)
		Com_DropError ("Map has too many models");

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
		Com_DropError ("HL map has invalid headnode = %d", map_cmodels[0].headnode);
}


void CMod_LoadHLClipnodes (lump_t *l)
{
	hl_dclipnode_t	*in;
	cnode_t		*out;
	int		i, j, count, child;
	dleaf_t		*leaf;

//--	in = (void *)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Com_DropError ("MOD_LoadBmodel: funny lump size");
	count = l->filelen / sizeof(*in);

	if (count < 1)
		Com_DropError ("Map has no nodes");
	if (count > MAX_MAP_NODES)
		Com_DropError ("Map has too many nodes");

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
					Com_DropError ("Mod_LoadHLClipnodes: not enough space to generate leafs");

				out->children[j] = -1 - numleafs; // points to leaf, that will be created below

				leaf = &map_leafs[numleafs++];

				if (child < 0 && child >= -HL_CONTENTS)
					child = HLContents[-child - 1];
				else
					Com_DropError ("Unknown node contents: %d", child);
				leaf->contents = child;
				leaf->cluster = i;//!! -2; // -1 - no cluster, -2 - calculate with nodes/leafs
				leaf->area = j;//!! 0; // HL map has no areas
				leaf->firstleafbrush = 0;
				leaf->numleafbrushes = 0;
			}
		}
	}
}


void CMod_LoadHLNodes (lump_t *l)
{
	hl_dnode_t	*in;
	cnode_t		*out;
	int		i, j, count, child;

//--	in = (void *)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Com_DropError ("MOD_LoadBmodel: funny lump size");
	count = l->filelen / sizeof(*in);

	if (count < 1)
		Com_DropError ("Map has no nodes");
	if (count + numnodes > MAX_MAP_NODES)
		Com_DropError ("Map has too many nodes");

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
	}
}


void CMod_LoadHLAreas ()
{
	numareas = 1;
	map_areas[0].numareaportals = 0;
	map_areas[0].firstareaportal = 0;
	map_areas[0].floodvalid = 0;
	map_areas[0].floodnum = 0;
}


// Generate brushes
void CMod_LoadHLBrushes ()
{
	dleaf_t		*in;
	cbrush_t	*out;
	int			i, count;

	in = map_leafs;
	count = numleafs;
	out = map_brushes;
	numbrushes = count;

	if (count > MAX_MAP_BRUSHES)
		Com_DropError ("Map has too many brushes");

	for (i = 0; i < count; i++, out++, in++)
	{
		out->firstbrushside = in->firstleafbrush; // hl_dleaf_t->firstmarksurface
		out->numsides = in->numleafbrushes;       // hl_dleaf_t->nummarksurfaces
		Com_Printf(",%d",out->numsides); //!!
		in->firstleafbrush = i; // points to current structure
		in->numleafbrushes = 1;
		out->contents = in->contents;
	}
}


// Generate brushsides from FACES, MARKSURFACES, VERTEXES, EDGES and SURFEDGES
void CMod_LoadHLBrushSides (lump_t *lf, lump_t *lm, lump_t *lv, lump_t *le, lump_t *lse)
{
	int		i;
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
		Com_DropError ("MOD_LoadBmodel: funny lump size");

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
			Com_DropError ("Bad marksurface");

		VectorClear (avg);
		vert_ct = 0;

		in->numsides = 0; //??

		for (k = 0; k < n; k++) // out++, count++
		{
			int	j, num, n1, m;
			dface_t	*face;

			if (count >= MAX_MAP_BRUSHSIDES)
				Com_DropError ("Map has too many brushsides");

			num = LittleShort (marksurfs[brushside + k]); // number of face
			if (num > numfaces)
				Com_DropError ("Bad brushside face");
			face = &faces[num];

			//?? skip planebacks
//??			if (LittleShort (face->side)) continue;

			num = LittleShort (face->planenum);
			out->plane = &map_planes[num];
			j = LittleShort (face->texinfo);
			if (j >= numtexinfo)
				Com_DropError ("Bad brushside texinfo");
			out->surface = &map_surfaces[j];

			out++;
			count++;
			in->numsides++; //??
		}
	}
	numbrushsides = count;
}


// generate leafbrushes
void CMod_LoadHLLeafBrushes ()
{
	int	i, count;

	count = numleafs;

	if (count > MAX_MAP_LEAFBRUSHES)
		Com_DropError ("Map has too many leafbrushes");

	numleafbrushes = count;

	for (i = 0; i < count; i++)
		map_leafbrushes[i] = i;
}

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

#endif

/*
==================
CM_LoadMap

Loads in the map and all submodels
==================
*/
cmodel_t *CM_LoadMap (char *name, bool clientload, unsigned *checksum)
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
		return &map_cmodels[0];
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
	map_bspfile = NULL;

	if (!name || !name[0])
	{
		numleafs = 1;
		numclusters = 1;
		numareas = 1;
		*checksum = 0;
		map_clientLoaded = false;
		return &map_cmodels[0];			// cinematic servers won't have anything at all
	}

	bsp = map_bspfile = LoadBspFile (name, clientload, &last_checksum);
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

	if (!name || name[0] != '*') Com_DropError ("CM_InlineModel: bad name");

	num = atoi (name+1);
	if (num < 1 || num >= numcmodels) Com_DropError ("CM_InlineModel: bad number");

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
	if (leafnum < 0 || leafnum >= numleafs) Com_DropError ("CM_LeafContents: bad number");
	return map_leafs[leafnum].contents;
}

int CM_LeafCluster (int leafnum)
{
	if (leafnum < 0 || leafnum >= numleafs) Com_DropError ("CM_LeafCluster: bad number");
	guard(CM_LeadCluster);
	return map_leafs[leafnum].cluster;
	unguard;
}

int CM_LeafArea (int leafnum)
{
	if (leafnum < 0 || leafnum >= numleafs) Com_DropError ("CM_LeafArea: bad number");
	return map_leafs[leafnum].area;
}

//=======================================================================


static cplane_t	*box_planes;
static int		box_headnode;
static cbrush_t	*box_brush;
static dleaf_t	*box_leaf;

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

	box_brush = &map_brushes[numbrushes];
	box_brush->numsides = 6;
	box_brush->firstbrushside = numbrushsides;
	box_brush->contents = CONTENTS_MONSTER;

	box_leaf = &map_leafs[numleafs];
	box_leaf->contents = CONTENTS_MONSTER;
	box_leaf->firstleafbrush = numleafbrushes;
	box_leaf->numleafbrushes = 1;

	map_leafbrushes[numleafbrushes] = numbrushes;

	for (i = 0; i < 6; i++)
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
		p->type = PLANE_X + (i>>1);
		VectorClear (p->normal);
		p->normal[i>>1] = 1;
		SetPlaneSignbits (p);

		p = &box_planes[i*2+1];
		p->type = PLANE_MX + (i>>1);
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


/*-----------------------------------------------------------------------------
	Point info
-----------------------------------------------------------------------------*/

int CM_PointLeafnum (vec3_t p, int num)
{
	guard(CM_PointLeafnum);
	if (!numplanes) return 0;			// map is not yet loaded
	while (num >= 0)
	{
		cnode_t	*node;
		float	d;

		node = map_nodes + num;
		d = DISTANCE_TO_PLANE(p, node->plane);
		num = node->children[IsNegative(d)];
	}

	c_pointcontents++;					// stats
	unguard;
	return -1 - num;
}


int CM_PointContents (vec3_t p, int headnode)
{
	if (!numnodes) return 0;			// map not loaded

	guard(CM_PointContents);
	return map_leafs[CM_PointLeafnum (p, headnode)].contents;
	unguard;
}


int	CM_TransformedPointContents (vec3_t p, int headnode, vec3_t origin, vec3_t angles)
{
	vec3_t	p1, tmp, axis[3];

	guard(CM_TransformedPointContents);
	if (headnode != box_headnode && (angles[0] || angles[1] || angles[2]))
	{
		AnglesToAxis (angles, axis);
		VectorSubtract (p, origin, tmp);
		p1[0] = DotProduct (tmp, axis[0]);
		p1[1] = DotProduct (tmp, axis[1]);
		p1[2] = DotProduct (tmp, axis[2]);
	}
	else
		VectorSubtract (p, origin, p1);

	return map_leafs[CM_PointLeafnum (p1, headnode)].contents;
	unguard;
}

int	CM_TransformedPointContents2 (vec3_t p, int headnode, vec3_t origin, vec3_t *axis)
{
	vec3_t	p1, tmp;

	guard(CM_TransformedPointContents2);
	if (headnode != box_headnode)
	{
		VectorSubtract (p, origin, tmp);
		p1[0] = DotProduct (tmp, axis[0]);
		p1[1] = DotProduct (tmp, axis[1]);
		p1[2] = DotProduct (tmp, axis[2]);
	}
	else
		VectorSubtract (p, origin, p1);

	return map_leafs[CM_PointLeafnum (p1, headnode)].contents;
	unguard;
}


/*-----------------------------------------------------------------------------
	Box info
-----------------------------------------------------------------------------*/

int CM_BoxLeafnums (vec3_t mins, vec3_t maxs, int *list, int listsize, int *topnode, int headnode)
{
	int		count, _topnode, nodenum;
	int		stack[MAX_TREE_DEPTH], sptr;		// stack

	guard(CM_BoxLeafnums);

	count = 0;
	_topnode = -1;
	nodenum = headnode;

	sptr = 0;
	while (true)
	{
		if (nodenum < 0)
		{
			if (count >= listsize) break;		// list is full

			list[count++] = -1 - nodenum;

			if (!sptr)
				break;							// whole tree visited
			else
			{
				nodenum = stack[--sptr];
				continue;
			}
		}

		cnode_t *node = &map_nodes[nodenum];
		switch (BOX_ON_PLANE_SIDE(mins, maxs, node->plane))
		{
		case 1:
			nodenum = node->children[0];
			break;
		case 2:
			nodenum = node->children[1];
			break;
		default:
			// go down both
			if (_topnode == -1)
				_topnode = nodenum;				// remember top node, which subdivides box
			nodenum = node->children[0];
			stack[sptr++] = node->children[1];
		}
	}

	if (topnode) *topnode = _topnode;
	unguard;

	return count;
}


/*-----------------------------------------------------------------------------
	Box trace
-----------------------------------------------------------------------------*/

#define	DIST_EPSILON	(1.0f/32)

static vec3_t	trace_start, trace_end;
static vec3_t	trace_mins, trace_maxs;
static vec3_t	trace_extents;

static trace_t	trace_trace;
static int		trace_contents;
static bool		trace_ispoint;		// optimized case


/*
================
ClipBoxToBrush
================
*/
static void ClipBoxToBrush (const cbrush_t *brush)
{
	int			i;
	cplane_t	*clipplane;
	float		enterfrac, leavefrac;
	bool		getout, startout;
	cbrushside_t *leadside, *side;

	if (!brush->numsides)
		return;

	enterfrac = -1;
	leavefrac = 1;						// used only for validating enterfrac
	clipplane = NULL;

	getout = startout = false;
	leadside = NULL;

	for (i = 0, side = &map_brushsides[brush->firstbrushside]; i < brush->numsides; i++, side++)
	{
		cplane_t *plane;
		float	d1, d2, dist, f;

		plane = side->plane;

		if (!trace_ispoint)
		{	// general box case
			vec3_t	ofs;
			int		j;

			if (plane->type <= PLANE_Z)
			{
				// HERE: plane.normal[i] == 0 || 1 for i==type
				dist = plane->dist - trace_mins[plane->type];
			}
			else if (plane->type <= PLANE_MZ)
			{
				// HERE: plane.normal[i] == 0 || -1 for i==type-3
				dist = plane->dist + trace_maxs[plane->type-3];
			}
			else
			{
				for (j = 0; j < 3; j++)
					ofs[j] = (plane->normal[j] < 0) ? trace_maxs[j] : trace_mins[j];
				dist = plane->dist - DotProduct (plane->normal, ofs);
			}
		}
		else
		{	// special point case
			dist = plane->dist;
		}

		if (plane->type <= PLANE_Z)
		{
			d1 = trace_start[plane->type] - dist;
			d2 = trace_end[plane->type] - dist;
		}
		else if (plane->type <= PLANE_MZ)
		{
			d1 = -trace_start[plane->type-3] - dist;
			d2 = -trace_end[plane->type-3] - dist;
		}
		else
		{
			d1 = DotProduct (plane->normal, trace_start) - dist;
			d2 = DotProduct (plane->normal, trace_end) - dist;
		}
		// d1 and d2: 0 -- on plane, <0 -- inside brush plane, >0 -- outside brush plane

		if (d1 > 0 && d2 > 0)
		{
			/* math note: here 2 situations
			 * 1) d1 > d2    -- enterfrac will be > 1
			 * 2) d1 < d2    -- leavefrac will be < 0
			 */
			return;
		}

		if (d1 <= 0 && d2 <= 0)			// completely in back of this face
			continue;

		if (d1 > 0) startout = true;	// startpoint is not inside this brush
		if (d2 > 0) getout = true;		// endpoint is not inside this brush

		// crosses face
		if (d1 > d2)
		{	// enter
			f = (d1-DIST_EPSILON) / (d1 - d2);
			if (f > enterfrac)
			{
				enterfrac = f;
				clipplane = plane;
				leadside = side;
			}
		}
		else
		{	// leave
			f = (d1+DIST_EPSILON) / (d1 - d2);
			if (f < leavefrac)
				leavefrac = f;
		}

		if (enterfrac >= leavefrac) return;		// intersects (at least) 2 faces outside the brush
	}

	if (!startout)
	{
		// original point was inside brush
		trace_trace.startsolid = true;
		trace_trace.allsolid = !getout;
		return;
	}

	if (enterfrac > -1 && enterfrac < trace_trace.fraction)
	{
		if (enterfrac < 0)			// when startsolid && allsolid
			enterfrac = 0;
		trace_trace.fraction = enterfrac;
		trace_trace.plane = *clipplane;
		trace_trace.surface = leadside->surface;
		trace_trace.contents = brush->contents;
	}
}

/*
================
TraceToLeaf
================
*/
bool trace_skipAlpha;		//!! need another way to pass through SURF_ALPHA (callbacks??); used in SV_TraceHook()
static int traceFrame;

static void TraceToLeaf (int leafnum)
{
	dleaf_t *leaf = &map_leafs[leafnum];
	if (!(leaf->contents & trace_contents))
		return;
	// trace line against all brushes in the leaf
	for (int i = 0; i < leaf->numleafbrushes; i++)
	{
		cbrush_t *b = map_brushes + map_leafbrushes[leaf->firstleafbrush + i];
		if (b->traceFrame == traceFrame)
			continue;					// already checked this brush in another leaf
		b->traceFrame = traceFrame;
		if (!(b->contents & trace_contents))
			continue;
		if (trace_skipAlpha && b->contents & CONTENTS_ALPHA)
			continue;

		ClipBoxToBrush (b);
		if (!trace_trace.fraction)		// when startsolid && allsolid
			return;
	}
}


/*
================
TestBoxInBrush

if box inside this brush or intersects its brushsides:
 1) trace->fraction=0
 2) trace->startsolid = true
 3) trace->allsolid = true
else trace is unchanged
================
*/
static bool TestBoxInBrush (const cbrush_t *brush)
{
	int		i, j;
	cbrushside_t *side, *clipside;
	cplane_t *clipplane;
	float	clipdist;

	if (!brush->numsides)
		return false;

	clipdist = -999999;
	clipside = NULL;
	clipplane = NULL;
	for (i = 0, side = &map_brushsides[brush->firstbrushside]; i < brush->numsides; i++, side++)
	{
		cplane_t *plane;
		float	dist, d;

		plane = side->plane;

		// FIXME: use signbits into 8 way lookup for each mins/maxs
		if (plane->type <= PLANE_Z)
			dist = plane->dist - trace_mins[plane->type];
		else if (plane->type <= PLANE_MZ)
			dist = plane->dist + trace_maxs[plane->type-3];
		else
		{
			vec3_t	ofs;

			for (j = 0; j < 3; j++)
				ofs[j] = (plane->normal[j] < 0) ? trace_maxs[j] : trace_mins[j];
			dist = plane->dist - DotProduct (plane->normal, ofs);
		}

		if (plane->type <= PLANE_Z)
			d = trace_start[plane->type] - dist;
		else if (plane->type <= PLANE_MZ)
			d = -trace_start[plane->type-3] - dist;
		else
			d = DotProduct (plane->normal, trace_start) - dist;

		// if completely in front of face, no intersection
		if (d > 0) return false;
		if (d > clipdist)
		{
			clipdist = d;
			clipplane = plane;
			clipside = side;
		}
	}

	// inside this brush
	trace_trace.startsolid = trace_trace.allsolid = true;
	trace_trace.fraction = 0;
	trace_trace.contents = brush->contents;
	trace_trace.plane = *clipplane;
	trace_trace.surface = clipside->surface;

	return true;
}


/*
================
TestInLeaf

Call TestBoxInBrush for each leafbrush.
When first intersection found (trace.fraction == 0) - return
================
*/
static void TestInLeaf (int leafnum)
{
	dleaf_t *leaf = &map_leafs[leafnum];
	if (!(leaf->contents & trace_contents))
		return;

	// trace line against all brushes in the leaf
	for (int i = 0; i < leaf->numleafbrushes; i++)
	{
		cbrush_t *b = map_brushes + map_leafbrushes[leaf->firstleafbrush + i];
		if (b->traceFrame == traceFrame)
			continue;	// already checked this brush in another leaf
		b->traceFrame = traceFrame;

		if (!(b->contents & trace_contents))
			continue;
		if (TestBoxInBrush (b)) return;
	}
}


/*
==================
RecursiveHullCheck

==================
*/
static void RecursiveHullCheck (int nodeNum, float p1f, float p2f, const vec3_t p1, const vec3_t p2)
{
	cnode_t		*node;
	cplane_t	*plane;
	float		t1, t2, offset;
	float		frac1, frac2, midf;
	int			i;
	vec3_t		mid, p1a;
	int			side;

	if (trace_trace.fraction <= p1f)
		return;		// already hit something nearer (??)

	while (true)
	{
		// if < 0, we are in a leaf node
		if (nodeNum < 0)
		{
			TraceToLeaf (-1-nodeNum);
			return;
		}

		// find the point distances to the separating plane
		// and the offset for the size of the box
		node = map_nodes + nodeNum;
		plane = node->plane;

		if (plane->type <= PLANE_Z)
		{
			t1 = p1[plane->type] - plane->dist;
			t2 = p2[plane->type] - plane->dist;
			offset = trace_extents[plane->type];
		}
		else if (plane->type <= PLANE_MZ)
		{
			t1 = -p1[plane->type-3] - plane->dist;
			t2 = -p2[plane->type-3] - plane->dist;
			offset = trace_extents[plane->type-3];
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
			nodeNum = node->children[0];
			continue;
		}
		if (t1 < -offset && t2 < -offset)
		{
			nodeNum = node->children[1];
			continue;
		}

		// put the crosspoint DIST_EPSILON pixels on the near side
		if (t1 == t2)
		{
			side = 0;
			frac1 = 1;
			frac2 = 0;
		}
		else
		{
			float	idist;

			idist = 1.0f / (t1 - t2);
			offset += DIST_EPSILON;
			if (t1 < t2)
			{
				frac1 = (t1 - offset) * idist;
				frac2 = (t1 + offset) * idist;
				side = 1;
			}
			else if (t1 > t2)
			{
				frac1 = (t1 + offset) * idist;
				frac2 = (t1 - offset) * idist;
				side = 0;
			}

			frac1 = bound(frac1, 0, 1);
			frac2 = bound(frac2, 0, 1);
		}


/*	{//!!!!
		char buf[256];
		appSprintf(ARRAY_ARG(buf),"RHC: divide (%d): t1=%g t2=%g offs=%g {N:%d %g-%g} {N:%d %g-%g} plane(d:%g %g,%g,%g)",num,t1,t2,offset,
			node->children[side], p1f, p1f + (p2f - p1f)*frac,
                        node->children[side^1], midf = p1f + (p2f - p1f)*frac2, p2f,
                        plane->dist,plane->normal[0],plane->normal[1],plane->normal[2]);
		re.DrawTextRight(buf,RGB(1,0.3,0.3));
	}//===*/

		// move up to the node
		midf = p1f + frac1 * (p2f - p1f);
		for (i = 0; i < 3; i++)
			mid[i] = p1[i] + frac1 * (p2[i] - p1[i]);

		RecursiveHullCheck (node->children[side], p1f, midf, p1, mid);


		// go past the node
		midf = p1f + frac2 * (p2f - p1f);
		for (i = 0; i < 3; i++)
			p1a[i] = p1[i] + frac2 * (p2[i] - p1[i]);

//--	RecursiveHullCheck (node->children[side^1], midf, p2f, mid, p2);
		p1f = midf;
		nodeNum = node->children[side^1];
		p1 = p1a;
	}
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
//?? check: can this be faster if trace with sphere
void CM_BoxTrace (trace_t *trace, const vec3_t start, const vec3_t end, const vec3_t mins, const vec3_t maxs, int headnode, int brushmask)
{
	int		i;

	guard(CM_BoxTrace);

	traceFrame++;						// for multi-check avoidance
	c_traces++;

	// fill in a default trace
	memset (&trace_trace, 0, sizeof(trace_trace));
	trace_trace.fraction = 1;
	trace_trace.surface = &nullsurface;

	if (!numnodes)	// map not loaded
	{
		*trace = trace_trace;
		return;
	}

	trace_contents = brushmask;
	VectorCopy (start, trace_start);
	VectorCopy (end, trace_end);
	VectorCopy (mins, trace_mins);
	VectorCopy (maxs, trace_maxs);

	// check for "position test" special case
	if (start[0] == end[0] && start[1] == end[1] && start[2] == end[2])
	{
		int		leafs[1024], numleafs;
		vec3_t	c1, c2;

		for (i = 0; i < 3; i++)
		{
			c1[i] = start[i] + mins[i] - 1;
			c2[i] = start[i] + maxs[i] + 1;
		}

		numleafs = CM_BoxLeafnums (c1, c2, ARRAY_ARG(leafs), NULL, headnode);
		for (int i = 0; i < numleafs; i++)
		{
			TestInLeaf (leafs[i]);
			if (trace_trace.allsolid)	// always set when 1st intersection by CM_TestBoxInBrush()
				break;
		}
		VectorCopy (start, trace_trace.endpos);
		*trace = trace_trace;
		return;
	}

	// check for "point" special case
	if (mins[0] == 0 && mins[1] == 0 && mins[2] == 0 &&
		maxs[0] == 0 && maxs[1] == 0 && maxs[2] == 0)
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
	RecursiveHullCheck (headnode, 0, 1, start, end);

	if (trace_trace.fraction == 1)
	{
		VectorCopy (end, trace_trace.endpos);
	}
	else
	{
		for (i = 0; i < 3; i++)
			trace_trace.endpos[i] = start[i] + trace_trace.fraction * (end[i] - start[i]);
	}

	*trace = trace_trace;
	return;
	unguard;
}


/*
==================
CM_TransformedBoxTrace

Handles offseting and rotation of the end points for moving and
rotating entities
==================
*/


void CM_TransformedBoxTrace (trace_t *trace, const vec3_t start, const vec3_t end, const vec3_t mins, const vec3_t maxs,
	int headnode, int brushmask, const vec3_t origin, const vec3_t angles)
{
	vec3_t	start1, end1, tmp;
	vec3_t	axis[3];
	bool	rotated;

	guard(CM_TransformedBoxTrace);

	// rotate start and end into the models frame of reference
	if (headnode != box_headnode && (angles[0] || angles[1] || angles[2]))
		rotated = true;
	else
		rotated = false;

	if (rotated)
	{
		AnglesToAxis (angles, axis);

		// transform start/end to axis (model coordinate system)
		VectorSubtract (start, origin, tmp);
		start1[0] = DotProduct (tmp, axis[0]);
		start1[1] = DotProduct (tmp, axis[1]);
		start1[2] = DotProduct (tmp, axis[2]);

		VectorSubtract (end, origin, tmp);
		end1[0] = DotProduct (tmp, axis[0]);
		end1[1] = DotProduct (tmp, axis[1]);
		end1[2] = DotProduct (tmp, axis[2]);
	}
	else
	{
		VectorSubtract (start, origin, start1);
		VectorSubtract (end, origin, end1);
	}

	// sweep the box through the model
	CM_BoxTrace (trace, start1, end1, mins, maxs, headnode, brushmask);

	// transform normal/endpos to world coordinate system
	if (trace->fraction != 1.0f && rotated)
	{
		VectorScale (axis[0], trace->plane.normal[0], tmp);
		VectorMA (tmp, trace->plane.normal[1], axis[1], tmp);
		VectorMA (tmp, trace->plane.normal[2], axis[2], trace->plane.normal);
	}

	if (rotated)
	{
		VectorMA (origin, trace->endpos[0], axis[0], tmp);
		VectorMA (tmp, trace->endpos[1], axis[1], tmp);
		VectorMA (tmp, trace->endpos[2], axis[2], trace->endpos);
	}
	else
		VectorAdd (trace->endpos, origin, trace->endpos);

	unguard;
}


void CM_TransformedBoxTrace2 (trace_t *trace, const vec3_t start, const vec3_t end, const vec3_t mins, const vec3_t maxs,
	int headnode, int brushmask, const vec3_t origin, const vec3_t *axis)
{
	vec3_t		start1, end1, tmp;

	guard(CM_TransformedBoxTrace2);

	// transform start/end to axis (model coordinate system)
	VectorSubtract (start, origin, tmp);
	start1[0] = DotProduct (tmp, axis[0]);
	start1[1] = DotProduct (tmp, axis[1]);
	start1[2] = DotProduct (tmp, axis[2]);

	VectorSubtract (end, origin, tmp);
	end1[0] = DotProduct (tmp, axis[0]);
	end1[1] = DotProduct (tmp, axis[1]);
	end1[2] = DotProduct (tmp, axis[2]);

	// sweep the box through the model
	CM_BoxTrace (trace, start1, end1, mins, maxs, headnode, brushmask);

	// transform normal/endpos to world coordinate system
	if (trace->fraction != 1.0f)
	{
		VectorScale (axis[0], trace->plane.normal[0], tmp);
		VectorMA (tmp, trace->plane.normal[1], axis[1], tmp);
		VectorMA (tmp, trace->plane.normal[2], axis[2], trace->plane.normal);
	}

	VectorMA (origin, trace->endpos[0], axis[0], tmp);
	VectorMA (tmp, trace->endpos[1], axis[1], tmp);
	VectorMA (tmp, trace->endpos[2], axis[2], trace->endpos);

	unguard;
}


/*-----------------------------------------------------------------------------
	Occlusion test
-----------------------------------------------------------------------------*/

static int trace_numBrushes, trace_maxBrushes;
static int *trace_brushes;


// returns true when start->end line intersects brush
static bool TestBrush (const vec3_t start, const vec3_t end, const cbrush_t *brush)
{
	int		i;
	cbrushside_t *side;

	float ef = -1;					// enterfrac
	float lf = 1;					// leavefrac
	for (i = 0, side = &map_brushsides[brush->firstbrushside]; i < brush->numsides; i++, side++)
	{
		float	d1, d2, f;

		cplane_t *plane = side->plane;
		if (plane->type <= PLANE_Z)
		{
			d1 = start[plane->type] - plane->dist;
			d2 = end[plane->type] - plane->dist;
		}
		else if (plane->type <= PLANE_MZ)
		{
			d1 = -start[plane->type-3] - plane->dist;
			d2 = -end[plane->type-3] - plane->dist;
		}
		else
		{
			d1 = DotProduct (plane->normal, start) - plane->dist;
			d2 = DotProduct (plane->normal, end) - plane->dist;
		}
		// d1 and d2: 0 -- on plane, <0 -- inside brush plane, >0 -- outside brush plane

		if (d1 > 0 && d2 > 0)		// start and end points are outside this plane
			return false;
		if (d1 < 0 && d2 < 0)		// both points inside plane
			continue;

		// check for situation, when line crosses 2 planes outside brush (in this case,
		// line will leave brush before enter -- "inside-out")
		// here: d1 != d1 (have different signs)
		f = d1 / (d1 - d2);
		if (d1 > d2)
		{
			if (f > ef) ef = f;
		}
		else
		{
			if (f < lf) lf = f;
		}
		if (ef > lf) return false;
	}
	return true;
}


static void RecursiveBrushTest (const vec3_t start, const vec3_t end, int nodeNum)
{
	cnode_t	*node;
	cplane_t *plane;
	float	frac1, frac2, t1, t2, idist;
	int		i, side, s1, s2;
	vec3_t	mid, start1;

	while (true)
	{
		if (trace_numBrushes >= trace_maxBrushes) return;		// buffer full

		if (nodeNum < 0)
		{
			dleaf_t	*leaf;

			//------------------ test leaf ---------------------
			leaf = &map_leafs[-1-nodeNum];
			if (!(leaf->contents & CONTENTS_SOLID)) return;		// we are checking only SOLID leafs

			for (i = 0; i < leaf->numleafbrushes; i++)
			{
				int		brushNum;
				cbrush_t *b;

				//-------------- test brush --------------------
				brushNum = map_leafbrushes[leaf->firstleafbrush + i];
				b = map_brushes + brushNum;
				if (b->traceFrame != traceFrame && (b->contents & CONTENTS_SOLID))
				{
					b->traceFrame = traceFrame;
					if (TestBrush (start, end, b))
					{
						trace_brushes[trace_numBrushes++] = brushNum;
						if (trace_numBrushes >= trace_maxBrushes)
							return;								// buffer full
					}
				}
			}
			return;
		}

		node = map_nodes + nodeNum;
		plane = node->plane;

		if (plane->type <= PLANE_Z)
		{
			t1 = start[plane->type] - plane->dist;
			t2 = end[plane->type] - plane->dist;
		}
		else if (plane->type <= PLANE_MZ)
		{
			t1 = -start[plane->type-3] - plane->dist;
			t2 = -end[plane->type-3] - plane->dist;
		}
		else
		{
			t1 = DotProduct (plane->normal, start) - plane->dist;
			t2 = DotProduct (plane->normal, end) - plane->dist;
		}

		s1 = IsNegative (t1); s2 = IsNegative (t2);
		if (!(s1 | s2))		// (t1 >= 0 && t2 >= 0)
		{
			nodeNum = node->children[0];
			continue;
		}
		if (s1 & s2)		// (t1 < 0 && t2 < 0)
		{
			nodeNum = node->children[1];
			continue;
		}

		// here: sign(t1) != sign(t2)
		idist = 1.0f / (t1 - t2);	// "t1 == t2" should not happen: different signs but same numbers
		if (t1 < t2)
		{
			frac1 = (t1 - DIST_EPSILON) * idist;
			frac2 = (t1 + DIST_EPSILON) * idist;
			side = 1;
		}
		else
		{
			frac1 = (t1 + DIST_EPSILON) * idist;
			frac2 = (t1 - DIST_EPSILON) * idist;
			side = 0;
		}
		frac1 = bound(frac1, 0, 1);
		frac2 = bound(frac2, 0, 1);

		// move up to the node
		for (i = 0; i < 3; i++)
			mid[i] = start[i] + frac1 * (end[i] - start[i]);

		RecursiveBrushTest (start, mid, node->children[side]);

		// go past the node
		for (i = 0; i < 3; i++)
			start1[i] = start[i] + frac2 * (end[i] - start[i]);

//--	RecursiveBrushTest (start1, end, node->children[side^1]);
		start = start1;
		nodeNum = node->children[side^1];
	}
}


// returns number of brushes, intersected with line
int CM_BrushTrace (const vec3_t start, const vec3_t end, int *brushes, int maxBrushes)
{
	guard(CM_BrushTrace);

	trace_numBrushes = 0;
	trace_brushes = brushes;
	trace_maxBrushes = maxBrushes;
	traceFrame++;

	RecursiveBrushTest (start, end, 0);
	return trace_numBrushes;

	unguard;
}


// gets list of brushes (taken from CM_BrushTrace() call) and removes brushes,
// which are not intersected with line "start-end"
int CM_RefineBrushTrace (const vec3_t start, const vec3_t end, int *brushes, int numBrushes)
{
	int		i, b, newNum, *src, *dst;

	guard(CM_RefineBrushTrace);

	src = dst = brushes;
	newNum = 0;
	for (i = 0; i < numBrushes; i++)
	{
		b = *src++;
		if (TestBrush (start, end, map_brushes + b))
		{
			*dst++ = b;
			newNum++;
		}
	}

	return newNum;
	unguard;
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
static void DecompressVis (const byte *in, byte *out)
{
	int		c;
	byte	*out_p;
	int		rowSize;

	rowSize = (numclusters + 7) >> 3;
	out_p = out;

	if (!in || !numvisibility)
	{
		// no vis info, so make all visible
		memset (out_p, 0xFF, rowSize);
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
		if ((out_p - out) + c > rowSize)
		{
			c = rowSize - (out_p - out);
			Com_DPrintf ("WARNING: Vis decompression overrun\n");
		}
		memset (out_p, 0, c);
		out_p += c;
	} while (out_p - out < rowSize);
}

static byte	pvsrow[MAX_MAP_LEAFS/8];
static byte	phsrow[MAX_MAP_LEAFS/8];

byte *CM_ClusterPVS (int cluster)
{
	int	i;

	guard(CM_ClusterPVS);

	if (cluster <= -1)
		memset (pvsrow, 0, (numclusters + 7) >> 3);
	else
	{
		i = ((dvis_t *)map_visibility)->bitofs[cluster][DVIS_PVS];
		if (i != -1)
			DecompressVis (map_visibility + i, pvsrow);
		else
			DecompressVis (NULL, pvsrow);
	}
	return pvsrow;
	unguard;
}

byte *CM_ClusterPHS (int cluster)
{
	int	i;

	guard(CM_ClusterPHS);

	if (cluster <= -1)
		memset (phsrow, 0, (numclusters + 7) >> 3);
	else
	{
		i = ((dvis_t *)map_visibility)->bitofs[cluster][DVIS_PHS];
		if (i != -1)
			DecompressVis (map_visibility + i, phsrow);
		else
			DecompressVis (NULL, phsrow);
	}
	return phsrow;
	unguard;
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
		Com_DropError ("FloodArea_r: reflooded");
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
	for (i = 1; i < numareas; i++)
	{
		area = &map_areas[i];
		if (area->floodvalid == floodvalid)
			continue;		// already flooded into
		floodnum++;
		FloodArea_r (area, floodnum);
	}

}

void CM_SetAreaPortalState (int portalnum, bool open)
{
	if (portalnum > numareaportals)
		Com_DropError ("areaportal > numareaportals");

	portalopen[portalnum] = open;
	FloodAreaConnections ();
}

// used by game dll
bool CM_AreasConnected (int area1, int area2)
{
	if (map_noareas->integer)
		return true;

	if (area1 > numareas || area2 > numareas)
		Com_DropError ("area > numareas");

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
bool CM_HeadnodeVisible (int nodenum, byte *visbits)
{
	cnode_t	*node;
	int		stack[MAX_TREE_DEPTH], sptr;

	guard(CM_HeadnodeVisible);

	sptr = 0;
	while (true)
	{
		if (nodenum < 0)
		{
			int		cluster;

			cluster = map_leafs[-1-nodenum].cluster;
			if (cluster == -1 || !(visbits[cluster>>3] & (1<<(cluster&7))))
			{
				if (!sptr) return false;		// whole tree visited
				nodenum = stack[--sptr];
				continue;
			}
			return true;
		}

		node = &map_nodes[nodenum];
		stack[sptr++] = node->children[0];
		nodenum = node->children[1];
	}

	unguard;
}
