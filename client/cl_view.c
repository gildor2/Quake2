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
// cl_view.c -- player rendering positioning

#include "client.h"

//=============
//
// development tools for weapons
//
int			gun_frame;
struct model_s	*gun_model;

//=============

cvar_t		*crosshair;
cvar_t		*crosshaircolor;
cvar_t		*cl_testparticles;
cvar_t		*cl_testentities;
cvar_t		*cl_testlights;
cvar_t		*cl_testblend;

cvar_t		*cl_stats;

cvar_t		*r_playerpos;
cvar_t		*r_drawfps;
cvar_t		*r_surfinfo;


int			r_numdlights;
dlight_t	r_dlights[MAX_DLIGHTS];

int			r_numentities;
entity_t	r_entities[MAX_ENTITIES];

particle_t	r_particles[MAX_PARTICLES];

lightstyle_t	r_lightstyles[MAX_LIGHTSTYLES];

char cl_weaponmodels[MAX_CLIENTWEAPONMODELS][MAX_QPATH];
int num_cl_weaponmodels;

/*
====================
V_ClearScene

Specifies the model that will be used as the world
====================
*/
void V_ClearScene (void)
{
	r_numdlights = 0;
	r_numentities = 0;
}


/*
=====================
V_AddEntity

=====================
*/
void V_AddEntity (entity_t *ent)
{
	if (r_numentities >= MAX_ENTITIES)
		return;
	r_entities[r_numentities++] = *ent;
}


/*
=====================
V_AddLight

=====================
*/
void V_AddLight (vec3_t org, float intensity, float r, float g, float b)
{
	dlight_t	*dl;

	if (r_numdlights >= MAX_DLIGHTS)
		return;
	dl = &r_dlights[r_numdlights++];
	VectorCopy (org, dl->origin);
	dl->intensity = intensity;
	dl->color[0] = r;
	dl->color[1] = g;
	dl->color[2] = b;
}


/*
================
V_TestParticles

If cl_testparticles is set, create 4096 particles in the view
================
*/
void V_TestParticles (void)
{
	particle_t	*p;
	int			i, j;
	float		d, r, u;

	CL_ClearParticles ();

	for (i = 0; i < MAX_PARTICLES; i++)
	{
		d = i / 4.0f;
		r = 4 * ((i&7) - 3.5f);
		u = 4 * (((i>>3)&7) - 3.5f);

		if (!(p = CL_AllocParticle ()))
			break;
		p->accel[2] = 0;

		for (j = 0; j < 3; j++)
			p->org[j] = cl.refdef.vieworg[j] + cl.v_forward[j] * d + cl.v_right[j] * r + cl.v_up[j] * u;

		p->color = 8;
		p->alpha = cl_testparticles->value;
		p->alphavel = -1;	// fade out with 1 second after "cl_testparticles" switched off
	}
}

/*
================
V_TestEntities

If cl_testentities is set, create 32 player models
================
*/
void V_TestEntities (void)
{
	int			i, j;
	float		f, r;
	entity_t	*ent;

	r_numentities = 32;
	memset (r_entities, 0, sizeof(r_entities));

	for (i=0 ; i<r_numentities ; i++)
	{
		ent = &r_entities[i];

		r = 64 * ( (i%4) - 1.5 );
		f = 64 * (i/4) + 128;

		for (j=0 ; j<3 ; j++)
			ent->origin[j] = cl.refdef.vieworg[j] + cl.v_forward[j]*f +
			cl.v_right[j]*r;

		ent->model = cl.baseclientinfo.model;
		ent->skin = cl.baseclientinfo.skin;
	}
}

/*
================
V_TestLights

If cl_testlights is set, create 32 lights models
================
*/
void V_TestLights (void)
{
	int			i, j;
	float		f, r;
	dlight_t	*dl;

	r_numdlights = MAX_DLIGHTS;
	memset (r_dlights, 0, sizeof(r_dlights));

	for (i = 0, dl = r_dlights; i < MAX_DLIGHTS; i++, dl++)
	{
#if 0
		if (!(cl_testlights->integer & (1<<i)))
		{
			dl--;
			r_numdlights--;
			continue;
		}
#endif
		r = 64 * ((i % 4) - 1.5);
		f = 64 * (i / 4) + 128;

		for (j = 0; j < 3; j++)
			dl->origin[j] = cl.refdef.vieworg[j] + cl.v_forward[j] * f + cl.v_right[j] * r;
		dl->color[0] = ((i % 6) + 1) & 1;
		dl->color[1] = (((i % 6) + 1) & 2) >> 1;
		dl->color[2] = (((i % 6) + 1) & 4) >> 2;
		dl->intensity = 200;
	}
}

