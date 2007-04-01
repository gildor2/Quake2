#include "qcommon.h"
#include "cmodel.h"


//#define ENT_STATS			1		// display stats about entity class counts
//#define SHOW_WRITE		1
//#define REMOVE_ENTS		1		// debug: remove most world entities

#if MAX_DEBUG
#undef ENT_STATS
#define ENT_STATS			1
#endif


/*-----------------------------------------------------------------------------
	Common field parser/holder
-----------------------------------------------------------------------------*/

#define MAX_ENT_FIELDS		32
#define MAX_TARGETS			256
#define MAX_SUNS			4		// arghrad limit
#define MAX_DOORS			256		// for Q1

struct entField_t
{
	char	name[64];
	char	value[256];				// should be large enough (for example, for "wad" field in "worldspawn", hl bsp)
};

struct target_t
{
	char	name[64];
	CVec3	origin;
};

struct door_t
{
	char	className[64];
	int		modelIdx;				// inline model for this door
	int		linkModel;				// when door already linked, this will contain model of master door
};


static bool haveErrors;
static entField_t entity[MAX_ENT_FIELDS];
static int numEntFields;

static target_t targets[MAX_TARGETS];
static int numTargets;

static char sunTarget[64][MAX_SUNS];
static slight_t *sunLight[MAX_SUNS];

static door_t doors[MAX_DOORS];
static int numDoors;

// some fields from current entity
static const char *classname;
static entField_t *classNameField;

static int spawnflags;
static entField_t *spawnflagsField;

static CVec3 entOrigin;
static entField_t *originField;

static entField_t *modelField;
static int modelIdx;
static CBspModel *entModel;


static entField_t *FindField(const char *name)
{
	int		i;
	entField_t *f;
	for (i = 0, f = entity; i < numEntFields; i++, f++)
		if (appMatchWildcard(f->name, name))		// case-sensitive
			return f;
	return NULL;
}


static void RemoveField(const char *name)
{
	if (entField_t *field = FindField(name))
		field->name[0] = 0;
}


static void AddField(const char *name, const char *value)
{
	if (numEntFields >= MAX_ENT_FIELDS) return;

	strcpy(entity[numEntFields].name, name);
	strcpy(entity[numEntFields].value, value);
	numEntFields++;
}


static void ErrMsg(const char *str)
{
	haveErrors = true;
	appWPrintf("EntString error: %s\n", str);
}


static bool ReadEntity(const char *&src)
{
	numEntFields = 0;
	if (!src) return false;

	const char *tok = COM_Parse(src);
	if (!tok[0]) return false;

	if (tok[0] != '{' || tok[1] != 0)
	{
		ErrMsg("expected \"{\"");
		return false;
	}

	entField_t *field = entity;
	while (true)
	{
		tok = COM_Parse(src);
		if (tok[0] == '}' && tok[1] == 0)
			break;

		// add field name
		appStrncpyz(field->name, tok, ARRAY_COUNT(field->name));

		// add field value
		tok = COM_Parse(src);
		if (!tok)
		{
			ErrMsg("unexpected end of data");
			return false;
		}
		appStrncpyz(field->value, tok, ARRAY_COUNT(field->value));

		if (numEntFields++ == MAX_ENT_FIELDS)
		{
			ErrMsg("MAX_ENT_FIELDS");
			return false;
		}
		field++;
	}

	return true;
}


static void WriteEntity(char **dst)
{
#if SHOW_WRITE
	const char *txt = *dst;
#endif
	strcpy(*dst, "{\n"); (*dst) += 2;
	for (int i = 0; i < numEntFields; i++)
		if (entity[i].name[0])	// may be removed field
			(*dst) += appSprintf(*dst, 1024, "\"%s\" \"%s\"\n", entity[i].name, entity[i].value);
	strcpy(*dst, "}\n"); (*dst) += 2;
#if SHOW_WRITE
	appPrintf(S_CYAN"%s", txt);
#endif
}

// gets "%f %f %f" or "%f"
static void GetVector(const char *str, CVec3 &vec)
{
	if (sscanf(str, "%f %f %f", VECTOR_ARG(&vec)) != 3)
		vec[1] = vec[2] = vec[0];
}


static void ProcessEntityTarget()
{
	if (entField_t *f = FindField("targetname"))
	{
		if (numTargets == MAX_TARGETS)
		{
			haveErrors = true;
			Com_DPrintf("MAX_TARGETS reached\n");
			return;
		}
		strcpy(targets[numTargets].name, f->value);
		if (f = FindField("origin"))	// target can be without origin, but we are not interested with it
		{
			GetVector(f->value, targets[numTargets].origin);
			numTargets++;
		}
	}
}


static const CVec3& FindEntityTarget(const char *name)
{
	for (int i = 0; i < numTargets; i++)
		if (!strcmp(targets[i].name, name))
			return targets[i].origin;

	Com_DPrintf("target \"%s\" is not found\n", name);
	return nullVec3;		// not found
}


/*-----------------------------------------------------------------------------
	Loading light entities
-----------------------------------------------------------------------------*/

