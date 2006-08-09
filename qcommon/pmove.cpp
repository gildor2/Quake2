/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "qcommon.h"
#include "protocol.h"


//#define PM_DEBUG		1


#if PM_DEBUG
void SV_DrawTextLeft (const char *text, unsigned rgba = RGB(1,1,1));
#endif


//#define OLD_PMOVE	1
#define	STEPSIZE	18

// all of the locals will be zeroed before each
// pmove, just to make damn sure we don't have
// any differences when running on client or server

struct pml_t
{
	CVec3		origin;			// full float precision
	CVec3		velocity;		// full float precision

	CVec3		forward, right, up;
	float		frametime;


	csurface_t	*groundsurface;
	CPlane		groundplane;
	int			groundcontents;

	short		previous_origin[3];

	bool		ladder;
	CPlane		ladderPlane;
};
static pml_t	pml;
static pmove_t	*pm;


// movement parameters
static float pm_stopspeed = 100;
static float pm_maxspeed  = 300;
static float pm_duckspeed = 100;
static float pm_accelerate      = 10;
float		 pm_airaccelerate   = 0;		// really, default value is 1.0f
static float pm_wateraccelerate = 10;
static float pm_friction      = 6;
static float pm_waterfriction = 1;
static float pm_waterspeed = 400;


inline void PM_Trace (trace_t &trace, const CVec3 &start, const CVec3 &end, const CBox &bounds)
{
	// original: trace = pm->trace (pml.origin, bounds.mins, bounds.maxs, end);
	pm->trace (trace, start, bounds.mins, bounds.maxs, end);
}


/*
==================
ClipVelocity

Slide off of the impacting object
returns the blocked flags (1 = floor, 2 = step / wall)
==================
*/
#define	STOP_EPSILON	0.1

static void ClipVelocity (const CVec3 &in, const CVec3 &normal, CVec3 &out, float overbounce)
{
	float backoff = dot (in, normal) * overbounce;
	VectorMA (in, -backoff, normal, out);

	for (int i = 0; i < 3; i++)
		if (out[i] > -STOP_EPSILON && out[i] < STOP_EPSILON)
			out[i] = 0;
}




/*
==================
StepSlideMove

Each intersection will try to step over the obstruction instead of
sliding along it.

Returns a new origin, velocity, and contact entity
Does not modify any world state?
==================
*/
#define	MIN_STEP_NORMAL	0.7		// can't step up onto very steep slopes
#define	MAX_CLIP_PLANES	5
#define OVERBOUNCE		1.01

static bool SlideMove ()
{
	CVec3 planes[MAX_CLIP_PLANES];
	CVec3 primal_velocity = pml.velocity;
	int numplanes;
#if OLD_PMOVE
	numplanes = 0;
#else
	if (pm->s.pm_flags & PMF_ON_GROUND)
	{
		planes[0] = pml.groundplane.normal;
		numplanes = 1;
	}
	else
		numplanes = 0;
#endif

	float time_left = pml.frametime;
	trace_t trace;

	int bumpcount;
	for (bumpcount = 0; bumpcount < 4; bumpcount++)
	{
		CVec3 end;
		VectorMA (pml.origin, time_left, pml.velocity, end);
		PM_Trace (trace, pml.origin, end, pm->bounds);

		if (trace.allsolid)
		{	// entity is trapped in another solid
			pml.velocity[2] = 0;	// don't build up falling damage
			return true;
		}

		if (trace.fraction > 0)
		{	// actually covered some distance
			pml.origin = trace.endpos;
			numplanes = 0;			//??
		}

		if (trace.fraction == 1)
			break;		// moved the entire distance

		// save entity for contact
		if (pm->numtouch < MAXTOUCH && trace.ent)
			pm->touchents[pm->numtouch++] = trace.ent;

		time_left *= 1 - trace.fraction;

		// slide along this plane
		if (numplanes >= MAX_CLIP_PLANES)
		{	// this shouldn't really happen
			pml.velocity.Zero();
			return true;
		}

		planes[numplanes] = trace.plane.normal;
		numplanes++;

#if OLD_PMOVE
		// modify original_velocity so it parallels all of the clip planes
		int i;
		for (i = 0; i < numplanes; i++)
		{
			ClipVelocity (pml.velocity, planes[i], pml.velocity, 1.01);
			int j;
			for (j = 0; j < numplanes; j++)
				if (j != i)
				{
					if (dot (pml.velocity, planes[j]) < 0)
						break;	// not ok
				}
			if (j == numplanes)
				break;
		}

		if (i != numplanes)
		{	// go along this plane
		}
		else
		{	// go along the crease
			if (numplanes != 2)
			{
//				appPrintf ("clip velocity, numplanes == %d\n",numplanes);
				pml.velocity.Zero();
				break;
			}
			CVec3 dir;
			cross (planes[0], planes[1], dir);
			float d = dot (dir, pml.velocity);
			VectorScale (dir, d, pml.velocity);
		}
#else
		// modify original velocity so it parallels all of the clip planes
		for (int i = 0; i < numplanes; i++)
		{
			if (dot (pml.velocity, planes[i]) >= 0) continue;
			ClipVelocity (pml.velocity, planes[i], pml.velocity, OVERBOUNCE);

			for (int j = 0; j < numplanes; j++)
			{
				if (j == i) continue;
				if (dot (pml.velocity, planes[j]) >= 0) continue;

				ClipVelocity (pml.velocity, planes[j], pml.velocity, OVERBOUNCE);
				// check for reentering plane[i] after clip against plane[j]
				if (dot (pml.velocity, planes[i]) >= 0) continue;

				// go along the crease
				CVec3 dir;
				cross (planes[i], planes[j], dir);
				dir.NormalizeFast ();
				float d = dot (dir, pml.velocity);
				VectorScale (dir, d, pml.velocity);
			}
			break;
		}
#endif

		// if velocity is against the original velocity, stop dead
		// to avoid tiny oscilations in sloping corners
		if (dot (pml.velocity, primal_velocity) <= 0)
		{
			pml.velocity.Zero();
			break;
		}
	}

	if (pm->s.pm_time)
		pml.velocity = primal_velocity;

	return (bumpcount > 0) || trace.fraction < 1;
}


