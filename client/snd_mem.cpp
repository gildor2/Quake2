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
// snd_mem.cpp -- sound caching

#include "client.h"
#include "snd_loc.h"

int			cache_full_cycle;

// forwards
static void GetWavinfo (wavinfo_t &info, char *name, byte *wav, int wavlength);


static void ResampleSfx (sfx_t *sfx, int inrate, int inwidth, byte *data)
{
	guard(ResampleSfx);

	int		outcount;
	int		srcsample;
	float	stepscale;
	int		i;
	int		sample, samplefrac, fracstep;
	sfxcache_t	*sc;

	if (!(sc = sfx->cache)) return;
	stepscale = (float)inrate / dma.speed;	// this is usually 0.5, 1, or 2

	outcount = appRound (sc->length / stepscale);
	sc->length = outcount;
	if (sc->loopstart != -1)
		sc->loopstart = appRound (sc->loopstart / stepscale);

	sc->speed = dma.speed;
	if (s_loadas8bit->integer)
		sc->width = 1;
	else
		sc->width = inwidth;
	sc->stereo = 0;

	// resample / decimate to the current source rate

	if (stepscale == 1 && inwidth == 1 && sc->width == 1)
	{
		// fast special case
		for (i = 0; i < outcount; i++)
			((signed char *)sc->data)[i] = (int)((unsigned char)(data[i]) - 128);
	}
	else
	{
		// general case
		samplefrac = 0;
		fracstep = appFloor (stepscale*256);
		for (i = 0; i < outcount; i++)
		{
			srcsample = samplefrac >> 8;
			samplefrac += fracstep;
			if (inwidth == 2)
				sample = LittleShort ( ((short *)data)[srcsample] );
			else
				sample = (int)( (unsigned char)(data[srcsample]) - 128) << 8;
			if (sc->width == 2)
				((short *)sc->data)[i] = sample;
			else
				((signed char *)sc->data)[i] = sample >> 8;
		}
	}

	unguard;
}

//=============================================================================

sfxcache_t *S_LoadSound (sfx_t *s)
{
	guard(S_LoadSound);

	char		namebuffer[MAX_QPATH], *name;
	byte		*data;
	int			len;
	unsigned	size;
	float		stepscale;
	sfxcache_t	*sc;

	if (s->Name[0] == '*')
		return NULL;

	// see if still in memory
	sc = s->cache;
	if (sc) return sc;

	// see if already tryed to load, but file not found
	if (s->absent) return NULL;

	// load it in
	name = s->TrueName[0] ? s->TrueName : s->Name;

	if (name[0] == '#')
		strcpy(namebuffer, &name[1]);
	else
		appSprintf (ARRAY_ARG(namebuffer), "sound/%s", name);

	if (!(data = (byte*) GFileSystem->LoadFile (namebuffer, &size)))
	{
		Com_DPrintf ("Couldn't load %s\n", namebuffer);
		s->absent = true;
		return NULL;
	}

	wavinfo_t info;
	GetWavinfo (info, s->Name, data, size);
	if (info.channels != 1)
	{
		appPrintf ("%s is a stereo sample\n", *s->Name);
		delete data;
		return NULL;
	}

	// compute size of buffer for resampled sfx
	stepscale = (float)info.rate / dma.speed;
	len = appRound (info.samples / stepscale);
	len = len * info.width * info.channels;

	// allocate memory for sfx info and data
	sc = s->cache = (sfxcache_t*)appMalloc (len + sizeof(sfxcache_t));
	if (!sc)
	{
		delete data;
		return NULL;
	}

	sc->length    = info.samples;
	sc->loopstart = info.loopstart;
	sc->speed     = info.rate;
	sc->width     = info.width;
	sc->stereo    = info.channels;

	// resample sfx: tune its rate and stereo/mono
	ResampleSfx (s, sc->speed, sc->width, data + info.dataofs);

	delete data;

	return sc;

	unguardf(("%s", *s->Name));
}



/*
===============================================================================

WAV loading

===============================================================================
*/


