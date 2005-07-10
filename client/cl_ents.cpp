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
// cl_ents.cpp -- entity parsing and management

#include "client.h"
#include "cmodel.h"


extern	CRenderModel	*cl_mod_powerscreen;

/*
=========================================================================

FRAME PARSING

=========================================================================
*/


/*
==================
CL_ParseDelta

Can go from either a baseline or a previous packet_entity
==================
*/
void CL_ParseDelta (clEntityState_t *from, clEntityState_t *to, int number, unsigned bits, bool baseline)
{
	guard(CL_ParseDelta);

	MSG_ReadDeltaEntity (&net_message, from, to, bits);
	to->number = number;		//?? is it needed ?

	//!! if remove line "if (bits & (...) || baseline) ...", can remove "baseline" arg and "ent->valid" field
//	if (bits & (U_ANGLE_N|U_MODEL_N) || baseline)
		to->axis.FromAngles (to->angles);

//	if (bits & (U_SOLID|U_ANGLE_N|U_ORIGIN_N|U_MODEL_N) || baseline || !to->valid)
	{
		if (to->solid && to->solid != 31)
		{
			int x = 8 * (to->solid & 31);
			int zd = 8 * ((to->solid>>5) & 31);
			int zu = 8 * ((to->solid>>10) & 63) - 32;

			to->bounds.mins.Set (-x, -x, -zd);
			to->bounds.maxs.Set (x, x, zu);

			CVec3 d;
			to->bounds.GetCenter (d);
			VectorAdd (to->origin, d, to->center);

			to->radius = VectorDistance (to->bounds.maxs, to->bounds.mins) / 2;
			to->valid = true;
		}
		else
		{
			cmodel_t *m = cl.model_clip[to->modelindex];
			if (m)
			{
				CVec3 v;
				m->bounds.GetCenter (v);
				UnTransformPoint (to->origin, to->axis, v, to->center);
				to->radius = m->radius;
				to->valid = true;
			}
		}
	}

	unguard;
}

/*
==================
CL_DeltaEntity

Parses deltas from the given base and adds the resulting entity
to the current frame
==================
*/
static void CL_DeltaEntity (frame_t *frame, int newnum, clEntityState_t *old, unsigned bits)
{
	centity_t &ent = cl_entities[newnum];
	clEntityState_t &state = cl_parse_entities[cl.parse_entities & (MAX_PARSE_ENTITIES-1)];

	cl.parse_entities++;
	frame->num_entities++;

	CL_ParseDelta (old, &state, newnum, bits, false);

	// some data changes will force no lerping
	if (state.modelindex  != ent.current.modelindex  ||
		state.modelindex2 != ent.current.modelindex2 ||
		state.modelindex3 != ent.current.modelindex3 ||
		state.modelindex4 != ent.current.modelindex4 ||
		fabs(state.origin[0] - ent.current.origin[0]) > 256 ||	// whole map max 4096, 1/10 map/sec speed is 409
		fabs(state.origin[1] - ent.current.origin[1]) > 256 ||
		fabs(state.origin[2] - ent.current.origin[2]) > 256 ||
		state.event == EV_PLAYER_TELEPORT ||
		state.event == EV_OTHER_TELEPORT
		)
	{
		ent.serverframe = -99;
	}

	if (ent.serverframe != cl.frame.serverframe - 1)
	{
		// duplicate the current state so lerping doesn't hurt anything
		ent.prev = state;			// new entity: prev = new
		if (state.event == EV_OTHER_TELEPORT)
			ent.prev.origin = state.origin;
		else
			ent.prev.origin = state.old_origin;
		// init some non-transferred fields (reset entity)
		ent.prevTrail = ent.prev.origin;
		ent.trailLen = 0;
	}
	else
		ent.prev = ent.current;		// normal delta: prev = current, current = new

	ent.serverframe = cl.frame.serverframe;
	ent.current = state;
}

