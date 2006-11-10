/*=============================================================================
	Game interface strucs/consts
=============================================================================*/

// game print flags; for svc_print command; client system uses PRINT_CHAT only; server - PRINT_HIGH
enum
{
	PRINT_LOW,						// pickup messages
	PRINT_MEDIUM,					// death messages
	PRINT_HIGH,						// critical messages
	PRINT_CHAT						// chat messages
};


// destination class for gi.multicast()
enum multicast_t
{
	MULTICAST_ALL,
	MULTICAST_PHS,					// PHS support removed: == MULTICAST_ALL
	MULTICAST_PVS,
	MULTICAST_ALL_R,
	MULTICAST_PHS_R,				// .. == MULTICAST_ALL_R
	MULTICAST_PVS_R
};

#define	MAX_EDICTS			1024	// must change protocol to increase more


/*-----------------------------------------------------------------------------
	Player movement code
-----------------------------------------------------------------------------*/

// pmove_state_t is the information necessary for client side movement prediction
enum pmtype_t
{
	// can accelerate and turn
	PM_NORMAL,
	PM_SPECTATOR,
	// no acceleration or turning
	PM_DEAD,
	PM_GIB,		// different bounding box
	PM_FREEZE
};

// pmove->pm_flags
#define	PMF_DUCKED			1
#define	PMF_JUMP_HELD		2
#define	PMF_ON_GROUND		4
#define	PMF_TIME_WATERJUMP	8		// pm_time is waterjump
#define	PMF_TIME_LAND		16		// pm_time is time before rejump
#define	PMF_TIME_TELEPORT	32		// pm_time is non-moving time
#define PMF_NO_PREDICTION	64		// temporarily disables prediction (used for grappling hook)

// This structure needs to be communicated bit-accurate from the server to the client to
// guarantee that prediction stays in sync, so no floats are used. If any part of the game
// code modifies this struct, it will result in a prediction error of some degree.
struct pmove_state_t
{
	pmtype_t	pm_type;

	short		origin[3];			// fixed 12.3
	short		velocity[3];		// fixed 12.3; unit/sec
	byte		pm_flags;			// ducked, jump_held, etc
	byte		pm_time;			// each unit = 8 ms
	short		gravity;
	short		delta_angles[3];	// add to command angles to get view direction; modified by game code only
};


// button bits
#define	BUTTON_ATTACK		1
#define	BUTTON_USE			2
#define	BUTTON_ANY			128		// any key whatsoever


// usercmd_t is sent to the server each client frame
struct usercmd_t
{
	byte	msec;
	byte	buttons;
	short	angles[3];
	short	forwardmove, sidemove, upmove;
	byte	impulse;				// remove? (unused !!)
	byte	lightlevel;				// light level the player is standing on
};


#define	MAXTOUCH	32
struct pmove_t
{
	// state (in / out)
	pmove_state_t s;

	// command (in)
	usercmd_t	cmd;
	bool		snapinitial;		// if s has been changed outside pmove
	byte		pad[3];				// qboolean pad

	// results (out)
	int			numtouch;
	struct edict_s	*touchents[MAXTOUCH];

	CVec3		viewangles;			// clamped
	float		viewheight;

	CBox		bounds;				// bounding box

	struct edict_s	*groundentity;
	int			watertype;
	int			waterlevel;

	// callbacks to test the world
	trace0_t*	(*trace) (trace0_t &trace, const CVec3 &start, const CVec3 &mins, const CVec3 &maxs, const CVec3 &end);
	int			(*pointcontents) (const CVec3 &point);
};


//=============================================================================

// sound channels
// channel 0 never willingly overrides
// other channels (1-7) allways override a playing sound on that channel
enum
{
	CHAN_AUTO,
	CHAN_WEAPON,
	CHAN_VOICE,
	CHAN_ITEM,
	CHAN_BODY,
	// modifier flags
	CHAN_NO_PHS_ADD = 8,	// send to all clients, not just ones in PHS (ATTN_NONE will also do this);
							// since PHS support removed, this flag is ignored
	CHAN_RELIABLE   = 16	// send by reliable message, not datagram
};


