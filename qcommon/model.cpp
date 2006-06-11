#include "qcommon.h"
#include "cmodel.h"


bspfile_t bspfile;
static lump_t *lumps;


//?? move swappers to qfiles.h as methods for structures
#if !LITTLE_ENDIAN
static void SwapQ2BspFile (bspfile_t *f)
{
	int		i, j;

	//!! ERROR HERE: should swap models before loading as cmodel_t!!
	// models
	for (i = 0; i < f->numModels; i++)
	{
		cmodel_t &d = f->models[i];
		LTL(d.firstface);
		LTL(d.numfaces);
		LTL(d.headnode);

		LTL(d.bounds.mins);
		LTL(d.bounds.maxs);
//		LTL(d.origin);
	}

	// vertexes
	for (i = 0; i < f->numVertexes; i++)
		LTL(f->vertexes[i]);

	// planes
	for (i = 0; i < f->numPlanes; i++)
	{
		LTL(f->planes[i].normal);
		LTL(f->planes[i].dist);
	}

	// texinfos
	for (i = 0; i < f->numTexinfo; i++)
	{
		dBsp2Texinfo_t &d = f->texinfo[i];
		float *vec = &d.vecs[0].vec[0];		// hack to get all ([3]+1)[2] == 8 components of dBsp2Texinfo_t.vecs[]
		for (j = 0; j < 8; j++)
			LTL(vec[j]);
		LTL(d.flags);
		LTL(d.value);
		LTL(d.nexttexinfo);
	}

	// faces
	for (i = 0; i < f->numFaces; i++)
	{
		dFace_t &face = f->faces[i];
		LTL(face.texinfo);
		LTL(face.planenum);
		LTL(face.side);
		LTL(face.lightofs);
		LTL(face.firstedge);
		LTL(face.numedges);
	}

	// nodes
	for (i = 0; i < f->numNodes; i++)
	{
		dBsp2Node_t &node = f->nodes2[i];
		LTL(node.planenum);
		for (j = 0; j < 3; j++)
		{
			LTL(node.mins[j]);
			LTL(node.maxs[j]);
		}
		LTL(node.children[0]);
		LTL(node.children[1]);
		LTL(node.firstface);
		LTL(node.numfaces);
	}

	// leafs
	for (i = 0; i < f->numLeafs; i++)
	{
		dBsp2Leaf_t &leaf = f->leafs2[i];
		LTL(leaf.contents);
		LTL(leaf.cluster);
		LTL(leaf.area);
		for (j = 0; j < 3; j++)
		{
			LTL(leaf.mins[j]);
			LTL(leaf.maxs[j]);
		}

		LTL(leaf.firstleafface);
		LTL(leaf.numleaffaces);
		LTL(leaf.firstleafbrush);
		LTL(leaf.numleafbrushes);
	}

	// leaffaces
	for (i = 0; i < f->numLeaffaces; i++)
		LTL(f->leaffaces[i]);

	// leafbrushes
	for (i = 0; i < f->numLeafbrushes; i++)
		LTL(f->leafbrushes[i]);

	// surfedges
	for (i = 0; i < f->numSurfedges; i++)
		LTL(f->surfedges[i]);

	// edges
	for (i = 0; i < f->numEdges; i++)
	{
		LTL(f->edges[i].v[0]);
		LTL(f->edges[i].v[1]);
	}

	// brushes
	for (i = 0; i < f->numBrushes; i++)
	{
		LTL(f->brushes[i].firstside);
		LTL(f->brushes[i].numsides);
		LTL(f->brushes[i].contents);
	}

	// areas
	for (i = 0; i < f->numAreas; i++)
	{
		LTL(f->areas[i].numareaportals);
		LTL(f->areas[i].firstareaportal);
	}

	// areasportals
	for (i = 0; i < f->numAreaportals; i++)
	{
		LTL(f->areaportals[i].portalnum);
		LTL(f->areaportals[i].otherarea);
	}

	// brushsides
	for (i = 0; i < f->numBrushSides; i++)
	{
		LTL(f->brushsides[i].planenum);
		LTL(f->brushsides[i].texinfo);
	}

	// visibility
	if (f->visDataSize)		// should process this only when map have visibility data
	{
		LTL(f->vis->numclusters);
		j = f->vis->numclusters;
		for (i = 0; i < j ; i++)
		{
			LTL(f->vis->bitofs[i][0]);
			LTL(f->vis->bitofs[i][1]);
		}
	}
}

static void SwapQ1BspFile (bspfile_t *f)
{
	//!! implement this
}

#endif // LITTLE_ENDIAN

