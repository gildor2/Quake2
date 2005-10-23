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

//#define WAVEOUT_DRV	1	-- not works anyway

#include "WinPrivate.h"
#include <mmsystem.h>		// required for dsound.h and for WAVEOUT_DRV

// declaration required for new dsound.h header, used from VC6
typedef unsigned long DWORD_PTR, *PDWORD_PTR;
#include <dsound.h>

#include "../client/client.h"
#include "../client/snd_loc.h"

// 64K is > 1 second at 16-bit, 22050 Hz
#define	WAV_BUFFERS				64
#define	WAV_MASK				0x3F
#define	WAV_BUFFER_SIZE			0x0400
#define SECONDARY_BUFFER_SIZE	0x10000

static bool	dsound_init;
static bool	wav_init;
static bool	snd_firsttime = true, snd_isdirect;
static bool	primary_format_set;

// starts at 0 for disabled
static int	snd_buffer_count = 0;
static int	sample16;


//-------------- WaveOut ---------------------

#if WAVEOUT_DRV

static bool			snd_iswave;
static cvar_t		*s_wavonly;

static HPSTR		lpData;
static LPWAVEHDR	lpWaveHdr;

static HWAVEOUT		hWaveOut;
static WAVEOUTCAPS	wavecaps;

static int	snd_sent, snd_completed;

#endif


//-------------- DirectSound -----------------

static IDirectSound *pDS;
static IDirectSoundBuffer *pDSBuf, *pDSPBuf;
static HINSTANCE hInstDS;

typedef HRESULT (WINAPI * pDirectSoundCreate_t) (GUID FAR*, LPDIRECTSOUND FAR*, IUnknown FAR*);
static pDirectSoundCreate_t pDirectSoundCreate;

// common ...

static DWORD	gSndBufSize;
static MMTIME	mmstarttime;


// forwards
static void FreeSound ();

static const char *DSoundError (int error)
{
	switch (error)
	{
	case DSERR_BUFFERLOST:
		return "DSERR_BUFFERLOST";
	case DSERR_INVALIDCALL:
		return "DSERR_INVALIDCALLS";
	case DSERR_INVALIDPARAM:
		return "DSERR_INVALIDPARAM";
	case DSERR_PRIOLEVELNEEDED:
		return "DSERR_PRIOLEVELNEEDED";
	}

	return "unknown";
}


