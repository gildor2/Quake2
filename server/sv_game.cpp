#include "server.h"

game_export_t	*ge;
static bool dummyGame;		// when demomap or cinematic loaded


#define SV_Pmove Pmove		// for profiling


/*
===============
PF_Unicast

Sends the contents of the mutlicast buffer to a single client
===============
*/
static void PF_Unicast (edict_t *ent, qboolean reliable)
{
	guard(PF_Unicast);

	if (!ent) return;
	int p = NUM_FOR_EDICT(ent);
	if (p < 1 || p > maxclients->integer)
		return;

	client_t *client = svs.clients + (p-1);

	if (reliable)
		SZ_Write (&client->netchan.message, sv.multicast.data, sv.multicast.cursize);
	else
		SZ_Write (&client->datagram, sv.multicast.data, sv.multicast.cursize);
	SZ_Clear (&sv.multicast);

	unguard;
}


/*
===============
PF_dprintf

Debug print to server console
===============
*/
static void PF_dprintf (char *fmt, ...)
{
	char	msg[1024];
	va_list	argptr;

	va_start (argptr,fmt);
	vsnprintf (ARRAY_ARG(msg), fmt, argptr);
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
	char	msg[1024];
	va_list	argptr;

	guard(PF_cprintf);
	va_start (argptr,fmt);
	vsnprintf (ARRAY_ARG(msg), fmt, argptr);
	va_end (argptr);

	if (ent)
	{
		int n = NUM_FOR_EDICT(ent);
		if (n < 1 || n > maxclients->integer)
			Com_DropError ("cprintf to a non-client");
		SV_ClientPrintf (svs.clients+(n-1), level, "%s", msg);
	}
	else
		Com_Printf ("%s", msg);
	unguard;
}


/*
===============
PF_centerprintf

centerprint to a single client
===============
*/
static void PF_centerprintf (edict_t *ent, char *fmt, ...)
{
	char	msg[1024];
	va_list	argptr;

	int n = NUM_FOR_EDICT(ent);
	if (n < 1 || n > maxclients->integer)
		return;	// Com_DropError ("centerprintf to a non-client");

	guard(PF_centerprintf);
	va_start (argptr,fmt);
	vsnprintf (ARRAY_ARG(msg), fmt, argptr);
	va_end (argptr);

	MSG_WriteByte (&sv.multicast,svc_centerprint);
	MSG_WriteString (&sv.multicast,msg);
	PF_Unicast (ent, true);
	unguard;
}


/*
===============
PF_error

Abort the server with a game error
===============
*/
static void PF_error (char *fmt, ...)
{
	char	msg[1024];
	va_list	argptr;

	va_start (argptr,fmt);
	vsnprintf (ARRAY_ARG(msg), fmt, argptr);
	va_end (argptr);

	Com_DropError ("Game Error: %s", msg);
}


/*
=================
PF_setmodel

Also sets mins and maxs for inline bmodels
=================
*/
static void PF_setmodel (edict_t *ent, char *name)
{
	guard(PF_setmodel);

	if (!name) Com_DropError ("PF_setmodel: NULL name");

	//	ent->model = name;
	ent->s.modelindex = SV_ModelIndex (name);

	// if it is an inline model, get the size information for it
	if (name[0] == '*')
	{
		cmodel_t *mod = CM_InlineModel (name);
		VectorCopy (mod->mins, ent->mins);
		VectorCopy (mod->maxs, ent->maxs);
		SV_LinkEdict (ent);
	}
	unguard;
}

/*
===============
PF_Configstring

===============
*/
static void PF_Configstring (int index, char *val)
{
	guard(PF_Configstring);
	if (index < 0 || index >= MAX_CONFIGSTRINGS)
		Com_DropError ("configstring: bad index %i\n", index);

	if (!val)
		val = "";

	// change the string in sv
	strcpy (sv.configstrings[index], val);

	if (sv.state != ss_loading)
	{
		// send the update to everyone
		SZ_Clear (&sv.multicast);
		MSG_WriteChar (&sv.multicast, svc_configstring);
		MSG_WriteShort (&sv.multicast, index);
		MSG_WriteString (&sv.multicast, val);

		SV_MulticastOld (vec3_origin, MULTICAST_ALL_R);
	}
	unguard;
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
	int		leafnum, cluster;
	int		area1, area2;
	byte	*mask;

	leafnum = CM_PointLeafnum (p1);
	cluster = CM_LeafCluster (leafnum);
	area1 = CM_LeafArea (leafnum);

	mask = CM_ClusterPVS (cluster);

	leafnum = CM_PointLeafnum (p2);
	cluster = CM_LeafCluster (leafnum);
	area2 = CM_LeafArea (leafnum);

	if (mask && (!(mask[cluster>>3] & (1<<(cluster&7)))))
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

	if (mask && (!(mask[cluster>>3] & (1<<(cluster&7)))))
		return false;		// more than one bounce away
	if (!CM_AreasConnected (area1, area2))
		return false;		// a door blocks hearing

	return true;
}

