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
// sv_game.c -- interface to the game dll

#include "server.h"

game_export_t	*ge;


/*
===============
PF_Unicast

Sends the contents of the mutlicast buffer to a single client
===============
*/
static void PF_Unicast (edict_t *ent, qboolean reliable)
{
	int		p;
	client_t	*client;

	if (!ent)
		return;

	p = NUM_FOR_EDICT(ent);
	if (p < 1 || p > maxclients->integer)
		return;

	client = svs.clients + (p-1);

	if (reliable)
		SZ_Write (&client->netchan.message, sv.multicast.data, sv.multicast.cursize);
	else
		SZ_Write (&client->datagram, sv.multicast.data, sv.multicast.cursize);

	SZ_Clear (&sv.multicast);
}


/*
===============
PF_dprintf

Debug print to server console
===============
*/
static void PF_dprintf (char *fmt, ...)
{
	char		msg[1024];
	va_list		argptr;

	va_start (argptr,fmt);
	vsprintf (msg, fmt, argptr);
	va_end (argptr);

	Com_Printf ("%s", msg);
}


/*
===============
PF_cprintf

Print to a single client
===============
*/
static void PF_cprintf (edict_t *ent, int level, char *fmt, ...)
{
	char		msg[1024];
	va_list		argptr;
	int			n;

	if (ent)
	{
		n = NUM_FOR_EDICT(ent);
		if (n < 1 || n > maxclients->integer)
			Com_Error (ERR_DROP, "cprintf to a non-client");
	}

	va_start (argptr,fmt);
	vsprintf (msg, fmt, argptr);
	va_end (argptr);

	if (ent)
		SV_ClientPrintf (svs.clients+(n-1), level, "%s", msg);
	else
		Com_Printf ("%s", msg);
}


/*
===============
PF_centerprintf

centerprint to a single client
===============
*/
static void PF_centerprintf (edict_t *ent, char *fmt, ...)
{
	char		msg[1024];
	va_list		argptr;
	int			n;

	n = NUM_FOR_EDICT(ent);
	if (n < 1 || n > maxclients->integer)
		return;	// Com_Error (ERR_DROP, "centerprintf to a non-client");

	va_start (argptr,fmt);
	vsprintf (msg, fmt, argptr);
	va_end (argptr);

	MSG_WriteByte (&sv.multicast,svc_centerprint);
	MSG_WriteString (&sv.multicast,msg);
	PF_Unicast (ent, true);
}


/*
===============
PF_error

Abort the server with a game error
===============
*/
static void PF_error (char *fmt, ...)
{
	char		msg[1024];
	va_list		argptr;

	va_start (argptr,fmt);
	vsprintf (msg, fmt, argptr);
	va_end (argptr);

	Com_Error (ERR_DROP, "Game Error: %s", msg);
}


/*
=================
PF_setmodel

Also sets mins and maxs for inline bmodels
=================
*/
static void PF_setmodel (edict_t *ent, char *name)
{
	int		i;
	cmodel_t	*mod;

	if (!name)
		Com_Error (ERR_DROP, "PF_setmodel: NULL");

	i = SV_ModelIndex (name);

//	ent->model = name;
	ent->s.modelindex = i;

// if it is an inline model, get the size information for it
	if (name[0] == '*')
	{
		mod = CM_InlineModel (name);
		VectorCopy (mod->mins, ent->mins);
		VectorCopy (mod->maxs, ent->maxs);
		SV_LinkEdict (ent);
	}

}

/*
===============
PF_Configstring

===============
*/
static void PF_Configstring (int index, char *val)
{
	if (index < 0 || index >= MAX_CONFIGSTRINGS)
		Com_Error (ERR_DROP, "configstring: bad index %i\n", index);

	if (!val)
		val = "";

	// change the string in sv
	strcpy (sv.configstrings[index], val);


	if (sv.state != ss_loading)
	{	// send the update to everyone
		SZ_Clear (&sv.multicast);
		MSG_WriteChar (&sv.multicast, svc_configstring);
		MSG_WriteShort (&sv.multicast, index);
		MSG_WriteString (&sv.multicast, val);

		SV_MulticastOld (vec3_origin, MULTICAST_ALL_R);
	}
}



static void PF_WriteChar (int c) {MSG_WriteChar (&sv.multicast, c);}
static void PF_WriteByte (int c) {MSG_WriteByte (&sv.multicast, c);}
static void PF_WriteShort (int c) {MSG_WriteShort (&sv.multicast, c);}
static void PF_WriteLong (int c) {MSG_WriteLong (&sv.multicast, c);}
static void PF_WriteFloat (float f) {MSG_WriteFloat (&sv.multicast, f);}
static void PF_WriteString (char *s) {MSG_WriteString (&sv.multicast, s);}
static void PF_WritePos (vec3_t pos) {MSG_WritePos (&sv.multicast, pos);}
static void PF_WriteDir (vec3_t dir) {MSG_WriteDir (&sv.multicast, dir);}
static void PF_WriteAngle (float f) {MSG_WriteAngle (&sv.multicast, f);}


