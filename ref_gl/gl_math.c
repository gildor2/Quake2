#include "gl_local.h"
#include "gl_math.h"

/*----- Tables for fast periodic function computation -----*/

float sinTable[TABLE_SIZE], squareTable[TABLE_SIZE], triangleTable[TABLE_SIZE],
		 sawtoothTable[TABLE_SIZE], inverseSwatoothTable[TABLE_SIZE];

float *mathFuncs[5] = {
	sinTable, squareTable, triangleTable, sawtoothTable, inverseSwatoothTable
};

/*---------- Tables for non-periodic functions ------------*/

float asinTable[TABLE_SIZE*2], acosTable[TABLE_SIZE*2];
float atanTable[TABLE_SIZE], atanTable2[TABLE_SIZE];


//!! these tables are unused now
float sqrtTable[256];
int   noiseTablei[256];
float noiseTablef[256];


void GL_InitFuncTables (void)
{
	int		i, n;

	for (i = 0; i < TABLE_SIZE; i++)
	{
		float	j, f;

		j = i;			// just to avoid "(float)i"
		f = j / TABLE_SIZE;
		squareTable[i] = (i < TABLE_SIZE/2) ? -1 : 1;
		sinTable[i] = sin (j / (TABLE_SIZE/2) * M_PI);		// 0 -- 0, last -- 2*pi
		sawtoothTable[i] = f;
		inverseSwatoothTable[i] = 1 - f;

		n = (i + TABLE_SIZE/4) & TABLE_MASK;				// make range -1..1..-1
		if (n >= TABLE_SIZE/2)
			n = TABLE_SIZE - n;
		triangleTable[i] = (float)(n - TABLE_SIZE/4) / (TABLE_SIZE/4);

		asinTable[i] = asin (f - 1);
		asinTable[i + TABLE_SIZE] = asin (f);
		acosTable[i] = acos (f - 1);
		acosTable[i + TABLE_SIZE] = acos (f);
		atanTable[i] = atan (f);
		atanTable2[i] = atan (1.0f / f);
	}

	for (i = 0; i < 256; i++)
	{
		sqrtTable[i] = pow (i / 255.0f, 0.5f);
		noiseTablei[i] = Q_round (rand() * 255.0f / RAND_MAX) & 0xFF;
		noiseTablef[i] = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;	// range -1..1
	}
}


// Project localOrigin to a world coordinate system
void ModelToWorldCoord (vec3_t localOrigin, refEntity_t *e, vec3_t center)
{
	VectorMA (e->origin, localOrigin[0], e->axis[0], center);
	VectorMA (center,	 localOrigin[1], e->axis[1], center);
	VectorMA (center,	 localOrigin[2], e->axis[2], center);
}


void WorldToModelCoord (vec3_t world, refEntity_t *e, vec3_t local)
{
	vec3_t	v;
	int		i;

	VectorSubtract (world, e->origin, v);
	for (i = 0; i < 3; i++)
		local[i] = DotProduct (v, e->axis[i]);
}


// 12 + 8 * 4 = 44 dots
// mins/maxs should be center-related (symmetric ?? -- replace with maxs only?)
qboolean GetBoxRect (refEntity_t *ent, vec3_t mins, vec3_t maxs, float mins2[2], float maxs2[2])
{
	vec3_t	axis[3], tmp;
	float	viewdist, x0, y0;
	int		i;

	// rotate screen axis
	for (i = 0; i < 3; i++)
	{
		axis[i][0] = DotProduct (vp.viewaxis[i], ent->axis[0]);
		axis[i][1] = DotProduct (vp.viewaxis[i], ent->axis[1]);
		axis[i][2] = DotProduct (vp.viewaxis[i], ent->axis[2]);
	}
	VectorSubtract (ent->center, vp.vieworg, tmp);
	x0 = DotProduct (tmp, vp.viewaxis[1]);
	y0 = DotProduct (tmp, vp.viewaxis[2]);
	viewdist = DotProduct (tmp, vp.viewaxis[0]);

	if (viewdist < 4) return false;

	// ClearBounds2D(mins2, maxs2)
	mins2[0] = mins2[1] = 999999;
	maxs2[0] = maxs2[1] = -999999;
	// enumerate all box points (can try to optimize: find contour ??)
	for (i = 0; i < 8; i++)
	{
		vec3_t	v;
		float	proj, x, y, scale;

		v[0] = (i & 1) ? maxs[0] : mins[0];
		v[1] = (i & 2) ? maxs[1] : mins[1];
		v[2] = (i & 4) ? maxs[2] : mins[2];

		proj = -DotProduct (v, axis[0]);
		if (proj >= viewdist) return false;			// this box vertex have negative z-coord
		VectorMA (v, -proj, axis[0], v);
		scale = viewdist / (viewdist - proj);
		x = (DotProduct (v, axis[1]) + x0) * scale - x0;
		y = (DotProduct (v, axis[2]) + y0) * scale - y0;
		EXPAND_BOUNDS(x, mins2[0], maxs2[0]);
		EXPAND_BOUNDS(y, mins2[1], maxs2[1]);
	}
	return true;
}
