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

#include "WinPrivate.h"
#include <mmsystem.h>
#include "../client/client.h"

static bool cdValid     = false;
static bool	playing     = false;
static bool	wasPlaying  = false;
static bool	initialized = false;
static bool	enabled     = false;
static bool playLooping = false;
static byte remap[100];
static byte	playTrack;
static byte	maxTrack;

static cvar_t *cd_nocd;
static cvar_t *cd_loopcount;
static cvar_t *cd_looptrack;

static UINT	wDeviceID;
static int	loopcounter;

// forwards
static void CDAudio_Pause ();


static void mciCommand (DWORD cmd, DWORD arg, const char *str = NULL)
{
	if (DWORD dwReturn = mciSendCommand(wDeviceID, cmd, arg, (DWORD)NULL))
	{
		if (str) Com_DPrintf ("CDAudio: %s failed (%d)\n", str, dwReturn);
	}
}

static bool mciStatus (DWORD arg, MCI_STATUS_PARMS &parm)
{
	if (DWORD dwReturn = mciSendCommand (wDeviceID, MCI_STATUS, arg, (DWORD)&parm))
	{
		Com_DPrintf ("CDAudio: STATUS failed (%d)\n", dwReturn);
		return false;
	}
	return true;
}


inline void CDAudio_Eject ()
{
	mciCommand (MCI_SET, MCI_SET_DOOR_OPEN, "SET_DOOR_OPEN");
}


inline void CDAudio_CloseDoor ()
{
	mciCommand (MCI_SET, MCI_SET_DOOR_CLOSED, "SET_DOOR_CLOSED");
}


static bool CDAudio_GetAudioDiskInfo ()
{
	MCI_STATUS_PARMS	mciStatusParms;

	cdValid = false;

	// check drive ready
	mciStatusParms.dwItem = MCI_STATUS_READY;
	if (!mciStatus (MCI_STATUS_ITEM|MCI_WAIT, mciStatusParms))
		return false;
	// verify result of command
	if (!mciStatusParms.dwReturn)
	{
		Com_DPrintf ("CDAudio: drive not ready\n");
		return false;
	}

	// get number of tracks
	mciStatusParms.dwItem = MCI_STATUS_NUMBER_OF_TRACKS;
	if (!mciStatus (MCI_STATUS_ITEM|MCI_WAIT, mciStatusParms))
		return false;
	// verify result of command
	if (mciStatusParms.dwReturn < 1)
	{
		Com_DPrintf ("CDAudio: no music tracks\n");
		return false;
	}

	cdValid  = true;
	maxTrack = mciStatusParms.dwReturn;

	return true;
}



static void CDAudio_Play2 (int track, bool looping)
{
	MCI_STATUS_PARMS mciStatusParms;

	if (!enabled)
		return;

	if (!cdValid)
	{
		if (!CDAudio_GetAudioDiskInfo ()) return;
	}

	track = remap[track];
	if (track < 1 || track > maxTrack)
	{
		CDAudio_Stop();
		return;
	}

	if (playing && playTrack == track) return;		// desired track currently played

	// don't try to play a non-audio track
	mciStatusParms.dwItem  = MCI_CDA_STATUS_TYPE_TRACK;
	mciStatusParms.dwTrack = track;
	if (!mciStatus (MCI_STATUS_ITEM|MCI_TRACK|MCI_WAIT, mciStatusParms))
		return;
	if (mciStatusParms.dwReturn != MCI_CDA_TRACK_AUDIO)
	{
		appPrintf ("CDAudio: track %d is not audio\n", track);
		return;
	}

	// get the length of the track to be played
	mciStatusParms.dwItem  = MCI_STATUS_LENGTH;
	mciStatusParms.dwTrack = track;
	if (!mciStatus (MCI_STATUS_ITEM|MCI_TRACK|MCI_WAIT, mciStatusParms))
		return;

	CDAudio_Stop();									// stop current playback
	MCI_PLAY_PARMS mciPlayParms;
	mciPlayParms.dwFrom     = MCI_MAKE_TMSF (track, 0, 0, 0);							// from = track
	mciPlayParms.dwTo       = MCI_MAKE_TMSF (track, mciStatusParms.dwReturn, 0, 0);		// to   = end of track
	mciPlayParms.dwCallback = (DWORD)cl_hwnd;
	if (DWORD dwReturn = mciSendCommand (wDeviceID, MCI_PLAY, MCI_FROM|MCI_TO|MCI_NOTIFY, (DWORD)&mciPlayParms))
	{
		Com_DPrintf ("CDAudio: PLAY failed (%d)\n", dwReturn);
		return;
	}

	playLooping = looping;
	playTrack   = track;
	playing     = true;

	if (cd_nocd->integer) CDAudio_Pause ();
}