static void StepSlideMove ()
{
#if PM_DEBUG
	SV_DrawTextLeft("slide");
#endif

	CVec3 start_o = pml.origin;
	CVec3 start_v = pml.velocity;

#if OLD_PMOVE
	if (!SlideMove ()) return;
#else
	SlideMove ();
#endif

	CVec3 down_o = pml.origin;
	CVec3 down_v = pml.velocity;

	CVec3 up = start_o;
	up[2] += STEPSIZE;

#if OLD_PMOVE
	trace_t trace;
	PM_Trace (trace, up, up, pm->bounds);
	if (trace.allsolid)
		return;		// can't step up

	float stepHeight = STEPSIZE;
	pml.origin = up;
	pml.velocity = start_v;
#else
	trace_t trace;
	PM_Trace (trace, start_o, up, pm->bounds);
	if (trace.allsolid)
	{
		// can't step up
#if PM_DEBUG
		SV_DrawTextLeft ("allsolid: no up",RGB(1,0,0));
#endif
		return;
	}

	// try sliding above
	float stepHeight = trace.endpos[2] - start_o[2];
	pml.origin = trace.endpos;
	pml.velocity = start_v;
#endif

//appPrintf(S_GREEN" up ");//!!
	SlideMove ();

	// push down the final amount
	CVec3 down = pml.origin;
	down[2] -= stepHeight;
	PM_Trace (trace, pml.origin, down, pm->bounds);
	if (!trace.allsolid)
		pml.origin = trace.endpos;

	up = pml.origin;

	// decide which one went farther
	float	down_dist, up_dist;
	down_dist = (down_o[0] - start_o[0]) * (down_o[0] - start_o[0])
			  + (down_o[1] - start_o[1]) * (down_o[1] - start_o[1]);
	up_dist = (up[0] - start_o[0]) * (up[0] - start_o[0])
			+ (up[1] - start_o[1]) * (up[1] - start_o[1]);

	if (down_dist > up_dist || trace.plane.normal[2] < MIN_STEP_NORMAL)
	{
#if PM_DEBUG
		SV_DrawTextLeft (va("down: d_dist=%g u_dist=%g norm[2]=%g\n", down_dist, up_dist, trace.plane.normal[2]));
#endif
		pml.origin   = down_o;
		pml.velocity = down_v;
		return;
	}
	//!! Special case
	// if we were walking along a plane, then we need to copy the Z over
	pml.velocity[2] = down_v[2];
}


// Handles both ground friction and water friction
static void Friction ()
{
	CVec3 &vel = pml.velocity;
	float speed = dot(vel, vel);		// this is (speed*speed)

	if (speed < 1)
	{
		vel[0] = 0;
		vel[1] = 0;
		return;
	}

	speed = SQRTFAST(speed);			// finish speed calculation
	float drop = 0;

	// apply ground friction
	if ((pm->groundentity && pml.groundsurface && !(pml.groundsurface->flags & SURF_SLICK)) || pml.ladder)
	{
#if PM_DEBUG
		SV_DrawTextLeft ("friction");
#endif
		float control = speed < pm_stopspeed ? pm_stopspeed : speed;
		drop += control * pm_friction * pml.frametime;
	}

	// apply water friction
	if (pm->waterlevel && !pml.ladder)
		drop += speed * pm_waterfriction * pm->waterlevel * pml.frametime;

	// scale the velocity
	float newspeed = speed - drop;
	if (newspeed < 0)
		newspeed = 0;
	else
		newspeed /= speed;

	vel[0] = vel[0] * newspeed;
	vel[1] = vel[1] * newspeed;
	vel[2] = vel[2] * newspeed;
}


