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
#include "client.h"

typedef struct
{
	byte	*data;
	int		count;
} cblock_t;

typedef struct
{
	qboolean restart_sound;		//?? is it really needed to restart sound for cinematics ?
	int		s_rate;
	int		s_width;
	int		s_channels;

	int		width;
	int		height;
	byte	*pic;
	byte	*pic_pending;
	char	imageName[MAX_OSPATH];		// used for "map image.pcx"

	// order 1 huffman stuff
	int		*hnodes1;					// [256][256][2];
	int		numhnodes1[256];

	int		h_used[512];
	int		h_count[512];
} cinematics_t;

static cinematics_t cin;

//=============================================================

/*
==================
SCR_StopCinematic
==================
*/
void SCR_StopCinematic (void)
{
	cl.cinematictime = 0;	// done
	if (cin.pic)
	{
		Z_Free (cin.pic);
		cin.pic = NULL;
	}
	if (cin.pic_pending)
	{
		Z_Free (cin.pic_pending);
		cin.pic_pending = NULL;
	}
	if (cl.cinematicpalette_active)
	{
		re.CinematicSetPalette(NULL);
		cl.cinematicpalette_active = false;
	}
	if (cl.cinematic_file)
	{
		FS_FCloseFile (cl.cinematic_file);
		cl.cinematic_file = NULL;
	}
	if (cin.hnodes1)
	{
		Z_Free (cin.hnodes1);
		cin.hnodes1 = NULL;
	}

	// restore sound rate if necessary
	if (cin.restart_sound)
	{
		cin.restart_sound = false;
		CL_Snd_Restart_f ();
	}

}

/*
====================
SCR_FinishCinematic

Called when either the cinematic completes, or it is aborted
====================
*/
void SCR_FinishCinematic (void)
{
	// tell the server to advance to the next map / cinematic
	MSG_WriteByte (&cls.netchan.message, clc_stringcmd);
	SZ_Print (&cls.netchan.message, va("nextserver %i\n", cl.servercount));
}

//==========================================================================

/*
==================
SmallestNode1
==================
*/
static int SmallestNode1 (int numhnodes)
{
	int		i;
	int		best, bestnode;

	best = 99999999;
	bestnode = -1;
	for (i=0 ; i<numhnodes ; i++)
	{
		if (cin.h_used[i])
			continue;
		if (!cin.h_count[i])
			continue;
		if (cin.h_count[i] < best)
		{
			best = cin.h_count[i];
			bestnode = i;
		}
	}

	if (bestnode == -1)
		return -1;

	cin.h_used[bestnode] = true;
	return bestnode;
}


/*
==================
Huff1TableInit

Reads the 64k counts table and initializes the node trees
==================
*/
static void Huff1TableInit (void)
{
	int		prev;
	int		j;
	int		*node, *nodebase;
	byte	counts[256];
	int		numhnodes;

	cin.hnodes1 = Z_Malloc (256*256*2*sizeof(int));
	memset (cin.hnodes1, 0, 256*256*2*sizeof(int));

	for (prev = 0; prev < 256; prev++)
	{
		memset (cin.h_count, 0, sizeof(cin.h_count));
		memset (cin.h_used, 0, sizeof(cin.h_used));

		// read a row of counts
		FS_Read (counts, sizeof(counts), cl.cinematic_file);
		for (j = 0; j < 256; j++)
			cin.h_count[j] = counts[j];

		// build the nodes
		numhnodes = 256;
		nodebase = cin.hnodes1 + prev*256*2;

		while (numhnodes != 511)
		{
			node = nodebase + (numhnodes-256)*2;

			// pick two lowest counts
			node[0] = SmallestNode1 (numhnodes);
			if (node[0] == -1)
				break;	// no more

			node[1] = SmallestNode1 (numhnodes);
			if (node[1] == -1)
				break;

			cin.h_count[numhnodes] = cin.h_count[node[0]] + cin.h_count[node[1]];
			numhnodes++;
		}

		cin.numhnodes1[prev] = numhnodes-1;
	}
}

