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
#include "OutputDeviceFile.h"

cvar_t	*rcon_client_password;
cvar_t	*rcon_address;

cvar_t	*cl_noskins;
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
cvar_t	*msg;		//?? what this cvar for (can remove - and no errors ...)
cvar_t	*hand;
cvar_t	*gender, *gender_auto;

cvar_t	*cl_vwep;
cvar_t	*cl_3rd_person;
cvar_t	*cl_cameraDist, *cl_cameraHeight, *cl_cameraAngle;

cvar_t  *cl_extProtocol;

cvar_t	*cl_newfx;
cvar_t	*cl_showbboxes;
cvar_t	*r_sfx_pause;

cvar_t	*cl_infps;

client_static_t	cls;
client_state_t	cl;

centity_t		*cl_entities;	// [MAX_EDICTS]

clEntityState_t	cl_parse_entities[MAX_PARSE_ENTITIES];


/*-----------------------------------------------------------------------------
	Requesting server status
-----------------------------------------------------------------------------*/

static bool statusRequest;
static bool cl_cheats;				// value of "cheats" cvar from server
static char	cl_mapname[MAX_QPATH];
static char	cl_gamename[MAX_QPATH];

static bool TryParseStatus (const char *str)
{
	if (!statusRequest) return false;

	TString<MAX_MSGLEN> Buf;
	Buf = str;
	// remove players info
	char *s = Buf.chr ('\n');
	if (!s) return false;
	*s = 0;

	// validate ServerInfo
	const char *v;
	if (!(v = Info_ValueForKey (Buf, "mapname"))) return false;
	strcpy (cl_mapname, v);

	if (!(v = Info_ValueForKey (Buf, "gamename"))) return false;
	strcpy (cl_gamename, v);

	if (!(v = Info_ValueForKey (Buf, "cheats"))) return false;
	cl_cheats = atoi (v) != 0;

	appPrintf ("map: %s game: %s cheats: %d\n", cl_mapname, cl_gamename, cl_cheats);

	statusRequest = false;
	return true;
}

static void SendStatusRequest ()
{
	statusRequest  = true;
	cl_cheats      = false;
	cl_mapname[0]  = 0;		// needed ??
	cl_gamename[0] = 0;
	Netchan_OutOfBandPrint (NS_CLIENT, cls.serverAddr, "status");
}

static void CloseStatusRequest ()
{
	if (statusRequest)
	{
		statusRequest  = false;
		Com_DPrintf ("no status answer\n");
	}
}


/*-----------------------------------------------------------------------------
	Client-side demo recording
-----------------------------------------------------------------------------*/

// Dumps the current net message, prefixed by the length
void CL_WriteDemoMessage ()
{
	// the first eight bytes are just packet sequencing stuff
	int len = net_message.cursize-8;
	int swlen = LittleLong(len);
	fwrite (&swlen, 4, 1, cls.demofile);
	fwrite (net_message.data+8,	len, 1, cls.demofile);
}


// stop recording a demo
static void CL_Stop_f ()
{
	if (!cls.demorecording)
	{
		appWPrintf ("Not recording a demo.\n");
		return;
	}

	// finish up
	int len = -1;
	fwrite (&len, 4, 1, cls.demofile);
	fclose (cls.demofile);
	cls.demofile = NULL;
	cls.demorecording = false;
	appPrintf ("Stopped demo.\n");
}


