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
// models.c -- model loading and caching

// models are the only shared resource between a client and server running
// on the same machine.

#include "r_local.h"

model_t	*loadmodel;
char	loadname[32];	// for hunk tags

void Mod_LoadSpriteModel (model_t *mod, void *buffer);
void Mod_LoadBrushModel (model_t *mod, bspfile_t *bsp);
void Mod_LoadAliasModel (model_t *mod, void *buffer);
model_t *Mod_LoadModel (model_t *mod, qboolean crash);

byte	mod_novis[MAX_MAP_LEAFS/8];

#define	MAX_MOD_KNOWN	256
model_t	mod_known[MAX_MOD_KNOWN];
int		mod_numknown;

// the inline * models from the current map are kept seperate
model_t	mod_inline[MAX_MOD_KNOWN];

int		registration_sequence;
int		modfilelen;

//===============================================================================


/*
================
Mod_Modellist_f
================
*/
void Mod_Modellist_f (void)
{
	int		i;
	model_t	*mod;
	int		total;

	total = 0;
	Com_Printf ("Loaded models:\n");
	for (i=0, mod=mod_known ; i < mod_numknown ; i++, mod++)
	{
		if (!mod->name[0])
			continue;
		Com_Printf ("%8i : %s\n",mod->extradatasize, mod->name);
		total += mod->extradatasize;
	}
	Com_Printf ("Total resident: %i\n", total);
}

/*
===============
Mod_Init
===============
*/
void Mod_Init (void)
{
	memset (mod_novis, 0xff, sizeof(mod_novis));
}

/*
==================
Mod_ForName

Loads in a model for the given name
==================
*/
model_t *Mod_ForName (char *name, qboolean crash)
{
	model_t	*mod;
	unsigned *buf;
	int		i;
	char	*ext;

	if (!name[0])
		Com_Error (ERR_DROP,"Mod_ForName: NULL name");

	//
	// inline models are grabbed only from worldmodel
	//
	if (name[0] == '*')
	{
		i = atoi(name+1);
		if (i < 1 || !r_worldmodel || i >= r_worldmodel->numsubmodels)
			Com_Error (ERR_DROP, "bad inline model number");
		return &mod_inline[i];
	}

	//
	// search the currently loaded models
	//
	for (i = 0, mod = mod_known; i < mod_numknown; i++, mod++)
	{
		if (!mod->name[0])
			continue;
		if (!strcmp (mod->name, name) )
			return mod;
	}

	//
	// find a free model slot spot
	//
	for (i = 0, mod = mod_known; i < mod_numknown; i++, mod++)
	{
		if (!mod->name[0])
			break;	// free spot
	}
	if (i == mod_numknown)
	{
		if (mod_numknown == MAX_MOD_KNOWN)
			Com_Error (ERR_DROP, "mod_numknown == MAX_MOD_KNOWN");
		mod_numknown++;
	}
	strcpy (mod->name, name);

	ext = strrchr (name, '.');
	if (ext && !stricmp (ext, ".bsp"))
	{
		bspfile_t *bsp;

		// map should be already loaded by client (or server)
		loadmodel = mod;
		loadmodel->extradata = Hunk_Begin (0x1000000);
		bsp = LoadBspFile (name, true, NULL);
		switch (bsp->type)
		{
		case map_q2:
			Mod_LoadBrushModel (mod, bsp);
			break;
//		case map_hl:
//			Mod_LoadHLBrushModel (mod, bsp);
//			break;
		default:	// should not happen! (map MUST be loaded by client)
			Com_Error (ERR_DROP, "Internal error while loadinf BSP file");
		}
		loadmodel->extradatasize = Hunk_End ();
	}
	else
	{
		// load the file
		modfilelen = FS_LoadFile (mod->name, (void **)&buf);
		if (!buf)
		{
			if (crash)
				Com_Error (ERR_DROP, "Mod_NumForName: %s not found", mod->name);
			memset (mod->name, 0, sizeof(mod->name));
			return NULL;
		}

		loadmodel = mod;

		// fill it in

		// call the apropriate loader

		switch (LittleLong(*(unsigned *)buf))
		{
		case IDALIASHEADER:
			loadmodel->extradata = Hunk_Begin (0x200000);
			Mod_LoadAliasModel (mod, buf);
			break;

		case IDSPRITEHEADER:
			loadmodel->extradata = Hunk_Begin (0x10000);
			Mod_LoadSpriteModel (mod, buf);
			break;

		default:
			Com_Error (ERR_DROP,"Mod_NumForName: unknown fileid for %s", mod->name);
			break;
		}

		loadmodel->extradatasize = Hunk_End ();

		FS_FreeFile (buf);
	}
	return mod;
}


