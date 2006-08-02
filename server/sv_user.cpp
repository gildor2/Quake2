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

edict_t	*sv_player;

/*
============================================================

USER STRINGCMD EXECUTION

sv_client and sv_player will be valid.
============================================================
*/

// Sends the first message from the server to a connected client.
// This will be sent on the initial connection and upon each server load.
static void SV_New_f (int argc, char **argv)
{
	Com_DPrintf ("New() from %s\n", *sv_client->Name);

	if (sv_client->state != cs_connected)
	{
		appPrintf ("New not valid -- already spawned\n");
		return;
	}

	// demo servers just dump the file message
	if (sv.state == ss_demo)
	{
		TString<MAX_OSPATH> Name;
		Name.sprintf ("demos/%s", sv.name);
		sv.rdemofile = GFileSystem->OpenFile (Name);
		if (!sv.rdemofile)
			Com_DropError ("Couldn't open %s\n", *Name);
		return;
	}

	// serverdata needs to go over for all types of servers
	// to make sure the protocol is right, and to set the gamedir
	MSG_WriteByte (&sv_client->netchan.message, svc_serverdata);
	MSG_WriteLong (&sv_client->netchan.message, PROTOCOL_VERSION);
	MSG_WriteLong (&sv_client->netchan.message, svs.spawncount);
	MSG_WriteByte (&sv_client->netchan.message, sv.attractloop);
	MSG_WriteString (&sv_client->netchan.message, FS_Gamedir());

	int playernum;
	if (sv.state == ss_cinematic || sv.state == ss_pic)
		playernum = -1;
	else
		playernum = sv_client - svs.clients;
	MSG_WriteShort (&sv_client->netchan.message, playernum);

	// send full levelname
	MSG_WriteString (&sv_client->netchan.message, sv.configstrings[CS_NAME]);

	// game server
	if (sv.state == ss_game)
	{
		// set up the entity for the client
		edict_t *ent = EDICT_NUM(playernum+1);
		ent->s.number = playernum+1;
		sv_client->edict = ent;
		memset (&sv_client->lastcmd, 0, sizeof(sv_client->lastcmd));

		// begin fetching configstrings
		MSG_WriteByte (&sv_client->netchan.message, svc_stufftext);
		MSG_WriteString (&sv_client->netchan.message, va("cmd configstrings %i 0\n",svs.spawncount) );
	}
}


static void SV_Configstrings_f (int argc, char **argv)
{
	Com_DPrintf ("Configstrings() from %s\n", *sv_client->Name);

	if (sv_client->state != cs_connected)
	{
		appPrintf ("configstrings not valid -- already spawned\n");
		return;
	}

	// handle the case of a level changing while a client was connecting
	if (atoi (argv[1]) != svs.spawncount)
	{
		appPrintf ("SV_Configstrings_f from different level\n");
		SV_New_f (argc, argv);
		return;
	}

	int start = atoi (argv[2]);

	// write a packet full of data
	int mid = sv_client->maxPacketSize / 2;
	while (sv_client->netchan.message.cursize < mid && start < MAX_CONFIGSTRINGS)
	{
		if (sv.configstrings[start][0])
		{
			MSG_WriteByte (&sv_client->netchan.message, svc_configstring);
			MSG_WriteShort (&sv_client->netchan.message, start);
			MSG_WriteString (&sv_client->netchan.message, sv.configstrings[start]);
		}
		start++;
	}

	// send next command
	if (start == MAX_CONFIGSTRINGS)
	{
		MSG_WriteByte (&sv_client->netchan.message, svc_stufftext);
		MSG_WriteString (&sv_client->netchan.message, va("cmd baselines %i 0\n",svs.spawncount) );
	}
	else
	{
		MSG_WriteByte (&sv_client->netchan.message, svc_stufftext);
		MSG_WriteString (&sv_client->netchan.message, va("cmd configstrings %i %i\n",svs.spawncount, start) );
	}
}


