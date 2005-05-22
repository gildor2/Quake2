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
// cl_tent.cpp -- client side temporary entities

#include "client.h"
#include "snd_loc.h"


#define	MAX_BEAMS	32
typedef struct
{
	int		entity;
	int		dest_entity;
	CRenderModel *model;
	int		endtime;
	vec3_t	offset;
	vec3_t	start, end;
} mBeam_t;		// beam with model

static mBeam_t	cl_mBeams[MAX_BEAMS];

//PMM - added this for player-linked beams.  Currently only used by the plasma beam
static mBeam_t	cl_mPlayerbeams[MAX_BEAMS];

//ROGUE
cl_sustain_t	cl_sustains[MAX_SUSTAINS];
extern void CL_TeleportParticles (vec3_t org);

void CL_BlasterParticles (vec3_t org, vec3_t dir);
void CL_ExplosionParticles (vec3_t org);
void CL_BFGExplosionParticles (vec3_t org);
// XATRIX
void CL_BlueBlasterParticles (vec3_t org, vec3_t dir);

sfx_t *cl_sfx_ric[3];
sfx_t *cl_sfx_lashit;
sfx_t *cl_sfx_spark5;
sfx_t *cl_sfx_spark6;
sfx_t *cl_sfx_spark7;
sfx_t *cl_sfx_railg;
sfx_t *cl_sfx_rockexp;
sfx_t *cl_sfx_grenexp;
sfx_t *cl_sfx_watrexp;
// XATRIX
sfx_t *cl_sfx_plasexp;

sfx_t *cl_sfx_fallshort;
sfx_t *cl_sfx_footsteps[4];

CRenderModel *cl_mod_explode;
CRenderModel *cl_mod_smoke;
CRenderModel *cl_mod_flash;
CRenderModel *cl_mod_parasite_segment;
CRenderModel *cl_mod_grapple_cable;
CRenderModel *cl_mod_parasite_tip;
CRenderModel *cl_mod_explo4;
CRenderModel *cl_mod_bfg_explo;
CRenderModel *cl_mod_powerscreen;
// XATRIX
CRenderModel *cl_mod_plasmaexplo;

// ROGUE
sfx_t *cl_sfx_lightning;
sfx_t *cl_sfx_disrexp;
CRenderModel *cl_mod_lightning;
CRenderModel *cl_mod_heatbeam;
CRenderModel *cl_mod_monster_heatbeam;
CRenderModel *cl_mod_explo4_big;


//--------------- Extended protocol -------------------

#define NUM_STEP_MATERIALS	12

typedef enum {
	IMPACT_SILENT = -1,
	IMPACT_DIRT,
	IMPACT_METAL,
	IMPACT_STONE,
	IMPACT_TIN,
	IMPACT_WATER,
	IMPACT_WOOD,

	IMPACT_COUNT
} impactType_t;

static const impactType_t materialImpacts[NUM_STEP_MATERIALS + 1] = {
	IMPACT_SILENT,		// silent
	IMPACT_STONE,
	IMPACT_WOOD,
	IMPACT_DIRT,
	IMPACT_METAL,
	IMPACT_TIN,
	IMPACT_SILENT,		// snow
	IMPACT_TIN,
	IMPACT_STONE,
	IMPACT_WOOD,
	IMPACT_SILENT,		// water
	IMPACT_SILENT,		//!! glass
	IMPACT_DIRT
};

sfx_t *cl_sfx_footsteps2[NUM_STEP_MATERIALS*4];
sfx_t *cl_sfx_fallshort2[NUM_STEP_MATERIALS];
static sfx_t *cl_sfx_bullethits[IMPACT_COUNT * 3];

//?? static sfx_t *cl_sfx_spectator[4];
sfx_t *cl_sfx_camper[9];

static const char *footstepTypes[NUM_STEP_MATERIALS] = {
	"pavement",
	"rug",
	"gravel",
	"metal",
	"metal_light",
	"snow",
	"tin",
	"marble",
	"wood",
	"water",
	"glass",
	"dirt"
};

static const char *impactSounds[IMPACT_COUNT] = {
	"dirt",
	"metal",
	"stone",
	"tin",
	"water",
	"wood"
};

static const char *camperSounds[9] = {
	"tinyfart",
	"cough1",
	"cough2",
	"sneeze1",
	"sneeze2",
	"yawn1",
	"yawn2",
	"burp1",
	"spit1"
};


/*
=================
CL_RegisterTEntSounds
=================
*/

void CL_RegisterTEntSounds (void)
{
	int		i, j;

	for (i = 0; i < 3; i++)
		cl_sfx_ric[i] = S_RegisterSound (va("world/ric%d.wav", i + 1));

	cl_sfx_lashit = S_RegisterSound ("weapons/lashit.wav");
	cl_sfx_spark5 = S_RegisterSound ("world/spark5.wav");
	cl_sfx_spark6 = S_RegisterSound ("world/spark6.wav");
	cl_sfx_spark7 = S_RegisterSound ("world/spark7.wav");
	cl_sfx_railg = S_RegisterSound ("weapons/railgf1a.wav");
	cl_sfx_rockexp = S_RegisterSound ("weapons/rocklx1a.wav");
	cl_sfx_grenexp = S_RegisterSound ("weapons/grenlx1a.wav");
	cl_sfx_watrexp = S_RegisterSound ("weapons/xpld_wat.wav");
	// XATRIX
	// cl_sfx_plasexp = S_RegisterSound ("weapons/plasexpl.wav");

	cl_sfx_fallshort = S_RegisterSound ("player/land1.wav");
	S_RegisterSound ("player/fall1.wav");
	S_RegisterSound ("player/fall2.wav");

	// we need this sounds even if newprotocol is set in a case (see server/sv_main::SV_PostprocessFrame())
	// when trace.fraction == 1.0
	for (i = 0; i < 4; i++)
		cl_sfx_footsteps[i] = S_RegisterSound (va("player/step%d.wav", i + 1));

	if (cls.newprotocol)
	{
		sfx_t	*sfx;

		for (i = 0; i < NUM_STEP_MATERIALS; i++)
		{
			sfx = S_RegisterSound (va("player/footsteps/%s/land1.wav", footstepTypes[i]));
			cl_sfx_fallshort2[i] = (!sfx || sfx->absent) ? cl_sfx_fallshort : sfx;

			for (j = 0; j < 4; j++)
			{
				sfx = S_RegisterSound (va("player/footsteps/%s/step%d.wav", footstepTypes[i], j + 1));
				cl_sfx_footsteps2[i*4 + j] = (!sfx || sfx->absent) ? cl_sfx_footsteps[j] : sfx;
			}
		}
		for (i = 0; i < IMPACT_COUNT; i++)
		{
			for (j = 0; j < 3; j++)
			{
				sfx = S_RegisterSound (va("weapons/%s/impact%d.wav", impactSounds[i], j + 1));
				cl_sfx_bullethits[i*3 + j] = (!sfx || sfx->absent) ? cl_sfx_ric[j] : sfx;
			}
		}
/*		for (i = 0; i < 3; i++)
		{
			cl_sfx_spectator[i] = S_RegisterSound (va("player/spectator/spect%i.wav", i + 1));
		} */
		for (i = 0; i < 9; i++)
			cl_sfx_camper[i] = S_RegisterSound (va("player/reallife/%s.wav", camperSounds[i]));
	}

	//PGM
	cl_sfx_lightning = S_RegisterSound ("weapons/tesla.wav");
	cl_sfx_disrexp = S_RegisterSound ("weapons/disrupthit.wav");
}