/*
===============
Mod_PointInLeaf
===============
*/
mleaf_t *Mod_PointInLeaf (vec3_t p, model_t *model)
{
	mnode_t		*node;
	float		d;
	cplane_t	*plane;

	if (!model || !model->nodes)
		Com_Error (ERR_DROP, "Mod_PointInLeaf: bad model");

	node = model->nodes;
	while (1)
	{
		if (node->contents != -1)
			return (mleaf_t *)node;
		plane = node->plane;
		d = DotProduct (p,plane->normal) - plane->dist;
		if (d > 0)
			node = node->children[0];
		else
			node = node->children[1];
	}

	return NULL;	// never reached
}


/*
===================
Mod_DecompressVis
===================
*/
byte *Mod_DecompressVis (byte *in, model_t *model)
{
	static byte	decompressed[MAX_MAP_LEAFS/8];
	int		c;
	byte	*out;
	int		row;

	row = (model->vis->numclusters+7)>>3;
	out = decompressed;

#if 0
	memcpy (out, in, row);
#else
	if (!in)
	{	// no vis info, so make all visible
		while (row)
		{
			*out++ = 0xff;
			row--;
		}
		return decompressed;
	}

	do
	{
		if (*in)
		{
			*out++ = *in++;
			continue;
		}

		c = in[1];
		in += 2;
		while (c)
		{
			*out++ = 0;
			c--;
		}
	} while (out - decompressed < row);
#endif

	return decompressed;
}

/*
==============
Mod_ClusterPVS
==============
*/
byte *Mod_ClusterPVS (int cluster, model_t *model)
{
	if (cluster == -1 || !model->vis)
		return mod_novis;
	return Mod_DecompressVis ( (byte *)model->vis + model->vis->bitofs[cluster][DVIS_PVS],
		model);
}

/*
===============================================================================

					BRUSHMODEL LOADING

===============================================================================
*/


/*
=================
Mod_LoadLighting

Converts the 24 bit lighting down to 8 bit
by taking the brightest component
=================
*/
void Mod_LoadLighting (byte *data, int size)
{
	int	i, r, g, b, c, s;
	byte	*in;

	if (!size)
	{
		loadmodel->lightdata = NULL;
		return;
	}
	s = size / 3;
	loadmodel->lightdata = Hunk_Alloc (size);
	in = data;
	for (i = 0; i < s; i++, in+=3)
	{
		r = in[0];
		g = in[1];
		b = in[2];
		if (r > g && r > b)
			c = r;
		else if (g > r && g > b)
			c = g;
		else
			c = b;
		loadmodel->lightdata[i] = c;
	}
}


int		r_leaftovis[MAX_MAP_LEAFS];
int		r_vistoleaf[MAX_MAP_LEAFS];
int		r_numvisleafs;

void	R_NumberLeafs (mnode_t *node)
{
	mleaf_t	*leaf;
	int		leafnum;

	if (node->contents != -1)
	{
		leaf = (mleaf_t *)node;
		leafnum = leaf - loadmodel->leafs;
		if (leaf->contents & CONTENTS_SOLID)
			return;
		r_leaftovis[leafnum] = r_numvisleafs;
		r_vistoleaf[r_numvisleafs] = leafnum;
		r_numvisleafs++;
		return;
	}

	R_NumberLeafs (node->children[0]);
	R_NumberLeafs (node->children[1]);
}