// sound attenuation values
enum
{
	ATTN_NONE,				// full volume the entire level
	ATTN_NORM,
	ATTN_IDLE,
	ATTN_STATIC				// diminish very rapidly with distance
};


enum
{
	MATERIAL_SILENT,		// no footstep sounds (and no bullethit sounds)
	MATERIAL_CONCRETE,		// standard sounds
	MATERIAL_FABRIC,		// rug
	MATERIAL_GRAVEL,		// gravel
	MATERIAL_METAL,			// metalh
	MATERIAL_METAL_L,		// metall
	MATERIAL_SNOW,			// tin (replace with pure snow from AHL??)
	MATERIAL_TIN,
	MATERIAL_TILE,			// marble (similar to concrete, but slightly muffled sound)
	MATERIAL_WOOD,			// wood
	MATERIAL_WATER,
	MATERIAL_GLASS,
	MATERIAL_DIRT,
	//!! reserved for constant MATERIAL_COUNT, but not implemented now:
	MATERIAL_R0,
	MATERIAL_R1,
	MATERIAL_R2,
	MATERIAL_R3,

	MATERIAL_COUNT			// must be last
};


/*
==========================================================

  ELEMENTS COMMUNICATED ACROSS THE NET

==========================================================
*/

// entity_state_t is the information conveyed from the server in an update message
// about entities that the client will need to render in some way
// this is a basic structure, supported by game library
struct entity_state_t
{
	int		number;			// edict index; real size is "short"

	CVec3	origin;
	CVec3	angles;
	CVec3	old_origin;		// for lerping
	int		modelindex;
	int		modelindex2, modelindex3, modelindex4;	// weapons, CTF flags, etc; real size is "byte"
							// modelindex2 used for weapon, 3 - for CTF flags, 4 - unused
	int		frame;			// real size is "short"
	int		skinnum;
	unsigned effects;
	unsigned renderfx;
	int		solid;			// 16-bit int; for client side prediction, 8*bits[0-4] is x/y radius; 8*bits[5-9] is z
							// down distance, 8*bits[10-15] is z up; gi.linkentity() sets this; real size is "short"
	int		sound;			// for looping sounds, to guarantee shutoff; real size is "byte"
	int		event;			// impulse events -- muzzle flashes, footsteps, etc; events only go out for a single frame,
							// they are automatically cleared each frame; real size is "byte"
};

// entityStateEx_t is extended entity_state_t structure
struct entityStateEx_t : entity_state_t
{
	// added since extended protocol v1
	unsigned anim;			// legs, torso animation + some features
	// functions to support additional fields
	inline void SetAnim(int legs, int torso, int moveDir = 0, float pitch = 0)
	{
		anim = legs | (torso << 6) | (moveDir << 12) | ((appRound(pitch) + 90) << 15);
	}
	inline void GetAnim(int &legs, int &torso, int &moveDir, float &pitch) const
	{
		legs    =   anim        & 0x3F;
		torso   =  (anim >> 6)  & 0x3F;
		moveDir =  (anim >> 12) & 7;
		pitch   = ((anim >> 15) & 0xFF) - 90.0f;
	}
};

//==============================================

#define	MAX_STATS	32		// max STAT_XXX value; limited by protocol: 1 bit of int32 for each stats


// player_state_t is the information needed in addition to pmove_state_t
// to rendered a view.  There will only be 10 (sv_fps) player_state_t sent
// each second, but the number of pmove_state_t changes will be relative to
// client frame rates
struct player_state_t
{
	pmove_state_t	pmove;	// for prediction

	// these fields do not need to be communicated bit-precise

	CVec3		viewangles;	// for fixed views
	CVec3		viewoffset;	// add to pmovestate->origin
	CVec3		kick_angles;// add to view direction to get render angles
							// set by weapon kicks, pain effects, etc

	CVec3		gunangles;
	CVec3		gunoffset;
	int			gunindex;
	int			gunframe;

	float		blend[4];	// rgba full screen effect
	float		fov;		// horizontal field of view
	int			rdflags;	// refdef flags

