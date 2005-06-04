// declared later in client.h:
struct clEntityState_t;
struct centity_t;

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
	short	frame;
	int		startTime;					// time of animation start
	int		oldTime;					// time of current animation frame start
	int		time;						// time of next animation frame
	bool	freezed;					// when true, display last animation frame until animation change
	CVec3	angles;
	bool	rotating[3];
};

struct playerAnim_t						//?? sometimes should initialize this struct
{
	animState_t legsAnim, torsoAnim;
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
			// animation parameters
			bool	fixedLegs, fixedTorso;
			bool	fixedAll;			// do not rotate head/torso/legs at all
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
void CL_LoadClientinfo (clientInfo_t &ci, const char *s, bool loadWeapons = true);
void RunAnimation (clientInfo_t &ci, animState_t &as, int animNum = ANIM_NOCHANGE);
int ParsePlayerEntity (centity_t *cent, clientInfo_t *ci, clEntityState_t *st, const entity_t &ent, entity_t *buf, int maxEnts, int weaponIndex = -1);