/*
=================
Mod_LoadVisibility
=================
*/
void Mod_LoadVisibility (dvis_t *data, int size)
{
	if (!size)
	{
		loadmodel->vis = NULL;
		return;
	}
	loadmodel->vis = data;
}


/*
=================
Mod_LoadVertexes
=================
*/
void Mod_LoadVertexes (dvertex_t *data, int size)
{
	mvertex_t	*out;

	out = Hunk_Alloc ((size+8)*sizeof(*out));	// extra for skybox (else we can do loadmodel->vertexes=data)
	memcpy (out, data, sizeof(*out)*size);
	loadmodel->vertexes = out;
	loadmodel->numvertexes = size;
}

/*
=================
Mod_LoadSubmodels
=================
*/
void Mod_LoadSubmodels (dmodel_t *data, int size)
{
	loadmodel->submodels = data;
	loadmodel->numsubmodels = size;
}

/*
=================
Mod_LoadEdges
=================
*/
void Mod_LoadEdges (dedge_t *data, int size)
{
	dedge_t *in;
	medge_t *out;
	int 	i;

	in = data;
	out = Hunk_Alloc ((size+13)*sizeof(*out));	// extra for skybox

	loadmodel->edges = out;
	loadmodel->numedges = size;

	for (i = 0; i < size; i++, in++, out++)
	{
		out->v[0] = (unsigned short)in->v[0];
		out->v[1] = (unsigned short)in->v[1];
	}
}

/*
=================
Mod_LoadTexinfo
=================
*/
void Mod_LoadTexinfo (texinfo_t *data, int size)
{
	texinfo_t *in;
	mtexinfo_t *out, *step;
	int 	i, j;
	float	len1, len2;
	char	name[MAX_QPATH];
	int		next;

	in = data;
	out = Hunk_Alloc ((size+6)*sizeof(*out));	// extra for skybox

	loadmodel->texinfo = out;
	loadmodel->numtexinfo = size;

	for (i = 0; i < size; i++, in++, out++)
	{
		for (j = 0; j < 8; j++)
			out->vecs[0][j] = in->vecs[0][j];
		len1 = VectorLength (out->vecs[0]);
		len2 = VectorLength (out->vecs[1]);
		len1 = (len1 + len2)/2;
		if (len1 < 0.32)
			out->mipadjust = 4;
		else if (len1 < 0.49)
			out->mipadjust = 3;
		else if (len1 < 0.99)
			out->mipadjust = 2;
		else
			out->mipadjust = 1;
#if 0
		if (len1 + len2 < 0.001)
			out->mipadjust = 1;		// don't crash
		else
			out->mipadjust = 1 / floor( (len1+len2)/2 + 0.1 );
#endif

		out->flags = in->flags;

		next = in->nexttexinfo;
		if (next > 0)
			out->next = loadmodel->texinfo + next;

		Com_sprintf (name, sizeof(name), "textures/%s.wal", in->texture);
		out->image = R_FindImage (name, it_wall);
		if (!out->image)
		{
			Com_WPrintf ("Couldn't load %s\n", name);
			out->image = r_notexture_mip; // texture not found
			out->flags = 0;
		}
	}

	// count animation frames
	for (i = 0; i < size; i++)
	{
		out = &loadmodel->texinfo[i];
		out->numframes = 1;
		for (step = out->next ; step && step != out ; step=step->next)
			out->numframes++;
	}
}

