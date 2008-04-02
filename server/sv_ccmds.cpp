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

void SCR_SetLevelshot(const char *name);	//?? implement as message to local client (require message system)


/*
====================
SV_SetMaster_f

Specify a list of master servers
====================
*/
static void SV_SetMaster_f(int argc, char **argv)
{
	int		i, slot;

	// only dedicated servers send heartbeats
	if (!DEDICATED)
	{
		appPrintf("Only dedicated servers use masters.\n");
		return;
	}

	// make sure the server is listed public
	Cvar_Set("public", "1");

	for (i = 1; i < MAX_MASTERS; i++)
		memset(&master_adr[i], 0, sizeof(master_adr[i]));

	slot = 1;		// slot 0 will always contain the id master
	for (i = 1; i < argc; i++)
	{
		if (slot == MAX_MASTERS)
			break;

		if (!NET_StringToAdr(argv[i], &master_adr[slot], PORT_MASTER))
		{
			appPrintf("Bad address: %s\n", argv[i]);
			continue;
		}
		appPrintf("Master server at %s\n", NET_AdrToString(&master_adr[slot]));
		appPrintf("Sending a ping.\n");

		Netchan_OutOfBandPrint(NS_SERVER, master_adr[slot], "ping");

		slot++;
	}

	svs.last_heartbeat = -BIG_NUMBER;
}



/*-----------------------------------------------------------------------------
	Savegames
-----------------------------------------------------------------------------*/

static void SV_WipeSavegame(const char *savename)
{
	Com_DPrintf("SV_WipeSaveGame(%s)\n", savename);
	FS_RemoveFiles(va("./%s/" SAVEGAME_DIRECTORY "/%s/*", FS_Gamedir(), savename));
}


static void SV_CopySaveGame(const char *src, const char *dst)
{
	Com_DPrintf("SV_CopySaveGame(%s, %s)\n", src, dst);
	SV_WipeSavegame(dst);
	FS_CopyFiles(va("./%s/" SAVEGAME_DIRECTORY "/%s/*", FS_Gamedir(), src),
				  va("./%s/" SAVEGAME_DIRECTORY "/%s/", FS_Gamedir(), dst));
}


static void SV_WriteLevelFile()
{
	Com_DPrintf("SV_WriteLevelFile()\n");

	TString<MAX_OSPATH> Name;
	Name.sprintf("./%s/" SAVEGAME_DIRECTORY "/current/%s." SAVEGAME_SERVER_EXTENSION, FS_Gamedir(), sv.name);
	FILE *f = fopen(Name, "wb");
	if (!f)
	{
		appWPrintf("Failed to open %s\n", *Name);
		return;
	}
	fwrite(sv.configstrings, sizeof(sv.configstrings), 1, f);
	CM_WritePortalState(f);
	fclose(f);

	Name.sprintf("./%s/" SAVEGAME_DIRECTORY "/current/%s." SAVEGAME_GAME_EXTENSION, FS_Gamedir(), sv.name);

	guardGame(ge.WriteLevel);
	ge->WriteLevel(Name);
	unguardGame;
}


void SV_ReadLevelFile()
{
	Com_DPrintf("SV_ReadLevelFile()\n");

	TString<MAX_OSPATH> Name;
	Name.sprintf("./%s/" SAVEGAME_DIRECTORY "/current/%s." SAVEGAME_SERVER_EXTENSION, FS_Gamedir(), sv.name);
	FILE *f = fopen(Name, "rb");
	if (!f)
	{
		appWPrintf("Failed to open %s\n", *Name);
		return;
	}
	fread(sv.configstrings, sizeof(sv.configstrings), 1, f);
	CM_ReadPortalState(f);
	fclose(f);

	Name.sprintf("./%s/" SAVEGAME_DIRECTORY "/current/%s." SAVEGAME_GAME_EXTENSION, FS_Gamedir(), sv.name);

	guardGame(ge.ReadLevel)
	ge->ReadLevel(Name);
	unguardGame;
}