	short		stats[MAX_STATS]; // fast status bar updates
};


/*=============================================================================
	Network communication
=============================================================================*/

// entity_state_t->effects
// Effects are things handled on the client side (lights, particles, frame animations)
// that happen constantly on the given entity.
// An entity that has effects will be sent to the client
// even if it has a zero index model.
#define	EF_ROTATE			0x00000001		// rotate (bonus items)
#define	EF_GIB				0x00000002		// blood trail
#define	EF_BLASTER			0x00000008		// light + trail; when EF_TRACKER set - different trail == EF_BLASTER2
#define	EF_ROCKET			0x00000010		// light + trail
#define	EF_GRENADE			0x00000020		// trail
#define	EF_HYPERBLASTER		0x00000040		// light (when EF_TRACKER set -- neg. light)
#define	EF_BFG				0x00000080		// light + trail
#define EF_COLOR_SHELL		0x00000100
#define EF_POWERSCREEN		0x00000200		// special
#define	EF_ANIM01			0x00000400		// automatically cycle between frames 0 and 1 at 2 hz
#define	EF_ANIM23			0x00000800		// automatically cycle between frames 2 and 3 at 2 hz
#define EF_ANIM_ALL			0x00001000		// automatically cycle through all frames at 2hz
#define EF_ANIM_ALLFAST		0x00002000		// automatically cycle through all frames at 10hz
#define	EF_FLIES			0x00004000		// draw flies around entity origin
#define	EF_QUAD				0x00008000		// == blue shell
#define	EF_PENT				0x00010000		// == red shell
#define	EF_TELEPORTER		0x00020000		// particle fountain
#define EF_FLAG1			0x00040000		// red flag: red light + trail
#define EF_FLAG2			0x00080000		// blue flag: blue light + trail
// XATRIX
#define EF_IONRIPPER		0x00100000		// light + trail
#define EF_GREENGIB			0x00200000		// green blood trail
#define	EF_BLUEHYPERBLASTER 0x00400000		// blue light
#define EF_SPINNINGLIGHTS	0x00800000		// red light, rotating around entity origin
#define EF_PLASMA			0x01000000		// light + (if EF_ANIM_ALLFAST set) blaster trail
#define EF_TRAP				0x02000000		// light + trail
// ROGUE
#define EF_TRACKER			0x04000000		// trail + neg. light ; see EF_BLASTER too
#define	EF_DOUBLE			0x08000000		// color shell
#define	EF_SPHERETRANS		0x10000000		// special (+ EF_TRACKERTRAIL flag usage for alpha control)
#define EF_TAGTRAIL			0x20000000		// light + trail
#define EF_HALF_DAMAGE		0x40000000		// color shell
#define EF_TRACKERTRAIL		0x80000000		// when EF_TRACKER set - pulse neg. light; when not set - neg. light + tracker shell


// entity_state_t->renderfx flags
#define	RF_MINLIGHT			0x00000001		// allways have some light (viewmodel)
#define	RF_VIEWERMODEL		0x00000002		// don't draw through eyes, only mirrors
#define	RF_WEAPONMODEL		0x00000004		// only draw through eyes
#define	RF_FULLBRIGHT		0x00000008		// allways draw full intensity
#define	RF_DEPTHHACK		0x00000010		// for view weapon Z crunching
#define	RF_TRANSLUCENT		0x00000020
#define	RF_FRAMELERP		0x00000040		// unused; set but game, but useless
#define RF_BEAM				0x00000080		// used for game->server->client only (not for renderer)
#define	RF_CUSTOMSKIN		0x00000100		// unused
#define	RF_GLOW				0x00000200		// pulse lighting for bonus items
#define RF_SHELL_RED		0x00000400
#define	RF_SHELL_GREEN		0x00000800
#define RF_SHELL_BLUE		0x00001000
//ROGUE
#define RF_IR_VISIBLE		0x00008000		// set IR skin for entity
#define	RF_SHELL_DOUBLE		0x00010000
#define	RF_SHELL_HALF_DAM	0x00020000
#define RF_USE_DISGUISE		0x00040000		// set disguise skin for entity
// 4.00
#define RF_MIRROR			0x20000000		// draw mirrored model (for right->left hand weapons)
#define RF_BBOX				0x40000000


