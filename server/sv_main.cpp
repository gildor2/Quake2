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

#include "server.h"
#include "cmodel.h"
#include "OutputDeviceMem.h"

netadr_t	master_adr[MAX_MASTERS];	// address of group servers

client_t	*sv_client;					// current client

cvar_t	*sv_paused;

cvar_t	*sv_enforcetime;

static cvar_t	*timeout;				// seconds without any message
static cvar_t	*zombietime;			// seconds to sink messages after disconnect

static cvar_t	*rcon_password;			// password for remote server commands

cvar_t *allow_download;
cvar_t *allow_download_players;
cvar_t *allow_download_models;
cvar_t *allow_download_sounds;
cvar_t *allow_download_maps;

cvar_t *sv_airaccelerate;

cvar_t	*sv_noreload;					// don't reload level state when reentering

cvar_t	*sv_deathmatch, *sv_coop, *sv_maxclients;

static cvar_t	*sv_showclamp;

static cvar_t	*hostname;
static cvar_t	*public_server;			// should heartbeats be sent

static cvar_t	*sv_reconnect_limit;	// minimum seconds between connect messages

cvar_t			*sv_extProtocol;
static cvar_t	*sv_camperSounds;
cvar_t			*sv_labels;
//static cvar_t	*sv_fps;

// forwards
static void Master_Heartbeat ();
static void Master_Shutdown ();


//============================================================================


/*
=====================
SV_DropClient

Called when the player is totally leaving the server, either willingly
or unwillingly.  This is NOT called if the entire server is quiting
or crashing.
=====================
*/
void SV_DropClient (client_t *drop, char *info)
{
	if (info)
		SV_BroadcastPrintf (PRINT_HIGH, "%s %s\n", *drop->Name, info);

	// add the disconnect
	MSG_WriteByte (&drop->netchan.message, svc_disconnect);

	if (drop->state == cs_spawned)
	{
		// call the prog function for removing a client
		// this will remove the body, among other things
		ge->ClientDisconnect (drop->edict);
	}

	if (drop->download)
	{
		delete drop->download;
		drop->download = NULL;
	}

	drop->state = cs_zombie;		// become free in a few seconds
	drop->Name[0] = 0;
}



/*
==============================================================================

CONNECTIONLESS COMMANDS

==============================================================================
*/

static void cPing (int argc, char **argv)
{
	// Just responds with an acknowledgement
	Netchan_OutOfBandPrint (NS_SERVER, net_from, "ack");
}

static void cAck (int argc, char **argv)
{
	appPrintf ("Ping acknowledge from %s\n", NET_AdrToString (&net_from));
}

/*
===============
SV_StatusString

Builds the string that is sent as heartbeats and status replies
===============
*/
static const char *SV_StatusString ()
{
	guard(SV_StatusString);
	static char	status[MAX_MSGLEN - 16];

	if (sv.attractloop) return "";

	int statusLength = appSprintf (ARRAY_ARG(status), "%s\n", Cvar_BitInfo (CVAR_SERVERINFO));
	for (int i = 0; i < sv_maxclients->integer; i++)
	{
		client_t *cl = &svs.clients[i];
		if (cl->state == cs_connected || cl->state == cs_spawned)
		{
			char player[256];
			int playerLength = appSprintf (ARRAY_ARG(player), "%d %d \"%s\"\n",
				cl->edict->client->ps.stats[STAT_FRAGS], cl->ping, *cl->Name);
			if (statusLength + playerLength >= sizeof(status))
				break;		// can't hold any more
			memcpy (status + statusLength, player, playerLength+1);
			statusLength += playerLength;
		}
	}

	return status;
	unguard;
}

static void cStatus (int argc, char **argv)
{
	// Responds with all the info that qplug or qspy can see
	Netchan_OutOfBandPrint (NS_SERVER, net_from, "print\n%s", SV_StatusString());
}


/*
================
cInfo

Responds with short info for broadcast scans
The second parameter should be the current protocol version number.
================
*/
static void cInfo (int argc, char **argv)
{
	if (sv_maxclients->integer == 1 || sv.state == ss_demo || sv.attractloop)
		return;					// ignore in single player or demoplay

	int version = atoi (argv[1]);

	if (!version)
	{
		// we should reject this packet -- this is our "info" answer to local client (loopback packet)
		Com_DPrintf ("rejected \"info\" answer\n");
		return;
	}

	char string[64];
	if (version != PROTOCOL_VERSION)
		appSprintf (ARRAY_ARG(string), "%s: wrong version %d\n", hostname->string, version);
	else
	{
		int count = 0;
		for (int i = 0; i < sv_maxclients->integer; i++)
			if (svs.clients[i].state >= cs_connected)
				count++;

		appSprintf (ARRAY_ARG(string), "%16s %8s %2d/%2d\n", hostname->string, sv.name, count, sv_maxclients->integer);
	}
	Netchan_OutOfBandPrint (NS_SERVER, net_from, "info\n%s", string);
}

/*
=================
cGetChallenge

Returns a challenge number that can be used
in a subsequent client_connect command.
We do this to prevent denial of service attacks that
flood the server with invalid connection IPs.  With a
challenge, they must give a valid IP address.
=================
*/
static void cGetChallenge (int argc, char **argv)
{
	int		i;

	int oldest = 0;
	int oldestTime = 0x7FFFFFFF;

	// see if we already have a challenge for this ip
	for (i = 0; i < MAX_CHALLENGES; i++)
	{
		if (NET_CompareBaseAdr (&net_from, &svs.challenges[i].adr))
			break;
		if (svs.challenges[i].time < oldestTime)
		{
			oldestTime = svs.challenges[i].time;
			oldest = i;
		}
	}

	if (i == MAX_CHALLENGES)
	{
		// overwrite the oldest
		svs.challenges[oldest].challenge = rand() & 0x7FFF;
		svs.challenges[oldest].adr = net_from;
		svs.challenges[oldest].time = appMilliseconds ();
		i = oldest;
	}

	// send it back
	Netchan_OutOfBandPrint (NS_SERVER, net_from, "challenge %d", svs.challenges[i].challenge);
}

