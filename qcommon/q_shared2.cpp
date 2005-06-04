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
//#include "q_shared2.h"
// we require some ri.func() declarations => some type defines etc
#include "qcommon.h"

//?? split to q_shared2.cpp, Core/{Math1D, Math3D, MathColor}.cpp
//?? color: (CColor3f : CVec3), (CColor4f : CColor3f + float alpha)


const CVec3 nullVec3 = {0, 0, 0};
const CAxis identAxis = {1,0,0,  0,1,0,  0,0,1};

//============================================================================

float NormalizeColor (const CVec3 &in, CVec3 &out)
{
	float	m;

	m = max(in[0], in[1]);
	m = max(m, in[2]);

	if (!m)
		out.Zero();
	else
	{
		float denom = 1.0f / m;
		out[0] = in[0] * denom;
		out[1] = in[1] * denom;
		out[2] = in[2] * denom;
	}
	return m;
}

float NormalizeColor255 (const CVec3 &in, CVec3 &out)
{
	float	m;

	m = max(in[0], in[1]);
	m = max(m, in[2]);

	if (!m)
		out.Zero();
	else
	{
		float denom = 255.0f / m;
		out[0] = in[0] * denom;
		out[1] = in[1] * denom;
		out[2] = in[2] * denom;
	}
	return m;
}

float ClampColor255 (const CVec3 &in, CVec3 &out)
{
	float	m;

	m = max(in[0], in[1]);
	m = max(m, in[2]);

	if (m < 255)
		out = in;
	else
	{
		float denom = 255.0f / m;
		out[0] = in[0] * denom;
		out[1] = in[1] * denom;
		out[2] = in[2] * denom;
	}
	return m;
}

float Q_rsqrt (float number)
{
	const float threehalfs = 1.5f;

	float x2 = number * 0.5f;
	float y  = number;
	long i = * reinterpret_cast<long*>(&y);
	i  = 0x5F3759DF - (i >> 1);
	y  = *(float*) &i;
	y  = y * (threehalfs - (x2 * y * y));	// 1st iteration
//	y  = y * (threehalfs - (x2 * y * y));	// 2nd iteration, this can be removed
	return y;
}


//?? rename to EulerToAxis etc
void AngleVectors (const CVec3 &angles, CVec3 *forward, CVec3 *right, CVec3 *up)
{
	float	angle;
	float	sr, sp, sy, cr, cp, cy;

	if (angles[YAW])
	{
		angle = angles[YAW] * (M_PI*2 / 360);
		sy = sin (angle);
		cy = cos (angle);
	}
	else
	{
		sy = 0;
		cy = 1;
	}

	if (angles[PITCH])
	{
		angle = angles[PITCH] * (M_PI*2 / 360);
		sp = sin (angle);
		cp = cos (angle);
	}
	else
	{
		sp = 0;
		cp = 1;
	}

	if (right || up)
	{
		if (angles[ROLL])
		{
			angle = angles[ROLL] * (M_PI*2 / 360);
			sr = sin (angle);
			cr = cos (angle);
		}
		else
		{
			sr = 0;
			cr = 1;
		}
	}

	if (forward)
		forward->Set (cp*cy, cp*sy, -sp);

	if (right)
		right->Set (-sr*sp*cy + cr*sy, -sr*sp*sy - cr*cy, -sr*cp);

	if (up)
		up->Set (cr*sp*cy + sr*sy, cr*sp*sy - sr*cy, cr*cp);
	// algles (0,0,0) => f(1,0,0) r(0,-1,0) u(0,0,1)
}


// Gives "forward" (normalized!) and makes 2 additional axes for it
void MakeNormalVectors (const CVec3 &forward, CVec3 &right, CVec3 &up)
{
	// this rotate and negate guarantees a vector
	// not colinear with the original
	right.Set (forward[2], -forward[0], forward[1]);
	float d = dot(right, forward);
	VectorMA (right, -d, forward);
	right.Normalize ();
	cross (right, forward, up);
}


float VectorLength (const CVec3 &v)
{
	return sqrt (dot (v, v));
}

float VectorDistance (const CVec3 &vec1, const CVec3 &vec2)
{
	CVec3	vec;
	VectorSubtract (vec1, vec2, vec);
	return VectorLength (vec);
}


