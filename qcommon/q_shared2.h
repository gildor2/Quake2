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

#ifdef _WIN32
// unknown pragmas are SUPPOSED to be ignored, but....
#pragma warning(disable : 4244)     // MIPS
#pragma warning(disable : 4136)     // X86
#pragma warning(disable : 4051)     // ALPHA

#pragma warning(disable : 4018)     // signed/unsigned mismatch
#pragma warning(disable : 4305)		// truncation from const double to float

#define DLL_EXPORT	__declspec(dllexport)

#ifndef vsnprintf
#  define vsnprintf	_vsnprintf
#endif

#else // _WIN32

#define DLL_EXPORT

#endif

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#define LITTLE_ENDIAN				//!! comment this string for big-endian platform (or make auto-detection)

// this is the define for determining if we have an asm version of a C function
#if (defined _M_IX86 || defined __i386__) && !defined C_ONLY && !defined __sun__
#define id386	1
#else
#define id386	0
#endif

#if defined _M_ALPHA && !defined C_ONLY
#define idaxp	1
#else
#define idaxp	0
#endif

#ifndef min
#define min(a,b)  (((a) < (b)) ? (a) : (b))
#endif
#ifndef max
#define max(a,b)  (((a) > (b)) ? (a) : (b))
#endif

#define bound(a,minval,maxval)  ( ((a) > (minval)) ? ( ((a) < (maxval)) ? (a) : (maxval) ) : (minval) )
#define EXPAND_BOUNDS(a,minval,maxval)	\
	if (a < minval) minval = a;			\
	if (a > maxval) maxval = a;

#define VECTOR_ARG(name)	name[0],name[1],name[2]
#define ARRAY_ARG(array)	array, sizeof(array)/sizeof(array[0])
#define ARRAY_COUNT(array)	(sizeof(array)/sizeof(array[0]))

typedef unsigned char		byte;
//typedef enum {false, true}	qboolean;
enum {false, true};
typedef unsigned int		qboolean;
typedef unsigned char		bool;			// C++ equalent


#ifndef NULL
#define NULL ((void *)0)
#endif

#if defined(_M_IX86) || defined(__i386__)

#define RETADDR_STR " (call from %08x)"
#define	GET_RETADDR(firstarg)	(* ( ((int*)&firstarg) -1 ) )

#else

// empty string
#define RETADDR_STR	"%s"
#define	GET_RETADDR(firstarg)	""

#endif


// use "STR(any_value)" to convert it to string (may be float value)
#define STR2(s) #s
#define STR(s) STR2(s)


#define BIG_NUMBER			0x1000000

//============================================================================


// angle indexes
enum {
	PITCH,							// up / down
	YAW,							// left / right
	ROLL							// fall over
};

#define	MAX_STRING_CHARS	1024	// max length of a string passed to Cmd_TokenizeString
#define	MAX_STRING_TOKENS	80		// max tokens resulting from Cmd_TokenizeString

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


// game print flags
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

typedef float vec3_t[3];


#ifndef M_PI
#define M_PI		3.14159265358979323846	// matches value in gcc v2 math.h
#endif

extern vec3_t vec3_origin;

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

__inline int Q_round (float f)
{
	int		i;

	__asm {
		fld		[f]
		fistp	[i]
	}
	return i;
}

__inline int Q_floor (float f)
{
	int		i;
	static const float h = 0.4999999;	// 0.5

	__asm {
		fld		[f]
		fsub	[h]
		fistp	[i]
	}
	return i;
}

__inline int Q_ceil (float f)
{
	int		i;
	static const float h = 0.4999999;	// 0.5

	__asm {
		fld		[f]
		fadd	[h]
		fistp	[i]
	}
	return i;
}

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
#define Q_ftol(f)	(long) (f)
#define Q_round(f)	(int) (f >= 0 ? (int)(f+0.5f) : (int)(f-0.5f))
#define Q_floor(f)	((int)floor(f))
#define Q_ceil(f)	((int)ceil(f))

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

// just in case you do't want to use the macros
float _DotProduct (vec3_t v1, vec3_t v2);
void _VectorSubtract (vec3_t veca, vec3_t vecb, vec3_t out);
void _VectorAdd (vec3_t veca, vec3_t vecb, vec3_t out);
void _VectorCopy (vec3_t in, vec3_t out);