// Handles user intended acceleration
static void Accelerate (const CVec3 &wishdir, float wishspeed, float accel)
{
	float currentspeed = dot (pml.velocity, wishdir);
	float addspeed = wishspeed - currentspeed;
	if (addspeed <= 0) return;

	float accelspeed = accel * pml.frametime * wishspeed;
	if (accelspeed > addspeed)
		accelspeed = addspeed;

	VectorMA (pml.velocity, accelspeed, wishdir);
}


static void AddCurrents (CVec3 &wishvel)
{
	CVec3	v;
	float	f;

	// account for ladders
	if (pml.ladder && pml.velocity[2] >= -200 && pml.velocity[2] <= 200)
	{
		if ((pm->viewangles[PITCH] >= 15 || pm->viewangles[PITCH] <= -15) && pm->cmd.forwardmove > 0)
		{
			float y = -400 / 90 * pm->viewangles[PITCH];	// full speed when pitch angle is 45 grad.

			y = bound(y, -200, 200);
			wishvel[2] = y;
		}
		else if (pm->cmd.upmove > 0)
			wishvel[2] = 200;
		else if ((pm->cmd.upmove < 0) || (pm->cmd.forwardmove < 0))
			wishvel[2] = -200;
		else
			wishvel[2] = 0;

#define LADDER_SIDE_SPEED	70
		// limit horizontal speed when on a ladder
		if (!(pm->s.pm_flags & PMF_ON_GROUND))		// full speed when hit ladder while stand on ground
		{
			if (pml.ladderPlane.normal[2] != 1 && pml.ladderPlane.normal[2] != -1)
			{
				static const CVec3 up = {0, 0, 1};
				CVec3	axis[2], vel_vert;

				// compute projections of velocity onto ladder plane axis
				cross (pml.ladderPlane.normal, up, axis[0]);
				axis[0].NormalizeFast ();
				cross (pml.ladderPlane.normal, axis[0], axis[1]);

				f = dot (wishvel, axis[1]);
				VectorScale (axis[1], f, vel_vert);

				// limit horizontal component of speed
				//!! NOTE: SlideMove() may add its own horizontal speed
				if (pm->cmd.sidemove == 0)
					wishvel = vel_vert;
				else
				{
					f = dot (wishvel, axis[0]);
					f = bound(f, -LADDER_SIDE_SPEED, LADDER_SIDE_SPEED);
					VectorMA (vel_vert, f, axis[0], wishvel);
				}
			}
		}
	}

	// add water currents
	if (pm->watertype & MASK_CURRENT)
	{
		v.Zero();
		if (pm->watertype & CONTENTS_CURRENT_0)		v[0] =  1;
		if (pm->watertype & CONTENTS_CURRENT_180)	v[0] = -1;
		if (pm->watertype & CONTENTS_CURRENT_90)	v[1] =  1;
		if (pm->watertype & CONTENTS_CURRENT_270)	v[1] = -1;
		if (pm->watertype & CONTENTS_CURRENT_UP) 	v[2] =  1;
		if (pm->watertype & CONTENTS_CURRENT_DOWN)	v[2] = -1;

		f = pm_waterspeed;
		if ((pm->waterlevel == 1) && (pm->groundentity))
			f /= 2;

		VectorMA (wishvel, f, v);
	}

	// add conveyor belt velocities
	if (pm->groundentity && (pml.groundcontents & MASK_CURRENT))
	{
		v.Zero();
		if (pml.groundcontents & CONTENTS_CURRENT_0)	v[0] =  1;
		if (pml.groundcontents & CONTENTS_CURRENT_180)	v[0] = -1;
		if (pml.groundcontents & CONTENTS_CURRENT_90)	v[1] =  1;
		if (pml.groundcontents & CONTENTS_CURRENT_270)	v[1] = -1;
		if (pml.groundcontents & CONTENTS_CURRENT_UP) 	v[2] =  1;
		if (pml.groundcontents & CONTENTS_CURRENT_DOWN)	v[2] = -1;

		VectorMA (wishvel, 100 /* pm->groundentity->speed */, v);
	}
}


static void WaterMove ()
{
	CVec3	wishvel;
	for (int i = 0; i < 3; i++)
		wishvel[i] = pml.forward[i] * pm->cmd.forwardmove + pml.right[i] * pm->cmd.sidemove;

	if (!pm->cmd.forwardmove && !pm->cmd.sidemove && !pm->cmd.upmove)
		wishvel[2] -= 60;		// drift towards bottom
	else
		wishvel[2] += pm->cmd.upmove;

	AddCurrents (wishvel);

	CVec3 wishdir = wishvel;
	float wishspeed = wishdir.Normalize();

	if (wishspeed > pm_maxspeed)
	{
		wishvel.Scale (pm_maxspeed/wishspeed);
		wishspeed = pm_maxspeed;
	}
	wishspeed *= 0.5;

	Accelerate (wishdir, wishspeed, pm_wateraccelerate);

	StepSlideMove ();
}


