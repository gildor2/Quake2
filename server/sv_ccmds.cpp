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
#include <time.h>

//?? only for screenshot
#include "../client/ref.h"
extern refExport_t re;
//void SCR_UpdateScreen (void);		//?? remove
void SCR_SetLevelshot (const char *name);

/*
===============================================================================

OPERATOR CONSOLE ONLY COMMANDS

These commands can only be entered from stdin or by a remote operator datagram
===============================================================================
*/

/*
====================
SV_SetMaster_f

Specify a list of master servers
====================
*/
static void SV_SetMaster_f (int argc, char **argv)
{
	int		i, slot;

	// only dedicated servers send heartbeats
	if (!DEDICATED)
	{
		Com_Printf ("Only dedicated servers use masters.\n");
		return;
	}

	// make sure the server is listed public
	Cvar_Set ("public", "1");

	for (i = 1; i < MAX_MASTERS; i++)
		memset (&master_adr[i], 0, sizeof(master_adr[i]));

	slot = 1;		// slot 0 will always contain the id master
	for (i = 1; i < argc; i++)
	{
		if (slot == MAX_MASTERS)
			break;

		if (!NET_StringToAdr (argv[i], &master_adr[i]))
		{
			Com_Printf ("Bad address: %s\n", argv[i]);
			continue;
		}
		if (master_adr[slot].port == 0)
			master_adr[slot].port = BigShort (PORT_MASTER);

		Com_Printf ("Master server at %s\n", NET_AdrToString (&master_adr[slot]));

		Com_Printf ("Sending a ping.\n");

		Netchan_OutOfBandPrint (NS_SERVER, master_adr[slot], "ping");

		slot++;
	}

	svs.last_heartbeat = -BIG_NUMBER;
}



/*
===============================================================================

SAVEGAME FILES

===============================================================================
*/

/*
=====================
SV_WipeSavegame

Delete save/<name>/*.*
=====================
*/
void SV_WipeSavegame (char *savename)
{
	Com_DPrintf ("SV_WipeSaveGame(%s)\n", savename);
	FS_RemoveFiles (va("%s/" SAVEGAME_DIRECTORY "/%s/*.*", FS_Gamedir (), savename));
}


/*
================
SV_CopySaveGame
================
*/
void SV_CopySaveGame (char *src, char *dst)
{
	Com_DPrintf ("SV_CopySaveGame(%s, %s)\n", src, dst);
	SV_WipeSavegame (dst);
	FS_CopyFiles (va("%s/" SAVEGAME_DIRECTORY "/%s/*.*", FS_Gamedir(), src),
				  va("%s/" SAVEGAME_DIRECTORY "/%s/", FS_Gamedir(), dst));
}


/*
==============
SV_WriteLevelFile

==============
*/
void SV_WriteLevelFile (void)
{
	char	name[MAX_OSPATH];
	FILE	*f;

	Com_DPrintf("SV_WriteLevelFile()\n");

	appSprintf (ARRAY_ARG(name), "%s/" SAVEGAME_DIRECTORY "/current/%s." SAVEGAME_SERVER_EXTENSION, FS_Gamedir(), sv.name);
	f = fopen (name, "wb");
	if (!f)
	{
		Com_WPrintf ("Failed to open %s\n", name);
		return;
	}
	fwrite (sv.configstrings, sizeof(sv.configstrings), 1, f);
	CM_WritePortalState (f);
	fclose (f);

	appSprintf (ARRAY_ARG(name), "%s/" SAVEGAME_DIRECTORY "/current/%s." SAVEGAME_GAME_EXTENSION, FS_Gamedir(), sv.name);
	ge->WriteLevel (name);
}

/*
==============
SV_ReadLevelFile

==============
*/

void SV_ReadLevelFile (void)
{
	char	name[MAX_OSPATH];
	FILE	*f;

	Com_DPrintf ("SV_ReadLevelFile()\n");

	appSprintf (ARRAY_ARG(name), "%s/" SAVEGAME_DIRECTORY "/current/%s." SAVEGAME_SERVER_EXTENSION, FS_Gamedir(), sv.name);
	f = fopen(name, "rb");
	if (!f)
	{
		Com_WPrintf ("Failed to open %s\n", name);
		return;
	}
	fread (sv.configstrings, sizeof(sv.configstrings), 1, f);
	CM_ReadPortalState (f);
	fclose (f);

	appSprintf (ARRAY_ARG(name), "%s/" SAVEGAME_DIRECTORY "/current/%s." SAVEGAME_GAME_EXTENSION, FS_Gamedir(), sv.name);
	ge->ReadLevel (name);
}

