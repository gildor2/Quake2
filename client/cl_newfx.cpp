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
// cl_newfx.cpp -- MORE entity effects parsing and management

#include "client.h"

/*
======
vectoangles2 - this is duplicated in the game DLL, but I need it here.
======
*/
static void vectoangles2 (const CVec3 &value1, CVec3 angles)
{
	float	forward;
	float	yaw, pitch;

	if (value1[1] == 0 && value1[0] == 0)
	{
		yaw = 0;
		if (value1[2] > 0)
			pitch = 90;
		else
			pitch = 270;
	}
	else
	{
	// PMM - fixed to correct for pitch of 0
		if (value1[0])
			yaw = atan2 (value1[1], value1[0]) * 180 / M_PI;
		else if (value1[1] > 0)
			yaw = 90;
		else
			yaw = 270;

		if (yaw < 0)
			yaw += 360;

		forward = sqrt (value1[0]*value1[0] + value1[1]*value1[1]);
		pitch = atan2 (value1[2], forward) * 180 / M_PI;
		if (pitch < 0)
			pitch += 360;
	}

	angles[PITCH] = -pitch;
	angles[YAW] = yaw;
	angles[ROLL] = 0;
}

//=============
//=============
void CL_Flashlight (int ent, const CVec3 &pos)
{
	cdlight_t *dl = CL_AllocDlight (ent, pos);
	dl->radius = 400;
	dl->die = cl.time + 100;
	dl->color[0] = 1;
	dl->color[1] = 1;
	dl->color[2] = 1;
}

/*
======
CL_ColorFlash - flash of light
======
*/
// This function is used only once: for TRACKER_EXPLOSION
void CL_ColorFlash (const CVec3 &pos, int ent, int intensity, float r, float g, float b)
{
	cdlight_t *dl = CL_AllocDlight (ent, pos);
	dl->radius = intensity;
	dl->die = cl.time + 100;
	dl->color[0] = r;
	dl->color[1] = g;
	dl->color[2] = b;
}


/*
======
CL_DebugTrail
======
*/
void CL_DebugTrail (const CVec3 &start, const CVec3 &end)
{
	CVec3		move, vec;
//	int			j;
	particle_t	*p;
	CVec3		right, up;
//	int			i;
//	float		d, c, s;
//	CVec3		dir;

	move = start;
	VectorSubtract (end, start, vec);
	float len = vec.NormalizeFast ();

	MakeNormalVectors (vec, right, up);

//	vec.Scale (RT2_SKIP);

//	dec = 1.0;
//	dec = 0.75;
	float dec = 3;
	vec.Scale (dec);
	move = start;

	while (len > 0)
	{
		len -= dec;

		if (!(p = CL_AllocParticle ()))
			return;

		p->accel[2] = 0;
		p->vel.Zero();
		p->alpha = 1.0;
		p->alphavel = -0.1;
//		p->alphavel = 0;
		p->color = 0x74 + (rand()&7);
		p->org = move;
/*
		for (j=0 ; j<3 ; j++)
		{
			p->org[j] = move[j] + crand()*2;
			p->vel[j] = crand()*3;
			p->accel[j] = 0;
		}
*/
		VectorAdd (move, vec, move);
	}

}


void CL_ForceWall (const CVec3 &start, const CVec3 &end, int color)
{
	CVec3		move, vec;
	int			j;
	particle_t	*p;

	move = start;
	VectorSubtract (end, start, vec);
	float len = vec.NormalizeFast ();

	vec.Scale (4);

	// FIXME: this is a really silly way to have a loop
	while (len > 0)
	{
		len -= 4;

		if (frand() > 0.3)
		{
			if (!(p = CL_AllocParticle ()))
				return;
			p->accel[2] = 0;

			p->alpha = 1.0;
			p->alphavel =  -1.0 / (3.0+frand()*0.5);
			p->color = color;
			for (j=0 ; j<3 ; j++)
				p->org[j] = move[j] + crand()*3;
			p->vel[0] = 0;
			p->vel[1] = 0;
			p->vel[2] = -40 - (crand()*10);
		}

		VectorAdd (move, vec, move);
	}
}


