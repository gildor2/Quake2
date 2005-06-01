#include "client.h"

//!! move gender-by-model selection code here (if not server-side)
//!! model-specific sounds placed in sound/player/[model]/*.wav
//!! implement player-selection menus with this API

/*-----------------------------------------------------------------------------
	Scanning for player models
-----------------------------------------------------------------------------*/

static CMemoryChain *pmiChain;
TList<playerModelInfo_t> pmiList;
int numPlayerModels;


static bool IsValidSkin (const char *skin, CStringItem *pcxfiles)
{
	const char *ext = strrchr (skin, '.');
	if (!ext) return false;

	if (stricmp (ext, ".pcx") && stricmp (ext, ".tga") && stricmp (ext, ".jpg"))
		return false;		// not image

	// modelname/skn_* have no icon
	const char *name = strrchr (skin, '/');
	if (!name) return false;
	else name++;			// skip '/'

	if (!memcmp (name, "skn_", 4))
		return true;
	char scratch[MAX_OSPATH];
	strcpy (scratch, skin);
	strcpy (scratch + (ext - skin), "_i.pcx");

	for (CStringItem *item = pcxfiles; item; item = item->next)
		if (!strcmp (item->name, scratch)) return true;
	return false;
}


void FreePlayerModelsInfo ()
{
	if (pmiChain)
	{
		delete pmiChain;
		pmiChain = NULL;
	}
	numPlayerModels = 0;
	pmiList.Reset();
}


static void ScanQuake2Models ()
{
	/*----- get a list of directories -----*/
	const char *path = NULL;
	TList<CStringItem> dirnames;
	while (path = FS_NextPath (path))
	{
		dirnames = FS_ListFiles (va("%s/players/*.*", path), LIST_DIRS);
		if (dirnames.First()) break;
	}
	if (!dirnames.First()) return;

	/*--- go through the subdirectories ---*/
	for (CStringItem *diritem = dirnames.First(); diritem; diritem = dirnames.Next(diritem))
	{
		// verify the existence of tris.md2
		if (!FS_FileExists (va("%s/tris.md2", diritem->name)))
			continue;

		// verify the existence of at least one pcx skin
		// "/*.pcx" -> pcx,tga,jpg (see IsValidSkin())
		TList<CStringItem> skinNames = FS_ListFiles (va("%s/*.*", diritem->name), LIST_FILES);
		if (!skinNames.First()) continue;

		// count valid skins, which consist of a skin with a matching "_i" icon
		TList<CStringItem> skins;
		int numSkins = 0;
		for (CStringItem *skinItem = skinNames.First(); skinItem; skinItem = skinNames.Next(skinItem))
			if (IsValidSkin (skinItem->name, skinNames.First()))
			{
				char *str = strrchr (skinItem->name, '/') + 1;
				char *ext = strrchr (str, '.');
				ext[0] = 0;
				skins.CreateAndInsert (str, pmiChain);
				numSkins++;
			}
		skinNames.Free();
		if (!numSkins) continue;

		// create model info
		playerModelInfo_t *info = new (strrchr (diritem->name, '/')+1, pmiChain) playerModelInfo_t;
		info->numSkins = numSkins;
		info->skins = skins;
		// add model info to pmi
		pmiList.Insert (info);

		numPlayerModels++;
	}
	dirnames.Free();
}


