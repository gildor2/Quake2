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
// cl_main.cpp  -- client main loop

#include "client.h"

cvar_t	*freelook;

cvar_t	*cl_stereo_separation;
cvar_t	*cl_stereo;

cvar_t	*rcon_client_password;
cvar_t	*rcon_address;

cvar_t	*cl_noskins;
cvar_t	*cl_autoskins;
cvar_t	*cl_footsteps;
cvar_t	*cl_timeout;
cvar_t	*cl_predict;
cvar_t	*cl_maxfps;
cvar_t	*cl_gun;

cvar_t	*cl_add_particles;
cvar_t	*cl_add_lights;
cvar_t	*cl_add_entities;
cvar_t	*cl_add_blend;

cvar_t	*cl_shownet;
cvar_t	*cl_showmiss;
cvar_t	*cl_showclamp;

cvar_t	*cl_paused;

cvar_t	*lookspring;
cvar_t	*lookstrafe;
cvar_t	*sensitivity;

cvar_t	*m_pitch;
cvar_t	*m_yaw;
cvar_t	*m_forward;
cvar_t	*m_side;

//
// userinfo
//
cvar_t	*info_password;
cvar_t	*info_spectator;
cvar_t	*name;
cvar_t	*skin;
cvar_t	*railcolor, *railtype;
cvar_t	*rate;
cvar_t	*fov;
cvar_t	*msg;
cvar_t	*hand;
cvar_t	*gender;
cvar_t	*gender_auto;

cvar_t	*cl_vwep;
cvar_t	*cl_3rd_person;
cvar_t	*cl_cameradist;
cvar_t	*cl_cameraheight;

cvar_t  *cl_extProtocol;

cvar_t	*cl_newfx;
cvar_t	*cl_showbboxes;
cvar_t	*r_sfx_pause;

cvar_t	*cl_infps;

client_static_t	cls;
client_state_t	cl;

centity_t		*cl_entities;	// [MAX_EDICTS]

entityState_t	cl_parse_entities[MAX_PARSE_ENTITIES];

//======================================================================


static bool statusRequest;
static bool cl_cheats;
static char	cl_mapname[MAX_QPATH];
static char	cl_gamename[MAX_QPATH];

static bool TryParseStatus (char *str)
{
	char	buf[MAX_MSGLEN];

	if (!statusRequest) return false;

	appStrncpyz (buf, str, sizeof(buf));
	// remove players info
	char *s = strchr (buf, '\n');
	if (!s) return false;
	*s = 0;

	// validate ServerInfo
	const char *v = Info_ValueForKey (buf, "mapname");
	if (!v) return false;
	strcpy (cl_mapname, v);

	v = Info_ValueForKey (buf, "gamename");
	if (!v) return false;
	strcpy (cl_gamename, v);

	v = Info_ValueForKey (buf, "cheats");
	if (!v) return false;
	cl_cheats = atoi (v) != 0;

//	Com_Printf (S_RED"map: %s  game: %s  cheats: %d\n", cl_mapname, cl_gamename, cl_cheats);

	statusRequest = false;
	return true;
}


/*
====================
CL_WriteDemoMessage

Dumps the current net message, prefixed by the length
====================
*/
void CL_WriteDemoMessage (void)
{
	int		len, swlen;

	// the first eight bytes are just packet sequencing stuff
	len = net_message.cursize-8;
	swlen = LittleLong(len);
	fwrite (&swlen, 4, 1, cls.demofile);
	fwrite (net_message.data+8,	len, 1, cls.demofile);
}


/*
====================
CL_Stop_f

stop recording a demo
====================
*/
static void CL_Stop_f (void)
{
	int		len;

	if (!cls.demorecording)
	{
		Com_WPrintf ("Not recording a demo.\n");
		return;
	}

	// finish up
	len = -1;
	fwrite (&len, 4, 1, cls.demofile);
	fclose (cls.demofile);
	cls.demofile = NULL;
	cls.demorecording = false;
	Com_Printf ("Stopped demo.\n");
}