/*-----------------------------------------------------------------------------
	Angle functions (taken from Quake3)
-----------------------------------------------------------------------------*/

//!! incorrect in most cases (should be slerp); used by cl_ents.cpp only
float LerpAngle (float a1, float a2, float frac)
{
	if (a2 - a1 > 180)
		a2 -= 360;
	if (a2 - a1 < -180)
		a2 += 360;
	return Lerp (a1, a2, frac);
}

float AngleMod (float a)
{
	a = (360.0f/65536) * (appRound (a*(65536.0f/360)) & 65535);
	return a;
}

float AngleSubtract (float a1, float a2)
{
	float a = a1 - a2;
	while (a > 180)
		a -= 360;
	while (a < -180)
		a += 360;
	return a;
}

void AnglesSubtract (const CVec3 &v1, const CVec3 &v2, CVec3 &v3)
{
	v3[0] = AngleSubtract (v1[0], v2[0]);
	v3[1] = AngleSubtract (v1[1], v2[1]);
	v3[2] = AngleSubtract (v1[2], v2[2]);
}


/*-----------------------------------------------------------------------------
	CVec3
-----------------------------------------------------------------------------*/

float CVec3::Normalize ()
{
	float length = sqrt (dot(*this, *this));
	if (length) Scale (1.0f/length);
	return length;
}

//?? move
float VectorNormalize (const CVec3 &v, CVec3 &out)
{
	float length = sqrt (dot(v, v));
	if (length)
	{
		float ilength = 1.0f / length;
		VectorScale (v, ilength, out);
	}
	return length;
}

float CVec3::NormalizeFast ()
{
	float len2 = dot(*this, *this);
	float denom = Q_rsqrt (len2);
	Scale (denom);
	return len2 * denom;
}

void cross (const CVec3 &v1, const CVec3 &v2, CVec3 &result)
{
	result[0] = v1[1] * v2[2] - v1[2] * v2[1];
	result[1] = v1[2] * v2[0] - v1[0] * v2[2];
	result[2] = v1[0] * v2[1] - v1[1] * v2[0];
}


/*-----------------------------------------------------------------------------
	CBox
-----------------------------------------------------------------------------*/

void CBox::Clear ()
{
	mins[0] = mins[1] = mins[2] = BIG_NUMBER;
	maxs[0] = maxs[1] = maxs[2] = -BIG_NUMBER;
}

void CBox::Expand (const CVec3 &p)
{
	for (int i = 0; i < 3; i++)
	{
		float val = p[i];
		EXPAND_BOUNDS(val, mins[i], maxs[i]);
	}
}

void CBox::Expand (const CBox &b)
{
	for (int i = 0; i < 3; i++)
	{
		if (mins[i] > b.mins[i]) mins[i] = b.mins[i];
		if (maxs[i] < b.maxs[i]) maxs[i] = b.maxs[i];
	}
}

void CBox::GetCenter (CVec3 &p)
{
	for (int i = 0; i < 3; i++)
		p[i] = (mins[i] + maxs[i]) / 2;
}

// returns 1, 2, or 1 + 2
int CBox::OnPlaneSide (const cplane_t &p) const
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

	int sides = 0;
	dist1 = p.dist - dist1;
	dist2 = dist2 - p.dist;
	sides = IsNegative (dist1) + (IsNegative (dist2) << 1);

	return sides;
}


/*-----------------------------------------------------------------------------
	CAxis
-----------------------------------------------------------------------------*/

void CAxis::FromAngles (const CVec3 &angles)
{
	CVec3	right;
	// AngleVectors() returns "right" instead of "y axis"
	AngleVectors (angles, &v[0], &right, &v[2]);
	VectorNegate (right, v[1]);
}

void CAxis::TransformVector (const CVec3 &src, CVec3 &dst) const
{
	dst[0] = dot(src, v[0]);
	dst[1] = dot(src, v[1]);
	dst[2] = dot(src, v[2]);
}

void CAxis::UnTransformVector (const CVec3 &src, CVec3 &dst) const
{
	CVec3 tmp;
	VectorScale (v[0], src[0], tmp);
	VectorMA (tmp, src[1], v[1], tmp);
	VectorMA (tmp, src[2], v[2], dst);
}