static void SV_WriteServerFile(bool autosave, const char *dir)
{
	Com_DPrintf("SV_WriteServerFile(%s, %s)\n", autosave ? "true" : "false", dir);

	TString<MAX_OSPATH> Name;
	Name.sprintf("./%s/" SAVEGAME_DIRECTORY "/%s/server." SAVEGAME_VARS_EXTENSION, FS_Gamedir(), dir);
	FILE *f = fopen(Name, "wb");
	if (!f)
	{
		appWPrintf("Couldn't write %s\n", *Name);
		return;
	}
	// write the comment field
	char comment[128];		// used 32 chars
	memset(comment, 0, sizeof(comment));
	if (!autosave)
		appSprintf(ARRAY_ARG(comment), "%s  %s", appTimestamp(), sv.configstrings[CS_NAME]);
	else
		appSprintf(ARRAY_ARG(comment), "ENTERING %s", sv.configstrings[CS_NAME]);	// autosaved
	fwrite(comment, 32, 1, f);

	// write the mapcmd
	fwrite(svs.mapcmd, 1, sizeof(svs.mapcmd), f);

	// write all CVAR_LATCH cvars
	// these will be things like coop, skill, deathmatch, etc
	for (cvar_t *var = cvar_t::vars; var ; var=var->next)
	{
		if (!(var->flags & CVAR_LATCH))
			continue;
		char name[128], string[128];
		if (strlen(var->name) >= sizeof(name)-1 || strlen(var->string) >= sizeof(string)-1)
		{
			appWPrintf("Cvar too long: %s = %s\n", var->name, var->string);
			continue;
		}
		memset(name, 0, sizeof(name));
		memset(string, 0, sizeof(string));
		strcpy(name, var->name);
		strcpy(string, var->string);
		fwrite(name, 1, sizeof(name), f);
		fwrite(string, 1, sizeof(string), f);
	}

	fclose(f);

	// write game state
	Name.sprintf("./%s/" SAVEGAME_DIRECTORY "/%s/game." SAVEGAME_VARS_EXTENSION, FS_Gamedir(), dir);

	guardGame(ge.WriteGame);
	ge->WriteGame(Name, autosave);
	unguardGame;

	// perform screenshot
	if (!DEDICATED)					// can be uninitialized when dedicated server
	{
		RE_Screenshot(SHOT_SMALL|SHOT_NOGAMMA|SHOT_SILENT|SHOT_NO_2D|SHOT_WAIT_3D|SHOT_JPEG,
			va("./%s/" SAVEGAME_DIRECTORY "/%s/shot", FS_Gamedir(), dir));
//--		SCR_UpdateScreen();			// required for screenshot performing
			// BUT: here we must wait for NEXT map, and if performing shot immediately, it can be from PREVIOUS map
	}
}


static void SV_ReadServerFile()
{
	char	comment[32], mapcmd[128];

	Com_DPrintf("SV_ReadServerFile()\n");
	if (!DEDICATED) SCR_SetLevelshot(SAVEGAME_DIRECTORY "/current/shot");

	TString<MAX_OSPATH> Name;
	Name.sprintf("./%s/" SAVEGAME_DIRECTORY "/current/server." SAVEGAME_VARS_EXTENSION, FS_Gamedir());
	FILE *f = fopen(Name, "rb");
	if (!f)
	{
		appWPrintf("Couldn't read %s\n", *Name);
		return;
	}

	// read the comment field
	fread(comment, sizeof(comment), 1, f);
	// read the mapcmd
	fread(mapcmd, sizeof(mapcmd), 1, f);

	// read all CVAR_LATCH cvars
	// these will be things like coop, skill, deathmatch, etc
	while (true)
	{
		char name[128], string[128];
		if (!fread(name, 1, sizeof(name), f)) break;
		fread(string, sizeof(string), 1, f);
		Com_DPrintf("Set %s = %s\n", name, string);
		Cvar_ForceSet(name, string);
	}

	fclose(f);

	// start a new game fresh with new cvars
	SV_InitGame();

	strcpy(svs.mapcmd, mapcmd);

	// read game state
	Name.sprintf("./%s/" SAVEGAME_DIRECTORY "/current/game." SAVEGAME_VARS_EXTENSION, FS_Gamedir());

	guardGame(ge.ReadGame);
	ge->ReadGame(Name);
	unguardGame;
}


