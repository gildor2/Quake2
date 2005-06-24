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
// cl_fx.cpp -- entity effects parsing and management

#include "client.h"

void CL_LogoutEffect (const CVec3 &org, int type);

/*
==============================================================

LIGHT STYLE MANAGEMENT

==============================================================
*/


lightstyle_t cl_lightstyles[MAX_LIGHTSTYLES];


void CL_ClearLightStyles ()
{
	memset (cl_lightstyles, 0, sizeof(cl_lightstyles));

	// Kingpin light styles
	// 12 FIRE FLICKER (third variety)
	CL_SetLightstyle (12, "mmnommomhkmmomnonmmonqnmmo");
	// 13 FLUORESCENT FLICKER (second variety)
	CL_SetLightstyle (13, "kmamaamakmmmaakmamakmakmmmma");
	// 14 FLUORESCENT FLICKER (third variety)
	CL_SetLightstyle (14, "kmmmakakmmaaamammamkmamakmmmma");
	// 15 REALISTIC FADE (first variety)
	CL_SetLightstyle (15, "mmnnoonnmmmmmmmmmnmmmmnonmmmmmmm");
	// 16 REALISTIC FADE (second variety)
	CL_SetLightstyle (16, "mmmmnonmmmmnmmmmmnonmmmmmnmmmmmmm");
	// 17 SLOW STRONG STROBE (out of phase with 9)
	CL_SetLightstyle (17, "zzzzzzzzaaaaaaaa");
	// 18 THREE CYCLE STROBE (cycle 1)
	CL_SetLightstyle (18, "zzzzzzzzaaaaaaaaaaaaaaaa");
	// 19 THREE CYCLE STROBE (cycle 2)
	CL_SetLightstyle (19, "aaaaaaaazzzzzzzzaaaaaaaa");
	// 20 THREE CYCLE STROBE (cycle 3)
	CL_SetLightstyle (20, "aaaaaaaaaaaaaaaazzzzzzzz");
}


void CL_RunLightStyles (void)
{
	int ofs = cl.time / 100;
	float frac1 = (cl.time % 100) / 100.0f;
	float frac = 1 - frac1;

	int		i;
	lightstyle_t *ls;
	for (i = 0, ls = cl_lightstyles; i < MAX_LIGHTSTYLES; i++, ls++)
	{
		if (!ls->length)
		{
			ls->value = 128;	// 1.0f
			continue;
		}
		if (ls->length == 1)
			ls->value = ls->map[0];
		else
		{
			int pos = ofs % ls->length;
			int pos1 = pos + 1;
			if (pos1 >= ls->length) pos1 = 0;

			ls->value = appRound (ls->map[pos] * frac + ls->map[pos1] * frac1);
		}
	}
}


void CL_SetLightstyle (int i, const char *s)
{
	int len = strlen (s);
	if (len >= MAX_QPATH)
		Com_DropError ("Lightstyle length = %d", len);

	cl_lightstyles[i].length = len;

	// 'a' -> 0; 'm' -> 128 (1.0f); 'z' -> 255 (2.0f)
	byte *dst = cl_lightstyles[i].map;
	for (int j = 0; j < len; j++)
	{
		int c = (*s++ - 'a') * 128 / ('m' - 'a');
		*dst++ = c > 255 ? 255 : c;
	}
}


/*
==============================================================

DLIGHT MANAGEMENT

==============================================================
*/

static cdlight_t cl_dlights[MAX_DLIGHTS];


void CL_ClearDlights (void)
{
	memset (cl_dlights, 0, sizeof(cl_dlights));
}


cdlight_t *CL_AllocDlight (int key, const CVec3 &origin)
{
	int		i;
	cdlight_t *dl;

	cdlight_t *dst = NULL;
	// first look for an exact key match
	if (key)
	{
		dl = cl_dlights;
		for (i = 0; i < MAX_DLIGHTS; i++, dl++)
			if (dl->key == key)
			{
				dst = dl;
				break;
			}
	}

	// then look for anything else
	if (!dst)
	{
		dl = cl_dlights;
		for (i = 0; i < MAX_DLIGHTS; i++, dl++)
			if (dl->die < cl.time)
			{
				dst = dl;
				break;
			}
	}

	if (!dst)
		dst = cl_dlights;

	memset (dst, 0, sizeof(cdlight_t));
	dst->key = key;
	dst->origin = origin;
	return dst;
}


void CL_AddDLights (void)
{
	int			i;
	cdlight_t	*dl;

	dl = cl_dlights;

	for (i = 0; i < MAX_DLIGHTS; i++, dl++)
	{
		if (!dl->radius) continue;
		if (dl->die < cl.time)
		{
			dl->radius = 0;
			continue;
		}
		V_AddLight (dl->origin, dl->radius, dl->color[0], dl->color[1], dl->color[2]);
	}
}


/*
==============================================================

PARTICLE MANAGEMENT

==============================================================
*/

particle_t	*active_particles;
static particle_t *free_particles, particles[MAX_PARTICLES];
//!! NOTE: particles[] array is 240 Kb of size !


struct particleTrace_t
{
	bool		allocated;
	particleType_t type;
	double		time;					// in sec
	float		lifeTime, fadeTime;		// in sec
	float		gravity, elasticity;
	byte		minAlpha, maxAlpha;
	CVec3		vel, pos;
	float		radius;
};


static particleTrace_t particleTraces[MAX_PARTICLE_TRACES];

beam_t	*active_beams;
static beam_t *free_beams, particleBeams[MAX_PARTICLE_BEAMS];


void CL_ClearParticles (void)
{
	int		i;

	// clear particles
	free_particles = particles;
	active_particles = NULL;
	for (i = 0; i < MAX_PARTICLES; i++)
		particles[i].next = &particles[i+1];
	particles[MAX_PARTICLES-1].next = NULL;

	// clear beams
	free_beams = particleBeams;
	active_beams = NULL;
	for (i = 0; i < MAX_PARTICLE_BEAMS; i++)
		particleBeams[i].next = &particleBeams[i+1];
	particleBeams[MAX_PARTICLE_BEAMS-1].next = NULL;

	// other
	memset (particleTraces, 0, sizeof(particleTraces));
}


static void CL_AddParticleTraces (float timeDelta)
{
	static int oldTime;
	int		i;
	particleTrace_t *p;
	float	fovScale;

	fovScale = tan (cl.refdef.fov_x / 2.0f / 180.0f * M_PI);
	for (p = particleTraces, i = 0; i < MAX_PARTICLE_TRACES; p++, i++)
	{
		CVec3	dir, pos, oldpos;
		float	speed, dist, distDelta, pos1;
		float	viewDist, t;
		trace_t	trace;

		if (!p->allocated)
			continue;

		oldpos = p->pos;
		// apply gravity
		if (p->gravity)
			p->vel[2] -= timeDelta * p->gravity;
		// get direction vector and speed
		speed = VectorNormalize (p->vel, dir);
		// update pos
		VectorMA (p->pos, timeDelta, p->vel);
		CL_Trace (trace, oldpos, p->pos, nullBox, CONTENTS_SOLID);
		if (p->elasticity && trace.fraction < 1.0f)
		{
			t = dot (trace.plane.normal, p->vel) * p->elasticity;
			for (int i = 0; i < 3; i++)
			{
				p->vel[i] -= t * trace.plane.normal[i];
				if (p->vel[i] > 0.1f && p->vel[i] < -0.1f) p->vel[i] = 0;
			}
			VectorAdd (trace.endpos, trace.plane.normal, p->pos);
			speed = VectorNormalize (p->vel, dir);
		}
		else
			if (trace.startsolid || trace.allsolid || trace.fraction < 1.0f)
			{
				p->allocated = false;
				continue;
			}

		// calculate loop parameters
		dist = VectorDistance (p->pos, oldpos);		// this is not timeDelta*speed (because pos may be changed)
		viewDist = VectorDistance (p->pos, cl.refdef.vieworg) * fovScale;
		if (viewDist > 128)
		{
			if (viewDist > 1024)
				t = 4.5;
			else
				t = (viewDist - 128) / (2048.0f - 128.0f) * 4.5f + 0.3f;
		}
		else
			t = 0.3f;
		distDelta = t * p->radius;
		// at least 600 particles per second
		t = viewDist / (600.0f * timeDelta);
		if (distDelta > t)
			distDelta = t;
		// but no more than 20 particles per frame
		t = dist / 20.0f;
		if (distDelta < t)
			distDelta = t;
		// and no longer than whole distance
		if (distDelta > dist)
			distDelta = dist;
		if (!distDelta)
			distDelta = 1;

		pos = oldpos;
		for (pos1 = distDelta; pos1 <= dist; pos1 += distDelta)
		{
			particle_t *cp;

			VectorMA (pos, distDelta, dir);
			//?? move this inside "switch"

			switch (p->type)
			{
			case PT_SPARKLE:
				if (!(cp = CL_AllocParticle ()))
				{
					pos1 = dist;
					break;
				}

				t = pos1 / dist * timeDelta;	// time offset

				cp->type = p->type;
				cp->color = 0;					// any -- ignored
				cp->alphavel = -1.0f / p->fadeTime;
				cp->alpha = (p->lifeTime - (p->time + t)) / p->lifeTime *
					(p->maxAlpha - p->minAlpha) + p->minAlpha -
					(timeDelta - t) / p->fadeTime;
				cp->accel[2] = 0;
				cp->vel.Zero();
				cp->org = pos;
				break;
			}
		}

		// update p->pos again (remove previous updates??)
		VectorMA (oldpos, pos1 - distDelta, dir, p->pos);
		p->time += timeDelta;
		if (p->time > p->lifeTime)
			p->allocated = false;
	}
}