/*
==================
CL_ParsePacketEntities

An svc_packetentities has just been parsed, deal with the
rest of the data stream.
==================
*/
static void CL_ParsePacketEntities (frame_t *oldframe, frame_t *newframe)
{
	clEntityState_t *oldstate;
	int		oldindex, oldnum, old_num_entities;

	newframe->parse_entities = cl.parse_entities;
	newframe->num_entities = 0;

	oldindex = 0;
	old_num_entities = oldframe ? oldframe->num_entities : 0;

	while (true)
	{
		unsigned bits;
		bool	remove;

		int newnum = MSG_ReadEntityBits (&net_message, &bits, &remove);
		if (net_message.readcount > net_message.cursize)
			Com_DropError ("CL_ParsePacketEntities: end of message");
		if (newnum >= MAX_EDICTS)
			Com_DropError ("CL_ParsePacketEntities: bad number: %d", newnum);

		if (!newnum) break;				// received end of packet entities (bits=0, entNum=0)

		if (oldindex >= old_num_entities)
			oldnum = BIG_NUMBER;
		else
		{
			oldstate = &cl_parse_entities[(oldframe->parse_entities+oldindex) & (MAX_PARSE_ENTITIES-1)];
			oldnum = oldstate->number;
		}

		while (oldnum < newnum)
		{
			// one or more entities from the old packet are unchanged
			if (cl_shownet->integer == 3)
				Com_Printf ("   unchanged: %d\n", oldnum);
			CL_DeltaEntity (newframe, oldnum, oldstate, 0);

			if (++oldindex >= old_num_entities)
			{
				oldnum = BIG_NUMBER;
				break;
			}
			oldstate = &cl_parse_entities[(oldframe->parse_entities+oldindex) & (MAX_PARSE_ENTITIES-1)];
			oldnum = oldstate->number;
		}

		if (remove)
		{	// the entity present in oldframe is not in the current frame
			if (cl_shownet->integer == 3)
				Com_Printf ("   remove: %d\n", newnum);
			if (oldnum != newnum)
				Com_WPrintf ("CL_ParsePacketEntities: remove: oldnum != newnum\n");

			oldindex++;
			continue;
		}

		if (oldnum == newnum)
		{
			// delta from previous state
			if (cl_shownet->integer == 3)
				Com_Printf ("   delta: %d\n", newnum);
			CL_DeltaEntity (newframe, newnum, oldstate, bits);

			oldindex++;
			continue;
		}

		if (oldnum > newnum)
		{
			// delta from baseline
			if (cl_shownet->integer == 3)
				Com_Printf ("   baseline: %d\n", newnum);
			CL_DeltaEntity (newframe, newnum, &cl_entities[newnum].baseline, bits);
			continue;
		}
	}

	// any remaining entities in the old frame are copied over
	while (oldindex < old_num_entities)
	{
		// one or more entities from the old packet are unchanged
		oldstate = &cl_parse_entities[(oldframe->parse_entities+oldindex) & (MAX_PARSE_ENTITIES-1)];
		oldnum = oldstate->number;

		if (cl_shownet->integer == 3)
			Com_Printf ("   unchanged: %d\n", oldnum);
		CL_DeltaEntity (newframe, oldnum, oldstate, 0);

		oldindex++;
	}
}



/*
==================
CL_FireEntityEvents

==================
*/
static void CL_FireEntityEvents (frame_t *frame)
{
	for (int pnum = 0 ; pnum<frame->num_entities ; pnum++)
	{
		int num = (frame->parse_entities + pnum)&(MAX_PARSE_ENTITIES-1);
		clEntityState_t *s1 = &cl_parse_entities[num];
		if (s1->event)
			CL_EntityEvent (s1);

		// EF_TELEPORTER acts like an event, but is not cleared each frame
		if (s1->effects & EF_TELEPORTER)
			CL_TeleporterParticles (s1);
	}
}