static void AirMove ()
{
#if PM_DEBUG
	SV_DrawTextLeft ("AirMove");
#endif
	float fmove = pm->cmd.forwardmove;
	float smove = pm->cmd.sidemove;
	if (pml.ladder && fmove < 0 && !(pm->s.pm_flags & PMF_ON_GROUND))
		fmove = 0;			// disable backward move when on a ladder

	CVec3 wishvel, wishdir;
	for (int i = 0; i < 2; i++)
		wishvel[i] = pml.forward[i]*fmove + pml.right[i]*smove;
	wishvel[2] = 0;

	AddCurrents (wishvel);
	float wishspeed = VectorNormalize (wishvel, wishdir);

	// clamp to server defined max speed
	float maxspeed = (pm->s.pm_flags & PMF_DUCKED) ? pm_duckspeed : pm_maxspeed;

	if (wishspeed > maxspeed)
	{
		wishvel.Scale (maxspeed/wishspeed);
		wishspeed = maxspeed;
	}

	if (pml.ladder)
	{
#if PM_DEBUG
		SV_DrawTextLeft ("ladder");
#endif
		Accelerate (wishdir, wishspeed, pm_accelerate);
		if (wishvel[2] == 0)		// stop moving on ladder
		{
			if (pml.velocity[2] > 0)
			{
				pml.velocity[2] -= pm->s.gravity * pml.frametime;
				if (pml.velocity[2] < 0)
					pml.velocity[2] = 0;
			}
			else
			{
				pml.velocity[2] += pm->s.gravity * pml.frametime;
				if (pml.velocity[2] > 0)
					pml.velocity[2] = 0;
			}
		}
		StepSlideMove ();
	}
	else if (pm->groundentity)
	{
#if PM_DEBUG
		SV_DrawTextLeft ("groundentity");
#endif
		// walking on ground
		pml.velocity[2] = 0; //!!! this is before the accel
		Accelerate (wishdir, wishspeed, pm_accelerate);
		pml.velocity[2] = (pm->s.gravity > 0) ? 0 : pml.velocity[2] -= pm->s.gravity * pml.frametime;

		if (!pml.velocity[0] && !pml.velocity[1])
			return;
		StepSlideMove ();
	}
	else
	{
#if PM_DEBUG
		SV_DrawTextLeft ("in air");
#endif
		// not on ground, so little effect on velocity
		Accelerate (wishdir, wishspeed, pm_airaccelerate ? pm_airaccelerate : 1);
		// add gravity
		pml.velocity[2] -= pm->s.gravity * pml.frametime;
		StepSlideMove ();
	}
}



static void CatagorizePosition ()
{
	// if the player hull point one unit down is solid, the player
	// is on ground

	CVec3 point = pml.origin;

	if (pml.velocity[2] > 180) //ZOID changed from 100 to 180 (ramp accel)
	{	// NOTE: if disable this code, jumppads will be VERY buggy; and waterjump will be very high
		pm->s.pm_flags &= ~PMF_ON_GROUND;
		pm->groundentity = NULL;
	}
	else
	{
		// see if standing on something solid
		trace_t trace;
		point[2] = pml.origin[2] - 0.25f;
		PM_Trace (trace, pml.origin, point, pm->bounds);
		pml.groundplane    = trace.plane;
		pml.groundsurface  = trace.surface;
		pml.groundcontents = trace.contents;

//		appPrintf("gr: n2=%g frac=%g sol=%d\n", trace.plane.normal[2], trace.fraction, trace.allsolid);//!!
//		if (trace.allsolid) appWPrintf("locked: %g %g %g\n", VECTOR_ARG(point));
		if (!trace.ent || (trace.plane.normal[2] < 0.7 && !trace.startsolid))
		{
			pm->groundentity = NULL;
			pm->s.pm_flags &= ~PMF_ON_GROUND;
#if PM_DEBUG
			SV_DrawTextLeft ("!ON_GROUND");
#endif
		}
		else
		{
			pm->groundentity = trace.ent;

			// hitting solid ground will end a waterjump
			if (pm->s.pm_flags & PMF_TIME_WATERJUMP)
			{
				pm->s.pm_flags &= ~(PMF_TIME_WATERJUMP|PMF_TIME_LAND|PMF_TIME_TELEPORT);
				pm->s.pm_time = 0;
			}

			if (!(pm->s.pm_flags & PMF_ON_GROUND))
			{	// just hit the ground
				pm->s.pm_flags |= PMF_ON_GROUND;
				// don't do landing time if we were just going down a slope
				if (pml.velocity[2] < -200)
				{
					pm->s.pm_flags |= PMF_TIME_LAND;
					// don't allow another jump for a little while
					if (pml.velocity[2] < -400)
						pm->s.pm_time = 25;
					else
						pm->s.pm_time = 18;
				}
			}
		}

#if 0
		if (trace.fraction < 1.0 && trace.ent && pml.velocity[2] < 0)
			pml.velocity[2] = 0;
#endif

		if (pm->numtouch < MAXTOUCH && trace.ent)
		{
			pm->touchents[pm->numtouch] = trace.ent;
			pm->numtouch++;
#if PM_DEBUG
			SV_DrawTextLeft (va("touched %d ents ($%X): plane: %g %g %g / %g",
				pm->numtouch, trace.ent, VECTOR_ARG(trace.plane.normal), trace.plane.dist),RGB(1,0,0));
#endif
		}
	}

	// get waterlevel, accounting for ducking
	pm->waterlevel = 0;
	pm->watertype  = 0;

	float sample2 = pm->viewheight - pm->bounds.mins[2];
	float sample1 = sample2 / 2;

	point[2] = pml.origin[2] + pm->bounds.mins[2] + 1;
	unsigned cont = pm->pointcontents (point);
	if (cont & MASK_WATER)
	{
		pm->watertype  = cont;
		pm->waterlevel = 1;
		point[2] = pml.origin[2] + pm->bounds.mins[2] + sample1;
		cont = pm->pointcontents (point);
		if (cont & MASK_WATER)
		{
			pm->waterlevel = 2;
			point[2] = pml.origin[2] + pm->bounds.mins[2] + sample2;
			cont = pm->pointcontents (point);
			if (cont & MASK_WATER)
				pm->waterlevel = 3;
		}
#if PM_DEBUG
		SV_DrawTextLeft (va("waterlevel=%d",pm->waterlevel),RGB(0,0.1,1));
#endif
	}
}


