#include "gl_local.h"
#include "gl_backend.h"
#include "gl_math.h"


/* NOTES:
 *  - we have used lightstyle 0 for sun and surface light
 *  - !! cached only style 0, in prescaled by style form; if style was changed before
 *    requesting light from grid, we will receive OLD light value (whould scale AFTER
 *    taking light from grid)
 *  - ambient light not scaled !
 *  - lightstyles used in a way, which provides double scale of computed light
 *    (which is used for model lighting src*color, allows double lighting)
 *  - now XXXX_SCALE==2 and lightstyle_scale==0.5
 *  - !! but color stored in a byte form (clamped before), which restricts us in max
 *    double-light == 255 (1.0f -- keep texture lights, no brighten texture!) even when
 *    overbright!=0 or modulate2x available
 *    (should store double-light * identityLight clamped by 255, and scale it after requesting
 *    255 -> 2.0, 128 -> 1.0; if lighting cannot be performed src*light*2 -- modulate by 2 HERE;
 *    - divide XXXX_SCALE by 2 (+ambient!) -OR- "lightStyles[i].value/255"->lightStyles[i]/512
 *      (needs ambient to be scaled by style 0!), modulate light in GL_ApplyEntitySpherelights()
 *      by 2 when needed (add arg to function: "bool scale")
 */


//#define TEST 1				//!!!!! REMOVE THIS
static cvar_t *test;		//!!!!!

static vec3_t entityColorAxis[6];
// array layout: back/front/right/left/bottom/top


static void LightLine (vec3_t *axis, vec3_t from, vec3_t to, float *color, float lightScale)
{
	gl_depthMode_t prevDepth;
	int		i, r, g, b;
	color_t	c;

	if (lightScale < gl_lightLines->value) return;
	if (vp.flags & RDF_NOWORLDMODEL) return;

	prevDepth = gl_state.currentDepthMode;


	glPushMatrix ();
	glLoadMatrixf (&vp.modelMatrix[0][0]);
	GL_SetMultitexture (0);		// disable texturing
	GL_State (GLSTATE_POLYGON_LINE|GLSTATE_DEPTHWRITE);
	GL_DepthRange (DEPTH_NEAR);

	r = Q_round (color[0] * lightScale);
	g = Q_round (color[1] * lightScale);
	b = Q_round (color[2] * lightScale);
	NORMALIZE_COLOR255(r, g, b);
	c.c[0] = r; c.c[1] = g; c.c[2] = b; c.c[3] = 255;
	glColor4ubv (c.c);
	glDisableClientState (GL_COLOR_ARRAY);
//	GL_DisableTexCoordArrays ();

	glBegin (GL_LINES);
	glVertex3fv (from);
	glVertex3fv (to);
	for (i = 0; i < 3; i++)
	{
		vec3_t	v;

		VectorMA (from, 10, axis[i], v);
		glVertex3fv (v);
		VectorMA (from, -10, axis[i], v);
		glVertex3fv (v);
	}
	glEnd ();

	// restore state
	glPopMatrix ();
	GL_DepthRange (prevDepth);
}


#define DEBUG_POINT_SIZE	16
#define DEBUG_SPOT_SIZE		40

