#include "qcommon.h"
#include "cmodel.h"


bspfile_t bspfile;
static lump_t *lumps;


//?? move swappers to FileFormats.h as methods for structures
#if !LITTLE_ENDIAN
static void SwapQ2BspFile(bspfile_t *f)
{
	int		i, j;

	//!! ERROR HERE: should swap models before loading as CBspModel !!
	// models
	for (i = 0; i < f->numModels; i++)
	{
		//!! swap dBsp2Model_t !!
		CBspModel &d = f->models[i];
		LTL(d.firstface);
		LTL(d.numfaces);
		LTL(d.headnode);

		LTL(d.bounds.mins);
		LTL(d.bounds.maxs);
//		LTL(d.origin);
	}

	// vertexes
	for (i = 0; i < f->numVertexes; i++)
		LTL(f->vertexes2[i]);

	// planes
	for (i = 0; i < f->numPlanes; i++)
	{
		LTL(f->planes2[i].normal);
		LTL(f->planes2[i].dist);
	}

	// texinfos
	for (i = 0; i < f->numTexinfo; i++)
	{
		dBsp2Texinfo_t &d = f->texinfo2[i];
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
		dBspFace_t &face = f->faces2[i];
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
		LTL(node.planeNum);
		for (j = 0; j < 3; j++)
		{
			LTL(node.mins[j]);
			LTL(node.maxs[j]);
		}
		LTL(node.children[0]);
		LTL(node.children[1]);
//		LTL(node.firstFace); -- unused
//		LTL(node.numFaces);
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
		LTL(f->leaffaces2[i]);

	// leafbrushes
	for (i = 0; i < f->numLeafbrushes; i++)
		LTL(f->leafbrushes2[i]);

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
		LTL(f->brushes2[i].firstside);
		LTL(f->brushes2[i].numsides);
		LTL(f->brushes2[i].contents);
	}

	// areas
	for (i = 0; i < f->numAreas; i++)
	{
		LTL(f->areas[i].numAreaPortals);
		LTL(f->areas[i].firstAreaPortal);
	}

	// areasportals
	for (i = 0; i < f->numAreaportals; i++)
	{
		LTL(f->areaportals[i].portalNum);
		LTL(f->areaportals[i].otherArea);
	}

	// brushsides
	for (i = 0; i < f->numBrushSides; i++)
	{
		LTL(f->brushsides2[i].planenum);
		LTL(f->brushsides2[i].texinfo);
	}
}

static void SwapQ1BspFile(bspfile_t *f)
{
	//!! implement this
	assert(0);
}

static void SwapQ3BspFile(bspfile_t *f)
{
	//!! implement this
	// NOTE: most work may be done with SwapDwordBlock() function (except shaders/fogs: have name[] fields) -
	//		 there is no byte/short fields, only int32/float
	assert(0);
}

#endif // LITTLE_ENDIAN


static void ProcessQ2BspFile(bspfile_t *f)
{
	int i, j;

	// models: inflate mins/maxs by 1
	//?? this is to avoid precision errors ?
	for (i = 0; i < f->numModels; i++)
	{
		CBspModel &d = f->models[i];

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
			*s = toLower(*s);
		s = d.texture;
		if (s[0] == '.') s++;
		if (s[0] == '/') s++;
		if (s != d.texture)
			strcpy(d.texture, s);
	}

	// silly check for correct lightmaps
	for (i = 0; i < f->numFaces; i++)
		if (f->faces2[i].lightofs > f->lightDataSize)
			f->faces2[i].lightofs = -1;

	// get numClusters (have in dBsp2Vis_t, but Q2 recomputes this ...)
	f->numClusters = 0;
	for (i = 0; i < f->numLeafs; i++)
		if (f->leafs2[i].cluster >= f->numClusters)
			f->numClusters = f->leafs2[i].cluster + 1;
}

static void ProcessQ1BspFile(bspfile_t *f)
{
	int i, j;

	// models: inflate mins/maxs by 1
	//?? this is to avoid precision errors ?
	for (i = 0; i < f->numModels; i++)
	{
		CBspModel &d = f->models[i];

		for (j = 0; j < 3; j++)
		{
			d.bounds.mins[j] -= 1;
			d.bounds.maxs[j] += 1;
		}
	}
	for (i = 0; i < f->numFaces; i++)
	{
		dBspFace_t &face = f->faces2[i];
		// silly check for correct lightmaps
		if (face.lightofs > f->lightDataSize)
			face.lightofs = -1;
	}
}

