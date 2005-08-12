#include "winquake.h"
#include "Core.h"

#include <direct.h>

#include "../client/client.h"		//!! for editLine[] and CompleteCommand() only

#define CRASH_LOG	"crash.log"		//!! required for history logging (should move to Core ??!)

int			starttime;
bool		ActiveApp, MinimizedApp, FullscreenApp;

unsigned	sys_frame_time;			//?? used in cl_input.cpp only


void Sys_Quit ()
{
#ifndef DEDICATED_ONLY
	if (DEDICATED) FreeConsole ();
#endif
	appExit ();
	exit (0);
}


static char *ScanForCD ()
{
#ifdef CD_PATH
	static TString<MAX_OSPATH> CdDir;
	static char drive[4] = "c:/";

	// no abort/retry/fail errors
	SetErrorMode (SEM_FAILCRITICALERRORS);

	// scan the drives
	for (drive[0] = 'c'; drive[0] <= 'z'; drive[0]++)
	{
		if (GetDriveType (drive) != DRIVE_CDROM) continue;
		if (FILE *f = fopen (va("%s"CD_CHECK, drive), "r"))
		{
			fclose (f);
			CdDir.filename (va("%s"CD_PATH, drive));
			return CdDir;
		}
	}
#endif
	return NULL;
}


/*-----------------------------------------------------------------------------
	Win32 console input/output
-----------------------------------------------------------------------------*/

static HANDLE hConInput, hConOutput;

#define MAX_CMDLINE		256		//?? same as in client/keys.h

static char	console_text[MAX_CMDLINE];
static int	console_textlen = 0;
static bool console_drawInput = true;


static void EraseConInput ()
{
	if (!console_drawInput)
	{
		char	text[MAX_CMDLINE+2];
		DWORD	dummy;

		text[0] = '\r';
		memset (&text[1], ' ', console_textlen+1);
		text[console_textlen+2] = '\r';
		text[console_textlen+3] = 0;
		WriteFile (hConOutput, text, console_textlen+3, &dummy, NULL);
	}
}


