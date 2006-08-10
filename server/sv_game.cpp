#include "server.h"
#include "cmodel.h"

#define SV_PROFILE	1		//?? change

game_export_t	*ge;
static bool dummyGame;		// when demomap or cinematic loaded


#define SV_Pmove Pmove		// for profiling


/*-----------------------------------------------------------------------------
	Interface functions for game library
-----------------------------------------------------------------------------*/

// Sends the contents of the mutlicast buffer to a single client
static void G_Unicast (edict_t *ent, qboolean reliable)
{
	guard(G_Unicast);

	if (!ent) return;
	int p = NUM_FOR_EDICT(ent);
	if (p < 1 || p > sv_maxclients->integer)
		return;

	client_t *client = svs.clients + (p-1);

	if (reliable)
		client->netchan.message.Write (sv.multicast);
	else
		client->datagram.Write (sv.multicast);
	sv.multicast.Clear ();

	unguard;
}


// Debug print to server console
static void G_dprintf (const char *fmt, ...)
{
	char	msg[1024];
	va_list	argptr;

	va_start (argptr,fmt);
	vsnprintf (ARRAY_ARG(msg), fmt, argptr);
	va_end (argptr);

	appPrintf ("%s", msg);
}



// Print to a single client
static void G_cprintf (edict_t *ent, int level, const char *fmt, ...)
{
	char	msg[1024];
	va_list	argptr;

	guard(G_cprintf);
	va_start (argptr,fmt);
	vsnprintf (ARRAY_ARG(msg), fmt, argptr);
	va_end (argptr);

	if (ent)
	{
		int n = NUM_FOR_EDICT(ent);
		if (n < 1 || n > sv_maxclients->integer)
			Com_DropError ("cprintf to a non-client");
		SV_ClientPrintf (svs.clients+(n-1), level, "%s", msg);
	}
	else
		appPrintf ("%s", msg);
	unguard;
}


// centerprint to a single client
static void G_centerprintf (edict_t *ent, const char *fmt, ...)
{
	char	msg[1024];
	va_list	argptr;

	int n = NUM_FOR_EDICT(ent);
	if (n < 1 || n > sv_maxclients->integer)
		return;	// Com_DropError ("centerprintf to a non-client");

	guard(G_centerprintf);
	va_start (argptr,fmt);
	vsnprintf (ARRAY_ARG(msg), fmt, argptr);
	va_end (argptr);

	MSG_WriteByte (&sv.multicast,svc_centerprint);
	MSG_WriteString (&sv.multicast,msg);
	G_Unicast (ent, true);
	unguard;
}


// Abort the server with a game error
static void G_error (const char *fmt, ...)
{
	char	msg[1024];
	va_list	argptr;

	va_start (argptr,fmt);
	vsnprintf (ARRAY_ARG(msg), fmt, argptr);
	va_end (argptr);

	Com_DropError ("Game Error: %s", msg);
}


// Also sets mins/maxs for inline bmodels
static void G_setmodel (edict_t *ent, char *name)
{
	guard(G_setmodel);

	if (!name) Com_DropError ("G_setmodel: NULL name");

	//	ent->model = name;
	ent->s.modelindex = SV_ModelIndex (name);

	// if it is an inline model, get the size information for it
	if (name[0] == '*')
	{
		cmodel_t *mod = CM_InlineModel (name);
		ent->bounds = mod->bounds;
		SV_LinkEdict (ent);
	}
	unguard;
}


static void G_Configstring (int index, const char *val)
{
	guard(G_Configstring);
	if (index < 0 || index >= MAX_CONFIGSTRINGS)
		Com_DropError ("configstring: bad index %i\n", index);

	if (!val) val = "";

	// change the string in sv
	// NOTE: cannot use appStrncpyz (sv.configstrings[index], val, sizeof(sv.configstrings[index])) here,
	//	because some configstrings are longer, than MAX_QPATH (one string uses few configstrings[]); for
	//	example, CS_STATUSBAR
	strcpy (sv.configstrings[index], val);

	if (sv.state != ss_loading)
	{
		// send the update to everyone
		sv.multicast.Clear ();
		MSG_WriteChar (&sv.multicast, svc_configstring);
		MSG_WriteShort (&sv.multicast, index);
		MSG_WriteString (&sv.multicast, val);

		SV_MulticastOld (nullVec3, MULTICAST_ALL_R);
	}
	unguard;
}


