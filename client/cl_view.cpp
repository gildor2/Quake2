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
// cl_view.cpp -- player rendering positioning

//?? combine with cl_tent.cpp and put as cl_scene.cpp ?

#include "client.h"

#ifdef GUN_DEBUG
// development tools for weapons
int		gun_frame;
model_t	*gun_model;
#endif

static cvar_t	*cl_testparticles;
static cvar_t	*cl_testentities;
static cvar_t	*cl_testlights;
static cvar_t	*cl_testblend;

static cvar_t	*r_playerpos;
static cvar_t	*r_drawfps;
static cvar_t	*r_surfinfo;

cvar_t			*scr_viewsize;		// non-static for menu

static int		r_numdlights;
static dlight_t	r_dlights[MAX_DLIGHTS];

static int		r_numentities;
static entity_t	r_entities[MAX_ENTITIES];

char cl_weaponmodels[MAX_CLIENTWEAPONMODELS][MAX_QPATH];
int num_cl_weaponmodels;

/*
====================
V_ClearScene

Specifies the model that will be used as the world
====================
*/
static void V_ClearScene (void)
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
//	if (!ent->model) re.DrawTextLeft (va("NULL model: %g %g %g from %X", VECTOR_ARG(ent->origin), GET_RETADDR(ent)), RGB(1,1,1));
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

		for (j = 0; j < 3; j++)
			ent->origin[j] = cl.refdef.vieworg[j] + cl.v_forward[j]*f + cl.v_right[j]*r;

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
//?? try to make with tables/automaton, call SCR_UpdateScreen() and check "disconnect" every step, + get keyboard events
void CL_PrepRefresh (void)
{
	char		mapname[32];
	int			i;
	char		name[MAX_QPATH];
	float		rotate;
	vec3_t		axis;
	static int start_time = 0;
	static int servercount = 0;

	if (!cl.configstrings[CS_MODELS+1][0])
		return;				// no map loaded

	SCR_SetLevelshot ();

	// wait a small time to let server complete initialization
	// allow map to be changed before initializing renderer, when loading savegames,
	// saved at point of changing map; this situation is possible because:
	//   - server frame#1:
	//      - loading savegame, will create configstrings + may be insert command string "gamemap <newmap>"
	//      - send configstrings#1
	//   - server frame#2:
	//      - exec "gamemap" command from previous frame, change map etc.
	//      - send configstrings#2
	// in some circumstances, client will receive configstrings#1 after server frame#2 - this will load
	// client map#1 after loading server map#2 (out-of-sync client/server maps -- fatal error)
	if (servercount != cl.servercount)
	{
		servercount = cl.servercount;
		start_time = cls.realtime;
		return;
	}
	if (cls.realtime < start_time + 300)
		return;

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
	strcpy (cl_weaponmodels[0], "weapon.md2");	//??

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
	Com_Printf (" \r");			// clear console notification about loading process; require to send space char !

	Con_ClearNotify ();

	SCR_UpdateScreen ();
	SCR_EndLoadingPlaque (true);
	cl.refresh_prepped = true;
	cl.force_refdef = true;	// make sure we have a valid refdef

	// start the cd track
	CDAudio_Play (atoi(cl.configstrings[CS_CDTRACK]), true);
}


/*-----------------------------------------------------------------------------
	Support for reducing rendering viewport
-----------------------------------------------------------------------------*/

static struct
{
	int		x, y, width, height;
} scr_vrect;					// position of render window (changed when scr_viewsize cvar modified)
//?? rename to v_vrect ?

static void CalcVrect (void)
{
	float frac = scr_viewsize->Clamp (40, 100) / 100.0f;

	scr_vrect.width = appRound (viddef.width * frac);
	scr_vrect.height = appRound (viddef.height * frac);

	scr_vrect.width &= ~7;		// align(8)
	scr_vrect.height &= ~7;		// align(8)

	scr_vrect.x = (viddef.width - scr_vrect.width) / 2;
	scr_vrect.y = (viddef.height - scr_vrect.height) / 2;
}


static void TileClear (void)
{
	const char *tileName = "backtile";	//?? try "/*detail" with a different color
	if (con_height == viddef.height || scr_viewsize->integer == 100) return;

	int y1 = scr_vrect.y;
	int y2 = y1 + scr_vrect.height;
	re.DrawTileClear (0, 0, viddef.width, y1, tileName);
	re.DrawTileClear (0, y2, viddef.width, viddef.height - y2, tileName);
	int x1 = scr_vrect.x;
	int x2 = x1 + scr_vrect.width;
	re.DrawTileClear (0, y1, x1, scr_vrect.height, tileName);
	re.DrawTileClear (x2, y1, viddef.width - x2, scr_vrect.height, tileName);

	return;
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
		Com_DropError ("Bad fov: %f", fov_x);

	x = width/tan(fov_x/360*M_PI);

	a = atan (height/x);

	a = a*360/M_PI;

	return a;
}