/*
================
CL_ParseFrame
================
*/
void CL_ParseFrame (void)
{
	frame_t		*old;

	memset (&cl.frame, 0, sizeof(cl.frame));

	cl.frame.serverframe = MSG_ReadLong (&net_message);
	cl.frame.deltaframe = MSG_ReadLong (&net_message);
	cl.frame.servertime = cl.frame.serverframe*100;

	// BIG HACK to let old demos continue to work
	if (cls.serverProtocol != 26)
		cl.surpressCount = MSG_ReadByte (&net_message);

	if (cl_shownet->integer == 3)
		Com_Printf ("   frame:%i  delta:%i\n", cl.frame.serverframe,
		cl.frame.deltaframe);

	cl.overtime = 0;

	// If the frame is delta compressed from data that we
	// no longer have available, we must suck up the rest of
	// the frame, but not use it, then ask for a non-compressed
	// message
	if (cl.frame.deltaframe <= 0)
	{
		cl.frame.valid = true;				// uncompressed frame
		old = NULL;
		cls.demowaiting = false;			// we can start recording now
	}
	else
	{
		old = &cl.frames[cl.frame.deltaframe & UPDATE_MASK];
		if (!old->valid)
		{	// should never happen
			Com_Printf ("Delta from invalid frame (not supposed to happen!).\n");
		}
		if (old->serverframe != cl.frame.deltaframe)
		{	// The frame that the server did the delta from
			// is too old, so we can't reconstruct it properly.
			Com_Printf ("Delta frame too old.\n");
		}
		else if (cl.parse_entities - old->parse_entities > MAX_PARSE_ENTITIES-128)
		{
			Com_Printf ("Delta parse_entities too old.\n");
		}
		else
			cl.frame.valid = true;			// valid delta parse
	}

	// clamp time
	if (cl.time > cl.frame.servertime)
	{
		cl.time = cl.frame.servertime;
		cl.ftime = cl.time / 1000.0f;
	}
	else if (cl.time < cl.frame.servertime - 100)
	{
		cl.time = cl.frame.servertime - 100;
		cl.ftime = cl.time / 1000.0f;
	}

	// read areabits
	int len = MSG_ReadByte (&net_message);
	MSG_ReadData (&net_message, &cl.frame.areabits, len);

	// read playerinfo
	int cmd = MSG_ReadByte (&net_message);
	SHOWNET(svc_strings[cmd]);
	if (cmd != svc_playerinfo)
		Com_DropError ("CL_ParseFrame: not playerinfo");

	MSG_ReadDeltaPlayerstate (&net_message, old ? &old->playerstate : NULL, &cl.frame.playerstate);
	if (cl.attractloop) cl.frame.playerstate.pmove.pm_type = PM_FREEZE;		//?? is it needed ?

	// read packet entities
	cmd = MSG_ReadByte (&net_message);
	SHOWNET(svc_strings[cmd]);
	if (cmd != svc_packetentities)
		Com_DropError ("CL_ParseFrame: not packetentities");
	CL_ParsePacketEntities (old, &cl.frame);

	// save the frame off in the backup array for later delta comparisons
	cl.frames[cl.frame.serverframe & UPDATE_MASK] = cl.frame;

	if (cl.frame.valid)
	{
		// getting a valid frame message ends the connection process
		if (cls.state != ca_active)
		{
			cls.state = ca_active;
			SCR_ShowConsole (false, true);	// hide console
			M_ForceMenuOff ();				// hide menu
			cl.forceViewFrame = true;
			cl.predicted_origin[0] = cl.frame.playerstate.pmove.origin[0] * 0.125f;
			cl.predicted_origin[1] = cl.frame.playerstate.pmove.origin[1] * 0.125f;
			cl.predicted_origin[2] = cl.frame.playerstate.pmove.origin[2] * 0.125f;
			cl.predicted_angles = cl.frame.playerstate.viewangles;
			SCR_EndLoadingPlaque (false);	// get rid of loading plaque
			CL_Pause (false);
		}
		cl.sound_prepped = true;			// can start mixing ambient sounds

		// fire entity events
		CL_FireEntityEvents (&cl.frame);
		CL_CheckPredictionError ();
	}
	// get oldFrame
	cl.oldFrame = &cl.frames[(cl.frame.serverframe - 1) & UPDATE_MASK];
	if (cl.oldFrame->serverframe != cl.frame.serverframe-1 || !cl.oldFrame->valid)
		cl.oldFrame = &cl.frame;			// previous frame was dropped or invalid
}


static void GetEntityInfo (int entityNum, clEntityState_t * &st, unsigned &eff, unsigned &rfx)
{
	int		effects, renderfx;
	clEntityState_t *state;

	state = &cl_parse_entities[(cl.frame.parse_entities + entityNum) & (MAX_PARSE_ENTITIES-1)];
	//?? is state == cl_entities[entityNum] ? no ... but why ?
	// -- if (memcmp (state, &cl_entities[entityNum].current, sizeof(clEntityState_t))) Com_WPrintf("%d\n");
	effects = state->effects;
	renderfx = state->renderfx;

	// convert some EF_XXX to EF_COLOR_SHELL+RF_XXX
	if (effects & (EF_PENT|EF_QUAD|EF_DOUBLE|EF_HALF_DAMAGE))
	{
		if (effects & EF_PENT)
			renderfx |= RF_SHELL_RED;
		if (effects & EF_QUAD)
			renderfx |= RF_SHELL_BLUE;
		if (effects & EF_DOUBLE)
			renderfx |= RF_SHELL_DOUBLE;
		if (effects & EF_HALF_DAMAGE)
			renderfx |= RF_SHELL_HALF_DAM;
		// clear processed effects and add color shell
		effects = effects | EF_COLOR_SHELL & ~(EF_PENT|EF_QUAD|EF_DOUBLE|EF_HALF_DAMAGE);
	}
	if ((effects & EF_COLOR_SHELL) && !stricmp (fs_gamedirvar->string, "rogue"))
	{
		// PMM - at this point, all of the shells have been handled
		// if we're in the rogue pack, set up the custom mixing, otherwise just
		// keep going
		// all of the solo colors are fine.  we need to catch any of the combinations that look bad
		// (double & half) and turn them into the appropriate color, and make double/quad something special
		if (renderfx & RF_SHELL_HALF_DAM)
		{
			// ditch the half damage shell if any of red, blue, or double are on
			if (renderfx & (RF_SHELL_RED|RF_SHELL_BLUE|RF_SHELL_DOUBLE))
				renderfx &= ~RF_SHELL_HALF_DAM;
		}

		if (renderfx & RF_SHELL_DOUBLE)
		{
				// lose the yellow shell if we have a red, blue, or green shell
			if (renderfx & (RF_SHELL_RED|RF_SHELL_BLUE|RF_SHELL_GREEN))
				renderfx &= ~RF_SHELL_DOUBLE;
			// if we have a red shell, turn it to purple by adding blue
			if (renderfx & RF_SHELL_RED)
				renderfx |= RF_SHELL_BLUE;
			// if we have a blue shell (and not a red shell), turn it to cyan by adding green
			else if (renderfx & RF_SHELL_BLUE)
				// go to green if it's on already, otherwise do cyan (flash green)
				if (renderfx & RF_SHELL_GREEN)
					renderfx &= ~RF_SHELL_BLUE;
				else
					renderfx |= RF_SHELL_GREEN;
		}
	}

	st  = state;
	eff = effects;
	rfx = renderfx;
}