particleTrace_t *CL_AllocParticleTrace (const CVec3 &pos, const CVec3 &vel, float lifeTime, float fadeTime)
{
	if (r_sfx_pause->integer == 2)
		return NULL;

	int		i;
	particleTrace_t *p;
	for (i = 0, p = particleTraces; i < MAX_PARTICLE_TRACES; i++, p++)
	{
		if (p->allocated) continue;

		p->allocated = true;
		p->lifeTime = lifeTime;
		p->fadeTime = fadeTime;

		CVec3	dir;
		VectorNormalize (vel, dir);
		p->pos = pos;
		p->vel = vel;

		p->time = 0;

		p->minAlpha = 0;
		p->maxAlpha = 1;

		p->gravity = 400;
		p->elasticity = 1.9;
		p->radius = 1;

		p->type = PT_SPARKLE;
		return p;
	}

	return NULL;
}


static void CL_UpdateParticleBeams (float timeDelta)
{
	beam_t	*b, *prev, *next;

	prev = NULL;
	for (b = active_beams; b; b = next)
	{
		next = b->next;

		float	alphaDelta;
		if (b->lifeTime > 0)
			alphaDelta = (b->alpha - b->dstAlpha) * timeDelta / b->lifeTime;
		else
			alphaDelta = 0;
		float radiusDelta = timeDelta * b->growSpeed;

		b->lifeTime -= timeDelta;
		b->alpha -= alphaDelta;
		b->color.c[3] = appRound (b->alpha * 255);
		b->radius += radiusDelta;

		if ((b->fadeTime > 0 && b->lifeTime <= 0) || b->alpha <= 0 || b->radius <= 0)
		// do not disappear when "fadeTime == 0" (lifetime is 1 frame)
		{
			if (prev)
				prev->next = b->next;
			else
				active_beams = b->next;

			b->next = free_beams;
			free_beams = b;
			continue;
		}
		if (b->fadeTime == 0)
			b->fadeTime = 1;		// beam will be removed next frame

		prev = b;
	}
}


beam_t *CL_AllocParticleBeam (const CVec3 &start, const CVec3 &end, float radius, float fadeTime)
{
	if (r_sfx_pause->integer == 2)
		return NULL;

	beam_t *b = free_beams;
	if (b)
	{
		free_beams = b->next;
		b->next = active_beams;
		active_beams = b;

		b->start      = start;
		b->end        = end;
		b->radius     = radius;
		b->fadeTime = b->lifeTime = fadeTime;
		b->dstAlpha = 0;

		b->color.rgba = RGBA(1,1,1,1);
		b->alpha      = 1;
		b->type       = BEAM_STANDARD;
		b->shader     = NULL;
		b->growSpeed  = 0;
	}
	return b;
}


particle_t *CL_AllocParticle (void)
{
	if (r_sfx_pause->integer == 2)
		return NULL;

	particle_t *p = free_particles;
	if (p)
	{
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY;
		p->type = PT_DEFAULT;
		p->isNew = true;
		p->leafNum = -1;
	}
	return p;
}



/*
===============
CL_UpdateParticles
===============
*/
void CL_UpdateParticles (void)
{
	guard(CL_UpdateParticles);

	static double oldTime;

	float timeDelta = cl.ftime - oldTime;			// time delta, ms
	oldTime = cl.ftime;
	if (r_sfx_pause->integer) timeDelta = 0;
	float time2 = timeDelta * timeDelta;

	CL_UpdateParticleBeams (timeDelta);
	CL_AddParticleTraces (timeDelta);

	particle_t *p, *prev, *next;
	prev = NULL;
	for (p = active_particles; p; p = next)
	{
		next = p->next;

		if (p->isNew)
		{
			p->isNew = false;	// particle is just created -- timeDelta == 0, nothing to update
			p->leafNum = CM_PointLeafnum (p->org);
		}
		else if (timeDelta > 0)
		{
			// update alpha
			p->alpha += timeDelta * p->alphavel;

			// update position
			if (p->vel[2] || p->vel[1] || p->vel[0])
			{
				p->org[0] += p->vel[0] * timeDelta + p->accel[0] * time2;
				p->org[1] += p->vel[1] * timeDelta + p->accel[1] * time2;
				p->org[2] += p->vel[2] * timeDelta + p->accel[2] * time2;
				p->leafNum = CM_PointLeafnum (p->org);
			}
			// update velocity
			p->vel[0] += timeDelta * p->accel[0];
			p->vel[1] += timeDelta * p->accel[1];
			p->vel[2] += timeDelta * p->accel[2];
		}

		// remove particle if it is faded out or out of world
		if (p->alpha <= 0 || CM_LeafContents (p->leafNum) & MASK_SOLID)
		{
			if (prev)
				prev->next = p->next;
			else
				active_particles = p->next;

			p->next = free_particles;
			free_particles = p;
			continue;
		}

		prev = p;
	}

	unguard;
}


void CL_MetalSparks (const CVec3 &pos, const CVec3 &dir, int count)
{
	for (int n = 0; n < count; n++)
	{
		CVec3	vel, pos2;
		for (int i = 0; i < 3; i++)
			vel[i] = (frand () * 150.0f + 100.0f) * dir[i] + crand () * 30.0f;
		// do not start inside wall
		VectorMA (pos, 2, dir, pos2);

		particleTrace_t *p = CL_AllocParticleTrace (pos2, vel, frand () * 0.64f, 0.1f);
		if (!p) return;
		//!! p->brightness = 32;
		if (!CL_AllocParticleTrace (pos2, vel, 0.2f, 0.05f)) return;
	}
}


/*-----------------------------------------------------------------------------
	Parsing network data
-----------------------------------------------------------------------------*/