/*
=================
CL_RegisterTEntModels
=================
*/

static CRenderModel *RegModel (const char *name)
{
	return RE_RegisterModel (va("models/%s/tris.md2", name));
}

void CL_RegisterTEntModels (void)
{
	static const char *modelNames[] = {
		"objects/laser",
		"objects/grenade2",
		"weapons/v_machn",
		"weapons/v_handgr",
		"weapons/v_shotg2",
		"objects/gibs/bone",
		"objects/gibs/sm_meat",
		"objects/gibs/bone2"
	};

	cl_mod_explode = RegModel ("objects/explode");
	cl_mod_smoke = RegModel ("objects/smoke");
	cl_mod_flash = RegModel ("objects/flash");
	cl_mod_parasite_segment = RegModel ("monsters/parasite/segment");
	cl_mod_grapple_cable = RegModel ("ctf/segment");
	cl_mod_parasite_tip = RegModel ("monsters/parasite/tip");
	cl_mod_explo4 = RegModel ("objects/r_explode");
	cl_mod_bfg_explo = RE_RegisterModel ("sprites/s_bfg2.sp2");
	cl_mod_powerscreen = RegModel ("items/armor/effect");

	for (int i = 0; i < ARRAY_COUNT(modelNames); i++)
		RegModel (modelNames[i]);
	// XARRIX
	// RegModel ("objects/blaser");

	//??
	RE_RegisterPic ("w_machinegun");
	RE_RegisterPic ("a_bullets");
	RE_RegisterPic ("i_health");
	RE_RegisterPic ("a_grenades");

	//ROGUE
	cl_mod_explo4_big = RegModel ("objects/r_explode2");
	cl_mod_lightning = RegModel ("proj/lightning");
	cl_mod_heatbeam = RegModel ("proj/beam");
	cl_mod_monster_heatbeam = RegModel ("proj/widowbeam");
}


typedef enum
{
	ex_free, ex_explosion, ex_misc, ex_flash, ex_mflash, ex_poly
} exptype_t;

typedef struct
{
	exptype_t	type;
	entity_t	ent;

	int			frames;			// lifetime in number of frames (0.1s units)
	float		light;
	vec3_t		lightcolor;
	float		time;
	int			baseframe;
} explosion_t;



#define	MAX_EXPLOSIONS	32
explosion_t	cl_explosions[MAX_EXPLOSIONS];
/*
=================
CL_ClearTEnts
=================
*/
void CL_ClearTEnts (void)
{
	memset (cl_mBeams, 0, sizeof(cl_mBeams));
	memset (cl_explosions, 0, sizeof(cl_explosions));

	//ROGUE
	memset (cl_mPlayerbeams, 0, sizeof(cl_mPlayerbeams));
	memset (cl_sustains, 0, sizeof(cl_sustains));
}

/*
=================
CL_AllocExplosion
=================
*/
explosion_t *CL_AllocExplosion (vec3_t origin, exptype_t type)
{
	int		i;
	explosion_t *ex, *dst;

	dst = NULL;
	// find another explosion with the same origin
	for (i = 0, ex = cl_explosions; i < MAX_EXPLOSIONS; i++, ex++)
	{
		if (ex->type == ex_free) continue;
		if (type == ex->type && VectorCompare (ex->ent.origin, origin))
		{
			dst = ex;
			break;
		}
    }

	// find free explosion
	if (!dst)
		for (i = 0, ex = cl_explosions; i < MAX_EXPLOSIONS; i++, ex++)
			if (ex->type == ex_free)
			{
				dst = ex;
				break;
			}

	if (!dst)
	{
		float	time;

		// find the oldest explosion (add priority for smoke/explode ??)
		time = 0;
		dst = cl_explosions;

		for (i = 1, ex = &cl_explosions[1]; i < MAX_EXPLOSIONS; i++, ex++)
			if (ex->time > time)
			{
				time = ex->time;
				dst = ex;
			}
	}

	memset (dst, 0, sizeof(explosion_t));
	VectorCopy (origin, dst->ent.origin);
	dst->time = -100;
	dst->type = type;
	return dst;
}


/*
=================
CL_AddExplosions
=================
*/
void CL_AddExplosions (void)
{
	entity_t	*ent;
	int			i, frm;
	explosion_t	*ex;
	float		 timeDelta, frac;
	static float oldTime;

	timeDelta = (cl.ftime - oldTime) * 1000.0f;		// msec
	oldTime = cl.ftime;

	if (r_sfx_pause->integer) timeDelta = 0;

	memset (&ent, 0, sizeof(ent));

	for (i = 0, ex = cl_explosions; i < MAX_EXPLOSIONS; i++, ex++)
	{
		if (ex->type == ex_free)
			continue;

		if (ex->time < 0)
			ex->time = 0;	// just started
		else
			ex->time += timeDelta;

		frac = ex->time / 100.0f;
		if (frac < 0) frac = 0;
		frm = appFloor (frac);

		if (frm >= ex->frames-1)
		{
			ex->type = ex_free;
			continue;
		}

		ent = &ex->ent;

		switch (ex->type)
		{
//		case ex_mflash:
//			break;

		case ex_misc:
			ent->alpha = 1.0f - frac / (ex->frames - 1);
			break;

		case ex_flash:
			ent->alpha = 1.0;
			break;

		case ex_poly:
//			ent->alpha = 1.0f - frm / 16.0f;		// ladder effect
			ent->alpha = 1.0f - frac / 16.0f;		// smooth
			if (frm < 10)
			{
				ent->skinnum = (frm >> 1);
				if (ent->skinnum < 0)
					ent->skinnum = 0;
			}
			else
			{
				ent->flags |= RF_TRANSLUCENT;
				if (frm < 13)
					ent->skinnum = 5;
				else
					ent->skinnum = 6;
			}
			break;
		}

//RE_DrawTextLeft (va("[%d / %g * %g] o(%g %g %g) c(%g %g %g)", i, ex->light, ent->alpha,
//VECTOR_ARG(ent->origin), VECTOR_ARG(ex->lightcolor)),RGB(1,1,1));//!!
		if (ex->light)
		{
//			V_AddLight (ent->origin, ex->light * ent->alpha, VECTOR_ARG(ex->lightcolor));
			V_AddLight (ent->origin, ex->light * ent->alpha,
				ex->lightcolor[0] * ent->alpha, ex->lightcolor[1] * ent->alpha, ex->lightcolor[2] * ent->alpha);
//			RE_DrawTextLeft (va("%d:%d = {%g %g %g} : %g %g %g : %g", ex->type, frm, VECTOR_ARG(ent->origin), ent->alpha, VECTOR_ARG(ex->lightcolor)),RGB(1,1,1));
		}

		if (!ent->model) continue;		// flash only

		VectorCopy (ent->origin, ent->oldorigin);

		ent->frame = ex->baseframe + frm + 1;
		ent->oldframe = ex->baseframe + frm;
		ent->backlerp = r_sfx_pause->integer ? 0 : 1.0f - (frac - frm);

		V_AddEntity (ent);
	}
}