void ScanQuake3Models ()
{
	/*----- get a list of directories -----*/
	const char *path = NULL;
	TList<CStringItem> dirnames;
	while (path = FS_NextPath (path))
	{
		dirnames = FS_ListFiles (va("%s/models/players/*.*", path), LIST_DIRS);
		if (dirnames.First()) break;
	}
	if (!dirnames.First()) return;

	/*--- go through the subdirectories ---*/
	for (CStringItem *diritem = dirnames.First(); diritem; diritem = dirnames.Next(diritem))
	{
		// verify the existence of animation.cfg
		if (!FS_FileExists (va("%s/animation.cfg", diritem->name)))
			continue;

		// verify the existence of at least one skin file
		TList<CStringItem> skinNames = FS_ListFiles (va("%s/icon_*", diritem->name), LIST_FILES);
		if (!skinNames.First()) continue;

		// count valid skins, which consist of a skin with a matching "_i" icon
		TList<CStringItem> skins;
		int numSkins = 0;
		for (CStringItem *skinItem = skinNames.First(); skinItem; skinItem = skinNames.Next(skinItem))
		{
			char *str = strrchr (skinItem->name, '/') + 1 /*skip '/'*/ + 5 /*skip "icon_"*/;
			char *ext = strrchr (str, '.');
			ext[0] = 0;
			skins.CreateAndInsert (str, pmiChain);
			numSkins++;
		}
		skinNames.Free();
		if (!numSkins) continue;

		// create model info
		playerModelInfo_t *info = new (strrchr (diritem->name, '/')+1, pmiChain) playerModelInfo_t;
		info->numSkins = numSkins;
		info->skins = skins;
		info->isQ3mdl = true;
		// add model info to pmi
		pmiList.Insert (info);

		numPlayerModels++;
	}
	dirnames.Free();
}


bool ScanPlayerModels ()
{
	FreePlayerModelsInfo ();
	pmiChain = new CMemoryChain;

	ScanQuake2Models ();
	ScanQuake3Models ();

	if (!numPlayerModels)
	{
		FreePlayerModelsInfo ();
		return false;
	}

	return true;
}


/*=============================================================================

	In-game player model drawing/loading code

=============================================================================*/

// WARNING: OpenGLDrv creates model_t for absent files
// ... so, we use wrapper functions
static CRenderModel *FindQ2Model (const char *name, const char *part)
{
	char filename[MAX_QPATH];
	appSprintf (ARRAY_ARG(filename), "players/%s/%s.md2", name, part);
	if (!FS_FileExists (filename)) return false;
	return RE_RegisterModel (filename);
}

static CRenderModel *FindQ3Model (const char *name, const char *part)
{
	char filename[MAX_QPATH];
	appSprintf (ARRAY_ARG(filename), "models/players/%s/%s.md3", name, part);
	if (!FS_FileExists (filename)) return false;
	return RE_RegisterModel (filename);
}


/*-----------------------------------------------------------------------------
	Quake3 player model support
-----------------------------------------------------------------------------*/

static clientInfo_t *gci;

static void cAnimGender (int argc, char **argv)
{
// 'F','f' -> female, 'N','n' -> neutral, other - male
//	gci->gender =
}

static void cAnimFixedLegs (int argc, char **argv)
{
	gci->fixedLegs = true;
}

static void cAnimFixedTorso (int argc, char **argv)
{
	gci->fixedTorso = true;
}


static const CSimpleCommand animCommands[] = {
	{"footsteps",	NULL},
	{"headoffset",	NULL},				// used in Q3 for HUD only
	{"sex",			cAnimGender},
	{"fixedlegs",	cAnimFixedLegs},
	{"fixedtorso",	cAnimFixedTorso}
};