/*
==============
SV_WriteServerFile

==============
*/
void SV_WriteServerFile (bool autosave, char *dir)
{
	FILE	*f;
	cvar_t	*var;
	char	name[MAX_OSPATH], string[128];
	char	comment[128];
	time_t	aclock;
	struct tm	*newtime;

	Com_DPrintf("SV_WriteServerFile(%s, %s)\n", autosave ? "true" : "false", dir);

	appSprintf (ARRAY_ARG(name), "%s/" SAVEGAME_DIRECTORY "/%s/server." SAVEGAME_VARS_EXTENSION, FS_Gamedir(), dir);
	f = fopen (name, "wb");
	if (!f)
	{
		Com_WPrintf ("Couldn't write %s\n", name);
		return;
	}
	// write the comment field
	memset (comment, 0, sizeof(comment));

	if (!autosave)
	{
		time (&aclock);
		newtime = localtime (&aclock);
		appSprintf (ARRAY_ARG(comment), "%2d:%d%d %2d/%2d  %s", newtime->tm_hour, newtime->tm_min/10,
			newtime->tm_min%10, newtime->tm_mon+1, newtime->tm_mday, sv.configstrings[CS_NAME]);
	}
	else
	{	// autosaved
		appSprintf (ARRAY_ARG(comment), "ENTERING %s", sv.configstrings[CS_NAME]);
	}

	fwrite (comment, 1, 32, f);

	// write the mapcmd
	fwrite (svs.mapcmd, 1, sizeof(svs.mapcmd), f);

	// write all CVAR_LATCH cvars
	// these will be things like coop, skill, deathmatch, etc
	for (var = cvar_vars; var ; var=var->next)
	{
		if (!(var->flags & CVAR_LATCH))
			continue;
		if (strlen(var->name) >= sizeof(name)-1 || strlen(var->string) >= sizeof(string)-1)
		{
			Com_WPrintf ("Cvar too long: %s = %s\n", var->name, var->string);
			continue;
		}
		memset (name, 0, sizeof(name));
		memset (string, 0, sizeof(string));
		strcpy (name, var->name);
		strcpy (string, var->string);
		fwrite (name, 1, sizeof(name), f);
		fwrite (string, 1, sizeof(string), f);
	}

	fclose (f);

	// write game state
	appSprintf (ARRAY_ARG(name), "%s/" SAVEGAME_DIRECTORY "/%s/game." SAVEGAME_VARS_EXTENSION, FS_Gamedir(), dir);
	ge->WriteGame (name, autosave);

	// perform screenshot
	if (!DEDICATED)					// can be uninitialized when dedicated server
	{
		re.Screenshot (SHOT_SMALL|SHOT_NOGAMMA|SHOT_SILENT|SHOT_NO_2D|SHOT_WAIT_3D|SHOT_JPEG,
			va("%s/" SAVEGAME_DIRECTORY "/%s/shot", FS_Gamedir(), dir));
//--		SCR_UpdateScreen ();			// required for screenshot performing
			// BUT: here we must wait for NEXT map, and if performing shot immediately, it can be from PREVIOUS map
	}
}

