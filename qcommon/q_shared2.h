/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#ifndef QSHARED_H
#define QSHARED_H

#ifndef _WIN32

#define DLL_EXPORT
#define NORETURN

#endif


// this is the define for determining if we have an asm version of a C function
#if (defined _M_IX86 || defined __i386__)
#define id386	1
#else
#define id386	0
#endif


#define BIG_NUMBER			0x1000000


//============================================================================


// angle indexes
enum {
	PITCH,							// up / down
	YAW,							// left / right
	ROLL							// fall over
};

#define	MAX_STRING_CHARS	1024	// max length of a string passed to TokenizeString()
#define	MAX_STRING_TOKENS	80		// max tokens resulting from TokenizeString()

#define	MAX_QPATH			64		// max length of a quake game pathname
#define	MAX_OSPATH			128		// max length of a filesystem pathname

// per-level limits
#define	MAX_CLIENTS			256		// absolute limit
#define	MAX_EDICTS			1024	// must change protocol to increase more
#define	MAX_LIGHTSTYLES		256
#define	MAX_MODELS			256		// these are sent over the net as bytes
#define	MAX_SOUNDS			256		// so they cannot be blindly increased
#define	MAX_IMAGES			256
#define	MAX_ITEMS			256
#define MAX_GENERAL			(MAX_CLIENTS*2)	// general config strings


// game print flags; for svc_print command; client system uses PRINT_CHAT only; server - PRINT_HIGH
enum {
	PRINT_LOW,						// pickup messages
	PRINT_MEDIUM,					// death messages
	PRINT_HIGH,						// critical messages
	PRINT_CHAT						// chat messages
};


// destination class for gi.multicast()
typedef enum
{
	MULTICAST_ALL,
	MULTICAST_PHS,
	MULTICAST_PVS,
	MULTICAST_ALL_R,
	MULTICAST_PHS_R,
	MULTICAST_PVS_R
} multicast_t;


/*
==============================================================

MATHLIB

==============================================================
*/

#ifndef M_PI
#define M_PI		3.14159265358979323846	// matches value in gcc v2 math.h
#endif

extern vec3_t vec3_origin;	// really, should be "const", but LOTS of functions uses this without "const" specifier ...

float NormalizeColor (const vec3_t in, vec3_t out);
float NormalizeColor255 (const vec3_t in, vec3_t out);

// rename to CLAMP_COLOR255 ??
#define NORMALIZE_COLOR255(r,g,b)	\
	if ((r|g|b) > 255)	\
	{					\
		int		m;		\
		m = max(r,g);	\
		m = max(m,b);	\
		m = 255 * 256 / m;	\
		r = r * m >> 8;	\
		g = g * m >> 8;	\
		b = b * m >> 8;	\
	}

float ClampColor255 (const vec3_t in, vec3_t out);

float VectorNormalizeFast (vec3_t v);
float Q_rsqrt (float number);
#define SQRTFAST(x)		(x * Q_rsqrt(x))

#if !defined C_ONLY && !defined __linux__ && !defined __sgi

//?? use C++ ...cast<>
#define uint_cast(f)	(* (unsigned *) &(f))
#define IsNegative(f)	(uint_cast(f) >> 31)
#define FAbsSign(f,d,s)	\
	{							\
		unsigned mask = uint_cast(f) & 0x80000000;	\
		uint_cast(d) = uint_cast(f) ^ mask;			\
		s = mask >> 31;			\
	}
#define FNegate(f,d)			\
	{							\
		uint_cast(d) = uint_cast(f) ^ 0x80000000;	\
	}

#else
#define appRound(f)	(int) (f >= 0 ? (int)(f+0.5f) : (int)(f-0.5f))
#define appFloor(f)	((int)floor(f))
#define appCeil(f)	((int)ceil(f))

#define IsNegative(f)	(f<0)
#define FAbsSign(f,d,s)			\
	{							\
		s = IsNegative(f);		\
		d = s ? -f : f;			\
	}
#define FNegate(f,d)			\
	{	d = -f;		}

#endif

typedef struct	// hack for VectorCopy
{
	float x, y, z;
} vec3a_t;