static void G_WriteChar (int c)		{ MSG_WriteChar (&sv.multicast, c); }
static void G_WriteByte (int c)		{ MSG_WriteByte (&sv.multicast, c); }
static void G_WriteShort (int c)	{ MSG_WriteShort (&sv.multicast, c); }
static void G_WriteLong (int c)		{ MSG_WriteLong (&sv.multicast, c); }
static void G_WriteFloat (float f)	{ MSG_WriteFloat (&sv.multicast, f); }
static void G_WriteString (char *s) { MSG_WriteString (&sv.multicast, s); }
static void G_WritePos (const CVec3 &pos) { MSG_WritePos (&sv.multicast, pos); }
static void G_WriteAngle (float f)	{ MSG_WriteAngle (&sv.multicast, f); }

static void G_WriteDir (const CVec3 *dir)
{
	if (dir)
		MSG_WriteDir (&sv.multicast, *dir);
	else
		MSG_WriteByte (&sv.multicast, 0);
}



// Also checks portalareas so that doors block sight
static qboolean G_inPVS (const CVec3 &p1, const CVec3 &p2)
{
	const CBspLeaf *leaf1 = CM_FindLeaf (p1);
	const CBspLeaf *leaf2 = CM_FindLeaf (p2);
	const byte *mask = CM_ClusterPVS (leaf1->cluster);
	int cluster  = leaf2->cluster;

	if (mask && (!(mask[cluster >> 3] & (1<<(cluster & 7)))))
		return false;

	if (!CM_AreasConnected (leaf1->area, leaf2->area))
		return false;		// a door blocks sight
	return true;
}


// Checks portalareas so that doors block sound
static qboolean G_inPHS (const CVec3 &p1, const CVec3 &p2)
{
	const CBspLeaf *leaf1 = CM_FindLeaf (p1);
	const CBspLeaf *leaf2 = CM_FindLeaf (p2);

	if (!CM_AreasConnected (leaf1->area, leaf2->area))
		return false;		// a door blocks hearing

	return true;
}


static void G_StartSound (edict_t *entity, int channel, int sound_num, float volume, float attenuation, float timeofs)
{
	if (!entity) return;
	SV_StartSoundOld (NULL, entity, channel, sound_num, volume, attenuation, timeofs);
}


static void	G_SetAreaPortalState (int portalnum, qboolean open)
{
	CM_SetAreaPortalState (portalnum, open != 0);
}


static qboolean G_AreasConnected (int area1, int area2)
{
	return CM_AreasConnected (area1, area2);
}


static trace0_t* G_Trace (trace0_t &trace, const CVec3 &start, const CVec3 *mins, const CVec3 *maxs, const CVec3 &end, edict_t *passedict, int contentmask)
{
	SV_TraceHook (trace, start, mins, maxs, end, passedict, contentmask);
	return &trace;
}


/*-------- Wrappers for some system functions ----------------*/

static cvar_t *G_Cvar_Get (const char *name, char *value, int flags)
{
	// fake cvars, not needed by engine, but may be needed by some mods
	static cvar_t d_basedir = {
		"basedir", "."
	}, d_gamedir = {
		"gamedir", d_gamedir.buf
	};
	if (!stricmp (name, "basedir"))
		return &d_basedir;
	else if (!stricmp (name, "gamedir"))
	{
		appStrncpyz (d_gamedir.buf, FS_Gamedir (), sizeof(d_gamedir.buf));
		return &d_gamedir;
	}
	// get cvar
	return Cvar_Get (name, value, flags|CVAR_GAME_CREATED|CVAR_NODEFAULT);
}


/*-----------------------------------------------------------------------------
	Support for tagged memory
-----------------------------------------------------------------------------*/

struct zhead_t
{
	zhead_t	*prev, *next;
	short	tag;
	int		size;		// for stats only
};

static zhead_t	z_chain = {&z_chain, &z_chain};
static int		z_count, z_bytes;


static void Z_Free (void *ptr)
{
	guard(Z_Free);
	if (!ptr) return;
	zhead_t *z = ((zhead_t*)ptr)-1;

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
	guard(Z_FreeTags);
	zhead_t	*z, *next;
	for (z = z_chain.next; z != &z_chain; z = next)
	{
		next = z->next;
		if (z->tag == tag) Z_Free (z+1);
	}
	unguard;
}