/*
==============
SV_ReadServerFile

==============
*/
void SV_ReadServerFile (void)
{
	FILE	*f;
	char	name[MAX_OSPATH], string[128];
	char	comment[32];
	char	mapcmd[128];

	Com_DPrintf("SV_ReadServerFile()\n");
	if (!DEDICATED) SCR_SetLevelshot ("/" SAVEGAME_DIRECTORY "/current/shot");

	appSprintf (ARRAY_ARG(name), "%s/" SAVEGAME_DIRECTORY "/current/server." SAVEGAME_VARS_EXTENSION, FS_Gamedir());
	f = fopen (name, "rb");
	if (!f)
	{
		Com_WPrintf ("Couldn't read %s\n", name);
		return;
	}
	// read the comment field
	fread (comment, sizeof(comment), 1, f);

	// read the mapcmd
	fread (mapcmd, sizeof(mapcmd), 1, f);

	// read all CVAR_LATCH cvars
	// these will be things like coop, skill, deathmatch, etc
	while (1)
	{
		if (!fread (name, 1, sizeof(name), f))
			break;
		fread (string, sizeof(string), 1, f);
		Com_DPrintf ("Set %s = %s\n", name, string);
		Cvar_ForceSet (name, string);
	}

	fclose (f);

	// start a new game fresh with new cvars
	SV_InitGame ();

	strcpy (svs.mapcmd, mapcmd);

	// read game state
	appSprintf (ARRAY_ARG(name), "%s/" SAVEGAME_DIRECTORY "/current/game." SAVEGAME_VARS_EXTENSION, FS_Gamedir());
	ge->ReadGame (name);
}


//=========================================================




/*
==================
SV_DemoMap_f

Puts the server in demo mode on a specific map/cinematic
==================
*/
static void SV_DemoMap_f (int argc, char **argv)
{
	const char *map, *ext;

	map = argv[1];
	ext = strchr (map, '.');

	if (Cvar_VariableInt ("nointro") == 0 && (!ext || !(stricmp (ext, ".dm2"))))
	{
		// allow .CIN files while playing intro (stop intro when trying to play demofile)
		Cvar_ForceSet ("nointro", "1");
		Cbuf_AddText ("nextserver=\"\"\n");
		if (sv.state) Cbuf_AddText ("disconnect\n");
		return;
	}

	if (!ext)
		map = va("%s.dm2", map);

	SV_Map (true, map, false);
}

/*
==================
SV_GameMap_f

Saves the state of the map just being exited and goes to a new map.

If the initial character of the map string is '*', the next map is
in a new unit, so the current savegame directory is cleared of
map files.

Example:

*inter.cin+jail

Clears the archived maps, plays the inter.cin cinematic, then
goes to map jail.bsp.
==================
*/
static void SV_GameMap_f (bool usage, int argc, char **argv)
{
	if (argc != 2 || usage)
	{
		Com_Printf ("Usage: gamemap <map>\n");
		return;
	}

	const char *map = argv[1];
	Com_DPrintf ("SV_GameMap(%s)\n", map);

	// check for clearing the current savegame
	if (map[0] == '*')
	{
		// wipe all the *.sav files
		SV_WipeSavegame ("current");
	}
	else
	{	// save the map just exited
		if (sv.state == ss_game)
		{
			int		i;
			bool	savedInUse[MAX_CLIENTS];
			client_t *cl;

			FS_CreatePath (va("%s/" SAVEGAME_DIRECTORY "/current/", FS_Gamedir()));
			// clear all the client.inuse flags before saving so that
			// when the level is re-entered, the clients will spawn
			// at spawn points instead of occupying body shells
			for (i = 0, cl = svs.clients; i < maxclients->integer; i++, cl++)
			{
				savedInUse[i] = cl->edict->inuse != 0;
				cl->edict->inuse = false;
			}

			SV_WriteLevelFile ();

			// we must restore these for clients to transfer over correctly
			for (i = 0, cl = svs.clients; i < maxclients->integer; i++, cl++)
				cl->edict->inuse = savedInUse[i];
		}
	}

	// start up the next map
	SV_Map (false, map, false );

	// archive server state
	appStrncpyz (svs.mapcmd, map, sizeof(svs.mapcmd));

	// copy off the level to the autosave slot
	if (!DEDICATED && !Cvar_VariableInt ("deathmatch"))
	{
		FS_CreatePath (va("%s/" SAVEGAME_DIRECTORY "/current/", FS_Gamedir()));
		SV_CopySaveGame ("current", "save0");
		SV_WriteServerFile (true, "save0");
	}
}

/*
==================
SV_Map_f

Goes directly to a given map without any savegame archiving.
For development work
==================
*/
static void SV_Map_f (int argc, char **argv)
{
	char	*map;

	// if not a pcx, demo, or cinematic, check to make sure the level exists
	map = argv[1];
	if (!strchr (map, '.'))
	{
		char	expanded[MAX_QPATH];

		appSprintf (ARRAY_ARG(expanded), "maps/%s.bsp", map);
		if (!FS_FileExists (expanded))
		{
			Com_WPrintf ("Can't find %s\n", expanded);
			return;
		}
	}

	sv.state = ss_dead;		// don't save current level when changing
	SV_WipeSavegame ("current");
	SV_GameMap_f (false, argc, argv);
}

