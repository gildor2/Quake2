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

netadr_t	master_adr[MAX_MASTERS];	// address of group servers

client_t	*sv_client;					// current client

cvar_t	*sv_paused;
static cvar_t	*sv_timedemo;

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

cvar_t	*maxclients;					// FIXME: rename sv_maxclients
static cvar_t	*sv_showclamp;

static cvar_t	*hostname;
static cvar_t	*public_server;			// should heartbeats be sent

static cvar_t	*sv_reconnect_limit;	// minimum seconds between connect messages

cvar_t			*sv_extProtocol;
static cvar_t	*sv_camperSounds;
//static cvar_t	*sv_fps;

void Master_Heartbeat (void);
void Master_Shutdown (void);


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
		SV_BroadcastPrintf (PRINT_HIGH, "%s %s\n", drop->name, info);

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
		FS_FreeFile (drop->download);
		drop->download = NULL;
	}

	drop->state = cs_zombie;		// become free in a few seconds
	drop->name[0] = 0;
}



/*
==============================================================================

CONNECTIONLESS COMMANDS

==============================================================================
*/

/*
===============
SV_StatusString

Builds the string that is sent as heartbeats and status replies
===============
*/
char *SV_StatusString (void)
{
	char	player[256];
	static char	status[MAX_MSGLEN - 16];
	client_t *cl;
	int		i, statusLength, playerLength;

	statusLength = Com_sprintf (status, sizeof(status), "%s\n", Cvar_Serverinfo ());
	for (i = 0; i < maxclients->integer; i++)
	{
		cl = &svs.clients[i];
		if (cl->state == cs_connected || cl->state == cs_spawned)
		{
			Com_sprintf (player, sizeof(player), "%d %d \"%s\"\n",
				cl->edict->client->ps.stats[STAT_FRAGS], cl->ping, cl->name);
			playerLength = strlen(player);
			if (statusLength + playerLength >= sizeof(status))
				break;		// can't hold any more
			strcpy (status + statusLength, player);
			statusLength += playerLength;
		}
	}

	return status;
}


/*
================
SVC_Info

Responds with short info for broadcast scans
The second parameter should be the current protocol version number.
================
*/
void SVC_Info (void)
{
	char	string[64];
	int		version;

	if (maxclients->integer == 1 || sv.state == ss_demo)
		return;					// ignore in single player or demoplay

	version = atoi (Cmd_Argv(1));

	if (!version)
	{	// we should reject this packet -- this is our "info" answer to local client
		Com_DPrintf ("rejected \"info\" answer\n");
		return;
	}

	if (version != PROTOCOL_VERSION)
		Com_sprintf (string, sizeof(string), "%s: wrong version %d\n", hostname->string, version);
	else
	{
		int		i, count;

		count = 0;
		for (i = 0; i < maxclients->integer; i++)
			if (svs.clients[i].state >= cs_connected)
				count++;

		Com_sprintf (string, sizeof(string), "%16s %8s %2d/%2d\n", hostname->string, sv.name, count, maxclients->integer);
	}
	Netchan_OutOfBandPrint (NS_SERVER, net_from, "info\n%s", string);
}

/*
=================
SVC_GetChallenge

Returns a challenge number that can be used
in a subsequent client_connect command.
We do this to prevent denial of service attacks that
flood the server with invalid connection IPs.  With a
challenge, they must give a valid IP address.
=================
*/
void SVC_GetChallenge (void)
{
	int		i, oldest, oldestTime;

	oldest = 0;
	oldestTime = 0x7FFFFFFF;

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
		svs.challenges[oldest].time = curtime;
		i = oldest;
	}

	// send it back
	Netchan_OutOfBandPrint (NS_SERVER, net_from, "challenge %d", svs.challenges[i].challenge);
}