static void ProcessQ3BspFile(bspfile_t *f)
{
	int i;
	// get numClusters (have in dBsp3Vis_t, but Q3 recomputes this ...)
	f->numClusters = 0;
	for (i = 0; i < f->numLeafs; i++)
		if (f->leafs3[i].cluster >= f->numClusters)
			f->numClusters = f->leafs3[i].cluster + 1;
}


//?? should move LoadQxSubmodels() to cmodel.cpp, but this work will be used for entstring processing
static void LoadQ2Submodels(bspfile_t *f, dBsp2Model_t *data)
{
	if (f->numModels < 1)
		Com_DropError("Map with no models");

	CBspModel *out = f->models = new (f->extraChain) CBspModel[f->numModels];
	for (int i = 0; i < f->numModels; i++, data++, out++)
	{
		out->bounds     = data->bounds;
		out->radius     = VectorDistance(out->bounds.mins, out->bounds.maxs) / 2;
		out->headnode   = data->headnode;
		out->flags      = 0;
		out->firstFace  = data->firstface;
		out->numFaces   = data->numfaces;
		out->color.rgba = RGBA(1,1,1,1);
		// dBsp2Model_t have unused field "origin"
	}
}

static void LoadQ1Submodels(bspfile_t *f, dBsp1Model_t *data)
{
	if (f->numModels < 1)
		Com_DropError("Map with no models");

	CBspModel *out = f->models = new (f->extraChain) CBspModel[f->numModels];
	for (int i = 0; i < f->numModels; i++, data++, out++)
	{
		out->bounds     = data->bounds;
		out->radius     = VectorDistance(out->bounds.mins, out->bounds.maxs) / 2;
		out->headnode   = data->headnode[0];
		out->flags      = 0;
		out->firstFace  = data->firstface;
		out->numFaces   = data->numfaces;
		out->color.rgba = RGBA(1,1,1,1);
		// dBsp1Model_t have unused field "origin"
	}
}

static void LoadQ3Submodels(bspfile_t *f, dBsp3Model_t *data)
{
	//!! implement
}


