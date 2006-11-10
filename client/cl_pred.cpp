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
#include "cmodel.h"


static float predictLerp = -1;		// >= 0 -- use this value for prediction; if < 0 -- use cl.lerpfrac


void CL_CheckPredictionError ()
{
	guard(CL_CheckPredictionError);

	if (!cl_predict->integer || (cl.frame.playerstate.pmove.pm_flags & PMF_NO_PREDICTION) || cl.attractloop)
		return;

	// calculate the last usercmd_t we sent that the server has processed
	int frame = cls.netchan.incoming_acknowledged & (CMD_BACKUP-1);

	// compare what the server returned with what we had predicted it to be
	short delta[3];
	VectorSubtract2 (cl.frame.playerstate.pmove.origin, cl.predicted_origins[frame], delta);

	// save the prediction error for interpolation
	int len = abs(delta[0]) + abs(delta[1]) + abs(delta[2]);	// vec.GetLength() ?
	if (len > 80 * 8)	// 80 world units (float->int scale is 8 ...)
	{	// a teleport or something
		cl.prediction_error.Zero();
	}
	else
	{
		// save for error itnerpolation
		for (int i = 0; i < 3; i++)
			cl.prediction_error[i] = delta[i] * 0.125f;

		if (cl_showmiss->integer && len)
			Com_DPrintf ("prediction miss on %d: %g %g %g\n", cl.frame.serverframe, VECTOR_ARG(cl.prediction_error));

		cl.predicted_origins[frame][0] = cl.frame.playerstate.pmove.origin[0];
		cl.predicted_origins[frame][1] = cl.frame.playerstate.pmove.origin[1];
		cl.predicted_origins[frame][2] = cl.frame.playerstate.pmove.origin[2];
	}

	unguard;
}


//#define NO_PREDICT_LERP	// debug

void CL_EntityTrace (trace_t &trace, const CVec3 &start, const CVec3 &end, const CBox &bounds, int contents)
{
	guard(CL_EntityTrace);

	float b1 = dot (bounds.mins, bounds.mins);
	float b2 = dot (bounds.maxs, bounds.maxs);
	float t = max(b1, b2);
	float traceWidth = SQRTFAST(t);
	CVec3 traceDir;
	VectorSubtract (end, start, traceDir);
	float traceLen = traceDir.Normalize () + traceWidth;

	for (int i = 0; i < cl.frame.num_entities; i++)
	{
		clEntityState_t &ent = cl_parse_entities[(cl.frame.parse_entities + i) & (MAX_PARSE_ENTITIES-1)];
		if (!ent.solid) continue;
		if (ent.number == cl.playernum+1) continue;		// no clip with player entity

		// compute lerped entity position
		centity_t &cent = cl_entities[ent.number];
		float frac = predictLerp;						// called CL_PredictMovement()
		if (frac < 0) frac = cl.lerpfrac;				// called from any other place
		CVec3 delta;
#ifndef NO_PREDICT_LERP
		for (int j = 0; j < 3; j++)
			delta[j] = (1.0f - frac) * (cent.current.origin[j] - cent.prev.origin[j]);
#else
		delta.Zero();
#endif

		CVec3 eCenter, eOrigin;
		VectorSubtract (ent.center, start, eCenter);
		VectorSubtract (eCenter, delta, eCenter);
		VectorSubtract (ent.origin, delta, eOrigin);

		// collision detection: line vs sphere
		// check position of point projection on line
		float entPos = dot (eCenter, traceDir);
		if (entPos < -traceWidth - ent.radius || entPos > traceLen + ent.radius)
			continue; // too near / too far

		//!! if entity rotated from prev to current, should lerp angles and recompute ent->axis
		// check distance between point and line
		CVec3 tmp;
		VectorMA (eCenter, -entPos, traceDir, tmp);
		float dist2 = dot (tmp, tmp);
		float dist0 = ent.radius + traceWidth;
		if (dist2 >= dist0 * dist0) continue;

		trace_t	tr;
		if (ent.solid == 31)
		{
			CBspModel *cmodel = cl.model_clip[ent.modelindex];
			if (!cmodel) continue;
			CM_TransformedBoxTrace (tr, start, end, bounds, cmodel->headnode, contents, eOrigin, ent.axis);
		}
		else
			CM_TransformedBoxTrace (tr, start, end, bounds, CM_HeadnodeForBox (ent.bounds), contents, eOrigin, nullVec3);
		if (CM_CombineTrace (trace, tr))
			trace.ent = (edict_s *)&ent;				// trace.ent is edict_t*, but ent is clEntityState_t*
		if (trace.allsolid) return;
	}

	unguard;
}


void CL_Trace (trace_t &tr, const CVec3 &start, const CVec3 &end, const CBox &bounds, int contents)
{
	guard(CL_Trace);
	CM_BoxTrace (tr, start, end, bounds, 0, contents);
	if (tr.fraction == 0) return;
	CM_ClipTraceToModels (tr, start, end, bounds, contents);
	if (tr.fraction == 0) return;
	CL_EntityTrace (tr, start, end, bounds, contents);
	unguard;
}