static void SV_Baselines_f (int argc, char **argv)
{
	guard(SV_Baselines_f);

	Com_DPrintf ("Baselines() from %s\n", *sv_client->Name);

	if (sv_client->state != cs_connected)
	{
		appPrintf ("baselines not valid -- already spawned\n");
		return;
	}

	// handle the case of a level changing while a client was connecting
	if (atoi (argv[1]) != svs.spawncount)
	{
		appPrintf ("SV_Baselines_f from different level\n");
		SV_New_f (argc, argv);
		return;
	}

	int start = atoi (argv[2]);

	entityStateEx_t	nullstate;
	memset (&nullstate, 0, sizeof(nullstate));

	// write a packet full of data
	int mid = sv_client->maxPacketSize / 2;
	while (sv_client->netchan.message.cursize < mid && start < MAX_EDICTS)
	{
		entityStateEx_t *base = &sv.baselines[start];
		if (base->modelindex || base->sound || base->effects)
		{
			MSG_WriteByte (&sv_client->netchan.message, svc_spawnbaseline);
			MSG_WriteDeltaEntity (&sv_client->netchan.message, &nullstate, base, true, true, sv_client->newprotocol);
		}
		start++;
	}

	// send next command
	if (start == MAX_EDICTS)
	{
		MSG_WriteByte (&sv_client->netchan.message, svc_stufftext);
		MSG_WriteString (&sv_client->netchan.message, va("precache %i\n", svs.spawncount) );
	}
	else
	{
		MSG_WriteByte (&sv_client->netchan.message, svc_stufftext);
		MSG_WriteString (&sv_client->netchan.message, va("cmd baselines %i %i\n",svs.spawncount, start) );
	}

	unguard;
}


static void SV_Begin_f (int argc, char **argv)
{
	Com_DPrintf ("Begin() from %s\n", *sv_client->Name);

	// handle the case of a level changing while a client was connecting
	if (atoi (argv[1]) != svs.spawncount)
	{
		appPrintf ("SV_Begin_f from different level\n");
		SV_New_f (argc, argv);
		return;
	}

	sv_client->state = cs_spawned;

	guard(ge.ClientBegin);
	// call the game begin function
	ge->ClientBegin (sv_player);
	unguard;

	Cbuf_InsertFromDefer ();	//!! unpause buffer
}


//=============================================================================

static void SV_NextDownload_f (int argc, char **argv)
{
	if (!sv_client->download)
		return;

	int r = sv_client->downloadsize - sv_client->downloadcount;
	int max;
	if (sv_client->newprotocol)	//!! and server is on LAN (not modem connection) or no active clients
		max = 10240;			// up to MAX_MSGLEN
	else
		max = 1024;				// up to MAX_MSGLEN_OLD
	if (r > max) r = max;

	MSG_WriteByte (&sv_client->netchan.message, svc_download);
	MSG_WriteShort (&sv_client->netchan.message, r);

	sv_client->downloadcount += r;
	int size = sv_client->downloadsize;
	if (!size)
		size = 1;
	int percent = sv_client->downloadcount * 100 / size;
	MSG_WriteByte (&sv_client->netchan.message, percent);
	sv_client->netchan.message.Write (sv_client->download + sv_client->downloadcount - r, r);

	if (sv_client->downloadcount != sv_client->downloadsize)
		return;

	delete sv_client->download;
	sv_client->download = NULL;
}


