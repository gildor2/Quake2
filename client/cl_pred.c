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

	if (!cl_predict->integer || (cl.frame.playerstate.pmove.pm_flags & PMF_NO_PREDICTION))
		return;

	// calculate the last usercmd_t we sent that the server has processed
	frame = cls.netchan.incoming_acknowledged;
	frame &= (CMD_BACKUP-1);

	// compare what the server returned with what we had predicted it to be
	VectorSubtract (cl.frame.playerstate.pmove.origin, cl.predicted_origins[frame], delta);

	// save the prediction error for interpolation
	len = abs(delta[0]) + abs(delta[1]) + abs(delta[2]);
	if (len > 640)	// 80 world units
	{	// a teleport or something
		VectorClear (cl.prediction_error);
	}
	else
	{
		if (cl_showmiss->integer && (delta[0] || delta[1] || delta[2]))
			Com_WPrintf ("prediction miss on %d: %d\n", cl.frame.serverframe,
				delta[0] + delta[1] + delta[2]);

		cl.predicted_origins[frame][0] = cl.frame.playerstate.pmove.origin[0];
		cl.predicted_origins[frame][1] = cl.frame.playerstate.pmove.origin[1];
		cl.predicted_origins[frame][2] = cl.frame.playerstate.pmove.origin[2];

		// save for error itnerpolation
		for (i = 0; i < 3; i++)
			cl.prediction_error[i] = delta[i] * 0.125f;
	}
}


/*
====================
CL_ClipMoveToEntities

====================
*/
void CL_ClipMoveToEntities (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, trace_t *tr)
{
	int		i;
	entityState_t *ent;
	trace_t	trace;
	float	t, traceLen, traceWidth, b1, b2;
	vec3_t	traceDir;

	b1 = DotProduct (mins, mins);
	b2 = DotProduct (maxs, maxs);
	t = b1 > b2 ? b1 : b2;
	traceWidth = SQRTFAST(t);
	VectorSubtract (end, start, traceDir);
	traceLen = VectorNormalize (traceDir) + traceWidth;

	for (i = 0; i < cl.frame.num_entities; i++)
	{
		cmodel_t *cmodel;
		vec3_t	center, tmp;
		float	entPos, dist2, dist0;

		ent = &cl_parse_entities[(cl.frame.parse_entities + i) & (MAX_PARSE_ENTITIES-1)];
		if (!ent->solid) continue;
		if (ent->number == cl.playernum+1) continue;	// no clip with player entity

		VectorSubtract (ent->center, start, center);
		// collision detection: line vs sphere
		// check position of point projection on line
		entPos = DotProduct (center, traceDir);
		if (entPos < -traceWidth - ent->radius || entPos > traceLen + ent->radius)
			continue; // too near / too far

		// check distance between point and line
		VectorMA (center, -entPos, traceDir, tmp);
		dist2 = DotProduct (tmp, tmp);
		dist0 = ent->radius + traceWidth;
		if (dist2 >= dist0 * dist0) continue;

		if (ent->solid == 31)
		{
			cmodel = cl.model_clip[ent->modelindex];
			if (!cmodel) continue;

			trace = CM_TransformedBoxTrace2 (start, end, mins, maxs, cmodel->headnode,  MASK_PLAYERSOLID, ent->origin, ent->axis);
		}
		else
			trace = CM_TransformedBoxTrace (start, end, mins, maxs,
				CM_HeadnodeForBox (ent->mins, ent->maxs), MASK_PLAYERSOLID, ent->origin, vec3_origin);

		if (trace.allsolid || trace.startsolid || trace.fraction < tr->fraction)
		{
			trace.ent = (struct edict_s *)ent;
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
trace_t CL_PMTrace (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end)
{
	trace_t	t;

	// check against world
	t = CM_BoxTrace (start, end, mins, maxs, 0, MASK_PLAYERSOLID);
	if (t.fraction < 1.0)
		t.ent = (struct edict_s *)1;

	// check all other solid models
	CL_ClipMoveToEntities (start, mins, maxs, end, &t);

	return t;
}

int CL_PMpointcontents (vec3_t point)
{
	int		i;
	entityState_t *ent;
	cmodel_t *cmodel;
	int		contents;
	vec3_t	delta;
	float	dist2;

	contents = CM_PointContents (point, 0);

	for (i = 0; i < cl.frame.num_entities; i++)
	{
		ent = &cl_parse_entities[(cl.frame.parse_entities + i) & (MAX_PARSE_ENTITIES-1)];

		if (ent->solid != 31)	// use pointcontents only for inline models
			continue;

		cmodel = cl.model_clip[ent->modelindex];
		if (!cmodel) continue;

		// check entity bounding sphere
		VectorSubtract (ent->center, point, delta);
		dist2 = DotProduct (delta, delta);
		if (dist2 > ent->radius * ent->radius) continue;
		// accurate check with trace
		contents |= CM_TransformedPointContents2 (point, cmodel->headnode, ent->origin, ent->axis);
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

	if (cls.state != ca_active)
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

	frame = 0;

	// run frames
	while (++ack < current)
	{
		frame = ack & (CMD_BACKUP-1);
		cmd = &cl.cmds[frame];

		pm.cmd = *cmd;
		Pmove (&pm);

		// save for debug checking
		cl.predicted_origins[frame][0] = pm.s.origin[0];
		cl.predicted_origins[frame][1] = pm.s.origin[1];
		cl.predicted_origins[frame][2] = pm.s.origin[2];
	}

	oldframe = (ack - 2) & (CMD_BACKUP - 1);
	oldz = cl.predicted_origins[oldframe][2];
	step = pm.s.origin[2] - oldz;
	if (step > 63 && step < 160 && (pm.s.pm_flags & PMF_ON_GROUND))
	{
		cl.predicted_step = step * 0.125;
		cl.predicted_step_time = cls.realtime - cls.frametime * 500;
	}

	// copy results out for rendering
	cl.predicted_origin[0] = pm.s.origin[0]*0.125;
	cl.predicted_origin[1] = pm.s.origin[1]*0.125;
	cl.predicted_origin[2] = pm.s.origin[2]*0.125;

	VectorCopy (pm.viewangles, cl.predicted_angles);
}