//------ console command ---------

static void SV_Loadgame_f(bool usage, int argc, char **argv)
{
	if (argc != 2 || usage)
	{
		appPrintf("Usage: loadgame <directory>\n");
		return;
	}

	appPrintf("Loading game...\n");

	const char *dir = argv[1];
	if (strstr(dir, "..") || strchr(dir, '/') || strchr(dir, '\\'))
	{
		appWPrintf("Bad savedir\n");
		return;
	}

	// make sure the server.ssv file exists
	TString<MAX_OSPATH> Name;
	Name.sprintf("./%s/" SAVEGAME_DIRECTORY "/%s/server." SAVEGAME_VARS_EXTENSION, FS_Gamedir(), argv[1]);
	FILE *f = fopen(Name, "rb");
	if (!f)
	{
		appWPrintf("No such savegame: %s\n", *Name);
		return;
	}
	fclose(f);

	SV_CopySaveGame(argv[1], "current");

	SV_ReadServerFile();

	// go to the map
	sv.state = ss_dead;		// don't save current level when changing
	SV_Map(false, svs.mapcmd, true);
}



static void SV_Savegame_f(bool usage, int argc, char **argv)
{
	if (argc != 2 || usage)
	{
		appPrintf("Usage: savegame <directory>\n");
		return;
	}

	if (sv.state != ss_game)
	{
		appWPrintf("You must be in a game to save.\n");
		return;
	}

	if (sv_deathmatch->integer)
	{
		appWPrintf("Can't savegame in a deathmatch\n");
		return;
	}

	if (!strcmp(argv[1], "current"))
	{
		appWPrintf("Can't save to 'current'\n");
		return;
	}

	if (svs.clients[0].edict->client->ps.stats[STAT_HEALTH] <= 0)
	{
		appWPrintf("Can't savegame while dead!\n");
		return;
	}

	const char *dir = argv[1];
	if (strstr(dir, "..") || strchr(dir, '/') || strchr(dir, '\\'))
	{
		appWPrintf("Bad savedir\n");
		return;
	}

	appPrintf("Saving game...\n");

	// archive current level, including all client edicts.
	// when the level is reloaded, they will be shells awaiting
	// a connecting client
	SV_WriteLevelFile();
	// copy it off
	SV_CopySaveGame("current", dir);

	// save server state
	SV_WriteServerFile(false, dir);

	appPrintf("Done.\n");
}


/*-----------------------------------------------------------------------------
	Starting a new map
-----------------------------------------------------------------------------*/

// Puts the server in demo mode on a specific map/cinematic
static void SV_DemoMap_f(int argc, char **argv)
{
	TString<128> Map; Map = argv[1];
	const char *ext = Map.chr('.');

	if (Cvar_VariableInt("nointro") == 0 && (!ext || !(stricmp(ext, ".dm2"))))
	{
		// allow .CIN files while playing intro (stop intro when trying to play demofile)
		Cvar_ForceSet("nointro", "1");
		Cbuf_AddText("nextserver=\"\"\n");
		if (sv.state) Cbuf_AddText("disconnect\n");
		return;
	}

	if (!ext) Map += ".dm2";

	SV_Map(true, Map, false);
}


