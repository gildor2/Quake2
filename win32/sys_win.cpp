#include "winquake.h"
#include "Core.h"
#include "OutputDeviceMem.h"

#include <direct.h>

#include "../client/client.h"		//!! for editLine[] and CompleteCommand() only

bool		ActiveApp, MinimizedApp, FullscreenApp;

unsigned	sys_frame_time;			//?? used in cl_input.cpp only

//#define IS_CONSOLE_APP

#if !defined(IS_CONSOLE_APP) && defined(DEDICATED_ONLY)
#define IS_CONSOLE_APP
#endif


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

static HANDLE hConInput;

#define MAX_CMDLINE		256		//?? same as in client/keys.h

static char	console_text[MAX_CMDLINE];
static int	console_textlen = 0;
static bool console_drawInput = true;


class COutputDeviceWin32Console : public COutputDevice
{
protected:
	HANDLE	hOutput;
public:
	void Init ()
	{
#ifndef IS_CONSOLE_APP
		AllocConsole ();
#endif
		hConInput  = GetStdHandle (STD_INPUT_HANDLE);		//?? not OutputDevice-stuff
		hOutput = GetStdHandle (STD_OUTPUT_HANDLE);
		Register ();
	}
	// writting without processing
	inline void WriteChar (char c)
	{
		DWORD dummy;
		WriteFile (hOutput, &c, 1, &dummy, NULL);
	}
	inline void WriteStr (const char *str)
	{
		DWORD dummy;
		WriteFile (hOutput, str, strlen (str), &dummy, NULL);
	}
	// writting with processing
	void Write (const char *str)
	{
		EraseInput ();
		// draw message
		while (char c = *str++)
		{
			// parse color info
			if (c == COLOR_ESCAPE && str[0] >= '0' && str[0] <= '7')
			{
				static const byte colorTable[8] = {0, 4, 2, 6, 1, 5, 3, 7};
				SetConsoleTextAttribute (hOutput, colorTable[str[0] - '0']);
				str++;
				continue;
			}
			WriteChar (c & 0x7F);		// clear 7th bit
		}
		SetConsoleTextAttribute (hOutput, 7);
		// should draw input again
		console_drawInput = true;
	}
	void EraseInput ()
	{
		if (console_drawInput) return;	// already erased
		char text[MAX_CMDLINE+2];
		text[0] = '\r';
		memset (&text[1], ' ', console_textlen+1);
		text[console_textlen+2] = '\r';
		text[console_textlen+3] = 0;
		DWORD dummy;
		WriteFile (hOutput, text, console_textlen+3, &dummy, NULL);
	}
};

static COutputDeviceWin32Console Win32Log;


char *Sys_ConsoleInput ()
{
	guard(Sys_ConsoleInput);

	if (console_drawInput)
	{
		// display input line
		Win32Log.WriteChar (']');
		if (console_textlen)
		{
			console_text[console_textlen] = 0;
			Win32Log.WriteStr (console_text);
		}
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
					Win32Log.WriteStr ("\r\n");
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
						Win32Log.WriteStr ("\b \b");
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
							Win32Log.EraseInput ();
							console_drawInput = true;	// next time ...
						}
					}
					break;

				case '\x1B':	// ESC
					Win32Log.EraseInput ();
					console_textlen = 0;
					console_text[0] = 0;
					break;

				default:
					if (ch >= ' ')
					{
						if (console_textlen < sizeof(console_text)-2)
						{
							Win32Log.WriteChar (ch);
							console_text[console_textlen++] = ch;
							console_text[console_textlen] = 0;
						}
					}
//					else
//						appPrintf ("%2X\n",ch);
					break;
			}
		}
	}

	return NULL;

	unguard;
}


void Sys_ProcessMessages ()
{
#ifndef DEDICATED_ONLY
	guard(Sys_ProcessMessages);
	MSG		msg;
	while (PeekMessage (&msg, NULL, 0, 0, PM_REMOVE))
	{
		if (msg.message == WM_QUIT)
			GIsRequestingExit = true;

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
				appWPrintf ("Sys_GetGameAPI(%s): failed to load library\n", *DllName);
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

extern COutputDevice *debugLog;
HINSTANCE global_hInstance;


#ifndef IS_CONSOLE_APP
int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#else
int main (int argc, const char **argv) // force to link as console application
#endif
{
	TRY
	{
#ifndef IS_CONSOLE_APP
		global_hInstance = hInstance;
		const char *cmdline = lpCmdLine;
#else
		global_hInstance = GetModuleHandle (NULL);
		const char *cmdline = GetCommandLine ();
#endif

#ifdef CD_PATH
		if (const char *cddir = ScanForCD ())
			Cvar_Set ("cddir", cddir);
#endif

#ifndef IS_CONSOLE_APP
		// init-time output
		COutputDeviceMem *TempLog = new COutputDeviceMem (16384);
		TempLog->Register ();
#else
		Win32Log.Init ();
#endif

		appInit ();		//!!!!
		Com_Init (cmdline);

#ifndef IS_CONSOLE_APP
		COutputDeviceCon ConLog;
		if (DEDICATED)
			Win32Log.Init ();
		else
			ConLog.Init ();
		// flush init-time log
		TempLog->Unregister ();
		appPrintf (TempLog->GetText ());
		delete TempLog;
#endif

		double oldtime = appMillisecondsf ();

		/*--------- main window message loop ------------*/
		guard(MainLoop);
		while (!GIsRequestingExit)
		{
#ifndef DEDICATED_ONLY
			if (!ActiveApp || DEDICATED)
				Sleep (10);		//?? what about client and server in one place: will server became slower ?
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
			TRY {
				Com_Frame (timeDelta);
			} CATCH {
				if (!GErr.nonFatalError) throw;		// go to outer CATCH{}

				//!! check old behaviour on DropError(NULL)
				SV_Shutdown (va("Server crashed: %s\n", *GErr.Message));	// message
				Com_DPrintf ("History: %s\n", *GErr.History);				// history
				if (!DEDICATED) CL_Drop (true);
				GErr.Reset ();
			}
			oldtime = newtime;
		}
		unguard;
	} CATCH {
		GIsFatalError = true;
		if (debugLog)
			debugLog->Printf ("***** CRASH *****\n%s\n*****************\n", *GErr.History);
	}

	// shutdown all systems
	TRY {
		SV_Shutdown (GIsFatalError ? va("Server fatal crashed: %s\n", *GErr.Message) : "Server quit\n");
		CL_Shutdown ();
#ifndef IS_CONSOLE_APP
		if (DEDICATED) FreeConsole ();
#endif
	}
	CATCH {}	// nothing ...

	appExit ();
	return 0;
}
