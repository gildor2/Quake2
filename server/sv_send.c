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
// sv_main.c -- server main program

#include "server.h"

/*
=============================================================================

Com_Printf redirection

=============================================================================
*/

#define	SV_OUTPUTBUF_LENGTH	(MAX_MSGLEN - 16)
static char sv_outputbuf[SV_OUTPUTBUF_LENGTH];

static void SV_FlushRedirect (char *outputbuf)
{
	guard(SV_FlushRedirect);

	// if server will be restarted during active redirection, drop data
	// (NOTE: we cannot call Com_EndRedirect(), because this will recurse flush())
	if (!sv_client->netchan.message.maxsize)
		return;
	// send redirected text
	MSG_WriteByte (&sv_client->netchan.message, svc_print);
	MSG_WriteByte (&sv_client->netchan.message, PRINT_HIGH);
	MSG_WriteString (&sv_client->netchan.message, outputbuf);

	unguard;
}


void SV_BeginRedirect (void)
{
	Com_BeginRedirect (sv_outputbuf, SV_OUTPUTBUF_LENGTH, SV_FlushRedirect);
}


/*
=============================================================================

EVENT MESSAGES

=============================================================================
*/


/*
=================
SV_ClientPrintf

Sends text across to be displayed if the level passes
=================
*/
void SV_ClientPrintf (client_t *cl, int level, char *fmt, ...)
{
	va_list		argptr;
	char		string[1024];

	if (level < cl->messagelevel)
		return;

	va_start (argptr,fmt);
	vsnprintf (ARRAY_ARG(string), fmt,argptr);
	va_end (argptr);

	MSG_WriteByte (&cl->netchan.message, svc_print);
	MSG_WriteByte (&cl->netchan.message, level);
	MSG_WriteString (&cl->netchan.message, string);
}

/*
=================
SV_BroadcastPrintf

Sends text to all active clients
=================
*/
void SV_BroadcastPrintf (int level, char *fmt, ...)
{
	va_list		argptr;
	char		string[2048];
	client_t	*cl;
	int			i;

	va_start (argptr,fmt);
	vsnprintf (ARRAY_ARG(string), fmt,argptr);
	va_end (argptr);

	// echo to console
	if (DEDICATED)
	{
		char	copy[1024];
		int		i;

		// mask off high bits
		for (i = 0; i < sizeof(copy)-1 && string[i] ; i++)
			copy[i] = string[i] & 127;
		copy[i] = 0;
		Com_Printf ("%s", copy);
	}

	for (i = 0, cl = svs.clients; i < maxclients->integer; i++, cl++)
	{
		if (level < cl->messagelevel)
			continue;
		if (cl->state != cs_spawned)
			continue;
		MSG_WriteByte (&cl->netchan.message, svc_print);
		MSG_WriteByte (&cl->netchan.message, level);
		MSG_WriteString (&cl->netchan.message, string);
	}
}

/*
=================
SV_BroadcastCommand

Sends text to all active clients
=================
*/
void SV_BroadcastCommand (char *fmt, ...)
{
	va_list		argptr;
	char		string[1024];

	if (!sv.state)
		return;
	va_start (argptr,fmt);
	vsnprintf (ARRAY_ARG(string), fmt,argptr);
	va_end (argptr);

	MSG_WriteByte (&sv.multicast, svc_stufftext);
	MSG_WriteString (&sv.multicast, string);
	SV_MulticastOld (NULL, MULTICAST_ALL_R);
}


