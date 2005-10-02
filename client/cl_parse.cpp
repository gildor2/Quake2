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
// cl_parse.cpp  -- parse a message received from the server

#include "client.h"

const char *svc_strings[svc_last] =
{
	"svc_bad",

	"svc_muzzleflash",
	"svc_muzzlflash2",
	"svc_temp_entity",
	"svc_layout",
	"svc_inventory",

	"svc_nop",
	"svc_disconnect",
	"svc_reconnect",
	"svc_sound",
	"svc_print",
	"svc_stufftext",
	"svc_serverdata",
	"svc_configstring",
	"svc_spawnbaseline",
	"svc_centerprint",
	"svc_download",
	"svc_playerinfo",
	"svc_packetentities",
	"svc_deltapacketentities",
	"svc_frame"
};

/*
=====================================================================

  SERVER CONNECTING MESSAGES

=====================================================================
*/

static void ParseServerData (void)
{
	Com_DPrintf ("Serverdata packet received.\n");

	// wipe the client_state_t struct
	CL_ClearState ();
	cls.state = ca_connected;

	// parse protocol version number
	int i = MSG_ReadLong (&net_message);
	cls.serverProtocol = i;

	if (Com_ServerState() == ss_demo)
	{
		if (cls.serverProtocol < 0)
		{
			cls.serverProtocol = -cls.serverProtocol;
			Com_DPrintf ("demo recorded with extended protocol\n");
		}
		else
		{
			cls.newprotocol = false;
			Com_DPrintf ("demo recorded with protocol %d\n", cls.serverProtocol);
		}
		// here: if serverProtocol != PROTOCOL_VERSION -- allow demo to play
	}
	else if (i != PROTOCOL_VERSION)
		Com_DropError ("Server returned version %d <> " STR(PROTOCOL_VERSION), i);

	cl.servercount = MSG_ReadLong (&net_message);
	cl.attractloop = MSG_ReadByte (&net_message) != 0;

	// game directory
	const char *str = MSG_ReadString (&net_message);
	appStrncpyz (cl.gamedir, str, sizeof(cl.gamedir));
	//?? Cvar_Set() -- will not change gamedir at all ("for next game"); Cvar_ForceSet() -- will change gamedir even when playing demos
	//?? best way: detect server's game before connection, and change when needed (may be, display warning for user)
	if (!cl.attractloop)	// not in demos ...
		Cvar_ForceSet ("game", str);

	// parse player entity number
	cl.playernum = MSG_ReadShort (&net_message);

	// get the full level name
	str = MSG_ReadString (&net_message);

	if (cl.playernum == -1)
	{	// playing a cinematic or showing a pic, not a level
		SCR_PlayCinematic (str);
	}
	else
	{
		appPrintf (S_RED"\n\n====================================\n\n");
//		appPrintf (S_RED"\n\n\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37\n\n");
		appPrintf (S_RED"%s\n", str);			// display map message

		// need to prep refresh at next oportunity
		cl.rendererReady = false;
	}
}


static void ParseBaseline (void)
{
	clEntityState_t nullstate;
	memset (&nullstate, 0, sizeof(nullstate));

	unsigned	bits;
	int newnum = MSG_ReadEntityBits (&net_message, &bits, NULL);
	CL_ParseDelta (&nullstate, &cl_entities[newnum].baseline, newnum, bits, true);
}


void CL_ParseClientinfo (int player)
{
	clientInfo_t &ci = cl.clientInfo[player];
	CL_LoadClientinfo (ci, cl.configstrings[player+CS_PLAYERSKINS]);

	if (ci.isQ3model && !cls.newprotocol)
	{
		appWPrintf ("No extended protocol support.\nForce player %s to use base model\n", *ci.PlayerName);
		ci.isValidModel = false;
	}
	if (!ci.isValidModel)
		CL_LoadClientinfo (ci, va("%s\\male/grunt", *ci.PlayerName));
	if (gender_auto->integer && player == cl.playernum)
	{
//		Com_Drintf("Fix gender: [%d]=%c\n",cl.playernum,cl.clientInfo[cl.playernum].modelGender);
		Cvar_Set ("gender", ci.modelGender == 'f' ? "female" : "male");
		gender->modified = false;
	}
}