/*
==================
cDirectConnect

A connection request that did not come from the master
==================
*/
static void cDirectConnect (int argc, char **argv)
{
	int		i;

	netadr_t adr = net_from;

	Com_DPrintf ("SVC_DirectConnect()\n");

	int version = atoi (argv[1]);
	if (version != PROTOCOL_VERSION)
	{
		Netchan_OutOfBandPrint (NS_SERVER, adr, "print\nServer is version %4.2f.\n", VERSION);
		Com_DPrintf ("  rejected connect from version %d\n", version);
		return;
	}

	int port = atoi (argv[2]);
	int challenge = atoi (argv[3]);

	// get userinfo
	char userinfo[MAX_INFO_STRING];
	appStrncpyz (userinfo, argv[4], sizeof(userinfo));
	// force the IP key/value pair to be in userinfo (for game-side IP filters)
	Info_SetValueForKey (userinfo, "ip", NET_AdrToString(&net_from));

	// attractloop servers are ONLY for local clients
	if (sv.attractloop)
	{
		if (!NET_IsLocalAddress (&adr))
		{
			appPrintf ("Remote connect in attract loop. Ignored.\n");
			Netchan_OutOfBandPrint (NS_SERVER, adr, "print\nConnection refused.\n");
			return;
		}
	}

	// see if the challenge is valid
	if (!NET_IsLocalAddress (&adr))
	{
		for (i = 0; i < MAX_CHALLENGES; i++)
			if (NET_CompareBaseAdr (&net_from, &svs.challenges[i].adr))
			{
				if (challenge == svs.challenges[i].challenge)
					break;		// good
				Netchan_OutOfBandPrint (NS_SERVER, adr, "print\nBad challenge.\n");
				return;
			}
		if (i == MAX_CHALLENGES)
		{
			Netchan_OutOfBandPrint (NS_SERVER, adr, "print\nNo challenge for address.\n");
			return;
		}
	}

	client_t temp, *cl;
	memset (&temp, 0, sizeof(client_t));
	client_t *newcl = NULL;

	// if there is already a slot for this ip, reuse it
	for (i = 0, cl = svs.clients; i < sv_maxclients->integer; i++,cl++)
	{
		if (cl->state == cs_free) continue;

		if (NET_CompareBaseAdr (&adr, &cl->netchan.remote_address) &&
			(cl->netchan.port == port || adr.port == cl->netchan.remote_address.port))
		{
			if (!NET_IsLocalAddress (&adr) && (svs.realtime - cl->lastconnect) < (sv_reconnect_limit->integer * 1000))
			{
				Com_DPrintf ("%s:reconnect rejected : too soon\n", NET_AdrToString (&adr));
				return;
			}
			appPrintf ("%s:reconnect\n", NET_AdrToString (&adr));
			newcl = cl;
			break;
		}
	}

	// find a client slot
	if (!newcl)
	{
		for (i = 0, cl = svs.clients; i < sv_maxclients->integer; i++,cl++)
			if (cl->state == cs_free)
			{
				newcl = cl;
				break;
			}
	}
	if (!newcl)
	{
		Netchan_OutOfBandPrint (NS_SERVER, adr, "print\nServer is full.\n");
		Com_DPrintf ("Rejected a connection.\n");
		return;
	}

	// build a new connection
	// accept the new client
	// this is the only place a client_t is ever initialized
	*newcl = temp;
	sv_client = newcl;
	int edictnum = (newcl-svs.clients)+1;
	edict_t *ent = EDICT_NUM(edictnum);
	newcl->edict = ent;
	newcl->challenge = challenge;		// save challenge for checksumming

	// get the game a chance to reject this connection or modify the userinfo
	if (!(ge->ClientConnect (ent, userinfo)))
	{
		if (*Info_ValueForKey (userinfo, "rejmsg"))
			Netchan_OutOfBandPrint (NS_SERVER, adr, "print\n%s\nConnection refused.\n",
				Info_ValueForKey (userinfo, "rejmsg"));
		else
			Netchan_OutOfBandPrint (NS_SERVER, adr, "print\nConnection refused.\n" );
		Com_DPrintf ("Game rejected a connection.\n");
		return;
	}

	// parse some info from the info strings
	newcl->Userinfo = userinfo;
	SV_UserinfoChanged (newcl);

	// check if client trying to connect with a new protocol
	newcl->newprotocol = !strcmp (argv [5], NEW_PROTOCOL_ID) && sv_extProtocol->integer;
	// send the connect packet to the client
	if (newcl->newprotocol)
	{
		int ver = atoi (argv[6]);
		if (ver != NEW_PROTOCOL_VERSION)
		{
			//?? we may also perform extended protocol checking by adding "[cl_]extprotocol" to userinfo
			Com_DPrintf ("Client used extended protocol %d != "STR(NEW_PROTOCOL_VERSION)"\n", ver);
			if (ver < NEW_PROTOCOL_VERSION)
				Netchan_OutOfBandPrint (NS_SERVER, adr, "print\n"
				S_YELLOW"Server provides newer version of extended protocol\nPlease upgrade your client\n" );
			newcl->newprotocol = false;
		}
	}
	if (newcl->newprotocol)
	{
		Com_DPrintf ("Connecting client using extended protocol\n");
		Netchan_OutOfBandPrint (NS_SERVER, adr, "client_connect "NEW_PROTOCOL_ID" "STR(NEW_PROTOCOL_VERSION));
	}
	else
		Netchan_OutOfBandPrint (NS_SERVER, adr, "client_connect");

	newcl->netchan.Setup (NS_SERVER, adr, port);
	newcl->maxPacketSize = MAX_MSGLEN_OLD;
	if (cl->newprotocol)
		newcl->maxPacketSize = MAX_MSGLEN;

	newcl->state = cs_connected;

	newcl->datagram.Init (newcl->datagram_buf, sizeof(newcl->datagram_buf));
	newcl->datagram.allowoverflow = true;
	newcl->lastmessage = svs.realtime;	// don't timeout
	newcl->lastconnect = svs.realtime;
}


