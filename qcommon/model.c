#include "qcommon.h"

bspfile_t bspfile;


//?? Should perform SwapBlock() (as in Q3 bsp tools)
void SwapQ2BspFile (bspfile_t *f)
{
	int		i, j;

	// models
	for (i = 0; i < f->numModels; i++)
	{
		dmodel_t	*d;

		d = &f->models[i];

		d->firstface = LittleLong (d->firstface);
		d->numfaces = LittleLong (d->numfaces);
		d->headnode = LittleLong (d->headnode);

		for (j = 0; j < 3; j++)
		{
			d->mins[j] = LittleFloat(d->mins[j]);
			d->maxs[j] = LittleFloat(d->maxs[j]);
			d->origin[j] = LittleFloat(d->origin[j]);
		}
	}

	// vertexes
	for (i = 0; i < f->numVertexes; i++)
	{
		for (j = 0; j < 3; j++)
			f->vertexes[i].point[j] = LittleFloat (f->vertexes[i].point[j]);
	}

	// planes
	for (i = 0; i < f->numPlanes; i++)
	{
		for (j = 0; j < 3; j++)
			f->planes[i].normal[j] = LittleFloat (f->planes[i].normal[j]);
		f->planes[i].dist = LittleFloat (f->planes[i].dist);
		f->planes[i].type = LittleLong (f->planes[i].type);
	}

	// texinfos
	for (i = 0; i < f->numTexinfo; i++)
	{
		texinfo_t *d;

		d = &f->texinfo[i];

		for (j = 0 ;j < 8; j++)
			d->vecs[0][j] = LittleFloat (d->vecs[0][j]);
		d->flags = LittleLong (d->flags);
		d->value = LittleLong (d->value);
		d->nexttexinfo = LittleLong (d->nexttexinfo);
	}

	// faces
	for (i = 0; i < f->numFaces; i++)
	{
		f->faces[i].texinfo = LittleShort (f->faces[i].texinfo);
		f->faces[i].planenum = LittleShort (f->faces[i].planenum);
		f->faces[i].side = LittleShort (f->faces[i].side);
		f->faces[i].lightofs = LittleLong (f->faces[i].lightofs);
		f->faces[i].firstedge = LittleLong (f->faces[i].firstedge);
		f->faces[i].numedges = LittleShort (f->faces[i].numedges);
	}

	// nodes
	for (i = 0; i < f->numNodes; i++)
	{
		f->nodes[i].planenum = LittleLong (f->nodes[i].planenum);
		for (j = 0; j < 3; j++)
		{
			f->nodes[i].mins[j] = LittleShort (f->nodes[i].mins[j]);
			f->nodes[i].maxs[j] = LittleShort (f->nodes[i].maxs[j]);
		}
		f->nodes[i].children[0] = LittleLong (f->nodes[i].children[0]);
		f->nodes[i].children[1] = LittleLong (f->nodes[i].children[1]);
		f->nodes[i].firstface = LittleShort (f->nodes[i].firstface);
		f->nodes[i].numfaces = LittleShort (f->nodes[i].numfaces);
	}

	// leafs
	for (i = 0; i < f->numLeafs; i++)
	{
		f->leafs[i].contents = LittleLong (f->leafs[i].contents);
		f->leafs[i].cluster = LittleShort (f->leafs[i].cluster);
		f->leafs[i].area = LittleShort (f->leafs[i].area);
		for (j = 0; j < 3; j++)
		{
			f->leafs[i].mins[j] = LittleShort (f->leafs[i].mins[j]);
			f->leafs[i].maxs[j] = LittleShort (f->leafs[i].maxs[j]);
		}

		f->leafs[i].firstleafface = LittleShort (f->leafs[i].firstleafface);
		f->leafs[i].numleaffaces = LittleShort (f->leafs[i].numleaffaces);
		f->leafs[i].firstleafbrush = LittleShort (f->leafs[i].firstleafbrush);
		f->leafs[i].numleafbrushes = LittleShort (f->leafs[i].numleafbrushes);
	}

	// leaffaces
	for (i = 0; i < f->numLeaffaces; i++)
		f->leaffaces[i] = LittleShort (f->leaffaces[i]);

	// leafbrushes
	for (i = 0; i < f->numLeafbrushes; i++)
		f->leafbrushes[i] = LittleShort (f->leafbrushes[i]);

	// surfedges
	for (i = 0; i < f->numSurfedges; i++)
		f->surfedges[i] = LittleLong (f->surfedges[i]);

	// edges
	for (i = 0; i < f->numEdges; i++)
	{
		f->edges[i].v[0] = LittleShort (f->edges[i].v[0]);
		f->edges[i].v[1] = LittleShort (f->edges[i].v[1]);
	}

	// brushes
	for (i = 0; i < f->numBrushes; i++)
	{
		f->brushes[i].firstside = LittleLong (f->brushes[i].firstside);
		f->brushes[i].numsides = LittleLong (f->brushes[i].numsides);
		f->brushes[i].contents = LittleLong (f->brushes[i].contents);
	}

	// areas
	for (i = 0; i < f->numAreas; i++)
	{
		f->areas[i].numareaportals = LittleLong (f->areas[i].numareaportals);
		f->areas[i].firstareaportal = LittleLong (f->areas[i].firstareaportal);
	}

	// areasportals
	for (i = 0; i < f->numAreaportals; i++)
	{
		f->areaportals[i].portalnum = LittleLong (f->areaportals[i].portalnum);
		f->areaportals[i].otherarea = LittleLong (f->areaportals[i].otherarea);
	}

	// brushsides
	for (i = 0; i < f->numBrushsides; i++)
	{
		f->brushsides[i].planenum = LittleShort (f->brushsides[i].planenum);
		f->brushsides[i].texinfo = LittleShort (f->brushsides[i].texinfo);
	}

	// visibility
	if (f->visDataSize)		// should process this only when map have visibility data
	{
		j = f->visibility->numclusters = LittleLong (f->visibility->numclusters);
		for (i = 0; i < j ; i++)
		{
			f->visibility->bitofs[i][0] = LittleLong (f->visibility->bitofs[i][0]);
			f->visibility->bitofs[i][1] = LittleLong (f->visibility->bitofs[i][1]);
		}
	}
}