/*
================
CalcSurfaceExtents

Fills in s->texturemins[] and s->extents[]
================
*/
void CalcSurfaceExtents (msurface_t *s)
{
	float	mins[2], maxs[2], val;
	int		i,j, e;
	mvertex_t	*v;
	mtexinfo_t	*tex;
	int		bmins[2], bmaxs[2];

	mins[0] = mins[1] = 999999;
	maxs[0] = maxs[1] = -99999;

	tex = s->texinfo;

	for (i=0 ; i<s->numedges ; i++)
	{
		e = loadmodel->surfedges[s->firstedge+i];
		if (e >= 0)
			v = &loadmodel->vertexes[loadmodel->edges[e].v[0]];
		else
			v = &loadmodel->vertexes[loadmodel->edges[-e].v[1]];

		for (j=0 ; j<2 ; j++)
		{
			val = v->position[0] * tex->vecs[j][0] +
				v->position[1] * tex->vecs[j][1] +
				v->position[2] * tex->vecs[j][2] +
				tex->vecs[j][3];
			if (val < mins[j])
				mins[j] = val;
			if (val > maxs[j])
				maxs[j] = val;
		}
	}

	for (i=0 ; i<2 ; i++)
	{
		bmins[i] = floor(mins[i]/16);
		bmaxs[i] = ceil(maxs[i]/16);

		s->texturemins[i] = bmins[i] * 16;
		s->extents[i] = (bmaxs[i] - bmins[i]) * 16;
		if (s->extents[i] < 16)
			s->extents[i] = 16;	// take at least one cache block
		if ( !(tex->flags & (SURF_WARP|SURF_SKY)) && s->extents[i] > 256)
			Com_Error (ERR_DROP,"Bad surface extents");
	}
}


/*
=================
Mod_LoadFaces
=================
*/
void Mod_LoadFaces (dface_t *data, int size)
{
	dface_t		*in;
	msurface_t 	*out;
	int			i, surfnum;
	int			ti;

	in = data;
	out = Hunk_Alloc ((size+6)*sizeof(*out));	// extra for skybox

	loadmodel->surfaces = out;
	loadmodel->numsurfaces = size;

	for (surfnum = 0; surfnum < size; surfnum++, in++, out++)
	{
		out->firstedge = in->firstedge;
		out->numedges = in->numedges;
		if (out->numedges < 3)
			Com_Error (ERR_DROP,"Surface with %s edges", out->numedges);
		out->flags = 0;

		if (in->side)
			out->flags |= SURF_PLANEBACK;

		out->plane = loadmodel->planes + in->planenum;

		ti = in->texinfo;
		if (ti < 0 || ti >= loadmodel->numtexinfo)
			Com_Error (ERR_DROP, "MOD_LoadBmodel: bad texinfo number");
		out->texinfo = loadmodel->texinfo + ti;

		CalcSurfaceExtents (out);

		// lighting info is converted from 24 bit on disk to 8 bit

		for (i = 0; i < MAXLIGHTMAPS; i++)
			out->styles[i] = in->styles[i];
		i = in->lightofs;
		if (i == -1)
			out->samples = NULL;
		else
			out->samples = loadmodel->lightdata + i/3;

		// set the drawing flags flag

		if (!out->texinfo->image)
			continue;
		if (out->texinfo->flags & SURF_SKY)
		{
			out->flags |= SURF_DRAWSKY;
			continue;
		}

		if (out->texinfo->flags & SURF_WARP)
		{
			out->flags |= SURF_DRAWTURB;
			for (i=0 ; i<2 ; i++)
			{
				out->extents[i] = 16384;
				out->texturemins[i] = -8192;
			}
			continue;
		}
//==============
//PGM
		// this marks flowing surfaces as turbulent, but with the new
		// SURF_FLOW flag.
		if (out->texinfo->flags & SURF_FLOWING)
		{
			out->flags |= SURF_DRAWTURB | SURF_FLOW;
			for (i=0 ; i<2 ; i++)
			{
				out->extents[i] = 16384;
				out->texturemins[i] = -8192;
			}
			continue;
		}
//PGM
//==============
	}
}


