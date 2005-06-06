#include "client.h"

#define QUAKE3_PLAYER_SCALE		0.9	// Cvar_VariableValue("test")

//!! move gender-by-model selection code here (if not server-side)
//!! model-specific sounds placed in sound/player/[model]/*.wav

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
		pmiList.Insert (info);	//?? check pmiList, if exists - do not add (have same Quake3 model)

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

	// give priority to Quake3 player models: when model with same name
	// presents in md2 format too, it will be ignored
	ScanQuake3Models ();
	ScanQuake2Models ();

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
	appSprintf (ARRAY_ARG(filename), "players/%s/%s", name, part);
	if (!strchr (part, '.')) strcat (filename, ".md2");
	return RE_RegisterModel (filename);
}

static CRenderModel *FindQ3Model (const char *name, const char *part)
{
	char filename[MAX_QPATH];
	appSprintf (ARRAY_ARG(filename), "models/players/%s/%s.md3", name, part);
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


//!! change, move outside, use another etc
static const char *parserText;

static void SetupTextParser (const char *text)
{
	parserText = text;
}

static const char *GetLine ()
{
	static char line[1024];

	char *d = line;
	char c = 0;
	while (d < line + sizeof(line) - 2)
	{
		c = *parserText;
		if (!c) break;						// end of text
		parserText++;
		if (c == '\r' || c == '\n')			// end of line
			break;
		if (c == '\t') c = ' ';				// tab -> space
		*d++ = c;
	}
	*d++ = 0;
	// cut "//" comments
	for (d = line; d[0]; d++)
		if (d[0] == '/' && d[1] == '/')		//?? later: not inside quotes
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
	// skip leading spaces
	d = line;
	while (d[0] == ' ') d++;

	return (c || d[0]) ? d : NULL;
}


static void LoadAnimationCfg (clientInfo_t &ci, const char *filename)
{
	char *buf = (char*) FS_LoadFile (filename);
	gci = &ci;
	int animNumber = 0;
	animation_t *anims = ci.animations;
	int frameSkip = 0;

	SetupTextParser (buf);

	while (true)
	{
		const char *line = GetLine ();
		if (!line) break;								// end of text

		// execute line
		if (line[0])
		{
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


// service functions

static bool attach (const entity_t &e1, entity_t &e2, const char *tag, const CVec3 *angles = NULL, float drawScale = QUAKE3_PLAYER_SCALE)
{
	CCoords lerped;		// get position modifier
	if (!e1.model->LerpTag (e1.frame, e1.oldframe, e1.backlerp, tag, lerped)) return false;	// no such tag
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


// animation support

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

/* TORSO_DROP */	{	TORSO_STAND		},
/* TORSO_RAISE */	{	TORSO_STAND		},

/* TORSO_STAND */	{	TORSO_STAND		},
/* TORSO_STAND2 */	{	TORSO_STAND2	},

/* LEGS_WALKCR */	{	LEGS_WALKCR		},
/* LEGS_WALK */		{	LEGS_WALK		},
/* LEGS_RUN */		{	LEGS_RUN		},
/* LEGS_BACK */		{	LEGS_BACK		},
/* LEGS_SWIM */		{	LEGS_SWIM		},

/* LEGS_JUMP */		{	ANIM_FREEZE		},
/* LEGS_LAND */		{	LEGS_IDLE		},	//??
/* LEGS_JUMPB */	{	ANIM_FREEZE		},
/* LEGS_LANDB */	{	LEGS_IDLE		},	//??

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
			as.freezed = true;
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
		as.freezed     = false;
		as.startTime   = as.time + anim->frameLerp;		// allow current frame to complete
	}

	// compute frame, oldFrame, time, oldTime (taken from Q3:CG_RunLerpFrame())
	if (as.freezed)
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
				as.time = curTime;						// no lerping
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


static bool IsCommonLegsAnim (int n)
{
	if (n == LEGS_RUN    || n == LEGS_BACK ||
		n == LEGS_WALK   || n == LEGS_BACKWALK ||
		n == LEGS_WALKCR || n == LEGS_BACKCR ||
		n == LEGS_IDLE   || n == LEGS_IDLECR)
		return true;
	return false;
}


// visible weapon support

static weaponInfo_t *loadingWeap;

static void cWeapModel (int argc, char **argv)
{
	loadingWeap->model = RE_RegisterModel (argv[1]);
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
	{"model",	cWeapModel},
	{"scale",	cWeapScale},
	{"offset",	cWeapOffset}
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
	SetupTextParser (buf);
	while (true)
	{
		const char *line = GetLine ();
		if (!line) break;
		if (!line[0]) continue;
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
}


/*-----------------------------------------------------------------------------
	Quake2 and common player model code
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
	appStrncpyz (ci.name, s, sizeof(ci.name));
	char *t = strchr (ci.name, '\\');
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
		ci.md2model = FindQ2Model (modelName, "tris");
		if (!ci.md2model)
		{
			strcpy (modelName, "male");
			ci.md2model = FindQ2Model ("male", "tris");
		}

		// skin
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

		// icon
		if (!memcmp (skinName, "skn_", 4))
			strcpy (ci.iconName, "/pics/default_icon");
		else
			appSprintf (ARRAY_ARG(ci.iconName), "/players/%s/%s_i", modelName, skinName);
		ci.icon = RE_RegisterPic (ci.iconName);

		// weapons
		ci.weaponModel[0].model = FindQ2Model (modelName, "weapon");
		if (loadWeapons)
			for (int i = 1; i < num_cl_weaponmodels; i++)
			{
				ci.weaponModel[i].model = FindQ2Model (modelName, cl_weaponmodels[i]);
				// HACK: cyborg have the weapon same models, as male model
				if (!ci.weaponModel[i].model && !strcmp (modelName, "cyborg"))
					ci.weaponModel[i].model = FindQ2Model ("male", cl_weaponmodels[i]);
			}

		// validate client info
		if (ci.md2skin && ci.icon && ci.md2model && ci.weaponModel[0].model)
			ci.isValidModel = true;
	}
	else
	{
		// Quake3 model -- try loading weapons
		if (!LoadWeaponInfo ("weapon.md2", ci.weaponModel[0]))
		{
			ci.isValidModel = false;
			EXEC_ONCE(	// do not show multiple times
				Com_WPrintf ("Weapon description for default weapon is not found\n");
			)
			return;
		}
		if (loadWeapons)
			for (int i = 1; i < num_cl_weaponmodels; i++)
				LoadWeaponInfo (cl_weaponmodels[i], ci.weaponModel[i]);
	}

	unguard;
}


int ParsePlayerEntity (centity_t *cent, clientInfo_t *ci, clEntityState_t *st, const entity_t &ent, entity_t *buf, int maxEnts, int weaponIndex)
{
	// argument usage: st->GetAnim(), cent->anim, cent->prev.GetAnim()
	guard(ParsePlayerEntity);

	if (!ci->isValidModel)
		ci = &cl.baseClientInfo;

	if (cent->clientInfoId != ci->id)
	{
		// clientInfo_t changed -> reset animations
		cent->clientInfoId = ci->id;
		memset (&cent->legsAnim, 0, sizeof(animState_t));
		cent->legsAnim.angles = ent.angles;
		memset (&cent->torsoAnim, 0, sizeof(animState_t));
		cent->torsoAnim.angles = ent.angles;
	}

	if (maxEnts > 0) memset (buf, 0, sizeof(entity_t) * maxEnts);
	if (weaponIndex >= 0 && !ci->weaponModel[weaponIndex].model)
		weaponIndex = 0;								// no model -> change to default model
	if (!ci->weaponModel[0].model) weaponIndex = -1;	// no model at all

	if (!ci->isQ3model)
	{
		if (maxEnts < 1) return 0;
		buf[0] = ent;
		buf[0].skin  = ci->md2skin;
		buf[0].model = ci->md2model;
		buf[0].pos.axis.FromAngles (ent.angles);
		if (maxEnts < 2 || weaponIndex < 0) return 1;	// no linked weapon
		// here: assume, that weapon[0] exists - else, clientInfo will be baseClientInfo
		buf[1]       = buf[0];
		buf[1].skin  = NULL;
		buf[1].model = ci->weaponModel[weaponIndex].model;
		return 2;
	}

	//!!! weapon for Q3

	animState_t &la = cent->legsAnim;
	animState_t &ta = cent->torsoAnim;

	// Quake3 player model
	int legsAnim, torsoAnim, movingDir;
	st->GetAnim (legsAnim, torsoAnim, movingDir);
	//?? do not exec jump animation, when falling from small height
	if (IsCommonLegsAnim (legsAnim))
	{
		int prevLegs, prevTorso, prevDir;
		cent->prev.GetAnim (prevLegs, prevTorso, prevDir);
		if (prevLegs == LEGS_JUMP)
			legsAnim = LEGS_LAND;
		else if (prevLegs == LEGS_JUMPB)
			legsAnim = LEGS_LANDB;
		else if (la.animNum == LEGS_LAND || la.animNum == LEGS_LANDB)
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

	if (!ci->fixedAll)
	{
		if (la.animNum != LEGS_IDLE || ta.animNum != TORSO_STAND)
		{
			// allow rotating when not standing
			ta.rotating[YAW]   = true;
			ta.rotating[PITCH] = true;
			la.rotating[YAW]   = true;
		}

		if (!(legsAnim == torsoAnim && legsAnim >= BOTH_DEATH1 && legsAnim <= BOTH_DEAD3))
		{
			torsoAngles[YAW] = headAngles[YAW] + angleDifs[movingDir] / 4.0f;
			legsAngles[YAW]  = headAngles[YAW] + angleDifs[movingDir];
		}
		// swing angles
		SwingAngle (torsoAngles[YAW], 25, 90, 0.3f, ta.angles[YAW], ta.rotating[YAW]);
		SwingAngle (legsAngles[YAW],  40, 90, 0.3f, la.angles[YAW], la.rotating[YAW]);
		torsoAngles[YAW] = ta.angles[YAW];
		legsAngles[YAW]  = la.angles[YAW];
	}

	// when standing and rotating, exec "turn" animation
	if (legsAnim == LEGS_IDLE && la.rotating[YAW])
		legsAnim = LEGS_TURN;

	//!! add looking up/down (should detect, when rotate all 3 ents, and when - don't rotate legs)
	//!! fixedTorso -- don't bend body when looking up/down

	// fixedLegs -- legs will stand absolutely vertical when moving (no lean to direction of movement)
	if (ci->fixedLegs)
	{
		legsAngles[YAW] = torsoAngles[YAW];
		legsAngles[PITCH] = legsAngles[ROLL] = 0;
	}

	// run animations
	RunAnimation (*ci, la, legsAnim);
	RunAnimation (*ci, ta, torsoAnim);

	if (maxEnts < 3) return 0;			// not enough destination size

	buf[0] = buf[1] = buf[2] = ent;
	// head frame is always 0
	buf[2].frame = buf[2].oldframe = 0;

	buf[0].pos.axis.FromAngles (ent.angles);
	buf[0].model = ci->legsModel;
	buf[1].model = ci->torsoModel;
	buf[2].model = ci->headModel;
	for (int i = 0; i < 3; i++)
		buf[i].scale = QUAKE3_PLAYER_SCALE;

	// -24 is pmove.mins[2]
	VectorMA (buf[0].pos.origin, (1.0f-QUAKE3_PLAYER_SCALE) * (-24), buf[0].pos.axis[2]);

	// animate models
	ApplyAnimation (*ci, la, buf[0]);
	ApplyAnimation (*ci, ta, buf[1]);

	// prepare angles
	AnglesSubtract (headAngles, torsoAngles, headAngles);
	AnglesSubtract (torsoAngles, legsAngles, torsoAngles);

	buf[0].pos.axis.FromAngles (legsAngles);
	attach (buf[0], buf[1], "tag_torso", &torsoAngles);
	attach (buf[1], buf[2], "tag_head", &headAngles);

	//!!!
	buf[0].skin = ci->legsSkin;
	buf[1].skin = ci->torsoSkin;
	buf[2].skin = ci->headSkin;

	if (maxEnts < 4) weaponIndex = -1;	// do not add weapon - no space

	if (weaponIndex >= 0)
	{
		attach (buf[1], buf[3], "tag_weapon");
		// setup model ant draw params
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
		weaponInfo_t &weap = ci->weaponModel[weaponIndex];
#endif

		buf[3].model = weap.model;
		buf[3].scale = weap.drawScale;
		buf[3].pos.UnTransformPoint (weap.origin, buf[3].pos.origin);
	}

	//!! lighting origin for all 3 (4) entities should be the same (ent.origin)

	return 3 + (weaponIndex >= 0);

	unguard;
}