/*
=================
PF_inPVS

Also checks portalareas so that doors block sight
=================
*/
static qboolean PF_inPVS (vec3_t p1, vec3_t p2)
{
	int		leafnum;
	int		cluster;
	int		area1, area2;
	byte	*mask;

	leafnum = CM_PointLeafnum (p1);
	cluster = CM_LeafCluster (leafnum);
	area1 = CM_LeafArea (leafnum);
	mask = CM_ClusterPVS (cluster);

	leafnum = CM_PointLeafnum (p2);
	cluster = CM_LeafCluster (leafnum);
	area2 = CM_LeafArea (leafnum);
	if ( mask && (!(mask[cluster>>3] & (1<<(cluster&7)) ) ) )
		return false;
	if (!CM_AreasConnected (area1, area2))
		return false;		// a door blocks sight
	return true;
}


/*
=================
PF_inPHS

Also checks portalareas so that doors block sound
=================
*/
static qboolean PF_inPHS (vec3_t p1, vec3_t p2)
{
	int		leafnum;
	int		cluster;
	int		area1, area2;
	byte	*mask;

	leafnum = CM_PointLeafnum (p1);
	cluster = CM_LeafCluster (leafnum);
	area1 = CM_LeafArea (leafnum);
	mask = CM_ClusterPHS (cluster);

	leafnum = CM_PointLeafnum (p2);
	cluster = CM_LeafCluster (leafnum);
	area2 = CM_LeafArea (leafnum);
	if ( mask && (!(mask[cluster>>3] & (1<<(cluster&7)) ) ) )
		return false;		// more than one bounce away
	if (!CM_AreasConnected (area1, area2))
		return false;		// a door blocks hearing

	return true;
}

static void PF_StartSound (edict_t *entity, int channel, int sound_num, float volume, float attenuation, float timeofs)
{
	if (!entity)
		return;
	SV_StartSoundOld (NULL, entity, channel, sound_num, volume, attenuation, timeofs);
}

//==============================================

/*
===============
SV_ShutdownGameProgs

Called when either the entire server is being killed, or
it is changing to a different game directory.
===============
*/
void SV_ShutdownGameProgs (void)
{
	if (!ge)
		return;
	ge->Shutdown ();
	Sys_UnloadGame ();
	ge = NULL;
}


/*
===============
SV_InitGameProgs

Init the game subsystem for a new map
===============
*/

/*------- Wrappers for some system functions -------*/

static void *PF_TagMalloc (int size, int tag)
{
	return Z_TagMalloc (size + 1, tag); // reserve 1 byte for buggy mods
}


static cvar_t *PF_Cvar_Get (char *name, char *value, int flags)
{
	return Cvar_Get (name, value, flags|CVAR_GAME_CREATED|CVAR_NODEFAULT);
}



void SCR_DebugGraph (float value, int color);


//----------------------------------------------------------------------------------

#define SV_Pmove Pmove


#define SV_PROFILE

#ifdef SV_PROFILE

int prof_times[256];
int prof_counts[256];


#pragma warning (push)
#pragma warning (disable : 4035)
#pragma warning (disable : 4715)
__inline unsigned cycles (void)	  // taken from UT
{
	__asm
	{
		xor   eax,eax	          // Required so that VC++ realizes EAX is modified.
		_emit 0x0F		          // RDTSC  -  Pentium+ time stamp register to EDX:EAX.
		_emit 0x31		          // Use only 32 bits in EAX - even a Ghz cpu would have a 4+ sec period.
		xor   edx,edx	          // Required so that VC++ realizes EDX is modified.
	}
}
#pragma warning (pop)

#define PROF						\
	int	st_t = cycles();

#define PROF2(type)					\
	type res;						\
	int st_t = cycles();			\
	res =

#define EPROF(n) 					\
	prof_times[n] += cycles()-st_t;	\
	prof_counts[n]++;

#define EPROF2(n) 					\
	prof_times[n] += cycles()-st_t;	\
	prof_counts[n]++;				\
	return res;


static void PSV_LinkEdict(edict_t *e)
{	PROF; SV_LinkEdict(e); EPROF(0);	}
#define SV_LinkEdict PSV_LinkEdict

static void PSV_UnlinkEdict (edict_t *e)
{	PROF; SV_UnlinkEdict(e); EPROF(1);	}
#define SV_UnlinkEdict PSV_UnlinkEdict