static void SV_BeginDownload_f (int argc, char **argv)
{
	extern	cvar_t *allow_download;
	extern	cvar_t *allow_download_players;
	extern	cvar_t *allow_download_models;
	extern	cvar_t *allow_download_sounds;
	extern	cvar_t *allow_download_maps;

	TString<MAX_OSPATH> Name;
	Name.filename (argv[1]);

	int offset = 0;
	if (argc > 2) offset = atoi (argv[2]); 		// downloaded offset

	// hacked by zoid to allow more conrol over download
	// first off, no .. or global allow check
	if (!allow_download->integer
		// leading dot is no good: "./dir" or "../dir"
		|| Name[0] == '.'
		// leading slash bad as well (root-relative path + network share)
		|| Name[0] == '/'
		// Win32 root-relative path
		|| Name[1] == ':'
		//?? simplify following part
		// next up, skin check
		|| (!memcmp (Name, "players/", 6) && !allow_download_players->integer)
		// now models
		|| (!memcmp (Name, "models/", 6) && !allow_download_models->integer)
		// now sounds
		|| (!memcmp (Name, "sound/", 6) && !allow_download_sounds->integer)
		// now maps (note special case for maps, must not be in pak)
		|| (!memcmp (Name, "maps/", 6) && !allow_download_maps->integer)
		// MUST be in a subdirectory
		|| !Name.chr ('/'))		//?? should allow to download from root dir *.pak files ?
	{	// don't allow anything with .. path
		MSG_WriteByte (&sv_client->netchan.message, svc_download);
		MSG_WriteShort (&sv_client->netchan.message, -1);
		MSG_WriteByte (&sv_client->netchan.message, 0);
		return;
	}


	if (sv_client->download)
		delete sv_client->download;

	sv_client->DownloadName  = Name;
	sv_client->download      = (byte*) GFileSystem->LoadFile (Name, &sv_client->downloadsize);
	sv_client->downloadcount = offset;

	if (offset > sv_client->downloadsize)
		sv_client->downloadcount = sv_client->downloadsize;

	if (!sv_client->download)
	{
		Com_DPrintf ("Couldn't download %s to %s\n", *Name, *sv_client->Name);
		if (sv_client->download) {
			delete sv_client->download;
			sv_client->download = NULL;
		}

		MSG_WriteByte (&sv_client->netchan.message, svc_download);
		MSG_WriteShort (&sv_client->netchan.message, -1);
		MSG_WriteByte (&sv_client->netchan.message, 0);
		return;
	}

	SV_NextDownload_f (argc, argv);
	Com_DPrintf ("Downloading %s to %s\n", *Name, *sv_client->Name);
}



//============================================================================


// The client is going to disconnect, so remove the connection immediately
static void SV_Disconnect_f (int argc, char **argv)
{
//	SV_EndRedirect ();
	SV_DropClient (sv_client, NULL);	// "user disconnected" will be printed by game dll
}


// Dumps the serverinfo info string
static void SV_ShowServerinfo_f (int argc, char **argv)
{
	Info_Print (Cvar_BitInfo (CVAR_SERVERINFO));
}


void SV_Nextserver ()
{
	if (sv.state == ss_game)
		return;			// can't nextserver while playing a normal game

	svs.spawncount++;	// make sure another doesn't sneak in
	const char *v = Cvar_VariableString ("nextserver");
	if (!v[0])
		Cbuf_AddText ("killserver\n");
	else
	{
		Cbuf_AddText (v);
		Cbuf_AddText ("\n");
	}
	Cvar_Set ("nextserver", "");
}


// A cinematic has completed or been aborted by a client, so move to the next server,
static void SV_Nextserver_f (int argc, char **argv)
{
	if (atoi (argv[1]) != svs.spawncount)
	{
		Com_DPrintf ("Nextserver() from wrong level, from %s\n", *sv_client->Name);
		return;		// leftover from last server
	}

	Com_DPrintf ("Nextserver() from %s\n", *sv_client->Name);

	SV_Nextserver ();
}

static const CSimpleCommand ucmds[] =
{
	// auto issued
	{"new", SV_New_f},
	{"configstrings", SV_Configstrings_f},
	{"baselines", SV_Baselines_f},
	{"begin", SV_Begin_f},

	{"nextserver", SV_Nextserver_f},

	{"disconnect", SV_Disconnect_f},

	// issued by hand at client consoles
	{"info", SV_ShowServerinfo_f},

	{"download", SV_BeginDownload_f},
	{"nextdl", SV_NextDownload_f}
};


static void SV_ExecuteUserCommand (const char *s)
{
	guard(SV_ExecuteUserCommand);

	sv_player = sv_client->edict;
	if (!ExecuteCommand (s, ARRAY_ARG(ucmds)) && sv.state == ss_game)
	{
		guard(ge.ClientCommand);
		SV_TokenizeString (s);
		ge->ClientCommand (sv_player);
		unguardf(("cmd=%s", s));
	}

	unguard;
}

