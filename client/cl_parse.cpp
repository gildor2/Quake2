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

/*
==================
ParseServerData
==================
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
	char *str = MSG_ReadString (&net_message);
	appStrncpyz (cl.gamedir, str, sizeof(cl.gamedir));
	Cvar_Set ("game", str);

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
		Com_Printf (S_RED"\n\n\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37\n\n");
		Com_Printf (S_RED"%s\n", str);			// display map message

		// need to prep refresh at next oportunity
		cl.refresh_prepped = false;
	}
}

/*
==================
ParseBaseline
==================
*/
static void ParseBaseline (void)
{
	entityState_t nullstate;
	memset (&nullstate, 0, sizeof(nullstate));

	unsigned	bits;
	int newnum = MSG_ReadEntityBits (&net_message, &bits, NULL);
	CL_ParseDelta (&nullstate, &cl_entities[newnum].baseline, newnum, bits, true);
}


/*
================
CL_LoadClientinfo

================
*/
void CL_LoadClientinfo (clientinfo_t *ci, char *s)
{
	char	model_name[MAX_QPATH], skin_name[MAX_QPATH];

	appStrncpyz (ci->cinfo, s, sizeof(ci->cinfo));

	// isolate the player's name
	appStrncpyz (ci->name, s, sizeof(ci->name));
	char *t = strchr (s, '\\');
	if (t)
	{
		ci->name[t-s] = 0;
		s = t+1;
	}

	if (cl_noskins->integer || *s == 0)
	{
		ci->model = re.RegisterModel ("players/male/tris.md2");
		memset (ci->weaponmodel, 0, sizeof(ci->weaponmodel));
		ci->weaponmodel[0] = re.RegisterModel ("players/male/weapon.md2");
		ci->skin = re.RegisterSkin ("players/male/grunt.pcx");
		strcpy (ci->iconname, "/players/male/grunt_i.pcx");
		ci->icon = re.RegisterPic (ci->iconname);
	}
	else
	{
		// isolate the model name
		strcpy (model_name, s);
		t = strchr (model_name, '/');
		if (!t)
			t = strchr (model_name, '\\');
		if (!t)
			t = model_name;
		*t = 0;

		// isolate the skin name
		strcpy (skin_name, s + strlen (model_name) + 1);

		// model file
		ci->model = re.RegisterModel (va("players/%s/tris.md2", model_name));
		if (!ci->model)
		{
			strcpy (model_name, "male");
			ci->model = re.RegisterModel ("players/male/tris.md2");
		}

		// skin file
		ci->skin = re.RegisterSkin (va("players/%s/%s.pcx", model_name, skin_name));

		// if we don't have the skin and the model wasn't male,
		// see if the male has it (this is for CTF's skins)
 		if (!ci->skin && stricmp(model_name, "male"))
		{
			// change model to male
			strcpy(model_name, "male");
			ci->model = re.RegisterModel ("players/male/tris.md2");

			// see if the skin exists for the male model
			ci->skin = re.RegisterSkin (va("players/%s/%s.pcx", model_name, skin_name));
		}

		// if we still don't have a skin, it means that the male model didn't have
		// it, so default to grunt
		if (!ci->skin)
		{
			// see if the skin exists for the male model
			ci->skin = re.RegisterSkin (va("players/%s/grunt.pcx", model_name));
		}

		// weapon file
		for (int i = 0; i < num_cl_weaponmodels; i++)
		{
			ci->weaponmodel[i] = re.RegisterModel(va("players/%s/%s", model_name, cl_weaponmodels[i]));
			if (!ci->weaponmodel[i] && strcmp(model_name, "cyborg") == 0)
			{
				// try male
				ci->weaponmodel[i] = re.RegisterModel(va("players/male/%s", cl_weaponmodels[i]));
			}
			if (!cl_vwep->integer)
				break; // only one when vwep is off
		}

		// icon file
		if (!memcmp (skin_name, "skn_", 4))
			strcpy (ci->iconname, "/pics/default_icon.pcx");
		else
			appSprintf (ci->iconname, sizeof(ci->iconname), "/players/%s/%s_i.pcx", model_name, skin_name);
		ci->icon = re.RegisterPic (ci->iconname);
	}

	// must have loaded all data types to be valud
	if (!ci->skin || !ci->icon || !ci->model || !ci->weaponmodel[0])
	{
		ci->skin = NULL;
		ci->icon = NULL;
		ci->model = NULL;
		ci->weaponmodel[0] = NULL;
	}
}