/*
====================
CL_Record_f

record <demoname>

Begins recording a demo from the current position
====================
*/
static void CL_Record_f (bool usage, int argc, char **argv)
{
	char	name[MAX_OSPATH];
	byte	buf_data[MAX_MSGLEN];
	sizebuf_t	buf;
	int		i, len;
	entityState_t	*ent;
	entity_state_t	nullstate;

	if (argc != 2 || usage)
	{
		Com_Printf ("Usage: record <demoname>\n");
		return;
	}

	if (cls.demorecording)
	{
		Com_WPrintf ("Already recording.\n");
		return;
	}

	if (cls.state != ca_active)
	{
		Com_WPrintf ("You must be in a level to record.\n");
		return;
	}

	// open the demo file
	appSprintf (ARRAY_ARG(name), "%s/demos/%s.dm2", FS_Gamedir(), argv[1]);

	Com_Printf ("recording to %s.\n", name);
	FS_CreatePath (name);
	cls.demofile = fopen (name, "wb");
	if (!cls.demofile)
	{
		Com_WPrintf ("Cannot create file %s\n", name);
		return;
	}
	cls.demorecording = true;

	// don't start saving messages until a non-delta compressed message is received
	cls.demowaiting = true;

	// write out messages to hold the startup information
	SZ_Init (&buf, buf_data, sizeof(buf_data));

	// send the serverdata
	MSG_WriteByte (&buf, svc_serverdata);
	if (cls.newprotocol)
		MSG_WriteLong (&buf, -PROTOCOL_VERSION);
	else
		MSG_WriteLong (&buf, PROTOCOL_VERSION);
	MSG_WriteLong (&buf, 0x10000 + cl.servercount);
	MSG_WriteByte (&buf, 1);	// demos are always attract loops
	MSG_WriteString (&buf, cl.gamedir);
	MSG_WriteShort (&buf, cl.playernum);

	MSG_WriteString (&buf, cl.configstrings[CS_NAME]);

	// configstrings
	for (i = 0; i < MAX_CONFIGSTRINGS; i++)
	{
		if (cl.configstrings[i][0])
		{
			if (buf.cursize + strlen (cl.configstrings[i]) + 32 > buf.maxsize)
			{	// write it out
				len = LittleLong (buf.cursize);
				fwrite (&len, 4, 1, cls.demofile);
				fwrite (buf.data, buf.cursize, 1, cls.demofile);
				buf.cursize = 0;
			}

			MSG_WriteByte (&buf, svc_configstring);
			MSG_WriteShort (&buf, i);
			MSG_WriteString (&buf, cl.configstrings[i]);
		}
	}

	// baselines
	memset (&nullstate, 0, sizeof(nullstate));
	for (i = 0; i < MAX_EDICTS; i++)
	{
		ent = &cl_entities[i].baseline;
		if (!ent->modelindex)
			continue;

		if (buf.cursize + 64 > buf.maxsize)
		{	// write it out
			len = LittleLong (buf.cursize);
			fwrite (&len, 4, 1, cls.demofile);
			fwrite (buf.data, buf.cursize, 1, cls.demofile);
			buf.cursize = 0;
		}

		MSG_WriteByte (&buf, svc_spawnbaseline);
		MSG_WriteDeltaEntity (&buf, &nullstate, (entity_state_t*)&cl_entities[i].baseline, true, true);
	}

	MSG_WriteByte (&buf, svc_stufftext);
	MSG_WriteString (&buf, "precache\n");

	// write it to the demo file

	len = LittleLong (buf.cursize);
	fwrite (&len, 4, 1, cls.demofile);
	fwrite (buf.data, buf.cursize, 1, cls.demofile);

	// the rest of the demo file will be individual frames
}

//======================================================================

static void NetchanAppendArgs (char **argv, int first, int last)
{
	char	buf[1024];
	buf[0] = 0;
	for (int i = first; i < last; i++)
	{
		if (i > first) appStrcatn (ARRAY_ARG(buf), " ");
		appStrcatn (ARRAY_ARG(buf), argv[i]);
	}
	MSG_WriteString (&cls.netchan.message, buf);
}

/*
===================
Cmd_ForwardToServer

adds the current command line as a clc_stringcmd to the client message.
things like godmode, noclip, etc, are commands directed to the server,
so when they are typed in at the console, they will need to be forwarded.
===================
*/
void Cmd_ForwardToServer (int argc, char **argv)
{
	const char *cmd = argv[0];
	if (cls.state <= ca_connected || *cmd == '-' || *cmd == '+')
	{
		Com_WPrintf ("Unknown command \"%s\"\n", cmd);
		return;
	}

	MSG_WriteByte (&cls.netchan.message, clc_stringcmd);
	NetchanAppendArgs (argv, 0, argc);
}

/*
==================
CL_ForwardToServer_f
==================
*/
void CL_ForwardToServer_f (int argc, char **argv)
{
	if (cls.state != ca_connected && cls.state != ca_active)
	{
		Com_WPrintf ("Can't \"%s\", not connected\n", argv[0]);	// argv[0] == "cmd"
		return;
	}

	// don't forward the first argument
	if (argc > 1)
	{
		MSG_WriteByte (&cls.netchan.message, clc_stringcmd);
		NetchanAppendArgs (argv, 1, argc);
	}
}


/*
==================
CL_Pause_f
==================
*/

void CL_Pause (bool enable)
{
	if ((Cvar_VariableInt ("maxclients") == 1 || cl_cheats) && Com_ServerState () || cl.attractloop)
		Cvar_SetInteger ("paused", enable);
	else
		Cvar_SetInteger ("paused", 0);
}


static void CL_Pause_f (void)
{
	CL_Pause (!cl_paused->integer);
}

/*
==================
CL_Quit_f
==================
*/
void CL_Quit_f (void)	//?? do we need 2 quit commands (in cl_main.c and common.c) ??
{
	CL_Disconnect ();
	Com_Quit ();
}

