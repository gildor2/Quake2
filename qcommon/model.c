#include "qcommon.h"


static bspfile_t bspfile;
static dheader_t *header;


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
			d->mins[j] = LittleFloat(d->mins[j]);
			d->maxs[j] = LittleFloat(d->maxs[j]);
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

static void ProcessQ2BspFile (bspfile_t *f)
{
	int i, j;

	// models: inflate mins/maxs by 1
	for (i = 0; i < f->numModels; i++)
	{
		cmodel_t	*d;

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


static void LoadQ2Submodels (bspfile_t *f, dmodel_t *data)
{
	cmodel_t *out;
	int		i;

	if (f->numModels < 1)
		Com_Error (ERR_DROP, "Map with no models");

	out = f->models = AllocChainBlock (f->extraChain, sizeof(cmodel_t) * f->numModels);
	for (i = 0; i < f->numModels; i++, data++, out++)
	{
		vec3_t	tmp;

		VectorCopy (data->mins, out->mins);
		VectorCopy (data->maxs, out->maxs);
		VectorSubtract (out->maxs, out->mins, tmp);
		out->radius = VectorLength (tmp) / 2;
		out->headnode = data->headnode;
		out->flags = 0;
		out->firstface = data->firstface;
		out->numfaces = data->numfaces;
		// dmodel_t have unused field "origin"
	}
}


static int CheckLump (int lump, void **ptr, int size)
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
	dmodel_t	*models;

	header = (dheader_t *) bspfile.file;

	// swap the header
	for (i = 0; i < sizeof(dheader_t) / 4; i++)
		((int *)header)[i] = LittleLong (((int *)header)[i]);

	if (header->version != BSP2_VERSION)
		Com_Error (ERR_DROP, "%s is version %i, not %i\n", bspfile.name, header->version, BSP2_VERSION);

	bspfile.type = map_q2;

	bspfile.lightDataSize = CheckLump (LUMP_LIGHTING, &bspfile.lighting, 1);
	bspfile.visDataSize = CheckLump (LUMP_VISIBILITY, &bspfile.visibility, 1);

	bspfile.numModels = CheckLump (LUMP_MODELS, &models, sizeof(dmodel_t));
	LoadQ2Submodels (&bspfile, models);
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

	// load entstring after all: we may require to change something
#if 0
	bspfile.entDataSize = CheckLump (LUMP_ENTITIES, &bspfile.entities, 1);
#else
	bspfile.entities = ProcessEntstring ((byte*)header + header->lumps[LUMP_ENTITIES].fileofs);
	bspfile.entDataSize = strlen (bspfile.entities);
#endif
}


bspfile_t *LoadBspFile (char *filename, qboolean clientload, unsigned *checksum)
{
	guard(LoadBspFile);

	if (!stricmp (filename, bspfile.name) && (clientload || !Cvar_VariableInt ("flushmap")))
	{
		if (checksum)
			*checksum = bspfile.checksum;
		return &bspfile;
	}

	if (clientload && Com_ServerState() && stricmp (filename, bspfile.name))
		Com_Error (ERR_DROP, "client trying to load map \"%s\" while server running \"%s\"", filename, bspfile.name);

	if (bspfile.name[0] && bspfile.file)
		FS_FreeFile (bspfile.file);
	if (bspfile.extraChain)
		FreeMemoryChain (bspfile.extraChain);

	memset (&bspfile, 0, sizeof(bspfile));
	strcpy (bspfile.name, filename);
	bspfile.length = FS_LoadFile (filename, &bspfile.file);
	if (!bspfile.file)
	{
		bspfile.name[0] = 0;
		Com_Error (ERR_DROP, "Couldn't load %s", filename);
	}
	bspfile.checksum = LittleLong (Com_BlockChecksum (bspfile.file, bspfile.length));
		if (checksum) *checksum = bspfile.checksum;
	bspfile.extraChain = CreateMemoryChain ();

	switch (LittleLong(*(unsigned *)bspfile.file))
	{
		case BSP2_IDENT:
			LoadQ2BspFile ();
			return &bspfile;
//??		case BSPHL_VERSION:
//			LoadHLBspFile ();
//			return &bspfile;
	}
	// error
	FS_FreeFile (bspfile.file);
	bspfile.name[0] = 0;
	bspfile.file = NULL;
	Com_Error (ERR_DROP, "LoadBrushModel: %s has a wrong BSP header\n", filename);
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
	vec3_t	origin;
} target_t;


static qboolean haveErrors;
static qboolean isPatch;
static entField_t entity[MAX_ENT_FIELDS];
static int numEntFields;

static target_t targets[MAX_TARGETS];
static int numTargets;

static char sunTarget[64];


static entField_t *FindField (char *name)
{
	int		i;
	entField_t *f;

	for (i = 0, f = entity; i < numEntFields; i++, f++)
		if (MatchWildcard (f->name, name))		// case-sensitive
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
	Com_WPrintf ("EntString error: %s\n", str);
}


static bool ReadEntity (char **src)
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
	while (1)
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
				int i;	\
				for (i = 0; i < numEntFields; i++)	\
					DebugPrintf("-- %s : %s\n", entity[i].name, entity[i].value);	\
			}