void GL_ShowLights (void)
{
	gl_slight_t	*sl;
	surfLight_t *rl;
	int		i, j;

	if (vp.flags & RDF_NOWORLDMODEL) return;

	glPushMatrix ();
	glLoadMatrixf (&vp.modelMatrix[0][0]);
	GL_SetMultitexture (0);		// disable texturing
	GL_State (GLSTATE_POLYGON_LINE|GLSTATE_DEPTHWRITE);
	glDisableClientState (GL_COLOR_ARRAY);

	for (i = 0, sl = map.slights; i < map.numSlights; i++, sl++)
	{
		glColor3fv (sl->color);
		if (sl->spot)
		{
			bufVertex_t	vecs[5];
			vec3_t	forward, right, up;
			float	dot, scale;
			static int	indexes[16] = {0, 1, 0, 2, 0, 3, 0, 4, 1, 2, 2, 3, 3, 4, 4, 1};

			if (sl->focus != 1)
			{
				dot = (sl->focus - 1) / sl->focus;
				if (dot < sl->spotDot) dot = sl->spotDot;
			}
			else
				dot = sl->spotDot;

			scale = sqrt (1.0f - dot * dot) * DEBUG_SPOT_SIZE;
			dot *= DEBUG_SPOT_SIZE;
			MakeNormalVectors (sl->spotDir, right, up);
			VectorMA (sl->origin, dot, sl->spotDir, forward);
			for (j = 0; j < 4; j++)
				VectorMA (forward, j & 2 ? scale : -scale, j & 1 ? up : right, vecs[j + 1].xyz);
			VectorCopy (sl->origin, vecs[0].xyz);
			glVertexPointer (3, GL_FLOAT, sizeof(bufVertex_t), vecs);
			glDrawElements (GL_LINES, 16, GL_UNSIGNED_INT, indexes);
		}
		else
		{
			vec3_t	v;

			glBegin (GL_LINES);
			for (j = 0; j < 3; j++)
			{
				VectorCopy (sl->origin, v);
				v[j] -= DEBUG_POINT_SIZE/2;
				glVertex3fv (v);
				v[j] += DEBUG_POINT_SIZE;
				glVertex3fv (v);
			}
			glEnd ();
		}
		if (gl_showLights->integer == 2)
			DrawText3D (sl->origin, va("%g", sl->intens), VECTOR_ARG(sl->color));
	}

	for (i = 0, rl = map.surfLights; i < map.numSurfLights; i++, rl = rl->next)
	{
		int		j;
		bufVertex_t	vecs[4];
		static int	indexes[8] = {0, 1, 1, 3, 3, 2, 2, 0};

		glColor3fv (rl->color);

		for (j = 0; j < 4; j++)
		{
			float	v[2];
			vec3_t	tmp;

			v[0] = j & 1 ? rl->pl->mins2[0] : rl->pl->maxs2[0];
			v[1] = j & 2 ? rl->pl->mins2[1] : rl->pl->maxs2[1];
			VectorScale (rl->pl->axis[0], v[0], tmp);
			VectorMA (tmp, v[1], rl->pl->axis[1], tmp);
			VectorMA (tmp, rl->pl->plane.dist + 1, rl->pl->plane.normal, vecs[j].xyz);
		}

		glVertexPointer (3, GL_FLOAT, sizeof(bufVertex_t), vecs);
		glDrawElements (GL_LINES, 8, GL_UNSIGNED_INT, indexes);
	}

	//?? show dlights

	glPopMatrix ();
}


static void AddLight (vec3_t *axis, vec3_t dir, float scale, vec3_t color)
{
	float	v;

#ifdef TEST	//!! del
	if (test->integer & 1) return;//!!
#endif
#if 0
	// light for sphere (will not work for sunlight! need correct dist); NOT WORKS NOW AT ALL
	{
		vec3_t	dir2;
		float	rad;

		rad = ent->radius / dist;
#define STEP(n)									\
	VectorMA (dir, n&1 ? rad : -rad, axis[n>>1], dir2);	\
	VectorNormalizeFast(dir2);					\
	v = DotProduct (dir2, axis[n>>1]) * scale;	\
	if (n&1) v = -v;							\
	if (v > 0)	VectorMA (entityColorAxis[n], v, color, entityColorAxis[n]);
		STEP(0); STEP(1); STEP(2); STEP(3); STEP(4); STEP(5);
#undef STEP
	}

#else

	// light for point
#define STEP(n)								\
	v = DotProduct (dir, axis[n]) * scale;	\
	if (v < 0)	VectorMA (entityColorAxis[n*2+1], -v, color, entityColorAxis[n*2+1]);	\
	else		VectorMA (entityColorAxis[n * 2],  v, color, entityColorAxis[n * 2]);
	STEP(0); STEP(1); STEP(2);
#undef STEP

#endif
}


#define LINEAR_SCALE	2.0f	// */ Cvar_VariableValue("lscale")
#define INV_SCALE		4.0f	// ??2.0f  */ Cvar_VariableValue("iscale")
#define SURF_SCALE		2.0f	// */ Cvar_VariableValue("sscale")
#define SUN_SCALE		2.0f


