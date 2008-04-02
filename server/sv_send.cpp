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
// sv_main.cpp -- server main program

#include "server.h"
#include "cmodel.h"

/*
=============================================================================

EVENT MESSAGES

=============================================================================
*/


// Sends text across to be displayed if the level passes
void SV_ClientPrintf(client_t *cl, int level, const char *fmt, ...)
{
	if (level < cl->messagelevel) return;

	va_list		argptr;
	char		string[1024];
	va_start(argptr,fmt);
	vsnprintf(ARRAY_ARG(string), fmt, argptr);
	va_end(argptr);

	MSG_WriteByte(&cl->netchan.message, svc_print);
	MSG_WriteByte(&cl->netchan.message, level);
	MSG_WriteString(&cl->netchan.message, string);
}


// Sends text to all active clients
void SV_BroadcastPrintf(int level, const char *fmt, ...)
{
	va_list		argptr;
	char		string[2048];
	va_start(argptr,fmt);
	vsnprintf(ARRAY_ARG(string), fmt, argptr);
	va_end(argptr);

	int			i;
	client_t	*cl;

	// echo to console
	if (DEDICATED)
	{
		char	copy[1024];

		// mask off high bits
		for (i = 0; i < ARRAY_COUNT(copy)-1 && string[i] ; i++)
			copy[i] = string[i] & 127;
		copy[i] = 0;
		appPrintf("%s", copy);
	}

	for (i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++)
	{
		if (level < cl->messagelevel)
			continue;
		if (cl->state != cs_spawned)
			continue;
		MSG_WriteByte(&cl->netchan.message, svc_print);
		MSG_WriteByte(&cl->netchan.message, level);
		MSG_WriteString(&cl->netchan.message, string);
	}
}


// Sends text to all active clients
void SV_BroadcastCommand(const char *fmt, ...)
{
	if (!sv.state) return;

	va_list		argptr;
	char		string[1024];
	va_start(argptr,fmt);
	vsnprintf(ARRAY_ARG(string), fmt, argptr);
	va_end(argptr);

	MSG_WriteByte(&sv.multicast, svc_stufftext);
	MSG_WriteString(&sv.multicast, string);
	SV_MulticastOld(nullVec3, MULTICAST_ALL_R);
}


/*
=================
SV_Multicast

Sends the contents of sv.multicast to a subset of the clients,
then clears sv.multicast.

MULTICAST_ALL	same as broadcast (origin ignored)
MULTICAST_PVS	send to clients potentially visible from org
MULTICAST_PHS	send to clients potentially hearable from org; currently == MULTICAST_ALL
=================
*/
void SV_Multicast(const CVec3 &origin, multicast_t to, bool oldclients)
{
	guard(SV_Multicast);

	int zone1;
	if (to != MULTICAST_ALL_R && to != MULTICAST_ALL)
		zone1 = CM_FindLeaf(origin)->zone;
	else
		zone1 = -1;

	// if doing a serverrecord, store everything
	if (svs.wdemofile)
		svs.demo_multicast.Write(sv.multicast);

	bool reliable = false;
	const byte *pvs = NULL;
	switch (to)
	{
	case MULTICAST_ALL_R:
		reliable = true;	// fallthrough
	case MULTICAST_ALL:
		break;

	case MULTICAST_PHS_R:
		reliable = true;	// fallthrough
	case MULTICAST_PHS:
//		mask = CM_ClusterPHS(CM_FindLeaf(origin)->cluster);
		// currently, == MULTICAST_ALL[_R]
		break;

	case MULTICAST_PVS_R:
		reliable = true;	// fallthrough
	case MULTICAST_PVS:
		pvs = CM_ClusterPVS(CM_FindLeaf(origin)->cluster);
		break;

	default:
		Com_DropError("SV_Multicast: bad to=%d", to);
	}

	// process transferring message and create a copy for new clients
	sizebuf_t *newBuf = (sv_extProtocol->integer) ? SV_MulticastHook(&sv.multicast, &sv.multicastNew) : &sv.multicast;

	// send the data to all relevent clients
	int			j;
	client_t	*client;
	for (j = 0, client = svs.clients; j < sv_maxclients->integer; j++, client++)
	{
		if (client->state == cs_free || client->state == cs_zombie)
			continue;
		if (client->state != cs_spawned && !reliable)
			continue;
		if (!oldclients && !client->newprotocol)
			continue;

		sizebuf_t *buf = (client->newprotocol) ? newBuf : &sv.multicast;

		if (zone1 != -1)
		{
			const CBspLeaf *leaf = CM_FindLeaf(client->edict->s.origin);
			int cluster = leaf->cluster;
			int zone2   = leaf->zone;
			if (!CM_ZonesConnected(zone1, zone2)) continue;
			if (pvs)
			{
				if (!( pvs[cluster>>3] & (1<<(cluster&7)) ))
				continue;
			}
		}

		if (reliable)
			client->netchan.message.Write(*buf);
		else
			client->datagram.Write(*buf);
	}

	sv.multicast.Clear();
	sv.multicastNew.Clear();

	unguard;
}