static void WriteEntity (char **dst)
{
	int		i;
#ifdef SHOW_WRITE
	char	*txt = *dst;
#endif

/* -- disabled: will remove { "classname" "worldspawn" }  (map "campblud")
	qboolean found;
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
			(*dst) += Com_sprintf (*dst, 1024, "\"%s\" \"%s\"\n", entity[i].name, entity[i].value);
	strcpy (*dst, "}\n"); (*dst) += 2;
#ifdef SHOW_WRITE
	Com_Printf ("^6%s", txt);
#endif
}

// gets "%f %f %f" or "%f"
static void GetVector (char *str, vec3_t vec)
{
	if (sscanf (str, "%f %f %f", &vec[0], &vec[1], &vec[2]) != 3)
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


static float *FindEntityTarget (char *name)
{
	int		i;

	for (i = 0; i < numTargets; i++)
		if (!strcmp (targets[i].name, name))
			return targets[i].origin;

	Com_DPrintf ("target \"%s\" is not found\n", name);
	return vec3_origin;		// not found
}


// Returns "true" if entity should be passed to game
static bool ProcessEntity ()
{
	entField_t *f;
	qboolean haveOrigin, haveModel;
	int		modelIdx;
	vec3_t	origin;
	int		spawnflags;
	char	*class;

	/*------------------ get some fields -------------------*/

	// get classname
	if (f = FindField ("classname"))
		class = f->value;
	else
		class = "";
	// get spawnflags
	if (f = FindField ("spawnflags"))
		spawnflags = atoi (f->value);
	else
		spawnflags = 0;
#if 0
#define SPAWNFLAG_NOT_DEATHMATCH	0x800
	if (Cvar_VariableInt("keep_sp") && (spawnflags & SPAWNFLAG_NOT_DEATHMATCH))//!!
	{
		spawnflags &= ~SPAWNFLAG_NOT_DEATHMATCH;
		Com_sprintf (f->value, sizeof(f->value), "%d", spawnflags);
	}
#endif
	// get origin
	if (f = FindField ("origin"))
	{
		haveOrigin = true;
		GetVector (f->value, origin);
	}
	else
		haveOrigin = false;
	// get inline model
	if (f = FindField ("model"))
	{
		haveModel = true;
		sscanf (f->value, "*%d", &modelIdx);
		if (modelIdx >= bspfile.numModels)
		{
			haveModel = false;
			Com_DPrintf ("invalid model index %d\n", modelIdx);
		}
	}
	else
		haveModel = false;

	/*----------- check movable inline models -----------*/

	if (haveModel && modelIdx > 0 && MatchWildcard (class,
		"func_plat,func_*rotating,func_door,func_train"))
		bspfile.models[modelIdx].flags |= CMODEL_MOVABLE;

	/*---------------- get lighting info ----------------*/

	if (!strncmp (class, "light", 5))
	{
		int		style;
		slight_t *slight;

		// "style": if >= 32 ... (def: 0)
		// "spawnflags": &2 -> flare, &4 -> resize, {&8 -> dynamic ?}
		// get common lighting fields
		if (f = FindField ("style,_style"))
			style = atoi (f->value);
		else
			style = 0;							// default

		/*--------------- lightflares -----------------*/

		if ((!class[5] || !strcmp (class + 5, "flare")) && (spawnflags & 2))	// "light" or "lightflare"
		{
			lightFlare_t *flare;

			// "health" -> size (def: 24)
			// "dmg": normal (0, def), sun, amber, red, blue, green
			// if "dmg" == SUN -> VectorNormalize(origin)

			flare = AllocChainBlock (bspfile.extraChain, sizeof(lightFlare_t));
			flare->next = bspfile.flares;
			bspfile.flares = flare;
			bspfile.numFlares++;

			if (f = FindField ("health"))
				flare->size = atof (f->value);
			else
				flare->size = 24;							// default size
			flare->style = style;

			VectorSet (flare->color, 255, 255, 255);
			flare->color[3] = 255;
			if (f = FindField ("dmg"))
			{
				switch (atoi (f->value))
				{
				case 1:
					if (bspfile.type == map_kp) DebugPrintf ("HAVE SUN FLARE: %s\n", bspfile.name);//!!
					flare->radius = -1;						// mark as "sun"
					flare->color[2] = 192;
					VectorNormalize (origin);				// just a sun direction
					break;
				case 2:		// amber
					VectorSet (flare->color, 255, 192, 0);
					break;
				case 3:		// red
					VectorSet (flare->color, 255, 0, 64);
					break;
				case 4:		// blue
					VectorSet (flare->color, 0, 192, 255);
					break;
				case 5:		// green
					VectorSet (flare->color, 0, 255, 128);
					break;
				}
			}
			VectorCopy(origin, flare->origin);
			if (f = FindField ("radius"))
				flare->radius = atof (f->value);
			if (f = FindField ("color"))					// can override sun color ...
				sscanf (f->value, "%d %d %d", &flare->color[0], &flare->color[1], &flare->color[2]);
			if (haveModel)
				flare->model = modelIdx;

			if (class[5]) return false;						// flares passed to renderer only (if classname = "light" -> flare+light)
		}

		/*-------------- light, light_mine -------------*/

		slight = AllocChainBlock (bspfile.extraChain, sizeof(slight_t));
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
			VectorSet (slight->color, 1, 1, 1);			// default

		if (f = FindField ("_falloff"))					// 0 - linear (default), 1 - 1/dist. 2 - 1/(dist*dist)
			slight->type = atoi (f->value);
		if (f = FindField ("_wait,_fade"))
			slight->fade = atof (f->value);
		if (!slight->fade) slight->fade = 1;			// default value + disallow "fade==0"

		VectorCopy (origin, slight->origin);

		if (f = FindField ("target"))
		{
			float	*dst;
			vec3_t	vec;

			slight->spot = true;
			dst = FindEntityTarget (f->value);
			VectorSubtract (dst, slight->origin, vec);
			VectorNormalize2 (vec, slight->spotDir);

			if (!strcmp (f->value, sunTarget))
			{
				// this spotlight used as sun target
				VectorCopy (slight->spotDir, bspfile.sunVec);
				// remove from list
				bspfile.slights = slight->next;
				bspfile.numSlights--;
//Com_Printf("^1sun: %g %g %g (dst: %g %g %g, org: %g %g %g)\n",VECTOR_ARG(bspfile.sunVec),VECTOR_ARG(dst),VECTOR_ARG(slight->spotDir));
			}
		}
		if (!strcmp (class + 5, "_spot") || FindField ("_spotvector,_spotpoint,_mangle,_spotangle"))
		{
			vec3_t	vec;

			slight->spot = true;
			if (f = FindField ("_spotvector"))
				GetVector (f->value, slight->spotDir);
			else if (f = FindField ("_spotpoint"))
			{
				GetVector (f->value, vec);
				VectorSubtract (vec, slight->origin, vec);
				VectorNormalize2 (vec, slight->spotDir);
			}
			else if (f = FindField ("_mangle,_spotangle"))
			{
				float	ang[3];

				sscanf (f->value, "%f %f", &ang[YAW], &ang[PITCH]);
				ang[PITCH] *= -1;
				AngleVectors (ang, slight->spotDir, NULL, NULL);
			}
			else if (f = FindField ("angle"))
			{
				float	angle;

				angle = atof (f->value);
				if (angle == -1)
					VectorSet (slight->spotDir, 0, 0, 1);
				else if (angle == -2)
					VectorSet (slight->spotDir, 0, 0, -1);
				else
				{
					angle *= M_PI / 180.0f;
					slight->spotDir[0] = cos(angle);
					slight->spotDir[1] = sin(angle);
					slight->spotDir[2] = 0;
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

		if ((!class[5] && style < 32) || !strcmp (class + 5, "flare"))
			return false;		// remove: "lightflare" or non-switchable "light"

		return true;
	}

	// our map script (temp !!)
	if (!strcmp (class, "surfparam"))
	{
		if (f = FindField ("name"))
		{
			int		i;
			texinfo_t *d;
			char	*name;

			name = f->value;
			if (f = FindField ("flags"))
			{
				qboolean found;
				int		flags, testflags, testmask;

				flags = atoi (f->value);
				if (f = FindField ("inflags"))
					sscanf (f->value, "%d %d", &testmask, &testflags);
				else
					testflags = testmask = 0;
				found = false;
				for (i = 0, d = bspfile.texinfo; i < bspfile.numTexinfo; i++, d++)
					if (MatchWildcard2 (d->texture, name, true) && ((d->flags & testmask) == testflags))
//					if (!strcmp (d->texture, name) && ((d->flags & testmask) == testflags))
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
		int		chk;

		// check entities to remove
		if (!strcmp (class, "junior"))
			return false;	// KP "junior" entity

		/*----- check entities with KP RF2_SURF_ALPHA flags ------*/
		chk = 0;
		if (!stricmp (class, "func_wall"))
			chk = 32;
		else if (!stricmp (class, "func_door"))
			chk = 128;
		else if (!stricmp (class, "func_door_rotating"))
			chk = 4;
		else if (!stricmp (class, "func_explosive") || !stricmp (class, "func_train") || !stricmp (class, "func_train_rotating"))
			chk = 8;

		if ((chk & spawnflags) && haveModel && modelIdx > 0)
			bspfile.models[modelIdx].flags |= CMODEL_ALPHA;

		// convert func_lift to func_train
		if (!strcmp (class, "func_lift"))
		{
			f = FindField ("classname");
			strcpy (f->value, "func_train");
		}
	}

	/*---------------------------------------------*/

	if (!strcmp (class, "worldspawn"))
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
			VectorSet (bspfile.sunVec, 0, 0, -1);		// will be default even if incorrect sun with spotlight
			if (f = FindField ("_sun_vector"))
			{
				GetVector (f->value, bspfile.sunVec);
				VectorNormalize (bspfile.sunVec);
			}
			else if (f = FindField ("_sun"))
				strcpy (sunTarget, f->value);	// worldspawn always 1st in entstring, so - just remember suntarget
			else if (f = FindField ("_sun_angle,_sun_mangle"))
			{
				float	ang[3];

				sscanf (f->value, "%f %f", &ang[YAW], &ang[PITCH]);
				ang[PITCH] *= -1;
				AngleVectors (ang, bspfile.sunVec, NULL, NULL);
			}
			if (f = FindField ("_sun_color"))
			{
				GetVector (f->value, bspfile.sunColor);
				NormalizeColor (bspfile.sunColor, bspfile.sunColor);
			}
			if (f = FindField ("_sun_light"))
			{
				int		i;

				bspfile.sunLight = atof (f->value);
				// can be up to 4 suns; just add its values
				for (i = 2; i <= 4; i++)
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

	if (!strcmp (class, "target_splash"))
	{
		if (haveOrigin)
		{
			splash_t	*spl;
			spl = AllocChainBlock (bspfile.extraChain, sizeof(splash_t));
			spl->next = bspfile.splashes;
			bspfile.splashes = spl;
			VectorCopy (origin, spl->origin);
			bspfile.numSplashes++;
		}

		return true;
	}

/*	Com_Printf("------------\n");
	for (i = 0; i < numEntFields; i++)
	{
		Com_Printf("\"%s\"=\"%s\"\n", entity[i].name, entity[i].value);
	} */

	return true;
}


char *ProcessEntstring (char *entString)
{
	char	*dst, *dst2, *src;
	// patch (temporary !!)
	int		plen;
	char	*patch;

	plen = FS_LoadFile (va("%s.add", bspfile.name), &patch) + 1;

	dst = dst2 = AllocChainBlock (bspfile.extraChain, strlen (entString) + 1 + plen);

	// detect Kingpin map
	if (strstr (entString, "\"classname\" \"junior\"") ||
		strstr (entString, "\"classname\" \"lightflare\"") ||
		strstr (entString, "\"fogdensity2\""))
	{
		bspfile.type = map_kp;
		Com_DPrintf ("Kingpin map detected\n");
	}

	// find target entities
	src = entString;
	numTargets = 0;
	while (!haveErrors && ReadEntity (&src))
		ProcessEntityTarget ();

	// parse main entstring
	src = entString;
	haveErrors = isPatch = false;
	sunTarget[0] = 0;
	while (!haveErrors && ReadEntity (&src))
	{
		if (ProcessEntity ())
			WriteEntity (&dst);
	}

	// parse patch
	if (plen)
	{
		Com_DPrintf ("Adding entity patch ...\n");
		isPatch = true;
		src = patch;
		while (!haveErrors && ReadEntity (&src))
		{
			if (ProcessEntity ())
				WriteEntity (&dst);
		}
	}

	if (haveErrors)
		return entString;

	Com_DPrintf ("ProcessEntstring(): old size = %d, new size = %d\n", strlen (entString), dst - dst2);
	return dst2;
}