static void LoadAnimationCfg (clientInfo_t &ci, const char *filename)
{
	char *buf = (char*) FS_LoadFile (filename);
	gci = &ci;
	int animNumber = 0;
	animation_t *anims = ci.animations;
	int frameSkip = 0;

	const char *s = buf;
	while (true)
	{
		char line[1024];
		char *d = line;
		char c = 0;
		//!! use external parser
		while (d < line + sizeof(line) - 2)
		{
			c = *s++;
			if (c == '\r' || c == '\n' || !c)	// end of line
				break;
			if (c == '\t') c = ' ';				// tab -> space
			*d++ = c;
		}
		*d++ = 0;
		// cut "//" comments
		for (d = line; d[0]; d++)
			if (d[0] == '/' && d[1] == '/')
			{
				*d = 0;
				break;
			}
		// cut trailing spaces
		for (d = strchr (line, 0) - 1; d >= line; d--)
		{
			if (d[0] != ' ') break;
			d[0] = 0;
		}
		// execute line
		d = line;
		while (d[0] == ' ') d++;				// skip leading spaces
		if (d[0])
		{
			if (d[0] >= '0' && d[0] <= '9')		// numbers => frame numbers
			{
				if (animNumber >= MAX_ANIMATIONS)
				{
					Com_WPrintf ("Too much animations in \"%s\"\n", filename);
					break;
				}
				int firstFrame, numFrames, loopFrames;
				float fps;
				if (sscanf (d, "%d %d %d %f", &firstFrame, &numFrames, &loopFrames, &fps) != 4)
				{
					Com_WPrintf ("Invalid frame info [%s] in \"%s\"\n", d, filename);
					break;
				}
				// some processing on acquired data + store info
				if (animNumber == LEGS_WALKCR)
				{
					frameSkip = firstFrame - anims[TORSO_GESTURE].firstFrame;
					firstFrame = anims[TORSO_GESTURE].firstFrame;
				}
				else if (animNumber > LEGS_WALKCR && animNumber < TORSO_GETFLAG)
					firstFrame -= frameSkip;
				if (numFrames < 0)
				{
					numFrames = -numFrames;
					anims[animNumber].reversed = true;
				}
				anims[animNumber].firstFrame = firstFrame;
				anims[animNumber].numFrames  = numFrames;
				anims[animNumber].loopFrames = loopFrames;
				if (fps < 1) fps = 1;
				anims[animNumber].frameLerp = appRound (1000.0f / fps);
				animNumber++;
			}
			else if (!ExecuteCommand (d, ARRAY_ARG(animCommands)))
			{
				Com_WPrintf ("Invalid line [%s] in \"%s\"\n", d, filename);
				break;
			}
		}
		if (!c) break;							// end of file
	}

	// copy gestures when absent
	if (animNumber >= TORSO_GETFLAG && animNumber <= TORSO_NEGATIVE)
		for (int i = animNumber; i <= TORSO_NEGATIVE; i++)
		{
			anims[i] = anims[TORSO_GESTURE];
			anims[i].reversed = false;
		}

	// crouch backward animation
	anims[LEGS_BACKCR] = anims[LEGS_WALKCR];
	anims[LEGS_BACKCR].reversed = true;
	// walk backward animation
	anims[LEGS_BACKWALK] = anims[LEGS_WALK];
	anims[LEGS_BACKWALK].reversed = true;
	// Q3 have some processing of flag animations ...

	for (int i = 0; i < MAX_TOTALANIMATIONS; i++)
		if (anims[i].frameLerp == 0) anims[i].frameLerp = 100;	// just in case

	FS_FreeFile (buf);
}

static bool TryQuake3Model (clientInfo_t &ci, const char *modelName, const char *skinName)
{
	char animCfg[MAX_QPATH];
	appSprintf (ARRAY_ARG(animCfg), "models/players/%s/animation.cfg", modelName);
	if (!FS_FileExists (animCfg)) return false;

	ci.legsModel  = FindQ3Model (modelName, "lower");
	ci.torsoModel = FindQ3Model (modelName, "upper");
	ci.headModel  = FindQ3Model (modelName, "head");
	if (!ci.legsModel || !ci.torsoModel || !ci.headModel) return false;

	LoadAnimationCfg (ci, animCfg);
	// init animations to something
	ci.legsAnim.animNum = ci.legsAnim.nextAnimNum = LEGS_IDLE;
	ci.torsoAnim.animNum = ci.torsoAnim.nextAnimNum = TORSO_STAND;

	appSprintf (ARRAY_ARG(ci.iconName), "/models/players/%s/icon_%s", modelName, skinName);
	ci.icon = RE_RegisterPic (ci.iconName);

	//!! load/parse skin

	//!! change:
	ci.legsSkin = RE_RegisterSkin (va("models/players/%s/%s", modelName, skinName));
	ci.torsoSkin = ci.headSkin = ci.legsSkin;

	ci.isQ3model = true;
	ci.isValidModel = true;
	return true;
}