// Begins recording a demo from the current position
static void CL_Record_f (bool usage, int argc, char **argv)
{
	int		i;

	if (argc != 2 || usage)
	{
		appPrintf ("Usage: record <demoname>\n");
		return;
	}

	if (cls.demorecording)
	{
		appWPrintf ("Already recording.\n");
		return;
	}

	if (cls.state != ca_active)
	{
		appWPrintf ("You must be in a level to record.\n");
		return;
	}

	// open the demo file
	TString<MAX_OSPATH> Name;
	Name.sprintf ("./%s/demos/%s.dm2", FS_Gamedir(), argv[1]);

	appPrintf ("recording to %s.\n", *Name);
	appMakeDirectoryForFile (Name);
	cls.demofile = fopen (Name, "wb");
	if (!cls.demofile)
	{
		appWPrintf ("Cannot create file %s\n", *Name);
		return;
	}
	cls.demorecording = true;

	// don't start saving messages until a non-delta compressed message is received
	cls.demowaiting = true;

	// write out messages to hold the startup information
	sizebuf_t buf;
	byte buf_data[MAX_MSGLEN];
	buf.Init (ARRAY_ARG(buf_data));

	// send the serverdata
	MSG_WriteByte (&buf, svc_serverdata);
	MSG_WriteLong (&buf, cls.newprotocol ? -PROTOCOL_VERSION : PROTOCOL_VERSION);
	MSG_WriteLong (&buf, 0x10000 + cl.servercount);
	MSG_WriteByte (&buf, 1);	// demos are always attract loops
	MSG_WriteString (&buf, cl.gamedir);	//?? original q2: game will not be changed correctly from demos; current: not changed at all;
										//?? so: can write ANY string here, and remove cl.gamedir[] at all
	MSG_WriteShort (&buf, cl.playernum);

	MSG_WriteString (&buf, cl.configstrings[CS_NAME]);

	// configstrings
	for (i = 0; i < MAX_CONFIGSTRINGS; i++)
	{
		if (cl.configstrings[i][0])
		{
			if (buf.cursize + strlen (cl.configstrings[i]) + 32 > buf.maxsize)
			{	// write it out
				int len = LittleLong (buf.cursize);
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
	entityStateEx_t	nullstate;
	memset (&nullstate, 0, sizeof(nullstate));
	for (i = 0; i < MAX_EDICTS; i++)
	{
		entityStateEx_t *ent = &cl_entities[i].baseline;
		if (!ent->modelindex)
			continue;

		if (buf.cursize + 64 > buf.maxsize)
		{	// write it out
			int len = LittleLong (buf.cursize);
			fwrite (&len, 4, 1, cls.demofile);
			fwrite (buf.data, buf.cursize, 1, cls.demofile);
			buf.cursize = 0;
		}

		MSG_WriteByte (&buf, svc_spawnbaseline);
		MSG_WriteDeltaEntity (&buf, &nullstate, &cl_entities[i].baseline, true, true, cls.newprotocol);
	}

	MSG_WriteByte (&buf, svc_stufftext);
	MSG_WriteString (&buf, "precache\n");

	// write it to the demo file

	int len = LittleLong (buf.cursize);
	fwrite (&len, 4, 1, cls.demofile);
	fwrite (buf.data, buf.cursize, 1, cls.demofile);

	// the rest of the demo file will be individual frames
}


//======================================================================

static void NetchanAppendArgs (char **argv, int first, int last)
{
	TString<1024> Buf; Buf[0] = 0;
	for (int i = first; i < last; i++)
	{
		if (i > first) Buf += " ";
		Buf += argv[i];
	}
	MSG_WriteString (&cls.netchan.message, Buf);
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
		appWPrintf ("Unknown command \"%s\"\n", cmd);
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
		appWPrintf ("Can't \"%s\", not connected\n", argv[0]);	// argv[0] == "cmd"
		return;
	}

	// don't forward the first argument
	if (argc > 1)
	{
		MSG_WriteByte (&cls.netchan.message, clc_stringcmd);
		NetchanAppendArgs (argv, 1, argc);
	}
}


void CL_Pause (bool enable)
{
	if ((Cvar_VariableInt ("maxclients") == 1 || cl_cheats) && Com_ServerState () || cl.attractloop)
		Cvar_SetInteger ("paused", enable);
	else
		Cvar_SetInteger ("paused", 0);
}


static void CL_Pause_f ()
{
	CL_Pause (!cl_paused->integer);
}


void CL_Drop (bool fromError)
{
	if (fromError) SCR_EndLoadingPlaque (true);
	if (cls.state != ca_uninitialized && cls.state != ca_disconnected)
		CL_Disconnect ();
	M_ForceMenuOff ();
}


// We have gotten a challenge from the server, so try to connect
void CL_SendConnectPacket ()
{
	cvar_t::modifiedFlags &= ~CVAR_USERINFO;

	const char *str = "";
	if (cl_extProtocol->integer)
		str = " "NEW_PROTOCOL_ID" "STR(NEW_PROTOCOL_VERSION);
	else
		Com_DPrintf ("Extended protocol disabled\n");
	// "connect protocolVer qport challenge "userinfo" [extProtocolInfo]"
	Netchan_OutOfBandPrint (NS_CLIENT, cls.serverAddr, "connect "STR(PROTOCOL_VERSION)" %d %d \"%s\"%s",
		Cvar_VariableInt ("qport"), cls.challenge, Cvar_BitInfo (CVAR_USERINFO), str);
}


static void InitServerAddress ()
{
	if (!NET_StringToAdr (cls.serverName, &cls.serverAddr, PORT_SERVER))
	{
		TString<64> NameCopy;
		NameCopy = cls.serverName;			// cls.serverName will be erased
		cls.state = ca_disconnected;		// without this, CL_Disconnect() will throw exception ...
		cls.serverName[0] = 0;
		Com_DropError ("Bad server address %s\n", *NameCopy);
		return;
	}
}


// Send connect message when needed + resend a connect message if the last one has timed out
static void CL_InitiateConnection ()
{
	// if the local server is running and we aren't then connect
	if (cls.state == ca_disconnected && Com_ServerState ())
	{
		Com_DPrintf ("CL: found running server: connecting\n");
		cls.state = ca_connecting;
		strcpy (cls.serverName, "localhost");
		InitServerAddress ();
		SendStatusRequest ();
		// we don't need a challenge on the localhost
		CL_SendConnectPacket ();
		return;
	}

	if (cls.state != ca_connecting) return;
	if (cls.realtime - cls.connect_time < 3000) return;

	cls.connect_time = cls.realtime;	// for retransmit requests

	appPrintf ("Connecting to %s\n", cls.serverName);

	InitServerAddress ();
	SendStatusRequest ();
	Netchan_OutOfBandPrint (NS_CLIENT, cls.serverAddr, "getchallenge");
}


void CL_Connect_f (bool usage, int argc, char **argv)
{
	if (argc != 2 || usage)
	{
		appPrintf ("Usage: connect <server>\n");
		return;
	}

	// if running a local server, kill it and reissue
	if (Com_ServerState ())
		SV_Shutdown ("Server quit\n");
	CL_Disconnect ();

	NET_Config (true);			// allow remote

	cls.state = ca_connecting;
	appStrncpyz (cls.serverName, argv[1], sizeof(cls.serverName));
	cls.connect_time = -BIG_NUMBER;	// CL_InitiateConnection() will fire immediately
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
	if (!rcon_client_password->string[0])
	{
		appWPrintf ("\"rcon_password\" is not set\n");
		return;
	}

	NET_Config (true);		// allow remote

	TString<1024> Message;
	Message.sprintf ("\xFF\xFF\xFF\xFFrcon %s", rcon_client_password->string);
	for (int i = 1; i < argc; i++)
		Message += va(" %s", argv[i]);

	netadr_t to;
	if (cls.state >= ca_connected)
		to = cls.netchan.remote_address;
	else
	{
		if (!rcon_address->string[0])
		{
			appWPrintf ("Not connected and \"rcon_address\" is not set\n");
			return;
		}
		NET_StringToAdr (rcon_address->string, &to, PORT_SERVER);
	}

	NET_SendPacket (NS_CLIENT, Message.len () + 1, Message, to);
}


/*
=====================
CL_Disconnect

Goes from a connected state to full screen console state
Sends a disconnect message to the server
=====================
*/
void CL_Disconnect ()
{
	guard(CL_Disconnect);

	if (cls.state == ca_uninitialized || cls.state == ca_disconnected)
		return;

	if (timedemo->integer)
	{
		int time = appMilliseconds () - cl.timedemoStart;
		if (time > 0)
			appPrintf (S_GREEN"Total %d frames, %3.1f seconds: %3.1f avg fps %3.1f min fps\n", cl.timedemoFrames, time / 1000.0f,
				1000.0f * cl.timedemoFrames / time, 1000.0f / cl.timedemoLongestFrame);
	}

	r_blend[3] = 0;
	cls.connect_time = 0;

	SCR_StopCinematic ();

	if (cls.demorecording)
		CL_Stop_f ();

	// send a disconnect message to the server
	char final[32];
	final[0] = clc_stringcmd;
	strcpy (final+1, "disconnect");
//	int len = sterlen(final); == 11
	cls.netchan.Transmit (final, 11);
	cls.netchan.Transmit (final, 11);
	cls.netchan.Transmit (final, 11);

	CL_ClearState ();

	// stop download
	if (cls.download)
	{
		fclose (cls.download);
		cls.download = NULL;
	}

	cls.state = ca_disconnected;

	unguard;
}

void CL_Disconnect_f ()
{
	Com_DropError ("Disconnected from server");
}


/*
=================
CL_Changing_f

Just sent as a hint to the client that they should
drop to full console
=================
*/
void CL_Changing_f ()
{
	//ZOID
	//if we are downloading, we don't change!  This so we don't suddenly stop downloading a map
	if (cls.download)
		return;

	SCR_BeginLoadingPlaque ();
	cls.state = ca_connected;	// not active anymore, but not disconnected
	appPrintf ("\nChanging map...\n");
}


// The server is changing levels
void CL_Reconnect_f ()
{
	//ZOID
	//if we are downloading, we don't change!  This so we don't suddenly stop downloading a map
	if (cls.download)
		return;

	S_StopAllSounds_f ();
	if (cls.state == ca_connected)
	{
		appPrintf ("reconnecting...\n");
		cls.state = ca_connected;
		MSG_WriteChar (&cls.netchan.message, clc_stringcmd);
		MSG_WriteString (&cls.netchan.message, "new");
		return;
	}

	if (!cls.serverName[0]) return;

	if (cls.state >= ca_connected)
	{
		CL_Disconnect ();
		cls.connect_time = cls.realtime - 1500;
	}
	else
		cls.connect_time = -BIG_NUMBER; // fire immediately

	cls.state = ca_connecting;
	appPrintf ("reconnecting...\n");
}


void CL_PingServers_f ()
{
	NET_Config (true);		// allow remote

	const char *cmd = "info " STR(PROTOCOL_VERSION);
	netadr_t adr;

	// send a broadcast packet
	appPrintf ("pinging broadcast...\n");
	NET_StringToAdr ("0.0.0.0", &adr, PORT_SERVER);		// address does not matters -- really, will only set port
	adr.type = NA_BROADCAST;
	Netchan_OutOfBandPrint (NS_CLIENT, adr, cmd);

	// implicitly check localhost for local dedicated server
	if (!Com_ServerState ())
	{
		NET_StringToAdr ("127.0.0.1", &adr, PORT_SERVER);
		Netchan_OutOfBandPrint (NS_CLIENT, adr, cmd);
	}

	// send a packet to each address book entry
	for (int i = 0; i < NUM_ADDRESSBOOK_ENTRIES; i++)
	{
		const char *adrstring = Cvar_VariableString (va("adr%d", i));
		if (!adrstring || !adrstring[0]) continue;

		appPrintf ("pinging %s...\n", adrstring);
		if (!NET_StringToAdr (adrstring, &adr, PORT_SERVER))
		{
			appPrintf ("Bad address: %s\n", adrstring);
			continue;
		}
		Netchan_OutOfBandPrint (NS_CLIENT, adr, cmd);
	}
}


/*
=================
CL_Skins_f

Load or download any custom player skins and models
=================
*/
void CL_Skins_f ()
{
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		if (!cl.configstrings[CS_PLAYERSKINS+i][0])
			continue;
		appPrintf ("client %d: %s\n", i, cl.configstrings[CS_PLAYERSKINS+i]);
		SCR_UpdateScreen ();
		Sys_ProcessMessages ();	// pump message loop
		CL_ParseClientinfo (i);
	}
}


/*-----------------------------------------------------------------------------
	Connectionless communication
	Responses to broadcasts, etc
-----------------------------------------------------------------------------*/

static char *connectStr;

static void cClientConnect (int argc, char **argv)
{
	CloseStatusRequest ();

	// server connection
	if (cls.state == ca_connected)
	{
		appPrintf ("dup connect received: ignored\n");
		return;
	}
	cls.netchan.Setup (NS_CLIENT, net_from, 0);	// use default qport value
	MSG_WriteChar (&cls.netchan.message, clc_stringcmd);
	MSG_WriteString (&cls.netchan.message, "new");
	cls.state       = ca_connected;
	cls.newprotocol = !strcmp (argv[1], NEW_PROTOCOL_ID);
	if (cls.newprotocol)
	{
		int ver = atoi (argv[2]);
		if (ver != NEW_PROTOCOL_VERSION)
		{
			if (ver < NEW_PROTOCOL_VERSION)
				appWPrintf ("Server supports older version of extended protocol\n");
			cls.newprotocol = false;
		}
		else
			Com_DPrintf ("Connected to server using extended protocol\n");
	}
	else if (cl_extProtocol->integer)
		Com_DPrintf ("Server does not support extended protocol\n");
}


static void cInfo (int argc, char **argv)
{
	// server responding to a status broadcast
	if (argc != 1)
	{
		// this can be a loopback message from ourselves -- request command
		// is "info <protocol>", answer is "info\n<data>"
		return;
	}

	appPrintf ("%s\n", connectStr);
	M_AddToServerList (net_from, connectStr);
}


static void cCmd (int argc, char **argv)
{
	if (!NET_IsLocalAddress (&net_from))
	{
		appPrintf ("Command packet from remote host. Ignored.\n");
		return;
	}
	Cbuf_AddText (connectStr);
	Cbuf_AddText ("\n");
}


static void cPrint (int argc, char **argv)
{
	// print command from somewhere
	if (TryParseStatus (connectStr)) return;	// do not print status message

	appPrintf ("%s", connectStr);
}


static void cPing (int argc, char **argv)
{
	Netchan_OutOfBandPrint (NS_CLIENT, net_from, "ack");
}


static void cChallenge (int argc, char **argv)
{
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
	{"info",	cInfo},
	{"cmd",		cCmd},
	{"print",	cPrint},
	{"ping",	cPing},
	{"challenge", cChallenge},
	{"echo",	cEcho}
};


void CL_ConnectionlessPacket ()
{
	guard(CL_ConnectionlessPacket);

	net_message.BeginReading ();
	MSG_ReadLong (&net_message);	// skip the -1

	const char *s = MSG_ReadString (&net_message);
	connectStr = strchr (s, '\n');
	char cmd[1024];
	if (connectStr)
	{
		connectStr++;
		appStrncpyz (cmd, s, connectStr - s);
	}
	else
	{
		strcpy (cmd, s);
		connectStr = "";			// just in case
	}
	// connectStr = NULL || next line ptr

	Com_DPrintf ("%s: %s\n", NET_AdrToString (&net_from), cmd);

	if (!ExecuteCommand (s, ARRAY_ARG(connectionlessCmds)))
		appPrintf ("Unknown command \"%s\".\n", cmd);

	unguard;
}


static void CL_ReadPackets ()
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
			appWPrintf ("%s: runt packet\n", NET_AdrToString (&net_from));
			continue;
		}

		// packet from server
		if (!NET_CompareAdr (&net_from, &cls.netchan.remote_address))
		{
			Com_DPrintf ("%s: sequenced packet without connection\n", NET_AdrToString (&net_from));
			continue;
		}
		if (!cls.netchan.Process (&net_message))
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
			appWPrintf ("\nServer connection timed out.\n");
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
CL_Userinfo_f
==============
*/
void CL_Userinfo_f ()
{
	appPrintf (S_GREEN"------- User info settings -------\n");
	Info_Print (Cvar_BitInfo (CVAR_USERINFO));
}