/*
================
CL_Drop

Called after an Com_DropError() was thrown
================
*/
void CL_Drop (bool fromError)
{
	if (fromError) SCR_EndLoadingPlaque (true);
	if (cls.state != ca_uninitialized && cls.state != ca_disconnected)
		CL_Disconnect ();
}


/*
=======================
CL_SendConnectPacket

We have gotten a challenge from the server, so try and
connect.
======================
*/
void CL_SendConnectPacket (void)
{
	netadr_t	adr;
	int		port;

	if (!NET_StringToAdr (cls.servername, &adr))
	{
		Com_WPrintf ("Bad server address %s\n", cls.servername);
		cls.connect_time = 0;
		return;
	}
	if (!adr.port) adr.port = BigShort (PORT_SERVER);

	port = Cvar_VariableInt ("qport");
	userinfo_modified = false;

	if (cl_extProtocol->integer)
		Netchan_OutOfBandPrint (NS_CLIENT, adr, "connect %d %d %d \"%s\" %s",
			PROTOCOL_VERSION, port, cls.challenge, Cvar_Userinfo (), NEW_PROTOCOL_ID);
	else
	{
		Com_DPrintf ("Extended protocol disabled\n");
		Netchan_OutOfBandPrint (NS_CLIENT, adr, "connect %d %d %d \"%s\"",
			PROTOCOL_VERSION, port, cls.challenge, Cvar_Userinfo ());
	}
}

/*
=================
CL_CheckForResend

Resend a connect message if the last one has timed out
=================
*/
void CL_CheckForResend (void)
{
	netadr_t	adr;

	// if the local server is running and we aren't then connect
	if (cls.state == ca_disconnected && Com_ServerState())
	{
		cls.state = ca_connecting;
		strcpy (cls.servername, "localhost");
		// we don't need a challenge on the localhost
		CL_SendConnectPacket ();
		return;
//		cls.connect_time = -BIG_NUMBER;	// CL_CheckForResend() will fire immediately
	}

	// resend if we haven't gotten a reply yet
	if (cls.state != ca_connecting)
		return;

	if (cls.realtime - cls.connect_time < 3000)
		return;

	if (!NET_StringToAdr (cls.servername, &adr))
	{
		Com_WPrintf ("Bad server address %s\n", cls.servername);
		cls.state = ca_disconnected;
		return;
	}
	if (!adr.port) adr.port = BigShort (PORT_SERVER);

	cls.connect_time = cls.realtime;	// for retransmit requests

	Com_Printf ("Connecting to %s...\n", cls.servername);

	statusRequest = true;
	Netchan_OutOfBandPrint (NS_CLIENT, adr, "status");
	Netchan_OutOfBandPrint (NS_CLIENT, adr, "getchallenge");
}


/*
================
CL_Connect_f

================
*/
void CL_Connect_f (bool usage, int argc, char **argv)
{
	char	*server;

	if (argc != 2 || usage)
	{
		Com_Printf ("Usage: connect <server>\n");
		return;
	}

	if (Com_ServerState ())
	{	// if running a local server, kill it and reissue
		SV_Shutdown (va("Server quit\n", msg), false);
	}
	else
	{
		CL_Disconnect ();
	}

	server = argv[1];

	NET_Config (true);			// allow remote

	CL_Disconnect ();

	cls.state = ca_connecting;
	appStrncpyz (cls.servername, server, sizeof(cls.servername));
	cls.connect_time = -BIG_NUMBER;	// CL_CheckForResend() will fire immediately
}


/*
=====================
CL_Rcon_f

  Send the rest of the command line over as
  an unconnected command.
=====================
*/
void CL_Rcon_f (int argc, char **argv)
{
	char	message[1024];
	int		i;
	netadr_t to;

	if (!rcon_client_password->string[0])
	{
		Com_WPrintf ("\"rcon_password\" is not set\n");
		return;
	}

	NET_Config (true);		// allow remote

	appSprintf (ARRAY_ARG(message), "\xFF\xFF\xFF\xFFrcon %s ", rcon_client_password->string);
	for (i = 1; i < argc; i++)
	{
		strcat (message, argv[i]);
		strcat (message, " ");
	}

	if (cls.state >= ca_connected)
		to = cls.netchan.remote_address;
	else
	{
		if (!rcon_address->string[0])
		{
			Com_WPrintf ("Not connected and \"rcon_address\" is not set\n");
			return;
		}
		NET_StringToAdr (rcon_address->string, &to);
		if (!to.port) to.port = BigShort (PORT_SERVER);
	}

	NET_SendPacket (NS_CLIENT, strlen(message)+1, message, to);
}