/*
=================
SV_Multicast

Sends the contents of sv.multicast to a subset of the clients,
then clears sv.multicast.

MULTICAST_ALL	same as broadcast (origin can be NULL)
MULTICAST_PVS	send to clients potentially visible from org
MULTICAST_PHS	send to clients potentially hearable from org
=================
*/
void SV_Multicast (vec3_t origin, multicast_t to, qboolean oldclients)
{
	client_t	*client;
	byte		*mask;
	int			leafnum, cluster;
	int			j;
	qboolean	reliable;
	int			area1, area2;
	sizebuf_t	*buf, *newBuf;

	guard(SV_Multicast);

	reliable = false;

	if (to != MULTICAST_ALL_R && to != MULTICAST_ALL)
	{
		leafnum = CM_PointLeafnum (origin);
		area1 = CM_LeafArea (leafnum);
	}
	else
	{
		leafnum = 0;	// just to avoid compiler warnings
		area1 = 0;
	}

	// if doing a serverrecord, store everything
	if (svs.demofile)
		SZ_Write (&svs.demo_multicast, sv.multicast.data, sv.multicast.cursize);

	switch (to)
	{
	case MULTICAST_ALL_R:
		reliable = true;	// intentional fallthrough
	case MULTICAST_ALL:
		leafnum = 0;
		mask = NULL;
		break;

	case MULTICAST_PHS_R:
		reliable = true;	// intentional fallthrough
	case MULTICAST_PHS:
		leafnum = CM_PointLeafnum (origin);
		cluster = CM_LeafCluster (leafnum);
		mask = CM_ClusterPHS (cluster);
		break;

	case MULTICAST_PVS_R:
		reliable = true;	// intentional fallthrough
	case MULTICAST_PVS:
		leafnum = CM_PointLeafnum (origin);
		cluster = CM_LeafCluster (leafnum);
		mask = CM_ClusterPVS (cluster);
		break;

	default:
		mask = NULL;
		Com_FatalError ("SV_Multicast: bad to:%i", to);
	}

	// process transferring message and create a copy for new clients
	newBuf = (sv_extProtocol->integer) ? SV_MulticastHook (&sv.multicast, &sv.multicastNew) : &sv.multicast;

	// send the data to all relevent clients
	for (j = 0, client = svs.clients; j < maxclients->integer; j++, client++)
	{
		if (client->state == cs_free || client->state == cs_zombie)
			continue;
		if (client->state != cs_spawned && !reliable)
			continue;
		if (!oldclients && !client->newprotocol)
			continue;

		buf = (client->newprotocol) ? newBuf : &sv.multicast;

		if (mask)
		{
			leafnum = CM_PointLeafnum (client->edict->s.origin);
			cluster = CM_LeafCluster (leafnum);
			area2 = CM_LeafArea (leafnum);
			if (!CM_AreasConnected (area1, area2))
				continue;
			if ( mask && (!(mask[cluster>>3] & (1<<(cluster&7)) ) ) )
				continue;
		}

		if (reliable)
			SZ_Write (&client->netchan.message, buf->data, buf->cursize);
		else
			SZ_Write (&client->datagram, buf->data, buf->cursize);
	}

	SZ_Clear (&sv.multicast);
	SZ_Clear (&sv.multicastNew);

	unguard;
}

void SV_MulticastOld (vec3_t origin, multicast_t to)
{
	SV_Multicast (origin, to, true);	// send to old and new clients
}

void SV_MulticastNew (vec3_t origin, multicast_t to)
{
	SV_Multicast (origin, to, false);	// send to new clients only
}