// Execute command on server from remove client with redirected console output
// Output will be sent back to client as "svc_print" command
static void cRemoteCommand (int argc, char **argv)
{
	guard(SVC_RemoteCommand);

	if (!rcon_password->string[0] || strcmp (argv[1], rcon_password->string))
	{
		appPrintf ("Bad rcon from %s:\n%s\n", NET_AdrToString (&net_from), net_message.data+4);
		Netchan_OutOfBandPrint (NS_SERVER, net_from, "print\nBad rcon password\n");
	}
	else
	{
		appPrintf ("Rcon from %s:\n%s\n", NET_AdrToString (&net_from), net_message.data+4);

		// fill line with a rest of command string (cut "rcon")
		TString<256> Cmd; Cmd[0] = 0;
		for (int i = 2; i < argc; i++)
		{
			if (i > 2) Cmd += " ";
			Cmd += argv[i];
		}

		// execute command with a redirected output
		//?? may be, if message is longer, than MAX_MSGLEN_OLD, flush and continue
		//?? buffering output
		COutputDeviceMem *Mem = new COutputDeviceMem (MAX_MSGLEN_OLD-16);
		Mem->NoColors = true;		// client may not support colored texts ...
		HookOutput(Mem);
		TRY {
			if (!ExecuteCommand (Cmd))
				appWPrintf ("Bad remote command \"%s\"\n", *Cmd);
			//?? move code to Mem->Flush(); call Flush() implicitly at end too
			// if server will be restarted during active redirection, drop data
			if (sv_client && sv_client->netchan.message.maxsize)
			{
				// send redirected text
				MSG_WriteByte (&sv_client->netchan.message, svc_print);
				MSG_WriteByte (&sv_client->netchan.message, PRINT_HIGH);
				MSG_WriteString (&sv_client->netchan.message, Mem->GetText ());
			}
		} CATCH {
			UnhookOutput;
			delete Mem;
			THROW_AGAIN;
		} END_CATCH
		UnhookOutput;
		delete Mem;
	}

	unguard;
}

/*
=================
SV_ConnectionlessPacket

A connectionless packet has four leading 0xff
characters to distinguish it from a game channel.
Clients that are in the game can still send
connectionless packets.
=================
*/

static const CSimpleCommand connectionlessCmds[] = {
	{"ping", cPing},
	{"ack", cAck},
	{"status", cStatus},
	{"info", cInfo},
	{"getchallenge", cGetChallenge},
	{"connect", cDirectConnect},
	{"rcon", cRemoteCommand}
};

static void SV_ConnectionlessPacket ()
{
	guard(SV_ConnectionlessPacket);
	net_message.BeginReading ();
	MSG_ReadLong (&net_message);		// skip the -1 marker

	char *s = MSG_ReadString (&net_message);
	//?? "s" may be too long (userinfo etc)
	Com_DPrintf ("SV: packet from %s: %s\n", NET_AdrToString (&net_from), s);

	if (SV_AddressBanned (&net_from))
	{
		Com_DPrintf ("... banned\n");
		return;
	}

	if (!ExecuteCommand (s, ARRAY_ARG(connectionlessCmds)))
		appWPrintf ("Bad connectionless packet from %s: \"%s\"\n", NET_AdrToString (&net_from), s);
	unguard;
}


//============================================================================

/*
===================
SV_CalcPings

Updates the cl->ping variables
===================
*/
void SV_CalcPings ()
{
	for (int i = 0; i < sv_maxclients->integer; i++)
	{
		client_t *cl = &svs.clients[i];
		if (cl->state != cs_spawned)
			continue;

#if 0
		if (cl->lastframe > 0)
			cl->frame_latency[sv.framenum&(LATENCY_COUNTS-1)] = sv.framenum - cl->lastframe + 1;
		else
			cl->frame_latency[sv.framenum&(LATENCY_COUNTS-1)] = 0;
#endif

		int total = 0;
		int count = 0;
		for (int j = 0; j < LATENCY_COUNTS; j++)
		{
			if (cl->frame_latency[j] > 0)
			{
				count++;
				total += cl->frame_latency[j];
			}
		}
		if (!count)
			cl->ping = 0;
		else
#if 0
			cl->ping = total*100/count - 100;
#else
			cl->ping = total / count;
#endif

		// let the game dll know about the ping
		cl->edict->client->ping = cl->ping;
	}
}