/*
=====================
CL_Disconnect

Goes from a connected state to full screen console state
Sends a disconnect message to the server
=====================
*/
void CL_Disconnect (void)
{
	char	final[32];

	guard(CL_Disconnect);

	if (cls.state == ca_disconnected)
		return;

	if (timedemo->integer)
	{
		int	time;

		time = appMilliseconds () - cl.timedemoStart;
		if (time > 0)
			Com_Printf (S_GREEN"Total %d frames, %3.1f seconds: %3.1f avg fps %3.1f min fps\n", cl.timedemoFrames, time / 1000.0f,
				1000.0f * cl.timedemoFrames / time, 1000.0f / cl.timedemoLongestFrame);
	}

	VectorClear (cl.refdef.blend);

	cls.connect_time = 0;

	SCR_StopCinematic ();

	if (cls.demorecording)
		CL_Stop_f ();

	// send a disconnect message to the server
	final[0] = clc_stringcmd;
	strcpy (final+1, "disconnect");
//	int len = sterlen(final); == 11
	Netchan_Transmit (&cls.netchan, 11, final);
	Netchan_Transmit (&cls.netchan, 11, final);
	Netchan_Transmit (&cls.netchan, 11, final);

	CL_ClearState ();

	// stop download
	if (cls.download)
	{
		fclose(cls.download);
		cls.download = NULL;
	}

	cls.state = ca_disconnected;

	unguard;
}

void CL_Disconnect_f (void)
{
	Com_DropError ("Disconnected from server");
}


/*
====================
CL_Packet_f (disabled, not linked to command system)

packet <destination> <contents>

Contents allows \n escape character
====================
*/
void CL_Packet_f (bool usage, int argc, char **argv)
{
	char	send[2048];
	int		i, l;
	char	*in, *out;
	netadr_t	adr;

	if (argc != 3 || usage)
	{
		Com_Printf ("Usage: packet <destination> <contents>\n");
		return;
	}

	NET_Config (true);		// allow remote

	if (!NET_StringToAdr (argv[1], &adr))
	{
		Com_WPrintf ("Bad address %s\n", argv[1]);
		return;
	}
	if (!adr.port)
		adr.port = BigShort (PORT_SERVER);

	in = argv[2];
	out = send+4;
	send[0] = send[1] = send[2] = send[3] = (char)0xff;

	l = strlen (in);
	for (i=0 ; i<l ; i++)
	{
		if (in[i] == '\\' && in[i+1] == 'n')
		{
			*out++ = '\n';
			i++;
		}
		else
			*out++ = in[i];
	}
	*out = 0;

	NET_SendPacket (NS_CLIENT, out-send, send, adr);
}

/*
=================
CL_Changing_f

Just sent as a hint to the client that they should
drop to full console
=================
*/
void CL_Changing_f (void)
{
	//ZOID
	//if we are downloading, we don't change!  This so we don't suddenly stop downloading a map
	if (cls.download)
		return;

	SCR_BeginLoadingPlaque ();
	cls.state = ca_connected;	// not active anymore, but not disconnected
	Com_Printf ("\nChanging map...\n");
}


/*
=================
CL_Reconnect_f

The server is changing levels
=================
*/
void CL_Reconnect_f (void)
{
	//ZOID
	//if we are downloading, we don't change!  This so we don't suddenly stop downloading a map
	if (cls.download)
		return;

	S_StopAllSounds_f ();
	if (cls.state == ca_connected) {
		Com_Printf ("reconnecting...\n");
		cls.state = ca_connected;
		MSG_WriteChar (&cls.netchan.message, clc_stringcmd);
		MSG_WriteString (&cls.netchan.message, "new");
		return;
	}

	if (*cls.servername)
	{
		if (cls.state >= ca_connected)
		{
			CL_Disconnect ();
			cls.connect_time = cls.realtime - 1500;
		}
		else
			cls.connect_time = -BIG_NUMBER; // fire immediately

		cls.state = ca_connecting;
		Com_Printf ("reconnecting...\n");
	}
}


/*
=================
CL_PingServers_f
=================
*/
void CL_PingServers_f (void)
{
	netadr_t adr;
	const char *adrstring;

	NET_Config (true);		// allow remote

	// send a broadcast packet
	Com_Printf ("pinging broadcast...\n");
	const char *cmd = "info " STR(PROTOCOL_VERSION);

	if (!Cvar_VariableInt ("noudp"))
	{
		adr.type = NA_BROADCAST;
		adr.port = BigShort(PORT_SERVER);
		Netchan_OutOfBandPrint (NS_CLIENT, adr, cmd);
		// implicitly check localhost for local dedicated server
		if (!Com_ServerState ())
		{
			NET_StringToAdr ("127.0.0.1", &adr);
			adr.port = BigShort(PORT_SERVER);
			Netchan_OutOfBandPrint (NS_CLIENT, adr, cmd);
		}
	}

	if (!Cvar_VariableInt ("noipx"))
	{
		adr.type = NA_BROADCAST_IPX;
		adr.port = BigShort(PORT_SERVER);
		Netchan_OutOfBandPrint (NS_CLIENT, adr, cmd);
	}



	// send a packet to each address book entry
	for (int i = 0; i < NUM_ADDRESSBOOK_ENTRIES; i++)
	{
		adrstring = Cvar_VariableString (va("adr%d", i));
		if (!adrstring || !adrstring[0])
			continue;

		Com_Printf ("pinging %s...\n", adrstring);
		if (!NET_StringToAdr (adrstring, &adr))
		{
			Com_Printf ("Bad address: %s\n", adrstring);
			continue;
		}
		if (!adr.port)
			adr.port = BigShort(PORT_SERVER);
		Netchan_OutOfBandPrint (NS_CLIENT, adr, cmd);
	}
}