/*
======================
CL_RegisterSounds
======================
*/
void CL_RegisterSounds ()
{
	S_BeginRegistration ();
	CL_RegisterTEntSounds ();

	for (int i = 1; i < MAX_SOUNDS; i++)
	{
		if (!cl.configstrings[CS_SOUNDS+i][0])
			break;
		cl.sound_precache[i] = S_RegisterSound (cl.configstrings[CS_SOUNDS+i]);
		Sys_ProcessMessages ();	// pump message loop
	}
	S_EndRegistration ();
}


// Restart the sound subsystem so it can pick up new parameters and flush all sounds
void CL_Snd_Restart_f ()
{
	S_Shutdown ();
	S_Init ();
	CL_RegisterSounds ();
}


//-----------------------------------------------------------------------------

void CL_WriteConfiguration (const char *filename)
{
	if (cls.state == ca_uninitialized)
		return;

	COutputDeviceFile Out(va("./%s/%s", FS_Gamedir(), filename), true);
	Out.NoColors = false;	// do not modify contents
	if (!Out.IsOpened ()) return;

	Out.Printf ("// generated by %s, do not modify\n", appPackage ());
	Out.Printf ("//\n// Key bindings\n//\n");
	Key_WriteBindings (&Out);
	Out.Printf ("//\n// Cvars\n//\n");
	Cvar_WriteVariables (&Out, 0xFFFFFFFF, CVAR_USER_CREATED|CVAR_GAME_CREATED, "");
	Cvar_WriteVariables (&Out, CVAR_USER_CREATED, CVAR_GAME_CREATED, "// user created variables\n");
	Cvar_WriteVariables (&Out, CVAR_GAME_CREATED, CVAR_USER_CREATED, "// game created variables\n");
	Out.Printf ("//\n// Aliases\n//\n");
	Cmd_WriteAliases (&Out);
}


