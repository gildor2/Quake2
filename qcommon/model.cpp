#include "qcommon.h"
#include "cmodel.h"


static bspfile_t bspfile;
static dBsp2Hdr_t *header;


//?? Should perform SwapBlock() (as in Q3 bsp tools)
static void SwapQ2BspFile (bspfile_t *f)
{
	int		i, j;

	// models
	for (i = 0; i < f->numModels; i++)
	{
		cmodel_t	*d;

		d = &f->models[i];

		d->firstface = LittleLong (d->firstface);
		d->numfaces = LittleLong (d->numfaces);
		d->headnode = LittleLong (d->headnode);

		for (j = 0; j < 3; j++)
		{
			d->bounds.mins[j] = LittleFloat(d->bounds.mins[j]);
			d->bounds.maxs[j] = LittleFloat(d->bounds.maxs[j]);
//			d->origin[j] = LittleFloat(d->origin[j]);
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

		float *vec = &d->vecs[0].vec[0];	// hack to get all ([3]+1)[2] == 8 components of texinfo_t.vecs[]
		for (j = 0; j < 8; j++)
			vec[j] = LittleFloat (vec[j]);
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
		texinfo_t *d = &f->texinfo[i];
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

}


static void LoadQ2Submodels (bspfile_t *f, dmodel_t *data)
{
	if (f->numModels < 1)
		Com_DropError ("Map with no models");

	cmodel_t *out = f->models = new (f->extraChain) cmodel_t[f->numModels];
	for (int i = 0; i < f->numModels; i++, data++, out++)
	{
		out->bounds = data->bounds;
		out->radius = VectorDistance (out->bounds.mins, out->bounds.maxs) / 2;
		out->headnode = data->headnode;
		out->flags = 0;
		out->firstface = data->firstface;
		out->numfaces = data->numfaces;
		// dmodel_t have unused field "origin"
	}
}


static int CheckLump (int lump, void **ptr, int size)
{
	int length = header->lumps[lump].filelen;
	int ofs = header->lumps[lump].fileofs;

	if (length % size)
		Com_DropError ("LoadBSPFile: incorrect lump size");

	*ptr = (byte *)header + ofs;

	return length / size;
}


void LoadQ2BspFile ()
{
	guard(LoadQ2BspFile);

	header = (dBsp2Hdr_t *) bspfile.file;

#ifndef LITTLE_ENDIAN
	// swap the header
	for (int i = 0; i < sizeof(dBsp2Hdr_t) / 4; i++)
		((int *)header)[i] = LittleLong (((int *)header)[i]);
#endif

	if (header->version != BSP2_VERSION)
		Com_DropError ("%s is version %d, not " STR(BSP2_VERSION) "\n", bspfile.name, header->version);

	bspfile.type = map_q2;

#define C(num,field,type)	CheckLump(LUMP_##num, (void**)&bspfile.field, sizeof(type))
	bspfile.lightDataSize =	C(LIGHTING, lighting, byte);
	bspfile.visDataSize =	C(VISIBILITY, visibility, byte);

	bspfile.numVertexes =	C(VERTEXES, vertexes, dvertex_t);
	bspfile.numPlanes =		C(PLANES, planes, dplane_t);
	bspfile.numLeafs =		C(LEAFS, leafs, dleaf_t);
	bspfile.numNodes =		C(NODES, nodes, dnode_t);
	bspfile.numTexinfo =	C(TEXINFO, texinfo, texinfo_t);
	bspfile.numFaces =		C(FACES, faces, dface_t);
	bspfile.numLeaffaces =	C(LEAFFACES, leaffaces, unsigned short);
	bspfile.numLeafbrushes = C(LEAFBRUSHES, leafbrushes, unsigned short);
	bspfile.numSurfedges =	C(SURFEDGES, surfedges, int);
	bspfile.numEdges =		C(EDGES, edges, dedge_t);
	bspfile.numBrushes =	C(BRUSHES, brushes, dbrush_t);
	bspfile.numBrushsides = C(BRUSHSIDES, brushsides, dbrushside_t);
	bspfile.numAreas =		C(AREAS, areas, darea_t);
	bspfile.numAreaportals = C(AREAPORTALS, areaportals, dareaportal_t);

	dmodel_t	*models;
	bspfile.numModels =		CheckLump (LUMP_MODELS, (void**)&models, sizeof(dmodel_t));	// not in bspfile_t struc
	LoadQ2Submodels (&bspfile, models);

#ifndef LITTLE_ENDIAN
	// swap everything
	SwapQ2BspFile (&bspfile);
#endif
	ProcessQ2BspFile (&bspfile);

	// load entstring after all: we may require to change something
#if 0
	bspfile.entDataSize = C(ENTITIES, entities, char);
#else
#undef C
	bspfile.entities = ProcessEntstring ((char*)header + header->lumps[LUMP_ENTITIES].fileofs);
	bspfile.entDataSize = strlen (bspfile.entities);
#endif

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
#if 0
		case BSPHL_VERSION:
			LoadHLBspFile ();
			return &bspfile;
#endif
	}
	// error
	delete bspfile.file;
	bspfile.name[0] = 0;
	bspfile.file = NULL;
	map_clientLoaded = false;
	Com_DropError ("%s has a wrong BSP header\n", filename);
	return NULL;		// make compiler happy

	unguard;
}


/*--------------------------------------------------------------------*/

#define MAX_ENT_FIELDS	32
#define MAX_TARGETS		256

typedef struct
{
	char	name[64];
	char	value[192];
} entField_t;

typedef struct
{
	char	name[64];
	CVec3	origin;
} target_t;


static bool haveErrors;
static bool isPatch;
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


static void RemoveField (char *name)
{
	entField_t *field;

	if (field = FindField (name))
		field->name[0] = 0;
}


static void AddField (char *name, char *value)
{
	if (numEntFields >= MAX_ENT_FIELDS) return;

	strcpy (entity[numEntFields].name, name);
	strcpy (entity[numEntFields].value, value);
	numEntFields++;
}


static void ErrMsg (char *str)
{
	haveErrors = true;
	appWPrintf ("EntString error: %s\n", str);
}


static bool ReadEntity (const char *&src)
{
	char	*tok;
	entField_t *field;

	numEntFields = 0;

	if (!src) return false;

	tok = COM_Parse (src);
	if (!tok[0]) return false;

	if (tok[0] != '{' || tok[1] != 0)
	{
		ErrMsg ("expected \"{\"");
		return false;
	}

	field = entity;
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


//#define SHOW_WRITE

// debug helper
#define DUMP_ENTITY	\
			{		\
				for (int i = 0; i < numEntFields; i++)	\
					DebugPrintf("-- %s : %s\n", entity[i].name, entity[i].value);	\
			}


static void WriteEntity (char **dst)
{
	int		i;
#ifdef SHOW_WRITE
	char	*txt = *dst;
#endif

/* -- disabled: will remove { "classname" "worldspawn" }  (map "campblud")
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
#ifdef SHOW_WRITE
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
	entField_t *f;

	if (f = FindField ("targetname"))
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


static const CVec3& FindEntityTarget (char *name)
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
	const char *classname;
	if (f = FindField ("classname"))
		classname = f->value;
	else
		classname = "";
	// get spawnflags
	int spawnflags;
	if (f = FindField ("spawnflags"))
		spawnflags = atoi (f->value);
	else
		spawnflags = 0;
#if 0
#define SPAWNFLAG_NOT_DEATHMATCH	0x800
	if (Cvar_VariableInt("keep_sp") && (spawnflags & SPAWNFLAG_NOT_DEATHMATCH))//!!
	{
		spawnflags &= ~SPAWNFLAG_NOT_DEATHMATCH;
		appSprintf (f->value, sizeof(f->value), "%d", spawnflags);
	}
#endif
	// get origin
	bool haveOrigin = false;
	CVec3 origin;
	if (f = FindField ("origin"))
	{
		haveOrigin = true;
		GetVector (f->value, origin);
	}
	// get inline model
	int modelIdx;
	bool haveModel = false;
	if (f = FindField ("model"))
	{
		sscanf (f->value, "*%d", &modelIdx);
		if (modelIdx >= bspfile.numModels)
		{
			haveModel = false;
			Com_DPrintf ("invalid model index %d\n", modelIdx);
		}
		else
			haveModel = true;
	}

	/*----------- check movable inline models -----------*/

	if (haveModel && modelIdx > 0 && appMatchWildcard (classname,
		"func_plat,func_*rotating,func_door,func_train"))
		bspfile.models[modelIdx].flags |= CMODEL_MOVABLE;

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
			if (haveModel)
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
				texinfo_t *d;
				for (i = 0, d = bspfile.texinfo; i < bspfile.numTexinfo; i++, d++)
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

		if ((chk & spawnflags) && haveModel && modelIdx > 0)
			bspfile.models[modelIdx].flags |= CMODEL_ALPHA;

		// convert func_lift to func_train
		if (!strcmp (classname, "func_lift"))
		{
			f = FindField ("classname");
			strcpy (f->value, "func_train");
		}
	}

	/*---------------------------------------------*/

	if (!strcmp (classname, "worldspawn"))
	{
		if (bspfile.type == map_kp && !FindField ("sky"))
			AddField ("sky", "sr");		// set default Kingpin sky
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

	/*---------------------------------------------*/

	if (!strcmp (classname, "target_splash"))
	{
		if (haveOrigin)
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


char *ProcessEntstring (char *entString)
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
	haveErrors = isPatch = false;
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
		isPatch = true;
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
