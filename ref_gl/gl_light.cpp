#include "OpenGLDrv.h"
#include "gl_light.h"
#include "gl_backend.h"
#include "gl_math.h"
#include "protocol.h"		//!! for RF_XXX consts only !


namespace OpenGLDrv {

//?? TODO: sun light, ambient -> separate CLight instances ?

/* NOTES:
 *  - we have used lightstyle 0 for sun, ambient and surface light
 */

#define MIN_POINT_LIGHT		2
#define MIN_SURF_LIGHT		2


#define SUN_SCALE			1.0f	// */ Cvar_VariableValue("sscale") // directional light from sun
#define SUN_AMBIENT_SCALE	0.75f	// */ Cvar_VariableValue("ascale") // ambient from sky surfaces
#define AMBIENT_SCALE		0.25f	// */ Cvar_VariableValue("ascale") // global ambient


#define CACHE_LIGHT_SCALE	2


static CVec3 entityColorAxis[6];
// contain color values for box
// array layout: back/front/right/left/bottom/top

static void AddLight (const CAxis &axis, const CVec3 &dir, float scale, const CVec3 &color)
{
	float	v;
	CVec3	*vec;

#if 0
	// light for sphere (will not work for sunlight! need correct dist); NOT WORKS NOW AT ALL
	CVec3	dir2;

	float rad = ent->radius / dist;
#define STEP(n)							\
	VectorMA (dir, n&1 ? rad : -rad, axis[n>>1], dir2);	\
	dir2.NormalizeFast()				\
	v = dot (dir2, axis[n>>1]) * scale;	\
	if (n&1) v = -v;					\
	if (!IsNegative(v))	VectorMA (entityColorAxis[n], v, color);
		STEP(0); STEP(1); STEP(2); STEP(3); STEP(4); STEP(5);
#undef STEP

#else

	// light for point
#if 1
	int		i;
#define STEP(n)						\
	v = dot(dir, axis[n]) * scale;	\
	FAbsSign(v,v,i);				\
	vec = &entityColorAxis[n*2+i];	\
	VectorMA (*vec, v, color);
#else
#define STEP(n)						\
	v = dot(dir, axis[n]) * scale;	\
	if (v < 0)	VectorMA (entityColorAxis[n*2+1], -v, color); \
	else		VectorMA (entityColorAxis[n * 2],  v, color);
#endif
	STEP(0); STEP(1); STEP(2);
#undef STEP

#endif
}


/*-----------------------------------------------------------------------------
	Debugging lights
-----------------------------------------------------------------------------*/

#if LIGHT_DEBUG

static void LightLine (const CAxis &axis, const CVec3 &from, const CVec3 &to, const CVec3 &color, float lightScale)
{
	if (lightScale < gl_lightLines->value) return;
	if (vp.flags & RDF_NOWORLDMODEL) return;

	gl_depthMode_t prevDepth = gl_state.currentDepthMode;

	glPushMatrix ();
	glLoadMatrixf (&vp.modelMatrix[0][0]);
	GL_SetMultitexture (0);		// disable texturing
	GL_State (GLSTATE_POLYGON_LINE|GLSTATE_DEPTHWRITE);
	GL_DepthRange (DEPTH_NEAR);

	int r = appRound (color[0] * lightScale);
	int g = appRound (color[1] * lightScale);
	int b = appRound (color[2] * lightScale);
	NORMALIZE_COLOR255(r, g, b);

	color_t	c;
	c.c[0] = r; c.c[1] = g; c.c[2] = b; c.c[3] = 255;
	glColor4ubv (c.c);
	glDisableClientState (GL_COLOR_ARRAY);
//	GL_DisableTexCoordArrays ();

	glBegin (GL_LINES);
	glVertex3fv (from.v);
	glVertex3fv (to.v);
	for (int i = 0; i < 3; i++)
	{
		CVec3 v;
		VectorMA (from, 10, axis[i], v);
		glVertex3fv (v.v);
		VectorMA (from, -10, axis[i], v);
		glVertex3fv (v.v);
	}
	glEnd ();

	// restore state
	glPopMatrix ();
	GL_DepthRange (prevDepth);
}


#define DEBUG_POINT_SIZE	16
#define DEBUG_SPOT_SIZE		40

void CPointLight::Show () const
{
	glColor3fv (color.v);
	if (spot)
	{
		static const int indexes[16] = {0, 1, 0, 2, 0, 3, 0, 4, 1, 2, 2, 3, 3, 4, 4, 1};

		float dot;
		if (focus != 1)
		{
			dot = (focus - 1) / focus;
			if (dot < spotDot) dot = spotDot;
		}
		else
			dot = spotDot;

		float scale = SQRTFAST(1.0f - dot * dot) * DEBUG_SPOT_SIZE;
		dot *= DEBUG_SPOT_SIZE;
		CVec3 forward, right, up;
		MakeNormalVectors (spotDir, right, up);
		VectorMA (origin, dot, spotDir, forward);
		bufVertex_t	vecs[5];
		for (int i = 0; i < 4; i++)
			VectorMA (forward, i & 2 ? scale : -scale, i & 1 ? up : right, vecs[i + 1].xyz);
		vecs[0].xyz = origin;
		glVertexPointer (3, GL_FLOAT, sizeof(bufVertex_t), vecs);
		glDrawElements (GL_LINES, 16, GL_UNSIGNED_INT, indexes);
	}
	else
	{
		CVec3 v;
		glBegin (GL_LINES);
		for (int i = 0; i < 3; i++)
		{
			v = origin;
			v[i] -= DEBUG_POINT_SIZE/2;
			glVertex3fv (v.v);
			v[i] += DEBUG_POINT_SIZE;
			glVertex3fv (v.v);
		}
		glEnd ();
	}
	if (gl_showLights->integer == 2)
		DrawText3D (origin, va("%g\nfall:%s", intens, GetName()), RGBS(color[0], color[1], color[2]));
}


void CSurfLight::Show () const
{
	bufVertex_t	vecs[4];
	static const int indexes[8] = {0, 1, 1, 3, 3, 2, 2, 0};

	glColor3fv (color.v);

	for (int i = 0; i < 4; i++)
	{
		float v[2];
		v[0] = i & 1 ? pl->mins2[0] : pl->maxs2[0];
		v[1] = i & 2 ? pl->mins2[1] : pl->maxs2[1];
		CVec3 tmp;
		VectorScale (pl->axis[0], v[0], tmp);
		VectorMA (tmp, v[1], pl->axis[1], tmp);
		VectorMA (tmp, pl->plane.dist + 1, pl->plane.normal, vecs[i].xyz);
	}

	glVertexPointer (3, GL_FLOAT, sizeof(bufVertex_t), vecs);
	glDrawElements (GL_LINES, 8, GL_UNSIGNED_INT, indexes);

	if (gl_showLights->integer == 2)
		DrawText3D (center, va("%g\n%g", intens, SQRTFAST(maxDist2)), RGBS(color[0], color[1], color[2]));
}


void ShowLights ()
{
	if (vp.flags & RDF_NOWORLDMODEL) return;

	glPushMatrix ();
	glLoadMatrixf (&vp.modelMatrix[0][0]);
	GL_SetMultitexture (0);		// disable texturing
	GL_State (GLSTATE_POLYGON_LINE|GLSTATE_DEPTHWRITE);
	glDisableClientState (GL_COLOR_ARRAY);

	const CLight *light;
	for (light = map.lights; light; light = light->next)
		light->Show ();
	for (light = map.flashLights; light; light = light->next)
		light->Show ();

	//?? show dlights

	glPopMatrix ();
}

#endif // LIGHT_DEBUG


/*-----------------------------------------------------------------------------
	Point lights
-----------------------------------------------------------------------------*/

#define LINEAR_SCALE		1.0f	// */ Cvar_VariableValue("lscale")
#define INV_SCALE			1.0f	// */ Cvar_VariableValue("iscale")

void CPointLight::Init ()
{
	cluster = PointInLeaf (origin)->cluster;
}

void CLightLinear::Init ()
{
	CPointLight::Init ();
	if (fade < 0.01f) fade = 0.01f;
	float f = (intens - MIN_POINT_LIGHT / LINEAR_SCALE) / fade;
	maxDist2 = f * f;
}

float CLightLinear::GetScale (float dist) const
{
	return (intens - dist * fade) * LINEAR_SCALE;
}


void CLightInv::Init ()
{
	CPointLight::Init ();
	float f = intens * INV_SCALE / MIN_POINT_LIGHT;
	maxDist2 = f * f;
}

float CLightInv::GetScale (float dist) const
{
	if (dist < 1) dist = 1;
	return intens / dist * INV_SCALE;
}


void CLightInv2::Init ()
{
	CPointLight::Init ();
	//!! wanna check this on a real map
	if (map_bspfile->type == map_q2 || map_bspfile->type == map_kp)
		DebugPrintf ("%s: inv2 slight at %g %g %g\n", *map.Name, VECTOR_ARG(origin));
	float f = SQRTFAST(intens * INV_SCALE / MIN_POINT_LIGHT);
	maxDist2 = f * f;
}

float CLightInv2::GetScale (float dist) const
{
	if (dist < 1) dist = 1;
	return intens * INV_SCALE / (dist * dist);
}


void CLightNofade::Init ()
{
	CPointLight::Init ();
	maxDist2 = BIG_NUMBER;			// infinite distance
}

float CLightNofade::GetScale (float dist) const
{
	return intens;					// no fading
}


void CPointLight::Add (const CVec3 &org, const CAxis &axis) const
{
	CVec3 dif;
	VectorSubtract (org, origin, dif);
	float dist = dot (dif, dif);						// dist*dist
	if (dist > maxDist2) return;						// too far

	// normalize dif and get its length
	float scale;
	if (spot)
	{
		dist = sqrt (dist);								// should be more precisious
		scale = 1.0f / dist;
	}
	else
	{
		scale = appRsqrt (dist);
		dist = 1.0f / scale;
	}
	dif.Scale (scale);

	scale = 1;
	if (spot)
	{
		float dot2 = dot (dif, spotDir);
		if (dot2 <= spotDot) return;					// outside the cone
		if (focus != 1)
		{
			dot2 = dot2 * focus - (focus - 1);
			if (dot2 <= 0) return;
			scale = dot2;
		}
	}
	scale *= GetScale (dist);
	scale *= vp.lightStyles[style].value / 128.0f;		// 0--0.0, 128--1.0, 256--2.0
	if (scale < MIN_POINT_LIGHT) return;				// "scale" will convert 0..1 range to 0..255

	int br;
	if (CM_BrushTrace (origin, org, &br, 1)) return;

	AddLight (axis, dif, scale, color);
#if LIGHT_DEBUG
	if (gl_lightLines->value)
		LightLine (axis, origin, org, color, scale);
#endif
}


/*-----------------------------------------------------------------------------
	Surface lights
-----------------------------------------------------------------------------*/

#define SURF_SCALE			1.0f	// */ Cvar_VariableValue("sscale")

void CSurfLight::Init ()
{
	cluster = -2;					// will be used later by GetSurfLightCluster()

	// compute surface center
	float x = (pl->mins2[0] + pl->maxs2[0]) / 2;
	float y = (pl->mins2[1] + pl->maxs2[1]) / 2;
	CVec3 v;
	VectorScale (pl->axis[0], x, v);
	VectorMA (v, y, pl->axis[1]);
	VectorMA (v, pl->plane.dist + 1, pl->plane.normal, center);

	// compute maxDist2
	// same distance equation, as for CLightInv2
	float f = SQRTFAST(intens * SURF_SCALE / MIN_SURF_LIGHT);	// max distance by normal
	x = (pl->maxs2[0] - pl->mins2[0]) / 2;		// max distance from center to be at normal
	y = (pl->maxs2[1] - pl->mins2[1]) / 2;
	maxDist2 = f*f + x*x + y*y;
}


static bool needSunAmbient;

void CSurfLight::Add (const CVec3 &org, const CAxis &axis) const
{
	CVec3	dir, dst;

	bool ambient = sky && needSunAmbient;				// sun ambient requirement
	if (!intens && !ambient) return;					// ambient-only surface

	float distN = pl->plane.DistanceTo (org);
	if (distN < 0.001f) return;							// backface culled

	// fast distance culling
	if (!ambient)
	{	// cull surface only if it is not for ambient sunlight
		if (distN * distN > maxDist2) return;			// too far from plane
		VectorSubtract (org, center, dir);
		float dist2 = dot (dir, dir);
		if (dist2 > maxDist2) return;					// too far from center
	}

	// determine nearest point on light surface rect
	float x = dot (org, pl->axis[0]);
	float y = dot (org, pl->axis[1]);
	bool slope = false;
	float dx = 0, dy = 0;
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

	float dist, realIntens;
	if (slope)
	{
		VectorScale (pl->axis[0], dx, dir);
		VectorMA (dir, dy, pl->axis[1]);
		VectorMA (dir, distN, pl->plane.normal);
		dist = dir.NormalizeFast ();
		realIntens = intens * distN / (dist * dist * dist) * SURF_SCALE;
	}
	else
	{
		dir = pl->plane.normal;
		dist = distN;
		realIntens = intens / (distN * distN) * SURF_SCALE;
	}

	realIntens *= vp.lightStyles[0].value / 128.0f;			// surface lights have style=0
	if (realIntens < MIN_SURF_LIGHT && !ambient) return;

	// compensate short distances
	float w0 = pl->maxs2[0] - pl->mins2[0];
	float h0 = pl->maxs2[1] - pl->mins2[1];
	float w  = min(w0, dist);
	float h  = min(h0, dist);
	if (w != w0 || h != h0)
	{
		realIntens *= w * h / (w0 * h0);
		if (realIntens < MIN_SURF_LIGHT && !ambient) return;
	}

	VectorMA (org, -dist + 1, dir, dst);				// need to shift in light direction because of trace bugs with non-axial planes
	int br[2];
	int numBr = CM_BrushTrace (org, dst, br, 2);
	if (numBr > 1) return;		// allow 0 or 1 brush intersection

	if (numBr)					// may be, light source is placed in niche -- check the center of surface
	{
		if (realIntens < 10 || !slope) return;
		dst = center;
		if (CM_RefineBrushTrace (org, dst, br, numBr)) return;	// 1st -- try to clip against previous brush
		if (CM_BrushTrace (dst, org, br, 1)) return;			// not clipped -- try other brushes too
	}

	if (ambient)				// current surface gives sun ambient
	{
		float scale = vp.lightStyles[0].value * SUN_AMBIENT_SCALE / 128.0f;
		CVec3 c;
		c[0] = map.sunColor[0] * map.sunAmbient[0] * scale;
		c[1] = map.sunColor[1] * map.sunAmbient[1] * scale;
		c[2] = map.sunColor[2] * map.sunAmbient[2] * scale;
		for (int i = 0; i < 6; i++)
			entityColorAxis[i].Add (c);
		needSunAmbient = false;
	}

	if (!realIntens) return;				// ambient-only surface
	AddLight (axis, dir, realIntens, color);
#if LIGHT_DEBUG
	if (gl_lightLines->value && realIntens > gl_lightLines->value)
		LightLine (axis, dst, org, color, realIntens);
#endif
}


/*-----------------------------------------------------------------------------
	Enumerating lights
-----------------------------------------------------------------------------*/

static void AddLightsChain (const CLight *first, const CVec3 &org, const CAxis &axis, const byte *vis)
{
	for (const CLight *light = first; light; light = light->next)
	{
		// PVS cull
		if (vis && light->cluster >= 0)
		{
			if (!(vis[light->cluster>>3] & (1<<(light->cluster&7))))
				continue;				// light is culled
		}
		light->Add (org, axis);
	}
}


static lightCell_t darkCell, outCell;	// zero-initialized

#define NORMALIZE_AXIS		1

static bool GetCellLight (const CVec3 *origin, int *coord, refEntity_t *ent)
{
	lightCell_t	**pcell, *cell;
	int		i;
	float	*out;
	const CAxis *axis;

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
			byte *src;
			for (i = 0, src = cell->c[0], out = entityColorAxis[0].v; i < 6*3; i++, src++, out++)
				*out = *src * CACHE_LIGHT_SCALE;
			return cell != &outCell;
		}
		axis = &identAxis;
	}
	else
	{
		pcell  = NULL;
		origin = &ent->center;
		axis   = &ent->coord.axis;
	}

	node_t *leaf = PointInLeaf (*origin);
	const byte *row = leaf->cluster < 0 || !map.visInfo ? NULL : map.visInfo + leaf->cluster * map.visRowSize;

	float scale = vp.lightStyles[0].value * AMBIENT_SCALE / 128.0f;
	for (i = 0; i < 6; i++)
		VectorScale (map.ambientLight, scale, entityColorAxis[i]);

	if (!row && map.visInfo)	//?? check correctness, when map w/o visinfo: how to detect out-of-world?
	{
		if (pcell) *pcell = &outCell;
		return false;		// point is outside the world
	}

	/*------------------ directed sunlight ------------------*/

	if (map.sunLight)
	{
		CVec3 dst;
		VectorMA (*origin, -32768, map.sunVec, dst);
		trace_t	tr;
		CM_BoxTrace (tr, *origin, dst, nullBox, 0, CONTENTS_SOLID);
		if (tr.surface->flags & SURF_SKY && !tr.startsolid)		// can be "startsolid" even if "row"!=NULL
		{
			float intens = map.sunLight * SUN_SCALE * vp.lightStyles[0].value / 128.0f;	// sun light have style=0
			AddLight (*axis, map.sunVec, intens, map.sunColor);
#if LIGHT_DEBUG
			if (gl_lightLines->value)
				LightLine (*axis, tr.endpos, *origin, map.sunColor, intens);
#endif
		}
	}

	// add static lights
	AddLightsChain (map.lights, *origin, *axis, row);

	if (ent)
	{
		// requested light without caching
		AddLightsChain (map.flashLights, *origin, *axis, row);
		return true;
	}

	// store computed light into grid

	// check gathered light ampout
	bool hasLight = false;
	for (i = 0, out = entityColorAxis[0].v; i < 6*3; i++, out++)
		if (*out)
		{	// have a non-zero color
			hasLight = true;
			break;
		}
	if (!hasLight)
	{
		*pcell = &darkCell;
		return true;
	}

	// alloc new cell
	cell = new (map.lightGridChain) lightCell_t;
	*pcell = cell;
	map.numLightCells++;

	byte *dst;