static void AddPointLight (gl_slight_t *sl, vec3_t origin, vec3_t *axis, byte *vis)
{
	float	dist, scale;
	vec3_t	dif;
	float	linearScale, invScale;
	int		br;

	if (vis && sl->cluster >= 0)
	{
		if (!(vis[sl->cluster>>3] & (1<<(sl->cluster&7))))
			return;										// light is culled with PVS
	}

	VectorSubtract (origin, sl->origin, dif);
	dist = VectorNormalizeFast (dif);

	linearScale = LINEAR_SCALE;
	invScale = INV_SCALE;
	if (sl->spot)
	{
		float	dot2;

		dot2 = DotProduct (dif, sl->spotDir);
		if (dot2 <= sl->spotDot) return;				// outside cone
		if (sl->focus != 1)
		{
			dot2 = dot2 * sl->focus - (sl->focus - 1);
			if (dot2 <= 0) return;
			linearScale = dot2 * LINEAR_SCALE;
			invScale = dot2 * INV_SCALE;
		}
	}

	switch (sl->type)
	{
	case sl_linear:
		scale = (sl->intens - dist * sl->fade) * linearScale;
//if (sl->spot) DrawTextLeft(va("org: %g %g %g -> obj: %g %g %g dist: %g int: %g scale: %g fade: %g",
//VECTOR_ARG(sl->origin),VECTOR_ARG(origin),dist,sl->intens,scale,sl->fade),1,1,1);
		break;
	case sl_inverse:
		if (dist < 16) dist = 16;
		scale = sl->intens / dist * invScale;
		break;
	case sl_inverse2:
		scale = sl->intens / (dist * dist);
		DrawTextLeft ("inv2 slight",1, 0, 0);//!!
		break;
	default:
		DrawTextLeft ("unknown point sl.type", 1, 0, 0);
	}

	scale = scale * vp.lightStyles[sl->style].value / 255.0f;		// 0--0.0, 128--1.0, 256--2.0
//if (sl->spot) DrawTextLeft(va("  scale=%g",scale),1,1,0);
	if (scale < 1) return;							// "scale" will convert 0..1 range to 0..255

#ifdef TEST //!! del
	if (!(test->integer & 2))
		CM_BrushTrace (sl->origin, origin, &br, 1);
#else
	if (CM_BrushTrace (sl->origin, origin, &br, 1)) return;
#endif

	AddLight (axis, dif, scale, sl->color);
	if (gl_lightLines->value)
		LightLine (axis, sl->origin, origin, sl->color, scale);
}


static qboolean needSunAmbient;