/*
==================
Huff1Decompress
==================
*/
static cblock_t Huff1Decompress (cblock_t in)
{
	byte	*input;
	byte	*out_p;
	int		nodenum;
	int		count;
	cblock_t out;
	int		inbyte;
	int		*hnodes, *hnodesbase;

	// get decompressed count
	count = in.data[0] + (in.data[1]<<8) + (in.data[2]<<16) + (in.data[3]<<24);
	input = in.data + 4;
	out_p = out.data = Z_Malloc (count);

	// read bits

	hnodesbase = cin.hnodes1 - 256*2;	// nodes 0-255 aren't stored

	hnodes = hnodesbase;
	nodenum = cin.numhnodes1[0];
	while (count)
	{
		inbyte = *input++;
#define PROCESS_BIT	\
		if (nodenum < 256)	\
		{					\
			hnodes = hnodesbase + (nodenum<<9);	\
			*out_p++ = nodenum;					\
			if (!--count)	\
				break;		\
			nodenum = cin.numhnodes1[nodenum];	\
		}					\
		nodenum = hnodes[nodenum*2 + (inbyte&1)];	\
		inbyte >>=1;
		PROCESS_BIT;
		PROCESS_BIT;
		PROCESS_BIT;
		PROCESS_BIT;
		PROCESS_BIT;
		PROCESS_BIT;
		PROCESS_BIT;
		PROCESS_BIT;
#undef PROCESS_BIT
	}

	if (input - in.data != in.count && input - in.data != in.count+1)
	{
		Com_WPrintf ("Decompression overread by %d", (input - in.data) - in.count);
	}
	out.count = out_p - out.data;

	return out;
}

/*
==================
SCR_ReadNextFrame
==================
*/
byte *SCR_ReadNextFrame (void)
{
	int		command;
	byte	samples[22050/14*4];
	byte	compressed[0x20000];
	int		size;
	byte	*pic;
	cblock_t	in, huf1;
	int		start, end, count;

	// read the next frame
	FS_Read (&command, 4, cl.cinematic_file);
	command = LittleLong(command);
	if (command == 2)
		return NULL;	// last frame marker

	if (command == 1)
	{	// read palette
		FS_Read (cl.cinematicpalette, sizeof(cl.cinematicpalette), cl.cinematic_file);
		cl.cinematicpalette_active=0;	// dubious....  exposes an edge case
	}

	// decompress the next frame
	FS_Read (&size, 4, cl.cinematic_file);
	size = LittleLong(size);
	if (size > sizeof(compressed) || size < 1)
		Com_Error (ERR_DROP, "Bad compressed frame size");
	FS_Read (compressed, size, cl.cinematic_file);

	// read sound
	start = cl.cinematicframe*cin.s_rate/14;
	end = (cl.cinematicframe+1)*cin.s_rate/14;
	count = end - start;

	FS_Read (samples, count*cin.s_width*cin.s_channels, cl.cinematic_file);

	S_RawSamples (count, cin.s_rate, cin.s_width, cin.s_channels, samples);

	in.data = compressed;
	in.count = size;

	huf1 = Huff1Decompress (in);

	pic = huf1.data;

	cl.cinematicframe++;

	return pic;
}


/*
==================
SCR_RunCinematic
==================
*/
void SCR_RunCinematic (void)
{
	int		frame;

	if (cl.cinematictime <= 0)
	{
		SCR_StopCinematic ();
		return;
	}

	if (cl.cinematicframe == -1)
		return;		// static image

	if (cls.key_dest != key_game)
	{	// pause if menu or console is up
		cl.cinematictime = cls.realtime - cl.cinematicframe * 1000 / 14;
		return;
	}

	frame = (cls.realtime - cl.cinematictime) * 14.0 / 1000;
	if (frame <= cl.cinematicframe)
		return;
	if (frame > cl.cinematicframe+1)
	{
		Com_DPrintf ("Dropped frame: %d > %d\n", frame, cl.cinematicframe+1);
		cl.cinematictime = cls.realtime - cl.cinematicframe * 1000 / 14;
	}
	if (cin.pic)
		Z_Free (cin.pic);
	cin.pic = cin.pic_pending;
	cin.pic_pending = NULL;
	cin.pic_pending = SCR_ReadNextFrame ();
	if (!cin.pic_pending)
	{
		SCR_StopCinematic ();
		SCR_FinishCinematic ();
		cl.cinematictime = 1;	// hack to get the black screen behind loading
		SCR_BeginLoadingPlaque ();
		cl.cinematictime = 0;
		return;
	}
}