/*
==============
CL_AddViewWeapon
==============
*/
static void AddViewWeapon (int renderfx)
{
	player_state_t *ps = &cl.frame.playerstate;
	player_state_t *ops = &cl.oldFrame->playerstate;

	// allow the gun to be completely removed
	if (!cl_gun->integer) return;
	if (hand->integer == 2) return;

	// don't draw gun if in wide angle view
	if (ps->fov > 90) return;

	entity_t gun;		// view model
	memset (&gun, 0, sizeof(gun));

#ifdef GUN_DEBUG
	if (gun_model)
		gun.model = gun_model;
	else
#endif
		gun.model = cl.model_draw[ps->gunindex];
	if (!gun.model) return;

	// lerp gun position
	for (int i = 0; i < 3; i++)
	{
		gun.pos.origin[i] = cl.refdef.vieworg[i] + Lerp (ops->gunoffset[i], ps->gunoffset[i], cl.lerpfrac);
		gun.angles[i] = cl.refdef.viewangles[i] + LerpAngle (ops->gunangles[i], ps->gunangles[i], cl.lerpfrac);
	}

#ifdef GUN_DEBUG
	if (gun_frame)
	{
		gun.frame = gun_frame;
		gun.oldframe = gun_frame;
	}
	else
#endif
	{
		gun.frame = ps->gunframe;
		if (gun.frame == 0)
			gun.oldframe = 0;				// just changed weapons, don't lerp from old
		else
			gun.oldframe = ops->gunframe;
	}

	gun.flags = RF_MINLIGHT|RF_DEPTHHACK|RF_WEAPONMODEL;
	if (hand->integer == 1) gun.flags |= RF_MIRROR;
	gun.backlerp = 1.0 - cl.lerpfrac;
	FNegate (gun.angles[2]);			// q2 bug: negate md2 angle (may be, for gun it is always 0?)
	AddEntityWithEffects (&gun, renderfx);
}


void CL_AddEntityBox (clEntityState_t *st, unsigned rgba)
{
	centity_t *cent = &cl_entities[st->number];

	entity_t ent;
	memset (&ent, 0, sizeof(ent));
	ent.flags = RF_BBOX;
	ent.color.rgba = rgba;

#if 1
	// lerp
	for (int i = 0; i < 3; i++)
		ent.angles[i] = LerpAngle (cent->prev.angles[i], cent->current.angles[i], cl.lerpfrac);
#else
	// don't lerp
	ent->angles = cent->current.angles;
#endif
	Lerp (cent->prev.center, cent->current.center, cl.lerpfrac, ent.pos.origin);

	CBox *box;
	if (st->solid == 31)
	{
		cmodel_t *cmodel = cl.model_clip[st->modelindex];
		box = &cmodel->bounds;
	}
	else
	{
		FNegate (ent.angles[2]);		// triangle models have bug in Q2: angles[2] should be negated
		box = &st->bounds;
	}
	VectorSubtract (box->maxs, box->mins, ent.size);
	ent.size.Scale (0.5f);

	V_AddEntity (&ent);
}


static void CL_AddDebugLines (void)
{
	if (!cl_showbboxes->integer) return;

	for (int pnum = 0; pnum < cl.frame.num_entities; pnum++)
	{
		clEntityState_t *st = &cl_parse_entities[(cl.frame.parse_entities+pnum)&(MAX_PARSE_ENTITIES-1)];
		// different color for collision -- non-collision entities
#if 0
		//BUGS! -- server not sent bbox params for this entity
		// most used for bbox drawing params computed (for non-inline models) from st->solid (encoded bbox)
		CL_AddEntityBox (st, st->solid ? RGB(0,0.5,0.5) : RGB(0,0.1,0.1));
#else
		if (st->solid) CL_AddEntityBox (st, RGB(0,0.5,0.5));
#endif
	}
}