static void AddSurfaceLight (surfLight_t *rl, vec3_t origin, vec3_t *axis, byte *vis)
{
	surfacePlanar_t *pl;
	float	dist, distN, intens, x, y, dx, dy;
	float	w, h, w0, h0;
	qboolean slope, ambient;
	trace_t	tr;
	vec3_t	dir, dst;
	int		br;

	if (vis && rl->cluster >= 0)
	{
		if (!(vis[rl->cluster>>3] & (1<<(rl->cluster&7))))
			return;										// light is culled with PVS
	}

#ifdef TEST //!!!
if (test->integer & 64) return;
#endif
	ambient = rl->sky && needSunAmbient;				// sun ambient requirement
	if (!rl->intens && !ambient) return;				// ambient-only surface

	pl = rl->pl;
	distN = DISTANCE_TO_PLANE(origin, &pl->plane);
	if (distN < 0.001f) return;							// backface culled

	x = DotProduct (origin, pl->axis[0]);
	y = DotProduct (origin, pl->axis[1]);
	slope = false;
	dx = dy = 0;
	if (x < pl->mins2[0])
	{
		slope = true;
		dx = x - pl->mins2[0];
	}
	else if (x > pl->maxs2[0])
	{
		slope = true;
		dx = x - pl->maxs2[0];
	}
	if (y < pl->mins2[1])
	{
		slope = true;
		dy = y - pl->mins2[1];
	}
	else if (y > pl->maxs2[1])
	{
		slope = true;
		dy = y - pl->maxs2[1];
	}

#ifdef TEST //!!!
if (test->integer & 32) return;
#endif
	if (slope)
	{
		VectorScale (pl->axis[0], dx, dir);
		VectorMA (dir, dy, pl->axis[1], dir);
		VectorMA (dir, distN, pl->plane.normal, dir);
		dist = VectorNormalizeFast (dir);
		intens = rl->intens * distN / (dist * dist * dist) * SURF_SCALE;
	}
	else
	{
		VectorCopy (pl->plane.normal, dir);
		dist = distN;
		intens = rl->intens / (distN * distN) * SURF_SCALE;
	}
#ifdef TEST //!!!
if (test->integer & 16) return;
#endif

	intens *= vp.lightStyles[0].value / 255.0f;		// surface lights have style=0
	if (intens < 1 && !ambient) return;

	w0 = pl->maxs2[0] - pl->mins2[0];
	h0 = pl->maxs2[1] - pl->mins2[1];
	w = min(w0, dist);
	h = min(h0, dist);
	if (w != w0 || h != h0)
	{
		intens *= w * h / (w0 * h0);
		if (intens < 1 && !ambient) return;
	}

	VectorMA (origin, -dist + 1, dir, dst);				// need to shift in light direction because of trace bugs with non-axial planes
#ifdef TEST	//!!
	if (!(test->integer & 2))
		CM_BoxTrace (&tr, dst, origin, vec3_origin, vec3_origin, 0, CONTENTS_SOLID);
#else
	CM_BoxTrace (&tr, dst, origin, vec3_origin, vec3_origin, 0, CONTENTS_SOLID);
	if (tr.fraction < 1) return;
#endif

	if (tr.startsolid)		//?? may be, light source is placed in niche -- check the center of surface
	{
		if (intens < 10 || !slope) return;

		//?? can optimize by checking niche while loading; or -- trace (size/distance < threshold) lights into center
		// try to trace into surface center
		x = (pl->mins2[0] + pl->maxs2[0]) / 2;
		y = (pl->mins2[1] + pl->maxs2[1]) / 2;
		VectorScale (pl->axis[0], x, dst);
		VectorMA (dst, y, pl->axis[1], dst);
		VectorMA (dst, pl->plane.dist + 1, pl->plane.normal, dst);
#ifdef TEST	//!!
		if (!(test->integer & 2))
			CM_BrushTrace (dst, origin, &br, 1);
#else
		if (CM_BrushTrace (dst, origin, &br, 1)) return;
#endif
	}

	if (ambient)
	{
		int		i;
		vec3_t	c;
		float	scale;

		scale = vp.lightStyles[0].value / 255.0f;
		c[0] = map.sunColor[0] * map.sunAmbient[0] * scale;
		c[1] = map.sunColor[1] * map.sunAmbient[1] * scale;
		c[2] = map.sunColor[2] * map.sunAmbient[2] * scale;
		for (i = 0; i < 6; i++)
			VectorAdd (entityColorAxis[i], c, entityColorAxis[i]);
		needSunAmbient = false;
	}

	if (!intens) return;				// ambient-only surface
	AddLight (axis, dir, intens, rl->color);
	if (gl_lightLines->value && intens > gl_lightLines->value)
		LightLine (axis, dst, origin, rl->color, intens);
}


static lightCell_t darkCell, outCell;	// zero-initialized

#define NORMALIZE_AXIS