void CL_WriteConfig_f (bool usage, int argc, char **argv)
{
	if (argc != 2 || usage)
	{
		appPrintf ("Usage: writeconfig <filename>\n");
		return;
	}
	TString<MAX_OSPATH> Name;
	Name = argv[1];
	if (!Name.chr ('.')) Name += ".cfg";
	CL_WriteConfiguration (Name);
}


static void CL_InitLocal ()
{
CVAR_BEGIN(vars)
	CVAR_FULL(&cl_add_blend, "cl_blend", "1", 0),
	CVAR_FULL(&cl_add_lights, "cl_lights", "1", 0),
	CVAR_FULL(&cl_add_particles, "cl_particles", "1", 0),
	CVAR_FULL(&cl_add_entities, "cl_entities", "1", 0),
	CVAR_VAR(cl_gun, 1, 0),
	CVAR_VAR(cl_footsteps, 1, 0),
	CVAR_VAR(cl_noskins, 0, 0),
	CVAR_VAR(cl_predict, 1, 0),
	CVAR_VAR(cl_maxfps, 0, CVAR_ARCHIVE),		// ignored by default

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
	CVAR_VAR(gender, male, CVAR_USERINFO|CVAR_ARCHIVE|CVAR_NOUPDATE),
	CVAR_VAR(gender_auto, 1, CVAR_ARCHIVE),

	CVAR_VAR(cl_vwep, 1, CVAR_ARCHIVE),
	CVAR_VAR(cl_3rd_person, 0, CVAR_ARCHIVE),		//?? cheat
	CVAR_VAR(cl_cameraDist, 80, CVAR_ARCHIVE),
	CVAR_VAR(cl_cameraHeight, 10, CVAR_ARCHIVE),
	CVAR_VAR(cl_cameraAngle, 0, 0),

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

	RegisterCommand ("connect", CL_Connect_f);
	RegisterCommand ("reconnect", CL_Reconnect_f);
	RegisterCommand ("rcon", CL_Rcon_f);

	RegisterCommand ("precache", CL_Precache_f);
	RegisterCommand ("download", CL_Download_f);

	RegisterCommand ("writeconfig", CL_WriteConfig_f);
}