/*
=================
CL_Skins_f

Load or download any custom player skins and models
=================
*/
void CL_Skins_f (void)
{
	int		i;

	for (i = 0; i < MAX_CLIENTS; i++)
	{
		if (!cl.configstrings[CS_PLAYERSKINS+i][0])
			continue;
		Com_Printf ("client %d: %s\n", i, cl.configstrings[CS_PLAYERSKINS+i]);
		SCR_UpdateScreen ();
		Sys_SendKeyEvents ();	// pump message loop
		CL_ParseClientinfo (i);
	}
}


/*
=================
CL_ConnectionlessPacket

Responses to broadcasts, etc
=================
*/

static char *cnstr;

static void cClientConnect (int argc, char **argv)
{
	// server connection
	if (cls.state == ca_connected)
	{
		Com_Printf ("dup connect received: ignored\n");
		return;
	}
	Netchan_Setup (NS_CLIENT, &cls.netchan, net_from, 0);	// use default qport value
	MSG_WriteChar (&cls.netchan.message, clc_stringcmd);
	MSG_WriteString (&cls.netchan.message, "new");
	cls.state = ca_connected;
	cls.newprotocol = !strcmp (argv[1], NEW_PROTOCOL_ID);
	if (cls.newprotocol)
		Com_DPrintf ("Connected using extended protocol\n");
	else if (cl_extProtocol->integer)
		Com_DPrintf ("Server does not support extended protocol\n");
}

static void cInfo (int argc, char **argv)
{
	// server responding to a status broadcast
	if (argc != 1)
	{
		// this can be message from ourselves -- request command is "info <protocol>", answer is "info\n<data>"
		return;
	}

	Com_Printf ("%s\n", cnstr);
	M_AddToServerList (net_from, cnstr);
}

static void cCmd (int argc, char **argv)
{
	if (!NET_IsLocalAddress (&net_from))
	{
		Com_Printf ("Command packet from remote host. Ignored.\n");
		return;
	}
	Cbuf_AddText (cnstr);
	Cbuf_AddText ("\n");
}

static void cPrint (int argc, char **argv)
{
	// print command from somewhere
	if (TryParseStatus (cnstr)) return;	// do not pring status message

	Com_Printf ("%s", cnstr);
}

static void cPing (int argc, char **argv)
{
	Netchan_OutOfBandPrint (NS_CLIENT, net_from, "ack");
}

static void cChallenge (int argc, char **argv)
{
	// challenge from the server we are connecting to
	// check status info
	if (statusRequest)
	{
		Com_DPrintf ("no status answer\n");
		statusRequest = false;
		cl_cheats = false;
		cl_mapname[0] = 0;		// needed ??
		cl_gamename[0] = 0;
	}
	Cvar_SetInteger ("cheats", cl_cheats);		// synchronize client "cheats" cvar with server

	cls.challenge = atoi (argv[1]);
	CL_SendConnectPacket ();
}

static void cEcho (int argc, char **argv)
{
	// echo request from server
	Netchan_OutOfBandPrint (NS_CLIENT, net_from, "%s", argv[1]);
}

static const CSimpleCommand connectionlessCmds[] = {
	{"client_connect", cClientConnect},
	{"info", cInfo},
	{"cmd", cCmd},
	{"print", cPrint},
	{"ping", cPing},
	{"challenge", cChallenge},
	{"echo", cEcho}
};


void CL_ConnectionlessPacket (void)
{
	char	cmd[1024];

	guard(CL_ConnectionlessPacket);

	MSG_BeginReading (&net_message);
	MSG_ReadLong (&net_message);	// skip the -1

	char *s = MSG_ReadString (&net_message);
	cnstr = strchr (s, '\n');
	if (cnstr)
	{
		cnstr++;
		appStrncpyz (cmd, s, cnstr - s);
	}
	else
	{
		strcpy (cmd, s);
		cnstr = "";				// just in case
	}
	// cnstr = NULL || next line ptr

	Com_DPrintf ("%s: %s\n", NET_AdrToString (&net_from), cmd);

	if (!ExecuteCommand (s, ARRAY_ARG(connectionlessCmds)))
		Com_Printf ("Unknown command \"%s\".\n", cmd);

	unguard;
}