void CDAudio_Play (int track, bool looping)
{
	// set a loop counter so that this track will change to the
	// looptrack later
	loopcounter = 0;
	CDAudio_Play2 (track, looping);
}

void CDAudio_Stop ()
{
	if (!enabled || !playing) return;

	mciCommand (MCI_STOP, 0, "STOP");
	wasPlaying = false;
	playing    = false;
}


static void CDAudio_Pause ()
{
	if (!enabled || !playing) return;

	MCI_GENERIC_PARMS mciGenericParms;
	mciGenericParms.dwCallback = (DWORD)cl_hwnd;
	if (DWORD dwReturn = mciSendCommand (wDeviceID, MCI_PAUSE, 0, (DWORD)&mciGenericParms))
		Com_DPrintf("CDAudio: PAUSE failed (%d)", dwReturn);

	wasPlaying = playing;
	playing = false;
}


static void CDAudio_Resume ()
{
	if (!enabled || !cdValid || !wasPlaying) return;

	MCI_PLAY_PARMS mciPlayParms;
	mciPlayParms.dwFrom     = MCI_MAKE_TMSF (playTrack, 0, 0, 0);		// from = track (unused: no MCI_FROM in flags -- using current pos)
	mciPlayParms.dwTo       = MCI_MAKE_TMSF (playTrack + 1, 0, 0, 0);	// to   = next track
	mciPlayParms.dwCallback = (DWORD)cl_hwnd;
	if (DWORD dwReturn = mciSendCommand (wDeviceID, MCI_PLAY, MCI_TO|MCI_NOTIFY, (DWORD)&mciPlayParms))
	{
		Com_DPrintf ("CDAudio: PLAY failed (%d)\n", dwReturn);
		return;
	}
	playing = true;
}


static void CD_f (int argc, char **argv)
{
	if (argc < 2) return;

	const char *command = argv[1];

	if (!stricmp (command, "on"))
	{
		enabled = true;
		return;
	}

	if (!stricmp (command, "off"))
	{
		if (playing)
			CDAudio_Stop();
		enabled = false;
		return;
	}

	if (!stricmp (command, "reset"))
	{
		enabled = true;
		if (playing)
			CDAudio_Stop();
		for (int n = 0; n < 100; n++)
			remap[n] = n;
		CDAudio_GetAudioDiskInfo ();
		return;
	}

	if (!stricmp (command, "remap"))
	{
		int ret = argc - 2;
		if (ret <= 0)
		{
			for (int n = 1; n < 100; n++)
				if (remap[n] != n)
					appPrintf ("  %d -> %d\n", n, remap[n]);
			return;
		}
		for (int n = 1; n <= ret; n++)
			remap[n] = atoi (argv[n+1]);
		return;
	}

	if (!strcmp (command, "close"))
	{
		CDAudio_CloseDoor ();
		return;
	}

	if (!cdValid)
	{
		if (!CDAudio_GetAudioDiskInfo ())
		{
			appPrintf ("No CD in player.\n");
			return;
		}
	}

	if (!stricmp (command, "play"))
	{
		CDAudio_Play (atoi (argv[2]), false);
		return;
	}

	if (!stricmp (command, "loop"))
	{
		CDAudio_Play (atoi (argv[2]), true);
		return;
	}

	if (!stricmp (command, "stop"))
	{
		CDAudio_Stop ();
		return;
	}

	if (!stricmp (command, "pause"))
	{
		CDAudio_Pause ();
		return;
	}

	if (!stricmp (command, "resume"))
	{
		CDAudio_Resume ();
		return;
	}

	if (!stricmp (command, "eject"))
	{
		if (playing)
			CDAudio_Stop ();
		CDAudio_Eject ();
		cdValid = false;
		return;
	}

	if (!stricmp (command, "info"))
	{
		appPrintf ("%d tracks\n", maxTrack);
		if (playing)
			appPrintf ("Currently %s track %d\n", playLooping ? "looping" : "playing", playTrack);
		else if (wasPlaying)
			appPrintf ("Paused %s track %d\n", playLooping ? "looping" : "playing", playTrack);
		return;
	}
}