static qboolean GetCellLight (vec3_t origin, int *coord, refEntity_t *ent)
{
	lightCell_t	**pcell, *cell;
	gl_slight_t *sl;
	surfLight_t *rl;
	trace_t	tr;
	int		i;
	byte	*row;
	float	*out, scale;
	qboolean hasLight;
	vec3_t	*axis;
	static vec3_t gridAxis[3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};

	if (!ent)
	{
		if (coord[0] < 0 || coord[0] >= map.mapGrid[0] ||
			coord[1] < 0 || coord[1] >= map.mapGrid[1] ||
			coord[2] < 0 ||	coord[2] >= map.mapGrid[2])
		{	// requested cell is outside the grid bounds
			memset (entityColorAxis, 0, sizeof(entityColorAxis));
			return false;
		}

		pcell = map.lightGrid + coord[0] + (coord[1] * map.mapGrid[0]) + (coord[2] * map.mapGrid[0] * map.mapGrid[1]);
		if (cell = *pcell)
		{	// get cached light
			byte	*src;

			for (i = 0, src = cell->c[0], out = entityColorAxis[0]; i < 6*3; i++, src++, out++)
				*out = *src;
			return cell != &outCell;
		}
		axis = gridAxis;
	}
	else
	{
		pcell = NULL;
		origin = ent->center;
		axis = ent->axis;
	}

	i = GL_PointInLeaf (origin)->cluster;
	row = i < 0 ? NULL : map.visInfo + i * map.visRowSize;

	scale = vp.lightStyles[0].value / 512.0f;
	for (i = 0; i < 6; i++)
		VectorScale (map.ambientLight, scale, entityColorAxis[i]);

	if (!row && map.visInfo)
	{
		if (pcell) *pcell = &outCell;
		return false;		// point is outside the world
	}

	/*------------------ directed sunlight ------------------*/

	if (map.sunLight)
	{
		vec3_t	dst;
		static vec3_t zero = {0, 0, 0};

		VectorMA (origin, -16384, map.sunVec, dst);
		CM_BoxTrace (&tr, origin, dst, zero, zero, 0, CONTENTS_SOLID);
		if (tr.surface->flags & SURF_SKY && !tr.startsolid)		// can be "startsolid" even if "row"!=NULL
		{
			float	intens;

			intens = map.sunLight * SUN_SCALE * vp.lightStyles[0].value / 255.0f;	// sun light have style=0
			AddLight (axis, map.sunVec, map.sunLight, map.sunColor);
			if (gl_lightLines->value)
				LightLine (axis, tr.endpos, origin, map.sunColor, map.sunLight);
		}
	}

	/*--------------- point/surface lights ------------------*/

	for (i = 0, sl = map.slights; i < map.numSlights; i++, sl++)
		if (ent || sl->style == 0)
			AddPointLight (sl, origin, axis, row);

	needSunAmbient = map.haveSunAmbient;
	for (i = 0, rl = map.surfLights; i < map.numSurfLights; i++, rl = rl->next)
		AddSurfaceLight (rl, origin, axis, row);

	if (ent) return true;

	// store computed light

	hasLight = false;
	for (i = 0, out = entityColorAxis[0]; i < 6 * 3; i++, out++)
		if (*out)
		{	// have a non-zero color
			hasLight = true;
			break;
		}

	if (hasLight)
	{
		byte	*dst;
		float	m;

		// alloc new cell
		cell = AllocChainBlock (map.lightGridChain, sizeof(lightCell_t));
		*pcell = cell;
		map.numLightCells++;

#ifdef NORMALIZE_AXIS
		// find maximal color
		m = 0;
		for (i = 0, out = entityColorAxis[0]; i < 6*3; i++, out++)
			if (*out > m) m = *out;
		// normalize color axis and copy to grid
		if (m > 255)
			m = 255.0f / m;
		else
			m = 1;
		for (i = 0, out = entityColorAxis[0], dst = cell->c[0]; i < 6*3; i++, out++, dst++)
			*dst = Q_round (*out * m);
#else
		for (i = 0, out = entityColorAxis[0], dst = cell->c[0]; i < 6; i++)
		{
			int		r, g, b;

			r = Q_round (*out++);
			g = Q_round (*out++);
			b = Q_round (*out++);
			NORMALIZE_COLOR255(r, g, b);
			*dst++ = r;
			*dst++ = g;
			*dst++ = b;
		}
#endif
	}
	else
		*pcell = &darkCell;

	return true;
}