//===================================================================

/*
=================
CL_PrepRefresh

Call before entering a new level, or after changing dlls
=================
*/
void CL_PrepRefresh (void)
{
	char		mapname[32];
	int			i;
	char		name[MAX_QPATH];
	float		rotate;
	vec3_t		axis;

	if (!cl.configstrings[CS_MODELS+1][0])
		return;		// no map loaded

	CL_ClearTEnts ();		// temp entities linked to models, which are invalid after vid_restart

	// let the render dll load the map
	strcpy (mapname, cl.configstrings[CS_MODELS+1] + 5);	// skip "maps/"
	mapname[strlen(mapname)-4] = 0;		// cut off ".bsp"

	// register models, pics, and skins
	Com_Printf ("Map: %s\r", mapname);
	SCR_UpdateScreen ();
	re.BeginRegistration (mapname);

	// precache status bar pics
	Com_Printf ("pics\r");
	SCR_UpdateScreen ();
	SCR_TouchPics ();

	CL_RegisterTEntModels ();

	num_cl_weaponmodels = 1;
	strcpy (cl_weaponmodels[0], "weapon.md2");

	for (i = 1; i < MAX_MODELS && cl.configstrings[CS_MODELS+i][0]; i++)
	{
		strcpy (name, cl.configstrings[CS_MODELS+i]);
		name[37] = 0;				// never go beyond one line
		if (name[0] != '*')
			Com_Printf ("%s\r", name);
		SCR_UpdateScreen ();
		Sys_SendKeyEvents ();		// pump message loop
		if (name[0] == '#')
		{
			// special player weapon model
			if (num_cl_weaponmodels < MAX_CLIENTWEAPONMODELS)
			{
				strncpy(cl_weaponmodels[num_cl_weaponmodels], cl.configstrings[CS_MODELS+i]+1,
					sizeof(cl_weaponmodels[num_cl_weaponmodels]) - 1);
				num_cl_weaponmodels++;
			}
		}
		else
		{
			char	*mdl, *ext;

			mdl = cl.configstrings[CS_MODELS + i];
			ext = strrchr (name, '.');
			if (!ext || stricmp (ext, ".bsp"))
				cl.model_draw[i] = re.RegisterModel (cl.configstrings[CS_MODELS+i]);
			else
				cl.model_draw[i] = NULL;	// do not reload BSP file
			if (name[0] == '*')
				cl.model_clip[i] = CM_InlineModel (cl.configstrings[CS_MODELS+i]);
			else
				cl.model_clip[i] = NULL;
		}
	}

	Com_Printf ("images\r", i);
	SCR_UpdateScreen ();
	for (i = 1; i < MAX_IMAGES && cl.configstrings[CS_IMAGES+i][0]; i++)
	{
		cl.image_precache[i] = re.RegisterPic (cl.configstrings[CS_IMAGES+i]);
		Sys_SendKeyEvents ();	// pump message loop
	}

	for (i = 0; i < MAX_CLIENTS; i++)
	{
		if (!cl.configstrings[CS_PLAYERSKINS+i][0])
			continue;
		Com_Printf ("client %i\r", i);
		SCR_UpdateScreen ();
		Sys_SendKeyEvents ();	// pump message loop
		CL_ParseClientinfo (i);
	}

	CL_LoadClientinfo (&cl.baseclientinfo, "unnamed\\male/grunt");

	// set sky textures and speed
	Com_Printf ("sky\r", i);
	SCR_UpdateScreen ();
	rotate = atof (cl.configstrings[CS_SKYROTATE]);
	sscanf (cl.configstrings[CS_SKYAXIS], "%f %f %f", &axis[0], &axis[1], &axis[2]);
	re.SetSky (cl.configstrings[CS_SKY], rotate, axis);

	// the renderer can now free unneeded stuff
	re.EndRegistration ();

	// clear any lines of console text
	Con_ClearNotify ();

	SCR_UpdateScreen ();
	cl.refresh_prepped = true;
	cl.force_refdef = true;	// make sure we have a valid refdef

	// start the cd track
	CDAudio_Play (atoi(cl.configstrings[CS_CDTRACK]), true);
}