/*
===============
CL_GenericParticleEffect
===============
*/
void CL_GenericParticleEffect (const CVec3 &org, const CVec3 &dir, int color, int count, int numcolors, int dirspread, float alphavel)
{
	int			i, j;
	particle_t	*p;
	float		d;

	for (i=0 ; i<count ; i++)
	{
		if (!(p = CL_AllocParticle ()))
			return;

		if (numcolors > 1)
			p->color = color + (rand() & numcolors);
		else
			p->color = color;

		d = rand() & dirspread;
		for (j=0 ; j<3 ; j++)
		{
			p->org[j] = org[j] + ((rand()&7)-4) + d*dir[j];
			p->vel[j] = crand()*20;
		}

//		p->accel = accel;
		p->alpha = 1.0;
		p->alphavel = -1.0 / (0.5 + frand()*alphavel);
//		p->alphavel = alphavel;
	}
}

/*
===============
CL_BubbleTrail2 (lets you control the # of bubbles by setting the distance between the spawns)

===============
*/
void CL_BubbleTrail2 (const CVec3 &start, const CVec3 &end, int dist)
{
	CVec3		move, vec;
	int			i, j;
	particle_t	*p;

	move = start;
	VectorSubtract (end, start, vec);
	int len = appFloor (vec.NormalizeFast ());

	vec.Scale (dist);

	for (i=0 ; i<len ; i+=dist)
	{
		if (!(p = CL_AllocParticle ()))
			return;
		p->accel[2] = 0;

		p->alpha = 1.0;
		p->alphavel = -1.0 / (1+frand()*0.1);
		p->color = 4 + (rand()&7);
		for (j=0 ; j<3 ; j++)
		{
			p->org[j] = move[j] + crand()*2;
			p->vel[j] = crand()*10;
		}
		p->org[2] -= 4;
//		p->vel[2] += 6;
		p->vel[2] += 20;

		VectorAdd (move, vec, move);
	}
}

//#define CORKSCREW		1
//#define DOUBLE_SCREW	1
#define	RINGS		1
//#define	SPRAY		1

