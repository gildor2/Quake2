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

#include "client.h"


static float predictLerp = -1;		// >= 0 -- use this value for prediction; if < 0 -- use cl.lerpfrac


/*
===================
CL_CheckPredictionError
===================
*/
void CL_CheckPredictionError (void)
{
	int		frame;
	int		delta[3];
	int		i, len;

	if (!cl_predict->integer || (cl.frame.playerstate.pmove.pm_flags & PMF_NO_PREDICTION) || cl.attractloop)
		return;

	// calculate the last usercmd_t we sent that the server has processed
	frame = cls.netchan.incoming_acknowledged;
	frame &= (CMD_BACKUP-1);

	// compare what the server returned with what we had predicted it to be
	VectorSubtract (cl.frame.playerstate.pmove.origin, cl.predicted_origins[frame], delta);

	// save the prediction error for interpolation
	len = abs(delta[0]) + abs(delta[1]) + abs(delta[2]);	//?? VectorLength()
	if (len > 640)	// 80 world units
	{	// a teleport or something
		VectorClear (cl.prediction_error);
	}
	else
	{
		// save for error itnerpolation
		for (i = 0; i < 3; i++)
			cl.prediction_error[i] = delta[i] * 0.125f;

		if (cl_showmiss->integer && len)
			Com_WPrintf ("prediction miss on %d: %g %g %g\n", cl.frame.serverframe, VECTOR_ARGS(cl.prediction_error));

		cl.predicted_origins[frame][0] = cl.frame.playerstate.pmove.origin[0];
		cl.predicted_origins[frame][1] = cl.frame.playerstate.pmove.origin[1];
		cl.predicted_origins[frame][2] = cl.frame.playerstate.pmove.origin[2];
	}
}


/*
====================
CL_ClipMoveToEntities

====================
*/
void CL_ClipMoveToEntities (trace_t *tr, vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end)
{
	int		i, j;
	trace_t	trace;
	float	t, traceLen, traceWidth, b1, b2, frac;
	vec3_t	traceDir;

	b1 = DotProduct (mins, mins);
	b2 = DotProduct (maxs, maxs);
	t = b1 > b2 ? b1 : b2;
	traceWidth = SQRTFAST(t);
	VectorSubtract (end, start, traceDir);
	traceLen = VectorNormalize (traceDir) + traceWidth;

	for (i = 0; i < cl.frame.num_entities; i++)
	{
		entityState_t *ent;
		centity_t *cent;
		cmodel_t *cmodel;
		vec3_t	tmp, delta, eCenter, eOrigin;
		float	entPos, dist2, dist0;

		ent = &cl_parse_entities[(cl.frame.parse_entities + i) & (MAX_PARSE_ENTITIES-1)];
		if (!ent->solid) continue;
		if (ent->number == cl.playernum+1) continue;	// no clip with player entity

		// compute lerped entity position
		cent = &cl_entities[ent->number];
		frac = predictLerp;							// called CL_PredictMovement()
		if (frac < 0) frac = cl.lerpfrac;			// called from any other place
		for (j = 0; j < 3; j++)
			delta[j] = (1.0f - frac) * (cent->current.origin[j] - cent->prev.origin[j]);

		VectorSubtract (ent->center, start, eCenter);
		VectorSubtract (eCenter, delta, eCenter);
		VectorSubtract (ent->origin, delta, eOrigin);

//if (delta[0]||delta[1]||delta[2])//!!
//Com_Printf("(%g %g %g)->(%g %g %g) =(%g %g %g):%g\n",VECTOR_ARGS(cent->prev.origin),VECTOR_ARGS(cent->current.origin),VECTOR_ARGS(eOrigin),frac);//!!

		// collision detection: line vs sphere
		// check position of point projection on line
		entPos = DotProduct (eCenter, traceDir);
		if (entPos < -traceWidth - ent->radius || entPos > traceLen + ent->radius)
			continue; // too near / too far

		//!! if entity rotated from prev to current, should lerp angles and recompute ent->axis
		// check distance between point and line
		VectorMA (eCenter, -entPos, traceDir, tmp);
		dist2 = DotProduct (tmp, tmp);
		dist0 = ent->radius + traceWidth;
		if (dist2 >= dist0 * dist0) continue;

		if (ent->solid == 31)
		{
			cmodel = cl.model_clip[ent->modelindex];
			if (!cmodel) continue;

			CM_TransformedBoxTrace2 (&trace, start, end, mins, maxs, cmodel->headnode,  MASK_PLAYERSOLID, eOrigin, ent->axis);
		}
		else
			CM_TransformedBoxTrace (&trace, start, end, mins, maxs,
				CM_HeadnodeForBox (ent->mins, ent->maxs), MASK_PLAYERSOLID, eOrigin, vec3_origin);

		if (trace.allsolid || trace.startsolid || trace.fraction < tr->fraction)
		{
			trace.ent = (struct edict_s *)ent;		// trace.ent is edict_t*, but ent is entityState_t*
		 	if (tr->startsolid)
			{
				*tr = trace;
				tr->startsolid = true;
			}
			else
				*tr = trace;
		}
		else if (trace.startsolid)
			tr->startsolid = true;
		if (tr->allsolid) return;
	}
}


