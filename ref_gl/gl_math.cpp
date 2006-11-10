#include "OpenGLDrv.h"
#include "gl_math.h"


namespace OpenGLDrv {


/*----- Tables for fast periodic function computation -----*/

float sinTable[TABLE_SIZE], squareTable[TABLE_SIZE], triangleTable[TABLE_SIZE], sawtoothTable[TABLE_SIZE];

const float *mathFuncs[] = {
	sinTable, squareTable, triangleTable, sawtoothTable
};

/*---------- Tables for non-periodic functions ------------*/

float asinTable[TABLE_SIZE*2], acosTable[TABLE_SIZE*2];
float atanTable[TABLE_SIZE], atanTable2[TABLE_SIZE];


//!! these tables are unused now
float sqrtTable[256];
int   noiseTablei[256];
float noiseTablef[256];


void InitFuncTables()
{
	int		i;

	for (i = 0; i < TABLE_SIZE; i++)
	{
		float j = i;			// just to avoid "(float)i"
		float f = j / TABLE_SIZE;
		squareTable[i] = (i < TABLE_SIZE/2) ? -1 : 1;
		sinTable[i] = sin(j / (TABLE_SIZE/2) * M_PI);		// 0 -- 0, last -- 2*pi
		sawtoothTable[i] = f;

		int n = (i + TABLE_SIZE/4) & TABLE_MASK;			// make range -1..1..-1
		if (n >= TABLE_SIZE/2)
			n = TABLE_SIZE - n;
		triangleTable[i] = (float)(n - TABLE_SIZE/4) / (TABLE_SIZE/4);

		asinTable[i] = asin(f - 1);
		asinTable[i + TABLE_SIZE] = asin(f);
		acosTable[i] = acos(f - 1);
		acosTable[i + TABLE_SIZE] = acos(f);
		atanTable[i] = atan(f);
		atanTable2[i] = atan(1.0f / f);
	}

	for (i = 0; i < 256; i++)
	{
		sqrtTable[i] = pow(i / 255.0f, 0.5f);
		noiseTablei[i] = appRound(rand() * 255.0f / RAND_MAX) & 0xFF;
		noiseTablef[i] = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;	// range -1..1
	}
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

void BuildRotationAxis(CAxis &r, const CVec3 &axis, float angle)
{
	CVec3	axisn;
#define x axisn[0]
#define y axisn[1]
#define z axisn[2]
	VectorNormalize(axis, axisn);
	float q = angle / 180 * M_PI;
	float sq = sin(q);
	float cq = cos(q); float ncq = 1 - cq;
	// compute uu'
	float xx = x * x; float xy = x * y; float xz = x * z;
	float yy = y * y; float yz = y * z; float zz = z * z;
	// compute R
	r[0][0] = xx + (1 - xx) * cq;    r[1][0] = xy * ncq - z * sq;    r[2][0] = xz * ncq + y * sq;
	r[0][1] = xy * ncq + z * sq;     r[1][1] = yy + (1 - yy) * cq;   r[2][1] = yz * ncq - x * sq;
	r[0][2] = xz * ncq - y * sq;     r[1][2] = yz * ncq + x * sq;    r[2][2] = zz + (1 - zz) * cq;
#undef x
#undef y
#undef z
}


// 3/12 + 4 * 3 = 15/24 dots (worldMatrix/non-worldMatrix)
// In: ent -- axis + center, center+size2 = maxs, center-size2 = mins
bool GetBoxRect(const refEntity_t *ent, const CVec3 &size2, float mins2[2], float maxs2[2], bool clamp)
{
	CAxis axis;
	if (!ent->worldMatrix)
	{
		// rotate screen axis
		for (int i = 0; i < 3; i++)
			ent->coord.axis.TransformVector(vp.view.axis[i], axis[i]);
	}
	else
		axis = vp.view.axis;

	// vp.view.TransformPoint(ent->center, {x0,y0,z0}) with fast z0 culling
	CVec3 tmp;
	VectorSubtract(ent->center, vp.view.origin, tmp);
	float z0 = dot(tmp, vp.view.axis[0]);				// get Z-coordinate
	if (z0 < 4) return false;							// too near to viewer (4==gl_znear)
	float x0 = dot(tmp, vp.view.axis[1]);
	float y0 = dot(tmp, vp.view.axis[2]);

	// {x0,y0,z0} - ent.center, projected to vp.view (3D projection of entity center to screen)
	// axis - vp.view.axis, rotated to entity coordinate system

	// ClearBounds2D(mins2, maxs2)
	mins2[0] = mins2[1] =  BIG_NUMBER;
	maxs2[0] = maxs2[1] = -BIG_NUMBER;

	// enumerate all box points (can try to optimize: find contour ??)
	CVec3 v;
	v[2] = size2[2];									// constant for all iterations
	// NOTE: in vertex iterations, we locked vector component 2, and
	// vary components 0 and 1; really, we can lock any component
	for (int i = 0; i < 4; i++)
	{
		v[0] = (i & 1) ? size2[0] : -size2[0];
		v[1] = (i & 2) ? size2[1] : -size2[1];

		// axis.TransformVector(v, {x,y,z}) with fast z culling
		float z = dot(v, axis[0]);						// get entity-relative Z-coordinate of vertex (size of entity along axis.Z)
		if ((z >= z0 || z <= -z0)) return false;		// |size| is greater than z0 (entity Z-coord) - box intersects screen
		float x = dot(v, axis[1]);						// entity-center-relative vertex position in 2D
		float y = dot(v, axis[2]);

		float	scale, x1, y1;
		// we use orthogonal projection of symmetric box - can use half of computations
		// we use computed point and diametrically opposite point (have negated
		// orthogonal-projected coords)
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
bool ProjectToScreen(const CVec3 &pos, int scr[2])
{
	CVec3	vec;
	VectorSubtract(pos, vp.view.origin, vec);

	float z = dot(vec, vp.view.axis[0]);
	if (z <= gl_znear->value) return false;			// not visible

	float x = dot(vec, vp.view.axis[1]) / z / vp.t_fov_x;
	if (x < -1 || x > 1) return false;

	float y = dot(vec, vp.view.axis[2]) / z / vp.t_fov_y;
	if (y < -1 || y > 1) return false;

	scr[0] = appRound(vp.x + vp.w * (0.5 - x / 2));
	scr[1] = appRound(vp.y + vp.h * (0.5 - y / 2));

	return true;
}


void SaturateColor3f(CVec3 &color)
{
	float sat = r_saturation->value;
	if (sat != 1.0f)
	{
		float r = color[0] * 255;
		float g = color[1] * 255;
		float b = color[2] * 255;
		float light = (r + g + b) / 3;
		SATURATE(r,light,sat);
		SATURATE(g,light,sat);
		SATURATE(b,light,sat);
		color[0] = r / 255;
		color[1] = g / 255;
		color[2] = b / 255;
	}
}


void SaturateColor4b(color_t *c)
{
	float sat = r_saturation->value;
	if (sat != 1.0f)
	{
		float r = c->c[0];
		float g = c->c[1];
		float b = c->c[2];
		float light = (r + g + b) / 3;
		SATURATE(r,light,sat);
		SATURATE(g,light,sat);
		SATURATE(b,light,sat);
		c->c[0] = appRound(r);
		c->c[1] = appRound(g);
		c->c[2] = appRound(b);
	}
}


} // namespace
