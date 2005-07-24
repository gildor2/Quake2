//?? rename to ConfigVar.h ??

// cvar_t->flags values

#define	CVAR_ARCHIVE		0x00001	// set to cause it to be saved to config file (config.cfg)

/*?? may be, remove game dependency (leave Quake2 consts), because Quake3 game library will create
 *	vars via wrapper function (which can reencode CVAR_XXX flags from Q3 to Q2 consts), and game
 *	will not be able to read var->flags field again (no this field in vmCvar_t struc)
 */

#if QUAKE2 //------------------------------------------------------------------

// Old flags
#define	CVAR_USERINFO		0x00002	// added to userinfo when changed, then sent to senver
#define	CVAR_SERVERINFO		0x00004	// added to serverinfo when changed
#define	CVAR_NOSET			0x00008	// don't allow change from console at all, but can be set from the command line
#define	CVAR_LATCH			0x00010	// save changes until server restart
// New flags
#define	CVAR_USER_CREATED	0x00020	// created by a set command
#define CVAR_GAME_CREATED	0x00040	// created from game library
#define CVAR_CHEAT			0x00080	// will be reset to its default value when cheat protection active

#elif QUAKE3 //----------------------------------------------------------------

// Old flags
#define	CVAR_USERINFO		0x00002	// sent to server on connect or change
#define	CVAR_SERVERINFO		0x00004	// sent in response to front end requests
#define	CVAR_SYSTEMINFO		0x00008	// these cvars will be duplicated on all clients
#define	CVAR_NOSET			0x00010	// don't allow change from console at all, but can be set from the command line
#define CVAR_INIT			CVAR_NOSET		//?? alias
#define	CVAR_LATCH			0x00020	// will only change when C code next does a Cvar_Get(), so it can't be changed
									// without proper initialization. "modified" will be set, even though the value
									// hasn't changed yet
#define	CVAR_ROM			0x00040	// display only, cannot be set by user at all
#define	CVAR_USER_CREATED	0x00080	// created by a set command
#define	CVAR_TEMP			0x00100	// can be set even when cheats are disabled, but is not archived
#define CVAR_CHEAT			0x00200	// can not be changed if cheats are disabled
#define CVAR_NORESTART		0x00400	// do not clear when a cvar_restart is issued
// New flags
#define CVAR_GAME_CREATED	0x00800	// created by game library

#else // non-Quake game -------------------------------------------------------

/*?? QUAKE NOTES (may be implemented after Q3 source release ...):
 *	1. Cleanup Quake3 flags (check: may be, some of them completely useless) and place here only usefull things
 *		usefull: USERINFO, SERVERINFO, NOSET (== ROM?), LATCH, CHEAT, USER_CREATED, GAME_CREATED
 *		useless??: SYSTEMINFO, ROM (== NOSET?), TEMP, NORESTART
 *	2. Make define QUAKE_GAME == QUAKE2||QUAKE3, and remove some flags (from cpp source too), which usefull
 *		for game engine only (GAME_CREATED, USERINFO, SERVERINFO, CHEAT etc)
 *	3. May be, NOSET can be set from cmdline, but ROM will be always in its default value (in Q3 NOSET==ROM, can be set from
 *		cmdline only ??)
 */

#define CVAR_NOSET
#define CVAR_LATCH
#define CVAR_CHEAT
//?? CVAR_USERARCHIVE -- when engine was not created this var as CVAR_ARCHIVE, but user created with "seta" command

#endif //----------------------------------------------------------------------

#define CVAR_FLAG_MASK		0x0FFFF	// mask of stored cvar flags

// Non-stored flags:
#define CVAR_NODEFAULT		0x10000	// do not store "default" value from this Cvar_Get() call
#define CVAR_UPDATE			0x20000	// set "modified" field after Cvar_Get() call
#define CVAR_NOUPDATE		0x40000	// reset "modified" field ...

#define	CVAR_BUFFER_SIZE	16		// size of buffer for var->string inside cvar_t


/* NOTE: layout of cvar_t fields is the same as in Quake2 (with a few additional fields), to allow game
 *	libraries to use them; Quake3 does not require to be a fixed field, because its VM uses vmCvar_t
 *	wrapper anyway.
 */
class CORE_API cvar_t
{
public:
	// Old fields (required for compatibility with Quake2 game libraries)
	char	*name;
	char	*string;
	char	*latchedString;			// for CVAR_LATCH vars
#ifdef QUAKE2
	unsigned flags;
	bool	modified;				// set each time the cvar is changed
	byte	pad[3];					// align to 4 bytes (Q2 qboolean)
#else
	short	flags;					// 2 bytes enough (even for Quake3)
	bool	modified;
#endif
	float	value;					// atof(string)
	cvar_t	*next;
	// New fields
	char	*resetString;			// default cvar value (unset for user-created vars)
	int		integer;				// atoi(string)
	cvar_t	*hashNext;
	int		stringLength;			// size of buffer, allocated for holding var->string (or 0 if var->buf used)
	char	buf[CVAR_BUFFER_SIZE];
	// Functions
	void Set (int value);
	void Set (float value);
	void Set (const char *value);
};


CORE_API cvar_t *Cvar_Get (const char *name, const char *value, int flags = 0);

CORE_API cvar_t *Cvar_Set (const char *name, int *value);
CORE_API cvar_t *Cvar_Set (const char *name, float *value);
CORE_API cvar_t *Cvar_Set (const char *name, const char *value);