/*
Saves the state of the map just being exited and goes to a new map.

If the initial character of the map string is '*', the next map is
in a new unit, so the current savegame directory is cleared of
map files.

Example:

*inter.cin+jail

Clears the archived maps, plays the inter.cin cinematic, then
goes to map jail.bsp.
*/
static void SV_GameMap_f(bool usage, int argc, char **argv)
{
	if (argc != 2 || usage)
	{
		appPrintf("Usage: gamemap <map>\n");
		return;
	}

	TString<128> Map; Map = argv[1];
	Com_DPrintf("SV_GameMap(%s)\n", *Map);

	// check for clearing the current savegame
	if (Map[0] == '*')
	{
		// wipe all the *.sav files
		SV_WipeSavegame("current");
	}
	else
	{	// save the map just exited
		if (sv.state == ss_game)
		{
			appMakeDirectory(va("./%s/" SAVEGAME_DIRECTORY "/current", FS_Gamedir()));

			// clear all the client.inuse flags before saving so that
			// when the level is re-entered, the clients will spawn
			// at spawn points instead of occupying body shells
			int		i;
			client_t *cl;
			bool	savedInUse[MAX_CLIENTS];
			for (i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++)
			{
				savedInUse[i] = cl->edict->inuse != 0;
				cl->edict->inuse = false;
			}

			SV_WriteLevelFile();

			// we must restore these for clients to transfer over correctly
			for (i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++)
				cl->edict->inuse = savedInUse[i];
		}
	}

	// start up the next map
	SV_Map(false, Map, false );

	// archive server state
	appStrncpyz(svs.mapcmd, Map, sizeof(svs.mapcmd));

	// copy off the level to the autosave slot
	if (!DEDICATED && !sv_deathmatch->integer)
	{
		appMakeDirectory(va("./%s/" SAVEGAME_DIRECTORY "/current", FS_Gamedir()));
		SV_CopySaveGame("current", "save0");
		SV_WriteServerFile(true, "save0");
	}
}


// Goes directly to a given map without any savegame archiving. For development work
static void SV_Map_f(int argc, char **argv)
{
	// if not a pcx, demo, or cinematic, check to make sure the level exists
	const char *map = argv[1];
	if (!strchr(map, '.'))
	{
		TString<MAX_QPATH> Expanded;
		Expanded.sprintf("maps/%s.bsp", map);
		if (!GFileSystem->FileExists(Expanded))
		{
			appWPrintf("Can't find %s\n", *Expanded);
			return;
		}
	}

	sv.state = ss_dead;		// don't save current level when changing
	SV_WipeSavegame("current");
	SV_GameMap_f(false, argc, argv);
}



/*-----------------------------------------------------------------------------
	Client manipulations
-----------------------------------------------------------------------------*/

static bool SetPlayer(const char *s)
{
	if (sv.attractloop)
		return false;
	// numeric values are just slot numbers
	if (s[0] >= '0' && s[0] <= '9')
	{
		int idnum = atoi(s);
		if (idnum >= sv_maxclients->integer)		// negative values are impossible here
		{
			appWPrintf("Bad client slot: %d\n", idnum);
			return false;
		}

		sv_client = &svs.clients[idnum];
		sv_player = sv_client->edict;
		if (!sv_client->state)
		{
			appWPrintf("Client %d is not active\n", idnum);
			return false;
		}
		return true;
	}

	// check for a name match
	client_t *cl;
	int		i;
	for (i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++)
	{
		if (!cl->state) continue;
		if (!(cl->Name.icmp(s)))
		{
			sv_client = cl;
			sv_player = sv_client->edict;
			return true;
		}
	}

	appWPrintf("Userid %s is not on the server\n", s);
	return false;
}


// Kick a user off of the server
static void SV_Kick_f(bool usage, int argc, char **argv)
{
	if (argc != 2 || usage)
	{
		appPrintf("Usage: kick <userid>\n");
		return;
	}

	if (!svs.initialized)
	{
		appWPrintf("No server running.\n");
		return;
	}

	if (!SetPlayer(argv[1]))
		return;

	if (sv_client->netchan.remote_address.type == NA_LOOPBACK)
	{
		appWPrintf("Cannot kick host player\n");
		return;
	}

	SV_DropClient(sv_client, "was kicked");
	sv_client->lastmessage = svs.realtime;			// min case there is a funny zombie
}