void CL_UpdatePlayerClientInfo ()
{
	appSprintf (ARRAY_ARG(cl.configstrings[CS_PLAYERSKINS+cl.playernum]),
		"%s\\%s", Cvar_VariableString("name"), Cvar_VariableString("skin"));
	CL_ParseClientinfo (cl.playernum);
}


/*
================
ParseConfigString
================
*/
static void ParseConfigString (void)
{
	int		i;
	char	*s;
	char	olds[MAX_QPATH];

	i = MSG_ReadShort (&net_message);
	if (i < 0 || i >= MAX_CONFIGSTRINGS)
		Com_DropError ("configstring > MAX_CONFIGSTRINGS");
	s = MSG_ReadString(&net_message);

	appStrncpyz (olds, cl.configstrings[i], sizeof(olds));

	strcpy (cl.configstrings[i], s);

	// do something apropriate

	if (i >= CS_LIGHTS && i < CS_LIGHTS+MAX_LIGHTSTYLES)
		CL_SetLightstyle (i - CS_LIGHTS, cl.configstrings[i]);
	else if (i == CS_CDTRACK)
	{
		if (cl.rendererReady)
			CDAudio_Play (atoi(cl.configstrings[CS_CDTRACK]), true);
	}
	else if (i >= CS_MODELS && i < CS_MODELS+MAX_MODELS)
	{
		if (cl.rendererReady)
		{
			cl.model_draw[i-CS_MODELS] = RE_RegisterModel (cl.configstrings[i]);
			if (cl.configstrings[i][0] == '*')
				cl.model_clip[i-CS_MODELS] = CM_InlineModel (cl.configstrings[i]);
			else
				cl.model_clip[i-CS_MODELS] = NULL;
		}
	}
	else if (i >= CS_SOUNDS && i < CS_SOUNDS+MAX_SOUNDS)
	{
		if (cl.rendererReady)
			cl.sound_precache[i-CS_SOUNDS] = S_RegisterSound (cl.configstrings[i]);
	}
	else if (i >= CS_IMAGES && i < CS_IMAGES+MAX_IMAGES)
	{
		if (cl.rendererReady)
			cl.image_precache[i-CS_IMAGES] = RE_RegisterPic (va("pics/%s", cl.configstrings[i]));
	}
	else if (i >= CS_PLAYERSKINS && i < CS_PLAYERSKINS+MAX_CLIENTS)
	{
		if (cl.rendererReady && strcmp(olds, s))
			CL_ParseClientinfo (i-CS_PLAYERSKINS);
	}
}


/*
=====================================================================

ACTION MESSAGES

=====================================================================
*/

static void ParseStartSoundPacket ()
{
	int flags = MSG_ReadByte (&net_message);
	int sound_num = MSG_ReadByte (&net_message);

	float volume = (flags & SND_VOLUME) ? MSG_ReadByte (&net_message) / 255.0 : DEFAULT_SOUND_PACKET_VOLUME;
	float attenuation = (flags & SND_ATTENUATION) ? MSG_ReadByte (&net_message) / 64.0 : DEFAULT_SOUND_PACKET_ATTENUATION;
	float ofs = (flags & SND_OFFSET) ? MSG_ReadByte (&net_message) / 1000.0 : 0;

	int 	channel, ent;
	if (flags & SND_ENT)
	{	// entity reletive
		channel = MSG_ReadShort(&net_message);
		ent = channel>>3;
		if (ent > MAX_EDICTS)
			Com_DropError ("ParseStartSoundPacket: ent = %i", ent);

		channel &= 7;
	}
	else
	{
		ent = 0;
		channel = 0;
	}

	CVec3	pos_v;
	CVec3	*pos;
	if (flags & SND_POS)
	{	// positioned in space
		MSG_ReadPos (&net_message, pos_v);
		pos = &pos_v;
	}
	else	// use entity number
		pos = NULL;

	if (!cl.sound_precache[sound_num])
		return;

	S_StartSound (pos, ent, channel, cl.sound_precache[sound_num], volume, attenuation, ofs);
}