void CL_ParseMuzzleFlash (void)
{
	int i = MSG_ReadShort (&net_message);
	if (i < 1 || i >= MAX_EDICTS)
		Com_DropError ("CL_ParseMuzzleFlash: bad entity");

	int weapon = MSG_ReadByte (&net_message);
	bool silenced = (weapon & MZ_SILENCED) != 0;
	weapon &= ~MZ_SILENCED;

	centity_t *pl = &cl_entities[i];
	cdlight_t *dl = CL_AllocDlight (i, pl->current.origin);

	VectorMA (dl->origin,  18, pl->current.axis[0]);
	VectorMA (dl->origin, -16, pl->current.axis[1]);	// axis[1] is left, move dlight to right
	if (silenced)
		dl->radius = 100 + (rand()&31);
	else
		dl->radius = 200 + (rand()&31);
	dl->die = cl.time + 10;

	float volume = (silenced) ? 0.2 : 1;

	if (RE_GetCaps() & REF_CONSOLE_ONLY)
		return;

	//	1. snd_name -- if not NULL:
	//		sound(NULL,i,CHAN_WEAPON,S_RegisterSound(snd_name),vol:full/silenced, ATTN_NORM, 0)
	//		snd_name may be %d:rand()%5+1 -- may always pass for sprintf()
	//		snd_name is "weapons/%s.wav"
	//	2. sound(NULL,i,CHAN_AUTO,<snd_name>,vol,ATTN_NORM,0.1)
	typedef struct {
		byte	r, g, b;
		bool	allowSilencer;		// if false, sound volume will be forced to 1
		const char *sound;
		const char *sound2;
	} mzTable_t;
	static const mzTable_t mzTable[] = {
#define F(r,g,b,sil,snd1,snd2)		{ r*255.0f,g*255.0f,b*255.0f,sil,snd1,snd2 }
	F(1,1,0,	true,	"blastf1a", NULL),			// MZ_BLASTER
	F(1,1,0,	true,	"machgf%db", NULL),			// MZ_MACHINEGUN
	F(1,1,0,	true,	"shotgf1b", "shotgr1b"),	// MZ_SHOTGUN
	F(1,0.25,0,	true,	"machgf%db", NULL),			// MZ_CHAINGUN1
	F(1,0.5,0,	true,	"machgf%db", NULL),			// MZ_CHAINGUN2
	F(1,1,0,	true,	"machgf%db", NULL),			// MZ_CHAINGUN3
	F(0.5,0.5,1, true,	"railgf1a", NULL),			// MZ_RAILGUN
	F(1,0.5,0.2, true,	"rocklf1a", "rocklr1b"),	// MZ_ROCKET
	F(1,0.5,0,	true,	"grenlf1a", "grenlr1b"),	// MZ_GRENADE
	F(0,1,0,	false,	"grenlf1a", NULL),			// MZ_LOGIN
	F(1,0,0,	false,	"grenlf1a", NULL),			// MZ_LOGOUT
	F(1,1,0,	false,	"grenlf1a", NULL),			// MZ_RESPAWN
	F(0,1,0,	true,	"bfg__f1y", NULL),			// MZ_BFG
	F(1,1,0,	true,	"sshotf1b", NULL),			// MZ_SSHOTGUN
	F(1,1,0,	true,	"hyprbf1a", NULL),			// MZ_HYPERBLASTER
	F(0,0,0,	false,	NULL, NULL),				// MZ_ITEMRESPAWN -- empty
	// XATRIX
	F(1,0.5, 0.5, true,	"rippfire", NULL),			// MZ_IONRIPPER
	F(0,0,1,	true,	"hyprbf1a", NULL),			// MZ_BLUEHYPERBLASTER
	F(1,0.5, 0.5, true,	"plasshot", NULL),			// MZ_PHALANX
	//-------------------------------------------
	// MZ_PHALANX == 18 ... MZ_ETF_RIFLE == 30
	//-------------------------------------------
	// ROGUE
	F(0.9,0.7,0, true,	"nail1", NULL),				// MZ_ETF_RIFLE
	F(0,0,0,	false,	NULL, NULL),				// MZ_UNUSED -- empty
	F(1,1,0,	true,	"shotg2", NULL),			// MZ_SHOTGUN2
	F(1,1,0,	false,	NULL/* "bfg__l1a" */, NULL),				// MZ_HEATBEAM
	F(0,1,0,	true,	"blastf1a", NULL),			// MZ_BLASTER2; FIXME - different sound for blaster2 ??
	F(-1,-1,-1,	true,	"disint2", NULL),			// MZ_TRACKER -- negative dlight !
	F(1,0,0,	false,	NULL, NULL),				// MZ_NUKE1
	F(1,1,0,	false,	NULL, NULL),				// MZ_NUKE2
	F(0,0,1,	false,	NULL, NULL),				// MZ_NUKE4
	F(0,1,1,	false,	NULL, NULL),				// MZ_NUKE8
	};
#undef F
	if (weapon < MZ_BLASTER || (weapon > MZ_PHALANX && weapon < MZ_ETF_RIFLE) || weapon > MZ_NUKE8)
		return;			// bad effect num

	// get table entry with squeezing MZ_PHALANX..MZ_ETF_RIFLE unused cells
	const mzTable_t *tbl = &mzTable[ (weapon < MZ_ETF_RIFLE) ? weapon : weapon - MZ_ETF_RIFLE + MZ_PHALANX + 1 ];

	// process table data
	dl->color.Set (tbl->r / 255.0f, tbl->g / 255.0f, tbl->b / 255.0f);
	if (!tbl->allowSilencer) volume = 1;
	if (tbl->sound)
		S_StartSound (NULL, i, CHAN_WEAPON, S_RegisterSound(va("weapons/%s.wav", va(tbl->sound, rand() % 5 + 1))), volume, ATTN_NORM, 0);
	if (tbl->sound2)
		S_StartSound (NULL, i, CHAN_AUTO, S_RegisterSound(va("weapons/%s.wav", tbl->sound2)), volume, ATTN_NORM, 0.1);

	// additional effects
	switch (weapon)
	{
	case MZ_CHAINGUN1:
		dl->radius = 200 + (rand()&31);
		break;
	case MZ_CHAINGUN2:
		dl->radius = 225 + (rand()&31);
		S_StartSound (NULL, i, CHAN_WEAPON, S_RegisterSound(va("weapons/machgf%ib.wav", (rand() % 5) + 1)), volume, ATTN_NORM, 0.05);
		break;
	case MZ_CHAINGUN3:
		dl->radius = 250 + (rand()&31);
		S_StartSound (NULL, i, CHAN_WEAPON, S_RegisterSound(va("weapons/machgf%ib.wav", (rand() % 5) + 1)), volume, ATTN_NORM, 0.033);
		S_StartSound (NULL, i, CHAN_WEAPON, S_RegisterSound(va("weapons/machgf%ib.wav", (rand() % 5) + 1)), volume, ATTN_NORM, 0.066);
		break;
	case MZ_LOGIN:
	case MZ_LOGOUT:
	case MZ_RESPAWN:
		CL_LogoutEffect (pl->current.origin, weapon);
		break;
	case MZ_HEATBEAM:
	case MZ_NUKE1:
	case MZ_NUKE2:
	case MZ_NUKE4:
	case MZ_NUKE8:
		dl->die = cl.time + 100;
		break;
	}
}


#include "monster_flash.h"

