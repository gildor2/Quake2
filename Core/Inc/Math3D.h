/*=============================================================================

	3D math library

=============================================================================*/

/*-----------------------------------------------------------------------------
	Vectors
-----------------------------------------------------------------------------*/

struct CORE_API CVec3
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
	inline void Add (const CVec3 &a) //?? == "operator += (CVec3&)"
	{
		v[0] += a.v[0];
		v[1] += a.v[1];
		v[2] += a.v[2];
	}
	//!! +ScaleTo(dst)
	float GetLength ();
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

CORE_API void cross (const CVec3 &v1, const CVec3 &v2, CVec3 &result);

//?? OLD-STYLE FUNCTIONS :

inline void VectorSubtract (const CVec3 &a, const CVec3 &b, CVec3 &d)
{
	d.v[0] = a.v[0] - b.v[0];
	d.v[1] = a.v[1] - b.v[1];
	d.v[2] = a.v[2] - b.v[2];
}

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

CORE_API float VectorDistance (const CVec3 &vec1, const CVec3 &vec2);
//?? VectorDistanceFast()
CORE_API float VectorNormalize (const CVec3 &v, CVec3 &out);


/*-----------------------------------------------------------------------------
	Planes
-----------------------------------------------------------------------------*/

struct CORE_API CPlane
{
	// normal type
	//?? hide consts, use from CPlane methods only
	enum
	{
		PLANE_X,					// { 1, 0, 0}
		PLANE_Y,					// { 0, 1, 0}
		PLANE_Z,					// { 0, 0, 1}
		PLANE_MX,					// {-1, 0, 0}
		PLANE_MY,					// { 0,-1, 0}
		PLANE_MZ,					// { 0, 0,-1}
		PLANE_NON_AXIAL				// any another normal
	};

	// really, plane can be specified with "normal" and "dist" only; other fields - for speedup some calculations
	CVec3	normal;
	float	dist;
	byte	type;					// for fast DistanceTo() tests; PLANE_[M]X|Y|Z
	byte	signbits;				// sign(x) + (sign(y)<<1) + (sign(z)<<2); used for speedup CBox::OnPlaneSide()
	byte	pad[2];					// for dword alignment
	// methods
	void Setup ();					// setup signbits+type
	// distance is positive, when point placed on a normal-looking side
	inline float DistanceTo (const CVec3 &vec) const
	{
		if (type <= PLANE_Z)
			return vec[type] - dist;
		else if (type <= PLANE_MZ)
			return -vec[type-3] - dist;
		else
			return dot(normal, vec) - dist;
	}
};


/*-----------------------------------------------------------------------------
	Axis aligned box
-----------------------------------------------------------------------------*/

struct CORE_API CBox
{
	// fields
	CVec3	mins, maxs;
	// methods
	void Clear ();							// set mins>maxs (negative volume)
	void Expand (const CVec3 &p);			// expand mins/maxs by point p
	void Expand (const CBox &b);			// include box b into volume
	void GetCenter (CVec3 &p) const;		// compute center point
	bool Contains (const CVec3 &p) const;	// true, when point is inside box volume
	bool Intersects (const CBox &b) const;	// true, when boxes have common volume
	// returns 1 when box placed on a normal-looking side of plane, 2 when on opposite side,
	// and 3 (1+2) when box intersects plane
	int OnPlaneSide (const CPlane &plane) const;
};

// CBox::OnPlaneSide(), optimized for axis-aligned planes
inline int BOX_ON_PLANE_SIDE (const CBox &box, const CPlane &plane)
{
	if (plane.type <= CPlane::PLANE_MZ)
	{
		if (plane.type <= CPlane::PLANE_Z)
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


/*-----------------------------------------------------------------------------
	Axis and coordinate system
-----------------------------------------------------------------------------*/

struct CORE_API CAxis
{
	// fields
	// NOTE: v[] can be private, but this will prevent from
	// initializing CAxis object with initializer list ( "= {n,n,n,n ...}" )
	CVec3	v[3];
	// methods
	void FromEuler (const CVec3 &angles);
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


struct CORE_API CCoords
{
	// fields
	CVec3	origin;
	CAxis	axis;
	// methods
	void TransformPoint (const CVec3 &src, CVec3 &dst) const;
	void UnTransformPoint (const CVec3 &src, CVec3 &dst) const;
	void TransformCoords (const CCoords &src, CCoords &dst) const;
	void UnTransformCoords (const CCoords &src, CCoords &dst) const;
	void TransformPlane (const CPlane &src, CPlane &dst) const;
};

// Functions for work with coordinate systems, not combined into CCoords class

// global coordinate system -> local coordinate system (src -> dst) by origin/axis coords
CORE_API void TransformPoint (const CVec3 &origin, const CAxis &axis, const CVec3 &src, CVec3 &dst);
// local coordinate system -> global coordinate system
CORE_API void UnTransformPoint (const CVec3 &origin, const CAxis &axis, const CVec3 &src, CVec3 &dst);


/*-----------------------------------------------------------------------------
	Angle math
-----------------------------------------------------------------------------*/

// angle indexes
enum
{
	PITCH,							// looking up and down (0=Straight Ahead, +Up, -Down).
	YAW,							// rotating around (running in circles), 0=East, +North, -South.
	ROLL							// rotation about axis of screen, 0=Straight, +Clockwise, -CCW.
};

CORE_API void Euler2Vecs (const CVec3 &angles, CVec3 *forward, CVec3 *right, CVec3 *up);
CORE_API void MakeNormalVectors (const CVec3 &forward, CVec3 &right, CVec3 &up);
CORE_API void Vec2Euler (const CVec3 &vec, CVec3 &angles);
CORE_API float Vec2Yaw (const CVec3 &vec);


/*-----------------------------------------------------------------------------
	Some constants
-----------------------------------------------------------------------------*/

CORE_API extern const CVec3 nullVec3;
CORE_API extern const CBox  nullBox;
CORE_API extern const CAxis identAxis;