void CL_FixCvarCheats ()
{
	bool cheats = cl_cheats;						// value from server

	if (cl.attractloop) cheats = true;
	if (cls.state != ca_connected && cls.state != ca_active) cheats = true;

	//?? another way to check for deathmatch?
	const char *str = cl.configstrings[CS_MAXCLIENTS];
	if ((str[0] == '1' && str[1] == 0) || str[0] == 0)	// == "1" or ""
		cheats = true;									// single player can cheat

	Cvar_Cheats (cheats);
}


/*-----------------------------------------------------------------------------
	Client frame (tick)
-----------------------------------------------------------------------------*/

#define MAX_GUI_FPS		60

extern void SV_DrawTexts ();		//?? move outside, different API ?


void CL_Frame (float msec, float realMsec)
{
	guard(CL_Frame);

	SV_DrawTexts ();

	static double extratime_real;	// real frame time, msec
	static double extratime;		// scaled frame time, sec

	extratime += msec / 1000.0f;
	extratime_real += realMsec;

	if (!timedemo->integer)			// ignore cl_maxfps in timedemo
	{
		int maxFps = cl_maxfps->integer;
		if (cls.key_dest != key_game &&	// GUI is active
			(!cl_maxfps->integer || cl_maxfps->integer > MAX_GUI_FPS))
			maxFps = MAX_GUI_FPS;
		// here extratime can be accumulated between frames
		if (maxFps > 0 && extratime_real < 1000.0f / maxFps)	// ignore when cl_maxfps==0
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
	cl.time  = appFloor (cl.ftime * 1000.0f);
	extratime      = 0;
	extratime_real = 0;

	cls.realtime = appMilliseconds ();

	if (cls.frametime > 0.2f)		// low FPS fix ?? (cannot send user commands with frame time > 255ms == 0.255s)
		cls.frametime = 0.2f;

	// if in the debugger last frame, don't timeout
	if (msec > 5000)
		cls.netchan.last_received = appMilliseconds ();

	// fetch results from server
	CL_ReadPackets ();

	Sys_ProcessMessages ();			// get OS events (??)
	Cbuf_Execute ();				// process console commands (??)
	CL_FixCvarCheats ();			// fix any cheating cvars

	// send a new command message to the server
	CL_SendCmd ();					// send intentions now (IN_Tick()?)
	CL_InitiateConnection ();		// resend a connection request if necessary
	// predict all unacknowledged movements
	CL_PredictMovement ();

	// allow renderer DLL change
	Vid_Tick ();
	if (!cl.rendererReady && cls.state == ca_active)
		V_InitRenderer ();

	// update the screen
	if (com_speeds->integer) time_before_ref = appCycles ();	//?? move
	SCR_UpdateScreen ();
	if (com_speeds->integer) time_after_ref = appCycles ();		//??

	// update audio
	S_Update (cl.refdef.vieworg, cl.v_right);

	CDAudio_Update();

	unguard;
}

//============================================================================

void CL_ClearState ()
{
	S_StopAllSounds_f ();
	CL_ClearEffects ();
	CL_ClearTEnts ();
	CL_ClearLightStyles ();

	// wipe the entire cl structure
	memset (&cl, 0, sizeof(cl));
	memset (cl_entities, 0, sizeof(centity_t[MAX_EDICTS]));

	cls.netchan.message.Clear ();
}


void CL_Init ()
{
	guard(CL_Init);

	Con_Init ();
	Vid_Init ();
	S_Init ();						// win32: sound must be initialized after window is created (can link to dummy wnd?)

	if (!DEDICATED) cl_entities = new centity_t [MAX_EDICTS];

	V_Init ();
	M_Init ();
	SCR_Init ();
	CDAudio_Init ();
	CL_InitLocal ();
	IN_Init ();

	unguard;
}


void CL_Shutdown ()
{
	EXEC_ONCE (						// no repeated shutdowns
		if (!DEDICATED)
		{
			CL_Disconnect ();
			delete cl_entities;
			if (!GIsFatalError)		// do not write configuration when error occured
				CL_WriteConfiguration (Cvar_VariableString ("cfgfile"));
			CDAudio_Shutdown ();
			S_Shutdown();
			IN_Shutdown ();
			Vid_Shutdown();
		}
	)
}