static void PF_StartSound (edict_t *entity, int channel, int sound_num, float volume, float attenuation, float timeofs)
{
	if (!entity) return;
	SV_StartSoundOld (NULL, entity, channel, sound_num, volume, attenuation, timeofs);
}

static void	PF_SetAreaPortalState (int portalnum, qboolean open)
{
	CM_SetAreaPortalState (portalnum, open != 0);
}

static qboolean PF_AreasConnected (int area1, int area2)
{
	return CM_AreasConnected (area1, area2);
}


/*-----------------------------------------------------------------------------
	Support for tagged memory
-----------------------------------------------------------------------------*/

typedef struct zhead_s
{
	struct zhead_s *prev, *next;
	short	tag;
	int		size;		// for stats only
} zhead_t;

static zhead_t	z_chain = {&z_chain, &z_chain};
static int		z_count, z_bytes;


static void Z_Free (void *ptr)
{
	zhead_t	*z;

	guard(Z_Free);
	if (!ptr) return;
	z = ((zhead_t*)ptr)-1;

	z->prev->next = z->next;
	z->next->prev = z->prev;

	z_count--;
	z_bytes -= z->size;
	guard(appFree);	//??
	appFree (z);
	unguard;		//??
	unguard;
}


static void Z_FreeTags (int tag)
{
	zhead_t	*z, *next;

	guard(Z_FreeTags);
	for (z = z_chain.next; z != &z_chain; z = next)
	{
		next = z->next;
		if (z->tag == tag) Z_Free (z+1);
	}
	unguard;
}


static void *Z_TagMalloc (int size, int tag)
{
	zhead_t	*z;

	guard(Z_TagMalloc);
	size += sizeof(zhead_t) + 1;	// reserve 1 byte for buggy mods

	z = (zhead_t*) appMalloc (size);
	z_count++;
	z_bytes += size;
	z->tag = tag;
	z->size = size;

	z->next = z_chain.next;
	z->prev = &z_chain;
	z_chain.next->prev = z;
	z_chain.next = z;

	return z+1;
	unguard;
}


static void ShutdownGameMemory (void)
{
	guard(ShutdownGameMemory);
	if (z_count || z_bytes)
	{
		zhead_t	*z, *next;

		Com_DPrintf ("Game library have memory leak of %d bytes in %d blocks\n", z_bytes, z_count);
		for (z = z_chain.next; z != &z_chain; z = next)
		{
			next = z->next;
			Z_Free (z+1);
		}
	}
	unguard;
}

// Memory status console command
static void GZ_Stats_f (void)
{
	Com_Printf ("Game memory:\n%d bytes in %d blocks\n", z_bytes, z_count);
}

/*-------- Wrappers for some system functions ----------------*/


static cvar_t *PF_Cvar_Get (char *name, char *value, int flags)
{
	return Cvar_Get (name, value, flags|CVAR_GAME_CREATED|CVAR_NODEFAULT);
}


//--------------------------------------------------------------

#define SV_PROFILE

#ifdef SV_PROFILE

int prof_times[256];
int prof_counts[256];


#pragma warning (push)
#pragma warning (disable : 4035)
#pragma warning (disable : 4715)
inline unsigned cycles (void)	  // taken from UT
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

static trace_t PSV_TraceHook (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, edict_t *passedict, int contentmask)
{	PROF2(trace_t)	SV_TraceHook(start,mins,maxs,end,passedict,contentmask); EPROF2(3);	}
#define SV_TraceHook PSV_TraceHook

static int PSV_PointContents (vec3_t p)
{	PROF2(int)	SV_PointContents(p); EPROF2(4);	}
#define SV_PointContents PSV_PointContents

static void PPmove (pmove_t *pmove)
{	PROF; Pmove(pmove); EPROF(5);	}
#undef SV_Pmove
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