/*
===================
SV_GiveMsec

Every few frames, gives all clients an allotment of milliseconds
for their command moves.  If they exceed it, assume cheating.
===================
*/
void SV_GiveMsec ()
{
	if (sv.framenum & 15)
		return;

	for (int i = 0; i < sv_maxclients->integer; i++)
	{
		client_t *cl = &svs.clients[i];
		if (cl->state == cs_free )
			continue;

		cl->commandMsec = 1800;		// 1600 + some slop
	}
}


void SV_ReadPackets ()
{
	guard(SV_ReadPackets);

	while (NET_GetPacket (NS_SERVER, &net_from, &net_message))
	{
		// check for connectionless packet (0xffffffff) first
		if (*(int *)net_message.data == -1)
		{
			SV_ConnectionlessPacket ();
			continue;
		}

		// read the qport out of the message so we can fix up
		// stupid address translating routers
		net_message.BeginReading ();
		MSG_ReadLong (&net_message);		// sequence number
		MSG_ReadLong (&net_message);		// sequence number
		int qport = MSG_ReadShort (&net_message) & 0xFFFF;

		// check for packets from connected clients
		int		i;
		client_t *cl;
		for (i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++)
		{
			if (cl->state == cs_free)
				continue;
			// compare address: ignore network port, but use qport
			if (!NET_CompareBaseAdr (&net_from, &cl->netchan.remote_address))
				continue;
			if (cl->netchan.port != qport)
				continue;
			// found a client
			if (cl->netchan.remote_address.port != net_from.port)
			{
				appWPrintf ("SV_ReadPackets: fixing up a translated port\n");
				cl->netchan.remote_address.port = net_from.port;
			}

			if (cl->netchan.Process (&net_message))
			{	// this is a valid, sequenced packet, so process it
				if (cl->state != cs_zombie)
				{
					cl->lastmessage = svs.realtime;	// don't timeout
					SV_ExecuteClientMessage (cl);
				}
			}
			break;
		}

//		if (i != sv_maxclients->integer) continue;
	}

	unguard;
}

/*
==================
SV_CheckTimeouts

If a packet has not been received from a client for timeout->value
seconds, drop the conneciton.  Server frames are used instead of
realtime to avoid dropping the local client while debugging.

When a client is normally dropped, the client_t goes into a zombie state
for a few seconds to make sure any final reliable message gets resent
if necessary
==================
*/
void SV_CheckTimeouts ()
{
	guard(SV_CheckTimeouts);
	int droppoint = svs.realtime - appRound (timeout->value * 1000);
	int zombiepoint = svs.realtime - appRound (zombietime->value * 1000);

	int		i;
	client_t *cl;
	for (i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++)
	{
		// message times may be wrong across a changelevel
		if (cl->lastmessage > svs.realtime)
			cl->lastmessage = svs.realtime;

		if (cl->state == cs_zombie && cl->lastmessage < zombiepoint)
		{
			cl->state = cs_free;	// can now be reused
			continue;
		}
		if ((cl->state == cs_connected || cl->state == cs_spawned) && cl->lastmessage < droppoint)
		{
			SV_DropClient (cl, "timed out");
			cl->state = cs_free;	// don't bother with zombie state
		}
	}
	unguard;
}

/*
================
SV_PrepWorldFrame

This has to be done before the world logic, because
player processing happens outside RunWorldFrame
================
*/
static void SV_PrepWorldFrame ()
{
	edict_t	*ent;
	int		i;

	for (i = 0; i < ge->num_edicts; i++, ent++)
	{
		ent = EDICT_NUM(i);
		// events only last for a single message
		ent->s.event = 0;
	}
}


/*=============================================================================
	Protocol extensions support
=============================================================================*/

static CVec3 shotStart, shotEnd;
static int shotLevel;