// muzzle flashes / player effects
enum
{
	MZ_BLASTER,
	MZ_MACHINEGUN,
	MZ_SHOTGUN,
	MZ_CHAINGUN1, MZ_CHAINGUN2, MZ_CHAINGUN3,
	MZ_RAILGUN,
	MZ_ROCKET,
	MZ_GRENADE,
	MZ_LOGIN,
	MZ_LOGOUT,
	MZ_RESPAWN,
	MZ_BFG,
	MZ_SSHOTGUN,
	MZ_HYPERBLASTER,
	MZ_ITEMRESPAWN,
	// XATRIX
	MZ_IONRIPPER,
	MZ_BLUEHYPERBLASTER,
	MZ_PHALANX,
	MZ_SILENCED =128,		// bit flag OR'ed with one of the above numbers
	// ROGUE
	MZ_ETF_RIFLE = 30,
	MZ_UNUSED,
	MZ_SHOTGUN2,
	MZ_HEATBEAM,
	MZ_BLASTER2,
	MZ_TRACKER,
	MZ_NUKE1, MZ_NUKE2, MZ_NUKE4, MZ_NUKE8
};


// monster muzzle flashes
enum
{
	MZ2_TANK_BLASTER_1 = 1, MZ2_TANK_BLASTER_2, MZ2_TANK_BLASTER_3,
	MZ2_TANK_MACHINEGUN_1, MZ2_TANK_MACHINEGUN_2, MZ2_TANK_MACHINEGUN_3,
	MZ2_TANK_MACHINEGUN_4, MZ2_TANK_MACHINEGUN_5, MZ2_TANK_MACHINEGUN_6,
	MZ2_TANK_MACHINEGUN_7, MZ2_TANK_MACHINEGUN_8, MZ2_TANK_MACHINEGUN_9,
	MZ2_TANK_MACHINEGUN_10, MZ2_TANK_MACHINEGUN_11, MZ2_TANK_MACHINEGUN_12,
	MZ2_TANK_MACHINEGUN_13, MZ2_TANK_MACHINEGUN_14, MZ2_TANK_MACHINEGUN_15,
	MZ2_TANK_MACHINEGUN_16, MZ2_TANK_MACHINEGUN_17, MZ2_TANK_MACHINEGUN_18,
	MZ2_TANK_MACHINEGUN_19,
	MZ2_TANK_ROCKET_1, MZ2_TANK_ROCKET_2, MZ2_TANK_ROCKET_3,

	MZ2_INFANTRY_MACHINEGUN_1, MZ2_INFANTRY_MACHINEGUN_2,
	MZ2_INFANTRY_MACHINEGUN_3, MZ2_INFANTRY_MACHINEGUN_4,
	MZ2_INFANTRY_MACHINEGUN_5, MZ2_INFANTRY_MACHINEGUN_6,
	MZ2_INFANTRY_MACHINEGUN_7, MZ2_INFANTRY_MACHINEGUN_8,
	MZ2_INFANTRY_MACHINEGUN_9, MZ2_INFANTRY_MACHINEGUN_10,
	MZ2_INFANTRY_MACHINEGUN_11, MZ2_INFANTRY_MACHINEGUN_12,
	MZ2_INFANTRY_MACHINEGUN_13,

	MZ2_SOLDIER_BLASTER_1, MZ2_SOLDIER_BLASTER_2,
	MZ2_SOLDIER_SHOTGUN_1, MZ2_SOLDIER_SHOTGUN_2,
	MZ2_SOLDIER_MACHINEGUN_1, MZ2_SOLDIER_MACHINEGUN_2,

