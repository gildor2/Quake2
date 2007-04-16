#include "CorePrivate.h"

/*-----------------------------------------------------------------------------
	Constant geometry objects
-----------------------------------------------------------------------------*/

const CVec3 nullVec3  = {0,0,0};
const CBox	nullBox   = {{0,0,0}, {0,0,0}};
const CAxis identAxis = {1,0,0,  0,1,0,  0,0,1};


/*-----------------------------------------------------------------------------
	CVec3
-----------------------------------------------------------------------------*/

float CVec3::GetLength()
{
	return sqrt(dot(*this, *this));
}

float CVec3::Normalize()
{
	float length = sqrt(dot(*this, *this));
	if (length) Scale(1.0f/length);
	return length;
}

//?? move
float VectorNormalize(const CVec3 &v, CVec3 &out)
{
	float length = sqrt(dot(v, v));
	if (length)
	{
		float ilength = 1.0f / length;
		VectorScale(v, ilength, out);
	}
	else
		out.Set(0, 0, 0);
	return length;
}

float CVec3::NormalizeFast()
{
	float len2 = dot(*this, *this);
	float denom = appRsqrt(len2);
	Scale(denom);
	return len2 * denom;
}

// source vector should be normalized
void CVec3::FindAxisVectors(CVec3 &right, CVec3 &up) const
{
	// create any vector, not collinear with "this"
	right.Set(v[2], -v[0], v[1]);
	// subtract projection on source (make vector orthogonal to source)
	VectorMA(right, -dot(right, *this), *this);
	right.Normalize();
	// generate third axis vector
	cross(right, *this, up);
}

void cross(const CVec3 &v1, const CVec3 &v2, CVec3 &result)
{
	result[0] = v1[1] * v2[2] - v1[2] * v2[1];
	result[1] = v1[2] * v2[0] - v1[0] * v2[2];
	result[2] = v1[0] * v2[1] - v1[1] * v2[0];
}

float VectorDistance(const CVec3 &vec1, const CVec3 &vec2)
{
	CVec3	vec;
	VectorSubtract(vec1, vec2, vec);
	return vec.GetLength();
}


/*-----------------------------------------------------------------------------
	CPlane
-----------------------------------------------------------------------------*/

void CPlane::Setup()
{
	signbits = IsNegative(normal[0]) + (IsNegative(normal[1]) << 1) + (IsNegative(normal[2]) << 2);
	if		(normal[0] ==  1.0f) type = PLANE_X;
	else if (normal[0] == -1.0f) type = PLANE_MX;
	else if (normal[1] ==  1.0f) type = PLANE_Y;
	else if (normal[1] == -1.0f) type = PLANE_MY;
	else if (normal[2] ==  1.0f) type = PLANE_Z;
	else if (normal[2] == -1.0f) type = PLANE_MZ;
	else type = PLANE_NON_AXIAL;
}


float CPlane::DistanceTo(const CBox& box) const
{
	float d;
	if (type <= PLANE_Z)
	{
		d = box.mins[type] - dist;
		if (!IsNegative(d)) return d; // >= 0
		d = box.maxs[type] - dist;
		if (IsNegative(d)) return d;
		return 0;
	}

	if (type <= PLANE_MZ)
	{
		d = -box.mins[type-3] - dist;
		if (!IsNegative(d)) return d;
		d = -box.maxs[type-3] - dist;
		if (IsNegative(d)) return d;
		return 0;
	}

	// non-axial plane
	// NOTE: code taken from CBox::OnPlaneSide()
	float i0 = normal[0] * box.mins[0];	// In -- mIns[n] * p.normal[n]
	float i1 = normal[1] * box.mins[1];
	float i2 = normal[2] * box.mins[2];
	float a0 = normal[0] * box.maxs[0];	// An -- mAxs[n] * p.normal[n]
	float a1 = normal[1] * box.maxs[1];
	float a2 = normal[2] * box.maxs[2];

	float	dist1, dist2;
	switch (signbits)
	{
	case 0:
		dist1 = a0 + a1 + a2;
		dist2 = i0 + i1 + i2;
		break;
	case 1:
		dist1 = i0 + a1 + a2;
		dist2 = a0 + i1 + i2;
		break;
	case 2:
		dist1 = a0 + i1 + a2;
		dist2 = i0 + a1 + i2;
		break;
	case 3:
		dist1 = i0 + i1 + a2;
		dist2 = a0 + a1 + i2;
		break;
	case 4:
		dist1 = a0 + a1 + i2;
		dist2 = i0 + i1 + a2;
		break;
	case 5:
		dist1 = i0 + a1 + i2;
		dist2 = a0 + i1 + a2;
		break;
	case 6:
		dist1 = a0 + i1 + i2;
		dist2 = i0 + a1 + a2;
		break;
	case 7:
//	default:					// shut up compiler
		dist1 = i0 + i1 + i2;
		dist2 = a0 + a1 + a2;
		break;
//	default:
//		dist1 = dist2 = 0;		// shut up compiler
//		break;
	}

	dist2 -= dist;
	if (!IsNegative(dist2)) return dist2;
	dist1 -= dist;
	if (IsNegative(dist1)) return dist1;
	return 0;
}