/*
=================
CL_SmokeAndFlash
=================
*/
void CL_SmokeAndFlash(vec3_t origin)
{
	explosion_t	*ex;

	ex = CL_AllocExplosion (origin, ex_misc);
	ex->frames = 4;
	ex->ent.flags = RF_TRANSLUCENT;
	ex->ent.model = cl_mod_smoke;

	ex = CL_AllocExplosion (origin, ex_flash);
	ex->ent.flags = RF_FULLBRIGHT;
	ex->frames = 2;
	ex->ent.model = cl_mod_flash;
}

/*
=================
CL_ParseParticles
=================
*/
void CL_ParseParticles (void)
{
	int		color, count;
	vec3_t	pos, dir;

	MSG_ReadPos (&net_message, pos);
	MSG_ReadDir (&net_message, dir);

	color = MSG_ReadByte (&net_message);

	count = MSG_ReadByte (&net_message);

	CL_ParticleEffect (pos, dir, color, count);
}

/*
=================
CL_ParseBeam
=================
*/
int CL_ParseBeam (CRenderModel *model)
{
	int		ent;
	vec3_t	start, end;
	mBeam_t	*b;
	int		i;

	ent = MSG_ReadShort (&net_message);

	MSG_ReadPos (&net_message, start);
	MSG_ReadPos (&net_message, end);

	// override any beam with the same entity
	for (i = 0, b = cl_mBeams; i < MAX_BEAMS ; i++, b++)
		if (b->entity == ent)
		{
			b->entity = ent;
			b->model = model;
			b->endtime = cl.time + 200;
			VectorCopy (start, b->start);
			VectorCopy (end, b->end);
			VectorClear (b->offset);
			return ent;
		}

	// find a free beam
	for (i = 0, b = cl_mBeams; i < MAX_BEAMS ; i++, b++)
	{
		if (!b->model || b->endtime < cl.time)
		{
			b->entity = ent;
			b->model = model;
			b->endtime = cl.time + 200;
			VectorCopy (start, b->start);
			VectorCopy (end, b->end);
			VectorClear (b->offset);
			return ent;
		}
	}
	Com_Printf ("beam list overflow!\n");
	return ent;
}

/*
=================
CL_ParseBeam2
=================
*/
int CL_ParseBeam2 (CRenderModel *model)
{
	int		ent;
	vec3_t	start, end, offset;
	mBeam_t	*b;
	int		i;

	ent = MSG_ReadShort (&net_message);

	MSG_ReadPos (&net_message, start);
	MSG_ReadPos (&net_message, end);
	MSG_ReadPos (&net_message, offset);

//	Com_Printf ("end- %f %f %f\n", VECTOR_ARG(end));

	// override any beam with the same entity
	for (i = 0, b = cl_mBeams; i < MAX_BEAMS; i++, b++)
		if (b->entity == ent)
		{
			b->entity = ent;
			b->model = model;
			b->endtime = cl.time + 200;
			VectorCopy (start, b->start);
			VectorCopy (end, b->end);
			VectorCopy (offset, b->offset);
			return ent;
		}

	// find a free beam
	for (i = 0, b = cl_mBeams; i < MAX_BEAMS ; i++, b++)
	{
		if (!b->model || b->endtime < cl.time)
		{
			b->entity = ent;
			b->model = model;
			b->endtime = cl.time + 200;
			VectorCopy (start, b->start);
			VectorCopy (end, b->end);
			VectorCopy (offset, b->offset);
			return ent;
		}
	}
	Com_Printf ("beam list overflow!\n");
	return ent;
}

// ROGUE
/*
=================
CL_ParsePlayerBeam
  - adds to the cl_mPlayerbeam array instead of the cl_mBeams array
=================
*/
int CL_ParsePlayerBeam (CRenderModel *model)
{
	int		ent;
	vec3_t	start, end, offset;
	mBeam_t	*b;
	int		i;

	ent = MSG_ReadShort (&net_message);

	MSG_ReadPos (&net_message, start);
	MSG_ReadPos (&net_message, end);
	// PMM - network optimization
	if (model == cl_mod_heatbeam)
		VectorSet(offset, 2, 7, -3);
	else if (model == cl_mod_monster_heatbeam)
	{
		model = cl_mod_heatbeam;
		VectorSet(offset, 0, 0, 0);
	}
	else
		MSG_ReadPos (&net_message, offset);

//	Com_Printf ("end- %f %f %f\n", VECTOR_ARG(end));

	// override any beam with the same entity
	// PMM - For player beams, we only want one per player (entity) so..
	for (i = 0, b = cl_mPlayerbeams; i < MAX_BEAMS ; i++, b++)
	{
		if (b->entity == ent)
		{
			b->entity = ent;
			b->model = model;
			b->endtime = cl.time + 200;
			VectorCopy (start, b->start);
			VectorCopy (end, b->end);
			VectorCopy (offset, b->offset);
			return ent;
		}
	}

	// find a free beam
	for (i = 0, b = cl_mPlayerbeams; i < MAX_BEAMS ; i++, b++)
	{
		if (!b->model || b->endtime < cl.time)
		{
			b->entity = ent;
			b->model = model;
			b->endtime = cl.time + 100;		// PMM - this needs to be 100 to prevent multiple heatbeams
			VectorCopy (start, b->start);
			VectorCopy (end, b->end);
			VectorCopy (offset, b->offset);
			return ent;
		}
	}
	Com_Printf ("beam list overflow!\n");
	return ent;
}
//rogue

/*
=================
CL_ParseLightning
=================
*/
int CL_ParseLightning (CRenderModel *model)
{
	int		srcEnt, destEnt;
	vec3_t	start, end;
	mBeam_t	*b;
	int		i;

	srcEnt = MSG_ReadShort (&net_message);
	destEnt = MSG_ReadShort (&net_message);

	MSG_ReadPos (&net_message, start);
	MSG_ReadPos (&net_message, end);

	// override any beam with the same source AND destination entities
	for (i = 0, b = cl_mBeams; i < MAX_BEAMS ; i++, b++)
		if (b->entity == srcEnt && b->dest_entity == destEnt)
		{
//			Com_Printf("%d: OVERRIDE  %d -> %d\n", cl.time, srcEnt, destEnt);
			b->entity = srcEnt;
			b->dest_entity = destEnt;
			b->model = model;
			b->endtime = cl.time + 200;
			VectorCopy (start, b->start);
			VectorCopy (end, b->end);
			VectorClear (b->offset);
			return srcEnt;
		}

	// find a free beam
	for (i = 0, b = cl_mBeams; i < MAX_BEAMS ; i++, b++)
	{
		if (!b->model || b->endtime < cl.time)
		{
//			Com_Printf("%d: NORMAL  %d -> %d\n", cl.time, srcEnt, destEnt);
			b->entity = srcEnt;
			b->dest_entity = destEnt;
			b->model = model;
			b->endtime = cl.time + 200;
			VectorCopy (start, b->start);
			VectorCopy (end, b->end);
			VectorClear (b->offset);
			return srcEnt;
		}
	}
	Com_Printf ("beam list overflow!\n");
	return srcEnt;
}