//?? use mem stream ?
static byte	*data_p;
static byte 	*iff_end;
static byte 	*last_chunk;
static byte 	*iff_data;
static int 	iff_chunk_len;

static short GetLittleShort(void)
{
	short val = 0;
	val = *data_p;
	val = val + (*(data_p+1)<<8);
	data_p += 2;
	return val;
}

static int GetLittleLong(void)
{
	int val = 0;
	val = *data_p;
	val = val + (*(data_p+1)<<8);
	val = val + (*(data_p+2)<<16);
	val = val + (*(data_p+3)<<24);
	data_p += 4;
	return val;
}

static void FindNextChunk (char *name)
{
	while (true)
	{
		data_p = last_chunk;

		if (data_p >= iff_end)
		{	// didn't find the chunk
			data_p = NULL;
			return;
		}

		data_p += 4;
		iff_chunk_len = GetLittleLong();
		if (iff_chunk_len < 0)
		{
			data_p = NULL;
			return;
		}
//		if (iff_chunk_len > 1024*1024)
//			Com_FatalError ("FindNextChunk: %i length is past the 1 meg sanity limit", iff_chunk_len);
		data_p -= 8;
		last_chunk = data_p + 8 + Align (iff_chunk_len, 2);
		if (!memcmp (data_p, name, 4)) return;
	}
}

static void FindChunk(char *name)
{
	last_chunk = iff_data;
	FindNextChunk (name);
}


#if 0
static void DumpChunks(void)
{
	char	str[5];

	str[4] = 0;
	data_p=iff_data;
	do
	{
		memcpy (str, data_p, 4);
		data_p += 4;
		iff_chunk_len = GetLittleLong();
		appPrintf ("0x%X : %s (%d)\n", (int)(data_p - 4), str, iff_chunk_len);
		data_p += Align (iff_chunk_len, 2);
	} while (data_p < iff_end);
}
#endif

static void GetWavinfo (wavinfo_t &info, char *name, byte *wav, int wavlength)
{
	int     i;
	int     format;
	int		samples;

	memset (&info, 0, sizeof(info));
	if (!wav) return;

	iff_data = wav;
	iff_end = wav + wavlength;

// find "RIFF" chunk
	FindChunk ("RIFF");
	if (!(data_p && !memcmp (data_p+8, "WAVE", 4)))
	{
		appWPrintf("Missing RIFF/WAVE chunks\n");
		return;
	}

// get "fmt " chunk
	iff_data = data_p + 12;
// DumpChunks ();

	FindChunk ("fmt ");
	if (!data_p)
	{
		appWPrintf("Missing fmt chunk\n");
		return;
	}
	data_p += 8;
	format = GetLittleShort();
	if (format != 1)
	{
		appWPrintf("Microsoft PCM format only\n");
		return;
	}

	info.channels = GetLittleShort();
	info.rate = GetLittleLong();
	data_p += 4+2;
	info.width = GetLittleShort() / 8;

	// get cue chunk
	FindChunk("cue ");
	if (data_p)
	{
		data_p += 32;
		info.loopstart = GetLittleLong();
//		appPrintf("loopstart=%d\n", sfx->loopstart);

		// if the next chunk is a LIST chunk, look for a cue length marker
		FindNextChunk ("LIST");
		if (data_p)
		{
			if (!memcmp (data_p + 28, "mark", 4))
			{	// this is not a proper parse, but it works with cooledit...
				data_p += 24;
				i = GetLittleLong ();	// samples in loop
				info.samples = info.loopstart + i;
//				appPrintf("looped length: %i\n", i);
			}
		}
	}
	else
		info.loopstart = -1;

	// find data chunk
	FindChunk("data");
	if (!data_p)
	{
		appWPrintf("Missing data chunk\n");
		return;
	}

	data_p += 4;
	samples = GetLittleLong () / info.width;

	if (info.samples)
	{
		if (samples < info.samples)
			Com_DropError ("Sound %s has a bad loop length", name);
	}
	else
		info.samples = samples;

	info.dataofs = data_p - wav;
}