/*
=================
CL_ReadPackets
=================
*/
void CL_ReadPackets (void)
{
	guard(CL_ReadPackets);

	while (NET_GetPacket (NS_CLIENT, &net_from, &net_message))
	{
		// remote command packet
		if (*(int *)net_message.data == -1)
		{
			CL_ConnectionlessPacket ();
			continue;
		}

		if (cls.state == ca_disconnected || cls.state == ca_connecting)
			continue;		// dump it if not connected

		if (net_message.cursize < 8)
		{
			Com_WPrintf ("%s: runt packet\n", NET_AdrToString (&net_from));
			continue;
		}

		// packet from server
		if (!NET_CompareAdr (&net_from, &cls.netchan.remote_address))
		{
			Com_DPrintf ("%s: sequenced packet without connection\n", NET_AdrToString (&net_from));
			continue;
		}
		if (!Netchan_Process (&cls.netchan, &net_message))
			continue;		// wasn't accepted for some reason
		CL_ParseServerMessage ();
	}

	// check timeout
	if (cls.state >= ca_connected &&
		cls.realtime - cls.netchan.last_received > cl_timeout->integer * 1000 &&		//?? realtime is not correct for timescale
		cls.netchan.remote_address.type != NA_LOOPBACK)		// check for multiplayer only
	{
		if (++cl.timeoutcount > 5)	// timeoutcount saves debugger
		{
			Com_WPrintf ("\nServer connection timed out.\n");
			CL_Disconnect ();
			return;
		}
	}
	else
		cl.timeoutcount = 0;

	unguard;
}


//=============================================================================

/*
==============
CL_FixUpGender_f
==============
*/
void CL_FixUpGender(void)
{
	char	*p, sk[MAX_QPATH];

	if (gender_auto->integer)
	{

		if (gender->modified)
		{
			// was set directly, don't override the user
			gender->modified = false;
			return;
		}

		appStrncpyz (sk, skin->string, sizeof(sk));
		if ((p = strchr(sk, '/')) != NULL)
			*p = 0;
		if (!stricmp(sk, "male") || !stricmp(sk, "cyborg"))
			Cvar_Set ("gender", "male");
		else if (S_IsFemale (sk))
			Cvar_Set ("gender", "female");
		else
			Cvar_Set ("gender", "none");
		gender->modified = false;
	}
}

/*
==============
CL_Userinfo_f
==============
*/
void CL_Userinfo_f (void)
{
	Com_Printf (S_GREEN"------- User info settings -------\n");
	Info_Print (Cvar_Userinfo());
}

/*
======================
CL_RegisterSounds
======================
*/
void CL_RegisterSounds (void)
{
	S_BeginRegistration ();
	CL_RegisterTEntSounds ();

	for (int i = 1; i < MAX_SOUNDS; i++)
	{
		if (!cl.configstrings[CS_SOUNDS+i][0])
			break;
		cl.sound_precache[i] = S_RegisterSound (cl.configstrings[CS_SOUNDS+i]);
		Sys_SendKeyEvents ();	// pump message loop
	}
	S_EndRegistration ();
}

/*
=================
CL_Snd_Restart_f

Restart the sound subsystem so it can pick up
new parameters and flush all sounds
=================
*/
void CL_Snd_Restart_f (void)
{
	S_Shutdown ();
	S_Init ();
	CL_RegisterSounds ();
}


//-----------------------------------------------------------------------------

/*
===============
CL_WriteConfiguration

Writes key bindings and archived cvars to config.cfg
===============
*/
void CL_WriteConfiguration (char *filename)
{
	FILE	*f;

	if (cls.state == ca_uninitialized)
		return;

	f = fopen (va("%s/%s", FS_Gamedir(), filename), "w");
	if (!f)
	{
		Com_Printf ("Couldn't write %s.\n", filename);
		return;
	}

	fprintf (f, "// generated by " APPNAME ", do not modify\n");
	fprintf (f, "//\n// Key bindings\n//\n");
	Key_WriteBindings (f);
	fprintf (f, "//\n// Cvars\n//\n");
	Cvar_WriteVariables (f, 0xFFFFFFFF, CVAR_USER_CREATED|CVAR_GAME_CREATED, "");
	Cvar_WriteVariables (f, CVAR_USER_CREATED, CVAR_GAME_CREATED, "// user created variables\n");
	Cvar_WriteVariables (f, CVAR_GAME_CREATED, CVAR_USER_CREATED, "// game created variables\n");
	fprintf (f, "//\n// Aliases\n//\n");
	Cmd_WriteAliases (f);
	fclose (f);
}


void CL_WriteConfig_f (bool usage, int argc, char **argv)
{
	char	name[MAX_OSPATH];

	if (argc != 2 || usage)
	{
		Com_Printf ("Usage: writeconfig <filename>\n");
		return;
	}
	strcpy (name, argv[1]);
	if (!strchr (name, '.'))
		strcat (name, ".cfg");
	CL_WriteConfiguration (name);
}