void ClearBounds (vec3_t mins, vec3_t maxs);
void AddPointToBounds (const vec3_t v, vec3_t mins, vec3_t maxs);
qboolean VectorCompare (const vec3_t v1, const vec3_t v2);
float VectorLength (const vec3_t v);
float VectorDistance (const vec3_t vec1, const vec3_t vec2);
void AnglesToAxis (const vec3_t angles, vec3_t axis[3]);
void AxisClear (vec3_t axis[3]);
#define AxisCopy(i,o)			memcpy(o,i,sizeof(vec3_t)*3)
void _AxisCopy (vec3_t in[3], vec3_t out[3]);
void CrossProduct (const vec3_t v1, const vec3_t v2, vec3_t cross);
float VectorNormalize (vec3_t v);		// returns vector length
float VectorNormalize2 (const vec3_t v, vec3_t out);
void VectorInverse (vec3_t v);
void VectorScale (const vec3_t in, float scale, vec3_t out);
int Q_log2(int val);

void MatrixMultiply (float in1[3][3], float in2[3][3], float out[3][3]);

void AngleVectors (const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up);
float	anglemod(float a);
float LerpAngle (float a1, float a2, float frac);

void ProjectPointOnPlane( vec3_t dst, const vec3_t p, const vec3_t normal );
void PerpendicularVector( vec3_t dst, const vec3_t src );
void RotatePointAroundVector( vec3_t dst, const vec3_t dir, const vec3_t point, float degrees );
void MakeNormalVectors (vec3_t forward, vec3_t right, vec3_t up);


//=============================================

char *COM_SkipPath (char *pathname);
void COM_StripExtension (char *in, char *out);
void COM_FileBase (char *in, char *out);
void COM_FilePath (char *in, char *out);
void COM_DefaultExtension (char *path, char *extension);

char *COM_ParseExt (char **data_p, qboolean allowLineBreaks);
//char *COM_Parse (char **data_p);
#define COM_Parse(p)	COM_ParseExt(p,true)
const char *COM_QuoteString (const char *str, bool alwaysQuote);
// data is an in/out parm, returns a parsed out token
void SkipBracedSection (char **program);
void SkipRestOfLine (char **data);

// format arguments with fmt and put result to dest with a maximum size of size; return strlen(dest)
int Com_sprintf (char *dest, int size, const char *fmt, ...);

void Com_PageInMemory (byte *buffer, int size);

//=============================================

// portable case insensitive compare
int		Q_stricmp (const char *s1, const char *s2);
int		Q_strcasecmp (const char *s1, const char *s2);
int		Q_strncasecmp (const char *s1, const char *s2, int n);

void	Q_strncpyz (char *dest, const char *src, int destsize);
void	Q_strncpylower (char *dest, const char *src, int len);
void	Q_CopyFilename (char *dest, const char *src, int len);

//=============================================

#ifdef LITTLE_ENDIAN

// functions for big-endian numbers
short	ShortSwap (short l);
int		LongSwap (int l);
float	FloatSwap (float f);

#define BigShort(n)		ShortSwap(n)
#define BigLong(n)		LongSwap(n)
#define BigFloat(n)		FloatSwap(n)
#define LittleShort(n)	((short)n)
#define LittleLong(n)	((long)n)
#define LittleFloat(n)	((float)n)

#define Swap_Init()		// empty definition

#else

short	BigShort(short l);
short	LittleShort(short l);
int		BigLong (int l);
int		LittleLong (int l);
float	BigFloat (float l);
float	LittleFloat (float l);

void	Swap_Init (void);

#endif

char	*va(char *format, ...);

//=============================================

//
// key / value info strings
//
#define	MAX_INFO_KEY		64
#define	MAX_INFO_VALUE		64
#define	MAX_INFO_STRING		512

char *Info_ValueForKey (char *s, char *key);
void Info_RemoveKey (char *s, char *key);
void Info_SetValueForKey (char *s, char *key, char *value);
qboolean Info_Validate (char *s);


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
typedef struct cvar_s
{
	char	*name;
	char	*string;
	char	*latched_string;		// for CVAR_LATCH vars
	int		flags;
	qboolean modified;				// set each time the cvar is changed
	float	value;					// atof(string)
	struct cvar_s *next;
	// added since 4.00
	char	*reset_string;			// default cvar value (unset for user-created vars)
	int		integer;				// atoi(string)
	int		string_length;			// size of buffer, allocated for holding var->string (or 0 if var->buf used)
	char	buf[CVAR_BUFFER_SIZE];
	struct cvar_s *hashNext;
} cvar_t;



