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
		noiseTablei[i] = appRound (rand() * 255.0f / RAND_MAX) & 0xFF;
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


/* Rotate basis counter-clockwise around 'axis' by 'angle' degrees
 *
 * Method (details can be found in OpenGL Rotate{T}() documentation):
 *
 *  u = VectorNormalize(v) = (x y z)'
 *
 *      |  0   -z    y  |            | xx  xy  xz |
 *  S = |  z    0   -x  |      uu' = | xy  yy  yz |
 *      | -y    x    0  |            | xz  yz  zz |
 *
 *  R = uu' + cosQ*(I - uu') + sinQ*S
 *
 *      | xx+(1-xx)cosQ        xy(1-cosQ)-z*sinQ    xz(1-cosQ)+y*sinQ |
 *  R = | xy(1-cosQ)+z*sinQ    yy+(1-yy)cosQ        yz(1-cosQ)-x*sinQ |
 *      | xz(1-cosQ)-y*sinQ    yz(1-cosQ)+x*sinQ    zz+(1-zz)cosQ     |
 */

void BuildRotationMatrix (float r[3][3], vec3_t axis, float angle)
{
	float	xx, xy, xz, yy, yz, zz;
	float	q, sq, cq, ncq;
	vec3_t	axisn;
#define x axisn[0]
#define y axisn[1]
#define z axisn[2]

	q = angle / 180 * M_PI;
	VectorNormalize2 (axis, axisn);
	sq = sin(q);
	cq = cos(q); ncq = 1 - cq;
	// compute uu'
	xx = x * x; xy = x * y; xz = x * z;
	yy = y * y; yz = y * z; zz = z * z;
	// compute R
	r[0][0] = xx + (1 - xx) * cq;    r[0][1] = xy * ncq - z * sq;    r[0][2] = xz * ncq + y * sq;
	r[1][0] = xy * ncq + z * sq;     r[1][1] = yy + (1 - yy) * cq;   r[1][2] = yz * ncq - x * sq;
	r[2][0] = xz * ncq - y * sq;     r[2][1] = yz * ncq + x * sq;    r[2][2] = zz + (1 - zz) * cq;
#undef x
#undef y
#undef z
}


// 3/12 + 4 * 3 = 15/24 dots (worldMatrix/non-worldMatrix)
// In: ent -- axis + center, center+size2 = maxs, center-size2 = mins
bool GetBoxRect (refEntity_t *ent, vec3_t size2, float mins2[2], float maxs2[2], bool clamp)
{
	vec3_t	axis[3], tmp, v;
	float	x0, y0, z0;
	int		i;

	if (!ent->worldMatrix)
	{
		// rotate screen axis
		for (i = 0; i < 3; i++)
		{
			axis[i][0] = DotProduct (vp.viewaxis[i], ent->axis[0]);
			axis[i][1] = DotProduct (vp.viewaxis[i], ent->axis[1]);
			axis[i][2] = DotProduct (vp.viewaxis[i], ent->axis[2]);
		}
	}
	else
	{
		VectorCopy (vp.viewaxis[0], axis[0]);
		VectorCopy (vp.viewaxis[1], axis[1]);
		VectorCopy (vp.viewaxis[2], axis[2]);
	}

	VectorSubtract (ent->center, vp.vieworg, tmp);
	z0 = DotProduct (tmp, vp.viewaxis[0]);
	if (z0 < 4) return false;
	x0 = DotProduct (tmp, vp.viewaxis[1]);
	y0 = DotProduct (tmp, vp.viewaxis[2]);

	// ClearBounds2D(mins2, maxs2)
	mins2[0] = mins2[1] = BIG_NUMBER;
	maxs2[0] = maxs2[1] = -BIG_NUMBER;

	// enumerate all box points (can try to optimize: find contour ??)
	v[2] = size2[2];									// constant for all iterations
	for (i = 0; i < 4; i++)
	{
		float	x, y, z, scale, x1, y1;

		v[0] = (i & 1) ? size2[0] : -size2[0];
		v[1] = (i & 2) ? size2[1] : -size2[1];

		z = DotProduct (v, axis[0]);
		if ((z >= z0 || z <= -z0)) return false;			// this box vertex have negative z-coord
		x = DotProduct (v, axis[1]);
		y = DotProduct (v, axis[2]);

		// we use orthogonal projection of symmetric box - can use half of computations
		scale = z0 / (z0 + z);
		x1 = (x0 + x) * scale - x0;
		y1 = (y0 + y) * scale - y0;
		EXPAND_BOUNDS(x1, mins2[0], maxs2[0]);
		EXPAND_BOUNDS(y1, mins2[1], maxs2[1]);

		scale = z0 / (z0 - z);
		x1 = (x0 - x) * scale - x0;
		y1 = (y0 - y) * scale - y0;
		EXPAND_BOUNDS(x1, mins2[0], maxs2[0]);
		EXPAND_BOUNDS(y1, mins2[1], maxs2[1]);
	}

	if (clamp)
	{
		float	f;

		f = vp.t_fov_x * z0;
		if (maxs2[0] + x0 < -f) return false;
		if (mins2[0] + x0 > f) return false;
		if (maxs2[0] + x0 > f) maxs2[0] = f - x0;
		if (mins2[0] + x0 < -f) mins2[0] = -f - x0;
		if (mins2[0] >= maxs2[0]) return false;

		f = vp.t_fov_y * z0;
		if (maxs2[1] + y0 < -f) return false;
		if (mins2[1] + y0 > f) return false;
		if (maxs2[1] + y0 > f) maxs2[1] = f - y0;
		if (mins2[1] + y0 < -f) mins2[1] = -f - y0;
		if (mins2[1] >= maxs2[1]) return false;
	}

	return true;
}


// Project 3D point to screen coordinates; return false when not in view frustum
bool ProjectToScreen (vec3_t pos, int *scr)
{
	vec3_t	vec;
	float	x, y, z;

	VectorSubtract (pos, vp.vieworg, vec);

	z = DotProduct (vec, vp.viewaxis[0]);
	if (z <= gl_znear->value) return false;			// not visible

	x = DotProduct (vec, vp.viewaxis[1]) / z / vp.t_fov_x;
	if (x < -1 || x > 1) return false;

	y = DotProduct (vec, vp.viewaxis[2]) / z / vp.t_fov_y;
	if (y < -1 || y > 1) return false;

	scr[0] = appRound (vp.x + vp.w * (0.5 - x / 2));
	scr[1] = appRound (vp.y + vp.h * (0.5 - y / 2));

	return true;
}


void SaturateColor3f (vec3_t color)
{
	float sat = r_saturation->value;
	if (sat != 1.0f)
	{
		float	r, g, b, light;

		r = color[0] * 255;
		g = color[1] * 255;
		b = color[2] * 255;
		light = (r + g + b) / 3;
		SATURATE(r,light,sat);
		SATURATE(g,light,sat);
		SATURATE(b,light,sat);
		color[0] = r / 255;
		color[1] = g / 255;
		color[2] = b / 255;
	}
}


void SaturateColor4b (color_t *c)
{
	float sat = r_saturation->value;
	if (sat != 1.0f)
	{
		float	r, g, b, light;

		r = c->c[0];
		g = c->c[1];
		b = c->c[2];
		light = (r + g + b) / 3;
		SATURATE(r,light,sat);
		SATURATE(g,light,sat);
		SATURATE(b,light,sat);
		c->c[0] = appRound (r);
		c->c[1] = appRound (g);
		c->c[2] = appRound (b);
	}
}