void CL_ParseMuzzleFlash2 (void)
{
	CVec3	origin;

	int ent = MSG_ReadShort (&net_message);
	if (ent < 1 || ent >= MAX_EDICTS)
		Com_DropError ("CL_ParseMuzzleFlash2: bad entity");

	int flash_number = MSG_ReadByte (&net_message);

	// locate the origin
	clEntityState_t *s = &cl_entities[ent].current;

	float sx = monster_flash_offset[flash_number][0] / MONSTER_FLASH_SCALE;
	float sy = monster_flash_offset[flash_number][1] / MONSTER_FLASH_SCALE;
	float sz = monster_flash_offset[flash_number][2] / MONSTER_FLASH_SCALE;
	VectorMA (s->origin, sx, s->axis[0], origin);	// forward direction
	VectorMA (origin,   -sy, s->axis[1], origin);	// left->right direction
	origin[2] += sz;

	cdlight_t *dl = CL_AllocDlight (ent, origin);
	dl->radius = 200 + (rand()&31);
	dl->die = cl.time + 10;
	// most used dlight color
	dl->color[0] = 1;
	dl->color[1] = 1;
	dl->color[2] = 0;

	if (RE_GetCaps() & REF_CONSOLE_ONLY)
		return;

	switch (flash_number)
	{
	case MZ2_TANK_BLASTER_1:
	case MZ2_TANK_BLASTER_2:
	case MZ2_TANK_BLASTER_3:
		S_StartSound (NULL, ent, CHAN_WEAPON, S_RegisterSound("tank/tnkatck3.wav"), 1, ATTN_NORM, 0);
		break;

	case MZ2_TANK_MACHINEGUN_1:
	case MZ2_TANK_MACHINEGUN_2:
	case MZ2_TANK_MACHINEGUN_3:
	case MZ2_TANK_MACHINEGUN_4:
	case MZ2_TANK_MACHINEGUN_5:
	case MZ2_TANK_MACHINEGUN_6:
	case MZ2_TANK_MACHINEGUN_7:
	case MZ2_TANK_MACHINEGUN_8:
	case MZ2_TANK_MACHINEGUN_9:
	case MZ2_TANK_MACHINEGUN_10:
	case MZ2_TANK_MACHINEGUN_11:
	case MZ2_TANK_MACHINEGUN_12:
	case MZ2_TANK_MACHINEGUN_13:
	case MZ2_TANK_MACHINEGUN_14:
	case MZ2_TANK_MACHINEGUN_15:
	case MZ2_TANK_MACHINEGUN_16:
	case MZ2_TANK_MACHINEGUN_17:
	case MZ2_TANK_MACHINEGUN_18:
	case MZ2_TANK_MACHINEGUN_19:
		CL_ParticleEffect (origin, nullVec3, 0, 40);
		CL_SmokeAndFlash (origin);
		S_StartSound (NULL, ent, CHAN_WEAPON, S_RegisterSound(va("tank/tnkatk2%c.wav", 'a' + rand() % 5)), 1, ATTN_NORM, 0);
		break;

	case MZ2_TANK_ROCKET_1:
	case MZ2_TANK_ROCKET_2:
	case MZ2_TANK_ROCKET_3:
		dl->color[0] = 1;dl->color[1] = 0.5;dl->color[2] = 0.2;
		S_StartSound (NULL, ent, CHAN_WEAPON, S_RegisterSound("tank/tnkatck1.wav"), 1, ATTN_NORM, 0);
		break;

	case MZ2_INFANTRY_MACHINEGUN_1:
	case MZ2_INFANTRY_MACHINEGUN_2:
	case MZ2_INFANTRY_MACHINEGUN_3:
	case MZ2_INFANTRY_MACHINEGUN_4:
	case MZ2_INFANTRY_MACHINEGUN_5:
	case MZ2_INFANTRY_MACHINEGUN_6:
	case MZ2_INFANTRY_MACHINEGUN_7:
	case MZ2_INFANTRY_MACHINEGUN_8:
	case MZ2_INFANTRY_MACHINEGUN_9:
	case MZ2_INFANTRY_MACHINEGUN_10:
	case MZ2_INFANTRY_MACHINEGUN_11:
	case MZ2_INFANTRY_MACHINEGUN_12:
	case MZ2_INFANTRY_MACHINEGUN_13:
		CL_ParticleEffect (origin, nullVec3, 0, 40);
		CL_SmokeAndFlash (origin);
		S_StartSound (NULL, ent, CHAN_WEAPON, S_RegisterSound("infantry/infatck1.wav"), 1, ATTN_NORM, 0);
		break;

	case MZ2_SOLDIER_MACHINEGUN_1:
	case MZ2_SOLDIER_MACHINEGUN_2:
	case MZ2_SOLDIER_MACHINEGUN_3:
	case MZ2_SOLDIER_MACHINEGUN_4:
	case MZ2_SOLDIER_MACHINEGUN_5:
	case MZ2_SOLDIER_MACHINEGUN_6:
	case MZ2_SOLDIER_MACHINEGUN_7:
	case MZ2_SOLDIER_MACHINEGUN_8:
		CL_ParticleEffect (origin, nullVec3, 0, 40);
		CL_SmokeAndFlash (origin);
		S_StartSound (NULL, ent, CHAN_WEAPON, S_RegisterSound("soldier/solatck3.wav"), 1, ATTN_NORM, 0);
		break;

	case MZ2_GUNNER_MACHINEGUN_1:
	case MZ2_GUNNER_MACHINEGUN_2:
	case MZ2_GUNNER_MACHINEGUN_3:
	case MZ2_GUNNER_MACHINEGUN_4:
	case MZ2_GUNNER_MACHINEGUN_5:
	case MZ2_GUNNER_MACHINEGUN_6:
	case MZ2_GUNNER_MACHINEGUN_7:
	case MZ2_GUNNER_MACHINEGUN_8:
		CL_ParticleEffect (origin, nullVec3, 0, 40);
		CL_SmokeAndFlash (origin);
		S_StartSound (NULL, ent, CHAN_WEAPON, S_RegisterSound("gunner/gunatck2.wav"), 1, ATTN_NORM, 0);
		break;

	case MZ2_ACTOR_MACHINEGUN_1:
	case MZ2_SUPERTANK_MACHINEGUN_1:
	case MZ2_SUPERTANK_MACHINEGUN_2:
	case MZ2_SUPERTANK_MACHINEGUN_3:
	case MZ2_SUPERTANK_MACHINEGUN_4:
	case MZ2_SUPERTANK_MACHINEGUN_5:
	case MZ2_SUPERTANK_MACHINEGUN_6:
	case MZ2_TURRET_MACHINEGUN:			// PGM
		CL_ParticleEffect (origin, nullVec3, 0, 40);
		CL_SmokeAndFlash (origin);
		S_StartSound (NULL, ent, CHAN_WEAPON, S_RegisterSound("infantry/infatck1.wav"), 1, ATTN_NORM, 0);
		break;

	case MZ2_BOSS2_MACHINEGUN_L1:
	case MZ2_BOSS2_MACHINEGUN_L2:
	case MZ2_BOSS2_MACHINEGUN_L3:
	case MZ2_BOSS2_MACHINEGUN_L4:
	case MZ2_BOSS2_MACHINEGUN_L5:
	case MZ2_CARRIER_MACHINEGUN_L1:		// PMM
	case MZ2_CARRIER_MACHINEGUN_L2:		// PMM
		CL_ParticleEffect (origin, nullVec3, 0, 40);
		CL_SmokeAndFlash (origin);
		S_StartSound (NULL, ent, CHAN_WEAPON, S_RegisterSound("infantry/infatck1.wav"), 1, ATTN_NONE, 0);
		break;

	case MZ2_SOLDIER_BLASTER_1:
	case MZ2_SOLDIER_BLASTER_2:
	case MZ2_SOLDIER_BLASTER_3:
	case MZ2_SOLDIER_BLASTER_4:
	case MZ2_SOLDIER_BLASTER_5:
	case MZ2_SOLDIER_BLASTER_6:
	case MZ2_SOLDIER_BLASTER_7:
	case MZ2_SOLDIER_BLASTER_8:
	case MZ2_TURRET_BLASTER:			// PGM
		S_StartSound (NULL, ent, CHAN_WEAPON, S_RegisterSound("soldier/solatck2.wav"), 1, ATTN_NORM, 0);
		break;

	case MZ2_FLYER_BLASTER_1:
	case MZ2_FLYER_BLASTER_2:
		S_StartSound (NULL, ent, CHAN_WEAPON, S_RegisterSound("flyer/flyatck3.wav"), 1, ATTN_NORM, 0);
		break;

	case MZ2_MEDIC_BLASTER_1:
		S_StartSound (NULL, ent, CHAN_WEAPON, S_RegisterSound("medic/medatck1.wav"), 1, ATTN_NORM, 0);
		break;

	case MZ2_HOVER_BLASTER_1:
		S_StartSound (NULL, ent, CHAN_WEAPON, S_RegisterSound("hover/hovatck1.wav"), 1, ATTN_NORM, 0);
		break;

	case MZ2_FLOAT_BLASTER_1:
		S_StartSound (NULL, ent, CHAN_WEAPON, S_RegisterSound("floater/fltatck1.wav"), 1, ATTN_NORM, 0);
		break;

	case MZ2_SOLDIER_SHOTGUN_1:
	case MZ2_SOLDIER_SHOTGUN_2:
	case MZ2_SOLDIER_SHOTGUN_3:
	case MZ2_SOLDIER_SHOTGUN_4:
	case MZ2_SOLDIER_SHOTGUN_5:
	case MZ2_SOLDIER_SHOTGUN_6:
	case MZ2_SOLDIER_SHOTGUN_7:
	case MZ2_SOLDIER_SHOTGUN_8:
		CL_SmokeAndFlash (origin);
		S_StartSound (NULL, ent, CHAN_WEAPON, S_RegisterSound("soldier/solatck1.wav"), 1, ATTN_NORM, 0);
		break;

	case MZ2_CHICK_ROCKET_1:
	case MZ2_TURRET_ROCKET:			// PGM
		dl->color[0] = 1;dl->color[1] = 0.5;dl->color[2] = 0.2;
		S_StartSound (NULL, ent, CHAN_WEAPON, S_RegisterSound("chick/chkatck2.wav"), 1, ATTN_NORM, 0);
		break;

	case MZ2_SUPERTANK_ROCKET_1:
	case MZ2_SUPERTANK_ROCKET_2:
	case MZ2_SUPERTANK_ROCKET_3:
	case MZ2_BOSS2_ROCKET_1:
	case MZ2_BOSS2_ROCKET_2:
	case MZ2_BOSS2_ROCKET_3:
	case MZ2_BOSS2_ROCKET_4:
	case MZ2_CARRIER_ROCKET_1:
//	case MZ2_CARRIER_ROCKET_2:
//	case MZ2_CARRIER_ROCKET_3:
//	case MZ2_CARRIER_ROCKET_4:
		dl->color[0] = 1;dl->color[1] = 0.5;dl->color[2] = 0.2;
		S_StartSound (NULL, ent, CHAN_WEAPON, S_RegisterSound("tank/rocket.wav"), 1, ATTN_NORM, 0);
		break;

	case MZ2_GUNNER_GRENADE_1:
	case MZ2_GUNNER_GRENADE_2:
	case MZ2_GUNNER_GRENADE_3:
	case MZ2_GUNNER_GRENADE_4:
		dl->color[0] = 1;dl->color[1] = 0.5;dl->color[2] = 0;
		S_StartSound (NULL, ent, CHAN_WEAPON, S_RegisterSound("gunner/gunatck3.wav"), 1, ATTN_NORM, 0);
		break;

	case MZ2_GLADIATOR_RAILGUN_1:
	// PMM
	case MZ2_CARRIER_RAILGUN:
	case MZ2_WIDOW_RAIL:
	// pmm
		dl->color[0] = 0.5;dl->color[1] = 0.5;dl->color[2] = 1.0;
		break;

	case MZ2_MAKRON_BFG:
		dl->color[0] = 0.5;dl->color[1] = 1 ;dl->color[2] = 0.5;
		//S_StartSound (NULL, ent, CHAN_WEAPON, S_RegisterSound("makron/bfg_fire.wav"), 1, ATTN_NORM, 0);
		break;

	case MZ2_MAKRON_BLASTER_1:
	case MZ2_MAKRON_BLASTER_2:
	case MZ2_MAKRON_BLASTER_3:
	case MZ2_MAKRON_BLASTER_4:
	case MZ2_MAKRON_BLASTER_5:
	case MZ2_MAKRON_BLASTER_6:
	case MZ2_MAKRON_BLASTER_7:
	case MZ2_MAKRON_BLASTER_8:
	case MZ2_MAKRON_BLASTER_9:
	case MZ2_MAKRON_BLASTER_10:
	case MZ2_MAKRON_BLASTER_11:
	case MZ2_MAKRON_BLASTER_12:
	case MZ2_MAKRON_BLASTER_13:
	case MZ2_MAKRON_BLASTER_14:
	case MZ2_MAKRON_BLASTER_15:
	case MZ2_MAKRON_BLASTER_16:
	case MZ2_MAKRON_BLASTER_17:
		S_StartSound (NULL, ent, CHAN_WEAPON, S_RegisterSound("makron/blaster.wav"), 1, ATTN_NORM, 0);
		break;

	case MZ2_JORG_MACHINEGUN_L1:
	case MZ2_JORG_MACHINEGUN_L2:
	case MZ2_JORG_MACHINEGUN_L3:
	case MZ2_JORG_MACHINEGUN_L4:
	case MZ2_JORG_MACHINEGUN_L5:
	case MZ2_JORG_MACHINEGUN_L6:
		CL_ParticleEffect (origin, nullVec3, 0, 40);
		CL_SmokeAndFlash (origin);
		S_StartSound (NULL, ent, CHAN_WEAPON, S_RegisterSound("boss3/xfire.wav"), 1, ATTN_NORM, 0);
		break;

	case MZ2_JORG_MACHINEGUN_R1:
	case MZ2_JORG_MACHINEGUN_R2:
	case MZ2_JORG_MACHINEGUN_R3:
	case MZ2_JORG_MACHINEGUN_R4:
	case MZ2_JORG_MACHINEGUN_R5:
	case MZ2_JORG_MACHINEGUN_R6:
		CL_ParticleEffect (origin, nullVec3, 0, 40);
		CL_SmokeAndFlash (origin);
		break;

	case MZ2_JORG_BFG_1:
		dl->color[0] = 0.5;dl->color[1] = 1 ;dl->color[2] = 0.5;
		break;

	case MZ2_BOSS2_MACHINEGUN_R1:
	case MZ2_BOSS2_MACHINEGUN_R2:
	case MZ2_BOSS2_MACHINEGUN_R3:
	case MZ2_BOSS2_MACHINEGUN_R4:
	case MZ2_BOSS2_MACHINEGUN_R5:
	case MZ2_CARRIER_MACHINEGUN_R1:			// PMM
	case MZ2_CARRIER_MACHINEGUN_R2:			// PMM
		CL_ParticleEffect (origin, nullVec3, 0, 40);
		CL_SmokeAndFlash (origin);
		break;

	case MZ2_STALKER_BLASTER:
	case MZ2_DAEDALUS_BLASTER:
	case MZ2_MEDIC_BLASTER_2:
	case MZ2_WIDOW_BLASTER:
	case MZ2_WIDOW_BLASTER_SWEEP1:
	case MZ2_WIDOW_BLASTER_SWEEP2:
	case MZ2_WIDOW_BLASTER_SWEEP3:
	case MZ2_WIDOW_BLASTER_SWEEP4:
	case MZ2_WIDOW_BLASTER_SWEEP5:
	case MZ2_WIDOW_BLASTER_SWEEP6:
	case MZ2_WIDOW_BLASTER_SWEEP7:
	case MZ2_WIDOW_BLASTER_SWEEP8:
	case MZ2_WIDOW_BLASTER_SWEEP9:
	case MZ2_WIDOW_BLASTER_100:
	case MZ2_WIDOW_BLASTER_90:
	case MZ2_WIDOW_BLASTER_80:
	case MZ2_WIDOW_BLASTER_70:
	case MZ2_WIDOW_BLASTER_60:
	case MZ2_WIDOW_BLASTER_50:
	case MZ2_WIDOW_BLASTER_40:
	case MZ2_WIDOW_BLASTER_30:
	case MZ2_WIDOW_BLASTER_20:
	case MZ2_WIDOW_BLASTER_10:
	case MZ2_WIDOW_BLASTER_0:
	case MZ2_WIDOW_BLASTER_10L:
	case MZ2_WIDOW_BLASTER_20L:
	case MZ2_WIDOW_BLASTER_30L:
	case MZ2_WIDOW_BLASTER_40L:
	case MZ2_WIDOW_BLASTER_50L:
	case MZ2_WIDOW_BLASTER_60L:
	case MZ2_WIDOW_BLASTER_70L:
	case MZ2_WIDOW_RUN_1:
	case MZ2_WIDOW_RUN_2:
	case MZ2_WIDOW_RUN_3:
	case MZ2_WIDOW_RUN_4:
	case MZ2_WIDOW_RUN_5:
	case MZ2_WIDOW_RUN_6:
	case MZ2_WIDOW_RUN_7:
	case MZ2_WIDOW_RUN_8:
		dl->color[0] = 0;dl->color[1] = 1;dl->color[2] = 0;
		S_StartSound (NULL, ent, CHAN_WEAPON, S_RegisterSound("tank/tnkatck3.wav"), 1, ATTN_NORM, 0);
		break;

	case MZ2_WIDOW_DISRUPTOR:
		dl->color[0] = -1;dl->color[1] = -1;dl->color[2] = -1;
		S_StartSound (NULL, ent, CHAN_WEAPON, S_RegisterSound("weapons/disint2.wav"), 1, ATTN_NORM, 0);
		break;

	case MZ2_WIDOW_PLASMABEAM:
	case MZ2_WIDOW2_BEAMER_1:
	case MZ2_WIDOW2_BEAMER_2:
	case MZ2_WIDOW2_BEAMER_3:
	case MZ2_WIDOW2_BEAMER_4:
	case MZ2_WIDOW2_BEAMER_5:
	case MZ2_WIDOW2_BEAM_SWEEP_1:
	case MZ2_WIDOW2_BEAM_SWEEP_2:
	case MZ2_WIDOW2_BEAM_SWEEP_3:
	case MZ2_WIDOW2_BEAM_SWEEP_4:
	case MZ2_WIDOW2_BEAM_SWEEP_5:
	case MZ2_WIDOW2_BEAM_SWEEP_6:
	case MZ2_WIDOW2_BEAM_SWEEP_7:
	case MZ2_WIDOW2_BEAM_SWEEP_8:
	case MZ2_WIDOW2_BEAM_SWEEP_9:
	case MZ2_WIDOW2_BEAM_SWEEP_10:
	case MZ2_WIDOW2_BEAM_SWEEP_11:
		dl->radius = 300 + (rand()&100);
		dl->die = cl.time + 200;
		break;
	}
}


