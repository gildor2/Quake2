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



#define	STEPSIZE	18

// all of the locals will be zeroed before each
// pmove, just to make damn sure we don't have
// any differences when running on client or server

typedef struct
{
	vec3_t		origin;			// full float precision
	vec3_t		velocity;		// full float precision

	vec3_t		forward, right, up;
	float		frametime;


	csurface_t	*groundsurface;
	cplane_t	groundplane;
	int			groundcontents;

	short		previous_origin[3];

	qboolean	ladder;
	cplane_t	ladderPlane;
} pml_t;

static pmove_t		*pm;
static pml_t		pml;


// movement parameters
static float pm_stopspeed = 100;
static float pm_maxspeed = 300;
static float pm_duckspeed = 100;
static float pm_accelerate = 10;
static float pm_wateraccelerate = 10;
static float pm_friction = 6;
static float pm_waterfriction = 1;
static float pm_waterspeed = 400;
float	pm_airaccelerate = 0;


/*
==================
ClipVelocity

Slide off of the impacting object
returns the blocked flags (1 = floor, 2 = step / wall)
==================
*/
#define	STOP_EPSILON	0.1

static void ClipVelocity (vec3_t in, vec3_t normal, vec3_t out, float overbounce)
{
	float	backoff;
	int		i;

	backoff = DotProduct (in, normal) * overbounce;
	VectorMA (in, -backoff, normal, out);

	for (i = 0; i < 3; i++)
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

static qboolean SlideMove (void)
{
	int			bumpcount, numbumps;
	vec3_t		dir;
	float		d;
	int			numplanes;
	vec3_t		planes[MAX_CLIP_PLANES];
	vec3_t		primal_velocity;
	int			i, j;
	trace_t		trace;
	vec3_t		end;
	float		time_left;

	numbumps = 4;

	VectorCopy (pml.velocity, primal_velocity);
#if 0
	//!! OLD
	numplanes = 0;
#else
	if (pm->s.pm_flags & PMF_ON_GROUND)
	{
		VectorCopy (pml.groundplane.normal, planes[0]);
		numplanes = 1;
	}
	else
		numplanes = 0;
#endif

	time_left = pml.frametime;

	for (bumpcount = 0; bumpcount < numbumps; bumpcount++)
	{
		VectorMA (pml.origin, time_left, pml.velocity, end);
		trace = pm->trace (pml.origin, pm->mins, pm->maxs, end);

		if (trace.allsolid)
		{	// entity is trapped in another solid
			pml.velocity[2] = 0;	// don't build up falling damage
			return true;
		}

		if (trace.fraction > 0)
		{	// actually covered some distance
			VectorCopy (trace.endpos, pml.origin);
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
			VectorClear (pml.velocity);
			return true;
		}

		VectorCopy (trace.plane.normal, planes[numplanes]);
		numplanes++;

#if 1
		// modify original velocity so it parallels all of the clip planes
		for (i = 0; i < numplanes; i++)
		{
			if (DotProduct (pml.velocity, planes[i]) >= 0) continue;
			ClipVelocity (pml.velocity, planes[i], pml.velocity, OVERBOUNCE);

			for (j = 0; j < numplanes; j++)
			{
				if (j == i) continue;
				if (DotProduct (pml.velocity, planes[j]) >= 0) continue;

				ClipVelocity (pml.velocity, planes[j], pml.velocity, OVERBOUNCE);
				// check for reentering plane[i] after clip against plane[j]
				if (DotProduct (pml.velocity, planes[i]) >= 0) continue;

				// go along the crease
				CrossProduct (planes[i], planes[j], dir);
				VectorNormalizeFast (dir);
				d = DotProduct (dir, pml.velocity);
				VectorScale (dir, d, pml.velocity);
			}

			break;
		}
#else
		//!! OLD
		// modify original_velocity so it parallels all of the clip planes
		for (i = 0; i < numplanes; i++)
		{
			ClipVelocity (pml.velocity, planes[i], pml.velocity, 1.01);
			for (j = 0; j < numplanes; j++)
				if (j != i)
				{
					if (DotProduct (pml.velocity, planes[j]) < 0)
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
//				Com_Printf ("clip velocity, numplanes == %d\n",numplanes);
				VectorCopy (vec3_origin, pml.velocity);
				break;
			}
			CrossProduct (planes[0], planes[1], dir);
			d = DotProduct (dir, pml.velocity);
			VectorScale (dir, d, pml.velocity);
		}
#endif

		// if velocity is against the original velocity, stop dead
		// to avoid tiny occilations in sloping corners
		if (DotProduct (pml.velocity, primal_velocity) <= 0)
		{
			VectorClear (pml.velocity);
			break;
		}
	}

	if (pm->s.pm_time)
		VectorCopy (primal_velocity, pml.velocity);

	return (bumpcount > 0) || trace.fraction < 1;
}


/*
==================
StepSlideMove
==================
*/
static void StepSlideMove (void)
{
	vec3_t		start_o, start_v;
	vec3_t		down_o, down_v;
	trace_t		trace;
	float		stepHeight, down_dist, up_dist;
	vec3_t		up, down;

	VectorCopy (pml.origin, start_o);
	VectorCopy (pml.velocity, start_v);

#if 0
	if (!SlideMove ()) return;
#else
	SlideMove ();
#endif

	VectorCopy (pml.origin, down_o);
	VectorCopy (pml.velocity, down_v);

	VectorCopy (start_o, up);
	up[2] += STEPSIZE;

#if 1
	trace = pm->trace (start_o, pm->mins, pm->maxs, up);
	if (trace.allsolid)
	{
//		Com_Printf("no up\n");//!!
		return;		// can't step up
	}

	// try sliding above
	stepHeight = trace.endpos[2] - start_o[2];
	VectorCopy (trace.endpos, pml.origin);
	VectorCopy (start_v, pml.velocity);
#else
	//!! OLD
	trace = pm->trace (up, pm->mins, pm->maxs, up);
	if (trace.allsolid)
		return;		// can't step up

	stepHeight = STEPSIZE;
	VectorCopy (up, pml.origin);
	VectorCopy (start_v, pml.velocity);
#endif

//Com_Printf("^2 up ");//!!
	SlideMove ();

	// push down the final amount
	VectorCopy (pml.origin, down);
	down[2] -= stepHeight;
	trace = pm->trace (pml.origin, pm->mins, pm->maxs, down);
	if (!trace.allsolid)
		VectorCopy (trace.endpos, pml.origin);

	VectorCopy (pml.origin, up);

	// decide which one went farther
	down_dist = (down_o[0] - start_o[0]) * (down_o[0] - start_o[0])
			  + (down_o[1] - start_o[1]) * (down_o[1] - start_o[1]);
	up_dist = (up[0] - start_o[0]) * (up[0] - start_o[0])
			+ (up[1] - start_o[1]) * (up[1] - start_o[1]);

	if (down_dist > up_dist || trace.plane.normal[2] < MIN_STEP_NORMAL)
	{
//		Com_Printf("down: d_d=%g u_d=%g n2=%g\n", down_dist, up_dist, trace.plane.normal[2]);//!!
		VectorCopy (down_o, pml.origin);
		VectorCopy (down_v, pml.velocity);
		return;
	}
	//!! Special case
	// if we were walking along a plane, then we need to copy the Z over
	pml.velocity[2] = down_v[2];
}


/*
==================
Friction

Handles both ground friction and water friction
==================
*/
static void Friction (void)
{
	float	*vel;
	float	speed, newspeed, control;
	float	friction;
	float	drop;

	vel = pml.velocity;

	speed = vel[0]*vel[0] + vel[1]*vel[1] + vel[2]*vel[2];		// this is (speed*speed)
	if (speed < 1)
	{
		vel[0] = 0;
		vel[1] = 0;
		return;
	}

	speed = sqrt (speed);	// finish speed calculation
	drop = 0;

	// apply ground friction
	if ((pm->groundentity && pml.groundsurface && !(pml.groundsurface->flags & SURF_SLICK)) || pml.ladder)
	{
		friction = pm_friction;
		control = speed < pm_stopspeed ? pm_stopspeed : speed;
		drop += control * friction * pml.frametime;
	}

	// apply water friction
	if (pm->waterlevel && !pml.ladder)
		drop += speed * pm_waterfriction * pm->waterlevel * pml.frametime;

	// scale the velocity
	newspeed = speed - drop;
	if (newspeed < 0)
		newspeed = 0;
	else
		newspeed /= speed;

	vel[0] = vel[0] * newspeed;
	vel[1] = vel[1] * newspeed;
	vel[2] = vel[2] * newspeed;
}


/*
==============
Accelerate

Handles user intended acceleration
==============
*/
static void Accelerate (vec3_t wishdir, float wishspeed, float accel)
{
	int			i;
	float		addspeed, accelspeed, currentspeed;

	currentspeed = DotProduct (pml.velocity, wishdir);
	addspeed = wishspeed - currentspeed;
	if (addspeed <= 0)
		return;
	accelspeed = accel * pml.frametime * wishspeed;
	if (accelspeed > addspeed)
		accelspeed = addspeed;

	for (i = 0; i < 3; i++)
		pml.velocity[i] += accelspeed * wishdir[i];
}


static void AirAccelerate (vec3_t wishdir, float wishspeed, float accel)
{
	int			i;
	float		addspeed, accelspeed, currentspeed, wishspd = wishspeed;

	if (wishspd > 30)
		wishspd = 30;
	currentspeed = DotProduct (pml.velocity, wishdir);
	addspeed = wishspd - currentspeed;

	if (addspeed <= 0) return;

	accelspeed = accel * wishspeed * pml.frametime;
	if (accelspeed > addspeed)
		accelspeed = addspeed;

	for (i = 0; i < 3; i++)
		pml.velocity[i] += accelspeed * wishdir[i];
}


/*
=============
AddCurrents
=============
*/
static void AddCurrents (vec3_t wishvel)
{
	vec3_t	v;
	float	f;

	// account for ladders
	if (pml.ladder && pml.velocity[2] >= -200 && pml.velocity[2] <= 200)
	{
		if ((pm->viewangles[PITCH] >= 15 || pm->viewangles[PITCH] <= -15) && pm->cmd.forwardmove > 0)
		{
			float	y;

			y = -400 / 90 * pm->viewangles[PITCH];	// full speed when pitch angle is 45 grad.

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
				static vec3_t up = {0, 0, 1};
				vec3_t	axis[2], vel_vert;

				// compute projections of velocity onto ladder plane axis
				CrossProduct (pml.ladderPlane.normal, up, axis[0]);
				VectorNormalizeFast (axis[0]);
				CrossProduct (pml.ladderPlane.normal, axis[0], axis[1]);

				f = DotProduct (wishvel, axis[1]);
				VectorScale (axis[1], f, vel_vert);

				// limit horizontal component of speed
				//!! NOTE: SlideMove() may add its own horizontal speed
				if (pm->cmd.sidemove == 0)
					f = 0;
				else
				{
					f = DotProduct (wishvel, axis[0]);
					f = bound(f, -LADDER_SIDE_SPEED, LADDER_SIDE_SPEED);
				}

				VectorMA (vel_vert, f, axis[0], wishvel);
			}
		}
	}

	// add water currents
	if (pm->watertype & MASK_CURRENT)
	{
		VectorClear (v);
		if (pm->watertype & CONTENTS_CURRENT_0)		v[0] = 1;
		if (pm->watertype & CONTENTS_CURRENT_180)	v[0] = -1;
		if (pm->watertype & CONTENTS_CURRENT_90)	v[1] = 1;
		if (pm->watertype & CONTENTS_CURRENT_270)	v[1] = -1;
		if (pm->watertype & CONTENTS_CURRENT_UP) 	v[2] = 1;
		if (pm->watertype & CONTENTS_CURRENT_DOWN)	v[2] = -1;

		f = pm_waterspeed;
		if ((pm->waterlevel == 1) && (pm->groundentity))
			f /= 2;

		VectorMA (wishvel, f, v, wishvel);
	}

	// add conveyor belt velocities
	if (pm->groundentity && (pml.groundcontents & MASK_CURRENT))
	{
		VectorClear (v);
		if (pml.groundcontents & CONTENTS_CURRENT_0)	v[0] = 1;
		if (pml.groundcontents & CONTENTS_CURRENT_180)	v[0] = -1;
		if (pml.groundcontents & CONTENTS_CURRENT_90)	v[1] = 1;
		if (pml.groundcontents & CONTENTS_CURRENT_270)	v[1] = -1;
		if (pml.groundcontents & CONTENTS_CURRENT_UP) 	v[2] = 1;
		if (pml.groundcontents & CONTENTS_CURRENT_DOWN)	v[2] = -1;

		VectorMA (wishvel, 100 /* pm->groundentity->speed */, v, wishvel);
	}
}