void SV_PostprocessFrame ()
{
	edict_t *ent;
	int e, i;
	client_t *cl;

	guard(SV_PostprocessFrame);

	for (e = 0; e < ge->num_edicts; e++)
	{
		ent = EDICT_NUM(e);

		// ignore ents without visible models
		if (ent->svflags & SVF_NOCLIENT)
			continue;

		if (ent->s.event == EV_FOOTSTEP || ent->s.event == EV_FALLSHORT)
		{
			CVec3	point;
			point[0] = ent->s.origin[0];
			point[1] = ent->s.origin[1];
			point[2] = ent->s.origin[2] - 64;
			trace_t trace;
			SV_Trace (trace, ent->s.origin, point, ent->bounds, ent, MASK_PLAYERSOLID|MASK_MONSTERSOLID|MASK_WATER);
			if (trace.fraction < 1)
			{
				int footsteptype = trace.surface->material - 1;

				if (footsteptype < 0)//?? || footsteptype >= MATERIAL_COUNT)	// i.e. MATERIAL_SILENT (== 0) or incorrect
					ent->s.event = 0;	// EV_FOOTSTEP ?
/*				else if (footsteptype > MATERIAL_WATER)
				{	//!! DEBUG !! (and hack) - should not happen
					developer->integer = 2;
					Com_DPrintf (S_RED"Bad material sound: idx=%d", footsteptype);
					ent->s.event = 0;
				}
*/				else if (ent->s.event == EV_FOOTSTEP)
					ent->s.event = EV_FOOTSTEP0 + footsteptype;
				else
					ent->s.event = EV_FALLSHORT0 + footsteptype;
			}
		}
	}

	// work only in deathmatch game, not in single-player or coop game
	if (sv_deathmatch->integer) //!! should be another variable; campersounds - below!! && sv_campersounds->integer)
	{
		unsigned t = appMilliseconds ();
		for (i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++)
		{
			if (cl->state != cs_spawned && cl->state != cs_connected)
			{
				cl->last_velocity2 = 0;
				continue;	// dead / not connected player
			}

			ent = cl->edict;
			pmove_state_t *pm = &ent->client->ps.pmove;

			int prev_vel = cl->last_velocity2;
			int curr_vel = pm->velocity[2];
			cl->last_velocity2 = curr_vel;

			if (cl->screaming && (curr_vel >= FALLING_SCREAM_VELOCITY1 || prev_vel >= FALLING_SCREAM_VELOCITY1
							 || pm->pm_type != PM_NORMAL))	// killed
			{	// stop scream
				SV_StartSoundNew (NULL, ent, CHAN_BODY, SV_SoundIndex ("*falling1.wav"), 0.0f, ATTN_NORM, 0);	// volume=0
				cl->screaming = false;
			}

			if (ent->s.event) continue;		// have a sound event

/*			if (pm->pm_type == PM_SPECTATOR) //!! don't works - server doesn't send info about spectators (need another way)
			{
				sfxstate = 1;
				if (cl->sfxstate != sfxstate)
				{	// just changed to spectator
					cl->sfxstate = sfxstate;
					cl->nextsfxtime = t + 1000 + (rand()&0x7FF); // ~1-3 sec.
					continue;
				}
				if (cl->nextsfxtime > t) continue; // waiting for sfx time
				cl->nextsfxtime = t + 1000 + (rand()&0x7FF);
				ent->s.event = EV_SPECTATOR0 + (rand()&3);
				appPrintf ("%i:SPECT-SFX\n",i);
			}
			else*/ if (pm->pm_type == PM_NORMAL)
			{
				CVec3	pm_origin;
				pm_origin[0] = pm->origin[0] / 8.0f;
				pm_origin[1] = pm->origin[1] / 8.0f;
				pm_origin[2] = pm->origin[2] / 8.0f;

				// check for falling sounds
				if (!cl->screaming)
				{
					if (curr_vel < FALLING_SCREAM_VELOCITY1 && prev_vel < FALLING_SCREAM_VELOCITY1)
					{
						CVec3 end = pm_origin;
						end[2] = pm_origin[2] - FALLING_SCREAM_HEIGHT_WATER;

						trace_t	trace;
						static const CBox bounds = {{-20, -20, -10}, {20, 20, 10}};
						SV_Trace (trace, pm_origin, end, bounds, NULL, CONTENTS_WATER);
						if (trace.fraction == 1.0 && !trace.startsolid)	// no water and start not in water
						{
							end[2] = pm_origin[2] - FALLING_SCREAM_HEIGHT_SOLID;
							SV_Trace (trace, pm_origin, end, bounds, NULL, CONTENTS_SOLID|CONTENTS_LAVA);
							if (trace.fraction == 1.0 || (!trace.ent && trace.plane.normal[2] < 0.5) ||
								trace.contents & CONTENTS_LAVA || trace.surface->flags & SURF_SKY)
								cl->screaming = true;
						}
					}
					if (cl->screaming)
					{
						SV_StartSoundNew (NULL, ent, CHAN_BODY, SV_SoundIndex ("*falling1.wav"), 1, ATTN_NORM, 0);
						continue;
					}
				}
				else if (curr_vel >= FALLING_SCREAM_VELOCITY1 || prev_vel >= FALLING_SCREAM_VELOCITY1)
				{	// stop scream
					if (cl->screaming)
						SV_StartSoundNew (NULL, ent, CHAN_BODY, SV_SoundIndex ("*falling1.wav"), 0.0, ATTN_NORM, 0);
					cl->screaming = false;
				}

				// check for idle (camper) sounds
				int leaf = CM_PointLeafnum (pm_origin);
				int clust = CM_LeafCluster (leaf);
				int sfxstate = 2;
				if (!clust)
					continue;	// possibly map without visinfo - no cluster partition (cannot detect campers with this way)

				if (clust != cl->lastcluster || cl->sfxstate != sfxstate || (CM_LeafContents (leaf) & MASK_WATER))
				{	// changed cluster or state - not camping
					cl->sfxstate = sfxstate;
					cl->lastcluster = clust;
					cl->nextsfxtime = t + CAMPER_TIMEOUT + rand()%CAMPER_TIMEOUT_DELTA;
					continue;
				}
				if (cl->nextsfxtime > t) continue; // waiting for sfx time
				cl->nextsfxtime = t + CAMPER_REPEAT + rand()%CAMPER_REPEAT_DELTA;
				int sfx0, sfxn;
				if (pm->pm_flags & PMF_DUCKED)
				{
					sfx0 = EV_CAMPER0;
					sfxn = 2;
				}
				else
				{
					sfx0 = EV_CAMPER0+2;
					sfxn = 7;
				}
				ent->s.event = sfx0 + rand() % sfxn;
			}
			else
			{
				cl->sfxstate = 0;
				continue;
			}
		}
	}

	unguard;
}


static client_t *FindClient (const CVec3 &origin, float maxDist2)
{
	client_t *cl;
	int		i;

//	appPrintf("find for %g %g %g dist=%g\n",VECTOR_ARG(origin),maxDist2);
	for (i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++)
	{
		edict_t *ent = cl->edict;
		if (cl->state != cs_spawned && cl->state != cs_connected)
			continue;

		pmove_state_t *pm = &ent->client->ps.pmove;
//		appPrintf("? %s");
		CVec3 pm_origin, delta;
		pm_origin[0] = pm->origin[0] / 8.0f;
		pm_origin[1] = pm->origin[1] / 8.0f;
		pm_origin[2] = pm->origin[2] / 8.0f;
		VectorSubtract (origin, pm_origin, delta);
		float dist2 = dot(delta, delta);
		if (dist2 > maxDist2)
		{
//			appPrintf(" far: %g\n", dist2);
			continue;
		}
//		appPrintf(" + %g\n", dist2);
		return cl;
	}

	return NULL;
}


