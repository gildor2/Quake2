#include "client.h"
#include "qfiles.h"			// we analyze some file structures to extract model/skin info

/*?? What we can do here (with extended protocol):
 *	1. compress data with zlib
 *	2. with data, send full filename too; this may be useful for:
 *		a) transferring images - any image type by single request name
 *		b) transferring mod data: specify, where data from: base dir or mod
 *		c) transferring paks: when file comes from pakfile, send pak and
 *			mount it atfer transfer (before other downloads!)
 */

extern	cvar_t *allow_download;
extern	cvar_t *allow_download_players;
extern	cvar_t *allow_download_models;
extern	cvar_t *allow_download_sounds;
extern	cvar_t *allow_download_maps;

static void RequestNextDownload ();


static void DownloadFileName (char *dest, int destlen, const char *filename)
{
	if (memcmp (filename, "players", 7) == 0)	//?? players - downloaded to baseq2 only
		appSprintf (dest, destlen, "%s/%s", BASEDIRNAME, filename);
	else
		appSprintf (dest, destlen, "%s/%s", FS_Gamedir(), filename);
}

/*
===============
CheckOrDownloadFile

Returns true if the file exists, otherwise it attempts
to start a download from the server.
===============
*/

// cache failed downloading names for preventing re-check/re-warning
#define MAX_CHECK_CACHE		32768
#define MAX_CHECK_NAMES		512
static char *checkedNames[MAX_CHECK_NAMES], checkNameCache[MAX_CHECK_CACHE];
static int numCheckedNames, nextCheckCachePos;