static void ProcessQ2BspFile (bspfile_t *f)
{
	int i, j;

	// models: inflate mins/maxs by 1
	//?? this is to avoid precision errors ?
	for (i = 0; i < f->numModels; i++)
	{
		cmodel_t &d = f->models[i];

		for (j = 0; j < 3; j++)
		{
			d.bounds.mins[j] -= 1;
			d.bounds.maxs[j] += 1;
		}
	}

	// texinfo: lowercase all filenames and remove leading "." and "/"
	for (i = 0; i < f->numTexinfo; i++)
	{
		dBsp2Texinfo_t &d = f->texinfo2[i];
		char *s = d.texture;
		for (j = 0; j < sizeof(d.texture); j++, s++)
			*s = toLower (*s);
		s = d.texture;
		if (s[0] == '.') s++;
		if (s[0] == '/') s++;
		if (s != d.texture)
			strcpy (d.texture, s);
	}

	// silly check for correct lightmaps
	for (i = 0; i < f->numFaces; i++)
		if (f->faces[i].lightofs > f->lightDataSize)
			f->faces[i].lightofs = -1;

	// get numClusters (have in dBsp2Vis_t, but Q2 recomputes this ...)
	f->numClusters = 0;
	for (i = 0; i < f->numLeafs; i++)
		if (f->leafs2[i].cluster >= f->numClusters)
			f->numClusters = f->leafs2[i].cluster + 1;
}


static void ProcessQ1BspFile (bspfile_t *f)
{
	int i, j;

	// models: inflate mins/maxs by 1
	//?? this is to avoid precision errors ?
	for (i = 0; i < f->numModels; i++)
	{
		cmodel_t &d = f->models[i];

		for (j = 0; j < 3; j++)
		{
			d.bounds.mins[j] -= 1;
			d.bounds.maxs[j] += 1;
		}
	}
	for (i = 0; i < f->numFaces; i++)
	{
		dFace_t &face = f->faces[i];
		// silly check for correct lightmaps
		if (face.lightofs > f->lightDataSize)
			face.lightofs = -1;
	}
}


static void LoadQ2Submodels (bspfile_t *f, dBsp2Model_t *data)
{
	if (f->numModels < 1)
		Com_DropError ("Map with no models");

	cmodel_t *out = f->models = new (f->extraChain) cmodel_t[f->numModels];
	for (int i = 0; i < f->numModels; i++, data++, out++)
	{
		out->bounds    = data->bounds;
		out->radius    = VectorDistance (out->bounds.mins, out->bounds.maxs) / 2;
		out->headnode  = data->headnode;
		out->flags     = 0;
		out->firstface = data->firstface;
		out->numfaces  = data->numfaces;
		out->color.rgba = RGBA(1,1,1,1);
		// dBsp2Model_t have unused field "origin"
	}
}

static void LoadQ1Submodels (bspfile_t *f, dBsp1Model_t *data)
{
	if (f->numModels < 1)
		Com_DropError ("Map with no models");

	cmodel_t *out = f->models = new (f->extraChain) cmodel_t[f->numModels];
	for (int i = 0; i < f->numModels; i++, data++, out++)
	{
		out->bounds    = data->bounds;
		out->radius    = VectorDistance (out->bounds.mins, out->bounds.maxs) / 2;
		out->headnode  = data->headnode[0];
		out->flags     = 0;
		out->firstface = data->firstface;
		out->numfaces  = data->numfaces;
		out->color.rgba = RGBA(1,1,1,1);
		// dBsp1Model_t have unused field "origin"
	}
}


static void DecompressVis (byte *dst, void *vis, int pos, int rowSize)
{
	if (pos == -1)
	{
		memset (dst, 0xFF, rowSize);	// all visible
		dst += rowSize;
		return;
	}

	byte *src = (byte*)vis + pos;
	// decompress vis
	for (int j = rowSize; j; /*empty*/)
	{
		byte c = *src++;
		if (c)
		{	// non-zero byte
			*dst++ = c;
			j--;
		}
		else
		{	// zero byte -- decompress RLE data (with filler 0)
			c = *src++;				// count
			c = min(c, j);			// should not be, but ...
			j -= c;
			while (c--)
				*dst++ = 0;
		}
	}
}

static void LoadQ2Vis (bspfile_t *f, dBsp2Vis_t *vis, int size)
{
	if (!size)
	{
		Com_DPrintf ("No visinfo in map\n");
		if (f->numClusters > 1)
		{
			if (developer->integer)
				appWPrintf ("WARNING: map with cluster info but without visinfo\n");
		}
		f->numClusters = 1;				// required
		f->visInfo     = NULL;
		return;
	}
	// NOTE: 'size' is ignored later

	int rowSize;
	f->visRowSize = rowSize = (f->numClusters + 7) >> 3;

	// decompress visinfo
	byte *dst = new (f->extraChain) byte [rowSize * f->numClusters];
	f->visInfo = dst;
	for (int i = 0; i < f->numClusters; i++, dst += rowSize)
		DecompressVis (dst, vis, vis->bitofs[i][DVIS_PVS], rowSize);
	Com_DPrintf ("Decompressed vis: %d -> %d bytes\n", size, rowSize * f->numClusters);
}