/*
=================
CL_ParseLaser
=================
*/
// Used for BFG laser only !
void CL_ParseLaser (unsigned colors)
{
	vec3_t	start, end;
	beam_t	*b;

	MSG_ReadPos (&net_message, start);
	MSG_ReadPos (&net_message, end);

	b = CL_AllocParticleBeam (start, end, 2.0f, 0.1f);
	if (!b) return;

	b->type = BEAM_STANDARD;
	b->color.rgba = 0;
	// the four beam colors are encoded in 32 bits of skinnum (hack)
	b->color.c[0] = (colors >> ((rand() % 4)*8)) & 0xFF;
	b->alpha = b->dstAlpha = 0.3f;
}

//=============
//ROGUE
void CL_ParseSteam (void)
{
	vec3_t	pos, dir;
	int		id, i;
	int		r;
	int		cnt;
	int		color;
	int		magnitude;
	cl_sustain_t	*s, *free_sustain;

	id = MSG_ReadShort (&net_message);		// an id of -1 is an instant effect
	if (id != -1) // sustains
	{
//			Com_Printf ("Sustain effect id %d\n", id);
		free_sustain = NULL;
		for (i=0, s=cl_sustains; i<MAX_SUSTAINS; i++, s++)
		{
			if (s->id == 0)
			{
				free_sustain = s;
				break;
			}
		}
		if (free_sustain)
		{
			s->id = id;
			s->count = MSG_ReadByte (&net_message);
			MSG_ReadPos (&net_message, s->org);
			MSG_ReadDir (&net_message, s->dir);
			r = MSG_ReadByte (&net_message);
			s->color = r & 0xff;
			s->magnitude = MSG_ReadShort (&net_message);
			s->endtime = cl.time + MSG_ReadLong (&net_message);
			s->think = CL_ParticleSteamEffect2;
			s->thinkinterval = 100;
			s->nextthink = cl.time;
		}
		else
		{
//				Com_Printf ("No free sustains!\n");
			// FIXME - read the stuff anyway
			cnt = MSG_ReadByte (&net_message);
			MSG_ReadPos (&net_message, pos);
			MSG_ReadDir (&net_message, dir);
			r = MSG_ReadByte (&net_message);
			magnitude = MSG_ReadShort (&net_message);
			magnitude = MSG_ReadLong (&net_message); // really interval
		}
	}
	else // instant
	{
		cnt = MSG_ReadByte (&net_message);
		MSG_ReadPos (&net_message, pos);
		MSG_ReadDir (&net_message, dir);
		r = MSG_ReadByte (&net_message);
		magnitude = MSG_ReadShort (&net_message);
		color = r & 0xff;
		CL_ParticleSteamEffect (pos, dir, color, cnt, magnitude);
//		S_StartSound (pos,  0, 0, cl_sfx_lashit, 1, ATTN_NORM, 0);
	}
}

void CL_ParseWidow (void)
{
	vec3_t	pos;
	int		id, i;
	cl_sustain_t	*s, *free_sustain;

	id = MSG_ReadShort (&net_message);

	free_sustain = NULL;
	for (i=0, s=cl_sustains; i<MAX_SUSTAINS; i++, s++)
	{
		if (s->id == 0)
		{
			free_sustain = s;
			break;
		}
	}
	if (free_sustain)
	{
		s->id = id;
		MSG_ReadPos (&net_message, s->org);
		s->endtime = cl.time + 2100;
		s->think = CL_Widowbeamout;
		s->thinkinterval = 1;
		s->nextthink = cl.time;
	}
	else // no free sustains
	{
		// FIXME - read the stuff anyway
		MSG_ReadPos (&net_message, pos);
	}
}

void CL_ParseNuke (void)
{
	vec3_t	pos;
	int		i;
	cl_sustain_t	*s, *free_sustain;

	free_sustain = NULL;
	for (i=0, s=cl_sustains; i<MAX_SUSTAINS; i++, s++)
	{
		if (s->id == 0)
		{
			free_sustain = s;
			break;
		}
	}
	if (free_sustain)
	{
		s->id = 21000;
		MSG_ReadPos (&net_message, s->org);
		s->endtime = cl.time + 1000;
		s->think = CL_Nukeblast;
		s->thinkinterval = 1;
		s->nextthink = cl.time;
	}
	else // no free sustains
	{
		// FIXME - read the stuff anyway
		MSG_ReadPos (&net_message, pos);
	}
}

//ROGUE
//=============


/*
=================
CL_ParseTEnt
=================
*/