/*-----------------------------------------------------------------------------
	Effects
-----------------------------------------------------------------------------*/

struct particleEffect_t
{
	byte	color, colorRand;		// color..colorRand-1
	CBox	bounds;
	CBox	velocity;
	float	fadeTime1, fadeTime2;
	float	gravity;
	float	width;
	float	velWidth;
	float	interval;
	float	density;
};

static void ParticleEffect (const CVec3 &origin, int count, const particleEffect_t &fx,
	const CVec3 &dir = nullVec3, const CVec3 &moveDir = nullVec3)
{
	CVec3 org = origin;
	for (int i = 0; i < count; i++)
	{
		if ((fx.density > 0 && fx.density < 1) && (frand () <= fx.density))
			continue;

		particle_t *p = CL_AllocParticle ();
		if (!p) return;					// no free particles

		p->color = fx.color;
		if (fx.colorRand > 1)
			p->color += rand() & (fx.colorRand-1);

		for (int j = 0; j < 3; j++)
		{
			p->org[j] = org[j] + Lerp (fx.bounds.mins[j], fx.bounds.maxs[j], frand());
			p->vel[j] = Lerp (fx.velocity.mins[j], fx.velocity.maxs[j], frand());
		}
		// stretch into "dir"/"width" direction
		float dist = frand() * fx.width;
		if (dist) VectorMA (p->org, dist, dir);
		// stretch velocity into velDir direction
		if (fx.velWidth) VectorMA (p->vel, fx.velWidth, dir);

		p->accel[2] = fx.gravity;
		p->alpha = 1.0f;
		p->alphavel = -1.0f / Lerp (fx.fadeTime1, fx.fadeTime2, frand());

		VectorAdd (org, moveDir, org);
	}
}


static void MovableParticleEffect (const CVec3 &start, const CVec3 &end, const particleEffect_t &fx)
{
	CVec3 delta;
	VectorSubtract (end, start, delta);
	float len = delta.NormalizeFast ();
	delta.Scale (fx.interval);
	ParticleEffect (start, appCeil (len / fx.interval), fx, nullVec3, delta);
}


/*-----------------------------------------------------------------------------
	Simple static particle effects
-----------------------------------------------------------------------------*/

// Most used values: dir=nullVec3, color=0, count=40
void CL_ParticleEffect (const CVec3 &org, const CVec3 &dir, int color, int count)
{
	static particleEffect_t fx = {
		0, 8,							// color -> from arg
		{-4, -4, -4,  4, 4, 4},			// bounds
		{-20,-20,-20,  20,20,20},		// velocity
		0.5, 0.8,						// fade time
		-PARTICLE_GRAVITY,				// gravity
		32								// width
	};
	fx.color = color;
	ParticleEffect (org, count, fx, dir);
}