static void LoadQ1Vis (bspfile_t *f, byte *vis, int size)
{
	if (!size)
	{
		Com_DPrintf ("No visinfo in map\n");
		f->numClusters = 1;				// required
		f->visInfo     = NULL;
		return;
	}
	// NOTE: 'size' is ignored later

	// create dummy clusters: 1 cluster per 1 leaf
	f->numClusters = f->numLeafs;

	int rowSize;
	f->visRowSize = rowSize = (f->numClusters + 7) >> 3;
	byte *dst = new (f->extraChain) byte [rowSize * f->numClusters];
	f->visInfo = dst;

	// start from leaf #1 (leaf 0 is CONTENTS_SOLID and have no stored visinfo)
	for (int i = 1; i < f->numLeafs; i++, dst += rowSize)
		DecompressVis (dst, vis, f->leafs1[i].visofs, rowSize);
	Com_DPrintf ("Decompressed vis: %d -> %d bytes\n", size, rowSize * f->numClusters);
}

static int CheckLump (int lump, void **ptr, int size)
{
	int length = lumps[lump].filelen;
	int ofs    = lumps[lump].fileofs;

	if (length % size)
		Com_DropError ("LoadBSPFile: incorrect lump size");

	*ptr = bspfile.file + ofs;

	return length / size;
}


