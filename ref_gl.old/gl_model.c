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

#include "gl_local.h"

model_t	*loadmodel;
int		modfilelen;

void Mod_LoadSpriteModel (model_t *mod, void *buffer);
void Mod_LoadBrushModel (model_t *mod, bspfile_t *bsp);
void Mod_LoadHLBrushModel (model_t *mod, void *buffer); //??
void Mod_LoadAliasModel (model_t *mod, void *buffer);
model_t *Mod_LoadModel (model_t *mod, qboolean crash);

byte	mod_novis[MAX_MAP_LEAFS/8];

#define	MAX_MOD_KNOWN	512
model_t	gl_mod_known[MAX_MOD_KNOWN];
int		gl_mod_numknown;

// the inline * models from the current map are kept seperate
model_t	mod_inline[MAX_MOD_KNOWN];

int		registration_sequence;

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
		Com_Error (ERR_DROP, "GL_PointInLeaf: bad model");

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

	return decompressed;
}


/*
byte *Mod_CompressVis (byte *in, model_t *model, int *size)
{
	static byte	compressed[MAX_MAP_LEAFS/8];
	int	count, c, zeros;
	byte	*out;

	count = (model->vis->numclusters+7)>>3;
	out = compressed;
	*size = 0;

	while (count)
	{
		c = *in++;
		count--;
		*out++ = c;
		(*size)++;
		if (!c)
		{
			zeros = 1;
			while (!*in && count && zeros < 255)
			{
				count--;
				in++;
				zeros++;
			}
			*out++ = zeros;
			(*size)++;
		}
	}

	return compressed;
}
*/

/*
==============
Mod_ClusterPVS
==============
*/
byte *Mod_ClusterPVS (int cluster, model_t *model)
{
	int i;

	if (cluster == -1 || !model->vis)
		return mod_novis;
	i = model->vis->bitofs[cluster][DVIS_PVS];
	if (i != -1)
		return Mod_DecompressVis ((byte *)model->vis + i, model);
	else
		return Mod_DecompressVis (NULL, model);
}


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
	for (i = 0, mod = gl_mod_known; i < gl_mod_numknown ; i++, mod++)
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
		Com_Error (ERR_DROP, "Mod_ForName: NULL name");

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
	for (i = 0, mod = gl_mod_known; i < gl_mod_numknown; i++, mod++)
	{
		if (!mod->name[0])
			continue;
		if (!strcmp (mod->name, name) )
			return mod;
	}

	//
	// find a free model slot spot
	//
	for (i = 0, mod = gl_mod_known; i < gl_mod_numknown; i++, mod++)
	{
		if (!mod->name[0])
			break;	// free spot
	}
	if (i == gl_mod_numknown)
	{
		if (gl_mod_numknown == MAX_MOD_KNOWN)
			Com_Error (ERR_DROP, "mod_numknown == MAX_MOD_KNOWN");
		gl_mod_numknown++;
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
			Com_Error (ERR_DROP, "Internal error while loading BSP file");
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
				Com_Error (ERR_DROP, "Mod_ModForName: %s not found", mod->name);
			memset (mod->name, 0, sizeof(mod->name));
			return NULL;
		}

		loadmodel = mod;

		// fill it in

		// call the apropriate loader

		switch (LittleLong(*buf))
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
===============================================================================

					BRUSHMODEL LOADING

===============================================================================
*/


/*
=================
Mod_LoadLighting
=================
*/