void ProcessQ2BspFile (bspfile_t *f)
{
	int i, j;

	// models: inflate mins/maxs by 1
	for (i = 0; i < f->numModels; i++)
	{
		dmodel_t	*d;

		d = &f->models[i];

		for (j = 0; j < 3; j++)
		{
			d->mins[j] = d->mins[j] - 1;
			d->maxs[j] = d->maxs[j] + 1;
		}
	}

	// texinfo: lowercase all filenames and remove leading "." and "/"
	for (i = 0; i < f->numTexinfo; i++)
	{
		texinfo_t *d;
		char *s, c;

		d = &f->texinfo[i];
		s = d->texture;
		for (j = 0; j < sizeof(d->texture); j++, s++)
		{
			c = *s;
			if (c >= 'A' && c <= 'Z') *s = c + 32;
		}
		s = d->texture;
		if (s[0] == '.') s++;
		if (s[0] == '/') s++;
		if (s != d->texture)
			strcpy (d->texture, s);
	}

	// silly check for correct lightmaps
	for (i = 0; i < f->numFaces; i++)
		if (f->faces[i].lightofs > f->lightDataSize)
			f->faces[i].lightofs = -1;
}


dheader_t	*header;

int CheckLump (int lump, void **ptr, int size)
{
	int		length, ofs;

	length = header->lumps[lump].filelen;
	ofs = header->lumps[lump].fileofs;

	if (length % size)
		Com_Error (ERR_DROP, "LoadBSPFile: funny lump size\n");

	*ptr = (byte *)header + ofs;

	return length / size;
}


