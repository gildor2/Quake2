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

#include "winquake.h"
#include "Core.h"

#include <direct.h>

#include "../client/client.h"		//!! for editLine[] and CompleteCommand() only

#define CRASH_LOG	"crash.log"		//!! required for history logging (should move to Core ??!)

int			starttime;
bool		ActiveApp, Minimized;

static HANDLE hinput, houtput;

unsigned	sys_msg_time;
unsigned	sys_frame_time;


/*
===============================================================================

SYSTEM IO

===============================================================================
*/


void Sys_Quit (void)
{
#ifndef DEDICATED_ONLY
	if (DEDICATED) FreeConsole ();
#endif
	appExit ();
	exit (0);
}


//================================================================


/*
================
Sys_ScanForCD

================
*/
static char *Sys_ScanForCD (void)
{
#ifdef CD_PATH
	static char cddir2[MAX_OSPATH], *cddir;
	static bool done = false;
	static char drive[4] = "c:/";

	if (done) return cddir;		// don't re-check
	done = true;

	// no abort/retry/fail errors
	SetErrorMode (SEM_FAILCRITICALERRORS);

	// scan the drives
	for (drive[0] = 'c'; drive[0] <= 'z'; drive[0]++)
	{
		if (GetDriveType (drive) != DRIVE_CDROM) continue;
		if (FILE *f = fopen (va("%s"CD_CHECK, drive), "r"))
		{
			fclose (f);
			appCopyFilename (cddir2, va("%s"CD_PATH, drive), sizeof(cddir));
			cddir = cddir2;
			return cddir;
		}
	}
#endif
	return NULL;
}

/*
================
Sys_CopyProtect

================
*/
void Sys_CopyProtect (void)
{
#ifdef CD_PATH
	if (!Sys_ScanForCD ())
		// FUNNY NOTE: FatalError() will provide ErrorHistory with path to Sys_CopyProtect()
		Com_FatalError ("You must have the "APPNAME" CD in the drive to play.");
#endif
}


/*
================
Sys_Init
================
*/
void Sys_Init (void)
{
	guard(Sys_Init);

	if (DEDICATED)
	{
#ifndef DEDICATED_ONLY
		AllocConsole ();
#endif
		hinput = GetStdHandle (STD_INPUT_HANDLE);
		houtput = GetStdHandle (STD_OUTPUT_HANDLE);
	}

	unguard;
}


#define MAX_CMDLINE		256		//?? same as in client/keys.h

static char	console_text[MAX_CMDLINE];
static int	console_textlen = 0;
static bool console_drawInput = true;

static void EraseConInput (void)
{
	if (!console_drawInput)
	{
		char	text[MAX_CMDLINE+2];
		DWORD	dummy;

		text[0] = '\r';
		memset (&text[1], ' ', console_textlen+1);
		text[console_textlen+2] = '\r';
		text[console_textlen+3] = 0;
		WriteFile (houtput, text, console_textlen+3, &dummy, NULL);
	}
}

/*
================
Sys_ConsoleInput
================
*/
char *Sys_ConsoleInput (void)
{
	INPUT_RECORD rec;
	int		ch, len;
	DWORD	numevents, dummy, numread;
	char	*s;

	guard(Sys_ConsoleInput);

	if (console_drawInput)
	{
		// display input line
		WriteFile (houtput, "]", 1, &dummy, NULL);
		if (console_textlen)
			WriteFile (houtput, console_text, console_textlen, &dummy, NULL);
		console_drawInput = false;
	}

	while (true)
	{
		GetNumberOfConsoleInputEvents (hinput, &numevents);
		if (numevents <= 0) break;
		ReadConsoleInput (hinput, &rec, 1, &numread);

		if (rec.EventType == KEY_EVENT && rec.Event.KeyEvent.bKeyDown)
		{
			ch = rec.Event.KeyEvent.uChar.AsciiChar;
			switch (ch)
			{
				case '\r':		// ENTER
					WriteFile (houtput, "\r\n", 2, &dummy, NULL);
					console_drawInput = true;
					if (console_textlen)
					{
						console_textlen = 0;
						return console_text;
					}
					break;

				case '\b':		// BACKSPACE
					if (console_textlen)
					{
						console_text[--console_textlen] = 0;
						WriteFile (houtput, "\b \b", 3, &dummy, NULL);
					}
					break;

				case '\t':		// TAB
					appSprintf (ARRAY_ARG(editLine), "]%s", console_text);
					CompleteCommand ();
					s = editLine;
					if (s[0] == ']') s++;
					if (s[0] == '/') s++;
					len = strlen (s);
					if (len > 0)
					{
						console_textlen = min(len, sizeof(console_text)-2);
						appStrncpyz (console_text, s, console_textlen+1);
						Sys_ConsoleOutput ("");		// update line
					}
					break;

				case '\x1B':	// ESC
					EraseConInput ();
					console_textlen = 0;
					console_text[0] = 0;
					break;

				default:
					if (ch >= ' ')
					{
						if (console_textlen < sizeof(console_text)-2)
						{
							WriteFile(houtput, &ch, 1, &dummy, NULL);
							console_text[console_textlen++] = ch;
							console_text[console_textlen] = 0;
						}
					}
//					else
//						Com_Printf ("%2X\n",ch);
					break;
			}
		}
	}

	return NULL;

	unguard;
}