#define DotProduct(x,y)			(x[0]*y[0]+x[1]*y[1]+x[2]*y[2])
#define VectorSubtract(a,b,c)	(c[0]=a[0]-b[0],c[1]=a[1]-b[1],c[2]=a[2]-b[2])
#define VectorAdd(a,b,c)		(c[0]=a[0]+b[0],c[1]=a[1]+b[1],c[2]=a[2]+b[2])
//#define VectorCopy(a,b)			(b[0]=a[0],b[1]=a[1],b[2]=a[2])
//#define VectorCopy(a,b)			memcpy(b, a, sizeof(vec3_t))		// should be faster when "memcpy" is intrinsic
#define VectorCopy(a,b)			(*(vec3a_t*)(b)=*(vec3a_t*)(a))
#define VectorClear(a)			(a[0]=a[1]=a[2]=0)
//#define VectorClear(a)			memset(a, 0, sizeof(vec3_t))
//#define VectorNegate(a,b)		(b[0]=-a[0],b[1]=-a[1],b[2]=-a[2])
#define VectorNegate(a,b)		{FNegate(a[0],b[0]);FNegate(a[1],b[1]);FNegate(a[2],b[2])}
#define VectorSet(v, x, y, z)	(v[0]=(x), v[1]=(y), v[2]=(z))
#define	VectorMA(v, s, b, o)	((o)[0]=(v)[0]+(b)[0]*(s),(o)[1]=(v)[1]+(b)[1]*(s),(o)[2]=(v)[2]+(b)[2]*(s))
//void VectorMA (vec3_t veca, float scale, vec3_t vecb, vec3_t vecc);

void ClearBounds (vec3_t mins, vec3_t maxs);
void AddPointToBounds (const vec3_t v, vec3_t mins, vec3_t maxs);
bool VectorCompare (const vec3_t v1, const vec3_t v2);
float VectorLength (const vec3_t v);
float VectorDistance (const vec3_t vec1, const vec3_t vec2);
void AnglesToAxis (const vec3_t angles, vec3_t axis[3]);
void AxisClear (vec3_t axis[3]);
#define AxisCopy(i,o)			memcpy(o,i,sizeof(vec3_t)*3)
void CrossProduct (const vec3_t v1, const vec3_t v2, vec3_t cross);
float VectorNormalize (vec3_t v);		// returns vector length
float VectorNormalize2 (const vec3_t v, vec3_t out);
void VectorInverse (vec3_t v);
void VectorScale (const vec3_t in, float scale, vec3_t out);

void MatrixMultiply (float in1[3][3], float in2[3][3], float out[3][3]);

void AngleVectors (const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up);
float anglemod(float a);
float LerpAngle (float a1, float a2, float frac);

void ProjectPointOnPlane( vec3_t dst, const vec3_t p, const vec3_t normal );
void PerpendicularVector( vec3_t dst, const vec3_t src );
void RotatePointAroundVector( vec3_t dst, const vec3_t dir, const vec3_t point, float degrees );
void MakeNormalVectors (vec3_t forward, vec3_t right, vec3_t up);


//=============================================

char *COM_Parse (const char *&data_p, bool allowLineBreaks = true);
const char *COM_QuoteString (const char *str, bool alwaysQuote);
// data is an in/out parm, returns a parsed out token
void SkipRestOfLine (char **data);

void Com_PageInMemory (void *buffer, int size);

//=============================================

#define BYTES4(a,b,c,d)	((a) | ((b)<<8) | ((c)<<16) | ((d)<<24))

//=============================================

#ifdef LITTLE_ENDIAN

// functions for big-endian numbers
short	ShortSwap (short l);
int		LongSwap (int l);
float	FloatSwap (float f);

#define BigShort(n)		ShortSwap(n)
#define LittleShort(n)	((short)n)
#define LittleLong(n)	((long)n)
#define LittleFloat(n)	((float)n)

#define Swap_Init()		// empty definition

#else

short	BigShort(short l);
short	LittleShort(short l);
int		LittleLong (int l);
float	LittleFloat (float l);

void	Swap_Init (void);

#endif

/*
==========================================================

CVARS (console variables)

==========================================================
*/