/*
===============
CL_AddPacketEntities
===============
*/
static void CL_AddPacketEntities (void)
{
	// bonus items rotate at a fixed rate
	float autorotate = AngleMod(cl.ftime * 100);
	// brush models can auto animate their frames
	int autoanim = 2 * cl.time / 1000;

	for (int pnum = 0; pnum < cl.frame.num_entities; pnum++)
	{
		clEntityState_t *st;
		unsigned effects, renderfx;
		GetEntityInfo (pnum, st, effects, renderfx);
		centity_t &cent = cl_entities[st->number];		// not "const", because used for trails

		// parse beams
		if (renderfx & RF_BEAM)
		{
			beam_t *b = CL_AllocParticleBeam (cent.current.origin, cent.current.old_origin, st->frame / 2.0f, 0);
			if (!b) continue;
			b->type = BEAM_STANDARD;
			b->color.rgba = 0;
			// the four beam colors are encoded in 32 bits of skinnum (hack)
			b->color.c[0] = (st->skinnum >> ((rand() % 4)*8)) & 0xFF;
			b->alpha = 0.3f;
			continue;
		}

		// create new entity
		entity_t ent;
		memset (&ent, 0, sizeof(ent));

		// set frame
		if (effects & EF_ANIM01)
			ent.frame = autoanim & 1;
		else if (effects & EF_ANIM23)
			ent.frame = 2 + (autoanim & 1);
		else if (effects & EF_ANIM_ALL)
			ent.frame = autoanim;
		else if (effects & EF_ANIM_ALLFAST)
			ent.frame = cl.time / 100;
		else
			ent.frame = st->frame;

		ent.oldframe = cent.prev.frame;
		ent.backlerp = 1.0f - cl.lerpfrac;

		// interpolate origin
		Lerp (cent.prev.origin, cent.current.origin, cl.lerpfrac, ent.pos.origin);

		// set skin
		clientInfo_t *ci = NULL;
		if (st->modelindex == 255)
		{
			// use custom player skin
			ci = &cl.clientInfo[st->skinnum & 0xFF];
			ent.skinnum = 0;
		}
		else
		{
			ent.skinnum = st->skinnum;
			ent.skin = NULL;
			ent.model = cl.model_draw[st->modelindex];
		}

		// only used for black hole model right now, FIXME: do better
		//?? use shader
		if (renderfx == RF_TRANSLUCENT)
			ent.alpha = 0.7f;

		// render effects (fullbright, translucent, etc)
		if (effects & EF_COLOR_SHELL)
			ent.flags = 0;	// renderfx go on color shell entity
		else
			ent.flags = renderfx;

		// calculate angles
		if (effects & EF_ROTATE)
		{	// some bonus items auto-rotate
			ent.angles.Set (0, autorotate, 0);
		}
		// XATRIX
		else if (effects & EF_SPINNINGLIGHTS)
		{
			CVec3 forward, start;

			ent.angles[0] = 0;
			ent.angles[1] = AngleMod(cl.time/2) + st->angles[1];
			ent.angles[2] = 180;
			AngleVectors (ent.angles, &forward, NULL, NULL);
			VectorMA (ent.pos.origin, 64, forward, start);
			V_AddLight (start, 100, 1, 0, 0);
		}
		else
		{	// interpolate angles
			for (int i = 0; i < 3; i++)
				ent.angles[i] = LerpAngle (cent.prev.angles[i], cent.current.angles[i], cl.lerpfrac);
		}
		if (st->solid != 31)
			FNegate (ent.angles[2]);		// triangle models have bug in Q2: angles[2] should be negated

		if ((effects & (EF_BFG|EF_ANIM_ALLFAST)) == EF_BFG)
		{
			// BFG explosion: compute fx time
			//?? may check model name
			ent.time = cl.ftime - (st->frame + cl.lerpfrac) / 10.0f;
		}

		if (st->number == cl.playernum+1)
		{
			ent.flags |= RF_VIEWERMODEL;	// only draw from mirrors

			if (!(cl.refdef.rdflags & RDF_THIRD_PERSON))
			{
				if (effects & EF_FLAG1)
					V_AddLight (ent.pos.origin, 100, 1.0, 0.1, 0.1);
				else if (effects & EF_FLAG2)
					V_AddLight (ent.pos.origin, 100, 0.1, 0.1, 1.0);
				else if (effects & EF_TAGTRAIL)							//PGM
					V_AddLight (ent.pos.origin, 100, 1.0, 1.0, 0.0);	//PGM
				else if (effects & EF_TRACKERTRAIL)						//PGM
					V_AddLight (ent.pos.origin, 100, -1.0, -1.0, -1.0);	//PGM

				AddViewWeapon (renderfx);
				ParsePlayerEntity (cent, *ci, st, ent, NULL, 0);		// do not compute entities, but update animation frames
				continue;		//?? extend when implement mirrors (with renderfx!); attention: be sure not to add effects later (i.e. twice)
			}
		}

		// if set to invisible, skip
		if (!st->modelindex)
			continue;

		// XATRIX
		if (effects & EF_PLASMA)
		{
			//?? model is "sprites/s_photon.sp2"
			ent.flags |= RF_TRANSLUCENT;
			ent.alpha = 0.6;
		}

		if (effects & EF_SPHERETRANS)
		{
			//?? models/items/spawngro/tris.md2, models/items/spawngro2/tris.md2
			//?? no translucent in rogue/g_newtarg.c:SP_target_blacklight()
			ent.flags |= RF_TRANSLUCENT;
			ent.alpha = 0.3;
		}

		// send to renderer
		if (ci)
		{
			int weaponIndex = -1;
			if (st->modelindex2 == 255)		// linked weapon model
				weaponIndex = st->skinnum >> 8;
			if (!cl_vwep->integer || weaponIndex > MAX_CLIENTWEAPONMODELS-1)
				weaponIndex = 0;			// default weapon model

			entity_t buf[16];
			int numEnts = ParsePlayerEntity (cent, *ci, st, ent, ARRAY_ARG(buf), weaponIndex);
			for (int i = 0; i < numEnts; i++)
				AddEntityWithEffects2 (&buf[i], renderfx);
		}
		else
			AddEntityWithEffects (&ent, renderfx);

		ent.skin = NULL;		// never use a custom skin on others
		ent.skinnum = 0;
		ent.flags = 0;
		ent.alpha = 0;

		/*---------------------- add linked models --------------------------*/

		if (st->modelindex2 && st->modelindex2 != 255)	// 255 - weapon model; already added
		{
			ent.model = cl.model_draw[st->modelindex2];
			AddEntityWithEffects (&ent, renderfx);
		}
		if (st->modelindex3)	//?? this may be a flag; add for Q3 player model as a tag attachment
		{
			ent.model = cl.model_draw[st->modelindex3];
			AddEntityWithEffects (&ent, renderfx);
		}
		if (st->modelindex4)
		{
			ent.model = cl.model_draw[st->modelindex4];
			AddEntityWithEffects (&ent, renderfx);
		}

		/*------------------------ parse effects ----------------------------*/

		if (effects & EF_POWERSCREEN)
		{
			// add power screen for entity
			ent.model = cl_mod_powerscreen;
			ent.frame = ent.oldframe = 0;
			//?? shader
			ent.flags |= RF_TRANSLUCENT|RF_SHELL_GREEN;
			ent.alpha = 0.3f;
			AddEntityWithEffects (&ent, ent.flags | RF_SHELL_GREEN);
		}

		// add automatic particle trails and dlights
		if (!(effects & ~EF_ROTATE)) continue;

		if (effects & EF_ROCKET) {
			CL_RocketTrail (cent);
			V_AddLight (ent.pos.origin, 200, 1, 1, 0);
		}
		// PGM - Do not reorder EF_BLASTER and EF_HYPERBLASTER.
		// EF_BLASTER | EF_TRACKER is a special case for EF_BLASTER2... Cheese!
		else if (effects & EF_BLASTER) {
			if (effects & EF_TRACKER)	// lame... problematic?
			{
				CL_BlasterTrail2 (cent);
				V_AddLight (ent.pos.origin, 200, 0, 1, 0);
			}
			else
			{
				CL_BlasterTrail (cent);
				V_AddLight (ent.pos.origin, 200, 1, 1, 0);
			}
		} else if (effects & EF_HYPERBLASTER) {
			if (effects & EF_TRACKER)						// PGM	overloaded for blaster2.
				V_AddLight (ent.pos.origin, 200, 0, 1, 0);	// PGM
			else											// PGM
				V_AddLight (ent.pos.origin, 200, 1, 1, 0);
		} else if (effects & (EF_GIB|EF_GREENGIB|EF_GRENADE)) {
			CL_DiminishingTrail (cent, effects);
		} else if (effects & EF_FLIES) {
			CL_FlyEffect (cent);
		} else if (effects & EF_BFG) {
			if (effects & EF_ANIM_ALLFAST)
			{
				CL_BfgParticles (ent.pos.origin);
				float extra = frand () / 2;
				V_AddLight (ent.pos.origin, 200, extra, 1, extra);
			}
			else
			{
				static const float bfg_lightramp[7] = {200, 300, 400, 600, 300, 150, 75};
				float intens = Lerp (bfg_lightramp[st->frame], bfg_lightramp[st->frame+1], cl.lerpfrac);
				float bright = st->frame > 2 ? (5.0f - st->frame - cl.lerpfrac) / (5 - 2) : 1;
				V_AddLight (ent.pos.origin, intens, 0, bright, 0);
			}
//			RE_DrawTextLeft (va("bfg: %d (%c) [%3.1f]", st->frame, effects & EF_ANIM_ALLFAST ? '*' : ' ', cl.lerpfrac));//!!
		}
		// XATRIX
		else if (effects & EF_TRAP) {
			ent.pos.origin[2] += 32;
			CL_TrapParticles (ent.pos.origin);
			V_AddLight (ent.pos.origin, (rand()%100) + 100, 1, 0.8, 0.1);
		} else if (effects & EF_FLAG1) {
			CL_FlagTrail (cent, 0xF2);
			V_AddLight (ent.pos.origin, 100, 1, 0.1, 0.1);
		} else if (effects & EF_FLAG2) {
			CL_FlagTrail (cent, 0x73);
			V_AddLight (ent.pos.origin, 100, 0.1, 0.1, 1);
		}
		//ROGUE
		else if (effects & EF_TAGTRAIL) {
			CL_TagTrail (cent);
			V_AddLight (ent.pos.origin, 100, 1.0, 1.0, 0.0);
		} else if (effects & EF_TRACKERTRAIL) {
			if (effects & EF_TRACKER)
			{
				float intensity = 50 + (500 * (sin(cl.time/500.0f) + 1.0f));
				V_AddLight (ent.pos.origin, intensity, -1.0, -1.0, -1.0);	//?? neg light
			}
			else
			{
				CL_TrackerShell (ent.pos.origin);
				V_AddLight (ent.pos.origin, 155, -1.0, -1.0, -1.0);	//?? neg light
			}
		} else if (effects & EF_TRACKER) {
			CL_TrackerTrail (cent);
			V_AddLight (ent.pos.origin, 200, -1, -1, -1);	//?? neg light
		}
		// XATRIX
		else if (effects & EF_IONRIPPER) {
			CL_IonripperTrail (cent);
			V_AddLight (ent.pos.origin, 100, 1, 0.5, 0.5);
		} else if (effects & EF_BLUEHYPERBLASTER) {
			V_AddLight (ent.pos.origin, 200, 0, 0, 1);
		} else if (effects & EF_PLASMA) {
			if (effects & EF_ANIM_ALLFAST)
				CL_BlasterTrail (cent);
			V_AddLight (ent.pos.origin, 130, 1, 0.5, 0.5);
		}
	}
}