void GL_LightForEntity (refEntity_t *ent)
{
	int		i;
	gl_slight_t *sl;
	refDlight_t	*dl;
	float	scale;
	gl_depthMode_t prevDepth;

#ifdef TEST	//!!!
	if (!test) test = Cvar_Get("test","0",0);
	if (test->integer & 8) return;
#endif
	// NOTE: sl.color is [0..1], out color is [0..255]

	memset (entityColorAxis, 0, sizeof(entityColorAxis));

	if (!(vp.flags & RDF_NOWORLDMODEL))
	{
		if (!gl_nogrid->integer)
		{
			byte	*row;
			vec3_t	accum[6];
			vec3_t	pos, frac;
			int		coord[3];
			float	totalFrac;

			i = GL_PointInLeaf (ent->center)->cluster;
			row = i < 0 ? NULL : map.visInfo + i * map.visRowSize;

			for (i = 0; i < 3; i++)
			{
				coord[i] = Q_floor (ent->center[i] / LIGHTGRID_STEP);
				pos[i] = coord[i] * LIGHTGRID_STEP;
				frac[i] = (ent->center[i] - pos[i]) / LIGHTGRID_STEP;
				coord[i] -= map.gridMins[i];
			}

			if (gl_showgrid->integer)
			{
//				DrawTextLeft (va("pos: %g %g %g frac: %g %g %g", VECTOR_ARG(pos), VECTOR_ARG(frac)), 1, 1, 1);
				prevDepth = gl_state.currentDepthMode;
				GL_DepthRange (DEPTH_NEAR);
				glPushMatrix ();
				glLoadMatrixf (&vp.modelMatrix[0][0]);
				GL_SetMultitexture (0);		// disable texturing
				GL_State (GLSTATE_POLYGON_LINE|GLSTATE_DEPTHWRITE);
				glDisableClientState (GL_COLOR_ARRAY);
			}

			memset (accum, 0, sizeof(accum));
			totalFrac = 0;
			for (i = 0; i < 8; i++)
			{
				vec3_t	origin;
				int		c[3], j;
				float	f;

				f = 1;
				for (j = 0; j < 3; j++)
					if (i & (1<<j))
					{
						origin[j] = pos[j];
						c[j] = coord[j];
						f *= 1 - frac[j];
					}
					else
					{
						origin[j] = pos[j] + LIGHTGRID_STEP;
						c[j] = coord[j] + 1;
						f *= frac[j];
					}
				if (!f) continue;			// will not add light anyway
				if (GetCellLight (origin, c, NULL))
				{
					totalFrac += f;
					for (j = 0; j < 6; j++)
						VectorMA (accum[j], f, entityColorAxis[j], accum[j]);

					if (gl_showgrid->integer)
					{
						glBegin (GL_LINES);
						for (j = 0; j < 6; j++)
						{
							vec3_t	origin2;

							glColor3f (entityColorAxis[j][0]/255, entityColorAxis[j][1]/255, entityColorAxis[j][2]/255);
							glVertex3fv (origin);
							VectorCopy (origin, origin2);
							origin2[j >> 1] += (j & 1) ? 2 : -2;
							glVertex3fv (origin2);
						}
						glEnd ();
					}
				}
			}
			if (totalFrac != 1)
			{	// if some points have missed (outside the world) -- scale light from correct points
				totalFrac = 1.0f / totalFrac;
				for (i = 0; i < 6; i++) VectorScale (accum[i], totalFrac, accum[i]);
			}
			if (gl_showgrid->integer)
			{
				glPopMatrix ();
				GL_DepthRange (prevDepth);
			}

			memset (entityColorAxis, 0, sizeof(entityColorAxis));
			for (i = 0; i < 6; i++)
			{
				float	v;

#define STEP(n)						\
				v = ent->axis[n][i>>1];	\
				if (i & 1) v = -v;		\
				if (v < 0)	VectorMA (entityColorAxis[n*2+1], -v, accum[i], entityColorAxis[n*2+1]);	\
				else		VectorMA (entityColorAxis[n * 2],  v, accum[i], entityColorAxis[n * 2]);
				STEP(0); STEP(1); STEP(2);
#undef STEP
			}

			/*--------------- point/surface lights ------------------*/

			for (i = 0, sl = map.slights; i < map.numSlights; i++, sl++)
				if (sl->style != 0)
					AddPointLight (sl, ent->center, ent->axis, row);
		}
		else
			GetCellLight (NULL, NULL, ent);
	}

	/*---------------------- dlights ------------------------*/

	for (i = 0, dl = vp.dlights; i < vp.numDlights; i++, dl++)
	{
		vec3_t	dst, color;
		float	dist, denom;

		VectorSubtract (ent->center, dl->origin, dst);
		dist = DotProduct (dst, dst);						// dist*dist
		if (dist > dl->intensity * dl->intensity) continue;	// dlight is too far
		denom = Q_rsqrt (dist);								// 1/sqrt(dist)
		dist = 1.0f / denom;
		VectorScale (dst, denom, dst);

		scale = (dl->intensity - dist) * LINEAR_SCALE;
		denom = 1.0f / gl_config.identityLightValue;
		color[0] = dl->c.c[0] * denom;
		color[1] = dl->c.c[1] * denom;
		color[2] = dl->c.c[2] * denom;
		AddLight (ent->axis, dst, scale, color);
		if (gl_lightLines->value)
			LightLine (ent->axis, dl->origin, ent->center, color, scale);
	}

	if (ent->mirror)
	{
		vec3_t	tmp;

		// swap left and right lights
		VectorCopy (entityColorAxis[2], tmp);
		VectorCopy (entityColorAxis[3], entityColorAxis[2]);
		VectorCopy (tmp, entityColorAxis[3]);
	}

	if (ent->flags & RF_GLOW)
	{
		float	glow;

#define MAX_GLOW	0.2		// Cvar_VariableValue("glow");
		glow = (SIN_FUNC(vp.time / 1.5f) + 1) / 2 * MAX_GLOW;	// 0..MAX_GLOW
		// lerp colors between color and 255 with glow
		for (i = 0; i < 6; i++)
		{
			int		j;

			for (j = 0; j < 3; j++)
				entityColorAxis[i][j] = (1 - glow) * entityColorAxis[i][j] + glow * 255;
		}
	}

#ifdef NORMALIZE_AXIS
	{
		float	m, *out;

		// find maximal color
		m = 0;
		for (i = 0, out = entityColorAxis[0]; i < 6*3; i++, out++)
			if (*out > m) m = *out;
		// normalize color axis and copy to grid
		if (m > 255)
		{
			m = 255.0f / m;
			for (i = 0, out = entityColorAxis[0]; i < 6*3; i++, out++)
				*out *= m;
		}
	}
#else
	for (i = 0; i < 6; i++)
		ClampColor255 (entityColorAxis[i], entityColorAxis[i]);
#endif
}