/*
==================
SV_StartSound

Each entity can have eight independant sound sources, like voice, weapon, feet, etc.

Channel 0 is an auto-allocate channel, the others override anything already running on that
entity/channel pair.

An attenuation of 0 will play full volume everywhere in the level. Larger attenuations will drop
off.  (max 4 attenuation)

Timeofs can range from 0.0 to 0.1 to cause sounds to be started later in the frame than they
normally would.

If origin is NULL, the origin is determined from the entity origin or the midpoint of the entity
box for bmodels.
==================
*/
void SV_StartSound(const CVec3 *origin, edict_t *entity, int channel,
	int soundindex, float volume, float attenuation, float timeofs, bool oldclients)
{
	guard(SV_StartSound);

	if (volume < 0 || volume > 1.0)
		appError("SV_StartSound: volume = %f", volume);

	if (attenuation < 0 || attenuation > 4)
		appError("SV_StartSound: attenuation = %f", attenuation);

//	if (channel < 0 || channel > 15)
//		appError("SV_StartSound: channel = %i", channel);

	if (timeofs < 0 || timeofs > 0.255)
		appError("SV_StartSound: timeofs = %f", timeofs);

	channel &= ~CHAN_NO_PHS_ADD;		// simply clear this flag (no PHS anymore)

	int ent      = NUM_FOR_EDICT(entity);
	int sendchan = (ent<<3) | (channel&7);

	int flags = 0;
	if (volume != DEFAULT_SOUND_PACKET_VOLUME)
		flags |= SND_VOLUME;
	if (attenuation != DEFAULT_SOUND_PACKET_ATTENUATION)
		flags |= SND_ATTENUATION;

	// the client doesn't know that bmodels have weird origins
	// the origin can also be explicitly set
	if ((entity->svflags & SVF_NOCLIENT) || (entity->solid == SOLID_BSP) || origin)
		flags |= SND_POS;

	// always send the entity number for channel overrides
	flags |= SND_ENT;

	if (timeofs)
		flags |= SND_OFFSET;

	// use the entity origin unless it is a bmodel or explicitly specified
	CVec3 origin2;
	if (!origin)
	{
		if (entity->solid == SOLID_BSP)
		{
			entity->bounds.GetCenter(origin2);
			origin2.Add(entity->s.origin);
			origin = &origin2;
		}
		else
			origin = &entity->s.origin;
	}

	MSG_WriteByte(&sv.multicast, svc_sound);
	MSG_WriteByte(&sv.multicast, flags);
	MSG_WriteByte(&sv.multicast, soundindex);

	if (flags & SND_VOLUME)			MSG_WriteByte (&sv.multicast, appRound(volume * 255));
	if (flags & SND_ATTENUATION)	MSG_WriteByte (&sv.multicast, appRound(attenuation * 64));
	if (flags & SND_OFFSET)			MSG_WriteByte (&sv.multicast, appRound(timeofs * 1000));
	if (flags & SND_ENT)			MSG_WriteShort(&sv.multicast, sendchan);
	if (flags & SND_POS)			MSG_WritePos  (&sv.multicast, *origin);

	multicast_t	to = (channel & CHAN_RELIABLE) ? MULTICAST_ALL_R : MULTICAST_ALL;

	if (oldclients)
		SV_MulticastOld(*origin, to);
	else
		SV_MulticastNew(*origin, to);

	unguard;
}