#define CAMERA_MINIMUM_DISTANCE	40

//#define FIXED_VIEW		// for debug purposes

void CL_OffsetThirdPersonView ()
{
	CVec3	forward, pos;
	trace_t	trace;
	float	camDist;//, dist;

	// algorithm was taken from FAKK2
	camDist = max(cl_cameraDist->value, CAMERA_MINIMUM_DISTANCE);
#ifdef FIXED_VIEW
	sscanf (Cvar_VariableString("3rd"), "%f %f %f", VECTOR_ARG(&cl.refdef.viewangles));
#endif
	CVec3 angles = cl.refdef.viewangles;
	angles[YAW] += cl_cameraAngle->value;
	AngleVectors (angles, &forward, NULL, NULL);
	VectorMA (cl.refdef.vieworg, -camDist, forward, pos);
	pos[2] += cl_cameraHeight->value;
	cl.refdef.viewangles[YAW] += cl_cameraAngle->value;

	static const CBox bounds = {{-5, -5, -5}, {5, 5, 5}};
	CL_Trace (trace, cl.refdef.vieworg, pos, bounds, MASK_SHOT|MASK_WATER);
	if (trace.fraction < 1)
		pos = trace.endpos;
#if 0
	dist = VectorDistance (pos, cl.refdef.vieworg);

	if (dist < CAMERA_MINIMUM_DISTANCE)
	{
		CVec3 angles = cl.refdef.viewangles;
		while (angles[PITCH] < 90)
		{
			angles[PITCH] += 2;
			AngleVectors (angles, &forward, NULL, NULL);
			VectorMA (cl.refdef.vieworg, -camDist, forward, pos);
			pos[2] += cl_cameraheight->value;

			trace = CM_BoxTrace (cl.refdef.vieworg, pos, bounds, 0, MASK_SHOT|MASK_WATER);
			pos = trace.endpos;
			dist = VectorDistance (pos, cl.refdef.vieworg);
			if (dist >= CAMERA_MINIMUM_DISTANCE)
			{
               cl.refdef.viewangles[PITCH] = (0.4f * angles[PITCH]) + (0.6f * cl.refdef.viewangles[PITCH]);
               break;
			}
        }
	}
#endif

	cl.refdef.vieworg = pos;
}