sizebuf_t *SV_MulticastHook (sizebuf_t *original, sizebuf_t *ext)
{
	CVec3	v1, v2;
	const char *s;

	guard(SV_MulticastHook);

	original->BeginReading ();

	// check for interesting messages
	if (MSG_ReadByte (original) != svc_temp_entity)
		return original;

	ext->Clear ();
	byte cmd = MSG_ReadByte (original);
	switch (cmd)
	{
	case TE_RAILTRAIL:
		{
			MSG_ReadPos (original, v1);		// start
			MSG_ReadPos (original, v2);		// end

			client_t *cl = FindClient (v1, 18*18);
			int rType = 1;
			int rColor = 0;
			if (cl)
			{
				if (s = Info_ValueForKey (cl->Userinfo, "railcolor"))
				{
					rColor = atoi (s);
					rColor = bound (rColor, 0, 7);
				}
				if (s = Info_ValueForKey (cl->Userinfo, "railtype"))
				{
					rType = atoi (s);
					rType = bound (rType, 0, 3);
				}
			}
			if (!rType) return original;	// type==0 -> original rail trail

			MSG_WriteByte (ext, svc_temp_entity);
			MSG_WriteByte (ext, TE_RAILTRAIL_EXT);
			MSG_WritePos (ext, v1);
			MSG_WritePos (ext, v2);
			MSG_WriteByte (ext, rColor);
			MSG_WriteByte (ext, rType);
		}
		return ext;

	case TE_GUNSHOT:
	case TE_SPARKS:
	case TE_BULLET_SPARKS:
		{
			if (shotLevel != 2) return original;

			MSG_ReadPos (original, v1);
			MSG_ReadDir (original, v2);

			// compute reflection vector
//			appPrintf("sp: (%g %g %g) -> (%g %g %g)\n",VECTOR_ARG(v1),VECTOR_ARG(v2));//!!
			CVec3 d;
			VectorSubtract (shotStart, shotEnd, d);
			d.NormalizeFast ();
			float back = dot (d, v2);
			for (int i = 0; i < 3; i++)
				v2[i] = d[i] - 2 * (d[i] - back * v2[i]);

			MSG_WriteByte (ext, svc_temp_entity);
			MSG_WriteByte (ext, cmd);
			MSG_WritePos (ext, v1);
			MSG_WriteDir (ext, v2);
		}
		return ext;

	case TE_SPLASH:
		{
			int count = MSG_ReadByte (original);
			MSG_ReadPos (original, v1);			// pos
			int tmp = MSG_ReadByte (original);	// dir
			int type = MSG_ReadByte (original);
			if (type != SPLASH_BLUE_WATER && type != SPLASH_BROWN_WATER)
				return original;			// not bullet effect

			// find splash origin in static map splashes to avoid bullethit sounds for waterfalls etc.
			int		i;
			splash_t *spl;
			for (i = 0, spl = map_bspfile->splashes; i < map_bspfile->numSplashes; i++, spl = spl->next)
			{
				if (fabs (spl->origin[0] - v1[0]) < 1 &&
					fabs (spl->origin[1] - v1[1]) < 1 &&
					fabs (spl->origin[2] - v1[2]) < 1)		// dir is quantized, so - make inprecisious compare
					return original;
			}

			//?? can add ripple effect on water, water smoke
			MSG_WriteByte (ext, svc_temp_entity);
			MSG_WriteByte (ext, TE_SPLASH);
			MSG_WriteByte (ext, count);
			MSG_WritePos (ext, v1);
			MSG_WriteByte (ext, tmp);
			MSG_WriteByte (ext, type - SPLASH_BLUE_WATER + SPLASH_BULLET_BLUE_WATER);
		}
		return ext;
	}

	return original;
	unguard;
}


extern bool trace_skipAlpha;	//!! hack

void SV_TraceHook (trace_t &trace, const CVec3 &start, const CVec3 *mins, const CVec3 *maxs, const CVec3 &end, edict_t *passedict, int contentmask)
{
	guard(SV_TraceHook);

	static edict_t *ent;
#define RESET  { shotLevel = 0; return; }

	trace_skipAlpha = true;		//!! hack for kingpin CONTENTS_ALPHA
	if (mins || maxs) trace_skipAlpha = false;

	CBox bounds = nullBox;
	if (mins) bounds.mins = *mins;		// required for game; may be NULL
	if (maxs) bounds.maxs = *maxs;

	SV_Trace (trace, start, end, bounds, passedict, contentmask);
	trace_skipAlpha = false;	//!!
	if (!sv_extProtocol->integer) return;

	if (mins || maxs) RESET

	// for details, look game/g_weapon.c :: fire_lead()
	if (contentmask == MASK_SHOT && ((unsigned)passedict) + 4 == (unsigned)&start)
	{
		if (ent == passedict && end == shotStart)
			RESET			// shotgun shot
		shotLevel = 1;
		shotStart = end;
		ent       = passedict;
	}
	else if (shotLevel == 1)
	{
		if (passedict != ent || start != shotStart ||
			(contentmask != (MASK_SHOT|MASK_WATER) && contentmask != MASK_SHOT))
			RESET
		shotEnd = end;

		shotLevel = 2;
	}
	else if (shotLevel > 1)
	{
		if (passedict != ent) RESET
	}

#undef RESET

	unguard;
}