#ifdef CORKSCREW
void CL_Heatbeam (const CVec3 &start, const CVec3 &end)
{
	CVec3		move, vec;
	int			j,k;
	particle_t	*p;
	CVec3		right, up;
	int			i;
	float		d, c, s;
	CVec3		dir;
	float		ltime;
	int			step = 5;

	move = start;
	VectorSubtract (end, start, vec);
	float len = vec.NormalizeFast ();

//	MakeNormalVectors (vec, right, up);
	right = cl.v_right;
	up = cl.v_up;
	VectorMA (move, -1, right);
	VectorMA (move, -1, up);

	vec.Scale (step);
	ltime = cl.ftime;

//	for (i=0 ; i<len ; i++)
	for (i=0 ; i<len ; i+=step)
	{
		d = i * 0.1 - fmod(ltime,16.0)*M_PI;
		c = cos(d)/1.75f;
		s = sin(d)/1.75f;
#ifdef DOUBLE_SCREW
		for (k=-1; k<2; k+=2)
		{
#else
		k=1;
#endif
			if (!(p = CL_AllocParticle ()))
				return;
			p->accel[2] = 0;

			p->alpha = 0.5f;
	//		p->alphavel = -1.0 / (1+frand()*0.2);
			// only last one frame!
			p->alphavel = INSTANT_PARTICLE;
	//		p->color = 0x74 + (rand()&7);
//			p->color = 223 - (rand()&7);
			p->color = 223;
//			p->color = 240;

			// trim it so it looks like it's starting at the origin
			if (i < 10)
			{
				VectorScale (right, c*(i/10.0f)*k, dir);
				VectorMA (dir, s*(i/10.0f)*k, up);
			}
			else
			{
				VectorScale (right, c*k, dir);
				VectorMA (dir, s*k, up);
			}

			for (j=0 ; j<3 ; j++)
			{
				p->org[j] = move[j] + dir[j]*3;
	//			p->vel[j] = dir[j]*6;
				p->vel[j] = 0;
			}
#ifdef DOUBLE_SCREW
		}
#endif
		VectorAdd (move, vec, move);
	}
}
#endif
#ifdef RINGS
void CL_Heatbeam (const CVec3 &start, const CVec3 &forward)
{
	CVec3		move, vec;
	int			j;
	particle_t	*p;
	CVec3		right, up;
	int			i;
	float		c, s;
	CVec3		dir;
	float		ltime;
	float		start_pt;
	float		rot;
	float		variance;
	CVec3		end;

	VectorMA (start, 4096, forward, end);

	move = start;
	VectorSubtract (end, start, vec);
	float len = vec.NormalizeFast ();

	// FIXME - pmm - these might end up using old values?
//	MakeNormalVectors (vec, right, up);
	right = cl.v_right;
	up = cl.v_up;
	VectorMA (move, -0.5f, right);
	VectorMA (move, -0.5f, up);
	// otherwise assume SOFT

	int step = 32;
	ltime = cl.ftime;
	start_pt = fmod(ltime*96.0f,step);
	VectorMA (move, start_pt, vec);

	vec.Scale (step);

//	Com_Printf ("%f\n", ltime);
	float rstep = M_PI/10.0f;
	for (i=appRound(start_pt) ; i<len ; i+=step)
	{
		if (i>step*5) // don't bother after the 5th ring
			break;

		for (rot = 0; rot < M_PI*2; rot += rstep)
		{
			if (!(p = CL_AllocParticle ()))
				return;
			p->accel[2] = 0;
//			rot+= fmod(ltime, 12.0)*M_PI;
//			c = cos(rot)/2.0;
//			s = sin(rot)/2.0;
//			variance = 0.4 + ((float)rand()/(float)RAND_MAX) *0.2;
			variance = 0.5f;
			c = cos(rot)*variance;
			s = sin(rot)*variance;

			// trim it so it looks like it's starting at the origin
			if (i < 10)
			{
				VectorScale (right, c*(i/10.0f), dir);
				VectorMA (dir, s*(i/10.0f), up);
			}
			else
			{
				VectorScale (right, c, dir);
				VectorMA (dir, s, up);
			}

			p->alpha = 0.5f;
	//		p->alphavel = -1.0 / (1+frand()*0.2);
			p->alphavel = -1000.0f;
	//		p->color = 0x74 + (rand()&7);
			p->color = 223 - (rand()&7);
			for (j=0 ; j<3 ; j++)
			{
				p->org[j] = move[j] + dir[j]*3;
	//			p->vel[j] = dir[j]*6;
				p->vel[j] = 0;
			}
		}
		VectorAdd (move, vec, move);
	}
}
#endif
#ifdef SPRAY
void CL_Heatbeam (const CVec3 &start, const CVec3 &end)
{
	CVec3		move, vec;
	particle_t	*p;
	CVec3		forward, right, up;
	int			i;
	float		d;
	CVec3		dir;
	float		ltime;
	float		step = 32.0, rstep;
	float		start_pt;
	float		rot;

	move = start;
	VectorSubtract (end, start, vec);
	float len = vec.NormalizeFast ();

//	MakeNormalVectors (vec, right, up);
	forward = cl.v_forward;
	right = cl.v_right;
	up = cl.v_up;
	VectorMA (move, -0.5, right);
	VectorMA (move, -0.5, up);

	for (i=0; i<8; i++)
	{
		if (!(p = CL_AllocParticle ()))
			return;
		p->accel[2] = 0;
		p->alpha = 1.0;
		p->alphavel = -5.0 / (1+frand());
		p->color = 223 - (rand()&7);

		p->org = move;
		d = crand()*M_PI;
		VectorScale (vec, 450, p->vel);
		VectorMA (p->vel, cos(d)*30, right);
		VectorMA (p->vel, sin(d)*30, up);
	}
}
#endif