void CAxis::TransformAxis (const CAxis &src, CAxis &dst) const
{
	CAxis tmp;
	TransformVector (src[0], tmp[0]);
	TransformVector (src[1], tmp[1]);
	TransformVector (src[2], tmp[2]);
	dst = tmp;
}

void CAxis::UnTransformAxis (const CAxis &src, CAxis &dst) const
{
	CAxis tmp;
	UnTransformVector (src[0], tmp[0]);
	UnTransformVector (src[1], tmp[1]);
	UnTransformVector (src[2], tmp[2]);
	dst = tmp;
}


/*-----------------------------------------------------------------------------
	CCoords
-----------------------------------------------------------------------------*/

void CCoords::TransformPoint (const CVec3 &src, CVec3 &dst) const
{
	CVec3 tmp;
	VectorSubtract (src, origin, tmp);
	dst[0] = dot(tmp, axis[0]);
	dst[1] = dot(tmp, axis[1]);
	dst[2] = dot(tmp, axis[2]);
}

void CCoords::UnTransformPoint (const CVec3 &src, CVec3 &dst) const
{
	CVec3 tmp;
	VectorMA (origin, src[0], axis[0], tmp);
	VectorMA (tmp,	  src[1], axis[1], tmp);
	VectorMA (tmp,	  src[2], axis[2], dst);
}

void CCoords::TransformCoords (const CCoords &src, CCoords &dst) const
{
	TransformPoint (src.origin, dst.origin);
	axis.TransformAxis (src.axis, dst.axis);
}

void CCoords::UnTransformCoords (const CCoords &src, CCoords &dst) const
{
	UnTransformPoint (src.origin, dst.origin);
	axis.UnTransformAxis (src.axis, dst.axis);
}

void CCoords::TransformPlane (const cplane_t &src, cplane_t &dst) const
{
	dst.dist = src.dist - dot(origin, src.normal);
	axis.TransformVector (src.normal, dst.normal);
	dst.SetSignbits ();
	// NOTE: dst.type is not set
}


void TransformPoint (const CVec3 &origin, const CAxis &axis, const CVec3 &src, CVec3 &dst)
{
	CVec3 tmp;
	VectorSubtract (src, origin, tmp);
	dst[0] = dot(tmp, axis[0]);
	dst[1] = dot(tmp, axis[1]);
	dst[2] = dot(tmp, axis[2]);
}

void UnTransformPoint (const CVec3 &origin, const CAxis &axis, const CVec3 &src, CVec3 &dst)
{
	CVec3 tmp;
	VectorMA (origin, src[0], axis[0], tmp);
	VectorMA (tmp,	  src[1], axis[1], tmp);
	VectorMA (tmp,	  src[2], axis[2], dst);
}


/*
============================================================================

					BYTE ORDER FUNCTIONS

============================================================================
*/

short ShortSwap (short l)
{
	byte b1 = l & 255;
	byte b2 = (l>>8) & 255;

	return (b1<<8) + b2;
}

short ShortNoSwap (short l)
{
	return l;
}

int LongSwap (int l)
{
	byte b1 = l & 255;
	byte b2 = (l>>8) & 255;
	byte b3 = (l>>16) & 255;
	byte b4 = (l>>24) & 255;

	return ((int)b1<<24) + ((int)b2<<16) + ((int)b3<<8) + b4;
}

int	LongNoSwap (int l)
{
	return l;
}

float FloatSwap (float f)
{
	union {
		float	f;
		byte	b[4];
	} dat1, dat2;

	dat1.f = f;
	dat2.b[0] = dat1.b[3];
	dat2.b[1] = dat1.b[2];
	dat2.b[2] = dat1.b[1];
	dat2.b[3] = dat1.b[0];
	return dat2.f;
}

float FloatNoSwap (float f)
{
	return f;
}

#ifndef LITTLE_ENDIAN

static bool bigendian;

// can't just use function pointers, or dll linkage can
// mess up when qcommon is included in multiple places
short	(*_BigShort) (short l);
short	(*_LittleShort) (short l);
int		(*_LittleLong) (int l);
float	(*_LittleFloat) (float l);

short	BigShort(short l){return _BigShort(l);}
short	LittleShort(short l) {return _LittleShort(l);}
int		LittleLong (int l) {return _LittleLong(l);}
float	LittleFloat (float l) {return _LittleFloat(l);}