/*
==================
SV_StartSound

Each entity can have eight independant sound sources, like voice,
weapon, feet, etc.

If cahnnel & 8, the sound will be sent to everyone, not just
things in the PHS.

FIXME: if entity isn't in PHS, they must be forced to be sent or
have the origin explicitly sent.

Channel 0 is an auto-allocate channel, the others override anything
already running on that entity/channel pair.

An attenuation of 0 will play full volume everywhere in the level.
Larger attenuations will drop off.  (max 4 attenuation)

Timeofs can range from 0.0 to 0.1 to cause sounds to be started
later in the frame than they normally would.

If origin is NULL, the origin is determined from the entity origin
or the midpoint of the entity box for bmodels.
==================
*/
void SV_StartSound (vec3_t origin, edict_t *entity, int channel,
		int soundindex, float volume, float attenuation, float timeofs, qboolean oldclients)
{
	int			sendchan, flags, i, ent, to;
	vec3_t		origin_v;
	qboolean	use_phs;

	guard(SV_StartSound);

	if (volume < 0 || volume > 1.0)
		Com_FatalError ("SV_StartSound: volume = %f", volume);

	if (attenuation < 0 || attenuation > 4)
		Com_FatalError ("SV_StartSound: attenuation = %f", attenuation);

//	if (channel < 0 || channel > 15)
//		Com_FatalError ("SV_StartSound: channel = %i", channel);

	if (timeofs < 0 || timeofs > 0.255)
		Com_FatalError ("SV_StartSound: timeofs = %f", timeofs);

	ent = NUM_FOR_EDICT(entity);

	if (channel & 8)	// no PHS flag
	{
		use_phs = false;
		channel &= 7;
	}
	else
		use_phs = true;

	sendchan = (ent<<3) | (channel&7);

	flags = 0;
	if (volume != DEFAULT_SOUND_PACKET_VOLUME)
		flags |= SND_VOLUME;
	if (attenuation != DEFAULT_SOUND_PACKET_ATTENUATION)
		flags |= SND_ATTENUATION;

	// the client doesn't know that bmodels have weird origins
	// the origin can also be explicitly set
	if ( (entity->svflags & SVF_NOCLIENT)
		|| (entity->solid == SOLID_BSP)
		|| origin )
		flags |= SND_POS;

	// always send the entity number for channel overrides
	flags |= SND_ENT;

	if (timeofs)
		flags |= SND_OFFSET;

	// use the entity origin unless it is a bmodel or explicitly specified
	if (!origin)
	{
		origin = origin_v;
		if (entity->solid == SOLID_BSP)
		{
			for (i=0 ; i<3 ; i++)
				origin_v[i] = entity->s.origin[i]+0.5*(entity->mins[i]+entity->maxs[i]);
		}
		else
		{
			VectorCopy (entity->s.origin, origin_v);
		}
	}

	MSG_WriteByte (&sv.multicast, svc_sound);
	MSG_WriteByte (&sv.multicast, flags);
	MSG_WriteByte (&sv.multicast, soundindex);

	if (flags & SND_VOLUME)
		MSG_WriteByte (&sv.multicast, Q_round (volume*255));
	if (flags & SND_ATTENUATION)
		MSG_WriteByte (&sv.multicast, Q_round (attenuation*64));
	if (flags & SND_OFFSET)
		MSG_WriteByte (&sv.multicast, Q_round (timeofs*1000));

	if (flags & SND_ENT)
		MSG_WriteShort (&sv.multicast, sendchan);

	if (flags & SND_POS)
		MSG_WritePos (&sv.multicast, origin);

	// if the sound doesn't attenuate,send it to everyone
	// (global radio chatter, voiceovers, etc)
	if (attenuation == ATTN_NONE)
		use_phs = false;

	if (channel & CHAN_RELIABLE)
	{
		if (use_phs)
			to = MULTICAST_PHS_R;
		else
			to = MULTICAST_ALL_R;
	}
	else
	{
		if (use_phs)
			to = MULTICAST_PHS;
		else
			to = MULTICAST_ALL;
	}

	if (oldclients)
		SV_MulticastOld (origin, to);
	else
		SV_MulticastNew (origin, to);

	unguard;
}


void SV_StartSoundOld (vec3_t origin, edict_t *entity, int channel,
		int soundindex, float volume, float attenuation, float timeofs)
{
	SV_StartSound (origin, entity, channel, soundindex, volume, attenuation, timeofs, true);
}

void SV_StartSoundNew (vec3_t origin, edict_t *entity, int channel,
		int soundindex, float volume, float attenuation, float timeofs)
{
	SV_StartSound (origin, entity, channel, soundindex, volume, attenuation, timeofs, false);
}

/*
===============================================================================

FRAME UPDATES

===============================================================================
*/