static void CheckJump ()
{
	if (pm->s.pm_flags & PMF_TIME_LAND)
	{	// hasn't been long enough since landing to jump again
		return;
	}

	if (pm->cmd.upmove < 10)
	{	// not holding jump
		pm->s.pm_flags &= ~PMF_JUMP_HELD;
		return;
	}

	// must wait for jump to be released
	if (pm->s.pm_flags & PMF_JUMP_HELD)
		return;

	if (pm->s.pm_type == PM_DEAD)
		return;

	if (pm->waterlevel >= 2)
	{	// swimming, not jumping
		pm->groundentity = NULL;

		if (pml.velocity[2] <= -300)
			return;

		if (pm->watertype == CONTENTS_WATER)
			pml.velocity[2] = 100;
		else if (pm->watertype == CONTENTS_SLIME)
			pml.velocity[2] = 80;
		else
			pml.velocity[2] = 50;
		return;
	}

	if (pm->groundentity == NULL)
	{
		return;		// in air, so no effect
	}

	pm->s.pm_flags |= PMF_JUMP_HELD;

	pm->groundentity = NULL;
	pml.velocity[2] += 270;
	if (pml.velocity[2] < 270)
		pml.velocity[2] = 270;
}


static void CheckSpecialMovement ()
{
	if (pm->s.pm_time) return;

	pml.ladder = false;

	// check for ladder
	CVec3 flatforward;
	flatforward[0] = pml.forward[0];
	flatforward[1] = pml.forward[1];
	flatforward[2] = 0;
	flatforward.Normalize ();

	float dist;
	if (pm->cmd.upmove < 0)
		dist = 30; // when crouched - try to detect ladders on a longer distances
	else
		dist = 1;
	CVec3 spot;
	VectorMA (pml.origin, dist, flatforward, spot);
	trace_t trace;
	PM_Trace (trace, pml.origin, spot, pm->bounds);
	if ((trace.fraction < 1) && (trace.contents & CONTENTS_LADDER))
	{
		pml.ladder      = true;
		pml.ladderPlane = trace.plane;
//		pm->groundentity = trace.ent;	// -- allows to move with entities, having CONTENTS_LADDER; requires some game changes
	}

	// check for water jump
	if (pm->waterlevel != 2)
		return;

	VectorMA (pml.origin, 30, flatforward, spot);
	spot[2] += 4;
	if (!(pm->pointcontents (spot) & CONTENTS_SOLID)) return;

	spot[2] += 16;
	if (pm->pointcontents (spot)) return;			// CONTENTS_EMPTY

	// jump out of water
	VectorScale (flatforward, 50, pml.velocity);
	pml.velocity[2] = 350;

	pm->s.pm_flags |= PMF_TIME_WATERJUMP;
	pm->s.pm_time = 255;
}


