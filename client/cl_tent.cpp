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
#include "snd_loc.h"


//ROGUE
#define MAX_SUSTAINS		32
struct cl_sustain_t
{
	bool	used;
	int		endtime;
	int		nextthink;
	CVec3	org;
	CVec3	dir;
	int		color;
	int		count;
	int		magnitude;
	void	(*think)(cl_sustain_t *self);
};
static cl_sustain_t cl_sustains[MAX_SUSTAINS];


#define	MAX_BEAMS	32
struct mBeam_t		// beam with model
{
	int		entity;
	int		dest_entity;
	CRenderModel *model;
	int		endtime;
	CVec3	offset;
	CVec3	start, end;
};
static mBeam_t	cl_mBeams[MAX_BEAMS];
//PMM - added this for player-linked beams. Currently only used by the plasma beam
static mBeam_t	cl_mPlayerbeams[MAX_BEAMS];

//?? externs
void CL_TeleportParticles (const CVec3 &org);
void CL_ExplosionParticles (const CVec3 &org);
void CL_BFGExplosionParticles (const CVec3 &org);


static sfx_t *cl_sfx_ric[3];
static sfx_t *cl_sfx_lashit;
static sfx_t *cl_sfx_spark5;
static sfx_t *cl_sfx_spark6;
static sfx_t *cl_sfx_spark7;
static sfx_t *cl_sfx_railg;
static sfx_t *cl_sfx_rockexp;
static sfx_t *cl_sfx_grenexp;
static sfx_t *cl_sfx_watrexp;
// XATRIX
static sfx_t *cl_sfx_plasexp;
// ROGUE
static sfx_t *cl_sfx_lightning;
static sfx_t *cl_sfx_disrexp;

static sfx_t *cl_sfx_fallshort;
sfx_t		 *cl_sfx_footsteps[4];


static CRenderModel *cl_mod_explode;
static CRenderModel *cl_mod_smoke;
static CRenderModel *cl_mod_flash;
static CRenderModel *cl_mod_parasite_segment;
static CRenderModel *cl_mod_grapple_cable;
static CRenderModel *cl_mod_parasite_tip;
static CRenderModel *cl_mod_explo4;
static CRenderModel *cl_mod_bfg_explo;
CRenderModel		*cl_mod_powerscreen;
// XATRIX
static CRenderModel *cl_mod_plasmaexplo;
// ROGUE
static CRenderModel *cl_mod_lightning;
static CRenderModel *cl_mod_heatbeam;
static CRenderModel *cl_mod_monster_heatbeam;
static CRenderModel *cl_mod_explo4_big;


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


void CL_RegisterTEntSounds (void)
{
	int		i, j;

	for (i = 0; i < 3; i++)
		cl_sfx_ric[i] = S_RegisterSound (va("world/ric%d.wav", i + 1));

	cl_sfx_lashit  = S_RegisterSound ("weapons/lashit.wav");
	cl_sfx_spark5  = S_RegisterSound ("world/spark5.wav");
	cl_sfx_spark6  = S_RegisterSound ("world/spark6.wav");
	cl_sfx_spark7  = S_RegisterSound ("world/spark7.wav");
	cl_sfx_railg   = S_RegisterSound ("weapons/railgf1a.wav");
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
		static const char *camperSounds[] = {
			"tinyfart", "cough1", "cough2", "sneeze1", "sneeze2",
			"yawn1", "yawn2", "burp1", "spit1"
		};
		for (i = 0; i < ARRAY_COUNT(camperSounds); i++)
			cl_sfx_camper[i] = S_RegisterSound (va("player/reallife/%s.wav", camperSounds[i]));
	}

	//PGM
	cl_sfx_lightning = S_RegisterSound ("weapons/tesla.wav");
	cl_sfx_disrexp = S_RegisterSound ("weapons/disrupthit.wav");
}


static CRenderModel *RegModel (const char *name)
{
	return RE_RegisterModel (va("models/%s/tris.md2", name));
}

static CRenderModel *RegFxModel (const char *oldName, const char *newName)
{
	CRenderModel *model = NULL;
	if (NEW_FX)
	{
		model = RE_RegisterModel (va("fx/%s", newName));
		if (!model) Com_DPrintf ("no fx model %s\n", newName);
	}
	if (!model)
		model = RegModel (oldName);
	return model;
}


#define EXPLO4_WALL_DIST	16