	MZ2_GUNNER_MACHINEGUN_1, MZ2_GUNNER_MACHINEGUN_2,
	MZ2_GUNNER_MACHINEGUN_3, MZ2_GUNNER_MACHINEGUN_4,
	MZ2_GUNNER_MACHINEGUN_5, MZ2_GUNNER_MACHINEGUN_6,
	MZ2_GUNNER_MACHINEGUN_7, MZ2_GUNNER_MACHINEGUN_8,
	MZ2_GUNNER_GRENADE_1, MZ2_GUNNER_GRENADE_2,
	MZ2_GUNNER_GRENADE_3, MZ2_GUNNER_GRENADE_4,

	MZ2_CHICK_ROCKET_1,

	MZ2_FLYER_BLASTER_1, MZ2_FLYER_BLASTER_2,

	MZ2_MEDIC_BLASTER_1,

	MZ2_GLADIATOR_RAILGUN_1,

	MZ2_HOVER_BLASTER_1,

	MZ2_ACTOR_MACHINEGUN_1,

	MZ2_SUPERTANK_MACHINEGUN_1, MZ2_SUPERTANK_MACHINEGUN_2, MZ2_SUPERTANK_MACHINEGUN_3,
	MZ2_SUPERTANK_MACHINEGUN_4, MZ2_SUPERTANK_MACHINEGUN_5, MZ2_SUPERTANK_MACHINEGUN_6,
	MZ2_SUPERTANK_ROCKET_1, MZ2_SUPERTANK_ROCKET_2, MZ2_SUPERTANK_ROCKET_3,

	MZ2_BOSS2_MACHINEGUN_L1, MZ2_BOSS2_MACHINEGUN_L2, MZ2_BOSS2_MACHINEGUN_L3,
	MZ2_BOSS2_MACHINEGUN_L4, MZ2_BOSS2_MACHINEGUN_L5,
	MZ2_BOSS2_ROCKET_1, MZ2_BOSS2_ROCKET_2, MZ2_BOSS2_ROCKET_3, MZ2_BOSS2_ROCKET_4,

	MZ2_FLOAT_BLASTER_1,

	MZ2_SOLDIER_BLASTER_3, MZ2_SOLDIER_SHOTGUN_3, MZ2_SOLDIER_MACHINEGUN_3,
	MZ2_SOLDIER_BLASTER_4, MZ2_SOLDIER_SHOTGUN_4, MZ2_SOLDIER_MACHINEGUN_4,
	MZ2_SOLDIER_BLASTER_5, MZ2_SOLDIER_SHOTGUN_5, MZ2_SOLDIER_MACHINEGUN_5,
	MZ2_SOLDIER_BLASTER_6, MZ2_SOLDIER_SHOTGUN_6, MZ2_SOLDIER_MACHINEGUN_6,
	MZ2_SOLDIER_BLASTER_7, MZ2_SOLDIER_SHOTGUN_7, MZ2_SOLDIER_MACHINEGUN_7,
	MZ2_SOLDIER_BLASTER_8, MZ2_SOLDIER_SHOTGUN_8, MZ2_SOLDIER_MACHINEGUN_8,

	MZ2_MAKRON_BFG,
	MZ2_MAKRON_BLASTER_1, MZ2_MAKRON_BLASTER_2, MZ2_MAKRON_BLASTER_3,
	MZ2_MAKRON_BLASTER_4, MZ2_MAKRON_BLASTER_5, MZ2_MAKRON_BLASTER_6,
	MZ2_MAKRON_BLASTER_7, MZ2_MAKRON_BLASTER_8, MZ2_MAKRON_BLASTER_9,
	MZ2_MAKRON_BLASTER_10, MZ2_MAKRON_BLASTER_11, MZ2_MAKRON_BLASTER_12,
	MZ2_MAKRON_BLASTER_13, MZ2_MAKRON_BLASTER_14, MZ2_MAKRON_BLASTER_15,
	MZ2_MAKRON_BLASTER_16, MZ2_MAKRON_BLASTER_17,
	MZ2_MAKRON_RAILGUN_1,
	MZ2_JORG_MACHINEGUN_L1, MZ2_JORG_MACHINEGUN_L2, MZ2_JORG_MACHINEGUN_L3,
	MZ2_JORG_MACHINEGUN_L4, MZ2_JORG_MACHINEGUN_L5, MZ2_JORG_MACHINEGUN_L6,
	MZ2_JORG_MACHINEGUN_R1, MZ2_JORG_MACHINEGUN_R2, MZ2_JORG_MACHINEGUN_R3,
	MZ2_JORG_MACHINEGUN_R4, MZ2_JORG_MACHINEGUN_R5, MZ2_JORG_MACHINEGUN_R6,
	MZ2_JORG_BFG_1,
	MZ2_BOSS2_MACHINEGUN_R1, MZ2_BOSS2_MACHINEGUN_R2, MZ2_BOSS2_MACHINEGUN_R3,
	MZ2_BOSS2_MACHINEGUN_R4, MZ2_BOSS2_MACHINEGUN_R5,