#if NORMALIZE_AXIS
	// find maximal color
	float m = 0;
	for (i = 0, out = entityColorAxis[0].v; i < 6*3; i++, out++)
		if (*out > m) m = *out;
	// normalize color axis and copy to grid
	if (m > 255 * CACHE_LIGHT_SCALE)
		m = 255.0f / CACHE_LIGHT_SCALE / m;
	else
		m = 1.0f / CACHE_LIGHT_SCALE;
	for (i = 0, out = entityColorAxis[0].v, dst = cell->c[0]; i < 6*3; i++, out++, dst++)
		*dst = appRound (*out * m);
#else
	for (i = 0, out = entityColorAxis[0].v, dst = cell->c[0]; i < 6; i++)
	{
		int r = appRound (*out++ / CACHE_LIGHT_SCALE);
		int g = appRound (*out++ / CACHE_LIGHT_SCALE);
		int b = appRound (*out++ / CACHE_LIGHT_SCALE);
		NORMALIZE_COLOR255(r, g, b);
		*dst++ = r;
		*dst++ = g;
		*dst++ = b;
	}
#endif

	return true;
}


static void GatherWorldLight (refEntity_t *ent)
{
	int i;

	// NOTE: sl.color is [0..1], out color is [0..255]

	if (vp.flags & RDF_NOWORLDMODEL) return;	// no world - no light

	if (gl_noGrid->integer)
	{
		GetCellLight (NULL, NULL, ent);
		return;
	}

	i = PointInLeaf (ent->center)->cluster;
	const byte *row = i < 0 || !map.visInfo ? NULL : map.visInfo + i * map.visRowSize;

	// prepare for grid enumeration
	CVec3	pos, frac;
	int		coord[3];
	for (i = 0; i < 3; i++)
	{
		coord[i] = appFloor (ent->center[i] / LIGHTGRID_STEP);
		pos  [i] = coord[i] * LIGHTGRID_STEP;
		frac [i] = (ent->center[i] - pos[i]) / LIGHTGRID_STEP;
		coord[i] -= map.gridMins[i];
	}

#if LIGHT_DEBUG
	gl_depthMode_t prevDepth;
	if (gl_showGrid->integer)
	{
//		DrawTextLeft (va("pos: %g %g %g frac: %g %g %g", VECTOR_ARG(pos), VECTOR_ARG(frac)));
		prevDepth = gl_state.currentDepthMode;
		GL_DepthRange (DEPTH_NEAR);
		glPushMatrix ();
		glLoadMatrixf (&vp.modelMatrix[0][0]);
		GL_SetMultitexture (0);		// disable texturing
		GL_State (GLSTATE_POLYGON_LINE|GLSTATE_DEPTHWRITE);
		glDisableClientState (GL_COLOR_ARRAY);
	}
#endif // LIGHT_DEBUG

	// enumerate 8 grid cells
	CVec3 accum[6];
	memset (accum, 0, sizeof(accum));
	float totalFrac = 0;
	for (i = 0; i < 8; i++)
	{
		CVec3	origin;
		int		c[3], j;

		float f = 1;
		for (j = 0; j < 3; j++)
			if (i & (1<<j))
			{
				origin[j] = pos[j];
				c[j]      = coord[j];
				f        *= 1 - frac[j];
			}
			else
			{
				origin[j] = pos[j] + LIGHTGRID_STEP;
				c[j]      = coord[j] + 1;
				f        *= frac[j];
			}
		if (!f) continue;			// will not add light from this cell
		if (!GetCellLight (&origin, c, NULL)) continue;
		// apply light from grid
		totalFrac += f;
		for (j = 0; j < 6; j++)
			VectorMA (accum[j], f, entityColorAxis[j]);
#if LIGHT_DEBUG
		// show lightgrid
		if (gl_showGrid->integer)
		{
			glBegin (GL_LINES);
			for (j = 0; j < 6; j++)
			{
				glColor3f (entityColorAxis[j][0]/255, entityColorAxis[j][1]/255, entityColorAxis[j][2]/255);
				glVertex3fv (origin.v);
				CVec3 origin2 = origin;
				origin2[j >> 1] += (j & 1) ? 2 : -2;
				glVertex3fv (origin2.v);
			}
			glEnd ();
		}
#endif // LIGHT_DEBUG
	}
	if (totalFrac != 1 && totalFrac != 0)	// != 1 -- optimization, != 0 -- zero divide (may happens, when model outside world)
	{	// if some points have missed (outside the world) -- scale light from correct points
		totalFrac = 1.0f / totalFrac;
		for (i = 0; i < 6; i++) accum[i].Scale (totalFrac);
	}
#if LIGHT_DEBUG
	if (gl_showGrid->integer)
	{
		glPopMatrix ();
		GL_DepthRange (prevDepth);
	}
#endif

	// rotate light axis to entity axis
	memset (entityColorAxis, 0, sizeof(entityColorAxis));
	for (i = 0; i < 6; i++)
	{
		float	v;
		int		side;

#if 1
#define STEP(n)							\
		v = ent->coord.axis[n][i>>1]; \
		FAbsSign(v,v,side);		\
		side ^= i & 1;			\
		VectorMA(entityColorAxis[n*2+side], v, accum[i]);
#else
#define STEP(n)							\
		v = ent->coord.axis[n][i>>1]; \
		if (i & 1) v = -v;		\
		if (v < 0)	VectorMA (entityColorAxis[n*2+1], -v, accum[i]); \
		else		VectorMA (entityColorAxis[n * 2],  v, accum[i]);
#endif
		STEP(0); STEP(1); STEP(2);
#undef STEP
	}

	// add dynamic (style!=0) lights
	AddLightsChain (map.flashLights, ent->center, ent->coord.axis, row);
}