//=============================================================================


void SV_Frame (float msec)
{
	guard(SV_Frame);

	time_before_game = time_after_game = 0;

	// if server is not active, do nothing
	if (!svs.initialized)
		return;

//	SV_DrawTextLeft(va("time: %10d rf: %10.5f d: %10.4f ri:%10d", sv.time, svs.realtimef, msec, svs.realtime));//!!
	svs.realtimef += msec;
	svs.realtime = appFloor (svs.realtimef);

	if (!sv.attractloop)
	{
		// check timeouts
		SV_CheckTimeouts ();
	}
	// get packets from clients
	SV_ReadPackets ();
	if (sv.state == ss_dead) return;	// server was killed from one of packet (e.g. "rcon killserver")

	int frameTime = 100;	// (sv_fps->integer > 10) ? (1000 / sv_fps->integer) : 100;

	// move autonomous things around if enough time has passed
	if (svs.realtime < sv.time)
	{
		// never let the time get too far off
		if (sv.time - svs.realtime > frameTime)
		{
			if (sv_showclamp->integer)
				appPrintf ("sv lowclamp s:%d -- r:%d -> %d\n", sv.time, svs.realtime, sv.time - frameTime);
			svs.realtime  = sv.time - frameTime;
			svs.realtimef = svs.realtime;
		}
		return;
	}

	SV_ClearTexts ();

	if (!sv.attractloop)
	{
		// update ping based on the last known frame from all clients
		SV_CalcPings ();
		// give the clients some timeslices
		SV_GiveMsec ();
	}

	// let everything in the world think and move
	/*-------- SV_RunGameFrame () ---------------*/
	// we always need to bump framenum, even if we
	// don't run the world, otherwise the delta
	// compression can get confused when a client
	// has the "current" frame
	sv.framenum++;
	sv.time += frameTime;	//  = sv.framenum*100; ??

	// don't run if paused
	if (!sv_paused->integer || sv_maxclients->integer > 1)
	{
		if (com_speeds->integer)
			time_before_game = appCycles();

		guard(ge.RunFrame);
		ge->RunFrame ();
		unguard;


		// never get more than one tic behind
		if (sv.time < svs.realtime)
		{
			if (sv_showclamp->integer)
				appPrintf ("sv highclamp s:%d r:%d -> %d\n", sv.time, svs.realtime, sv.time);
			svs.realtime  = sv.time;
			svs.realtimef = sv.time;
		}
		if (com_speeds->integer)
			time_after_game = appCycles();
	}

	// if extended protocol used, recalculate footstep sounds, mdx/mdl/md3 frames etc.
	if (sv_extProtocol->integer && !sv.attractloop)
		SV_PostprocessFrame ();

	// send messages back to the clients that had packets read this frame
	SV_SendClientMessages ();

	// save the entire world state if recording a serverdemo
	SV_RecordDemoMessage ();

	// send a heartbeat to the master if needed
	Master_Heartbeat ();

	// clear teleport flags, etc for next frame
	SV_PrepWorldFrame ();

	unguard;
}

//============================================================================

/*
================
Master_Heartbeat

Send a message to the master every few minutes to
let it know we are alive, and log information
================
*/
#define	HEARTBEAT_SECONDS	300
static void Master_Heartbeat ()
{
	if (!DEDICATED)
		return;		// only dedicated servers send heartbeats

	if (!public_server->integer)
		return;		// a private dedicated game

	// check for time wraparound
	if (svs.last_heartbeat > svs.realtime)
		svs.last_heartbeat = svs.realtime;

	if (svs.realtime - svs.last_heartbeat < HEARTBEAT_SECONDS*1000)
		return;		// not time to send yet

	svs.last_heartbeat = svs.realtime;

	// send the same string that we would give for a status OOB command
	const char *string = SV_StatusString();

	// send to group master
	for (int i = 0; i < MAX_MASTERS; i++)
		if (master_adr[i].port)
		{
			appPrintf ("Sending heartbeat to %s\n", NET_AdrToString (&master_adr[i]));
			Netchan_OutOfBandPrint (NS_SERVER, master_adr[i], "heartbeat\n%s", string);
		}
}

/*
=================
Master_Shutdown

Informs all masters that this server is going down
=================
*/
static void Master_Shutdown (void)
{
	if (!DEDICATED) return;						// only dedicated servers send heartbeats
	if (!public_server->integer) return;		// a private dedicated game

	// send to group master
	for (int i = 0; i < MAX_MASTERS; i++)
		if (master_adr[i].port)
		{
			if (i != 0)
				appPrintf ("Sending heartbeat to %s\n", NET_AdrToString (&master_adr[i]));
			Netchan_OutOfBandPrint (NS_SERVER, master_adr[i], "shutdown");
		}
}

//============================================================================


/*
=================
SV_UserinfoChanged

Pull specific info from a newly changed userinfo string
into a more C freindly form.
=================
*/
void SV_UserinfoChanged (client_t *cl)
{
	const char *val;

	// call game code to allow overrides
	ge->ClientUserinfoChanged (cl->edict, cl->Userinfo);

	// name
	cl->Name = Info_ValueForKey (cl->Userinfo, "name");
	// mask off high bit
	for (int i = 0; i < sizeof(cl->Name); i++) cl->Name[i] &= 127;	//??

	// rate
	val = Info_ValueForKey (cl->Userinfo, "rate");
	if (strlen(val))
	{
		int i = atoi(val);
		cl->rate = bound(i, 100, 15000);
	}
	else
		cl->rate = 5000;

	// msg
	val = Info_ValueForKey (cl->Userinfo, "msg");
	if (strlen(val))
	{
		cl->messagelevel = atoi(val);
	}
}