static void attach (const entity_t &e1, entity_t &e2, const char *tag)
{
	CCoords lerped;		// get position modifier
	e1.model->LerpTag (e1.frame, e1.oldframe, e1.backlerp, tag, lerped);
	e1.pos.UnTransformCoords (lerped, e2.pos);
}


/* Quake2 animation frames:
	name		frames	num	loop stop simple	torso		legs
	---------------------------------------------------------------------------
	stand	 	0-39	40	+			+		STAND[2]	IDLE
	run			40-45	6	+			+		STAND[2]	** (WALK,BACKWALK,RUN,BACK)
	attack		46-53	8						ATTACK[2]	**
	pain1		54-57	4						(angle)		**
	pain2		58-61	4						(angle)		**
	pain3		62-65	4						(angle)		**
	jump		66-71	6						IDLE		** (JUMP,LAND, JUMPB, LANDB)
	flip		72-83	12						GESTURE		**
	salute		84-94	11						GESTURE		**
	taunt		95-111	17						GESTURE		**
	wave		112-122	11						GESTURE		**
	point		123-134	12						GESTURE		**
	crstnd		135-153	19	+			+		STAND[2]	IDLECR
	crwalk		154-159	6	+			+		STAND[2]	WALKCR
	crattack	160-168	9						ATTACK[2]	** (IDLECR,WALKCR+BACKCR) !!+ attack in jump
	crpain		169-172	4						(angle)		----//----
	crdeath		173-177	5		  +		+		? (BOTH_DEATHn ?)
	death1		178-183	6		  +		+		BOTH_DEATH1->DEAD1
	death2		184-189	6		  +		+		BOTH_DEATH2->DEAD2
	death3		190-197	8		  +		+		BOTH_DEATH3->DEAD3
	---------------------------------------------------------------------------
	stop - stop on last frame (death animation)
	simple - basic animation, almost (or completely) idle; other (!simple) - may be combined with simple animation
	TORSO_STAND2, TORSO_ATTACK2 - for special weapons only (gauntlet)
	** - compute from movement
	(angle) - pain animation in Q3 made with torso movement
	JUMP,LAND - forward, JUMPB,LANDB - backward movement

	"crpain" and "pain3" (in std q2 game) may be used in reverse order to signal weapon dropping/changing
	"wave" may be used in reverse order for grenade throwing (1st 8 frames)
	"jump" used 3-6 frames; frame #2 - for "in air" state (static); strange code: game/p_view.c :: G_SetClientFrame()

	Unused in Q2, but present in Q3: LEGS_SWIM, LEGS_TURN, TORSO_DROP, TORSO_RAISE
*/

struct animInfo_t
{
	int		nextAnim;
};