static void DecompressVis(byte *dst, void *vis, int pos, int rowSize)
{
	if (pos == -1)
	{
		memset(dst, 0xFF, rowSize);	// all visible
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

static void LoadQ2Vis(bspfile_t *f, dBsp2Vis_t *vis, int size)
{
#if !LITTLE_ENDIAN
	if (size)		// should process this only when map have visibility data
	{
		LTL(vis->numClusters);
		int j = vis->numClusters;
		for (int i = 0; i < j ; i++)
		{
			LTL(vis->bitOfs[i][0]);
			LTL(vis->bitOfs[i][1]);
		}
	}
#endif

	if (!size)
	{
		Com_DPrintf("No visinfo in map\n");
		if (f->numClusters > 1)
		{
			if (developer->integer)
				appWPrintf("WARNING: map with cluster info but without visinfo\n");
		}
		f->numClusters = 1;				// required
		f->visRowSize  = 1;				// required
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
		DecompressVis(dst, vis, vis->bitOfs[i][dBsp2Vis_t::PVS], rowSize);
	Com_DPrintf("Decompressed vis: %d -> %d bytes\n", size, rowSize * f->numClusters);
}

static void LoadQ1Vis(bspfile_t *f, byte *vis, int size)
{
	if (!size)
	{
		Com_DPrintf("No visinfo in map\n");
		f->numClusters = 1;				// required
		f->visRowSize  = 1;				// required
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
		DecompressVis(dst, vis, f->leafs1[i].visofs, rowSize);
	Com_DPrintf("Decompressed vis: %d -> %d bytes\n", size, rowSize * f->numClusters);
}

static void LoadQ3Vis(bspfile_t *f, dBsp3Vis_t* vis, int size)
{
	if (!size)
	{
		Com_DPrintf("No visinfo in map\n");
		if (f->numClusters > 1)
		{
			if (developer->integer)
				appWPrintf("WARNING: map with cluster info but without visinfo\n");
		}
		f->numClusters = 1;				// required
		f->visRowSize  = 1;				// required
		f->visInfo     = NULL;
		return;
	}
	// NOTE: 'size' is ignored later

	int rowSize = (f->numClusters + 7) >> 3;
	if (rowSize != vis->rowSize) Com_DPrintf("LoadQ3Vis: vis.rowSize=%d != computed=%d\n", vis->rowSize, rowSize);

	f->visInfo = (byte*)vis + sizeof(dBsp3Vis_t);
	f->visRowSize = vis->rowSize;
}


static int CheckLump(int lump, void **ptr, int size)
{
	int length = lumps[lump].filelen;
	int ofs    = lumps[lump].fileofs;

	if (length % size)
		Com_DropError("LoadBSPFile: incorrect lump size");

	*ptr = bspfile.file + ofs;

	return length / size;
}


void LoadQ2BspFile()
{
	guard(LoadQ2BspFile);

	dBsp2Hdr_t *header = (dBsp2Hdr_t *) bspfile.file;
	lumps = header->lumps;

#if !LITTLE_ENDIAN
	// swap the header
	for (int i = 0; i < sizeof(dBsp2Hdr_t) / 4; i++)
		((int *)bspfile.file)[i] = LittleLong(((int *)bspfile.file)[i]);
#endif

	bspfile.type = map_q2;
	Com_DPrintf("Loading Q2 bsp %s\n", *bspfile.Name);

#define C(num,field,count,type) \
	bspfile.count = CheckLump(dBsp2Hdr_t::LUMP_##num, (void**)&bspfile.field, sizeof(type))
	C(LIGHTING, lighting, lightDataSize, byte);
	C(VERTEXES, vertexes2, numVertexes, CVec3);
	C(PLANES, planes2, numPlanes, dBsp2Plane_t);
	C(LEAFS, leafs2, numLeafs, dBsp2Leaf_t);
	C(NODES, nodes2, numNodes, dBsp2Node_t);
	C(TEXINFO, texinfo2, numTexinfo, dBsp2Texinfo_t);
	C(FACES, faces2, numFaces, dBspFace_t);
	C(LEAFFACES, leaffaces2, numLeaffaces, unsigned short);
	C(LEAFBRUSHES, leafbrushes2, numLeafbrushes, unsigned short);
	C(SURFEDGES, surfedges, numSurfedges, int);
	C(EDGES, edges, numEdges, dEdge_t);
	C(BRUSHES, brushes2, numBrushes, dBsp2Brush_t);
	C(BRUSHSIDES, brushsides2, numBrushSides, dBsp2Brushside_t);
	C(AREAS, areas, numAreas, dArea_t);
	C(AREAPORTALS, areaportals, numAreaportals, dAreaPortal_t);

	dBsp2Model_t *models;
	bspfile.numModels = CheckLump(dBsp2Hdr_t::LUMP_MODELS, (void**)&models, sizeof(dBsp2Model_t));	// not in bspfile_t struc
	LoadQ2Submodels(&bspfile, models);

#if !LITTLE_ENDIAN
	// swap everything
	SwapQ2BspFile(&bspfile);
#endif
	ProcessQ2BspFile(&bspfile);

	// load visinfo
	dBsp2Vis_t *vis;
	int visDataSize = CheckLump(dBsp2Hdr_t::LUMP_VISIBILITY, (void**)&vis, 1);
	LoadQ2Vis(&bspfile, vis, visDataSize);
	// load entstring after all: we may require to change something
	char *entString = (char*)header + header->lumps[dBsp2Hdr_t::LUMP_ENTITIES].fileofs;
	// detect kingpin map entities
	if (strstr(entString, "\"classname\" \"junior\"") ||
		strstr(entString, "\"classname\" \"lightflare\"") ||
		strstr(entString, "\"fogdensity2\""))
	{
		bspfile.type = map_kp;
		Com_DPrintf("Kingpin map detected\n");
	}
	bspfile.entStr = ProcessEntstring(entString);

#undef C
	unguard;
}


void LoadQ1BspFile()
{
	guard(LoadQ1BspFile);

	dBsp1Hdr_t *header = (dBsp1Hdr_t *) bspfile.file;
	lumps = header->lumps;

#if !LITTLE_ENDIAN
	// swap the header
	for (int i = 0; i < sizeof(dBsp1Hdr_t) / 4; i++)
		((int *)bspfile.file)[i] = LittleLong(((int *)bspfile.file)[i]);
#endif

	if (header->version == BSP1_VERSION)
		bspfile.type = map_q1;
	else
		bspfile.type = map_hl;
	Com_DPrintf("Loading %s bsp %s\n", bspfile.type == map_q1 ? "Q1" : "HL", *bspfile.Name);

#define C(num,field,count,type) \
	bspfile.count = CheckLump(dBsp1Hdr_t::LUMP_##num, (void**)&bspfile.field, sizeof(type))
	C(LIGHTING, lighting, lightDataSize, byte);
	C(VERTEXES, vertexes2, numVertexes, CVec3);
	C(PLANES, planes2, numPlanes, dBsp2Plane_t);
	C(LEAFS, leafs1, numLeafs, dBsp1Leaf_t);
	C(NODES, nodes1, numNodes, dBsp1Node_t);
	C(TEXINFO, texinfo1, numTexinfo, dBsp1Texinfo_t);
	C(FACES, faces2, numFaces, dBspFace_t);
	C(MARKSURFACES, leaffaces2, numLeaffaces, unsigned short);
	C(SURFEDGES, surfedges, numSurfedges, int);
	C(EDGES, edges, numEdges, dEdge_t);

	// load miptex lump
	bspfile.miptex1 = (dBsp1MiptexLump_t*)(bspfile.file + lumps[dBsp1Hdr_t::LUMP_TEXTURES].fileofs);
#if !LITTLE_ENDIAN
	//!! swap miptex1 lump
#endif
	for (int miptex = 0; miptex < bspfile.miptex1->nummiptex; miptex++)
	{
		int offset = bspfile.miptex1->dataofs[miptex];
		if (offset == -1)
			continue;
		dBsp1Miptex_t *tex = (dBsp1Miptex_t*)( (byte*)bspfile.miptex1 + offset );
		// texture with data, but without name -- create default name
		if (!tex->name[0])
			appSprintf(ARRAY_ARG(tex->name), "unnamed#%d", miptex);
	}

	dBsp1Model_t *models;
	bspfile.numModels = CheckLump(dBsp1Hdr_t::LUMP_MODELS, (void**)&models, sizeof(dBsp1Model_t));	// not in bspfile_t struc
	LoadQ1Submodels(&bspfile, models);

#if !LITTLE_ENDIAN
	// swap everything
	SwapQ1BspFile(&bspfile);
#endif
	ProcessQ1BspFile(&bspfile);

	// load visinfo
	byte *vis;
	int visDataSize = CheckLump(dBsp1Hdr_t::LUMP_VISIBILITY, (void**)&vis, 1);
	LoadQ1Vis(&bspfile, vis, visDataSize);
	// load entstring after all: we may require to change something
	char *entString = (char*)header + header->lumps[dBsp1Hdr_t::LUMP_ENTITIES].fileofs;
	bspfile.entStr = ProcessEntstring(entString);

#undef C
	unguard;
}


void LoadQ3BspFile()
{
	guard(LoadQ3BspFile);

	dBsp3Hdr_t *header = (dBsp3Hdr_t *) bspfile.file;
	lumps = header->lumps;

#if !LITTLE_ENDIAN
	// swap the header
	for (int i = 0; i < sizeof(dBsp3Hdr_t) / 4; i++)
		((int *)bspfile.file)[i] = LittleLong(((int *)bspfile.file)[i]);
#endif

	bspfile.type = map_q3;
	Com_DPrintf("Loading Q3 bsp %s\n", *bspfile.Name);

#define C(num,field,count,type) \
	bspfile.count = CheckLump(dBsp3Hdr_t::LUMP_##num, (void**)&bspfile.field, sizeof(type))
//	C(LIGHTING, lighting, lightDataSize, byte);
	C(VERTEXES, vertexes3, numVertexes, dBsp3Vert_t);
	C(INDEXES, indexes3, numIndexes, int);
	C(PLANES, planes3, numPlanes, dBsp3Plane_t);
	C(LEAFS, leafs3, numLeafs, dBsp3Leaf_t);
	C(NODES, nodes3, numNodes, dBsp3Node_t);
	C(SHADERS, texinfo3, numTexinfo, dBsp3Shader_t);
	C(FACES, faces3, numFaces, dBsp3Face_t);
	C(LEAFFACES, leaffaces3, numLeaffaces, int);
	C(LEAFBRUSHES, leafbrushes3, numLeafbrushes, int);
	C(BRUSHES, brushes3, numBrushes, dBsp2Brush_t);
	C(BRUSHSIDES, brushsides3, numBrushSides, dBsp2Brushside_t);
	//!! C(FOGS, ...);

	dBsp3Model_t *models;
	bspfile.numModels = CheckLump(dBsp3Hdr_t::LUMP_MODELS, (void**)&models, sizeof(dBsp3Model_t));	// not in bspfile_t struc
	LoadQ3Submodels(&bspfile, models);		//!!! empty

#if !LITTLE_ENDIAN
	// swap everything
	SwapQ3BspFile(&bspfile);
#endif
	ProcessQ3BspFile(&bspfile);

	// load visinfo
	dBsp3Vis_t *vis;
	int visDataSize = CheckLump(dBsp3Hdr_t::LUMP_VISIBILITY, (void**)&vis, 1);
	LoadQ3Vis(&bspfile, vis, visDataSize);
	// load entstring after all: we may require to change something
	char *entString = (char*)header + header->lumps[dBsp3Hdr_t::LUMP_ENTITIES].fileofs;
	bspfile.entStr = ProcessEntstring(entString);

#undef C
	unguard;
}


bspfile_t *LoadBspFile(const char *filename, bool clientload, unsigned *checksum)
{
	guard(LoadBspFile);

	if (!stricmp(filename, bspfile.Name) && (clientload || !Cvar_VariableInt("flushmap")))
	{
		if (checksum)
			*checksum = bspfile.checksum;
		bspfile.clientLoaded |= clientload;
		return &bspfile;
	}

	server_state_t ss = Com_ServerState();
	if (clientload && (ss == ss_loading || ss == ss_game) && stricmp(filename, bspfile.Name) && bspfile.Name[0])
		Com_DropError("client trying to load map \"%s\" while server running \"%s\"", filename, *bspfile.Name);

	if (bspfile.Name[0] && bspfile.file)
		delete bspfile.file;
	if (bspfile.extraChain)
		delete bspfile.extraChain;

	memset(&bspfile, 0, sizeof(bspfile));
	strcpy(bspfile.Name, filename);
	if (!(bspfile.file = (byte*) GFileSystem->LoadFile(filename, &bspfile.length)))
	{
		bspfile.Name[0] = 0;
		Com_DropError("Couldn't load %s", filename);
	}
	bspfile.checksum = LittleLong(Com_BlockChecksum(bspfile.file, bspfile.length));
	if (checksum) *checksum = bspfile.checksum;
	bspfile.extraChain = new CMemoryChain;

	bspfile.clientLoaded = clientload;
	unsigned *h = (unsigned*)bspfile.file;
	unsigned id1 = LittleLong(h[0]);
	unsigned id2 = LittleLong(h[1]);
	switch (id1)
	{
	case BSP_IDENT:
		switch (id2)
		{
		case BSP2_VERSION:
			LoadQ2BspFile();
			return &bspfile;
		case BSP3_VERSION:
			LoadQ3BspFile();
			return &bspfile;
		default:
			appPrintf(S_RED"Unknown bsp version %d\n", id2);
		};
		break;
	case BSP1_VERSION:
	case BSPHL_VERSION:
		LoadQ1BspFile();
		return &bspfile;
	}
	// error
	delete bspfile.file;
	memset(&bspfile, 0, sizeof(bspfile));
	Com_DropError("%s has a wrong BSP header", filename);
	return NULL;		// make compiler happy

	unguard;
}