/*
===============
CL_ParticleSteamEffect

Puffs with velocity along direction, with some randomness thrown in
===============
*/
void CL_ParticleSteamEffect (const CVec3 &org, const CVec3 &dir, int color, int count, int magnitude)
{
	int			i, j;
	particle_t	*p;
	float		d;
	CVec3		r, u;

	MakeNormalVectors (dir, r, u);

	for (i=0 ; i<count ; i++)
	{
		if (!(p = CL_AllocParticle ()))
			return;

		p->color = color + (rand()&7);

		for (j=0 ; j<3 ; j++)
		{
			p->org[j] = org[j] + magnitude*0.1*crand();
//			p->vel[j] = dir[j]*magnitude;
		}
		VectorScale (dir, magnitude, p->vel);
		d = crand()*magnitude/3;
		VectorMA (p->vel, d, r);
		d = crand()*magnitude/3;
		VectorMA (p->vel, d, u);

		p->accel[2] = -PARTICLE_GRAVITY/2;
		p->alpha = 1.0;
		p->alphavel = -1.0 / (0.5 + frand()*0.3);
	}
}

void CL_ParticleSteamEffect2 (cl_sustain_t *self)
{
	int			i, j;
	particle_t	*p;
	float		d;
	CVec3		r, u;
	CVec3		dir;

	dir = self->dir;
	MakeNormalVectors (dir, r, u);

	for (i=0 ; i<self->count ; i++)
	{
		if (!(p = CL_AllocParticle ()))
			return;

		p->color = self->color + (rand()&7);

		for (j=0 ; j<3 ; j++)
		{
			p->org[j] = self->org[j] + self->magnitude*0.1*crand();
//			p->vel[j] = dir[j]*magnitude;
		}
		VectorScale (dir, self->magnitude, p->vel);
		d = crand()*self->magnitude/3;
		VectorMA (p->vel, d, r);
		d = crand()*self->magnitude/3;
		VectorMA (p->vel, d, u);

		p->accel[2] = -PARTICLE_GRAVITY/2;
		p->alpha = 1.0;
		p->alphavel = -1.0 / (0.5 + frand()*0.3);
	}
	self->nextthink += self->thinkinterval;
}

/*
===============
CL_TrackerTrail
===============
*/
void CL_TrackerTrail (const CVec3 &start, const CVec3 &end, int particleColor)
{
	CVec3		move, vec;
	CVec3		forward,right,up;
	int			j;
	particle_t	*p;
	int			dec;
	float		dist;

	move = start;
	VectorSubtract (end, start, vec);
	float len = vec.NormalizeFast ();

	forward = vec;
#if 0
	CVec3		angle_dir;
	//?? VERY silly way to get "right" and "up" vectors
	//?? may check ref_gl tcGen environment for another sample
	vectoangles2 (forward, angle_dir);
	AngleVectors (angle_dir, &forward, &right, &up);
#else
	MakeNormalVectors (forward, right, up);
#endif

	dec = 3;
	vec.Scale (3);

	// FIXME: this is a really silly way to have a loop
	while (len > 0)
	{
		len -= dec;

		if (!(p = CL_AllocParticle ()))
			return;
		p->accel[2] = 0;

		p->alpha = 1.0;
		p->alphavel = -2.0;
		p->color = particleColor;
		dist = dot(move, forward);
		VectorMA(move, 8 * cos(dist), up, p->org);
		for (j=0 ; j<3 ; j++)
		{
//			p->org[j] = move[j] + crand();
			p->vel[j] = 0;
		}
		p->vel[2] = 5;

		VectorAdd (move, vec, move);
	}
}

void CL_Tracker_Shell(const CVec3 &origin)
{
	CVec3			dir;
	int				i;
	particle_t		*p;

	for(i=0;i<300;i++)
	{
		if (!(p = CL_AllocParticle ()))
			return;
		p->accel[2] = 0;

		p->alpha = 1.0;
		p->alphavel = INSTANT_PARTICLE;
		p->color = 0;

		dir[0] = crand();
		dir[1] = crand();
		dir[2] = crand();
		dir.NormalizeFast ();

		VectorMA(origin, 40, dir, p->org);
	}
}

