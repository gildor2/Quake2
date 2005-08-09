#include "client.h"

#define QUAKE3_PLAYER_SCALE		0.9	// Cvar_VariableValue("test")

//?? add Q3TA team player models (players/characters); ui/ui_main.c::UI_LoadTeams(),UI_ParseTeamInfo()


/*-----------------------------------------------------------------------------
	Scanning for player models
-----------------------------------------------------------------------------*/

static CMemoryChain *pmiChain;
TList<playerModelInfo_t> pmiList;
int numPlayerModels;


static bool Md2SkinExists (const char *skin, CStringList &pcxfiles)
{
	const char *ext = strrchr (skin, '.');
	if (!ext) return false;

	if (stricmp (ext, ".pcx") && stricmp (ext, ".tga") && stricmp (ext, ".jpg"))
		return false;		// not image

	// modelname/skn_* have no icon
	const char *name = strrchr (skin, '/');
	if (!name) name = skin;
	else name++;			// skip '/'

	if (!memcmp (name, "skn_", 4))
		return true;
	char scratch[MAX_OSPATH];
	strcpy (scratch, skin);
	strcpy (scratch + (ext - skin), "_i.pcx");	//?? this will allow .PCX only

	for (CListIterator item = pcxfiles; item; ++item)
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


static void ScanQuake2Models (const char *path)
{
	/*----- get a list of directories -----*/
	TList<CStringItem> dirnames;
	dirnames = FS_ListFiles (va("%s/players/*.*", path), LIST_DIRS);
	if (!dirnames) return;

	/*--- go through the subdirectories ---*/
	for (CListIterator diritem = dirnames; diritem; ++diritem)
	{
		const char *infoName = strrchr (diritem->name, '/')+1;
		if (pmiList.Find (infoName)) continue;	// already have model with the same name

		// verify the existence of tris.md2
		if (!FS_FileExists (va("%s/tris.md2", diritem->name)))
			continue;

		// verify the existence of at least one pcx skin
		// "/*.pcx" -> pcx,tga,jpg (see Md2SkinExists())
		TList<CStringItem> skinNames = FS_ListFiles (va("%s/*.*", diritem->name), LIST_FILES);
		if (!skinNames) continue;

		// count valid skins, which consist of a skin with a matching "_i" icon
		TList<CStringItem> skins;
		int numSkins = 0;
		for (CListIterator skinItem = skinNames; skinItem; ++skinItem)
			if (Md2SkinExists (skinItem->name, skinNames))
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
		playerModelInfo_t *info = new (infoName, pmiChain) playerModelInfo_t;
		info->numSkins = numSkins;
		info->skins = skins;
		// add model info to pmi
		pmiList.Insert (info);

		numPlayerModels++;
	}
	dirnames.Free();
}


void ScanQuake3Models (const char *path)
{
	/*----- get a list of directories -----*/
	TList<CStringItem> dirnames;
	dirnames = FS_ListFiles (va("%s/models/players/*.*", path), LIST_DIRS);
	if (!dirnames) return;

	/*--- go through the subdirectories ---*/
	for (CListIterator diritem = dirnames; diritem; ++diritem)
	{
		const char *infoName = strrchr (diritem->name, '/')+1;
		if (pmiList.Find (infoName)) continue;	// already have model with the same name

		// verify the existence of animation.cfg
		if (!FS_FileExists (va("%s/animation.cfg", diritem->name)))
			continue;

		// verify the existence of at least one skin file
		TList<CStringItem> skinNames = FS_ListFiles (va("%s/lower_*.skin", diritem->name), LIST_FILES);
		if (!skinNames) continue;

		// count valid skins, which consist of a skin with a matching "_i" icon
		TList<CStringItem> skins;
		int numSkins = 0;
		for (CListIterator skinItem = skinNames; skinItem; ++skinItem)
		{
			char *str = strrchr (skinItem->name, '/') + 1 /*skip '/'*/ + 6 /*skip "lower_"*/;
			char *ext = strrchr (str, '.');
			ext[0] = 0;
			// check at least one model
			if (!FS_FileExists (va("%s/lower.md3", diritem->name))) continue;
			// may be, have "icon_file.jpg" and "icon_file.tga" ...
			if (skins.Find (str)) continue;
			skins.CreateAndInsert (str, pmiChain);
			numSkins++;
		}
		skinNames.Free();
		if (!numSkins) continue;

		// create model info
		playerModelInfo_t *info = new (infoName, pmiChain) playerModelInfo_t;
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

	const char *path = NULL;
	while (path = FS_NextPath (path))
	{
		// give priority to Quake3 player models: when model with same name
		// presents in md2 format too, it will be ignored
		ScanQuake3Models (path);
		ScanQuake2Models (path);
		if (pmiList) break;			// models found
	}

	if (!numPlayerModels)
	{
		FreePlayerModelsInfo ();
		return false;
	}

	return true;
}


/*-----------------------------------------------------------------------------
	Gender-by-model detection
-----------------------------------------------------------------------------*/

#define		MAX_FEMALE_MODELS		128
#define		MAX_FEMALE_MODEL_BUF	(MAX_FEMALE_MODELS*16)
static const char *femaleModels[MAX_FEMALE_MODELS];
static int  numFemaleModels;
static char femaleModelBuf[MAX_FEMALE_MODEL_BUF];

static void ReadModelsGenderList ()
{
	femaleModels[0] = "female";
	femaleModels[1] = "crakhor";
	numFemaleModels = 2;

	//?? allow multiple files (place in special directory)
	//?? OR allow separate file for each model (placed in model dir) - in this case, don't need this functions
	//??  (can check model gender at loading time); if remove "model.lst" feature, should change documentaion ...
	char	*buf;
	if (!(buf = (char*) FS_LoadFile ("players/model.lst")))
	{
		Com_DPrintf ("players/model.lst is not found\n");
		return;
	}

	CSimpleParser text;
	text.InitFromBuf (buf);

	char *out = femaleModelBuf;
	int free = MAX_FEMALE_MODEL_BUF;

	while (const char *line = text.GetLine ())
	{
		int n = strlen (line) + 1;
		Com_DPrintf("female model: %s\n",line);
		free -= n;
		if (free < 0 || numFemaleModels >= MAX_FEMALE_MODELS)
		{
			Com_WPrintf ("model.lst is too large\n");
			break;
		}
		memcpy (out, line, n);
		femaleModels[numFemaleModels++] = out;
		out += n;
	}

	Com_DPrintf ("Parsed %d model genders\n", numFemaleModels);

	FS_FreeFile (buf);
}


static bool IsFemaleModel (const char *model)
{
	static char lastGameDir[MAX_QPATH];
	const char *gameDir = FS_Gamedir ();
	if (strcmp (gameDir, lastGameDir))
	{
		ReadModelsGenderList ();
		strcpy (lastGameDir, gameDir);
	}

	for (int i = 0; i < numFemaleModels; i++)
		if (!stricmp (femaleModels[i], model))
			return true;

	return false;
}


/*-----------------------------------------------------------------------------
	Model skins
-----------------------------------------------------------------------------*/

static void SetSimpleSkin (const char *name, CModelSkin &skin)
{
	skin.surf[0].surfName[0] = 0;	// empty surface name
	CBasicImage *shader = RE_RegisterSkin (name);
	skin.surf[0].shader = shader;
	skin.numSurfs = (shader != NULL) ? 1 : 0;
}


// false when: a) no skin file b) absent one of skin shaders
static bool SetMd3Skin (const char *skinName, CModelSkin &skin)
{
	// load skin file
	char	*buf;
	char	filename[MAX_QPATH];
	appSprintf (ARRAY_ARG(filename), "models/players/%s.skin", skinName);
	if (!(buf = (char*) FS_LoadFile (filename)))
	{
		Com_DPrintf ("no skin: %s\n", filename);
		return false;
	}

	// parse skin
	memset (&skin, 0, sizeof(skin));
	bool result = true;
	CSimpleParser text;
	text.InitFromBuf (buf);

	int numSurfs = 0;
	while (const char *line = text.GetLine ())
	{
		char *p = strchr (line, ',');
		if (!p || !p[1]) continue;		// no shader
		// check/load shader

		// special processing of "nodraw" shader
		const char *n = strrchr (p+1, '/');
		if (n) n++; else n = p+1;
		if (!strnicmp (n, "nodraw", 6))
		{
			if (n[6] == 0 || n[6] == '.')	// "nodraw" or "nodraw.ext"
			{
				Com_DPrintf("nodraw for %s/%s\n", skinName, line);
				continue;
			}
		}

		CBasicImage *shader = RE_RegisterSkin (p+1);
		if (!shader)
		{
			// code based on gl_trimodel.cpp::SetMd3Skin()
			char	mName[MAX_QPATH];	// new skin name
			// try to find skin forcing model directory
			appCopyFilename (mName, filename, sizeof(mName));
			char *mPtr = strrchr (mName, '/');
			if (mPtr)	mPtr++;			// skip '/'
			else		mPtr = mName;

			char *sPtr = strrchr (p+1, '/');
			if (sPtr)	sPtr++;			// skip '/'
			else		sPtr = p+1;

			strcpy (mPtr, sPtr);		// make "modelpath/skinname"
			shader = RE_RegisterSkin (mName, true);
		}
		if (numSurfs >= ARRAY_COUNT(skin.surf))
		{
			Com_WPrintf ("Too much skin surfaces in %s\n", skinName);
			result = false;
			break;
		}
		// store info
		appStrncpylwr (skin.surf[numSurfs].surfName, line, p - line + 1);
		skin.surf[numSurfs].shader = shader;
//		Com_Printf("%s %d : [%s] <- %s\n", skinName, numSurfs, skin.surf[numSurfs].surfName, shader->name);
		numSurfs++;
	}
	if (result)
		skin.numSurfs = numSurfs;

	FS_FreeFile (buf);

	return result;
}


/*-----------------------------------------------------------------------------
	Quake3 player model loading
-----------------------------------------------------------------------------*/

static clientInfo_t *gci;

static void cAnimGender (int argc, char **argv)
{
	// 'F','f' -> female, 'N','n' -> neutral, other - male
	if (argc < 2) return;
	gci->modelGender = toLower (argv[1][0]);
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
	{"footsteps",	NULL},				//!! use it
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

	ci.modelGender = 'm';								// male; default

	CSimpleParser text;
	text.InitFromBuf (buf);

	while (const char *line = text.GetLine ())
	{
		// execute line
		if (line[0] >= '0' && line[0] <= '9')		// numbers => frame numbers
		{
			if (animNumber >= MAX_ANIMATIONS)
			{
				Com_WPrintf ("Too much animations in \"%s\"\n", filename);
				break;
			}
			int firstFrame, numFrames, loopFrames;
			float fps;
			if (sscanf (line, "%d %d %d %f", &firstFrame, &numFrames, &loopFrames, &fps) != 4)
			{
				Com_WPrintf ("Invalid frame info [%s] in \"%s\"\n", line, filename);
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
		else if (!ExecuteCommand (line, ARRAY_ARG(animCommands)))
		{
			Com_WPrintf ("Invalid line [%s] in \"%s\"\n", line, filename);
			break;
		}
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


static bool SetQuake3Skin (clientInfo_t &ci, const char *modelName, const char *skinName)
{
	bool result = SetMd3Skin (va("%s/lower_%s", modelName, skinName), ci.legsSkin);
	result     &= SetMd3Skin (va("%s/upper_%s", modelName, skinName), ci.torsoSkin);
	result     &= SetMd3Skin (va("%s/head_%s", modelName, skinName), ci.headSkin);

	return result;
}


static CRenderModel *FindQ3Model (const char *name, const char *part)
{
	char filename[MAX_QPATH];
	appSprintf (ARRAY_ARG(filename), "models/players/%s/%s.md3", name, part);
	return RE_RegisterModel (filename);
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

	if (!SetQuake3Skin (ci, modelName, skinName))
	{
//		skinName = "default";
		if (!SetQuake3Skin (ci, modelName, "default"))
			return false;			// no default skin
	}

	appSprintf (ARRAY_ARG(ci.iconName), "models/players/%s/icon_%s", modelName, skinName);
	ci.icon = RE_RegisterPic (ci.iconName);

	ci.isQ3model = true;
	ci.isValidModel = true;
	return true;
}


/*-----------------------------------------------------------------------------
	Quake3 model animation
-----------------------------------------------------------------------------*/

struct animInfo_t
{
	int		nextAnim;
};

#define ANIM_FREEZE		-1					//!! useless

static const animInfo_t animInfo[] = {
//	animation			next
/* BOTH_DEATH1 */	{	ANIM_FREEZE		},
/* BOTH_DEAD1 */	{	ANIM_FREEZE		},
/* BOTH_DEATH2 */	{	ANIM_FREEZE		},
/* BOTH_DEAD2 */	{	ANIM_FREEZE		},
/* BOTH_DEATH3 */	{	ANIM_FREEZE		},
/* BOTH_DEAD3 */	{	ANIM_FREEZE		},

/* TORSO_GESTURE */	{	TORSO_STAND		},

/* TORSO_ATTACK */	{	TORSO_STAND		},
/* TORSO_ATTACK2 */	{	TORSO_STAND2	},

/* TORSO_DROP */	{	TORSO_RAISE		},
/* TORSO_RAISE */	{	TORSO_STAND		},

/* TORSO_STAND */	{	TORSO_STAND		},
/* TORSO_STAND2 */	{	TORSO_STAND2	},

/* LEGS_WALKCR */	{	LEGS_WALKCR		},
/* LEGS_WALK */		{	LEGS_WALK		},
/* LEGS_RUN */		{	LEGS_RUN		},
/* LEGS_BACK */		{	LEGS_BACK		},
/* LEGS_SWIM */		{	LEGS_SWIM		},

/* LEGS_JUMP */		{	ANIM_FREEZE		},
/* LEGS_LAND */		{	ANIM_FREEZE		},
/* LEGS_JUMPB */	{	ANIM_FREEZE		},
/* LEGS_LANDB */	{	ANIM_FREEZE		},

/* LEGS_IDLE */		{	LEGS_IDLE		},
/* LEGS_IDLECR */	{	LEGS_IDLECR		},

/* LEGS_TURN */		{	LEGS_TURN		},

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

#define curTime	cl.time	//????

void RunAnimation (clientInfo_t &ci, animState_t &as, int animNum)
{
	// just in case ...
	if (animNum < 0 || animNum >= MAX_TOTALANIMATIONS) animNum = 0;

	if (animNum == ANIM_NOCHANGE)
		animNum = as.animNum;

	// get animation
	animation_t *anim = &ci.animations[animNum];

	//!! check for correct animation changing w/o lerping (teleport, respawn ...)
	//!! can make this using teleport detection (significant changing of origin):
	//!! when detected, lerp frames from 0

	// check for next animation in sequence
	int wholeAnimTime = anim->numFrames * anim->frameLerp;
	if (animNum == as.animNum && (curTime >= as.startTime + wholeAnimTime - anim->frameLerp))	// start next sequence 1 frame earlier
	{
		if (as.nextAnimNum == ANIM_FREEZE)
			as.completed = true;
		else if (!anim->loopFrames)
		{
			// animation sequence
			animNum = as.nextAnimNum;
			as.nextAnimNum = animInfo[animNum].nextAnim;
			anim = &ci.animations[animNum];
		}
	}

	if (animNum != as.animNum)
	{
		// animation was changed
		as.animNum     = animNum;
		as.nextAnimNum = animInfo[animNum].nextAnim;
		as.completed   = false;
		as.startTime   = as.time + anim->frameLerp;		// allow current frame to complete
	}

	// compute frame, oldFrame, time, oldTime (taken from Q3:CG_RunLerpFrame())
	if (as.completed)
	{
		as.time = as.oldTime = curTime;
		if (!anim->reversed)
			as.frame = anim->firstFrame + anim->numFrames - 1;
		else
			as.frame = anim->firstFrame;
	}
	else if (curTime >= as.time)
	{
		// copy frame -> oldframe
		as.oldTime  = as.time;
		as.oldFrame = as.frame;
		as.time    += anim->frameLerp;
		int frm = (as.time - as.startTime) / anim->frameLerp;
		if (frm >= anim->numFrames)
		{
			frm %= anim->numFrames;
			if (anim->loopFrames)
				frm = frm % anim->loopFrames + (anim->numFrames - anim->loopFrames);
			else
			{
				frm = anim->numFrames - 1;				// hold last frame
				as.time = curTime;						// without lerp
			}
		}
		if (!anim->reversed)
			as.frame = anim->firstFrame + frm;
		else
			as.frame = anim->firstFrame + anim->numFrames - 1 - frm;
	}
	// clamp times
	if (as.time > curTime + 200)
		as.time = curTime;
	if (as.oldTime > curTime)
		as.oldTime = curTime;
	// when we was skippen few RunAnimation() calls before ...
	if (as.time < curTime)
		as.time = as.oldTime = curTime;
}


static void ApplyAnimation (clientInfo_t &ci, animState_t &as, entity_t &ent)
{
	RunAnimation (ci, as);

	animation_t &anim = ci.animations[as.animNum];

	ent.oldframe = as.oldFrame;
	ent.frame    = as.frame;
	ent.backlerp = (as.time > as.oldTime) ? 1.0f - (float)(curTime - as.oldTime) / (as.time - as.oldTime) : 0;
//	RE_DrawTextLeft (va("frm: %d -> %d %g\nold:%d new:%d curr:%d",ent.oldframe, ent.frame, ent.backlerp,as.oldTime,as.time,curTime));
}


static bool IsGroundLegsAnim (int n)
{
	if (n == LEGS_RUN    || n == LEGS_BACK ||
		n == LEGS_WALK   || n == LEGS_BACKWALK ||
		n == LEGS_WALKCR || n == LEGS_BACKCR ||
		n == LEGS_IDLE   || n == LEGS_IDLECR ||
		n == LEGS_LAND   || n == LEGS_LANDB)
		return true;
	return false;
}


// visible weapon support

static weaponInfo_t *loadingWeap;

static void cWeapModel (int argc, char **argv)
{
	loadingWeap->model = RE_RegisterModel (argv[1]);
}

static void cWeapSkin (int argc, char **argv)
{
	loadingWeap->skin = RE_RegisterSkin (argv[1]);
}

static void cWeapScale (int argc, char **argv)
{
	loadingWeap->drawScale = atof (argv[1]);
}

static void cWeapOffset (int argc, char **argv)
{
	for (int i = 0; i < 3; i++)
		loadingWeap->origin[i] = atof (argv[i+1]);
}

static const CSimpleCommand weapCommands[] = {
	{"model",	cWeapModel	},
	{"skin",	cWeapSkin	},
	{"scale",	cWeapScale	},
	{"offset",	cWeapOffset	}
	//?? add flashOffset
};


static bool LoadWeaponInfo (const char *filename, weaponInfo_t &weap)
{
	char *buf = (char*) FS_LoadFile (va("models/weapons/%s.cfg", filename));
	if (!buf) return false;

	memset (&weap, 0, sizeof(weaponInfo_t));
	loadingWeap = &weap;
	// defaults
	weap.drawScale = 1;
	bool result = true;
	CSimpleParser text;
	text.InitFromBuf (buf);
	while (const char *line = text.GetLine ())
	{
		if (!ExecuteCommand (line, ARRAY_ARG(weapCommands)))
		{
			Com_WPrintf ("Invalid line [%s] in \"%s.cfg\"\n", line, filename);
			result = false;
			break;
		}
	}
	weap.origin.Scale (weap.drawScale);
	FS_FreeFile (buf);
	return result && weap.model != NULL;
	//?? NOTE: currently, "false" result ignored ...
}


/*-----------------------------------------------------------------------------
	Quake2 player model loading
-----------------------------------------------------------------------------*/

static CRenderModel *FindQ2Model (const char *name, const char *part)
{
	char filename[MAX_QPATH];
	appSprintf (ARRAY_ARG(filename), "players/%s/%s", name, part);
	if (!strchr (part, '.')) strcat (filename, ".md2");
	return RE_RegisterModel (filename);
}


static bool TryQuake2Model (clientInfo_t &ci, const char *modelName, const char *skinName, bool lockSkin = false)
{
	// model
	ci.md2model = FindQ2Model (modelName, "tris");
	if (!ci.md2model) return false;

	// skin
	SetSimpleSkin (va("players/%s/%s", modelName, skinName), ci.md2skin);
	if (!ci.md2skin.IsValid())
	{
		if (lockSkin) return false;

		// try skin == model
		SetSimpleSkin (va("players/%s/%s", modelName, modelName), ci.md2skin);
		if (!ci.md2skin.IsValid ())
		{
			// try any skin
			TList<CStringItem> skinNames = FS_ListFiles (va("players/%s/*.*", modelName), LIST_FILES);
			for (CListIterator skinItem = skinNames; skinItem; ++skinItem)
				if (Md2SkinExists (skinItem->name, skinNames))
				{
					const char *t = strrchr (skinItem->name, '/');
					if (!t) t = skinItem->name;
					else t++;							// skip '/'
					SetSimpleSkin (va("players/%s/%s", modelName, skinItem->name), ci.md2skin);
					if (ci.md2skin.IsValid ()) break;	// done
				}
			skinNames.Free ();
			if (!ci.md2skin.IsValid ()) return false;	// skins not found
			// can change icon info too, but skin is not as specified - so, use "default_icon"
		}
	}

	// icon
	appSprintf (ARRAY_ARG(ci.iconName), "players/%s/%s_i", modelName, skinName);
	ci.icon = RE_RegisterPic (ci.iconName);

	ci.isQ3model = false;
	ci.isValidModel = true;
	return true;
}

/*-----------------------------------------------------------------------------
	Loading client information
-----------------------------------------------------------------------------*/

void CL_LoadClientinfo (clientInfo_t &ci, const char *s, bool loadWeapons)
{
	guard (CL_LoadClientinfo);

	char modelName[MAX_QPATH], skinName[MAX_QPATH];
	static unsigned id_count = 1;

	memset (&ci, 0, sizeof(clientInfo_t));
	ci.id = id_count++;
	if (!cl_vwep->integer) loadWeapons = false;

	// get player's name
	appStrncpyz (ci.playerName, s, sizeof(ci.playerName));
	char *t = strchr (ci.playerName, '\\');
	if (t)
	{
		*t = 0;
		s = t + 1;
	}

	if (cl_noskins->integer || !t || !s[0])
		s = "male/grunt";				// default skin

	// get model name
	strcpy (modelName, s);
	t = strchr (modelName, '/');
	if (!t) t = strchr (modelName, '\\');
	if (!t) t = modelName;
	*t = 0;

	// get skin name
	strcpy (skinName, t + 1);

	// try loading Quake3 model
	if (!TryQuake3Model (ci, modelName, skinName))
	{
		// no such Quake3 player model - load Quake2 model
		if (!TryQuake2Model (ci, modelName, skinName))
		{
			// try "male" model with the same skin
			if (!TryQuake2Model (ci, "male", skinName, true))
			{
				// try "male/grunt"
				if (!TryQuake2Model (ci, "male", "grunt"))
					return;				// at this point, client info is invalid, and no way to make it valid
				strcpy (skinName, "grunt");
			}
			strcpy (modelName, "male");
		}

		// weapons
		ci.weaponModel[0].model = FindQ2Model (modelName, "weapon");
		if (loadWeapons)
			for (int i = 1; i < num_cl_weaponmodels; i++)
			{
				ci.weaponModel[i].model = FindQ2Model (modelName, cl_weaponmodels[i]);
#if 0
				// HACK: cyborg have the weapon same models, as male model
				if (!ci.weaponModel[i].model && !strcmp (modelName, "cyborg"))
					ci.weaponModel[i].model = FindQ2Model ("male", cl_weaponmodels[i]);
#endif
			}

		ci.modelGender = (IsFemaleModel (modelName)) ? 'f' : 'm';
	}
	else
	{
		// Quake3 model -- try loading weapons
#if 0
		if (!LoadWeaponInfo ("weapon.md2", ci.weaponModel[0]))
		{
			ci.isValidModel = false;
			EXEC_ONCE(	// do not show multiple times
				Com_WPrintf ("Weapon description for default weapon is not found\n");
			)
			return;
		}
#else
		LoadWeaponInfo ("weapon.md2", ci.weaponModel[0]);
#endif
		if (loadWeapons)
			for (int i = 1; i < num_cl_weaponmodels; i++)
				LoadWeaponInfo (cl_weaponmodels[i], ci.weaponModel[i]);
	}

	// save model name
	strcpy (ci.modelName, modelName);

	// default icon
	if (!ci.icon)
	{
		strcpy (ci.iconName, "pics/default_icon");
		ci.icon = RE_RegisterPic (ci.iconName);
	}

	cl.forceViewFrame = true;	// when paused, force to update scene

	unguard;
}


/*-----------------------------------------------------------------------------
	Preparing data for display
-----------------------------------------------------------------------------*/

static bool attach (const entity_t &e1, entity_t &e2, const char *tag, const CVec3 *angles = NULL, float drawScale = QUAKE3_PLAYER_SCALE)
{
	CCoords lerped;		// get position modifier
	if (!e1.model->LerpTag (e1.frame, e1.oldframe, e1.backlerp, tag, lerped))
	{
		if (DEVELOPER)
			RE_DrawTextLeft(va("%s: no tag \"%s\"", e1.model->name, tag));
		return false;	// no such tag
	}
	// some player models (at least, "dangergirl") have lerped origin set very far from base model
	// to hide attached model (this hack used, because quake3 cgame code not allows to remove most tags)
	if (fabs (lerped.origin[0]) > 1000 || fabs (lerped.origin[1]) > 1000 || fabs (lerped.origin[2]) > 1000)
	{
		if (DEVELOPER)
			RE_DrawTextLeft(va("%s: removed tag \"%s\"", e1.model->name, tag));
		return false;
	}

	if (angles)
	{
		CAxis rotate;
		rotate.FromAngles (*angles);
		lerped.axis.UnTransformAxis (rotate, lerped.axis);
	}
	if (drawScale != 1.0f)
		lerped.origin.Scale (drawScale);
	e1.pos.UnTransformCoords (lerped, e2.pos);
	return true;
}


// from Quake3; optimized
static void SwingAngle (float dst, float tolerance, float clamp, float speed, float &angle, bool &swinging)
{
	float swing = AngleSubtract (dst, angle);
	float absSwing = fabs (swing);

	// do not swing when angle delta less than tolerance
	if (absSwing > tolerance)
		swinging = true;

	if (!swinging) return;

	// non-linear rotation speed
	float scale;
	if (absSwing < tolerance / 2)
		scale = 0.5f;
	else if (absSwing < tolerance)
		scale = 1.0f;
	else
		scale = 2.0f;
	// swing
	float move = cls.frametime * 1000 /*sec->msec*/ * scale * speed;
	if (move >= absSwing)
	{
		// moved to destination
		move = absSwing;
		swinging = false;
	}
	else if (absSwing - move > clamp)
	{
		// do not allow angle distance to be more than clamp
		// if no - angle between torso and head may be too large (and similar situations)
		move = absSwing - clamp;
	}

	if (!IsNegative (swing))
		angle = AngleMod (angle + move);
	else
		angle = AngleMod (angle - move);
}


// clamp angle to [dst-clamp .. dst+clamp]
static bool ClampAngle (float dst, float clamp, float &angle)
{
	float swing = AngleSubtract (dst, angle);
	float absSwing = fabs (swing);
	if (absSwing <= clamp) return false;

	float move = absSwing - clamp;
	if (!IsNegative (swing))
		angle = AngleMod (angle + move);
	else
		angle = AngleMod (angle - move);
	return true;
}


int ParsePlayerEntity (centity_t &cent, clientInfo_t &ci, clEntityState_t *st, const entity_t &ent, entity_t *buf, int maxEnts, int weaponIndex)
{
	// argument usage: st->GetAnim(), cent.anim, cent.prev.GetAnim()
	guard(ParsePlayerEntity);

	if (!ci.isValidModel)
	{
		RE_DrawText3D (ent.pos.origin, va("%s\ninvalid model", ci.playerName), RGB(1,0,0));
		return 0;
	}

	if (cent.clientInfoId != ci.id)
	{
		// clientInfo_t changed -> reset animations for new model
		cent.clientInfoId = ci.id;
		memset (&cent.legsAnim, 0, sizeof(animState_t));
		cent.legsAnim.angles = ent.angles;
		memset (&cent.torsoAnim, 0, sizeof(animState_t));
		cent.torsoAnim.angles = ent.angles;
	}

	if (maxEnts > 0) memset (buf, 0, sizeof(entity_t) * maxEnts);
	if (weaponIndex >= 0 && !ci.weaponModel[weaponIndex].model)
		weaponIndex = 0;								// no model -> change to default model
	if (!ci.weaponModel[0].model) weaponIndex = -1;		// no model at all

	if (!ci.isQ3model)
	{
		if (maxEnts < 1) return 0;
		buf[0] = ent;
		buf[0].skin  = &ci.md2skin;
		buf[0].model = ci.md2model;
		buf[0].pos.axis.FromAngles (ent.angles);
		if (maxEnts < 2 || weaponIndex < 0) return 1;	// no linked weapon
		// here: assume, that weapon[0] exists
		buf[1]       = buf[0];
		buf[1].skin  = NULL;
		buf[1].model = ci.weaponModel[weaponIndex].model;
		return 2;
	}

	animState_t &la = cent.legsAnim;
	animState_t &ta = cent.torsoAnim;
	animState_t &ha = cent.headAnim;

	//---------- Quake3 player model --------------
	int legsAnim, torsoAnim, movingDir;
	float pitchAngle;
	st->GetAnim (legsAnim, torsoAnim, movingDir, pitchAngle);
	//?? do not exec jump animation, when falling from small height

	int prevLegs = ANIM_NOCHANGE,
		prevTorso = ANIM_NOCHANGE,
		prevDir = LEGS_NEUTRAL;
	float prevPitch = pitchAngle;
	if (IsGroundLegsAnim (legsAnim))
	{
		cent.prev.GetAnim (prevLegs, prevTorso, prevDir, prevPitch);
		if (prevLegs == LEGS_JUMP)
			legsAnim = LEGS_LAND;
		else if (prevLegs == LEGS_JUMPB)
			legsAnim = LEGS_LANDB;
		else if ((la.animNum == LEGS_LAND || la.animNum == LEGS_LANDB) && !la.completed && legsAnim != LEGS_IDLECR)
			legsAnim = ANIM_NOCHANGE;
	}

#if 0
	//!! testing
	legsAnim = torsoAnim = ANIM_NOCHANGE;
	static cvar_t *v1, *v2, *v3;
	EXEC_ONCE (
		v1 = Cvar_Get ("v1", "14", 0);
		v2 = Cvar_Get ("v2", "11", 0);
		v3 = Cvar_Get ("v3", "0", 0);
		v1->modified = v2->modified = true;
	)
	if (v1->modified) {
		legsAnim = v1->integer;
		v1->modified = false;
	}
	if (v2->modified) {
		torsoAnim = v2->integer;
		v2->modified = false;
	}
	movingDir = v3->integer;
#endif

	// rotate models
	CVec3 legsAngles, torsoAngles, headAngles;
	static const float angleDifs[] = {0, 22.5, 45, -22.5, -45};	// corresponds to LEGS_[NEUTRAL/LEFT_XXX/RIGHT_XXX]

	headAngles  = ent.angles;
	torsoAngles = headAngles;
	legsAngles  = headAngles;

	if (!ci.fixedAll)
	{
		if (la.animNum != LEGS_IDLE || ta.animNum != TORSO_STAND)
		{
			// allow rotating when not standing
			ta.rotating[YAW]   = true;
			ta.rotating[PITCH] = true;
			la.rotating[YAW]   = true;
		}

		// YAW angles
		if (!(legsAnim == torsoAnim && legsAnim >= BOTH_DEATH1 && legsAnim <= BOTH_DEAD3))
		{
			torsoAngles[YAW] = headAngles[YAW] + angleDifs[movingDir] / 4.0f;
			legsAngles[YAW]  = headAngles[YAW] + angleDifs[movingDir];
		}
		// swing angles
		SwingAngle (torsoAngles[YAW], 25, 80, 0.15f, ta.angles[YAW], ta.rotating[YAW]);
		SwingAngle (legsAngles[YAW],  40, 90, 0.3f, la.angles[YAW], la.rotating[YAW]);

		// PITCH angles (looking up/down)
#if 0
		la.angles[PITCH]  = 0;		//?? when standing/run/walk only; process ANIM_NOCHANGE too
		la.angles[ROLL]   = 0;		//??
		headAngles[PITCH] = Lerp (prevPitch, pitchAngle, cl.lerpfrac);
		float dst = headAngles[PITCH];
		if (dst > 180) dst -= 360;
		dst *= 0.75f;
		SwingAngle (dst, 15, 30, 0.1f, ta.angles[PITCH], ta.rotating[PITCH]);

		// clamp angles
		ClampAngle (ent.angles[PITCH], 45, la.angles[PITCH]);
		ClampAngle (la.angles[PITCH],  45, ta.angles[PITCH]);
		ClampAngle (ta.angles[PITCH],  45, headAngles[PITCH]);
#else
		headAngles[PITCH] = Lerp (prevPitch, pitchAngle, cl.lerpfrac);
		float rot = ent.angles[PITCH];
		if (rot > 180)
			rot -= 360;
		if (rot > 35 || rot < -35)
			headAngles[PITCH] = rot;
		SwingAngle (headAngles[PITCH], 0, 180, 0.3f, ha.angles[PITCH], ha.rotating[PITCH]);

		if (!ClampAngle (ha.angles[PITCH], 45, ta.angles[PITCH]))
		{
			float dst = LerpAngle (la.angles[PITCH], headAngles[PITCH], 0.6f);
			SwingAngle (dst, 15, 60, 0.15f, ta.angles[PITCH], ta.rotating[PITCH]);
		}
		if (IsGroundLegsAnim(legsAnim))
			SwingAngle (0, 0, 60, 0.3f, la.angles[PITCH], la.rotating[PITCH]);
		else
			ClampAngle (ta.angles[PITCH], 30, la.angles[PITCH]);
		headAngles[PITCH] = ha.angles[PITCH];
#endif

		// ROLL angles
		float dst = IsGroundLegsAnim (legsAnim) ? 0 : headAngles[ROLL];
		ta.angles[ROLL] = la.angles[ROLL] = headAngles[ROLL] = dst;

		torsoAngles = ta.angles;
		legsAngles  = la.angles;

		// lean legs, dependent on movement
		CVec3 posDelta;
		VectorSubtract (cent.current.origin, cent.prev.origin, posDelta);
		if (posDelta[0] || posDelta[1] || posDelta[2])
		{
			CAxis axis;
			axis.FromAngles (ent.angles);
//			RE_DrawTextLeft(va("[0]=%g\n[1]=%g\n[2]=%g", dot(posDelta,axis[0]), dot(posDelta,axis[1]), dot(posDelta,axis[2])));
			legsAngles[PITCH] += dot(posDelta, axis[0]) / 3;
			legsAngles[ROLL]  -= dot(posDelta, axis[1]) / 3;
		}
	}

	// when standing and rotating, exec "turn" animation
	if (legsAnim == LEGS_IDLE && la.rotating[YAW])
		legsAnim = LEGS_TURN;

	// fixedLegs -- legs will stand absolutely vertical when moving (no lean to direction of movement)
	if (ci.fixedLegs)
	{
		legsAngles[YAW] = torsoAngles[YAW];
		legsAngles[PITCH] = legsAngles[ROLL] = 0;
	}
	// fixedTorso -- do not add PITCH angle to torso, when look up/down
	if (ci.fixedTorso)
		torsoAngles[PITCH] = 0;

	// run animations
	RunAnimation (ci, la, legsAnim);
	RunAnimation (ci, ta, torsoAnim);

	if (maxEnts < 3) return 0;			// not enough destination size

	buf[0] = buf[1] = buf[2] = ent;
	// head frame is always 0
	buf[2].frame = buf[2].oldframe = 0;

	buf[0].pos.axis.FromAngles (ent.angles);
	buf[0].model = ci.legsModel;
	buf[1].model = ci.torsoModel;
	buf[2].model = ci.headModel;
	for (int i = 0; i < 3; i++)
		buf[i].scale = QUAKE3_PLAYER_SCALE;

	// -24 is pmove.mins[2]
	VectorMA (buf[0].pos.origin, (1.0f-QUAKE3_PLAYER_SCALE) * (-24), buf[0].pos.axis[2]);

	// animate models
	ApplyAnimation (ci, la, buf[0]);
	ApplyAnimation (ci, ta, buf[1]);

	// prepare angles
	AnglesSubtract (headAngles, torsoAngles, headAngles);
	AnglesSubtract (torsoAngles, legsAngles, torsoAngles);

	buf[0].pos.axis.FromAngles (legsAngles);
	attach (buf[0], buf[1], "tag_torso", &torsoAngles);
	attach (buf[1], buf[2], "tag_head", &headAngles);

	buf[0].skin = &ci.legsSkin;
	buf[1].skin = &ci.torsoSkin;
	buf[2].skin = &ci.headSkin;

	if (maxEnts < 4) weaponIndex = -1;	// do not add weapon - no space

	bool weaponAttached = false;
	if (weaponIndex >= 0 && attach (buf[1], buf[3], "tag_weapon"))
	{
		// setup model ant draw params
		//?? allow weapon model debug to be enabled with cvar at any time
#if 0
		// model debugging
		weaponInfo_t weap;
		weap.model = RE_RegisterModel (va("models/%s/tris.md2", Cvar_VariableString("weap")));
		weap.drawScale = Cvar_VariableValue ("sc");
		CVec3 offs;
		offs.Zero ();	// for scanf
		sscanf (Cvar_VariableString("offs"), "%f %f %f", VECTOR_ARG(&offs));
		offs.Scale (weap.drawScale);
		weap.origin = offs;
#else
		weaponInfo_t &weap = ci.weaponModel[weaponIndex];
#endif

		buf[3].model        = weap.model;
		buf[3].customShader = weap.skin;
		buf[3].scale        = weap.drawScale;
		buf[3].pos.UnTransformPoint (weap.origin, buf[3].pos.origin);
		weaponAttached = true;
	}

	//!! lighting origin for all 3 (4) entities should be the same (ent.origin)

	return 3 + weaponAttached;

	unguard;
}
