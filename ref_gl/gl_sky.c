#include "gl_local.h"
#include "gl_sky.h"


static float skyMins[2][6], skyMaxs[2][6];


#define	ON_EPSILON		0.1f			// point on plane side epsilon
#define	MAX_CLIP_VERTS	64

enum {SIDE_FRONT, SIDE_BACK, SIDE_ON};

static void ClipSkyPolygon (int numVerts, vec3_t verts, int stage)
{
	float	*norm;
	float	*vec;
	qboolean	front, back;
	float	dists[MAX_CLIP_VERTS];
	int		sides[MAX_CLIP_VERTS];
	vec3_t	newv[2][MAX_CLIP_VERTS];	// new polys
	int		newc[2];					// number of verts in new polys
	int		i;

	static vec3_t skyClip[6] = {
		{ 1, 1, 0},
		{ 1,-1, 0},
		{ 0,-1, 1},
		{ 0, 1, 1},
		{ 1, 0, 1},
		{-1, 0, 1}
	};

	static int vecToSt[6][3] = {		// s = [0]/[2], t = [1]/[2]
		{-2, 3, 1},
		{ 2, 3,-1},
		{ 1, 3, 2},
		{-1, 3,-2},
		{-2,-1, 3},
		{-2, 1,-3}
	};


	if (!numVerts) return;				// empty polygon

	if (numVerts > MAX_CLIP_VERTS - 2)
		Com_Error (ERR_DROP, "ClipSkyPolygon: MAX_CLIP_VERTS hit");

	if (stage == 6)
	{	// fully clipped -- update skymins/skymaxs
		int		axis;
		vec3_t	av, v;
		float	*vp;

		// decide which face it maps to
		VectorClear (v);
		for (i = 0, vp = verts; i < numVerts; i++, vp += 3)
			VectorAdd (vp, v, v);
		av[0] = fabs(v[0]);
		av[1] = fabs(v[1]);
		av[2] = fabs(v[2]);
		// Here: v = sum vector, av = abs(v)
		if (av[0] > av[1] && av[0] > av[2])
			axis = (v[0] < 0) ? 1 : 0;
		else if (av[1] > av[2] && av[1] > av[0])
			axis = (v[1] < 0) ? 3 : 2;
		else
			axis = (v[2] < 0) ? 5 : 4;

		// project new texture coords
		for (i = 0; i < numVerts; i++, verts += 3)
		{
			int		j;
			float	s, t, dv;

			j = vecToSt[axis][2];
			if (j < 0)	dv = -verts[-j - 1];
			else		dv = verts[j - 1];

			if (dv < 0.001f) continue;	// don't divide by zero

			j = vecToSt[axis][0];
			if (j < 0)	s = -verts[-j - 1] / dv;
			else		s = verts[j - 1] / dv;

			j = vecToSt[axis][1];
			if (j < 0)	t = -verts[-j - 1] / dv;
			else		t = verts[j - 1] / dv;

			EXPAND_BOUNDS(s, skyMins[0][axis], skyMaxs[0][axis]);
			EXPAND_BOUNDS(t, skyMins[1][axis], skyMaxs[1][axis]);
		}
		return;
	}

	front = back = false;
	norm = skyClip[stage];
	for (i = 0, vec = verts; i < numVerts; i++, vec += 3)
	{
		float	d;

		d = DotProduct (vec, norm);
		if (d > ON_EPSILON)
		{
			front = true;
			sides[i] = SIDE_FRONT;
		}
		else if (d < -ON_EPSILON)
		{
			back = true;
			sides[i] = SIDE_BACK;
		}
		else
			sides[i] = SIDE_ON;
		dists[i] = d;
	}

	if (!front || !back)
	{	// not clipped
		ClipSkyPolygon (numVerts, verts, stage + 1);
		return;
	}

	// clip it
	sides[i] = sides[0];
	dists[i] = dists[0];
	VectorCopy (verts, (verts + (i * 3)));
	newc[0] = newc[1] = 0;

	for (i = 0, vec = verts; i < numVerts; i++, vec += 3)
	{
		int		j;
		float	d;

		switch (sides[i])
		{
		case SIDE_FRONT:
			VectorCopy (vec, newv[0][newc[0]]);
			newc[0]++;	// cannot insert this into VectorCopy(), because it is a macro ...
			break;
		case SIDE_BACK:
			VectorCopy (vec, newv[1][newc[1]]);
			newc[1]++;
			break;
		case SIDE_ON:
			VectorCopy (vec, newv[0][newc[0]]);
			newc[0]++;
			VectorCopy (vec, newv[1][newc[1]]);
			newc[1]++;
			break;
		}

		if (sides[i] == SIDE_ON || sides[i+1] == SIDE_ON || sides[i+1] == sides[i])
			continue;		// line placed on one side of clip plane

		// line intersects clip plane: split line to 2 parts by adding new point
		d = dists[i] / (dists[i] - dists[i+1]);
		for (j = 0; j < 3; j++)
		{
			float	e;

			e = vec[j] + d * (vec[j + 3] - vec[j]);
			newv[0][newc[0]][j] = e;
			newv[1][newc[1]][j] = e;
		}
		newc[0]++;
		newc[1]++;
	}

	// process new polys
	if (newc[0]) ClipSkyPolygon (newc[0], newv[0][0], stage + 1);
	if (newc[1]) ClipSkyPolygon (newc[1], newv[1][0], stage + 1);
}