/*
==============================================================

COLLISION DETECTION

==============================================================
*/

// gi.BoxEdicts() can return a list of either solid or trigger entities
// FIXME: eliminate AREA_ distinction?
#define	AREA_SOLID		1
#define	AREA_TRIGGERS	2

// normal type for plane
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
	byte	signbits;		// signx + (signy<<1) + (signz<<2)
	byte	pad[2];
} cplane_t;

void SetPlaneSignbits (cplane_t *out);

int BoxOnPlaneSide (const vec3_t emins, const vec3_t emaxs, const cplane_t *plane);

#if 1

__inline int PlaneTypeForNormal (const vec3_t p)
{
	if (p[0] == 1.0f) return PLANE_X;
	else if (p[0] == -1.0f) return PLANE_MX;
	else if (p[1] == 1.0f) return PLANE_Y;
	else if (p[1] == -1.0f) return PLANE_MY;
	else if (p[2] == 1.0f) return PLANE_Z;
	else if (p[2] == -1.0f) return PLANE_MZ;
	else return PLANE_NON_AXIAL;
}

__inline int BOX_ON_PLANE_SIDE (const vec3_t mins, const vec3_t maxs, const cplane_t *plane)
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

__inline float DISTANCE_TO_PLANE (const vec3_t vec, const cplane_t *plane)
{
	if (plane->type <= PLANE_Z)
		return vec[plane->type] - plane->dist;
	else if (plane->type <= PLANE_MZ)
		return -vec[plane->type - 3] - plane->dist;
	else
		return DotProduct (plane->normal, vec) - plane->dist;
}

#else

#define PlaneTypeForNormal(x) (x[0] == 1.0 ? PLANE_X : (x[1] == 1.0 ? PLANE_Y : (x[2] == 1.0 ? PLANE_Z : PLANE_NON_AXIAL) ) )
#define BOX_ON_PLANE_SIDE(mins, maxs, p)	\
	(((p)->type <= PLANE_Z) ?				\
	(										\
		((p)->dist <= (mins)[(p)->type]) ?	\
			1								\
		:									\
		(									\
			((p)->dist >= (maxs)[(p)->type]) ?\
				2							\
			:								\
				3							\
		)									\
	)										\
	:										\
		BoxOnPlaneSide ((mins), (maxs), (p)))

#define DISTANCE_TO_PLANE(vec,plane)		\
	(										\
		((plane)->type <= PLANE_Z) ?		\
		(vec)[(plane)->type] - (plane)->dist\
	:										\
		DotProduct((plane)->normal,(vec)) - (plane)->dist\
	)

#endif


typedef struct
{
	// standard csurface_t fields (do not change this!)
	char	name[16];
	int		flags;
	int		value;
	// field from mapsurface_t (so, csurface_t now contains old csurface_t and mapsurface_t)
	char	rname[32];		// used internally due to name len probs: ZOID (used only for precaching?)
	// fields added since 4.00
	int		material;
} csurface_t;

/* -- now this structure is csurface_t
typedef struct mapsurface_s
{
	csurface_t	c;
} mapsurface_t;
*/

// a trace is returned when a box is swept through the world
typedef struct
{
	qboolean	allsolid;		// if true, plane is not valid
	qboolean	startsolid;		// if true, the initial point was in a solid area
	float		fraction;		// time completed, 1.0 = didn't hit anything
	vec3_t		endpos;			// final position
	cplane_t	plane;			// surface normal at impact
	csurface_t	*surface;		// surface hit
	int			contents;		// contents on other side of surface hit
	struct edict_s	*ent;		// not set by CM_*() functions
} trace_t;



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
#define	PMF_TIME_WATERJUMP	8	// pm_time is waterjump
#define	PMF_TIME_LAND		16	// pm_time is time before rejump
#define	PMF_TIME_TELEPORT	32	// pm_time is non-moving time
#define PMF_NO_PREDICTION	64	// temporarily disables prediction (used for grappling hook)

// this structure needs to be communicated bit-accurate
// from the server to the client to guarantee that
// prediction stays in sync, so no floats are used.
// if any part of the game code modifies this struct, it
// will result in a prediction error of some degree.
typedef struct
{
	pmtype_t	pm_type;

	short		origin[3];		// 12.3
	short		velocity[3];	// 12.3
	byte		pm_flags;		// ducked, jump_held, etc
	byte		pm_time;		// each unit = 8 ms
	short		gravity;
	short		delta_angles[3];	// add to command angles to get view direction
									// changed by spawns, rotating objects, and teleporters
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
	byte	impulse;		// remove? (unused !!)
	byte	lightlevel;		// light level the player is standing on
} usercmd_t;


