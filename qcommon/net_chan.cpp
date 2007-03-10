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

/*

packet header
-------------
31	sequence
1	does this message contain a reliable payload
31	acknowledge sequence
1	acknowledge receipt of even/odd message
16	qport

The remote connection never knows if it missed a reliable message, the
local side detects that it has been dropped by seeing a sequence acknowledge
higher thatn the last reliable sequence, but without the correct evon/odd
bit for the reliable set.

If the sender notices that a reliable message has been dropped, it will be
retransmitted.  It will not be retransmitted again until a message after
the retransmit has been acknowledged and the reliable still failed to get there.

if the sequence number is -1, the packet should be handled without a netcon

The reliable message can be added to at any time by doing
MSG_Write* (&netchan->message, <data>).

If the message buffer is overflowed, either by a single message, or by
multiple frames worth piling up while the last reliable transmit goes
unacknowledged, the netchan signals a fatal error.

Reliable messages are always placed first in a packet, then the unreliable
message is included if there is sufficient room.

To the receiver, there is no distinction between the reliable and unreliable
parts of the message, they are just processed out as a single larger message.

Illogical packet sequence numbers cause the packet to be dropped, but do
not kill the connection.  This, combined with the tight window of valid
reliable acknowledgement numbers provides protection against malicious
address spoofing.


The qport field is a workaround for bad address translating routers that
sometimes remap the client's source port on a packet during gameplay.

If the base part of the net address matches and the qport matches, then the
channel matches even if the IP port differs.  The IP port should be updated
to the new value before sending out any replies.


If there is no information that needs to be transfered on a given frame,
such as during the connection stage while waiting for the client to load,
then a packet only needs to be delivered if there is something in the
unacknowledged reliable
*/

static cvar_t *showpackets;
static cvar_t *showdrop;
static cvar_t *qport;

netadr_t	net_from;
sizebuf_t	net_message;
static byte	message_buffer[MAX_MSGLEN];


void Netchan_Init()
{
	showpackets = Cvar_Get("showpackets", "0");
	showdrop    = Cvar_Get("showdrop", "0");
	// port value that should be nice and random
	qport       = Cvar_Get("qport", va("%d", appMilliseconds() & 0xFFFF), CVAR_NOSET);
	net_message.Init(ARRAY_ARG(message_buffer));
}


// Sends a text message in an out-of-band datagram
void Netchan_OutOfBandPrint(netsrc_t net_socket, netadr_t adr, const char *format, ...)
{
	va_list argptr;
	char string[MAX_MSGLEN - 4];

	va_start(argptr, format);
	int len = vsnprintf(ARRAY_ARG(string), format,argptr);
	va_end(argptr);
	if (len < 0) len = ARRAY_COUNT(string);	// buffer overflowed

	// write the packet header
	sizebuf_t	send;		//?? almost useless here
	byte		send_buf[MAX_MSGLEN];
	send.Init(ARRAY_ARG(send_buf));
	MSG_WriteLong(&send, -1);	// -1 sequence means out of band
	send.Write(string, len);

	// send the datagram
	NET_SendPacket(net_socket, send.cursize, send.data, adr);
}


// called to open a channel to a remote system
void netchan_t::Setup(netsrc_t sock, netadr_t adr, int qport)
{
	memset(this, 0, sizeof(netchan_t));

	this->sock        = sock;
	remote_address    = adr;
	port              = qport;
	last_received     = appMilliseconds();
	incoming_sequence = 0;
	outgoing_sequence = 1;

	this->message.Init(ARRAY_ARG(message_buf));
	this->message.allowoverflow = true;
}


bool netchan_t::NeedReliable()
{
	// if the remote side dropped the last reliable message, resend it
	if (incoming_acknowledged > last_reliable_sequence &&
		incoming_reliable_acknowledged != reliable_sequence)
		return true;

	// if the reliable transmit buffer is empty, copy the current message out
	if (!reliable_length && message.cursize)
		return true;

	return false;
}