static bool DS_CreateBuffers ()
{
	DSBUFFERDESC	dsbuf;
	DSBCAPS			dsbcaps;
	WAVEFORMATEX	pformat, format;
	DWORD			dwWrite;

	memset (&format, 0, sizeof(format));
	format.wFormatTag = WAVE_FORMAT_PCM;
	format.nChannels = dma.channels;
	format.wBitsPerSample = dma.samplebits;
	format.nSamplesPerSec = dma.speed;
	format.nBlockAlign = format.nChannels * format.wBitsPerSample / 8;	// bytes per samples for all channels
	format.cbSize = 0;
	format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;	// bytes per sec for all channels

	appPrintf ("Creating DS buffers\n");

	Com_DPrintf ("...setting PRIORITY coop level: ");
	if (DS_OK != pDS->SetCooperativeLevel (cl_hwnd, DSSCL_PRIORITY))
	{
		appPrintf ("failed\n");
		FreeSound ();
		return false;
	}
	Com_DPrintf ("ok\n");

	// get access to the primary buffer, if possible, so we can set the
	// sound hardware format
	memset (&dsbuf, 0, sizeof(dsbuf));
	dsbuf.dwSize = sizeof(dsbuf);
	dsbuf.dwFlags = DSBCAPS_PRIMARYBUFFER;
	dsbuf.dwBufferBytes = 0;
	dsbuf.lpwfxFormat = NULL;

	memset (&dsbcaps, 0, sizeof(dsbcaps));
	dsbcaps.dwSize = sizeof(dsbcaps);
	primary_format_set = false;

	Com_DPrintf ("...creating primary buffer: ");
	if (DS_OK == pDS->CreateSoundBuffer (&dsbuf, &pDSPBuf, NULL))
	{
		pformat = format;

		Com_DPrintf ("ok\n");
		if (DS_OK != pDSPBuf->SetFormat (&pformat))
		{
			if (snd_firsttime)
				Com_DPrintf ("...setting primary sound format: failed\n");
		}
		else
		{
			if (snd_firsttime)
				Com_DPrintf ("...setting primary sound format: ok\n");

			primary_format_set = true;
		}
	}
	else
		appPrintf ("failed\n");

	if (!primary_format_set || !s_primary->integer)
	{
		// create the secondary buffer we'll actually work with
		memset (&dsbuf, 0, sizeof(dsbuf));
		dsbuf.dwSize = sizeof (DSBUFFERDESC);
		dsbuf.dwFlags = DSBCAPS_CTRLFREQUENCY|DSBCAPS_LOCSOFTWARE;
		dsbuf.dwBufferBytes = SECONDARY_BUFFER_SIZE;
		dsbuf.lpwfxFormat = &format;

		memset (&dsbcaps, 0, sizeof(dsbcaps));
		dsbcaps.dwSize = sizeof(dsbcaps);

		Com_DPrintf ("...creating secondary buffer: ");
		if (DS_OK != pDS->CreateSoundBuffer (&dsbuf, &pDSBuf, NULL))
		{
			appPrintf ("failed\n");
			FreeSound ();
			return false;
		}
		Com_DPrintf ("ok\n");

		dma.channels = format.nChannels;
		dma.samplebits = format.wBitsPerSample;
		dma.speed = format.nSamplesPerSec;

		if (DS_OK != pDSBuf->GetCaps (&dsbcaps))
		{
			appWPrintf ("*** GetCaps failed ***\n");
			FreeSound ();
			return false;
		}

		appPrintf ("...using secondary sound buffer\n");
	}
	else
	{
		appPrintf ("...using primary buffer\n");

		Com_DPrintf ("...setting WRITEPRIMARY coop level: ");
		if (DS_OK != pDS->SetCooperativeLevel (cl_hwnd, DSSCL_WRITEPRIMARY))
		{
			appPrintf ("failed\n");
			FreeSound ();
			return false;
		}
		Com_DPrintf ("ok\n");

		if (DS_OK != pDSPBuf->GetCaps (&dsbcaps))
		{
			appWPrintf ("*** GetCaps failed ***\n");
			return false;
		}

		pDSBuf = pDSPBuf;
	}

	// Make sure mixer is active
	pDSBuf->Play (0, 0, DSBPLAY_LOOPING);

	if (snd_firsttime)
		appPrintf ("   %d channel(s)\n"
				   "   %d bits/sample\n"
				   "   %d bytes/sec\n",
			dma.channels, dma.samplebits, dma.speed);

	gSndBufSize = dsbcaps.dwBufferBytes;

	/* we don't want anyone to access the buffer directly w/o locking it first. */
//	lpData = NULL;

	{//!!!
		unsigned long freq;

		pDSBuf->GetFrequency (&freq);
		pDSBuf->SetFrequency (appRound (freq * timescale->value));
	}

	pDSBuf->Stop ();
	pDSBuf->GetCurrentPosition (&mmstarttime.u.sample, &dwWrite);
	pDSBuf->Play (0, 0, DSBPLAY_LOOPING);

	dma.samples = gSndBufSize/(dma.samplebits/8);
	dma.samplepos = 0;
	dma.submission_chunk = 1;
	dma.buffer = NULL;	//?? (unsigned char *) lpData;
	sample16 = (dma.samplebits/8) - 1;

	return true;
}


static void DS_DestroyBuffers ()
{
	Com_DPrintf ("Destroying DS buffers\n");
	if (pDS)
	{
		Com_DPrintf ("...setting NORMAL coop level\n");
		pDS->SetCooperativeLevel (cl_hwnd, DSSCL_NORMAL);
	}

	if (pDSBuf)
	{
		Com_DPrintf ("...stopping and releasing sound buffer\n");
		pDSBuf->Stop ();
		pDSBuf->Release ();
	}

	// only release primary buffer if it's not also the mixing buffer we just released
	if (pDSPBuf && pDSBuf != pDSPBuf)
	{
		Com_DPrintf ("...releasing primary buffer\n");
		pDSPBuf->Release ();
	}
	pDSBuf = NULL;
	pDSPBuf = NULL;

	dma.buffer = NULL;
}