/*
==================
SCR_DrawCinematic

Returns true if a cinematic is active, meaning the view rendering
should be skipped
==================
*/
qboolean SCR_DrawCinematic (void)
{
	if (cl.cinematictime <= 0)
	{
		return false;
	}

	if (cls.key_dest == key_menu)
	{	// blank screen and pause if menu is up
		re.CinematicSetPalette(NULL);
		cl.cinematicpalette_active = false;
		return true;
	}

	if (cl.cinematicframe == -1)
	{	// static image
		re.DrawStretchPic (0, 0, viddef.width, viddef.height, cin.imageName);
		return true;
	}

	if (!cl.cinematicpalette_active)
	{
		re.CinematicSetPalette(cl.cinematicpalette);
		cl.cinematicpalette_active = true;
	}

	if (!cin.pic)
		return true;

	re.DrawStretchRaw (0, 0, viddef.width, viddef.height, cin.width, cin.height, cin.pic);

	return true;
}

/*
==================
SCR_PlayCinematic
==================
*/
void SCR_PlayCinematic (char *arg)
{
	int		width, height;
	char	*ext;
	int		old_khz;

	// make sure CD isn't playing music
	CDAudio_Stop();

	if (*re.flags & REF_CONSOLE_ONLY)
	{	// no cinematic for text-only mode
		SCR_FinishCinematic ();
		cl.cinematictime = 0;
		return;
	}


	cl.cinematicframe = 0;
	ext = strchr (arg, '.');

	if (ext && (!strcmp (ext, ".pcx") || !strcmp (ext, ".jpg") || !strcmp (ext, ".tga")))
	{	// static image
		Q_CopyFilename (cin.imageName, arg, sizeof(cin.imageName)-1);
		cin.imageName[ext - arg] = 0;		// cut extension
		re.DrawGetPicSize (&cin.width, &cin.height, cin.imageName);
		cl.cinematicframe = -1;			// flag signalling "draw pic"
		cl.cinematictime = 1;
		SCR_EndLoadingPlaque ();
		cls.state = ca_active;
		if (!cin.width)
		{
			Com_WPrintf ("%s not found.\n", arg);
			cl.cinematictime = 0;
		}
		return;
	}

	FS_FOpenFile (va("video/%s", arg), &cl.cinematic_file);
	if (!cl.cinematic_file)
	{
//		Com_Error (ERR_DROP, "Cinematic %s not found.\n", name);
		SCR_FinishCinematic ();
		cl.cinematictime = 0;	// done
		return;
	}

	SCR_EndLoadingPlaque ();

	cls.state = ca_active;

	FS_Read (&width, 4, cl.cinematic_file);
	FS_Read (&height, 4, cl.cinematic_file);
	cin.width = LittleLong(width);
	cin.height = LittleLong(height);

	FS_Read (&cin.s_rate, 4, cl.cinematic_file);
	cin.s_rate = LittleLong(cin.s_rate);
	FS_Read (&cin.s_width, 4, cl.cinematic_file);
	cin.s_width = LittleLong(cin.s_width);
	FS_Read (&cin.s_channels, 4, cl.cinematic_file);
	cin.s_channels = LittleLong(cin.s_channels);

	Huff1TableInit ();

	// switch up to 22 khz sound if necessary
	old_khz = Cvar_VariableInt ("s_khz");
	if (old_khz != cin.s_rate / 1000)
	{
		cin.restart_sound = true;
		Cvar_SetInteger ("s_khz", cin.s_rate / 1000);
		CL_Snd_Restart_f ();
		Cvar_SetInteger ("s_khz", old_khz);
	}

	cl.cinematicframe = 0;
	cin.pic = SCR_ReadNextFrame ();
	cl.cinematictime = Sys_Milliseconds ();
}
