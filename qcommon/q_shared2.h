#ifndef QSHARED_H
#define QSHARED_H


#define	MAX_STRING_CHARS	1024	// max length of a string passed to TokenizeString()
#define	MAX_STRING_TOKENS	80		// max tokens resulting from TokenizeString()

#define	MAX_QPATH			64		// max length of a quake game pathname
#define	MAX_OSPATH			128		// max length of a filesystem pathname


// game print flags; for svc_print command; client system uses PRINT_CHAT only; server - PRINT_HIGH
enum {
	PRINT_LOW,						// pickup messages
	PRINT_MEDIUM,					// death messages
	PRINT_HIGH,						// critical messages
	PRINT_CHAT						// chat messages
};


// destination class for gi.multicast()
enum multicast_t
{
	MULTICAST_ALL,
	MULTICAST_PHS,
	MULTICAST_PVS,
	MULTICAST_ALL_R,
	MULTICAST_PHS_R,
	MULTICAST_PVS_R
};

#define	MAX_EDICTS			1024	// must change protocol to increase more


/*-----------------------------------------------------------------------------
	Math library
-----------------------------------------------------------------------------*/

// float numbers

//!! change, rename
#undef M_PI
#define M_PI				(3.14159265358979323846)

float appRsqrt (float number);
#define SQRTFAST(x)			(x * appRsqrt(x))

#if 1

#define uint_cast(f)		(reinterpret_cast<unsigned&>(f))
#define uint_cast_const(f)	(reinterpret_cast<const unsigned&>(f))

inline unsigned IsNegative (float f)
{
	return uint_cast(f) >> 31;
}

#define FAbsSign(f,d,s)	\
	{					\
		unsigned mask = uint_cast_const(f) & 0x80000000; \
		uint_cast(d)  = uint_cast_const(f) ^ mask;		\
		s = mask >> 31;	\
	}

inline void FAbs (float &a)
{
	uint_cast(a) &= 0x7FFFFFFF;
}

inline void FNegate (const float &a, float &b)
{
	uint_cast(b) = uint_cast_const(a) ^ 0x80000000;
}

inline void FNegate (float &a)
{
	uint_cast(a) ^= 0x80000000;
}

#else

//!! should be in a Core (appRound,appFloor,appCeil)
#define appRound(f)			((int) (f >= 0 ? (f)+0.5f : (f)-0.5f))
#define appFloor(f)			((int) floor(f))
#define appCeil(f)			((int) ceil(f))

inline unsigned IsNegative(f)
{
	return f<0;
}
#define FAbsSign(f,d,s)		\
	{						\
		s = IsNegative(f);	\
		d = fabs(f);		\
	}

inline void FAbs (float &a)
{
	a = fabs (a);
}

inline void FNegate (const float &a, float &b)
{
	b = -a;
}
inline void FNegate (float &a)
{
	a = -a;
}

#endif

// vector math

struct CVec3
{
	float	v[3];
	// access to data
	inline float& operator[] (int index)
	{
		return v[index];
	}
	inline const float& operator[] (int index) const
	{
		return v[index];
	}
	// NOTE: for fnctions, which requires CVec3 -> float*, can easily do it using CVec3.v field
	// trivial setup functions
	inline void Zero ()
	{
		memset (this, 0, sizeof(CVec3));
	}
	inline void Set (float x, float y, float z)
	{
		v[0] = x; v[1] = y; v[2] = z;
	}
	// simple math
	inline void Negate ()
	{
		FNegate (v[0]);
		FNegate (v[1]);
		FNegate (v[2]);
	}
	//!! +NegateTo(dst)
	inline void Scale (float scale) //?? == "operator *= (float)"
	{
		v[0] *= scale;
		v[1] *= scale;
		v[2] *= scale;
	}
	//!! +ScaleTo(dst)
	float Normalize ();			// returns vector length
	float NormalizeFast ();		//?? 2-arg version too?
};

inline bool operator== (const CVec3 &v1, const CVec3 &v2)
{
	return memcmp (&v1, &v2, sizeof(CVec3)) == 0;
}

inline bool operator!= (const CVec3 &v1, const CVec3 &v2)
{
	return memcmp (&v1, &v2, sizeof(CVec3)) != 0;
}

