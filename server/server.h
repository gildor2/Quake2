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

#include "qcommon.h"
#include "protocol.h"

// need qboolean declaration for game library interface
typedef unsigned int		qboolean;

#include "game.h"

//=============================================================================

#define	MAX_MASTERS	8				// max recipients for heartbeat packets


typedef struct
{
	server_state_t	state;				// precache commands are only valid during load

	bool		attractloop;			// running cinematics and demos for the local system only
	bool		loadgame;				// client begins should reuse existing entity

	unsigned	time;					// always sv.framenum * 100; msec
	int			framenum;

	char		name[MAX_QPATH];		// map name, or cinematic name
	cmodel_t	*models[MAX_MODELS];

	char		configstrings[MAX_CONFIGSTRINGS][MAX_QPATH];
	entityStateEx_t	baselines[MAX_EDICTS];

	// the multicast buffer is used to send a message to a set of clients
	// it is only used to marshall data until SV_Multicast is called
	sizebuf_t	multicast;
	byte		multicast_buf[MAX_MSGLEN];
	sizebuf_t	multicastNew;
	byte		multicast_bufNew[MAX_MSGLEN];

	// demo server information
	QFILE		*rdemofile;				// reading demos
} server_t;

#define EDICT_NUM(n) ((edict_t *)((byte *)ge->edicts + ge->edict_size*(n)))
#define NUM_FOR_EDICT(e) ( ((byte *)(e)-(byte *)ge->edicts ) / ge->edict_size)


typedef enum
{
	cs_free,							// can be reused for a new connection
	cs_zombie,							// client has been disconnected, but don't reuse
										// connection for a couple seconds
	cs_connected,						// has been assigned to a client_t, but not in game yet
	cs_spawned							// client is fully in game
} client_state_t;

typedef struct
{
	int			areabytes;
	byte		areabits[MAX_MAP_AREAS/8]; // portalarea visibility bits
	player_state_t ps;
	int			num_entities;
	int			first_entity;			// into the circular sv_packet_entities[]
	int			senttime;				// for ping calculations
} client_frame_t;

#define	LATENCY_COUNTS	16
#define	RATE_MESSAGES	10

typedef struct
{
	client_state_t state;

	char		userinfo[MAX_INFO_STRING]; // name, etc

	int			lastframe;				// for delta compression
	usercmd_t	lastcmd;				// for filling in big drops

	int			commandMsec;			// every seconds this is reset, if user
										// commands exhaust it, assume time cheating

	int			frame_latency[LATENCY_COUNTS];
	int			ping;

	int			message_size[RATE_MESSAGES]; // used to rate drop packets
	int			rate;
	int			surpressCount;			// number of messages rate supressed

	edict_t		*edict;					// EDICT_NUM(clientnum+1)
	char		name[32];				// extracted from userinfo, high bits masked
	int			messagelevel;			// for filtering printed messages

	// The datagram is written to by sound calls, prints, temp ents, etc.
	// It can be harmlessly overflowed.
	sizebuf_t	datagram;
	byte		datagram_buf[MAX_MSGLEN];

	client_frame_t frames[UPDATE_BACKUP]; // updates can be delta'd from here

	byte		*download;				// file being downloaded
	unsigned	downloadsize;			// total bytes (can't use EOF because of paks)
	unsigned	downloadcount;			// bytes sent

	int			lastmessage;			// sv.framenum when packet was last received
	int			lastconnect;

	int			challenge;				// challenge of this user, randomly generated

	netchan_t	netchan;

	//------ extended protocol fields --------
	int			maxPacketSize;
	bool		newprotocol;			// true if client uses a new communication protocol
	// stuff for anti-camper sounds
	int			lastcluster;
	int			nextsfxtime;
	int			sfxstate;				// 0-none, 1-spectator (don't works now!!), 2-normal (may be, camper)
	// falling sounds
	int			last_velocity2;
	bool		screaming;
} client_t;

// a client can leave the server in one of four ways:
// dropping properly by quiting or disconnecting
// timing out if no valid messages are received for timeout.value seconds
// getting kicked off by the server operator
// a program error, like an overflowed reliable buffer

//=============================================================================

// MAX_CHALLENGES is made large to prevent a denial
// of service attack that could cycle all of them
// out before legitimate users connected
#define	MAX_CHALLENGES	1024

typedef struct
{
	netadr_t	adr;
	int			challenge;
	int			time;
} challenge_t;


typedef struct
{
	bool		initialized;				// sv_init has completed
	int			realtime;					// always increasing, no clamping, etc
	double		realtimef;					// same as "realtime", but more precisious (may be fractional, when timescale < 1)

	char		mapcmd[128];				// ie: *intro.cin+base

	int			spawncount;					// incremented each server start
											// used to check late spawns

	client_t	*clients;					// [maxclients->integer];
	int			num_client_entities;		// maxclients->integer*UPDATE_BACKUP*MAX_PACKET_ENTITIES
	int			next_client_entities;		// next client_entity to use
	entityStateEx_t	*client_entities;		// [num_client_entities]

	int			last_heartbeat;

	challenge_t	challenges[MAX_CHALLENGES];	// to prevent invalid IPs from connecting

	// serverrecord values
	FILE		*wdemofile;					// writting demos
	sizebuf_t	demo_multicast;
	byte		demo_multicast_buf[MAX_MSGLEN];
} server_static_t;

//=============================================================================

extern	netadr_t	master_adr[MAX_MASTERS];	// address of the master server

extern	server_static_t	svs;				// persistant server info
extern	server_t		sv;					// local server

extern	cvar_t	*sv_paused;
extern	cvar_t	*maxclients;
extern	cvar_t	*sv_noreload;			// don't reload level state when reentering
extern	cvar_t	*sv_airaccelerate;		// don't reload level state when reentering
extern	cvar_t	*sv_extProtocol;
extern	cvar_t	*sv_labels;