/*-----------------------------------------------------------------------------
	CBox
-----------------------------------------------------------------------------*/

void CBox::Clear()
{
	mins[0] = mins[1] = mins[2] =  BIG_NUMBER;
	maxs[0] = maxs[1] = maxs[2] = -BIG_NUMBER;
}

void CBox::Expand(const CVec3 &p)
{
	for (int i = 0; i < 3; i++)
	{
		float val = p[i];
		EXPAND_BOUNDS(val, mins[i], maxs[i]);
	}
}

void CBox::Expand(const CBox &b)
{
	for (int i = 0; i < 3; i++)
	{
		if (mins[i] > b.mins[i]) mins[i] = b.mins[i];
		if (maxs[i] < b.maxs[i]) maxs[i] = b.maxs[i];
	}
}

void CBox::GetCenter(CVec3 &p) const
{
	for (int i = 0; i < 3; i++)
		p[i] = (mins[i] + maxs[i]) / 2;
}

bool CBox::Contains(const CVec3 &p) const
{
	for (int i = 0; i < 3; i++)
		if (p[i] < mins[i] || p[i] > maxs[i]) return false;
	return true;
}

bool CBox::Intersects(const CBox &b) const
{
	for (int i = 0; i < 3; i++)
	{
		if (mins[i] >= b.maxs[i]) return false;
		if (maxs[i] <= b.mins[i]) return false;
	}
	return true;
}

// returns 1, 2, or 1 + 2
int CBox::OnPlaneSide(const CPlane &p) const
{
	float i0 = p.normal[0] * mins[0];	// In -- mIns[n] * p.normal[n]
	float i1 = p.normal[1] * mins[1];
	float i2 = p.normal[2] * mins[2];
	float a0 = p.normal[0] * maxs[0];	// An -- mAxs[n] * p.normal[n]
	float a1 = p.normal[1] * maxs[1];
	float a2 = p.normal[2] * maxs[2];

	float	dist1, dist2;
	switch (p.signbits)
	{
	case 0:
		dist1 = a0 + a1 + a2;
		dist2 = i0 + i1 + i2;
		break;
	case 1:
		dist1 = i0 + a1 + a2;
		dist2 = a0 + i1 + i2;
		break;
	case 2:
		dist1 = a0 + i1 + a2;
		dist2 = i0 + a1 + i2;
		break;
	case 3:
		dist1 = i0 + i1 + a2;
		dist2 = a0 + a1 + i2;
		break;
	case 4:
		dist1 = a0 + a1 + i2;
		dist2 = i0 + i1 + a2;
		break;
	case 5:
		dist1 = i0 + a1 + i2;
		dist2 = a0 + i1 + a2;
		break;
	case 6:
		dist1 = a0 + i1 + i2;
		dist2 = i0 + a1 + a2;
		break;
	case 7:
//	default:					// shut up compiler
		dist1 = i0 + i1 + i2;
		dist2 = a0 + a1 + a2;
		break;
//	default:
//		dist1 = dist2 = 0;		// shut up compiler
//		break;
	}

	dist1 = p.dist - dist1;
	dist2 = dist2 - p.dist;
	return IsNegative(dist1) + (IsNegative(dist2) << 1);
}


/*-----------------------------------------------------------------------------
	CAxis
-----------------------------------------------------------------------------*/

void CAxis::FromEuler(const CVec3 &angles)
{
	CVec3	right;
	// Euler2Vecs() returns "right" instead of "y axis"
	Euler2Vecs(angles, &v[0], &right, &v[2]);
	VectorNegate(right, v[1]);
}

void CAxis::TransformVector(const CVec3 &src, CVec3 &dst) const
{
	dst[0] = dot(src, v[0]);
	dst[1] = dot(src, v[1]);
	dst[2] = dot(src, v[2]);
}

void CAxis::UnTransformVector(const CVec3 &src, CVec3 &dst) const
{
	CVec3 tmp;
	VectorScale(v[0], src[0], tmp);
	VectorMA(tmp, src[1], v[1], tmp);
	VectorMA(tmp, src[2], v[2], dst);
}

void CAxis::TransformAxis(const CAxis &src, CAxis &dst) const
{
	CAxis tmp;
	TransformVector(src[0], tmp[0]);
	TransformVector(src[1], tmp[1]);
	TransformVector(src[2], tmp[2]);
	dst = tmp;
}

void CAxis::UnTransformAxis(const CAxis &src, CAxis &dst) const
{
	CAxis tmp;
	UnTransformVector(src[0], tmp[0]);
	UnTransformVector(src[1], tmp[1]);
	UnTransformVector(src[2], tmp[2]);
	dst = tmp;
}