char *Sys_ConsoleInput ()
{
	guard(Sys_ConsoleInput);

	DWORD	dummy;

	if (console_drawInput)
	{
		// display input line
		WriteFile (hConOutput, "]", 1, &dummy, NULL);
		if (console_textlen)
			WriteFile (hConOutput, console_text, console_textlen, &dummy, NULL);
		console_drawInput = false;
	}

	while (true)
	{
		DWORD	numevents, numread;
		GetNumberOfConsoleInputEvents (hConInput, &numevents);
		if (numevents <= 0) break;

		INPUT_RECORD rec;
		ReadConsoleInput (hConInput, &rec, 1, &numread);
		if (rec.EventType == KEY_EVENT && rec.Event.KeyEvent.bKeyDown)
		{
			int ch = rec.Event.KeyEvent.uChar.AsciiChar;
			switch (ch)
			{
				case '\r':		// ENTER
					WriteFile (hConOutput, "\r\n", 2, &dummy, NULL);
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
						WriteFile (hConOutput, "\b \b", 3, &dummy, NULL);
					}
					break;

				case '\t':		// TAB
					{
						appSprintf (ARRAY_ARG(editLine), "]%s", console_text);
						CompleteCommand ();
						const char *s = editLine;
						if (s[0] == ']') s++;
						if (s[0] == '/') s++;
						int len = strlen (s);
						if (len > 0)
						{
							console_textlen = min(len, sizeof(console_text)-2);
							appStrncpyz (console_text, s, console_textlen+1);
							Sys_ConsoleOutput ("");		// update line
						}
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
							WriteFile(hConOutput, &ch, 1, &dummy, NULL);
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


static void InitWin32Console ()
{
#ifndef DEDICATED_ONLY
	AllocConsole ();
#endif
	hConInput  = GetStdHandle (STD_INPUT_HANDLE);
	hConOutput = GetStdHandle (STD_OUTPUT_HANDLE);
}


void Sys_ConsoleOutput (const char *string)
{
	if (!DEDICATED) return;

	EXEC_ONCE(InitWin32Console ();)

	EraseConInput ();

	// draw message
	while (char c = string[0])
	{
		// parse color info
		if (c == COLOR_ESCAPE && string[1] >= '0' && string[1] <= '7')
		{
			static const byte colorTable[8] = {0, 4, 2, 6, 1, 5, 3, 7};
			SetConsoleTextAttribute (hConOutput, colorTable[string[1] - '0']);
			string += 2;
			continue;
		}
		c &= 0x7F;		// clear 7th bit
		DWORD	dummy;
		WriteFile (hConOutput, &c, 1, &dummy, NULL);
		string++;
	}
	SetConsoleTextAttribute (hConOutput, 7);

	console_drawInput = true;
}


void Sys_ProcessMessages ()
{
#ifndef DEDICATED_ONLY
	guard(Sys_ProcessMessages);
	MSG		msg;
	while (PeekMessage (&msg, NULL, 0, 0, PM_REMOVE))
	{
		if (msg.message == WM_QUIT) Com_Quit ();

		// TranslateMessage() used for convert VK_XXX messages to WM_CHAR-like
		// We don't need this, but keep TranslateMessage() just in case of compatibility
		// with future Windows versions (at least in Win2K MS implemented "some feature",
		// which requires to call this function -- what feature - they don't tell)
		TranslateMessage (&msg);

		guard(DispatchMessage);
		DispatchMessage (&msg);
		unguardf(("msg=%X", msg.message));
	}

	// grab frame time
	sys_frame_time = appMilliseconds ();	// FIXME: should this be at start?
	unguard;
#endif
}



/*-----------------------------------------------------------------------------
	Game DLL
-----------------------------------------------------------------------------*/

static HINSTANCE game_library;


void Sys_UnloadGame ()
{
	guard(Sys_UnloadGame);
	if (!FreeLibrary (game_library))
		Com_FatalError ("Cannot unload game library");
	Com_DPrintf ("FreeLibrary(game.dll)\n");
	game_library = NULL;
	unguard;
}


void *Sys_GetGameAPI (void *parms)
{
	guard(Sys_GetGameAPI);

	if (game_library)
		Com_FatalError ("Sys_GetGameAPI() without Sys_UnloadGame()");

#if defined(_M_IX86)
#	define GAME_DLL "gamex86.dll"
#elif defined(_M_ALPHA)
#	define GAME_DLL "gameaxp.dll"
#else
#	error "Don't know, how game dll named"
#endif

	// run through the search paths
	const char *path = NULL;
	while (path = FS_NextPath (path))
	{
		TString<MAX_OSPATH> DllName;
		DllName.sprintf ("%s/" GAME_DLL, path);
		if (FILE *f = fopen (DllName, "rb"))	// check file presence
		{
			fclose (f);
			if (game_library = LoadLibrary (DllName))
			{
				Com_DPrintf ("LoadLibrary (%s)\n", *DllName);
				break;
			}
			else
				Com_WPrintf ("Sys_GetGameAPI(%s): failed to load library\n", *DllName);
		}
		else
			Com_DPrintf ("Sys_GetGameAPI(%s): file not found\n", *DllName);
	}
	if (!game_library)
		return NULL;		// couldn't find one anywhere

	typedef void * (* pGetGameApi_t)(void *);
	pGetGameApi_t pGetGameAPI;
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


/*-----------------------------------------------------------------------------
	Program startup, main loop and exception handling
-----------------------------------------------------------------------------*/

extern bool debugLogged;
HINSTANCE global_hInstance;


#ifndef DEDICATED_ONLY
int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#else
int main (int argc, const char **argv) // force to link as console application
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
		if (char *cddir = ScanForCD ())
		{
			// add to the start of cmdline, so, if already specified - will not override option
			appSprintf (ARRAY_ARG(cmdline2), "-cddir=\"%s\" %s", cddir, cmdline);
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
			Sys_ProcessMessages ();
#else
			Sleep (10);
#endif

			// do not allow Com_Frame(0)
			double timeDelta, newtime;
			while (true)
			{
				newtime = appMillisecondsf ();	//?? can use appCycles() for measuring time delta, but (currently) this is Pentium-only
				timeDelta = newtime - oldtime;
				if (timeDelta < 0)
				{
					// may be, resumed from sleep mode - fix delta
					//?? may be, implement fix in appMilliseconds() (for global effect) ?
					Com_DPrintf ("fixing timeDelta < 0\n");
					timeDelta = 100;
					break;
				}
				if (timeDelta > 1.0f) break;	//?? client (or server?) time bugs with ">0" condition & cl_maxfps < realFPS -- net/prediction errors
				Sleep (0);
			}
			GUARD_BEGIN {
				Com_Frame (timeDelta);
			} GUARD_CATCH {
				if (GErr.nonFatalError)
				{
					//!! TEST: dedicated: error -drop, error -gpf, error "msg"
					//!! check old behaviour on DropError(NULL)
					SV_Shutdown (va("Server crashed: %s\n", *GErr.Message), false);	// message
					Com_DPrintf ("History: %s\n", *GErr.History);					// history
					if (!DEDICATED) CL_Drop (true);
					GErr.Reset ();
				}
				else
					throw;		// go to outer GUARD_CATCH{}
			}
			oldtime = newtime;
		}
		unguard;
	} GUARD_CATCH {
		//?? should move softError and history logging to Core
		// log error
		if (FILE *f = fopen (CRASH_LOG, "a+"))
		{
			if (GErr.swError)
				fprintf (f, "----- " APPNAME " software exception -----\n%s\n\n", *GErr.Message);
			fprintf (f, "History: %s\n\n", *GErr.History);
			fclose (f);
		}
		if (debugLogged)
			DebugPrintf ("***** CRASH *****\n%s\n*****************\n", *GErr.History);

		GUARD_BEGIN {
			// shutdown all subsystems
			SV_Shutdown (va("Server fatal crashed: %s\n", *GErr.Message), false);	// message
			CL_Shutdown (true);
			Com_Shutdown ();
		} GUARD_CATCH {
			// nothing here ...
		}
		// display error
#ifndef DEDICATED_ONLY
		MessageBox (NULL, va("%s\n\nHistory: %s", *GErr.Message, *GErr.History),
			APPNAME ": fatal error", MB_OK|MB_ICONSTOP/*|MB_TOPMOST*/|MB_SETFOREGROUND);
#else
		Sys_ConsoleOutput ("\n\n"S_RED"--------------------\n" APPNAME " fatal error\n");
		Sys_ConsoleOutput (va("%s\nHistory: %s\n", *GErr.Message, *GErr.History));
#endif
	}
	// will get here only when fatal error happens
	Sys_Quit ();
	// and will never get here ... (NORETURN Sys_Quit())
	return 0;
}