static void AddNetgraph (void)
{
	int		i;

	for (i = 0; i < cls.netchan.dropped; i++)
		SCR_DebugGraph (30, 0x40);

	for (i = 0; i < cl.surpressCount ; i++)
		SCR_DebugGraph (30, 0xDF);

	// see what the latency was on this packet
	int in = cls.netchan.incoming_acknowledged & (CMD_BACKUP-1);
	int ping = (cls.realtime - cl.cmd_time[in]) / 30;
	if (ping > 30) ping = 30;
	SCR_DebugGraph (ping, 0xD0);
}


void CL_ParseServerMessage (void)
{
	int cmd = svc_bad;
	guard(CL_ParseServerMessage);

	// if recording demos, copy the message out
	if (cl_shownet->integer == 1)
		appPrintf ("%i ",net_message.cursize);
	else if (cl_shownet->integer >= 2)
		appPrintf ("------------------\n");

	// parse the message
	while (true)
	{
		const char *s;
		if (net_message.readcount > net_message.cursize)
		{
			Com_DropError ("CL_ParseServerMessage: Bad server message");
			break;
		}

		cmd = MSG_ReadByte (&net_message);

		if (cmd == -1)
		{
			SHOWNET("END OF MESSAGE");
			break;
		}

		if (cmd >= svc_last)
			Com_DropError ("bad net cmd %d", cmd);

		if (cl_shownet->integer >= 2)
			SHOWNET(svc_strings[cmd]);

		// other commands
		switch (cmd)
		{
		case svc_nop:
			break;

		case svc_disconnect:
			// either client kicked/banned (dropped from server), or server was quit
			Com_DropError (NULL);	// no message
			break;

		case svc_reconnect:
			appPrintf ("Server disconnected, reconnecting\n");
			if (cls.download)
			{
				//ZOID, close download
				fclose (cls.download);
				cls.download = NULL;
			}
			cls.state = ca_connecting;
			cls.connect_time = -BIG_NUMBER;	// CL_InitiateConnection() will fire immediately
			break;

		case svc_print:
			if (MSG_ReadByte (&net_message) == PRINT_CHAT)
			{
				S_StartLocalSound ("misc/talk.wav");
				s = S_GREEN;
			}
			else
				s = "";
			appPrintf ("%s%s", s, MSG_ReadString (&net_message));
			break;

		case svc_centerprint:
			SCR_CenterPrint (MSG_ReadString (&net_message));
			break;

		case svc_stufftext:
			s = MSG_ReadString (&net_message);
			if (cl.attractloop && (memcmp (s, "precache", 8) && memcmp (s, "changing", 8) && memcmp (s, "reconnect", 9)))
			// demo playback: allow special stufftext only (should disable messages from demofile and allow local !!)
			{
				Com_DPrintf ("stuff (disabled): %s\n", s);
				break;
			}
			Com_DPrintf ("stufftext: %s\n", s);
			Cbuf_AddText (s);
			break;

		case svc_serverdata:
			Cbuf_Execute ();		// make sure any stuffed commands are done
			ParseServerData ();
			break;

		case svc_configstring:
			ParseConfigString ();
			break;

		case svc_sound:
			ParseStartSoundPacket();
			break;

		case svc_spawnbaseline:
			ParseBaseline ();
			break;

		case svc_temp_entity:
			CL_ParseTEnt ();
			break;

		case svc_muzzleflash:
			CL_ParseMuzzleFlash ();
			break;

		case svc_muzzleflash2:
			CL_ParseMuzzleFlash2 ();
			break;

		case svc_download:
			CL_ParseDownload ();
			break;

		case svc_frame:
			CL_ParseFrame ();
			break;

		case svc_inventory:
			{
				for (int i = 0; i < MAX_ITEMS; i++)
					cl.inventory[i] = MSG_ReadShort (&net_message);
			}
			break;

		case svc_layout:
			s = MSG_ReadString (&net_message);
			appStrncpyz (cl.layout, s, sizeof(cl.layout));
			break;

		case svc_playerinfo:
		case svc_packetentities:
		case svc_deltapacketentities:
			Com_DropError ("Out of place frame data");
			break;

		default:
			Com_DropError ("CL_ParseServerMessage: bad server message %s", svc_strings[cmd]);
		}
	}

	AddNetgraph ();

	// we don't know if it is ok to save a demo message until
	// after we have parsed the frame
	if (cls.demorecording && !cls.demowaiting)
		CL_WriteDemoMessage ();

	unguardf(("cmd=%s", svc_strings[cmd]));
}