static void GatherDlights (refEntity_t *ent)
{
	int i;
	refDlight_t	*dl;
	for (i = 0, dl = vp.dlights; i < vp.numDlights; i++, dl++)
	{
		CVec3	dst;
		VectorSubtract (ent->center, dl->origin, dst);
		float dist = dot (dst, dst);						// dist*dist
		if (dist > dl->intensity * dl->intensity) continue;	// dlight is too far
		float denom = appRsqrt (dist);						// 1/sqrt(dist)
		dist = 1.0f / denom;
		dst.Scale (denom);

		float scale = (dl->intensity - dist) * LINEAR_SCALE;
		denom = 1.0f / gl_config.identityLightValue;
		CVec3 color;
		color[0] = dl->c.c[0] * denom;
		color[1] = dl->c.c[1] * denom;
		color[2] = dl->c.c[2] * denom;
		AddLight (ent->coord.axis, dst, scale, color);
#if LIGHT_DEBUG
		if (gl_lightLines->value)
			LightLine (ent->coord.axis, dl->origin, ent->center, color, scale);
#endif
	}
}


/*-----------------------------------------------------------------------------
	Entity lighting
-----------------------------------------------------------------------------*/

void LightForEntity (refEntity_t *ent)
{
	STAT(clock(gl_stats.entLight));

	memset (entityColorAxis, 0, sizeof(entityColorAxis));
	GatherWorldLight (ent);
	GatherDlights (ent);


	// swap left and right lights when mirrored entity
	if (ent->mirror)
		Exchange (entityColorAxis[2], entityColorAxis[3]);

	if (ent->flags & RF_GLOW)
	{
#define MAX_GLOW	0.2		// Cvar_VariableValue("glow");
		float glow = (SIN_FUNC(vp.time / 1.5f) + 1) / 2 * MAX_GLOW;	// 0..MAX_GLOW
		// lerp colors between color and 255 with glow
		for (int i = 0; i < 6; i++)
			for (int j = 0; j < 3; j++)
				entityColorAxis[i][j] = (1 - glow) * entityColorAxis[i][j] + glow * 255;
	}

#if NORMALIZE_AXIS
	// find maximal color
	int i;
	float m = 0;
	float *out;
	for (i = 0, out = entityColorAxis[0].v; i < 6*3; i++, out++)
		if (*out > m) m = *out;
	// normalize color axis and copy to grid
	if (m > 255)
	{
		m = 255.0f / m;
		for (i = 0, out = entityColorAxis[0].v; i < 6*3; i++, out++)
			*out *= m;
	}
#else
	for (int i = 0; i < 6; i++)
		ClampColor255 (entityColorAxis[i], entityColorAxis[i]);
#endif

	STAT(unclock(gl_stats.entLight));
}