void CL_MonsterPlasma_Shell(const CVec3 &origin)
{
	CVec3			dir;
	int				i;
	particle_t		*p;

	for(i=0;i<40;i++)
	{
		if (!(p = CL_AllocParticle ()))
			return;
		p->accel[2] = 0;

		p->alpha = 1.0;
		p->alphavel = INSTANT_PARTICLE;
		p->color = 0xe0;

		dir[0] = crand();
		dir[1] = crand();
		dir[2] = crand();
		dir.NormalizeFast ();

		VectorMA(origin, 10, dir, p->org);
//		VectorMA(origin, 10*(((rand () & 0x7fff) / ((float)0x7fff))), dir, p->org);
	}
}

void CL_Widowbeamout (cl_sustain_t *self)
{
	CVec3			dir;
	int				i;
	particle_t		*p;
	static int colortable[4] = {2*8,13*8,21*8,18*8};
	float			ratio;

	ratio = 1.0 - (((float)self->endtime - (float)cl.time)/2100.0);

	for(i=0;i<300;i++)
	{
		if (!(p = CL_AllocParticle ()))
			return;
		p->accel[2] = 0;

		p->alpha = 1.0;
		p->alphavel = INSTANT_PARTICLE;
		p->color = colortable[rand()&3];

		dir[0] = crand();
		dir[1] = crand();
		dir[2] = crand();
		dir.NormalizeFast ();

		VectorMA(self->org, (45.0 * ratio), dir, p->org);
//		VectorMA(origin, 10*(((rand () & 0x7fff) / ((float)0x7fff))), dir, p->org);
	}
}

void CL_Nukeblast (cl_sustain_t *self)
{
	CVec3			dir;
	int				i;
	particle_t		*p;
	static int colortable[4] = {110, 112, 114, 116};
	float			ratio;

	ratio = 1.0 - (((float)self->endtime - (float)cl.time)/1000.0);

	for(i=0;i<700;i++)
	{
		if (!(p = CL_AllocParticle ()))
			return;
		p->accel[2] = 0;

		p->alpha = 1.0;
		p->alphavel = INSTANT_PARTICLE;
		p->color = colortable[rand()&3];

		dir[0] = crand();
		dir[1] = crand();
		dir[2] = crand();
		dir.NormalizeFast ();

		VectorMA(self->org, (200.0 * ratio), dir, p->org);
//		VectorMA(origin, 10*(((rand () & 0x7fff) / ((float)0x7fff))), dir, p->org);
	}
}

void CL_WidowSplash (const CVec3 &org)
{
	static int colortable[4] = {2*8,13*8,21*8,18*8};
	int			i;
	particle_t	*p;
	CVec3		dir;

	for (i=0 ; i<256 ; i++)
	{
		if (!(p = CL_AllocParticle ()))
			return;

		p->color = colortable[rand()&3];

		dir[0] = crand();
		dir[1] = crand();
		dir[2] = crand();
		dir.NormalizeFast ();
		VectorMA(org, 45.0, dir, p->org);
		VectorScale(dir, 40.0, p->vel);

//??		p->accel[2] = (originally -- undefined) ??
		p->alpha = 1.0;
		p->alphavel = -0.8 / (0.5 + frand()*0.3);
	}

}

void CL_Tracker_Explode(const CVec3	&origin)
{
	CVec3			dir, backdir;
	int				i;
	particle_t		*p;

	for(i=0;i<300;i++)
	{
		if (!(p = CL_AllocParticle ()))
			return;
		p->accel[2] = 0;

		p->alpha = 1.0;
		p->alphavel = -1.0;
		p->color = 0;

		dir[0] = crand();
		dir[1] = crand();
		dir[2] = crand();
		dir.NormalizeFast ();
		VectorScale(dir, -1, backdir);

		VectorMA(origin, 64, dir, p->org);
		VectorScale(backdir, 64, p->vel);
	}

}