static void *Z_TagMalloc (int size, int tag)
{
	guard(Z_TagMalloc);
#if !SV_PROFILE
	MEM_ALLOCATOR(size);
#endif
	size += sizeof(zhead_t) + 1;	// reserve 1 byte for buggy mods

	zhead_t *z = (zhead_t*) appMalloc (size);
	z_count++;
	z_bytes += size;
	z->tag  = tag;
	z->size = size;

	z->next = z_chain.next;
	z->prev = &z_chain;
	z_chain.next->prev = z;
	z_chain.next = z;

	return z+1;
	unguard;
}


static void ShutdownGameMemory ()
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
//?? make as global "meminfo" subsystem
static void GZ_Stats_f ()
{
	appPrintf ("Game memory:\n%d bytes in %d blocks\n", z_bytes, z_count);
}


/*-----------------------------------------------------------------------------
	Server profiling
-----------------------------------------------------------------------------*/

#if SV_PROFILE

unsigned prof_times[256];
unsigned prof_counts[256];


#define PROF						\
	unsigned st_t = appCycles();

#define EPROF(n) 					\
	prof_times[n] += appCycles()-st_t; \
	prof_counts[n]++;

#define PROF2(type)					\
	PROF; type res =

#define EPROF2(n) 					\
	EPROF(n); return res;

static void PSV_LinkEdict(edict_t *e)
{	PROF; SV_LinkEdict(e); EPROF(0);	}
#define SV_LinkEdict PSV_LinkEdict

static void PSV_UnlinkEdict (edict_t *e)
{	PROF; SV_UnlinkEdict(e); EPROF(1);	}
#define SV_UnlinkEdict PSV_UnlinkEdict

static int PSV_AreaEdicts (const CVec3 &mins, const CVec3 &maxs, edict_t **list, int maxcount, int areatype)
{	PROF2(int)	SV_AreaEdicts(mins,maxs,list,maxcount,areatype); EPROF2(2);	}
#define SV_AreaEdicts PSV_AreaEdicts

static trace0_t* PSV_Trace (trace0_t &trace, const CVec3 &start, const CVec3 *mins, const CVec3 *maxs, const CVec3 &end, edict_t *passedict, int contentmask)
{	PROF2(trace0_t*) G_Trace(trace,start,mins,maxs,end,passedict,contentmask); EPROF2(3); }
#define G_Trace PSV_Trace

static int PSV_PointContents (const CVec3 &p)
{	PROF2(int)	SV_PointContents(p); EPROF2(4);	}
#define SV_PointContents PSV_PointContents

static void PPmove (pmove_t *pmove)
{	PROF; Pmove(pmove); EPROF(5);	}
#undef SV_Pmove
#define SV_Pmove PPmove

static int PSV_ModelIndex (const char *name)
{	PROF2(int)	SV_ModelIndex(name); EPROF2(6);	}
#define SV_ModelIndex PSV_ModelIndex

static int PSV_ImageIndex (const char *name)
{	PROF2(int)	SV_ImageIndex(name); EPROF2(7);	}
#define SV_ImageIndex PSV_ImageIndex

static int PSV_SoundIndex (const char *name)
{	PROF2(int)	SV_SoundIndex(name); EPROF2(8);	}
#define SV_SoundIndex PSV_SoundIndex

static void* PZ_TagMalloc (int size, int tag)
{	PROF; MEM_ALLOCATOR(size); void *res = Z_TagMalloc (size, tag); EPROF2(9);	}
#define Z_TagMalloc PZ_TagMalloc

static void PZ_Free (void *ptr)
{	PROF;	Z_Free (ptr); EPROF(10);	}
#define Z_Free PZ_Free

static void PZ_FreeTags (int tag)
{	PROF;	Z_FreeTags (tag); EPROF(11);	}
#define Z_FreeTags PZ_FreeTags

#endif // SV_PROFILE


/*-----------------------------------------------------------------------------
	Library dynamic binding
-----------------------------------------------------------------------------*/

static CDynamicLib gameLib;

static void UnloadGame ()
{
	guard(UnloadGame);
	gameLib.Free ();
	Com_DPrintf ("Free(game"DLLEXT")\n");
	unguard;
}