static const animInfo_t animInfo[] = {
//	animation			next
/* BOTH_DEATH1 */	{	BOTH_DEAD1		},
/* BOTH_DEAD1 */	{	BOTH_DEAD1		},
/* BOTH_DEATH2 */	{	BOTH_DEAD2		},
/* BOTH_DEAD2 */	{	BOTH_DEAD2		},
/* BOTH_DEATH3 */	{	BOTH_DEAD3		},
/* BOTH_DEAD3 */	{	BOTH_DEAD3		},

/* TORSO_GESTURE */	{	TORSO_STAND		},

/* TORSO_ATTACK */	{	TORSO_STAND		},
/* TORSO_ATTACK2 */	{	TORSO_STAND2	},

/* TORSO_DROP */	{	TORSO_STAND		},
/* TORSO_RAISE */	{	TORSO_STAND		},

/* TORSO_STAND */	{	TORSO_STAND		},
/* TORSO_STAND2 */	{	TORSO_STAND2	},

/* LEGS_WALKCR */	{	LEGS_WALKCR		},
/* LEGS_WALK */		{	LEGS_WALK		},
/* LEGS_RUN */		{	LEGS_RUN		},
/* LEGS_BACK */		{	LEGS_BACK		},
/* LEGS_SWIM */		{	LEGS_SWIM		},

/* LEGS_JUMP */		{	LEGS_LAND		},	//??
/* LEGS_LAND */		{	LEGS_IDLE		},	//?? run, other
/* LEGS_JUMPB */	{	LEGS_LANDB		},	//??
/* LEGS_LANDB */	{	LEGS_IDLE		},	//??

/* LEGS_IDLE */		{	LEGS_IDLE		},
/* LEGS_IDLECR */	{	LEGS_IDLECR		},

/* LEGS_TURN */		{	LEGS_IDLE		},	//??

/* TORSO_GETFLAG */	{	TORSO_STAND		},
/* TORSO_GUARDBASE */{	TORSO_STAND		},
/* TORSO_PATROL */	{	TORSO_STAND		},
/* TORSO_FOLLOWME */{	TORSO_STAND		},
/* TORSO_AFFIRMATIVE */{TORSO_STAND		},
/* TORSO_NEGATIVE */{	TORSO_STAND		},

/* MAX_ANIMATIONS */{	0				},	// no such animation
/* LEGS_BACKCR */	{	LEGS_BACKCR		},
/* LEGS_BACKWALK */	{	LEGS_BACKWALK	},
#if 0
/* FLAG_RUN */		{	FLAG_RUN		},
/* FLAG_STAND */	{	FLAG_STAND		},
/* FLAG_STAND2RUN */{	FLAG_RUN		},
#endif
/* MAX_TOTALANIMATIONS */{0				}	// no such animation
};


static void SetQuake3Animation (clientInfo_t &ci, animState_t &as, int animNum, int curTime)
{
	// just in case ...
	if (animNum < 0 || animNum >= MAX_TOTALANIMATIONS) animNum = 0;
	// get animation
	animation_t *anim = &ci.animations[animNum];

	//!! check for correct animation changing w/o lerping (teleport, respawn ...)
	//!! can make this using teleport detection (significant changing of origin):
	//!! when detected, lerp frames from 0

	bool force = false;	//!! true when change animation w/o lerping; USE IT!!!

	int wholeAnimTime = anim->numFrames * anim->frameLerp;
	// check for next animation in sequence
	if (animNum == as.animNum && (curTime >= as.startTime + wholeAnimTime - anim->frameLerp))	// start next sequence 1 frame earlier
	{
		animNum = as.nextAnimNum;
		as.nextAnimNum = animInfo[animNum].nextAnim;
		anim = &ci.animations[animNum];
	}
	if (animNum != as.animNum)
	{
		// animation was changed
		if (force)
			as.startTime = curTime;
		else
		{
			animation_t *oldAnim = &ci.animations[as.animNum];
			int timeDelta = curTime - as.startTime;
			// get current frame frac
			float curLerp = 1.0f - as.nearLerp;
			as.oldFrame = as.nearFrame;
			// apply this frac for next animation
			as.startTime = curTime + appFloor ((1.0f - curLerp) * anim->frameLerp);
		}
		as.animNum = animNum;
		as.nextAnimNum = animInfo[animNum].nextAnim;
	}
	else
	{
		// continue animation
		// wrap frame start time (happens for looping animations only)
		if (curTime >= as.startTime + wholeAnimTime)
			as.startTime += (curTime - as.startTime) / wholeAnimTime * wholeAnimTime;
	}
}