/*
=====================================================================

  SAVEGAMES

=====================================================================
*/


/*
==============
SV_Loadgame_f

==============
*/
static void SV_Loadgame_f (bool usage, int argc, char **argv)
{
	char	name[MAX_OSPATH];
	FILE	*f;
	char	*dir;

	if (argc != 2 || usage)
	{
		Com_Printf ("Usage: loadgame <directory>\n");
		return;
	}

	Com_Printf ("Loading game...\n");

	dir = argv[1];
	if (strstr (dir, "..") || strchr (dir, '/') || strchr (dir, '\\'))
	{
		Com_WPrintf ("Bad savedir\n");
		return;
	}

	// make sure the server.ssv file exists
	appSprintf (ARRAY_ARG(name), "%s/" SAVEGAME_DIRECTORY "/%s/server." SAVEGAME_VARS_EXTENSION, FS_Gamedir(), argv[1]);
	f = fopen (name, "rb");
	if (!f)
	{
		Com_WPrintf ("No such savegame: %s\n", name);
		return;
	}
	fclose (f);

	SV_CopySaveGame (argv[1], "current");

	SV_ReadServerFile ();

	// go to the map
	sv.state = ss_dead;		// don't save current level when changing
	SV_Map (false, svs.mapcmd, true);
}



/*
==============
SV_Savegame_f

==============
*/
static void SV_Savegame_f (bool usage, int argc, char **argv)
{
	char	*dir;

	if (argc != 2 || usage)
	{
		Com_Printf ("Usage: savegame <directory>\n");
		return;
	}

	if (sv.state != ss_game)
	{
		Com_WPrintf ("You must be in a game to save.\n");
		return;
	}

	if (Cvar_VariableInt ("deathmatch"))
	{
		Com_WPrintf ("Can't savegame in a deathmatch\n");
		return;
	}

	if (!strcmp (argv[1], "current"))
	{
		Com_WPrintf ("Can't save to 'current'\n");
		return;
	}

	if (maxclients->integer == 1 && svs.clients[0].edict->client->ps.stats[STAT_HEALTH] <= 0)
	{
		Com_WPrintf ("\nCan't savegame while dead!\n");
		return;
	}

	dir = argv[1];
	if (strstr (dir, "..") || strchr (dir, '/') || strchr (dir, '\\'))
	{
		Com_WPrintf ("Bad savedir\n");
		return;
	}

	Com_Printf ("Saving game...\n");

	// archive current level, including all client edicts.
	// when the level is reloaded, they will be shells awaiting
	// a connecting client
	SV_WriteLevelFile ();
	// copy it off
	SV_CopySaveGame ("current", dir);

	// save server state
	SV_WriteServerFile (false, dir);

	Com_Printf ("Done.\n");
}


//===============================================================

static bool SetPlayer (char *s)
{
	client_t *cl;
	int		i, idnum;

	if (sv.attractloop)
		return false;
	// numeric values are just slot numbers
	if (s[0] >= '0' && s[0] <= '9')
	{
		idnum = atoi (s);
		if (idnum >= maxclients->integer)		// negative values are impossible here
		{
			Com_WPrintf ("Bad client slot: %d\n", idnum);
			return false;
		}

		sv_client = &svs.clients[idnum];
		sv_player = sv_client->edict;
		if (!sv_client->state)
		{
			Com_WPrintf ("Client %d is not active\n", idnum);
			return false;
		}
		return true;
	}

	// check for a name match
	for (i = 0, cl = svs.clients; i < maxclients->integer; i++, cl++)
	{
		if (!cl->state) continue;
		if (!stricmp (cl->name, s))
		{
			sv_client = cl;
			sv_player = sv_client->edict;
			return true;
		}
	}

	Com_WPrintf ("Userid %s is not on the server\n", s);
	return false;
}


