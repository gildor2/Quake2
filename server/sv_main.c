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

client_t	*sv_client;			// current client

cvar_t	*sv_paused;
static cvar_t	*sv_timedemo;

cvar_t	*sv_enforcetime;

static cvar_t	*timeout;		// seconds without any message
static cvar_t	*zombietime;	// seconds to sink messages after disconnect

static cvar_t	*rcon_password;	// password for remote server commands

cvar_t *allow_download;
cvar_t *allow_download_players;
cvar_t *allow_download_models;
cvar_t *allow_download_sounds;
cvar_t *allow_download_maps;

cvar_t *sv_airaccelerate;

cvar_t	*sv_noreload;			// don't reload level state when reentering

cvar_t	*maxclients;			// FIXME: rename sv_maxclients
static cvar_t	*sv_showclamp;

static cvar_t	*hostname;
static cvar_t	*public_server;			// should heartbeats be sent

static cvar_t	*sv_reconnect_limit;	// minimum seconds between connect messages

static cvar_t	*sv_extProtocol;
static cvar_t	*sv_camperSounds;

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
void SV_DropClient (client_t *drop)
{
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
char	*SV_StatusString (void)
{
	char	player[1024];
	static char	status[MAX_MSGLEN - 16];
	int		i;
	client_t	*cl;
	int		statusLength;
	int		playerLength;

	strcpy (status, Cvar_Serverinfo());
	strcat (status, "\n");
	statusLength = strlen(status);

	for (i = 0; i < maxclients->integer; i++)
	{
		cl = &svs.clients[i];
		if (cl->state == cs_connected || cl->state == cs_spawned )
		{
			Com_sprintf (player, sizeof(player), "%i %i \"%s\"\n",
				cl->edict->client->ps.stats[STAT_FRAGS], cl->ping, cl->name);
			playerLength = strlen(player);
			if (statusLength + playerLength >= sizeof(status) )
				break;		// can't hold any more
			strcpy (status + statusLength, player);
			statusLength += playerLength;
		}
	}

	return status;
}

/*
================
SVC_Status

Responds with all the info that qplug or qspy can see
================
*/
void SVC_Status (void)
{
	Netchan_OutOfBandPrint (NS_SERVER, net_from, "print\n%s", SV_StatusString());
#if 0
	Com_BeginRedirect (RD_PACKET, sv_outputbuf, SV_OUTPUTBUF_LENGTH, SV_FlushRedirect);
	Com_Printf (SV_StatusString());
	Com_EndRedirect ();
#endif
}

/*
================
SVC_Ack

================
*/
void SVC_Ack (void)
{
	Com_Printf ("Ping acknowledge from %s\n", NET_AdrToString(net_from));
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
	int		i, count;
	int		version;

	if (maxclients->integer == 1)
		return;		// ignore in single player

	version = atoi (Cmd_Argv(1));

	if (version != PROTOCOL_VERSION)
		Com_sprintf (string, sizeof(string), "%s: wrong version\n", hostname->string, sizeof(string));
	else
	{
		count = 0;
		for (i = 0; i < maxclients->integer; i++)
			if (svs.clients[i].state >= cs_connected)
				count++;

		Com_sprintf (string, sizeof(string), "%16s %8s %2i/%2i\n", hostname->string, sv.name, count, maxclients->integer);
	}

	Netchan_OutOfBandPrint (NS_SERVER, net_from, "info\n%s", string);
}

/*
================
SVC_Ping

Just responds with an acknowledgement
================
*/
void SVC_Ping (void)
{
	Netchan_OutOfBandPrint (NS_SERVER, net_from, "ack");
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
	int		i;
	int		oldest;
	int		oldestTime;

	oldest = 0;
	oldestTime = 0x7fffffff;

	// see if we already have a challenge for this ip
	for (i = 0 ; i < MAX_CHALLENGES ; i++)
	{
		if (NET_CompareBaseAdr (net_from, svs.challenges[i].adr))
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
		svs.challenges[oldest].challenge = rand() & 0x7fff;
		svs.challenges[oldest].adr = net_from;
		svs.challenges[oldest].time = curtime;
		i = oldest;
	}

	// send it back
	Netchan_OutOfBandPrint (NS_SERVER, net_from, "challenge %i", svs.challenges[i].challenge);
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
	int			version;
	int			qport;
	int			challenge;

	adr = net_from;

	Com_DPrintf ("SVC_DirectConnect ()\n");

	version = atoi(Cmd_Argv(1));
	if (version != PROTOCOL_VERSION)
	{
		Netchan_OutOfBandPrint (NS_SERVER, adr, "print\nServer is version %4.2f.\n", VERSION);
		Com_DPrintf ("  rejected connect from version %d\n", version);
		return;
	}

	qport = atoi(Cmd_Argv(2));

	challenge = atoi(Cmd_Argv(3));

	strncpy (userinfo, Cmd_Argv(4), sizeof(userinfo)-1);
	userinfo[sizeof(userinfo) - 1] = 0;

	// force the IP key/value pair so the game can filter based on ip
	Info_SetValueForKey (userinfo, "ip", NET_AdrToString(net_from));

	// attractloop servers are ONLY for local clients
	if (sv.attractloop)
	{
		if (!NET_IsLocalAddress (adr))
		{
			Com_Printf ("Remote connect in attract loop.  Ignored.\n");
			Netchan_OutOfBandPrint (NS_SERVER, adr, "print\nConnection refused.\n");
			return;
		}
	}

	// see if the challenge is valid
	if (!NET_IsLocalAddress (adr))
	{
		for (i = 0; i < MAX_CHALLENGES; i++)
		{
			if (NET_CompareBaseAdr (net_from, svs.challenges[i].adr))
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

	newcl = &temp;
	memset (newcl, 0, sizeof(client_t));

	// if there is already a slot for this ip, reuse it
	for (i = 0, cl = svs.clients; i < maxclients->integer; i++,cl++)
	{
		if (cl->state == cs_free)
			continue;
		if (NET_CompareBaseAdr (adr, cl->netchan.remote_address)
			&& ( cl->netchan.qport == qport
			|| adr.port == cl->netchan.remote_address.port ) )
		{
			if (!NET_IsLocalAddress (adr) && (svs.realtime - cl->lastconnect) < ((int)sv_reconnect_limit->value * 1000))
			{
				Com_DPrintf ("%s:reconnect rejected : too soon\n", NET_AdrToString (adr));
				return;
			}
			Com_Printf ("%s:reconnect\n", NET_AdrToString (adr));
			newcl = cl;
			goto gotnewcl;
		}
	}

	// find a client slot
	newcl = NULL;
	for (i = 0, cl = svs.clients; i < maxclients->integer; i++,cl++)
	{
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

gotnewcl:
	// build a new connection
	// accept the new client
	// this is the only place a client_t is ever initialized
	*newcl = temp;
	sv_client = newcl;
	edictnum = (newcl-svs.clients)+1;
	ent = EDICT_NUM(edictnum);
	newcl->edict = ent;
	newcl->challenge = challenge; // save challenge for checksumming

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

	Netchan_Setup (NS_SERVER, &newcl->netchan , adr, qport);

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
		Com_Printf ("Bad rcon from %s:\n%s\n", NET_AdrToString (net_from), net_message.data+4);
	else
		Com_Printf ("Rcon from %s:\n%s\n", NET_AdrToString (net_from), net_message.data+4);

	Com_BeginRedirect (RD_PACKET, sv_outputbuf, SV_OUTPUTBUF_LENGTH, SV_FlushRedirect);

	if (!Rcon_Validate ())
	{
		Com_WPrintf ("Bad rcon_password.\n");
	}
	else
	{
		remaining[0] = 0;

		for (i=2 ; i<Cmd_Argc() ; i++)
		{
			strcat (remaining, Cmd_Argv(i) );
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
	char	*s;
	char	*c;

	MSG_BeginReading (&net_message);
	MSG_ReadLong (&net_message);		// skip the -1 marker

	s = MSG_ReadStringLine (&net_message);

	Cmd_TokenizeString (s, false);

	c = Cmd_Argv(0);
	Com_DPrintf ("Packet %s : %s\n", NET_AdrToString(net_from), c);

	if (!strcmp (c, "ping"))
		SVC_Ping ();
	else if (!strcmp (c, "ack"))
		SVC_Ack ();
	else if (!strcmp (c, "status"))
		SVC_Status ();
	else if (!strcmp (c, "info"))
		SVC_Info ();
	else if (!strcmp (c, "getchallenge"))
		SVC_GetChallenge ();
	else if (!strcmp (c, "connect"))
		SVC_DirectConnect ();
	else if (!strcmp (c, "rcon"))
		SVC_RemoteCommand ();
	else
		Com_WPrintf ("Bad connectionless packet from %s:\n%s\n", NET_AdrToString (net_from), s);
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
		for (j=0 ; j<LATENCY_COUNTS ; j++)
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
		qport = MSG_ReadShort (&net_message) & 0xffff;

		// check for packets from connected clients
		for (i = 0, cl = svs.clients; i < maxclients->integer; i++,cl++)
		{
			if (cl->state == cs_free)
				continue;
			if (!NET_CompareBaseAdr (net_from, cl->netchan.remote_address))
				continue;
			if (cl->netchan.qport != qport)
				continue;
			if (cl->netchan.remote_address.port != net_from.port)
			{
				Com_Printf ("SV_ReadPackets: fixing up a translated port\n");
				cl->netchan.remote_address.port = net_from.port;
			}

			if (Netchan_Process(&cl->netchan, &net_message))
			{	// this is a valid, sequenced packet, so process it
				if (cl->state != cs_zombie)
				{
					cl->lastmessage = svs.realtime;	// don't timeout
					SV_ExecuteClientMessage (cl);
				}
			}
			break;
		}

		if (i != maxclients->integer)
			continue;
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
			SV_BroadcastPrintf (PRINT_HIGH, "%s timed out\n", cl->name);
			SV_DropClient (cl);
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
void SV_PrepWorldFrame (void)
{
	edict_t	*ent;
	int		i;

	for (i=0 ; i<ge->num_edicts ; i++, ent++)
	{
		ent = EDICT_NUM(i);
		// events only last for a single message
		ent->s.event = 0;
	}

}


/*
=================
SV_RunGameFrame
=================
*/
void SV_RunGameFrame (void)
{
	if (com_speeds->integer)
		time_before_game = Sys_Milliseconds ();

	// we always need to bump framenum, even if we
	// don't run the world, otherwise the delta
	// compression can get confused when a client
	// has the "current" frame
	sv.framenum++;
	sv.time = sv.framenum*100;

	// don't run if paused
	if (!sv_paused->integer || maxclients->integer > 1)
	{
		ge->RunFrame ();

		// never get more than one tic behind
		if (sv.time < svs.realtime)
		{
			if (sv_showclamp->integer)
				Com_Printf ("sv highclamp\n");
			svs.realtime = sv.time;
		}
	}

	if (com_speeds->integer)
		time_after_game = Sys_Milliseconds ();

}


/*
==================
SV_PostprocessFrame
==================
*/
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
			point[2] = ent->s.origin[2] - 64;		// - 0.25; ?
			trace = SV_Trace (ent->s.origin, ent->mins, ent->maxs, point, ent, MASK_PLAYERSOLID|MASK_MONSTERSOLID|MASK_WATER);
			if (trace.fraction < 1)
			{
				if (!trace.surface)
					footsteptype = -1;	//?? silent
				else
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
				SV_StartSoundNew (NULL, ent, CHAN_BODY, SV_SoundIndex ("*fall.wav"), 0.0, ATTN_NORM, 0);
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
						trace = SV_Trace (pm_origin, mins, maxs, end, NULL, CONTENTS_WATER);
						if (trace.fraction == 1.0 && !trace.startsolid)	// no water and start not in water
						{
							end[2] = pm_origin[2] - FALLING_SCREAM_HEIGHT_SOLID;
							trace = SV_Trace (pm_origin, mins, maxs, end, NULL, CONTENTS_SOLID);
							if (trace.fraction == 1.0 || (!trace.ent && trace.plane.normal[2] < 0.5))
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


/*
==================
SV_Frame

==================
*/
void SV_Frame (int msec)
{
	time_before_game = time_after_game = 0;

	// if server is not active, do nothing
	if (!svs.initialized)
		return;

	svs.realtime += msec;

	// keep the random time dependent
	rand ();

	// check timeouts
	SV_CheckTimeouts ();

	// get packets from clients
	SV_ReadPackets ();

	// move autonomous things around if enough time has passed
	if (!sv_timedemo->integer && svs.realtime < sv.time)
	{
		// never let the time get too far off
		if (sv.time - svs.realtime > 100)
		{
			if (sv_showclamp->integer)
				Com_Printf ("sv lowclamp\n");
			svs.realtime = sv.time - 100;
		}
		NET_Sleep(sv.time - svs.realtime);
		return;
	}

	// update ping based on the last known frame from all clients
	SV_CalcPings ();

	// give the clients some timeslices
	SV_GiveMsec ();

	// let everything in the world think and move
	SV_RunGameFrame ();

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
void Master_Heartbeat (void)
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
	for (i=0 ; i<MAX_MASTERS ; i++)
		if (master_adr[i].port)
		{
			Com_Printf ("Sending heartbeat to %s\n", NET_AdrToString (master_adr[i]));
			Netchan_OutOfBandPrint (NS_SERVER, master_adr[i], "heartbeat\n%s", string);
		}
}

/*
=================
Master_Shutdown

Informs all masters that this server is going down
=================
*/
void Master_Shutdown (void)
{
	int			i;

	// pgm post3.19 change, cvar pointer not validated before dereferencing
	if (!dedicated || !dedicated->integer)
		return;		// only dedicated servers send heartbeats

	// pgm post3.19 change, cvar pointer not validated before dereferencing
	if (!public_server || !public_server->integer)
		return;		// a private dedicated game

	// send to group master
	for (i = 0; i < MAX_MASTERS; i++)
		if (master_adr[i].port)
		{
			if (i > 0)
				Com_Printf ("Sending heartbeat to %s\n", NET_AdrToString (master_adr[i]));
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
		cl->rate = i;
		if (cl->rate < 100)
			cl->rate = 100;
		if (cl->rate > 15000)
			cl->rate = 15000;
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
	CVAR_NULL(cheats, 0, CVAR_SERVERINFO|CVAR_LATCH),
	CVAR_VAR(maxclients, 1, CVAR_SERVERINFO|CVAR_LATCH),
	CVAR_VAR(hostname, noname, CVAR_SERVERINFO|CVAR_ARCHIVE),
	CVAR_VAR(timeout, 125, 0),
	CVAR_VAR(zombietime, 2, 0),
	{&sv_showclamp, "showclamp", "0", 0},
	{&sv_paused, "paused", "0", 0},
	{&sv_timedemo, "timedemo", "0", 0},
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
	CVAR_VAR(sv_camperSounds, 1, CVAR_SERVERINFO|CVAR_ARCHIVE),
CVAR_END

	SV_InitOperatorCommands	();

	CVAR_GET_VARS(vars);
	Cvar_Get ("dmflags", va("%d", DF_INSTANT_ITEMS), CVAR_SERVERINFO);
	Cvar_Get ("protocol", va("%d", PROTOCOL_VERSION), CVAR_SERVERINFO|CVAR_NOSET);

	SZ_Init (&net_message, net_message_buffer, sizeof(net_message_buffer));
}

/*
==================
SV_FinalMessage

Used by SV_Shutdown to send a final message to all
connected clients before the server goes down.  The messages are sent immediately,
not just stuck on the outgoing message list, because the server is going
to totally exit after returning from this function.
==================
*/
void SV_FinalMessage (char *message, qboolean reconnect)
{
	int			i;
	client_t	*cl;

	SZ_Clear (&net_message);
	MSG_WriteByte (&net_message, svc_print);
	MSG_WriteByte (&net_message, PRINT_HIGH);
	MSG_WriteString (&net_message, message);

	if (reconnect)
		MSG_WriteByte (&net_message, svc_reconnect);
	else
		MSG_WriteByte (&net_message, svc_disconnect);

	// send it twice
	// stagger the packets to crutch operating system limited buffers

	for (i = 0, cl = svs.clients; i < maxclients->integer; i++, cl++)
		if (cl->state >= cs_connected)
			Netchan_Transmit (&cl->netchan, net_message.cursize
			, net_message.data);

	for (i = 0, cl = svs.clients; i < maxclients->integer; i++, cl++)
		if (cl->state >= cs_connected)
			Netchan_Transmit (&cl->netchan, net_message.cursize
			, net_message.data);
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
		SV_FinalMessage (finalmsg, reconnect);

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
}