void CL_ParseTEnt (void)
{
	vec3_t	pos, pos2, dir;
	explosion_t	*ex;
	int		cnt, color, ent;

	int type = MSG_ReadByte (&net_message);
	switch (type)
	{
	case TE_BLOOD:			// bullet hitting flesh
		MSG_ReadPos (&net_message, pos);
		MSG_ReadDir (&net_message, dir);
		CL_ParticleEffect (pos, dir, 0xe8, 60);
		break;

	case TE_GUNSHOT:			// bullet hitting wall
	case TE_SPARKS:
	case TE_BULLET_SPARKS:
		MSG_ReadPos (&net_message, pos);
		MSG_ReadDir (&net_message, dir);

		if (cl_newfx->integer)
		{	//!! check for metal surface (smoke for non-metals) + check for "type" (see below) (see KP)
			// AND: should make sparks sometimes not appear (or very short lifetime ?)
			CL_MetalSparks (pos, dir, rand() % 5 + 1);
		}
		else
		{
			if (type == TE_GUNSHOT)
				CL_ParticleEffect (pos, dir, 0, 40);
			else
				CL_ParticleEffect (pos, dir, 0xE0, 6);
		}

		if (type != TE_SPARKS)
		{
			//!! Add stuff for bulletholes here
			if (cls.newprotocol)
			{
				trace_t	trace;
				vec3_t	zero = {0, 0, 0};
				vec3_t	start, end;

				VectorAdd (pos, dir, start);
				VectorMA (start, -2, dir, end);
				CL_Trace (&trace, start, end, zero, zero, MASK_ALL);
				if (trace.fraction < 1.0)
				{
					csurface_t *surf = trace.surface;
					int impactType = materialImpacts[surf->material];
					if (impactType < 0) break;		// silent

					CL_SmokeAndFlash (pos);
					S_StartSound (pos, 0, 0, cl_sfx_bullethits[impactType*3 + rand()%3], 1, ATTN_NORM, 0);
					break;
				}
			}

			CL_SmokeAndFlash(pos);

			// impact sound
			cnt = rand()%10;//&15;
			if (cnt >= 0 && cnt <= 2)
				S_StartSound (pos, 0, 0, cl_sfx_ric[cnt], 1, ATTN_NORM, 0);
		}

		break;

	case TE_SCREEN_SPARKS:
	case TE_SHIELD_SPARKS:
		MSG_ReadPos (&net_message, pos);
		MSG_ReadDir (&net_message, dir);
		if (type == TE_SCREEN_SPARKS)
			CL_ParticleEffect (pos, dir, 0xd0, 40);
		else
			CL_ParticleEffect (pos, dir, 0xb0, 40);
		//FIXME : replace or remove this sound
		S_StartSound (pos, 0, 0, cl_sfx_lashit, 1, ATTN_NORM, 0);
		break;

	case TE_SHOTGUN:			// bullet hitting wall
		MSG_ReadPos (&net_message, pos);
		MSG_ReadDir (&net_message, dir);
		CL_ParticleEffect (pos, dir, 0, 20);
		CL_SmokeAndFlash(pos);
		break;

	case TE_SPLASH:
		{
			cnt = MSG_ReadByte (&net_message);
			MSG_ReadPos (&net_message, pos);
			MSG_ReadDir (&net_message, dir);
			int r = MSG_ReadByte (&net_message);
			if (r > 8)
				color = 0x00;
			else
			{
				static byte splash_color[] = {
					0x00, 0xE0, 0xB0, 0x50, 0xD0, 0xE0, 0xE8,	// unk, sparks, blue water, brown water, slime, lava, blood
					0xB0, 0x50			// bullet blue water, bullet brown water
				};
				color = splash_color[r];
			}
			CL_ParticleEffect (pos, dir, color, cnt);

			if (r == SPLASH_SPARKS)
			{
				// just particles -- not water
				r = rand() & 3;
				if (r == 0)
					S_StartSound (pos, 0, 0, cl_sfx_spark5, 1, ATTN_STATIC, 0);
				else if (r == 1)
					S_StartSound (pos, 0, 0, cl_sfx_spark6, 1, ATTN_STATIC, 0);
				else
					S_StartSound (pos, 0, 0, cl_sfx_spark7, 1, ATTN_STATIC, 0);
			}
			else if ((r == SPLASH_BULLET_BLUE_WATER || r == SPLASH_BULLET_BROWN_WATER) && cls.newprotocol)
			{
				// bullet hitting water
				S_StartSound (pos, 0, 0, cl_sfx_bullethits[IMPACT_WATER*3 + rand()%3], 1, ATTN_NORM, 0);
			}
		}
		break;

	case TE_LASER_SPARKS:
		cnt = MSG_ReadByte (&net_message);
		MSG_ReadPos (&net_message, pos);
		MSG_ReadDir (&net_message, dir);
		color = MSG_ReadByte (&net_message);
		CL_ParticleEffect2 (pos, dir, color, cnt);
		break;

	// RAFAEL
	case TE_BLUEHYPERBLASTER:
		MSG_ReadPos (&net_message, pos);
		MSG_ReadPos (&net_message, dir);
		CL_BlasterParticles (pos, dir);
		break;

	case TE_BLASTER:			// blaster hitting wall
		MSG_ReadPos (&net_message, pos);
		MSG_ReadDir (&net_message, dir);
		CL_BlasterParticles (pos, dir);

		ex = CL_AllocExplosion (pos, ex_misc);
		ex->ent.angles[0] = acos(dir[2])/M_PI*180;
		// PMM - fixed to correct for pitch of 0
		if (dir[0])
			ex->ent.angles[1] = atan2(dir[1], dir[0])/M_PI*180;
		else if (dir[1] > 0)
			ex->ent.angles[1] = 90;
		else if (dir[1] < 0)
			ex->ent.angles[1] = 270;
		else
			ex->ent.angles[1] = 0;

		ex->ent.flags = RF_FULLBRIGHT|RF_TRANSLUCENT;
		ex->light = 150;
		ex->lightcolor[0] = 1;
		ex->lightcolor[1] = 1;
		ex->ent.model = cl_mod_explode;
		ex->frames = 4;
		S_StartSound (pos,  0, 0, cl_sfx_lashit, 1, ATTN_NORM, 0);
		break;

	case TE_RAILTRAIL:			// railgun effect
		MSG_ReadPos (&net_message, pos);
		MSG_ReadPos (&net_message, pos2);
		if (cls.newprotocol)
			CL_RailTrail (pos, pos2);
		else
			CL_RailTrailExt (pos, pos2, 1, 0);
		S_StartSound (pos2, 0, 0, cl_sfx_railg, 1, ATTN_NORM, 0);
		break;

	case TE_RAILTRAIL_EXT:		// advanced railgun effect
		{
			MSG_ReadPos (&net_message, pos);
			MSG_ReadPos (&net_message, pos2);
			byte rColor = MSG_ReadByte (&net_message);
			byte rType = MSG_ReadByte (&net_message);
			CL_RailTrailExt (pos, pos2, rType, rColor);
			S_StartSound (pos2, 0, 0, cl_sfx_railg, 1, ATTN_NORM, 0);
		}
		break;

	case TE_EXPLOSION2:
	case TE_GRENADE_EXPLOSION:
	case TE_GRENADE_EXPLOSION_WATER:
		MSG_ReadPos (&net_message, pos);

		ex = CL_AllocExplosion (pos, ex_poly);
		ex->ent.flags = RF_FULLBRIGHT;
		ex->light = 350;
		ex->lightcolor[0] = 1.0;
		ex->lightcolor[1] = 0.5;
		ex->lightcolor[2] = 0.5;
		ex->ent.model = cl_mod_explo4;
		ex->frames = 15;	//?? original: max frame was 19; for this explosion type should be 15 (can be negative ex->alpha)
		ex->baseframe = 30;
		ex->ent.angles[1] = rand() % 360;
		CL_ExplosionParticles (pos);
		if (type == TE_GRENADE_EXPLOSION_WATER)
			S_StartSound (pos, 0, 0, cl_sfx_watrexp, 1, ATTN_NORM, 0);
		else
			S_StartSound (pos, 0, 0, cl_sfx_grenexp, 1, ATTN_NORM, 0);
		break;

	// XATRIX
	case TE_PLASMA_EXPLOSION:
		MSG_ReadPos (&net_message, pos);
		ex = CL_AllocExplosion (pos, ex_poly);
		ex->ent.flags = RF_FULLBRIGHT;
		ex->light = 350;
		ex->lightcolor[0] = 1.0;
		ex->lightcolor[1] = 0.5;
		ex->lightcolor[2] = 0.5;
		ex->ent.angles[1] = rand() % 360;
		ex->ent.model = cl_mod_explo4;
		if (frand() < 0.5)
			ex->baseframe = 15;
		ex->frames = 15;
		CL_ExplosionParticles (pos);
		S_StartSound (pos, 0, 0, cl_sfx_rockexp, 1, ATTN_NORM, 0);
		break;

	case TE_EXPLOSION1:
	case TE_EXPLOSION1_BIG:						// PMM
	case TE_ROCKET_EXPLOSION:
	case TE_ROCKET_EXPLOSION_WATER:
	case TE_EXPLOSION1_NP:						// PMM
		MSG_ReadPos (&net_message, pos);

		ex = CL_AllocExplosion (pos, ex_poly);
		ex->ent.flags = RF_FULLBRIGHT;
		ex->light = 350;
		ex->lightcolor[0] = 1.0;
		ex->lightcolor[1] = 0.5;
		ex->lightcolor[2] = 0.5;
		ex->ent.angles[1] = rand() % 360;
		ex->ent.model = (type != TE_EXPLOSION1_BIG) ? cl_mod_explo4 : cl_mod_explo4_big;			// PMM
		if (frand() < 0.5)
			ex->baseframe = 15;
		ex->frames = 15;
		if ((type != TE_EXPLOSION1_BIG) && (type != TE_EXPLOSION1_NP))		// PMM
			CL_ExplosionParticles (pos);									// PMM
		S_StartSound (pos, 0, 0, (type == TE_ROCKET_EXPLOSION_WATER) ? cl_sfx_watrexp : cl_sfx_rockexp,
			1, ATTN_NORM, 0);
		break;

	case TE_BFG_EXPLOSION:
		MSG_ReadPos (&net_message, pos);
		ex = CL_AllocExplosion (pos, ex_poly);
		ex->ent.flags = RF_FULLBRIGHT;
		ex->light = 350;
		ex->lightcolor[0] = 0.0;
		ex->lightcolor[1] = 1.0;
		ex->lightcolor[2] = 0.0;
		ex->ent.model = cl_mod_bfg_explo;
		ex->ent.flags |= RF_TRANSLUCENT;
		ex->ent.alpha = 0.30;
		ex->frames = 4;
		break;

	case TE_BFG_BIGEXPLOSION:
		MSG_ReadPos (&net_message, pos);
		CL_BFGExplosionParticles (pos);
		break;

	case TE_BFG_LASER:
		CL_ParseLaser (0xD0D1D2D3);
		break;

	case TE_BUBBLETRAIL:
		MSG_ReadPos (&net_message, pos);
		MSG_ReadPos (&net_message, pos2);
		CL_BubbleTrail (pos, pos2);
		break;

	case TE_PARASITE_ATTACK:
	case TE_MEDIC_CABLE_ATTACK:
		ent = CL_ParseBeam (cl_mod_parasite_segment);
		break;

	case TE_BOSSTPORT:					// boss teleporting to station
		MSG_ReadPos (&net_message, pos);
		CL_BigTeleportParticles (pos);
		S_StartSound (pos, 0, 0, S_RegisterSound ("misc/bigtele.wav"), 1, ATTN_NONE, 0);
		break;

	case TE_GRAPPLE_CABLE:
		ent = CL_ParseBeam2 (cl_mod_grapple_cable);
		break;

	// XATRIX
	case TE_WELDING_SPARKS:
		cnt = MSG_ReadByte (&net_message);
		MSG_ReadPos (&net_message, pos);
		MSG_ReadDir (&net_message, dir);
		color = MSG_ReadByte (&net_message);
		CL_ParticleEffect2 (pos, dir, color, cnt);

		ex = CL_AllocExplosion (pos, ex_flash);
		ex->light = 100 + (rand()%75);
		ex->lightcolor[0] = 1.0f;
		ex->lightcolor[1] = 1.0f;
		ex->lightcolor[2] = 0.3f;
		ex->ent.model = NULL;			// flash only
		ex->frames = 2;
		break;

	case TE_GREENBLOOD:
		MSG_ReadPos (&net_message, pos);
		MSG_ReadDir (&net_message, dir);
		CL_ParticleEffect2 (pos, dir, 0xDF, 30);
		break;

	// XATRIX
	case TE_TUNNEL_SPARKS:
		cnt = MSG_ReadByte (&net_message);
		MSG_ReadPos (&net_message, pos);
		MSG_ReadDir (&net_message, dir);
		color = MSG_ReadByte (&net_message);
		CL_ParticleEffect3 (pos, dir, color, cnt);
		break;

	// ROGUE
	// PMM - following code integrated for flechette (different color)
	case TE_BLASTER2:			// green blaster hitting wall
	case TE_FLECHETTE:			// flechette
		MSG_ReadPos (&net_message, pos);
		MSG_ReadDir (&net_message, dir);

		CL_BlasterParticles2 (pos, dir, (type == TE_BLASTER2) ? 0xD0 : 0x6F);

		ex = CL_AllocExplosion (pos, ex_misc);
		ex->ent.angles[0] = acos(dir[2])/M_PI*180;
		// PMM - fixed to correct for pitch of 0
		if (dir[0])
			ex->ent.angles[1] = atan2(dir[1], dir[0])/M_PI*180;
		else if (dir[1] > 0)
			ex->ent.angles[1] = 90;
		else if (dir[1] < 0)
			ex->ent.angles[1] = 270;
		else
			ex->ent.angles[1] = 0;

		ex->ent.flags = RF_FULLBRIGHT|RF_TRANSLUCENT;
		ex->ent.skinnum = (type == TE_BLASTER2) ? 1 : 2;
		ex->light = 150;
		if (type == TE_BLASTER2)
			ex->lightcolor[1] = 1;
		else // flechette
		{
			ex->lightcolor[0] = 0.19;
			ex->lightcolor[1] = 0.41;
			ex->lightcolor[2] = 0.75;
		}
		ex->ent.model = cl_mod_explode;
		ex->frames = 4;
		S_StartSound (pos,  0, 0, cl_sfx_lashit, 1, ATTN_NORM, 0);
		break;


	case TE_LIGHTNING:
		ent = CL_ParseLightning (cl_mod_lightning);
		S_StartSound (NULL, ent, CHAN_WEAPON, cl_sfx_lightning, 1, ATTN_NORM, 0);
		break;

	case TE_DEBUGTRAIL:
		MSG_ReadPos (&net_message, pos);
		MSG_ReadPos (&net_message, pos2);
		CL_DebugTrail (pos, pos2);
		break;

	case TE_PLAIN_EXPLOSION:
		MSG_ReadPos (&net_message, pos);

		ex = CL_AllocExplosion (pos, ex_poly);
		ex->ent.flags = RF_FULLBRIGHT;
		ex->light = 350;
		ex->lightcolor[0] = 1.0;
		ex->lightcolor[1] = 0.5;
		ex->lightcolor[2] = 0.5;
		ex->ent.angles[1] = rand() % 360;
		ex->ent.model = cl_mod_explo4;
		if (frand() < 0.5)
			ex->baseframe = 15;
		ex->frames = 15;
		if (type == TE_ROCKET_EXPLOSION_WATER)
			S_StartSound (pos, 0, 0, cl_sfx_watrexp, 1, ATTN_NORM, 0);
		else
			S_StartSound (pos, 0, 0, cl_sfx_rockexp, 1, ATTN_NORM, 0);
		break;

	case TE_FLASHLIGHT:
		MSG_ReadPos(&net_message, pos);
		ent = MSG_ReadShort(&net_message);
		CL_Flashlight(ent, pos);
		break;

	case TE_FORCEWALL:
		MSG_ReadPos(&net_message, pos);
		MSG_ReadPos(&net_message, pos2);
		color = MSG_ReadByte (&net_message);
		CL_ForceWall(pos, pos2, color);
		break;

	case TE_HEATBEAM:
		ent = CL_ParsePlayerBeam (cl_mod_heatbeam);
		break;

	case TE_MONSTER_HEATBEAM:
		ent = CL_ParsePlayerBeam (cl_mod_monster_heatbeam);
		break;

	case TE_HEATBEAM_SPARKS:
		cnt = 50;
		MSG_ReadPos (&net_message, pos);
		MSG_ReadDir (&net_message, dir);
		CL_ParticleSteamEffect (pos, dir, 8, cnt, 60);
		S_StartSound (pos,  0, 0, cl_sfx_lashit, 1, ATTN_NORM, 0);
		break;

	case TE_HEATBEAM_STEAM:
		cnt = 20;
		MSG_ReadPos (&net_message, pos);
		MSG_ReadDir (&net_message, dir);
		CL_ParticleSteamEffect (pos, dir, 0xE0, cnt, 60);
		S_StartSound (pos,  0, 0, cl_sfx_lashit, 1, ATTN_NORM, 0);
		break;

	case TE_STEAM:
		CL_ParseSteam();
		break;

	case TE_BUBBLETRAIL2:
		cnt = 8;
		MSG_ReadPos (&net_message, pos);
		MSG_ReadPos (&net_message, pos2);
		CL_BubbleTrail2 (pos, pos2, cnt);
		S_StartSound (pos,  0, 0, cl_sfx_lashit, 1, ATTN_NORM, 0);
		break;

	case TE_MOREBLOOD:
		MSG_ReadPos (&net_message, pos);
		MSG_ReadDir (&net_message, dir);
		CL_ParticleEffect (pos, dir, 0xe8, 250);
		break;

	case TE_CHAINFIST_SMOKE:
		dir[0]=0; dir[1]=0; dir[2]=1;
		MSG_ReadPos(&net_message, pos);
		CL_ParticleSmokeEffect (pos, dir, 0, 20, 20);
		break;

	case TE_ELECTRIC_SPARKS:
		MSG_ReadPos (&net_message, pos);
		MSG_ReadDir (&net_message, dir);
//		CL_ParticleEffect (pos, dir, 109, 40);
		CL_ParticleEffect (pos, dir, 0x75, 40);
		//FIXME : replace or remove this sound
		S_StartSound (pos, 0, 0, cl_sfx_lashit, 1, ATTN_NORM, 0);
		break;

	case TE_TRACKER_EXPLOSION:
		MSG_ReadPos (&net_message, pos);
		CL_ColorFlash (pos, 0, 150, -1, -1, -1);
		CL_ColorExplosionParticles (pos, 0, 1);
//		CL_Tracker_Explode (pos);
		S_StartSound (pos, 0, 0, cl_sfx_disrexp, 1, ATTN_NORM, 0);
		break;

	case TE_TELEPORT_EFFECT:
	case TE_DBALL_GOAL:
		MSG_ReadPos (&net_message, pos);
		CL_TeleportParticles (pos);
		break;

	case TE_WIDOWBEAMOUT:
		CL_ParseWidow ();
		break;

	case TE_NUKEBLAST:
		CL_ParseNuke ();
		break;

	case TE_WIDOWSPLASH:
		MSG_ReadPos (&net_message, pos);
		CL_WidowSplash (pos);
		break;

	default:
		Com_DropError ("CL_ParseTEnt: bad type");
	}
}