// same as CL_ParticleEffect(), but with single color (no rand)
void CL_ParticleEffect2 (const CVec3 &org, const CVec3 &dir, int color, int count)
{
	static particleEffect_t fx = {
		0, 1,							// color -> from arg
		{-4, -4, -4,  4, 4, 4},			// bounds
		{-20,-20,-20,  20,20,20},		// velocity
		0.5, 0.8,						// fade time
		-PARTICLE_GRAVITY,				// gravity
		8								// width
	};
	ParticleEffect (org, count, fx, dir);
}


// same as CL_ParticleEffect2(), but with positive gravity
void CL_ParticleEffect3 (const CVec3 &org, const CVec3 &dir, int color, int count)
{
	static particleEffect_t fx = {
		0, 1,							// color -> from arg
		{-4, -4, -4,  4, 4, 4},			// bounds
		{-20,-20,-20,  20,20,20},		// velocity
		0.5, 0.8,						// fade time
		PARTICLE_GRAVITY,				// gravity
		8								// width
	};
	fx.color = color;
	ParticleEffect (org, count, fx, dir);
}


void CL_TeleporterParticles (clEntityState_t *ent)
{
	static const particleEffect_t fx = {
		0xDB, 1,						// color
		{-16,-16,-8,  16,16, 0},		// bounds
		{-14,-14,80,  14,14,88},		// velocity
		2, 2,							// fade time
		-PARTICLE_GRAVITY				// gravity
	};
	ParticleEffect (ent->origin, 8, fx);
}


void CL_LogoutEffect (const CVec3 &org, int type)
{
	int color = 0xE0;		// yellow
	if (type == MZ_LOGIN)
		color = 0xD0;		// green
	else if (type = MZ_LOGOUT)
		color = 0x40;		// red

	static particleEffect_t fx = {
		0, 8,							// color
		{-16,-16,-24, 16,16,32},		// bounds
		{-20,-20,-20, 20,20,20},		// velocity
		1.0, 1.3,						// fade time
		-PARTICLE_GRAVITY				// gravity
	};
	fx.color = color;
	ParticleEffect (org, 500, fx);
}


void CL_ItemRespawnParticles (const CVec3 &org)
{
	static const particleEffect_t fx = {
		0xD4, 4,						// color
		{-8,-8,-8,  8, 8, 8},			// bounds
		{-8,-8,-8,  8, 8, 8},			// velocity
		1.0, 1.3,						// fade time
		-PARTICLE_GRAVITY*0.2			// gravity
	};
	ParticleEffect (org, 64, fx);
}


void CL_ExplosionParticles (const CVec3 &org)
{
	static const particleEffect_t fx = {
		0xE0, 8,						// color
		{-16,-16,-16, 16,16,16},		// bounds
		{-192,-192,-192, 192,192,192},	// velocity
		0.625, 1,						// fade time
		-PARTICLE_GRAVITY				// gravity
	};
	ParticleEffect (org, 256, fx);
}


void CL_BFGExplosionParticles (const CVec3 &org)
{
	static const particleEffect_t fx = {
		0xD0, 8,						// color
		{-16,-16,-16, 16,16,16},		// bounds
		{-192,-192,-192, 192,192,192},	// velocity
		0.625, 1,						// fade time
		-PARTICLE_GRAVITY				// gravity
	};
	ParticleEffect (org, 256, fx);
}


// used for tracker explosion only, color=1, run=1
void CL_ColorExplosionParticles (const CVec3 &org, int color, int run)
{
	static particleEffect_t fx = {
		0, 1,							// both params -> from args
		{-16,-16,-16, 16,16,16},		// bounds
		{-128,-128,-128, 128,128,128},	// velocity
		1.5, 2.0,						// fade time
		-PARTICLE_GRAVITY				// gravity
	};
	fx.color = color;
	fx.colorRand = run;
	ParticleEffect (org, 128, fx);
}


void CL_BlasterParticles (const CVec3 &org, const CVec3 &dir, unsigned color)
{
	static particleEffect_t fx = {
		0, 8,							// color -> from arg
		{-4, -4, -4,  4, 4, 4},			// bounds
		{-40,-40,-40, 40,40,40},		// velocity
		0.5, 0.8,						// fade time
		-PARTICLE_GRAVITY,				// gravity
		15,								// width
		30								// vel width
	};
	fx.color = color;
	ParticleEffect (org, 40, fx, dir);
}


/*-----------------------------------------------------------------------------
	Simple particle trails
-----------------------------------------------------------------------------*/

void CL_BlasterTrail (const CVec3 &start, const CVec3 &end)
{
	static const particleEffect_t fx = {
		0xE0, 1,						// color
		{-1, -1, -1,  1, 1, 1},			// bounds
		{-5, -5, -5,  5, 5, 5},			// velocity
		1.5/20, 2.5/20,					// fade time
		0,								// gravity
		0,								// width
		0,								// velWidth
		1,								// interval
		0								// density
	};
	MovableParticleEffect (start, end, fx);
}


// same as CL_BlasterTrail, but green
void CL_BlasterTrail2 (const CVec3 &start, const CVec3 &end)
{
	static const particleEffect_t fx = {
		0xD0, 1,						// color
		{-1, -1, -1,  1, 1, 1},			// bounds
		{-5, -5, -5,  5, 5, 5},			// velocity
		1.5/20, 2.5/20,					// fade time
		0,								// gravity
		0,								// width
		0,								// velWidth
		1,								// interval
		0								// density
	};
	MovableParticleEffect (start, end, fx);
}


void CL_FlagTrail (const CVec3 &start, const CVec3 &end, int color)
{
	static particleEffect_t fx = {
		0, 1,							// color -> from arg
		{-16,-16,-16, 16,16,16},		// bounds
		{-5, -5, -5,  5, 5, 5},			// velocity
		0.8, 1.0,						// fade time
		0,								// gravity
		0,								// width
		0,								// velWidth
		5,								// interval
		0								// density
	};
	fx.color = color;
	MovableParticleEffect (start, end, fx);
}


void CL_RocketTrail (const CVec3 &start, const CVec3 &end, centity_t *old)
{
	// smoke
	CL_DiminishingTrail (start, end, old, EF_ROCKET);
	// fire
	static const particleEffect_t fx = {
		0xDC, 4,						// color
		{-5, -5, -5,  5, 5, 5},			// bounds
		{-20,-20,-20, 20,20,20},		// velocity
		1.0, 1.2,						// fade time
		-PARTICLE_GRAVITY,				// gravity
		0,								// width
		0,								// velWidth
		1,								// interval
		0.125							// density
	};
	MovableParticleEffect (start, end, fx);
}


void CL_BubbleTrail (const CVec3 &start, const CVec3 &end)
{
	static const particleEffect_t fx = {
		4, 8,							// color
		{-2, -2, -2,  2, 2, 2},			// bounds
		{-5, -5,  1,  5, 5,11},			// velocity
		1.0, 1.2,						// fade time
		0,								// gravity
		0,								// width
		0,								// velWidth
		32,								// interval
		0								// density
	};
	MovableParticleEffect (start, end, fx);
}


void CL_BubbleTrail2 (const CVec3 &start, const CVec3 &end, int dist)
{
	static particleEffect_t fx = {
		4, 8,							// color
		{-2, -2, -6,  2, 2,-2},			// bounds
		{-10,-10,10,  10,10,30},		// velocity
		1.0, 1.1,						// fade time
		0,								// gravity
		0,								// width
		0,								// velWidth
		1,								// interval -> from arg
		0								// density
	};
	fx.interval = dist;
	MovableParticleEffect (start, end, fx);
}


void CL_TagTrail (const CVec3 &start, const CVec3 &end, int color)
{
	static particleEffect_t fx = {
		0, 1,							// color -> from arg
		{-16,-16,-16,  16,16,16},		// bounds
		{-5,-5,-5,   5, 5, 5},			// velocity
		0.8, 1.0,						// fade time
		0,								// gravity
		0,								// width
		0,								// velWidth
		5,								// interval
		0								// density
	};
	fx.color = color;
	MovableParticleEffect (start, end, fx);
}


void CL_DebugTrail (const CVec3 &start, const CVec3 &end)
{
	static const particleEffect_t fx = {
		0x74, 8,						// color
		{-2, -2, -6,  2, 2,-2},			// bounds
		{0, 0, 0,   0, 0, 0},			// velocity
		10, 10,							// fade time
		0,								// gravity
		0,								// width
		0,								// velWidth
		3,								// interval
		0								// density
	};
	MovableParticleEffect (start, end, fx);
}


void CL_ForceWall (const CVec3 &start, const CVec3 &end, int color)
{
	static particleEffect_t fx = {
		0, 1,							// color -> from arg
		{-3, -3, -3,  3, 3, 3},			// bounds
		{0, 0,-50,  0,0,-30},			// velocity
		3.0, 3.5,						// fade time
		0,								// gravity
		0,								// width
		0,								// velWidth
		4,								// interval
		0.7								// density
	};
	fx.color = color;
	MovableParticleEffect (start, end, fx);
}