extern	cvar_t	*sv_enforcetime;

extern	client_t	*sv_client;
extern	edict_t		*sv_player;

//===========================================================

//
// sv_main.c
//
void SV_DropClient (client_t *drop, char *info);

int SV_ModelIndex (char *name);
int SV_SoundIndex (char *name);
int SV_ImageIndex (char *name);

void SV_WriteClientdataToMessage (client_t *client, sizebuf_t *msg);

void SV_ExecuteUserCommand (char *s);
void SV_InitOperatorCommands (void);

void SV_SendServerinfo (client_t *client);
void SV_UserinfoChanged (client_t *cl);

sizebuf_t *SV_MulticastHook (sizebuf_t *original, sizebuf_t *ext);
trace_t SV_TraceHook (const CVec3 &start, const CVec3 *mins, const CVec3 *maxs, const CVec3 &end, edict_t *passedict, int contentmask);


//
// sv_init.c
//
void SV_InitGame (void);
void SV_Map (bool attractloop, const char *levelstring, bool loadgame);


//
// sv_send.c
//
void SV_BeginRedirect (void);

void SV_DemoCompleted (void);
void SV_SendClientMessages (void);

// send message to all clients
void SV_MulticastOld (const CVec3 &origin, multicast_t to);
// send message only to clients with a new protocol
void SV_MulticastNew (const CVec3 &origin, multicast_t to);
void SV_StartSoundOld (const CVec3 *origin, edict_t *entity, int channel, int soundindex, float volume, float attenuation, float timeofs);
void SV_StartSoundNew (const CVec3 *origin, edict_t *entity, int channel, int soundindex, float volume, float attenuation, float timeofs);
void SV_ClientPrintf (client_t *cl, int level, const char *fmt, ...);
void SV_BroadcastPrintf (int level, const char *fmt, ...);
void SV_BroadcastCommand (const char *fmt, ...);

//
// sv_user.c
//
void SV_Nextserver (void);
void SV_ExecuteClientMessage (client_t *cl);

//
// sv_ccmds.c
//
bool SV_AddressBanned (netadr_t *a);
void SV_ReadLevelFile (void);

//
// sv_ents.c
//
void SV_WriteFrameToClient (client_t *client, sizebuf_t *msg);
void SV_RecordDemoMessage (void);


void SV_Error (char *error, ...);

//
// sv_game.c
//
extern	game_export_t	*ge;

void SV_InitGameLibrary (bool dummy);
void SV_ShutdownGameLibrary (void);
void SV_InitEdict (edict_t *e);



//============================================================

//
// sv_world.c
//

//
// high level object sorting to reduce interaction tests
//

void SV_ClearWorld (void);
// called after the world model has been loaded, before linking any entities

void SV_UnlinkEdict (edict_t *ent);
// call before removing an entity, and before trying to move one,
// so it doesn't clip against itself

void SV_LinkEdict (edict_t *ent);
// Needs to be called any time an entity changes origin, mins, maxs,
// or solid.  Automatically unlinks if needed.
// sets ent->v.absmin and ent->v.absmax
// sets ent->leafnums[] for pvs determination even if the entity
// is not solid

int SV_AreaEdicts (const CVec3 &mins, const CVec3 &maxs, edict_t **list, int maxcount, int areatype);
// fills in a table of edict pointers with edicts that have bounding boxes
// that intersect the given area.  It is possible for a non-axial bmodel
// to be returned that doesn't actually intersect the area on an exact
// test.
// returns the number of pointers filled in
// ??? does this always return the world?

//===================================================================

//
// functions that interact with everything apropriate
//
int SV_PointContents (const CVec3 &p);
// returns the CONTENTS_* value from the world at the given point.
// Also check entities, to allow moving liquids


void SV_Trace (trace_t &tr, const CVec3 &start, const CVec3 &end, const CBox &bounds, edict_t *passedict, int contentmask);
// bounds are relative

// if the entire move stays in a solid volume, trace.allsolid will be set,
// trace.startsolid will be set, and trace.fraction will be 0

// if the starting point is in a solid, it will be allowed to move out
// to an open area

// passedict is explicitly excluded from clipping checks (normally NULL)


//===================================================================
//
// sv_tokenize.cpp
//
void	SV_TokenizeString (const char *text);
int		SV_Argc (void);
char *	SV_Argv (int arg);
char *	SV_Args (void);

//
// sv_text.cpp
//
void SV_ClearTexts ();
void SV_DrawTexts ();
void SV_DrawTextLeft (const char *text, unsigned rgba = 0xFFFFFFFF);
void SV_DrawTextRight (const char *text, unsigned rgba = 0xFFFFFFFF);
void SV_DrawText3D (const CVec3 &pos, const char *text, unsigned rgba = 0xFFFFFFFF);

//
// sv_anim.cpp
//
void SV_ComputeAnimation (player_state_t *ps, entityStateEx_t &ent, entityStateEx_t *oldent, edict_t *edict);


//------------- Constants for new protocol ---------------------

// Camper sounds: value = [CAMPER_XXX, CAMPER_XXX+CAMPER_XXX_DELTA]
#define CAMPER_TIMEOUT			30000
#define CAMPER_TIMEOUT_DELTA	15000
#define CAMPER_REPEAT			15000
#define CAMPER_REPEAT_DELTA		15000

#define FALLING_SCREAM_VELOCITY1		-4000
#define FALLING_SCREAM_VELOCITY2		-6000
#define FALLING_SCREAM_HEIGHT_SOLID		400
#define FALLING_SCREAM_HEIGHT_WATER		600