	// ROGUE
	MZ2_CARRIER_MACHINEGUN_L1, MZ2_CARRIER_MACHINEGUN_R1,
	MZ2_CARRIER_GRENADE,
	MZ2_TURRET_MACHINEGUN,
	MZ2_TURRET_ROCKET,
	MZ2_TURRET_BLASTER,
	MZ2_STALKER_BLASTER,
	MZ2_DAEDALUS_BLASTER,
	MZ2_MEDIC_BLASTER_2,
	MZ2_CARRIER_RAILGUN,
	MZ2_WIDOW_DISRUPTOR, MZ2_WIDOW_BLASTER, MZ2_WIDOW_RAIL,
	MZ2_WIDOW_PLASMABEAM,			// PMM - not used
	MZ2_CARRIER_MACHINEGUN_L2,
	MZ2_CARRIER_MACHINEGUN_R2,
	MZ2_WIDOW_RAIL_LEFT,
	MZ2_WIDOW_RAIL_RIGHT,
	MZ2_WIDOW_BLASTER_SWEEP1, MZ2_WIDOW_BLASTER_SWEEP2, MZ2_WIDOW_BLASTER_SWEEP3,
	MZ2_WIDOW_BLASTER_SWEEP4, MZ2_WIDOW_BLASTER_SWEEP5, MZ2_WIDOW_BLASTER_SWEEP6,
	MZ2_WIDOW_BLASTER_SWEEP7, MZ2_WIDOW_BLASTER_SWEEP8, MZ2_WIDOW_BLASTER_SWEEP9,
	MZ2_WIDOW_BLASTER_100, MZ2_WIDOW_BLASTER_90, MZ2_WIDOW_BLASTER_80,
	MZ2_WIDOW_BLASTER_70, MZ2_WIDOW_BLASTER_60, MZ2_WIDOW_BLASTER_50,
	MZ2_WIDOW_BLASTER_40, MZ2_WIDOW_BLASTER_30, MZ2_WIDOW_BLASTER_20,
	MZ2_WIDOW_BLASTER_10, MZ2_WIDOW_BLASTER_0,
	MZ2_WIDOW_BLASTER_10L, MZ2_WIDOW_BLASTER_20L, MZ2_WIDOW_BLASTER_30L,
	MZ2_WIDOW_BLASTER_40L, MZ2_WIDOW_BLASTER_50L, MZ2_WIDOW_BLASTER_60L,
	MZ2_WIDOW_BLASTER_70L,
	MZ2_WIDOW_RUN_1, MZ2_WIDOW_RUN_2, MZ2_WIDOW_RUN_3,
	MZ2_WIDOW_RUN_4, MZ2_WIDOW_RUN_5, MZ2_WIDOW_RUN_6,
	MZ2_WIDOW_RUN_7, MZ2_WIDOW_RUN_8,
	MZ2_CARRIER_ROCKET_1, MZ2_CARRIER_ROCKET_2,
	MZ2_CARRIER_ROCKET_3, MZ2_CARRIER_ROCKET_4,
	MZ2_WIDOW2_BEAMER_1, MZ2_WIDOW2_BEAMER_2, MZ2_WIDOW2_BEAMER_3,
	MZ2_WIDOW2_BEAMER_4, MZ2_WIDOW2_BEAMER_5,
	MZ2_WIDOW2_BEAM_SWEEP_1, MZ2_WIDOW2_BEAM_SWEEP_2, MZ2_WIDOW2_BEAM_SWEEP_3,
	MZ2_WIDOW2_BEAM_SWEEP_4, MZ2_WIDOW2_BEAM_SWEEP_5, MZ2_WIDOW2_BEAM_SWEEP_6,
	MZ2_WIDOW2_BEAM_SWEEP_7, MZ2_WIDOW2_BEAM_SWEEP_8, MZ2_WIDOW2_BEAM_SWEEP_9,
	MZ2_WIDOW2_BEAM_SWEEP_10, MZ2_WIDOW2_BEAM_SWEEP_11
};