#define	CVAR_ARCHIVE		0x00001	// set to cause it to be saved to config file (config.cfg)
#define	CVAR_USERINFO		0x00002	// added to userinfo when changed, then sent to senver
#define	CVAR_SERVERINFO		0x00004	// added to serverinfo when changed
#define	CVAR_NOSET			0x00008	// don't allow change from console at all, but can be set from the command line
#define	CVAR_LATCH			0x00010	// save changes until server restart
// added since 4.00
#define	CVAR_USER_CREATED	0x00020	// created by a set command
#define CVAR_GAME_CREATED	0x00040	// created from game library
#define CVAR_CHEAT			0x00080	// will be reset to its default value when cheat protection active
#define CVAR_CMDLINE		0x00100	// variable was set from command line

#define CVAR_FLAG_MASK		0x0FFFF	// mask of stored cvar flags

// not stored flags:
#define CVAR_NODEFAULT		0x10000	// do not store "default" value from this Cvar_Get() call
#define CVAR_UPDATE			0x20000	// set "modified" field after Cvar_Get() call
#define CVAR_NOUPDATE		0x40000	// reset "modified" field ...

#define	CVAR_BUFFER_SIZE	16		// size of buffer for var->string inside cvar_t

// nothing outside the Cvar_*() functions should modify these fields!
struct cvar_t
{
	char	*name;
	char	*string;
	char	*latchedString;			// for CVAR_LATCH vars
	unsigned flags;
	bool	modified;				// set each time the cvar is changed; originally - qboolean
	byte	pad[3];					// qboolean pad (4 bytes)
	float	value;					// atof(string)
	cvar_t	*next;
	// added since 4.00
	char	*resetString;			// default cvar value (unset for user-created vars)
	int		integer;				// atoi(string)
	int		stringLength;			// size of buffer, allocated for holding var->string (or 0 if var->buf used)
	char	buf[CVAR_BUFFER_SIZE];
	cvar_t	*hashNext;
	// functions
	inline bool IsDefault ()
	{
		return resetString && strcmp (string, resetString) == 0;
	}
	inline void Reset ()
	{
		//!! not optimal - should use cvar_t struc instead of "name"
		Cvar_Set (name, resetString);
	}
	inline float Clamp (float low, float high)
	{
		return Cvar_Clamp (this, low, high);
	}
};



/*
==============================================================

COLLISION DETECTION

==============================================================
*/

// gi.BoxEdicts() can return a list of either solid or trigger entities
// FIXME: eliminate AREA_ distinction?
#define	AREA_SOLID		1
#define	AREA_TRIGGERS	2

// normal type for cplane_t
#define	PLANE_X			0			// { 1, 0, 0}
#define	PLANE_Y			1			// { 0, 1, 0}
#define	PLANE_Z			2			// { 0, 0, 1}
#define PLANE_MX		3			// {-1, 0, 0}
#define PLANE_MY		4			// { 0,-1, 0}
#define PLANE_MZ		5			// { 0, 0,-1}
#define	PLANE_NON_AXIAL	6			// any another normal

// plane_t structure (the same as Q2 dplane_t, but "int type" -> "byte type,signbits,pad[2]")
typedef struct
{
	vec3_t	normal;
	float	dist;
	byte	type;			// for fast side tests; PLANE_[M]X|Y|Z
	byte	signbits;		// sign(x) + (sign(y)<<1) + (sign(z)<<2)
	byte	pad[2];
} cplane_t;

void SetPlaneSignbits (cplane_t *out);

int BoxOnPlaneSide (const vec3_t emins, const vec3_t emaxs, const cplane_t *plane);

inline int PlaneTypeForNormal (const vec3_t p)
{
	if (p[0] == 1.0f) return PLANE_X;
	else if (p[0] == -1.0f) return PLANE_MX;
	else if (p[1] == 1.0f) return PLANE_Y;
	else if (p[1] == -1.0f) return PLANE_MY;
	else if (p[2] == 1.0f) return PLANE_Z;
	else if (p[2] == -1.0f) return PLANE_MZ;
	else return PLANE_NON_AXIAL;
}

inline int BOX_ON_PLANE_SIDE (const vec3_t mins, const vec3_t maxs, const cplane_t *plane)
{
	if (plane->type <= PLANE_MZ)
	{
		int		type;
		float	dist;

		if (plane->type <= PLANE_Z)
		{
			type = plane->type;
			dist = plane->dist;
			if (dist <= mins[type])			return 1;
			else if (dist >= maxs[type])	return 2;
		}
		else
		{
			type = plane->type - 3;
			dist = -plane->dist;
			if (dist <= mins[type])			return 2;
			else if (dist >= maxs[type])	return 1;
		}
		return 3;
	}
	return BoxOnPlaneSide (mins, maxs, plane);
}

