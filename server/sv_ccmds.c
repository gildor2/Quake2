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

//?? only for screenshot
#include "../client/ref.h"
extern refExport_t re;
void SCR_UpdateScreen (void);		//?? remove
void SCR_SetLevelshot (char *name);

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
void SV_SetMaster_f (void)
{
	int		i, slot;

	// only dedicated servers send heartbeats
	if (!dedicated->integer)
	{
		Com_Printf ("Only dedicated servers use masters.\n");
		return;
	}

	// make sure the server is listed public
	Cvar_Set ("public", "1");

	for (i = 1; i < MAX_MASTERS; i++)
		memset (&master_adr[i], 0, sizeof(master_adr[i]));

	slot = 1;		// slot 0 will always contain the id master
	for (i = 1; i < Cmd_Argc(); i++)
	{
		if (slot == MAX_MASTERS)
			break;

		if (!NET_StringToAdr (Cmd_Argv(i), &master_adr[i]))
		{
			Com_Printf ("Bad address: %s\n", Cmd_Argv(i));
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

	Com_sprintf (ARRAY_ARG(name), "%s/" SAVEGAME_DIRECTORY "/current/%s." SAVEGAME_SERVER_EXTENSION, FS_Gamedir(), sv.name);
	f = fopen (name, "wb");
	if (!f)
	{
		Com_WPrintf ("Failed to open %s\n", name);
		return;
	}
	fwrite (sv.configstrings, sizeof(sv.configstrings), 1, f);
	CM_WritePortalState (f);
	fclose (f);

	Com_sprintf (ARRAY_ARG(name), "%s/" SAVEGAME_DIRECTORY "/current/%s." SAVEGAME_GAME_EXTENSION, FS_Gamedir(), sv.name);
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

	Com_sprintf (ARRAY_ARG(name), "%s/" SAVEGAME_DIRECTORY "/current/%s." SAVEGAME_SERVER_EXTENSION, FS_Gamedir(), sv.name);
	f = fopen(name, "rb");
	if (!f)
	{
		Com_WPrintf ("Failed to open %s\n", name);
		return;
	}
	fread (sv.configstrings, sizeof(sv.configstrings), 1, f);
	CM_ReadPortalState (f);
	fclose (f);

	Com_sprintf (ARRAY_ARG(name), "%s/" SAVEGAME_DIRECTORY "/current/%s." SAVEGAME_GAME_EXTENSION, FS_Gamedir(), sv.name);
	ge->ReadLevel (name);
}

/*
==============
SV_WriteServerFile

==============
*/
void SV_WriteServerFile (qboolean autosave, char *dir)
{
	FILE	*f;
	cvar_t	*var;
	char	name[MAX_OSPATH], string[128];
	char	comment[128];
	time_t	aclock;
	struct tm	*newtime;

	Com_DPrintf("SV_WriteServerFile(%s, %s)\n", autosave ? "true" : "false", dir);

	Com_sprintf (ARRAY_ARG(name), "%s/" SAVEGAME_DIRECTORY "/%s/server." SAVEGAME_VARS_EXTENSION, FS_Gamedir(), dir);
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
		Com_sprintf (ARRAY_ARG(comment), "%2d:%d%d %2d/%2d  %s", newtime->tm_hour, newtime->tm_min/10,
			newtime->tm_min%10, newtime->tm_mon+1, newtime->tm_mday, sv.configstrings[CS_NAME]);
	}
	else
	{	// autosaved
		Com_sprintf (ARRAY_ARG(comment), "ENTERING %s", sv.configstrings[CS_NAME]);
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
	Com_sprintf (ARRAY_ARG(name), "%s/" SAVEGAME_DIRECTORY "/%s/game." SAVEGAME_VARS_EXTENSION, FS_Gamedir(), dir);
	ge->WriteGame (name, autosave);

	// perform screenshot
	if (re.Screenshot)					// can be uninitialized when dedicated server
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
	SCR_SetLevelshot ("/" SAVEGAME_DIRECTORY "/current/shot.pcx");

	Com_sprintf (ARRAY_ARG(name), "%s/" SAVEGAME_DIRECTORY "/current/server." SAVEGAME_VARS_EXTENSION, FS_Gamedir());
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
	Com_sprintf (ARRAY_ARG(name), "%s/" SAVEGAME_DIRECTORY "/current/game." SAVEGAME_VARS_EXTENSION, FS_Gamedir());
	ge->ReadGame (name);
}


//=========================================================




/*
==================
SV_DemoMap_f

Puts the server in demo mode on a specific map/cinematic
==================
*/
void SV_DemoMap_f (void)
{
	char	*map, *ext;

	map = Cmd_Argv(1);
	ext = strchr (map, '.');

	if (Cvar_VariableInt ("nointro") == 0 && (!ext || (strcmp (ext, ".cin") && strcmp (ext, ".pcx"))))
	{
		// allow .CIN files while playing intro (stop intro when trying to play demofile)
		Cvar_ForceSet ("nointro", "1");
		Cbuf_AddText ("nextserver=\"\";disconnect\n");
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
void SV_GameMap_f (void)
{
	char		*map;
	int			i;
	client_t	*cl;

	if (Cmd_Argc() != 2)
	{
		Com_Printf ("Usage: gamemap <map>\n");
		return;
	}

	map = Cmd_Argv(1);
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
			qboolean	*savedInuse;

			FS_CreatePath (va("%s/" SAVEGAME_DIRECTORY "/current/", FS_Gamedir()));
			// clear all the client.inuse flags before saving so that
			// when the level is re-entered, the clients will spawn
			// at spawn points instead of occupying body shells
			savedInuse = Z_Malloc (maxclients->integer * sizeof(qboolean));
			for (i = 0, cl = svs.clients; i < maxclients->integer; i++, cl++)
			{
				savedInuse[i] = cl->edict->inuse;
				cl->edict->inuse = false;
			}

			SV_WriteLevelFile ();

			// we must restore these for clients to transfer over correctly
			for (i = 0, cl = svs.clients; i < maxclients->integer; i++, cl++)
				cl->edict->inuse = savedInuse[i];
			Z_Free (savedInuse);
		}
	}

	// start up the next map
	SV_Map (false, map, false );

	// archive server state
	Q_strncpyz (svs.mapcmd, map, sizeof(svs.mapcmd));

	// copy off the level to the autosave slot
	if (!dedicated->integer && !Cvar_VariableInt ("deathmatch"))
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
void SV_Map_f (void)
{
	char	*map;

	// if not a pcx, demo, or cinematic, check to make sure the level exists
	map = Cmd_Argv(1);
	if (!strchr (map, '.'))
	{
		char	expanded[MAX_QPATH];

		Com_sprintf (ARRAY_ARG(expanded), "maps/%s.bsp", map);
		if (!FS_FileExists (expanded))
		{
			Com_WPrintf ("Can't find %s\n", expanded);
			return;
		}
	}

	sv.state = ss_dead;		// don't save current level when changing
	SV_WipeSavegame ("current");
	SV_GameMap_f ();
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
void SV_Loadgame_f (void)
{
	char	name[MAX_OSPATH];
	FILE	*f;
	char	*dir;

	if (Cmd_Argc() != 2)
	{
		Com_Printf ("Usage: loadgame <directory>\n");
		return;
	}

	Com_Printf ("Loading game...\n");

	dir = Cmd_Argv(1);
	if (strstr (dir, "..") || strchr (dir, '/') || strchr (dir, '\\'))
	{
		Com_WPrintf ("Bad savedir\n");
		return;
	}

	// make sure the server.ssv file exists
	Com_sprintf (ARRAY_ARG(name), "%s/" SAVEGAME_DIRECTORY "/%s/server." SAVEGAME_VARS_EXTENSION, FS_Gamedir(), Cmd_Argv(1));
	f = fopen (name, "rb");
	if (!f)
	{
		Com_WPrintf ("No such savegame: %s\n", name);
		return;
	}
	fclose (f);

	SV_CopySaveGame (Cmd_Argv(1), "current");

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
void SV_Savegame_f (void)
{
	char	*dir;

	if (sv.state != ss_game)
	{
		Com_Printf ("You must be in a game to save.\n");
		return;
	}

	if (Cmd_Argc() != 2)
	{
		Com_Printf ("Usage: savegame <directory>\n");
		return;
	}

	if (Cvar_VariableInt ("deathmatch"))
	{
		Com_WPrintf ("Can't savegame in a deathmatch\n");
		return;
	}

	if (!strcmp (Cmd_Argv(1), "current"))
	{
		Com_WPrintf ("Can't save to 'current'\n");
		return;
	}

	if (maxclients->integer == 1 && svs.clients[0].edict->client->ps.stats[STAT_HEALTH] <= 0)
	{
		Com_WPrintf ("\nCan't savegame while dead!\n");
		return;
	}

	dir = Cmd_Argv(1);
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

static qboolean SetPlayer (char *s)
{
	client_t *cl;
	int		i, idnum;

	// numeric values are just slot numbers
	if (s[0] >= '0' && s[0] <= '9')
	{
		idnum = atoi (Cmd_Argv(1));
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
		if (!cl->state)
			continue;
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
static void SV_Kick_f (void)
{
	if (!svs.initialized)
	{
		Com_WPrintf ("No server running.\n");
		return;
	}

	if (Cmd_Argc() != 2)
	{
		Com_Printf ("Usage: kick <userid>\n");
		return;
	}

	if (!SetPlayer (Cmd_Argv(1)))
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


qboolean SV_AddressBanned (netadr_t *a)
{
	int		i;

	if (a->type != NA_IP) return false;				// non-IP address
	for (i = 0; i < banCount; i++)
		if (IPWildcard (a, banlist[i]))
			return true;							// banned
	return false;
}


static void SV_Ban_f (void)
{
	char	ban[40];
	byte	*ip;

	if (!svs.initialized)		// ban by userid requires user to be connected, so -- server must be launched
	{
		Com_WPrintf ("No server running.\n");
		return;
	}

	if (Cmd_Argc() != 2)
	{
		Com_Printf ("Usage: ban <userid>\n");
		return;
	}

	if (!SetPlayer (Cmd_Argv(1)))
		return;

	if (sv_client->netchan.remote_address.type != NA_IP)
	{
		Com_WPrintf ("Cannot ban non-IP client\n");
		return;
	}

	// add to ban list
	ip = sv_client->netchan.remote_address.ip;
	Com_sprintf (ARRAY_ARG(ban), "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
	AddBanString (ban);

	SV_DropClient (sv_client, "was banned");
	sv_client->lastmessage = svs.realtime;		// min case there is a funny zombie
}


static void SV_BanIP_f (void)
{
	char	*str;
	client_t *cl;
	int		i;

	// can modify banlist before server launched
	if (Cmd_Argc() != 2)
	{
		Com_Printf ("Usage: banip <ip-mask>\n");
		return;
	}

	str = Cmd_Argv(1);
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


static void SV_BanRemove_f (void)
{
	char	*str;
	int		n;

	if (Cmd_Argc() != 2)
	{
		Com_Printf ("Usage: banremove <list-index or ip-mask>\n");
		return;
	}
	str = Cmd_Argv(1);
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
		n = atoi(str);
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
static void SV_DumpUser_f (void)
{
	if (Cmd_Argc() != 2)
	{
		Com_Printf ("Usage: info <userid>\n");
		return;
	}

	if (!SetPlayer (Cmd_Argv(1)))
		return;

	Com_Printf ("userinfo\n");
	Com_Printf ("--------\n");
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

	Com_Printf ("num score ping name            lastmsg address               qport \n");
	Com_Printf ("--- ----- ---- --------------- ------- --------------------- ------\n");
	for (i = 0, cl = svs.clients; i < maxclients->integer; i++,cl++)
	{
		if (!cl->state)
			continue;
		Com_Printf ("%3d %5d ", i, cl->edict->client->ps.stats[STAT_FRAGS]);

		if (cl->state == cs_connected)
			Com_Printf ("CNCT ");
		else if (cl->state == cs_zombie)
			Com_Printf ("^1ZMBI ");
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
static void SV_ConSay_f (void)
{
	client_t *client;
	int		j;
	char	*p;
	char	text[1024];

	if (Cmd_Argc () < 2)
		return;

	strcpy (text, "console: ");
	p = Cmd_Args();

	if (*p == '"')
	{
		p++;
		p[strlen(p)-1] = 0;
	}

	strcat(text, p);

	for (j = 0, client = svs.clients; j < maxclients->integer; j++, client++)
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
	Com_Printf ("Server info settings:\n");
	Info_Print (Cvar_Serverinfo());
}


/*
==============
SV_ServerRecord_f

Begins server demo recording.  Every entity and every message will be
recorded, but no playerinfo will be stored.  Primarily for demo merging.
==============
*/
static void SV_ServerRecord_f (void)
{
	char	name[MAX_OSPATH], buf_data[32768];
	int		len, i;
	sizebuf_t buf;

	if (Cmd_Argc() != 2)
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
	Com_sprintf (ARRAY_ARG(name), "%s/demos/%s.dm2", FS_Gamedir(), Cmd_Argv(1));

	Com_Printf ("recording to %s\n", name);
	FS_CreatePath (name);
	svs.demofile = fopen (name, "wb");
	if (!svs.demofile)
	{
		Com_WPrintf ("ERROR: couldn't open\n");
		return;
	}

	// setup a buffer to catch all multicasts
	SZ_Init (&svs.demo_multicast, svs.demo_multicast_buf, sizeof(svs.demo_multicast_buf));

	// write a single giant fake message with all the startup info
	SZ_Init (&buf, buf_data, sizeof(buf_data));

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
	Com_DPrintf ("signon message length: %i\n", buf.cursize);
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
	NET_Config ( false );	// close network sockets
}

/*
===============
SV_ServerCommand_f

Let the game dll handle a command
===============
*/
static void SV_ServerCommand_f (void)
{
	if (!ge)
	{
		Com_Printf ("No game loaded.\n");
		return;
	}

	ge->ServerCommand();
}

//===========================================================

/*
==================
SV_InitOperatorCommands
==================
*/
void SV_InitOperatorCommands (void)
{
	Cmd_AddCommand ("heartbeat", SV_Heartbeat_f);

	Cmd_AddCommand ("kick", SV_Kick_f);
	Cmd_AddCommand ("ban", SV_Ban_f);
	Cmd_AddCommand ("banip", SV_BanIP_f);
	Cmd_AddCommand ("banlist", SV_BanList_f);
	Cmd_AddCommand ("banremove", SV_BanRemove_f);

	Cmd_AddCommand ("status", SV_Status_f);
	Cmd_AddCommand ("serverinfo", SV_Serverinfo_f);
	Cmd_AddCommand ("dumpuser", SV_DumpUser_f);

	Cmd_AddCommand ("map", SV_Map_f);
	Cmd_AddCommand ("demomap", SV_DemoMap_f);
	Cmd_AddCommand ("gamemap", SV_GameMap_f);
	Cmd_AddCommand ("setmaster", SV_SetMaster_f);

	if (dedicated->integer)
		Cmd_AddCommand ("say", SV_ConSay_f);

	Cmd_AddCommand ("serverrecord", SV_ServerRecord_f);
	Cmd_AddCommand ("serverstop", SV_ServerStop_f);

	Cmd_AddCommand ("save", SV_Savegame_f);
	Cmd_AddCommand ("load", SV_Loadgame_f);

	Cmd_AddCommand ("killserver", SV_KillServer_f);

	Cmd_AddCommand ("sv", SV_ServerCommand_f);
}