/*
=======================
SV_SendClientDatagram
=======================
*/
qboolean SV_SendClientDatagram (client_t *client)
{
	byte		msg_buf[MAX_MSGLEN];
	sizebuf_t	msg;

	SV_BuildClientFrame (client);

	SZ_Init (&msg, msg_buf, client->maxPacketSize);
	msg.allowoverflow = true;

	// send over all the relevant entity_state_t
	// and the player_state_t
	SV_WriteFrameToClient (client, &msg);

	// copy the accumulated multicast datagram
	// for this client out to the message
	// it is necessary for this to be after the WriteEntities
	// so that entity references will be current
	if (client->datagram.overflowed)
		Com_WPrintf ("Datagram overflowed for %s\n", client->name);
	else
		SZ_Write (&msg, client->datagram.data, client->datagram.cursize);
	SZ_Clear (&client->datagram);

	if (msg.overflowed)
	{	// must have room left for the packet header
		Com_WPrintf ("Msg overflowed for %s\n", client->name);
		SZ_Clear (&msg);
	}

	// send the datagram
	Netchan_Transmit (&client->netchan, msg.cursize, msg.data);

	// record the size for rate estimation
	client->message_size[sv.framenum % RATE_MESSAGES] = msg.cursize;

	return true;
}


/*
==================
SV_DemoCompleted
==================
*/
void SV_DemoCompleted (void)
{
	if (sv.demofile)
	{
		FS_FCloseFile (sv.demofile);
		sv.demofile = NULL;
	}
	SV_Nextserver ();
}


/*
=======================
SV_RateDrop

Returns true if the client is over its current
bandwidth estimation and should not be sent another packet
=======================
*/
qboolean SV_RateDrop (client_t *c)
{
	int		total;
	int		i;

	// never drop over the loopback
	if (c->netchan.remote_address.type == NA_LOOPBACK)
		return false;

	total = 0;
	for (i = 0 ; i < RATE_MESSAGES ; i++)
		total += c->message_size[i];

	if (total > c->rate)
	{
		c->surpressCount++;
		c->message_size[sv.framenum % RATE_MESSAGES] = 0;
		return true;
	}

	return false;
}

/*
=======================
SV_SendClientMessages
=======================
*/
void SV_SendClientMessages (void)
{
	int			i;
	client_t	*c;
	int			msglen;
	byte		msgbuf[MAX_MSGLEN];

	msglen = 0;

	// read the next demo message if needed
	if (sv.state == ss_demo && sv.demofile)
	{
		if (sv_paused->integer)
			msglen = 0;
		else
		{
			// get the next message
			FS_Read (&msglen, 4, sv.demofile);
			msglen = LittleLong (msglen);
			if (msglen == -1)
			{
				SV_DemoCompleted ();
				return;
			}
			if (msglen > MAX_MSGLEN)
				Com_DropError ("SV_SendClientMessages: msglen > MAX_MSGLEN");
			FS_Read (msgbuf, msglen, sv.demofile);
		}
	}

	// send a message to each connected client
	for (i = 0, c = svs.clients ; i < maxclients->integer; i++, c++)
	{
		if (!c->state) continue;
		// if the reliable message overflowed, drop the client
		if (c->netchan.message.overflowed)
		{
			SZ_Clear (&c->netchan.message);
			SZ_Clear (&c->datagram);
			SV_DropClient (c, "overflowed");
		}

		if (sv.state == ss_cinematic || sv.state == ss_demo || sv.state == ss_pic)
			Netchan_Transmit (&c->netchan, msglen, msgbuf);
		else if (c->state == cs_spawned)
		{
			// don't overrun bandwidth
			if (SV_RateDrop (c)) continue;
			SV_SendClientDatagram (c);
		}
		else
		{
			// just update reliable	if needed
			if (c->netchan.message.cursize || curtime - c->netchan.last_sent > 1000)
				Netchan_Transmit (&c->netchan, 0, NULL);
		}
	}
}