/*
================
Sys_ConsoleOutput

Print text to the dedicated console
================
*/
void Sys_ConsoleOutput (const char *string)
{
	DWORD	dummy;
	char	c;

	if (!DEDICATED) return;

	EraseConInput ();

	// draw message
	while (c = string[0])
	{
		// parse color info
		if (c == COLOR_ESCAPE && string[1] >= '0' && string[1] <= '7')
		{
			static const byte colorTable[8] = {0, 4, 2, 6, 1, 5, 3, 7};
			SetConsoleTextAttribute (houtput, colorTable[string[1] - '0']);
			string += 2;
			continue;
		}
		c &= 0x7F;		// clear 7th bit
		WriteFile (houtput, &c, 1, &dummy, NULL);
		string++;
	}
	SetConsoleTextAttribute (houtput, 7);

	console_drawInput = true;
}


/*
================
Sys_SendKeyEvents

Send Key_Event calls
================
*/
void Sys_SendKeyEvents (void)
{
#ifndef DEDICATED_ONLY
	MSG		msg;

	guard(Sys_SendKeyEvents);
	while (PeekMessage (&msg, NULL, 0, 0, PM_REMOVE))
	{
		if (msg.message == WM_QUIT)
			Com_Quit ();
		sys_msg_time = appMilliseconds ();	//?? msg.time;

		guard(TranslateMessage);
		TranslateMessage (&msg);
		unguardf(("msg=%X", msg.message));

		guard(DispatchMessage);
		DispatchMessage (&msg);
		unguardf(("msg=%X", msg.message));
	}

	// grab frame time
	sys_frame_time = appMilliseconds ();	// FIXME: should this be at start?
	unguard;
#endif
}



/*
========================================================================

GAME DLL

========================================================================
*/

static HINSTANCE game_library;

/*
=================
Sys_UnloadGame
=================
*/
void Sys_UnloadGame (void)
{
	guard(Sys_UnloadGame);
	if (!FreeLibrary (game_library))
		Com_FatalError ("Cannot unload game library");
	Com_DPrintf ("FreeLibrary(game.dll)\n");
	game_library = NULL;
	unguard;
}

/*
=================
Sys_GetGameAPI

Loads the game dll
=================
*/
void *Sys_GetGameAPI (void *parms)
{
	typedef void * (* pGetGameApi_t)(void *);
	pGetGameApi_t pGetGameAPI;
	char	name[MAX_OSPATH], *path;
#if defined _M_IX86
	const char *gamename = "gamex86.dll";
#elif defined _M_ALPHA
	const char *gamename = "gameaxp.dll";
#endif

	guard(Sys_GetGameAPI);

	if (game_library)
		Com_FatalError ("Sys_GetGameAPI() without Sys_UnloadGame()");

	// run through the search paths
	path = NULL;
	while (path = FS_NextPath (path))
	{
		appSprintf (ARRAY_ARG(name), "%s/%s", path, gamename);
		if (FILE *f = fopen (name, "rb"))	// check file presence
		{
			fclose (f);
			if (game_library = LoadLibrary (name))
			{
				Com_DPrintf ("LoadLibrary (%s)\n",name);
				break;
			}
			else
				Com_WPrintf ("Sys_GetGameAPI(%s): failed to load library\n", name);
		}
		else
			Com_DPrintf ("Sys_GetGameAPI(%s): file not found\n", name);
	}
	if (!game_library)
		return NULL;		// couldn't find one anywhere

	pGetGameAPI = (pGetGameApi_t)GetProcAddress (game_library, "GetGameAPI");
	if (!pGetGameAPI)
	{
		Sys_UnloadGame ();
		return NULL;
	}

	guard(GetGameAPI);
	return pGetGameAPI (parms);
	unguard;

	unguard;
}