static void* PZ_TagMalloc (int size, int tag)
{	PROF2(void*)	Z_TagMalloc (size, tag); EPROF2(9);	}
#define Z_TagMalloc PZ_TagMalloc

static void PZ_Free (void *ptr)
{	PROF;	Z_Free (ptr); EPROF(10);	}
#define Z_Free PZ_Free

static void PZ_FreeTags (int tag)
{	PROF;	Z_FreeTags (tag); EPROF(11);	}
#define Z_FreeTags PZ_FreeTags

#endif // SV_PROFILE


// dummy functions
static void D_Func0 (void) {}
static void D_Func1 (char *a) {}
static void D_Func1A (edict_t *a) {}
static void D_Func2A (char *a, qboolean b) {}
static qboolean D_ClientConnect (edict_t *a, char *b) { return true; }
static void D_Func2B (edict_t *a, char *b) {}
static void D_Func2C (edict_t *a, usercmd_t *b) {}
static void D_Func3 (char *a, char *b, char *c) {}


static const char *gameCommands[] = {"wave", "inven", "kill", "use",
	"drop", "say", "say_team", "give", "god", "notarget", "noclip",
	"invuse", "invprev", "invnext", "invdrop", "weapnext", "weapprev"};

void SV_InitGameLibrary (bool dummy)
{
	game_import_t import;
	static const game_import_t import2 = {
		SV_BroadcastPrintf, PF_dprintf, PF_cprintf, PF_centerprintf,
		PF_StartSound, SV_StartSoundOld,
		PF_Configstring,
		PF_error,
		SV_ModelIndex, SV_SoundIndex, SV_ImageIndex,
		PF_setmodel,
		SV_TraceHook, SV_PointContents,
		PF_inPVS, PF_inPHS, PF_SetAreaPortalState, PF_AreasConnected,
		SV_LinkEdict, SV_UnlinkEdict, SV_AreaEdicts,
		SV_Pmove,
		SV_MulticastOld, PF_Unicast,
		PF_WriteChar, PF_WriteByte, PF_WriteShort, PF_WriteLong, PF_WriteFloat,
		PF_WriteString, PF_WritePos, PF_WriteDir, PF_WriteAngle,
		Z_TagMalloc, Z_Free, Z_FreeTags,
		PF_Cvar_Get, Cvar_Set, Cvar_ForceSet,
		Cmd_Argc, Cmd_Argv, Cmd_Args,
		Cbuf_AddText,
		SCR_DebugGraph
	};
	static game_export_t dge = {
		GAME_API_VERSION,
		D_Func0, D_Func0,
		D_Func3,
		D_Func2A,
		D_Func1, D_Func1, D_Func1,
		D_ClientConnect,
		D_Func1A,
		D_Func2B,
		D_Func1A, D_Func1A,
		D_Func2C,
		D_Func0, D_Func0,
		NULL,
		sizeof(edict_t),
		0, 0
	};

	guard(SV_InitGameLibrary);

	dummyGame = dummy;
	if (dummyGame)
	{
		ge = &dge;
		return;
	}

	// unload anything we have now
	if (ge) SV_ShutdownGameLibrary ();

	// load game library
	import = import2;
	ge = (game_export_t*) Sys_GetGameAPI (&import);

	if (!ge)
		Com_DropError ("failed to load game library");
	if (ge->apiversion != GAME_API_VERSION)
		Com_DropError ("game is version %d, not " STR(GAME_API_VERSION), ge->apiversion);

	guard(ge.Init);
	ge->Init ();
	unguard;

	RegisterCommand ("gz_stats", GZ_Stats_f);
	// register NULL commands for ability to complete them from console (will be forwarded to server)
	for (int i = 0; i < ARRAY_COUNT(gameCommands); i++)
		RegisterCommand (gameCommands[i], NULL, 0);

	unguard;
}


void SV_ShutdownGameLibrary (void)
{
	guard(SV_ShutdownGameLibrary);
	if (dummyGame)
	{
		ge = NULL;
		return;
	}
	if (!ge) return;

	// unload game library
	guard(ge.Shutdown);
	ge->Shutdown ();
	unguard;
	Sys_UnloadGame ();
	ge = NULL;

	// free tagged memory (in a case of memory leaks)
	ShutdownGameMemory ();

	UnregisterCommand ("gz_stats");
	// unregister game commands
	for (int i = 0; i < ARRAY_COUNT(gameCommands); i++)
		UnregisterCommand (gameCommands[i]);
	unguard;
}
