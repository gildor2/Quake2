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
		noiseTablei[i] = (int)(rand() * 255.0f / RAND_MAX) & 0xFF;
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