/*
====================
CalcFov
====================
*/
float CalcFov (float fov_x, float width, float height)
{
	float	a;
	float	x;

	if (fov_x < 1 || fov_x > 179)
		Com_Error (ERR_DROP, "Bad fov: %f", fov_x);

	x = width/tan(fov_x/360*M_PI);

	a = atan (height/x);

	a = a*360/M_PI;

	return a;
}

//============================================================================

// gun frame debugging functions
void V_Gun_Next_f (void)
{
	gun_frame++;
	Com_Printf ("frame %i\n", gun_frame);
}

void V_Gun_Prev_f (void)
{
	gun_frame--;
	if (gun_frame < 0)
		gun_frame = 0;
	Com_Printf ("frame %i\n", gun_frame);
}

void V_Gun_Model_f (void)
{
	if (Cmd_Argc() != 2)
	{
		gun_model = NULL;
		return;
	}
	gun_model = re.RegisterModel (va("models/%s/tris.md2", Cmd_Argv(1)));
}

//============================================================================


/*
=================
SCR_DrawCrosshair
=================
*/
void SCR_DrawCrosshair (void)
{
	if (!crosshair->integer || cl.refdef.rdflags & RDF_THIRD_PERSON)
		return;

	if (crosshair->modified)
	{
		crosshair->modified = false;
		SCR_TouchPics ();
	}

	if (!crosshair_pic[0])
		return;

	re_DrawPicColor (scr_vrect.x + ((scr_vrect.width - crosshair_width)>>1),
	  scr_vrect.y + ((scr_vrect.height - crosshair_height)>>1), crosshair_pic, crosshaircolor->integer);
}


typedef struct
{
	int		code;
	char	*name;
} flagInfo_t;

static void DrawFlag (int flag, flagInfo_t *info, int numFlags, char *prefix)
{
#if 0
	int		i;

	for (i = 0; i < numFlags; i++, info++)
		if (flag & info->code)
			re.DrawTextLeft (va("%s%s", prefix, info->name), 0.3, 0.6, 0.4);
#else
	char	buf[256];
	int		i;

	buf[0] = 0;
	for (i = 0; i < numFlags; i++, info++)
		if (flag & info->code)
		{
			if (buf[0])
				strncat (buf, va(" %s%s", prefix, info->name), sizeof(buf));
			else
				Com_sprintf (ARRAY_ARG(buf), "%s%s", prefix, info->name);
			if (strlen (buf) > 40)
			{
				re.DrawTextLeft (buf, 0.3, 0.6, 0.4);
				buf[0] = 0;
			}
			flag &= ~info->code;
		}
	if (buf[0])
		re.DrawTextLeft (buf, 0.3, 0.6, 0.4);
	if (flag)
		re.DrawTextLeft (va("%sUNK_%X", prefix, flag), 0.6, 0.3, 0.4);
#endif
}


static void DecodeContents (int i)
{
	static flagInfo_t contentsNames[] = {
#define T(name)		{CONTENTS_##name, #name}
		T(SOLID),	T(WINDOW),		T(AUX),		T(LAVA),	T(SLIME),		T(WATER),
		T(MIST),	T(ALPHA),		T(AREAPORTAL),	T(PLAYERCLIP),	T(MONSTERCLIP),
		T(CURRENT_0),	T(CURRENT_90),	T(CURRENT_180),	T(CURRENT_270),
		T(CURRENT_UP),	T(CURRENT_DOWN),T(ORIGIN),		T(MONSTER),
		T(DEADMONSTER),	T(DETAIL),		T(TRANSLUCENT),	T(LADDER)
#undef T
	};

//	re.DrawTextLeft ("Contents:", 0.4, 0.4, 0.6);
//	re.DrawTextLeft ("---------", 0.4, 0.4, 0.6);
	if (!i)
		re.DrawTextLeft ("CONTENTS_EMPTY", 0.3, 0.6, 0.4);
	else
		DrawFlag (i, ARRAY_ARG(contentsNames), "CONTENTS_");
}


static char *ModelName (int modelIndex)
{
	if (!modelIndex) return NULL;
	if (modelIndex < 255)
		return cl.configstrings[CS_MODELS + modelIndex];
	else
		return "(custom)";
}