/*
===============================================================================

FRAME UPDATES

===============================================================================
*/

bool SV_SendClientDatagram(client_t *client)
{
	// prepare msg
	sizebuf_t msg;
	byte	msg_buf[MAX_MSGLEN];
	msg.Init(msg_buf, client->maxPacketSize);
	msg.allowoverflow = true;

	// send over all the relevant entity states
	// and the player_state_t
	SV_WriteFrameToClient(client, &msg);

	// copy the accumulated multicast datagram
	// for this client out to the message
	// it is necessary for this to be after the WriteEntities
	// so that entity references will be current
	if (client->datagram.overflowed)
		appWPrintf("Datagram overflowed for %s\n", *client->Name);
	else
		msg.Write(client->datagram);
	client->datagram.Clear();

	if (msg.overflowed)
	{	// must have room left for the packet header
		appWPrintf("Msg overflowed for %s\n", *client->Name);
		msg.Clear();
	}

	// send the datagram
	client->netchan.Transmit(msg.data, msg.cursize);

	// record the size for rate estimation
	client->message_size[sv.framenum % RATE_MESSAGES] = msg.cursize;

	return true;
}


void SV_DemoCompleted()
{
	if (sv.rdemofile)
	{
		delete sv.rdemofile;
		sv.rdemofile = NULL;
	}
	SV_Nextserver();
}


// Returns true if the client is over its current
// bandwidth estimation and should not be sent another packet
static bool RateDrop(client_t *c)
{
	// never drop over the loopback
	if (c->netchan.remote_address.type == NA_LOOPBACK)
		return false;

	int total = 0;
	for (int i = 0; i < RATE_MESSAGES; i++)
		total += c->message_size[i];

	if (total > c->rate)
	{
		c->surpressCount++;
		c->message_size[sv.framenum % RATE_MESSAGES] = 0;
		return true;
	}

	return false;
}


void SV_SendClientMessages()
{
	int msglen = 0;
	byte msgbuf[MAX_MSGLEN];

	// read the next demo message if needed
	if (sv.state == ss_demo && sv.rdemofile)
	{
		if (sv_paused->integer)
			msglen = 0;
		else
		{
			// get the next message
			msglen = sv.rdemofile->ReadInt();
			if (msglen == -1)
			{
				SV_DemoCompleted();
				return;
			}
			if (msglen > MAX_MSGLEN)
				Com_DropError("SV_SendClientMessages: msglen > MAX_MSGLEN");
			sv.rdemofile->Read(msgbuf, msglen);
		}
	}

	// send a message to each connected client
	unsigned curtime = appMilliseconds();
	int i;
	client_t *c;
	for (i = 0, c = svs.clients ; i < sv_maxclients->integer; i++, c++)
	{
		if (!c->state) continue;
		// if the reliable message overflowed, drop the client
		if (c->netchan.message.overflowed)
		{
			c->netchan.message.Clear();
			c->datagram.Clear();
			SV_DropClient(c, "overflowed");
		}

		if (sv.state == ss_cinematic || sv.state == ss_demo || sv.state == ss_pic)
			c->netchan.Transmit(msgbuf, msglen);
		else if (c->state == cs_spawned)
		{
			// don't overrun bandwidth
			if (RateDrop(c))
			{
				if (sv_shownet->integer) appPrintf("send data to %s\n", *c->Name);
				continue;
			}
			if (sv_shownet->integer) appPrintf("send data to %s\n", *c->Name);
			SV_SendClientDatagram(c);
		}
		else
		{
			// just update reliable	if needed
			if (c->netchan.message.cursize || curtime - c->netchan.last_sent > 1000)
				c->netchan.Transmit(NULL, 0);
		}
	}
}
