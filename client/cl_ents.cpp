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
// cl_ents.c -- entity parsing and management

#include "client.h"


extern	struct model_s	*cl_mod_powerscreen;

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
void CL_ParseDelta (entityState_t *from, entityState_t *to, int number, unsigned bits, bool baseline)
{
	guard(CL_ParseDelta);

	// set everything to the state we are delta'ing from
	*to = *from;

	VectorCopy (from->origin, to->old_origin);				// before MSG_ReadDeltaEntity()

	MSG_ReadDeltaEntity (&net_message, from, to, bits);

	to->number = number;

	//!! if remove line "if (bits & (...) || baseline) ...", can remove "baseline" arg and "ent->valid" field
//	if (bits & (U_ANGLE_N|U_MODEL_N) || baseline)
		AnglesToAxis (to->angles, to->axis);

//	if (bits & (U_SOLID|U_ANGLE_N|U_ORIGIN_N|U_MODEL_N) || baseline || !to->valid)
	{
		if (to->solid && to->solid != 31)
		{
			vec3_t	d;

			int x = 8 * (to->solid & 31);
			int zd = 8 * ((to->solid>>5) & 31);
			int zu = 8 * ((to->solid>>10) & 63) - 32;

			VectorSet (to->mins, -x, -x, -zd);
			VectorSet (to->maxs, x, x, zu);
			VectorAdd (to->maxs, to->mins, d);
			VectorMA (to->origin, 0.5f, d, to->center);
			to->radius = VectorDistance (to->maxs, to->mins) / 2;
			to->valid = true;
		}
		else
		{
			cmodel_t *m = cl.model_clip[to->modelindex];
			if (m)
			{
				vec3_t	v, tmp;

				VectorAdd (m->mins, m->maxs, v);
				VectorScale (v, 0.5f, v);
				VectorMA (to->origin, v[0], to->axis[0], tmp);
				VectorMA (tmp, v[1], to->axis[1], tmp);
				VectorMA (tmp, v[2], to->axis[2], to->center);
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
static void CL_DeltaEntity (frame_t *frame, int newnum, entityState_t *old, unsigned bits)
{
	centity_t	*ent;
	entityState_t	*state;

	ent = &cl_entities[newnum];

	state = &cl_parse_entities[cl.parse_entities & (MAX_PARSE_ENTITIES-1)];
	cl.parse_entities++;
	frame->num_entities++;

	CL_ParseDelta (old, state, newnum, bits, false);

	// some data changes will force no lerping
	if (state->modelindex != ent->current.modelindex ||
		state->modelindex2 != ent->current.modelindex2 ||
		state->modelindex3 != ent->current.modelindex3 ||
		state->modelindex4 != ent->current.modelindex4 ||
		fabs(state->origin[0] - ent->current.origin[0]) > 512 ||
		fabs(state->origin[1] - ent->current.origin[1]) > 512 ||
		fabs(state->origin[2] - ent->current.origin[2]) > 512 ||
		state->event == EV_PLAYER_TELEPORT ||
		state->event == EV_OTHER_TELEPORT
		)
	{
		ent->serverframe = -99;
	}

	if (ent->serverframe != cl.frame.serverframe - 1)
	{	// wasn't in last update, so initialize some things
		ent->trailcount = 1024;		// for diminishing rocket / grenade trails
		// duplicate the current state so lerping doesn't hurt anything
		ent->prev = *state;
		if (state->event == EV_OTHER_TELEPORT)
		{
			VectorCopy (state->origin, ent->prev.origin);
			VectorCopy (state->origin, ent->lerp_origin);
		}
		else
		{
			VectorCopy (state->old_origin, ent->prev.origin);
			VectorCopy (state->old_origin, ent->lerp_origin);
		}
	}
	else
	{	// shuffle the last state to previous
		ent->prev = ent->current;
	}

	ent->serverframe = cl.frame.serverframe;
	ent->current = *state;
}

/*
==================
CL_ParsePacketEntities

An svc_packetentities has just been parsed, deal with the
rest of the data stream.
==================
*/
void CL_ParsePacketEntities (frame_t *oldframe, frame_t *newframe)
{
	entityState_t *oldstate;
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
void CL_FireEntityEvents (frame_t *frame)
{
	for (int pnum = 0 ; pnum<frame->num_entities ; pnum++)
	{
		int num = (frame->parse_entities + pnum)&(MAX_PARSE_ENTITIES-1);
		entityState_t *s1 = &cl_parse_entities[num];
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
			cl.force_refdef = true;
			cl.predicted_origin[0] = cl.frame.playerstate.pmove.origin[0] * 0.125f;
			cl.predicted_origin[1] = cl.frame.playerstate.pmove.origin[1] * 0.125f;
			cl.predicted_origin[2] = cl.frame.playerstate.pmove.origin[2] * 0.125f;
			VectorCopy (cl.frame.playerstate.viewangles, cl.predicted_angles);
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

/*
==========================================================================

INTERPOLATE BETWEEN FRAMES TO GET RENDERING PARMS

==========================================================================
*/

struct model_s *S_RegisterSexedModel (entityState_t *ent, char *base)
{
	int		n;
	char	*p, model[MAX_QPATH];
	struct model_s *mdl;

	// determine what model the client is using
	model[0] = 0;
	n = CS_PLAYERSKINS + ent->number - 1;
	if (cl.configstrings[n][0])
	{
		p = strchr(cl.configstrings[n], '\\');
		if (p)
		{
			p += 1;
			strcpy(model, p);
			p = strchr(model, '/');
			if (p)
				*p = 0;
		}
	}
	// if we can't figure it out, they're male
	if (!model[0])
		strcpy(model, "male");

	mdl = re.RegisterModel (va("players/%s/%s", model, base+1));
	if (!mdl)
	{
		// not found, try default weapon model
		mdl = re.RegisterModel (va("players/%s/weapon.md2", model));
		if (!mdl)
		{
			// no, revert to the male model
			mdl = re.RegisterModel (va("players/%s/%s", "male", base+1));
			if (!mdl)
			{
				// last try, default male weapon.md2
				mdl = re.RegisterModel ("players/male/weapon.md2");
			}
		}
	}

	return mdl;
}


static void GetEntityInfo (int entityNum, entityState_t **st, unsigned *eff, unsigned *rfx)
{
	int		effects, renderfx;
	entityState_t *state;

	state = &cl_parse_entities[(cl.frame.parse_entities + entityNum) & (MAX_PARSE_ENTITIES-1)];
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

	if (st)  *st  = state;
	if (eff) *eff = effects;
	if (rfx) *rfx = renderfx;
}


static void AddEntityWithEffects (entity_t *ent, int fx)
{
	V_AddEntity (ent);

	// color shells generate a seperate entity for the main model
	if (fx & (RF_SHELL_RED|RF_SHELL_GREEN|RF_SHELL_BLUE|RF_SHELL_DOUBLE|RF_SHELL_HALF_DAM))
	{
		ent->flags |= (fx | RF_TRANSLUCENT);
		ent->alpha = 0.30;
		V_AddEntity (ent);
	}
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

	// don't draw gun if in wide angle view
	if (ps->fov > 90) return;

	entity_t	gun;		// view model
	memset (&gun, 0, sizeof(gun));

#ifdef GUN_DEBUG
	if (gun_model)
		gun.model = gun_model;
	else
#endif
		gun.model = cl.model_draw[ps->gunindex];
	if (!gun.model)
		return;

	// set up gun position
	for (int i = 0; i < 3; i++)
	{
		gun.origin[i] = cl.refdef.vieworg[i] + ops->gunoffset[i] + cl.lerpfrac * (ps->gunoffset[i] - ops->gunoffset[i]);
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
	gun.backlerp = 1.0 - cl.lerpfrac;
	VectorCopy (gun.origin, gun.oldorigin);	// don't lerp at all
	AddEntityWithEffects (&gun, renderfx);
}


void CL_AddEntityBox (entityState_t *st, unsigned rgba)
{
	entity_t	ent;
	memset (&ent, 0, sizeof(ent));

	centity_t *cent = &cl_entities[st->number];

	ent.flags = RF_BBOX;
	ent.color.rgba = rgba;

	VectorCopy (cent->current.angles, ent.angles);
	VectorCopy (cent->prev.center, ent.oldorigin);
	VectorCopy (cent->current.center, ent.origin);
	ent.backlerp = 1.0f - cl.lerpfrac;

	if (st->solid == 31)
	{
		cmodel_t *cmodel = cl.model_clip[st->modelindex];
		VectorSubtract (cmodel->maxs, cmodel->mins, ent.size);
		VectorScale (ent.size, 0.5f, ent.size);
	}
	else
	{
		ent.angles[2] = -ent.angles[2];
		VectorSubtract (st->maxs, st->mins, ent.size);
		VectorScale (ent.size, 0.5f, ent.size);
	}

	V_AddEntity (&ent);
}


static void CL_AddDebugLines (void)
{
	int			pnum;
	entityState_t *st;

	if (!cl_showbboxes->integer) return;

	for (pnum = 0; pnum < cl.frame.num_entities; pnum++)
	{
		st = &cl_parse_entities[(cl.frame.parse_entities+pnum)&(MAX_PARSE_ENTITIES-1)];
		if (!st->solid) continue;			// no collision with this entity

		CL_AddEntityBox (st, 0xFF808000);
	}
}


/*
===============
CL_AddPacketEntities

===============
*/
static void CL_AddPacketEntities (void)
{
	entity_t	ent;
	entityState_t *s1;
	float		autorotate;
	int			i, pnum;
	centity_t	*cent;
	clientinfo_t *ci;
	int			autoanim;
	unsigned 	effects, renderfx;

	// bonus items rotate at a fixed rate
	autorotate = anglemod(cl.ftime * 100);
	// brush models can auto animate their frames
	autoanim = 2 * cl.time / 1000;

	memset (&ent, 0, sizeof(ent));

	for (pnum = 0; pnum < cl.frame.num_entities; pnum++)
	{
		GetEntityInfo (pnum, &s1, &effects, &renderfx);
		s1 = &cl_parse_entities[(cl.frame.parse_entities+pnum)&(MAX_PARSE_ENTITIES-1)];
		cent = &cl_entities[s1->number];

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
			ent.frame = s1->frame;

		ent.oldframe = cent->prev.frame;
		ent.backlerp = 1.0f - cl.lerpfrac;

		if (renderfx & (RF_FRAMELERP|RF_BEAM))
		{	// step origin discretely, because the frames do the animation properly
			VectorCopy (cent->current.origin, ent.origin);
			VectorCopy (cent->current.old_origin, ent.oldorigin);
		}
		else
		{	// interpolate origin
			for (i = 0; i < 3; i++)
			{
				ent.origin[i] = ent.oldorigin[i] = cent->prev.origin[i] + cl.lerpfrac *
					(cent->current.origin[i] - cent->prev.origin[i]);
			}
		}

		// create a new entity

		// tweak the color of beams
		if (renderfx & RF_BEAM)
		{
			beam_t	*b;

			b = CL_AllocParticleBeam (ent.origin, ent.oldorigin, ent.frame / 2.0f, 0);
			if (!b) continue;
			b->type = BEAM_STANDARD;
			b->color.rgba = 0;
			// the four beam colors are encoded in 32 bits of skinnum (hack)
			b->color.c[0] = (s1->skinnum >> ((rand() % 4)*8)) & 0xFF;
			b->alpha = 0.3f;
			continue;
		}

		// set skin
		if (s1->modelindex == 255)
		{	// use custom player skin
			ent.skinnum = 0;
			ci = &cl.clientinfo[s1->skinnum & 0xFF];
			ent.skin = ci->skin;
			ent.model = ci->model;
			if (!ent.skin || !ent.model)
			{
				ent.skin = cl.baseclientinfo.skin;
				ent.model = cl.baseclientinfo.model;
			}

//============
//PGM
			if (renderfx & RF_USE_DISGUISE)
			{
				if(!memcmp((char *)ent.skin, "players/male", 12))
				{
					ent.skin = re.RegisterSkin ("players/male/disguise.pcx");
					ent.model = re.RegisterModel ("players/male/tris.md2");
				}
				else if(!memcmp((char *)ent.skin, "players/female", 14))
				{
					ent.skin = re.RegisterSkin ("players/female/disguise.pcx");
					ent.model = re.RegisterModel ("players/female/tris.md2");
				}
				else if(!memcmp((char *)ent.skin, "players/cyborg", 14))
				{
					ent.skin = re.RegisterSkin ("players/cyborg/disguise.pcx");
					ent.model = re.RegisterModel ("players/cyborg/tris.md2");
				}
			}
//PGM
//============
		}
		else
		{
			ent.skinnum = s1->skinnum;
			ent.skin = NULL;
			ent.model = cl.model_draw[s1->modelindex];
		}

		// only used for black hole model right now, FIXME: do better
		if (renderfx == RF_TRANSLUCENT)
			ent.alpha = 0.70;

		// render effects (fullbright, translucent, etc)
		if ((effects & EF_COLOR_SHELL))
			ent.flags = 0;	// renderfx go on color shell entity
		else
			ent.flags = renderfx;

		// calculate angles
		if (effects & EF_ROTATE)
		{	// some bonus items auto-rotate
			ent.angles[0] = 0;
			ent.angles[1] = autorotate;
			ent.angles[2] = 0;
		}
		// RAFAEL
		else if (effects & EF_SPINNINGLIGHTS)
		{
			vec3_t forward, start;

			ent.angles[0] = 0;
			ent.angles[1] = anglemod(cl.time/2) + s1->angles[1];
			ent.angles[2] = 180;
			AngleVectors (ent.angles, forward, NULL, NULL);
			VectorMA (ent.origin, 64, forward, start);
			V_AddLight (start, 100, 1, 0, 0);
		}
		else
		{	// interpolate angles
			float	a1, a2;

			for (i=0 ; i<3 ; i++)
			{
				a1 = cent->current.angles[i];
				a2 = cent->prev.angles[i];
				ent.angles[i] = LerpAngle (a2, a1, cl.lerpfrac);
			}
		}

		if (s1->number == cl.playernum+1)
		{
/*			Why disabled: works only for CURRENT player (not for others)
			And: some mods creates many color shells -- glow should be a server-side decision ...
			// add glow around player with COLOR_SHELL (have active powerups)
			if (renderfx & (RF_SHELL_RED|RF_SHELL_GREEN|RF_SHELL_BLUE))
			{
				float	r, g, b;

				r = g = b = frand () * 0.2f;
				if (renderfx & RF_SHELL_RED)	r = 1;
				if (renderfx & RF_SHELL_GREEN)	g = 1;
				if (renderfx & RF_SHELL_BLUE)	b = 1;
				V_AddLight (ent.origin, 96 + (frand() * 10), r, g, b);
			} */

			ent.flags |= RF_VIEWERMODEL;	// only draw from mirrors

			if (!(cl.refdef.rdflags & RDF_THIRD_PERSON))
			{
				if (effects & EF_FLAG1)
					V_AddLight (ent.origin, 100, 1.0, 0.1, 0.1);
				else if (effects & EF_FLAG2)
					V_AddLight (ent.origin, 100, 0.1, 0.1, 1.0);
				else if (effects & EF_TAGTRAIL)						//PGM
					V_AddLight (ent.origin, 100, 1.0, 1.0, 0.0);	//PGM
				else if (effects & EF_TRACKERTRAIL)					//PGM
					V_AddLight (ent.origin, 100, -1.0, -1.0, -1.0);	//PGM

				AddViewWeapon (renderfx);
				continue;		//?? extend when implement mirrors (with renderfx!); attention: be sure not to add effects later (i.e. twice)
			}
		}

		// if set to invisible, skip
		if (!s1->modelindex)
			continue;

		if (effects & EF_BFG)
		{
			ent.flags |= RF_TRANSLUCENT;
			ent.alpha = 0.30;
		}

		// RAFAEL
		if (effects & EF_PLASMA)
		{
			ent.flags |= RF_TRANSLUCENT;
			ent.alpha = 0.6;
		}

		if (effects & EF_SPHERETRANS)
		{
			ent.flags |= RF_TRANSLUCENT;
			// PMM - *sigh*  yet more EF overloading
			ent.alpha = (effects & EF_TRACKERTRAIL) ? 0.6 : 0.3;
		}
//pmm
		// add to refresh list
		AddEntityWithEffects (&ent, renderfx);

		ent.skin = NULL;		// never use a custom skin on others
		ent.skinnum = 0;
		ent.flags = 0;
		ent.alpha = 0;

		// duplicate for linked models
		if (s1->modelindex2)
		{
			if (s1->modelindex2 == 255)
			{	// custom weapon
				ci = &cl.clientinfo[s1->skinnum & 0xff];
				i = (s1->skinnum >> 8);
				if (!cl_vwep->integer || i > MAX_CLIENTWEAPONMODELS - 1)
					i = 0;		// 0 is default weapon model
				ent.model = ci->weaponmodel[i];
				if (!ent.model)
				{
					ent.model = ci->weaponmodel[0];
					if (!ent.model)	ent.model = cl.baseclientinfo.weaponmodel[0];
				}
			}
			else
				ent.model = cl.model_draw[s1->modelindex2];

			// PMM - check for the defender sphere shell .. make it translucent
			// replaces the previous version which used the high bit on modelindex2 to determine transparency
			if (!stricmp (cl.configstrings[CS_MODELS+(s1->modelindex2)], "models/items/shell/tris.md2"))
			{
				ent.alpha = 0.32;
				ent.flags = RF_TRANSLUCENT;
			}
			// pmm

			AddEntityWithEffects (&ent, renderfx);
//			V_AddEntity (&ent);

			//PGM - make sure these get reset.
			ent.flags = 0;
			ent.alpha = 0;
			//PGM
		}
		if (s1->modelindex3)
		{
			ent.model = cl.model_draw[s1->modelindex3];
			AddEntityWithEffects (&ent, renderfx);
//			V_AddEntity (&ent);
		}
		if (s1->modelindex4)
		{
			ent.model = cl.model_draw[s1->modelindex4];
			AddEntityWithEffects (&ent, renderfx);
//			V_AddEntity (&ent);
		}

		if (effects & EF_POWERSCREEN)
		{
			ent.model = cl_mod_powerscreen;
			ent.oldframe = 0;
			ent.frame = 0;
			ent.flags |= (RF_TRANSLUCENT | RF_SHELL_GREEN);
			ent.alpha = 0.30;
			V_AddEntity (&ent);
		}

		// add automatic particle trails
		if (effects & ~EF_ROTATE)
		{
			if (effects & EF_ROCKET)
			{
				CL_RocketTrail (cent->lerp_origin, ent.origin, cent);
				V_AddLight (ent.origin, 200, 1, 1, 0);
			}
			// PGM - Do not reorder EF_BLASTER and EF_HYPERBLASTER.
			// EF_BLASTER | EF_TRACKER is a special case for EF_BLASTER2... Cheese!
			else if (effects & EF_BLASTER)
			{
//				CL_BlasterTrail (cent->lerp_origin, ent.origin);
//PGM
				if (effects & EF_TRACKER)	// lame... problematic?
				{
					CL_BlasterTrail2 (cent->lerp_origin, ent.origin);
					V_AddLight (ent.origin, 200, 0, 1, 0);
				}
				else
				{
					CL_BlasterTrail (cent->lerp_origin, ent.origin);
					V_AddLight (ent.origin, 200, 1, 1, 0);
				}
//PGM
			}
			else if (effects & EF_HYPERBLASTER)
			{
				if (effects & EF_TRACKER)						// PGM	overloaded for blaster2.
					V_AddLight (ent.origin, 200, 0, 1, 0);		// PGM
				else											// PGM
					V_AddLight (ent.origin, 200, 1, 1, 0);
			}
			else if (effects & EF_GIB)
			{
				CL_DiminishingTrail (cent->lerp_origin, ent.origin, cent, effects);
			}
			else if (effects & EF_GRENADE)
			{
				CL_DiminishingTrail (cent->lerp_origin, ent.origin, cent, effects);
			}
			else if (effects & EF_FLIES)
			{
				CL_FlyEffect (cent, ent.origin);
			}
			else if (effects & EF_BFG)
			{
				if (effects & EF_ANIM_ALLFAST)
				{
					float	extra;

					CL_BfgParticles (&ent);
					extra = frand () * 0.5;
					V_AddLight (ent.origin, 200, extra, 1, extra);
				}
				else
				{
					static float bfg_lightramp[7] = {200, 300, 400, 600, 300, 150, 75};
					float	intens, prev, bright;

					intens = bfg_lightramp[s1->frame + 1];
					prev = bfg_lightramp[s1->frame];
					intens = prev * (1.0f - cl.lerpfrac) + intens * cl.lerpfrac;
					bright = s1->frame > 2 ? (5.0f - s1->frame) / (5 - 2) : 1;
					V_AddLight (ent.origin, intens, 0, bright, 0);
				}
//				re.DrawTextLeft(va("bfg: %d (%c) [%3.1f]", s1->frame, effects & EF_ANIM_ALLFAST ? '*' : ' ', cl.lerpfrac),RGB(1,1,1));//!!
			}
			// RAFAEL
			else if (effects & EF_TRAP)
			{
				ent.origin[2] += 32;
				CL_TrapParticles (&ent);
				i = (rand()%100) + 100;
				V_AddLight (ent.origin, i, 1, 0.8, 0.1);
			}
			else if (effects & EF_FLAG1)
			{
				CL_FlagTrail (cent->lerp_origin, ent.origin, 242);
				V_AddLight (ent.origin, 100, 1, 0.1, 0.1);
			}
			else if (effects & EF_FLAG2)
			{
				CL_FlagTrail (cent->lerp_origin, ent.origin, 115);
				V_AddLight (ent.origin, 100, 0.1, 0.1, 1);
			}
//======
//ROGUE
			else if (effects & EF_TAGTRAIL)
			{
				CL_TagTrail (cent->lerp_origin, ent.origin, 220);
				V_AddLight (ent.origin, 100, 1.0, 1.0, 0.0);
			}
			else if (effects & EF_TRACKERTRAIL)
			{
				if (effects & EF_TRACKER)
				{
					float intensity;

					intensity = 50 + (500 * (sin(cl.time/500.0) + 1.0));
					// FIXME - check out this effect in rendition
					V_AddLight (ent.origin, intensity, -1.0, -1.0, -1.0);	//?? neg light
				}
				else
				{
					CL_Tracker_Shell (cent->lerp_origin);
					V_AddLight (ent.origin, 155, -1.0, -1.0, -1.0);	//?? neg light
				}
			}
			else if (effects & EF_TRACKER)
			{
				CL_TrackerTrail (cent->lerp_origin, ent.origin, 0);
				// FIXME - check out this effect in rendition
				V_AddLight (ent.origin, 200, -1, -1, -1);	//?? neg light
			}
//ROGUE
//======
			// RAFAEL
			else if (effects & EF_GREENGIB)
			{
				CL_DiminishingTrail (cent->lerp_origin, ent.origin, cent, effects);
			}
			// RAFAEL
			else if (effects & EF_IONRIPPER)
			{
				CL_IonripperTrail (cent->lerp_origin, ent.origin);
				V_AddLight (ent.origin, 100, 1, 0.5, 0.5);
			}
			// RAFAEL
			else if (effects & EF_BLUEHYPERBLASTER)
			{
				V_AddLight (ent.origin, 200, 0, 0, 1);
			}
			// RAFAEL
			else if (effects & EF_PLASMA)
			{
				if (effects & EF_ANIM_ALLFAST)
				{
					CL_BlasterTrail (cent->lerp_origin, ent.origin);
				}
				V_AddLight (ent.origin, 130, 1, 0.5, 0.5);
			}
		}

		VectorCopy (ent.origin, cent->lerp_origin);
	}
}


#define CAMERA_MINIMUM_DISTANCE	40

//#define FIXED_VIEW		// for debug purposes

void CL_OffsetThirdPersonView (void)
{
	vec3_t	forward, pos;
	trace_t	trace;
	float	camDist;//, dist;
	static vec3_t mins = {-5, -5, -5}, maxs = {5, 5, 5};

	// algorithm was taken from FAKK2
	camDist = max(cl_cameradist->value, CAMERA_MINIMUM_DISTANCE);
#ifdef FIXED_VIEW
	sscanf (Cvar_VariableString("3rd"), "%g %g %g", VECTOR_ARG(&cl.refdef.viewangles));
#endif
	AngleVectors (cl.refdef.viewangles, forward, NULL, NULL);
	VectorMA (cl.refdef.vieworg, -camDist, forward, pos);
	pos[2] += cl_cameraheight->value;

	CL_Trace (&trace, cl.refdef.vieworg, pos, mins, maxs, MASK_SHOT|MASK_WATER);
	if (trace.fraction < 1)
		VectorCopy (trace.endpos, pos);
/*	dist = VectorDistance (pos, cl.refdef.vieworg);

	if (dist < CAMERA_MINIMUM_DISTANCE)
	{
		vec3_t	angles;

		VectorCopy (cl.refdef.viewangles, angles);
		while (angles[PITCH] < 90)
		{
			angles[PITCH] += 2;
			AngleVectors (angles, forward, NULL, NULL);
			VectorMA (cl.refdef.vieworg, -camDist, forward, pos);
			pos[2] += cl_cameraheight->value;

			trace = CM_BoxTrace (cl.refdef.vieworg, pos, mins, maxs, 0, MASK_SHOT|MASK_WATER);
			VectorCopy (trace.endpos, pos);
			dist = VectorDistance (pos, cl.refdef.vieworg);
			if (dist >= CAMERA_MINIMUM_DISTANCE)
			{
               cl.refdef.viewangles[PITCH] = (0.4f * angles[PITCH]) + (0.6f * cl.refdef.viewangles[PITCH]);
               break;
			}
        }
	} */

	VectorCopy(pos, cl.refdef.vieworg);
}


/*
===============
CL_CalcViewValues

Sets cl.refdef view values
===============
*/
void CL_CalcViewValues (void)
{
	int			i;
	float		lerp, backlerp;
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
	lerp = cl.lerpfrac;

	// calculate the origin
	if ((cl_predict->integer) && !(cl.frame.playerstate.pmove.pm_flags & PMF_NO_PREDICTION)
		&& !cl.attractloop)	// demos have no movement prediction ability (client commands are not stored)
	{	// use predicted values
		unsigned	delta;

		backlerp = 1.0 - lerp;
		for (i = 0; i < 3; i++)
		{
			cl.modelorg[i] = cl.predicted_origin[i] - backlerp * cl.prediction_error[i];
			cl.refdef.vieworg[i] = cl.modelorg[i] + ops->viewoffset[i] + lerp * (ps->viewoffset[i] - ops->viewoffset[i]);
		}

		// smooth out stair climbing
		delta = cls.realtime - cl.predicted_step_time;
		if (delta < 100)
			cl.refdef.vieworg[2] -= cl.predicted_step * (100 - delta) * 0.01f;
	}
	else
	{	// just use interpolated values
		for (i = 0; i < 3; i++)
		{
			cl.modelorg[i] = ops->pmove.origin[i] * 0.125f + lerp * (ps->pmove.origin[i] - ops->pmove.origin[i]) * 0.125f;
			cl.refdef.vieworg[i] = cl.modelorg[i] + ops->viewoffset[i] + lerp * (ps->viewoffset[i] - ops->viewoffset[i]);
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
	cl.refdef.fov_x = ops->fov + lerp * (ps->fov - ops->fov);

	for (i = 0; i < 4; i++)
//		cl.refdef.blend[i] = ps->blend[i]; // originally: no blend ??
		cl.refdef.blend[i] = ops->blend[i] + lerp * (ps->blend[i] - ops->blend[i]);

	if ((cl_3rd_person->integer || cl.frame.playerstate.stats[STAT_HEALTH] <= 0) &&
		cl.frame.playerstate.pmove.pm_type != PM_SPECTATOR)
	{
		cl.refdef.rdflags |= RDF_THIRD_PERSON;
		CL_OffsetThirdPersonView ();
		if (CM_PointContents (cl.refdef.vieworg, 0) & MASK_WATER)		//?? use different point
			cl.refdef.rdflags |= RDF_UNDERWATER;
		// compute cl.modelorg for 3rd person view from client entity
		ent = &cl_entities[cl.playernum+1];
		for (i = 0; i < 3; i++)
			cl.modelorg[i] = ent->prev.origin[i] + cl.lerpfrac * (ent->current.origin[i] - ent->prev.origin[i]);
	}

	AngleVectors (cl.refdef.viewangles, cl.v_forward, cl.v_right, cl.v_up);
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
		cl.lerpfrac = 1.0f - (cl.frame.servertime - cl.time) / 100.0f;

//	if (timedemo->integer)
//		cl.lerpfrac = 1.0;

	CL_CalcViewValues ();
	// PMM - moved this here so the heat beam has the right values for the vieworg, and can lock the beam to the gun
	CL_AddPacketEntities ();
	CL_AddTEnts ();
	CL_AddDLights ();
	CL_RunLightStyles ();	// migrated here from CL_Frame() because of clump time
	CL_AddDebugLines ();

	unguard;
}



/*
===============
CL_GetEntitySoundOrigin

Called to get the sound spatialization origin
===============
*/
void CL_GetEntitySoundOrigin (int ent, vec3_t org)
{
	centity_t	*old;

	if (ent < 0 || ent >= MAX_EDICTS)
		Com_DropError ("CL_GetEntitySoundOrigin: bad ent");
	old = &cl_entities[ent];
	VectorCopy (old->lerp_origin, org);

	// FIXME: bmodel issues...
}