inline void Lerp (const CVec3 &A, const CVec3 &B, float Alpha, CVec3 &dst)
{
	dst[0] = A.v[0] + Alpha * (B.v[0]-A.v[0]);
	dst[1] = A.v[1] + Alpha * (B.v[1]-A.v[1]);
	dst[2] = A.v[2] + Alpha * (B.v[2]-A.v[2]);
}


inline float dot (const CVec3 &a, const CVec3 &b)
{
//	return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
	return a.v[0]*b.v[0] + a.v[1]*b.v[1] + a.v[2]*b.v[2];
}

void cross (const CVec3 &v1, const CVec3 &v2, CVec3 &result);


//?? OLD FUNCTIONS :

inline void VectorSubtract (const CVec3 &a, const CVec3 &b, CVec3 &d)
{
	d.v[0] = a.v[0] - b.v[0];
	d.v[1] = a.v[1] - b.v[1];
	d.v[2] = a.v[2] - b.v[2];
}
// macro version for any types (used 2 times)
#define VectorSubtract2(a,b,c)	(c[0]=a[0]-b[0],c[1]=a[1]-b[1],c[2]=a[2]-b[2])

inline void VectorAdd (const CVec3 &a, const CVec3 &b, CVec3 &d)
{
	d.v[0] = a.v[0] + b.v[0];
	d.v[1] = a.v[1] + b.v[1];
	d.v[2] = a.v[2] + b.v[2];
}

inline void VectorNegate (const CVec3 &a, CVec3 &b)
{
	FNegate (a.v[0], b.v[0]);
	FNegate (a.v[1], b.v[1]);
	FNegate (a.v[2], b.v[2]);
}

inline void VectorScale (const CVec3 &a, float scale, CVec3 &b)
{
	b.v[0] = scale * a.v[0];
	b.v[1] = scale * a.v[1];
	b.v[2] = scale * a.v[2];
}

// d = a + scale * b
inline void VectorMA (const CVec3 &a, float scale, const CVec3 &b, CVec3 &d)
{
	d.v[0] = a.v[0] + scale * b.v[0];
	d.v[1] = a.v[1] + scale * b.v[1];
	d.v[2] = a.v[2] + scale * b.v[2];
}

// a += scale * b
inline void VectorMA (CVec3 &a, float scale, const CVec3 &b)	//!! method, rename?
{
	a.v[0] += scale * b.v[0];
	a.v[1] += scale * b.v[1];
	a.v[2] += scale * b.v[2];
}

float VectorLength (const CVec3 &v);	//!! method
float VectorDistance (const CVec3 &vec1, const CVec3 &vec2);
//?? VectorDistanceFast()
float VectorNormalize (const CVec3 &v, CVec3 &out);

// normal type for cplane_t
enum
{
	PLANE_X,						// { 1, 0, 0}
	PLANE_Y,						// { 0, 1, 0}
	PLANE_Z,						// { 0, 0, 1}
	PLANE_MX,						// {-1, 0, 0}
	PLANE_MY,						// { 0,-1, 0}
	PLANE_MZ,						// { 0, 0,-1}
	PLANE_NON_AXIAL					// any another normal
};

struct cplane_t		//?? rename
{
	// really, plane can be specified with "normal" and "dist" only; other fields - for speedup some calculations
	CVec3	normal;
	float	dist;
	byte	type;					// for fast DistanceTo() tests; PLANE_[M]X|Y|Z
	byte	signbits;				// sign(x) + (sign(y)<<1) + (sign(z)<<2); used for speedup CBox::OnPlaneSide()
	byte	pad[2];					// for dword alignment
	// methods
	inline void SetSignbits ()
	{
		signbits = IsNegative(normal[0]) + (IsNegative(normal[1]) << 1) + (IsNegative(normal[2]) << 2);
	}
	inline void SetType ()
	{
		if		(normal[0] ==  1.0f) type = PLANE_X;
		else if (normal[0] == -1.0f) type = PLANE_MX;
		else if (normal[1] ==  1.0f) type = PLANE_Y;
		else if (normal[1] == -1.0f) type = PLANE_MY;
		else if (normal[2] ==  1.0f) type = PLANE_Z;
		else if (normal[2] == -1.0f) type = PLANE_MZ;
		else type = PLANE_NON_AXIAL;
	}
	// distance is positive, when point placed on a normal-looking side
	inline float DistanceTo (const CVec3 &vec)
	{
		if (type <= PLANE_Z)
			return vec[type] - dist;
		else if (type <= PLANE_MZ)
			return -vec[type-3] - dist;
		else
			return dot(normal, vec) - dist;
	}
};