static void FreeSound ()
{
	appPrintf ("Shutting down sound system\n");

	if (pDS)
		DS_DestroyBuffers ();

#if WAVEOUT_DRV
	if (hWaveOut)
	{
		Com_DPrintf ("...resetting waveOut\n");
		waveOutReset (hWaveOut);

		if (lpWaveHdr)
		{
			Com_DPrintf ("...unpreparing headers\n");
			for (int i = 0; i < WAV_BUFFERS; i++)
				waveOutUnprepareHeader (hWaveOut, lpWaveHdr+i, sizeof(WAVEHDR));
		}

		Com_DPrintf ("...closing waveOut\n");
		waveOutClose (hWaveOut);

		if (lpWaveHdr)
			appFree (lpWaveHdr);
		if (lpData)
			appFree (lpData);
	}
#endif

	if (pDS)
	{
		Com_DPrintf ("...releasing DS object\n");
		pDS->Release ();
	}

	if (hInstDS)
	{
		Com_DPrintf ("...freeing DSOUND.DLL\n");
		FreeLibrary (hInstDS);
		hInstDS = NULL;
	}

	pDS         = NULL;
	pDSBuf      = NULL;
	pDSPBuf     = NULL;
	dsound_init = false;
#if WAVEOUT_DRV
	hWaveOut  = 0;
	lpData    = NULL;
	lpWaveHdr = NULL;
	wav_init  = false;
#endif
}

/*
==================
SNDDMA_InitDirect

Direct-Sound support
==================
*/
static bool SNDDMA_InitDirect ()
{
//	DSCAPS	dscaps;
	int		i;

	dma.channels = 2;
	dma.samplebits = 16;

	if (s_khz->integer == 44)
		dma.speed = 44100;
	else if (s_khz->integer == 22)
		dma.speed = 22050;
	else
		dma.speed = 11025;

	appPrintf ("Initializing DirectSound\n");

	if (!hInstDS)
	{
		Com_DPrintf ("...loading dsound.dll: ");

		hInstDS = LoadLibrary ("dsound.dll");

		if (hInstDS == NULL)
		{
			appPrintf ("failed\n");
			return false;
		}

		Com_DPrintf ("ok\n");
		pDirectSoundCreate = (pDirectSoundCreate_t)GetProcAddress (hInstDS, "DirectSoundCreate");

		if (!pDirectSoundCreate)
		{
			appWPrintf ("*** couldn't get DS proc addr ***\n");
			return false;
		}
	}


	Com_DPrintf ("...creating DS object: ");
	for (i = 0; /* have condition inside loop */; i++)
	{
		HRESULT hresult = pDirectSoundCreate (NULL, &pDS, NULL);
		if (hresult == DS_OK)
		{
			Com_DPrintf ("ok\n");
			break;
		}
		else
		{
			if (hresult != DSERR_ALLOCATED)
			{
				Com_DPrintf ("failed\n");
				return false;
			}

			if (i == 2)
			{
				appWPrintf ("failed, hardware already in use\n");
				return false;
			}
			Com_DPrintf ("retrying...\n");
			Sleep (2000);
		}
	}

/*	dscaps.dwSize = sizeof(dscaps);

	if (DS_OK != pDS->GetCaps (&dscaps))
		appPrintf ("*** couldn't get DS caps ***\n");

	if (dscaps.dwFlags & DSCAPS_EMULDRIVER)
	{
		Com_DPrintf ("...no DSound driver found\n");
		FreeSound ();
		return SIS_FAILURE;
	}
*/
	if (!DS_CreateBuffers ())
		return false;

	dsound_init = true;

	Com_DPrintf ("...completed successfully\n");

	return true;
}