/*-----------------------------------------------------------------------------
	Non-trivial particle effects
-----------------------------------------------------------------------------*/

void CL_BigTeleportParticles (const CVec3 &org)
{
	int			i;
	particle_t	*p;
	float		angle, dist;
	static int colortable[4] = {2*8,13*8,21*8,18*8};

	for (i=0 ; i<4096 ; i++)
	{
		if (!(p = CL_AllocParticle ()))
			return;

		p->color = colortable[rand()&3];

		angle = M_PI*2*(rand()&1023)/1023.0;
		dist = rand()&31;
		p->org[0] = org[0] + cos(angle)*dist;
		p->vel[0] = cos(angle)*(70+(rand()&63));
		p->accel[0] = -cos(angle)*200;

		p->org[1] = org[1] + sin(angle)*dist;
		p->vel[1] = sin(angle)*(70+(rand()&63));
		p->accel[1] = -sin(angle)*200;

		p->org[2] = org[2] + 8 + (rand()%90);
		p->vel[2] = -100 + (rand()&31);
		p->accel[2] = PARTICLE_GRAVITY*4;
		p->alpha = 1.0;

		p->alphavel = -0.3 / (0.5 + frand()*0.3);
	}
}


void CL_DiminishingTrail (const CVec3 &start, const CVec3 &end, centity_t *old, int flags)
{
	CVec3		move, vec;
	int			j;
	particle_t	*p;
	float		orgscale;
	float		velscale;

	move = start;
	VectorSubtract (end, start, vec);
	float len = vec.NormalizeFast ();

	float dec = 0.5f;
	vec.Scale (dec);

	if (old->trailcount > 900)
	{
		orgscale = 4;
		velscale = 15;
	}
	else if (old->trailcount > 800)
	{
		orgscale = 2;
		velscale = 10;
	}
	else
	{
		orgscale = 1;
		velscale = 5;
	}

	while (len > 0)
	{
		len -= dec;

		// drop less particles as it flies
		if ((rand()&1023) < old->trailcount)
		{
			if (!(p = CL_AllocParticle ()))
				return;
			p->accel[2] = 0;		// accel = 0

			if (flags & EF_GIB)
			{
				p->alpha = 1.0f;
				p->alphavel = -1.0f / (1+frand()*0.4f);
				p->color = 0xe8 + (rand()&7);
				for (j=0 ; j<3 ; j++)
				{
					p->org[j] = move[j] + crand()*orgscale;
					p->vel[j] = crand()*velscale;
				}
				p->vel[2] -= PARTICLE_GRAVITY;
			}
			else if (flags & EF_GREENGIB)
			{
				p->alpha = 1.0f;
				p->alphavel = -1.0f / (1+frand()*0.4f);
				p->color = 0xdb + (rand()&7);
				for (j=0; j< 3; j++)
				{
					p->org[j] = move[j] + crand()*orgscale;
					p->vel[j] = crand()*velscale;
				}
				p->vel[2] -= PARTICLE_GRAVITY;
			}
			else
			{
				p->alpha = 1.0f;
				p->alphavel = -1.0f / (1+frand()*0.2f);
				p->color = 4 + (rand()&7);
				for (j=0 ; j<3 ; j++)
				{
					p->org[j] = move[j] + crand()*orgscale;
					p->vel[j] = crand()*velscale;
				}
				p->accel[2] = PARTICLE_GRAVITY/2;
			}
		}

		old->trailcount -= 5;
		if (old->trailcount < 100)
			old->trailcount = 100;
		VectorAdd (move, vec, move);
	}
}


void CL_RailTrail (const CVec3 &start, const CVec3 &end)
{
	CVec3		move, vec;
	float		d, c, s;
	int			i, j;
	particle_t	*p;
	CVec3		right, up, dir;
	byte		clr = 0x74;

	move = start;
	VectorSubtract (end, start, vec);
	float len = vec.NormalizeFast ();

	MakeNormalVectors (vec, right, up);

	// spiral
	for (i = 0; i < len; i++)
	{
		if (!(p = CL_AllocParticle ()))
			return;
		p->accel[2] = 0;

		d = i * 0.1;
		c = cos(d);		//!!
		s = sin(d);		//!!

		VectorScale (right, c, dir);
		VectorMA (dir, s, up);

		p->alpha = 1.0;
		p->alphavel = -1.0 / (1+frand()*0.2);
		p->color = clr + (rand()&7);
		for (j = 0; j < 3; j++)
		{
			p->org[j] = move[j] + dir[j]*3;
			p->vel[j] = dir[j]*6;
		}

		VectorAdd (move, vec, move);
	}

	// rail core: simple trail
	static const particleEffect_t fx = {
		0, 16,							// color
		{-3, -3, -3,  3, 3, 3},			// bounds
		{-3, -3, -3,  3, 3, 3},			// velocity
		0.6, 0.8,						// fade time
		0,								// gravity
		0,								// width
		0,								// velWidth
		0.75,							// interval
		0								// density
	};
	MovableParticleEffect (start, end, fx);
}


void CL_RailTrailExt (const CVec3 &start, const CVec3 &end, byte rType, byte rColor)
{
	beam_t	*b;
	static const unsigned colorTable[8] = {
		RGB255(23, 83, 111), RGB(1,0,0), RGB(0,1,0), RGB(1,1,0),
		RGB(0,0,1), RGB(1,0,1), RGB(0,1,1), RGB(1,1,1)
	};

#	define I 255
#	define o 128
	static const unsigned colorTable2[8] = {
		RGB255(I, I, I), RGB255(I, o, o), RGB255(o, I, o), RGB255(I, I, o),
		RGB255(o, o, I), RGB255(I, o, I), RGB255(o, I, I), RGB255(I, I, I)
	};
#	undef I
#	undef o

	if (!rType || !NEW_FX || !railBeamShader || !railSpiralShader || !railRingsShader)
	{
		CL_RailTrail (start, end);
		return;
	}

	b = CL_AllocParticleBeam (start, end, 3, 0.6);
	if (!b) return;
	b->type       = BEAM_STAR;
	b->shader     = railBeamShader;
	b->color.rgba = colorTable2[rColor];
//	b->alpha      = 1.0f;
	b->growSpeed  = 6;

	switch (rType)
	{
	case 1:
		b = CL_AllocParticleBeam (start, end, 1, 0.8);
		if (!b) return;
		b->type       = BEAM_CYLINDER;
		b->shader     = railSpiralShader;
		b->color.rgba = colorTable[rColor];
		b->growSpeed  = 6;
		break;
	case 2:
		b = CL_AllocParticleBeam (start, end, 2, 0.8);
		if (!b) return;
		b->type       = BEAM_CYLINDER;
		b->shader     = railRingsShader;
		b->color.rgba = colorTable[rColor];
		b->alpha      = 0.5f;
		b->growSpeed  = 2;
		break;
	}
}


// XATRIX
/*
===============
CL_IonripperTrail
===============
*/
void CL_IonripperTrail (const CVec3 &start, const CVec3 &ent)
{
	CVec3	move, vec;
	particle_t *p;
	int     left = 0;

	move = start;
	VectorSubtract (ent, start, vec);
	float len = vec.NormalizeFast ();

	int dec = 5;
	vec.Scale (5);

	while (len > 0)
	{
		len -= dec;

		if (!(p = CL_AllocParticle ()))
			return;
		p->accel[2] = 0;

		p->alpha = 0.5;
		p->alphavel = -1.0 / (0.3 + frand() * 0.2);
		p->color = 0xE4 + (rand()&3);

		p->org = move;
		if (left)
		{
			left = 0;
			p->vel[0] = 10;
		}
		else
		{
			left = 1;
			p->vel[0] = -10;
		}

		p->vel[1] = 0;
		p->vel[2] = 0;

		VectorAdd (move, vec, move);
	}
}


/*
===============
CL_FlyParticles
===============
*/
static CVec3 avelocities[NUMVERTEXNORMALS];

static void InitAVelocities ()
{
	for (int i = 0; i < NUMVERTEXNORMALS*3; i++)
		avelocities[0][i] = (rand()&255) * 0.01f;
}


#define	BEAMLENGTH			16
void CL_FlyParticles (const CVec3 &origin, int count)
{
	float		angle;
	CVec3		forward;

	if (count > NUMVERTEXNORMALS)
		count = NUMVERTEXNORMALS;

	if (!avelocities[0][0])
		InitAVelocities ();

	float ltime = cl.ftime;
	for (int i = 0; i < count; i += 2)
	{
		angle = ltime * avelocities[i][0];
		float sy = sin(angle);
		float cy = cos(angle);
		angle = ltime * avelocities[i][1];
		float sp = sin(angle);
		float cp = cos(angle);
		angle = ltime * avelocities[i][2];
		float sr = sin(angle);
		float cr = cos(angle);

		forward[0] = cp*cy;
		forward[1] = cp*sy;
		forward[2] = -sp;

		particle_t *p;
		if (!(p = CL_AllocParticle ())) return;

		float dist = sin(ltime + i)*64;
		p->org[0] = origin[0] + bytedirs[i][0]*dist + forward[0]*BEAMLENGTH;
		p->org[1] = origin[1] + bytedirs[i][1]*dist + forward[1]*BEAMLENGTH;
		p->org[2] = origin[2] + bytedirs[i][2]*dist + forward[2]*BEAMLENGTH;

		p->vel.Zero();
		p->accel[2] = 0;

		p->color = 0;

		p->alpha = 1;
		p->alphavel = -100;
	}
}