void LoadQ2BspFile (void)
{
	int			i;

	header = (dheader_t *) bspfile.file;

	// swap the header
	for (i = 0; i < sizeof(dheader_t) / 4; i++)
		((int *)header)[i] = LittleLong (((int *)header)[i]);

	if (header->version != BSPVERSION)
		Com_Error (ERR_DROP, "%s is version %i, not %i\n", bspfile.name, header->version, BSPVERSION);

	bspfile.lightDataSize = CheckLump (LUMP_LIGHTING, &bspfile.lighting, 1);
	bspfile.entDataSize = CheckLump (LUMP_ENTITIES, &bspfile.entities, 1);
	bspfile.visDataSize = CheckLump (LUMP_VISIBILITY, &bspfile.visibility, 1);

	bspfile.numModels = CheckLump (LUMP_MODELS, &bspfile.models, sizeof(dmodel_t));
	bspfile.numVertexes = CheckLump (LUMP_VERTEXES, &bspfile.vertexes, sizeof(dvertex_t));
	bspfile.numPlanes = CheckLump (LUMP_PLANES, &bspfile.planes, sizeof(dplane_t));
	bspfile.numLeafs = CheckLump (LUMP_LEAFS, &bspfile.leafs, sizeof(dleaf_t));
	bspfile.numNodes = CheckLump (LUMP_NODES, &bspfile.nodes, sizeof(dnode_t));
	bspfile.numTexinfo = CheckLump (LUMP_TEXINFO, &bspfile.texinfo, sizeof(texinfo_t));
	bspfile.numFaces = CheckLump (LUMP_FACES, &bspfile.faces, sizeof(dface_t));
	bspfile.numLeaffaces = CheckLump (LUMP_LEAFFACES, &bspfile.leaffaces, sizeof(unsigned short));
	bspfile.numLeafbrushes = CheckLump (LUMP_LEAFBRUSHES, &bspfile.leafbrushes, sizeof(unsigned short));
	bspfile.numSurfedges = CheckLump (LUMP_SURFEDGES, &bspfile.surfedges, sizeof(int));
	bspfile.numEdges = CheckLump (LUMP_EDGES, &bspfile.edges, sizeof(dedge_t));
	bspfile.numBrushes = CheckLump (LUMP_BRUSHES, &bspfile.brushes, sizeof(dbrush_t));
	bspfile.numBrushsides = CheckLump (LUMP_BRUSHSIDES, &bspfile.brushsides, sizeof(dbrushside_t));
	bspfile.numAreas = CheckLump (LUMP_AREAS, &bspfile.areas, sizeof(darea_t));
	bspfile.numAreaportals = CheckLump (LUMP_AREAPORTALS, &bspfile.areaportals, sizeof(dareaportal_t));

	// swap everything
	SwapQ2BspFile (&bspfile);
	ProcessQ2BspFile (&bspfile);
}


bspfile_t *LoadBspFile (char *filename, qboolean clientload, unsigned *checksum)
{
	if (!stricmp (filename, bspfile.name) && (clientload || !Cvar_VariableInt ("flushmap")))
	{
		if (checksum)
			*checksum = bspfile.checksum;
		return &bspfile;
	}

	if (bspfile.name[0] && bspfile.file)
		FS_FreeFile (bspfile.file);

	strcpy (bspfile.name, filename);
	bspfile.length = FS_LoadFile (filename, &bspfile.file);
	if (!bspfile.file)
	{
		bspfile.name[0] = 0;
		Com_Error (ERR_DROP, "Couldn't load %s", filename);
	}
	bspfile.checksum = LittleLong (Com_BlockChecksum (bspfile.file, bspfile.length));
		if (checksum)
			*checksum = bspfile.checksum;

	switch (LittleLong(*(unsigned *)bspfile.file))
	{
		case IDBSPHEADER:
			LoadQ2BspFile ();
			return &bspfile;
//??		case HL_BSPVERSION:
//			LoadHLBspFile ();
//			return &bspfile;
	}
	FS_FreeFile (bspfile.file);
	bspfile.name[0] = 0;
	bspfile.file = NULL;
	Com_Error (ERR_DROP, "LoadBrushModel: %s has a wrong BSP header\n", filename);
	return NULL;		// make compiler happy
}