/*
==================
SV_Kick_f

Kick a user off of the server
==================
*/
static void SV_Kick_f (bool usage, int argc, char **argv)
{
	if (argc != 2 || usage)
	{
		Com_Printf ("Usage: kick <userid>\n");
		return;
	}

	if (!svs.initialized)
	{
		Com_WPrintf ("No server running.\n");
		return;
	}

	if (!SetPlayer (argv[1]))
		return;

	if (sv_client->netchan.remote_address.type == NA_LOOPBACK)
	{
		Com_WPrintf ("Cannot kick host player\n");
		return;
	}

	SV_DropClient (sv_client, "was kicked");
	sv_client->lastmessage = svs.realtime;			// min case there is a funny zombie
}


#define MAX_BAN_LIST	256

static int banCount;
static char banlist[40][MAX_BAN_LIST];				// longest ban string is "NNN-NNN" * 4 + '.' * 3 + #0 -> 32 bytes


static int FindBanString (char *str)
{
	int		i;

	for (i = 0; i < banCount; i++)
		if (!strcmp (banlist[i], str))
			return i;
	return -1;
}


static void AddBanString (char *str)
{
	if (FindBanString (str) >= 0)
	{
		Com_WPrintf ("%s already banned\n", str);
		return;
	}
	if (banCount >= MAX_BAN_LIST)
	{
		Com_WPrintf ("Cannot ban %s: ban list full\n", str);
		return;
	}
	Com_DPrintf ("Add %s to banlist\n", str);
	strcpy (banlist[banCount++], str);
}


bool SV_AddressBanned (netadr_t *a)
{
	int		i;

	if (a->type != NA_IP) return false;				// non-IP address
	for (i = 0; i < banCount; i++)
		if (IPWildcard (a, banlist[i]))
			return true;							// banned
	return false;
}