static void RunQuake3Animation (clientInfo_t &ci, animState_t &as, entity_t &ent, int curTime)
{
	int timeDelta = curTime - as.startTime;

	SetQuake3Animation (ci, as, as.animNum, curTime);
	animation_t &anim = ci.animations[as.animNum];

	if (timeDelta < 0)
	{
		// HERE: timeDelta is in [-frameLerp .. 0] range
		// lerp from previous animation
		ent.backlerp = -timeDelta / (float)anim.frameLerp;
		ent.oldframe = as.oldFrame;
		ent.frame    = anim.reversed ? anim.firstFrame + anim.numFrames - 1 : anim.firstFrame;
	}
	else
	{
		if (anim.numFrames > 1)
		{
			// lerp current animation
			int oldFrame = timeDelta / anim.frameLerp;
			int frame    = oldFrame + 1;
			ent.backlerp = 1.0f - (float)(timeDelta % anim.frameLerp) / anim.frameLerp;
			// wrap frames
			if (oldFrame >= anim.numFrames) oldFrame = 0;
			if (frame >= anim.numFrames) frame = 0;
			// reverse when needed
			if (anim.reversed)
			{
				oldFrame = anim.numFrames - oldFrame - 1;
				frame    = anim.numFrames - frame - 1;
			}
			// store
			ent.oldframe = anim.firstFrame + oldFrame;
			ent.frame    = anim.firstFrame + frame;
		}
		else
		{
			ent.frame    = ent.oldframe = anim.firstFrame;
			ent.backlerp = 0;
		}
		// save oldframe
		as.oldFrame = ent.oldframe;
	}
	// save nearest frame params
	if (ent.backlerp > Cvar_VariableValue("test"))// 0.5f)
	{
		as.nearFrame = ent.oldframe;
		as.nearLerp  = ent.backlerp;
	}
	else
	{
		as.nearFrame = ent.frame;
		as.nearLerp  = 1 - ent.backlerp;
	}
}


/*-----------------------------------------------------------------------------
	Quake2 and common player model code
-----------------------------------------------------------------------------*/

void CL_LoadClientinfo (clientInfo_t &ci, const char *s)
{
	guard (CL_LoadClientinfo);

	char modelName[MAX_QPATH], skinName[MAX_QPATH];

	memset (&ci, 0, sizeof(clientInfo_t));

	// get player's name
	appStrncpyz (ci.name, s, sizeof(ci.name));
	char *t = strchr (ci.name, '\\');
	if (t)
	{
		*t = 0;
		s = t + 1;
	}

	if (cl_noskins->integer || *s == 0)
	{
		ci.md2model = FindQ2Model ("male", "tris");
		ci.md2skin = RE_RegisterSkin ("players/male/grunt");
		strcpy (ci.iconName, "/players/male/grunt_i");
		ci.icon = RE_RegisterPic (ci.iconName);
		ci.weaponmodel[0] = FindQ2Model ("male", "weapon");
	}
	else
	{
		// get model name
		strcpy (modelName, s);
		t = strchr (modelName, '/');
		if (!t) t = strchr (modelName, '\\');
		if (!t) t = modelName;
		*t = 0;

		// get skin name
		strcpy (skinName, t + 1);

		// try loading Quake3 model
		if (TryQuake3Model (ci, modelName, skinName))
			return;

		// model file
		ci.md2model = FindQ2Model (modelName, "tris");
		if (!ci.md2model)
		{
			strcpy (modelName, "male");
			ci.md2model = FindQ2Model ("male", "tris");
		}

		// skin file
		ci.md2skin = RE_RegisterSkin (va("players/%s/%s", modelName, skinName));

		// if we don't have the skin and the model wasn't male,
		// see if the male has it (this is for CTF's skins)
		if (!ci.md2skin && stricmp (modelName, "male"))
		{
			//?? find any skin, then (if no) change model to male (should be done for non-teamplay games only,
			//?? can detect this by using of only 2 skins for all players)
			// change model to male
			strcpy (modelName, "male");
			ci.md2model = FindQ2Model ("male", "tris");
			ci.md2skin = RE_RegisterSkin (va("players/%s/%s", modelName, skinName));
		}

		// if we still don't have a skin, it means that the male model didn't have
		// it, so default to grunt
		if (!ci.md2skin)
			// see if the skin exists for the male model
			ci.md2skin = RE_RegisterSkin (va("players/male/grunt"));

		// weapon file
		for (int i = 0; i < num_cl_weaponmodels; i++)
		{
			ci.weaponmodel[i] = RE_RegisterModel (va("players/%s/%s", modelName, cl_weaponmodels[i]));
			// HACK: cyborg have the weapon same models, as male model
			if (!ci.weaponmodel[i] && !strcmp(modelName, "cyborg"))
				ci.weaponmodel[i] = RE_RegisterModel (va("players/male/%s", cl_weaponmodels[i]));
			if (!cl_vwep->integer)
				break; // only one when vwep is off
		}

		// icon file
		if (!memcmp (skinName, "skn_", 4))
			strcpy (ci.iconName, "/pics/default_icon");
		else
			appSprintf (ARRAY_ARG(ci.iconName), "/players/%s/%s_i", modelName, skinName);
		ci.icon = RE_RegisterPic (ci.iconName);
	}

	// must have loaded all data types to be valud
	if (!ci.md2skin || !ci.icon || !ci.md2model || !ci.weaponmodel[0])
	{
		ci.md2skin = NULL;
		ci.icon = NULL;
		ci.md2model = NULL;
		ci.weaponmodel[0] = NULL;
	}
	else
		ci.isValidModel = true;

	unguard;
}