static int PSV_AreaEdicts (vec3_t mins, vec3_t maxs, edict_t **list, int maxcount, int areatype)
{	PROF2(int)	SV_AreaEdicts(mins,maxs,list,maxcount,areatype); EPROF2(2);	}
#define SV_AreaEdicts PSV_AreaEdicts

static trace_t PSV_Trace (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, edict_t *passedict, int contentmask)
{	PROF2(trace_t)	SV_Trace(start,mins,maxs,end,passedict,contentmask); EPROF2(3);	}
#define SV_Trace PSV_Trace

static int PSV_PointContents (vec3_t p)
{	PROF2(int)	SV_PointContents(p); EPROF2(4);	}
#define SV_PointContents PSV_PointContents

static void PPmove (pmove_t *pmove)
{	PROF; Pmove(pmove); EPROF(5);	}
#define SV_Pmove PPmove

static int PSV_ModelIndex (char *name)
{	PROF2(int)	SV_ModelIndex(name); EPROF2(6);	}
#define SV_ModelIndex PSV_ModelIndex

static int PSV_ImageIndex (char *name)
{	PROF2(int)	SV_ImageIndex(name); EPROF2(7);	}
#define SV_ImageIndex PSV_ImageIndex

static int PSV_SoundIndex (char *name)
{	PROF2(int)	SV_SoundIndex(name); EPROF2(8);	}
#define SV_SoundIndex PSV_SoundIndex

static void* PPF_TagMalloc (int size, int tag)
{	PROF2(void*)	PF_TagMalloc (size, tag); EPROF2(9);	}
#define PF_TagMalloc PPF_TagMalloc

static void PZ_Free (void *ptr)
{	PROF;	Z_Free (ptr); EPROF(10);	}
#define Z_Free PZ_Free

static void PZ_FreeTags (int tag)
{	PROF;	Z_FreeTags (tag); EPROF(11);	}
#define Z_FreeTags PZ_FreeTags

#endif // SV_PROFILE


void SV_InitGameProgs (void)
{
	game_import_t	import;

	// unload anything we have now
	if (ge)
		SV_ShutdownGameProgs ();


	// load a new game dll
	import.multicast = SV_MulticastOld;	//?? we can hook some messages in this function
	import.unicast = PF_Unicast;
	import.bprintf = SV_BroadcastPrintf;
	import.dprintf = PF_dprintf;
	import.cprintf = PF_cprintf;
	import.centerprintf = PF_centerprintf;
	import.error = PF_error;

	import.linkentity = SV_LinkEdict;
	import.unlinkentity = SV_UnlinkEdict;
	import.BoxEdicts = SV_AreaEdicts;
	import.trace = SV_Trace;
	import.pointcontents = SV_PointContents;
	import.setmodel = PF_setmodel;
	import.inPVS = PF_inPVS;
	import.inPHS = PF_inPHS;
	import.Pmove = SV_Pmove;	// really, this is a Pmove() !!

	import.modelindex = SV_ModelIndex;
	import.soundindex = SV_SoundIndex;
	import.imageindex = SV_ImageIndex;

	import.configstring = PF_Configstring;
	import.sound = PF_StartSound;
	import.positioned_sound = SV_StartSoundOld;

	import.WriteChar = PF_WriteChar;
	import.WriteByte = PF_WriteByte;
	import.WriteShort = PF_WriteShort;
	import.WriteLong = PF_WriteLong;
	import.WriteFloat = PF_WriteFloat;
	import.WriteString = PF_WriteString;
	import.WritePosition = PF_WritePos;
	import.WriteDir = PF_WriteDir;
	import.WriteAngle = PF_WriteAngle;

	import.TagMalloc = PF_TagMalloc;		// Z_TagMalloc
	import.TagFree = Z_Free;
	import.FreeTags = Z_FreeTags;

	import.cvar = PF_Cvar_Get;				// Cvar_Get
	import.cvar_set = Cvar_Set;
	import.cvar_forceset = Cvar_ForceSet;

	import.argc = Cmd_Argc;
	import.argv = Cmd_Argv;
	import.args = Cmd_Args;
	import.AddCommandString = Cbuf_AddText;

	import.DebugGraph = SCR_DebugGraph;
	import.SetAreaPortalState = CM_SetAreaPortalState;
	import.AreasConnected = CM_AreasConnected;

	ge = (game_export_t *)Sys_GetGameAPI (&import);

	if (!ge)
		Com_Error (ERR_DROP, "failed to load game DLL");
	if (ge->apiversion != GAME_API_VERSION)
		Com_Error (ERR_DROP, "game is version %i, not %i", ge->apiversion,
		GAME_API_VERSION);

	ge->Init ();
}