static void SV_Ban_f (bool usage, int argc, char **argv)
{
	char	ban[40];
	byte	*ip;

	if (argc != 2 || usage)
	{
		Com_Printf ("Usage: ban <userid>\n");
		return;
	}

	if (!svs.initialized)		// ban by userid requires user to be connected, so -- server must be launched
	{
		Com_WPrintf ("No server running.\n");
		return;
	}

	if (!SetPlayer (argv[1]))
		return;

	if (sv_client->netchan.remote_address.type != NA_IP)
	{
		Com_WPrintf ("Cannot ban non-IP client\n");
		return;
	}

	// add to ban list
	ip = sv_client->netchan.remote_address.ip;
	appSprintf (ARRAY_ARG(ban), "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
	AddBanString (ban);

	SV_DropClient (sv_client, "was banned");
	sv_client->lastmessage = svs.realtime;		// min case there is a funny zombie
}


static void SV_BanIP_f (bool usage, int argc, char **argv)
{
	char	*str;
	client_t *cl;
	int		i;

	// can modify banlist before server launched
	if (argc != 2 || usage)
	{
		Com_Printf ("Usage: banip <ip-mask>\n");
		return;
	}

	str = argv[1];
	if (strlen (str) > 32)
	{
		Com_WPrintf ("IP-mask is too long\n");
		return;
	}
	AddBanString (str);

	if (!svs.initialized) return;				// server is not running -- just remember IP for future

	// find client with banned IP
	sv_client = NULL;
	for (i = 0, cl = svs.clients; i < maxclients->integer; i++, cl++)
	{
		if (!cl->state) continue;
		if (!IPWildcard (&cl->netchan.remote_address, str)) continue;

		SV_DropClient (cl, "was banned");
		cl->lastmessage = svs.realtime;			// min case there is a funny zombie
	}
}


static void SV_BanList_f (void)
{
	int		i;

	Com_Printf ("Banned IP's:\n");
	for (i = 0; i < banCount; i++)
		Com_Printf ("%-2d: %s\n", i, banlist[i]);
}


static void SV_BanRemove_f (bool usage, int argc, char **argv)
{
	char	*str;
	int		n;

	if (argc != 2 || usage)
	{
		Com_Printf ("Usage: banremove <list-index or ip-mask>\n");
		return;
	}
	str = argv[1];
	if (strchr (str, '.'))
	{
		n = FindBanString (str);
		if (n < 0)
		{
			Com_WPrintf ("Mask %s is not in ban list\n", str);
			return;
		}
	}
	else
	{
		if (str[0] < '0' || str[0] > '9')
		{
			Com_WPrintf ("Bad list index %d\n", str);
			return;
		}
		n = atoi (str);
		if (n >= banCount)		// do not need "< 0" check: if number have "-" - will not get here
		{
			Com_WPrintf ("Index %d is out of list bounds\n", n);
			return;
        }
	}
	if (n < banCount - 1)
		memcpy (banlist[n], banlist[n+1], sizeof(banlist[0]) * (banCount - n - 1));
	banCount--;
}


/*
===========
SV_DumpUser_f

Examine all a users info strings
===========
*/
static void SV_DumpUser_f (bool usage, int argc, char **argv)
{
	if (argc != 2 || usage)
	{
		Com_Printf ("Usage: dumpuser <userid>\n");
		return;
	}

	if (!SetPlayer (argv[1]))
		return;

	Com_Printf (S_GREEN"userinfo\n--------\n");
	Info_Print (sv_client->userinfo);
}


/*
================
SV_Status_f
================
*/
static void SV_Status_f (void)
{
	int		i;
	client_t *cl;

	if (!svs.clients)
	{
		Com_WPrintf ("No server running.\n");
		return;
	}
	Com_Printf ("map: %s\n", sv.name);

	Com_Printf ("--n-score-ping-name------------lastmsg-address---------------qport-\n");
	for (i = 0, cl = svs.clients; i < maxclients->integer; i++,cl++)
	{
		if (!cl->state) continue;
		if (!sv.attractloop)
			Com_Printf ("%3d %5d ", i, cl->edict->client->ps.stats[STAT_FRAGS]);
		else	// cannot access edict for demo client
			Com_Printf ("%3d "S_RED"----- "S_WHITE, i);

		if (cl->state == cs_connected)
			Com_Printf ("CNCT ");
		else if (cl->state == cs_zombie)
			Com_Printf (S_RED"ZMBI "S_WHITE);
		else
			Com_Printf ("%4d ", min (cl->ping, 9999));

		Com_Printf ("%-15s %7d %-21s %5d\n", cl->name, svs.realtime - cl->lastmessage,
			NET_AdrToString (&cl->netchan.remote_address), cl->netchan.qport);
	}
	Com_Printf ("\n");
}

/*
==================
SV_ConSay_f
==================
*/
static void SV_ConSay_f (int argc, char **argv)
{
	int		i;
	client_t *client;
	char	text[1024];

	if (argc < 2) return;

	strcpy (text, "console:");
	for (i = 1; i < argc; i++)
		appStrcatn (ARRAY_ARG(text), va(" %s", argv[i]));

	for (i = 0, client = svs.clients; i < maxclients->integer; i++, client++)
	{
		if (client->state != cs_spawned)
			continue;
		SV_ClientPrintf (client, PRINT_CHAT, "%s\n", text);
	}
}


/*
==================
SV_Heartbeat_f
==================
*/
static void SV_Heartbeat_f (void)
{
	svs.last_heartbeat = -BIG_NUMBER;
}


/*
===========
SV_Serverinfo_f

  Examine or change the serverinfo string
===========
*/
static void SV_Serverinfo_f (void)
{
	Com_Printf (S_GREEN"------ Server info settings ------\n");
	Info_Print (Cvar_Serverinfo());
}


/*
==============
SV_ServerRecord_f

Begins server demo recording.  Every entity and every message will be
recorded, but no playerinfo will be stored.  Primarily for demo merging.
==============
*/
static void SV_ServerRecord_f (bool usage, int argc, char **argv)
{
	char	name[MAX_OSPATH], buf_data[32768];
	int		len, i;
	sizebuf_t buf;

	if (argc != 2 || usage)
	{
		Com_Printf ("Usage: serverrecord <demoname>\n");
		return;
	}

	if (svs.demofile)
	{
		Com_WPrintf ("Already recording\n");
		return;
	}

	if (sv.state != ss_game)
	{
		Com_WPrintf ("You must be in a level to record\n");
		return;
	}

	// open the demo file
	appSprintf (ARRAY_ARG(name), "%s/demos/%s.dm2", FS_Gamedir(), argv[1]);

	Com_Printf ("recording to %s\n", name);
	FS_CreatePath (name);
	svs.demofile = fopen (name, "wb");
	if (!svs.demofile)
	{
		Com_WPrintf ("ERROR: couldn't open\n");
		return;
	}

	// setup a buffer to catch all multicasts
	svs.demo_multicast.Init (ARRAY_ARG(svs.demo_multicast_buf));

	// write a single giant fake message with all the startup info
	buf.Init (ARRAY_ARG(buf_data));

	// serverdata needs to go over for all types of servers
	// to make sure the protocol is right, and to set the gamedir
	//
	// send the serverdata
	MSG_WriteByte (&buf, svc_serverdata);
	MSG_WriteLong (&buf, PROTOCOL_VERSION);
	MSG_WriteLong (&buf, svs.spawncount);
	// 2 means server demo
	MSG_WriteByte (&buf, 2);	// demos are always attract loops
	MSG_WriteString (&buf, Cvar_VariableString ("gamedir"));
	MSG_WriteShort (&buf, -1);
	// send full levelname
	MSG_WriteString (&buf, sv.configstrings[CS_NAME]);

	for (i = 0; i < MAX_CONFIGSTRINGS; i++)
		if (sv.configstrings[i][0])
		{
			MSG_WriteByte (&buf, svc_configstring);
			MSG_WriteShort (&buf, i);
			MSG_WriteString (&buf, sv.configstrings[i]);
		}

	// write it to the demo file
	len = LittleLong (buf.cursize);
	fwrite (&len, 4, 1, svs.demofile);
	fwrite (buf.data, buf.cursize, 1, svs.demofile);

	// the rest of the demo file will be individual frames
}


/*
==============
SV_ServerStop_f

Ends server demo recording
==============
*/
static void SV_ServerStop_f (void)
{
	if (!svs.demofile)
	{
		Com_Printf ("Not doing a serverrecord.\n");
		return;
	}
	fclose (svs.demofile);
	svs.demofile = NULL;
	Com_Printf ("Recording completed.\n");
}


/*
===============
SV_KillServer_f

Kick everyone off, possibly in preparation for a new game

===============
*/
static void SV_KillServer_f (void)
{
	if (!svs.initialized)
		return;
	SV_Shutdown ("Server was killed.\n", false);
	NET_Config (false);		// close network sockets
}

/*
===============
SV_ServerCommand_f

Let the game library handle a command
===============
*/
static void SV_ServerCommand_f (int argc, char **argv)
{
	if (!ge)
	{
		Com_WPrintf ("No game loaded.\n");
		return;
	}

	// pass command to server string tokenizer
	char buffer[1024];
	for (int i = 0; i < argc; i++)
	{
		if (i > 0) appStrcatn (ARRAY_ARG(buffer), " ");
		appStrcatn (ARRAY_ARG(buffer), argv[i]);
	}
	SV_TokenizeString (buffer);

	guard(ge.ServerCommand);
	ge->ServerCommand ();
	unguard;
}

//===========================================================

/*
==================
SV_InitOperatorCommands
==================
*/
void SV_InitOperatorCommands (void)
{
	RegisterCommand ("heartbeat", SV_Heartbeat_f);

	RegisterCommand ("kick", SV_Kick_f);
	RegisterCommand ("ban", SV_Ban_f);
	RegisterCommand ("banip", SV_BanIP_f);
	RegisterCommand ("banlist", SV_BanList_f);
	RegisterCommand ("banremove", SV_BanRemove_f);

	RegisterCommand ("status", SV_Status_f);
	RegisterCommand ("serverinfo", SV_Serverinfo_f);
	RegisterCommand ("dumpuser", SV_DumpUser_f);

	RegisterCommand ("map", SV_Map_f);
	RegisterCommand ("demomap", SV_DemoMap_f);
	RegisterCommand ("gamemap", SV_GameMap_f);
	RegisterCommand ("setmaster", SV_SetMaster_f);

	if (DEDICATED)
		RegisterCommand ("say", SV_ConSay_f);

	RegisterCommand ("serverrecord", SV_ServerRecord_f);
	RegisterCommand ("serverstop", SV_ServerStop_f);

	RegisterCommand ("save", SV_Savegame_f);
	RegisterCommand ("load", SV_Loadgame_f);

	RegisterCommand ("killserver", SV_KillServer_f);

	RegisterCommand ("sv", SV_ServerCommand_f);
}