/*
=================
CL_AddBeams
=================
*/
void CL_AddBeams (void)
{
	int			i,j;
	mBeam_t		*b;
	vec3_t		dist, org;
	float		d;
	entity_t	ent;
	float		yaw, pitch;
	float		forward;
	float		len, steps;
	float		model_length;

	// update beams
	for (i = 0, b = cl_mBeams; i < MAX_BEAMS ; i++, b++)
	{
		if (!b->model || b->endtime < cl.time)
			continue;

		// if coming from the player, update the start position
		if (b->entity == cl.playernum+1)	// entity 0 is the world
			VectorCopy (cl.modelorg, b->start);
		VectorAdd (b->start, b->offset, org);

		// calculate pitch and yaw
		VectorSubtract (b->end, org, dist);

		if (dist[1] == 0 && dist[0] == 0)
		{
			yaw = 0;
			if (dist[2] > 0)
				pitch = 90;
			else
				pitch = 270;
		}
		else
		{
			// PMM - fixed to correct for pitch of 0
			if (dist[0])
				yaw = (atan2(dist[1], dist[0]) * 180 / M_PI);
			else if (dist[1] > 0)
				yaw = 90;
			else
				yaw = 270;
			if (yaw < 0) yaw += 360;

			forward = sqrt (dist[0]*dist[0] + dist[1]*dist[1]);	//!! slow
			pitch = (atan2(dist[2], forward) * -180.0 / M_PI);	//
			if (pitch < 0) pitch += 360.0;
		}

		// add new entities for the beams
		d = VectorNormalize(dist);

		memset (&ent, 0, sizeof(ent));
		if (b->model == cl_mod_lightning)
		{
			model_length = 35.0;
			d-= 20.0;  // correction so it doesn't end in middle of tesla
		}
		else
		{
			model_length = 30.0;
		}
		steps = appCeil (d/model_length);
		len = (d-model_length)/(steps-1);

		// PMM - special case for lightning model .. if the real length is shorter than the model,
		// flip it around & draw it from the end to the start.  This prevents the model from going
		// through the tesla mine (instead it goes through the target)
		if ((b->model == cl_mod_lightning) && (d <= model_length))
		{
//			Com_Printf ("special case\n");
			VectorCopy (b->end, ent.origin);
			// offset to push beam outside of tesla model (negative because dist is from end to start
			// for this beam)
//			for (j = 0; j < 3 ; j++)
//				ent.origin[j] -= dist[j]*10.0;
			ent.model = b->model;
			ent.flags = RF_FULLBRIGHT;
			ent.angles[0] = pitch;
			ent.angles[1] = yaw;
			ent.angles[2] = rand()%360;
			V_AddEntity (&ent);
			return;
		}

		while (d > 0)
		{
			VectorCopy (org, ent.origin);
			ent.model = b->model;
			if (b->model == cl_mod_lightning)
			{
				ent.flags = RF_FULLBRIGHT;
				ent.angles[0] = -pitch;
				ent.angles[1] = yaw + 180.0;
				ent.angles[2] = rand()%360;
			}
			else
			{
				ent.angles[0] = pitch;
				ent.angles[1] = yaw;
				ent.angles[2] = rand()%360;
			}

//			Com_Printf("B: %d -> %d\n", b->entity, b->dest_entity);
			V_AddEntity (&ent);

			for (j = 0; j < 3; j++)
				org[j] += dist[j]*len;
			d -= model_length;
		}
	}
}


