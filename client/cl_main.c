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
// cl_main.c  -- client main loop

#include "client.h"

cvar_t	*freelook;

cvar_t	*adr0, *adr1, *adr2, *adr3, *adr4, *adr5, *adr6, *adr7, *adr8;

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
cvar_t	*cl_draw2d;
cvar_t	*cl_showbboxes;
cvar_t	*r_sfx_pause;

cvar_t	*cl_infps;

client_static_t	cls;
client_state_t	cl;

centity_t		cl_entities[MAX_EDICTS];

entityState_t	cl_parse_entities[MAX_PARSE_ENTITIES];

extern	cvar_t *allow_download;
extern	cvar_t *allow_download_players;
extern	cvar_t *allow_download_models;
extern	cvar_t *allow_download_sounds;
extern	cvar_t *allow_download_maps;

//======================================================================


static qboolean statusRequest;

char	cl_mapname[MAX_QPATH];
char	cl_gamename[MAX_QPATH];
qboolean cl_cheats;

static qboolean TryParseStatus (char *str)
{
	char	buf[MAX_MSGLEN], *s;

	if (!statusRequest) return false;

	Q_strncpyz (buf, str, sizeof(buf));
	// remove players info
	s = strchr (buf, '\n');
	if (!s) return false;
	*s = 0;
	// validate ServerInfo
	s = Info_ValueForKey (buf, "mapname");
	if (!s) return false;
	strcpy (cl_mapname, s);

	s = Info_ValueForKey (buf, "gamename");
	if (!s) return false;
	strcpy (cl_gamename, s);

	s = Info_ValueForKey (buf, "cheats");
	if (!s) return false;
	cl_cheats = atoi (s);

//	Com_Printf ("^1map: %s  game: %s  cheats: %d\n", cl_mapname, cl_gamename, cl_cheats);

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
void CL_Stop_f (void)
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
void CL_Record_f (void)
{
	char	name[MAX_OSPATH];
	char	buf_data[MAX_MSGLEN];
	sizebuf_t	buf;
	int		i, len;
	entityState_t	*ent;
	entity_state_t	nullstate;

	if (Cmd_Argc() != 2)
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
	Com_sprintf (ARRAY_ARG(name), "%s/demos/%s.dm2", FS_Gamedir(), Cmd_Argv(1));

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
		MSG_WriteDeltaEntity (&nullstate, (entity_state_t*)&cl_entities[i].baseline, &buf, true, true);
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

/*
===================
Cmd_ForwardToServer

adds the current command line as a clc_stringcmd to the client message.
things like godmode, noclip, etc, are commands directed to the server,
so when they are typed in at the console, they will need to be forwarded.
===================
*/
void Cmd_ForwardToServer (void)
{
	char	*cmd;

	cmd = Cmd_Argv(0);
	if (cls.state <= ca_connected || *cmd == '-' || *cmd == '+')
	{
		Com_WPrintf ("Unknown command \"%s\"\n", cmd);
		return;
	}

	MSG_WriteByte (&cls.netchan.message, clc_stringcmd);
	SZ_Print (&cls.netchan.message, cmd);
	if (Cmd_Argc() > 1)
	{
		SZ_Print (&cls.netchan.message, " ");
		SZ_Print (&cls.netchan.message, Cmd_Args());
	}
}

void CL_Setenv_f (void)
{
	int		argc;

	argc = Cmd_Argc();
	if (argc > 2)
	{
		char	buffer[1024];
		int		i;

		strcpy (buffer, Cmd_Argv(1));
		strcat (buffer, "=");

		for (i = 2; i < argc; i++)
		{
			strcat (buffer, Cmd_Argv(i));
			strcat (buffer, " ");
		}

		putenv (buffer);
	}
	else if (argc == 2)
	{
		char *env = getenv (Cmd_Argv(1));

		if (env)
			Com_Printf ("%s=%s\n", Cmd_Argv(1), env);
		else
			Com_Printf ("%s undefined\n", Cmd_Argv(1), env);
	}
}


/*
==================
CL_ForwardToServer_f
==================
*/
void CL_ForwardToServer_f (void)
{
	if (cls.state != ca_connected && cls.state != ca_active)
	{
		Com_WPrintf ("Can't \"%s\", not connected\n", Cmd_Argv(0));
		return;
	}

	// don't forward the first argument
	if (Cmd_Argc() > 1)
	{
		MSG_WriteByte (&cls.netchan.message, clc_stringcmd);
		SZ_Print (&cls.netchan.message, Cmd_Args());
	}
}


/*
==================
CL_Pause_f
==================
*/
void CL_Pause_f (void)
{
	// never pause in multiplayer
	if ((Cvar_VariableInt ("maxclients") > 1 && !cl_cheats) || !Com_ServerState ())
	{
		Cvar_SetInteger ("paused", 0);
		return;
	}

	Cvar_SetInteger ("paused", !cl_paused->integer);
}

/*
==================
CL_Quit_f
==================
*/
void CL_Quit_f (void)
{
	CL_Disconnect ();
	Com_Quit ();
}

/*
================
CL_Drop

Called after an ERR_DROP was thrown
================
*/
void CL_Drop (void)
{
	if (cls.state == ca_uninitialized || cls.state == ca_disconnected)
		return;

	CL_Disconnect ();

	// drop loading plaque unless this is the initial game start
	if (cls.disable_servercount != -1)
		SCR_EndLoadingPlaque ();	// get rid of loading plaque
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
void CL_Connect_f (void)
{
	char	*server;

	if (Cmd_Argc() != 2)
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

	server = Cmd_Argv (1);

	NET_Config (true);			// allow remote

	CL_Disconnect ();

	cls.state = ca_connecting;
	Q_strncpyz (cls.servername, server, sizeof(cls.servername));
	cls.connect_time = -BIG_NUMBER;	// CL_CheckForResend() will fire immediately
}


/*
=====================
CL_Rcon_f

  Send the rest of the command line over as
  an unconnected command.
=====================
*/
void CL_Rcon_f (void)
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

	Com_sprintf (ARRAY_ARG(message), "\xFF\xFF\xFF\xFFrcon %s ", rcon_client_password->string);
	for (i = 1; i < Cmd_Argc(); i++)
	{
		strcat (message, Cmd_Argv(i));
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
CL_ClearState

=====================
*/
void CL_ClearState (void)
{
	S_StopAllSounds ();
	CL_ClearEffects ();
	CL_ClearTEnts ();

	// wipe the entire cl structure
	memset (&cl, 0, sizeof(cl));
	memset (&cl_entities, 0, sizeof(cl_entities));

	SZ_Clear (&cls.netchan.message);
}

/*
=====================
CL_Disconnect

Goes from a connected state to full screen console state
Sends a disconnect message to the server
This is also called on Com_Error, so it shouldn't cause any errors
=====================
*/
void CL_Disconnect (void)
{
	byte	final[32];

	if (cls.state == ca_disconnected)
		return;

	if (timedemo->integer)
	{
		int	time;

		time = Sys_Milliseconds () - cl.timedemoStart;
		if (time > 0)
			Com_Printf ("^2Total %d frames, %3.1f seconds: %3.1f avg fps %3.1f min fps\n", cl.timedemoFrames, time / 1000.0f,
				1000.0f * cl.timedemoFrames / time, 1000.0f / cl.timedemoLongestFrame);
	}

	VectorClear (cl.refdef.blend);
	re.SetRawPalette(NULL);

	M_ForceMenuOff ();

	cls.connect_time = 0;

	SCR_StopCinematic ();

	if (cls.demorecording)
		CL_Stop_f ();

	// send a disconnect message to the server
	final[0] = clc_stringcmd;
	strcpy ((char *)final+1, "disconnect");
	Netchan_Transmit (&cls.netchan, strlen(final), final);
	Netchan_Transmit (&cls.netchan, strlen(final), final);
	Netchan_Transmit (&cls.netchan, strlen(final), final);

	CL_ClearState ();

	// stop download
	if (cls.download)
	{
		fclose(cls.download);
		cls.download = NULL;
	}

	cls.state = ca_disconnected;
}

void CL_Disconnect_f (void)
{
	Com_Error (ERR_DROP, "Disconnected from server");
}


/*
====================
CL_Packet_f (disabled, not linked to command system)

packet <destination> <contents>

Contents allows \n escape character
====================
*/
void CL_Packet_f (void)
{
	char	send[2048];
	int		i, l;
	char	*in, *out;
	netadr_t	adr;

	if (Cmd_Argc() != 3)
	{
		Com_Printf ("Usage: packet <destination> <contents>\n");
		return;
	}

	NET_Config (true);		// allow remote

	if (!NET_StringToAdr (Cmd_Argv(1), &adr))
	{
		Com_WPrintf ("Bad address %s\n", Cmd_Argv(1));
		return;
	}
	if (!adr.port)
		adr.port = BigShort (PORT_SERVER);

	in = Cmd_Argv(2);
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

	S_StopAllSounds ();
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
			CL_Disconnect();
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
	int			i;
	netadr_t	adr;
	char		*adrstring;
	cvar_t		*noudp;
	cvar_t		*noipx;

	NET_Config (true);		// allow remote

	// send a broadcast packet
	Com_Printf ("pinging broadcast...\n");

	noudp = Cvar_Get ("noudp", "", CVAR_NODEFAULT);
	if (!noudp->integer)
	{
		adr.type = NA_BROADCAST;
		adr.port = BigShort(PORT_SERVER);
		Netchan_OutOfBandPrint (NS_CLIENT, adr, va("info %d", PROTOCOL_VERSION));
//		Netchan_OutOfBandPrint (NS_CLIENT, adr, va("info %d\nUDP", PROTOCOL_VERSION));
	}

	noipx = Cvar_Get ("noipx", "", CVAR_NODEFAULT);
	if (!noipx->integer)
	{
		adr.type = NA_BROADCAST_IPX;
		adr.port = BigShort(PORT_SERVER);
		Netchan_OutOfBandPrint (NS_CLIENT, adr, va("info %d", PROTOCOL_VERSION));
//		Netchan_OutOfBandPrint (NS_CLIENT, adr, va("info %d\nIPX", PROTOCOL_VERSION));
	}

	// send a packet to each address book entry
	for (i = 0; i < 16; i++)
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
		Netchan_OutOfBandPrint (NS_CLIENT, adr, va("info %d", PROTOCOL_VERSION));
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

void CL_ConnectionlessPacket (void)
{
	char	*s, *s1, *c;

	MSG_BeginReading (&net_message);
	MSG_ReadLong (&net_message);	// skip the -1

	s = MSG_ReadString (&net_message);
	s1 = strchr (s, '\n');
	if (s1) s1++;					// s1 = NULL || next line ptr
	else s1 = "";					// just in case

	Cmd_TokenizeString (s, false);

	c = Cmd_Argv(0);

	Com_DPrintf ("%s: %s\n", NET_AdrToString (&net_from), c);

	// server connection
	if (!strcmp (c, "client_connect"))
	{
		if (cls.state == ca_connected)
		{
			Com_Printf ("dup connect received: ignored\n");
			return;
		}
		Netchan_Setup (NS_CLIENT, &cls.netchan, net_from, 0);	// use default qport value
		MSG_WriteChar (&cls.netchan.message, clc_stringcmd);
		MSG_WriteString (&cls.netchan.message, "new");
		cls.state = ca_connected;
		cls.newprotocol = !strcmp (Cmd_Argv (1), NEW_PROTOCOL_ID);
		if (cls.newprotocol)
			Com_DPrintf ("Connected using extended protocol\n");
		else if (cl_extProtocol->integer)
			Com_DPrintf ("Server does not support extended protocol\n");
		return;
	}

	// server responding to a status broadcast
	if (!strcmp (c, "info"))
	{
		if (Cmd_Argc() != 1)
		{
			// this can be message from ourselves -- request command is "info <protocol>", answer is "info\n<data>"
			return;
		}

		Com_Printf ("%s\n", s1);
		M_AddToServerList (net_from, s1);
		return;
	}

	// remote command from gui front end
	if (!strcmp (c, "cmd"))
	{
		if (!NET_IsLocalAddress (&net_from))
		{
			Com_Printf ("Command packet from remote host.  Ignored.\n");
			return;
		}
		Sys_AppActivate ();					//??
		Cbuf_AddText (s1);
		Cbuf_AddText ("\n");
		return;
	}
	// print command from somewhere
	if (!strcmp (c, "print"))
	{
		if (TryParseStatus (s1)) return;	// do not pring status message

		Com_Printf ("%s", s1);
		return;
	}

	// ping from somewhere
	if (!strcmp (c, "ping"))
	{
		Netchan_OutOfBandPrint (NS_CLIENT, net_from, "ack");
		return;
	}

	// challenge from the server we are connecting to
	if (!strcmp (c, "challenge"))
	{
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

		cls.challenge = atoi(Cmd_Argv(1));
		CL_SendConnectPacket ();
		return;
	}

	// echo request from server
	if (!strcmp (c, "echo"))
	{
		Netchan_OutOfBandPrint (NS_CLIENT, net_from, "%s", Cmd_Argv(1) );
		return;
	}

	Com_Printf ("Unknown command \"%s\".\n", c);
}


/*
=================
CL_ReadPackets
=================
*/
void CL_ReadPackets (void)
{
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

		Q_strncpyz (sk, skin->string, sizeof(sk));
		if ((p = strchr(sk, '/')) != NULL)
			*p = 0;
		if (!Q_stricmp(sk, "male") || !Q_stricmp(sk, "cyborg"))
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
	Com_Printf ("User info settings:\n");
	Info_Print (Cvar_Userinfo());
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

static int precache_check;		// for autodownload of precache items
static int precache_spawncount;
static int precache_tex;
static int precache_model_skin;

byte *precache_model;			// used for skin checking in alias models

#define PLAYER_MULT 5

// ENV_CNT is map load, ENV_CNT+1 is first env map
#define ENV_CNT (CS_PLAYERSKINS + MAX_CLIENTS * PLAYER_MULT)
#define TEXTURE_CNT (ENV_CNT+13)

void CL_RequestNextDownload (void)
{
	unsigned	map_checksum;		// for detecting cheater maps
	dmdl_t *pheader;

	if (cls.state != ca_connected)
		return;

	if (cls.netchan.remote_address.type == NA_LOOPBACK)
		precache_check = TEXTURE_CNT+999;	// local server: download is nonsense (skip this phase)


	if (!allow_download->integer && precache_check < ENV_CNT)
		precache_check = ENV_CNT;

//ZOID
	if (precache_check == CS_MODELS) { // confirm map
		precache_check = CS_MODELS+2; // 0 isn't used
		if (allow_download_maps->integer)
			if (!CL_CheckOrDownloadFile(cl.configstrings[CS_MODELS+1]))
				return; // started a download
	}
	if (precache_check >= CS_MODELS && precache_check < CS_MODELS+MAX_MODELS) {
		if (allow_download_models->integer) {
			while (precache_check < CS_MODELS+MAX_MODELS &&
				cl.configstrings[precache_check][0]) {
				if (cl.configstrings[precache_check][0] == '*' ||
					cl.configstrings[precache_check][0] == '#') {
					precache_check++;
					continue;
				}
				if (precache_model_skin == 0) {
					if (!CL_CheckOrDownloadFile(cl.configstrings[precache_check])) {
						precache_model_skin = 1;
						return; // started a download
					}
					precache_model_skin = 1;
				}

				// checking for skins in the model
				if (!precache_model) {

					FS_LoadFile (cl.configstrings[precache_check], (void **)&precache_model);
					if (!precache_model) {
						precache_model_skin = 0;
						precache_check++;
						continue; // couldn't load it
					}
					if (LittleLong(*(unsigned *)precache_model) != MD2_IDENT) {
						// not an alias model
						FS_FreeFile(precache_model);
						precache_model = 0;
						precache_model_skin = 0;
						precache_check++;
						continue;
					}
					pheader = (dmdl_t *)precache_model;
					if (LittleLong (pheader->version) != MD2_VERSION) {
						precache_check++;
						precache_model_skin = 0;
						continue; // couldn't load it
					}
				}

				pheader = (dmdl_t *)precache_model;

				while (precache_model_skin - 1 < LittleLong(pheader->numSkins)) {
					if (!CL_CheckOrDownloadFile((char *)precache_model + LittleLong(pheader->ofsSkins) +
						(precache_model_skin - 1)*MD2_MAX_SKINNAME))
					{
						precache_model_skin++;
						return; // started a download
					}
					precache_model_skin++;
				}
				if (precache_model)
				{
					FS_FreeFile(precache_model);
					precache_model = 0;
				}
				precache_model_skin = 0;
				precache_check++;
			}
		}
		precache_check = CS_SOUNDS;
	}
	if (precache_check >= CS_SOUNDS && precache_check < CS_SOUNDS+MAX_SOUNDS)
	{
		if (allow_download_sounds->integer)
		{
			if (precache_check == CS_SOUNDS)
				precache_check++; // zero is blank
			while (precache_check < CS_SOUNDS+MAX_SOUNDS && cl.configstrings[precache_check][0])
			{
				if (cl.configstrings[precache_check][0] == '*')
				{
					precache_check++;
					continue;
				}
				if (!CL_CheckOrDownloadFile(va("sound/%s", cl.configstrings[precache_check++])))
					return; // started a download
			}
		}
		precache_check = CS_IMAGES;
	}
	if (precache_check >= CS_IMAGES && precache_check < CS_IMAGES+MAX_IMAGES)
	{
		if (precache_check == CS_IMAGES) precache_check++; // zero is blank
		while (precache_check < CS_IMAGES+MAX_IMAGES && cl.configstrings[precache_check][0])
		{
			if (!CL_CheckOrDownloadFile(va("pics/%s.pcx", cl.configstrings[precache_check++])))
				return; // started a download
		}
		precache_check = CS_PLAYERSKINS;
	}
	// skins are special, since a player has three things to download:
	// model, weapon model and skin
	// so precache_check is now *3
	if (precache_check >= CS_PLAYERSKINS && precache_check < CS_PLAYERSKINS + MAX_CLIENTS * PLAYER_MULT) {
		if (allow_download_players->integer) {
			while (precache_check < CS_PLAYERSKINS + MAX_CLIENTS * PLAYER_MULT) {
				int		i, n;
				char model[MAX_QPATH], skin[MAX_QPATH], *p;

				i = (precache_check - CS_PLAYERSKINS)/PLAYER_MULT;
				n = (precache_check - CS_PLAYERSKINS)%PLAYER_MULT;

				if (!cl.configstrings[CS_PLAYERSKINS+i][0]) {
					precache_check = CS_PLAYERSKINS + (i + 1) * PLAYER_MULT;
					continue;
				}

				if ((p = strchr(cl.configstrings[CS_PLAYERSKINS+i], '\\')) != NULL)
					p++;
				else
					p = cl.configstrings[CS_PLAYERSKINS+i];
				strcpy(model, p);
				p = strchr(model, '/');
				if (!p)
					p = strchr(model, '\\');
				if (p)
				{
					*p++ = 0;
					strcpy(skin, p);
				}
				else
					*skin = 0;

				switch (n) {
				case 0: // model
					if (!CL_CheckOrDownloadFile(va("players/%s/tris.md2", model)))
					{
						precache_check = CS_PLAYERSKINS + i * PLAYER_MULT + 1;
						return; // started a download
					}
					n++;
					/*FALL THROUGH*/

				case 1: // weapon model
					if (!CL_CheckOrDownloadFile(va("players/%s/weapon.md2", model)))
					{
						precache_check = CS_PLAYERSKINS + i * PLAYER_MULT + 2;
						return; // started a download
					}
					n++;
					/*FALL THROUGH*/

				case 2: // weapon skin
					if (!CL_CheckOrDownloadFile(va("players/%s/weapon.pcx", model)))
					{
						precache_check = CS_PLAYERSKINS + i * PLAYER_MULT + 3;
						return; // started a download
					}
					n++;
					/*FALL THROUGH*/

				case 3: // skin
					if (!CL_CheckOrDownloadFile(va("players/%s/%s.pcx", model, skin)))
					{
						precache_check = CS_PLAYERSKINS + i * PLAYER_MULT + 4;
						return; // started a download
					}
					n++;
					/*FALL THROUGH*/

				case 4: // skin_i
					if (!CL_CheckOrDownloadFile(va("players/%s/%s_i.pcx", model, skin)))
					{
						precache_check = CS_PLAYERSKINS + i * PLAYER_MULT + 5;
						return; // started a download
					}
					// move on to next model
					precache_check = CS_PLAYERSKINS + (i + 1) * PLAYER_MULT;
				}
			}
		}
		// precache phase completed
		precache_check = ENV_CNT;
	}

	if (precache_check == ENV_CNT)
	{
		precache_check = ENV_CNT + 1;

		CM_LoadMap (cl.configstrings[CS_MODELS+1], true, &map_checksum);

		if (map_checksum != atoi(cl.configstrings[CS_MAPCHECKSUM]))
		{
			Com_Error (ERR_DROP, "Local map version differs from server: %i != '%s'\n",
				map_checksum, cl.configstrings[CS_MAPCHECKSUM]);
			return;
		}
	}

	if (precache_check > ENV_CNT && precache_check < TEXTURE_CNT)
	{
		if (allow_download->integer && allow_download_maps->integer)
		{
			while (precache_check < TEXTURE_CNT)
			{
				static char *env_suf[6] = {"rt", "bk", "lf", "ft", "up", "dn"};
				int n = precache_check++ - ENV_CNT - 1;

				if (!CL_CheckOrDownloadFile(va("env/%s%s.pcx", cl.configstrings[CS_SKY], env_suf[n/2])))
					return; // started a download
			}
		}
		precache_check = TEXTURE_CNT;
	}

	if (precache_check == TEXTURE_CNT)
	{
		precache_check = TEXTURE_CNT+1;
		precache_tex = 0;
	}

	// confirm existance of textures, download any that don't exist
	if (precache_check == TEXTURE_CNT+1)
	{
		// from qcommon/cmodel.c
		extern int			numtexinfo;
		extern csurface_t	map_surfaces[];

		if (allow_download->integer && allow_download_maps->integer)
		{
			while (precache_tex < numtexinfo)
			{
				if (!CL_CheckOrDownloadFile(va("textures/%s.wal", map_surfaces[precache_tex++].rname)))
					return; // started a download
			}
		}
		precache_check = TEXTURE_CNT+999;
	}

	//ZOID
	CL_RegisterSounds ();
	CL_PrepRefresh ();

	MSG_WriteByte (&cls.netchan.message, clc_stringcmd);
	MSG_WriteString (&cls.netchan.message, va("begin %d\n", precache_spawncount));
}

/*
=================
CL_Precache_f

The server will send this command right
before allowing the client into the server
=================
*/
void CL_Precache_f (void)
{
	char	mapname[MAX_QPATH], levelshot[MAX_QPATH], *tmp;

	// set levelshot
	Q_CopyFilename (mapname, cl.configstrings[CS_MODELS+1], sizeof(levelshot));
	tmp = strchr (mapname, '.');
	if (tmp) tmp[0] = 0;
	tmp = strrchr (mapname, '/');
	if (!tmp)
		tmp = mapname;
	else
		tmp++;				// skip '/'
	Com_sprintf (ARRAY_ARG(levelshot), "/levelshots/%s.pcx", tmp);
	SCR_SetLevelshot (levelshot);

	//Yet another hack to let old demos work
	//the old precache sequence
	if (Cmd_Argc() < 2)
	{
		unsigned	map_checksum;		// for detecting cheater maps

		CM_LoadMap (cl.configstrings[CS_MODELS+1], true, &map_checksum);
		CL_RegisterSounds ();
		CL_PrepRefresh ();
		return;
	}

	precache_check = CS_MODELS;
	precache_spawncount = atoi(Cmd_Argv(1));
	precache_model = 0;
	precache_model_skin = 0;

	CL_RequestNextDownload();
}


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


void CL_WriteConfig_f (void)
{
	char	name[MAX_OSPATH];

	if (Cmd_Argc () != 2)
	{
		Com_Printf ("Usage: writeconfig <filename>\n");
		return;
	}
	strcpy (name, Cmd_Argv(1));
	if (!strchr (name, '.'))
		strcat (name, ".cfg");
	CL_WriteConfiguration (name);
}


/*
=================
CL_InitLocal
=================
*/
void CL_InitLocal (void)
{
CVAR_BEGIN(vars)
	{&adr0, "adr0", "", CVAR_ARCHIVE},
	{&adr1, "adr1", "", CVAR_ARCHIVE},
	{&adr2, "adr2", "", CVAR_ARCHIVE},
	{&adr3, "adr3", "", CVAR_ARCHIVE},
	{&adr4, "adr4", "", CVAR_ARCHIVE},
	{&adr5, "adr5", "", CVAR_ARCHIVE},
	{&adr6, "adr6", "", CVAR_ARCHIVE},
	{&adr7, "adr7", "", CVAR_ARCHIVE},
	{&adr8, "adr8", "", CVAR_ARCHIVE},

	CVAR_VAR(cl_stereo_separation, 0.4, CVAR_ARCHIVE),
	CVAR_VAR(cl_stereo, 0, 0),

	{&cl_add_blend, "cl_blend", "1", 0},
	{&cl_add_lights, "cl_lights", "1", 0},
	{&cl_add_particles, "cl_particles", "1", 0},
	{&cl_add_entities, "cl_entities", "1", 0},
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
	{&cl_showclamp, "showclamp", "0", 0},
	CVAR_VAR(cl_timeout, 120, 0),
	{&cl_paused, "paused", "0", CVAR_CHEAT},

	{&rcon_client_password, "rcon_password", "", 0},
	{&rcon_address, "rcon_address", "", 0},

	// userinfo
	{&info_password, "password", "", CVAR_USERINFO},
	{&info_spectator, "spectator", "0", CVAR_USERINFO},
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
	CVAR_VAR(cl_draw2d, 1, 0),
	CVAR_VAR(r_sfx_pause, 0, 0),
	CVAR_VAR(cl_infps, 80, 0),
	CVAR_VAR(cl_showbboxes, 0, CVAR_CHEAT)
CVAR_END

	Cvar_GetVars (ARRAY_ARG(vars));
	cls.state = ca_disconnected;
	cls.realtime = Sys_Milliseconds ();

	CL_InitInput ();

	gender->modified = false; // clear this so we know when user sets it manually (??)

	// register our commands
	Cmd_AddCommand ("cmd", CL_ForwardToServer_f);
	Cmd_AddCommand ("pause", CL_Pause_f);
	Cmd_AddCommand ("pingservers", CL_PingServers_f);
	Cmd_AddCommand ("skins", CL_Skins_f);

	Cmd_AddCommand ("userinfo", CL_Userinfo_f);
	Cmd_AddCommand ("snd_restart", CL_Snd_Restart_f);

	Cmd_AddCommand ("changing", CL_Changing_f);
	Cmd_AddCommand ("disconnect", CL_Disconnect_f);
	Cmd_AddCommand ("record", CL_Record_f);
	Cmd_AddCommand ("stop", CL_Stop_f);

	Cmd_AddCommand ("quit", CL_Quit_f);

	Cmd_AddCommand ("connect", CL_Connect_f);
	Cmd_AddCommand ("reconnect", CL_Reconnect_f);
	Cmd_AddCommand ("rcon", CL_Rcon_f);

//	Cmd_AddCommand ("packet", CL_Packet_f);		// this is dangerous to leave in

	Cmd_AddCommand ("setenv", CL_Setenv_f );

	Cmd_AddCommand ("precache", CL_Precache_f);
	Cmd_AddCommand ("download", CL_Download_f);

	Cmd_AddCommand ("writeconfig", CL_WriteConfig_f);

	// the only thing this does is allow command completion
	// to work -- all unknown commands are automatically
	// forwarded to the server
	Cmd_AddCommand ("wave", NULL);
	Cmd_AddCommand ("inven", NULL);
	Cmd_AddCommand ("kill", NULL);
	Cmd_AddCommand ("use", NULL);
	Cmd_AddCommand ("drop", NULL);
	Cmd_AddCommand ("say", NULL);
	Cmd_AddCommand ("say_team", NULL);
	Cmd_AddCommand ("info", NULL);
	Cmd_AddCommand ("prog", NULL);
	Cmd_AddCommand ("give", NULL);
	Cmd_AddCommand ("god", NULL);
	Cmd_AddCommand ("notarget", NULL);
	Cmd_AddCommand ("noclip", NULL);
	Cmd_AddCommand ("invuse", NULL);
	Cmd_AddCommand ("invprev", NULL);
	Cmd_AddCommand ("invnext", NULL);
	Cmd_AddCommand ("invdrop", NULL);
	Cmd_AddCommand ("weapnext", NULL);
	Cmd_AddCommand ("weapprev", NULL);
}


/*
==================
CL_FixCvarCheats

==================
*/

void CL_FixCvarCheats (void)
{
	qboolean cheats;
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

void CL_Frame (float msec, int realMsec)
{
	static int extratime_real;
	static float extratime;

	if (dedicated->integer)
		return;

	extratime += msec / 1000.0f;
	extratime_real += realMsec;

	if (!timedemo->integer)			// ignore cl_maxfps in timedemo
	{
		// here extratime can be accumulated between frames
		if (cls.state == ca_connected && extratime < 0.1f)
			return;					// don't flood packets out while connecting ??
		if (cl_maxfps->integer > 0 && extratime_real < 1000.0f / cl_maxfps->value)	// ignore when cl_maxfps==0
			return;					// framerate is too high
	}

	IN_Frame ();

	// decide the simulation time
	cls.frametime = extratime;
	cl.ftime += extratime;
	cl.time = Q_floor (cl.ftime * 1000.0f);
	extratime = 0;
	extratime_real = 0;

	cls.realtime = curtime;

	cls.newfx = cl_newfx->integer && (*re.flags & REF_NEW_FX);

	if (cls.frametime > (1.0f / 5))	// low FPS fix ?? (cannot send user commands with frame time > 255ms)
		cls.frametime = (1.0f / 5);

	// if in the debugger last frame, don't timeout
	if (msec > 5000)
		cls.netchan.last_received = Sys_Milliseconds ();

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
	if (com_speeds->integer)
		time_before_ref = Sys_Milliseconds ();
	SCR_UpdateScreen ();
	if (com_speeds->integer)
		time_after_ref = Sys_Milliseconds ();

	// update audio
	S_Update (cl.refdef.vieworg, cl.v_forward, cl.v_right, cl.v_up);

	CDAudio_Update();

	// advance local effects for next frame
	SCR_RunCinematic ();
	SCR_RunConsole ();

	if (log_stats->integer && cls.state == ca_active)
	{
		static int  lasttimecalled;

		if (!lasttimecalled)
		{
			lasttimecalled = Sys_Milliseconds ();
			if (log_stats_file)
				fprintf (log_stats_file, "0\n");
		}
		else
		{
			int now = Sys_Milliseconds();

			if (log_stats_file)
				fprintf (log_stats_file, "%d\n", now - lasttimecalled);
			lasttimecalled = now;
		}
	}
}


//============================================================================

/*
====================
CL_Init
====================
*/
void CL_Init (void)
{
	if (dedicated->integer)
		return;		// nothing running on the client

	// all archived variables will now be loaded

	Con_Init ();
#if defined __linux__ || defined __sgi
	S_Init ();
	Vid_Init ();
#else
	Vid_Init ();
	S_Init ();	// sound must be initialized after window is created
#endif

	V_Init ();

	net_message.data = net_message_buffer;
	net_message.maxsize = sizeof(net_message_buffer);

	M_Init ();

	SCR_Init ();
//	cls.disable_screen = true;	// don't draw yet

	CDAudio_Init ();
	CL_InitLocal ();
	IN_Init ();
}


/*
===============
CL_Shutdown

FIXME: this is a callback from Sys_Quit and Com_Error.  It would be better
to run quit through here before the final handoff to the sys code.
===============
*/
void CL_Shutdown(void)
{
	static qboolean isdown = false;

	if (isdown)
	{
		printf ("recursive shutdown\n");
		return;
	}
	isdown = true;

	CL_WriteConfiguration (Cvar_VariableString ("cfgfile"));

	CDAudio_Shutdown ();
	S_Shutdown();
	IN_Shutdown ();
	Vid_Shutdown();
}
