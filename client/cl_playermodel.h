// declared later in client.h:
struct clEntityState_t;
struct centity_t;


struct animation_t
{
	short	firstFrame;
	short	numFrames;
	short	loopFrames;					// number of last frames, which will be looped
	bool	reversed;					// true if animation is reversed
	int		frameLerp;					// msec between frames (rename ??)
};


struct animState_t
{
	short	animNum;					// number of current animation
	short	nextAnimNum;				// number of animation, which will be launched after current
	short	oldFrame;					// old frame numbers (may be, from another animation)
	short	frame;
	int		startTime;					// time of animation start
	int		oldTime;					// time of current animation frame start
	int		time;						// time of next animation frame
	bool	completed;					// when true, display last animation frame until animation change
	CVec3	angles;
	bool	rotating[3];
};


struct weaponInfo_t						// weapon for Quake3 player model
{
	float	drawScale;
	CVec3	origin;
	CRenderModel *model;
	CBasicImage *skin;
};


#define MAX_CLIENTWEAPONMODELS	32

struct clientInfo_t
{
	unsigned id;						// for detecting changes
	char	playerName[MAX_QPATH];
	char	modelName[MAX_QPATH];
	char	iconName[MAX_QPATH];
	bool	isQ3model;
	bool	isValidModel;
	char	modelGender;				// 'm' - male, 'f' - female, 'n' - neutral
	CBasicImage	*icon;					// used as "!= NULL" only
										//?? may be used instead of iconName[] for drawing; require CBasicImage.Draw()
	// data for Quake2 player model (use one of Q3 model info??)
	CModelSkin	md2skin;
	CRenderModel *md2model;
	// data for Quake3 player model
	// ... models
	CRenderModel *legsModel;
	CRenderModel *torsoModel;
	CRenderModel *headModel;
	// ... skins
	CModelSkin	legsSkin;
	CModelSkin	torsoSkin;
	CModelSkin	headSkin;
	// ... animation parameters (should be linked to model ??)
	bool	fixedLegs, fixedTorso;
	bool	fixedAll;			// do not rotate head/torso/legs at all
	animation_t animations[MAX_TOTALANIMATIONS];
	// weapon info
	weaponInfo_t weaponModel[MAX_CLIENTWEAPONMODELS];
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
int ParsePlayerEntity (centity_t &cent, clientInfo_t &ci, clEntityState_t *st, const entity_t &ent, entity_t *buf, int maxEnts, int weaponIndex = -1);