// temp entity events
// Temp entity events are for things that happen at a location seperate from any existing entity.
// Temporary entity messages are explicitly constructed and broadcast.
enum
{
	TE_GUNSHOT,
	TE_BLOOD,
	TE_BLASTER,
	TE_RAILTRAIL,
	TE_SHOTGUN,
	TE_EXPLOSION1,
	TE_EXPLOSION2,
	TE_ROCKET_EXPLOSION,
	TE_GRENADE_EXPLOSION,	// in original Q2 same as TE_EXPLOSION2
	TE_SPARKS,
	TE_SPLASH,
	TE_BUBBLETRAIL,
	TE_SCREEN_SPARKS,
	TE_SHIELD_SPARKS,
	TE_BULLET_SPARKS,
	TE_LASER_SPARKS,
	TE_PARASITE_ATTACK,
	TE_ROCKET_EXPLOSION_WATER,
	TE_GRENADE_EXPLOSION_WATER,
	TE_MEDIC_CABLE_ATTACK,
	TE_BFG_EXPLOSION,
	TE_BFG_BIGEXPLOSION,
	TE_BOSSTPORT,			// used as '22' in a map, so DON'T RENUMBER !
	TE_BFG_LASER,
	TE_GRAPPLE_CABLE,
	TE_WELDING_SPARKS,
	TE_GREENBLOOD,
	TE_BLUEHYPERBLASTER,
	TE_PLASMA_EXPLOSION,
	TE_TUNNEL_SPARKS,
	// ROGUE
	TE_BLASTER2,
	TE_RAILTRAIL2,
	TE_FLAME,
	TE_LIGHTNING,
	TE_DEBUGTRAIL,
	TE_PLAIN_EXPLOSION,
	TE_FLASHLIGHT,
	TE_FORCEWALL,
	TE_HEATBEAM,
	TE_MONSTER_HEATBEAM,
	TE_STEAM,
	TE_BUBBLETRAIL2,
	TE_MOREBLOOD,
	TE_HEATBEAM_SPARKS,
	TE_HEATBEAM_STEAM,
	TE_CHAINFIST_SMOKE,
	TE_ELECTRIC_SPARKS,
	TE_TRACKER_EXPLOSION,
	TE_TELEPORT_EFFECT,
	TE_DBALL_GOAL,
	TE_WIDOWBEAMOUT,
	TE_NUKEBLAST,
	TE_WIDOWSPLASH,
	TE_EXPLOSION1_BIG,
	TE_EXPLOSION1_NP,
	TE_FLECHETTE,
	// extended protocol constants
	TE_RAILTRAIL_EXT = 128		// railgun trail with specified color and type
};



enum
{
	SPLASH_UNKNOWN,
	SPLASH_SPARKS,
	SPLASH_BLUE_WATER,
	SPLASH_BROWN_WATER,
	SPLASH_SLIME,
	SPLASH_LAVA,
	SPLASH_BLOOD,
	// extended protocol
	SPLASH_BULLET_BLUE_WATER,
	SPLASH_BULLET_BROWN_WATER
};