//!! replace menu.cpp player config + menu_test with this API
//!! should add weapon model code here
int ParsePlayerEntity (clientInfo_t *ci, const entity_t &ent, entity_t *buf, int maxEnts)
{
	guard(ParsePlayerEntity);

	if (maxEnts < 1) return 0;

 	if (!ci->isValidModel)
		ci = &cl.baseClientInfo;

	if (!ci->isQ3model)
	{
		buf[0] = ent;
		buf[0].skin = ci->md2skin;
		buf[0].model = ci->md2model;
		buf[0].pos.axis.FromAngles (ent.angles);
		return 1;
	}
	// Quake3 player model
	if (maxEnts < 3) return 0;			// not enough destination size
	buf[0] = buf[1] = buf[2] = ent;

	//!!-------- testing --------------
	//?? is curTime always == cl.time ?
	{
		static cvar_t *v1, *v2;
		EXEC_ONCE (
			v1 = Cvar_Get ("v1", "14", 0);
			v2 = Cvar_Get ("v2", "11", 0);
			v1->modified = v2->modified = true;
		)
		if (v1->modified) {
			SetQuake3Animation (*ci, ci->legsAnim, v1->integer, cl.time);
			v1->modified = false;
		}
		if (v2->modified) {
			SetQuake3Animation (*ci, ci->torsoAnim, v2->integer, cl.time);
			v2->modified = false;
		}
	}
	RunQuake3Animation (*ci, ci->legsAnim, buf[0], cl.time);
	RunQuake3Animation (*ci, ci->torsoAnim, buf[1], cl.time);
	//!!-------------------------------
	//?? head frame is always 0
	buf[2].frame = buf[2].oldframe = 0;

	buf[0].pos.axis.FromAngles (ent.angles);
	buf[0].model = ci->legsModel;
	buf[1].model = ci->torsoModel;
	buf[2].model = ci->headModel;
	//!! skins, frames
	attach (buf[0], buf[1], "tag_torso");
	attach (buf[1], buf[2], "tag_head");
	//!!!
	buf[0].skin = ci->legsSkin;
	buf[1].skin = ci->torsoSkin;
	buf[2].skin = ci->headSkin;

	return 3;

	unguard;
}