/*-----------------------------------------------------------------------------
	CCoords
-----------------------------------------------------------------------------*/

void CCoords::TransformPoint(const CVec3 &src, CVec3 &dst) const
{
	CVec3 tmp;
	VectorSubtract(src, origin, tmp);
	dst[0] = dot(tmp, axis[0]);
	dst[1] = dot(tmp, axis[1]);
	dst[2] = dot(tmp, axis[2]);
}

void CCoords::UnTransformPoint(const CVec3 &src, CVec3 &dst) const
{
	CVec3 tmp;
	VectorMA(origin, src[0], axis[0], tmp);
	VectorMA(tmp,	  src[1], axis[1], tmp);
	VectorMA(tmp,	  src[2], axis[2], dst);
}

void CCoords::TransformCoords(const CCoords &src, CCoords &dst) const
{
	TransformPoint(src.origin, dst.origin);
	axis.TransformAxis(src.axis, dst.axis);
}

void CCoords::UnTransformCoords(const CCoords &src, CCoords &dst) const
{
	UnTransformPoint(src.origin, dst.origin);
	axis.UnTransformAxis(src.axis, dst.axis);
}

void CCoords::TransformPlane(const CPlane &src, CPlane &dst) const
{
	dst.dist = src.dist - dot(origin, src.normal);
	axis.TransformVector(src.normal, dst.normal);
	dst.Setup();
	// NOTE: dst.type is not set
}


void TransformPoint(const CVec3 &origin, const CAxis &axis, const CVec3 &src, CVec3 &dst)
{
	CVec3 tmp;
	VectorSubtract(src, origin, tmp);
	dst[0] = dot(tmp, axis[0]);
	dst[1] = dot(tmp, axis[1]);
	dst[2] = dot(tmp, axis[2]);
}

void UnTransformPoint(const CVec3 &origin, const CAxis &axis, const CVec3 &src, CVec3 &dst)
{
	CVec3 tmp;
	VectorMA(origin, src[0], axis[0], tmp);
	VectorMA(tmp,	  src[1], axis[1], tmp);
	VectorMA(tmp,	  src[2], axis[2], dst);
}


/*-----------------------------------------------------------------------------
	Angle math
-----------------------------------------------------------------------------*/

// algles (0,0,0) => f(1,0,0) r(0,-1,0) u(0,0,1)
void Euler2Vecs(const CVec3 &angles, CVec3 *forward, CVec3 *right, CVec3 *up)
{
	float	angle;
	float	sr, sp, sy, cr, cp, cy;	// roll/pitch/yaw sin/cos

	if (angles[YAW])
	{
		angle = angles[YAW] * M_PI / 180;
		sy = sin(angle);
		cy = cos(angle);
	}
	else
	{
		sy = 0;
		cy = 1;
	}

	if (angles[PITCH])
	{
		angle = angles[PITCH] * M_PI / 180;
		sp = sin(angle);
		cp = cos(angle);
	}
	else
	{
		sp = 0;
		cp = 1;
	}

	if (forward)
		forward->Set(cp*cy, cp*sy, -sp);

	if (!right && !up) return;

	if (angles[ROLL])
	{
		angle = angles[ROLL] * M_PI / 180;
		sr = sin(angle);
		cr = cos(angle);
	}
	else
	{
		sr = 0;
		cr = 1;
	}

	if (right)
		right->Set(-sr*sp*cy + cr*sy, -sr*sp*sy - cr*cy, -sr*cp);

	if (up)
		up->Set(cr*sp*cy + sr*sy, cr*sp*sy - sr*cy, cr*cp);
}


void Vec2Euler(const CVec3 &vec, CVec3 &angles)
{
	angles[ROLL] = 0;
	if (vec[0] == 0 && vec[1] == 0)
	{
		angles[YAW]   = 0;
		angles[PITCH] = (IsNegative(vec[2])) ? 90 : 270;
	}
	else
	{
		float yaw;
		if (vec[0])
		{
			yaw = atan2(vec[1], vec[0]) * 180 / M_PI;
			if (IsNegative(yaw)) yaw += 360;
		}
		else
			yaw = (!IsNegative(vec[1])) ? 90 : 270;

		float forward = sqrt(vec[0]*vec[0] + vec[1]*vec[1]);
		float pitch   = -atan2(vec[2], forward) * 180 / M_PI;
		if (IsNegative(pitch)) pitch += 360;

		angles[PITCH] = pitch;
		angles[YAW]   = yaw;
	}
}


// short version of Vec2Euler()
float Vec2Yaw(const CVec3 &vec)
{
	if (vec[0] == 0 && vec[1] == 0)
		return 0;

	if (vec[0])
	{
		float yaw = atan2(vec[1], vec[0]) * 180 / M_PI;
		if (IsNegative(yaw)) yaw += 360;
		return yaw;
	}
	else
		return !IsNegative(vec[1]) ? 90 : 270;
}