static void DrawSurfInfo (void)
{
	vec3_t	start, end;
	trace_t	trace;
	static vec3_t zero = {0, 0, 0};
	csurface_t	*surf;
	vec3_t	norm;
	char	*s;

	static flagInfo_t surfNames[] = {
#define T(name)		{SURF_##name, #name}
		T(LIGHT), T(SLICK), T(SKY), T(WARP),
		T(TRANS33), T(TRANS66), T(FLOWING), T(NODRAW),
		// Kingpin flags
		T(ALPHA), T(DIFFUSE), T(SPECULAR),
		// new flags (temp ??)
		T(AUTOFLARE)
#undef T
	};

	static char *materialNames[MATERIAL_COUNT] = {
		"silent",
		"concrete",
		"fabric",
		"gravel",
		"metal",
		"metal_l",
		"snow",
		"tin",
		"tile",
		"wood",
		"water",
		"glass",
		"dirt"
	};

	VectorCopy (cl.refdef.vieworg, start);
	AngleVectors (cl.refdef.viewangles, end, NULL, NULL);
	VectorScale (end, 500, end);
	VectorAdd (start, end, end);

	CM_BoxTrace (&trace, start, end, zero, zero, 0, r_surfinfo->integer & 4 ? MASK_ALL : MASK_SHOT|MASK_WATER);
	if (!(r_surfinfo->integer & 2))
		CL_ClipMoveToEntities (&trace, start, zero, zero, end);

	if (trace.fraction < 1.0)
	{
		re.DrawTextLeft ("Surface info:", 0.4, 0.4, 0.6);
		re.DrawTextLeft ("-------------", 0.4, 0.4, 0.6);
		re.DrawTextLeft (va("Point: %g  %g  %g", VECTOR_ARG(trace.endpos)),
			0.2, 0.4, 1);
		surf = trace.surface;
		if (surf->rname[0])		// non-null surface
		{
			re.DrawTextLeft (va("Texture: %s", surf->rname), 0.2, 0.4, 1);
			VectorCopy (trace.plane.normal, norm);
			re.DrawTextLeft (va("Normal: %g  %g  %g", VECTOR_ARG(norm)), 0.2, 0.4, 1);
			if (surf->value)
				re.DrawTextLeft (va("Value: %d", surf->value), 0.2, 0.4, 1);
			DrawFlag (surf->flags, ARRAY_ARG(surfNames), "SURF_");
//#define SURF_KNOWN	(0xFF|SURF_ALPHA|SURF_DIFFUSE|SURF_SPECULAR|SURF_AUTOFLARE)
//			if (surf->flags & ~SURF_KNOWN) // unknown flags
//				re.DrawTextLeft (va("SURF_UNK_%X", surf->flags & ~SURF_KNOWN), 0.6, 0.3, 0.4);
			// material
			if (surf->material >= MATERIAL_FIRST && surf->material <= MATERIAL_LAST)
				s = materialNames[surf->material];
			else
				s = "?? bad ??";
			re.DrawTextLeft (va("Material: %s", s), 0.3, 0.6, 0.4);
		}
		DecodeContents (trace.contents);
		if (trace.ent)
		{
			entityState_t *ent;

			ent = (entityState_t*)trace.ent;
			re.DrawTextLeft ("Entity:", 0.4, 0.4, 0.6);
			re.DrawTextLeft ("-------", 0.4, 0.4, 0.6);
			re.DrawTextLeft (va("Origin: %g %g %g", VECTOR_ARG(ent->origin)), 0.4, 0.4, 0.6);
			re.DrawTextLeft (va("fx: %X  rfx: %X", ent->effects, ent->renderfx), 0.2, 0.4, 1);
			if (ent->modelindex)	re.DrawTextLeft (va("model: %s", ModelName (ent->modelindex)), 0.2, 0.4, 1);
			if (ent->modelindex2)	re.DrawTextLeft (va("model2: %s", ModelName (ent->modelindex2)), 0.2, 0.4, 1);
			if (ent->modelindex3)	re.DrawTextLeft (va("model3: %s", ModelName (ent->modelindex3)), 0.2, 0.4, 1);
			if (ent->modelindex4)	re.DrawTextLeft (va("model4: %s", ModelName (ent->modelindex4)), 0.2, 0.4, 1);

			CL_AddEntityBox (ent, 0xFF2020FF);
		}
		re.DrawTextLeft ("", 0, 0, 0);
	}
}