//============================================================================

/*
===============
SV_Init

Only called at program startup, not for each game
===============
*/

void SV_InitVars ()
{
CVAR_BEGIN(vars)
	// LATCH cvars (note: all are SERVERINFO)
	CVAR_FULL(&sv_deathmatch, "deathmatch", "0", CVAR_SERVERINFO|CVAR_LATCH),
	CVAR_FULL(&sv_coop, "coop", "0", CVAR_SERVERINFO|CVAR_LATCH),
	CVAR_FULL(&sv_maxclients, "maxclients", "1", CVAR_SERVERINFO|CVAR_LATCH),
	CVAR_VAR(sv_extProtocol, 1, CVAR_SERVERINFO|CVAR_LATCH),
	// cvars from outside the server: here for LATCH updating only
	//?? may be, move this cvars here COMPLETELY?
	CVAR_FULL(NULL, "cheats", "", CVAR_SERVERINFO|CVAR_LATCH|CVAR_NODEFAULT),
	CVAR_FULL(NULL, "game", "", CVAR_SERVERINFO|CVAR_LATCH|CVAR_NODEFAULT),
	// other SERVERINFO vars
	CVAR_NULL(fraglimit,  0, CVAR_SERVERINFO),
	CVAR_NULL(timelimit,  0, CVAR_SERVERINFO),
	CVAR_VAR(hostname, noname, CVAR_SERVERINFO),
	CVAR_NULL(dmflags, 16, CVAR_SERVERINFO),		//?? old default value: 0 for game, DF_INSTANT_ITEMS (16) for server
	CVAR_VAR(sv_camperSounds, 1, CVAR_SERVERINFO),
	// read-only SERVERINFO
	CVAR_FULL(NULL, "protocol", STR(PROTOCOL_VERSION), CVAR_SERVERINFO|CVAR_NOSET),
	// non-archive LATCH vars
	CVAR_VAR(sv_airaccelerate, 0, CVAR_LATCH),		// send: CS_AIRACCEL -> CL_PredictMovement() -> pm_airaccelerate;
													// no other use (including server/game!)

	CVAR_VAR(timeout, 125, 0),
	CVAR_VAR(zombietime, 2, 0),

	CVAR_FULL(&rcon_password, "rcon_password", "", 0),
	CVAR_NULL(skill, 1, 0),

	CVAR_FULL(&sv_showclamp, "showclamp", "0", 0),
	CVAR_FULL(&sv_paused, "paused", "0", CVAR_CHEAT),
	CVAR_VAR(allow_download, 1, CVAR_ARCHIVE),
	CVAR_VAR(allow_download_players, 0, CVAR_ARCHIVE),
	CVAR_VAR(allow_download_models, 1, CVAR_ARCHIVE),
	CVAR_VAR(allow_download_sounds, 1, CVAR_ARCHIVE),
	CVAR_VAR(allow_download_maps, 1, CVAR_ARCHIVE),

	CVAR_VAR(sv_enforcetime, 0, 0),					//??
	CVAR_VAR(sv_noreload, 0, 0),					//??

	CVAR_FULL(&public_server, "public", "0", 0),

	CVAR_VAR(sv_reconnect_limit, 3, CVAR_ARCHIVE),

	CVAR_VAR(sv_labels, 0, CVAR_CHEAT)
//	CVAR_VAR(sv_fps, 20, 0)	// archive/serverinfo ??
CVAR_END
	Cvar_GetVars (ARRAY_ARG(vars));
}

void SV_Init ()
{
	guard(SV_Init);
	SV_InitVars ();
	SV_InitCommands ();
	unguard;
}

/*
================
SV_Shutdown

Called when each game quits, and before exitting application
================
*/
void SV_Shutdown (const char *finalmsg, bool reconnect)
{
	guard(SV_Shutdown);

	if (svs.clients)
	// NOTE: this required even for attractloop: if disable, when demo finishes, client will not be automatically disconnected
	{
		int			i;
		client_t	*cl;

		// Send a final message to all connected clients before the server goes down.
		//  The messages are sent immediately, not just stuck on the outgoing message
		// list, because the server is going to totally exit after returning from this function.
		for (i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++)
		{
			if (cl->state < cs_connected)
				continue;

			net_message.Clear ();
			MSG_WriteByte (&net_message, svc_print);
			MSG_WriteByte (&net_message, PRINT_HIGH);
			if (cl->newprotocol && !reconnect)	// colorize exit message
				MSG_WriteString (&net_message, va(S_RED"%s", finalmsg));
			else
				MSG_WriteString (&net_message, finalmsg);
			MSG_WriteByte (&net_message, reconnect ? svc_reconnect : svc_disconnect);

			cl->netchan.Transmit (net_message.data, net_message.cursize);
		}
	}

	Master_Shutdown ();
	SV_ShutdownGameLibrary ();

	// free current level
	if (sv.rdemofile)
		delete sv.rdemofile;
	memset (&sv, 0, sizeof(sv));
	Com_SetServerState (ss_dead);	// == 0

	// free server static data
	sv_client = NULL;
	if (svs.clients) delete svs.clients;
	if (svs.client_entities) delete svs.client_entities;
	if (svs.wdemofile)
		fclose (svs.wdemofile);
	memset (&svs, 0, sizeof(svs));

	SV_InitVars ();	// called for unlocking latched vars
	Cvar_ForceSet ("nointro", "1");

	unguard;
}