static bool LoadLight()
{
	guard(LoadLight);

	entField_t *f;

	// "style": if >= 32 ... (def: 0)
	// "spawnflags": &2 -> flare, &4 -> resize, {&8 -> dynamic ?}
	// get common lighting fields
	int style = 0;
	if (f = FindField("style,_style"))
		style = atoi(f->value);

	// HL!!: "light": may have pattern (lightstyle: "aazzazz")

	/*--------------- lightflares -----------------*/

	if ((!classname[5] || !strcmp(classname + 5, "flare")) && (spawnflags & 2))	// "light" or "lightflare"
	{
		// "health" -> size (def: 24)
		// "dmg": normal (0, def), sun, amber, red, blue, green
		// if "dmg" == SUN -> origin.Normalize()

		lightFlare_t *flare = new (bspfile.extraChain) lightFlare_t;
		flare->next = bspfile.flares;
		bspfile.flares = flare;
		bspfile.numFlares++;

		if (f = FindField("health"))
			flare->size = atof(f->value);
		else
			flare->size = 24;						// default size
		flare->style = style;

		flare->color.rgba = RGB(1,1,1);
		if (f = FindField("dmg"))
		{
			switch (atoi(f->value))
			{
			case 1:
				if (bspfile.type == map_kp) DebugPrintf("HAVE SUN FLARE: %s\n", *bspfile.Name);//!!
				flare->radius = -1;					// mark as "sun"
				flare->color.c[2] = 192;
				entOrigin.NormalizeFast();			// just a sun direction
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
		flare->origin = entOrigin;
		if (f = FindField("radius"))
			flare->radius = atof(f->value);
		if (f = FindField("color"))				// can override sun color ...
		{
			// NOTE: cannot scanf() into byte-sized vars
			int	v[3];
			sscanf(f->value, "%d %d %d", VECTOR_ARG(&v));
			flare->color.rgba = RGB255(v[0],v[1],v[2]);
		}
		if (entModel)
			flare->model = modelIdx;

		if (classname[5]) return false;				// flares passed to renderer only (if classname = "light" -> flare+light)
	}

	/*-------------- light, light_mine -------------*/

	slight_t *slight = new (bspfile.extraChain) slight_t;
	slight->next = bspfile.slights;
	bspfile.slights = slight;

	slight->style  = style;
	slight->origin = entOrigin;

	if (!strcmp(classname + 5, "_environment"))	// HL sun
	{
		bspfile.sunCount++;
		slight->sun = true;
	}

	// ArghRad, ZHLT: "_falloff": 0 - linear (default), 1 - 1/dist. 2 - 1/(dist*dist)
	if (f = FindField("_falloff"))
		slight->type = (slightType_t)atoi(f->value);
	if (f = FindField("_wait,_fade,wait"))			// ArghRad + TyrLite
		slight->fade = atof(f->value);
	if (!slight->fade) slight->fade = 1;			// default value + disallow "fade==0"

	if (bspfile.type != map_hl)
	{
		// Quake1, Quake2, Quake3, Kingpin
		if (f = FindField("light,_light"))
			slight->intens = atof(f->value);
		else
			slight->intens = 300;					// default

		if (f = FindField("color,_color"))
		{
			GetVector(f->value, slight->color);
			NormalizeColor(slight->color, slight->color);
		}
		else
			slight->color.Set(1, 1, 1);			// default
	}

	if (bspfile.type == map_q1)
	{
		// IKLite: "angle" field can change attenuation
		if (!FindField("target") &&				// not spotlight
			(f = FindField("angle")))
		{
			static const slightType_t types[] = {sl_linear, sl_inverse2, sl_nofade, sl_inverse};
			int n = atoi(f->value);
			if (n >= 0 && n <= 3)				// if outside range - possibly, not attenuation field ...
				slight->type = types[n];
		}
		// TyrLite: "delay": 0 - linear, 1 - 1/dist, 2 - 1/(dist*dist), 3 - no fade
		if (f = FindField("delay"))
			slight->type = (slightType_t)atoi(f->value);
		// adjust intensity for inverse lights (take from TyrLite source)
		if (slight->type == sl_inverse)
			slight->intens *= 128;
		else if (slight->type == sl_inverse2)
			slight->intens *= 16384;
	}

	if (bspfile.type == map_q3)
	{
		// Quake3
		slight->type   = (spawnflags & 1) ? sl_linear : sl_inverse2;
		slight->intens *= 7500;					// pointScale in q3map
	}

	if (bspfile.type == map_hl)
	{
		// Half-Life light entity
		if (f = FindField("_light"))
		{
			float bright;
			int i = sscanf(f->value, "%f %f %f %f", VECTOR_ARG(&slight->color), &bright);
			if (i == 1)
				slight->color[1] = slight->color[2] = slight->color[0];
			else if (i == 4)
				slight->color.Scale(bright / 255.0f);
			slight->type   = sl_inverse2;
			slight->intens = slight->color.Normalize();
			slight->intens *= 256*25.6;				// qrad: max * max / 10, but operates with 0..255 values
		}
		// else - bad (will be zero)
		if (slight->sun)
			VectorScale(slight->color, slight->intens / (30000 * 1000), bspfile.sunSurface);
	}

	// targetted spotlight
	if (f = FindField("target"))
	{
		slight->spot = true;
		const CVec3 &dst = FindEntityTarget(f->value);
		CVec3	vec;
		VectorSubtract(dst, slight->origin, vec);
		float dist = VectorNormalize(vec, slight->spotDir);

		if (bspfile.type == map_q3)
		{
			entField_t *f2;
			float radius = 64;
			if (f2 = FindField("radius"))
				radius = atof(f2->value);
			if (!dist) dist = 64;
			float radiusByDist = (radius + 16) / dist;
			slight->spotDot = cos(atan(radiusByDist));
			slight->type    = sl_inverse2;
		}

		for (int i = 0; i < MAX_SUNS; i++)
			if (!strcmp(f->value, sunTarget[i]))
			{
				// this spotlight used as sun target
				if (sunLight[i])
					sunLight[i]->spotDir = slight->spotDir;
				// remove from list
				bspfile.slights = slight->next;
			}
	}

	// spotlight with specified direction
	// NOTE: qrad3 have "light_spot" entity support, but game will display error "no spawn function"
	if (!strcmp(classname + 5, "_spot") || FindField("_spotvector,_spotpoint,_mangle,_spotangle") || slight->sun)
	{
		CVec3	vec;

		slight->spot = true;
		if (f = FindField("_spotvector"))
			GetVector(f->value, slight->spotDir);
		else if (f = FindField("_spotpoint"))
		{
			GetVector(f->value, vec);
			VectorSubtract(vec, slight->origin, vec);
			VectorNormalize(vec, slight->spotDir);
		}
		else if (f = FindField("_mangle,_spotangle"))
		{
			CVec3	ang;
			ang.Zero();
			sscanf(f->value, "%f %f", &ang[YAW], &ang[PITCH]);
			ang[PITCH] *= -1;
			Euler2Vecs(ang, &slight->spotDir, NULL, NULL);
		}
		else
		{
			CVec3	ang;
			float	angle;
			ang.Zero();

			if (bspfile.type == map_hl && (f = FindField("angles")))	// ZHLT
				sscanf(f->value, "%f %f", &ang[YAW], &ang[PITCH]);

			angle = 0;
			if (f = FindField("angle"))			// yaw
				angle = atof(f->value);
			if (angle == -1)					// ANGLE_UP
				slight->spotDir.Set(0, 0, 1);
			else if (angle == -2)				// ANGLE_DOWN
				slight->spotDir.Set(0, 0, -1);
			else
			{
				if (!angle) angle = ang[YAW];
				angle *= M_PI / 180.0f;
				slight->spotDir.Set(cos(angle), sin(angle), 0);
			}
			// HL have "pitch" support (this code is clean, so - keep it for all map types)
			angle = 0;
			if (f = FindField("pitch"))
				angle = atof(f->value);
			if (!angle)
				angle = ang[PITCH];
			if (angle)
			{
				angle *= M_PI / 180.0f;
				float tmp = cos(angle);
				slight->spotDir[0] *= tmp;
				slight->spotDir[1] *= tmp;
				slight->spotDir[2] = sin(angle);
			}
		}
	}

	// additional spotlight params
	if (slight->spot)
	{
		if (!slight->spotDot)		// may be set by q3 parser
		{
			float	cone;

			if (f = FindField("_cone"))
				cone = atof(f->value);
			else
				cone = 10;			// default

			// Q1 bsp uses "angle" field for cone; default = 20
			if (bspfile.type == map_q1)
			{
				cone = 0;
				if (f = FindField("angle"))
					cone = atof(f->value) / 2;
				if (!cone) cone = 20;
			}

			slight->spotDot = cos(cone * M_PI / 180.0f);
		}

		if (f = FindField("_focus"))
			slight->focus = atof(f->value);
		else
			slight->focus = 1;
	}

	// additiona Quake3 processing (light type may be changed during processing above)
	if (bspfile.type == map_q3)
	{
		if (slight->type == sl_linear)
			slight->intens /= 8000;		// linearScale in q3map
	}

	if ((!classname[5] && style < 32) || classname[5])
		return false;		// remove: "lightflare", non-switchable "light" etc

	return true;

	unguard;
}


/*-----------------------------------------------------------------------------
	Replacing/adjusting items
-----------------------------------------------------------------------------*/

#define CHANGE(orig, new)			\
	if (!strcmp(classname, orig))	\
		strcpy(classNameField->value, new);
#define REMOVE(orig)				\
	if (!strcmp(classname, orig))	\
		return false;

static bool ProcessEntity1()
{
	bool resetSpawnflags = false;
	bool shiftUp         = false;

	// teleport
	if (!strcmp(classname, "trigger_teleport"))
	{
		strcpy(classNameField->value, "misc_teleporter");
		if (entModel)
		{
			CVec3 org;
			entModel->bounds.GetCenter(org);
			org[2] = entModel->bounds.mins[2] - 17;
			AddField("origin", va("%g %g %g", VECTOR_ARG(org)));
		}
	}
	else if (!strcmp(classname, "info_teleport_destination"))
	{
		strcpy(classNameField->value, "misc_teleporter_dest");
		entOrigin[2] += 8;
		strcpy(originField->value, va("%g %g %g", VECTOR_ARG(entOrigin)));
	}
	else if (!strncmp(classname, "func_door", 9) && !(spawnflags & 4))
	{
		// implement linked doors
		if (numDoors >= MAX_DOORS)
		{
			appWPrintf("MAX_DOORS reached\n");
			return true;
		}
		// remember door
		strcpy(doors[numDoors].className, classname);
		doors[numDoors].modelIdx  = modelIdx;
		doors[numDoors].linkModel = -1;
		// find touched door
		int masterModel = modelIdx;
		CBox thisDoor = bspfile.models[modelIdx].bounds;
		thisDoor.Inflate(1);				// cannot do this when loading map (see ProcessQ1BspFile())
		for (int i = 0; i < numDoors; i++)	// do not compare with self
		{
			if (strcmp(doors[i].className, classname) != 0)	// class differs
				continue;
			CBox masterDoor = bspfile.models[doors[i].modelIdx].bounds;
			masterDoor.Inflate(1);
			if (thisDoor.Intersects(masterDoor))
			{
				masterModel = doors[i].linkModel;
				if (masterModel == -1) masterModel = doors[i].modelIdx;
				doors[numDoors].linkModel = masterModel;
				break;
			}
		}

		AddField("team", va("door_%d", masterModel));
		// and only now increment numDoors
		numDoors++;
	}
	// weapons/ammo
	// weapon_: q2 have: supershotgun, rocketlauncher, grenadelauncher
		//!! should reset spawnflags for them!
	else CHANGE("weapon_nailgun",      "weapon_machinegun")
	else CHANGE("weapon_supernailgun", "weapon_chaingun")
	else CHANGE("weapon_lightning",    "weapon_railgun")
	// ammo
	else CHANGE("item_shells",  "ammo_shells")		// shotguns
	else CHANGE("item_spikes",  "ammo_bullets")		// nailgun
	else CHANGE("item_rockets", "ammo_grenades")	// rocketlauncher, grenadelauncher : ammo_rockets, ammo_grenades
	//!! NOTE: q1 uses rockets for rocketlauncher and grenadelauncher
	//!! We should check: if map have grenadelauncher - use ammo_grenades,
	//!! otherwise use ammo_rockets
	else CHANGE("item_cells", "ammo_slugs")		// lightning

	// item_health
	else if (!strcmp(classname, "item_health"))
	{
		if (spawnflags == 1)						// small, "ROTTEN": 15 points
			strcpy(classNameField->value, "item_health_small");
		else if (spawnflags == 2)					// mega, "MEGA": 100 points w/o limit
			strcpy(classNameField->value, "item_health_mega");
		else										// normal, 25 points
			strcpy(classNameField->value, "item_health_large");
	}

	// item_weapon
	else if (!strcmp(classname, "item_weapon"))
	{
		if (spawnflags & 2)      // rockets
			strcpy(classNameField->value, "ammo_grenades");  //!! or rockets!
		else if (spawnflags & 4) // nailgun
			strcpy(classNameField->value, "ammo_bullets");
		else if (spawnflags & 1) // shotgun
			strcpy(classNameField->value, "ammo_shells");
	}

	// item_armor
	else CHANGE("item_armor1",   "item_armor_jacket")	// 0.3/100
	else CHANGE("item_armor2",   "item_armor_combat") 	// 0.6/150
	else CHANGE("item_armorInv", "item_armor_body") 	// 0.8/200

	else CHANGE("item_artifact_invulnerability", "item_invulnerability")
	else CHANGE("item_artifact_envirosuit", "item_enviro")
	// item_artifact_invisibility
	else CHANGE("item_artifact_super_damage", "item_quad")
	// Q2: item_breather, item_silencer, item_adrenaline, item_bandolier, item_pack
	// Q2: key_data_cd, key_power_cube, key_pyramid, key_data_spinner, key_pass,
	// Q2: key_blue_key, key_red_key, key_oommander_head, key_airstrike_target
	else REMOVE("info_intermission")

//	else CHANGE("trigger_secret", "target_secret")	//?? not works

//	else CHANGE("trigger_changelevel", "target_changelevel")

	// note: have few items, which do not require conversion
	// but should fix item position
	// also remove spawnflags for weapons/items and correct their position
	// to avoid "droptofloor: startsolid" problem
	if (!strncmp(classname, "weapon_", 7) ||
		!strncmp(classname, "item_", 5)   ||
		!strncmp(classname, "ammo_", 5))
	{
		resetSpawnflags = true;
		shiftUp         = true;
	}

	if (resetSpawnflags && spawnflagsField)
		strcpy(spawnflagsField->value, "0");
	//?? changed 9.12.06 -- rename 'shiftUp', remove shiftUp+resetSpawnflags from CHANGE0() macro ...
	if (shiftUp)
	{
		// try to move entity out of the walls
		bool done = false;
		CVec3 org;
		org = entOrigin;
		entOrigin[2] += 10;
		for (int tries = 0; tries < 30; tries++)
		{
			static const CBox bounds = {{-20,-20,-20}, {20,20,20}}; // taken from game::droptofloor()
			trace_t tr;
			CM_BoxTrace(tr, org, org, bounds, 0, MASK_SOLID);
			if (!tr.allsolid)
			{
//				appPrintf(S_YELLOW"...... ok!\n");
				done = true;
				break;
			}
#if 0
			//?? try to compute distance
			CBox bounds1;
			bounds1 = bounds;
			bounds1.mins.Add(org);
			bounds1.maxs.Add(org);
			appPrintf("%s[%d] : {%g %g %g} += {%g %g %g}+%g  (%g)\n", classname, tries, VECTOR_ARG(org), VECTOR_ARG(tr.plane.normal), tr.plane.dist,
			tr.plane.DistanceTo(bounds1));
#endif
			//?? try to compute distance between bounds1.GetVertex(tr.plane.signbits) and plane to shift from this plane with
			//?? a single iteration
			VectorMA(org, 2, tr.plane.normal);
		}
		if (done)
		{
			entOrigin = org;
			strcpy(originField->value, va("%g %g %g", VECTOR_ARG(entOrigin)));
		}
	}

	return true;
}


static bool ProcessEntityHL()
{
	entField_t *f;

	color_t entColor;
	entColor.rgba = RGBA(1,1,1,1);
	// read render properties
	if (f = FindField("rendercolor"))
	{
#if 0
		-- used for few "rendermode" types only; not for brush models!
		int c[3];
		if (sscanf(f->value, "%f %f %f", VECTOR_ARG(&c)) != 3)
			c[0] = c[1] = c[2] = 255;
		entColor.c[0] = c[0];
		entColor.c[1] = c[1];
		entColor.c[2] = c[2];
#endif
		RemoveField("rendercolor");
	}
	if (f = FindField("renderamt"))
	{
		entColor.c[3] = atoi(f->value);
		RemoveField("renderamt");
	}
	if (f = FindField("rendermode"))
	{
		int rendermode = atoi(f->value);
		if (rendermode == 0) entColor.c[3] = 255;	// ignore renderamt in rendermode 0 ("normal")
		RemoveField("rendermode");
	}
	else
		entColor.c[3] = 255;						// default rendermode == 0
	RemoveField("renderfx");						//?? use this field

	//?? check: is it possible, that one inline model used by few entities?
	//?? if so, cannot use CMODEL_LOCAL flag for this model!
	if (entModel)
	{
		entModel->color = entColor;
		// check ability to remove entity
		bool canRemove = false;
		if (!spawnflagsField && !FindField("*target*"))
			canRemove = true;
		else if (f = FindField("dmg"))
			canRemove = (strcmp(f->value, "0") == 0);
		if (!strcmp(classname, "func_ladder"))
		{
			entModel->flags |= CMODEL_CONTENTS|CMODEL_NODRAW; // mark model
			entModel->contents = CONTENTS_LADDER|CONTENTS_SOLID;
			if (canRemove)
			{
				entModel->flags |= CMODEL_WALL;
				return false;
			}
			strcpy(classNameField->value, "func_wall"); // else - replace entity
		}
		else if (!strcmp(classname, "func_water"))
		{
			if (f = FindField("skin"))
			{
				int type = atoi(f->value);
				switch (type)
				{
				case Q1_CONTENTS_WATER:
					entModel->contents = CONTENTS_WATER;
					break;
				case Q1_CONTENTS_SLIME:
					entModel->contents = CONTENTS_SLIME;
					break;
				case Q1_CONTENTS_LAVA:
					entModel->contents = CONTENTS_LAVA;
					break;
				default:
					Com_DPrintf("func_water: bad skin=%s\n", f->value);
					entModel->contents = CONTENTS_SOLID;
				}
				entModel->flags |= CMODEL_CONTENTS;
			}
			//?? process additional fields
			if (canRemove)
			{
				entModel->flags |= CMODEL_WALL|CMODEL_SHOW;
				return false;
			}
		}
		else if (!strcmp(classname, "func_wall"))
		{
			if (canRemove)
			{
				entModel->flags |= CMODEL_WALL|CMODEL_SHOW;
				return false;
			}
		}
		else if (!strcmp(classname,"func_conveyor"))
		{
			// quake2 game have "func_conveyor", but its implementation is not working;
			// and I have not found any quake2 map with this entity ...
			// So, we should replace half-life entity with something static
			if (spawnflagsField) strcpy(spawnflagsField->value,"0");	//??
			//?? spawnflags & 2 -> visual only (non-solid)
			// "angle" have encoded CONTENTS_CURRENT_XXX
			if (f = FindField("angle"))
			{
				static const struct {
					int angle;
					unsigned contents;
				} angleCont[] = {
					{ -1, CONTENTS_CURRENT_UP},			// ANGLE_UP
					{ -2, CONTENTS_CURRENT_DOWN},		// ANGLE_DOWN
					{  0, CONTENTS_CURRENT_0},
					{ 90, CONTENTS_CURRENT_90},
					{180, CONTENTS_CURRENT_180},
					{270, CONTENTS_CURRENT_270}
				};
				int a = atoi(f->value);
				unsigned cont = 0;
				for (int i = 0; i < ARRAY_COUNT(angleCont); i++)
					if (angleCont[i].angle == a)
					{
						cont = angleCont[i].contents;
						break;
					}
				if (!cont) Com_DPrintf("func_conveyor: unknown angle %d\n", a);
				entModel->flags |= CMODEL_CONTENTS;
				entModel->contents = cont|CONTENTS_SOLID;
			}
			if (spawnflags & 1)							// "not push" flag
				entModel->contents &= ~MASK_CURRENT;
			if (spawnflags & 2)							//?? may be, WALL, but non-SOLID ? (allow OR of other contents)
			{
				entModel->flags |= CMODEL_SHOW;			// visual only
				entModel->contents &= ~CONTENTS_SOLID;
			}
			else
			{
				entModel->flags |= CMODEL_WALL|CMODEL_SHOW;
			}
			entModel->flags |= CMODEL_SCROLL;
			//?? flag: SURF_FLOWING for this entity
			return false;								// do not send to game
		}
		else if (!strcmp(classname, "func_rotating"))
		{
			if (spawnflags & 64)						// non-solid
			{
				entModel->flags |= CMODEL_CONTENTS;
				entModel->contents = CONTENTS_MIST;
			}
			// spawnflags & 32 -> pain
			if (spawnflags & (64|128))
			{
				spawnflags &= ~(64|128);					// no in HL
				appSprintf(ARRAY_ARG(spawnflagsField->value), "%d", spawnflags);
			}
		}
		else if (!strcmp(classname, "trigger_push"))
		{
			if (!FindField("angle") && !FindField("angles"))
				AddField("angles", "0 0 0");
			return false;		//?? cannot get this to work
		}
	}
	if (!strcmp(classname, "env_sprite") || !(strcmp(classname, "env_glow")))
	{
		// convert standard effects
		// default effect is "glow"
		const char *mdl;
		if (modelField)
		{
			mdl = modelField->value;
			if (strncmp(mdl, "sprites/", 8) != 0)
			{
				Com_DPrintf("env_sprite: non-sprite model\n");
				return false;
			}
			mdl += 8;									// skip "sprites/"
		}
		else
			mdl = "glow";								// hack ... default model
		// create lightflare
		if (strstr(mdl, "flare") || strstr(mdl, "glow"))
		{
			color_t c;
			c.rgba = entColor.rgba;
			if (!strncmp(mdl, "blue", 4))
				c.rgba = RGB255(0, 192, 255);
			else if (!strncmp(mdl, "red", 3))
				c.rgba = RGB255(255, 0, 64);
			else if (!strncmp(mdl, "yel", 3))
				c.rgba = RGB255(255, 192, 0);

			lightFlare_t *flare = new (bspfile.extraChain) lightFlare_t;
			flare->next = bspfile.flares;
			bspfile.flares = flare;
			bspfile.numFlares++;
			flare->size  = 48;
			flare->style = 0;
			flare->color.rgba = c.rgba;
			flare->origin = entOrigin;
		}
		return false;
	}
	// func_breakable
	else if (!strcmp(classname, "func_breakable"))
	{
		//!!
//		strcpy(classNameField->value, "func_explosive");
		strcpy(classNameField->value, "func_wall");
		if (spawnflagsField) strcpy(spawnflagsField->value, "0");
	}

#define CHANGE0(orig, new)			\
	if (!strcmp(classname, orig))	\
	{								\
		strcpy(classNameField->value, new); \
		shiftUp = true;				\
	}
	bool shiftUp = false;

	// weapons
	CHANGE0("weapon_handgrenade", "ammo_grenades")
	else CHANGE0("weapon_tripmine", "ammo_grenades")
	else CHANGE0("weapon_rpg", "weapon_rocketlauncher")
	else CHANGE0("ammo_rpgclip", "ammo_rockets")
	else CHANGE0("weapon_crossbow", "weapon_railgun")
	else CHANGE0("ammo_crossbow", "ammo_slugs")
	else CHANGE0("weapon_gauss", "weapon_hyperblaster")
	else CHANGE0("ammo_gaussclip", "ammo_cells")
	else CHANGE0("weapon_9mmAR", "weapon_machinegun") // machinegun with grenadelauncher
	else CHANGE0("ammo_9mmclip", "ammo_bullets")
	else CHANGE0("ammo_9mmAR", "ammo_bullets") // grenades?
	else CHANGE0("weapon_shotgun", "weapon_supershotgun")
	else CHANGE0("ammo_buckshot", "ammo_shells")
	else CHANGE0("weapon_satchel", "ammo_grenades") // bomb with radio-controlled explosion
	// items
	else CHANGE0("item_healthkit", "item_health_large")
	else CHANGE0("item_battery", "item_armor_shard")
	// info
	else REMOVE("info_intermission")
	else REMOVE("infodecal")	//!! implement

	if (shiftUp)
	{
		entOrigin[2] += 20;
		strcpy(originField->value, va("%g %g %g", VECTOR_ARG(entOrigin)));
	}

	return true;
#undef CHANGE0
}


static bool ProcessEntityKP()
{
	// check entities to remove
	if (!strcmp(classname, "junior"))
		return false;	// KP "junior" entity

	/*----- check entities with RF2_SURF_ALPHA flags ------*/
	int chk = 0;
	if (!strcmp(classname, "func_wall"))
		chk = 32;
	else if (!strcmp(classname, "func_door"))
		chk = 128;
	else if (!strcmp(classname, "func_door_rotating"))
		chk = 4;
	else if (!strcmp(classname, "func_explosive") || !strcmp(classname, "func_train") || !strcmp(classname, "func_train_rotating"))
		chk = 8;

	if ((chk & spawnflags) && entModel)
		entModel->flags |= CMODEL_ALPHA;

	// convert func_lift to func_train
	CHANGE("func_lift", "func_train");

	//!! convert KP items

	return true;
}


static void ParseWorldSpawn()
{
	entField_t *f;
	// sky
	if (bspfile.type == map_kp && !FindField("sky"))
		AddField("sky", "sr");		// set default Kingpin sky
	if (bspfile.type == map_q1 || bspfile.type == map_q3)
		AddField("sky", "default");	// do not allow q2 game to set default sky
	if (bspfile.type == map_hl && (f = FindField("skyname")))
		strcpy(f->name, "sky");
	// map message
	if (f = FindField("message"))
	{
		// replace
		char copy[256];
		char *s = f->value, *d = copy;
		while (char c = *s++)
		{
//			appPrintf("[%c == %02X  -> ", c, c);
			if (bspfile.type == map_q1)
			{
				// 0x12 - 10 chars of '0'..'9'
				if (c >= 0x12 && c <= 0x12+9)
					*d++ = c - 0x12 + '0' + 0x80;
				else
					*d++ = c;
			}
			else
				*d++ = c;
//			appPrintf("[%c == %02X\n", d[-1], d[-1]);
		}
		*d = 0;
		strcpy(f->value, copy);
	}
	// wads
	if (bspfile.type == map_hl && (f = FindField("wad")))
	{
		bspfile.Wads[0] = 0;
		// parse field: remove path and extension
		for (TStringSplitter<64, ';'> wad = f->value; wad; ++wad)
		{
			if (!wad[0]) continue;
			char   *s = strrchr(wad, '/');
			if (!s) s = strrchr(wad, '\\');
			if (!s) s = wad;
			else	s++;								// skip path separator
			if (char *ext = strchr(s, '.')) *ext = 0;	// cut extension
			if (bspfile.Wads[0]) bspfile.Wads += ";";
			bspfile.Wads += s;
		}
		// remove field
		f->name[0] = 0;
	}
	// kp fog
	if (f = FindField("fogval"))
	{
		GetVector(f->value, bspfile.fogColor);
		if (f = FindField("fogdensity"))
		{
			bspfile.fogDens = atof(f->value);
			bspfile.fogMode = fog_exp;
		}
		RemoveField("fogval");
		RemoveField("fogdensity");
		// Voodoo fog params
		RemoveField("fogval2");
		RemoveField("fogdensity2");
	}
//	if (f = FindField("_ambient,light,_minlight")) -- it seems, this is not always true ... detect ambient by outselves (in renderer)
//		GetVector(f->value, bspfile.ambientLight);
	// sun
	for (int sunNum = 0; sunNum < MAX_SUNS; sunNum++)
	{
		const char *sunSuffix[] = {
			"", "2", "3", "4"
		};
		const char *suffix = sunSuffix[sunNum];
		if (!FindField(va("_sun%s,_sun%s_*", suffix, suffix)))
			break;
		// alloc new light
		slight_t *sun = new (bspfile.extraChain) slight_t;
		sun->next = bspfile.slights;
		bspfile.slights = sun;
		bspfile.sunCount++;
		sun->sun = true;
		// have ArghRad sun
		// get sun direction
		sun->spotDir.Set(0, 0, -1);		// will be default even if incorrect sun with spotlight
		if (f = FindField(va("_sun%s_vector", suffix)))
		{
			GetVector(f->value, sun->spotDir);
			sun->spotDir.NormalizeFast();
		}
		else if (f = FindField(va("_sun%s", suffix)))
		{
			strcpy(sunTarget[sunNum], f->value);	// worldspawn always 1st in entstring, so - just remember suntarget
			sunLight[sunNum] = sun;
		}
		else if (f = FindField(va("_sun%s_angle,_sun%s_mangle", suffix, suffix)))
		{
			CVec3 ang;
			sscanf(f->value, "%f %f", &ang[YAW], &ang[PITCH]);
			ang[PITCH] *= -1;
			Euler2Vecs(ang, &sun->spotDir, NULL, NULL);
		}
		// get sun color info
		if (f = FindField(va("_sun%s_color", suffix)))
		{
			GetVector(f->value, sun->color);
			NormalizeColor(sun->color, sun->color);
		}
		if (f = FindField(va("_sun%s_light", suffix)))
			sun->intens = atof(f->value);
		else
			sun->intens = 200;				// default
	}
	// global sun parameters
	if (f = FindField("_sun_ambient"))
		GetVector(f->value, bspfile.sunAmbient);
	if (f = FindField("_sun_surface"))
		GetVector(f->value, bspfile.sunSurface);
}


// our map script (temp !! better to use inline shaders/templates?)
static void ParseSurfparam()
{
	entField_t *f;

	if (!(f = FindField("name")))
	{
		Com_DPrintf("surfparam without \"name\"\n");
		return;
	}
	const char *name = f->value;

	if (!(f = FindField("flags")))
	{
		Com_DPrintf("no flags specified for \"%s\"\n", name);
		return;
	}
	int flags = atoi(f->value);

	int testflags, testmask;
	if (f = FindField("inflags"))
		sscanf(f->value, "%d %d", &testmask, &testflags);
	else
		testflags = testmask = 0;

	bool found = false;
	int i;
	dBsp2Texinfo_t *d;
	for (i = 0, d = bspfile.texinfo2; i < bspfile.numTexinfo; i++, d++)	//?? note: does not works with Q1/HL maps
		if (appMatchWildcard(d->texture, name, true) && ((d->flags & testmask) == testflags))
		{
			d->flags = d->flags & ~testmask | flags;
			found = true;
			// do not "break": process all matching surfaces
		}
	if (!found)
		Com_DPrintf("texinfo %s is not found\n", name);
}


/*-----------------------------------------------------------------------------
	Parsing entities
-----------------------------------------------------------------------------*/

#if ENT_STATS
static char entClassNames[128][64];
static int  entClassCounts[128];
static int  entClassModelCounts[128];
int numEntClassNames;
#endif


// Returns "true" if entity should be passed to game
static bool ProcessEntity()
{
	/*------------------ get some fields -------------------*/

	// get classname
	classNameField = FindField("classname");
	classname      = classNameField ? classNameField->value : "";

#if REMOVE_ENTS
	EXEC_ONCE(appPrintf(S_RED"REMOVING MOST ENTITIES\n"));
	if (!memcmp(classname, "func_", 5) || !memcmp(classname, "weapon_", 7) || !memcmp(classname, "trigger_", 8) ||
		!memcmp(classname, "env_", 4) || !memcmp(classname, "ammo_", 5) || !memcmp(classname, "item_", 5)) return false;
#endif

	// get spawnflags
	spawnflagsField = FindField("spawnflags");
	spawnflags      = spawnflagsField ? atoi(spawnflagsField->value) : 0;
#if 0
#define SPAWNFLAG_NOT_DEATHMATCH	0x800
	if (Cvar_VariableInt("keep_sp") && (spawnflags & SPAWNFLAG_NOT_DEATHMATCH))//!!
	{
		spawnflags &= ~SPAWNFLAG_NOT_DEATHMATCH;
		appSprintf(ARRAY_ARG(f->value), "%d", spawnflags);
	}
#endif
	// get origin
	if (originField = FindField("origin"))
		GetVector(originField->value, entOrigin);
	else
		entOrigin.Zero();

	// get inline model
	entModel = NULL;
	if (modelField = FindField("model"))
	{
		//!! note: HL can use different types of models here (non-inline)
		if (sscanf(modelField->value, "*%d", &modelIdx) == 1)
		{
			if (modelIdx >= bspfile.numModels)
				Com_DPrintf("invalid model index %d\n", modelIdx);
			else
			{
				entModel = &bspfile.models[modelIdx];
				if (originField)		//!! WARNING: collision is not supported with biased origin
					entModel->origin = entOrigin;
			}
		}
	}

#if ENT_STATS
	{
		int i;
		for (i = 0; i < numEntClassNames; i++)
			if (!strcmp(entClassNames[i], classname)) break;
		if (i >= numEntClassNames)
		{
			// not appeared before
			if (numEntClassNames >= ARRAY_COUNT(entClassCounts)-1)
			{
				i = 0;
				strcpy(entClassNames[0], "<< OVERFLOW >>");
			}
			numEntClassNames++;
			strcpy(entClassNames[i], classname);
			entClassCounts[i]      = 0;
			entClassModelCounts[i] = 0;
		}
		entClassCounts[i]++;
		if (entModel) entClassModelCounts[i]++;
	}
#endif

	/*----------- check movable inline models -----------*/

	if (entModel && appMatchWildcard(classname, "func_plat,func_*rotating,func_door,func_train"))
		entModel->flags |= CMODEL_MOVABLE;

	/*---------------- get lighting info ----------------*/

	if (!memcmp(classname, "light", 5))
		return LoadLight();

	/*---------------------------------------------*/

	if (!strcmp(classname, "worldspawn"))
	{
		ParseWorldSpawn();
		return true;
	}

	if (!strcmp(classname, "surfparam"))
	{
		ParseSurfparam();
		return false;
	}

	switch (bspfile.type)
	{
	case map_q1:
		if (!ProcessEntity1()) return false;
		break;
	case map_hl:
		if (!ProcessEntityHL()) return false;
		break;
	case map_kp:
		if (!ProcessEntityKP()) return false;
		break;
	}

	// q1/hl
	if (entModel && !strcmp(classname, "func_illusionary"))
	{
		entModel->flags |= CMODEL_SHOW;
		return false;									// use in renderer only
	}

	/*---------------------------------------------*/

	if (!strcmp(classname, "target_splash"))
	{
		if (originField)
		{
			splash_t *spl = new (bspfile.extraChain) splash_t;
			spl->next = bspfile.splashes;
			bspfile.splashes = spl;
			spl->origin = entOrigin;
		}

		return true;
	}

#if 0
	appPrintf("------------\n");
	for (i = 0; i < numEntFields; i++)
		appPrintf("\"%s\"=\"%s\"\n", entity[i].name, entity[i].value);
#endif

	return true;
}


const char *ProcessEntstring(const char *entString)
{
	guard(ProcessEntstring);

	unsigned plen;
	char *patch = (char*) GFileSystem->LoadFile(va("%s.add", *bspfile.Name), &plen);
	plen++;	// add 1 byte for trailing zero

	char *dst, *dst2;
	dst = dst2 = (char*) bspfile.extraChain->Alloc(strlen(entString) + 1 + plen);

#if ENT_STATS
	numEntClassNames = 0;
#endif

	// find all target entities
	const char *src = entString;
	numTargets = numDoors = 0;
	while (!haveErrors && ReadEntity(src))
		ProcessEntityTarget();

	// parse main entstring
	src = entString;
	haveErrors = false;
	memset(sunTarget, 0, sizeof(sunTarget));
	memset(sunLight, 0, sizeof(sunLight));
	while (!haveErrors && ReadEntity(src))
	{
		if (ProcessEntity())
			WriteEntity(&dst);
	}

#if ENT_STATS
	Com_DPrintf("---- Entity statistics: ----\n-n--ct-mdl-name-------\n");
	for (int num = 0; num < numEntClassNames; num++)
		Com_DPrintf("%2d %3d %3d %s\n", num+1, entClassCounts[num], entClassModelCounts[num], entClassNames[num]);
#endif

	// parse patch
	if (patch)
	{
		Com_DPrintf("Adding entity patch ...\n");
		src = patch;
		while (!haveErrors && ReadEntity(src))
		{
			if (ProcessEntity())
				WriteEntity(&dst);
		}
		delete patch;
	}

	if (haveErrors)
		return entString;

	Com_DPrintf("ProcessEntstring: size = %d -> %d\n", strlen(entString), dst - dst2);
	return dst2;

	unguard;
}