void CL_FlyEffect (centity_t *ent, const CVec3 &origin)
{
	int		count, starttime;

	if (ent->fly_stoptime < cl.time)
	{
		starttime = cl.time;
		ent->fly_stoptime = cl.time + 60000;
	}
	else
	{
		starttime = ent->fly_stoptime - 60000;
	}

	int n = cl.time - starttime;
	if (n < 20000)
		count = n * 162 / 20000;
	else
	{
		n = ent->fly_stoptime - cl.time;
		if (n < 20000)
			count = n * 162 / 20000;
		else
			count = 162;
	}

	CL_FlyParticles (origin, count);
}


/*
===============
CL_BfgParticles
===============
*/

#define	BEAMLENGTH			16
void CL_BfgParticles (entity_t *ent)
{
	float	angle;
	CVec3	forward;
	CVec3	v;

	if (!avelocities[0][0])
		InitAVelocities ();

	float ltime = cl.ftime;
	for (int i=0 ; i<NUMVERTEXNORMALS ; i++)
	{
		angle = ltime * avelocities[i][0];
		float sy = sin(angle);
		float cy = cos(angle);
		angle = ltime * avelocities[i][1];
		float sp = sin(angle);
		float cp = cos(angle);
		angle = ltime * avelocities[i][2];
		float sr = sin(angle);
		float cr = cos(angle);

		forward[0] = cp*cy;
		forward[1] = cp*sy;
		forward[2] = -sp;

		particle_t *p;
		if (!(p = CL_AllocParticle ())) return;

		float dist = sin(ltime + i)*64;
		p->org[0] = ent->pos.origin[0] + bytedirs[i][0]*dist + forward[0]*BEAMLENGTH;
		p->org[1] = ent->pos.origin[1] + bytedirs[i][1]*dist + forward[1]*BEAMLENGTH;
		p->org[2] = ent->pos.origin[2] + bytedirs[i][2]*dist + forward[2]*BEAMLENGTH;

		p->vel.Zero();
		p->accel[2] = 0;

		VectorSubtract (p->org, ent->pos.origin, v);
		dist = VectorLength(v) / 90.0;
		p->color = appFloor (0xd0 + dist * 7);

		p->alpha = 1.0 - dist;
		p->alphavel = -100;
	}
}


/*
===============
CL_TrapParticles
===============
*/
// XATRIX
void CL_TrapParticles (entity_t *ent)
{
	// simple trail
	CVec3 start, end;
	start = end = ent->pos.origin;
	start[2] -= 14;
	end[2] += 64;

	static const particleEffect_t fx = {
		0xE0, 1,						// color
		{-1, -1, -1,  1, 1, 1},			// bounds
		{-15,-15,-15,  15,15,15},		// velocity
		0.3, 0.5,						// fade time
		PARTICLE_GRAVITY,				// gravity
		0,								// width
		0,								// velWidth
		5,								// interval
		0								// density
	};
	MovableParticleEffect (start, end, fx);

	// complex
	CVec3 org = ent->pos.origin;
	for (int i= -2; i <= 2; i += 4)
		for (int j = -2; j <= 2; j += 4)
			for (int k = -2; k <= 4; k += 4)
			{
				particle_t *p;
				if (!(p = CL_AllocParticle ())) return;

				p->color = 0xE0 + (rand()&3);

				p->alpha = 1.0;
				p->alphavel = -1.0 / (0.3 + (rand()&7) * 0.02);

				p->org[0] = org[0] + i + ((rand()&23) * crand());
				p->org[1] = org[1] + j + ((rand()&23) * crand());
				p->org[2] = org[2] + k + ((rand()&23) * crand());

				CVec3 dir;
				dir[0] = j * 8;
				dir[1] = i * 8;
				dir[2] = k * 8;

				dir.NormalizeFast ();
				float vel = 50 + rand()&63;
				VectorScale (dir, vel, p->vel);
			}
}


/*
===============
CL_TeleportParticles
===============
*/
void CL_TeleportParticles (const CVec3 &org)
{
	particle_t	*p;
	CVec3		dir;

	for (int i = -16; i <= 16; i += 4)
		for (int j = -16; j <= 16; j += 4)
			for (int k = -16; k <= 32; k += 4)
			{
				if (!(p = CL_AllocParticle ()))
					return;

				p->color = 7 + (rand()&7);

				p->alpha = 1.0;
				p->alphavel = -1.0 / (0.3 + (rand()&7) * 0.02);

				p->org[0] = org[0] + i + (rand()&3);
				p->org[1] = org[1] + j + (rand()&3);
				p->org[2] = org[2] + k + (rand()&3);

				dir[0] = j*8;
				dir[1] = i*8;
				dir[2] = k*8;

				dir.NormalizeFast ();
				float vel = 50 + (rand()&63);
				VectorScale (dir, vel, p->vel);
			}
}


/*
==============
CL_EntityEvent

An entity has just been parsed that has an event value

the female events are there for backwards compatability
==============
*/
extern sfx_t *cl_sfx_footsteps[4];
extern sfx_t *cl_sfx_footsteps2[9*4];
extern sfx_t *cl_sfx_fallshort2[9];
//?? extern sfx_t *cl_sfx_spectator[4];
extern sfx_t *cl_sfx_camper[9];


void CL_EntityEvent (clEntityState_t *ent)
{
	if (RE_GetCaps() & REF_CONSOLE_ONLY)
		return;

	if (ent->event >= EV_FOOTSTEP0 && ent->event < EV_FOOTSTEP0 + MATERIAL_COUNT)
	{
		if (cl_footsteps->integer)
			S_StartSound (NULL, ent->number, CHAN_BODY, cl_sfx_footsteps2[4*(ent->event-EV_FOOTSTEP0) + (rand()&3)],
				1, ATTN_NORM, 0);
		return;
	}
	else if (ent->event >= EV_FALLSHORT0 && ent->event < EV_FALLSHORT0 + MATERIAL_COUNT)
	{
		S_StartSound (NULL, ent->number, CHAN_AUTO, cl_sfx_fallshort2[ent->event-EV_FALLSHORT0], 1, ATTN_NORM, 0);
		return;
	}
/*??	else if (ent->event >= EV_SPECTATOR0 && ent->event <= EV_SPECTATOR3)
	{
		S_StartSound (NULL, ent->number, CHAN_AUTO, cl_sfx_spectator[ent->event-EV_SPECTATOR0], 1, ATTN_NORM, 0);
		return;
	} */
	else if (ent->event >= EV_CAMPER0 && ent->event < EV_CAMPER0+NUM_CAMPER_EVENTS)
	{
		S_StartSound (NULL, ent->number, CHAN_AUTO, cl_sfx_camper[ent->event-EV_CAMPER0], 1, ATTN_NORM, 0);
		return;
	}

	switch (ent->event)
	{
	case EV_ITEM_RESPAWN:
		S_StartSound (NULL, ent->number, CHAN_WEAPON, S_RegisterSound("items/respawn1.wav"), 1, ATTN_IDLE, 0);
		CL_ItemRespawnParticles (ent->origin);
		break;
	case EV_PLAYER_TELEPORT:
		S_StartSound (NULL, ent->number, CHAN_WEAPON, S_RegisterSound("misc/tele1.wav"), 1, ATTN_IDLE, 0);
		CL_TeleportParticles (ent->origin);
		break;
	case EV_FOOTSTEP:
		if (cl_footsteps->integer)
			S_StartSound (NULL, ent->number, CHAN_BODY, cl_sfx_footsteps[rand()&3], 1, ATTN_NORM, 0);
		break;
	case EV_FALLSHORT:
		S_StartSound (NULL, ent->number, CHAN_AUTO, S_RegisterSound ("player/land1.wav"), 1, ATTN_NORM, 0);
		break;
	case EV_FALL:
		S_StartSound (NULL, ent->number, CHAN_AUTO, S_RegisterSound ("*fall2.wav"), 1, ATTN_NORM, 0);
		break;
	case EV_FALLFAR:
		S_StartSound (NULL, ent->number, CHAN_AUTO, S_RegisterSound ("*fall1.wav"), 1, ATTN_NORM, 0);
		break;
	}
}


/*
==============
CL_ClearEffects

==============
*/
void CL_ClearEffects ()
{
	CL_ClearParticles ();
	CL_ClearDlights ();
}