#define MAX_BAN_LIST	256

static int banCount;
static char banlist[40][MAX_BAN_LIST];				// longest ban string is "NNN-NNN" * 4 + '.' * 3 + #0 -> 32 bytes


static int FindBanString(const char *str)
{
	for (int i = 0; i < banCount; i++)
		if (!strcmp(banlist[i], str))
			return i;
	return -1;
}


static void AddBanString(const char *str)
{
	if (FindBanString(str) >= 0)
	{
		appWPrintf("%s already banned\n", str);
		return;
	}
	if (banCount >= MAX_BAN_LIST)
	{
		appWPrintf("Cannot ban %s: ban list full\n", str);
		return;
	}
	Com_DPrintf("Add %s to banlist\n", str);
	strcpy(banlist[banCount++], str);
}


bool SV_AddressBanned(netadr_t *a)
{
	if (a->type != NA_IP) return false;				// non-IP address
	for (int i = 0; i < banCount; i++)
		if (IPWildcard(a, banlist[i]))
			return true;							// banned
	return false;
}


static void SV_Ban_f(bool usage, int argc, char **argv)
{
	if (argc != 2 || usage)
	{
		appPrintf("Usage: ban <userid>\n");
		return;
	}

	if (!svs.initialized)		// ban by userid requires user to be connected, so -- server must be launched
	{
		appWPrintf("No server running.\n");
		return;
	}

	if (!SetPlayer(argv[1]))
		return;

	if (sv_client->netchan.remote_address.type == NA_LOOPBACK)
	{
		appWPrintf("Cannot ban local client\n");
		return;
	}

	// add to ban list
	byte *ip = sv_client->netchan.remote_address.ip;
	TString<40> Ban;
	Ban.sprintf("%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
	AddBanString(Ban);

	SV_DropClient(sv_client, "was banned");
	sv_client->lastmessage = svs.realtime;		// min case there is a funny zombie
}


static void SV_BanIP_f(bool usage, int argc, char **argv)
{
	// can modify banlist before server launched
	if (argc != 2 || usage)
	{
		appPrintf("Usage: banip <ip-mask>\n");
		return;
	}

	const char *str = argv[1];
	if (strlen(str) > 32)
	{
		appWPrintf("IP-mask is too long\n");
		return;
	}
	AddBanString(str);

	if (!svs.initialized) return;				// server is not running -- just remember IP for future

	// find client with banned IP
	sv_client = NULL;
	client_t *cl;
	int		i;
	for (i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++)
	{
		if (!cl->state) continue;
		if (!IPWildcard(&cl->netchan.remote_address, str)) continue;

		SV_DropClient(cl, "was banned");
		cl->lastmessage = svs.realtime;			// min case there is a funny zombie
	}
}


static void SV_BanList_f()
{
	appPrintf("Banned IP's:\n");
	for (int i = 0; i < banCount; i++)
		appPrintf("%-2d: %s\n", i, banlist[i]);
}


static void SV_BanRemove_f(bool usage, int argc, char **argv)
{
	if (argc != 2 || usage)
	{
		appPrintf("Usage: banremove <list-index or ip-mask>\n");
		return;
	}

	const char *str = argv[1];
	int		n;
	if (strchr(str, '.'))
	{
		n = FindBanString(str);
		if (n < 0)
		{
			appWPrintf("Mask %s is not in ban list\n", str);
			return;
		}
	}
	else
	{
		if (str[0] < '0' || str[0] > '9')
		{
			appWPrintf("Bad list index %s\n", str);
			return;
		}
		n = atoi(str);
		if (n >= banCount)		// do not need "< 0" check: if number have "-" - will not get here
		{
			appWPrintf("Index %d is out of list bounds\n", n);
			return;
        }
	}
	if (n < banCount - 1)
		memcpy(banlist[n], banlist[n+1], sizeof(banlist[0]) * (banCount - n - 1));
	banCount--;
}


// Examine all a users info strings
static void SV_DumpUser_f(bool usage, int argc, char **argv)
{
	if (argc != 2 || usage)
	{
		appPrintf("Usage: dumpuser <userid>\n");
		return;
	}

	if (!SetPlayer(argv[1]))
		return;

	appPrintf(S_GREEN"userinfo\n--------\n");
	Info_Print(sv_client->Userinfo);
}


//-----------------------------------------------------------------------------

static void SV_Status_f()
{
	if (!svs.clients)
	{
		appWPrintf("No server running.\n");
		return;
	}
	appPrintf("map: %s\n", sv.name);

	appPrintf("--n-score-ping-name------------lastmsg-address---------------qport-\n");
	int		i;
	client_t *cl;
	for (i = 0, cl = svs.clients; i < sv_maxclients->integer; i++,cl++)
	{
		if (!cl->state) continue;
		if (!sv.attractloop)
			appPrintf("%3d %5d ", i, cl->edict->client->ps.stats[STAT_FRAGS]);
		else	// cannot access edict for demo client
			appPrintf("%3d "S_RED"----- "S_WHITE, i);

		if (cl->state == cs_connected)
			appPrintf("CNCT ");
		else if (cl->state == cs_zombie)
			appPrintf(S_RED"ZMBI "S_WHITE);
		else
			appPrintf("%4d ", min(cl->ping, 9999));
		// display info
		appPrintf("%-15s %7d %-21s %5d", *cl->Name, svs.realtime - cl->lastmessage,
			NET_AdrToString(&cl->netchan.remote_address), cl->netchan.port);
		// display downloading info
		if (cl->download)
		{
			int size = cl->downloadsize;
			if (!size) size = 1;
			appPrintf(S_BLUE"  dwnl: %s (%d%%)", *cl->DownloadName, cl->downloadcount * 100 / size);
		}
		appPrintf("\n");
	}
	appPrintf("\n");
}


// "say" for dedicated server
static void SV_ConSay_f(int argc, char **argv)
{
	if (argc < 2) return;

	int		i;
	TString<1024> Buf;
	Buf[0] = 0;
	for (i = 1; i < argc; i++)
		Buf += va(" %s", argv[i]);

	client_t *client;
	for (i = 0, client = svs.clients; i < sv_maxclients->integer; i++, client++)
	{
		if (client->state != cs_spawned)
			continue;
		SV_ClientPrintf(client, PRINT_CHAT, "server: %s\n", *Buf);
	}
}


static void SV_Heartbeat_f()
{
	svs.last_heartbeat = -BIG_NUMBER;
}


// Examine the serverinfo string
static void SV_Serverinfo_f()
{
	appPrintf(S_GREEN"------ Server info settings ------\n");
	Info_Print(Cvar_BitInfo(CVAR_SERVERINFO));
}



// Begins server demo recording.  Every entity and every message will be
// recorded, but no playerinfo will be stored.  Primarily for demo merging.
static void SV_ServerRecord_f(bool usage, int argc, char **argv)
{
	if (argc != 2 || usage)
	{
		appPrintf("Usage: serverrecord <demoname>\n");
		return;
	}

	if (svs.wdemofile)
	{
		appWPrintf("Already recording\n");
		return;
	}

	if (sv.state != ss_game)
	{
		appWPrintf("You must be in a level to record\n");
		return;
	}

	// open the demo file
	TString<MAX_OSPATH> Name;
	Name.sprintf("./%s/demos/%s.dm2", FS_Gamedir(), argv[1]);

	appMakeDirectoryForFile(Name);
	svs.wdemofile = fopen(Name, "wb");
	if (!svs.wdemofile)
	{
		appWPrintf("ERROR: couldn't write %s\n", *Name);
		return;
	}

	appPrintf("recording to %s\n", *Name);
	// setup a buffer to catch all multicasts
	svs.demo_multicast.Init(ARRAY_ARG(svs.demo_multicast_buf));

	// write a single giant fake message with all the startup info
	sizebuf_t buf;
	char buf_data[32768];
	buf.Init(ARRAY_ARG(buf_data));

	// serverdata needs to go over for all types of servers
	// to make sure the protocol is right, and to set the gamedir
	//
	// send the serverdata
	MSG_WriteByte(&buf, svc_serverdata);
	MSG_WriteLong(&buf, PROTOCOL_VERSION);
	MSG_WriteLong(&buf, svs.spawncount);
	// 2 means server demo
	MSG_WriteByte(&buf, 2);	// demos are always attract loops
	MSG_WriteString(&buf, FS_Gamedir());
	MSG_WriteShort(&buf, -1);
	// send full levelname
	MSG_WriteString(&buf, sv.configstrings[CS_NAME]);

	for (int i = 0; i < MAX_CONFIGSTRINGS; i++)
		if (sv.configstrings[i][0])
		{
			MSG_WriteByte(&buf, svc_configstring);
			MSG_WriteShort(&buf, i);
			MSG_WriteString(&buf, sv.configstrings[i]);
		}

	// write it to the demo file
	int len = LittleLong(buf.cursize);
	fwrite(&len, 4, 1, svs.wdemofile);
	fwrite(buf.data, buf.cursize, 1, svs.wdemofile);

	// the rest of the demo file will be individual frames
}


// Ends server demo recording
static void SV_ServerStop_f()
{
	if (!svs.wdemofile)
	{
		appPrintf("Not doing a serverrecord.\n");
		return;
	}
	fclose(svs.wdemofile);
	svs.wdemofile = NULL;
	appPrintf("Recording completed\n");
}


// Kick everyone off, possibly in preparation for a new game
static void SV_KillServer_f(void)
{
	if (!svs.initialized)
		return;
	SV_Shutdown("Server was killed\n");
	NET_Config(false);		// close network sockets
}


// Let the game library handle a command
static void SV_ServerCommand_f(int argc, char **argv)
{
	if (!ge)
	{
		appWPrintf("No game loaded\n");
		return;
	}

	// pass command to server string tokenizer
	TString<1024> Buf; Buf[0] = 0;
	for (int i = 0; i < argc; i++)
	{
		if (i > 0) Buf += " ";
		Buf += argv[i];
	}
	SV_TokenizeString(Buf);

	guardGame(ge.ServerCommand);
	ge->ServerCommand();
	unguardGame;
}


void SV_InitCommands()
{
	RegisterCommand("heartbeat", SV_Heartbeat_f);

	RegisterCommand("kick", SV_Kick_f);
	RegisterCommand("ban", SV_Ban_f);
	RegisterCommand("banip", SV_BanIP_f);
	RegisterCommand("banlist", SV_BanList_f);
	RegisterCommand("banremove", SV_BanRemove_f);

	RegisterCommand("status", SV_Status_f);
	RegisterCommand("serverinfo", SV_Serverinfo_f);
	RegisterCommand("dumpuser", SV_DumpUser_f);

	RegisterCommand("map", SV_Map_f);
	RegisterCommand("demomap", SV_DemoMap_f);
	RegisterCommand("gamemap", SV_GameMap_f);
	RegisterCommand("setmaster", SV_SetMaster_f);

	RegisterCommand("serverrecord", SV_ServerRecord_f);
	RegisterCommand("serverstop", SV_ServerStop_f);

	RegisterCommand("save", SV_Savegame_f);
	RegisterCommand("load", SV_Loadgame_f);

	RegisterCommand("killserver", SV_KillServer_f);

	RegisterCommand("sv", SV_ServerCommand_f);

	if (DEDICATED)
		RegisterCommand("say", SV_ConSay_f);
}