// player_state->stats[] indexes
enum
{
	STAT_HEALTH_ICON,
	STAT_HEALTH,
	STAT_AMMO_ICON,
	STAT_AMMO,
	STAT_ARMOR_ICON,
	STAT_ARMOR,
	STAT_SELECTED_ICON,
	STAT_PICKUP_ICON,
	STAT_PICKUP_STRING,
	STAT_TIMER_ICON,
	STAT_TIMER,
	STAT_HELPICON,
	STAT_SELECTED_ITEM,
	STAT_LAYOUTS,
	STAT_FRAGS,
	STAT_FLASHES,					// cleared each frame, 1 = health, 2 = armor
	STAT_CHASE,
	STAT_SPECTATOR
};



// entity_state_t->event values
// ertity events are for effects that take place reletive
// to an existing entities origin.  Very network efficient.
// All muzzle flashes really should be converted to events...
#define NUM_CAMPER_EVENTS	9

enum
{
	EV_NONE,
	EV_ITEM_RESPAWN,
	EV_FOOTSTEP,
	EV_FALLSHORT,
	EV_FALL,
	EV_FALLFAR,
	EV_PLAYER_TELEPORT,
	EV_OTHER_TELEPORT,

	// added since 4.00

	// ext protocol v0
	EV_FOOTSTEP0 = 100,
	EV_FALLSHORT0 = EV_FOOTSTEP0 + MATERIAL_COUNT,
	EV_SPECTATOR0 = EV_FALLSHORT0 + MATERIAL_COUNT,		//?? unused in v0
	EV_CAMPER0 = EV_SPECTATOR0 + 4,
	//?? for extending (rename EV_NEXT to something new):
	EV_NEXT = EV_CAMPER0 + NUM_CAMPER_EVENTS
};



// config strings are a general means of communication from the server to
// all connected clients. Each config string can be at most MAX_QPATH characters
// (exception: CS_STATUSBAR)

#define	MAX_MODELS			256
#define	MAX_SOUNDS			256
#define	MAX_IMAGES			256
#define	MAX_LIGHTSTYLES		256
#define	MAX_ITEMS			256
#define	MAX_CLIENTS			256
#define MAX_GENERAL			(MAX_CLIENTS*2)		// reserved for game developers

enum
{
	CS_NAME,
	CS_CDTRACK,
	CS_SKY,
	CS_SKYAXIS,									// %f %f %f format
	CS_SKYROTATE,
	CS_STATUSBAR,								// display program string (NOTE: few configstring indexes used)

	CS_AIRACCEL = 29,							// air acceleration control
	CS_MAXCLIENTS,
	CS_MAPCHECKSUM,								// for catching cheater maps

	CS_MODELS,
	CS_SOUNDS = CS_MODELS+MAX_MODELS,
	CS_IMAGES = CS_SOUNDS+MAX_SOUNDS,			// static 2D images (from pics/ directory)
	CS_LIGHTS = CS_IMAGES+MAX_IMAGES,			// lightstyles
	CS_ITEMS = CS_LIGHTS+MAX_LIGHTSTYLES,		// inventory to draw
	CS_PLAYERSKINS = CS_ITEMS+MAX_ITEMS,		// model/skin for each client
	CS_GENERAL = CS_PLAYERSKINS+MAX_CLIENTS,	// reserved

	MAX_CONFIGSTRINGS = CS_GENERAL+MAX_GENERAL
};


/*-----------------------------------------------------------------------------
	Stuff from Quake3 SDK
-----------------------------------------------------------------------------*/

// animations
enum
{
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

	// Q3 TA animations
	TORSO_GETFLAG,
	TORSO_GUARDBASE,
	TORSO_PATROL,
	TORSO_FOLLOWME,
	TORSO_AFFIRMATIVE,
	TORSO_NEGATIVE,

	MAX_ANIMATIONS,

	// computed animations
	LEGS_BACKCR,
	LEGS_BACKWALK,
#if 0
	// Q3 TA
	FLAG_RUN,
	FLAG_STAND,
	FLAG_STAND2RUN,
#endif

	MAX_TOTALANIMATIONS
};

#define ANIM_NOCHANGE		MAX_ANIMATIONS	// unused value


// torso rotate
enum
{
	LEGS_NEUTRAL,			// original legs position
	LEGS_LEFT_45,
	LEGS_LEFT_90,
	LEGS_RIGHT_45,
	LEGS_RIGHT_90
};