static void DrawOriginInfo (void)
{
	vec3_t view;
	vec3_t zero = {0, 0, 0};
	int i;

	re.DrawTextLeft ("Player position:", 0.4, 0.4, 0.6);
	re.DrawTextLeft ("----------------", 0.4, 0.4, 0.6);
	re.DrawTextLeft (va("Point: %.0f  %.0f  %.0f", VECTOR_ARG(cl.refdef.vieworg)), 0.2, 0.4, 0.1);

	AngleVectors (cl.refdef.viewangles, view, NULL, NULL);
	re.DrawTextLeft (va("View direction: %g  %g  %g", VECTOR_ARG(view)), 0.2, 0.4, 0.1);

	i = CM_PointLeafnum (cl.refdef.vieworg);
	re.DrawTextLeft (va("Leaf number: %d, cluster: %i, area: %i",
		i, CM_LeafCluster (i), CM_LeafArea (i)),
		0.1, 0.2, 1);
	DecodeContents (CM_PointContents (cl.refdef.vieworg, 0));
	re.DrawTextLeft ("", 0, 0, 0);	// empty line
}


extern int fileFromPak;

static void DrawFpsInfo (void)
{
	static int startSecTime, lastFrameTime, frames;
	static float avgFps, minFps, maxFps;
	int		delta, time;

	time = Sys_Milliseconds ();

	if (cls.key_dest != key_game || (time - lastFrameTime > 2000))
	{	// reinitialize counters
		startSecTime = lastFrameTime = time;
		frames = 0;
		maxFps = avgFps = 0;
		minFps = 9999;
		return;
	}

	// update min/max stats
	if (!fileFromPak)		// ignore frame if packfile was read
	{
		float	tmpFps;

		tmpFps = 1000.0f / (time - lastFrameTime);
		if (tmpFps > maxFps) maxFps = tmpFps;
		if (tmpFps < minFps) minFps = tmpFps;
	}
	else
		fileFromPak = 0;

	lastFrameTime = time;

	// update avg stats
	delta = time - startSecTime;
	frames++;
	if (delta >= 500)					// update 2 times per second
	{
		avgFps = frames * 1000.0f / delta;
		startSecTime = time;
		frames = 0;
	}

	// draw info
	re.DrawTextRight (va("FPS: %.2f  min: %.2f  max: %.2f", avgFps, minFps, maxFps), 0.8, 1, 0);
}


static void Screenshot_f (void)
{
	int		i, flags;
	static char filename[MAX_OSPATH], tmpName[MAX_OSPATH];

	filename[0] = 0;
	flags = 0;

	if (Cmd_Argc() == 2 && !strcmp(Cmd_Argv(1), "/?"))
	{
		Com_Printf ("Usage: screenshot [-levelshot] [-no2d] [-nogamma] [-silent] [-jpeg] [<filename>]\n");
		return;
	}

	for (i = 1; i < Cmd_Argc(); i++)
	{
		char	*opt;

		opt = Cmd_Argv(i);
		if (opt[0] == '-')
		{
			opt++;
			if (!stricmp (opt, "levelshot"))
			{
				char	*tmp;

				if (cls.state != ca_active)
				{
					Com_WPrintf ("No levelshots in disconnected state\n");
					return;
				}

				if (!(tmp = strrchr (map_name, '/')))
				{
					Com_WPrintf ("Invalid map_name: %s\n", map_name);
					return;
				}
				tmp++;	// skip '/'

				flags |= SHOT_SMALL|SHOT_NO_2D|SHOT_NOGAMMA;
				Com_sprintf (ARRAY_ARG(filename), "%s/levelshots/%s", FS_Gamedir (), tmp);
				// cut extension
				tmp = strrchr (filename, '.');
				if (tmp) *tmp = 0;
			}
			else if (!stricmp (opt, "no2d"))
				flags |= SHOT_NO_2D;
			else if (!stricmp (opt, "nogamma"))
				flags |= SHOT_NOGAMMA;
			else if (!stricmp (opt, "silent"))
				flags |= SHOT_SILENT;
			else if (!stricmp (opt, "jpeg"))
				flags |= SHOT_JPEG;
			else
			{
				Com_WPrintf ("Unknown option: %s\n", opt);
				return;
			}
		}
		else
		{
			if (filename[0])
				Com_WPrintf ("WARNING: name already specified (%s). Changed.\n", filename);
			Com_sprintf (ARRAY_ARG(tmpName), "%s/screenshots/%s", FS_Gamedir (), opt);
			Q_CopyFilename (filename, tmpName, sizeof(filename));
		}
	}

	re.Screenshot (flags, filename);
}