/*
================
CL_ParseClientinfo

Load the skin, icon, and model for a client
================
*/
void CL_ParseClientinfo (int player)
{
	CL_LoadClientinfo (&cl.clientinfo[player], cl.configstrings[player+CS_PLAYERSKINS]);
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
		if (cl.refresh_prepped)
			CDAudio_Play (atoi(cl.configstrings[CS_CDTRACK]), true);
	}
	else if (i >= CS_MODELS && i < CS_MODELS+MAX_MODELS)
	{
		if (cl.refresh_prepped)
		{
			cl.model_draw[i-CS_MODELS] = re.RegisterModel (cl.configstrings[i]);
			if (cl.configstrings[i][0] == '*')
				cl.model_clip[i-CS_MODELS] = CM_InlineModel (cl.configstrings[i]);
			else
				cl.model_clip[i-CS_MODELS] = NULL;
		}
	}
	else if (i >= CS_SOUNDS && i < CS_SOUNDS+MAX_MODELS)
	{
		if (*re.flags & REF_CONSOLE_ONLY)
			return;

		if (cl.refresh_prepped)
			cl.sound_precache[i-CS_SOUNDS] = S_RegisterSound (cl.configstrings[i]);
	}
	else if (i >= CS_IMAGES && i < CS_IMAGES+MAX_MODELS)
	{
		if (cl.refresh_prepped)
			cl.image_precache[i-CS_IMAGES] = re.RegisterPic (cl.configstrings[i]);
	}
	else if (i >= CS_PLAYERSKINS && i < CS_PLAYERSKINS+MAX_CLIENTS)
	{
		if (cl.refresh_prepped && strcmp(olds, s))
			CL_ParseClientinfo (i-CS_PLAYERSKINS);
	}
}


/*
=====================================================================

ACTION MESSAGES

=====================================================================
*/

/*
==================
ParseStartSoundPacket
==================
*/
static void ParseStartSoundPacket(void)
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

	vec3_t  pos_v;
	float	*pos;
	if (flags & SND_POS)
	{	// positioned in space
		MSG_ReadPos (&net_message, pos_v);
		pos = pos_v;
	}
	else	// use entity number
		pos = NULL;

	if (!cl.sound_precache[sound_num])
		return;

	S_StartSound (pos, ent, channel, cl.sound_precache[sound_num], volume, attenuation, ofs);
}


/*
==============
AddNetgraph

A new packet was just parsed
==============
*/
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


/*
=====================
CL_ParseServerMessage
=====================
*/
void CL_ParseServerMessage (void)
{
	int		cmd;
	const char *s;

	guard(CL_ParseServerMessage);

	// if recording demos, copy the message out
	if (cl_shownet->integer == 1)
		Com_Printf ("%i ",net_message.cursize);
	else if (cl_shownet->integer >= 2)
		Com_Printf ("------------------\n");

	// parse the message
	while (true)
	{
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
			Com_Printf ("Server disconnected, reconnecting\n");
			if (cls.download)
			{
				//ZOID, close download
				fclose (cls.download);
				cls.download = NULL;
			}
			cls.state = ca_connecting;
			cls.connect_time = -BIG_NUMBER;	// CL_CheckForResend() will fire immediately
			break;

		case svc_print:
			if (MSG_ReadByte (&net_message) == PRINT_CHAT)
			{
				S_StartLocalSound ("misc/talk.wav");
				s = S_GREEN;
			}
			else
				s = "";
			Com_Printf ("%s%s", s, MSG_ReadString (&net_message));
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