#define	MAXTOUCH	32
typedef struct
{
	// state (in / out)
	pmove_state_t	s;

	// command (in)
	usercmd_t		cmd;
	qboolean		snapinitial;	// if s has been changed outside pmove

	// results (out)
	int			numtouch;
	struct edict_s	*touchents[MAXTOUCH];

	vec3_t		viewangles;			// clamped
	float		viewheight;

	vec3_t		mins, maxs;			// bounding box size

	struct edict_s	*groundentity;
	int			watertype;
	int			waterlevel;

	// callbacks to test the world
	trace_t		(*trace) (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end);
	int			(*pointcontents) (vec3_t point);
} pmove_t;


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
// RAFAEL
#define EF_IONRIPPER		0x00100000		// light + trail
#define EF_GREENGIB			0x00200000		// green blood trail
#define	EF_BLUEHYPERBLASTER 0x00400000		// blue light
#define EF_SPINNINGLIGHTS	0x00800000		// red light, rotating around entity origin
#define EF_PLASMA			0x01000000		// light + (if EF_ANIM_ALLFAST set) blaster trail
#define EF_TRAP				0x02000000		// light + trail

//ROGUE
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
#define	RF_FRAMELERP		0x00000040
#define RF_BEAM				0x00000080
#define	RF_CUSTOMSKIN		0x00000100		// skin is an index in image_precache
#define	RF_GLOW				0x00000200		// pulse lighting for bonus items
#define RF_SHELL_RED		0x00000400
#define	RF_SHELL_GREEN		0x00000800
#define RF_SHELL_BLUE		0x00001000

//ROGUE
#define RF_IR_VISIBLE		0x00008000
#define	RF_SHELL_DOUBLE		0x00010000
#define	RF_SHELL_HALF_DAM	0x00020000
#define RF_USE_DISGUISE		0x00040000

// 4.00
#define RF_BBOX				0x40000000


// player_state_t->refdef flags
#define	RDF_UNDERWATER		1		// warp the screen as apropriate
#define RDF_NOWORLDMODEL	2		// used for player configuration screen

//ROGUE
#define	RDF_IRGOGGLES		4
#define RDF_UVGOGGLES		8		//?? unused
//ROGUE

#define RDF_THIRD_PERSON	0x10

// muzzle flashes / player effects
enum {
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
	// RAFAEL
	MZ_IONRIPPER,
	MZ_BLUEHYPERBLASTER,
	MZ_PHALANX,
	MZ_SILENCED =128,		// bit flag OR'ed with one of the above numbers
	//ROGUE
	MZ_ETF_RIFLE = 30,
	MZ_UNUSED,
	MZ_SHOTGUN2,
	MZ_HEATBEAM,
	MZ_BLASTER2,
	MZ_TRACKER,
	MZ_NUKE1, MZ_NUKE2, MZ_NUKE4, MZ_NUKE8
};

//
// monster muzzle flashes
//
enum {
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

	// --- Xian shit below ---
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

	//ROGUE
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


// ROGUE

extern	vec3_t monster_flash_offset [];


// temp entity events
//
// Temp entity events are for things that happen
// at a location seperate from any existing entity.
// Temporary entity messages are explicitly constructed
// and broadcast.
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
	TE_GRENADE_EXPLOSION,
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
	TE_BOSSTPORT,			// used as '22' in a map, so DON'T RENUMBER!!!
	TE_BFG_LASER,
	TE_GRAPPLE_CABLE,
	TE_WELDING_SPARKS,
	TE_GREENBLOOD,
	TE_BLUEHYPERBLASTER,
	TE_PLASMA_EXPLOSION,
	TE_TUNNEL_SPARKS,
	//ROGUE
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
	TE_RAILTRAIL_EXT = 128
};

