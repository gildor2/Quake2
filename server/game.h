// This is a modified (for compile compatibility) version of game.h from quake2 SDK
// Most changes:
//	- vec3_t -> CVec3
//	- added "const" for "char*"

#define	GAME_API_VERSION	3

// need qboolean declaration for game library interface
typedef unsigned			qboolean;

// gi.BoxEdicts() can return a list of either solid or trigger entities; migrated from q_shared.h
#define	AREA_SOLID			1
#define	AREA_TRIGGERS		2

// edict->svflags

#define	SVF_NOCLIENT		0x00000001	// don't send entity to clients, even if it has effects
#define	SVF_DEADMONSTER		0x00000002	// treat as CONTENTS_DEADMONSTER for collision
#define	SVF_MONSTER			0x00000004	// treat as CONTENTS_MONSTER for collision

// edict->solid values

typedef enum
{
	SOLID_NOT,							// no interaction with other objects
	SOLID_TRIGGER,						// only touch when inside, after moving
	SOLID_BBOX,							// touch on edge
	SOLID_BSP							// bsp clip, touch on edge
} solid_t;


// link_t is only used for entity area links
struct link_t
{
	struct link_t *prev, *next;
};

#define	MAX_ENT_CLUSTERS	16

typedef struct edict_s edict_t;
typedef struct gclient_s gclient_t;

struct gclient_s
{
	player_state_t ps;					// communicated by server to clients
	int			ping;

	// the game dll can add anything it wants after this point in the structure
	// ...
};


struct edict_s
{
	entity_state_t s;
	struct gclient_s *client;
	qboolean	inuse;
	int			linkcount;

	// FIXME: move these fields to a server private sv_entity_t
	link_t		area;					// linked to a division node or leaf

	int			num_clusters;			// if -1, use headnode instead
	int			clusternums[MAX_ENT_CLUSTERS];
	int			headnode;				// unused if num_clusters != -1
	int			areanum, areanum2;

	int			svflags;				// SVF_NOCLIENT, SVF_DEADMONSTER, SVF_MONSTER, etc
	CBox		bounds;
	CBox		absBounds;
	CVec3		size;
	solid_t		solid;
	int			clipmask;
	edict_t		*owner;

	// the game dll can add anything it wants after this point in the structure
	// ...
};


/*-----------------------------------------------------------------------------
	Functions provided by the main engine
-----------------------------------------------------------------------------*/

struct game_import_t
{
	// special messages
	void	(*bprintf) (int printlevel, const char *fmt, ...);
	void	(*dprintf) (const char *fmt, ...);
	void	(*cprintf) (edict_t *ent, int printlevel, const char *fmt, ...);
	void	(*centerprintf) (edict_t *ent, const char *fmt, ...);
	void	(*sound) (edict_t *ent, int channel, int soundindex, float volume, float attenuation, float timeofs);
#if 0
	void	(*positioned_sound) (vec3_t origin, edict_t *ent, int channel, int soundinedex, float volume, float attenuation, float timeofs);
#else
	void	(*positioned_sound) (const CVec3 *origin, edict_t *ent, int channel, int soundinedex, float volume, float attenuation, float timeofs);
#endif

	// config strings hold all the index strings, the lightstyles,
	// and misc data like the sky definition and cdtrack.
	// All of the current configstrings are sent to clients when
	// they connect, and changes are sent to all connected clients.
	void	(*configstring) (int num, const char *string);

	void	(*error) (const char *fmt, ...);

	// the *index functions create configstrings and some internal server state
	int		(*modelindex) (const char *name);
	int		(*soundindex) (const char *name);
	int		(*imageindex) (const char *name);

	void	(*setmodel) (edict_t *ent, char *name);

	// collision detection
	trace_t	(*trace) (const CVec3 &start, const CVec3 *mins, const CVec3 *maxs, const CVec3 &end, edict_t *passent, int contentmask);
	int		(*pointcontents) (const CVec3 &point);
	qboolean (*inPVS) (const CVec3 &p1, const CVec3 &p2);
	qboolean (*inPHS) (const CVec3 &p1, const CVec3 &p2);
	void	(*SetAreaPortalState) (int portalnum, qboolean open);
	qboolean (*AreasConnected) (int area1, int area2);