/*------------------- Interface ---------------------*/


#define SKY_TESS_SIZE	4
#define SKY_CELLS		(SKY_TESS_SIZE*2)
#define SKY_VERTS		(SKY_TESS_SIZE*2+1)

#if SKY_VERTS * SKY_VERTS > MAX_VERTEXES
#  error Not enough vertex buffer size: cannot tesselate sky surface
#endif


static byte skySideVisible[6];
static byte skyVis[6][SKY_CELLS*SKY_CELLS];


void GL_ClearSkyBox (void)
{
	memset (skyVis, 0, sizeof(skyVis));
	memset (skySideVisible, 0, sizeof(skySideVisible));
}


void GL_ShowWholeSky (void)
{
	memset (skyVis, 0xFF, sizeof(skyVis));
	memset (skySideVisible, 0xFF, sizeof(skySideVisible));
}


void GL_AddSkySurface (surfacePlanar_t *pl, vec3_t vieworg)
{
	vec3_t	verts[MAX_CLIP_VERTS];
	int		i, side;
	vertex_t *v;

	if (gl_fastsky->integer)
		return;

	// clear bounds for all sky box planes
	for (side = 0; side < 6; side++)
	{
		skyMins[0][side] = skyMins[1][side] = 999999;
		skyMaxs[0][side] = skyMaxs[1][side] = -999999;
	}
	// add verts to bounds
	for (i = 0, v = pl->verts; i < pl->numVerts; i++, v++)
		VectorSubtract (v->xyz, vieworg, verts[i]);
	ClipSkyPolygon (pl->numVerts, verts[0], 0);

	// analyse skyMins/skyMaxs, detect visible cells
	for (side = 0; side < 6; side++)
	{
		float	v, f;
		int		x, y, w, h, j, stride;
		byte	*ptr;

		if (skyMins[0][side] > skyMaxs[0][side]) continue;	// not appied to this side

		skySideVisible[side] = true;
		// get cell's "x" and "w"
		v = skyMins[0][side];
		for (f = -1, x = -1; x < SKY_CELLS; f += 1.0f / SKY_TESS_SIZE, x++)
			if (v < f) break;
		v = skyMaxs[0][side];
		for (w = x; w < SKY_CELLS; f += 1.0f / SKY_TESS_SIZE, w++)
			if (v < f) break;
		if (x < 0) x = 0;
		if (w == SKY_CELLS) w--;
		w = w - x + 1;
		// get cell's "y" and "h"
		v = skyMins[1][side];
		for (f = -1, y = -1; y < SKY_CELLS; f += 1.0f / SKY_TESS_SIZE, y++)
			if (v < f) break;
		v = skyMaxs[1][side];
		for (h = y; h < SKY_CELLS; f += 1.0f / SKY_TESS_SIZE, h++)
			if (v < f) break;
		if (y < 0) y = 0;
		if (h == SKY_CELLS) h--;
		h = h - y + 1;
		// fill skyVis rect (x, y, w, h)
		ptr = skyVis[side] + y * SKY_CELLS + x;
		stride = SKY_CELLS - w;
		for (i = 0; i < h; i++)
		{
			for (j = 0; j < w; j++)
				*ptr++ = 0xFF;
			ptr += stride;
		}
	}
}


/*------------------- Tesselation -------------------*/