//?? really, should move this func to backend, but must access entityColorAxis[]
void DiffuseLight (color_t *dst, float lightScale)
{
	int		i, tmp;
	bufExtra_t *ex;

	STAT(clock(gl_stats.meshLight));

	CVec3 maxColor = entityColorAxis[0];
	maxColor.Zero ();
	for (i = 1; i < 6; i++)
	{
		CVec3 &c = entityColorAxis[i];
		if (maxColor[0] < c[0]) maxColor[0] = c[0];
		if (maxColor[1] < c[1]) maxColor[1] = c[1];
		if (maxColor[2] < c[2]) maxColor[2] = c[2];
	}
#define COLOR_STEP(src,dst,n)		\
		tmp = appRound (src[n]);	\
		dst.c[n] = (tmp > 255) ? 255 : tmp;
	color_t c2;
	COLOR_STEP(maxColor,c2,0); COLOR_STEP(maxColor,c2,1); COLOR_STEP(maxColor,c2,2);

	float light = lightScale * 2 * gl_config.identityLightValue_f;	// *2 -- because 1 == double light (similar to lightmaps)
	for (i = 0, ex = gl_extra; i < gl_numExtra; i++, ex++)
	{
		float	val;
		CVec3	*axis;

		const CVec3 &norm = ex->normal;
		color_t	c;

		if (norm[0] || norm[1] || norm[2])
		{
			// compute color
			CVec3 color;
			color.Zero ();
#if 1
#define STEP(n)		\
			FAbsSign(norm[n],val,tmp);	\
			val  *= light;				\
			axis = &entityColorAxis[n*2+1-tmp];
#else
#define STEP(n)		\
			axis = &entityColorAxis[(norm[n] < 0 ? (n * 2) : (n * 2 + 1))];	\
			val  = fabs (norm[n]) * light;
#endif

			STEP(0);
			VectorScale (*axis, val, color);
			STEP(1);
			VectorMA (color, val, *axis);
			STEP(2);
			VectorMA (color, val, *axis);
#undef STEP

			// apply color
			COLOR_STEP(color,c,0); COLOR_STEP(color,c,1); COLOR_STEP(color,c,2);
#undef COLOR_STEP
			c.c[3] = 255;
		}
		else
			c.rgba = c2.rgba;

		for (int j = 0; j < ex->numVerts; j++, dst++)	// normally, here will be only 1 iteration ...
			dst->rgba = c.rgba;							// just put computed color
	}

	STAT(unclock(gl_stats.meshLight));
}