#define MIN_WATER_DISTANCE	4.5

static void FixWaterVis (void)
{
	vec3_t		p;			// point
	int			cont;		// its contents
	qboolean	w, w1;		// "in water" flag
	trace_t		trace;
	static vec3_t zero = {0, 0, 0};

	if (cl.refdef.rdflags & RDF_THIRD_PERSON)
		w1 = CM_PointContents (cl.refdef.vieworg, 0) & MASK_WATER;	// need to recompute UNDERWATER flag
	else
		w1 = (cl.refdef.rdflags & RDF_UNDERWATER) != 0;
	p[0] = cl.refdef.vieworg[0];
	p[1] = cl.refdef.vieworg[1];

	// check point below
	p[2] = cl.refdef.vieworg[2] - MIN_WATER_DISTANCE;
	cont = CM_PointContents (p, 0);
	w = (cont & MASK_WATER) != 0;

	if (w1 == w || cont & MASK_SOLID)
	{
		// check point above
		p[2] += 2 * MIN_WATER_DISTANCE;
		cont = CM_PointContents (p, 0);
		w = (cont & MASK_WATER) != 0;
	}

	// do not require to fix position
	if (w1 == w || cont & MASK_SOLID)
		return;

	//?? improve by disabling this trick for correct maps

	// trace from air to water
	if (!w1)
		CM_BoxTrace (&trace, cl.refdef.vieworg, p, zero, zero, 0, MASK_WATER);
	else
		CM_BoxTrace (&trace, p, cl.refdef.vieworg, zero, zero, 0, MASK_WATER);
	if (trace.fraction < 1.0f && !(trace.surface->flags & (SURF_TRANS33|SURF_TRANS66)))
	{
//		re.DrawTextLeft("WATER FIX!", 1, 1, 1);
		if (cl.refdef.vieworg[2] < trace.endpos[2])
			cl.refdef.vieworg[2] = trace.endpos[2] - MIN_WATER_DISTANCE;
		else
			cl.refdef.vieworg[2] = trace.endpos[2] + MIN_WATER_DISTANCE;
	}
}


/*
==================
V_RenderView

==================
*/
static int entitycmpfnc (const entity_t *a, const entity_t *b)
{
	if (a->model == b->model)
		return ((int) a->skin - (int) b->skin);
	else
		return ((int) a->model - (int) b->model);
}