static trace0_t* CL_PMTrace (trace0_t &trace, const CVec3 &start, const CVec3 &mins, const CVec3 &maxs, const CVec3 &end)
{
	guard(CL_PMTrace);
	// check against world
	CBox bounds;
	bounds.mins = mins;
	bounds.maxs = maxs;
	// extended trace
	trace_t trace1;
	CM_BoxTrace (trace1, start, end, bounds, 0, MASK_PLAYERSOLID);
	CM_ClipTraceToModels (trace1, start, end, bounds, MASK_PLAYERSOLID);
	if (trace1.fraction < 1.0)
		trace1.ent = (edict_s *)1;	//??

	// check all other solid models
	CL_EntityTrace (trace1, start, end, bounds, MASK_PLAYERSOLID);
	// copy base fields to trace0_t
	trace = trace1;

	return &trace;
	unguard;
}


int CL_PMpointcontents (const CVec3 &point)
{
	guard(CL_PMpointcontents);

	unsigned contents = CM_PointContents (point, 0);
	contents |= CM_PointModelContents (point);

#ifndef NO_PREDICT_LERP
	float backlerp = 1.0f - cl.lerpfrac;
#endif
	for (int i = 0; i < cl.frame.num_entities; i++)
	{
		CVec3	 tmp, delta, eCenter, eOrigin;

		clEntityState_t *ent = &cl_parse_entities[(cl.frame.parse_entities + i) & (MAX_PARSE_ENTITIES-1)];
		if (ent->solid != 31) continue;	// use pointcontents() only for inline models

		CBspModel *cmodel = cl.model_clip[ent->modelindex];
		if (!cmodel) continue;

		// compute lerped entity position
		centity_t *cent = &cl_entities[ent->number];
#ifndef NO_PREDICT_LERP
		for (int j = 0; j < 3; j++)
			delta[j] = backlerp * (cent->current.origin[j] - cent->prev.origin[j]);
#else
		delta.Zero();
#endif
		VectorSubtract (ent->center, delta, eCenter);
		VectorSubtract (ent->origin, delta, eOrigin);

//if (delta[0]||delta[1]||delta[2])//!!
//appWPrintf("(%g %g %g)->(%g %g %g) =(%g %g %g):%g\n",VECTOR_ARG(cent->prev.origin),VECTOR_ARG(cent->current.origin),VECTOR_ARG(eOrigin),cl.lerpfrac);//!!

		// check entity bounding sphere
		VectorSubtract (eCenter, point, tmp);
		float dist2 = dot (tmp, tmp);
		if (dist2 > ent->radius * ent->radius) continue;
		// accurate check with trace
		contents |= CM_TransformedPointContents (point, cmodel->headnode, eOrigin, ent->axis);
	}

	return contents;
	unguard;
}


// Sets cl.predicted_origin and cl.predicted_angles

void CL_PredictMovement ()
{
	guard(CL_PredictMovement);

	if (!bspfile.clientLoaded)		// the code below will use trace(), but map is not yet ready ...
		return;

	if (cls.state != ca_active || cl.attractloop)
		return;

	if (cl_paused->integer)
		return;

	if (!cl_predict->integer || (cl.frame.playerstate.pmove.pm_flags & PMF_NO_PREDICTION))
	{	// just set angles
		for (int i = 0; i < 3; i++)
			cl.predicted_angles[i] = cl.viewangles[i] + SHORT2ANGLE(cl.frame.playerstate.pmove.delta_angles[i]);
		return;
	}

	int ack     = cls.netchan.incoming_acknowledged;
	int current = cls.netchan.outgoing_sequence;
	if (cls.netFrameDropped) current++;

	// if we are too far out of date, just freeze
	if (current - ack >= CMD_BACKUP)
	{
		if (cl_showmiss->integer)
			appWPrintf ("CL_PredictMovement: exceeded CMD_BACKUP\n");
		return;
	}

	// copy current state to pmove
	pmove_t	pm;
	memset (&pm, 0, sizeof(pm));
	pm.trace         = CL_PMTrace;
	pm.pointcontents = CL_PMpointcontents;
	pm.s             = cl.frame.playerstate.pmove;

	pm_airaccelerate = atof (cl.configstrings[CS_AIRACCEL]);

	// immediately after server frame, there will be 1 Pmove() cycle; till next server frame this
	// number will be incremented up to (FPS / sv_fps)
	bool predicted = false;
	while (++ack < current)
	{
		int frame = ack & (CMD_BACKUP-1);
		pm.cmd = cl.cmds[frame];
		predictLerp = 1;
		Pmove (&pm);

		// save for error checking
		cl.predicted_origins[frame][0] = pm.s.origin[0];
		cl.predicted_origins[frame][1] = pm.s.origin[1];
		cl.predicted_origins[frame][2] = pm.s.origin[2];
		predicted = true;
	}
	predictLerp = -1;			// flag to use cl.lerpfrac

	//?? predict ladders
	int oldframe = (ack - 2) & (CMD_BACKUP - 1);
	int oldz = cl.predicted_origins[oldframe][2];
	int step = pm.s.origin[2] - oldz;
	if (step > 63 && step < 160 && (pm.s.pm_flags & PMF_ON_GROUND))
	{
		cl.predicted_step = step * 0.125f;
		cl.predicted_step_time = cls.realtime - appRound (cls.frametime * 500);	// back 1/2 frame ?
	}

	// copy results out for rendering
	cl.predicted_origin[0] = pm.s.origin[0]*0.125f;
	cl.predicted_origin[1] = pm.s.origin[1]*0.125f;
	cl.predicted_origin[2] = pm.s.origin[2]*0.125f;

	if (predicted)				// there can be a situation (when very fast fps or small timescale), when ack+1==current
		cl.predicted_angles = pm.viewangles;

	unguard;
}