inline float DISTANCE_TO_PLANE (const vec3_t vec, const cplane_t *plane)
{
	if (plane->type <= PLANE_Z)
		return vec[plane->type] - plane->dist;
	else if (plane->type <= PLANE_MZ)
		return -vec[plane->type - 3] - plane->dist;
	else
		return DotProduct (plane->normal, vec) - plane->dist;
}


typedef struct
{
	// standard csurface_t fields (do not change this!)
	char	name[16];
	unsigned flags;
	int		value;
	// field from mapsurface_t (so, csurface_t now contains old csurface_t and mapsurface_t)
	char	rname[32];		// used internally due to name len probs: ZOID (used only for precaching??)
	// fields added since 4.00
	int		material;
} csurface_t;

// a trace is returned when a box is swept through the world
struct trace_t
{
	bool		allsolid;		// if true, plane is not valid
	byte		pad1[3];		// qboolean pad
	bool		startsolid;		// if true, the initial point was in a solid area
	byte		pad2[3];		// qboolean pad
	float		fraction;		// time completed, 1.0 = didn't hit anything
	vec3_t		endpos;			// final position
	cplane_t	plane;			// surface normal at impact
	csurface_t	*surface;		// surface hit
	int			contents;		// contents on other side of surface hit
	struct edict_s	*ent;		// not set by CM_*() functions
};



// pmove_state_t is the information necessary for client side movement
// prediction
typedef enum
{
	// can accelerate and turn
	PM_NORMAL,
	PM_SPECTATOR,
	// no acceleration or turning
	PM_DEAD,
	PM_GIB,		// different bounding box
	PM_FREEZE
} pmtype_t;

// pmove->pm_flags
#define	PMF_DUCKED			1
#define	PMF_JUMP_HELD		2
#define	PMF_ON_GROUND		4
#define	PMF_TIME_WATERJUMP	8		// pm_time is waterjump
#define	PMF_TIME_LAND		16		// pm_time is time before rejump
#define	PMF_TIME_TELEPORT	32		// pm_time is non-moving time
#define PMF_NO_PREDICTION	64		// temporarily disables prediction (used for grappling hook)

// this structure needs to be communicated bit-accurate
// from the server to the client to guarantee that
// prediction stays in sync, so no floats are used.
// if any part of the game code modifies this struct, it
// will result in a prediction error of some degree.
typedef struct
{
	pmtype_t	pm_type;

	short		origin[3];			// 12.3
	short		velocity[3];		// 12.3
	byte		pm_flags;			// ducked, jump_held, etc
	byte		pm_time;			// each unit = 8 ms
	short		gravity;
	short		delta_angles[3];	// add to command angles to get view direction changed by spawns, rotating objects, and teleporters
} pmove_state_t;


//
// button bits
//
#define	BUTTON_ATTACK		1
#define	BUTTON_USE			2
#define	BUTTON_ANY			128			// any key whatsoever


// usercmd_t is sent to the server each client frame
typedef struct
{
	byte	msec;
	byte	buttons;
	short	angles[3];
	short	forwardmove, sidemove, upmove;
	byte	impulse;			// remove? (unused !!)
	byte	lightlevel;			// light level the player is standing on
} usercmd_t;


#define	MAXTOUCH	32
typedef struct
{
	// state (in / out)
	pmove_state_t s;

	// command (in)
	usercmd_t	cmd;
	bool		snapinitial;	// if s has been changed outside pmove
	byte		pad[3];			// qboolean pad

	// results (out)
	int			numtouch;
	struct edict_s	*touchents[MAXTOUCH];

	vec3_t		viewangles;		// clamped
	float		viewheight;

	vec3_t		mins, maxs;		// bounding box size

	struct edict_s	*groundentity;
	int			watertype;
	int			waterlevel;

	// callbacks to test the world
	trace_t		(*trace) (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end);
	int			(*pointcontents) (vec3_t point);
} pmove_t;


// player_state_t->rdflags (refdef flags)
#define	RDF_UNDERWATER		1				// warp the screen as apropriate
#define RDF_NOWORLDMODEL	2				// used for player configuration screen
//ROGUE
#define	RDF_IRGOGGLES		4
#define RDF_UVGOGGLES		8		//?? unused
// new since 4.00
#define RDF_THIRD_PERSON	0x10



