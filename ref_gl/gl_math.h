#ifndef __GL_MATH_INCLUDED__
#define __GL_MATH_INCLUDED__


#include "gl_frontend.h"


#define TABLE_SIZE	1024
#define TABLE_MASK	(TABLE_SIZE-1)

extern float sinTable[], squareTable[], triangleTable[], sawtoothTable[], inverseSwatoothTable[];
extern float asinTable[], acosTable[];
extern float atanTable[], atanTable2[];

// for float 0..1 == 0..2*pi
#define PERIODIC_FUNC(tbl,val)			tbl[Q_round((val)*TABLE_SIZE) & TABLE_MASK]
// for integer 0..range == 0..2*pi
#define PERIODIC_FUNC2(tbl,val,range)	tbl[((val) * (int)(TABLE_SIZE / (range))) & TABLE_MASK]

// NOTE: SIN/COS_FUNC(v) will accept float value in a range 0..1 ~ 0..2*PI
#define SIN_FUNC(val)					PERIODIC_FUNC(sinTable,val)
#define SIN_FUNC2(val,range)			PERIODIC_FUNC2(sinTable,val,range)
#define COS_FUNC(val)					PERIODIC_FUNC(sinTable,(val)+0.25f)
#define COS_FUNC2(val,range)			PERIODIC_FUNC2(sinTable,(val)+(range)/4,range)

// WARNING: these functions are not periodic and not clamped; input value should be exactly in [-1..1] range
// Used (TABLE_SIZE-0.1f) to avoid [TABLE_SIZE*2] index for 1.0f value, should be [TABLE_SIZE*2-1] (OR: use index clamping ??)
#define ASIN_FUNC(val)					asinTable[Q_floor ((val) * (TABLE_SIZE-0.1f)) + TABLE_SIZE]
#define ACOS_FUNC(val)					acosTable[Q_floor ((val) * (TABLE_SIZE-0.1f)) + TABLE_SIZE]


__inline float ATAN2_FUNC(float y, float x)
{
	float	val, m;

	if (x == 0) return y > 0 ? M_PI / 2 : - M_PI / 2;

	val = y / x;
	if (val < 0)
	{
		val = -val;
		m = -1;
	}
	else
		m = 1;
	if (val <= 1.0f)
		val = atanTable[Q_floor (val * (TABLE_SIZE - 0.1f))];
	else
		val = atanTable2[Q_floor (1.0f / val * (TABLE_SIZE - 0.1f))];
	val *= m;
	if (x < 0)
	{
		if (y > 0)
			val += M_PI;
		else
			val -= M_PI;
	}
	return val;
}

#define SATURATE(c,l,v) c = l+(c-l)*v; c = bound(c, 0, 255);


extern float *mathFuncs[];


//!! these tables are unused now
extern float sqrtTable[];
extern int   noiseTablei[];
extern float noiseTablef[];


void GL_InitFuncTables (void);
void ModelToWorldCoord (vec3_t localOrigin, refEntity_t *e, vec3_t center);
void WorldToModelCoord (vec3_t world, refEntity_t *e, vec3_t local);
bool GetBoxRect (refEntity_t *ent, vec3_t size2, float mins2[2], float maxs2[2], bool clamp);


#endif