static bool CDAudio_MsgHook (UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg != MM_MCINOTIFY || lParam != wDeviceID)
		return false;

	switch (wParam)
	{
		case MCI_NOTIFY_SUCCESSFUL:
			if (playing)
			{
				playing = false;
				if (playLooping)
				{
					// if the track has played the given number of times,
					// go to the ambient track
					if (++loopcounter >= cd_loopcount->integer)
						CDAudio_Play2 (cd_looptrack->integer, true);
					else
						CDAudio_Play2 (playTrack, true);
				}
			}
			break;

		case MCI_NOTIFY_ABORTED:
		case MCI_NOTIFY_SUPERSEDED:
			break;

		case MCI_NOTIFY_FAILURE:
			Com_DPrintf ("MCI_NOTIFY_FAILURE\n");
			CDAudio_Stop ();
			cdValid = false;
			break;

		default:
			Com_DPrintf ("Unexpected MM_MCINOTIFY type (%d)\n", wParam);
	}

	return false;
}


void CDAudio_Update ()
{
	if (cd_nocd->integer != !enabled)
	{
		if (cd_nocd->integer)
		{
			CDAudio_Stop ();
			enabled = false;
		}
		else
		{
			enabled = true;
			CDAudio_Resume ();
		}
	}
}


bool CDAudio_Init ()
{
	DWORD	dwReturn;

CVAR_BEGIN(vars)
	CVAR_VAR(cd_nocd, 0, CVAR_ARCHIVE),
	CVAR_VAR(cd_loopcount, 4, 0),
	CVAR_VAR(cd_looptrack, 11, 0)
CVAR_END

	Cvar_GetVars (ARRAY_ARG(vars));
	if (cd_nocd->integer)
		return false;

	AddMsgHook (CDAudio_MsgHook);

	MCI_OPEN_PARMS mciOpenParms;
	mciOpenParms.lpstrDeviceType = "cdaudio";
	if (dwReturn = mciSendCommand (0, MCI_OPEN, MCI_OPEN_TYPE|MCI_OPEN_SHAREABLE, (DWORD)&mciOpenParms))
	{
		Com_DPrintf ("CDAudio_Init: OPEN failed (%d)\n", dwReturn);
		return false;
	}
	wDeviceID = mciOpenParms.wDeviceID;

	// Set the time format to track/minute/second/frame (TMSF).
	MCI_SET_PARMS mciSetParms;
	mciSetParms.dwTimeFormat = MCI_FORMAT_TMSF;
	if (dwReturn = mciSendCommand (wDeviceID, MCI_SET, MCI_SET_TIME_FORMAT, (DWORD)&mciSetParms))
	{
		Com_DPrintf ("CDAudio: SET_TIME_FORMAT failed (%d)\n", dwReturn);
		mciCommand (MCI_CLOSE, 0);
		return false;
	}

	for (int n = 0; n < 100; n++)
		remap[n] = n;
	initialized = true;
	enabled     = true;

	if (!CDAudio_GetAudioDiskInfo ())
	{
//		appPrintf("CDAudio_Init: No CD in player.\n");
		enabled = false;
	}

	RegisterCommand ("cd", CD_f);

	appPrintf("CD Audio Initialized\n");

	return true;
}


void CDAudio_Shutdown ()
{
	if (!initialized) return;
	CDAudio_Stop ();
	mciCommand (MCI_CLOSE, MCI_WAIT, "CLOSE");
}


// Called when the main window gains or loses focus. The window have been
// destroyed and recreated between a deactivate and an activate.
void CDAudio_Activate (bool active)
{
	if (active)
		CDAudio_Resume ();
	else
		CDAudio_Pause ();
}