void CL_RegisterTEntModels ()
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

	cl_mod_explode     = RegModel ("objects/explode");
	cl_mod_smoke       = RegFxModel ("objects/smoke", "particles/smoke.spr");
	cl_mod_flash       = RegModel ("objects/flash");
	cl_mod_parasite_segment = RegModel ("monsters/parasite/segment");
	cl_mod_grapple_cable = RegModel ("ctf/segment");
	cl_mod_parasite_tip = RegModel ("monsters/parasite/tip");
	cl_mod_explo4      = RegFxModel ("objects/r_explode", "particles/explode.spr");
	cl_mod_bfg_explo   = RE_RegisterModel ("sprites/s_bfg2.sp2");
	cl_mod_powerscreen = RegModel ("items/armor/effect");

	for (int i = 0; i < ARRAY_COUNT(modelNames); i++)
		RegModel (modelNames[i]);

	//ROGUE
	cl_mod_explo4_big  = RegModel ("objects/r_explode2");
	cl_mod_lightning   = RegModel ("proj/lightning");
	cl_mod_heatbeam    = RegModel ("proj/beam");
	cl_mod_monster_heatbeam = RegModel ("proj/widowbeam");
}


typedef enum
{
	ex_free, ex_explosion, ex_misc, ex_flash, ex_mflash, ex_poly
} exptype_t;

struct explosion_t
{
	exptype_t	type;
	entity_t	ent;

	int			frames;			// lifetime in number of frames (0.1s units)
	float		light;
	CVec3		lightcolor;
	float		time;
	int			baseframe;
};


#define	MAX_EXPLOSIONS	32
explosion_t	cl_explosions[MAX_EXPLOSIONS];

void CL_ClearTEnts ()
{
	memset (cl_mBeams, 0, sizeof(cl_mBeams));
	memset (cl_explosions, 0, sizeof(cl_explosions));
	//ROGUE
	memset (cl_mPlayerbeams, 0, sizeof(cl_mPlayerbeams));
	memset (cl_sustains, 0, sizeof(cl_sustains));
}


/*-----------------------------------------------------------------------------
	Explosions
-----------------------------------------------------------------------------*/