enum {
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


// player_state->stats[] indexes
enum {
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

#define	MAX_STATS				32


/*
ROGUE - VERSIONS
1234	08/13/1998		Activision
1235	08/14/1998		Id Software
1236	08/15/1998		Steve Tietze
1237	08/15/1998		Phil Dobranski
1238	08/15/1998		John Sheley
1239	08/17/1998		Barrett Alexander
1230	08/17/1998		Brandon Fish
1245	08/17/1998		Don MacAskill
1246	08/17/1998		David "Zoid" Kirsch
1247	08/17/1998		Manu Smith
1248	08/17/1998		Geoff Scully
1249	08/17/1998		Andy Van Fossen
1240	08/20/1998		Activision Build 2
1256	08/20/1998		Ranger Clan
1257	08/20/1998		Ensemble Studios
1258	08/21/1998		Robert Duffy
1259	08/21/1998		Stephen Seachord
1250	08/21/1998		Stephen Heaslip
1267	08/21/1998		Samir Sandesara
1268	08/21/1998		Oliver Wyman
1269	08/21/1998		Steven Marchegiano
1260	08/21/1998		Build #2 for Nihilistic
1278	08/21/1998		Build #2 for Ensemble

9999	08/20/1998		Internal Use
*/
#define ROGUE_VERSION_ID		1278

#define ROGUE_VERSION_STRING	"08/21/1998 Beta 2 for Ensemble"

// ROGUE
/*
==========================================================

  ELEMENTS COMMUNICATED ACROSS THE NET

==========================================================
*/

#define	ANGLE2SHORT(x)	(Q_round ((x)*65535.0f/360) & 65535)
#define	SHORT2ANGLE(x)	((x)*(360.0f/65536))


//
// config strings are a general means of communication from
// the server to all connected clients.
// Each config string can be at most MAX_QPATH characters.
//
enum
{
	CS_NAME,
	CS_CDTRACK,
	CS_SKY,
	CS_SKYAXIS,			// %f %f %f format
	CS_SKYROTATE,
	CS_STATUSBAR,		// display program string

	CS_AIRACCEL = 29,	// air acceleration control
	CS_MAXCLIENTS,
	CS_MAPCHECKSUM,		// for catching cheater maps

	CS_MODELS,
	CS_SOUNDS = CS_MODELS+MAX_MODELS,
	CS_IMAGES = CS_SOUNDS+MAX_SOUNDS,
	CS_LIGHTS = CS_IMAGES+MAX_IMAGES,
	CS_ITEMS = CS_LIGHTS+MAX_LIGHTSTYLES,
	CS_PLAYERSKINS = CS_ITEMS+MAX_ITEMS,
	CS_GENERAL = CS_PLAYERSKINS+MAX_CLIENTS,

	MAX_CONFIGSTRINGS = CS_GENERAL+MAX_GENERAL
};

//==============================================


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
	//!! reserved, but not implemented now:
	MATERIAL_R0,
	MATERIAL_R1,
	MATERIAL_R2,
	MATERIAL_R3,

	MATERIAL_COUNT		// must be last
} material_t;

#define MATERIAL_FIRST	0
#define MATERIAL_LAST	(MATERIAL_COUNT-1)

// entity_state_t->event values
// ertity events are for effects that take place reletive
// to an existing entities origin.  Very network efficient.
// All muzzle flashes really should be converted to events...
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
	EV_FOOTSTEP0 = 100,
	EV_FALLSHORT0 = EV_FOOTSTEP0 + MATERIAL_COUNT,

	EV_SPECTATOR0 = EV_FALLSHORT0 + MATERIAL_COUNT,
	EV_SPECTATOR1,
	EV_SPECTATOR2,
	EV_SPECTATOR3,

	EV_CAMPER0,
	EV_CAMPER1,
	EV_CAMPER2,
	EV_CAMPER3,
	EV_CAMPER4,
	EV_CAMPER5,
	EV_CAMPER6,
	EV_CAMPER7,
	EV_CAMPER8
};


// entity_state_t is the information conveyed from the server
// in an update message about entities that the client will
// need to render in some way
typedef struct
{
	int		number;			// edict index

	vec3_t	origin;
	vec3_t	angles;
	vec3_t	old_origin;		// for lerping
	int		modelindex;
	int		modelindex2, modelindex3, modelindex4;	// weapons, CTF flags, etc
	int		frame;
	int		skinnum;
	unsigned int		effects;		// PGM - we're filling it, so it needs to be unsigned
	int		renderfx;
	int		solid;			// for client side prediction, 8*(bits 0-4) is x/y radius
							// 8*(bits 5-9) is z down distance, 8*(bits10-15) is z up
							// gi.linkentity sets this properly
	int		sound;			// for looping sounds, to guarantee shutoff
	int		event;			// impulse events -- muzzle flashes, footsteps, etc
							// events only go out for a single frame, they
							// are automatically cleared each frame
} entity_state_t;

//==============================================


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