/*
=================
CL_InitLocal
=================
*/
static void CL_InitLocal (void)
{
CVAR_BEGIN(vars)
	CVAR_VAR(cl_stereo_separation, 0.4, CVAR_ARCHIVE),
	CVAR_VAR(cl_stereo, 0, 0),

	CVAR_FULL(&cl_add_blend, "cl_blend", "1", 0),
	CVAR_FULL(&cl_add_lights, "cl_lights", "1", 0),
	CVAR_FULL(&cl_add_particles, "cl_particles", "1", 0),
	CVAR_FULL(&cl_add_entities, "cl_entities", "1", 0),
	CVAR_VAR(cl_gun, 1, 0),
	CVAR_VAR(cl_footsteps, 1, 0),
	CVAR_VAR(cl_noskins, 0, 0),
	CVAR_VAR(cl_autoskins, 0, 0),
	CVAR_VAR(cl_predict, 1, 0),
	CVAR_VAR(cl_maxfps, 0, CVAR_ARCHIVE),		// ignored by default

	CVAR_VAR(cl_upspeed, 200, 0),
	CVAR_VAR(cl_forwardspeed, 200, 0),
	CVAR_VAR(cl_sidespeed, 200, 0),
	CVAR_VAR(cl_yawspeed, 140, 0),
	CVAR_VAR(cl_pitchspeed, 150, 0),
	CVAR_VAR(cl_anglespeedkey, 1.5, 0),

	CVAR_VAR(cl_run, 0, CVAR_ARCHIVE),
	CVAR_VAR(freelook, 1, CVAR_ARCHIVE),
	CVAR_VAR(lookspring, 0, CVAR_ARCHIVE),
	CVAR_VAR(lookstrafe, 0, CVAR_ARCHIVE),
	CVAR_VAR(sensitivity, 3, CVAR_ARCHIVE),

	CVAR_VAR(m_pitch, 0.022, CVAR_ARCHIVE),
	CVAR_VAR(m_yaw, 0.022, 0),
	CVAR_VAR(m_forward, 1, 0),
	CVAR_VAR(m_side, 1, 0),

	CVAR_VAR(cl_shownet, 0, 0),
	CVAR_VAR(cl_showmiss, 0, 0),
	CVAR_FULL(&cl_showclamp, "showclamp", "0", 0),
	CVAR_VAR(cl_timeout, 120, 0),
	CVAR_FULL(&cl_paused, "paused", "0", CVAR_CHEAT),

	CVAR_FULL(&rcon_client_password, "rcon_password", "", 0),
	CVAR_FULL(&rcon_address, "rcon_address", "", 0),

	// userinfo
	CVAR_FULL(&info_password, "password", "", CVAR_USERINFO),
	CVAR_FULL(&info_spectator, "spectator", "0", CVAR_USERINFO),
	CVAR_VAR(name, unnamed, CVAR_USERINFO|CVAR_ARCHIVE),
	CVAR_VAR(skin, male/grunt, CVAR_USERINFO|CVAR_ARCHIVE),
	CVAR_VAR(railcolor, 0, CVAR_USERINFO|CVAR_ARCHIVE),
	CVAR_VAR(railtype, 1, CVAR_USERINFO|CVAR_ARCHIVE),
	CVAR_VAR(rate, 25000, CVAR_USERINFO|CVAR_ARCHIVE),				// FIXME
	CVAR_VAR(msg, 1, CVAR_USERINFO|CVAR_ARCHIVE),
	CVAR_VAR(hand, 0, CVAR_USERINFO|CVAR_ARCHIVE),
	CVAR_VAR(fov, 90, CVAR_USERINFO|CVAR_ARCHIVE),
	CVAR_VAR(gender, male, CVAR_USERINFO|CVAR_ARCHIVE),
	CVAR_VAR(gender_auto, 1, CVAR_ARCHIVE),

	CVAR_VAR(cl_vwep, 1, CVAR_ARCHIVE),
	CVAR_VAR(cl_3rd_person, 0, CVAR_ARCHIVE),
	CVAR_VAR(cl_cameradist, 80, CVAR_ARCHIVE),
	CVAR_VAR(cl_cameraheight, 10, CVAR_ARCHIVE),

	CVAR_VAR(cl_extProtocol, 1, CVAR_ARCHIVE),

	CVAR_VAR(cl_newfx, 1, CVAR_ARCHIVE),
	CVAR_VAR(r_sfx_pause, 0, 0),
	CVAR_VAR(cl_infps, 80, 0),
	CVAR_VAR(cl_showbboxes, 0, CVAR_CHEAT)
CVAR_END

	Cvar_GetVars (ARRAY_ARG(vars));
	for (int i = 0; i < NUM_ADDRESSBOOK_ENTRIES; i++)
		Cvar_Get (va("adr%d", i), "", CVAR_ARCHIVE);

	cls.state = ca_disconnected;
	cls.realtime = appMilliseconds ();

	CL_InitInput ();

	gender->modified = false; // clear this so we know when user sets it manually (??)

	// register our commands
	RegisterCommand ("cmd", CL_ForwardToServer_f);
	RegisterCommand ("pause", CL_Pause_f);
	RegisterCommand ("pingservers", CL_PingServers_f);
	RegisterCommand ("skins", CL_Skins_f);

	RegisterCommand ("userinfo", CL_Userinfo_f);
	RegisterCommand ("snd_restart", CL_Snd_Restart_f);

	RegisterCommand ("changing", CL_Changing_f);
	RegisterCommand ("disconnect", CL_Disconnect_f);
	RegisterCommand ("record", CL_Record_f);
	RegisterCommand ("stop", CL_Stop_f);

	RegisterCommand ("quit", CL_Quit_f);

	RegisterCommand ("connect", CL_Connect_f);
	RegisterCommand ("reconnect", CL_Reconnect_f);
	RegisterCommand ("rcon", CL_Rcon_f);

//	RegisterCommand ("packet", CL_Packet_f);		// this is dangerous to leave in

	RegisterCommand ("precache", CL_Precache_f);
	RegisterCommand ("download", CL_Download_f);

	RegisterCommand ("writeconfig", CL_WriteConfig_f);
}