void LoadQ2BspFile ()
{
	guard(LoadQ2BspFile);

	dBsp2Hdr_t *header = (dBsp2Hdr_t *) bspfile.file;
	lumps = header->lumps;

#if !LITTLE_ENDIAN
	// swap the header
	for (int i = 0; i < sizeof(dBsp2Hdr_t) / 4; i++)
		((int *)bspfile.file)[i] = LittleLong (((int *)bspfile.file)[i]);
#endif

	if (header->version != BSP2_VERSION)
		Com_DropError ("%s is version %d, not " STR(BSP2_VERSION) "\n", bspfile.name, header->version);

	bspfile.type = map_q2;
	Com_DPrintf ("Loading Q2 bsp %s\n", bspfile.name);

#define C(num,field,count,type) \
	bspfile.count = CheckLump(Q2LUMP_##num, (void**)&bspfile.field, sizeof(type))
	C(LIGHTING, lighting, lightDataSize, byte);
	C(VERTEXES, vertexes, numVertexes, CVec3);
	C(PLANES, planes, numPlanes, dPlane_t);
	C(LEAFS, leafs2, numLeafs, dBsp2Leaf_t);
	C(NODES, nodes2, numNodes, dBsp2Node_t);
	C(TEXINFO, texinfo2, numTexinfo, dBsp2Texinfo_t);
	C(FACES, faces, numFaces, dFace_t);
	C(LEAFFACES, leaffaces, numLeaffaces, unsigned short);
	C(LEAFBRUSHES, leafbrushes, numLeafbrushes, unsigned short);
	C(SURFEDGES, surfedges, numSurfedges, int);
	C(EDGES, edges, numEdges, dEdge_t);
	C(BRUSHES, brushes, numBrushes, dBsp2Brush_t);
	C(BRUSHSIDES, brushsides, numBrushSides, dBsp2Brushside_t);
	C(AREAS, areas, numAreas, darea_t);
	C(AREAPORTALS, areaportals, numAreaportals, dareaportal_t);

	dBsp2Model_t *models;
	bspfile.numModels = CheckLump (Q2LUMP_MODELS, (void**)&models, sizeof(dBsp2Model_t));	// not in bspfile_t struc
	LoadQ2Submodels (&bspfile, models);

#if !LITTLE_ENDIAN
	// swap everything
	SwapQ2BspFile (&bspfile);
#endif
	ProcessQ2BspFile (&bspfile);

	// load visinfo
	dBsp2Vis_t *vis;
	int visDataSize = CheckLump (Q2LUMP_VISIBILITY, (void**)&vis, 1);
	LoadQ2Vis (&bspfile, vis, visDataSize);
	// load entstring after all: we may require to change something
	char *entString = (char*)header + header->lumps[Q2LUMP_ENTITIES].fileofs;
	// detect kingpin map entities
	if (strstr (entString, "\"classname\" \"junior\"") ||
		strstr (entString, "\"classname\" \"lightflare\"") ||
		strstr (entString, "\"fogdensity2\""))
	{
		bspfile.type = map_kp;
		Com_DPrintf ("Kingpin map detected\n");
	}
	bspfile.entStr = ProcessEntstring (entString);
	bspfile.entStrSize = strlen (bspfile.entStr);

#undef C
	unguard;
}


void LoadQ1BspFile ()
{
	guard(LoadQ1BspFile);

	dBsp1Hdr_t *header = (dBsp1Hdr_t *) bspfile.file;
	lumps = header->lumps;

#if !LITTLE_ENDIAN
	// swap the header
	for (int i = 0; i < sizeof(dBsp1Hdr_t) / 4; i++)
		((int *)bspfile.file)[i] = LittleLong (((int *)bspfile.file)[i]);
#endif

	if (header->version == BSP1_VERSION)
		bspfile.type = map_q1;
	else
		bspfile.type = map_hl;
	Com_DPrintf ("Loading %s bsp %s\n", bspfile.type == map_q1 ? "Q1" : "HL", bspfile.name);

#define C(num,field,count,type) \
	bspfile.count = CheckLump(Q1LUMP_##num, (void**)&bspfile.field, sizeof(type))
	C(LIGHTING, lighting, lightDataSize, byte);
	C(VERTEXES, vertexes, numVertexes, CVec3);
	C(PLANES, planes, numPlanes, dPlane_t);
	C(LEAFS, leafs1, numLeafs, dBsp1Leaf_t);
	C(NODES, nodes1, numNodes, dBsp1Node_t);
	C(TEXINFO, texinfo1, numTexinfo, dBsp1Texinfo_t);
	C(FACES, faces, numFaces, dFace_t);
	C(MARKSURFACES, leaffaces, numLeaffaces, unsigned short);
	C(SURFEDGES, surfedges, numSurfedges, int);
	C(EDGES, edges, numEdges, dEdge_t);

	bspfile.miptex1 = (dBsp1MiptexLump_t*)(bspfile.file + lumps[Q1LUMP_TEXTURES].fileofs);
#if !LITTLE_ENDIAN
	//!! swap miptex1 lump
#endif

	dBsp1Model_t *models;
	bspfile.numModels = CheckLump (Q1LUMP_MODELS, (void**)&models, sizeof(dBsp1Model_t));	// not in bspfile_t struc
	LoadQ1Submodels (&bspfile, models);

#if !LITTLE_ENDIAN
	// swap everything
	SwapQ1BspFile (&bspfile);
#endif
	ProcessQ1BspFile (&bspfile);

	// load visinfo
	byte *vis;
	int visDataSize = CheckLump (Q1LUMP_VISIBILITY, (void**)&vis, 1);
	LoadQ1Vis (&bspfile, vis, visDataSize);
	// load entstring after all: we may require to change something
	char *entString = (char*)header + header->lumps[Q2LUMP_ENTITIES].fileofs;
	bspfile.entStr = ProcessEntstring (entString);
	bspfile.entStrSize = strlen (bspfile.entStr);

#undef C
	unguard;
}


bspfile_t *LoadBspFile (const char *filename, bool clientload, unsigned *checksum)
{
	guard(LoadBspFile);

	if (!stricmp (filename, bspfile.name) && (clientload || !Cvar_VariableInt ("flushmap")))
	{
		if (checksum)
			*checksum = bspfile.checksum;
		map_clientLoaded |= clientload;
		return &bspfile;
	}

	server_state_t ss = Com_ServerState ();
	if (clientload && (ss == ss_loading || ss == ss_game) && stricmp (filename, bspfile.name) && bspfile.name[0])
		Com_DropError ("client trying to load map \"%s\" while server running \"%s\"", filename, bspfile.name);

	if (bspfile.name[0] && bspfile.file)
		delete bspfile.file;
	if (bspfile.extraChain)
		delete bspfile.extraChain;

	memset (&bspfile, 0, sizeof(bspfile));
	strcpy (bspfile.name, filename);
	if (!(bspfile.file = (byte*) GFileSystem->LoadFile (filename, &bspfile.length)))
	{
		bspfile.name[0] = 0;
		Com_DropError ("Couldn't load %s", filename);
	}
	bspfile.checksum = LittleLong (Com_BlockChecksum (bspfile.file, bspfile.length));
	if (checksum) *checksum = bspfile.checksum;
	bspfile.extraChain = new CMemoryChain;

	map_clientLoaded = clientload;
	switch (LittleLong(*(unsigned *)bspfile.file))
	{
		case BSP2_IDENT:
			LoadQ2BspFile ();
			return &bspfile;
		case BSP1_VERSION:
		case BSPHL_VERSION:
			LoadQ1BspFile ();
			return &bspfile;
	}
	// error
	delete bspfile.file;
	bspfile.name[0]  = 0;
	bspfile.file     = NULL;
	map_clientLoaded = false;
	Com_DropError ("%s has a wrong BSP header", filename);
	return NULL;		// make compiler happy

	unguard;
}