/*
================
CL_PMTrace
================
*/
//?? same as void CL_PMTrace(&trace, start, ...) -- but called from PMove() ...
trace_t CL_PMTrace (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end)
{
	trace_t	trace;

	// check against world
	CM_BoxTrace (&trace, start, end, mins, maxs, 0, MASK_PLAYERSOLID);
	if (trace.fraction < 1.0)
		trace.ent = (struct edict_s *)1;	//??

	// check all other solid models
	CL_ClipMoveToEntities (&trace, start, mins, maxs, end);

	return trace;
}


int CL_PMpointcontents (vec3_t point)
{
	int		i, j;
	cmodel_t *cmodel;
	int		contents;
	float	dist2;

	contents = CM_PointContents (point, 0);

	for (i = 0; i < cl.frame.num_entities; i++)
	{
		entityState_t *ent;
		centity_t *cent;
		vec3_t	 tmp, delta, eCenter, eOrigin;

		ent = &cl_parse_entities[(cl.frame.parse_entities + i) & (MAX_PARSE_ENTITIES-1)];

		if (ent->solid != 31)	// use pointcontents only for inline models
			continue;

		cmodel = cl.model_clip[ent->modelindex];
		if (!cmodel) continue;

		// compute lerped entity position
		cent = &cl_entities[ent->number];
		for (j = 0; j < 3; j++)
			delta[j] = (1.0f - cl.lerpfrac) * (cent->current.origin[j] - cent->prev.origin[j]);
		VectorSubtract (ent->center, delta, eCenter);
		VectorSubtract (ent->origin, delta, eOrigin);

//if (delta[0]||delta[1]||delta[2])//!!
//Com_WPrintf("(%g %g %g)->(%g %g %g) =(%g %g %g):%g\n",VECTOR_ARGS(cent->prev.origin),VECTOR_ARGS(cent->current.origin),VECTOR_ARGS(eOrigin),cl.lerpfrac);//!!

		// check entity bounding sphere
		VectorSubtract (eCenter, point, tmp);
		dist2 = DotProduct (tmp, tmp);
		if (dist2 > ent->radius * ent->radius) continue;
		// accurate check with trace
		contents |= CM_TransformedPointContents2 (point, cmodel->headnode, eOrigin, ent->axis);
	}

	return contents;
}


/*
=================
CL_PredictMovement

Sets cl.predicted_origin and cl.predicted_angles
=================
*/
void CL_PredictMovement (void)
{
	int		ack, current;
	int		frame, oldframe;
	usercmd_t *cmd;
	pmove_t	pm;
	int		i, step, oldz;

	if (cls.state != ca_active || cl.attractloop)
		return;

	if (cl_paused->integer)
		return;

	if (!cl_predict->integer || (cl.frame.playerstate.pmove.pm_flags & PMF_NO_PREDICTION))
	{	// just set angles
		for (i = 0; i < 3; i++)
			cl.predicted_angles[i] = cl.viewangles[i] + SHORT2ANGLE(cl.frame.playerstate.pmove.delta_angles[i]);
		return;
	}

	ack = cls.netchan.incoming_acknowledged;
	current = cls.netchan.outgoing_sequence;

	// if we are too far out of date, just freeze
	if (current - ack >= CMD_BACKUP)
	{
		if (cl_showmiss->integer)
			Com_WPrintf ("CL_PredictMovement: exceeded CMD_BACKUP\n");
		return;
	}

	// copy current state to pmove
	memset (&pm, 0, sizeof(pm));
	pm.trace = CL_PMTrace;
	pm.pointcontents = CL_PMpointcontents;

	pm_airaccelerate = atof(cl.configstrings[CS_AIRACCEL]);

	pm.s = cl.frame.playerstate.pmove;

//	SCR_DebugGraph (current - ack - 1, 0);

	// immediately after server frame, there will be 1 Pmove() cycle; till next server frame this
	// number will be incremented up to (FPS / sv_fps)
	while (++ack < current)
	{
		frame = ack & (CMD_BACKUP-1);
		cmd = &cl.cmds[frame];

		pm.cmd = *cmd;
		predictLerp = 1;
		Pmove (&pm);

		// save for error checking
		cl.predicted_origins[frame][0] = pm.s.origin[0];
		cl.predicted_origins[frame][1] = pm.s.origin[1];
		cl.predicted_origins[frame][2] = pm.s.origin[2];
	}
	predictLerp = -1;			// flag to use cl.lerpfrac

	//?? predict ladders
	oldframe = (ack - 2) & (CMD_BACKUP - 1);
	oldz = cl.predicted_origins[oldframe][2];
	step = pm.s.origin[2] - oldz;
	if (step > 63 && step < 160 && (pm.s.pm_flags & PMF_ON_GROUND))
	{
		cl.predicted_step = step * 0.125f;
		cl.predicted_step_time = cls.realtime - cls.frametime * 500;	// back 1/2 frame ?
	}

	// copy results out for rendering
	cl.predicted_origin[0] = pm.s.origin[0]*0.125f;
	cl.predicted_origin[1] = pm.s.origin[1]*0.125f;
	cl.predicted_origin[2] = pm.s.origin[2]*0.125f;

	VectorCopy (pm.viewangles, cl.predicted_angles);
}