void V_RenderView (float stereo_separation)
{
	if (cls.state != ca_active)
		return;

	if (!cl.refresh_prepped)
		return;			// still loading

	if (timedemo->integer)
	{
		int		time, timeDelta;
		static int lastTime;

		time = Sys_Milliseconds ();
		if (!cl.timedemoStart)
		{
			cl.timedemoStart = time;
//			cl.timedemoLongestFrame = 0;	-- cleared anyway within a new server map
//			cl.timedemoFrames = 0;
		}
		else
		{
			timeDelta = time - lastTime;
			if (timeDelta > cl.timedemoLongestFrame && !fileFromPak)
				cl.timedemoLongestFrame = timeDelta;
		}
		lastTime = time;
		cl.timedemoFrames++;
	}

	// an invalid frame will just use the exact previous refdef
	// we can't use the old frame if the video mode has changed, though...
	if (cl.frame.valid && (cl.force_refdef || !cl_paused->integer))
	{
		cl.force_refdef = false;

		V_ClearScene ();

		cl.refdef.rdflags = cl.frame.playerstate.rdflags;

		// build a refresh entity list and calc cl.sim*
		// this also calls CL_CalcViewValues which loads
		// v_forward, etc.
		CL_AddEntities ();

		if (cl_testparticles->value)		// float value
			V_TestParticles ();
		CL_UpdateParticles ();

		if (cl_testentities->integer)	V_TestEntities ();
		if (cl_testlights->integer)		V_TestLights ();
		if (cl_testblend->integer)
		{
			cl.refdef.blend[0] = 1;
			cl.refdef.blend[1] = 0.5;
			cl.refdef.blend[2] = 0.25;
			cl.refdef.blend[3] = 0.5;
		}

		// offset vieworg appropriately if we're doing stereo separation
		if (stereo_separation)
		{
			vec3_t tmp;

			VectorScale (cl.v_right, stereo_separation, tmp);
			VectorAdd (cl.refdef.vieworg, tmp, cl.refdef.vieworg);
		}

		// debug output
		if (r_playerpos->integer)	DrawOriginInfo ();
		if (r_surfinfo->integer)	DrawSurfInfo ();

		// never let it sit exactly on a node line, because a water plane can
		// dissapear when viewed with the eye exactly on it.
		// the server protocol only specifies to 1/8 pixel, so add 1/16 in each axis
		cl.refdef.vieworg[0] += 1.0f/16;
		cl.refdef.vieworg[1] += 1.0f/16;
		cl.refdef.vieworg[2] += 1.0f/16;

		cl.refdef.x = scr_vrect.x;
		cl.refdef.y = scr_vrect.y;
		cl.refdef.width = scr_vrect.width;
		cl.refdef.height = scr_vrect.height;
		cl.refdef.fov_y = CalcFov (cl.refdef.fov_x, cl.refdef.width, cl.refdef.height);
		cl.refdef.time = cl.ftime;

		cl.refdef.areabits = cl.frame.areabits;

		if (!cl_add_entities->integer)	r_numentities = 0;
		if (!cl_add_lights->integer)	r_numdlights = 0;
		if (!cl_add_blend->integer || cl.refdef.rdflags & RDF_THIRD_PERSON)
			memset (cl.refdef.blend, 0, sizeof(cl.refdef.blend));

		cl.refdef.num_entities = r_numentities;
		cl.refdef.entities = r_entities;
		cl.refdef.particles = cl_add_particles->integer ? active_particles : NULL;
		cl.refdef.num_dlights = r_numdlights;
		cl.refdef.dlights = r_dlights;
		cl.refdef.lightstyles = cl_lightstyles;

		// underwater fov warp (taken from Q3 game source)
		if (cl.refdef.rdflags & RDF_UNDERWATER)
		{
			float	v;

			v = sin (cl.refdef.time * 0.4 * M_PI * 2);
			cl.refdef.fov_x += v;
			cl.refdef.fov_y -= v;
		}
		FixWaterVis ();

		// sort entities for better cache locality
		qsort (cl.refdef.entities, cl.refdef.num_entities, sizeof(cl.refdef.entities[0]),
			(int (*)(const void *, const void *))entitycmpfnc);
	}

	re.RenderFrame (&cl.refdef);

	// stats
	if (r_drawfps->integer)		DrawFpsInfo ();

	if (cl_stats->integer)
	{
		re.DrawTextLeft (va("ent:%d  lt:%d",	//?? particle stats (remove)
			r_numentities, r_numdlights),
			1, 1, 1);
	}

	if (log_stats->integer && (log_stats_file != 0))
		fprintf (log_stats_file, "%d,%d,", r_numentities, r_numdlights);	//?? particle stats (remove)

	SCR_DrawCrosshair ();
}


/*
=============
V_Init
=============
*/
void V_Init (void)
{
CVAR_BEGIN(vars)
	CVAR_VAR(crosshair, 0, CVAR_ARCHIVE),
	CVAR_VAR(crosshaircolor, 7, CVAR_ARCHIVE),

	CVAR_VAR(cl_testblend, 0, 0),
	CVAR_VAR(cl_testparticles, 0, 0),
	CVAR_VAR(cl_testentities, 0, 0),
	CVAR_VAR(cl_testlights, 0, CVAR_CHEAT),

	CVAR_VAR(cl_stats, 0, 0),

	CVAR_VAR(r_drawfps, 0, 0),
	CVAR_VAR(r_playerpos, 0, CVAR_CHEAT),
	CVAR_VAR(r_surfinfo, 0, CVAR_CHEAT)
CVAR_END

	Cvar_GetVars (ARRAY_ARG(vars));

	Cmd_AddCommand ("gun_next", V_Gun_Next_f);
	Cmd_AddCommand ("gun_prev", V_Gun_Prev_f);
	Cmd_AddCommand ("gun_model", V_Gun_Model_f);
	Cmd_AddCommand ("screenshot", Screenshot_f);
}