//============================================================================

#ifdef GUN_DEBUG

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

void V_Gun_Model_f (int argc, char **argv)
{
	if (argc != 2)
	{
		gun_model = NULL;
		return;
	}
	gun_model = re.RegisterModel (va("models/%s/tris.md2", argv[1]));
}

#endif

/*
=================
SCR_Sky_f

Set a specific sky and rotation speed
=================
*/
static void Sky_f (bool usage, int argc, char **argv)
{
	float	rotate;
	vec3_t	axis;

	if (argc < 2 || usage)
	{
		Com_Printf ("Usage: sky <basename> <rotate> <axis x y z>\n");
		return;
	}
	if (argc > 2)
		rotate = atof (argv[2]);
	else
		rotate = 0;
	if (argc == 6)
	{
		axis[0] = atof (argv[3]);
		axis[1] = atof (argv[4]);
		axis[2] = atof (argv[5]);
	}
	else
	{
		axis[0] = 0;
		axis[1] = 0;
		axis[2] = 1;
	}

	re.SetSky (argv[1], rotate, axis);
}

//============================================================================

typedef struct
{
	int		code;
	char	*name;
} flagInfo_t;

static void DrawFlag (int flag, const flagInfo_t *info, int numFlags, char *prefix)
{
#if 0
	int		i;

	for (i = 0; i < numFlags; i++, info++)
		if (flag & info->code)
			re.DrawTextLeft (va("%s%s", prefix, info->name), RGB(0.3, 0.6, 0.4));
#else
	char	buf[256];

	buf[0] = 0;
	for (int i = 0; i < numFlags; i++, info++)
		if (flag & info->code)
		{
			if (buf[0])
				appStrcatn (ARRAY_ARG(buf), va(" %s%s", prefix, info->name));
			else
				appSprintf (ARRAY_ARG(buf), "%s%s", prefix, info->name);
			if (strlen (buf) > 40)
			{
				re.DrawTextLeft (buf, RGB(0.3, 0.6, 0.4));
				buf[0] = 0;
			}
			flag &= ~info->code;
		}
	if (buf[0])
		re.DrawTextLeft (buf, RGB(0.3, 0.6, 0.4));
	if (flag)
		re.DrawTextLeft (va("%sUNK_%X", prefix, flag), RGB(0.6, 0.3, 0.4));
#endif
}


