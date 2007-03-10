/*=============================================================================

	Miscellaneous math support

=============================================================================*/

/*-----------------------------------------------------------------------------
	Scalar math
-----------------------------------------------------------------------------*/

// same name/value in gcc's "math.h"
#undef M_PI
#define M_PI				(3.14159265358979323846)

CORE_API float appRsqrt(float number);
#define SQRTFAST(x)			(x * appRsqrt(x))

#if 1

#define uint_cast(f)		(reinterpret_cast<unsigned&>(f))
#define uint_cast_const(f)	(reinterpret_cast<const unsigned&>(f))

inline unsigned IsNegative(float f)
{
	return uint_cast(f) >> 31;
}

#define FAbsSign(f,d,s)	\
	{					\
		unsigned mask = uint_cast_const(f) & 0x80000000; \
		uint_cast(d)  = uint_cast_const(f) ^ mask;		\
		s = mask >> 31;	\
	}

#define FAbsSign2(f,d,s) \
	{					\
		s = uint_cast_const(f) & 0x80000000;	\
		uint_cast(d) = uint_cast_const(f) ^ s;	\
	}

inline void FAbs(float &a)
{
	uint_cast(a) &= 0x7FFFFFFF;
}

// NOTE: FNegate(0) will produce -0 (0x00000000 -> 0x80000000)
inline void FNegate(const float &a, float &b)
{
	uint_cast(b) = uint_cast_const(a) ^ 0x80000000;
}

inline void FNegate(float &a)
{
	uint_cast(a) ^= 0x80000000;
}

// change sign of 'f' using value 's', taken from FAbsSign2()
#define FChangeSign(f,s) \
	uint_cast(f) ^= s;

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

#define FAbsSign2(f,d,s)	FAbsSign(f,d,s)

inline void FAbs(float &a)
{
	a = fabs(a);
}

inline void FNegate(const float &a, float &b)
{
	b = -a;
}
inline void FNegate(float &a)
{
	a = -a;
}

#define FChangeSign(f,s)			\
	{								\
		float mul[] = { 1, -1 };	\
		f *= mul[s];				\
	}

#endif


/*-----------------------------------------------------------------------------
	Color math
-----------------------------------------------------------------------------*/

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


CORE_API float NormalizeColor(const CVec3 &in, CVec3 &out);
CORE_API float NormalizeColor255(const CVec3 &in, CVec3 &out);

//?? NOTE: small values will not be normalized (so - rename to CLAMP_COLOR() ?)
#define NORMALIZE_COLOR255(r,g,b) \
	if ((r|g|b) > 255)		\
	{						\
		int		m;			\
		m = max(r,g);		\
		m = max(m,b);		\
		m = (255 << 16) / m;\
		r = r * m >> 16;	\
		g = g * m >> 16;	\
		b = b * m >> 16;	\
	}

CORE_API float ClampColor255(const CVec3 &in, CVec3 &out);