static bool CheckOrDownloadFile (const char *fmt, ...)
{
	char	name[MAX_OSPATH], name2[MAX_OSPATH], *ext;

	guard(CheckOrDownloadFile);

	va_list	argptr;
	va_start (argptr, fmt);
	vsnprintf (ARRAY_ARG(name), fmt, argptr);
	va_end (argptr);

	appCopyFilename (name, name, sizeof(name));		// in-place
	if (!memcmp (name, "../", 3))					// if trying to leave quake root dir, "../" will be at start
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
	for (int i = 0; i < numCheckedNames; i++)
		if (!strcmp (name, checkedNames[i]))
			return true;	// already checked, return "found" even if not found
	// register new name
	if (numCheckedNames < MAX_CHECK_NAMES)
	{
		int len = strlen (name);
		if (len + nextCheckCachePos + 1 < MAX_CHECK_CACHE)
		{
			char *s = checkedNames[numCheckedNames++] = &checkNameCache[nextCheckCachePos];
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

	if (FS_FileExists (name))	// it exists, no need to download
		return true;

	strcpy (cls.downloadname, name);

	// download to a temp name, and only rename
	// to the real name when done, so if interrupted
	// a runt file wont be left
	appSprintf (ARRAY_ARG(cls.downloadtempname), "%s.tmp", name);

	// check to see if we already have a tmp for this file, if so, try to resume
	// open the file if not opened yet
	DownloadFileName (ARRAY_ARG(name), cls.downloadtempname);

	FILE *fp = fopen (name, "r+b");
	if (fp)
	{
		fseek (fp, 0, SEEK_END);
		int len = ftell (fp);
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
	unguardf(("file=%s", name));
}

/*
===============
CL_Download_f

Request a download from the server
===============
*/
void CL_Download_f (bool usage, int argc, char **argv)
{
	char filename[MAX_OSPATH];

	if (argc != 2 || usage)
	{
		Com_Printf("Usage: download <filename>\n");
		return;
	}

	if (cls.state != ca_connected && cls.state != ca_active)
	{
		Com_WPrintf ("Not connected\n");
		return;
	}

	appCopyFilename (filename, argv[1], sizeof(filename));

	if (FS_FileExists (filename))
	{	// it exists, no need to download
		Com_Printf ("File already exists.\n");
		return;
	}

	CheckOrDownloadFile (filename);
}

/*
=====================
CL_ParseDownload

A download message has been received from the server
=====================
*/
void CL_ParseDownload ()
{
	char	name[MAX_OSPATH];

	// read the data
	int size = MSG_ReadShort (&net_message);
	int percent = MSG_ReadByte (&net_message);
	if (size == -1)
	{
		Com_WPrintf ("Server does not have this file.\n");
		if (cls.download)
		{
			// if here, we tried to resume a file but the server said no
			fclose (cls.download);
			cls.download = NULL;
		}
		RequestNextDownload ();
		return;
	}

	// open the file if not opened yet
	if (!cls.download)
	{
		DownloadFileName (ARRAY_ARG(name), cls.downloadtempname);

		FS_CreatePath (name);

		cls.download = fopen (name, "wb");
		if (!cls.download)
		{
			net_message.readcount += size;
			Com_WPrintf ("Failed to open %s\n", cls.downloadtempname);
			RequestNextDownload ();
			return;
		}
	}

	fwrite (net_message.data + net_message.readcount, 1, size, cls.download);
	net_message.readcount += size;

	if (percent < 100)
	{
		// request next block
		cls.downloadpercent = percent;

		MSG_WriteByte (&cls.netchan.message, clc_stringcmd);
		MSG_WriteString (&cls.netchan.message, "nextdl");
	}
	else
	{
		char	oldn[MAX_OSPATH];
		char	newn[MAX_OSPATH];

		fclose (cls.download);

		// rename the temp file to it's final name
		DownloadFileName (ARRAY_ARG(oldn), cls.downloadtempname);
		DownloadFileName (ARRAY_ARG(newn), cls.downloadname);
		if (rename (oldn, newn))
			Com_WPrintf ("failed to rename \"%s\" to \"%s\"\n", oldn, newn);

		cls.download = NULL;
		cls.downloadpercent = 0;

		// get another file if needed
		//?? Is it ALWAYS needed ? Downloading may be initiated with "download" console command
		RequestNextDownload ();
	}
}


/*-----------------------------------------------------------------------------
	Precaching and downloading
-----------------------------------------------------------------------------*/

static int precache_check;					// for autodownload of precache items
static int precache_spawncount;
static int precache_tex;
static int precache_model_skin;

static byte *precache_model;				// used for skin checking in alias models

// player has few things to download: model, weapon model, skins for player and weapon,
// and player icon; so PLAYER_MULT = 5
#define PLAYER_MULT		5
// Dummy configstring indexes:
#define DCS_START		0
#define DCS_MAP_LOAD	(CS_PLAYERSKINS + MAX_CLIENTS * PLAYER_MULT)
#define DCS_SKY			(DCS_MAP_LOAD + 1)
#define DCS_TEXTURE		(DCS_SKY + 6)		// 6 sides
#define DCS_SKIP_ALL	(DCS_TEXTURE + 99999)

//!! NOTE: when downloading one of wal/pcx/tga/jpg image formats, or downloading map patch, should
//!!	   not display error when file absent on server (but texture: error when no image at all).
//!!	   Make this as flag for CheckOrDownloadFile()

static void RequestNextDownload ()
{
	guard(RequestNextDownload);

	if (cls.state != ca_connected)
		return;

	if (cls.netchan.remote_address.type == NA_LOOPBACK)
		precache_check = DCS_SKIP_ALL;		// local server: download is nonsense (skip this phase)

	if (!allow_download->integer && precache_check < DCS_MAP_LOAD)
		precache_check = DCS_MAP_LOAD;

	//----------------- downloading ---------------------

	if (precache_check == DCS_START) {
		// levelshot
		precache_check++;
		char *s = strrchr (cl.configstrings[CS_MODELS+1], '/');
		if (s) {
			char mapname[MAX_QPATH];
			strcpy (mapname, s+1);
			if (s = strrchr (mapname, '.')) *s = 0;
			if (!CheckOrDownloadFile ("levelshots/%s.jpg", mapname))	//??? should download any image (not JPG only) !!
				return;
		}
	}
	if (precache_check == DCS_START + 1) {
		// map patch
		precache_check = CS_MODELS + 1;		// models[0] is not used
		if (!CheckOrDownloadFile ("%s.add", cl.configstrings[CS_MODELS+1]))
			return;
	}

	if (precache_check == CS_MODELS + 1) {
		// map itself
		precache_check = CS_MODELS + 2;
		SCR_SetLevelshot ();
		if (allow_download_maps->integer)
			if (!CheckOrDownloadFile(cl.configstrings[CS_MODELS+1]))
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
					if (!CheckOrDownloadFile(cl.configstrings[precache_check])) {
						precache_model_skin = 1;
						return; // started a download
					}
					precache_model_skin = 1;
				}

				// checking for skins in the model
				if (!precache_model) {
					if (!(precache_model = (byte*) FS_LoadFile (cl.configstrings[precache_check]))) {
						precache_model_skin = 0;
						precache_check++;
						continue; // couldn't load it
					}
					dMd2_t *hdr = (dMd2_t *)precache_model;
					if (LittleLong(hdr) != MD2_IDENT || LittleLong(hdr->version) != MD2_VERSION) {
						// not an alias model, or wrong model version
						FS_FreeFile (precache_model);
						precache_model = NULL;
						precache_model_skin = 0;
						precache_check++;
						continue;
					}
				}

				dMd2_t *hdr = (dMd2_t *)precache_model;
				while (precache_model_skin - 1 < LittleLong(hdr->numSkins)) {
					if (!CheckOrDownloadFile((char *)precache_model + LittleLong(hdr->ofsSkins) +
						(precache_model_skin - 1)*MD2_MAX_SKINNAME))
					{
						precache_model_skin++;
						return; // started a download
					}
					precache_model_skin++;
				}
				if (precache_model)
				{
					FS_FreeFile (precache_model);
					precache_model = NULL;
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
				if (!CheckOrDownloadFile("sound/%s", cl.configstrings[precache_check++]))
					return; // started a download
			}
		}
		precache_check = CS_IMAGES;
	}

	if (precache_check >= CS_IMAGES && precache_check < CS_IMAGES+MAX_IMAGES)
	{
		if (precache_check == CS_IMAGES) precache_check++; // zero is blank
		while (precache_check < CS_IMAGES+MAX_IMAGES && cl.configstrings[precache_check][0])
			if (!CheckOrDownloadFile("pics/%s.pcx", cl.configstrings[precache_check++]))	//??? not PCX only
				return; // started a download
		precache_check = CS_PLAYERSKINS;
	}

	if (precache_check >= CS_PLAYERSKINS && precache_check < CS_PLAYERSKINS + MAX_CLIENTS * PLAYER_MULT) {
		if (allow_download_players->integer) {
			while (precache_check < CS_PLAYERSKINS + MAX_CLIENTS * PLAYER_MULT) {
				int i = (precache_check - CS_PLAYERSKINS) / PLAYER_MULT;	// player index

				if (!cl.configstrings[CS_PLAYERSKINS+i][0]) {
					precache_check = CS_PLAYERSKINS + (i + 1) * PLAYER_MULT;
					continue;
				}

				char *p = strchr (cl.configstrings[CS_PLAYERSKINS+i], '\\');
				if (p)
					p++;
				else
					p = cl.configstrings[CS_PLAYERSKINS+i];

				char model[MAX_QPATH];
				strcpy (model, p);		// format: model/skin
				const char *skin = "";
				p = strchr (model, '/');
				if (!p)
					p = strchr (model, '\\');
				if (p)
				{
					*p = 0;				// "model" will contain "modelName<0>skinName"
					skin = p+1;
				}

				switch ((precache_check - CS_PLAYERSKINS) % PLAYER_MULT)	// 0..PLAYER_MULT-1 -- what to check
				{
				case 0:		// player model
					precache_check++;
					if (!CheckOrDownloadFile("players/%s/tris.md2", model))
						return;
					// FALL THROUGH
				case 1:		// weapon model
					precache_check++;
					if (!CheckOrDownloadFile("players/%s/weapon.md2", model))
						return;
					// FALL THROUGH
				case 2:		// weapon skin
					precache_check++;
					if (!CheckOrDownloadFile("players/%s/weapon.pcx", model))	//??? PCX
						return;
					// FALL THROUGH
				case 3:		// player skin
					precache_check++;
					if (!CheckOrDownloadFile("players/%s/%s.pcx", model, skin))	//??? PCX
						return;
					// FALL THROUGH
				case 4:		// skin_i
					precache_check++;
					if (!CheckOrDownloadFile("players/%s/%s_i.pcx", model, skin))	//??? PCX
						return;
				}
				// move on to next model
			}
		}
		// precache phase completed
		precache_check = DCS_MAP_LOAD;
	}

	if (precache_check == DCS_MAP_LOAD)
	{
		unsigned map_checksum;		// for detecting cheater maps

		precache_check++;			// DCS_SKY
		CM_LoadMap (cl.configstrings[CS_MODELS+1], true, &map_checksum);
		// verify map checksum
		if (map_checksum != atoi(cl.configstrings[CS_MAPCHECKSUM]))
			Com_DropError ("Local map version differs from server: %d != \"%s\"",
				map_checksum, cl.configstrings[CS_MAPCHECKSUM]);
	}

	if (precache_check >= DCS_SKY && precache_check < DCS_TEXTURE)
	{
		if (allow_download->integer && allow_download_maps->integer)
			while (precache_check < DCS_TEXTURE)
			{
				static const char *env_suf[6] = {"rt", "bk", "lf", "ft", "up", "dn"};
				precache_check++;
				if (!CheckOrDownloadFile("env/%s%s.pcx", cl.configstrings[CS_SKY], env_suf[precache_check - DCS_SKY - 1]))	//??? PCX
					return; // started a download
			}
		precache_check = DCS_TEXTURE;
	}

	if (precache_check == DCS_TEXTURE)
	{
		precache_check = DCS_TEXTURE + 1;
		precache_tex = 0;
	}

	// confirm existance of textures, download any that don't exist
	if (precache_check == DCS_TEXTURE + 1)
	{
		// from qcommon/cmodel.cpp
		extern int			numTexinfo;
		extern csurface_t	*map_surfaces;

		if (allow_download->integer && allow_download_maps->integer)
			while (precache_tex < numTexinfo)
				if (!CheckOrDownloadFile("textures/%s.wal", map_surfaces[precache_tex++].fullName))	//??? WAL
					return; // started a download
		precache_check = DCS_SKIP_ALL;
	}

	CL_RegisterSounds ();
	V_InitRenderer ();

	MSG_WriteByte (&cls.netchan.message, clc_stringcmd);
	MSG_WriteString (&cls.netchan.message, va("begin %d\n", precache_spawncount));

	unguardf(("idx=%d", precache_check));
}

/*
=================
CL_Precache_f

The server will send this command right
before allowing the client into the server
=================
*/
void CL_Precache_f (int argc, char **argv)
{
	SCR_SetLevelshot ();
	//Yet another hack to let old demos work
	//the old precache sequence
	if (argc < 2)
	{
		unsigned map_checksum;		// for detecting cheater maps

		CM_LoadMap (cl.configstrings[CS_MODELS+1], true, &map_checksum);
		CL_RegisterSounds ();
		V_InitRenderer ();
		return;
	}

	precache_check = DCS_START;
	precache_spawncount = atoi (argv[1]);
	precache_model = NULL;
	precache_model_skin = 0;

	RequestNextDownload ();
}