/*
===================
WaterMove
===================
*/
static void WaterMove (void)
{
	int		i;
	vec3_t	wishvel;
	float	wishspeed;
	vec3_t	wishdir;

	// user intentions
	for (i = 0; i < 3; i++)
		wishvel[i] = pml.forward[i]*pm->cmd.forwardmove + pml.right[i]*pm->cmd.sidemove;

	if (!pm->cmd.forwardmove && !pm->cmd.sidemove && !pm->cmd.upmove)
		wishvel[2] -= 60;		// drift towards bottom
	else
		wishvel[2] += pm->cmd.upmove;

	AddCurrents (wishvel);

	VectorCopy (wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);

	if (wishspeed > pm_maxspeed)
	{
		VectorScale (wishvel, pm_maxspeed/wishspeed, wishvel);
		wishspeed = pm_maxspeed;
	}
	wishspeed *= 0.5;

	Accelerate (wishdir, wishspeed, pm_wateraccelerate);

	StepSlideMove ();
}


/*
===================
AirMove
===================
*/
static void AirMove (void)
{
	int		i;
	float	fmove, smove;
	vec3_t	wishvel, wishdir;
	float	wishspeed, maxspeed;

	fmove = pm->cmd.forwardmove;
	smove = pm->cmd.sidemove;
	if (pml.ladder && fmove < 0 && !(pm->s.pm_flags & PMF_ON_GROUND))
		fmove = 0;			// disable backward move when on a ladder

//!!!!! pitch should be 1/3 so this isn't needed??!
#if 0
	pml.forward[2] = 0;
	pml.right[2] = 0;
	VectorNormalize (pml.forward);
	VectorNormalize (pml.right);
#endif

	for (i = 0; i < 2; i++)
		wishvel[i] = pml.forward[i]*fmove + pml.right[i]*smove;
	wishvel[2] = 0;
//Com_Printf("v1(%g|%g|%g)",VECTOR_ARG(wishvel));//!!

	AddCurrents (wishvel);
	wishspeed = VectorNormalize2 (wishvel, wishdir);

	// clamp to server defined max speed
	maxspeed = (pm->s.pm_flags & PMF_DUCKED) ? pm_duckspeed : pm_maxspeed;

	if (wishspeed > maxspeed)
	{
		VectorScale (wishvel, maxspeed/wishspeed, wishvel);
		wishspeed = maxspeed;
	}

	if (pml.ladder)
	{
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
	{	// walking on ground
		pml.velocity[2] = 0; //!!! this is before the accel
		Accelerate (wishdir, wishspeed, pm_accelerate);

		if(pm->s.gravity > 0)
			pml.velocity[2] = 0;
		else
			pml.velocity[2] -= pm->s.gravity * pml.frametime;

		if (!pml.velocity[0] && !pml.velocity[1])
			return;
		StepSlideMove ();
	}
	else
	{	// not on ground, so little effect on velocity
//Com_Printf("v2(%g|%g|%g)*%g ",VECTOR_ARG(wishdir),wishspeed);//!!
		if (pm_airaccelerate)
			AirAccelerate (wishdir, wishspeed, pm_accelerate);
		else
			Accelerate (wishdir, wishspeed, 1);
//Com_Printf("vel0(%g|%g|%g) o0(%g|%g|%g)\n",VECTOR_ARG(pml.velocity),VECTOR_ARG(pml.origin));//!!
		// add gravity
		pml.velocity[2] -= pm->s.gravity * pml.frametime;
		StepSlideMove ();
//Com_Printf("tm=%g vel1(%g|%g|%g) o1(%g|%g|%g)\n",pml.frametime, VECTOR_ARG(pml.velocity),VECTOR_ARG(pml.origin));//!!
	}
//Com_Printf("\n");//!!
}



/*
=============
CatagorizePosition
=============
*/
static void CatagorizePosition (void)
{
	vec3_t		point;
	int			cont;
	trace_t		trace;
	float		sample1, sample2;

	// if the player hull point one unit down is solid, the player
	// is on ground

	// see if standing on something solid
	point[0] = pml.origin[0];
	point[1] = pml.origin[1];
	point[2] = pml.origin[2] - 0.25f;

	if (pml.velocity[2] > 180) //ZOID changed from 100 to 180 (ramp accel)
	{	// NOTE: if disable this code, jumppads will be VERY buggy; and waterjump will be very high
		pm->s.pm_flags &= ~PMF_ON_GROUND;
		pm->groundentity = NULL;
	}
	else
	{
		trace = pm->trace (pml.origin, pm->mins, pm->maxs, point);
		pml.groundplane = trace.plane;
		pml.groundsurface = trace.surface;
		pml.groundcontents = trace.contents;

//		Com_Printf("gr: n2=%g frac=%g sol=%d\n", trace.plane.normal[2], trace.fraction, trace.allsolid);//!!
//		if (trace.allsolid) Com_WPrintf("locked: %g %g %g\n", VECTOR_ARG(point));
		if (!trace.ent || (trace.plane.normal[2] < 0.7 && !trace.startsolid))
		{
			pm->groundentity = NULL;
			pm->s.pm_flags &= ~PMF_ON_GROUND;
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
		}
	}

	// get waterlevel, accounting for ducking
	pm->waterlevel = 0;
	pm->watertype = 0;

	sample2 = pm->viewheight - pm->mins[2];
	sample1 = sample2 / 2;

	point[2] = pml.origin[2] + pm->mins[2] + 1;
	cont = pm->pointcontents (point);
	if (cont & MASK_WATER)
	{
		pm->watertype = cont;
		pm->waterlevel = 1;
		point[2] = pml.origin[2] + pm->mins[2] + sample1;
		cont = pm->pointcontents (point);
		if (cont & MASK_WATER)
		{
			pm->waterlevel = 2;
			point[2] = pml.origin[2] + pm->mins[2] + sample2;
			cont = pm->pointcontents (point);
			if (cont & MASK_WATER)
				pm->waterlevel = 3;
		}
	}
}


/*
=============
CheckJump
=============
*/
static void CheckJump (void)
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


/*
=============
CheckSpecialMovement
=============
*/
static void CheckSpecialMovement (void)
{
	vec3_t	spot;
	vec3_t	flatforward;
	trace_t	trace;
	float	dist;

	if (pm->s.pm_time) return;

	pml.ladder = false;

	// check for ladder
	flatforward[0] = pml.forward[0];
	flatforward[1] = pml.forward[1];
	flatforward[2] = 0;
	VectorNormalize (flatforward);

	if (pm->cmd.upmove < 0)
		dist = 30; // when crouched - try to detect ladders on a longer distances
	else
		dist = 1;
	VectorMA (pml.origin, dist, flatforward, spot);
	trace = pm->trace (pml.origin, pm->mins, pm->maxs, spot);
	if ((trace.fraction < 1) && (trace.contents & CONTENTS_LADDER))
	{
		pml.ladder = true;
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


/*
===============
FlyMove
===============
*/
static void FlyMove (qboolean doclip)
{
	float	speed, drop, friction, control, newspeed;
	float	currentspeed, addspeed, accelspeed;
	int		i;
	vec3_t	wishvel;
	float	fmove, smove;
	vec3_t	wishdir;
	float	wishspeed;
	vec3_t	end;
	trace_t	trace;

	pm->viewheight = 22;

	// friction
	speed = VectorLength (pml.velocity);
	if (speed < 1)
		VectorClear (pml.velocity);
	else
	{
		drop = 0;

		friction = pm_friction * 1.5f;	// extra friction
		control = speed < pm_stopspeed ? pm_stopspeed : speed;
		drop += control * friction * pml.frametime;

		// scale the velocity
		newspeed = speed - drop;
		if (newspeed < 0)
			newspeed = 0;
		else
			newspeed /= speed;

		VectorScale (pml.velocity, newspeed, pml.velocity);
	}

	// accelerate
	fmove = pm->cmd.forwardmove;
	smove = pm->cmd.sidemove;

	VectorNormalize (pml.forward);
	VectorNormalize (pml.right);

	for (i = 0; i < 3; i++)
		wishvel[i] = pml.forward[i] * fmove + pml.right[i] * smove;
	wishvel[2] += pm->cmd.upmove;

	VectorCopy (wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);

	// clamp to server defined max speed
	if (wishspeed > pm_maxspeed)
	{
		VectorScale (wishvel, pm_maxspeed/wishspeed, wishvel);
		wishspeed = pm_maxspeed;
	}


	currentspeed = DotProduct(pml.velocity, wishdir);
	addspeed = wishspeed - currentspeed;
	if (addspeed <= 0)
		return;
	accelspeed = pm_accelerate * pml.frametime * wishspeed;
	if (accelspeed > addspeed)
		accelspeed = addspeed;

	for (i = 0; i < 3; i++)
		pml.velocity[i] += accelspeed * wishdir[i];

	if (doclip)
	{
		for (i = 0; i < 3; i++)
			end[i] = pml.origin[i] + pml.frametime * pml.velocity[i];

		trace = pm->trace (pml.origin, pm->mins, pm->maxs, end);

		VectorCopy (trace.endpos, pml.origin);
	}
	else
	{
		// move
		VectorMA (pml.origin, pml.frametime, pml.velocity, pml.origin);
	}
}


/*
==============
CheckDuck

Sets mins, maxs, and pm->viewheight
==============
*/
static void CheckDuck (void)
{
	trace_t	trace;

	pm->mins[0] = -16;
	pm->mins[1] = -16;

	pm->maxs[0] = 16;
	pm->maxs[1] = 16;

	if (pm->s.pm_type == PM_GIB)
	{
		pm->mins[2] = 0;
		pm->maxs[2] = 16;
		pm->viewheight = 8;
		return;
	}

	pm->mins[2] = -24;

	if (pm->s.pm_type == PM_DEAD)
	{
		pm->s.pm_flags |= PMF_DUCKED;
	}
	else if (pm->cmd.upmove < 0 && (pm->s.pm_flags & PMF_ON_GROUND))
	{	// duck
		pm->s.pm_flags |= PMF_DUCKED;
	}
	else
	{	// stand up if possible
		if (pm->s.pm_flags & PMF_DUCKED)
		{
			// try to stand up
			pm->maxs[2] = 32;
			trace = pm->trace (pml.origin, pm->mins, pm->maxs, pml.origin);
			if (!trace.allsolid)
				pm->s.pm_flags &= ~PMF_DUCKED;
		}
	}

	if (pm->s.pm_flags & PMF_DUCKED)
	{
		pm->maxs[2] = 4;		// KP: 24, Q2: 4
		pm->viewheight = -2;	// KP: 18, Q2: -2
	}
	else
	{
		pm->maxs[2] = 32;		// KP: 48, Q2: 32
		pm->viewheight = 22;	// KP: 36, Q2: 22
	}
}


/*
==============
DeadMove
==============
*/
static void DeadMove (void)
{
	float	forward;

	if (!pm->groundentity)
		return;

	// extra friction

	forward = VectorLength (pml.velocity);
	forward -= 20;
	if (forward <= 0)
	{
		VectorClear (pml.velocity);
	}
	else
	{
		VectorNormalize (pml.velocity);
		VectorScale (pml.velocity, forward, pml.velocity);
	}
}


static qboolean GoodPosition (void)
{
	trace_t	tr;
	vec3_t	v;
	int		i;

	if (pm->s.pm_type == PM_SPECTATOR)
		return true;

	for (i = 0; i < 3; i++)
		v[i] = pm->s.origin[i] / 8.0f;
	tr = pm->trace (v, pm->mins, pm->maxs, v);

	return !tr.allsolid;
}

/*
================
SnapPosition

On exit, the origin will have a value that is pre-quantized to the 0.125
precision of the network channel and in a valid position.
================
*/
static void SnapPosition (void)
{
#if 1
	int		i, j;
	vec3_t	v;
	int		delta[3];

	for (i = 0; i < 3; i++)
	{
		pm->s.origin[i] = Q_round (pml.origin[i] * 8);
		pm->s.velocity[i] = Q_round (pml.velocity[i] * 8);
	}
	if (pm->s.pm_type == PM_SPECTATOR) return;

//if (pml.velocity[0] || pml.velocity[1] || pml.velocity[2])//!!
//Com_Printf("g:%d org: %g %g %g (pr: %d %d %d)  sp: %g %g %g\n", (pm->s.pm_flags & PMF_ON_GROUND) != 0,
//VECTOR_ARG(pml.origin),VECTOR_ARG(pml.previous_origin),VECTOR_ARG(pml.velocity));//!!

	delta[0] = delta[1] = delta[2] = 0;
	for (i = 0; i < 5; i++)
	{
		trace_t tr;

		for (j = 0; j < 3; j++) v[j] = pm->s.origin[j] / 8.0f;
		tr = pm->trace (v, pm->mins, pm->maxs, v);
		if (!tr.allsolid)
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
		if (tr.plane.normal[n] < -0.2) {	\
			pm->s.origin[n]--;				\
			delta[n]--;						\
		} else if (tr.plane.normal[n] > 0.2) {\
			pm->s.origin[n]++;				\
			delta[n]++;						\
		}
		STEP(0); STEP(1); STEP(2);
//		Com_Printf("#%d(%g %g %g)->(%d %d %d)\n",i,VECTOR_ARG(tr.plane.normal),VECTOR_ARG(pm->s.origin));//!!
	}
//	Com_Printf("^1***\n");//!!

#else
	//!! OLD
	int		sign[3];
	int		i, j, bits;
	short	base[3];
	// try all single bits first
	static int jitterbits[8] = {0,4,1,2,3,5,6,7};

	// snap velocity to eigths
	for (i = 0; i < 3; i++)
		pm->s.velocity[i] = (int)(pml.velocity[i] * 8);

	for (i = 0; i < 3; i++)
	{
		if (pml.origin[i] >= 0)
			sign[i] = 1;
		else
			sign[i] = -1;
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
#endif

//	Com_Printf("BAD!\n");//!!
	// go back to the last position
	pm->s.origin[0] = pml.previous_origin[0];
	pm->s.origin[1] = pml.previous_origin[1];
	pm->s.origin[2] = pml.previous_origin[2];
}

/*
================
InitialSnapPosition
================
*/
//?? combine ...SnapPosition() funcs, +GoodPosition()
static void InitialSnapPosition(void)
{
	int        x, y, z;
	short      base[3];
	static int offset[3] = {0, -1, 1};

	base[0] = pm->s.origin[0];
	base[1] = pm->s.origin[1];
	base[2] = pm->s.origin[2];

	for (z = 0; z < 3; z++)
	{
		pm->s.origin[2] = base[2] + offset[z];
		for (y = 0; y < 3; y++)
		{
			pm->s.origin[1] = base[1] + offset[y];
			for (x = 0; x < 3; x++)
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


/*
================
ClampAngles
================
*/
static void ClampAngles (void)
{
	short	temp;
	int		i;

	if (pm->s.pm_flags & PMF_TIME_TELEPORT)
	{
		pm->viewangles[YAW] = SHORT2ANGLE(pm->cmd.angles[YAW] + pm->s.delta_angles[YAW]);
		pm->viewangles[PITCH] = 0;
		pm->viewangles[ROLL] = 0;
	}
	else
	{
		// circularly clamp the angles with deltas
		for (i = 0; i < 3; i++)
		{
			temp = pm->cmd.angles[i] + pm->s.delta_angles[i];
			pm->viewangles[i] = SHORT2ANGLE(temp);
		}

		// don't let the player look up or down more than 90 degrees
		if (pm->viewangles[PITCH] > 89 && pm->viewangles[PITCH] < 180)
			pm->viewangles[PITCH] = 89;
		else if (pm->viewangles[PITCH] < 271 && pm->viewangles[PITCH] >= 180)
			pm->viewangles[PITCH] = 271;
	}
	AngleVectors (pm->viewangles, pml.forward, pml.right, pml.up);
}

/*
================
Pmove

Can be called by either the server or the client
================
*/
void Pmove (pmove_t *pmove)
{
	guard(Pmove);

	pm = pmove;

	// clear results
	pm->numtouch = 0;
	VectorClear (pm->viewangles);
	pm->viewheight = 0;
	pm->groundentity = NULL;
	pm->watertype = 0;
	pm->waterlevel = 0;

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
		pm->cmd.sidemove = 0;
		pm->cmd.upmove = 0;
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
		int		quant;		// each unit is 8ms

		quant = pm->cmd.msec >> 3;
		if (!quant)
			quant = 1;
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
			vec3_t	angles;

			VectorCopy(pm->viewangles, angles);
			if (angles[PITCH] > 180)
				angles[PITCH] = angles[PITCH] - 360;
			angles[PITCH] /= 3;

			AngleVectors (angles, pml.forward, pml.right, pml.up);

			AirMove ();
		}
	}

	// set groundentity, watertype, and waterlevel for final spot
	CatagorizePosition ();

	SnapPosition ();

	unguard;
}