static void FlyMove (bool doclip)
{
	int		i;

	pm->viewheight = 22;

	// friction
	float speed = pml.velocity.GetLength ();
	if (speed < 1)
		pml.velocity.Zero();
	else
	{
		float friction = pm_friction * 1.5f;	// extra friction
		float control  = speed < pm_stopspeed ? pm_stopspeed : speed;
		float drop     = control * friction * pml.frametime;

		// scale the velocity
		float newspeed = speed - drop;
		if (newspeed < 0)
			newspeed = 0;
		else
			newspeed /= speed;

		pml.velocity.Scale (newspeed);
	}

	// accelerate
	float fmove = pm->cmd.forwardmove;
	float smove = pm->cmd.sidemove;

	pml.forward.Normalize ();
	pml.right.Normalize ();

	CVec3 wishvel;
	for (i = 0; i < 3; i++)
		wishvel[i] = pml.forward[i] * fmove + pml.right[i] * smove;
	wishvel[2] += pm->cmd.upmove;

	CVec3 wishdir = wishvel;
	float wishspeed = wishdir.Normalize ();

	// clamp to server defined max speed
	if (wishspeed > pm_maxspeed)
	{
		wishvel.Scale (pm_maxspeed/wishspeed);
		wishspeed = pm_maxspeed;
	}


	float currentspeed = dot(pml.velocity, wishdir);
	float addspeed = wishspeed - currentspeed;
	if (addspeed <= 0)
		return;
	float accelspeed = pm_accelerate * pml.frametime * wishspeed;
	if (accelspeed > addspeed)
		accelspeed = addspeed;

	for (i = 0; i < 3; i++)
		pml.velocity[i] += accelspeed * wishdir[i];

	if (doclip)
	{
		CVec3 end;
		for (i = 0; i < 3; i++)
			end[i] = pml.origin[i] + pml.frametime * pml.velocity[i];
		trace_t trace;
		PM_Trace (trace, pml.origin, end, pm->bounds);
		pml.origin = trace.endpos;
	}
	else
		VectorMA (pml.origin, pml.frametime, pml.velocity);
}


/*
==============
CheckDuck

Sets mins, maxs, and pm->viewheight
==============
*/
/* View parameters:
--------------------+-------------------------------+-------------------------------+
	name			|	Quake 2						|	Quake 3						|
--------------------+-------------------------------+-------------------------------+
DEAD_VIEWHEIGHT		|	== crouch					|	-16							|
CROUCH_VIEWHEIGHT	|	-2							|	12							|
DEFAULT_VIEWHEIGHT	|	22							|	26							|
BOX					| {-16,-16,-24} - {16,16,MAXZ}	| {-15,-15,-24} - {15,15,MAXZ}	|
MAXZ_DEAD			|	== crouch					|	8							|
MAXZ_DUCKED			|	4							|	16							|
MAXD_NORMAL			|	32							|	32							|
--------------------+-------------------------------+-------------------------------+
*/

static void CheckDuck ()
{
	pm->bounds.mins[0] = -16;
	pm->bounds.mins[1] = -16;

	pm->bounds.maxs[0] = 16;
	pm->bounds.maxs[1] = 16;

	if (pm->s.pm_type == PM_GIB)
	{
		pm->bounds.mins[2] = 0;
		pm->bounds.maxs[2] = 16;
		pm->viewheight = 8;
		return;
	}

	pm->bounds.mins[2] = -24;

	if (pm->s.pm_type == PM_DEAD)
		pm->s.pm_flags |= PMF_DUCKED;
	else if (pm->cmd.upmove < 0 && (pm->s.pm_flags & PMF_ON_GROUND))
		pm->s.pm_flags |= PMF_DUCKED;
	else
	{	// stand up if possible
		if (pm->s.pm_flags & PMF_DUCKED)
		{
			// try to stand up
			pm->bounds.maxs[2] = 32;
			trace_t trace;
			PM_Trace (trace, pml.origin, pml.origin, pm->bounds);
			if (!trace.allsolid)
				pm->s.pm_flags &= ~PMF_DUCKED;
		}
	}

	if (pm->s.pm_flags & PMF_DUCKED)
	{
		pm->bounds.maxs[2] = 4;		// KP: 24, Q2: 4
		pm->viewheight = -2;		// KP: 18, Q2: -2
	}
	else
	{
		pm->bounds.maxs[2] = 32;	// KP: 48, Q2: 32
		pm->viewheight = 22;		// KP: 36, Q2: 22
	}
}


static void DeadMove ()
{
	if (!pm->groundentity) return;

	float forward = pml.velocity.GetLength ();
	// extra friction
	if (forward <= 20)
		pml.velocity.Zero();
	else
		pml.velocity.Scale ((forward - 20) / forward);
}


static bool GoodPosition ()
{
	if (pm->s.pm_type == PM_SPECTATOR) return true;

	CVec3	v;
	for (int i = 0; i < 3; i++)
		v[i] = pm->s.origin[i] / 8.0f;
	trace_t trace;
	PM_Trace (trace, v, v, pm->bounds);

	return !trace.allsolid;
}