/*
=================
Mod_SetParent
=================
*/
void Mod_SetParent (mnode_t *node, mnode_t *parent)
{
	node->parent = parent;
	if (node->contents != -1)
		return;
	Mod_SetParent (node->children[0], node);
	Mod_SetParent (node->children[1], node);
}

/*
=================
Mod_LoadNodes
=================
*/
void Mod_LoadNodes (dnode_t *data, int size)
{
	int			i, j, p;
	dnode_t		*in;
	mnode_t 	*out;

	in = data;
	out = Hunk_Alloc (size*sizeof(*out));

	loadmodel->nodes = out;
	loadmodel->numnodes = size;

	for (i = 0; i < size; i++, in++, out++)
	{
		for (j = 0; j < 3; j++)
		{
			out->minmaxs[j] = in->mins[j];
			out->minmaxs[3+j] = in->maxs[j];
		}

		out->plane = loadmodel->planes + in->planenum;

		out->firstsurface = in->firstface;
		out->numsurfaces = in->numfaces;
		out->contents = CONTENTS_NODE;	// differentiate from leafs

		for (j = 0; j < 2; j++)
		{
			p = in->children[j];
			if (p >= 0)
				out->children[j] = loadmodel->nodes + p;
			else
				out->children[j] = (mnode_t *)(loadmodel->leafs + (-1 - p));
		}
	}

	Mod_SetParent (loadmodel->nodes, NULL);	// sets nodes and leafs
}

/*
=================
Mod_LoadLeafs
=================
*/
void Mod_LoadLeafs (dleaf_t *data, int size)
{
	dleaf_t 	*in;
	mleaf_t 	*out;
	int			i, j;

	in = data;
	out = Hunk_Alloc (size*sizeof(*out));

	loadmodel->leafs = out;
	loadmodel->numleafs = size;

	for (i = 0; i < size; i++, in++, out++)
	{
		for (j = 0; j < 3; j++)
		{
			out->minmaxs[j] = in->mins[j];
			out->minmaxs[3+j] = in->maxs[j];
		}

		out->contents = in->contents;
		out->cluster = in->cluster;
		out->area = in->area;

		out->firstmarksurface = loadmodel->marksurfaces + in->firstleafface;
		out->nummarksurfaces = in->numleaffaces;
	}
}


/*
=================
Mod_LoadMarksurfaces
=================
*/
void Mod_LoadMarksurfaces (unsigned short *data, int size)
{
	int		i, j;
	short		*in;
	msurface_t **out;

	in = data;
	out = Hunk_Alloc (size*sizeof(*out));

	loadmodel->marksurfaces = out;
	loadmodel->nummarksurfaces = size;

	for (i = 0; i < size; i++)
	{
		j = in[i];
		if (j < 0 || j >= loadmodel->numsurfaces)
			Com_Error (ERR_DROP, "Mod_ParseMarksurfaces: bad surface number");
		out[i] = loadmodel->surfaces + j;
	}
}

/*
=================
Mod_LoadSurfedges
=================
*/
void Mod_LoadSurfedges (int *data, int size)
{
	int		*out;

	out = Hunk_Alloc ((size+24)*sizeof(*out));	// extra for skybox (if not this, we can just set pointer: loadmodel->surfedges=data!)

	loadmodel->surfedges = out;
	loadmodel->numsurfedges = size;

	memcpy (out, data, sizeof(*out)*size);
}