/*
===============
CL_TagTrail

===============
*/
void CL_TagTrail (const CVec3 &start, const CVec3 &end, int color)
{
	CVec3		move;
	CVec3		vec;
	int			j;
	particle_t	*p;

	move = start;
	VectorSubtract (end, start, vec);
	float len = vec.NormalizeFast ();

	int dec = 5;
	vec.Scale (5);

	while (len >= 0)
	{
		len -= dec;

		if (!(p = CL_AllocParticle ()))
			return;
		p->accel[2] = 0;

		p->alpha = 1.0f;
		p->alphavel = -1.0f / (0.8f+frand()*0.2f);
		p->color = color;
		for (j=0 ; j<3 ; j++)
		{
			p->org[j] = move[j] + crand()*16;
			p->vel[j] = crand()*5;
		}

		VectorAdd (move, vec, move);
	}
}

/*
===============
CL_ColorExplosionParticles
===============
*/
void CL_ColorExplosionParticles (const CVec3 &org, int color, int run)
{
	int			i, j;
	particle_t	*p;

	for (i=0 ; i<128 ; i++)
	{
		if (!(p = CL_AllocParticle ()))
			return;
		p->color = color + (rand() % run);

		for (j=0 ; j<3 ; j++)
		{
			p->org[j] = org[j] + ((rand()%32)-16);
			p->vel[j] = (rand()%256)-128;
		}

		p->alpha = 1.0;
		p->alphavel = -0.4 / (0.6 + frand()*0.2);
	}
}

/*
===============
CL_ParticleSmokeEffect - like the steam effect, but unaffected by gravity
===============
*/
void CL_ParticleSmokeEffect (const CVec3 &org, const CVec3 &dir, int color, int count, int magnitude)
{
	int			i, j;
	particle_t	*p;
	float		d;
	CVec3		r, u;

	MakeNormalVectors (dir, r, u);

	for (i=0 ; i<count ; i++)
	{
		if (!(p = CL_AllocParticle ()))
			return;
		p->accel[2] = 0;
		p->color = color + (rand()&7);

		for (j=0 ; j<3 ; j++)
		{
			p->org[j] = org[j] + magnitude*0.1*crand();
//			p->vel[j] = dir[j]*magnitude;
		}
		VectorScale (dir, magnitude, p->vel);
		d = crand()*magnitude/3;
		VectorMA (p->vel, d, r);
		d = crand()*magnitude/3;
		VectorMA (p->vel, d, u);

		p->alpha = 1.0;
		p->alphavel = -1.0 / (0.5 + frand()*0.3);
	}
}

/*
===============
CL_BlasterParticles2

Wall impact puffs (Green)
===============
*/
void CL_BlasterParticles2 (const CVec3 &org, const CVec3 &dir, unsigned int color)
{
	int			i, j;
	particle_t	*p;
	float		d;
	int			count;

	count = 40;
	for (i=0 ; i<count ; i++)
	{
		if (!(p = CL_AllocParticle ()))
			return;
		p->color = color + (rand()&7);

		d = rand()&15;
		for (j=0 ; j<3 ; j++)
		{
			p->org[j] = org[j] + ((rand()&7)-4) + d*dir[j];
			p->vel[j] = dir[j] * 30 + crand()*40;
		}

		p->alpha = 1.0;
		p->alphavel = -1.0 / (0.5 + frand()*0.3);
	}
}

/*
===============
CL_BlasterTrail2

Green!
===============
*/
void CL_BlasterTrail2 (const CVec3 &start, const CVec3 &end)
{
	CVec3		move, vec;
	int			j;
	particle_t	*p;
	int			dec;

	move = start;
	VectorSubtract (end, start, vec);
	float len = vec.NormalizeFast ();

	dec = 5;
	vec.Scale (5);

	// FIXME: this is a really silly way to have a loop
	while (len > 0)
	{
		len -= dec;

		if (!(p = CL_AllocParticle ()))
			return;
		p->accel[2] = 0;

		p->alpha = 1.0;
		p->alphavel = -1.0 / (0.3+frand()*0.2);
		p->color = 0xd0;
		for (j=0 ; j<3 ; j++)
		{
			p->org[j] = move[j] + crand();
			p->vel[j] = crand()*5;
		}

		VectorAdd (move, vec, move);
	}
}
