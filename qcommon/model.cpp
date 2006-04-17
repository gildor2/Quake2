#include "qcommon.h"
#include "cmodel.h"

//#undef LITTLE_ENDIAN		// test Swap() functions

//!! move to common headers
inline void LTL(short &v)
{
	v = LittleShort (v);
}
inline void LTL(unsigned short &v)
{
	v = LittleShort (v);
}
inline void LTL(int &v)
{
	v = LittleLong (v);
}
inline void LTL(unsigned &v)
{
	v = LittleLong (v);
}
inline void LTL(float &v)
{
	v = LittleFloat (v);
}
inline void LTL(CVec3 &v)
{
	for (int i = 0; i < 3; i++)
		v[i] = LittleFloat (v[i]);
}

static bspfile_t bspfile;
static lump_t *lumps;

// forwards
static const char *ProcessEntstring (const char *entString);


//?? move swappers to qfiles.h as methods for structures
#if !LITTLE_ENDIAN
static void SwapQ2BspFile (bspfile_t *f)
{
	int		i, j;

	//!! ERROR HERE: should swap models before loading as cmodel_t!!
	// models
	for (i = 0; i < f->numModels; i++)
	{
		cmodel_t *d = &f->models[i];
		LTL(d->firstface);
		LTL(d->numfaces);
		LTL(d->headnode);

		LTL(d->bounds.mins);
		LTL(d->bounds.maxs);
//		LTL(d->origin);
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
		dBsp2Texinfo_t *d = &f->texinfo[i];
		float *vec = &d->vecs[0].vec[0];	// hack to get all ([3]+1)[2] == 8 components of dBsp2Texinfo_t.vecs[]
		for (j = 0; j < 8; j++)
			LTL(vec[j]);
		LTL(d->flags);
		LTL(d->value);
		LTL(d->nexttexinfo);
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
		cmodel_t *d = &f->models[i];

		for (j = 0; j < 3; j++)
		{
			d->bounds.mins[j] -= 1;
			d->bounds.maxs[j] += 1;
		}
	}

	// texinfo: lowercase all filenames and remove leading "." and "/"
	for (i = 0; i < f->numTexinfo; i++)
	{
		dBsp2Texinfo_t *d = &f->texinfo2[i];
		char *s = d->texture;
		for (j = 0; j < sizeof(d->texture); j++, s++)
			*s = toLower (*s);
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
		cmodel_t *d = &f->models[i];

		for (j = 0; j < 3; j++)
		{
			d->bounds.mins[j] -= 1;
			d->bounds.maxs[j] += 1;
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
		//?? dBsp1Model_t have unused field "origin"
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
		if (f->numClusters > 1)
		{
			if (developer->integer)
				appWPrintf ("WARNING: map with cluster info but without visinfo\n");
		}
		f->numClusters = 0;
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
		f->numClusters = 0;
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
#if 0
	bspfile.entDataSize = C(ENTITIES, entities, char);
#else
	bspfile.entStr = ProcessEntstring ((char*)header + header->lumps[Q2LUMP_ENTITIES].fileofs);
	bspfile.entStrSize = strlen (bspfile.entStr);
#endif

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
#if 0
	bspfile.entDataSize = C(ENTITIES, entities, char);
#else
	bspfile.entStr = ProcessEntstring ((char*)header + header->lumps[Q1LUMP_ENTITIES].fileofs);
	bspfile.entStrSize = strlen (bspfile.entStr);
#endif

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


/*--------------------------------------------------------------------*/

#define MAX_ENT_FIELDS	32
#define MAX_TARGETS		256

struct entField_t
{
	char	name[64];
	char	value[192];
};

struct target_t
{
	char	name[64];
	CVec3	origin;
};


static bool haveErrors;
static entField_t entity[MAX_ENT_FIELDS];
static int numEntFields;

static target_t targets[MAX_TARGETS];
static int numTargets;

static char sunTarget[64];


static entField_t *FindField (const char *name)
{
	int		i;
	entField_t *f;

	for (i = 0, f = entity; i < numEntFields; i++, f++)
		if (appMatchWildcard (f->name, name))		// case-sensitive
			return f;
	return NULL;
}


static void RemoveField (const char *name)
{
	if (entField_t *field = FindField (name))
		field->name[0] = 0;
}


static void AddField (const char *name, const char *value)
{
	if (numEntFields >= MAX_ENT_FIELDS) return;

	strcpy (entity[numEntFields].name, name);
	strcpy (entity[numEntFields].value, value);
	numEntFields++;
}


static void ErrMsg (const char *str)
{
	haveErrors = true;
	appWPrintf ("EntString error: %s\n", str);
}


static bool ReadEntity (const char *&src)
{
	numEntFields = 0;
	if (!src) return false;

	const char *tok = COM_Parse (src);
	if (!tok[0]) return false;

	if (tok[0] != '{' || tok[1] != 0)
	{
		ErrMsg ("expected \"{\"");
		return false;
	}

	entField_t *field = entity;
	while (true)
	{
		tok = COM_Parse (src);
		if (tok[0] == '}' && tok[1] == 0)
			break;

		// add field name
		strcpy (field->name, tok);

		// add field value
		tok = COM_Parse (src);
		if (!tok)
		{
			ErrMsg ("unexpected end of data");
			return false;
		}
		strcpy (field->value, tok);

		if (numEntFields++ == MAX_ENT_FIELDS)
		{
			ErrMsg ("MAX_ENT_FIELDS");
			return false;
		}
		field++;
	}

	return true;
}


//#define SHOW_WRITE		1

// debug helper
#define DUMP_ENTITY	\
			{		\
				for (int i = 0; i < numEntFields; i++)	\
					DebugPrintf("-- %s : %s\n", entity[i].name, entity[i].value);	\
			}


static void WriteEntity (char **dst)
{
	int		i;
#if SHOW_WRITE
	char	*txt = *dst;
#endif

/* -- disabled: will remove { "classname" "worldspawn" }  (empty worldspawn, map "campblud")
	bool found;
	// check presence of any non-removed field
	found = false;
	for (i = 0; i < numEntFields; i++)
		if (entity[i].name[0] && strcmp (entity[i].name, "classname"))
		{
			found = true;
			break;
		}
	if (!found) { DUMP_ENTITY return; }
*/

	strcpy (*dst, "{\n"); (*dst) += 2;
	for (i = 0; i < numEntFields; i++)
		if (entity[i].name[0])	// may be removed field
			(*dst) += appSprintf (*dst, 1024, "\"%s\" \"%s\"\n", entity[i].name, entity[i].value);
	strcpy (*dst, "}\n"); (*dst) += 2;
#if SHOW_WRITE
	appPrintf (S_CYAN"%s", txt);
#endif
}

// gets "%f %f %f" or "%f"
static void GetVector (const char *str, CVec3 &vec)
{
	if (sscanf (str, "%f %f %f", VECTOR_ARG(&vec)) != 3)
		vec[1] = vec[2] = vec[0];
}


static void ProcessEntityTarget ()
{
	if (entField_t *f = FindField ("targetname"))
	{
		if (numTargets == MAX_TARGETS)
		{
			haveErrors = true;
			Com_DPrintf ("MAX_TARGETS reached\n");
			return;
		}
		strcpy (targets[numTargets].name, f->value);
		if (f = FindField ("origin"))	// target can be without origin, but we are not interested with it
		{
			GetVector (f->value, targets[numTargets].origin);
			numTargets++;
		}
	}
}


static const CVec3& FindEntityTarget (const char *name)
{
	for (int i = 0; i < numTargets; i++)
		if (!strcmp (targets[i].name, name))
			return targets[i].origin;

	Com_DPrintf ("target \"%s\" is not found\n", name);
	return nullVec3;		// not found
}


// Returns "true" if entity should be passed to game
static bool ProcessEntity ()
{
	entField_t *f;

	/*------------------ get some fields -------------------*/

	// get classname
	const char *classname = "";
	entField_t *classNameField = FindField ("classname");
	if (classNameField)
		classname = classNameField->value;
	// get spawnflags
	int spawnflags = 0;
	entField_t *spawnflagsField = FindField ("spawnflags");
	if (spawnflagsField)
		spawnflags = atoi (spawnflagsField->value);
#if 0
#define SPAWNFLAG_NOT_DEATHMATCH	0x800
	if (Cvar_VariableInt("keep_sp") && (spawnflags & SPAWNFLAG_NOT_DEATHMATCH))//!!
	{
		spawnflags &= ~SPAWNFLAG_NOT_DEATHMATCH;
		appSprintf (f->value, sizeof(f->value), "%d", spawnflags);
	}
#endif
	// get origin
	CVec3 origin;
	entField_t *originField = FindField ("origin");
	if (originField)
		GetVector (originField->value, origin);
	// get inline model
	cmodel_t *model = NULL;
	int modelIdx;
	if (f = FindField ("model"))
	{
		sscanf (f->value, "*%d", &modelIdx);
		if (modelIdx >= bspfile.numModels)
			Com_DPrintf ("invalid model index %d\n", modelIdx);
		else
			model = &bspfile.models[modelIdx];
	}

	/*----------- check movable inline models -----------*/

	if (model && appMatchWildcard (classname, "func_plat,func_*rotating,func_door,func_train"))
		model->flags |= CMODEL_MOVABLE;

	/*---------------- get lighting info ----------------*/

	if (!memcmp (classname, "light", 5))
	{
		// "style": if >= 32 ... (def: 0)
		// "spawnflags": &2 -> flare, &4 -> resize, {&8 -> dynamic ?}
		// get common lighting fields
		int style = 0;
		if (f = FindField ("style,_style"))
			style = atoi (f->value);

		/*--------------- lightflares -----------------*/

		if ((!classname[5] || !strcmp (classname + 5, "flare")) && (spawnflags & 2))	// "light" or "lightflare"
		{
			// "health" -> size (def: 24)
			// "dmg": normal (0, def), sun, amber, red, blue, green
			// if "dmg" == SUN -> origin.Normalize()

			lightFlare_t *flare = new (bspfile.extraChain) lightFlare_t;
			flare->next = bspfile.flares;
			bspfile.flares = flare;
			bspfile.numFlares++;

			if (f = FindField ("health"))
				flare->size = atof (f->value);
			else
				flare->size = 24;							// default size
			flare->style = style;

			flare->color.rgba = RGB(1,1,1);
			if (f = FindField ("dmg"))
			{
				switch (atoi (f->value))
				{
				case 1:
					if (bspfile.type == map_kp) DebugPrintf ("HAVE SUN FLARE: %s\n", bspfile.name);//!!
					flare->radius = -1;						// mark as "sun"
					flare->color.c[2] = 192;
					origin.NormalizeFast ();				// just a sun direction
					break;
				case 2:		// amber
					flare->color.rgba = RGB255(255, 192, 0);
					break;
				case 3:		// red
					flare->color.rgba = RGB255(255, 0, 64);
					break;
				case 4:		// blue
					flare->color.rgba = RGB255(0, 192, 255);
					break;
				case 5:		// green
					flare->color.rgba = RGB255(0, 255, 128);
					break;
				}
			}
			flare->origin = origin;
			if (f = FindField ("radius"))
				flare->radius = atof (f->value);
			if (f = FindField ("color"))					// can override sun color ...
			{
				// NOTE: cannot scanf() into byte-sized vars
				int	v[3];
				sscanf (f->value, "%d %d %d", VECTOR_ARG(&v));
				flare->color.rgba = RGB255(v[0],v[1],v[2]);
			}
			if (model)
				flare->model = modelIdx;

			if (classname[5]) return false;					// flares passed to renderer only (if classname = "light" -> flare+light)
		}

		/*-------------- light, light_mine -------------*/

		slight_t *slight = new (bspfile.extraChain) slight_t;
		slight->next = bspfile.slights;
		bspfile.slights = slight;
		bspfile.numSlights++;

		slight->style = style;

		if (f = FindField ("light,_light"))
			slight->intens = atof (f->value);
		else
			slight->intens = 300;						// default

		if (f = FindField ("color,_color"))
		{
			GetVector (f->value, slight->color);
			NormalizeColor (slight->color, slight->color);
		}
		else
			slight->color.Set (1, 1, 1);				// default

		if (f = FindField ("_falloff"))					// 0 - linear (default), 1 - 1/dist. 2 - 1/(dist*dist)
			slight->type = (slightType_t)atoi (f->value);
		if (f = FindField ("_wait,_fade"))
			slight->fade = atof (f->value);
		if (!slight->fade) slight->fade = 1;			// default value + disallow "fade==0"

		slight->origin = origin;

		if (f = FindField ("target"))
		{
			slight->spot = true;
			const CVec3 &dst = FindEntityTarget (f->value);
			CVec3	vec;
			VectorSubtract (dst, slight->origin, vec);
			VectorNormalize (vec, slight->spotDir);

			if (!strcmp (f->value, sunTarget))
			{
				// this spotlight used as sun target
				bspfile.sunVec = slight->spotDir;
				// remove from list
				bspfile.slights = slight->next;
				bspfile.numSlights--;
//appPrintf(S_RED"sun: %g %g %g (dst: %g %g %g, org: %g %g %g)\n",VECTOR_ARG(bspfile.sunVec),VECTOR_ARG(dst),VECTOR_ARG(slight->spotDir));
			}
		}
		if (!strcmp (classname + 5, "_spot") || FindField ("_spotvector,_spotpoint,_mangle,_spotangle"))
		{
			CVec3	vec;

			slight->spot = true;
			if (f = FindField ("_spotvector"))
				GetVector (f->value, slight->spotDir);
			else if (f = FindField ("_spotpoint"))
			{
				GetVector (f->value, vec);
				VectorSubtract (vec, slight->origin, vec);
				VectorNormalize (vec, slight->spotDir);
			}
			else if (f = FindField ("_mangle,_spotangle"))
			{
				CVec3	ang;
				sscanf (f->value, "%f %f", &ang[YAW], &ang[PITCH]);
				ang[PITCH] *= -1;
				AngleVectors (ang, &slight->spotDir, NULL, NULL);
			}
			else if (f = FindField ("angle"))
			{
				float angle = atof (f->value);
				if (angle == -1)
					slight->spotDir.Set (0, 0, 1);
				else if (angle == -2)
					slight->spotDir.Set (0, 0, -1);
				else
				{
					angle *= M_PI / 180.0f;
					slight->spotDir.Set (cos(angle), sin(angle), 0);
				}
			}
			else
				Com_DPrintf ("spotlight without direction\n");
		}
		if (slight->spot)
		{
			float	cone;

			if (f = FindField ("_cone"))
				cone = atof (f->value);
			else
				cone = 10;		// default
			slight->spotDot = cos(cone * M_PI / 180.0f);
			if (f = FindField ("_focus"))
				slight->focus = atof (f->value);
			else
				slight->focus = 1;
		}

		if ((!classname[5] && style < 32) || !strcmp (classname + 5, "flare"))
			return false;		// remove: "lightflare" or non-switchable "light"

		return true;
	}

	// our map script (temp !!)
	if (!strcmp (classname, "surfparam"))
	{
		if (f = FindField ("name"))
		{
			const char *name = f->value;
			if (f = FindField ("flags"))
			{
				int flags = atoi (f->value);
				int testflags, testmask;
				if (f = FindField ("inflags"))
					sscanf (f->value, "%d %d", &testmask, &testflags);
				else
					testflags = testmask = 0;
				bool found = false;
				int i;
				dBsp2Texinfo_t *d;
				for (i = 0, d = bspfile.texinfo2; i < bspfile.numTexinfo; i++, d++)
					if (appMatchWildcard (d->texture, name, true) && ((d->flags & testmask) == testflags))
					{
						d->flags = d->flags & ~testmask | flags;
						found = true;
					}
				if (!found)
					Com_DPrintf ("texinfo %s is not found\n", name);
			}
			else
				Com_DPrintf ("no flags specified for %s\n", name);
		}
		return false;
	}

	/*---------------------------------------------*/

	if (!strcmp (classname, "worldspawn"))
	{
		if (bspfile.type == map_kp && !FindField ("sky"))
			AddField ("sky", "sr");		// set default Kingpin sky
		if (bspfile.type == map_hl && (f = FindField ("skyname")))
			strcpy (f->name, "sky");
		if (f = FindField ("fogval"))
		{
			GetVector (f->value, bspfile.fogColor);
			f->name[0] = 0;				// remove field
			if (f = FindField ("fogdensity"))
			{
				bspfile.fogDens = atof (f->value);
				f->name[0] = 0;			// remove field
				bspfile.fogMode = fog_exp;
			}
		}
//		if (f = FindField ("_ambient,light,_minlight")) -- it seems, this is not always true ... detect ambient by outselves (in renderer)
//			GetVector (f->value, bspfile.ambientLight);
		if (f = FindField ("_sun*"))
		{
			// have ArghRad sun
			bspfile.sunVec.Set (0, 0, -1);		// will be default even if incorrect sun with spotlight
			if (f = FindField ("_sun_vector"))
			{
				GetVector (f->value, bspfile.sunVec);
				bspfile.sunVec.NormalizeFast ();
			}
			else if (f = FindField ("_sun"))
				strcpy (sunTarget, f->value);	// worldspawn always 1st in entstring, so - just remember suntarget
			else if (f = FindField ("_sun_angle,_sun_mangle"))
			{
				CVec3 ang;
				sscanf (f->value, "%f %f", &ang[YAW], &ang[PITCH]);
				ang[PITCH] *= -1;
				AngleVectors (ang, &bspfile.sunVec, NULL, NULL);
			}
			if (f = FindField ("_sun_color"))
			{
				GetVector (f->value, bspfile.sunColor);
				NormalizeColor (bspfile.sunColor, bspfile.sunColor);
			}
			if (f = FindField ("_sun_light"))
			{
				bspfile.sunLight = atof (f->value);
				// can be up to 4 suns; just add its values
				for (int i = 2; i <= 4; i++)
					if (f = FindField (va("_sun%d_light", i)))
						bspfile.sunLight += atof (f->value);
					else
						break;
			}
			else
				bspfile.sunLight = 200;			// default
			if (f = FindField ("_sun_ambient"))
				GetVector (f->value, bspfile.sunAmbient);
			if (f = FindField ("_sun_surface"))
				GetVector (f->value, bspfile.sunSurface);
		}
		// Voodoo fog params
		RemoveField ("fogdensity2");
		RemoveField ("fogval2");

		return true;
	}

	/*-------------- Quake1 support ---------------*/
	bool resetSpawnflags = false;
	bool shiftUp = false;
#define CHANGE(orig, new)			\
	if (!strcmp (classname, orig))	\
		strcpy (classNameField->value, new);
#define CHANGE0(orig, new)			\
	if (!strcmp (classname, orig))	\
	{								\
		strcpy (classNameField->value, new); \
		resetSpawnflags = true;		\
		shiftUp = true;				\
	}
	if (bspfile.type == map_q1)
	{
		// teleport
		if (!strcmp (classname, "trigger_teleport"))
		{
			strcpy (classNameField->value, "misc_teleporter");
			if (model)
			{
				CVec3 org;
				model->bounds.GetCenter (org);
				org[2] = model->bounds.mins[2] - 17;
				AddField ("origin", va("%g %g %g", VECTOR_ARG(org)));
			}
		}
		else if (!strcmp (classname, "info_teleport_destination"))
		{
			strcpy (classNameField->value, "misc_teleporter_dest");
			origin[2] += 2;
			strcpy (originField->value, va("%g %g %g", VECTOR_ARG(origin)));
		}
		// weapons/ammo
		// weapon_: q2 have: supershotgun, rocketlauncher, grenadelauncher
		else CHANGE0("weapon_nailgun", "weapon_machinegun")
		else CHANGE0("weapon_supernailgun", "weapon_chaingun")
		else CHANGE0("weapon_lightning", "weapon_railgun")
		// ammo
		else CHANGE0("item_shells", "ammo_shells")		// shotguns
		else CHANGE0("item_spikes", "ammo_bullets")		// nailgun
		else CHANGE0("item_rockets", "ammo_grenades")	// rocketlauncher, grenadelauncher : ammo_rockets, ammo_grenades
		//!! NOTE: q1 uses rockets for rocketlauncher and grenadelauncher
		//!! We should check: if map have grenadelauncher - use ammo_grenades,
		//!! otherwise use ammo_rockets
		else CHANGE0("item_cells", "ammo_slugs")		// lightning

		// item_health
		else if (!strcmp (classname, "item_health"))
		{
			if (spawnflags == 1)					// small, "ROTTEN": 15 points
				strcpy (classNameField->value, "item_health_small");
			else if (spawnflags == 2)				// mega, "MEGA": 100 points w/o limit
				strcpy (classNameField->value, "item_health_mega");
			else									// normal, 25 points
				strcpy (classNameField->value, "item_health_large");
			RemoveField ("spawnflags");
			shiftUp = true;
		}

		// item_armor
		else CHANGE0("item_armor1", "item_armor_jacket")	// 0.3/100
		else CHANGE0("item_armor2", "item_armor_combat") 	// 0.6/150
		else CHANGE0("item_armorInv", "item_armor_body") 	// 0.8/200

		else CHANGE0("item_artifact_invulnerability", "item_invulnerability")
		else CHANGE0("item_artifact_envirosuit", "item_enviro")
		// item_artifact_invisibility
		else CHANGE0("item_artifact_super_damage", "item_quad")
		// Q2: item_breather, item_silencer, item_adrenaline, item_bandolier, item_pack
		// Q2: key_data_cd, key_power_cube, key_pyramid, key_data_spinner, key_pass,
		// Q2: key_blue_key, key_red_key, key_oommander_head, key_airstrike_target

//		else CHANGE("trigger_secret", "target_secret")	//?? not works

//		else CHANGE("trigger_changelevel", "target_changelevel")

		if (resetSpawnflags && spawnflagsField)
			strcpy (spawnflagsField->value, "0");
		if (shiftUp)
		{
			origin[2] += 20;
			strcpy (originField->value, va("%g %g %g", VECTOR_ARG(origin)));
		}
	}

	/*-------------- Kingpin support --------------*/

	if (bspfile.type == map_kp)
	{
		// check entities to remove
		if (!strcmp (classname, "junior"))
			return false;	// KP "junior" entity

		/*----- check entities with KP RF2_SURF_ALPHA flags ------*/
		int chk = 0;
		if (!stricmp (classname, "func_wall"))	//?? case insensitive ?? (check game source, KP game source ...)
			chk = 32;
		else if (!stricmp (classname, "func_door"))
			chk = 128;
		else if (!stricmp (classname, "func_door_rotating"))
			chk = 4;
		else if (!stricmp (classname, "func_explosive") || !stricmp (classname, "func_train") || !stricmp (classname, "func_train_rotating"))
			chk = 8;

		if ((chk & spawnflags) && model)
			model->flags |= CMODEL_ALPHA;

		// convert func_lift to func_train
		if (!strcmp (classname, "func_lift"))
		{
			f = FindField ("classname");
			strcpy (f->value, "func_train");
		}
	}

	/*---------------------------------------------*/

	if (!strcmp (classname, "target_splash"))
	{
		if (originField)
		{
			splash_t *spl = new (bspfile.extraChain) splash_t;
			spl->next = bspfile.splashes;
			bspfile.splashes = spl;
			spl->origin = origin;
			bspfile.numSplashes++;
		}

		return true;
	}

/*	appPrintf("------------\n");
	for (i = 0; i < numEntFields; i++)
	{
		appPrintf("\"%s\"=\"%s\"\n", entity[i].name, entity[i].value);
	} */

	return true;
}


static const char *ProcessEntstring (const char *entString)
{
	guard(ProcessEntstring);

	// patch (temporary !!)
	unsigned plen;
	char *patch = (char*) GFileSystem->LoadFile (va("%s.add", bspfile.name), &plen);
	plen++;	// add 1 byte for trailing zero

	char *dst, *dst2;
	dst = dst2 = (char*) bspfile.extraChain->Alloc (strlen (entString) + 1 + plen);

	// detect Kingpin map
	if (strstr (entString, "\"classname\" \"junior\"") ||
		strstr (entString, "\"classname\" \"lightflare\"") ||
		strstr (entString, "\"fogdensity2\""))
	{
		bspfile.type = map_kp;
		Com_DPrintf ("Kingpin map detected\n");
	}

	// find target entities
	const char *src = entString;
	numTargets = 0;
	while (!haveErrors && ReadEntity (src))
		ProcessEntityTarget ();

	// parse main entstring
	src = entString;
	haveErrors = false;
	sunTarget[0] = 0;
	while (!haveErrors && ReadEntity (src))
	{
		if (ProcessEntity ())
			WriteEntity (&dst);
	}

	// parse patch
	if (patch)
	{
		Com_DPrintf ("Adding entity patch ...\n");
		src = patch;
		while (!haveErrors && ReadEntity (src))
		{
			if (ProcessEntity ())
				WriteEntity (&dst);
		}
		delete patch;
	}

	if (haveErrors)
		return entString;

	Com_DPrintf ("ProcessEntstring: size = %d -> %d\n", strlen (entString), dst - dst2);
	return dst2;

	unguard;
}