/*
===============
CL_CalcViewValues

Sets cl.refdef view values
===============
*/
static void CL_CalcViewValues (void)
{
	int			i;
	centity_t	*ent;
	player_state_t	*ps, *ops;

	// find the previous frame to interpolate from
	ps = &cl.frame.playerstate;
	ops = &cl.oldFrame->playerstate;

	// see if the player entity was teleported this frame
	if (abs(ops->pmove.origin[0] - ps->pmove.origin[0]) > 256*8 ||
		abs(ops->pmove.origin[1] - ps->pmove.origin[1]) > 256*8 ||
		abs(ops->pmove.origin[2] - ps->pmove.origin[2]) > 256*8)
		ops = ps;		// don't interpolate

	ent = &cl_entities[cl.playernum+1];
	float lerp = cl.lerpfrac;

	// calculate the origin
	if ((cl_predict->integer) && !(cl.frame.playerstate.pmove.pm_flags & PMF_NO_PREDICTION)
		&& !cl.attractloop)	// demos have no movement prediction ability (client commands are not stored)
	{	// use predicted values
		float backlerp = 1 - lerp;
		for (i = 0; i < 3; i++)
		{
			cl.modelorg[i] = cl.predicted_origin[i] - backlerp * cl.prediction_error[i];
			cl.refdef.vieworg[i] = cl.modelorg[i] + Lerp (ops->viewoffset[i], ps->viewoffset[i], lerp);
		}

		// smooth out stair climbing
		unsigned delta = cls.realtime - cl.predicted_step_time;
		if (delta < 100)
			cl.refdef.vieworg[2] -= cl.predicted_step * (100 - delta) * 0.01f;
	}
	else
	{	// just use interpolated values
		for (i = 0; i < 3; i++)
		{
			cl.modelorg[i] = ops->pmove.origin[i] * 0.125f + lerp * (ps->pmove.origin[i] - ops->pmove.origin[i]) * 0.125f;
			cl.refdef.vieworg[i] = cl.modelorg[i] + Lerp (ops->viewoffset[i], ps->viewoffset[i], lerp);
		}
	}

	// if not running a demo or on a locked frame, add the local angle movement
	if (cl.frame.playerstate.pmove.pm_type < PM_DEAD)
	{	// use predicted values
		for (i = 0; i < 3; i++)
			cl.refdef.viewangles[i] = cl.predicted_angles[i];
	}
	else
	{	// just use interpolated values
		for (i = 0; i < 3; i++)
			cl.refdef.viewangles[i] = LerpAngle (ops->viewangles[i], ps->viewangles[i], lerp);
	}

	for (i = 0; i < 3; i++)
		cl.refdef.viewangles[i] += LerpAngle (ops->kick_angles[i], ps->kick_angles[i], lerp);

	// interpolate field of view
	cl.refdef.fov_x = Lerp (ops->fov, ps->fov, lerp);

	for (i = 0; i < 4; i++)
		r_blend[i] = Lerp (ops->blend[i], ps->blend[i], lerp);

	if ((cl_3rd_person->integer || cl.frame.playerstate.stats[STAT_HEALTH] <= 0) &&
		cl.frame.playerstate.pmove.pm_type != PM_SPECTATOR)
	{
		cl.refdef.rdflags |= RDF_THIRD_PERSON;
		CL_OffsetThirdPersonView ();
		if (CM_PointContents (cl.refdef.vieworg, 0) & MASK_WATER)		//?? use different point
			cl.refdef.rdflags |= RDF_UNDERWATER;
		// compute cl.modelorg for 3rd person view from client entity
		ent = &cl_entities[cl.playernum+1];
		Lerp (ent->prev.origin, ent->current.origin, cl.lerpfrac, cl.modelorg);
	}

	AngleVectors (cl.refdef.viewangles, &cl.v_forward, &cl.v_right, &cl.v_up);
}