/*
================
SnapPosition

On exit, the origin will have a value that is pre-quantized to the 0.125
precision of the network channel and in a valid position.
================
*/
static void SnapPosition ()
{
#if OLD_PMOVE
	int		sign[3];
	int		i, j, bits;
	short	base[3];
	// try all single bits first
	static const byte jitterbits[8] = {0,4,1,2,3,5,6,7};

	// snap velocity to eigths
	for (i = 0; i < 3; i++)
		pm->s.velocity[i] = (int)(pml.velocity[i] * 8);

	for (i = 0; i < 3; i++)
	{
		sign[i] = (pml.origin[i] >= 0) ? 1 : -1;
		pm->s.origin[i] = (int)(pml.origin[i] * 8);
		if (pm->s.origin[i] / 8.0f == pml.origin[i])
			sign[i] = 0;
	}
	base[0] = pm->s.origin[0];
	base[1] = pm->s.origin[1];
	base[2] = pm->s.origin[2];

	// try all combinations
	for (j = 0; j < 8; j++)
	{
		bits = jitterbits[j];
		for (i = 0; i < 3; i++)
			if (bits & (1<<i))
				pm->s.origin[i] = base[i] + sign[i];
			else
				pm->s.origin[i] = base[i];

		if (GoodPosition ())
			return;
	}
#else
	int		i, j;
	CVec3	v;

	for (i = 0; i < 3; i++)
	{
		pm->s.origin[i] = appRound (pml.origin[i] * 8);
		pm->s.velocity[i] = appRound (pml.velocity[i] * 8);
	}
	if (pm->s.pm_type == PM_SPECTATOR) return;

//if (pml.velocity[0] || pml.velocity[1] || pml.velocity[2])//!!
//appPrintf("g:%d org: %g %g %g (pr: %d %d %d)  sp: %g %g %g\n", (pm->s.pm_flags & PMF_ON_GROUND) != 0,
//VECTOR_ARG(pml.origin),VECTOR_ARG(pml.previous_origin),VECTOR_ARG(pml.velocity));//!!

	int delta[3];
	delta[0] = delta[1] = delta[2] = 0;
	for (i = 0; i < 5; i++)
	{
		for (j = 0; j < 3; j++) v[j] = pm->s.origin[j] / 8.0f;
		trace_t trace;
		PM_Trace (trace, v, v, pm->bounds);
		if (!trace.allsolid)
		{
			for (j = 0; j < 3; j++)
			{
				if (!delta[j]) continue;
				if (pm->s.origin[j] != pml.previous_origin[j]) continue;
				if ((delta[j] > 0 && pm->s.velocity[j] > 0) ||
					(delta[j] < 0 && pm->s.velocity[j] < 0)) continue;
				pm->s.velocity[j] = 0;
			}
			return;
		}
#define STEP(n)								\
		if (trace.plane.normal[n] < -0.2) {	\
			pm->s.origin[n]--;				\
			delta[n]--;						\
		} else if (trace.plane.normal[n] > 0.2) {\
			pm->s.origin[n]++;				\
			delta[n]++;						\
		}
		STEP(0); STEP(1); STEP(2);
//		appPrintf("#%d(%g %g %g)->(%d %d %d)\n",i,VECTOR_ARG(tr.plane.normal),VECTOR_ARG(pm->s.origin));//!!
	}
//	appPrintf(S_RED"***\n");//!!

#endif

//	appPrintf("BAD!\n");//!!
	// go back to the last position
	pm->s.origin[0] = pml.previous_origin[0];
	pm->s.origin[1] = pml.previous_origin[1];
	pm->s.origin[2] = pml.previous_origin[2];
}


//?? combine ...SnapPosition() funcs, +GoodPosition()
static void InitialSnapPosition()
{
	static const int offset[3] = {0, -1, 1};

	short base[3];
	base[0] = pm->s.origin[0];
	base[1] = pm->s.origin[1];
	base[2] = pm->s.origin[2];

	for (int z = 0; z < 3; z++)
	{
		pm->s.origin[2] = base[2] + offset[z];
		for (int y = 0; y < 3; y++)
		{
			pm->s.origin[1] = base[1] + offset[y];
			for (int x = 0; x < 3; x++)
			{
				pm->s.origin[0] = base[0] + offset[x];
				if (GoodPosition ())
				{
					pml.origin[0] = pm->s.origin[0] / 8.0f;
					pml.origin[1] = pm->s.origin[1] / 8.0f;
					pml.origin[2] = pm->s.origin[2] / 8.0f;
					pml.previous_origin[0] = pm->s.origin[0];
					pml.previous_origin[1] = pm->s.origin[1];
					pml.previous_origin[2] = pm->s.origin[2];
					return;
				}
			}
		}
	}

	Com_DPrintf ("Bad InitialSnapPosition\n");
}


static void ClampAngles ()
{
	if (pm->s.pm_flags & PMF_TIME_TELEPORT)
	{
		pm->viewangles[YAW]   = SHORT2ANGLE(pm->cmd.angles[YAW] + pm->s.delta_angles[YAW]);
		pm->viewangles[PITCH] = 0;
		pm->viewangles[ROLL]  = 0;
	}
	else
	{
		// circularly clamp the angles with deltas
		for (int i = 0; i < 3; i++)
		{
			short temp = pm->cmd.angles[i] + pm->s.delta_angles[i];
			pm->viewangles[i] = SHORT2ANGLE(temp);
		}

		// don't let the player look up or down more than 90 degrees
		if (pm->viewangles[PITCH] > 89 && pm->viewangles[PITCH] < 180)
			pm->viewangles[PITCH] = 89;
		else if (pm->viewangles[PITCH] < 271 && pm->viewangles[PITCH] >= 180)
			pm->viewangles[PITCH] = 271;
	}
	Euler2Vecs (pm->viewangles, &pml.forward, &pml.right, &pml.up);
}