// axis aligned box
struct CBox
{
	// fields
	CVec3	mins, maxs;
	// methods
	void Clear ();						// set mins>maxs (negative volume)
	void Expand (const CVec3 &p);		// expand mins/maxs by point p
	void Expand (const CBox &b);		// include box b into volume
	void GetCenter (CVec3 &p);			// compute center point
	// returns 1 when box placed on a normal-looking side of plane, 2 when on opposite side,
	// and 3 (1+2) when box intersects plane
	int OnPlaneSide (const cplane_t &plane) const;
};

// CBox::OnPlaneSide(), optimized for axis-aligned planes
inline int BOX_ON_PLANE_SIDE (const CBox &box, const cplane_t &plane)
{
	if (plane.type <= PLANE_MZ)
	{
		if (plane.type <= PLANE_Z)
		{
			int type = plane.type;
			float dist = plane.dist;
			if (dist <= box.mins[type])			return 1;
			else if (dist >= box.maxs[type])	return 2;
		}
		else
		{
			int type = plane.type - 3;
			float dist = -plane.dist;
			if (dist <= box.mins[type])			return 2;
			else if (dist >= box.maxs[type])	return 1;
		}
		return 3;
	}
	return box.OnPlaneSide (plane);
}


struct CAxis
{
	// fields
	// NOTE: v[] can be private, but this will prevent from
	// initializing CAxis object with initializer list ( "= {n,n,n,n ...}" )
	CVec3	v[3];
	// methods
	void FromAngles (const CVec3 &angles);
	void TransformVector (const CVec3 &src, CVec3 &dst) const;
	void UnTransformVector (const CVec3 &src, CVec3 &dst) const;
	void TransformAxis (const CAxis &src, CAxis &dst) const;
	void UnTransformAxis (const CAxis &src, CAxis &dst) const;
	// indexed access
	inline CVec3& operator[] (int index)
	{
		return v[index];
	}
	inline const CVec3& operator[] (int index) const
	{
		return v[index];
	}
};
// NOTE: to produce inverted axis, can transpose matrix (valid for orthogonal matrix only)


//?? CCoords : CVector, CAxis (will require to rename CVector.v or CAxis.v); can this be useful?
struct CCoords
{
	// fields
	CVec3	origin;
	CAxis	axis;
	// methods
	void TransformPoint (const CVec3 &src, CVec3 &dst) const;
	void UnTransformPoint (const CVec3 &src, CVec3 &dst) const;
	void TransformCoords (const CCoords &src, CCoords &dst) const;
	void UnTransformCoords (const CCoords &src, CCoords &dst) const;
	void TransformPlane (const cplane_t &src, cplane_t &dst) const;
};

// Functions for work with coordinate systems, not combined into CCoords class

// global coordinate system -> local coordinate system (src -> dst) by origin/axis coords
void TransformPoint (const CVec3 &origin, const CAxis &axis, const CVec3 &src, CVec3 &dst);
// local coordinate system -> global coordinate system
void UnTransformPoint (const CVec3 &origin, const CAxis &axis, const CVec3 &src, CVec3 &dst);


// misc

extern const CVec3 nullVec3;
extern const CBox  nullBox;
extern const CAxis identAxis;

// angle indexes
enum
{
	PITCH,							// looking up and down (0=Straight Ahead, +Up, -Down).
	YAW,							// rotating around (running in circles), 0=East, +North, -South.
	ROLL							// rotation about axis of screen, 0=Straight, +Clockwise, -CCW.
};

void AngleVectors (const CVec3 &angles, CVec3 *forward, CVec3 *right, CVec3 *up);
float AngleMod (float a);
float LerpAngle (float a1, float a2, float frac);
float AngleSubtract (float a1, float a2);
void AnglesSubtract (const CVec3 &v1, const CVec3 &v2, CVec3 &v3);


void MakeNormalVectors (const CVec3 &forward, CVec3 &right, CVec3 &up);
void Vec2Angles (const CVec3 &vec, CVec3 &angles);
float Vec2Yaw (const CVec3 &vec);

// colors

//?? implement as class
union color_t
{
	byte	c[4];
	unsigned rgba;
};