/*-----------------------------------------------------------------------------
	Initialization
-----------------------------------------------------------------------------*/

void InitLightGrid ()
{
	for (int i = 0; i < 3; i++)
	{
		map.gridMins[i] = appFloor (map.nodes[0].bounds.mins[i] / LIGHTGRID_STEP);
		map.mapGrid[i]  = appCeil  (map.nodes[0].bounds.maxs[i] / LIGHTGRID_STEP) - map.gridMins[i];
	}
	map.lightGridChain = new CMemoryChain;
	map.lightGrid      = new (map.lightGridChain) lightCell_t* [map.mapGrid[0] * map.mapGrid[1] * map.mapGrid[2]];
	map.numLightCells  = 0;
}


static void GetSurfLightCluster ()
{
	int i, j;
	node_t	*n;
	// enumerate all leafs
	for (i = 0, n = map.nodes + map.numNodes; i < map.numLeafNodes - map.numNodes; i++, n++)
	{
		int cl = n->cluster;
		surfaceBase_t **s;
		for (j = 0, s = n->leafFaces; j < n->numLeafFaces; j++, s++)
			if ((*s)->type == SURFACE_PLANAR && !(*s)->owner)		//?? other types
			{
				surfacePlanar_t *pl = static_cast<surfacePlanar_t*>(*s);
				if (pl->light)
				{
					if (pl->light->cluster == -2)	// uninitialized
						pl->light->cluster = cl;
					else if (pl->light->cluster != cl)
						pl->light->cluster = -1;	// single surface in few clusters
				}
			}
	}
}


void PostLoadLights ()
{
	CLight *light;
	for (light = map.lights; light; light = light->next)
		light->Init ();
	for (light = map.flashLights; light; light = light->next)
		light->Init ();
	GetSurfLightCluster ();
}


} // namespace
