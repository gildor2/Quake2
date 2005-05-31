/*-----------------------------------------------------------------------------
	Staff from Quake3 SDK
-----------------------------------------------------------------------------*/

// animations
typedef enum {
	BOTH_DEATH1,
	BOTH_DEAD1,
	BOTH_DEATH2,
	BOTH_DEAD2,
	BOTH_DEATH3,
	BOTH_DEAD3,

	TORSO_GESTURE,

	TORSO_ATTACK,
	TORSO_ATTACK2,

	TORSO_DROP,
	TORSO_RAISE,

	TORSO_STAND,
	TORSO_STAND2,

	LEGS_WALKCR,
	LEGS_WALK,
	LEGS_RUN,
	LEGS_BACK,
	LEGS_SWIM,

	LEGS_JUMP,
	LEGS_LAND,

	LEGS_JUMPB,
	LEGS_LANDB,

	LEGS_IDLE,
	LEGS_IDLECR,

	LEGS_TURN,

	TORSO_GETFLAG,
	TORSO_GUARDBASE,
	TORSO_PATROL,
	TORSO_FOLLOWME,
	TORSO_AFFIRMATIVE,
	TORSO_NEGATIVE,

	MAX_ANIMATIONS,
	LEGS_BACKCR,
	LEGS_BACKWALK,
#if 0
	FLAG_RUN,
	FLAG_STAND,
	FLAG_STAND2RUN,
#endif
	MAX_TOTALANIMATIONS
} animNumber_t;


struct animation_t
{
	short	firstFrame;
	short	numFrames;
	short	loopFrames;					// 0 to numFrames
	bool	reversed;					// true if animation is reversed
//	bool	flipflop;					// true if animation should flipflop back to base
	int		frameLerp;					// msec between frames (rename ??)
};


#define MAX_CLIENTWEAPONMODELS	32

struct animState_t
{
	short	animNum;					// number of current animation
	short	nextAnimNum;				// number of animation, which will be launched after current
	short	oldFrame;					// old frame numbers (may be, from another animation)
	int		startTime;					// time of current animation start (1st frame of current animation)
};

struct clientInfo_t
{
	char	name[MAX_QPATH];			// player name
	char	iconName[MAX_QPATH];
	bool	isQ3model;
	bool	isValidModel;
	CBasicImage	*icon;					// used as "!= NULL" only
	union {
		struct { // data for Quake2 player model
			CBasicImage	*md2skin;
			CRenderModel *md2model;
		};
		struct { // data for Quake3 player model
			// models
			CRenderModel *legsModel;
			CRenderModel *torsoModel;
			CRenderModel *headModel;
			// skins
			CBasicImage  *legsSkin;
			CBasicImage  *torsoSkin;
			CBasicImage  *headSkin;
			// animation states
			animState_t legsAnim, torsoAnim;
			// animation parameters
			bool	fixedLegs, fixedTorso;			//?? unused now
			animation_t animations[MAX_TOTALANIMATIONS];
		};
	};
	CRenderModel *weaponmodel[MAX_CLIENTWEAPONMODELS];
};


class playerModelInfo_t : public CStringItem
{
public:
	bool	isQ3mdl;		// true for Quake3 player model, false for Quake2; unused_now!!
	int		numSkins;
	TList<CStringItem> skins;
};

extern TList<playerModelInfo_t> pmiList;
extern int numPlayerModels;

void FreePlayerModelsInfo ();
bool ScanPlayerModels ();
void CL_LoadClientinfo (clientInfo_t &ci, const char *s);
int ParsePlayerEntity (clientInfo_t *ci, const entity_t &ent, entity_t *buf, int maxEnts);