// In: s, t in range [-1..1]; out: tex = {s,t}, vec
static int AddSkyVec (float s, float t, int axis, float scale, bufVertex_t **vec, bufTexCoord_t **tex)
{
	vec3_t		b;
	int			i;

	static int stToVec[6][3] = {	// 1 = s, 2 = t, 3 = zFar
		{ 3,-1, 2},
		{-3, 1, 2},
		{ 1, 3, 2},
		{-1,-3, 2},
		{-2,-1, 3},		// 0 degrees yaw, look straight up
		{ 2,-1,-3}		// look straight down
	};

	b[0] = s * scale;
	b[1] = t * scale;
	b[2] = scale;

	for (i = 0; i < 3; i++)
	{
		int		tmp;

		tmp = stToVec[axis][i];
		(*vec)->xyz[i] = tmp < 0 ? -b[-tmp - 1] : b[tmp - 1];
	}
	(*vec)++;

	// convert range [-1, 1] to [0, 1]
	s = (s + 1) / 2;
	s = bound(s, 0, 1);
	t = (t + 1) / 2;
	t = bound(t, 0, 1);
#if 0
	if (!currentShader->bad && Cvar_VariableInt("skyfix"))
	{
		float	fix, fix2;

		fix = 0.5f / currentShader->width * Cvar_VariableInt("skyfix");
		fix2 = 1 - 2 * fix;
		s = fix + fix2 * s;
		t = fix + fix2 * t;
	}
#endif
	(*tex)->tex[0] = s;
	(*tex)->tex[1] = 1.0f - t;
	(*tex)++;

	return gl_numVerts++;
}


int GL_TesselateSkySide (int side, bufVertex_t *vec, bufTexCoord_t *tex, float zFar)
{
	float	s, t, scale;
	int		numIndexes;
	int		grid[SKY_VERTS*SKY_VERTS];
	int		*idx, *grid1, *grid2;
	byte	*ptr;

#if 0
	DrawTextLeft(va("side %d:", side),1,1,1);
	for (numIndexes = 0; numIndexes < SKY_CELLS; numIndexes++)
	{
		byte	*p;

		p = skyVis[side] + numIndexes * SKY_CELLS;
#define C(x) p[x] ? 'X' : ' '
		DrawTextLeft(va("[ %c %c %c %c %c %c %c %c ]", C(0),C(1),C(2),C(3),C(4),C(5),C(6),C(7)), 1, 0.5, 0.5);
#undef C
		p += SKY_CELLS;
	}
#endif

	if (!skySideVisible[side]) return 0;

	// generate side vertexes
	gl_numVerts = 0;
	scale = zFar / 3;				// any non-zero value not works on TNT2 (but works with GeForce2)
	memset (grid, 0, sizeof(grid));	// "0" is valid, because index=0 only for upper-left vertex, which is used only for 1 cell ...
	ptr = skyVis[side];
	grid1 = grid;
	grid2 = grid + SKY_VERTS;
	idx = gl_indexesArray;
	numIndexes = 0;
	for (t = -1; t < 1; t += 1.0f / SKY_TESS_SIZE)
	{
		for (s = -1; s < 1; s += 1.0f / SKY_TESS_SIZE, ptr++, grid1++, grid2++)
		{
			if (!*ptr) continue;		// this cell is not visible
			// this 2 verts can be filled by previous line
			if (!grid1[0])	grid1[0] = AddSkyVec (s, t, side, scale, &vec, &tex);
			if (!grid1[1])	grid1[1] = AddSkyVec (s + 1.0f / SKY_TESS_SIZE, t, side, scale, &vec, &tex);
			// this vertex can be filled by previous cell
			if (!grid2[0])	grid2[0] = AddSkyVec (s, t + 1.0f / SKY_TESS_SIZE, side, scale, &vec, &tex);
			// this vertex cannot be filled by previous cells
			grid2[1] = AddSkyVec (s + 1.0f / SKY_TESS_SIZE, t + 1.0f / SKY_TESS_SIZE, side, scale, &vec, &tex);
			// generate indexes
			// g1(1) ----- g1+1(2)
			//  |           |
			//  |           |
			// g2(3) ----- g2+1(4)
			*idx++ = grid1[0]; *idx++ = grid2[0]; *idx++ = grid1[1];	// triangle 1 (1-3-2)
			*idx++ = grid2[0]; *idx++ = grid2[1]; *idx++ = grid1[1];	// triangle 2 (3-4-2)
			numIndexes += 6;
		}
		// sky verts are not wrapped -- skip seam
		grid1++;
		grid2++;
	}
//	DrawTextLeft(va("side %d:  %d verts  %d idx", side, gl_numVerts, numIndexes),1,1,1);

	return numIndexes;
}