/*
===============
CL_AddEntities

Emits all entities, particles, and lights to the refresh
===============
*/
void CL_AddEntities (void)
{
	guard(CL_AddEntities);

	if (cls.state != ca_active)
		return;

	if (cl.time > cl.frame.servertime)
	{
		if (cl_showclamp->integer)
			Com_Printf ("high clamp %i\n", cl.time - cl.frame.servertime);
		cl.overtime += cl.time - cl.frame.servertime;
		cl.time = cl.frame.servertime;
		cl.ftime = cl.time / 1000.0f;
		cl.lerpfrac = 1.0;
	}
	else if (cl.time < cl.frame.servertime - 100)
	{
		if (cl_showclamp->integer)
			Com_Printf ("low clamp %i\n", cl.frame.servertime - 100 - cl.time);
		cl.overtime = 0;
		cl.time = cl.frame.servertime - 100;
		cl.ftime = cl.time / 1000.0f;
		cl.lerpfrac = 0;
	}
	else
		cl.lerpfrac = 1.0f - (cl.frame.servertime - cl.time) / 100.0f;		//?? use cl.ftime

	CL_CalcViewValues ();
	// PMM - moved this here so the heat beam has the right values for the vieworg, and can lock the beam to the gun
	CL_AddPacketEntities ();
	CL_AddDebugLines ();

	unguard;
}