#if WAVEOUT_DRV
/*
==================
SNDDM_InitWav

Crappy windows multimedia base
==================
*/
static bool SNDDMA_InitWav ()
{
	WAVEFORMATEX format;
	HRESULT	hr;

	appPrintf ("Initializing wave sound\n");

	snd_sent = 0;
	snd_completed = 0;

	dma.channels = 2;
	dma.samplebits = 16;

	if (s_khz->integer == 44)
		dma.speed = 44100;
	else if (s_khz->integer == 22)
		dma.speed = 22050;
	else
		dma.speed = 11025;

	memset (&format, 0, sizeof(format));
	format.wFormatTag = WAVE_FORMAT_PCM;
	format.nChannels = dma.channels;
	format.wBitsPerSample = dma.samplebits;
	format.nSamplesPerSec = dma.speed;
	format.nBlockAlign = format.nChannels * format.wBitsPerSample / 8;
	format.cbSize = 0;
	format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;

	/* Open a waveform device for output using window callback. */
	Com_DPrintf ("...opening waveform device: ");
	while ((hr = waveOutOpen((LPHWAVEOUT)&hWaveOut, WAVE_MAPPER,
					&format,
					0, 0L, CALLBACK_NULL)) != MMSYSERR_NOERROR)
	{
		if (hr != MMSYSERR_ALLOCATED)
		{
			appPrintf ("failed\n");
			return false;
		}

		if (MessageBox (NULL,
						"The sound hardware is in use by another app.\n\n"
					    "Select Retry to try to start sound again or Cancel to run "APPNAME" with no sound.",
						"Sound not available",
						MB_RETRYCANCEL | MB_SETFOREGROUND | MB_ICONEXCLAMATION) != IDRETRY)
		{
			appPrintf ("hw in use\n");
			return false;
		}
	}
	Com_DPrintf ("ok\n");

	gSndBufSize = WAV_BUFFERS*WAV_BUFFER_SIZE;
	lpData = (char*) appMalloc (gSndBufSize);
	memset (lpData, 0, gSndBufSize);

	lpWaveHdr = (LPWAVEHDR) appMalloc (sizeof(WAVEHDR) * WAV_BUFFERS);
	memset (lpWaveHdr, 0, sizeof(WAVEHDR) * WAV_BUFFERS);

	/* After allocation, set up and prepare headers. */
	Com_DPrintf ("...preparing headers: ");
	for (int i = 0; i < WAV_BUFFERS; i++)
	{
		lpWaveHdr[i].dwBufferLength = WAV_BUFFER_SIZE;
		lpWaveHdr[i].lpData = lpData + i*WAV_BUFFER_SIZE;

		if (waveOutPrepareHeader (hWaveOut, lpWaveHdr+i, sizeof(WAVEHDR)) != MMSYSERR_NOERROR)
		{
			appPrintf ("failed\n");
			FreeSound ();
			return false;
		}
	}
	Com_DPrintf ("ok\n");

	dma.samples = gSndBufSize / (dma.samplebits/8);
	dma.samplepos = 0;
	dma.submission_chunk = 512;
	dma.buffer = (unsigned char *) lpData;
	sample16 = (dma.samplebits/8) - 1;

	wav_init = true;

	return true;
}

#endif


/*
==================
SNDDMA_Init

Try to find a sound device to mix for.
Returns false if nothing is found.
==================
*/
bool SNDDMA_Init ()
{
	memset ((void *)&dma, 0, sizeof (dma));

#if WAVEOUT_DRV
	s_wavonly = Cvar_Get ("s_wavonly", "0");
#endif

	dsound_init = wav_init = 0;

	/*-------- Init DirectSound ----------*/
#if WAVEOUT_DRV
	if (!s_wavonly->integer)
#endif
	{
		if (snd_firsttime || snd_isdirect)
		{
			if (SNDDMA_InitDirect ())
			{
				snd_isdirect = true;

				if (snd_firsttime)
					appPrintf ("dsound init succeeded\n");
			}
			else
			{
				snd_isdirect = false;
				appPrintf ("*** dsound init failed ***\n");
			}
		}
	}

#if WAVEOUT_DRV
	// if DirectSound didn't succeed in initializing, try to initialize
	// waveOut sound, unless DirectSound failed because the hardware is
	// already allocated (in which case the user has already chosen not
	// to have sound)
	if (!dsound_init)
	{
		if (snd_firsttime || snd_iswave)
		{
			if (snd_iswave = SNDDMA_InitWav ())
			{
				if (snd_firsttime)
					appPrintf ("Wave sound init succeeded\n");
			}
			else
			{
				appPrintf ("Wave sound init failed\n");
			}
		}
	}
#endif

	snd_firsttime = false;

	snd_buffer_count = 1;

#if WAVEOUT_DRV
	if (!dsound_init && !wav_init)
#else
	if (!dsound_init)
#endif
	{
		if (snd_firsttime)
			appPrintf ("*** No sound device initialized ***\n");

		return false;
	}

	return true;
}