static explosion_t *AllocExplosion (const CVec3 &origin, exptype_t type, CRenderModel *model, float wallOffset = 0)
{
	int		i;
	explosion_t *ex;

	// move explosion from wall
	CVec3 origin2 = origin;
	if (wallOffset)
	{
		CBox bounds;
		for (i = 0; i < 3; i++)
		{
			bounds.mins[i] = -wallOffset/2;
			bounds.maxs[i] =  wallOffset/2;
		}
		trace_t trace;
		CL_Trace (trace, origin, origin, bounds, CONTENTS_SOLID);
		if (trace.allsolid)
			VectorMA (origin, wallOffset, trace.plane.normal, origin2);
	}

	explosion_t *dst = NULL;
	// find another explosion with the same origin
	for (i = 0, ex = cl_explosions; i < MAX_EXPLOSIONS; i++, ex++)
	{
		if (ex->type == ex_free) continue;
		if (type == ex->type && ex->ent.pos.origin == origin2)
		{
			//!! accumulate color?!
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
		// find the oldest explosion (add priority for smoke/explode ??)
		float time = 0;
		dst = cl_explosions;

		for (i = 1, ex = &cl_explosions[1]; i < MAX_EXPLOSIONS; i++, ex++)
			if (ex->time > time)
			{
				time = ex->time;
				dst = ex;
			}
	}

	memset (dst, 0, sizeof(explosion_t));
	dst->ent.pos.origin = origin2;
	dst->time = -100;
	dst->type = type;
	dst->ent.time = cl.ftime;
	dst->ent.model = model;
	return dst;
}


static void AddExplosions ()
{
	static double oldTime;
	float timeDelta = (cl.ftime - oldTime) * 1000.0f;	// msec
	oldTime = cl.ftime;

	float incTime = 0;
	if (r_sfx_pause->integer)
	{
		incTime = timeDelta / 1000.0f;					// sec
		timeDelta = 0;
	}

	int i;
	explosion_t	*ex;
	for (i = 0, ex = cl_explosions; i < MAX_EXPLOSIONS; i++, ex++)
	{
		if (ex->type == ex_free)
			continue;

		if (ex->time < 0)
			ex->time = 0;	// just started
		else
			ex->time += timeDelta;

		float frac = ex->time / 100.0f;
		if (frac < 0) frac = 0;
		int frm = appFloor (frac);

		if (frm >= ex->frames-1)
		{
			ex->type = ex_free;
			continue;
		}

		entity_t *ent = &ex->ent;

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

		if (ex->light)
		{
			V_AddLight (ent->pos.origin, ex->light * ent->alpha,
				ex->lightcolor[0] * ent->alpha, ex->lightcolor[1] * ent->alpha, ex->lightcolor[2] * ent->alpha);
		}

		if (!ent->model) continue;		// flash only

		ent->frame    = ex->baseframe + frm + 1;
		ent->oldframe = ex->baseframe + frm;
		ent->backlerp = 1.0f - (frac - frm);
		ent->time += incTime;			// modified when r_sfx_pause only
		if (ent->time > cl.ftime) ent->time = cl.ftime;

		V_AddEntity (ent);
	}
}


/*-----------------------------------------------------------------------------
	Sustains
-----------------------------------------------------------------------------*/

static cl_sustain_t *AllocSustain ()
{
	int i;
	cl_sustain_t *s;
	for (i = 0, s = cl_sustains; i < MAX_SUSTAINS; i++, s++)
		if (!s->used)
		{
			s->used = true;
			return s;
		}
	return NULL;
}


static void ProcessSustains ()
{
	int i;
	cl_sustain_t *s;
	for (i = 0, s = cl_sustains; i < MAX_SUSTAINS; i++, s++)
	{
		if (!s->used) continue;

		if ((s->endtime >= cl.time) && (cl.time >= s->nextthink))
			s->think (s);
		else if (s->endtime < cl.time)
			s->used = false;		// free sustain
	}
}


/*
=================
CL_SmokeAndFlash
=================
*/
void CL_SmokeAndFlash (const CVec3 &origin)
{
	explosion_t	*ex;

	ex = AllocExplosion (origin, ex_misc, cl_mod_smoke, 4);
	ex->frames = 4;
	ex->ent.flags = RF_TRANSLUCENT;

	ex = AllocExplosion (origin, ex_flash, cl_mod_flash);	//??
	ex->frames = 2;
	ex->ent.flags = RF_FULLBRIGHT;
}

/*
=================
CL_ParseParticles
=================
*/
void CL_ParseParticles (void)
{
	CVec3	pos, dir;
	MSG_ReadPos (&net_message, pos);
	MSG_ReadDir (&net_message, dir);

	int color = MSG_ReadByte (&net_message);
	int count = MSG_ReadByte (&net_message);

	CL_ParticleEffect (pos, dir, color, count);
}

/*
=================
CL_ParseBeam
=================
*/
static void CL_ParseBeam (CRenderModel *model)
{
	int		i;
	mBeam_t	*b;

	int ent = MSG_ReadShort (&net_message);

	CVec3	start, end;
	MSG_ReadPos (&net_message, start);
	MSG_ReadPos (&net_message, end);

	// override any beam with the same entity
	for (i = 0, b = cl_mBeams; i < MAX_BEAMS ; i++, b++)
		if (b->entity == ent)
		{
			b->entity = ent;
			b->model = model;
			b->endtime = cl.time + 200;
			b->start = start;
			b->end = end;
			b->offset.Zero();
			return;
		}

	// find a free beam
	for (i = 0, b = cl_mBeams; i < MAX_BEAMS ; i++, b++)
		if (!b->model || b->endtime < cl.time)
		{
			b->entity = ent;
			b->model = model;
			b->endtime = cl.time + 200;
			b->start = start;
			b->end = end;
			b->offset.Zero();
			return;
		}

	Com_Printf ("beam list overflow!\n");
	return;
}

/*
=================
CL_ParseBeam2
=================
*/
static void CL_ParseBeam2 (CRenderModel *model)
{
	int ent = MSG_ReadShort (&net_message);

	CVec3	start, end, offset;
	MSG_ReadPos (&net_message, start);
	MSG_ReadPos (&net_message, end);
	MSG_ReadPos (&net_message, offset);

	// override any beam with the same entity
	int		i;
	mBeam_t	*b;
	for (i = 0, b = cl_mBeams; i < MAX_BEAMS; i++, b++)
		if (b->entity == ent)
		{
			b->entity = ent;
			b->model = model;
			b->endtime = cl.time + 200;
			b->start = start;
			b->end = end;
			b->offset = offset;
			return;
		}

	// find a free beam
	for (i = 0, b = cl_mBeams; i < MAX_BEAMS ; i++, b++)
		if (!b->model || b->endtime < cl.time)
		{
			b->entity = ent;
			b->model = model;
			b->endtime = cl.time + 200;
			b->start = start;
			b->end = end;
			b->offset = offset;
			return;
		}

	Com_Printf ("beam list overflow!\n");
	return;
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
	int ent = MSG_ReadShort (&net_message);

	CVec3	start, end, offset;
	MSG_ReadPos (&net_message, start);
	MSG_ReadPos (&net_message, end);
	// PMM - network optimization
	if (model == cl_mod_heatbeam)
		offset.Set (2, 7, -3);
	else if (model == cl_mod_monster_heatbeam)
	{
		model = cl_mod_heatbeam;
		offset.Zero ();
	}
	else
		MSG_ReadPos (&net_message, offset);

	// override any beam with the same entity
	// PMM - For player beams, we only want one per player (entity) so..
	int		i;
	mBeam_t	*b;
	for (i = 0, b = cl_mPlayerbeams; i < MAX_BEAMS ; i++, b++)
		if (b->entity == ent)
		{
			b->entity = ent;
			b->model = model;
			b->endtime = cl.time + 200;
			b->start = start;
			b->end = end;
			b->offset = offset;
			return ent;
		}

	// find a free beam
	for (i = 0, b = cl_mPlayerbeams; i < MAX_BEAMS ; i++, b++)
		if (!b->model || b->endtime < cl.time)
		{
			b->entity = ent;
			b->model = model;
			b->endtime = cl.time + 100;		// PMM - this needs to be 100 to prevent multiple heatbeams
			b->start = start;
			b->end = end;
			b->offset = offset;
			return ent;
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
	int srcEnt = MSG_ReadShort (&net_message);
	int destEnt = MSG_ReadShort (&net_message);

	CVec3	start, end;
	MSG_ReadPos (&net_message, start);
	MSG_ReadPos (&net_message, end);

	// override any beam with the same source AND destination entities
	int		i;
	mBeam_t	*b;
	for (i = 0, b = cl_mBeams; i < MAX_BEAMS ; i++, b++)
		if (b->entity == srcEnt && b->dest_entity == destEnt)
		{
			b->entity = srcEnt;
			b->dest_entity = destEnt;
			b->model = model;
			b->endtime = cl.time + 200;
			b->start = start;
			b->end = end;
			b->offset.Zero();
			return srcEnt;
		}

	// find a free beam
	for (i = 0, b = cl_mBeams; i < MAX_BEAMS ; i++, b++)
		if (!b->model || b->endtime < cl.time)
		{
			b->entity = srcEnt;
			b->dest_entity = destEnt;
			b->model = model;
			b->endtime = cl.time + 200;
			b->start = start;
			b->end = end;
			b->offset.Zero();
			return srcEnt;
		}

	Com_Printf ("beam list overflow!\n");
	return srcEnt;
}


static void CL_ParseBFGLaser (unsigned colors)
{
	CVec3 start, end;
	MSG_ReadPos (&net_message, start);
	MSG_ReadPos (&net_message, end);

	beam_t *b = CL_AllocParticleBeam (start, end, 2.0f, 0.1f);
	if (!b) return;

	b->type = BEAM_STANDARD;
	b->color.rgba = 0;
	b->color.c[0] = (colors >> ((rand() % 4)*8)) & 0xFF;
	b->alpha = b->dstAlpha = 0.3f;
}

//=============
//ROGUE

static void ParticleSteamEffect (cl_sustain_t *self)
{
	CL_ParticleSteamEffect (self->org, self->dir, self->color, self->count, self->magnitude);
	self->nextthink += 100;		// 10 times/sec
}


static void CL_ParseSteam (void)
{
	CVec3	pos, dir;
	int id = MSG_ReadShort (&net_message);		// an id of -1 is an instant effect
	int count = MSG_ReadByte (&net_message);
	MSG_ReadPos (&net_message, pos);
	MSG_ReadDir (&net_message, dir);
	int color = MSG_ReadByte (&net_message);
	int magnitude = MSG_ReadShort (&net_message);

	if (id != -1) // sustains
	{
		cl_sustain_t *s = AllocSustain ();

		int interval = MSG_ReadLong (&net_message);
		if (s)
		{
			s->count = count;
			s->org = pos;
			s->dir = dir;
			s->color = color;
			s->magnitude = magnitude;
			s->endtime = cl.time + interval;
			s->think = ParticleSteamEffect;
			s->nextthink = cl.time;
		}
	}
	else
		CL_ParticleSteamEffect (pos, dir, color, count, magnitude);	// instant
}


static void Widowbeamout (cl_sustain_t *self)
{
	static const int colortable[4] = {2*8,13*8,21*8,18*8};
	float ratio = 1.0f - (self->endtime - cl.time) / 2100.0f;

	for (int i = 0; i < 300; i++)
	{
		particle_t *p;
		if (!(p = CL_AllocParticle ())) return;
		p->accel[2] = 0;

		p->alphavel = INSTANT_PARTICLE;
		p->color = colortable[rand()&3];

		CVec3 dir;
		dir.Set (crand(), crand(), crand());
		dir.NormalizeFast ();

		VectorMA(self->org, (45.0 * ratio), dir, p->org);
	}
	// nextthink is not updated - call every frame
}

void CL_ParseWidow (void)
{
	/* int id = */ MSG_ReadShort (&net_message); // unused value
	CVec3 pos;
	MSG_ReadPos (&net_message, pos);
	cl_sustain_t *s = AllocSustain ();
	if (s)
	{
		s->org = pos;
		s->endtime = cl.time + 2100;
		s->think = Widowbeamout;
		s->nextthink = cl.time;
	}
}

static void Nukeblast (cl_sustain_t *self)
{
	static const int colortable[4] = {110, 112, 114, 116};
	float ratio = 1.0f - (self->endtime - cl.time) / 1000.0f;

	for (int i = 0; i < 700; i++)
	{
		particle_t *p;
		if (!(p = CL_AllocParticle ())) return;
		p->accel[2] = 0;

		p->alphavel = INSTANT_PARTICLE;
		p->color = colortable[rand()&3];

		CVec3 dir;
		dir.Set (crand(), crand(), crand());
		dir.NormalizeFast ();

		VectorMA(self->org, (200.0 * ratio), dir, p->org);
	}
	// nextthink is not updated - call every frame
}

void CL_ParseNuke (void)
{
	cl_sustain_t *s = AllocSustain ();
	CVec3 pos;
	MSG_ReadPos (&net_message, pos);

	if (s)
	{
		s->org = pos;
		s->endtime = cl.time + 1000;
		s->think = Nukeblast;
		s->nextthink = cl.time;
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
	CVec3	pos, pos2, dir;
	explosion_t	*ex;
	int		cnt, color, ent;

	int type = MSG_ReadByte (&net_message);
	sfx_t *sound = NULL;

	switch (type)
	{
	case TE_GUNSHOT:			// bullet hitting wall
	case TE_SPARKS:
	case TE_BULLET_SPARKS:
		MSG_ReadPos (&net_message, pos);
		MSG_ReadDir (&net_message, dir);

		if (NEW_FX)
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
			CL_SmokeAndFlash (pos);
			if (cls.newprotocol)
			{
				//!! Add stuff for bulletholes here
				trace_t	trace;
				CVec3 start, end;

				VectorAdd (pos, dir, start);
				VectorMA (start, -2, dir, end);
				CL_Trace (trace, start, end, nullBox, MASK_ALL);
				if (trace.fraction < 1.0)
				{
					csurface_t *surf = trace.surface;
					int impactType = materialImpacts[surf->material];
					if (impactType < 0) break;		// silent

					sound = cl_sfx_bullethits[impactType*3 + rand()%3];
				}
			}
			else
			{
				// impact sound
				cnt = rand() % 10;
				if (cnt <= 2) sound = cl_sfx_ric[cnt];
			}
		}
		break;

	case TE_SCREEN_SPARKS:
	case TE_SHIELD_SPARKS:
	case TE_ELECTRIC_SPARKS:
		MSG_ReadPos (&net_message, pos);
		MSG_ReadDir (&net_message, dir);
		if (type == TE_SCREEN_SPARKS)
			color = 0xD0;
		else if (type == TE_SHIELD_SPARKS)
			color = 0xB0;
		else // TE_ELECTRIC_SPARKS
			color = 0x75;
		CL_ParticleEffect (pos, dir, color, 40);
		//FIXME : replace or remove this sound
		sound = cl_sfx_lashit;
		break;

	case TE_SHOTGUN:			// bullet hitting wall
		MSG_ReadPos (&net_message, pos);
		MSG_ReadDir (&net_message, dir);
		CL_ParticleEffect (pos, dir, 0, 20);
		CL_SmokeAndFlash (pos);
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
				static const byte splash_color[] = {
					0x00, 0xE0, 0xB0, 0x50, 0xD0, 0xE0, 	// unk, sparks, blue water, brown water, slime, lava
					0xE8, 0xB0, 0x50						// blood, bullet blue water, bullet brown water
				};
				color = splash_color[r];
			}
			CL_ParticleEffect (pos, dir, color, cnt);

			if (r == SPLASH_SPARKS)
			{
				// just particles -- not water
				static sfx_t **sfxTbl[] = {&cl_sfx_spark5, &cl_sfx_spark6, &cl_sfx_spark7, &cl_sfx_spark7};
				S_StartSound (&pos, 0, 0, *sfxTbl[rand() & 3], 1, ATTN_STATIC, 0);	//?? ATTN_STATIC ?
			}
			else if ((r == SPLASH_BULLET_BLUE_WATER || r == SPLASH_BULLET_BROWN_WATER) && cls.newprotocol)
			{
				// bullet hitting water
				sound = cl_sfx_bullethits[IMPACT_WATER*3 + rand()%3];
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

	case TE_BLASTER:			// blaster hitting wall
	case TE_BLASTER2:			// green blaster hitting wall
	case TE_FLECHETTE:			// flechette
		MSG_ReadPos (&net_message, pos);
		MSG_ReadDir (&net_message, dir);
		ex = AllocExplosion (pos, ex_misc, cl_mod_explode);
		if (type == TE_BLASTER)
		{
			color = 0xE0;
			ex->lightcolor.Set (1, 1, 0);
			ex->ent.skinnum = 0;
		}
		else if (type == TE_BLASTER2)
		{
			color = 0xD0;
			ex->lightcolor.Set (0, 1, 0);
			ex->ent.skinnum = 1;
		}
		else // TE_FLECHETTE
		{
			color = 0x6F;
			ex->lightcolor.Set (0.19, 0.41, 0.75);
			ex->ent.skinnum = 2;
		}
		CL_BlasterParticles (pos, dir, color);

		//!! can precompute Dir2Angles() for all anorms
		ex->ent.angles[PITCH] = acos (dir[2]) / M_PI * 180;
		ex->ent.angles[YAW]   = Vec2Yaw (dir);
		ex->ent.flags = RF_FULLBRIGHT|RF_TRANSLUCENT;
		ex->light = 150;
		ex->frames = 4;
		sound = cl_sfx_lashit;
		break;

	case TE_RAILTRAIL:			// railgun effect
		MSG_ReadPos (&net_message, pos);
		MSG_ReadPos (&net_message, pos2);
		if (cls.newprotocol)
			CL_RailTrail (pos, pos2);
		else
			CL_RailTrailExt (pos, pos2, 1, 0);
		sound = cl_sfx_railg;
		break;

	case TE_RAILTRAIL_EXT:		// advanced railgun effect
		{
			MSG_ReadPos (&net_message, pos);
			MSG_ReadPos (&net_message, pos2);
			byte rColor = MSG_ReadByte (&net_message);
			byte rType = MSG_ReadByte (&net_message);
			CL_RailTrailExt (pos, pos2, rType, rColor);
			sound = cl_sfx_railg;
		}
		break;

	case TE_EXPLOSION1:
	case TE_EXPLOSION1_BIG:
	case TE_EXPLOSION1_NP:
	case TE_EXPLOSION2:
	case TE_GRENADE_EXPLOSION:
	case TE_GRENADE_EXPLOSION_WATER:
	case TE_ROCKET_EXPLOSION:
	case TE_ROCKET_EXPLOSION_WATER:
	case TE_PLAIN_EXPLOSION:
	case TE_PLASMA_EXPLOSION:
		MSG_ReadPos (&net_message, pos);
		ex = AllocExplosion (pos, ex_poly, (type != TE_EXPLOSION1_BIG) ? cl_mod_explo4 : cl_mod_explo4_big, EXPLO4_WALL_DIST);
		ex->ent.flags = RF_FULLBRIGHT;
		ex->light = 350;
		ex->lightcolor.Set (1.0, 0.5, 0.5);
		ex->ent.angles[YAW] = rand() % 360;
		if (type == TE_EXPLOSION2 || type == TE_GRENADE_EXPLOSION || type == TE_GRENADE_EXPLOSION_WATER)
		{
			ex->frames = 15;	//?? original: max frame was 19; for this explosion type should be 15 (can be negative ex->alpha)
			ex->baseframe = 30;
		}
		else
		{
			ex->frames = 15;
			if (rand() & 1) ex->baseframe = 15;
		}
		if (type != TE_EXPLOSION1_BIG && type != TE_EXPLOSION1_NP && type != TE_PLAIN_EXPLOSION)
			CL_ExplosionParticles (pos);
		switch (type)
		{
		case TE_EXPLOSION2:
		case TE_GRENADE_EXPLOSION:
			sound = cl_sfx_grenexp;
			break;
		case TE_GRENADE_EXPLOSION_WATER:
		case TE_ROCKET_EXPLOSION_WATER:
			sound = cl_sfx_watrexp;
			break;
		default:
			sound = cl_sfx_rockexp;
		}
		break;

	case TE_BFG_EXPLOSION:
		MSG_ReadPos (&net_message, pos);
		ex = AllocExplosion (pos, ex_poly, cl_mod_bfg_explo);
		ex->ent.flags = RF_FULLBRIGHT;
		ex->light = 350;
		ex->lightcolor.Set (0, 1, 0);
		ex->frames = 4;
		break;

	case TE_BFG_BIGEXPLOSION:
		MSG_ReadPos (&net_message, pos);
		CL_BFGExplosionParticles (pos);
		break;

	case TE_BFG_LASER:
		CL_ParseBFGLaser (0xD0D1D2D3);
		break;

	case TE_BUBBLETRAIL:
	case TE_DEBUGTRAIL:
		MSG_ReadPos (&net_message, pos);
		MSG_ReadPos (&net_message, pos2);
		if (type == TE_BUBBLETRAIL)
			CL_BubbleTrail (pos, pos2);
		else
			CL_DebugTrail (pos, pos2);
		break;

	case TE_BLUEHYPERBLASTER:	// unused ??
		MSG_ReadPos (&net_message, pos);
		MSG_ReadPos (&net_message, dir);		// dir sent as pos?
		CL_BlasterParticles (pos, dir, 0xE0);
		break;

	case TE_BLOOD:
	case TE_GREENBLOOD:
	case TE_MOREBLOOD:
		MSG_ReadPos (&net_message, pos);
		MSG_ReadDir (&net_message, dir);
		switch (type)
		{
		case TE_BLOOD:
			CL_ParticleEffect (pos, dir, 0xE8, 60);
			break;
		case TE_GREENBLOOD:
			CL_ParticleEffect2 (pos, dir, 0xDF, 30);
			break;
		case TE_MOREBLOOD:
			CL_ParticleEffect (pos, dir, 0xE8, 250);
			break;
		}
		break;

	case TE_PARASITE_ATTACK:
	case TE_MEDIC_CABLE_ATTACK:
		CL_ParseBeam (cl_mod_parasite_segment);
		break;

	case TE_GRAPPLE_CABLE:
		CL_ParseBeam2 (cl_mod_grapple_cable);
		break;

	case TE_BOSSTPORT:					// boss teleporting to station
		MSG_ReadPos (&net_message, pos);
		CL_BigTeleportParticles (pos);
		sound = S_RegisterSound ("misc/bigtele.wav");
		break;

	// XATRIX
	case TE_WELDING_SPARKS:
		cnt = MSG_ReadByte (&net_message);
		MSG_ReadPos (&net_message, pos);
		MSG_ReadDir (&net_message, dir);
		color = MSG_ReadByte (&net_message);
		CL_ParticleEffect2 (pos, dir, color, cnt);
		ex = AllocExplosion (pos, ex_flash, NULL);	// flash only; fading dlight
		ex->light = 100 + (rand() % 75);
		ex->lightcolor.Set (1.0f, 1.0f, 0.3f);
		ex->frames = 2;
		break;

	// XATRIX
	case TE_TUNNEL_SPARKS:
		cnt = MSG_ReadByte (&net_message);
		MSG_ReadPos (&net_message, pos);
		MSG_ReadDir (&net_message, dir);
		color = MSG_ReadByte (&net_message);
		CL_ParticleEffect3 (pos, dir, color, cnt);
		break;

	case TE_LIGHTNING:
		ent = CL_ParseLightning (cl_mod_lightning);
		S_StartSound (NULL, ent, CHAN_WEAPON, cl_sfx_lightning, 1, ATTN_NORM, 0);
		break;

	case TE_FLASHLIGHT:
		{
			MSG_ReadPos(&net_message, pos);
			ent = MSG_ReadShort(&net_message);
			cdlight_t *dl = CL_AllocDlight (ent, pos);
			dl->radius = 400;
			dl->die = cl.time + 100;
			dl->color.Set (1, 1, 1);
		}
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
	case TE_HEATBEAM_STEAM:
	case TE_BUBBLETRAIL2:
		MSG_ReadPos (&net_message, pos);
		MSG_ReadDir (&net_message, dir);
		switch (type)
		{
		case TE_HEATBEAM_SPARKS:
			CL_ParticleSteamEffect (pos, dir, 8, 50, 60);
			break;
		case TE_HEATBEAM_STEAM:
			CL_ParticleSteamEffect (pos, dir, 0xE0, 20, 60);
			break;
		case TE_BUBBLETRAIL2:
			CL_BubbleTrail2 (pos, pos2);
			break;
		}
		sound = cl_sfx_lashit;
		break;

		MSG_ReadPos (&net_message, pos);
		MSG_ReadDir (&net_message, dir);
		CL_ParticleSteamEffect (pos, dir, 0xE0, 20, 60);
		sound = cl_sfx_lashit;
		break;

	case TE_STEAM:
		CL_ParseSteam();
		break;

	case TE_CHAINFIST_SMOKE:
		dir.Set (0, 0, 1);
		MSG_ReadPos (&net_message, pos);
		CL_ParticleSmokeEffect (pos, dir);
		break;

	case TE_TRACKER_EXPLOSION:
		{
			MSG_ReadPos (&net_message, pos);
			cdlight_t *dl = CL_AllocDlight (0, pos);
			dl->radius = 150;
			dl->die = cl.time + 100;
			dl->color.Set (-1, -1, -1);
			CL_TrackerExplosionParticles (pos);
			sound = cl_sfx_disrexp;
		}
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

	if (sound)
		S_StartSound (&pos, 0, 0, sound, 1, ATTN_NORM, 0);
}

/*
=================
CL_AddBeams
=================
*/
void CL_AddBeams (void)
{
	int			i;
	mBeam_t		*b;
	for (i = 0, b = cl_mBeams; i < MAX_BEAMS ; i++, b++)
	{
		if (!b->model || b->endtime < cl.time)
			continue;

		// if coming from the player, update the start position
		if (b->entity == cl.playernum+1)	// entity 0 is the world
			b->start = cl.modelorg;
		CVec3 org;
		VectorAdd (b->start, b->offset, org);

		// calculate pitch and yaw
		CVec3 dist;
		VectorSubtract (b->end, org, dist);

		// add new entities for the beams
		float d = dist.NormalizeFast ();

		entity_t ent;
		memset (&ent, 0, sizeof(ent));
		Vec2Angles (dist, ent.angles);
		float model_length;
		if (b->model == cl_mod_lightning)
		{
			model_length = 35.0;
			d -= 20.0;  // correction so it doesn't end in middle of tesla
		}
		else
			model_length = 30.0;

		float steps = appCeil (d/model_length);
		float len = (d-model_length)/(steps-1);

		ent.model = b->model;

		// PMM - special case for lightning model .. if the real length is shorter than the model,
		// flip it around & draw it from the end to the start.  This prevents the model from going
		// through the tesla mine (instead it goes through the target)
		if ((b->model == cl_mod_lightning) && (d <= model_length))
		{
			ent.pos.origin = b->end;
			ent.flags = RF_FULLBRIGHT;
			ent.angles[ROLL] = rand() % 360;
			V_AddEntity (&ent);
			return;
		}

		if (b->model == cl_mod_lightning)
		{
			ent.angles[PITCH] = -ent.angles[PITCH];
			ent.angles[YAW] += 180;
			ent.flags = RF_FULLBRIGHT;
		}

		while (d > 0)
		{
			ent.pos.origin = org;
			ent.angles[ROLL] = rand() % 360;
			V_AddEntity (&ent);

			VectorMA (org, len, dist);
			d -= model_length;
		}
	}
}


/*
=================
ROGUE - draw player locked beams
CL_AddPlayerBeams
=================
*/
void CL_AddPlayerBeams (void)
{
	//PMM
	float hand_multiplier;
	if (hand->integer == 2)
		hand_multiplier = 0;
	else if (hand->integer == 1)
		hand_multiplier = -1;
	else
		hand_multiplier = 1;

	// update beams
	int		i;
	mBeam_t	*b;
	for (i = 0, b = cl_mPlayerbeams; i < MAX_BEAMS; i++, b++)
	{
		if (!b->model || b->endtime < cl.time)
			continue;

		bool isHeatbeam = cl_mod_heatbeam && (b->model == cl_mod_heatbeam);
		bool isPlayer = b->entity == cl.playernum + 1;

		CVec3 org;
		if (isHeatbeam)
		{
			// if coming from the player, update the start position
			if (isPlayer)
			{
				// set up gun position
				// code straight out of CL_AddViewWeapon
				player_state_t *ps = &cl.frame.playerstate;
				player_state_t *ops = &cl.oldFrame->playerstate;
				for (int j = 0; j < 3; j++)
					b->start[j] = cl.modelorg[j] + Lerp (ops->gunoffset[j], ps->gunoffset[j], cl.lerpfrac);
				VectorMA (b->start, (hand_multiplier * b->offset[0]), cl.v_right, org);
				VectorMA (org, b->offset[1], cl.v_forward);
				VectorMA (org, b->offset[2], cl.v_up);
				if (hand->integer == 2)
					VectorMA (org, -1, cl.v_up);
			}
			else
				org = b->start;
		}
		else
		{
			// if coming from the player, update the start position
			if (isPlayer)
				b->start = cl.modelorg;
			VectorAdd (b->start, b->offset, org);
		}

		// calculate pitch and yaw
		CVec3 dist;
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

		CVec3 angles;
		Vec2Angles (dist, angles);

		int framenum;
		if (isHeatbeam)
		{
			if (!isPlayer)
			{
				// third person
				framenum = 2;
				CVec3 angles;
				angles.Set (-angles[PITCH], angles[YAW] + 180.0f, 0);
				CVec3 u, f, r;
				AngleVectors (angles, &f, &r, &u);

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
					CL_MonsterPlasmaShell (b->start);
				}
			}
			else
				framenum = 1;
		}

		// if it's the heatbeam, draw the particle effect
		if (isHeatbeam && isPlayer)
			CL_Heatbeam (org, dist);

		// add new entities for the beams
		float d = dist.NormalizeFast ();

		entity_t ent;
		memset (&ent, 0, sizeof(ent));
		float model_length;
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
		float steps = appCeil(d/model_length);
		float len = (d-model_length) / (steps-1);

		ent.model = b->model;

		// PMM - special case for lightning model .. if the real length is shorter than the model,
		// flip it around & draw it from the end to the start.  This prevents the model from going
		// through the tesla mine (instead it goes through the target)
		if ((b->model == cl_mod_lightning) && (d <= model_length))
		{
			ent.pos.origin = b->end;
			ent.flags = RF_FULLBRIGHT;
			ent.angles = angles;
			ent.angles[ROLL] = rand() % 360;
			V_AddEntity (&ent);
			return;
		}

		while (d > 0)
		{
			ent.pos.origin = org;
			if (isHeatbeam)
			{
				ent.flags = RF_FULLBRIGHT;
				ent.angles.Set (-angles[PITCH], angles[YAW] + 180, (cl.time) % 360);
				ent.frame = framenum;
			}
			else if (b->model == cl_mod_lightning)
			{
				ent.flags = RF_FULLBRIGHT;
				ent.angles.Set (-angles[PITCH], angles[YAW] + 180, rand() % 360);
			}
			else
			{
				ent.angles = angles;
				ent.angles[ROLL] = rand() % 360;
			}

//			Com_Printf("B: %d -> %d\n", b->entity, b->dest_entity);
			V_AddEntity (&ent);

			VectorMA (org, len, dist);
			d -= model_length;
		}
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
	AddExplosions ();
	// PMM - set up sustain
	ProcessSustains ();
}