static game_export_t *GetGameAPI (game_import_t *parms)
{
	guard(GetGameAPI);

	//?? linux: check: used "setreuid(getuid(), getuid());" + "setegid(getgid());"
	// find library file
#define GAME_DLL	"game"CPUSTRING DLLEXT		// name of dynamic library to load
	CFile *Lib = GFileSystem->OpenFile (GAME_DLL, FS_OS);
	if (!Lib)
		return NULL;
	TString<256> DllName; DllName.sprintf ("%s/%s", Lib->Owner->name, *Lib->Name);
	delete Lib;

	if (gameLib.Load (DllName))
		Com_DPrintf ("Load(%s)\n", *DllName);
	else
	{
		appWPrintf ("GetGameAPI(%s): failed to load\n", *DllName);
		return NULL;
	}
	//!! NOTE: if game dll is on remote file system, can use FS_CopyFile() function

	typedef game_export_t* (* pGetGameApi_t)(game_import_t *);
	pGetGameApi_t pGetGameAPI;
	if (!gameLib.GetProc ("GetGameAPI", &pGetGameAPI))
	{
		UnloadGame ();
		return NULL;
	}

	guard(dll.GetGameAPI);
	return pGetGameAPI (parms);
	unguard;

	unguard;
}


static void cFwdToServer (const char *args)
{
	ExecuteCommand (va("cmd %s\n", args));
}


// dummy functions
static void D_Func0 (void) {}
static void D_Func1 (char *a) {}
static void D_Func1A (edict_t *a) {}
static void D_Func2A (char *a, qboolean b) {}
static qboolean D_ClientConnect (edict_t *a, char *b) { return true; }
static void D_Func2B (edict_t *a, char *b) {}
static void D_Func2C (edict_t *a, usercmd_t *b) {}
static void D_Func3 (const char *a, const char *b, const char *c) {}


static const char *gameCommands[] = {"wave", "inven", "kill", "use",
	"drop", "say", "say_team", "give", "god", "notarget", "noclip",
	"invuse", "invprev", "invnext", "invdrop", "weapnext", "weapprev"};

void SV_InitGameLibrary (bool dummy)
{
	static const game_import_t importFuncs = {
		SV_BroadcastPrintf, G_dprintf, G_cprintf, G_centerprintf,
		G_StartSound, SV_StartSoundOld,
		G_Configstring,
		G_error,
		SV_ModelIndex, SV_SoundIndex, SV_ImageIndex,
		G_setmodel,
		G_Trace, SV_PointContents,
		G_inPVS, G_inPHS, G_SetAreaPortalState, G_AreasConnected,
		SV_LinkEdict, SV_UnlinkEdict, SV_AreaEdicts,
		SV_Pmove,
		SV_MulticastOld, G_Unicast,
		G_WriteChar, G_WriteByte, G_WriteShort, G_WriteLong, G_WriteFloat,
		G_WriteString, G_WritePos, G_WriteDir, G_WriteAngle,
		Z_TagMalloc, Z_Free, Z_FreeTags,
		G_Cvar_Get, Cvar_Set, Cvar_ForceSet,
		SV_Argc, SV_Argv, SV_Args,
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
	// make a local copy in a case GetGameAPI() modify it (no "const" modifier for this arg ...)
	game_import_t import = importFuncs;
	ge = GetGameAPI (&import);

	if (!ge)
		Com_DropError ("failed to load game library");
	if (ge->apiversion != GAME_API_VERSION)
		Com_DropError ("game is version %d, not " STR(GAME_API_VERSION), ge->apiversion);

	guard(ge.Init);
	ge->Init ();
	unguard;

	RegisterCommand ("gz_stats", GZ_Stats_f);
	// register NULL commands for ability to complete them from console (will be forwarded to server)
	if (!DEDICATED)
		for (int i = 0; i < ARRAY_COUNT(gameCommands); i++)
			RegisterCommand (gameCommands[i], cFwdToServer);

	unguard;
}


void SV_ShutdownGameLibrary ()
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
	UnloadGame ();
	ge = NULL;

	// free tagged memory (in a case of memory leaks)
	ShutdownGameMemory ();

	UnregisterCommand ("gz_stats");
	// unregister game commands
	if (!DEDICATED)
		for (int i = 0; i < ARRAY_COUNT(gameCommands); i++)
			UnregisterCommand (gameCommands[i]);
	unguard;
}
