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
// cl_parse.c  -- parse a message received from the server

#include "client.h"

char *svc_strings[256] =
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

//=============================================================================

static void DownloadFileName (char *dest, int destlen, char *fn)	//?? players - from baseq2 only ??
{
	if (memcmp (fn, "players", 7) == 0)
		Com_sprintf (dest, destlen, "%s/%s", BASEDIRNAME, fn);
	else
		Com_sprintf (dest, destlen, "%s/%s", FS_Gamedir(), fn);
}

/*
===============
CL_CheckOrDownloadFile

Returns true if the file exists, otherwise it attempts
to start a download from the server.
===============
*/

#define MAX_CHECK_CACHE		32768
#define MAX_CHECK_NAMES		512
static char *checkedNames[MAX_CHECK_NAMES], checkNameCache[MAX_CHECK_CACHE];
static int numCheckedNames, nextCheckCachePos;


bool CL_CheckOrDownloadFile (char *filename)
{
	FILE	*fp;
	char	name[MAX_OSPATH], name2[MAX_OSPATH], *ext;
	int		i;

	Q_CopyFilename (name, filename, sizeof(name));
	if (strstr (name, ".."))
	{
		Com_Printf ("Refusing to download a path with ..\n");
		return true;
	}

	strcpy (name2, name);
	ext = strrchr (name2, '.');

	// checking map - this should be a first precache request, so - clear name cache
	if (ext && !strcmp (ext, ".bsp"))
		numCheckedNames = nextCheckCachePos = 0;
	// prevent from checking the same file twice
	for (i = 0; i < numCheckedNames; i++)
		if (!strcmp (name, checkedNames[i]))
		{
//			Com_Printf(S_RED"chk: %s\n", name);//!!
			return true;	// already checked, return "found" even if not found
		}
	// register new name
	if (numCheckedNames < MAX_CHECK_NAMES)
	{
		int		len;

		len = strlen (name);
		if (len + nextCheckCachePos + 1 < MAX_CHECK_CACHE)
		{
			char	*s;

//			Com_Printf(S_GREEN"new: %s\n", name);//!!
			s = checkedNames[numCheckedNames++] = &checkNameCache[nextCheckCachePos];
			strcpy (s, name);
			nextCheckCachePos += len + 1;
		}
	}

	// check all image format for .PCX and .WAL extension
	if (ext && (!strcmp (ext, ".pcx") || !strcmp (ext, ".wal")))
	{
		*ext = 0; // strip extension
		if (ImageExists (name2, IMAGE_ANY))
			return true;
	}

	if (FS_FileExists (name))
	{	// it exists, no need to download
		return true;
	}

	strcpy (cls.downloadname, name);

	// download to a temp name, and only rename
	// to the real name when done, so if interrupted
	// a runt file wont be left
#if 0
	COM_StripExtension (cls.downloadname, cls.downloadtempname);
#else
	strcpy (cls.downloadtempname, name);
#endif
	strcat (cls.downloadtempname, ".tmp");

//ZOID
	// check to see if we already have a tmp for this file, if so, try to resume
	// open the file if not opened yet
	DownloadFileName (name, sizeof(name), cls.downloadtempname);

//	FS_CreatePath (name);

	fp = fopen (name, "r+b");
	if (fp)
	{ // it exists
		int		len;

		fseek (fp, 0, SEEK_END);
		len = ftell (fp);
		cls.download = fp;

		// give the server an offset to start the download
		Com_Printf ("Resuming %s\n", cls.downloadname);
		MSG_WriteByte (&cls.netchan.message, clc_stringcmd);
		MSG_WriteString (&cls.netchan.message, va("download %s %i", cls.downloadname, len));
	}
	else
	{
		Com_Printf ("Downloading %s\n", cls.downloadname);
		MSG_WriteByte (&cls.netchan.message, clc_stringcmd);
		MSG_WriteString (&cls.netchan.message, va("download %s", cls.downloadname));
	}

	cls.downloadnumber++;

	return false;
}