// Can be called by either the server or the client
void Pmove (pmove_t *pmove)
{
	guard(Pmove);

#if PM_DEBUG
	SV_DrawTextLeft ("******** PMOVE ********", RGB(1,1,0.2));
#endif

	pm = pmove;

	// clear results
	pm->numtouch     = 0;
	pm->viewangles.Zero();
	pm->viewheight   = 0;
	pm->groundentity = NULL;
	pm->watertype    = 0;
	pm->waterlevel   = 0;

	// clear all pmove local vars
	memset (&pml, 0, sizeof(pml));

	// convert origin and velocity to float values
	pml.origin[0] = pm->s.origin[0] * 0.125f;
	pml.origin[1] = pm->s.origin[1] * 0.125f;
	pml.origin[2] = pm->s.origin[2] * 0.125f;

	pml.velocity[0] = pm->s.velocity[0] * 0.125f;
	pml.velocity[1] = pm->s.velocity[1] * 0.125f;
	pml.velocity[2] = pm->s.velocity[2] * 0.125f;

	// save old org in case we get stuck
	pml.previous_origin[0] = pm->s.origin[0];
	pml.previous_origin[1] = pm->s.origin[1];
	pml.previous_origin[2] = pm->s.origin[2];

	pml.frametime = pm->cmd.msec * 0.001f;
#if PM_DEBUG
	SV_DrawTextLeft (va("org={%g %g %g} vel={%g %g %g}", VECTOR_ARG(pml.origin), VECTOR_ARG(pml.velocity)), RGB(0,1,1));
#endif

	ClampAngles ();

	if (pm->s.pm_type == PM_SPECTATOR)
	{
		FlyMove (false);
		SnapPosition ();
		return;
	}

	if (pm->s.pm_type >= PM_DEAD)
	{
		pm->cmd.forwardmove = 0;
		pm->cmd.sidemove    = 0;
		pm->cmd.upmove      = 0;
	}

	if (pm->s.pm_type == PM_FREEZE)
		return;		// no movement at all

	// set mins, maxs, and viewheight
	CheckDuck ();

	if (pm->snapinitial)
		InitialSnapPosition ();

	// set groundentity, watertype, and waterlevel
	CatagorizePosition ();

	if (pm->s.pm_type == PM_DEAD)
		DeadMove ();

	CheckSpecialMovement ();

	// drop timing counter
	if (pm->s.pm_time)
	{
		int quant = pm->cmd.msec >> 3;		// each unit is 8ms
		if (!quant) quant = 1;
		if (quant >= pm->s.pm_time)
		{
			pm->s.pm_flags &= ~(PMF_TIME_WATERJUMP|PMF_TIME_LAND|PMF_TIME_TELEPORT);
			pm->s.pm_time = 0;
		}
		else
			pm->s.pm_time -= quant;
	}

	if (pm->s.pm_flags & PMF_TIME_TELEPORT)
	{	// teleport pause stays exactly in place
	}
	else if (pm->s.pm_flags & PMF_TIME_WATERJUMP)
	{	// waterjump has no control, but falls
		pml.velocity[2] -= pm->s.gravity * pml.frametime;
		if (pml.velocity[2] < 0)
		{	// cancel as soon as we are falling down again
			pm->s.pm_flags &= ~(PMF_TIME_WATERJUMP|PMF_TIME_LAND|PMF_TIME_TELEPORT);
			pm->s.pm_time = 0;
		}

		StepSlideMove ();
	}
	else
	{
		CheckJump ();
		Friction ();

		if (pm->waterlevel >= 2)
			WaterMove ();
		else
		{
			CVec3 angles = pm->viewangles;
			if (angles[PITCH] > 180)
				angles[PITCH] -= 360;
			angles[PITCH] /= 3;
			Euler2Vecs (angles, &pml.forward, &pml.right, &pml.up);

			AirMove ();
		}
	}

	// set groundentity, watertype, and waterlevel for final spot
	CatagorizePosition ();
	SnapPosition ();
#if PM_DEBUG
	CVec3 org, vel;
	for (int i = 0; i < 3; i++)
	{
		org[i] = pmove->s.origin[i] / 8.0f;
		vel[i] = pmove->s.velocity[i] / 8.0f;
	}
	SV_DrawTextLeft (va("org={%g %g %g} vel={%g %g %g}", VECTOR_ARG(org), VECTOR_ARG(vel)), RGB(0,1,1));
	SV_DrawTextLeft (va("type=%d water=%d groundEnt=$%X touch=%d flags=$%X",
		pmove->s.pm_type, pmove->waterlevel, pmove->groundentity, pmove->numtouch, pmove->s.pm_flags), RGB(0.8,0.8,0));
#endif

	unguard;
}