static void DecodeContents (int i)
{
	static const flagInfo_t contentsNames[] = {
#define T(name)		{CONTENTS_##name, #name}
		T(SOLID),	T(WINDOW),		T(AUX),		T(LAVA),	T(SLIME),		T(WATER),
		T(MIST),	T(ALPHA),		T(AREAPORTAL),	T(PLAYERCLIP),	T(MONSTERCLIP),
		T(CURRENT_0),	T(CURRENT_90),	T(CURRENT_180),	T(CURRENT_270),
		T(CURRENT_UP),	T(CURRENT_DOWN),T(ORIGIN),		T(MONSTER),
		T(DEADMONSTER),	T(DETAIL),		T(TRANSLUCENT),	T(LADDER)
#undef T
	};

//	re.DrawTextLeft ("Contents:", RGB(0.4, 0.4, 0.6));
//	re.DrawTextLeft ("---------", RGB(0.4, 0.4, 0.6));
	if (!i)
		re.DrawTextLeft ("CONTENTS_EMPTY", RGB(0.3, 0.6, 0.4));
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
	const char *s;
	int		cont;

	static const flagInfo_t surfNames[] = {
#define T(name)		{SURF_##name, #name}
		T(LIGHT), T(SLICK), T(SKY), T(WARP),
		T(TRANS33), T(TRANS66), T(FLOWING), T(NODRAW),
		// Kingpin flags
		T(ALPHA), T(DIFFUSE), T(SPECULAR),
		// new flags (temp ??)
		T(AUTOFLARE)
#undef T
	};

	static const char *materialNames[MATERIAL_COUNT] = {
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

	cont = r_surfinfo->integer & 4 ? MASK_ALL : MASK_SHOT|MASK_WATER;
	CM_BoxTrace (&trace, start, end, zero, zero, 0, cont);
	if (!(r_surfinfo->integer & 2))
		CL_EntityTrace (&trace, start, end, zero, zero, cont);

	if (trace.fraction < 1.0)
	{
		re.DrawTextLeft ("Surface info:", RGB(0.4, 0.4, 0.6));
		re.DrawTextLeft ("-------------", RGB(0.4, 0.4, 0.6));
		re.DrawTextLeft (va("Point: %g  %g  %g", VECTOR_ARG(trace.endpos)),
			RGBA(0.2, 0.4, 1, 1));
		surf = trace.surface;
		if (surf->rname[0])		// non-null surface
		{
			re.DrawTextLeft (va("Texture: %s", surf->rname), RGB(0.2, 0.4, 1));
			VectorCopy (trace.plane.normal, norm);
			re.DrawTextLeft (va("Normal: %g  %g  %g", VECTOR_ARG(norm)), RGB(0.2, 0.4, 1));
			if (surf->value)
				re.DrawTextLeft (va("Value: %d", surf->value), RGB(0.2, 0.4, 1));
			DrawFlag (surf->flags, ARRAY_ARG(surfNames), "SURF_");
//#define SURF_KNOWN	(0xFF|SURF_ALPHA|SURF_DIFFUSE|SURF_SPECULAR|SURF_AUTOFLARE)
//			if (surf->flags & ~SURF_KNOWN) // unknown flags
//				re.DrawTextLeft (va("SURF_UNK_%X", surf->flags & ~SURF_KNOWN), RGB(0.6, 0.3, 0.4));
			// material
			if (surf->material >= MATERIAL_FIRST && surf->material <= MATERIAL_LAST)
				s = materialNames[surf->material];
			else
				s = "?? bad ??";
			re.DrawTextLeft (va("Material: %s", s), RGB(0.3, 0.6, 0.4));
		}
		DecodeContents (trace.contents);
		if (trace.ent)
		{
			entityState_t *ent;

			ent = (entityState_t*)trace.ent;
			re.DrawTextLeft ("Entity:", RGB(0.4, 0.4, 0.6));
			re.DrawTextLeft ("-------", RGB(0.4, 0.4, 0.6));
			re.DrawTextLeft (va("Origin: %g %g %g", VECTOR_ARG(ent->origin)), RGB(0.4, 0.4, 0.6));
			re.DrawTextLeft (va("fx: %X  rfx: %X", ent->effects, ent->renderfx), RGB(0.2, 0.4, 1));
			if (ent->modelindex)	re.DrawTextLeft (va("model: %s", ModelName (ent->modelindex)), RGB(0.2, 0.4, 1));
			if (ent->modelindex2)	re.DrawTextLeft (va("model2: %s", ModelName (ent->modelindex2)), RGB(0.2, 0.4, 1));
			if (ent->modelindex3)	re.DrawTextLeft (va("model3: %s", ModelName (ent->modelindex3)), RGB(0.2, 0.4, 1));
			if (ent->modelindex4)	re.DrawTextLeft (va("model4: %s", ModelName (ent->modelindex4)), RGB(0.2, 0.4, 1));

			CL_AddEntityBox (ent, 0xFF2020FF);
		}
		re.DrawTextLeft ("", RGB(0, 0, 0));
	}
}

static void DrawOriginInfo (void)
{
	vec3_t view;
	vec3_t zero = {0, 0, 0};
	int i;

	re.DrawTextLeft ("Player position:", RGB(0.4, 0.4, 0.6));
	re.DrawTextLeft ("----------------", RGB(0.4, 0.4, 0.6));
	re.DrawTextLeft (va("Point: %.0f  %.0f  %.0f", VECTOR_ARG(cl.refdef.vieworg)), RGB(0.2, 0.4, 0.1));

	AngleVectors (cl.refdef.viewangles, view, NULL, NULL);
	re.DrawTextLeft (va("View direction: %g  %g  %g", VECTOR_ARG(view)), RGB(0.2, 0.4, 0.1));

	i = CM_PointLeafnum (cl.refdef.vieworg);
	re.DrawTextLeft (va("Leaf number: %d, cluster: %i, area: %i",
		i, CM_LeafCluster (i), CM_LeafArea (i)),
		RGB(0.1, 0.2, 1));
	DecodeContents (CM_PointContents (cl.refdef.vieworg, 0));
	re.DrawTextLeft ("", RGB(0, 0, 0));	// empty line
}


extern int fileFromPak;

//#define SMOOTH_FPS_COUNTER

static void DrawFpsInfo (void)
{
	static float startSecTime, lastFrameTime;
	static float avgFps, minFps, maxFps;
	static int frames;

	float time = appMillisecondsf ();

	if (cls.key_dest != key_game || (time - lastFrameTime > 2000))
	{	// reinitialize counters
		startSecTime = lastFrameTime = time;
		frames = 0;
		maxFps = avgFps = 0;
		minFps = BIG_NUMBER;
		return;
	}

	// update min/max stats
	if (!fileFromPak)					// ignore frame if packfile was read
	{
#ifdef SMOOTH_FPS_COUNTER
		static float frameTime[5];
		static float timeSum = 0;
		static int frameIdx = 0;
		float frmTime = time - lastFrameTime;
		// smooth tmpFps
		if (minFps > maxFps)
		{
			for (int i = 0; i < ARRAY_COUNT(frameTime); i++) frameTime[i] = frmTime;
			timeSum = ARRAY_COUNT(frameTime) * frmTime;
		}
		else
		{
			timeSum = timeSum - frameTime[frameIdx] + frmTime;
			frameTime[frameIdx] = frmTime;
			if (++frameIdx >= ARRAY_COUNT(frameTime)) frameIdx = 0;
		}
		float tmpFps = timeSum / ARRAY_COUNT(frameTime);
		if (tmpFps > 0)
		{
			tmpFps = 1000.0f / tmpFps;
			EXPAND_BOUNDS(tmpFps, minFps, maxFps);
		}
#else
		// avoid zero-divide
		float tmpFps = (time == lastFrameTime) ? 1000 : 1000.0f / (time - lastFrameTime);
		//?? may be, draw FPS graph on screen (to detect min/max FPS spikes reason)
		EXPAND_BOUNDS(tmpFps, minFps, maxFps);
#endif
	}
	else
		fileFromPak = 0;

	lastFrameTime = time;

	// update avg stats
	float delta = time - startSecTime;
	frames++;
	if (delta >= 500)					// update 2 times per second
	{
		avgFps = frames * 1000.0f / delta;
		startSecTime = time;
		frames = 0;
	}

	// draw info
	unsigned color;
	if (avgFps < 15) color = RGB(1,0,0);
	else if (avgFps < 30) color = RGB(1,0,1);
	else if (avgFps < 60) color = RGB(1,1,0);
	else color = RGB(0,1,0);
	re.DrawTextRight (va("FPS: %.2f  min: %.2f  max: %.2f", avgFps, minFps, maxFps), color);
}


#define MIN_WATER_DISTANCE	4.5

static void FixWaterVis (void)
{
	vec3_t		p;			// point
	int			cont;		// its contents
	int			w, w1;		// "in water" flag
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


bool V_RenderView (float stereo_separation)
{
	guard(V_RenderView);

	if (cls.state != ca_active)
		return false;
	if (!cl.refresh_prepped)
		return false;			// still loading
	if (!map_clientLoaded)
		return false;			// map already unloaded (by server), but client is still active (ca_active)

	if (timedemo->integer)
	{
		static int lastTime = 0;

		int time = appMilliseconds ();
		if (!cl.timedemoStart)
		{
			cl.timedemoStart = time;
//			cl.timedemoLongestFrame = 0;	-- cleared anyway within a new server map
//			cl.timedemoFrames = 0;
		}
		else
		{
			int timeDelta = time - lastTime;
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
		CalcVrect ();
		TileClear ();

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
		cl.refdef.vieworg[0] += 1.0f/16;	//??
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
		cl.refdef.beams = active_beams;
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

		// sort entities for better cache locality - ???
		qsort (cl.refdef.entities, cl.refdef.num_entities, sizeof(cl.refdef.entities[0]),
			(int (*)(const void *, const void *))entitycmpfnc);
	}

	re.RenderFrame (&cl.refdef);

	// stats
	if (r_drawfps->integer)
		DrawFpsInfo ();

	return true;
	unguard;
}


/*
=============
V_Init
=============
*/
void V_Init (void)
{
CVAR_BEGIN(vars)
	CVAR_VAR(cl_testblend, 0, 0),
	CVAR_VAR(cl_testparticles, 0, 0),
	CVAR_VAR(cl_testentities, 0, 0),
	CVAR_VAR(cl_testlights, 0, CVAR_CHEAT),

	CVAR_VAR(r_drawfps, 0, 0),
	CVAR_VAR(r_playerpos, 0, CVAR_CHEAT),
	CVAR_VAR(r_surfinfo, 0, CVAR_CHEAT),

	CVAR_VAR(scr_viewsize, 100, CVAR_ARCHIVE)	//?? should rename: not scr var
CVAR_END

	Cvar_GetVars (ARRAY_ARG(vars));

#ifdef GUN_DEBUG
	RegisterCommand ("gun_next", V_Gun_Next_f);
	RegisterCommand ("gun_prev", V_Gun_Prev_f);
	RegisterCommand ("gun_model", V_Gun_Model_f);
#endif
	RegisterCommand ("sky", Sky_f);
}