// sound channels
// channel 0 never willingly overrides
// other channels (1-7) allways override a playing sound on that channel
enum {
	CHAN_AUTO,
	CHAN_WEAPON,
	CHAN_VOICE,
	CHAN_ITEM,
	CHAN_BODY,
	// modifier flags
	CHAN_NO_PHS_ADD = 8,	// send to all clients, not just ones in PHS (ATTN 0 will also do this)
	CHAN_RELIABLE = 16		// send by reliable message, not datagram
};


// sound attenuation values
enum {
	ATTN_NONE,				// full volume the entire level
	ATTN_NORM,
	ATTN_IDLE,
	ATTN_STATIC				// diminish very rapidly with distance
};


typedef enum
{
	MATERIAL_SILENT,	// no footstep sounds (and no bullethit sounds)
	MATERIAL_CONCRETE,	// standard sounds
	MATERIAL_FABRIC,	// rug
	MATERIAL_GRAVEL,	// gravel
	MATERIAL_METAL,		// metalh
	MATERIAL_METAL_L,	// metall
	MATERIAL_SNOW,		// tin (replace with pure snow from AHL??)
	MATERIAL_TIN,
	MATERIAL_TILE,		// marble (similar to concrete, but slightly muffled sound)
	MATERIAL_WOOD,		// wood
	MATERIAL_WATER,
	MATERIAL_GLASS,
	MATERIAL_DIRT,
	//!! reserved for constant MATERIAL_COUNT, but not implemented now:
	MATERIAL_R0,
	MATERIAL_R1,
	MATERIAL_R2,
	MATERIAL_R3,

	MATERIAL_COUNT		// must be last
} material_t;

#define MATERIAL_FIRST	0
#define MATERIAL_LAST	(MATERIAL_COUNT-1)


/*
==========================================================

  ELEMENTS COMMUNICATED ACROSS THE NET

==========================================================
*/

#define	ANGLE2SHORT(x)	(appRound ((x)*65535.0f/360) & 65535)
#define	SHORT2ANGLE(x)	((x)*(360.0f/65536))


//==============================================


// entity_state_t is the information conveyed from the server
// in an update message about entities that the client will
// need to render in some way
typedef struct
{
	int		number;			// edict index; real size is "short"

	vec3_t	origin;
	vec3_t	angles;
	vec3_t	old_origin;		// for lerping
	int		modelindex;
	int		modelindex2, modelindex3, modelindex4;	// weapons, CTF flags, etc; real size is "byte"
	int		frame;			// real size is "short"
	int		skinnum;
	unsigned effects;
	unsigned renderfx;
	int		solid;			// 16-bit int; for client side prediction, 8*bits[0-4] is x/y radius; 8*bits[5-9] is z
							// down distance, 8*bits[10-15] is z up; gi.linkentity() sets this; real size is "short"
	int		sound;			// for looping sounds, to guarantee shutoff; real size is "byte"
	int		event;			// impulse events -- muzzle flashes, footsteps, etc; events only go out for a single frame,
							// they are automatically cleared each frame; real size is "byte"
} entity_state_t;

//==============================================

#define	MAX_STATS	32		// max STAT_XXX value; limited by protocol: 1 bit of int32 for each stats


// player_state_t is the information needed in addition to pmove_state_t
// to rendered a view.  There will only be 10 player_state_t sent each second,
// but the number of pmove_state_t changes will be reletive to client
// frame rates
typedef struct
{
	pmove_state_t	pmove;		// for prediction

	// these fields do not need to be communicated bit-precise

	vec3_t		viewangles;		// for fixed views
	vec3_t		viewoffset;		// add to pmovestate->origin
	vec3_t		kick_angles;	// add to view direction to get render angles
								// set by weapon kicks, pain effects, etc

	vec3_t		gunangles;
	vec3_t		gunoffset;
	int			gunindex;
	int			gunframe;

	float		blend[4];		// rgba full screen effect

	float		fov;			// horizontal field of view

	int			rdflags;		// refdef flags

	short		stats[MAX_STATS];		// fast status bar updates
} player_state_t;


#endif // QSHARED_H