/*
==================
CL_FixCvarCheats

==================
*/

void CL_FixCvarCheats (void)
{
	bool	cheats;
	char	*str;

	cheats = false;

	if (cl.attractloop) cheats = true;
	if (cls.state != ca_connected && cls.state != ca_active) cheats = true;

	str = cl.configstrings[CS_MAXCLIENTS];
	if ((str[0] == '1' && str[1] == 0) || str[0] == 0)	// == "1" or ""
		cheats = true;									// single player can cheat

	if (sv_cheats->integer)
		cheats = true;

	Cvar_Cheats (cheats);
}

//============================================================================


/*
==================
CL_Frame

==================
*/

void CL_Frame (float msec, float realMsec)
{
	static double extratime_real;	// real frame time, msec
	static double extratime;		// scaled frame time, sec

	guard(CL_Frame);

	extratime += msec / 1000.0f;
	extratime_real += realMsec;

	if (!timedemo->integer)			// ignore cl_maxfps in timedemo
	{
		// here extratime can be accumulated between frames
		if (cl_maxfps->integer > 0 && extratime_real < 1000.0f / cl_maxfps->value)	// ignore when cl_maxfps==0
			return;					// framerate is too high
		if (cls.state == ca_connected && extratime < 0.1f)
		{
			SCR_UpdateScreen ();
			return;					// don't flood packets out while connecting
		}
	}

	IN_Frame ();

	// decide the simulation time
	cls.frametime = extratime;
	cl.ftime += extratime;
	cl.time = appFloor (cl.ftime * 1000.0f);
	extratime = 0;
	extratime_real = 0;

	cls.realtime = appMilliseconds ();

	if (cls.frametime > (1.0f / 5))	// low FPS fix ?? (cannot send user commands with frame time > 255ms)
		cls.frametime = (1.0f / 5);

	// if in the debugger last frame, don't timeout
	if (msec > 5000)
		cls.netchan.last_received = appMilliseconds ();

	// fetch results from server
	CL_ReadPackets ();

	Sys_SendKeyEvents ();			// get OS events
	IN_Commands ();					// allow mice or other external controllers to add commands
	Cbuf_Execute ();				// process console commands
	CL_FixCvarCheats ();			// fix any cheating cvars

	// send a new command message to the server
	CL_SendCmd ();					// send intentions now
	CL_CheckForResend ();			// resend a connection request if necessary

	// predict all unacknowledged movements
	CL_PredictMovement ();

	// allow rendering DLL change
	Vid_CheckChanges ();
	if (!cl.refresh_prepped && cls.state == ca_active)
		CL_PrepRefresh ();

	// update the screen
	if (com_speeds->integer) time_before_ref = appCycles ();
	SCR_UpdateScreen ();
	if (com_speeds->integer) time_after_ref = appCycles ();

	// update audio
	S_Update (cl.refdef.vieworg, cl.v_forward, cl.v_right, cl.v_up);

	CDAudio_Update();

	unguard;
}

//============================================================================

/*
=====================
CL_ClearState

=====================
*/
void CL_ClearState (void)
{
	S_StopAllSounds_f ();
	CL_ClearEffects ();
	CL_ClearTEnts ();

	// wipe the entire cl structure
	memset (&cl, 0, sizeof(cl));
	memset (cl_entities, 0, sizeof(centity_t[MAX_EDICTS]));

	SZ_Clear (&cls.netchan.message);
}

/*
====================
CL_Init
====================
*/
void CL_Init (void)
{
	guard(CL_Init);

	Con_Init ();
#if defined __linux__ || defined __sgi
	S_Init ();
	Vid_Init ();
#else
	Vid_Init ();
	S_Init ();	// sound must be initialized after window is created
#endif

	net_message.data = net_message_buffer;
	net_message.maxsize = sizeof(net_message_buffer);
	if (!DEDICATED) cl_entities = new centity_t [MAX_EDICTS];

	V_Init ();
	M_Init ();
	SCR_Init ();
	CDAudio_Init ();
	CL_InitLocal ();
	IN_Init ();

	unguard;
}


/*
===============
CL_Shutdown
===============
*/
void CL_Shutdown (bool error)
{
	static bool isdown = false;

	if (isdown) return;
	isdown = true;

	if (!DEDICATED)
	{
		delete cl_entities;
		if (!error)		// do not write configuration when error occured
			CL_WriteConfiguration (Cvar_VariableString ("cfgfile"));
		CDAudio_Shutdown ();
		S_Shutdown();
		IN_Shutdown ();
		Vid_Shutdown();
	}
}