extern cvar_t *hand;

/*
=================
ROGUE - draw player locked beams
CL_AddPlayerBeams
=================
*/
void CL_AddPlayerBeams (void)
{
	int		i, j;
	mBeam_t	*b;
	vec3_t		dist, org;
	float		d;
	entity_t	ent;
	float		yaw, pitch;
	float		forward;
	float		len, steps;
	float		model_length;
	float		hand_multiplier;

//PMM
	if (hand->integer == 2)
		hand_multiplier = 0;
	else if (hand->integer == 1)
		hand_multiplier = -1;
	else
		hand_multiplier = 1;
//PMM

	// update beams
	for (i = 0, b = cl_mPlayerbeams; i < MAX_BEAMS; i++, b++)
	{
		bool	isHeatbeam, isPlayer;
		player_state_t *ps, *ops;
		int		framenum;

		if (!b->model || b->endtime < cl.time)
			continue;

		isHeatbeam = cl_mod_heatbeam && (b->model == cl_mod_heatbeam);
		isPlayer = b->entity == cl.playernum + 1;

		if (isHeatbeam)
		{

			// if coming from the player, update the start position
			if (isPlayer)
			{
				// set up gun position
				// code straight out of CL_AddViewWeapon
				ps = &cl.frame.playerstate;
				ops = &cl.oldFrame->playerstate;
				for (j = 0; j < 3; j++)
				{
					b->start[j] = cl.modelorg[j] + ops->gunoffset[j]
						+ cl.lerpfrac * (ps->gunoffset[j] - ops->gunoffset[j]);
				}
				VectorMA (b->start, (hand_multiplier * b->offset[0]), cl.v_right, org);
				VectorMA (org, b->offset[1], cl.v_forward);
				VectorMA (org, b->offset[2], cl.v_up);
				if (hand->integer == 2)
					VectorMA (org, -1, cl.v_up);
			}
			else
				VectorCopy (b->start, org);
		}
		else
		{
			// if coming from the player, update the start position
			if (isPlayer)
				VectorCopy (cl.modelorg, b->start);
			VectorAdd (b->start, b->offset, org);
		}

		// calculate pitch and yaw
		VectorSubtract (b->end, org, dist);

//PMM
		if (isHeatbeam && isPlayer)
		{
			float len = VectorLength (dist);
			VectorScale (cl.v_forward, len, dist);
			VectorMA (dist, (hand_multiplier * b->offset[0]), cl.v_right);
			VectorMA (dist, b->offset[1], cl.v_forward);
			VectorMA (dist, b->offset[2], cl.v_up);
			if (hand->integer == 2)
				VectorMA (org, -1, cl.v_up, org);
		}
//PMM

		if (dist[1] == 0 && dist[0] == 0)
		{
			yaw = 0;
			if (dist[2] > 0)
				pitch = 90;
			else
				pitch = 270;
		}
		else
		{
			// PMM - fixed to correct for pitch of 0
			if (dist[0])
				yaw = (atan2(dist[1], dist[0]) * 180 / M_PI);
			else if (dist[1] > 0)
				yaw = 90;
			else
				yaw = 270;
			if (yaw < 0) yaw += 360.0f;

			forward = sqrt (dist[0]*dist[0] + dist[1]*dist[1]);	//!! slow
			pitch = (atan2(dist[2], forward) * -180.0 / M_PI);	//
			if (pitch < 0) pitch += 360.0f;
		}

		if (isHeatbeam)
		{
			if (!isPlayer)
			{
				vec3_t	u, f, r;

				// third person
				framenum = 2;
				ent.angles[0] = -pitch;
				ent.angles[1] = yaw + 180.0f;
				ent.angles[2] = 0;
//				Com_Printf ("%f %f - %f %f %f\n", -pitch, yaw+180.0, b->offset[0], b->offset[1], b->offset[2]);
				AngleVectors(ent.angles, f, r, u);

				// if it's a non-origin offset, it's a player, so use the hardcoded player offset
				if (b->offset[0] || b->offset[1] || b->offset[2])
				{
					VectorMA (org, -(b->offset[0])+1, r);
					VectorMA (org, -(b->offset[1]), f);
					VectorMA (org, -(b->offset[2])-10, u);
				}
				else
				{
					// if it's a monster, do the particle effect
					CL_MonsterPlasma_Shell(b->start);
				}
			}
			else
			{
				framenum = 1;
			}
		}

		// if it's the heatbeam, draw the particle effect
		if (isHeatbeam && isPlayer)
		{
			CL_Heatbeam (org, dist);
		}

		// add new entities for the beams
		d = VectorNormalize(dist);

		memset (&ent, 0, sizeof(ent));
		if (b->model == cl_mod_heatbeam)
		{
			model_length = 32.0;
		}
		else if (b->model == cl_mod_lightning)
		{
			model_length = 35.0;
			d-= 20.0;  // correction so it doesn't end in middle of tesla
		}
		else
		{
			model_length = 30.0;
		}
		steps = appCeil(d/model_length);
		len = (d-model_length) / (steps-1);

		// PMM - special case for lightning model .. if the real length is shorter than the model,
		// flip it around & draw it from the end to the start.  This prevents the model from going
		// through the tesla mine (instead it goes through the target)
		if ((b->model == cl_mod_lightning) && (d <= model_length))
		{
//			Com_Printf ("special case\n");
			VectorCopy (b->end, ent.origin);
			// offset to push beam outside of tesla model (negative because dist is from end to start
			// for this beam)
//			for (j=0 ; j<3 ; j++)
//				ent.origin[j] -= dist[j]*10.0;
			ent.model = b->model;
			ent.flags = RF_FULLBRIGHT;
			ent.angles[0] = pitch;
			ent.angles[1] = yaw;
			ent.angles[2] = rand()%360;
			V_AddEntity (&ent);
			return;
		}
		while (d > 0)
		{
			VectorCopy (org, ent.origin);
			ent.model = b->model;
			if (isHeatbeam)
			{
//				ent.flags = RF_FULLBRIGHT|RF_TRANSLUCENT;
//				ent.alpha = 0.3;
				ent.flags = RF_FULLBRIGHT;
				ent.angles[0] = -pitch;
				ent.angles[1] = yaw + 180.0;
				ent.angles[2] = (cl.time) % 360;
//				ent.angles[2] = rand()%360;
				ent.frame = framenum;
			}
			else if (b->model == cl_mod_lightning)
			{
				ent.flags = RF_FULLBRIGHT;
				ent.angles[0] = -pitch;
				ent.angles[1] = yaw + 180.0;
				ent.angles[2] = rand()%360;
			}
			else
			{
				ent.angles[0] = pitch;
				ent.angles[1] = yaw;
				ent.angles[2] = rand()%360;
			}

//			Com_Printf("B: %d -> %d\n", b->entity, b->dest_entity);
			V_AddEntity (&ent);

			for (j = 0; j < 3; j++)
				org[j] += dist[j]*len;
			d -= model_length;
		}
	}
}


/* PMM - CL_Sustains */
void CL_ProcessSustain ()
{
	cl_sustain_t	*s;
	int				i;

	for (i = 0, s = cl_sustains; i < MAX_SUSTAINS; i++, s++)
	{
		if (s->id)
			if ((s->endtime >= cl.time) && (cl.time >= s->nextthink))
			{
//				Com_Printf ("think %d %d %d\n", cl.time, s->nextthink, s->thinkinterval);
				s->think (s);
			}
			else if (s->endtime < cl.time)
				s->id = 0;
	}
}

/*
=================
CL_AddTEnts
=================
*/
void CL_AddTEnts (void)
{
	CL_AddBeams ();
	// PMM - draw plasma beams
	CL_AddPlayerBeams ();
	CL_AddExplosions ();
	// PMM - set up sustain
	CL_ProcessSustain();
}
