#include "CorePrivate.h"

//?? color: (CColor3f : CVec3), (CColor4f : CColor3f + float alpha)


float appRsqrt (float number)
{
	float x2 = number * 0.5f;
	uint_cast(number) = 0x5F3759DF - (uint_cast(number) >> 1);
	number = number * (1.5f - (x2 * number * number));	// 1st iteration
//	number = number * (1.5f - (x2 * number * number));	// 2nd iteration, this can be removed
	return number;
}


/*-----------------------------------------------------------------------------
	Color functions
-----------------------------------------------------------------------------*/

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