//=======================================================================

extern bool debugLogged;

/*
==================
WinMain

==================
*/
HINSTANCE	global_hInstance;

#ifndef DEDICATED_ONLY
int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#else
int main (int argc, const char **argv)
#endif
{
	GUARD_BEGIN
	{
#ifndef DEDICATED_ONLY
		global_hInstance = hInstance;
		const char *cmdline = lpCmdLine;
#else
		global_hInstance = GetModuleHandle (NULL);
		const char *cmdline = GetCommandLine ();
#endif

#ifdef CD_PATH
		char cmdline2[1024];
		// if we find the CD, add a "-cddir=xxx" command line
		if (char *cddir = Sys_ScanForCD ())
		{
			// add to the end of cmdline, so, if already specified - will not override option
			appSprintf (ARRAY_ARG(cmdline2), "%s -cddir=\"%s\"", cmdline, cddir);
			cmdline = cmdline2;
		}
#endif

		appInit ();		//!!!!
		Com_Init (cmdline);

		double oldtime = appMillisecondsf ();

		/*--------- main window message loop ------------*/
		guard(MainLoop);
		while (true)
		{
#ifndef DEDICATED_ONLY
			if (!ActiveApp || DEDICATED)
				Sleep (10);		//?? what about client and server in one place: will server become slower ?

			MSG		msg;
			while (PeekMessage (&msg, NULL, 0, 0, PM_REMOVE))
			{
				if (msg.message == WM_QUIT) Com_Quit ();
				sys_msg_time = appMilliseconds ();	//?? msg.time;

				guard(TranslateMessage);
				TranslateMessage (&msg);
				unguardf(("msg=%X", msg.message));

				guard(DispatchMessage);
				DispatchMessage (&msg);
				unguardf(("msg=%X", msg.message));
			}
#else
			Sleep (10);
#endif

			// do not allow Com_Frame(0)
			double timeDelta, newtime;
			while (true)
			{
				newtime = appMillisecondsf ();	//?? can use appCycles() for measuring time delta, but (currently) this is Pentium-only
				timeDelta = newtime - oldtime;
				if (timeDelta > 1) break;		//?? client (or server?) time bugs with ">0" condition & cl_maxfps!=0 -- net/prediction errors
				Sleep (0);
			}
			GUARD_BEGIN
			{
				Com_Frame (timeDelta);
			}
			GUARD_CATCH
			{
				if (GErr.nonFatalError)
				{
					//!! TEST: dedicated: error -drop, error -gpf, error "msg"
					//!! check old behaviour on DropError(NULL)
					SV_Shutdown (va("Server crashed: %s\n", GErr.message), false);	// message
					Com_DPrintf ("History: %s\n", GErr.history);					// history
					if (!DEDICATED) CL_Drop (true);
					GErr.Reset ();
				}
				else
					throw;
			}
			oldtime = newtime;
		}
		unguard;
	}
	GUARD_CATCH
	{
		//?? should move softError and history logging to Core
		// log error
		if (FILE *f = fopen (CRASH_LOG, "a+"))
		{
			if (GErr.swError)
				fprintf (f, "----- " APPNAME " software exception -----\n%s\n\n", GErr.message);
			fprintf (f, "History: %s\n\n", GErr.history);
			fclose (f);
		}
		if (debugLogged)
			DebugPrintf ("***** CRASH *****\n%s\n*****************\n", GErr.history);

		GUARD_BEGIN {
			// shutdown all subsystems
			SV_Shutdown (va("Server fatal crashed: %s\n", GErr.message), false);	// message
			CL_Shutdown (true);
			Com_Shutdown ();
		} GUARD_CATCH {
			// nothing here ...
		}
		// display error
#ifndef DEDICATED_ONLY
		MessageBox (NULL, va("%s\n\nHistory: %s", GErr.message, GErr.history),
			APPNAME ": fatal error", MB_OK|MB_ICONSTOP/*|MB_TOPMOST*/|MB_SETFOREGROUND);
#else
		Sys_ConsoleOutput ("\n\n"S_RED"--------------------\n" APPNAME " fatal error\n");
		Sys_ConsoleOutput (va("%s\nHistory: %s\n", GErr.message, GErr.history));
#endif
	}
	// will get here only when fatal error happens
	Sys_Quit ();
	// and will neverget here ... (NORETURN Sys_Quit())
	return 0;
}