/*
=================
Mod_LoadPlanes
=================
*/
void Mod_LoadPlanes (dplane_t *data, int size)
{
	int			i;
	cplane_t	*out;
	dplane_t 	*in;

	in = data;
	out = Hunk_Alloc ((size+6)*sizeof(*out));		// extra for skybox

	loadmodel->planes = out;
	loadmodel->numplanes = size;

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
Mod_LoadBrushModel
=================
*/
void Mod_LoadBrushModel (model_t *mod, bspfile_t *bsp)
{
	int			i;
	dmodel_t 	*bm;

	loadmodel->type = mod_brush;
	if (loadmodel != mod_known)
		Com_Error (ERR_DROP, "Loaded a brush model after the world");

	// load into heap
	Mod_LoadVertexes (bsp->vertexes, bsp->numVertexes);
	Mod_LoadEdges (bsp->edges, bsp->numEdges);
	Mod_LoadSurfedges (bsp->surfedges, bsp->numSurfedges);
	Mod_LoadLighting (bsp->lighting, bsp->lightDataSize);
	Mod_LoadPlanes (bsp->planes, bsp->numPlanes);
	Mod_LoadTexinfo (bsp->texinfo, bsp->numTexinfo);
	Mod_LoadFaces (bsp->faces, bsp->numFaces);
	Mod_LoadMarksurfaces (bsp->leaffaces, bsp->numLeaffaces);
	Mod_LoadVisibility (bsp->visibility, bsp->visDataSize);
	Mod_LoadLeafs (bsp->leafs, bsp->numLeafs);
	Mod_LoadNodes (bsp->nodes, bsp->numNodes);
	Mod_LoadSubmodels (bsp->models, bsp->numModels);

	r_numvisleafs = 0;
	R_NumberLeafs (loadmodel->nodes);

//
// set up the submodels
//
	for (i=0 ; i<mod->numsubmodels ; i++)
	{
		model_t	*starmod;

		bm = &mod->submodels[i];
		starmod = &mod_inline[i];

		*starmod = *loadmodel;

		starmod->firstmodelsurface = bm->firstface;
		starmod->nummodelsurfaces = bm->numfaces;
		starmod->firstnode = bm->headnode;
		if (starmod->firstnode >= loadmodel->numnodes)
			Com_Error (ERR_DROP, "Inline model %i has bad firstnode", i);

		VectorCopy (bm->maxs, starmod->maxs);
		VectorCopy (bm->mins, starmod->mins);

		if (i == 0)
			*loadmodel = *starmod;
	}

	R_InitSkyBox ();
}

/*
==============================================================================

ALIAS MODELS

==============================================================================
*/

/*
=================
Mod_LoadAliasModel
=================
*/
void Mod_LoadAliasModel (model_t *mod, void *buffer)
{
	int			i, j;
	dmdl_t		*pinmodel, *pheader;
	dStVert_t	*pinst, *poutst;
	dTriangle_t	*pintri, *pouttri;
	dAliasFrame_t *pinframe, *poutframe;
	int			*pincmd, *poutcmd;
	int			version;

	pinmodel = (dmdl_t *)buffer;

	version = LittleLong (pinmodel->version);
	if (version != ALIAS_VERSION)
		Com_Error (ERR_DROP, "%s has wrong version number (%i should be %i)",
				 mod->name, version, ALIAS_VERSION);

	pheader = Hunk_Alloc (LittleLong(pinmodel->ofsEnd));

	// byte swap the header fields and sanity check
	for (i=0 ; i<sizeof(dmdl_t)/4 ; i++)
		((int *)pheader)[i] = LittleLong (((int *)buffer)[i]);

	if (pheader->skinHeight > MAX_LBM_HEIGHT)
		Com_Error (ERR_DROP, "model %s has a skin taller than %d", mod->name, MAX_LBM_HEIGHT);

	if (pheader->numXyz <= 0)
		Com_Error (ERR_DROP, "model %s has no vertices", mod->name);

	if (pheader->numXyz > MAX_VERTS)
		Com_Error (ERR_DROP, "model %s has too many vertices", mod->name);

	if (pheader->numSt <= 0)
		Com_Error (ERR_DROP, "model %s has no st vertices", mod->name);

	if (pheader->numTris <= 0)
		Com_Error (ERR_DROP, "model %s has no triangles", mod->name);

	if (pheader->numFrames <= 0)
		Com_Error (ERR_DROP, "model %s has no frames", mod->name);

//
// load base s and t vertices (not used in gl version)
//
	pinst = (dStVert_t *) ((byte *)pinmodel + pheader->ofsSt);
	poutst = (dStVert_t *) ((byte *)pheader + pheader->ofsSt);

	for (i=0 ; i<pheader->numSt ; i++)
	{
		poutst[i].s = LittleShort (pinst[i].s);
		poutst[i].t = LittleShort (pinst[i].t);
	}

//
// load triangle lists
//
	pintri = (dTriangle_t *) ((byte *)pinmodel + pheader->ofsTris);
	pouttri = (dTriangle_t *) ((byte *)pheader + pheader->ofsTris);

	for (i=0 ; i<pheader->numTris ; i++)
	{
		for (j=0 ; j<3 ; j++)
		{
			pouttri[i].indexXyz[j] = LittleShort (pintri[i].indexXyz[j]);
			pouttri[i].indexSt[j] = LittleShort (pintri[i].indexSt[j]);
		}
	}

//
// load the frames
//
	for (i=0 ; i<pheader->numFrames ; i++)
	{
		pinframe = (dAliasFrame_t *) ((byte *)pinmodel + pheader->ofsFrames + i * pheader->frameSize);
		poutframe = (dAliasFrame_t *) ((byte *)pheader + pheader->ofsFrames + i * pheader->frameSize);

		memcpy (poutframe->name, pinframe->name, sizeof(poutframe->name));
		for (j=0 ; j<3 ; j++)
		{
			poutframe->scale[j] = LittleFloat (pinframe->scale[j]);
			poutframe->translate[j] = LittleFloat (pinframe->translate[j]);
		}
		// verts are all 8 bit, so no swapping needed
		memcpy (poutframe->verts, pinframe->verts, pheader->numXyz*sizeof(dTriVertx_t));

	}

	mod->type = mod_alias;

	//
	// load the glcmds
	//
	pincmd = (int *) ((byte *)pinmodel + pheader->ofsGlcmds);
	poutcmd = (int *) ((byte *)pheader + pheader->ofsGlcmds);
	for (i=0 ; i<pheader->numGlcmds ; i++)
		poutcmd[i] = LittleLong (pincmd[i]);


	// register all skins
	memcpy ((char *)pheader + pheader->ofsSkins, (char *)pinmodel + pheader->ofsSkins,
		pheader->numSkins*MAX_SKINNAME);
	for (i=0 ; i<pheader->numSkins ; i++)
	{
		mod->skins[i] = R_FindImage ((char *)pheader + pheader->ofsSkins + i*MAX_SKINNAME, it_skin);
	}
}

/*
==============================================================================

SPRITE MODELS

==============================================================================
*/

/*
=================
Mod_LoadSpriteModel
=================
*/
void Mod_LoadSpriteModel (model_t *mod, void *buffer)
{
	dsprite_t	*sprin, *sprout;
	int			i;

	sprin = (dsprite_t *)buffer;
	sprout = Hunk_Alloc (modfilelen);

	sprout->ident = LittleLong (sprin->ident);
	sprout->version = LittleLong (sprin->version);
	sprout->numframes = LittleLong (sprin->numframes);

	if (sprout->version != SPRITE_VERSION)
		Com_Error (ERR_DROP, "%s has wrong version number (%i should be %i)",
				 mod->name, sprout->version, SPRITE_VERSION);

	if (sprout->numframes > MAX_MD2SKINS)
		Com_Error (ERR_DROP, "%s has too many frames (%i > %i)",
				 mod->name, sprout->numframes, MAX_MD2SKINS);

	// byte swap everything
	for (i=0 ; i<sprout->numframes ; i++)
	{
		sprout->frames[i].width = LittleLong (sprin->frames[i].width);
		sprout->frames[i].height = LittleLong (sprin->frames[i].height);
		sprout->frames[i].origin_x = LittleLong (sprin->frames[i].origin_x);
		sprout->frames[i].origin_y = LittleLong (sprin->frames[i].origin_y);
		memcpy (sprout->frames[i].name, sprin->frames[i].name, MAX_SKINNAME);
		mod->skins[i] = R_FindImage (sprout->frames[i].name, it_sprite);
	}

	mod->type = mod_sprite;
}

//=============================================================================

/*
@@@@@@@@@@@@@@@@@@@@@
R_BeginRegistration

Specifies the model that will be used as the world
@@@@@@@@@@@@@@@@@@@@@
*/
void R_BeginRegistration (char *model)
{
	char	fullname[MAX_QPATH];
	cvar_t	*flushmap;

	registration_sequence++;
	r_oldviewcluster = -1;		// force markleafs
	Com_sprintf (fullname, sizeof(fullname), "maps/%s.bsp", model);

	D_FlushCaches ();
	// explicitly free the old map if different
	// this guarantees that mod_known[0] is the world map
	flushmap = Cvar_Get ("flushmap", "0", 0);
	if ( strcmp(mod_known[0].name, fullname) || flushmap->integer)
		Mod_Free (&mod_known[0]);
	r_worldmodel = R_RegisterModel (fullname);
	R_NewMap ();
}


/*
@@@@@@@@@@@@@@@@@@@@@
R_RegisterModel

@@@@@@@@@@@@@@@@@@@@@
*/
struct model_s *R_RegisterModel (char *name)
{
	model_t	*mod;
	int		i;
	dsprite_t	*sprout;
	dmdl_t		*pheader;

	mod = Mod_ForName (name, false);
	if (mod)
	{
		if (mod->registration_sequence == registration_sequence)	//?? added 25.02.2002
			return mod;

		mod->registration_sequence = registration_sequence;

		// register any images used by the models
		if (mod->type == mod_sprite)
		{
			sprout = (dsprite_t *)mod->extradata;
			for (i=0 ; i<sprout->numframes ; i++)
				mod->skins[i] = R_FindImage (sprout->frames[i].name, it_sprite);
		}
		else if (mod->type == mod_alias)
		{
			pheader = (dmdl_t *)mod->extradata;
			for (i=0 ; i<pheader->numSkins ; i++)
				mod->skins[i] = R_FindImage ((char *)pheader + pheader->ofsSkins + i*MAX_SKINNAME, it_skin);
//PGM
			mod->numframes = pheader->numFrames;
//PGM
		}
		else if (mod->type == mod_brush)
		{
			for (i=0 ; i<mod->numtexinfo ; i++)
				mod->texinfo[i].image->registration_sequence = registration_sequence;
		}
	}
	return mod;
}

/*
@@@@@@@@@@@@@@@@@@@@@
R_EndRegistration

@@@@@@@@@@@@@@@@@@@@@
*/
void R_EndRegistration (void)
{
	int		i;
	model_t	*mod;

	for (i=0, mod=mod_known ; i<mod_numknown ; i++, mod++)
	{
		if (!mod->name[0])
			continue;
		if (mod->registration_sequence != registration_sequence)
		{	// don't need this model
			Hunk_Free (mod->extradata);
			memset (mod, 0, sizeof(*mod));
		}
		else
		{	// make sure it is paged in
			Com_PageInMemory (mod->extradata, mod->extradatasize);
		}
	}

	R_FreeUnusedImages ();
}


//=============================================================================

/*
================
Mod_Free
================
*/
void Mod_Free (model_t *mod)
{
	Hunk_Free (mod->extradata);
	memset (mod, 0, sizeof(*mod));
}

/*
================
Mod_FreeAll
================
*/
void Mod_FreeAll (void)
{
	int		i;

	for (i=0 ; i<mod_numknown ; i++)
	{
		if (mod_known[i].extradatasize)
			Mod_Free (&mod_known[i]);
	}
}