/*
==============
SNDDMA_GetDMAPos

return the current sample position (in mono samples read)
inside the recirculating dma buffer, so the mixing code will know
how many sample are required to fill it up.
===============
*/
int SNDDMA_GetDMAPos ()
{
	int		s;

	if (dsound_init)
	{
		MMTIME	mmtime;
		DWORD	dwWrite;

		mmtime.wType = TIME_SAMPLES;
		pDSBuf->GetCurrentPosition (&mmtime.u.sample, &dwWrite);
		s = mmtime.u.sample - mmstarttime.u.sample;
	}
#if WAVEOUT_DRV
	else if (wav_init)
	{
		s = snd_sent * WAV_BUFFER_SIZE;
	}
#endif


	s >>= sample16;

	s &= (dma.samples-1);

	return s;
}

/*
==============
SNDDMA_BeginPainting

Makes sure dma.buffer is valid
===============
*/
DWORD	locksize;
void SNDDMA_BeginPainting ()
{
	int		reps;
	DWORD	dwSize2;
	void	*pbuf, *pbuf2;
	HRESULT	hresult;
	DWORD	dwStatus;

	if (!pDSBuf) return;

	// if the buffer was lost or stopped, restore it and/or restart it
	if (pDSBuf->GetStatus (&dwStatus) != DS_OK)
		appPrintf ("Couldn't get sound buffer status\n");

	if (dwStatus & DSBSTATUS_BUFFERLOST)
		pDSBuf->Restore ();

	if (!(dwStatus & DSBSTATUS_PLAYING))
		pDSBuf->Play (0, 0, DSBPLAY_LOOPING);

	// lock the dsound buffer

	reps = 0;
	dma.buffer = NULL;

	while ((hresult = pDSBuf->Lock (0, gSndBufSize, &pbuf, &locksize, &pbuf2, &dwSize2, 0)) != DS_OK)
	{
		if (hresult != DSERR_BUFFERLOST)
		{
			appWPrintf ("S_TransferStereo16: Lock failed with error '%s'\n", DSoundError (hresult));
			S_Shutdown ();
			return;
		}
		else
		{
			pDSBuf->Restore ();
		}

		if (++reps > 2)
			return;
	}
	dma.buffer = (unsigned char *)pbuf;
}

/*
==============
SNDDMA_Submit

Send sound to device if buffer isn't really the dma buffer
Also unlocks the dsound buffer
===============
*/
void SNDDMA_Submit ()
{
	if (!dma.buffer)
		return;

	// unlock the dsound buffer
	if (pDSBuf)
		pDSBuf->Unlock (dma.buffer, locksize, NULL, 0);

#if WAVEOUT_DRV
	if (!wav_init)
		return;

	// find which sound blocks have completed
	while (1)
	{
		if (snd_completed == snd_sent)
		{
			Com_DPrintf ("Sound overrun\n");
			break;
		}

		if (! (lpWaveHdr[ snd_completed & WAV_MASK].dwFlags & WHDR_DONE))
		{
			break;
		}

		snd_completed++;	// this buffer has been played
	}

//appPrintf ("completed %i\n", snd_completed);
	//
	// submit a few new sound blocks
	//
	while (((snd_sent - snd_completed) >> sample16) < 8)
	{
		LPWAVEHDR	h;
		int			wResult;

		h = lpWaveHdr + ( snd_sent&WAV_MASK );
		if (paintedtime/256 <= snd_sent)
		break;	//	appPrintf ("submit overrun\n");
//appPrintf ("send %i\n", snd_sent);
		snd_sent++;
		/*
		 * Now the data block can be sent to the output device. The
		 * waveOutWrite function returns immediately and waveform
		 * data is sent to the output device in the background.
		 */
		wResult = waveOutWrite (hWaveOut, h, sizeof(WAVEHDR));

		if (wResult != MMSYSERR_NOERROR)
		{
			appPrintf ("Failed to write block to device\n");
			FreeSound ();
			return;
		}
	}
#endif
}

/*
==============
SNDDMA_Shutdown

Reset the sound device for exiting
===============
*/
void SNDDMA_Shutdown ()
{
	FreeSound ();
}


/*
===========
S_Activate

Called when the main window gains or loses focus.
The window have been destroyed and recreated
between a deactivate and an activate.
===========
*/
void S_Activate (bool active)
{
	if (pDS && cl_hwnd && snd_isdirect)
	{
		if (active)
			DS_CreateBuffers();
		else
			DS_DestroyBuffers();
	}
}