// tries to send an unreliable message to a connection, and handles the
// transmition / retransmition of the reliable messages.
// A 0 length will still generate a packet and deal with the reliable messages (retransmit).
void netchan_t::Transmit(void *data, int length)
{
	// check for message overflow
	if (message.overflowed)
	{
		appWPrintf("%s: outgoing message overflow\n", NET_AdrToString(&remote_address));
		return;
	}

	bool send_reliable = NeedReliable();

	if (!reliable_length && message.cursize)
	{
		memcpy(reliable_buf, message_buf, message.cursize);
		reliable_length = message.cursize;
		message.cursize = 0;
		reliable_sequence ^= 1;
	}

	// write the packet header
	sizebuf_t	send;
	byte		send_buf[MAX_MSGLEN];
	send.Init(ARRAY_ARG(send_buf));

	unsigned w1 = (outgoing_sequence & 0x7FFFFFFF) | (send_reliable << 31);
	unsigned w2 = (incoming_sequence & 0x7FFFFFFF) | (incoming_reliable_sequence << 31);

	outgoing_sequence++;
	last_sent = appMilliseconds();

	MSG_WriteLong(&send, w1);
	MSG_WriteLong(&send, w2);

	// send the qport if we are a client
	if (sock == NS_CLIENT)
		MSG_WriteShort(&send, qport->integer);

	// copy the reliable message to the packet first
	if (send_reliable)
	{
		send.Write(reliable_buf, reliable_length);
		last_reliable_sequence = outgoing_sequence;
	}

	// add the unreliable part if space is available
	if (send.maxsize - send.cursize >= length)
		send.Write(data, length);
	else
		appPrintf("netchan_t::Transmit: dumped unreliable\n");

	// send the datagram
	NET_SendPacket(sock, send.cursize, send.data, remote_address);

	if (showpackets->integer)
	{
		if (send_reliable)
			appPrintf("send %4i : s=%i reliable=%i ack=%i rack=%i\n",
				send.cursize, outgoing_sequence - 1, reliable_sequence, incoming_sequence, incoming_reliable_sequence);
		else
			appPrintf("send %4i : s=%i ack=%i rack=%i\n",
				send.cursize, outgoing_sequence - 1, incoming_sequence, incoming_reliable_sequence);
	}
}


// called when the current net_message is from remote_address
// modifies net_message so that it points to the packet payload
// NOTE: arg msg is always = net_message
bool netchan_t::Process(sizebuf_t *msg)
{
	// get sequence numbers
	msg->BeginReading();

	unsigned sequence     = MSG_ReadLong(msg);
	unsigned sequence_ack = MSG_ReadLong(msg);
	unsigned reliable_message = sequence     >> 31;
	unsigned reliable_ack     = sequence_ack >> 31;
	sequence     &= 0x7FFFFFFF;
	sequence_ack &= 0x7FFFFFFF;

	// read the qport if we are a server
	// value of qport analyzed in SV_ReadPackets() (function, called net_message.Process())
	if (sock == NS_SERVER) MSG_ReadShort(msg);

	if (showpackets->integer)
	{
		if (reliable_message)
			appPrintf("recv %4i : s=%i reliable=%i ack=%i rack=%i\n",
				msg->cursize, sequence, incoming_reliable_sequence ^ 1, sequence_ack, reliable_ack);
		else
			appPrintf("recv %4i : s=%i ack=%i rack=%i\n",
				msg->cursize, sequence, sequence_ack, reliable_ack);
	}

	// discard stale or duplicated packets
	if (sequence <= incoming_sequence)
	{
		if (showdrop->integer)
			appPrintf("%s: out of order packet %i at %i\n",
				NET_AdrToString(&remote_address), sequence, incoming_sequence);
		return false;
	}

	// dropped packets don't keep the message from being used
	dropped = sequence - (incoming_sequence+1);
	if (dropped > 0)
	{
		if (showdrop->integer)
			appPrintf("%s: dropped %i packets at %i\n",
				NET_AdrToString(&remote_address), dropped, sequence);
	}

	// if the current outgoing reliable message has been acknowledged
	// clear the buffer to make way for the next
	if (reliable_ack == reliable_sequence)
		reliable_length = 0;		// it has been received

	// if this message contains a reliable message, bump incoming_reliable_sequence
	incoming_sequence              = sequence;
	incoming_acknowledged          = sequence_ack;
	incoming_reliable_acknowledged = reliable_ack;
	if (reliable_message)
		incoming_reliable_sequence ^= 1;

	// the message can now be read from the current message pointer
	last_received = appMilliseconds();

	return true;
}