/*
==================
SVC_DirectConnect

A connection request that did not come from the master
==================
*/
void SVC_DirectConnect (void)
{
	char		userinfo[MAX_INFO_STRING];
	netadr_t	adr;
	int			i;
	client_t	*cl, *newcl;
	client_t	temp;
	edict_t		*ent;
	int			edictnum;
	int			version, port, challenge;

	adr = net_from;

	Com_DPrintf ("SVC_DirectConnect()\n");

	version = atoi(Cmd_Argv(1));
	if (version != PROTOCOL_VERSION)
	{
		Netchan_OutOfBandPrint (NS_SERVER, adr, "print\nServer is version %4.2f.\n", VERSION);
		Com_DPrintf ("  rejected connect from version %d\n", version);
		return;
	}

	port = atoi(Cmd_Argv(2));
	challenge = atoi(Cmd_Argv(3));

	Q_strncpyz (userinfo, Cmd_Argv(4), sizeof(userinfo));

	// force the IP key/value pair so the game can filter based on ip
	Info_SetValueForKey (userinfo, "ip", NET_AdrToString(&net_from));

	// attractloop servers are ONLY for local clients
	if (sv.attractloop)
	{
		if (!NET_IsLocalAddress (&adr))
		{
			Com_Printf ("Remote connect in attract loop.  Ignored.\n");
			Netchan_OutOfBandPrint (NS_SERVER, adr, "print\nConnection refused.\n");
			return;
		}
	}

	// see if the challenge is valid
	if (!NET_IsLocalAddress (&adr))
	{
		for (i = 0; i < MAX_CHALLENGES; i++)
		{
			if (NET_CompareBaseAdr (&net_from, &svs.challenges[i].adr))
			{
				if (challenge == svs.challenges[i].challenge)
					break;		// good
				Netchan_OutOfBandPrint (NS_SERVER, adr, "print\nBad challenge.\n");
				return;
			}
		}
		if (i == MAX_CHALLENGES)
		{
			Netchan_OutOfBandPrint (NS_SERVER, adr, "print\nNo challenge for address.\n");
			return;
		}
	}

	memset (&temp, 0, sizeof(client_t));
	newcl = NULL;

	// if there is already a slot for this ip, reuse it
	for (i = 0, cl = svs.clients; i < maxclients->integer; i++,cl++)
	{
		if (cl->state == cs_free) continue;

		if (NET_CompareBaseAdr (&adr, &cl->netchan.remote_address) &&
			(cl->netchan.qport == port || adr.port == cl->netchan.remote_address.port))
		{
			if (!NET_IsLocalAddress (&adr) && (svs.realtime - cl->lastconnect) < (sv_reconnect_limit->integer * 1000))
			{
				Com_DPrintf ("%s:reconnect rejected : too soon\n", NET_AdrToString (&adr));
				return;
			}
			Com_Printf ("%s:reconnect\n", NET_AdrToString (&adr));
			newcl = cl;
			break;
		}
	}

	// find a client slot
	if (!newcl)
	{
		for (i = 0, cl = svs.clients; i < maxclients->integer; i++,cl++)
		{
			if (cl->state == cs_free)
			{
				newcl = cl;
				break;
			}
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
	memcpy (newcl, &temp, sizeof(client_t));
	sv_client = newcl;
	edictnum = (newcl-svs.clients)+1;
	ent = EDICT_NUM(edictnum);
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
	strncpy (newcl->userinfo, userinfo, sizeof(newcl->userinfo)-1);
	SV_UserinfoChanged (newcl);

	// check if client trying to connect with a new protocol
	newcl->newprotocol = !strcmp (Cmd_Argv (5), NEW_PROTOCOL_ID) && sv_extProtocol->integer;
	// send the connect packet to the client
	if (newcl->newprotocol)
	{
		Com_DPrintf ("Connecting client using extended protocol\n");
		Netchan_OutOfBandPrint (NS_SERVER, adr, "client_connect %s", NEW_PROTOCOL_ID);
	}
	else
		Netchan_OutOfBandPrint (NS_SERVER, adr, "client_connect");

	Netchan_Setup (NS_SERVER, &newcl->netchan, adr, port);
	newcl->maxPacketSize = MAX_MSGLEN_OLD;
	if (cl->newprotocol && (adr.type == NA_IP || adr.type == NA_LOOPBACK))
		newcl->maxPacketSize = MAX_MSGLEN;

	newcl->state = cs_connected;

	SZ_Init (&newcl->datagram, newcl->datagram_buf, sizeof(newcl->datagram_buf));
	newcl->datagram.allowoverflow = true;
	newcl->lastmessage = svs.realtime;	// don't timeout
	newcl->lastconnect = svs.realtime;
}


int Rcon_Validate (void)
{
	if (!strlen (rcon_password->string))
		return 0;

	if (strcmp (Cmd_Argv(1), rcon_password->string) )
		return 0;

	return 1;
}

/*
===============
SVC_RemoteCommand

A client issued an rcon command.
Shift down the remaining args
Redirect all printfs
===============
*/
void SVC_RemoteCommand (void)
{
	int		i;
	char	remaining[1024];

	i = Rcon_Validate ();

	if (i == 0)
		Com_Printf ("Bad rcon from %s:\n%s\n", NET_AdrToString (&net_from), net_message.data+4);
	else
		Com_Printf ("Rcon from %s:\n%s\n", NET_AdrToString (&net_from), net_message.data+4);

	Com_BeginRedirect (RD_PACKET, sv_outputbuf, SV_OUTPUTBUF_LENGTH, SV_FlushRedirect);

	if (!Rcon_Validate ())
	{
		Com_WPrintf ("Bad rcon_password.\n");
	}
	else
	{
		remaining[0] = 0;

		for (i = 2; i < Cmd_Argc(); i++)
		{
			strcat (remaining, Cmd_Argv(i));
			strcat (remaining, " ");
		}

		Cmd_ExecuteString (remaining);
	}

	Com_EndRedirect ();
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
void SV_ConnectionlessPacket (void)
{
	char	*s, *c;

	MSG_BeginReading (&net_message);
	MSG_ReadLong (&net_message);		// skip the -1 marker

	s = MSG_ReadString (&net_message);

	Cmd_TokenizeString (s, false);

	c = Cmd_Argv(0);
	Com_DPrintf ("Packet %s : %s\n", NET_AdrToString (&net_from), c);
	if (SV_AddressBanned (&net_from))
	{
		Com_DPrintf ("... banned\n");
		return;
	}

	if (!strcmp (c, "ping"))
	{
		// Just responds with an acknowledgement
		Netchan_OutOfBandPrint (NS_SERVER, net_from, "ack");
	}
	else if (!strcmp (c, "ack"))
		Com_Printf ("Ping acknowledge from %s\n", NET_AdrToString (&net_from));
	else if (!strcmp (c, "status"))
	{
		// Responds with all the info that qplug or qspy can see
		Netchan_OutOfBandPrint (NS_SERVER, net_from, "print\n%s", SV_StatusString());
#if 0
		Com_BeginRedirect (RD_PACKET, sv_outputbuf, SV_OUTPUTBUF_LENGTH, SV_FlushRedirect);
		Com_Printf (SV_StatusString());
		Com_EndRedirect ();
#endif
	}
	else if (!strcmp (c, "info"))
		SVC_Info ();
	else if (!strcmp (c, "getchallenge"))
		SVC_GetChallenge ();
	else if (!strcmp (c, "connect"))
		SVC_DirectConnect ();
	else if (!strcmp (c, "rcon"))
		SVC_RemoteCommand ();
	else
		Com_WPrintf ("Bad connectionless packet from %s: \"%s\"\n", NET_AdrToString (&net_from), s);
}


//============================================================================

/*
===================
SV_CalcPings

Updates the cl->ping variables
===================
*/
void SV_CalcPings (void)
{
	int			i, j;
	client_t	*cl;
	int			total, count;

	for (i = 0; i < maxclients->integer; i++)
	{
		cl = &svs.clients[i];
		if (cl->state != cs_spawned )
			continue;

#if 0
		if (cl->lastframe > 0)
			cl->frame_latency[sv.framenum&(LATENCY_COUNTS-1)] = sv.framenum - cl->lastframe + 1;
		else
			cl->frame_latency[sv.framenum&(LATENCY_COUNTS-1)] = 0;
#endif

		total = 0;
		count = 0;
		for (j = 0; j < LATENCY_COUNTS; j++)
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
void SV_GiveMsec (void)
{
	int			i;
	client_t	*cl;

	if (sv.framenum & 15)
		return;

	for (i = 0; i < maxclients->integer; i++)
	{
		cl = &svs.clients[i];
		if (cl->state == cs_free )
			continue;

		cl->commandMsec = 1800;		// 1600 + some slop
	}
}


/*
=================
SV_ReadPackets
=================
*/
void SV_ReadPackets (void)
{
	int			i;
	client_t	*cl;
	int			qport;

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
		MSG_BeginReading (&net_message);
		MSG_ReadLong (&net_message);		// sequence number
		MSG_ReadLong (&net_message);		// sequence number
		qport = MSG_ReadShort (&net_message) & 0xFFFF;

		// check for packets from connected clients
		for (i = 0, cl = svs.clients; i < maxclients->integer; i++, cl++)
		{
			if (cl->state == cs_free)
				continue;
			if (!NET_CompareBaseAdr (&net_from, &cl->netchan.remote_address))
				continue;
			if (cl->netchan.qport != qport)
				continue;
			if (cl->netchan.remote_address.port != net_from.port)
			{
				Com_Printf ("SV_ReadPackets: fixing up a translated port\n");
				cl->netchan.remote_address.port = net_from.port;
			}

			if (Netchan_Process (&cl->netchan, &net_message))
			{	// this is a valid, sequenced packet, so process it
				if (cl->state != cs_zombie)
				{
					cl->lastmessage = svs.realtime;	// don't timeout
					SV_ExecuteClientMessage (cl);
				}
			}
			break;
		}

//		if (i != maxclients->integer) continue;
	}
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
void SV_CheckTimeouts (void)
{
	int		i;
	client_t	*cl;
	int			droppoint;
	int			zombiepoint;

	droppoint = svs.realtime - 1000 * timeout->value;
	zombiepoint = svs.realtime - 1000 * zombietime->value;

	for (i = 0, cl = svs.clients; i < maxclients->integer; i++, cl++)
	{
		// message times may be wrong across a changelevel
		if (cl->lastmessage > svs.realtime)
			cl->lastmessage = svs.realtime;

		if (cl->state == cs_zombie
		&& cl->lastmessage < zombiepoint)
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
}

/*
================
SV_PrepWorldFrame

This has to be done before the world logic, because
player processing happens outside RunWorldFrame
================
*/
static void SV_PrepWorldFrame (void)
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

static vec3_t shotStart, shotEnd;
static int shotLevel;


void SV_PostprocessFrame (void)
{
	edict_t *ent;
	int e, i, t;
	client_t *cl;

	for (e = 0; e < ge->num_edicts; e++)
	{
		ent = EDICT_NUM(e);

		// ignore ents without visible models
		if (ent->svflags & SVF_NOCLIENT)
			continue;

		if (ent->s.event == EV_FOOTSTEP || ent->s.event == EV_FALLSHORT)
		{
			trace_t trace;
			vec3_t point;
			int footsteptype;

			point[0] = ent->s.origin[0];
			point[1] = ent->s.origin[1];
			point[2] = ent->s.origin[2] - 64;
			SV_Trace (&trace, ent->s.origin, ent->mins, ent->maxs, point, ent, MASK_PLAYERSOLID|MASK_MONSTERSOLID|MASK_WATER);
			if (trace.fraction < 1)
			{
				footsteptype = trace.surface->material - 1;

				if (footsteptype < 0)//?? || footsteptype >= MATERIAL_COUNT)	// i.e. MATERIAL_SILENT (== 0) or incorrect
					ent->s.event = 0;	// EV_FOOTSTEP ?
/*				else if (footsteptype > MATERIAL_WATER)
				{	//!! DEBUG !! (and hack) - should not happen
					developer->integer = 2;
					Com_DPrintf ("^1Bad material sound: idx=%d", footsteptype);
					ent->s.event = 0;
				}
*/				else if (ent->s.event == EV_FOOTSTEP)
					ent->s.event = EV_FOOTSTEP0 + footsteptype;
				else
					ent->s.event = EV_FALLSHORT0 + footsteptype;
			}
		}
	}

	// work only in network game, not single-player
	if (maxclients->integer > 1) //!! should be another variable; campersounds - below!! && sv_campersounds->integer)
	{
		t = Sys_Milliseconds ();
		for (i = 0, cl = svs.clients; i < maxclients->integer; i++, cl++)
		{
			pmove_state_t *pm;
			int sfxstate, curr_vel, prev_vel;

			ent = cl->edict;
			pm = &ent->client->ps.pmove;

			if (cl->state != cs_spawned && cl->state != cs_connected)
			{
				cl->last_velocity2 = 0;
				continue;	// dead / not connected player
			}

			prev_vel = cl->last_velocity2;
			curr_vel = pm->velocity[2];
			cl->last_velocity2 = curr_vel;

			if (cl->screaming && (curr_vel >= FALLING_SCREAM_VELOCITY1 || prev_vel >= FALLING_SCREAM_VELOCITY1
							 || pm->pm_type != PM_NORMAL))	// killed
			{	// stop scream
				SV_StartSoundNew (NULL, ent, CHAN_BODY, SV_SoundIndex ("*fall.wav"), 0.0f, ATTN_NORM, 0);	// volume=0
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
				Com_Printf ("%i:SPECT-SFX\n",i);
			}
			else*/ if (pm->pm_type == PM_NORMAL)
			{
				int leaf, clust;
				vec3_t pm_origin;
				int sfx0, sfxn;

				pm_origin[0] = pm->origin[0] / 8.0f;
				pm_origin[1] = pm->origin[1] / 8.0f;
				pm_origin[2] = pm->origin[2] / 8.0f;

				// check for falling sounds
				if (!cl->screaming)
				{
					if (curr_vel < FALLING_SCREAM_VELOCITY1 && prev_vel < FALLING_SCREAM_VELOCITY1)
					{
						trace_t	trace;
						vec3_t	end;
						static vec3_t mins = {-20, -20, -10};
						static vec3_t maxs = {20, 20, 10};

						end[0] = pm_origin[0];
						end[1] = pm_origin[1];
						end[2] = pm_origin[2] - FALLING_SCREAM_HEIGHT_WATER;
						SV_Trace (&trace, pm_origin, mins, maxs, end, NULL, CONTENTS_WATER);
						if (trace.fraction == 1.0 && !trace.startsolid)	// no water and start not in water
						{
							end[2] = pm_origin[2] - FALLING_SCREAM_HEIGHT_SOLID;
							SV_Trace (&trace, pm_origin, mins, maxs, end, NULL, CONTENTS_SOLID|CONTENTS_LAVA);
							if (trace.fraction == 1.0 || (!trace.ent && trace.plane.normal[2] < 0.5) ||
								trace.contents & CONTENTS_LAVA || trace.surface->flags & SURF_SKY)
								cl->screaming = true;
						}
					}
					if (cl->screaming)
					{
						SV_StartSoundNew (NULL, ent, CHAN_BODY, SV_SoundIndex ("*fall.wav"), 1, ATTN_NORM, 0);
						continue;
					}
				}
				else if (curr_vel >= FALLING_SCREAM_VELOCITY1 || prev_vel >= FALLING_SCREAM_VELOCITY1)
				{	// stop scream
					if (cl->screaming)
						SV_StartSoundNew (NULL, ent, CHAN_BODY, SV_SoundIndex ("*fall.wav"), 0.0, ATTN_NORM, 0);
					cl->screaming = false;
				}

				// check for idle (camper) sounds
				leaf = CM_PointLeafnum (pm_origin);
				clust = CM_LeafCluster (leaf);
				sfxstate = 2;
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
				if (pm->pm_flags & PMF_DUCKED)
				{
					sfx0 = EV_CAMPER0;
					sfxn = 2;
				}
				else
				{
					sfx0 = EV_CAMPER2;
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
}


static client_t *FindClient (vec3_t origin, float maxDist2)
{
	client_t *cl;
	pmove_state_t *pm;
	edict_t	*ent;
	vec3_t	pm_origin, delta;
	float	dist2;
	int		i;

	for (i = 0, cl = svs.clients; i < maxclients->integer; i++, cl++)
	{
		ent = cl->edict;
		pm = &ent->client->ps.pmove;
		if (cl->state != cs_spawned && cl->state != cs_connected)
			continue;

		pm_origin[0] = pm->origin[0] / 8.0f;
		pm_origin[1] = pm->origin[1] / 8.0f;
		pm_origin[2] = pm->origin[2] / 8.0f;
		VectorSubtract (origin, pm_origin, delta);
		dist2 = DotProduct (delta, delta);
		if (dist2 > maxDist2) continue;
		return cl;
	}

	return NULL;
}


sizebuf_t *SV_MulticastHook (sizebuf_t *original, sizebuf_t *ext)
{
	byte	cmd;
	vec3_t	v1, v2;
	char	*s;

	MSG_BeginReading (original);

	// check for interesting messages
	if (MSG_ReadByte (original) != svc_temp_entity)
		return original;

	SZ_Clear (ext);
	cmd = MSG_ReadByte (original);
	switch (cmd)
	{
	case TE_RAILTRAIL:
		{
			int		rColor, rType;
			client_t *cl;

			MSG_ReadPos (original, v1);		// start
			MSG_ReadPos (original, v2);		// end

			cl = FindClient (v1, 16*16);
			rType = 1;
			rColor = 0;
			if (cl)
			{
				if (s = Info_ValueForKey (cl->userinfo, "railcolor"))
				{
					rColor = atoi (s);
					rColor = bound (rColor, 0, 7);
				}
				if (s = Info_ValueForKey (cl->userinfo, "railtype"))
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
			vec3_t	d;
			float	back;
			int		i;

			if (shotLevel != 2) return original;

			MSG_ReadPos (original, v1);
			MSG_ReadDir (original, v2);

			// compute reflection vector
//			Com_Printf("sp: (%g %g %g) -> (%g %g %g)\n",VECTOR_ARGS(v1),VECTOR_ARGS(v2));//!!
			VectorSubtract (shotStart, shotEnd, d);
			VectorNormalizeFast (d);
			back = DotProduct (d, v2);
			for (i = 0; i < 3; i++)
				v2[i] = d[i] - 2 * (d[i] - back * v2[i]);

			MSG_WriteByte (ext, svc_temp_entity);
			MSG_WriteByte (ext, cmd);
			MSG_WritePos (ext, v1);
			MSG_WriteDir (ext, v2);
		}
		return ext;

//	case TE_SPLASH:		?? replace with TE_WATER_SPLASH (?) when shotLevel>0
	}

	return original;
}


trace_t SV_TraceHook (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, edict_t *passedict, int contentmask)
{
	trace_t	tr;
	static edict_t *ent;

#define RESET  { shotLevel = 0; return tr; }

	SV_Trace (&tr, start, mins, maxs, end, passedict, contentmask);
	if (!sv_extProtocol->integer) return tr;

	if (mins || maxs) RESET

	// for details, look game/g_weapon.c :: fire_lead()
	if (contentmask == MASK_SHOT && ((unsigned)passedict) + 4 == (unsigned)start)
	{
		if (ent == passedict && VectorCompare (end, shotStart))
			RESET			// shotgun shot
		shotLevel = 1;
		VectorCopy (end, shotStart);
		ent = passedict;
	}
	else if (shotLevel == 1)
	{
		if (passedict != ent || !VectorCompare (start, shotStart) ||
			(contentmask != (MASK_SHOT|MASK_WATER) && contentmask != MASK_SHOT))
			RESET
		VectorCopy (end, shotEnd);

		shotLevel = 2;
	}
	else if (shotLevel > 1)
	{
		if (passedict != ent) RESET
	}

#undef RESET
	return tr;
}


//=============================================================================


/*
==================
SV_Frame

==================
*/

//void DrawTextLeft(char *fmt, float r, float g, float b);//!!

void SV_Frame (float msec)
{
	int		frameTime;

	time_before_game = time_after_game = 0;

	// if server is not active, do nothing
	if (!svs.initialized)
		return;

//	DrawTextLeft(va("time: %10d rf: %10.5f d: %10.4f ri:%10d", sv.time, svs.realtimef, msec, svs.realtime),1,1,1);//!!
	svs.realtimef += msec;
	svs.realtime = (int)svs.realtimef;		// WARNING: Q_ftol() will produce bad result

	// keep the random time dependent (??)
	rand ();

	// check timeouts
	SV_CheckTimeouts ();

	// get packets from clients
	SV_ReadPackets ();

	frameTime = 100;	// (sv_fps->integer > 10) ? (1000 / sv_fps->integer) : 100;

	// move autonomous things around if enough time has passed
	if (!sv_timedemo->integer && svs.realtime < sv.time)
	{
		// never let the time get too far off
		if (sv.time - svs.realtime > frameTime)
		{
			if (sv_showclamp->integer)
				Com_Printf ("sv lowclamp s:%d -- r:%d -> %d\n", sv.time, svs.realtime, sv.time - frameTime);
			svs.realtime = sv.time - frameTime;
			svs.realtimef = svs.realtime;
		}
//		NET_Sleep(sv.time - svs.realtime);		//?? timescaled value
		return;
	}

	// update ping based on the last known frame from all clients
	SV_CalcPings ();

	// give the clients some timeslices
	SV_GiveMsec ();

	// let everything in the world think and move
	/*-------- SV_RunGameFrame () ---------------*/
	// we always need to bump framenum, even if we
	// don't run the world, otherwise the delta
	// compression can get confused when a client
	// has the "current" frame
	sv.framenum++;
	sv.time += frameTime;	//  = sv.framenum*100; ??

	// don't run if paused
	if (!sv_paused->integer || maxclients->integer > 1)
	{
		if (com_speeds->integer)
			time_before_game = Sys_Milliseconds ();

		ge->RunFrame ();

		// never get more than one tic behind
		if (sv.time < svs.realtime)
		{
			if (sv_showclamp->integer)
				Com_Printf ("sv highclamp s:%d r:%d -> %d\n", sv.time, svs.realtime, sv.time);
			svs.realtime = sv.time;
			svs.realtimef = sv.time;
		}
		if (com_speeds->integer)
			time_after_game = Sys_Milliseconds ();
	}

	// if extended protocol used, recalculate footstep sounds, mdx/mdl/md3 frames etc.
	if (sv_extProtocol->integer)
		SV_PostprocessFrame ();

	// send messages back to the clients that had packets read this frame
	SV_SendClientMessages ();

	// save the entire world state if recording a serverdemo
	SV_RecordDemoMessage ();

	// send a heartbeat to the master if needed
	Master_Heartbeat ();

	// clear teleport flags, etc for next frame
	SV_PrepWorldFrame ();
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
static void Master_Heartbeat (void)
{
	char		*string;
	int			i;

	// pgm post3.19 change, cvar pointer not validated before dereferencing
	if (!dedicated || !dedicated->integer)
		return;		// only dedicated servers send heartbeats

	// pgm post3.19 change, cvar pointer not validated before dereferencing
	if (!public_server || !public_server->integer)
		return;		// a private dedicated game

	// check for time wraparound
	if (svs.last_heartbeat > svs.realtime)
		svs.last_heartbeat = svs.realtime;

	if (svs.realtime - svs.last_heartbeat < HEARTBEAT_SECONDS*1000)
		return;		// not time to send yet

	svs.last_heartbeat = svs.realtime;

	// send the same string that we would give for a status OOB command
	string = SV_StatusString();

	// send to group master
	for (i = 0; i < MAX_MASTERS; i++)
		if (master_adr[i].port)
		{
			Com_Printf ("Sending heartbeat to %s\n", NET_AdrToString (&master_adr[i]));
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
	int			i;

	if (!dedicated || !dedicated->integer)
		return;		// only dedicated servers send heartbeats

	if (!public_server || !public_server->integer)
		return;		// a private dedicated game

	// send to group master
	for (i = 0; i < MAX_MASTERS; i++)
		if (master_adr[i].port)
		{
			if (i > 0)
				Com_Printf ("Sending heartbeat to %s\n", NET_AdrToString (&master_adr[i]));
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
	char	*val;
	int		i;

	// call prog code to allow overrides
	ge->ClientUserinfoChanged (cl->edict, cl->userinfo);

	// name for C code
	strncpy (cl->name, Info_ValueForKey (cl->userinfo, "name"), sizeof(cl->name)-1);
	// mask off high bit
	for (i = 0; i < sizeof(cl->name); i++)
		cl->name[i] &= 127;

	// rate command
	val = Info_ValueForKey (cl->userinfo, "rate");
	if (strlen(val))
	{
		i = atoi(val);
		cl->rate = bound(i, 100, 15000);
	}
	else
		cl->rate = 5000;

	// msg command
	val = Info_ValueForKey (cl->userinfo, "msg");
	if (strlen(val))
	{
		cl->messagelevel = atoi(val);
	}

}


//============================================================================

/*
===============
SV_Init

Only called at quake2.exe startup, not for each game
===============
*/
void SV_Init (void)
{
CVAR_BEGIN(vars)
	{&rcon_password, "rcon_password", "", 0},
	CVAR_NULL(skill, 1, 0),
	CVAR_NULL(deathmatch, 0, CVAR_LATCH),
	CVAR_NULL(coop, 0, CVAR_LATCH),
	CVAR_NULL(fraglimit, 0, CVAR_SERVERINFO),
	CVAR_NULL(timelimit, 0, CVAR_SERVERINFO),
	CVAR_VAR(maxclients, 1, CVAR_SERVERINFO|CVAR_LATCH),
	CVAR_VAR(hostname, noname, CVAR_SERVERINFO|CVAR_ARCHIVE),
	CVAR_VAR(timeout, 125, 0),
	CVAR_VAR(zombietime, 2, 0),
	{&sv_showclamp, "showclamp", "0", 0},
	{&sv_paused, "paused", "0", CVAR_CHEAT},
	{&sv_timedemo, "timedemo", "0", CVAR_CHEAT},
	{&sv_enforcetime, "sv_enforcetime", "0", 0},
	CVAR_VAR(allow_download, 1, CVAR_ARCHIVE),
	CVAR_VAR(allow_download_players, 0, CVAR_ARCHIVE),
	CVAR_VAR(allow_download_models, 1, CVAR_ARCHIVE),
	CVAR_VAR(allow_download_sounds, 1, CVAR_ARCHIVE),
	CVAR_VAR(allow_download_maps, 1, CVAR_ARCHIVE),

	CVAR_VAR(sv_noreload, 0, 0),

	CVAR_VAR(sv_airaccelerate, 0, CVAR_LATCH),

	{&public_server, "public", "0", 0},

	CVAR_VAR(sv_reconnect_limit, 3, CVAR_ARCHIVE),

	CVAR_VAR(sv_extProtocol, 1, CVAR_SERVERINFO|CVAR_LATCH|CVAR_ARCHIVE),
	CVAR_VAR(sv_camperSounds, 1, CVAR_SERVERINFO|CVAR_ARCHIVE)
//	CVAR_VAR(sv_fps, 20, 0)	// archive/serverinfo ??
CVAR_END

	SV_InitOperatorCommands	();

	CVAR_GET_VARS(vars);
	Cvar_Get ("dmflags", va("%d", DF_INSTANT_ITEMS), CVAR_SERVERINFO);
	Cvar_Get ("protocol", va("%d", PROTOCOL_VERSION), CVAR_SERVERINFO|CVAR_NOSET);

	SZ_Init (&net_message, net_message_buffer, sizeof(net_message_buffer));
}

/*
================
SV_Shutdown

Called when each game quits,
before Sys_Quit or Sys_Error
================
*/
void SV_Shutdown (char *finalmsg, qboolean reconnect)
{
	if (svs.clients)
	{
		int			i;
		client_t	*cl;

		// Send a final message to all connected clients before the server goes down.
		//  The messages are sent immediately, not just stuck on the outgoing message
		// list, because the server is going to totally exit after returning from this function.
		for (i = 0, cl = svs.clients; i < maxclients->integer; i++, cl++)
		{
			if (cl->state < cs_connected)
				continue;

			SZ_Clear (&net_message);
			MSG_WriteByte (&net_message, svc_print);
			MSG_WriteByte (&net_message, PRINT_HIGH);
			if (cl->newprotocol && !reconnect)	// colorize exit message
				MSG_WriteString (&net_message, va("^1%s", finalmsg));
			else
				MSG_WriteString (&net_message, finalmsg);
			MSG_WriteByte (&net_message, reconnect ? svc_reconnect : svc_disconnect);

			Netchan_Transmit (&cl->netchan, net_message.cursize, net_message.data);
		}
	}

	Master_Shutdown ();
	SV_ShutdownGameProgs ();

	// free current level
	if (sv.demofile)
		FS_FCloseFile (sv.demofile);
	memset (&sv, 0, sizeof(sv));
	Com_SetServerState (sv.state);

	// free server static data
	if (svs.clients)
		Z_Free (svs.clients);
	if (svs.client_entities)
		Z_Free (svs.client_entities);
	if (svs.demofile)
		fclose (svs.demofile);
	memset (&svs, 0, sizeof(svs));

	Cvar_GetLatchedVars ();
}