	// an entity will never be sent to a client or used for collision
	// if it is not passed to linkentity.  If the size, position, or
	// solidity changes, it must be relinked.
	void	(*linkentity) (edict_t *ent);
	void	(*unlinkentity) (edict_t *ent);			// call before removing an interactive edict
	int		(*BoxEdicts) (const CVec3 &mins, const CVec3 &maxs, edict_t **list, int maxcount, int areatype);
	void	(*Pmove) (pmove_t *pmove);				// player movement code common with client prediction

	// network messaging
	void	(*multicast) (const CVec3 &origin, multicast_t to);
	void	(*unicast) (edict_t *ent, qboolean reliable);
	void	(*WriteChar) (int c);
	void	(*WriteByte) (int c);
	void	(*WriteShort) (int c);
	void	(*WriteLong) (int c);
	void	(*WriteFloat) (float f);
	void	(*WriteString) (char *s);
	void	(*WritePosition) (const CVec3 &pos);	// some fractional bits
	void	(*WriteDir) (const CVec3 *dir);			// single byte encoded, very coarse; dir may be NULL => {0,0,0}
	void	(*WriteAngle) (float f);

	// managed memory allocation
	void	*(*TagMalloc) (int size, int tag);
	void	(*TagFree) (void *block);
	void	(*FreeTags) (int tag);

	// console variable interaction
	cvar_t	*(*cvar) (const char *var_name, char *value, int flags);
	cvar_t	*(*cvar_set) (const char *var_name, const char *value);
	cvar_t	*(*cvar_forceset) (const char *var_name, const char *value);

	// ClientCommand and ServerCommand parameter access
	int		(*argc) ();
	char	*(*argv) (int n);
	char	*(*args) ();							// concatenation of all argv >= 1

	// add commands to the server console as if they were typed in
	// for map changing, etc
	void	(*AddCommandString) (const char *text);

	void	(*DebugGraph) (float value, int color);
};


/*-----------------------------------------------------------------------------
	Functions exported by the game subsystem
-----------------------------------------------------------------------------*/

struct game_export_t
{
	int		apiversion;

	// the init function will only be called when a game starts,
	// not each time a level is loaded.  Persistant data for clients
	// and the server can be allocated in init
	void	(*Init) ();
	void	(*Shutdown) ();

	// each new level entered will cause a call to SpawnEntities
	void	(*SpawnEntities) (const char *mapname, const char *entstring, const char *spawnpoint);

	// Read/Write Game is for storing persistant cross level information
	// about the world state and the clients.
	// WriteGame is called every time a level is exited.
	// ReadGame is called on a loadgame.
	void	(*WriteGame) (char *filename, qboolean autosave);
	void	(*ReadGame) (char *filename);

	// ReadLevel is called after the default map information has been
	// loaded with SpawnEntities
	void	(*WriteLevel) (char *filename);
	void	(*ReadLevel) (char *filename);

	qboolean (*ClientConnect) (edict_t *ent, char *userinfo);
	void	(*ClientBegin) (edict_t *ent);
	void	(*ClientUserinfoChanged) (edict_t *ent, char *userinfo);
	void	(*ClientDisconnect) (edict_t *ent);
	void	(*ClientCommand) (edict_t *ent);
	void	(*ClientThink) (edict_t *ent, usercmd_t *cmd);

	void	(*RunFrame) ();

	// ServerCommand will be called when an "sv <command>" command is issued on the
	// server console.
	// The game can issue gi.argc() / gi.argv() commands to get the rest
	// of the parameters
	void	(*ServerCommand) ();

	// global variables shared between game and server

	// The edict array is allocated in the game dll so it
	// can vary in size from one game to another.
	//
	// The size will be fixed when ge->Init() is called
	struct edict_s *edicts;
	int		edict_size;
	int		num_edicts;		// current number, <= max_edicts
	int		max_edicts;
};