/*
===========================================================================

USER CMD EXECUTION

===========================================================================
*/



void SV_ClientThink (client_t *cl, usercmd_t *cmd)
{
	cl->commandMsec -= cmd->msec;

	if (cl->commandMsec < 0 && sv_enforcetime->integer)
	{
		Com_DPrintf ("commandMsec underflow from %s\n", *cl->Name);
		return;
	}

	guard(ge.ClientThink);
	ge->ClientThink (cl->edict, cmd);
	unguard;
}


static void SV_MoveClient (client_t *cl)
{
	int checksumIndex = net_message.readcount;
	int checksum      = MSG_ReadByte (&net_message);
	int lastframe     = MSG_ReadLong (&net_message);
	if (lastframe != cl->lastframe)
	{
		cl->lastframe = lastframe;
		if (cl->lastframe > 0)
		{
			cl->frame_latency[cl->lastframe&(LATENCY_COUNTS-1)] =
				svs.realtime - cl->frames[cl->lastframe & UPDATE_MASK].senttime;
		}
	}

	static usercmd_t nullcmd;	// zero-filled
	usercmd_t oldest, oldcmd, newcmd;
	MSG_ReadDeltaUsercmd (&net_message, &nullcmd, &oldest);
	MSG_ReadDeltaUsercmd (&net_message, &oldest, &oldcmd);
	MSG_ReadDeltaUsercmd (&net_message, &oldcmd, &newcmd);

	if (cl->state != cs_spawned)
	{
		cl->lastframe = -1;
		return;
	}

	// if the checksum fails, ignore the rest of the packet
	int calculatedChecksum = Com_BlockSequenceCRCByte (
		net_message.data + checksumIndex + 1,
		net_message.readcount - checksumIndex - 1,
		cl->netchan.incoming_sequence);

	if (calculatedChecksum != checksum)
	{
		Com_DPrintf ("Failed command checksum for %s (%d != %d)/%d\n",
			*cl->Name, calculatedChecksum, checksum,
			cl->netchan.incoming_sequence);
		return;
	}

	if (!sv_paused->integer || sv_maxclients->integer > 1)
	{
		int net_drop = cl->netchan.dropped;
		if (net_drop > 0 && net_drop < 20)
		{
			while (net_drop > 2)
			{
				SV_ClientThink (cl, &cl->lastcmd);
				net_drop--;
			}
			if (net_drop > 1)
				SV_ClientThink (cl, &oldest);

			if (net_drop > 0)
				SV_ClientThink (cl, &oldcmd);

		}
		SV_ClientThink (cl, &newcmd);
	}

	cl->lastcmd = newcmd;
}


#define	MAX_STRINGCMDS	8

// The current net_message is parsed for the given client
void SV_ExecuteClientMessage (client_t *cl)
{
	int c = -1;
	guard(SV_ExecuteClientMessage);

	sv_client = cl;
	sv_player = sv_client->edict;

	// only allow one move command
	bool move_issued = false;
	int stringCmdCount = 0;

	while (true)
	{
		if (net_message.readcount > net_message.cursize)
		{
			appPrintf ("SV_ReadClientMessage: badread\n");
			SV_DropClient (cl, NULL);
			return;
		}

		c = MSG_ReadByte (&net_message);
		if (c == -1) break;

		switch (c)
		{
		case clc_nop:
			break;

		case clc_userinfo:
			cl->Userinfo = MSG_ReadString (&net_message);
			SV_UserinfoChanged (cl);
			break;

		case clc_move:
			if (move_issued) return;		// someone is trying to cheat...
			move_issued = true;
			SV_MoveClient (cl);
			break;

		case clc_stringcmd:
			{
				const char *s = MSG_ReadString (&net_message);
				// malicious users may try using too many string commands
				if (++stringCmdCount < MAX_STRINGCMDS)
					SV_ExecuteUserCommand (s);

				if (cl->state == cs_zombie)
					return;	// disconnect command
			}
			break;

		default:
			appPrintf ("SV_ExecuteClientMessage: unknown command char\n");
			SV_DropClient (cl, NULL);
			return;
		}
	}
	unguardf(("%d", c));
}