//!! rename; + bool arg: "modulate by 2"
void GL_ApplyEntitySpherelights (color_t *dst)
{
	int		i, j;
	bufExtra_t *ex;
	float	light;

#ifdef TEST	//!!!
	if (test->integer & 4) return;
#endif
	light = gl_config.identityLightValue_f * 2;			// *2 -- because 1 == double light (similar to lightmaps)
	for (i = 0, ex = gl_extra; i < gl_numExtra; i++, ex++)
	{
		float	*norm, *axis, val;
		vec3_t	color;
		color_t	c;
		int		tmp;

		norm = ex->normal;

		VectorClear (color);
		// compute color
#if 1
#define STEP(n)		\
		FAbsSign(norm[n],val,tmp);	\
		val *= light;				\
		axis = entityColorAxis[n*2+1-tmp];
#else
#define STEP(n)		\
		axis = entityColorAxis[(norm[n] < 0 ? (n * 2) : (n * 2 + 1))];	\
		val = fabs (norm[n]) * light;
#endif

		STEP(0);
		VectorScale (axis, val, color);
		STEP(1);
		VectorMA (color, val, axis, color);
		STEP(2);
		VectorMA (color, val, axis, color);
#undef STEP

		// apply color
#define STEP(n)		\
		tmp = Q_round (color[n]);	\
		c.c[n] = (tmp > 255) ? 255 : tmp;
		STEP(0); STEP(1); STEP(2);
#undef STEP
		c.c[3] = 255;

		for (j = 0; j < ex->numVerts; j++, dst++)		// normally, here will be only 1 iteration ...
			dst->rgba = c.rgba;							// just put computed color
	}
}