void CreateSolarColor (float a, float x, float y, float *vec)
{
	float	a0, s, t;
	int		i;

	a0 = a * 5;
	i = (int) a0;
	a0 = a0 - i;	// frac part
	s = (1.0 - x) * y;
	t = (1.0 - a0 * x) * y;
	a0 = (1.0 - (1.0 - a0) * x) * y;
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

void Mod_LoadLighting (byte *data, int size)
{
	float sat;
	float r, g, b, light;
	byte *p;
	int i, c;

	if (!size)
	{
		loadmodel->lightdata = NULL;
		return;
	}
	loadmodel->lightdata = Hunk_Alloc (size);
	memcpy (loadmodel->lightdata, data, size);

	c = size / 3;
	p = loadmodel->lightdata;

	if (gl_lightmap->integer == 2)	// make solarised lightmaps
	{
		for (i = 0; i < c; i++, p+=3)
		{
			float vec[3];

			r = p[0]; g = p[1]; b = p[2];

			light = r * 0.33 + g * 0.685 * b * 0.063;
			if (light > 255)
				light = 1;
			else
				light = 1 / light;

			CreateSolarColor (light, 1, 0.5, vec);
			p[0] = vec[0] * 255;
			p[1] = vec[1] * 255;
			p[2] = vec[2] * 255;
		}
		return;
	}

	sat = saturation->value;

	if (gl_lightmap->integer == 3)	// make inversed saturation on lightmaps
		sat = -sat;
	if (sat != 1.0)
	{
		for (i = 0; i < c; i++, p+=3)
		{
#define SATURATE(c,l,v) c = (l+0.5+(c-l)*v); if (c < 0) c = 0; else if (c > 255) c = 255;
			// get color
			r = p[0];  g = p[1];  b = p[2];
			// change saturation
			light = (r + g + b) / 3.0;
			SATURATE(r,light,sat);
			SATURATE(g,light,sat);
			SATURATE(b,light,sat);
			// put color
			p[0] = r;  p[1] = g;  p[2] = b;
		}
	}
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
	loadmodel->vertexes = (mvertex_t *)data;
	loadmodel->numvertexes = size;
}

/*
=================
RadiusFromBounds
=================
*/
float RadiusFromBounds (vec3_t mins, vec3_t maxs)
{
	int		i;
	vec3_t	corner;

	for (i=0 ; i<3 ; i++)
	{
		corner[i] = fabs(mins[i]) > fabs(maxs[i]) ? fabs(mins[i]) : fabs(maxs[i]);
	}

	return VectorLength (corner);
}


/*
=================
Mod_LoadSubmodels
=================
*/
void Mod_LoadSubmodels (dmodel_t *data, int size)
{
	dmodel_t	*in;
	mmodel_t	*out;
	int			i, j;

	in = data;
	out = Hunk_Alloc (size*sizeof(*out));

	loadmodel->submodels = out;
	loadmodel->numsubmodels = size;

	for (i = 0; i < size; i++, in++, out++)
	{
		for (j = 0; j < 3; j++)
		{	// spread the mins / maxs by a pixel
			out->mins[j] = in->mins[j]; // -1 -- migrated to LoadBspFile()
			out->maxs[j] = in->maxs[j]; // +1
			out->origin[j] = in->origin[j];
		}
		out->radius = RadiusFromBounds (out->mins, out->maxs);
		out->headnode = in->headnode;
		out->firstface = in->firstface;
		out->numfaces = in->numfaces;
	}
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
	out = Hunk_Alloc ((size+1)*sizeof(*out));

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
	char	name[MAX_QPATH];
	int		next;

	in = data;
	out = Hunk_Alloc (size*sizeof(*out));

	loadmodel->texinfo = out;
	loadmodel->numtexinfo = size;

	for (i = 0; i < size; i++, in++, out++)
	{
		for (j = 0; j < 8; j++)
			out->vecs[0][j] = in->vecs[0][j];

		out->flags = in->flags;
		next = in->nexttexinfo;
		if (next > 0)
			out->next = loadmodel->texinfo + next;
		else
			out->next = NULL;

		Com_sprintf (name, sizeof(name), "textures/%s.wal", in->texture);
		out->image = GL_FindImage (name, it_wall);
		if (!out->image)
		{
			Com_WPrintf ("Couldn't load %s\n", name);
			out->image = r_notexture;
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

	for (i = 0; i < s->numedges; i++)
	{
		e = loadmodel->surfedges[s->firstedge+i];
		if (e >= 0)
			v = &loadmodel->vertexes[loadmodel->edges[e].v[0]];
		else
			v = &loadmodel->vertexes[loadmodel->edges[-e].v[1]];

		for (j = 0; j < 2; j++)
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

	for (i = 0; i < 2; i++)
	{
		bmins[i] = floor(mins[i]/16);
		bmaxs[i] = ceil(maxs[i]/16);

		s->texturemins[i] = bmins[i] * 16;
		s->extents[i] = (bmaxs[i] - bmins[i]) * 16;

//		if ( !(tex->flags & TEX_SPECIAL) && s->extents[i] > 512 /* 256 */ )
//			Com_Error (ERR_DROP, "Bad surface extents");
	}
}


void GL_BuildPolygonFromSurface(msurface_t *fa);
void GL_CreateSurfaceLightmap (msurface_t *surf);
void GL_EndBuildingLightmaps (void);
void GL_BeginBuildingLightmaps (model_t *m);

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
	out = Hunk_Alloc (size*sizeof(*out));

	loadmodel->surfaces = out;
	loadmodel->numsurfaces = size;

	currentmodel = loadmodel;

	GL_BeginBuildingLightmaps (loadmodel);

	for (surfnum = 0; surfnum < size; surfnum++, in++, out++)
	{
		out->firstedge = in->firstedge;
		out->numedges = in->numedges;
		out->flags = 0;
		out->polys = NULL;

		if (in->side)
			out->flags |= SURF_PLANEBACK;

		out->plane = loadmodel->planes + in->planenum;

		ti = in->texinfo;
		if (ti < 0 || ti >= loadmodel->numtexinfo)
			Com_Error (ERR_DROP, "MOD_LoadBmodel: bad texinfo number");
		out->texinfo = loadmodel->texinfo + ti;

		CalcSurfaceExtents (out);

		// lighting info

		for (i = 0; i < MAXLIGHTMAPS; i++)
			out->styles[i] = in->styles[i];
		i = in->lightofs;
		if (i == -1)
			out->samples = NULL;
		else
			out->samples = loadmodel->lightdata + i;

		// set the drawing flags

		if (out->texinfo->flags & SURF_WARP)
		{
			out->flags |= SURF_DRAWTURB;
			for (i=0 ; i<2 ; i++)
			{
				out->extents[i] = 16384;
				out->texturemins[i] = -8192;
			}
			GL_SubdivideSurface (out);	// cut up polygon for warps
		}

		// create lightmaps and polygons
		if ( !(out->texinfo->flags & (SURF_SKY|SURF_TRANS33|SURF_TRANS66|SURF_WARP) ) )
			GL_CreateSurfaceLightmap (out);

		if (! (out->texinfo->flags & SURF_WARP) )
			GL_BuildPolygonFromSurface(out);
	}

	GL_EndBuildingLightmaps ();
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
		out->contents = -1;	// differentiate from leafs

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

		// gl underwater warp
#if 0
		if (out->contents & (CONTENTS_WATER|CONTENTS_SLIME|CONTENTS_LAVA|CONTENTS_THINWATER) )
		{
			for (j=0 ; j<out->nummarksurfaces ; j++)
			{
				out->firstmarksurface[j]->flags |= SURF_UNDERWATER;
				for (poly = out->firstmarksurface[j]->polys ; poly ; poly=poly->next)
					poly->flags |= SURF_UNDERWATER;
			}
		}
#endif
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
//	if (size < 1 || size >= MAX_MAP_SURFEDGES)
//		Com_Error (ERR_DROP, "MOD_LoadBmodel: bad surfedges count in %s: %i",
//		loadmodel->name, size);

	loadmodel->surfedges = data;
	loadmodel->numsurfedges = size;
}


/*
=================
Mod_LoadPlanes
=================
*/
void Mod_LoadPlanes (dplane_t *data, int size)
{
	int			i, j;
	cplane_t	*out;
	dplane_t 	*in;
	int			bits;

	in = data;
	out = Hunk_Alloc (size*sizeof(*out));	//?? old:  "size*2*sizeof(*out)"; WHY ??!! (Q3 has the same ...); removed 13.03.2002

	loadmodel->planes = out;
	loadmodel->numplanes = size;

	for (i = 0; i < size; i++, in++, out++)
	{
		bits = 0;
		for (j = 0; j < 3; j++)
		{
			out->normal[j] = in->normal[j];
			if (out->normal[j] < 0)
				bits |= 1<<j;
		}

		out->dist = in->dist;
		out->type = in->type;
		out->signbits = bits;
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
	mmodel_t 	*bm;

	loadmodel->type = mod_brush;
	if (loadmodel != gl_mod_known)
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
	mod->numframes = 2;		// regular and alternate animation

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
		starmod->firstnode = bm->headnode;		//?? used for dlights markup
		if (starmod->firstnode >= loadmodel->numnodes)
			Com_Error (ERR_DROP, "Inline model %i has bad firstnode", i);

		VectorCopy (bm->maxs, starmod->maxs);
		VectorCopy (bm->mins, starmod->mins);
		starmod->radius = bm->radius;

		if (i == 0)
			*loadmodel = *starmod;

		starmod->numleafs = 0; //?? unused and undefined ?? bm->visleafs;
	}
}

/*
==============================================================================

HALF-LIFE MAP SUPPORT

==============================================================================
*/

/*
=================
Mod_LoadHLTextures
=================
*/

//!! we should store info in special array
void Mod_LoadHLTextures (lump_t *l)
{
	hl_dmiptexlump_t *lump;

}

/*
=================
Mod_LoadHLTexinfo
=================
*/

void Mod_LoadHLTexinfo (lump_t *l)
{
	hl_texinfo_t *in;
	mtexinfo_t *out, *step;
	int 	i, j, count;
	char	name[MAX_QPATH];
	int		next;

//--	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Com_Error (ERR_DROP, "MOD_LoadBmodel: funny lump size in %s",loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = Hunk_Alloc ( count*sizeof(*out));

	loadmodel->texinfo = out;
	loadmodel->numtexinfo = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		for (j=0 ; j<8 ; j++)
			out->vecs[0][j] = LittleFloat (in->vecs[0][j]);

		out->flags = 0; //?? LittleLong (in->flags); hl_texinfo_t.flags ignored by Q1?
//		next = -1; //?? LittleLong (in->nexttexinfo);
//		if (next > 0)
//			out->next = loadmodel->texinfo + next;
//		else
			out->next = NULL;

//		Com_sprintf (name, sizeof(name), "textures/%s.wal", in->texture);
//		out->image = GL_FindImage (name, it_wall);
//		if (!out->image)
//		{
//			Com_Printf ("Couldn't load %s\n", name);
			out->image = r_notexture;
//		}
	}

	// count animation frames
	for (i=0 ; i<count ; i++)
	{
		out = &loadmodel->texinfo[i];
		out->numframes = 1;
//		for (step = out->next ; step && step != out ; step=step->next)
//			out->numframes++;
	}
}


/*
=================
Mod_LoadHLNodes
=================
*/
void Mod_LoadHLNodes (lump_t *l)
{
	int			i, j, count, p;
	hl_dnode_t		*in;
	mnode_t 	*out;

//--	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Com_Error (ERR_DROP, "MOD_LoadHLBmodel: funny lump size in %s",loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = Hunk_Alloc ( count*sizeof(*out));

	loadmodel->nodes = out;
	loadmodel->numnodes = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		for (j=0 ; j<3 ; j++)
		{
			out->minmaxs[j] = LittleShort (in->mins[j]);
			out->minmaxs[3+j] = LittleShort (in->maxs[j]);
		}

		p = LittleLong(in->planenum);
		out->plane = loadmodel->planes + p;

		out->firstsurface = LittleShort (in->firstface);
		out->numsurfaces = LittleShort (in->numfaces);
		out->contents = -1;	// differentiate from leafs

		for (j=0 ; j<2 ; j++)
		{
			p = LittleShort (in->children[j]);
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
Mod_LoadHLSubmodels
=================
*/
void Mod_LoadHLSubmodels (lump_t *l)
{
	hl_dmodel_t	*in;
	mmodel_t	*out;
	int			i, j, count;

//--	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Com_Error (ERR_DROP, "MOD_LoadBmodel: funny lump size in %s",loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = Hunk_Alloc (count*sizeof(*out));

	loadmodel->submodels = out;
	loadmodel->numsubmodels = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		for (j=0 ; j<3 ; j++)
		{	// spread the mins / maxs by a pixel
			out->mins[j] = LittleFloat (in->mins[j]) - 1;
			out->maxs[j] = LittleFloat (in->maxs[j]) + 1;
			out->origin[j] = LittleFloat (in->origin[j]);
		}
		out->radius = RadiusFromBounds (out->mins, out->maxs);
		out->headnode = LittleLong (in->headnode[0]); // use only headnode[0]
		out->firstface = LittleLong (in->firstface);
		out->numfaces = LittleLong (in->numfaces);
	}
}


/*
=================
Load HL Visibility and Leafs
=================
*/

#define HL_CONTENTS 15
static int HLContents[HL_CONTENTS] =
{
	0,
	CONTENTS_SOLID,
	CONTENTS_WATER,
	CONTENTS_SLIME,
	CONTENTS_LAVA,
	0, // SKY
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

void Mod_LoadHLLeafs (lump_t *l)
{
	hl_dleaf_t 	*in;
	mleaf_t 	*out;
	int			i, j, count, p;

//--	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
		Com_Error (ERR_DROP, "MOD_LoadBmodel: funny lump size in %s",loadmodel->name);
	count = l->filelen / sizeof(*in);
	out = Hunk_Alloc ( count*sizeof(*out));

	loadmodel->leafs = out;
	loadmodel->numleafs = count;

	for (i = 0; i < count; i++, in++, out++)
	{
		for (j=0 ; j<3 ; j++)
		{
			out->minmaxs[j] = LittleShort (in->mins[j]);
			out->minmaxs[3+j] = LittleShort (in->maxs[j]);
		}

		p = LittleLong(in->contents);
		if (p < 0 && p >= -HL_CONTENTS)
			p = HLContents[- p - 1];
		else
			p = CONTENTS_SOLID; // error message?
		out->contents = p;

		out->cluster = LittleLong(in->visofs); // will be used later in Mod_LoadHLVisibility()
		out->area = 0; // no in Q1/HL

		out->firstmarksurface = loadmodel->marksurfaces +
			LittleShort(in->firstmarksurface);
		out->nummarksurfaces = LittleShort(in->nummarksurfaces);
	}
}


/*
 * Generate dvis_t structure { numclusters: int; bitofs: array[][2] of int }
 * SHOULD BE CALLED AFTER Mod_LoadHLLeafs() !
 */
void Mod_LoadHLVisibility (lump_t *l)
{
	int		i, size, num, vis;

	if (!l->filelen)
	{
		loadmodel->vis = NULL;
		return;
	}

	num = loadmodel->numleafs;
	size = 4 + 8 * num; // sizeof(dvis_t)
	loadmodel->vis = Hunk_Alloc (l->filelen + size);
        loadmodel->vis->numclusters = num;
//--	memcpy ((char*)loadmodel->vis + size, mod_base + l->fileofs, l->filelen);

	for (i = 0 ; i < num; i++)
	{
		vis = loadmodel->leafs[i].cluster; // this field was temporarily stored visofs
		if (vis != -1) vis += size; // -1 will be -1
		loadmodel->vis->bitofs[i][DVIS_PVS] = vis;
		loadmodel->vis->bitofs[i][DVIS_PHS] = -1; // no visinfo
		loadmodel->leafs[i].cluster = i; //?? -1?
	}
}



/*
=================
Mod_LoadHLBrushModel
=================
*/
void Mod_LoadHLBrushModel (model_t *mod, void *buffer)
{
/*	int			i;
	hl_dheader_t	*header;
	mmodel_t 	*bm;

	loadmodel->type = mod_hl_brush;
	if (loadmodel != gl_mod_known)
		Com_Error (ERR_DROP, "Loaded a brush model after the world");

	header = (hl_dheader_t *)buffer;

	// This check is not required: Mod_LoadHLBrushModel() called after detecting HL_BSPVERSION
//	i = LittleLong (header->version);
//	if (i != HL_BSPVERSION)
//		Com_Error (ERR_DROP, "Mod_LoadBrushModel: %s has wrong version number (%i should be %i)", mod->name, i, BSPVERSION);

	// swap all the lumps
	mod_base = (byte *)header;

	for (i=0 ; i<sizeof(hl_dheader_t)/4 ; i++)
		((int *)header)[i] = LittleLong ( ((int *)header)[i]);

	// load into heap
	//!! We must parse entities before to detect "wad" name (may be multiple files, ignore path part...)
	// Some lumps are the same as in Q2, some absent in Q2, some diferent...
	Mod_LoadVertexes (&header->lumps[HL_LUMP_VERTEXES]);	// same as Q2
	Mod_LoadEdges (&header->lumps[HL_LUMP_EDGES]);		// same as Q2
	Mod_LoadSurfedges (&header->lumps[HL_LUMP_SURFEDGES]);	// same as Q2

//??	HL_Mod_LoadTextures (&header->lumps[HL_LUMP_TEXTURES]);	// no in Q2
	Mod_LoadLighting (&header->lumps[HL_LUMP_LIGHTING]);	// same as Q2

	Mod_LoadPlanes (&header->lumps[HL_LUMP_PLANES]);	// same as Q2
	Mod_LoadHLTextures (&header->lumps[HL_LUMP_TEXTURES]);	// no in Q2
	Mod_LoadHLTexinfo (&header->lumps[HL_LUMP_TEXINFO]);	// different - use WADs (!! unimplemented now !!)
	Mod_LoadFaces (&header->lumps[HL_LUMP_FACES]); //!! check HL-Q2 SURF_XXX flags
	Mod_LoadMarksurfaces (&header->lumps[HL_LUMP_MARKSURFACES]);

	Mod_LoadHLLeafs (&header->lumps[HL_LUMP_LEAFS]);
	Mod_LoadHLVisibility (&header->lumps[HL_LUMP_VISIBILITY]);

	Mod_LoadHLNodes (&header->lumps[HL_LUMP_NODES]);
	Mod_LoadHLSubmodels (&header->lumps[HL_LUMP_MODELS]);
//??	Mod_LoadEntities (&header->lumps[HL_LUMP_ENTITIES]);	// no in Q2
	mod->numframes = 2;		// regular and alternate animation

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
		starmod->radius = bm->radius;

		if (i == 0)
			*loadmodel = *starmod;

		starmod->numleafs = bm->visleafs;
	}
*/
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

	/*---- byte swap the header fields and sanity check ----*/
	for (i=0 ; i<sizeof(dmdl_t)/4 ; i++)
		((int *)pheader)[i] = LittleLong (((int *)buffer)[i]);

	if (pheader->skinHeight > MAX_LBM_HEIGHT)	//?? unneeded
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

	/*---- load base s and t vertices (not used in gl version) ----*/
	pinst = (dStVert_t *) ((byte *)pinmodel + pheader->ofsSt);
	poutst = (dStVert_t *) ((byte *)pheader + pheader->ofsSt);

	for (i=0 ; i<pheader->numSt ; i++)
	{
		poutst[i].s = LittleShort (pinst[i].s);
		poutst[i].t = LittleShort (pinst[i].t);
	}

	/*---------------- load triangle lists ------------------------*/
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

	/*------------ load the frames -------------*/
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
		memcpy (poutframe->verts, pinframe->verts,
			pheader->numXyz*sizeof(dTriVertx_t));
	}

	mod->type = mod_alias;

	/*------- load the glcmds ------------*/
	pincmd = (int *) ((byte *)pinmodel + pheader->ofsGlcmds);
	poutcmd = (int *) ((byte *)pheader + pheader->ofsGlcmds);
	for (i=0 ; i<pheader->numGlcmds ; i++)
		poutcmd[i] = LittleLong (pincmd[i]);


	/*------- register all skins -----------*/
	memcpy ((char *)pheader + pheader->ofsSkins, (char *)pinmodel + pheader->ofsSkins,
		pheader->numSkins*MAX_SKINNAME);
	for (i = 0; i < pheader->numSkins; i++)
	{
//		Com_Printf ("^1skin #%d  %s\n", i, (char *)pheader + pheader->ofsSkins + i*MAX_SKINNAME);
		mod->skins[i] = GL_FindImage ((char *)pheader + pheader->ofsSkins + i*MAX_SKINNAME, it_skin);
	}
//	mod->numframes = pheader->num_frames;

	mod->mins[0] = -32;		//?? compute from verts, each frame - own bbox
	mod->mins[1] = -32;
	mod->mins[2] = -32;
	mod->maxs[0] = 32;
	mod->maxs[1] = 32;
	mod->maxs[2] = 32;
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
		mod->skins[i] = GL_FindImage (sprout->frames[i].name, it_sprite);
		if (!mod->skins[i]) Com_DPrintf ("Cannot find frame #%d (%s)\n", i, sprout->frames[i].name);
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
	r_viewcluster = -2;
	r_oldviewcluster = -2;		// force markleafs

	Com_sprintf (fullname, sizeof(fullname), "maps/%s.bsp", model);

	// explicitly free the old map if different
	// this guarantees that gl_mod_known[0] is the world map
	flushmap = Cvar_Get ("flushmap", "0", 0);
	if ( strcmp(gl_mod_known[0].name, fullname) || flushmap->integer)
		Mod_Free (&gl_mod_known[0]);
	r_worldmodel = Mod_ForName(fullname, true);
	r_worldmodel->registration_sequence = registration_sequence;
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
		if (mod->registration_sequence == registration_sequence)
			return mod;

		mod->registration_sequence = registration_sequence;

		// register any images used by the models
		if (mod->type == mod_sprite)
		{
			sprout = (dsprite_t *)mod->extradata;
			for (i=0 ; i<sprout->numframes ; i++)
				mod->skins[i] = GL_FindImage (sprout->frames[i].name, it_sprite);
		}
		else if (mod->type == mod_alias)
		{
			pheader = (dmdl_t *)mod->extradata;
			for (i=0 ; i<pheader->numSkins ; i++)
				mod->skins[i] = GL_FindImage ((char *)pheader + pheader->ofsSkins + i*MAX_SKINNAME, it_skin);
//PGM
			mod->numframes = pheader->numFrames;
//PGM
		}
		else if (mod->type == mod_brush || mod->type == mod_hl_brush)
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

	for (i = 0, mod = gl_mod_known; i < gl_mod_numknown ; i++, mod++)
	{
		if (!mod->name[0])
			continue;
		if (mod->registration_sequence != registration_sequence)
		{	// don't need this model
			Mod_Free (mod);
		}
	}

	GL_FreeUnusedImages ();
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

	for (i = 0; i < gl_mod_numknown ; i++)
	{
		if (gl_mod_known[i].extradatasize)
			Mod_Free (&gl_mod_known[i]);
	}
}