#undef RGBA
#undef RGB
#undef RGB255
#undef RGBA255

// constant colors
#define RGBA(r,g,b,a)		((int)((r)*255) | ((int)((g)*255)<<8) | ((int)((b)*255)<<16) | ((int)((a)*255)<<24))
#define RGB(r,g,b)			RGBA(r,g,b,1)
#define RGB255(r,g,b)		((r) | ((g)<<8) | ((b)<<16) | (255<<24))
#define RGBA255(r,g,b,a)	((r) | ((g)<<8) | ((b)<<16) | ((a)<<24))

// computed colors
//?? make as methods; or - constructor or CColor
#define RGBAS(r,g,b,a)		(appRound((r)*255) | (appRound((g)*255)<<8) | (appRound((b)*255)<<16) | (appRound((a)*255)<<24))
#define RGBS(r,g,b)			(appRound((r)*255) | (appRound((g)*255)<<8) | (appRound((b)*255)<<16) | (255<<24))


float NormalizeColor (const CVec3 &in, CVec3 &out);
float NormalizeColor255 (const CVec3 &in, CVec3 &out);

// rename to CLAMP_COLOR255 ??
#define NORMALIZE_COLOR255(r,g,b) \
	if ((r|g|b) > 255)		\
	{						\
		int		m;			\
		m = max(r,g);		\
		m = max(m,b);		\
		m = 255 * 256 / m;	\
		r = r * m >> 8;		\
		g = g * m >> 8;		\
		b = b * m >> 8;		\
	}

float ClampColor255 (const CVec3 &in, CVec3 &out);


/*-----------------------------------------------------------------------------
	Text parser
-----------------------------------------------------------------------------*/

char *COM_Parse (const char *&data_p, bool allowLineBreaks = true);
const char *COM_QuoteString (const char *str, bool alwaysQuote);
// data is an in/out parm, returns a parsed out token

/*-----------------------------------------------------------------------------
	Byte-order functions
-----------------------------------------------------------------------------*/

#ifdef LITTLE_ENDIAN

#define LittleShort(n)	((short)n)
#define LittleLong(n)	((long)n)
#define LittleFloat(n)	((float)n)

#else

short	LittleShort (short l);
int		LittleLong (int l);
float	LittleFloat (float l);

#endif


/*-----------------------------------------------------------------------------
	Collision detection (??)
-----------------------------------------------------------------------------*/

struct csurface_t
{
	// standard csurface_t fields (do not change this - used in trace
	//  functions, which are exported to game !)
	char	shortName[16];
	unsigned flags;
	int		value;
	// field from mapsurface_t (so, csurface_t now contains old
	//  csurface_t and mapsurface_t)
	char	fullName[32];	// shortName[] is too short ...
	// fields added since 4.00
	int		material;
};

// a trace is returned when a box is swept through the world
struct trace_t
{
	bool		allsolid;			// if true, plane is not valid
	byte		pad1[3];			// qboolean pad
	bool		startsolid;			// if true, the initial point was in a solid area
	byte		pad2[3];			// qboolean pad
	float		fraction;			// time completed, 1.0 = didn't hit anything
	CVec3		endpos;				// final position
	cplane_t	plane;				// surface normal at impact
	csurface_t	*surface;			// surface hit
	int			contents;			// contents on other side of surface hit
	struct edict_s *ent;			// not set by CM_*() functions; unknown type at this point !
};


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
	trace_t		(*trace) (const CVec3 &start, const CVec3 &mins, const CVec3 &maxs, const CVec3 &end);
	int			(*pointcontents) (const CVec3 &point);
};


// player_state_t->rdflags (refdef flags)
#define	RDF_UNDERWATER		1		// warp the screen as apropriate
#define RDF_NOWORLDMODEL	2		// used for player configuration screen
//ROGUE
#define	RDF_IRGOGGLES		4
#define RDF_UVGOGGLES		8		//?? unused
// new since 4.00
#define RDF_THIRD_PERSON	0x10


//=============================================================================

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
	inline void SetAnim (int legs, int torso, int moveDir = 0, float pitch = 0)
	{
		anim = legs | (torso << 6) | (moveDir << 12) | ((appRound (pitch) + 90) << 15);
	}
	inline void GetAnim (int &legs, int &torso, int &moveDir, float &pitch) const
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


#endif // QSHARED_H