/*
================
Swap_Init
================
*/
void Swap_Init (void)
{
	static const byte swaptest[2] = {1,0};

	// set the byte swapping variables in a portable manner
	if ( *(short *)swaptest == 1)
	{
		bigendian = false;
		_BigShort = ShortSwap;
		_LittleShort = ShortNoSwap;
		_LittleLong = LongNoSwap;
		_LittleFloat = FloatNoSwap;
	}
	else
	{
		bigendian = true;
		_BigShort = ShortNoSwap;
		_LittleShort = ShortSwap;
		_LittleLong = LongSwap;
		_LittleFloat = FloatSwap;
	}

}

#endif

static char com_token[MAX_STRING_CHARS];
static int	com_lines;

/*
============================================================================

					TEXT PARSER

============================================================================
*/

/*
==============
COM_Parse

Parse a token out of a string
==============
*/
static const char *SkipWhitespace (const char *data, bool *hasNewLines)
{
	int		c;

	while ((c = *data) <= ' ')
	{
		if (!c) return NULL;
		if (c == '\n')
		{
			com_lines++;
			*hasNewLines = true;
		}
		data++;
	}

	return data;
}

char *COM_Parse (const char *&data_p, bool allowLineBreaks)
{
	int		c, len;
	bool	hasNewLines;
	const char *data;

	data = data_p;
	len = c = 0;
	com_token[0] = 0;
	hasNewLines = false;

	if (!data)					// all data is out
		return com_token;		// ""

	while (1)
	{
		// skip whitespace
		data = SkipWhitespace (data, &hasNewLines);
		if (!data)
		{
			data_p = NULL;
			return com_token;
		}
		if (hasNewLines && !allowLineBreaks)
		{
			data_p = data;
			return com_token;
		}

		c = *data;

		// skip double slash comments
		if (c == '/' && data[1] == '/')
		{
			data += 2;
			while (*data && *data != '\n') data++;
		}
		// skip /* */ comments
		else if (c=='/' && data[1] == '*')
		{
			data += 2;
			while (*data && (*data != '*' || data[1] != '/'))
				data++;
			if (*data)
				data += 2;
		}
		else
			break;
	}

	// handle quoted strings
	if (c == '\"')
	{
		data++;
		while (1)
		{
			c = *data++;
			if (c == '\"' && *data == '\"')
			{
				// doubled quotes
				data++;				// skip both quotes
				if (len < sizeof(com_token))
					com_token[len++] = c;
				continue;
			}

			if (c=='\"' || !c)
			{
				com_token[len] = 0;
				data_p = (char *) data;
				return com_token;
			}

			if (len < sizeof(com_token))
				com_token[len++] = c;
		}
	}

	// parse a regular word
	do
	{
		if (len < sizeof(com_token))
		{
			com_token[len] = c;
			len++;
		}
		data++;
		c = *data;
		if (c == '\n')
			com_lines++;
	} while (c > 32);

	if (len == sizeof(com_token))
	{
//		Com_WPrintf ("Token exceeded %d chars, discarded.\n", sizeof(com_token));
		len = 0;
	}
	com_token[len] = 0;

	data_p = (char *) data;
	return com_token;
}

const char *COM_QuoteString (const char *str, bool alwaysQuote)
{
	char	*dst, c;

	if (!alwaysQuote)
	{
		const char *s;

		s = str;
		while (c = *s++)
			if (c == ' ' || c == '\"' || c == ';') break;
		if (!c) return str;				// line have no chars, which requires quoting
	}

	dst = com_token;
	*dst++ = '\"';
	while (c = *str++)
	{
		*dst++ = c;
		if (c == '\"') *dst++ = c;
	}
	*dst++ = '\"';
	*dst = 0;

	return com_token;
}


/*
=================
SkipRestOfLine
=================
*/
void SkipRestOfLine (char **data)
{
	char *p = *data;
	while (char c = *p++)
	{
		if (c == '\n')
		{
			com_lines++;
			break;
		}
	}

	*data = p;
}



/*
===============
Com_PageInMemory

===============
*/
int	paged_total;

void Com_PageInMemory (void *buffer, int size)
{
	byte	*buf = (byte*)buffer;
	for (int i=size-1 ; i>0 ; i-=4096)
		paged_total += buf[i];
}