/*
===============
CL_Download_f

Request a download from the server
===============
*/
void CL_Download_f (void)
{
	char filename[MAX_OSPATH];

	if (Cmd_Argc() != 2)
	{
		Com_Printf("Usage: download <filename>\n");
		return;
	}

	Q_CopyFilename (filename, Cmd_Argv(1), sizeof(filename));

	if (strstr (filename, ".."))
	{
		Com_Printf ("Refusing to download a path with ..\n");
		return;
	}

	if (FS_FileExists (filename))
	{	// it exists, no need to download
		Com_Printf("File already exists.\n");
		return;
	}

	strcpy (cls.downloadname, filename);
	Com_Printf ("Downloading %s\n", cls.downloadname);

	// download to a temp name, and only rename
	// to the real name when done, so if interrupted
	// a runt file wont be left
#if 0
	COM_StripExtension (cls.downloadname, cls.downloadtempname);
#else
	strcpy (cls.downloadtempname, filename);
#endif
	strcat (cls.downloadtempname, ".tmp");

	MSG_WriteByte (&cls.netchan.message, clc_stringcmd);
	MSG_WriteString (&cls.netchan.message, va("download %s", cls.downloadname));

	cls.downloadnumber++;
}

/*
======================
CL_RegisterSounds
======================
*/
void CL_RegisterSounds (void)
{
	int		i;

	S_BeginRegistration ();
	CL_RegisterTEntSounds ();

	for (i = 1; i < MAX_SOUNDS; i++)
	{
		if (!cl.configstrings[CS_SOUNDS+i][0])
			break;
		cl.sound_precache[i] = S_RegisterSound (cl.configstrings[CS_SOUNDS+i]);
		Sys_SendKeyEvents ();	// pump message loop
	}
	S_EndRegistration ();
}


/*
=====================
CL_ParseDownload

A download message has been received from the server
=====================
*/
void CL_ParseDownload (void)
{
	int		size, percent;
	char	name[MAX_OSPATH];
	int		r;

	// read the data
	size = MSG_ReadShort (&net_message);
	percent = MSG_ReadByte (&net_message);
	if (size == -1)
	{
		Com_WPrintf ("Server does not have this file.\n");
		if (cls.download)
		{
			// if here, we tried to resume a file but the server said no
			fclose (cls.download);
			cls.download = NULL;
		}
		CL_RequestNextDownload ();
		return;
	}

	// open the file if not opened yet
	if (!cls.download)
	{
		DownloadFileName (name, sizeof(name), cls.downloadtempname);

		FS_CreatePath (name);

		cls.download = fopen (name, "wb");
		if (!cls.download)
		{
			net_message.readcount += size;
			Com_WPrintf ("Failed to open %s\n", cls.downloadtempname);
			CL_RequestNextDownload ();
			return;
		}
	}

	fwrite (net_message.data + net_message.readcount, 1, size, cls.download);
	net_message.readcount += size;

	if (percent != 100)
	{
		// request next block
// change display routines by zoid
#if 0
		Com_Printf (".");
		if (10*(percent/10) != cls.downloadpercent)
		{
			cls.downloadpercent = 10*(percent/10);
			Com_Printf ("%i%%", cls.downloadpercent);
		}
#endif
		cls.downloadpercent = percent;

		MSG_WriteByte (&cls.netchan.message, clc_stringcmd);
		SZ_Print (&cls.netchan.message, "nextdl");
	}
	else
	{
		char	oldn[MAX_OSPATH];
		char	newn[MAX_OSPATH];

//		Com_Printf ("100%%\n");

		fclose (cls.download);

		// rename the temp file to it's final name
		DownloadFileName (oldn, sizeof(oldn), cls.downloadtempname);
		DownloadFileName (newn, sizeof(newn), cls.downloadname);
		r = rename (oldn, newn);
		if (r)
			Com_WPrintf ("failed to rename.\n");

		cls.download = NULL;
		cls.downloadpercent = 0;

		// get another file if needed
		CL_RequestNextDownload ();
	}
}


/*
=====================================================================

  SERVER CONNECTING MESSAGES

=====================================================================
*/

/*
==================
CL_ParseServerData
==================
*/
void CL_ParseServerData (void)
{
	char	*str;
	int		i;

	Com_DPrintf ("Serverdata packet received.\n");

	// wipe the client_state_t struct
	CL_ClearState ();
	cls.state = ca_connected;

	// parse protocol version number
	i = MSG_ReadLong (&net_message);
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
		Com_DropError ("Server returned version %d <> %d", i, PROTOCOL_VERSION);

	cl.servercount = MSG_ReadLong (&net_message);
	cl.attractloop = MSG_ReadByte (&net_message) != 0;

	// game directory
	str = MSG_ReadString (&net_message);
	Q_strncpyz (cl.gamedir, str, sizeof(cl.gamedir));
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
		// seperate the printfs so the server message can have a color
		Com_Printf("\n\n\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37\n\n");
		Com_Printf (S_RED"%s\n", str);				// display map message

		// need to prep refresh at next oportunity
		cl.refresh_prepped = false;
	}
}

/*
==================
CL_ParseBaseline
==================
*/
void CL_ParseBaseline (void)
{
	unsigned	bits, newnum;
	entityState_t *es, nullstate;

	memset (&nullstate, 0, sizeof(nullstate));

	newnum = CL_ParseEntityBits (&bits);
	es = &cl_entities[newnum].baseline;
	CL_ParseDelta (&nullstate, es, newnum, bits, true);
}


/*
================
CL_LoadClientinfo

================
*/
void CL_LoadClientinfo (clientinfo_t *ci, char *s)
{
	int		i;
	char	*t, model_name[MAX_QPATH], skin_name[MAX_QPATH];

	Q_strncpyz (ci->cinfo, s, sizeof(ci->cinfo));

	// isolate the player's name
	Q_strncpyz (ci->name, s, sizeof(ci->name));
	t = strchr (s, '\\');
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
		for (i = 0; i < num_cl_weaponmodels; i++)
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
			Com_sprintf (ci->iconname, sizeof(ci->iconname), "/players/%s/%s_i.pcx", model_name, skin_name);
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
CL_ParseConfigString
================
*/
void CL_ParseConfigString (void)
{
	int		i;
	char	*s;
	char	olds[MAX_QPATH];

	i = MSG_ReadShort (&net_message);
	if (i < 0 || i >= MAX_CONFIGSTRINGS)
		Com_DropError ("configstring > MAX_CONFIGSTRINGS");
	s = MSG_ReadString(&net_message);

	Q_strncpyz (olds, cl.configstrings[i], sizeof(olds));

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
CL_ParseStartSoundPacket
==================
*/
void CL_ParseStartSoundPacket(void)
{
	vec3_t  pos_v;
	float	*pos;
	int 	channel, ent;
	int 	sound_num;
	float 	volume;
	float 	attenuation;
	int		flags;
	float	ofs;

	flags = MSG_ReadByte (&net_message);
	sound_num = MSG_ReadByte (&net_message);

	if (flags & SND_VOLUME)
		volume = MSG_ReadByte (&net_message) / 255.0;
	else
		volume = DEFAULT_SOUND_PACKET_VOLUME;

	if (flags & SND_ATTENUATION)
		attenuation = MSG_ReadByte (&net_message) / 64.0;
	else
		attenuation = DEFAULT_SOUND_PACKET_ATTENUATION;

	if (flags & SND_OFFSET)
		ofs = MSG_ReadByte (&net_message) / 1000.0;
	else
		ofs = 0;

	if (flags & SND_ENT)
	{	// entity reletive
		channel = MSG_ReadShort(&net_message);
		ent = channel>>3;
		if (ent > MAX_EDICTS)
			Com_DropError ("CL_ParseStartSoundPacket: ent = %i", ent);

		channel &= 7;
	}
	else
	{
		ent = 0;
		channel = 0;
	}

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
=====================
CL_ParseServerMessage
=====================
*/
void CL_ParseServerMessage (void)
{
	int		cmd;
	char	*s;

	guard(CL_ParseServerMessage);

	// if recording demos, copy the message out
	if (cl_shownet->integer == 1)
		Com_Printf ("%i ",net_message.cursize);
	else if (cl_shownet->integer >= 2)
		Com_Printf ("------------------\n");

	// parse the message
	while (1)
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

		if (cl_shownet->integer >= 2)
		{
			if (!svc_strings[cmd])
				Com_WPrintf ("%3d:bad cmd %d\n", net_message.readcount-1,cmd);
			else
				SHOWNET(svc_strings[cmd]);
		}

		// other commands
		switch (cmd)
		{
		default:
			Com_DropError ("CL_ParseServerMessage: Illegible server message\n");
			break;

		case svc_nop:
//			Com_Printf ("svc_nop\n");
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
			CL_ParseServerData ();
			break;

		case svc_configstring:
			CL_ParseConfigString ();
			break;

		case svc_sound:
			CL_ParseStartSoundPacket();
			break;

		case svc_spawnbaseline:
			CL_ParseBaseline ();
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
			CL_ParseInventory ();
			break;

		case svc_layout:
			s = MSG_ReadString (&net_message);
			Q_strncpyz (cl.layout, s, sizeof(cl.layout));
			break;

		case svc_playerinfo:
		case svc_packetentities:
		case svc_deltapacketentities:
			Com_DropError ("Out of place frame data");
			break;
		}
	}

	CL_AddNetgraph ();

	// we don't know if it is ok to save a demo message until
	// after we have parsed the frame
	if (cls.demorecording && !cls.demowaiting)
		CL_WriteDemoMessage ();

	unguardf(("cmd=%s", svc_strings[cmd]));
}
